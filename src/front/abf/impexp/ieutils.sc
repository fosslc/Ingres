/*
**	Copyright (c) 1993, 2004 Ingres Corporation	
*/

# include	<compat.h>
# include	<si.h>
# include	<me.h>
# include	<st.h>
# include       <gl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <ooclass.h>
# include       <oocat.h>
# include       <cu.h>
# include	<lo.h>
# include	<ug.h>
# include	"erie.h"
# include	"ieimpexp.h"
# include   <uigdata.h>

/**
** Name:        ieutils.sc -    Utility routines for iiexport/iiimport
**
** Description:
**     This file defines:
**
**	IIIEepv_ExtractParamValue()	extracts params
**	IIIEesp_ExtractStringParam()	extract a single string param
**	IIIEeum_ExportUsageMsg()	prints usage message for iiexport
**	IIIEium_ImportUsageMsg()	prints usage message for iiimport
**	IIIEsor_SkipObjectRec()		sets file pointing at the next object
**	IIIEfoi_FindObjectId()		is object is in this application?
**	IIIElos_LoadObjrecStruct()	populates structure from line of file
**	IIIEpar_ProcessAppRecord()	"skip" main application record
**	IIIEfco_FoundCursorObject()	find cursor object in IEPARAM list
**	IIIEfoe_FileOpenError()		issue errors related to file shme
**
** History:
**      30-jun-1993 (daver)
**		First Written.
**	26-Aug-1993 (fredv)
**		Included <me.h>.
**	25-aug-1993 (daver)
**		Added some nifty ER-style messages
**	02-sep-1993 (daver)
**		Added IIIEfoe_FileOpenError() from iecpysrc.sc
**	05-oct-1993 (daver)
**		Catch the user specifying a flag w/o a correponding value.
**	23-nov-1993 (daver)
**		Check EOS of proper variable in IIIEepv_ExtractParamValue. 
**		Fixes bug 57244. 
**	22-dec-1993 (daver)
**		Fix bug 57928 in IIIEesp_ExtractStringParam().
**		Special case "user", since the DBMS wants "-u" and the LRC 
**		wanted "-user". Add the '-' and the 'u' before the username.
**	21-jan-1994 (daver)
**		Turn cat writes back on after calling IIOOnewId().
**		Affects IIIEfoi_FindObjectId(). Fixes bug 58569.
**	24-jan-1994 (daver)
**		Call LOtos w/char ** rather than buffer. Fixes bug 58567.
**	22-feb-1994 (daver)
**		Enforce frame/procedure name space in IIIEfoi_FindObjectId().
**		Fixes bug 59886.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


EXEC SQL INCLUDE SQLCA;
EXEC SQL WHENEVER SQLERROR CALL SQLPRINT;


/*
** Name: IIIEepv_ExtractParamValue	-- extract a parameter's value
**
** Description:
**	IIIEepv_ExtractParamValue takes an entire parameter in the form of
**
**		-frame=name,name,name,...
**
**	and breaks up all characters after the '=' into comma seperated
**	words, creates a bucket for storege, and places them in the right 
**	array slot in the paramlist. 
**
**	If no '=' is found, we issue a usage message and exit.
**
** Input:
**	char            *flagname	name of flag: frame, procedure, etc
**	char            *value		entire string from argv: -frame=f1
**	IEPARAMS        **buffer	2D array of parameter values
**
** Output:
**	IEPARAMS	**buffer	creates a bucket and places it
**					in the correct spot. Each object type 
**					(frame, proc, record, glob, const)
**					has its own slot in the array.
** History:
**	08-sep-1993 (daver)
**		Documented, cleaned, messages to erie.msg.
**	05-oct-1993 (daver)
**		Catch the user specifying a flag w/o a correponding value.
**	23-nov-1993 (daver)
**		Check EOS of "start" rather than "buffer". Fixes bug 57244. 
*/
VOID   
IIIEepv_ExtractParamValue(flagname, value, buffer)
char		*flagname;
char		*value;
IEPARAMS	**buffer;
{
	char		*start = value;	
	char		*comma;
	char		noise[2];
	i4		len;
	IEPARAMS	*head, *newone;

	/*
	** first, make sure its a full on valid flag, rather than just
	** the first character!
	*/

	/* bump start to move past the '-' */
	start++;
	len = STlength(flagname);

	/*
	** we know the flag's name is in flagname. Use STbcompare to 
	** look at the just the actual flagname, as entered on the 
	** command name, and compare it against what the real name is
	** called. continue if we get a match, otherwise user might've
	** type in flame instead of frame, and we don't want to allow
	** that, do we?
	*/
	if (STbcompare(flagname, len, start, len, TRUE) != 0 )
	{
		IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR, 
					2, start, ERx("IIEXPORT"));
		IIIEeum_ExportUsageMsg();
	}

	/* find the = via STindex; it returns a NULL if not found */
	STcopy("=", noise);
	start = STindex(value, noise, 0);
	if (start == NULL)
	{
		IIUGerr(E_IE0011_No_equals_used, UG_ERR_ERROR, 1, flagname);
		IIIEeum_ExportUsageMsg();
	}

	/* bump it, we don't want the = */
	start++;

	/*
	** change "buffer" to "start". fixes bug 57244. (daver)
	**
	** start is EOS if 'frame=' (w/o a value) was specified (or user 
	** just "spaced" and put a space after the flagname. catch it here.
	*/
	if (*start == EOS)
	{
		IIUGerr(E_IE003E_No_param_value, UG_ERR_ERROR, 1, flagname);
		IIIEeum_ExportUsageMsg();
	}

	/* see if there's more than one; if so, null out the comma */
	STcopy(",", noise);

	/* initialize ptr; h = null, loop {n = new(), n.next = h, h = n} */
	head = NULL;

	do
	{
		STATUS	rval;
		/* figure out where the end is, EOS it out... */
		comma = STindex(start, noise, 0);

		if (comma != NULL)
		{
			*comma = EOS;
			comma++;	/* bump past the "," */
		}
		/* even if it *is* null, we add the last one to the list */
		newone = (IEPARAMS *) MEreqmem((u_i4)0, 
	    			(u_i4) sizeof(IEPARAMS), TRUE, &rval);
	
		/* lots pass NULL for status and ignore */
		if (rval != OK)
		{
			IIUGerr(E_IE001A_Memory_Alloc_Err, UG_ERR_FATAL, 1,
							(PTR)ERx("IIEXPORT"));
		}

		STcopy(start, newone->name );
		newone->id = -1;		/* undefined yet */
		newone->next = head;
		head = newone;
		
		start = comma;

	} while (comma != NULL); 

	*buffer = head;
}


/*
** Name: IIIEesp_ExtractStringParam	-- extract a single string param
**
** Description:
**	IIIEesp_ExtractStringParam takes a parameter in the form of
**
**		-user=username
**
**	and places all characters after the '=' into the appropriate buffer.
**	If no '=' is found, we issue a usage message and exit.
**
** Input:
**	char            *flagname	name of flag: frame, procedure, etc
**	char            *value		entire string from argv: -frame=f1
**	char		*buffer		char buffer to place param
**	bool		iiexport	TRUE if iiexport; FALSE if iiimport
**
** Output:
**	char		*buffer		places single string param here.
**
** History:
**	08-sep-1993 (daver)
**		First Documented/messaged/cleaned.
**	05-oct-1993 (daver)
**		Catch the user specifying a flag w/o a correponding value.
**	22-dec-1993 (daver)
**		Special case "user", since the DBMS wants "-u" and the LRC 
**		wanted "-user". Add the '-' and the 'u' before the username.
**		Fixes bug 57928.
*/
VOID   
IIIEesp_ExtractStringParam(flagname, value, buffer, iiexport)
char	*flagname;
char	*value;
char	*buffer;
bool	iiexport;
{
	char	*start = value;	
	char	equals[2];
	i4	len;

	/*
	** first, make sure its a full on valid flag, rather than just
	** the first character!
	*/

	/* bump start to move past the '-' */
	start++;
	len = STlength(flagname);

	/*
	** we know the flag's name is in flagname. Use STbcompare to 
	** look at the just the actual flagname, as entered on the 
	** command name, and compare it against what the real name is
	** called. continue if we get a match, otherwise user might've
	** type in flame instead of frame, and we don't want to allow
	** that, do we?
	*/
	if (STbcompare(flagname, len, start, len, TRUE) != 0 )
	{
		if (iiexport)
		{
			IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR, 
					2, start, ERx("IIEXPORT"));
			IIIEeum_ExportUsageMsg();
		}
		else
		{
			IIUGerr(E_IE0010_Illegal_flag, UG_ERR_ERROR, 
					2, start, ERx("IIIMPORT"));
			IIIEium_ImportUsageMsg();
		}

	}

	STcopy("=", equals);
	start = STindex(value, equals, 0);
	if (start == NULL)
	{
		IIUGerr(E_IE0011_No_equals_used, UG_ERR_ERROR, 1, flagname);
		if (iiexport)
			IIIEeum_ExportUsageMsg();
		else
			IIIEium_ImportUsageMsg();
	}

	/* put the -u before the actual user name, bump the buffer pointer */
	if (STcompare(flagname, ERx("user")) == 0)
	{
		STcopy("-u", buffer);
		buffer = buffer + 2;
	}
		
	/* bump it, we don't want the = */
	start++;
	STcopy(start, buffer);
	/*
	** buffer is EOS if someone specified 'intfile=', or just had a
	** space after the flagname. catch it, so we don't let LOfroms AV
	*/
	if (*buffer == EOS)
	{
		IIUGerr(E_IE003E_No_param_value, UG_ERR_ERROR, 1, flagname);
		if (iiexport)
			IIIEeum_ExportUsageMsg();
		else
			IIIEium_ImportUsageMsg();
	}
}


/*
** Name: IIIEeum_ExportUsageMsg	-- issue usage for IIEXPORT
**
** Description:
**	Prints usage message for IIEXPORT.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Side Effects:
**	Prints a message, and exits.
**
** History:
**	08-sep-1993 (daver)
**		First Documented/messaged/cleaned.
*/
VOID
IIIEeum_ExportUsageMsg()
{
	IIUGmsg(ERget(S_IE001B_Correct_Syntax_Is), FALSE, 0);

	IIUGmsg(ERget(S_IE001C_IIExport_Usage), FALSE, 0);

	IIUGmsg(ERget(S_IE003C_IIExport_Usage2), FALSE, 0);

        IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
}

/*
** Name: IIIEium_ImportUsageMsg	-- issue usage for IIIMPORT
**
** Description:
**	Prints usage message for IIIMPORT.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Side Effects:
**	Prints a message, and exits.
**
** History:
**	08-sep-1993 (daver)
**		First Documented/messaged/cleaned.
*/
VOID
IIIEium_ImportUsageMsg()
{

	IIUGmsg(ERget(S_IE001B_Correct_Syntax_Is), FALSE, 0);

	IIUGmsg(ERget(S_IE001D_IIImport_Usage), FALSE, 0);

	IIUGmsg(ERget(S_IE003D_IIImport_Usage2), FALSE, 0);

        IIUGerr(E_IE0005_Exitmsg, UG_ERR_FATAL, 0);
}


/*
** Name: IIIEsor_SkipObjectRec	-- sets file pointing at the next object
**
** Description:
**	Skips current object. Used for -check option; once we determine
*	if object exists or not, we simply skip it.
**
** Input:
**	FILE	*fp		pointer to intermediate file
**	char	*buffer		current line in file
**
** Output:
**	FILE	*fp		now pointing to next object in file. 
**	char	*buffer		same
**
** Returns:
**	OK		More file to process
**	ENDFILE		That's all, folks!
**
** History:
**	08-sep-1993 (daver)
**		First Documented/messaged/cleaned.
*/
STATUS
IIIEsor_SkipObjectRec(fp, buffer)
FILE	*fp;
char	*buffer;
{

	if (IEDebug)
		SIprintf("\nIn IIIEsor_SkipObjectRec\n\n");

	for (;;)
	{
		/* i hate the ternary operator "?" */
		if (SIgetrec(buffer, IE_LINEMAX, fp) == ENDFILE)
		{
			return (ENDFILE);
		}
		/* 
		** cu_rectype is a wierd one. The "rectype" it returns
		** is either a positive number (and the object class,
		** indicating the start of a new object record), or a 
		** negative number, which is a bit mask indicating
		** whether its a detail or component record of our current
		** object in question. Therefore, we can just check to see
		** if the return value from cu_rectype is positive to 
		** tell if we've found our next object record
		*/
		if (cu_rectype(buffer) > 0)
		{
			/* found our next object! */
			return(OK);
		}
	}
}



/*
** Name: IIIEfoi_FindObjectId	-- see if object exists in a given app
**
** Description:
**	Depending on the class of the object, issue the appropriate
**	query to determine if a object exists or not. If it doesn't
**	exist, call the appropriate routine to get an object id.
**
** Input:
**	IEOBJREC	*objrec		object in question; ->class, ->name
**					are used in query to catalogs
**			 
**	bool		just_check	TRUE if -check is issued; if so,
**					don't create new object id 
**
**	i4		applid		application id of object
**
**	i4		recid		if obj is a recmem, the id of
**					the parent record object
**
** Output:
**	IEOBJREC	*objrec		->id gets a new id if the object
**					doesn't already exist
**					->update is TRUE if the object
**					exists; false if its to be created
**
** Returns:
**	OK				Everything hunky-dory
**	Ingres Error			Something went wrong in the DB selects
**	FAIL				More than one row was found
**
**	Callers will abort transaction and exit if anything but OK is returned.
**
**
** History:
**	08-sep-1993 (daver)
**		First Written/Documented/messages put in erie.msg
**	21-jan-1994 (daver)
**		Turn cat writes back on after calling IIOOnewId().
**		Fixes bug 58569.
**	22-feb-1994 (daver)
**		Enforce frame/procedure name space; if a frame/procedure 
**		doesn't exist, make sure a frame/procedure that's of a
**		different object class (e.g., menu frame != browse frame) 
**		but of the same name doesn't also exist. Otherwise
**		catalogs get all screwed up. Fixes bug 59886.
**  01-Aug-1997 (rodjo04) Bug 84140
**      When trying to recieve the Object ID for OC_FORM, We
**      should get the ID for the 'Effective user'. If you
**      don't, multiple rows can be returned. 
*/
STATUS
IIIEfoi_FindObjectId(objrec, just_check, applid, recid)
IEOBJREC	*objrec;
bool		just_check;
EXEC SQL BEGIN DECLARE SECTION;
i4		applid;
i4		recid;
EXEC SQL END DECLARE SECTION;
{

	EXEC SQL BEGIN DECLARE SECTION;
	char	*object_name;
	i4	object_id;
	i4	object_class;
	i4	dup_class;	/* for error checking */
	i4	rcount;
	i4	errnum;
    char *EffectiveUser = "";
	EXEC SQL END DECLARE SECTION;

    EffectiveUser = IIUIdbdata()->user;   
	object_name = objrec->name;
	object_class = objrec->class;

	if (IEDebug)
		SIprintf("FINDing object id for %s, a %d\n", 
		object_name, object_class);
	/* 
	** depending on the class, we'll issue different queries.
	** that's because not all application objects are stored in
	** ii_abfobjects; only some of them. how nice and consistant.
	** anyway, if its a form, check ii_forms for existance. if its a
	** joindef, check ii_joindefs (there may be others).
	** otherwise, it belongs in ii_abfobjects, and we can use this join. 
	*/
	switch (objrec->class)
	{
	    case OC_FORM:

		EXEC SQL SELECT o.object_id
			 INTO	:object_id
			 FROM   ii_objects o, ii_forms f
			 WHERE 	o.object_name = :object_name
			 AND	o.object_class = :object_class
			 AND	o.object_id = f.object_id
             AND    o.object_owner = :EffectiveUser;  
       	break;
	    case OC_JOINDEF:

		EXEC SQL SELECT DISTINCT o.object_id
			 INTO	:object_id
			 FROM   ii_objects o, ii_joindefs j
			 WHERE 	o.object_name = :object_name
			 AND	o.object_class = :object_class
			 AND	o.object_id = j.object_id;
		break;
	    case OC_RECMEM:

		EXEC SQL SELECT o.object_id
			 INTO	:object_id
			 FROM 	ii_objects o, ii_abfclasses a
			 WHERE	o.object_name = :object_name
			 AND	o.object_class = 2133  /* OC_RECMEM */
			 AND	a.class_id = :recid
			 AND	a.appl_id = :applid
			 AND	a.catt_id = o.object_id;
		break;
	    case OC_REPORT:
		
		EXEC SQL SELECT o.object_id
			 INTO	:object_id
			 FROM	ii_objects o, ii_reports r
			 WHERE	o.object_name = :object_name
			 AND	o.object_class = :object_class
			 AND	o.object_id = r.object_id;
		break;
	    default:
	    /* object lives in ii_abfobjects */

		EXEC SQL SELECT o.object_id
			 INTO	:object_id
			 FROM   ii_objects o, ii_abfobjects a
			 WHERE 	o.object_name = :object_name
			 AND	o.object_class = :object_class
			 AND	o.object_id = a.object_id
			 AND	a.applid = :applid;
		break;
	}

	EXEC SQL INQUIRE_INGRES (:errnum = errorno, :rcount = rowcount);

	if (errnum != 0)
		return errnum;

	if (rcount < 1) 
	{
		/* 
		** Check if object exists under another class. This
		** enforces the frame/procedure namespace. So.. if the
		** object_class of our object is a frame or procedure,
		** see if another frame or procedure exists that's not
		** this object's class. If so, report the error and exit.
		*/
		if ( (  (object_class >= OC_APPLPROC) && 
			(object_class <= OC_AMAXPROC) ) ||
		     (	(object_class >= OC_APPLFRAME) && 
			(object_class <= OC_AMAXFRAME) ) )
		{
			if (IEDebug)
				SIprintf("Does %s, a %d, have another name?\n",
				object_name, object_class);

			EXEC SQL SELECT o.object_id, o.object_class
				 INTO	:object_id, :dup_class
			 	 FROM   ii_objects o, ii_abfobjects a
				 WHERE 	o.object_name = :object_name
	 			 AND	o.object_class <> :object_class
				 AND 	( (o.object_class >= 2020 AND
				 	   o.object_class <= 2100)     OR
					  (o.object_class >= 2200 AND
					   o.object_class <= 2300) )
		 		 AND	o.object_id = a.object_id
			 	 AND	a.applid = :applid;
			EXEC SQL BEGIN
				;
			EXEC SQL END;
			
			EXEC SQL INQUIRE_INGRES (:errnum = errorno, 
						 :rcount = rowcount);
			if (errnum != 0)
				return errnum;
			
			/* 
			** We've detected a potential problem is rcount > 0.
			** IIimport is about to violate the frame/procedure
			** name space. Don't let it! Another object exists in
			** this application that's a frame or procedure with 
			** the same name. Generate an error, and exit.
			*/
			if (rcount > 0)
			{
				IIUGerr(E_IE0043_FrameProc_Name, 
					UG_ERR_ERROR, 3,
					IICUrtoRectypeToObjname(dup_class), 
					object_name,
					IICUrtoRectypeToObjname(object_class));

				return FAIL;
			}
		} /* end if a frame or procedure */

		objrec->update = FALSE;
		if (just_check)
		{
			objrec->id = -1;
		}
		else
		{
			objrec->id = IIOOnewId();
			/*
			** have to turn cat writes back on; 
			** IIOOnewId() is "kind" enough to turn them off
			** for us. How special. Fixes Bug 58569.
			*/
			iiuicw1_CatWriteOn();
			if (IEDebug)
				SIprintf("\nNEW OBJECT! The id is %d\n", 
				objrec->id);
		}
	}
	else if (rcount == 1)
	{
		objrec->update = TRUE;
		objrec->id = object_id;
		if (IEDebug)
			SIprintf("\nFOUND IT! Updating %d\n", object_id);
	}
	else /* rcount > 1 ! */
	{
		IIUGerr(E_IE001E_Multiple_Obj_Found, UG_ERR_ERROR, 2,
			IICUrtoRectypeToObjname(object_class), object_name);

		return FAIL;
	}

	return OK;
}


/*
** Name: IIIElos_LoadObjrecStruct - load objrec structure from file buffer
**
** Description:
**	Fills in the objrec structure with info stored in the file buffer.
**	This is the first line of an object; the data gets written to 
**	ii_objects via IIIEwio_WriteIIObjects().
**
** Input:
**	IEOBJREC *objrec		pointer to the structure
**	char	 *inbuf			line buffer containin the info
**
** Output:
**	IEOBJREC *objrec		filled in structure
**
** Returns:
**	None
**
** History:
**	11-aug-93 (daver)
**	First Written; needed since processing record types requires 
**	this same functionality too, and its good programming shme to put it
**	in one routine.
*/
VOID
IIIElos_LoadObjrecStruct(objrec, inbuf)
IEOBJREC	*objrec;
char		*inbuf;
{
	/*
	** load the structure; get the class from cu_rectype. NOTE:
	** cu_gettok (in abf!copyutil!cuutils.c) works on a string
	** at a time. The first call with a string initializes it; each
	** subsequent call with a NULL char pointer says resume in the
	** string where we left off. That's how we can get the next
	** elements in the OBJECT Record line. (hey, the routine
	** existed, and I'm just *using* it, ok?).
	**
	** Also note: by COPYING the string off the inbuf into the
	** record, the buffer is now usable; otherwise, each SIgetrec
	** would wipe out the current buffer. So we always copy the data
	** off the buffer for safe keeping.
	*/
	objrec->class = (i4)cu_rectype(inbuf);  /* class is an i4 (OOID)*/
	cu_gettok(inbuf);               	/* clear the rectype token */
	CVan(cu_gettok((char *)NULL), &(objrec->level));
	STcopy(cu_gettok((char *)NULL), objrec->name);
	STcopy(cu_gettok((char *)NULL), objrec->short_remark);
	STcopy(cu_gettok((char *)NULL), objrec->long_remark);
}
	


/*{
** Name: IIIEpar_ProcessAppRecord      -- "skip" main application record
**
** Description:
**	Skips the record in the intermediate file beginning with
**	OC_APPL. This application record is irrelevant, since the application
**	itself already exists. However, we want to pickup the source code
**	directory for the application we're copying in, in order to find source
**	files to copy.
**
** Input:
**      FILE            *fp             file pointer
**      char            *inbuf          current line in file
**	char		*srcdir		pointer to a buffer
**
** Output:
**	FILE		*fp		file pointer, updated
**      char            *inbuf          current line in file (past OC_APPL)
**	char		*srcdir		filled in with source code
**					directory from file
** Returns:
**      OK				yippie-eye-ehh, little doggie
**      ENDFILE				At EOF
**	E_IE001F_No_Source_Line		File is corrupted
**
** History:
**	18-aug-1993 (daver)
**		First Written/Documented.
**	08-sep-1993 (daver)
**		Messages to erie.msg; more comments.
*/
STATUS
IIIEpar_ProcessAppRecord(fp, inbuf, srcdir)
FILE		*fp;
char		*inbuf;
char		*srcdir;
{
	STATUS	stat;
	i4	rectype;
	char	*s;

	if (IEDebug)
		SIprintf("\nIn IIIEpar_ProcessAppRecord\n\n");

	/* NEED TO raise an error, too, I would think! */
	if (SIgetrec(inbuf, IE_LINEMAX, fp) == ENDFILE)
		return (ENDFILE);
	
	rectype = cu_rectype(inbuf);
	
	if (rectype != CUD_LINE)
	{
		IIUGerr(E_IE001F_No_Source_Line, UG_ERR_ERROR, 0);
		return(E_IE001F_No_Source_Line);
	}

	cu_gettok(inbuf);		/* clear opening tab */
	s = cu_gettok((char *)NULL);	/* now get source code directory */
	STcopy(s, srcdir);

	/*
	** and that's all we wanna know! find the next object record, 
	** and return.
	*/
	return(IIIEsor_SkipObjectRec(fp, inbuf));
}


/*{
** Name: IIIEfco_FoundCursorObject      -- find cursor object in IEPARAM list
**
** Description:
**      Given an id, a linked list of name/id records, and allflags bit
**	mask, this routine returns TRUE if -allframes was specified or
**	if the id is in this list, and false otherwised. Used in cursor 
**	selects of frames.
**
** Input:
**	IEPARAMS	*framelist	list of frames on command line
**	i4		allflags	bit mask of -allflags
**	i4		id		object in question's id
**
** Output:
**	None
**
** Returns:
**      TRUE		id in list or ALLFRAMES bit turned on
**      FALSE		otherwise
**
** History:
**	18-aug-1993 (daver)
**		First Written.
**	08-sep-1993 (daver)
**		Documented.
*/
bool
IIIEfco_FoundCursorObject(framelist, allflags, id)
IEPARAMS	*framelist;
i4		allflags;
i4		id;
{
	IEPARAMS	*f;
	bool		found;


	if (allflags & ALLFRAMES)
		return TRUE;
	
	f = framelist;
	found = FALSE;

	while (f != NULL)
	{
		if (f->id == id)
		{
			found = TRUE;
			break;
		}
		f = f->next;
	}
	return (found);
}


/*{
** Name: IIIEfoe_FileOpenError      -- issue errors related to file shme
**
** Description:
**      This routine is called a few places, and thus modularized. Its
**	shared by IIEXPORT and IIIMPORT (the former opening listing and
**	intermediate files, the latter opening listing, intermediate and 
**	source files. It prints the appropriate message depending on
**	whether we're just opening or creating a file.
**
** Input:
**	LOCATION	*loc		LOCATION of troubled filename 
**	char		*type		"intermediate", "temporary", etc
**	bool		create		True if creating; False if opening
**	char		imp_or_exp	either "IIIMPORT" or "IIEXPORT"
**	
**
** Output:
**	None
**
** Side Effects:
**      Prints the appropriate error message.
**
** History:
**	18-aug-1993 (daver)
**		First Written/Documented.
**	03-sep-1993 (daver)
**		Extracted to ieutils while fixing listfile problems.
**	24-jan-1994 (daver)
**		Call LOtos w/char ** rather than buffer. Fixes bug 58567.
*/
VOID
IIIEfoe_FileOpenError(loc, type, create, imp_or_exp)
LOCATION *loc;
char	*type;
bool	create;
char	*imp_or_exp;
{
	char *fname;

	LOtos(loc, &fname);

	/* I refuse to use the ternary operator `?' */
	if (create)
	{
		/* this message is shared with IIEXPORT */
		IIUGerr(E_IE000B_No_File_Create, UG_ERR_ERROR, 3, 
				fname, type, imp_or_exp);
	}
	else
	{
		IIUGerr(E_IE000A_No_File_Copy, UG_ERR_ERROR, 1, fname);
	}
}
