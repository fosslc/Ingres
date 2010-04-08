/*
** Copyright (c) 1982, 2005 Ingres Corporation
**
*/

#include	<compat.h>
# include       <pc.h>          /* 6-x_PC_80x86 */
#include	<ex.h>
#include	<lo.h>
#include	<st.h>
#include	<si.h>
#include	<me.h>
#include	<er.h>
#include	<nm.h>
#include	<pm.h>
#include	<cv.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<adf.h>
#include	<fdesc.h>
#include	<abfrts.h>
#include	<rtsdata.h>
#include	<erar.h>
#include	<abfcnsts.h>
#include	<ooclass.h>
#include	"eras.h"
/**
** Name:	abrtmain.c - ABF Application Image Main Line Routine.
**
** Description:
**	Main routine of an ABF image.  Depends on the values of certain
**	runtime system variables.  Defines:
**
**	IIARmain()	ABF application image main line.
**
** History:
**	Revision  2.0  4/15/82 (jrc)
**	Initial revision.
**
**	Revision  3.0
**	8/85 (jhw) -- Modified to pass object tables to 'IIARstart()'.
**
**	Revision  5.0
**	10-jul-86 (marian)	bug 8980
**	Changed (*argv)++ to (*argv)-- so the -u flag will be
**	passed correctly.
**
**	Revision  6.0  88/01  wong
**	Extracted main line from "abfrts/abfmain.c".
**	Added support for passing additional user-specified
**	arguments to the DBMS.
**
**	Revision 6.2  89/07  wong
**	Start DBMS connection before starting FRS.  SIR #11248.  Also, test
**	for temporary directory writeability.
**
**	Revision 6.3/03/00  89/12  wong
**	Added support for group, role ID, and user password flags, and
**	backwards compatibility for the run-time structure.
**	03-jan-90 (kenl)
**		Added support for +c flag (WITH clause on database connection).
**
**	Revision 6.3/03/01
**	03/06/91 (emerson)
**		Fix for bug 36315 in IIARmain
**	03/21/91 (emerson)
**		Fixed a bug in my last change to IIARmain.
**
**	Revision 6.4
**	03/22/91 (emerson)
**		Enhancement to IIARmain (-a flag).
**	03/26/91 (emerson)
**		Enhancement to IIARmain (-b flag).
**	03/27/91 (emerson)
**		Change -b flag to -noforms flag (per LRC).
**	03/29/91 (emerson)
**		Changed IIARmain to call iiARsptSetParmText instead of setting 
**		a global variable (for VMS shared segments).
**
**	Revision 6.5
**	06-nov-92 (davel)
**		Added support for -nodatabase and -database= commandline flags.
**	01-dec-92 (essi)
**		Added support for -constants_file flag. It is used to 
**		overwrite the global constants at application's runtime.
**	01-dec-92 (davel)
**		Correct logic for when to use pre-6.5 constant files.
**	10-dec-92 (essi)
**		Added 'break' after checking for DB_DEC_TYPE in case
**		statement.
**	09-feb-92 (essi)
**		Added call to IIUGvfVerifyFile to check for constants_file
**		validity.
**	15-jul-93 (essi)
**		Changed the PM calls with PMm calls. PMm calls provide
**		explicit context.
**      26-aug-93 (huffman)
**              STindex requires 3 parameters, now passing 3.
**	31-aug-1993 (mgw)
**		Cast args to MECOPY_CONST_MACRO() correctly in
**		old_load_constants(). Also recast some MEcopy args for
**		correctness.
**
**	Revision OPING1.1
**	18-may-1995 (harpa06)
**		Cross Integration fix for bug 53689 by angusm - if application
**		is called with -R but no role,  ensure that prompted for ROLE 
**		as well as password.
**      22-may-1995 (wolf) 
**              Modify fix for 53689 to call IIUIroleID instead of IIUIrole.
**		Declare IIUIpassword and IIUIroleID as FUNC_EXTERNs.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      29-Nov-1999 (hanch04)
**          First stage of updating ME calls to use OS memory management.
**          Make sure me.h is included.  Use SIZE_TYPE for lengths.
**	17-Jun-2004 (schka24)
**	    Safe env variable handling.
**	07-sep-2005 (abbjo03)
**	    Replace xxx_FILE constants by SI_xxx.
*/

FUNC_EXTERN	bool		IIARisFrm();
FUNC_EXTERN	bool		IIARisProc();
FUNC_EXTERN	VOID		iiARsptSetParmText();
FUNC_EXTERN	char		*IIUIpassword();
FUNC_EXTERN	char	    	*IIUIroleID();

GLOBALREF	STATUS		IIarStatus;
GLOBALREF	bool		IIarBatch;
GLOBALREF	bool		IIarNoDB;
static		VOID		old_load_constants();
static		VOID		load_constfile();

/*{
** Name:	IIARmain() -	ABF Run-Time Image Main Line.
**
** Description:
**	Parse command-line arguments and initialize ABF run-time system
**	for running an application from an initial frame.
**
** Input:
**	argc	{nat}  The number of command line arguments.
**	argv	{char **}  Reference to array of command-line arguments.
**	abrtobj	{ABRTSOBJ *}  ABF run-time structure.
**		(The following should be removed at some point ...)
**	aret	{nat *}  Reference to 2.0 OSL return value.
**	caret	{nat *}  Reference to 2.0 OSL return value (WSC compatibility.)
**
** History:
**	01/88 (jhw) -- Extracted from "abfrts/abfmain.c".
**	12/89 (jhw) -- Corrected to allow procedure name as part of procedure
**		flag.  JupBug #9077.
**	08/90  (jhw)  Correction for #21516:  Do not start with procedure if
**	default start procedure was overriden with a frame name.
**	03/06/91 (emerson)
**		Fix for bug 36315: If Application Run-Time Structure
**		is from an extract file built by ABF 6.3/02 or earlier,
**		then the procedure table must be rebuilt (since the entries
**		in it got bigger).
**	03/21/91 (emerson)
**		Fixed a bug in my fix for bug 36315:  On call to FEreqmem,
**		get a tag rather using tag 0 (we haven't done a FEbegintag yet;
**		I think it would have worked, but it's not kosher).  Also
**		added a call to IIUGbmaBadMemoryAllocation if FEreqmem failed.
**	03/22/91 (emerson)
**		If "-a " or "-A " parm is found, make iiARptsParmTextString
**		point to a copy of the remaining parms.  This is in support
**		of the new system procedure CommandLineParameters.
**		(Topaz project).
**	03/26/91 (emerson)
**		If "-b " parm is found, mark the application as "batch",
**		and bypass starting the forms system.  Also change
**		the logic that determines where to start the application:
**		if a start name was specified without a preceding -p,
**		search for either a frame or a procedure of that name.
**	03/27/91 (emerson)
**		Change -b flag to -noforms flag (per LRC).
**		Also add -forms flag (also per LRC).
**	03/29/91 (emerson)
**		Changed to call iiARsptSetParmText instead of setting 
**		a global variable (for VMS shared segments).
**	5-aug-91 (blaise)
**		Added error handling for when IIUIroleID returns NULL; this
**		happens when the user attempts to use the now-disallowed
**		syntax "-R<role_id>/<passwd>". Bug #36540.
*/

/*}
** Name:	ABRTS_V1 -	Version 1 Application Run-Time Structure.
**
** Description:
**	Version 1 of the ABF application run-time structure, ABRTSOBJ, defined
**	in <abfrts.h>
**
** History:
**	03/90 (jhw) -- Extracted from 6.3 <abfrts.h>.
*/
typedef struct {
	i4		abroversion;	/* A version number */
	char		*abrodbname;	/* The database name */
	char		*abrosframe;	/* The default starting frame */
	i2		abdml;		/* The application DML */
	i1		start_proc;	/* Whether to start with a procedure */
	i1		batch;		/* Whether application is batch */
	i4		abrohcnt;	/* Number of element in hash table */
	i4		*abrofhash;	/* The frame hash table */
	i4		*abrophash;	/* The procedure hash table */
	i4		abrofcnt;	/* The number of frames in abroftab. */
	i4		abropcnt;	/* The number of procs in abroptab. */
	i4		abrofocnt;	/* The number of forms in abrofotab. */
	ABRTSFRM	*abroftab;	/* The frame table */
	ABRTSPRO	*abroptab;	/* The procedure table */
	ABRTSFO		*abrofotab;	/* The form table */
} ABRTS_V1;

/*}
** Name:	ABRTSPRO_V1 -	Version 1 Application Procedure Table Structure.
**
** Description:
**	Version 1 of the ABF application Procedure Table structure, ABRTSPRO, 
**	defined in <abfrts.h>
**
** History:
**	03/06/91 (emerson)
**		Extracted from 6.3/02 <abfrts.h>.
**      18-may-1995 (harpa06)
**              Cross Integration fix for bug 53689 by angusm - if application
**              is called with -R but no role,  ensure that prompted for ROLE
**              as well as password.
**      22-may-1995 (wolf) 
**              Modify fix for 53689 to call IIUIroleID instead of IIUIrole.
*/
typedef struct
{
	char    *abrpname;
	i4     abrplang;
	DB_DATA_VALUE   abrprettype;
	VOID    (*abrpfunc)();
	i4      abrpfid;
	i4     abrpnext;
} ABRTSPRO_V1;

static	DB_TEXT_STRING	NullTextString = {0, ERx("")};

VOID
IIARmain ( argc, argv, abrtobj, aret, caret )
i4		argc;
char		**argv;
ABRTSOBJ	*abrtobj;
i4		*aret,		/* 2.0 OSL compatibility */
		*caret;		/* 2.0 WSC OSL compatibility */
{
	char		*appl;
	char		*frame = (char *)NULL;
	char		*user = ERx("");
	char		*groupID = ERx("");
	char		*password = ERx("");
	char		*roleID = ERx("");
	char		*xflag = ERx("");
	char		*dbargs[10];
	i4		ndbargs = 0;
	bool		proc = FALSE;
	bool		dbmsflag = FALSE;
        bool            roleflag = FALSE;
	char		*cfn = NULL;
	char		*dbname = abrtobj->abrodbname;
	ABRTSOBJ	ver1;		/* compatibility run-time structure */
	EX_CONTEXT	abcontext;

	/*
	** By default, set TextString to point an empty text string.
	** If we find an -a or -A flag, we'll set TextString to
	** point to a copy of the remaining command-line parms (if any).
	*/
	DB_TEXT_STRING	*TextString = &NullTextString;

	char	*dump = (char *)NULL;
	char	*debug = (char *)NULL;
	char	*test = (char *)NULL;

	STATUS	FEchkname();
	EX	IIARexchand();
	DB_LANG	IIAFdsDmlSet();

	/*
	** Tag for memory that will persist until the application terminates.
	*/
	TAGID	basetag = FEgettag();

	IIarBatch = abrtobj->batch;

	/*
	** Convert old version run-time structure for backwards compatibility.
	** Also convert the procedure table (if any), since the entries in it
	** got bigger.  Note that fields that didn't exist in the old versions
	** are set to binary zeroes, except that abrpdml in ABRTSPRO
	** is copied from abdml in ABRTSOBJ.
	*/
	if ( abrtobj->abroversion < ABRT_VERSION )
	{
		ABRTSPRO_V1 	*optab;
		ABRTSPRO 	*nptab;
		i4		pcnt;
		i4		dml;
		
		MEfill((u_i2)sizeof(ABRTSOBJ), '\0', (char *)&ver1);
		MEcopy((PTR)abrtobj, (u_i2)sizeof(ABRTS_V1), (PTR)&ver1);
		abrtobj = &ver1;
		pcnt = abrtobj->abropcnt;
		if (pcnt > 0)
		{
			dml = abrtobj->abdml;
			optab = (ABRTSPRO_V1 *) abrtobj->abroptab;
			nptab = (ABRTSPRO *) FEreqmem( basetag,
						sizeof(ABRTSPRO) * pcnt,
						TRUE, (STATUS*)NULL );
			if (nptab == (ABRTSPRO *)NULL)
			{
				IIUGbmaBadMemoryAllocation( ERx("IIARmain") );
			}
			abrtobj->abroptab = nptab;
			for ( ; pcnt > 0; pcnt--, optab++, nptab++ )
			{
				MEcopy( (PTR)optab, (u_i2)sizeof(ABRTSPRO_V1),
					(PTR)nptab );
				nptab->abrpdml = dml;
			}
		}
	}

	/* Parse command line arguments */

	appl = argv[0];
	while (--argc > 0)
	{
		if ( **++argv == '+' )
		{
			if ( *(*argv+1) == 'c' )
			{
				if ( *(*argv + 2) != EOS )
					IIARswc_SetWithClause(*argv + 2);
			}
			else
			{
				dbmsflag = TRUE;
			}
		}
		else if ( **argv == '-' )
		{ /* flags */
			switch (*(*argv + sizeof(ERx("-")) - 1))
			{
			  case 'd':
			    {
				char *cp;

				cp = STindex( *argv, ERx("="),0);
				if (cp != NULL)
				{
				    *cp++ = EOS;
				    if ( STequal( *argv, ERx("-database") ) )
				    {
					abrtobj->abrodbname = cp;
				    }
				    else
				    {
					dbmsflag = TRUE;
				    }
				}
				else
				{
					abrtobj->abrodbname = *argv
						+ (sizeof(ERx("-d")) - 1);
				}
				break;
			    }

			  case 'c':
			    {
				char *cp;
				cfn = NULL;
				cp = STindex( *argv, ERx("="),0);
				if (cp != NULL)
				{
				    *cp++ = EOS;
				    if (STequal( *argv,ERx("-constants_file")))
					cfn = cp;
				}
				else
					dbmsflag = TRUE;
				break;
			    }
			  case 'f':
				if ( STequal( *argv, ERx("-forms") ) )
				{
					IIarBatch = FALSE;
				}
				else
				{
					dbmsflag = TRUE;
				}
				break;

			  case 'n':
				if ( STequal( *argv, ERx("-noforms") ) )
				{
					IIarBatch = TRUE;
				}
				else if ( STequal( *argv, ERx("-nodatabase") ) )
				{
					IIarNoDB = TRUE;
				}
				else
				{
					dbmsflag = TRUE;
				}
				break;

			  case 'p':
				proc = TRUE;
				if ( *(*argv + 2) != EOS )
					frame = *argv + 2;
				break;

			  case 'u':
				user = *argv;
				break;

			  case 'G':
				groupID = *argv;
				break;

			  case 'R':
				if ( abrtobj->abroleid != NULL
						&& *abrtobj->abroleid != EOS )
				{
					IIUGerr( E_AR0026_RoleOverride,
						 UG_ERR_FATAL, 0
					);
				}
				else
				{
                                        roleflag = TRUE;
                                        if ( *(*argv + 2) != EOS )
                                                roleID = *argv;
				}
				break;

			  case 'P':
				password = *argv;
				break;

			  case 'X':
				xflag = *argv;
				break;

			  case 'D':
				debug = *argv + (sizeof(ERx("-D")) - 1);
				break;

			  case 'I':
				dump = *argv + (sizeof(ERx("-I")) - 1);
				break;

			  case 'Z':
				test = *argv + (sizeof(ERx("-Z")) - 1);
				break;

			  case 'A':
			  case 'a':
				if ((*(*argv + 2)) != EOS)
				{
					/*
					** If -a or -A not followed by space
					** (in command line),
					** pass the whole parm to the DBMS.
					*/
					dbmsflag = TRUE;
				}
				else if (argc > 1)
				{
					/*
					** If -a or -A followed by space and
					** further parms, make TextString point
					** to a copy of the remaining parms.
					*/
					char		**v;
					i4		c, size;
					char		*p;

					size = -1;
					for ( c = argc - 1, v = argv + 1; c > 0;
						c--, v++ )
					{
						size += STlength( *v ) + 1;
					}
					TextString =
					    (DB_TEXT_STRING *)FEreqmem(
						basetag,
						size + sizeof(DB_TEXT_STRING) -
						sizeof(TextString->db_t_text),
						(bool)FALSE, (STATUS *)NULL );

					if (TextString ==
						(DB_TEXT_STRING *)NULL)
					{
						IIUGbmaBadMemoryAllocation(
							ERx("IIARmain") );
					}
					TextString->db_t_count = size;
					p = (char *)TextString->db_t_text;

					c = argc - 1, v = argv + 1;
					for ( ;; )
					{
						size = STlength( *v );
						MEcopy( (PTR)*v, (u_i2)size,
						        (PTR)p );
						p += size;
						c--, v++;
						if ( c <= 0 ) break;
						*p++ = ' ';
					}
					argc = 1;  /* force exit from loop */
				}
				break;

			  default:
				dbmsflag = TRUE;
				break;
			}
		}
		else
		{
			if (frame == (char *)NULL || *frame == EOS)
				frame = *argv;
			else
				IIUGerr( E_AS0002_ExtraFrame, UG_ERR_ERROR,
						1, (PTR)*argv
				);
		}
		if (dbmsflag)
		{
			dbmsflag = FALSE;
			if (ndbargs < (sizeof(dbargs)/sizeof(dbargs[0]) - 1))
			{
				dbargs[ndbargs++] = *argv;
			}
			else
			{
				i4	num = ndbargs;
	
				IIUGerr( E_AS0001_TooManyArgs,
					UG_ERR_ERROR, 1, (PTR)&num
				);
			}
		}
	} /* end while */
	dbargs[ndbargs] = (char *)NULL;

	/*
	** Provide command-line parameters for IIARclpCmdLineParms
	*/
	iiARsptSetParmText( TextString, (DB_TEXT_STRING *(*)())NULL );

	/*
	** BUG 4409
	**
	** Must check here if first frame has been named, since there
	** may have been flags on command line that increased argc.
	*/
	if (frame == (char *)NULL || *frame == EOS)
	{
		if ((frame = abrtobj->abrosframe)[0] == EOS)
		{ /* Requires a frame/procedure name to run */
			IIUGerr(E_AS0003_FrameRequired, UG_ERR_FATAL, 0);
		}
	}

	if ( FEchkenv(FENV_TWRITE) != OK )
	{ /* Cannot write temporary directory */
		IIUGerr( E_AR0052, UG_ERR_FATAL, 0 );
	}

	/*
	** If command line flag `-constants_file' is specified then 
	** use it, otherwise do it the old way for backward compatability.
	*/
	
	if (cfn != NULL)
	{
		_VOID_ IIUGvfVerifyFile( cfn, UG_IsExistingFile, FALSE, 
			TRUE, UG_ERR_FATAL );
		load_constfile(abrtobj,cfn);
	}
	else
	{

		/*
 		** For applications that were imaged in 6.3/03/00 through
 		** 6.4/0x/xx, alternate sets of application constants
 	 	** are automatically created at image time into files
 		** named (e.g. for german):
 		**	$ING_ABFDIR/dbname/german/appname.con
 		** This case is identifed by all of the following:
 		**	- The run-time structure version is 3 or greater
 		**	- The abcndate member of the run-time structure != NULL
 		**	- The language member of the run-time structure > 0
 		** This method of constant value override depends on the
 		** value of logical II_APPLICATION_LANGUAGE and/or II_LANGUAGE,
 		** and was obsolesced beginning in version 6.5/00.
 		**
		*/
		if (abrtobj->abcndate != NULL && abrtobj->abroversion >= 3 &&
			abrtobj->language > 0)
		{
			old_load_constants(abrtobj, dbname);
		}
	}

	/*
	** The code inside the declare is the normal case.
	*/
	if ( EXdeclare(IIARexchand, &abcontext) == OK )
	{
		_VOID_ IIAFdsDmlSet(abrtobj->abdml);	/* set run-time DML */
                if ( *password != EOS || roleflag == TRUE)
		{ /* prompt for password */
			if ( ! IIarBatch )
			{ /* FRS setup */
				IIARidbgInit(appl, dump, debug, test);
				IIARfrsInit( TRUE );
			}
			if ( *password != EOS )
				password = IIUIpassword(password);
                        if ( roleflag == TRUE)
                        {
                                /*
                                ** Bug 53689
                                ** -R flag on command line but no role
                                */
 
                                if (*roleID == EOS)
                                        roleID = IIUIroleID(roleID);

				/*
				** IIUIroleID() returns NULL if the user
				** attempts to use the "-R<roleid>/<passwd>"
				** syntax. Give an error and exit.
				*/
				if ((roleID = IIUIroleID(roleID)) == NULL)
				{
					IIUGerr(E_AS0009_BadArgument, 0, 1,
									*argv);
					IIUGerr(E_AS0010_R_FlagSyntax,
							UG_ERR_FATAL, 0);
				}
			}
		}
		if ( abrtobj->abroleid != NULL && *abrtobj->abroleid != EOS )
			roleID = abrtobj->abroleid;
		if (!IIarNoDB)
		{
			IIARdbConnect( abrtobj->abrodbname, 
				user, xflag, groupID, roleID, password,
				dbargs[0], dbargs[1], dbargs[2], dbargs[3],
				dbargs[4], dbargs[5], dbargs[6], dbargs[7],
				dbargs[8], dbargs[9], (char *)NULL
			);
		}
		if ( ! IIarBatch && *password == EOS
			&& ( roleID == abrtobj->abroleid || *roleID == EOS ) )
		{ /* FRS setup */
			IIARidbgInit(appl, dump, debug, test);
			IIARfrsInit( TRUE );
		}
		IIARstart(abrtobj);
		iiarOldReturns(aret, caret);	/* remove at some point ... */

		/*
		** Try to start the application.  
		**
		** If a starting "object" was specified without a preceding -p,
		** we'll assume it can be either a frame or a procedure.
		** We search for frames before procedures.  (This search order
		** is an issue only for images created under old releases
		** where frames and procedures were in different name spaces).
		** Note that it's an error if the starting object is a frame 
		** but the user requested "batch" mode (-noforms).
		**
		** If -p was specified or if a default start procedure was
		** specified without having been overriden by specification
		** of a frame on the command-line, we search only for a
		** procedure.
		*/
		if ( proc ||
		     ( abrtobj->start_proc && frame == abrtobj->abrosframe ) )
		{
			if ( IIARisProc( frame ) )
			{
				IIARprocCall( frame, (ABRTSPRM *)NULL );
			}
			else
			{
				IIUGerr( E_AR0081_StartProcNotFound,
					 UG_ERR_FATAL, 1, frame );
			}
		}
		else	/* starting object could be frame or procedure */
		{
			if ( IIARisFrm( frame ) )
			{
				if ( IIarBatch )
				{
					IIUGerr( E_AR0082_BatchStartFrame,
						 UG_ERR_FATAL, 1, frame );
				}
				else
				{
					IIARfrmCall( frame, (ABRTSPRM *)NULL );
				}
			}
			else if ( IIARisProc( frame ) )
			{
				IIARprocCall( frame, (ABRTSPRM *)NULL );
			}
			else
			{
				IIUGerr( E_AR0080_StartObjNotFound,
					 UG_ERR_FATAL, 1, frame );
			}
		}

		abexcexit(OK);
	}
	/* Abnormal exit */
	if ( ! IIarBatch )
		IIARcdbgClose();
	EXdelete();
	PCexit( IIarStatus );
}

/*
** Load application constants from file for pre-6.5 imaged applications.
** No status is returned, since all errors are fatal.
*/
static VOID
old_load_constants(abrtobj, dbname)
ABRTSOBJ *abrtobj;
char	*dbname;
{
	STATUS		status;
	char 		*ing_abfdir;
	LOCATION 	loc;
	char		locbuf[MAX_LOC+1];
	FILE		*filep;
	LOINFORMATION loinfo;
	i4		flags;
	char 		filename[LO_NM_LEN+1];
	i4		dummy;
	ABRTSGL		*gp;
	i4		gcnt;
	char		*membuf;
	char		*cp;

 	char *language;
 	i4  code;
 
 	/* Check the language. */
 	NMgtAt(ERx("II_APPLICATION_LANGUAGE"), &language);
 	if (language == NULL || *language == EOS)
 		NMgtAt(ERx("II_LANGUAGE"), &language);
 
 	if (language == NULL || *language == EOS)
 		return;
 
 	language = STalloc(language);
 	if (ERlangcode(language, &code) != OK)
 		IIUGerr(E_AS0005_Bad_language, UG_ERR_FATAL, 1, language);
 
 	if (code == abrtobj->language)
 		return;
 
	/* 
	** First, be sure ING_ABFDIR is defined, since it's the root of our
	** directory tree.
	*/	
	NMgtAt(ERx("ING_ABFDIR"), &ing_abfdir);
	if (ing_abfdir == NULL || *ing_abfdir == EOS)
		IIUGerr(E_AS0004_No_ING_ABFDIR, UG_ERR_FATAL, 0);

	/* 
	** Now construct the file name.  ING_ABFDIR!dbname!language!appl.con
	*/
	STlcopy(ing_abfdir, locbuf, sizeof(locbuf)-1);
	status = LOfroms(PATH, locbuf, &loc);
	if (status == OK)
		status = LOfaddpath(&loc, dbname, &loc);
	if (status == OK)
		status = LOfaddpath(&loc, language, &loc);
	if (status == OK)
	{
		_VOID_ STprintf(filename, ABCONFILE, abrtobj->abroappl);
		status = LOfstfile(filename, &loc);
	}

	if (status != OK)
		IIUGerr(E_AS0006_Cant_Open_Constants, UG_ERR_FATAL, 0);

	flags = LO_I_SIZE;
	status = LOinfo(&loc, &flags, &loinfo);
	if ((flags & LO_I_SIZE) == 0)
		status = FAIL;

	/* Let's open the file */
	if (status == OK)
	{
		membuf = MEreqmem(0, (SIZE_TYPE)(loinfo.li_size + 1), 
				  FALSE, (STATUS *)NULL);
		if (membuf == NULL)
			EXsignal(EXFEMEM, 1, ERx("IIARmain"));

		status = SIfopen(&loc, "r", SI_VAR, 0, &filep);
	}	
	if (status == OK)
	{
		status = SIread(filep, loinfo.li_size, &dummy, membuf);
		_VOID_ SIclose(filep);
	}

	if (status != OK)
	{
		char *fn;
		_VOID_ LOtos(&loc, &fn);
		IIUGerr(E_AS0007_Cant_Read_Constants, UG_ERR_FATAL, 1, fn);
	}

	/* We've got the data from the file.  Let's chech the datestamp */
	membuf[loinfo.li_size] = EOS;
	cp = STindex(membuf, ERx("\n"), 0);
	if (cp != NULL)
		*cp++ = EOS;
	if (cp == NULL || !STequal(abrtobj->abcndate, membuf))
	{
		char *fn;

		_VOID_ LOtos(&loc, &fn);
		IIUGerr(E_AS0008_Wrong_File_Version, UG_ERR_FATAL, 1, fn);
	}

	/* It checks.  Now load the constants. */
	for (gp = abrtobj->abrogltab, gcnt = 0; 
	     gcnt < abrtobj->abroglcnt;
	     gp++, gcnt++
	    )
	{
		if (gp->abrgtype == OC_CONST)
		{
			DB_DATA_VALUE *dbdv = &gp->abrgdattype;
			bool varying;
			u_i2 length;

			switch (dbdv->db_datatype)
			{ 
			case DB_VCH_TYPE:
			case DB_TXT_TYPE:
				varying = TRUE;
				break;

			case DB_CHR_TYPE:
			case DB_CHA_TYPE:
				varying = FALSE;
				break;

			default:
				/* Not a string type */
				continue;
			}
		
			MECOPY_CONST_MACRO((PTR) cp, (u_i2) sizeof(u_i2),
			                   (PTR) &length);
			if (varying)
			{
				/* A varchar */
				dbdv->db_length = length + 2;
				dbdv->db_data = (PTR)cp;
			}
			else
			{
				/* A char */
				dbdv->db_length = length;
				dbdv->db_data = (PTR)(cp + 2);
			}
			cp += length + 3;
		}
	}
}


/*
** Load constants from a user specified file.  All errors are considered
** fatal.
*/

static VOID
load_constfile( ABRTSOBJ *abrtobj, char *cons_fn )
{
	STATUS		status;
	LOCATION 	loc;
	char		locbuf[MAX_LOC+1];
	i4		flags;
	ABRTSGL		*gp;
	i4		gcnt;
	char		*cp;
	char		*gcvp;
	PTR		data;
	PM_CONTEXT	*cntx;

	/*
	** Initialize and load the user specified constant file.
	*/

	STcopy(cons_fn, locbuf);
	status = LOfroms( PATH & FILENAME, locbuf, &loc );
	if (status != OK)
	{
		IIUGerr(E_AS000B_NameToLoc,UG_ERR_FATAL, 0);
	}
	PMmInit( &cntx );
	if ( PMmLoad( cntx, &loc,(PM_ERR_FUNC *) NULL)  != OK )
	{
		IIUGerr(E_AS000A_BadConstantFile,UG_ERR_FATAL, 1, cons_fn);	
	}

	for (gp = abrtobj->abrogltab, gcnt = 0; 
		gcnt < abrtobj->abroglcnt;
		gp++, gcnt++
	    )
	{
	    if (gp->abrgtype == OC_CONST)
	    {
    		DB_DATA_VALUE *dbdv = &gp->abrgdattype;
    		i4 length;
    
    		if ( PMmGet(cntx, gp->abrgname, &gcvp) != OK )
		{
			continue;
		}
    		switch (dbdv->db_datatype)
    		{ 
    			case DB_VCH_TYPE: 
    			case DB_TXT_TYPE:
    			{
				register char  *bp;
				DB_TEXT_STRING *txt;
				if ( ( bp = MEreqmem
					      ( 0, 
					        dbdv->db_length+1, 
					        TRUE, (STATUS *)NULL
					      )
					 ) == NULL 
				       )
				 {
		    		 	IIUGerr(E_AS000D_AllocFail, 
						UG_ERR_FATAL,1 , gp->abrgname);	
				 }
				 dbdv->db_data = bp;
				 txt = (DB_TEXT_STRING *)dbdv->db_data;
				 length = STlength( gcvp );
				 if (length > 
					(dbdv->db_length - DB_CNTSIZE))
				 {
					length = dbdv->db_length - DB_CNTSIZE;
    				 	*(gcvp + length) = EOS;
				 }
				 txt->db_t_count = length;
				 MEcopy( (PTR)gcvp,
					(u_i2)(txt->db_t_count + 1), /*EOS*/
					(PTR)txt->db_t_text);
				 break;
    			}
    			case DB_CHR_TYPE:
    			case DB_CHA_TYPE:
    			{
				register char  *bp;
				if ( ( bp = MEreqmem
					      ( 0, 
					        dbdv->db_length+1, 
					        TRUE, (STATUS *)NULL
					      )
			              ) == NULL 
				    )
				{
		    		        IIUGerr(E_AS000D_AllocFail, 
						UG_ERR_FATAL,1, gp->abrgname);	
				}
				dbdv->db_data = bp;
				length = STlength(gcvp);
				if (length > dbdv->db_length)
				{
					length = dbdv->db_length;
					*(gcvp + length) = EOS;
				}
				STmove( gcvp,' ', dbdv->db_length, 
					dbdv->db_data );
				*(dbdv->db_data + dbdv->db_length) = EOS;
				break;
    			}
    			case DB_INT_TYPE:
    			{
               			i4 value;
    				status = CVal(gcvp, &value);
    				if (status != OK)
    				{
    					IIUGerr(E_AS000E_IntConvFailed,
    						UG_ERR_FATAL, 1, gp->abrgname);
    				}
    				if ( dbdv->db_length == sizeof(i1) )
    				{
    					*((i1 *)dbdv->db_data) = value;
    				}
    				else if ( dbdv->db_length == sizeof(i2) )
    				{
    					*((i2 *)dbdv->db_data) = value;
    				}
    				else /* ( dbdv->db_length == sizeof(i4) ) */
    				{
    					*((i4 *)dbdv->db_data) = value;
    				}
    				break;
    			}
    			case DB_FLT_TYPE:
    			{
    				f8      float_value;
    				status = CVaf(gcvp, '.', &float_value);
    				if (status != OK)
    				{
    					IIUGerr(E_AS000F_FloatConvFailed,
    						UG_ERR_FATAL, 1, gp->abrgname);
    				}
    				if ( dbdv->db_length == sizeof(f4) )
    				{
    					*((f4 *)dbdv->db_data) = float_value;
    				}
    				else /* ( dbdv->db_length == sizeof(f8) ) */
    				{
    					*((f8 *)dbdv->db_data) = float_value;
    				}
    				break;
    			}
    			case DB_DEC_TYPE:
    			{
    				i4 prec = DB_P_DECODE_MACRO(dbdv->db_prec);
    				i4 scale = DB_S_DECODE_MACRO(dbdv->db_prec);
    				status = CVapk(gcvp,'.',prec,scale,
    					(char *)dbdv->db_data);
    				if (status == CV_TRUNCATE ||
    					status == OK)
    				{
    					;
    				}
    				else
    				{
    					IIUGerr(E_AS0011_PackConvFailed,
    						UG_ERR_FATAL, 1, gp->abrgname);
    				}
				break;
    			}
    			default:
    				/* Unsupported datatypes are ignored */
    				continue;
    		}
	    }
	}
	PMmFree( cntx );
}
