/*
**	Copyright (c) 1983, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<cv.h>		 
# include	<lo.h>
# include	<nm.h>
# include	<me.h>
# include	<si.h>
# include	<st.h>
# include	<ex.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ooclass.h>
# include	<oocat.h>
# include	<cu.h>
# include	<ui.h>
# include	<ug.h>
# include	"erie.h"
# include	"ieimpexp.h"

/*
** Name:	ieimport.c - "guts of iiimport" routine
**
** Defines:
**		IIIEiii_IIImport()	the "guts"
**		IIIEieh_IEHandler()	static interrupt handler
**		IIIEap_AbortProtect()	transaction proc for IIIEieh_IEHandler
**		IIIEbe_Bad_Exit()	common error cleanup, exit routine
**
** History:
**	20-jul-1993 (daver)
**		First Written
**	13-sep-1993 (daver)
**		Cleaned up error handling, added IIIEbe_Bad_Exit(),
**		messages to erie.msg, etc.
**	26-jan-1994 (daver)
**		Store object_id of records, use it when "finding" record
**		member objects. Fixes bug 58570.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

FUNC_EXTERN     STATUS  IIUICheckDictVer();

i4		(*IIseterr())();

/*
** the following statics were stolen from cu_rdobj(); its how Joe
** originally did copyapp, and we all liked Joe, so I used his technique too.
*/
static STATUS	handstatus	= OK;
static FILE	*handfp		= NULL;

static i4	IIIEap_AbortProtect();


/*{
** Name:	IIIEiii_IIImport - read in a file containing FE objects
**
** Description:
**	Reads an intermediate file containing definitions for one or more FE
**	objects and writes them to the open database.  
**
**
** Input:
**	OOID	appid		object id of the application specified 
**					on the command line
**
**	char	*appowner	owner of application specified as above
**
**	i4	allflags	contains settings for copysrc and check flags
**
**	char	*oldsrcdir	ptr to buffer, for src code dir found in file
**
**	IEPARAMS **srclist	pointer to parameter list. 
**
** Output:
**	IEPARAMS **srclist	link list of objects which have source files 
**				to be copied (i.e., User Frames, customized
**				Vision frames, 4GL/3GL procedures).
**
** Returns:
**	OK		completed successfully.
**	CUQUIT		tells parent to issue that "Processing Stops" message
**      CHECKFAIL       -check was specified, and we at least one object
**                      found in intermediate file did NOT exist in the
**                      target application
**      CHECKOK         -check was specified, and everything existed in
**                      the target application.
**
** Side Effects:
**	This routine performs the abort transaction if an error is detected
**	during a dbms read/write; by returning CUQUIT, the parent exits and
**	prints that ever-popular "Processing Stops. No changes will be made."
**	message.
**
** History:
**	14-jul-1993 (daver)
**		First written. Some of this routine was taken from cu_rdobj(); 
**		but the copyutil library is so hosed up it is easier to 
**		re-write for this specific task than to use the "cute", 
**		twisted logic that exists.
**	13-sep-1993 (daver)
**		Documented, msgs extracted to erie.msg, cleaned up.
**	04-jan-1994 (daver)
**		Build a srclist iff we're copying source or we're
**		writing a listing file. Return different values
**		(CHECKOK, CHECKFAIL) if the -check flag is specified.
**		Fixes bug 57935.
**	26-jan-1994 (daver)
**		Store object_id of records, use it when "finding" record
**		member objects. Fixes bug 58570.
*/
STATUS
IIIEiii_IIImport(appid, appowner, filename, allflags, oldsrcdir, srclist)
OOID		appid;
char		*appowner;
char		*filename;
i4		allflags;
char		*oldsrcdir;
IEPARAMS	**srclist;
{
	STATUS		IIIEieh_IEHandler();	/* ex. handler from copyutil */
	EX_CONTEXT	context;
	i4		(*savefunc)();

	LOCATION	loc_forms;	/* file handling shme from copyutil */
	FILE		*fp;
	STATUS		filestatus;	/* tells us if we're at EOF or not */

	char		inbuf[IE_LINEMAX];	/* character buf; its HUGE! */

	STATUS		rval;		/* ubiquitous return value */

	bool		novision = FALSE;
	bool		check_ok = TRUE;	/* check flag return status */

	i4		majv;	/* for checking file version */
	i4		minv;

	bool		just_checking;
	i4		record_id = 0;
	bool		copy_source;
	bool		write_listfile;

	IEPARAMS	*new = NULL;	/* for copysrc processing */
	IEPARAMS	*head = NULL; 

	/* 
	** extract bitmasks from flags; just makes the code a bit 
	** more readable for anyone else who has to deal with this...
	*/
	just_checking = (allflags & CHECKFLAG);
	copy_source = (allflags & COPYSRCFLAG);
	write_listfile = (allflags & LISTFILEFLAG);


	/* initialize OO, I presume ... NEED TO verify if is this needed*/
	IIOOinit((OO_OBJECT **)NULL);


	/* Open intermediate file */
	if ( LOfroms(PATH & FILENAME, filename, &loc_forms) != OK  ||
			SIopen(&loc_forms, ERx("r"), &fp) != OK )
	{
		IIUGerr(E_IE0020_Cannot_Read_File, UG_ERR_ERROR, 1, 
							(PTR) filename);
		return CUQUIT;
	}


	/* get first record */
	if (SIgetrec(inbuf, IE_LINEMAX, fp) == ENDFILE)
	{
		IIUGerr(E_IE0021_Bad_Format, UG_ERR_ERROR, 1, (PTR)filename);
		return IIIEbe_Bad_Exit(fp);
	}

	/* 
	** I'm not sure why we initialize our exception handler after we
	** open up the file, but copyapp did it this way, so I am too.
	*/
	handstatus = CUQUIT;
	handfp = fp;
	if (EXdeclare(IIIEieh_IEHandler, &context) != OK)
	{
		EXdelete();

		/*
		** If the transaction was aborted by the dbms, then
		** the abort statement will get an error.
		** IIIEap_AbortProtect ignores any error so it isn't printed.
		*/
		savefunc = IIseterr(IIIEap_AbortProtect);

		/* abort transaction */
		IIUIabortXaction();

		_VOID_ IIseterr(savefunc);
		iiuicw0_CatWriteOff();	/* In case it is turned on. */
		return handstatus;
	}


	/*
	** Check if file is appropriate version. CopyApp had a check for
	** pre-6.0 files, but that's impossible with IIIMPORT. 
	*/
	rval = iicuChkHdr("IIEXPORT", inbuf, &majv, &minv);
	switch(rval)
	{
	  case CU_CMPAT:
		/* compatible version */
		rval = OK;
		break;
	  case CU_UPWD:
		/* intermediate file version is newer than copyapp version */
		IIUGerr(E_IE0022_Not_Compatible, UG_ERR_ERROR, 1,
			(PTR)filename);

		return IIIEbe_Bad_Exit(fp);
	  default:
	  	/*
		** and case CU_FAIL:
		** (( intermediate file has a bad first line ))
		*/
		IIUGerr(E_IE0021_Bad_Format, UG_ERR_ERROR, 1, (PTR)filename);

		return IIIEbe_Bad_Exit(fp);
	}

	/* scan away first token */
	cu_gettok(inbuf);

	/*
	** I'll assume we still need to do this, so it remains...
	**
	** Check whether the destination database's dictionary will
	** support Vision - this is used in 2 places inside the for loop below.
	*/
	if (IIUICheckDictVer(ERx("VISION"), FE_VISIONVER) != OK)
	{
		novision = TRUE;
	}

	/* Turn on catalog access; we'll need it in the upcoming loop */
	iiuicw1_CatWriteOn();           


	/* 
	** WE'VE JUST READ THE FIRST LINE. ITS THE HEADER.
	** The next line will contain application information, including the
	** source code directory of where the application came from. This
	** will come in handy if we need to copy the source around.
	*/

	if (IEDebug)
		SIprintf("\nOPENED the FILE, read the header ...\n\n");
	
	/*
	** Read the first record. This will be the application record.
	** This main loop processes a single object record at a time; its
	** subroutines process the entire object (and all its little
	** subcomponents and friends). When each of these routines (in the
	** case statement, below) returns, we expect that the file pointer
	** "fp" and the current line of the file "inbuf" will be set to the
	** next object to be processed (or EOF).
	*/
	filestatus = SIgetrec(inbuf, IE_LINEMAX, fp);


	/* ENDFILE will drop us out of the loop */
	while (filestatus == OK)
	{
		i4		rectype;	
		IEOBJREC	objectrec;

		/* read until end of file reached */

		rectype = cu_rectype(inbuf);

		/* we expect a new object. file was probably edited. exit */
		if (rectype < OC_OBJECT)
		{
			IIUGerr(E_IE0023_Out_Of_Order, UG_ERR_ERROR, 0);
			filestatus = CUQUIT;
			break;
		}


		/* 
		** If vision not supported, we want to copy Vision-specific 
		** menu, browse, append and update frames in as ABF user frames.
		*/
		if (novision)
		{
			if (rectype == OC_MUFRAME || rectype == OC_APPFRAME ||
			    rectype == OC_UPDFRAME || rectype == OC_BRWFRAME)
			{
				rectype = OC_OSLFRAME;
			}
		}

		/*
		** I've special cased OC_APPL because of the different
		** requirements iiimport has on it. IF we were to re-implement
		** copyapp with this, then we might expect that an application 
		** object might be written/updated here. With iiimport 
		** however, the application must already exist -- or we never
		** get here. In fact, the only reason the application object
		** is written to an iiexport file is that we might need to
		** know the source code directory if the user wants to copy
		** some files over. If its an OC_APPL, get source code
		** directory out of the detail record (if we care), call
		** IIIEsor_SkipObjectRec to get the file pointer set to the
		** next object, and that's about it. We already know that
		** the target application exists.
		*/
		if (rectype == OC_APPL)
		{
			if (copy_source)
				filestatus = IIIEpar_ProcessAppRecord(fp, 
							inbuf, oldsrcdir);
			else
				filestatus = IIIEsor_SkipObjectRec(fp, inbuf); 

			if (IEDebug)
				SIprintf("THE SOURCE DIR IS: %s\n", oldsrcdir);

			continue;
		}

		/*
		** fill in the objectrec structure with the file info in
		** inbuf. this has to be done a couple of places, so we
		** call a routine to do it. this also assigns rectype to
		** objectrec.class (rectype is still on the current line
		** in the file, in inbuf).
		*/
		IIIElos_LoadObjrecStruct(&objectrec, inbuf);

		/* We know the class, its in rectype. the name is in inbuf
		** somewhere. Lets do this:
		** Call routine to see if it exists.
		** If we're just checking:
		**		and it exists,
		**			everything is fine, say so.
		**		and it DOES NOT exist:
		**			keep processing, Say so, and
		**			eventually exit (IECHECK)
		*/

		/*
		** this will also get us a new id if one does not exist.
		** 4th param (record_id) only used if the object in
		** question is a record member (OC_RECMEM).
		*/
		if (IIIEfoi_FindObjectId(&objectrec, just_checking,
						appid, record_id) != OK)
		{
			/* error message printed inside IIIEfoi_FindObjectId */
			return IIIEbe_Bad_Exit(fp);
		}

		/*
		** if the object is a record type, then its record members
		** (which follow the record in the file) will need to know 
		** the object_id of this "parent" record. fixes bug 58570.
		*/
		if (rectype == OC_RECORD)
		{
			record_id = objectrec.id;
		}

		/*
		** Here's where we print our messages.
		**
		** If we're "just checking", we don't get any further than the
		** end of this if-statement; we continue the loop if we have 
		** more to go, and we break out if we're at EOF
		**
		** If we're really importing, mention what we're importing, 
		** what it is, and if we're updating or inserting 
		**
		** Both cases, however, ignore the FORMREF, which is
		** effectively nothing more than a marker that says that
		** the form object is going to be processes later. If
		** the object is an OC_AFORMREF, don't say anything.
		*/
		if (just_checking)
		{
			if (objectrec.class != OC_AFORMREF)
			{
				if (objectrec.update)
				{
					IIUGmsg(ERget(S_IE0024_Check_Found),
					    FALSE, 2, objectrec.name,
					    IICUrtoRectypeToObjname(rectype));
				}
				else
				{
					IIUGmsg(ERget(S_IE0025_No_Check_Found),
					    FALSE, 2, objectrec.name,
					    IICUrtoRectypeToObjname(rectype));

					check_ok = FALSE;
				}
			}

			/* either way, go on to the next one... */
			filestatus = IIIEsor_SkipObjectRec(fp, inbuf); 

			/* unless this *was* the last one! */
			if (filestatus == ENDFILE)
				break;		
			else
				continue;
		}
		else
		{
			if (objectrec.class != OC_AFORMREF)
			{
				if (objectrec.update)
					IIUGmsg(ERget(S_IE0000_Updating_Obj), 
					    FALSE, 2, 
					    IICUrtoRectypeToObjname(rectype),
					    objectrec.name);
				else
					IIUGmsg(ERget(S_IE0001_Creating_Obj), 
					    FALSE, 2, 
					    IICUrtoRectypeToObjname(rectype),
					    objectrec.name);
			}
			if ( (copy_source) || (write_listfile) )
			{
				i4		c;

				if (IEDebug)
					SIprintf("ooh, building da srclist\n");
				/*
				** Okay. If the object is a procedure or a
				** frame, lets add it to the list. We need to
				** allocate memory, so this is yet another
				** possible place for conservation of speed (by
				** allocating one big chunk at the beginning of
				** the program). So put 'em in backwards:
				**
				** IF procedure OR frame
				**	new = MEreqmem()
				**	new.data = data shme
				**	new.next = head
				**	head = new
				**
				** procedures fall into classes between
				** 2020 (OC_APPLPROC) and 2100 (OC_AMAXPROC);
				** frames fall between
				** 2200 (OC_APPLFRAME) and 2300 (OC_AMAXFRAME)
				*/

				c = objectrec.class; /* for readability */

				if ( ((c >= OC_APPLPROC)&&(c < OC_AMAXPROC))
				     ||
				     ((c >= OC_APPLFRAME)&&(c < OC_AMAXFRAME))
				   )
				{
					new = (IEPARAMS *)MEreqmem((u_i4)0,
					  (u_i4)sizeof(IEPARAMS), TRUE, &rval);

					if (rval != OK)
					{
						IIUGerr(
						    E_IE001A_Memory_Alloc_Err,
						    UG_ERR_ERROR, 1, 
						    (PTR)ERx("IIIMPORT") );

						return IIIEbe_Bad_Exit(fp);
					}

					STcopy(objectrec.name, new->name);
					new->id = objectrec.id;
					new->next = head;
					head = new;
				}
			} /* if copy_source or write_listfile */
		} /* else (not) just_checking */

		/*
		** keep processing ... call the routines to do the
		** real work. Write the object at hand out to the 
		** ii_objects catalog. The information we need is in 
		** the objectrec structure. Then write the detail
		** line for it. Then process the components.
		** 
		** if performance/locking is really an issue, what we could
		** do is build a linked list of a component structure, and 
		** read the rest of the object into the elements of this 
		** structure; multiple components, multiple buckets. if 
		** we *Really* wanted to, we could probably create a 
		** representation of the entire file internally. Realisticly,
		** we probably would never need to store more than a single
		** object in memory, and then write the whole thing out to 
		** the database.
		** 
		** for now, lets just write as we go, shall we? it means 
		** doing file i/o while we hold locks out on the database,
		** but so be it. we know what to do if we get those 
		** "iiimport takes forever and locks up my database"
		** calls to tech support
		*/
		if (IEDebug)
			SIprintf("TOKEN: %d;%s;%s\n", rectype,
			IICUrtnRectypeToName(rectype),
			IICUrtoRectypeToObjname(rectype));

		/*
		** write the objectrec struct to ii_objects. No file i/o here;
		** fp, inbuf still pointing to line with object on it
		*/
		filestatus = IIIEwio_WriteIIObjects(&objectrec, appid,
								appowner);
		if (filestatus != OK)
			break;

		/*
		** now write the detail line. Gotta do a one-linefile i/o 
		** to get it!  The file i/o is in the DoObjectDetail
		** routine itself, which then calls the appropriate
		** catalog-writing routine based on the class of the
		** objectrec we pass in.
		**
		** fp, inbuf now pointing to detail line
		*/
		filestatus = IIIEdod_DoObjectDetail(&objectrec, appid, 
							appowner, inbuf, fp);
		if (filestatus != OK)
			break;

		/*
		** special case those classes have read the 'next'
		** object record into inbuf. in this case, continue the
		** loop at the top, processing this new object record
		** this is true for joindef's (who have no components),
		** and records/recmems, whose model is all screwed up
		** anyway (records don't have components, they have
		** other objects -- recmems -- which have to be processed
		** as part of the record detail. in fact, OC_RECMEM
		** should never occur at this level, since it will get
		** processed in the DoObjectDetail routine, above.)
		*/
		if ( (objectrec.class == OC_JOINDEF) ||
		     (objectrec.class == OC_RECORD)  ||
		     (objectrec.class == OC_RECMEM) )
			continue;
		/*
		** now get all the components. lots of i/o. big fun. 
		** fp, inbuf point to the line AFTER the last component;
		** that is, the NEXT object to be processed, or EOF.
		*/

		filestatus = IIIEdcr_DoComponentRecs(&objectrec, appid,
							appowner, inbuf, fp);

		/* filestatus is checked at the top of the loop */

	} /* end while filestatus == OK (i.e., not ENDFILE) */

	if (filestatus != ENDFILE)
	{
		/* something went wrong -- close file, abort, & exit */
		return IIIEbe_Bad_Exit(fp);
	}
	*srclist = head;

	/* close intermediate file */
	SIclose(fp);

	iiuicw0_CatWriteOff();	/* In case it is turned on. */
	EXdelete();				 

        if (just_checking)
        {
                if (check_ok)
                        return (CHECKOK);
                else
                        return (CHECKFAIL);
        }

	return (OK);
}

/*{
** Name:	IIIEieh_IEHandler	- Handler for object utility
**
** Description:
**	The handler for the object utility.
**	The copy utility only issues a EXFEBUG of 1 argument.
**	The argument values used are:
**		CUQUIT
**
**	If the handler recognizes the argument it does a DECLARE
**	which allows curdobj to return.  If it doesn't recognize
**	if it RESIGNALs.
**
** Inputs:
**	exarg		The exception arguments.
**
** Side Effects:
**	Closes the file used by iiimport.
**
** History:
**	14-jul-1993 (daver)
**	Stolen from Joe's cuhand in utils!copyutil!curdobj.qsc
**	(so I can't really say "First Written").
*/
STATUS
IIIEieh_IEHandler(exarg)
EX_ARGS	*exarg;
{
    if (exarg->exarg_num == EX_UNWIND)
	SIclose(handfp);
    if (exarg->exarg_num == EXFEBUG && exarg->exarg_count == 1)
    {
	handstatus = (STATUS) exarg->exarg_array[0];
	if (handstatus == CUQUIT)
	{
	    SIclose(handfp);
	    return EXDECLARE;
	}
    }
    return EXRESIGNAL;
}



/*{
** Name:	IIIEap_AbortProtect	- Abort routine for handler
**
** Description:
**	Part of the handler for the copy utility.
**	I assume this is set up so if you issue a couple of abort's in a
**	row, the second and subsequant abort's won't print an error about not
**	being in a transaction. Joe did it this way, and I copied it.
**
** Inputs:
**	errnum, ignored.
**
** History:
**	14-jul-1993 (daver)
**		Stolen from Joe's cuhand in utils!copyutil!curdobj.qsc
**		(so I can't really say "First Written").
**	13-sep-1993 (daver)
**		changed to IIIEap_AbortProtect() and documented
*/
static i4
IIIEap_AbortProtect(errnum)
i4	errnum;
{
    return 0;
}



/*{
** Name:	IIIEbe_Bad_Exit		- Common "bad" error routine
**
** Description:
**	Called from a few places, this routine issues an abort
**	transaction, closes the intermediate file, turns off catalog writing,
**	and returns CUQUIT, which tells the ieimain to print a
**	"Processing Stops" message.
**
** Inputs:
**	FILE		*fp		intermediate file
**
** Output:
**	None
**
** Returns:
**	CUQUIT. Always.
**
** Side Effects:
**	Closes the file used by iiimport.
**
** History:
**	13-sep-1993 (daver)
**		First Written/Extracted/Documented.
**
*/
STATUS
IIIEbe_Bad_Exit(filep)
FILE	*filep;
{

	IIUIabortXaction();	/* abort the transaction */

	SIclose(filep);		 /* close intermediate file */

	iiuicw0_CatWriteOff();	/* In case it is turned on. */

	EXdelete();				 

	return CUQUIT;
}
