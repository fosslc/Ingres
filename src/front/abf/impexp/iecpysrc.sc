/*
**	Copyright (c) 1985, 2004 Ingres Corporation
*/

# include	<compat.h>
# include       <lo.h>
# include	<pe.h>		/* 6-x_PC_80x86 */
# include	<me.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include       <er.h>
# include       <nm.h>
# include       <si.h>
# include       <st.h>
# include	<ooclass.h>
# include	<oocat.h>
# include	<abclass.h>
# include	<ug.h>
# include	"erie.h"
# include	"ieimpexp.h"

/*
** Name: iecpysrc.sc		-- copy the source files, if specified
**
** Defines:
**	IIIEcsf_CopySourceFiles()	- process the copysrc flag
**	IIIEci_Copy_It()		- actually perform the file xfer
**
** Description:
**	Handles the -copysrc and -listing flags, which require going through
**	the list of sources in the application, so this is where we do it.
**
** History:
**	25-aug-93 (daver)
**		First Written/Stolen from abf!copyapp!insrcs.qsc
**	14-sep-1993 (daver)
**		Got rid of statics, re-documented.
**	01-dec-1993 (daver)
**		Used char buffers directly with LOtos() to determine if
**		the source and destination directories are the same.
**		Fixes bug #57350.
**	04-jan-1994 (daver)
**		Fixed bugs 57935 and 58031: Allowed listing files to 
**		be generated when the source code directories of both 
**		the incoming and target frames are the same. Also fixed 
**		problems in marking these frames to be regenerated when
**		said directories are the same, and when they're not.
**	15-may-97 (mcgem01)
**		lo.h must be included before pe.h
*/

EXEC SQL INCLUDE SQLCA;
EXEC SQL WHENEVER SQLERROR CALL SQLPRINT;

static	VOID	IIIEci_Copy_It();

/*
** Name: IIIEcsf_CopySourceFiles	- copy source files!
**
** Description:
**	Handles the -copysrc and -listing flags, which require going through
**	the list of sources in the application, so this is where we do it.
**
** Input:
**	char		*oldsrcdir	old source directory.
**	char		*newsrcdir	new source directory 
**	i4		applid		application id.
**	bool		xfer		TRUE if we are to transfer the files.
**	char		*lfile		list file; = EOS if no listing desired
**	IEPARAMS	*srclist	list of candidate frame/procs to copy
**
** Output:
**	None
**
** Side Effects:
**	Transfers files of those frames and procedures on the srclist.
**	Opens and writes to the lfile (if desired) the files copied.
**
** History
**	25-aug-1993 (daver)
**		First stolen/written from copysrcs in abf!copyapp!insrcs.qsc
**	01-dec-1993 (daver)
**		Used char buffers directly with LOtos() to determine if
**		the source and destination directories are the same.
**		Fixes bug #57350.
**	04-jan-1994 (daver)
**		Fixed bugs 57935 and 58031: Allowed listing files to 
**		be generated when the source code directories of both 
**		the incoming and target frames are the same. Also fixed 
**		problems in marking these frames to be regenerated when
**		said directories are the same, and when they're not.
*/
VOID
IIIEcsf_CopySourceFiles(oldsrcdir, newsrcdir, applid, xfer, lfile, srclist)
char		*oldsrcdir;
char		*newsrcdir;
EXEC SQL BEGIN DECLARE SECTION;
i4		applid;
EXEC SQL END DECLARE SECTION;
bool		xfer;
char		*lfile;
IEPARAMS	*srclist;
{
	EXEC SQL BEGIN DECLARE SECTION;
	char		tempbuf[MAX_LOC +1];    /* buffer for temploc */
	i4		flags;
	i4		class;
	i4		oid;
	char		oname[FE_MAXNAME+1];
	EXEC SQL END DECLARE SECTION;

	LOCATION	lfileloc;
	LOCATION	temploc;	/* for filename retrieved by LOlist */

	char		*newptr;
	char		*oldptr;
	bool		same_dir = FALSE; /* source and dest the same dir? */
	bool		physical_xfer;	/* if source=dest, don't xfer */

	char		istr[FE_MAXNAME +1];
	char		isrcdir[MAX_LOC +1];

	LOCATION	Ofileloc;	/* file to copy to */
	LOCATION	Nfileloc;	/* file to copy from */
	LOCATION	Ifileloc;	/* intermediary test file for copy */

	FILE		*lfp;

	if (IEDebug)
		SIprintf("\n COPYING SOURCES:\n\to: %s\n\tn: %s\n\tl: %s\n",
				oldsrcdir, newsrcdir, lfile);
	/*
	** open list file if specified.
	*/
	if (*lfile != EOS)
	{
		LOfroms(PATH&FILENAME, lfile, &lfileloc);
		if (SIopen(&lfileloc, ERx("w"), &lfp) != OK)
		{
			IIIEfoe_FileOpenError(&lfileloc, "listing", 
					TRUE, ERx("IIIMPORT"));
			lfp = NULL;
		}
	}
	else
	{
		lfp = NULL;
	}

	if (xfer)
	{
		IIUGmsg(ERget(S_IE0006_Copying_source_files), FALSE, 0);
	}

	/*
	** setup destination location
	*/
	if (newsrcdir != NULL && *newsrcdir != EOS)
	{
		/* 
		** So, Nfileloc is our newsrcdir location... the
		** target application's source code direcotry. 
		*/
		LOfroms(PATH, newsrcdir, &Nfileloc);
	}

	if (IEDebug)
		SIprintf("\tlist file specified, locations set up ...\n");

	/*
	** we only care about the status of the source file directory if
	** we're really transferring stuff.
	*/
	if (xfer)
	{
		/*
		** If our old directory and new directory are the same, we've
		** "copied" everything without doing anything - simply return.
		** Pass through LO to make this comparison so that we have
		** some chance of resolving different string representations
		** for the same directory to some canonical form which will
		** compare equal.
		*/

		/*
		** a bad, zero length source dir? someone's been messing
		** with the intermediate file!!
		*/
		if (STlength(oldsrcdir) <= 0)
		{
			IIUGerr(E_IE0007_No_Source_Dir, UG_ERR_ERROR, 0);
			return;
		}

		/*
		** verify old source dir exists. fair enough. 
		** Ofileloc is our "source", the oldsrcdir. 
		*/
		LOfroms(PATH, oldsrcdir, &Ofileloc);
		if (LOexist(&Ofileloc) != OK)
		{
			IIUGerr(E_IE0008_Dir_not_exist, UG_ERR_ERROR, 
								1, oldsrcdir);
			return;
		}

		/* if oldsrc and newsrc are the same, why copy files ? */
		LOtos(&Ofileloc, &oldptr);
		LOtos(&Nfileloc, &newptr);
		same_dir = STequal(newptr,oldptr);

		if (!same_dir)
		{
			/*
			** Ifileloc is set to the newsrcdir, which is the 
			** target application's source code directory... 
			** (and where we'll want to create files ...)
			*/
			STcopy(newsrcdir, isrcdir);
			LOfroms(PATH, isrcdir, &Ifileloc);

			/*
			** make up a dummy file name. put it in tempbuf, make a
			** location out of it in temploc, and copy just the 
			** filename element to our Ifileloc, our target 
			** application source directory (that's what LOstfile 
			** does -- just copied the filename element).
			*/
			STcopy(ERx("iiIEsrcX.tmp"),istr);
			LOfroms(FILENAME, istr, &temploc);
			LOstfile(&temploc, &Ifileloc);
		}
	}

	/*
	** Only *physically transfer* the files if we're told to by the xfer
	** flag, AND we're not transfering to the same directory!! Used
	** in IIIEci_Copy_It() which performs the physical copy of source files.
	*/
	physical_xfer = (xfer && !(same_dir));

	/* 
	** handle each source file for frames and procedures; try using
	** a cursor...
	*/
	if (IEDebug)
		SIprintf("\n COPYSRC cursor fetch... \n\n");

	/*
	** Here, we make some assumptions based on how the objects are
	** numbered: irrelevant stuff less than 2020; procedures between
	** 2020 (OC_APPLPROC) and 2100 (OC_AMAXPROC); frames between 
	** 2200 (OC_APPLFRAME) and 2300 (OC_AMAXFRAME). 
	** So, in restricting the query, check for procedure objects 
	** and frame objects between their respective ranges...
	*/
	EXEC SQL DECLARE iicpysrc CURSOR FOR
	SELECT DISTINCT a.abf_source, a.abf_flags,
			a.object_id, o.object_class,
			o.object_name
	FROM ii_abfobjects a, ii_objects o
	WHERE	a.applid = :applid
	AND	a.object_id = o.object_id
	AND	a.abf_source <> ''	
	AND ( 	(o.object_class >= 2020 AND o.object_class < 2100)
		OR
		(o.object_class >= 2200 AND o.object_class < 2300)
	    );

	EXEC SQL OPEN iicpysrc;

	while (sqlca.sqlcode == 0)
	{
		EXEC SQL FETCH iicpysrc 	/* woof! woof! */
			 INTO :tempbuf, :flags, :oid, :class, :oname;

		if (sqlca.sqlcode != 0)
			break;

		if (IEDebug)
			SIprintf("cursor: class is %d, id is %d, name is %s\n",
				class, oid, oname);
		
		/*
		** Here, must check id of object in loop vs found frame/proc
		** id list. if item is in the list, then process; else
		** continue with the next item in the list. pass in 0
		** to indicate to check all objects in list.
		*/
		if (!IIIEfco_FoundCursorObject(srclist, 0, oid))
			continue;

		if (IEDebug)
			SIprintf("\tYESSSS!! MADE THE CUT!!\n");

		switch(class)
		{
		case OC_BRWFRAME:
		case OC_APPFRAME:
		case OC_MUFRAME:
		case OC_UPDFRAME:
			/*
			** Vision frames are either CUSTOM, or they can be
			** re-generated. If they are CUSTOM, call the routine 
			** to physically copy the file (and write to the 
			** listing file, if necessary).  Copyapp does a bunch 
			** of other stuff (see insrcs.qsc) that is irrelevant 
			** (and incorrect) for iiimport.
			** Fixes bug 58031, 57935.
			*/
			if ((flags & CUSTOM_EDIT) != 0)
			{
				IIIEci_Copy_It(tempbuf, physical_xfer, lfp, 
					&Ofileloc, &Nfileloc, &Ifileloc);
			}
			/*
			** The regenerate-able frames are marked 'new' when 
			** they are read in via IIIEwao_WriteIIAbfObjects().
			** Part of fix for bug 58031.
			*/
			break;
		default:
			/*
			** "Other" frames and procedures (e.g, USER frames) 
			** have files associated with them; they'll need to get
			** copied to the new directory and/or written out to
			** the listing file too. It seems that they don't
			** need to be marked as new since they don't get
			** generated, anyway.
			*/
			IIIEci_Copy_It(tempbuf, physical_xfer, lfp,
					&Ofileloc, &Nfileloc, &Ifileloc);
			break;
		}

	} /* end while sqlcode == 0 */

	EXEC SQL CLOSE iicpysrc;

}

/*
** Name: IIIEci_Copy_It		- actualy perform the copy 
**
** Description:
**	Does the actual copy, via SIcat; also adds a line to the list
**	file, if necessary, noting that some source got copied. Local to
**	this file only.
**
** Input:
**
**	char            *name		name of the file being transfered
**
**	bool            xfer		really do the transfer(false means that
**					just the -listing flag was specified
**
**	FILE            *lfp;		listing file (NULL if not specified)
**
**	LOCATION        *oldloc;	"from" source code dir location
**
**	LOCATION        *newloc;	"to" source code dir location
**
**	LOCATION        *intloc;	same as above; we copy thru a 
**					dummy file name
** Output:
**	None
**
** Side Effects:
**	Transfers files of those frames and procedures on the srclist.
**	Opens and writes to the listing file (if desired) the names of the 
**	files copied.
**
** History
**	25-aug-1993 (daver)
**		First stolen/written from copysrcs in abf!copyapp!insrcs.qsc
**	14-sep-1993 (daver)
**		Got rid of statics, re-documented.
*/
static VOID
IIIEci_Copy_It(name, xfer, lfp, oldloc, newloc, intloc)
char		*name;
bool		xfer;
FILE		*lfp;
LOCATION	*oldloc;
LOCATION	*newloc;
LOCATION	*intloc;
{
	LOCATION	temploc;	/* for filename retrieved by LOlist */
	FILE		*fptr;

	if (IEDebug)
	{
		SIprintf("In IIIEci_Copy_It: %s  ", name);
		if (xfer) SIprintf (" XFER ");
		if (lfp == NULL) SIprintf ("No List\n");
		else SIprintf ("YES LIST!\n");
	}

	/* write to listing file. that's it!! */
	if (lfp != NULL)
		SIfprintf(lfp,ERx("%s\n"),name);

	if (xfer)
	{
		LOfroms(FILENAME, name, &temploc);
		/*
		** LOstfile copies the filename field to the dest loc 
		** so after these two statements, both oldloc and
		** newloc have the same filename field. so far so good.
		*/
		LOstfile(&temploc, oldloc);
		LOstfile(&temploc, newloc);

		/*
		** open up the intloc. this is the one with the dummy
		** name in it... (if we can't open it, issue the first
		** error). SIcat performs the copy... so copy
		** the file in "source" to our newly opened temp file ...
		** (if we can't copy it, issue the second error, close 
		** the temp file with the funny name, and delete it).
		** Once all is well, rename the funny file in the target
		** directory (but first delete an existing file if
		** necessary). Why, you ask?
		**
		** We "copy" through a temp file so that
		** we don't wipe out any copies of the
		** target file which might exist until
		** we've successfully copied.  This takes
		** care of us if we are actually copying a
		** directory onto itself in some way that
		** the directory name check doesn't catch,
		** such as scenarios involving symbolic
		** links.
		*/
		if (SIopen(intloc, ERx("w"), &fptr))
		{
			IIIEfoe_FileOpenError(intloc, "temporary", 
						TRUE, ERx("IIIMPORT"));
		}
		else if (SIcat(oldloc, fptr))
		{
			IIIEfoe_FileOpenError(oldloc, "", 
						FALSE, ERx("IIIMPORT"));
			SIclose(fptr);

			/* delete the 0 length file */
			LOdelete(intloc);
		}
		else
		{
			SIclose(fptr);
			_VOID_ LOdelete(newloc);
			LOrename(intloc, newloc);
		}
	}
}
