/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<pc.h>		/* 6-x_PC_80x86 */
# include	<ci.h>
# include	<me.h>
# include	<ex.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<tu.h>
# include	<ug.h>
# include	"ertl.h"

/**
** Name:    tables.c -	Tables Utility Main Line and Entry Point.
**
** Description:
**	This is the stand-alone Tables utility.  It runs through the standard
**	front-end checks, then calls the procedure 'tu_main()' to run the Tables
**	utility.  This enables QBF or ABF to call the Tables utility as a
**	subroutine by also calling the procedure 'tu_main()'.  The 'tu_main()'
**	routine and all other tables utility routines are in the "tblutil"
**	directory, and in the "tblutillib" library.
**
**	Functionally, the code does the following:
**
**	1.  Check to see if site is authorized to use this program.
**	2.  Take out the parameters from the command line.
**	3.  Initialize the testing system.
**	4.  Check the user environment for catastrophic problems.
**	5.  Set up the exception handler stack.
**	6.  Start up the forms system.
**	7.  Print a copyright message.
**	8.  Start up the database manager.
**	9.  Call the main routine, tu_main.
**	10. Close down the forms system, the database, and exception
**	    system, and the testing system.
**	11.  Exit to the operating system.
**
**	This file also defines the set of MKMFIN hints needed to
**	link the program.
**
**	This file defines:
**
**	main()		main line and entry point for the tables utility.
**
** History:
**	Revision 6.0  86/12/14  peter
**	14-dec-1986 (peter)	Written.
**	05-mar-1987 (daver)	Modified for Tables Utility use.
**	30-nov-1987 (peter)	Add calls to QBF and REPORT in std utility.
**	30-nov-1989 (sandyd)	Minor fix to NEEDLIBS and UNDEFS.
**	28-dec-89 (kenl)	Added handling of +c flag.
**	28-aug-1990 (pete)	Added support for -P<password> flag. Also
**				call FEningres rather than FEingres; Dictionary
**				check isn't necessary for tables. Also,
**				initialize argument structure in declaration,
**				rather than in assignment stmts.
**      01-oct-1990 (stevet)    Add call to IIUGinit().
**	22-apr-92 (seg)		EX handlers return STATUS, not EX.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	25-may-95 (emmag)
**	    Conflict with connect within Microsofts VC++
**	15-jun-95 (emmag)
**	    Define main as ii_user_main on NT.
**	24-sep-95 (tutto01)
**	    Restored main, run as console app on NT.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**/

# ifdef DGC_AOS
# define main IITLrsmRingSevenMain
# endif

# ifdef NT_GENERIC
# define connect ii_connect
# endif

/*
**	MKMFIN Hints
PROGRAM =	tables

NEEDLIBS =	TBLUTILLIB \
			UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
			UILIB LIBQSYSLIB LIBQLIB UGLIB LIBQGCALIB CUFLIB \
			FMTLIB AFELIB GCFLIB ADFLIB \
			COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

FUNC_EXTERN	char	*IIUIpassword();

VOID	FEsetdbname();
VOID	FEsetecats();
VOID	tu_main();	/* main for Tables Utility */


/*{
** Name:    main() -	Main Line and Entry Point for Tables Utility.
**
** Description:
**	This is a the Tables Utility main program. This performs
**	the tasks shown above, for the "Tables Utility" front end
**	program.  The calling sequence for 'tables' is:
**
**	    tables dbname [-uuser] [-e]
**		[-Ifile] [-Zfiles] [-Dfile] [-Xflags] [-P<password>]
**
**	    where:
**
**		dbname		is a database name.  If not specified,
**				the user will be prompted.
**		-e		if empty catalogs are to be used.
**		-uuser		alternate username.
**		-I-Z-D		testing flags.
**		-X		equel flags if called with a backend
**				process.
**		-P		Password flag.
**
**	In order to use the FEuta routines to parse the command line,
**	the following entry has been put into the UTEXE.DEF file:
**
**	tables	tables	PROCESS -	dbname [-e] [-uuser]
**		database	"%S"	1	Database		%S
**		emptycat	"-e"	2	Empty catalog		-e
**		user		"-u%S"	3	?			-u%-
**		equel		"-X%S"	4	?			-X%-
**		specflag	"-f%N"	5	?			-f%-
**		test		"-Z%S"
**		dump		"-D%S"
**		keystroke	"-I%S"
**		flags		%B	0	?			%?
**		password	-P	0
**
** Returns:
**	Return to operating system.
**
**	OK if everything ok.
**	status, usually FAIL, if anything goes wrong.
**
** History:
**	15-dec-1986 (peter)	Written.
**	30-nov-1987 (peter)
**		Enable QBF and REPORT.
**	28-aug-1990 (pete)	Added support for -P<password> flag. Also
**				call FEningres rather than FEingres; Dictionary
**				check isn't necessary for tables.
*/

static	char	Gs_Tables[]	= ERx("tables");
static	char	Gs_Password[]	= ERx("password");

static	char	*dbname;	/* database name */
static	char	*dfile;		/* -D parameter */
static	char	*ifile;		/* -I parameter */
static	char	*zfile;		/* -Z parameter */
static	char	*xpipe;		/* Pipe names, with -X flag.
				** NOTE:  These must be set to an empty string
				** or a value from the command line.  It must
				** not be NULL in the call to 'FEingres()'.
				*/
static	char	*uname;		/* Username, with -u flag.
				** NOTE:  These must be set to an empty string
				** or a value from the command line.  It must
				** not be NULL in the call to 'FEingres()'.
				*/
static	bool	emptycats;	/* if "-e" is used */
static	char	*connect;
static	i4	special;	/* special bitmask flags for Tables */

/* Parameter Description */

static ARG_DESC
    args[] = {
	/* Required */
	    {ERx("database"),	DB_CHR_TYPE, FARG_PROMPT, (PTR)&dbname},
	/* Internal optional */
	    {ERx("dump"),	DB_CHR_TYPE, FARG_FAIL,   (PTR)&dfile},
	    {ERx("keystroke"),	DB_CHR_TYPE, FARG_FAIL,	  (PTR)&ifile},
	    {ERx("test"),	DB_CHR_TYPE, FARG_FAIL,	  (PTR)&zfile},
	    {ERx("equel"),	DB_CHR_TYPE, FARG_FAIL,	  (PTR)&xpipe},
	/* Optional */
	    {ERx("user"),	DB_CHR_TYPE, FARG_FAIL,	  (PTR)&uname},
	    {ERx("emptycat"),	DB_BOO_TYPE, FARG_FAIL,	  (PTR)&emptycats},
	    {ERx("connect"),	DB_CHR_TYPE, FARG_FAIL,	  (PTR)&connect},
	/* Undocumented */
	    {ERx("specflag"),	DB_INT_TYPE, FARG_FAIL,	  (PTR)&special},
	    NULL
};

i4
main(argc, argv)
i4	argc;
char	**argv;
{
    EX_CONTEXT	context;	/* Used for stack context */
    ARGRET	one_arg;	/* info on a single argument */
    i4		arg_pos;	/* position of single argument in "args[]" */
    char	*password;	/* -P flag to pass to FEningres() */

    /* External routines */

    STATUS	FEhandler();	/* interrupt handler */
    STATUS	FEforms();	/* turn on forms system */

    /* Start of routine */

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise( ME_INGRES_ALLOC );	/* use ME allocation */

    /* Add call to IIUGinit to initialize character set table and others */

    if ( IIUGinit() != OK)
    {
	PCexit(FAIL);
    }

    ifile = NULL;			/* No -I, -Z, -D files */
    dfile = NULL;
    zfile = NULL;
    xpipe = uname = connect = password = ERx("");
    emptycats = FALSE;
    special = TU_QBF_ENABLED | TU_REPORT_ENABLED;

    /*
    ** The following parses the command line and returns values according
    ** to the specifications given in the entry in the "utexe.def" file.
    **
    ** The command line argument -P is checked for later after FRS is up,
    ** so the password can be prompted for in a popup.
    */
    if (IIUGuapParse(argc, argv, Gs_Tables, args) != OK)
	PCexit(FAIL);

    FEsetdbname(dbname);

    /*
    ** The following call takes the three flags for forms testing and
    ** initializes the testing files, etc.  You should call this routine
    ** whether or not the -I, -D and  -Z flags are specified, in case you
    ** want to check on the status of testing through calls to 'FEtstmake()'
    ** and 'FEtstrun()'.
    */

    if (FEtstbegin(dfile, ifile, zfile) != OK)
    {
	IIUGerr(E_TL0001_TestingError, UG_ERR_FATAL, 0);
    }

    /* Set program specific options */

    FEsetecats(emptycats);

    /*
    ** Check the user's environment.  In this case, we want to check to see that
    ** the ING_TEMP area is writable before continuing.  If it is NOT writable,
    ** we exit now since we cannot recover later.
    */

    if (FEchkenv(FENV_TWRITE) != OK)
    {	/* Check user environment.  */
	IIUGerr(E_TL0002_BadTempDir, UG_ERR_FATAL, 0);
    }

    /*
    ** Set up the exception handling system now that the command line has been
    ** checked.  This sets up the generic 'FEhandler()' exception handler for
    ** the program, which handles all exceptions and will exit gracefully under
    ** most conditions.
    */

    if (EXdeclare(FEhandler, &context) != OK)
    {
	EXdelete();
	IIUGerr(E_TL0003_BadSetup, UG_ERR_FATAL, 0);
    }

    /*
    ** Open up the Forms system.
    */

    if (FEforms() != OK)
    {
	EXdelete();
	PCexit(FAIL);
    }

    /* check if -P specified */
    if (FEutaopen(argc, argv, Gs_Tables) != OK)
	PCexit(FAIL);

    if (FEutaget (Gs_Password, 0, FARG_FAIL, &one_arg, &arg_pos) == OK)
    {
	/* User specified -P. IIUIpassword prompts for password
	** value, if user didn't specify it. Note: If user specified a password
	** attached to -P (e.g. -Pfoobar, which isn't legal), then FEutaget will
	** issue an error message and exit (so this block won't be run).
	*/
	password = IIUIpassword(ERx("-P"));
	password = (password != (char *) NULL) ? password : ERx("");
    }

    FEutaclose();

    /* Write out the copyright messages AFTER forms are on. */

    FEcopyright(ERx("TABLES UTILITY"), ERx("1986"));

    if (*connect != EOS)
    {
	IIUIswc_SetWithClause(connect);
    }

    /*
    ** Connect to the DMBS.
    */

    if (FEningres(NULL, (i4) 0, dbname, xpipe, uname, password,
	(char *)NULL) != OK)
    {
	FEendforms();		/* Close forms */
	EXdelete();
	PCexit(FAIL);
    }

    if (emptycats)
    {
	special |= TU_EMPTYCATS;
    }

    /* Make sure that Quit menu item is available */
    tu_main(dbname, NULL, special|TU_ONLYQUIT_ENABLED, NULL);

    /*
    ** Close down the DBMS, the Forms system,
    ** the exception system, and the testing environment.
    */

    FEing_exit();		/* Close database */

    FEendforms();		/* Close forms */

    EXdelete();			/* Get rid of exception handler */

    FEtstend();			/* Close testing files */

    PCexit(OK);
}
