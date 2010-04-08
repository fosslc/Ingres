/*
** Copyright (c) 1982, 2008 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<cv.h>
#include	<ex.h>
#include	<st.h>
#include	<te.h>				 
#include	<tm.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ci.h>
#include	<uf.h>
#include	<ug.h>
#include	<ui.h>
#include	<uigdata.h>
#include	<feconfig.h>
#include	<ooclass.h>
#include	<abftrace.h>
#include	<abfcnsts.h>
#include	"erab.h"

/**
** Name:	abf.c -	ABF Main Line Module.
**
** Description:
**	Contains the main-line routine for ABF.	 Defines:
**
**	IIabf()		ABF main line.
**
** History:
**	Revision 2.0  82/04  joe
**	Initial revision.
**
**	Revision 4.0  86/01/15  12:31:23  joe
**
**	6.0 catalog / name generation changes  (87/06  bobm)
**
**	Revision 6.0  87/07  wong
**	'FEingres()' now accepts a NULL-terminated parameter list.
**	Replace "## endforms" and "## exit" with calls to 'FEendforms()'
**	and 'FEing_exit()', resp.; removed PC INGRES catalog check code.
**	Moved check for osNewVersion to aboslver.c  (87/06  joe)
**
**	Revision 6.1  88/02  wong
**	Changed for eventual new catalog interface.
**
**	12/19/89 (dkh) - VMS shared lib changes - References to IIarStatus
**			 is now through IIar_Status and refernces to
**			 IIarDbname is now through IIar_Dbname.
**	Revision 6.3/03/00  90/03  wong
**	Modified to check for functionality argument as 'argv[1]' to
**	distinguish ABF and Vision.
**
**	03-may-1990 (pete)	Change program name; Changed
**				FEingres --> FEningres and added 2 new args.
**	02-13-91 (tom) - added trial version test
**
**	Revision 6.4
**	24-jun-91 (blaise)
**		Replaced ClVBldVer (#defined locally) with FE_VISIONVER
**		(defined in fe.h) as the version of Vision passed to FEingres.
**		Copy utilities now use this macro too.
**	08/16/91 (emerson)
**		Change to IIabf for bug 37878.
**	02-dec-91 (leighb) DeskTop Porting Change:
**		Added routines to pass data across ABF/IAOM boundary.
**	21-feb-92 (mgw) Bug #41515
**		Turn off TR level tracing for now. This was written in
**		abftrace.c by Joe in 1985 and isn't currently used. Someone
**		may want to re-activate this stuff for development and/or
**		internal debugging at some point, but it's not currently used
**		so this change just prevents it from even creating the output
**		file (and thus, trashing any file of the same name).
**	4-May-92 (blaise)
**		Moved the check for whether roles are supported from main()
**		to IIabf(); we now look at iidbcapabilities rather than at 
**		the authorization string, and the new check is made after
**		connecting to the dbms. (bug #43745)
**	26-may-92 (leighb) DeskTop Porting Change:
**		Added code to set GUI title correctly inside a GUITITLE ifdef.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**	24-Aug-2009 (kschendel) 121804
**	    Need ui.h to satisfy gcc 4.3.
**/
FUNC_EXTERN VOID 		IIUGdp_DateParts();
FUNC_EXTERN VOID		IIAMsivSetIIab_Vision();      
FUNC_EXTERN OO_OBJECT **	IIAMgobGetIIamObjects();      

/*{
** Name:	IIabf() -	ABF Main Line.
**
** Description:
**	It determines the way that abf is being called and
**	call the proper internal routines
**
** Input:
**	argc	{nat}  Command-line parameter count.
**	argv	{char *[]}  Command-line parameters.
**
** Returns:
**	{STATUS}  OK if no errors.
**
** Called by:
**	'main()'
**
** History:
**	2-15-91 (tom) - added demo password capability.
**	5-Nov-90 (pete)
**		Change argument abf passes to FEningres from VISIBUILD to
**		VISION.
**	90/03 (jhw)  Corrected to get application ID before editing application,
**		otherwise the entire application will be fetched.  #20229.
**	08/16/91 (emerson)
**		Save pointers to -u and +c flags in new globals
**		IIabUserName and IIabConnect (for bug 37878);
**		abfrun.qsc now looks at these when spawning the interpreter.
*/

GLOBALREF char	**IIar_Dbname;
GLOBALREF char	*IIabGroupID;
GLOBALREF char	*IIabPassword;
GLOBALREF char	*IIabUserName;
GLOBALREF char	*IIabConnect;
GLOBALREF bool	IIabWopt;
GLOBALREF bool	IIabOpen;
GLOBALREF bool	IIabC50;
GLOBALREF bool	IIabVision;
GLOBALREF bool	IIabAbf;
GLOBALREF bool	IIabfmFormsMode;
GLOBALREF bool	IIabKME;


static i4	lock_cleanup();
static STATUS	check_trial();
static VOID	trial_password();


/* Parameter Description */

static ARG_DESC
	args[] = {
		/* Required */
		    {ERx("database"),	DB_CHR_TYPE,	FARG_PROMPT,	NULL},
		/* Optional */
		    {ERx("application"),DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("frame"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("open"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		    {ERx("warning"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		    {ERx("5.0"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		    {ERx("emptycat"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		    {ERx("user"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("groupid"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("password"),	DB_BOO_TYPE,	FARG_FAIL,	NULL},
		    {ERx("connect"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		/* Internal optional */
		    {ERx("equel"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("test"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("dump"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("keystroke"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    {ERx("trace"),	DB_CHR_TYPE,	FARG_FAIL,	NULL},
		    NULL
};

GLOBALREF STATUS	*IIar_Status;
GLOBALREF char		*IIabExename;

STATUS
IIabf (argc, argv)
i4	argc;
char	**argv;
{
	static const	char	_ABF[]		= ERx("abf");
	static const	char	_Vision[]	= ERx("vision");

	/* "Client" info passed to FEningres for use by IIUICheckDictVer */
	static const char	_ClVision[]	= ERx("VISION");
	static const char	_ClIngres[]	= ERx("INGRES");
#define				_ClIngVer	2	/* client version */

	char	*client_dict;
	i4	client_vers;

	STATUS	status;
	char	*appname = (char *)NULL;
	char	*frame = (char *)NULL;
	char	*xpipe = ERx("");
	bool	expert = FALSE;
	bool	password = FALSE;

	char	*test = (char *)NULL;
	char	*dump = (char *)NULL;
	char	*debug = (char *)NULL;
	char	*trace = (char *)NULL;

	bool	trial_test = FALSE; /* set to true if we are to request 
					a trial password */

	EX_CONTEXT	context;

	EX	IIARexchand();
	STATUS	FEchkname();
	STATUS	IIABeditApp();
	char	*IIUIpassword();

	/* We're in Vision/ABF not appimage */
	IIabfmFormsMode = TRUE;

	/*
	** Vision/ABF:
	**	if argv[1] == _ABF we are using old ABF functionality
	**	if argv[1] == _Vision we are using Vision functionality.
	**	Otherwise, we go according to the capability bits. 
	*/
	if ( argc > 1 && STbcompare(argv[1], 0, _Vision, 0, TRUE) == 0 )
	{ /* VISION function */

		/* we set trial_test to true, so we can check latter
		   if the user entered the trial password, this must
		   be done after the form system is initialized */
		trial_test = TRUE;

		IIabVision = TRUE;

		IIabAbf = TRUE;
		client_dict = _ClVision;
		client_vers = FE_VISIONVER;
		--argc; ++argv;
	}
	else if ( argc > 1 && STbcompare(argv[1], 0, _ABF, 0, TRUE) == 0 )
	{ /* ABF function */
		IIabVision = FALSE;
		IIabAbf = TRUE;
		client_dict = _ClIngres;
		client_vers = _ClIngVer;
		--argc; ++argv;
	}
	else
	{ /* check capability bits for function */
		IIabVision = TRUE;
		IIabAbf = TRUE;

		if (IIabVision)
		{
			*argv = _Vision;
			client_dict = _ClVision;
			client_vers = FE_VISIONVER;
		}
		else
		{
			*argv = _ABF;
			client_dict = _ClIngres;
			client_vers = _ClIngVer;
		}
	}
	IIAMsivSetIIab_Vision(IIabVision);		 

	/* Get command line arguments (See 'args[]' above.) */
	args[0]._ref = (PTR)IIar_Dbname;
	args[1]._ref = (PTR)&appname;
	args[2]._ref = (PTR)&frame;
	args[3]._ref = (PTR)&IIabOpen;
	args[4]._ref = (PTR)&IIabWopt;
	args[5]._ref = (PTR)&IIabC50;
	args[6]._ref = (PTR)&expert;
	args[7]._ref = (PTR)&IIabUserName;
	args[8]._ref = (PTR)&IIabGroupID;
	args[9]._ref = (PTR)&password;
	args[10]._ref = (PTR)&IIabConnect;
	args[11]._ref = (PTR)&xpipe;
	args[12]._ref = (PTR)&test;
	args[13]._ref = (PTR)&debug;
	args[14]._ref = (PTR)&dump;
	args[15]._ref = (PTR)&trace;

	if (IIUGuapParse(argc, argv, *argv, args) != OK)
		return FAIL;

	IIabExename = ( IIabVision ) ? ERx("Vision") : ERx("ABF");

#ifdef	GUITITLE				 
	{
		char	title[20];

		STcopy(ERx("INGRES "), title);
		STcat(title, IIabExename);
		TEsetTitle(title);
	}
#endif

	if ( EXdeclare( IIARexchand, &context ) != OK )
	{
		EXdelete();
		return *IIar_Status;
	}

	abtrcinit(argv);
/*
**	if (trace != NULL) 	Turn off TR level tracing for now.
**	{
**		abtrctime(-1);
**
**		if (*trace != EOS)
**			abtrcopen(trace);
**	}
*/
	IIARidbgInit(appname, dump, debug, test);

	aboslver();
	/*
	** If a frame was specified then we are in application test mode
	** and need some of the forms used by the ABF RTS.
	*/
	IIABiniFrm( (bool)(frame == NULL) );

	/*
	** Bug 6549 - if TEMP_INGRES isn't pointing to a writable directory
	** and neither is the current directory, then exit here because
	** subequent calls to DSwrite (which calls NMt_open()) would cause AV.
	*/
	if ( FEchkenv(FENV_TWRITE) != OK )
	{
		FEendforms();
		IIUGerr( ABTMPDIR, UG_ERR_ERROR, 0 );
		EXdelete();
		return FAIL;
	}

	IIABlnkInit( /* dynamiclink = */ TRUE );

	if ( password )
	{ /* check and prompt for password */
		char	*p;

		if ( (p = IIUIpassword(ERx("-P"))) != NULL )
		{
			IIabPassword = p;
		}
		else
		{
			FEendforms();
			EXdelete();
			FEutaerr(BADARG, 1, IIabPassword);
			return FAIL;
		}
	}

	/* if the user tried to invoke Vision, 
	   but didn't have Vision capability,
	   then quiz them on the product trial password */
	if (  trial_test == TRUE
	   && IIabVision == FALSE
	   )
	{
		/* if they don't know that password.. then exit */
	   	if (check_trial(_ClVision) != OK)
		{
			FEendforms();
			EXdelete();
			return FAIL;
		}
	   	IIabVision = TRUE; 
		IIAMsivSetIIab_Vision(IIabVision);	 
	}

	if (*IIabConnect != EOS)
	{
		IIUIswc_SetWithClause(IIabConnect);
	}

	if ( FEningres(client_dict, client_vers, *IIar_Dbname, IIabUserName,
			xpipe, IIabPassword, IIabGroupID, (char *)NULL) != OK )
	{
		FEendforms();
		IIUGerr( NOSUCHDB, UG_ERR_ERROR, 1, *IIar_Dbname );
		EXdelete();
		return FAIL;
	}

	IIAMsetDBsession();	/* set lock mode for DBMS session */

	/* See if Roles are supported */
	IIabKME = IIUIdco_role();

	/* make sure the ABF directories exist. */
	IIABidirInit(*IIar_Dbname);

	IIOOinit( IIAMgobGetIIamObjects() );		 
	IIOOsetidDelta(3);

	IIABliLockInit();

	FEset_exit(lock_cleanup);

	status = OK;
	if ( appname != NULL && *appname != EOS )
	{ /* edit or run the application */
		bool	ign;

		CVlower(appname);
		if ( FEchkname(appname) != OK )
		{
			IIUGerr(APPNAME, UG_ERR_ERROR, 1, appname);
			status = FAIL;
		}
		else if ( frame != NULL && *frame != EOS )
		{ /* run the application starting with frame */
			CVlower(frame);
			IIABrun( appname, OC_UNDEFINED, frame );
		}
		else
		{ /* edit the application */
			OOID	appid;
			bool	ign;

			if (iiabGtAppID( appname, &appid ) == OK)
			{
				/* The application exists */
				status = IIABeditApp(appname, appid, expert,
						     FALSE, &ign);
				if (status != OK)
					status = FAIL;
			}
			else
			{
				/* Application not found */
				status = OK;
				IIABcatalogs( expert, appname );
			}
		}
	}
	else
	{ /* get application catalogs */
		IIABcatalogs( expert, appname );
	}

	FEclr_exit(lock_cleanup);

	lock_cleanup();

	FEing_exit();
	FEendforms();
	IIARcdbgClose();
	EXdelete();

	return status;
}

/*
** want to zap session locks on exception exit.
*/
static i4
lock_cleanup ()
{
	IIABrlRemLock(OC_UNDEFINED);
	IIAMclrDBsession();	/* clear lock mode for DBMS session */
}



/*{
** Name:	check_trial() -	check for trial use of vision.
**
** Description:
**		This routine accepts a seed string which, when coupled with
**		the date, will be able verify if a user entered password
**		is valid.
**
** Input:
**	char *seed;	- the seed to use
**
** Returns:
**	{STATUS}  OK if no errors.
**
** History:
**	2-13-91 (tom) - created 
**	25-march-91 (blaise)
**	    Change call to IIUGdp_DateParts; pass structure stime by reference.
**	    Bug #36666.
*/
static STATUS
check_trial(seed)
char *seed;
{
	char user_result[FE_PROMPTSIZE + 1];
	char pass_result[FE_PROMPTSIZE + 1];
	SYSTIME stime;
	i4 year, month, day;
	i4 junk; 
	i4 i;

	/* get the current year and month according to the system clock */
	TMnow(&stime);

	IIUGdp_DateParts(&stime, &year, &month, &day, &junk, &junk, &junk);

	user_result[0] = EOS;
	IIUFprompt(ERget(S_AB0080_DemoPasswordPrompt), 1, FALSE, 
		user_result, STlength(seed), 0);

	if (user_result[0] == EOS)
	{
		return FAIL;
	}

	trial_password(seed, year, month, day > 15, pass_result);

	if (STbcompare(user_result, 0, pass_result, 0, TRUE) == 0)
	{
		user_result[0] = EOS;
		IIUFprompt(ERget(S_AB0081_DemoWarn), 1, FALSE, 
			user_result, 1, 0);
		return OK;
	}
	else
	{

		IIUGerr(E_AB0083_DemoError, UG_ERR_ERROR, 0);
		return FAIL;
	}
}



/******************************************************

The following main function (along with the trial_password 
function) can be extracted out into it's own file and compiled
to form an executable which will spit out the passwords
for future time periods.

******************************************************/


# ifdef WANT_TRIAL_PASSWORD_GENERATOR

#include <stdio.h>

#define PRETTY

#define i4  int
#define VOID 
#define ERx(x) (x)
main_function()
{
	static char *mname[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	int year, month;
	char result[8]; 
	
	for (year = 1990; year <= 2000; year++)
	{
		for (month = 1; month <= 12; month++)
		{

#ifdef PRETTY
			if (month == 1)
			{
				printf("\n--------------\n");
				printf(  "---  %d  ---\n", year);
				printf(  "--------------\n\n");
			}
			trial_password("VISION", year, month, 0, result);
			printf("%s(1st-15th): %s", mname[month - 1], result);
			trial_password("VISION", year, month, 1, result);
			printf("  (16th on): %s\n", result);
#else
			trial_password("VISION", year, month, 0, result);
			printf("%s\n", result);
			trial_password("VISION", year, month, 1, result);
			printf("%s\n", result);
#endif
		}
	}
}

#endif

/*{
** Name:	trial_password() -	return a trial password for given month 
**
** Description:
**		This routine generates the appropriate password based on
**		seed string, and the year/month.
**		The password returned is exactly as long as the seed string.
**		This routine will only work on ASCII.
**
** Input:
**	char *seed;	- the seed to use
**	i4 year;	- relevent year
**	i4 month;	- relevent month
**	i4 half;	- relevent half of month (0 or 1)
**	char *result;	- pointer to caller's buffer for the result
**	
**
** Returns:
**
** History:
**	2-13-91 (tom) - created 
*/
static VOID
trial_password(seed, year, month, half, result)
char *seed;
i4  year;
i4  month;
char *result;
{
	i4 x;
	i4 len;
	char *rp;

	x = year * month * (half + 183) * 173;

	for (rp = result; *seed; seed++)
	{
		*rp++ = (x & 0x55 + *seed) % 26 + ERx('A'); 
		x >>= 2;
	}

	*rp = '\0';
}
