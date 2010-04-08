/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>		/* 6-x_PC_80x86 */
#include	<ci.h>
#include	<me.h>
#include	<ex.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	"erim.h"
#include	"imconst.h"
#include	"imextern.h"

/**
** Name:	main.c -	Main Routine and Entry Point for IngMenu.
**
** Description:
**	This is a modified version of Peter's Generic main FE program.
**	It runs through the standard frontend checks, then calls the procedure
**	naive() to actually run Ingmenu.
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
**	9.  Call the main routine, naive.
**	10. Close down the forms system, the database, and exception
**	    system, and the testing system.
**	11.  Exit to the operating system.
**
**	This file also defines the set of MKMFIN hints needed to
**	link the program.
**
**	The main program itself calls the underlying imu_main
**	routine to do the actual setup of the front end program,
**	so that 'return' values from imu_main can call the PCexit
**	routine.
**
**	This file defines:
**
**	main		The main program.
**	imu_main	The FE program template.
**
** History:
**	14-dec-1986 (peter)	Written.
**	18-mar-1987 (daver)	Modified for Ingmenu use.
**
**	Revision 6.1  88/09/08  dave
**	Added code to support saving keystroke files in subprocess frontends.
**
**	Revision 6.2  89/03  wong
**	Made IngMenu function 'IIIMnaive()' a callable interface, and
**	modified the main routine to call it.  Also, changed to call
**	'IIUGuapParse()' for arguments.
**
**	30-nov-89 (sandyd)
**		Added MEadvise() call, MALLOCLIB, and II_copyright undef.  
**		Also replaced obsolete COMPAT with COMPATLIB.
**		(89/10 wong also Modified to use INGRES allocator. JupBug 8403.)
**      29-aug-1990 (pete)
**		Added support for -P<password> flag. Also
**              call FEningres rather than FEingres; Dictionary
**              check isn't necessary for ingmenu. Also,
**              initialize argument structure in declaration,
**              rather than in assignment stmts.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**	25-may-95 (emmag)
**	    Conflict with Microsoft's definition of connect.
**      26-Jun-95 (fanra01)
**              Redirect program entry to initialise MS Windows first.
**	28-sep-95 (emmag)
**	    Use main.
**	15-jul-1998 (ocoro02)
**	    Bugs 76262/39702 add support for -G flag.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
*/


/*{
** Name:	main() -	Main Entry Point for IngMenu.
**
** Description:
**	This is a the Ingmenu main program. This performs
**	the tasks shown above, for the "Ingmenu" front end
**	program.  The calling sequence for 'ingmenu' is:
**
**	    ingmenu dbname [-uuser] [-e]
**		[-Ifile] [-Zfiles] [-Dfile] [-Xflags] [-P]
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
**
**	In order to use the FEuta routines to parse the command line,
**	the following entry has been put into the UTEXE.DEF file:
**
**	ingmenu ingmenu PROCESS -	dbname [-e] [-uuser] [-P]
**		database	"%S"	1	Database		%S
**		emptycat	"-e"	2	Empty catalog		-e
**		user		"-u%S"	3	?			-u%-
**		equel		"-X%S"	4	?			-X%-
**		password	-P	0
**		test		"-Z%S"
**		dump		"-D%S"
**		keystroke	"-I%S"
**		flags		%B	0	?			%?
**		connect		+c%S
**		groupid		-G%S
**
**	Returns:
**		Return to operating system.
**
**		OK if everything ok.
**		status, usually FAIL, if anything goes wrong.
**
** Side Effects:
**	none.
**
** History:
**	15-dec-1986 (peter)	Written.
**	18-mar-1987 (daver)	Modified for Ingmenu use.
**	15-oct-1989 (jhw)  Modified to use INGRES allocator.  (JupBug #8403.)
**      11-sep-1990 (stevet) Add call to IIUGinit.
*/

# ifdef NT_GENERIC
# define connect ii_connect
# endif

# ifdef DGC_AOS
# define main IIIMrsmRingSevenMain
# endif

/*
**	MKMFIN Hints
PROGRAM =	ingmenu

NEEDLIBS =	INGMENULIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB LIBQGCALIB CUFLIB FMTLIB \
		AFELIB GCFLIB ADFLIB COMPATLIB MALLOCLIB

UNDEFS =	II_copyright
*/

static		STATUS	imu_main();

FUNC_EXTERN	VOID IIIMnaive();
FUNC_EXTERN     char    *IIUIpassword();

i4
main(argc, argv)
i4	argc;
char	**argv;
{
	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the INGRES memory allocator */
	MEadvise( ME_INGRES_ALLOC );

        /* Add call to IIUGinit to initialize character set table and others */

	if ( IIUGinit() != OK )
	{ /* Prints an error if not allowed. */
		return FAIL;
	}

	/* simply call routine below for work */

	PCexit(imu_main(argc, argv));

	/*NOTREACHED*/
}



/*{
** Name:	imu_main() -	Main Routine for Program IngMenu.
**
** Description:
**	Main routine for 'ingmenu.'  See comments from above.
**
** Inputs:
**	argc		number of arguments from command line.
**	argv		argument vector, from command line.
**
** Outputs:
** Returns:
**	{STATUS}  OK	if everthing fine.
**		  FAIL	if anything wrong.
**
** History:
**	15-dec-1986 (peter)	Written.
**	09/08/88 (dkh) - Added code to support saving keystroke files
**			 in subprocess frontends.
**	03/88 (jhw) Made IngMenu function 'IIIMnaive()' a callable interface,
**		and modified this routine to call it.  Also, changed to call
**		'IIUGuapParse()' for arguments.
**	02-jan-90 (kenl)
**		Added support for the +c flag (WITH clause on DB connection).
**      29-aug-1990 (pete)
**		Added support for -P<password> flag. Also
**              call FEningres rather than FEingres; Dictionary
**              check isn't necessary for ingmenu. Also,
**              initialize argument structure in declaration,
**              rather than in assignment stmts.
*/

static	char	Gs_Ingmenu[]	= ERx("ingmenu");
static  char    Gs_Password[]   = ERx("password");
static  char	Gs_EOS[]	= ERx("");

static	char	*dbname = NULL;		/* database name */
static	bool	expert = FALSE;		/* -e flag */
static	char	*user = Gs_EOS;		/* -uusername flag */
static	char	*xpipe = Gs_EOS;	/* -X flag */

static	char	*test = NULL;
static	char	*dump = NULL;
static	char	*input = NULL;
static	char	*connect = Gs_EOS;
static	char	*groupid = NULL;

/* Parameter Description */

static ARG_DESC
	args[] = {
	/* Required */
	    {ERx("database"),	DB_CHR_TYPE,	FARG_PROMPT,	(PTR)&dbname},
	/* Optional */
	    {ERx("emptycat"),	DB_BOO_TYPE,	FARG_FAIL,	(PTR)&expert},
	    {ERx("user"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&user},
	/* Internal optional */
	    {ERx("equel"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&xpipe},
	    {ERx("test"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&test},
	    {ERx("dump"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&dump},
	    {ERx("keystroke"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&input},
	    {ERx("connect"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&connect},
	    {ERx("groupid"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&groupid},
	    NULL
};

static STATUS
imu_main ( argc, argv )
i4	argc;
char	**argv;
{
	ARGRET	one_arg;		/* info on a single argument */
	i4	arg_pos;		/* position of single arg in "args[]"*/
	char	*password = Gs_EOS;	/* -P flag to pass to FEningres() */
	EX_CONTEXT	context;	/* Used for stack context */

	/* External routines */

	EX	FEhandler();	/* interrupt handler */
	STATUS	FEforms();	/* turn on forms system */

	/*
	** The following parses the command line and returns values according
	** to the specifications given in the entry in the "utexe.def" file.
	**
	** The command line argument -P is checked for later after FRS is up,
	** so the password can be prompted for in a popup.
	*/
	if (IIUGuapParse(argc, argv, Gs_Ingmenu, args) != OK)
		return FAIL;

	/*
	** The following call takes the three flags for forms
	** testing and initializes the testing files, etc.
	** You should call this routine whether or not the
	** -I, -D and  -Z flags are specified, in case you
	** want to check on the status of testing through
	** calls to FEtstmake and FEtstrun.
	*/

	if ( FEtstbegin(dump, input, test) != OK )
	{
		IIUGerr(E_IM0005_Testing_could_not_beg, UG_ERR_FATAL, 0);
		return FAIL;
	}

	/*
	** Check the user's environment, if needed.  In this case,
	** we want to check to see that the ING_TEMP area is
	** writable before going on.  If it is NOT writable,
	** programs such as VIFRED and QBF which use it for
	** temporary storage discover it too late to recover
	** easily.
	*/

	if ( FEchkenv(FENV_TWRITE) != OK )
	{ /* Check user environment.  */
		IIUGerr(E_IM0006_User_environment_impr, UG_ERR_FATAL, 0);
		/*NOT REACHED*/
	}

	/*
	** Set up the exception handling system now that the
	** command line has been checked.   This sets up the
	** generic FEhandler exception handler for the program,
	** which handles all exceptions and will exit gracefully
	** in most conditions.
	**
	** Subsequent calls can be made to FEset_exit and
	** FEclr_exit to add and delete additional exit
	** handlers, which are critically needed for specific
	** cleanup tasks, for a program.  This standard template
	** will cleanup common things (such as turning off the
	** forms system, closing files, etc.) without any
	** special exit handlers.
	*/

	if ( EXdeclare(FEhandler, &context) != OK )
	{
		EXdelete();
		IIUGerr(E_IM0007_ingmenu__could_not_se, UG_ERR_FATAL, 0);
	}

	/*
	** Open up the forms system for use in a front
	** end program.	 Subsequent calls to IIUGerr,
	** FEprompt, and FEerror will correctly write
	** messages and errors in forms or non-forms
	** mode if this routine is first called.
	** Also, check the return status from this
	** and exit on FAIL, as something went wrong
	** in forms initialization (such as a bad
	** terminal name) and this should be fatal.
	*/

	if ( FEforms() != OK )
	{
		EXdelete();
		return FAIL;
	}

	/* check if -P specified */
	if (FEutaopen(argc, argv, Gs_Ingmenu) != OK)
            PCexit(FAIL);

	if (FEutaget (Gs_Password, 0, FARG_FAIL, &one_arg, &arg_pos) == OK)
	{
            /* User specified -P. IIUIpassword prompts for password
            ** value, if user didn't specify it. Note: If user specified
	    ** a password attached to -P (e.g. -Pfoobar, which isn't legal),
	    ** then FEutaget will issue an error message and exit (so this
	    ** block won't be run).
            */
            password = IIUIpassword(ERx("-P"));
            password = (password != (char *) NULL) ? password : ERx("");
	}

	FEutaclose();

	/* Write out the copyright messages AFTER forms are on. */

	FEcopyright(ERx("INGMENU"), ERx("1987"));

	/* 
	** INGMENU can call ISQL, so set the flag in FEingres which 
	** forces checking of the user's autocommit state at startup time
	*/
	
	IIUIck1stAutoCom();

	if (*connect != EOS)
	{
		IIUIswc_SetWithClause(connect);
	}

	/*
	** Now open up the database, with the xpipe and user
	** flags set up to NULL or values from the command line.
	** The FEningres call also gets the Usercode and Dba
	** into global variables, and allows subsequent use
	** of the FEconnect and FEdisconnect calls.
	*/

	if ( FEningres(NULL, (i4) 0, dbname, xpipe, user, password,
	     groupid, (char *)NULL) != OK )
	{
		FEendforms();		/* Close forms */
		EXdelete();
		return FAIL;
	}

	/*  AFTER ALL THIS, WE ARE READY TO CALL THE ACTUAL PROGRAM */

	IIIMnaive( dbname, user, (bool)( *xpipe != EOS ), expert );

	/*
	** Close down INGRES, the forms system, the testing environment,
	** and the exception system.
	*/

	FEendforms();		/* Close forms */

	FEing_exit();			/* Close database */

	EXdelete();			/* Get rid of exception handler */

	FEtstend();			/* Close testing files */

	return OK;
}
