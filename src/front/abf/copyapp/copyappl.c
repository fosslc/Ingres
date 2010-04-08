/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>
#include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ooclass.h>
#include	<cu.h>
#include	"copyappl.h"

/**
** Name:	copyappl.c
**
**	Defines
**		copyappl--main routine of the copyapp utility
**
**	Parameters:
**		argc		{int}
**		argv[]		{ char *}
**				passed directly from main
**
**	Returns:
**		Nothing
**
**	Called by:
**		main
**
**	Side Effects:
**		Creates and writes to intermediate file on copy out.
**		Creates temp files and writes to system catalogs on a copy in.
**
**	Trace Flags:
**		None
**
**	Error Messages:
**		If bad command line (missing arguments, bad flag name, etc.);
**		fatal error in intermediate file; inability to open a file.
**
**	History:
**		Written 11/15/83 (agh)
**		18-sep-86 (marian)
**			Pass the new name of the application to copysrcs so
**			the correct replace on the system catalog can be made.
**		19-sep-86 (marian)	bug # 10399
**			Add error to disallow -q and -r flag.
**
**		15-oct-1986 (sandyd)
**			Saturn changes.	 Use FEingres() and FEing_exit().
**		12/01/86 (a.dea) -- Change some ifndef CMS into FT3270
**			in order to make QA testing in CMS work.
**		7-aug-1987 (Joe)
**			Started changes for 6.0.  Don't use forms
**			mode.  Take out FT3270 ifdefs.
**
**		5-oct-1987 (Joe)
**			Fixes for 6.0 bug 1191.  Made the -d flag
**			work again.
**		28-dec-89 (kenl)
**			Added handling of the +c (connection/with clause) flag.
**
**		12-jan-1990 (blaise)
**			Added #ifndef NO_VIGRAPH ... to prevent graph functions
**			from being called for ports that don't have VIGRAPH.
**
**		2/90 (bobm)
**			Fix interaction between -r and -p flags (bug 7718).
**			-p no longer prevents -r.
**		3/90 (bobm)
**			Fix -q flag to work as documented, ie. you get
**			one big transaction around the whole mess if you
**			specify -q (bug 20077).
**		4/90 (Mike S)
**			IBM porting changes.  No Vigraph, handle -s, -t, and
**			someday -a flags.
**		9/90 (Mike S)
**			HP porting change 130378 from mrelac.
**		14-Nov-1990 (blaise)
**			Display an informative usage message when the command
**			is issued with a bad argument. (bug 34312).
**		16-aug-91 (leighb)
**			There is NO_VIGRAPH on the IBM/PC (PMFE).
**		02-dec-91 (leighb) DeskTop Porting Change:
**			Added routines to pass data across ABF/IAOM boundary.
**			Added call to IIAMgobGetIIamObjects() routine to pass 
**			data across ABF/IAOM boundary.
**			(See iaom/abfiaom.c for more information).
**		27-jan-92 (davel)
**			Fixed bug 40231 in addfunc().
**		19-aug-92 (davel)
**			Allow -a to not take a directory specification; use
**			current working directory if none specified.
**		11-nov-92 (essi)
**			Fixed bug 45937. The 'in' phase of the copyapp
**			blanked out the source field if -l was specified
**			without -a or -s. We now default to current
**			directory if no directory was specified.
**		02-Dec-92 (fredb)
**			MPE/iX (hp9_mpe) porting change: Changed temporary
**			filename from "iicopyapp", which was too long anyway,
**			to "iicopyap.tmp".
**		7-jan-93 (blaise)
**			Removed convert parameter from call to cu_rdobj();
**			from 6.5 we no longer attempt to convert pre-6.0 
**			intermediate files.
**		26-jan-93 (essi)
**			Backed out my previous fix. Implemented the fix in
**			insrcs.qsc.
**		17-Apr-93 (fredb)
**			More MPE/iX (hp9_mpe) porting changes/fixes:
**			- MPE does not support Vigraph, define NO_VIGRAPH
**			- Substitute calls to the special MPE CL routine,
**			  LOabf_get_src_dir(), for calls to LOgt() when
**			  processing the '-s' and '-a' flags.  LOgt() does
**			  not return what we want given the MPE file system
**			  structure. (Bug 50549)
**		02-aug-93 (daver)
**			Addded 4th param to fetch_sdir: applcation owner. This 
**			is unused here, but required for iiimport. So copyappl 
**			passes in a NULL char pointer.
**		25-may-95 (emmag)
**			NT doesn't support VIGRAPH.  Define NO_VIGRAPH
**      	24-sep-96 (hanch04)
**              	Global data moved to data.c
**      21-jan-98 (rodjo04)
**          Bug 88370. If -q is passed from the command line 
**          we must pass TRUE to new function SetQuit_flag so 
**          that IICUpfaPostFormAdd() in formadd.sc will know
**          that -q flag was sent.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/


FUNC_EXTERN OO_OBJECT **	IIAMgobGetIIamObjects();      
FUNC_EXTERN  VOID           SetQuit_flag();

STATUS IICA4nf4GLNewFrame();

# ifdef IBM370
# define NO_VIGRAPH
# endif

# if defined (PMFE) || defined (NT_GENERIC)
# define NO_VIGRAPH
# endif

# ifdef hp9_mpe
# define NO_VIGRAPH
# endif

#ifndef NO_VIGRAPH
GLOBALREF OO_OBJECT	*IIgrObjects[];
#endif

GLOBALREF	bool	caPromptflag ;
		/*
		** -p flag.  Default is to prompt if an object is found in
		** the database with the same name as the one being read in.
		*/
GLOBALREF	bool	caReplaceflag ;	/* -r flag */
GLOBALREF	bool	caQuitflag ;	/* -q flag */
GLOBALREF	char	*caUname ;	/* name of user for -u flag */
GLOBALREF	char	*caXpipe ;	/* for -X flag */

/*
** These are static since used by addfunc
*/
static char	*caApplname;
static char	*caNewname ZERO_FILL;
static bool	caNameflag = FALSE;	/* -n flag */
static	STATUS		addfunc();
static	STATUS		conproc();
static VOID 		IICAprompt();
static char		Srcdir[240];
static i4		Applid = -1;
static bool		HasMeta;

VOID
copyappl(argc, argv)
i4	argc;
char	*argv[];
{
	char		*dbname = ERx("");	  /* name of the old database */
	char		*dirname = ERx("");	     /* intermediate dir name */
	char		*filename =  ERx("");	/* intermediate file name */
	char		*newsrcdir = ERx("");	/* new directory for src files*/
	char		newsrcbuf[MAX_LOC+1];	/* buffer for default loc */
	char		direction[FE_MAXNAME+1];/* direction of copyapp */
	char		response[FE_PROMPTSIZE];/* buffer for prompt response */
	char		locbuf[MAX_LOC+1];	/* buffer for location loc */
	char		filebuf[MAX_LOC+1];	/* location buffer */
	char		*with_ptr = ERx("");	/* connection (with) params. */
	bool		inflag = TRUE;

				/*
				** *listfile = EOS implies stdout!!!
				*/
	char		*listfile = NULL;
		/*
		** TRUE if application to be copied to an intermediate file;
		** FALSE if application to be put into a new database
		*/
	bool		dirflag = FALSE;	/* TRUE if -d flag set */
	bool		tempflag = FALSE;	/* -t flag */
	bool		srcflag = FALSE;	/* -s flag */
	bool		cleansrcflag = FALSE;	/* -g flag */
	bool		cleanflag = FALSE;	/* -c flag */
	register i4	i;
	LOCATION	loc;
	CUCONFLICT	conflict;
	i4		xactnum;

	STATUS		IICAcaoCopyApplOut();

	/*
	** Process the command line.
	** First pull out any flags.
	*/

	for ( i = 2 ; i < argc ; ++i )
	{
		if (*(argv[i]) == '-')	/* flag found. Set and clear out */
		{
			switch (*(argv[i]+1))
			{
			  case 'p':
				caPromptflag = TRUE;
				break;
			  case 'd':
				dirflag = TRUE;
				dirname = argv[i] + 2;
				if (STlength(dirname) <= 0)
				{
					IIUGerr(FLAGARG, 0, 1, ERx("-d"));
					errexit(FALSE);
				}
				break;
			  case 't':
				tempflag = TRUE;
				filename = argv[i] + 2;
				if (STlength(filename) <= 0)
				{
					IIUGerr(FLAGARG, 0, 1, ERx("-t"));
					errexit(FALSE);
				}
				break;
			  case 's':
				srcflag = TRUE;
				newsrcdir = argv[i] + 2;
				if (cleansrcflag)
				{
					IIUGerr(E_CA0022_GSCNFLT, 0, 0);
					errexit(FALSE);
				}
				else if (STlength(newsrcdir) <= 0)
				{
#ifdef hp9_mpe
					/*
					** newsrcbuf will contain the
					** current MPE account name.
					** (Bug 50549)
					*/
					newsrcbuf[0] = EOS;
					LOabf_get_src_dir(newsrcbuf);
#else
					if ( LOgt(newsrcbuf, &loc) != OK )
					{
						IIUGerr(DIREXIST, 0, 1, 
								newsrcbuf);
						errexit(FALSE);
					}
#endif
					newsrcdir = newsrcbuf;
				}
				break;
			  case 'a':
				cleansrcflag = TRUE;
				newsrcdir = argv[i] + 2;
				if (srcflag)
				{
					IIUGerr(E_CA0022_GSCNFLT, 0, 0);
					errexit(FALSE);
				}
				else if (STlength(newsrcdir) <= 0)
				{
#ifdef hp9_mpe
					/*
					** newsrcbuf will contain the
					** current MPE account name.
					** (Bug 50549)
					*/
					newsrcbuf[0] = EOS;
					LOabf_get_src_dir(newsrcbuf);
#else
					if ( LOgt(newsrcbuf, &loc) != OK )
					{
						IIUGerr(DIREXIST, 0, 1, 
								newsrcbuf);
						errexit(FALSE);
					}
#endif
					newsrcdir = newsrcbuf;
				}
				break;

			  case 'r':
				caReplaceflag = TRUE;
				/* bug 10399
				**	make sure -q isn't already set
				*/
				if (caQuitflag == TRUE)
				{
					IIUGerr(FLAGCNFLT, 0, 0);
					errexit(FALSE);
				}
				break;

			  case 'q':
				caQuitflag = TRUE;
				SetQuit_flag(TRUE);
				/* bug 10399
				**	make sure -r isn't already set
				*/
				if (caReplaceflag == TRUE)
				{
					IIUGerr(FLAGCNFLT, 0, 0);
					errexit(FALSE);
				}
				break;

			  case 'c':
				cleanflag = TRUE;
				break;

			  case 'l':
				/*
				** Use current directory if none is specified.
				** This fixes bug 45937 where 'copyapp in'
				** blanks out the source directory if -l
				** is specified without -a or -s.
				*/
				if (STlength(newsrcdir) <= 0)
				{
					if ( LOgt(newsrcbuf, &loc) != OK )
					{
						IIUGerr(DIREXIST, 0, 1, 
								newsrcbuf);
						errexit(FALSE);
					}
					newsrcdir = newsrcbuf;
				}
				listfile = argv[i] + 2;
				break;

			/*
			** BUG 4391
			**
			** Check for '-n' flag.
			*/
			  case 'n':
				caNameflag = TRUE;
				caNewname = argv[i] + 2;
				if ( STtrmwhite(caNewname) <= 0 )
				{
					IIUGerr(FLAGARG, 0, 1, ERx("-n"));
					errexit(FALSE);
				}
				else if ( FEchkname(caNewname) != OK )
				{
					IIUGerr(S_CA0027_IllegalName, 0, 1, 
						caNewname);
					errexit(FALSE);
				}
				break;

			  case 'u':
			  case 'U':
				caUname = argv[i];
				break;

			  case 'X':
				caXpipe = argv[i];
				break;

			  default:
				IIUGerr(BADFLAG, 0, 1, argv[i]);
				IIUGerr(S_CA0026_Correct_syntax, 0, 1, argv[i]);
				FEexits((char *)NULL);
				PCexit(FAIL);
			}
			argv[i] = NULL;		/* clear out the flag */
		}
		else if (*(argv[i]) == '+')
		{
			switch(*(argv[i]+1))
			{
			  case('c'):	/* connection parameters */
			  case('C'):
				/* set up WITH clause for CONNECT */
				with_ptr = STalloc(argv[i] + 2);
				IIUIswc_SetWithClause(with_ptr);
				break;

			  default:
				IIUGerr(BADFLAG, 0, 1, argv[i]);
				IIUGerr(S_CA0026_Correct_syntax, 0, 1, argv[i]);
				FEexits((char *)NULL);
				PCexit(FAIL);
			}
			argv[i] = NULL;		/* clear out the flag */
		}
	}
	FEcopyright(ERx("COPYAPP"), ERx("1984"));
	/* next determine the direction of copying */

	/*
	** BUG 4791
	**
	** Check that at least one argument has been specified.
	*/
	if (argc <= 1)
	{
		IICAprompt(ERget(S_CA0001_Direction_of_copy_), response);
		STlcopy(response, direction, 12);
	}
	else
	{
		STlcopy(argv[1], direction, 12);
		argv[1] = NULL;
	}

	if (STbcompare(direction, 0, ERget(F_CA0001_out), 0, TRUE) == 0)
	{
		inflag = FALSE;

# ifdef DGC_AOS
		if (*with_ptr == EOS)
		{
			with_ptr = STalloc(ERx("dgc_mode='reading'");
			IIUIswc_SetWithClause(with_ptr);
		}
# endif
	}
	else if (STbcompare(direction, 0, ERget(F_CA0002_in), 0, TRUE) != 0)
	{
		IIUGerr(DIRECTION, 0, 0);
		IIUGerr(S_CA0026_Correct_syntax, 0, 1, argv[i]);
		FEexits((char *)NULL);
		PCexit(FAIL);
	}

	/*
	** Now check to make sure that the name of the (old or new)
	** database was specified.  If not, ask for it with a prompt.
	*/

	for ( i = 1 ; i < argc ; ++i )
	{
		if ( argv[i] != NULL )
		{
			dbname = argv[i++];
			break;
		}
	}

	if (STlength(dbname) <= 0)
	{
		IICAprompt(ERget(S_CA0002_Database_name_), response);
		dbname = STalloc(response);
	}

	/*
	** The next parameter is either the application name (on a copy out),
	** or the name of the intermediate file (on a copy in)
	*/

	caApplname = ERx("");
	for ( ; i < argc; ++i )
	{
		if ( argv[i] != NULL )
		{
			if (inflag)
				filename = argv[i++];
			else
				caApplname = argv[i++];
			break;
		}
	}

	if (inflag)
	{
		if (STlength(filename) <= 0)
		{	/* no name given for intermediate file.	 Prompt. */
			IICAprompt(ERget(S_CA0003_Name_of_intermediate_), response);
			filename = STalloc(response);
		}
	}
	else if (STlength(caApplname) <= 0)
	{ /* no application name given */
		IICAprompt(ERget(S_CA0004_Application_name_), response);
		caApplname = STalloc(response);
	}

	if ( !inflag && !tempflag )
	{ /* default name of intermediate file */
# ifdef CMS
		filename = ERx("copyapp.ingtemp");
# else
#  ifdef hp9_mpe
		filename = ERx("iicopyap.tmp");
#  else
		filename = ERx("iicopyapp.tmp");
#  endif
#endif
	}

# ifdef CMS
	if ( dirflag )
		STpolycat(3, dirname, ":", filename, locbuf);
	else
		STcopy( filename, locbuf);
	/* check for write access comes later */
	LOfroms(PATH&FILENAME, locbuf, &loc);
# else
#  ifdef hp9_mpe
	STcopy( filename, locbuf);
	LOfroms(PATH&FILENAME, locbuf, &loc);
#  else
	if ( !dirflag )
	{
		if ( LOgt(locbuf, &loc) != OK )
		{
			IIUGerr(DIREXIST, 0, 1, locbuf);
			errexit(FALSE);
		}
	}
	else
	{ /* create location for directory with intermediate file */
		STlcopy(dirname, locbuf, sizeof(locbuf));
		if ( LOfroms(PATH, locbuf, &loc) != OK || LOexist(&loc) != OK )
		{
			IIUGerr(DIREXIST, 0, 1, locbuf);
			errexit(FALSE);
		}
	}

	LOfstfile(filename, &loc);
#  endif	/* hp9_mpe */
# endif	/* CMS */

	LOtos(&loc, &filename);

	/* start processing the command line */

	if ( FEingres(dbname, caUname, caXpipe, (char *)NULL) != OK )
	{
		IIUGerr( E_UG000F_NoDBconnect, UG_ERR_ERROR, 1, dbname );
		errexit( FALSE );
	}

	/*
	** if -q flag was specified, create one big transaction for
	** the whole copyapp operation.
	*/
	if ( inflag && caQuitflag )
		IIUIlxbLabelXactBegin(&xactnum);

	IIOOinit( IIAMgobGetIIamObjects() );		 

#ifndef NO_VIGRAPH
	IIOOinit( IIgrObjects );	/* to copy graphs */
#endif

	if ( inflag )
	{ /* do a copy in */
		STATUS	rval;

		HasMeta = FALSE;

		conflict.cuwhen = CUFOREACH;
		conflict.cuconflict = conproc;
		rval = cu_rdobj(filename, ERx("COPYAPP"), &conflict, addfunc);
		/*
		** this will either commit the one big transaction,
		** or abort it again to make sure if anything inside
		** it aborted.
		*/
		if (caQuitflag)
			IIUIlxeLabelXactEnd(xactnum);
		if (rval == OK)
		{
			if (Applid < 0)
			{
				/*
				** add 4th param, applcation owner, as a
				** NULL pointer. Needed for iiimport.
				** 2-aug-93 (daver)
				*/
	    			fetch_sdir(caApplname, Srcdir, 
						&Applid, (char *)NULL);
			}

			IICApfdPatchFormDates(Applid);
			if (cleanflag)
			{
				STlcopy(filename, filebuf, sizeof(filebuf));
				LOfroms(FILENAME, filebuf, &loc);
				LOdelete(&loc);
			}
			if (srcflag || cleansrcflag || listfile != NULL)
				copysrcs(Srcdir, newsrcdir, Applid,
							srcflag, listfile);

			/*
			** if an emerald application has been copied without
			** source directory options, warn them.
			*/
			if (!srcflag && !cleansrcflag && HasMeta)
				IIUGerr (S_CA0025_No_a_or_s, UG_ERR_ERROR, 0);
		}
		else if (rval == CUQUIT)
		{
			errexit(FALSE);
		}
	}
	else
	{ /* must be a copy out to an intermediate file */
		FILE	*fptr;

		if ( SIfopen(&loc, ERx("w"), SI_TXT, 512, &fptr) != OK )
		{
			IIUGerr(E_CA0028_FILECRE, 0, 1, locbuf);
			errexit(FALSE);
		}
		if (IICAcaoCopyApplOut(caApplname, fptr) != OK)
		{
			SIclose(fptr);
			errexit(FALSE);
		}
		SIclose(fptr);
		if (listfile != NULL)
		{
			/*
			** add 4th param, applcation owner, as a
			** NULL pointer. Needed for iiimport.
			** 2-aug-93 (daver)
			*/
	    		fetch_sdir(caApplname, Srcdir, &Applid, (char *)NULL);
			copysrcs(Srcdir, NULL, Applid, FALSE, listfile);
		}
	}
	FEing_exit();
}

static VOID
IICAprompt(prompt,answer)
   char *prompt;
   char *answer;
{
   SIprintf(ERx("%s"),prompt);
   SIgetrec(answer,81,stdin);
   answer[ STlength(answer) - 1 ] = EOS;
}

/*{
** Name:	conproc		- Handle a conflict.
**
** Description:
**	This function is called each time the copy utility detects
**	a conflict when reading in an object.  A conflict is when
**	an object in the copy input file already exists in the database.
**	Using the setting of caPromptflag, caReplaceflag and caQuitflag
**	it returns the appropriate value to the copy utility to cause
**	it to take the appropriate action. The rules for the appropriate
**	action were taken from 5.0 copyapp.  They are:
**
**		print an error about duplicate objects.
**		if object is an application then:
**			if !caReplaceflag print error and quit.
**
**		otherwise, if the quitflag is set, quit.
**		if the replace flag is set replace the object
**		otherwise don't do anything.
**		if the promptflag is set print the action that was taken
**
** Inputs:
**	class		The class of the object with a conflict.
**
**	name		The name of the object with a conflict.
**
** Outputs:
**	Returns:
**		A bit mask which tells the copy utility what
**		to do.
**
** History:
**	8-aug-1987 (Joe)
**		Initial Version
*/
static i4
conproc(class, name)
OOID	class;
char	*name;
{
    char	*classname;
    char	*IICUrtoRectypeToObjname();

    if ((classname = IICUrtoRectypeToObjname(class)) == NULL)
	classname = ERget(F_CA0003_UNKNOWN);
    IIUGerr(DUPOBJNAME, 0, 2, name, classname);
    /*
    ** If its the application, and we are not supposed
    ** to replace, then set the quit flag and print
    ** out the NOCHANGE message.
    */
    if (class == OC_APPL && !caReplaceflag)
    {
	caQuitflag = TRUE;
	IIUGerr(NOCHANGE, 0, 2, name, ERget(F_CA0004_APPLICATION));
    }
    if (caQuitflag)
    {
	return CUQUIT;
    }
    if (caReplaceflag)
    {
	if (!caPromptflag)
	    IIUGerr(REPLACE, 0, 2, name, classname);
	return CUREPLACE;
    }
    if (!caPromptflag)
	IIUGerr(NOCHANGE, 0, 2, name, classname);

    /*
    ** if we don't replace a form, backdate the gendates for any
    ** metaframes which might use it, so we see the form as custom
    ** editted.
    **
    ** This depends on the fact that we do our forms AFTER we've done
    ** our frames.
    */
    if (class == OC_FORM)
    {
	if (Applid < 0)
	{
		/*
		** add 4th param, applcation owner, as a
		** NULL pointer. Needed for iiimport.
		** 2-aug-93 (daver)
		*/
		fetch_sdir(caApplname, Srcdir, &Applid, (char *)NULL);
	}
	IICAbdfBackDateForm(Applid,name);
    }

    return 0;
}

/*{
** Name:	addfunc		-	Function called when an object added.
**
** Description:
**	The copy utility calls this function each time an object is
**	to be added.
**
** Inputs:
**	class		The class of the object.
**
**	name		The name of the object.
**
** Outputs:
**	name		If the object is an application and we
**			are supposed to change its name this
**			will get the new name.
**
** History:
**	8-aug-1987 (Joe)
**		Initial Version
**	10/90 (bobm) check for existence of metaframe types.
**	27-jan-92 (davel)  bug 40231
**		added logic to prevent multiple objects being entered for
**		the same global constant.  COPYAPP output files for releases
**		6.3/03 through 6.4/01 will contain duplicate OC_CONST entries 
**		for global constants that had more than one ii_abfobjects 
**		detail record stored for it (for multi-language constant 
**		values).  These extra OC_CONST entries must be skipped.
*/
static char _LastConstantName[FE_MAXNAME+1] = {EOS};

static STATUS
addfunc(class, name)
OOID	class;
char	*name;
{
    switch (class)
    {
    case OC_APPL:
	if (caNameflag)
	{
	    STcopy(caNewname, name);
	}
	caApplname = STalloc(name);
	*_LastConstantName = EOS;
	break;
    case OC_MUFRAME:
    case OC_BRWFRAME:
    case OC_APPFRAME:
    case OC_UPDFRAME:
	HasMeta = TRUE;
	break;
    case OC_CONST:
	if (name != NULL && *name != EOS && (*_LastConstantName == EOS || 
		(STcompare(name, _LastConstantName) != 0)) )
	{
		STcopy (name, _LastConstantName);
		return OK;
	}
	else
	{
		/* duplicate constant name - must be due to multiple
		** languages defined for a string constant.  By returning FAIL
		** here, cuaddobj() will not try to add a new object.
		*/
		return FAIL;
	}
    }

    return OK;
}
