/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
#include	<pc.h>		 
#include	<st.h>		 
#include	<me.h>
#include	<ex.h>
#include	<ci.h>
#include	<si.h>
#include	<te.h>		 
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<ui.h>
#include	<adf.h>
#include	<fedml.h>
#include	<fstm.h>
#include	<fmt.h>
#include	<frame.h>
#include	<frsctblk.h>
#include	<qr.h>
#include	<uigdata.h>
#include	<ermf.h>
#include	<itline.h>
#include	<libq.h>

# ifndef	CMS
# ifndef	PCINGRES
# define	SQL_OPTION
# endif		/* PCINGRES */
# endif		/* CMS */

/**
** Name:	fstm.c -	Main Routine for Interactive Terminal Monitor.
**
** Description:
**	Contains the main entry point and routine for the QUEL/SQL interactive
**	terminal monitor.  This routine is implements the IQUEL program, with
**	the command line arguments set in the ISQL command script if the
**	version is SQL.  Defines:
**
**	main()		main routine for interactive terminal monitor.
**
** History:
**	Revision 6.1  88/05/31  wong
**	Rewrote pushing forms setup into 'FSrun()' with 'dbname' and the DML
**	being passed instead of the form/field names.  Also, rewrote 'FSargs()'
**	to use ARG_DESC, which is passed in from here.  Also, now calls
**	'IIAFdsDmlSet()' and 'FEingres()' to start up the DBMS connection with
**	the proper DML.
**
**	Revision 4.0  85/12/08  stevel
**	Initial revision.
**
**	Revision 5.0
**	16-jun-86 (marian)	bug 9195 and 9445
**		Make a call to IIbreak() in main() to make sure the
**		pipes are cleared before returning.  This is necessary
**		because FSTM does a 'set noequel'.
**	13-nov-86 (marian)	bug 10693
**		Set IIfstm_on to FALSE before the set equel nasty_hack
**		command so the BE and FE will be sync'ed up.
**		[ code later deleted -- 4/87 (peterk) -- since no longer
**		  use set equel nasty_hack. ]
**	11-feb-87 (jas)
**		Remove unnecessary IIbreak() calls introduced in first
**		fix above.  All that was needed was to set synchronous
**		operation, as was done in second fix above.
**
**	Revision 6.0
**	2/18/87 (daveb)	 -- Call to MEadvise, added MALLOCLIB hint.
**	12-may-87 (daver)
**		Took out SQLLIB, commented out sql_ioinit call as we don't
**		just translate SQL anymore. Also made changes to re-do
**		error processing to allow for multiple error messages from
**		the backend (removed setting of IIwin_err).
**	08/25/87 (scl) #ifdef out FTdiag for FT3270; it has its own
**	11/12/87 (dkh) - Renamed FTdiag to FSdiag.
**	06/05/88 (dkh) - Put back MTLIB into NEEDLIBS.  Don't know who
**			 deleted it.
**	21-oct-88 (bruceb)	Fix for bug 3685
**		Quit the program (somewhat more gracefully than previous AV)
**		if FSinit fails.  This means that user gets warning if
**		II_TEMPORARY directory is unwritable and then program exits.
**	26-may-89 (teresal) Fixes from PC group.
**	19-jun-89 (teresal) Bug#6385 fix for access violation when invoking 
**		IQUEL or ISQL with command line output flags (-c, -t, -i, -f).
**		ADF control structure needed to be initialized before calling
**		FSargs.
**	03-oct-89 (sandyd)
**		Integrated part of ingresug UNIX porting change #90025.
**		Changed "PROGRAM = iquel" Ming hint to "PROGRAM = itm", to
**		coordinate with the new "iquel" and "isql" cover scripts.
**	15-oct-89 (teresal) Init diagnostic function name to FSdiag.  Needed 
**		for pulling tablefield edit funtions out of FSTM and into
**		UF and preserving functionality.
**	16-oct-89 (sylviap) 
**		Changed FSinit calls to IIUFint_Init.  Changed FSdone calls to 
**		  IIUFdone.
**	14-nov-89 (teresal)
**		Added prompting for passwords for the '-P' and '-R' dbms flags.
**	03-jan-90 (sylviap) 
**		Added new parameter to IIUFint_Init for the title to the output
**		browser.  For the terminal monitor, there is no title, so pass 
**		NULL.
**	13-mar-90 (teresal)
**		Clean up of NEEDLIBS in r63: replaced COMPAT with COMPATLIB.
**      29-aug-90 (pete)
**              Change to call FEningres, rather than FEingres and to pass
**              a NULL client name so no dictionary check will be done.
**	09/05/90 (dkh) - Integrated round 3 of MACWS changes.
**      10/10/90 (stevet)
**              Added IIUGinit call to initialize character set attribute
**              table.
**	17-mar-92 (seg)
**		EX handlers return STATUS, not EX.
**	10-jul-92 (leighb) DeskTop Porting Change:
**		Set correct Title in Windows 3.x environment.
**      16-nov-1992 (larrym)
**          Added CUFLIB to NEEDLIBS.  CUF is the new common utility facility.
**	03/25/94 (dkh) - Added call to EXsetclient().
**      31-may-1995 (harpa06) bug #68876
**              Since NO QUEL queries are currently supported by STAR, added
**              message E_MF2107 to tell the user that QUEL is currently not
**              supported bu STAR and gracefully exit.
**	16-jun-95 (emmag)
**		main defined as ii_user_main on NT.
**	24-sep-95 (tutto01)
**		Restored main, run as a console app on NT.
**      03-oct-1996 (rodjo04)
**              Changed "ermf.h" to <ermf.h>
**      18-Feb-1999 (hweho01)
**              Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**              redefinition on AIX 4.3, it is used in <sys/context.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	27-mar-2004 (drivi01)
**	   Changed the name of the globalref debug to fs_debug,
**	   to minimize possible conflicts of varaibles.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	24-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**      03-Sep-2009 (frima01) 122490
**          Add include of itline.h
**      24-Nov-2009 (frima01) Bug 122490
**          Added attributes to FSdiag to represent variable argument list.
**      07-Dec-2009 (maspa05) bug 122806
**          set date_format etc, now change the attribute in the dynamically
**          allocated ADF_CB rather than the static default one. Changed
**          FS_adfscb to be initialized to the dynamic version using
**          IILQasGetThrAdfcb() instead of the static one (FEadfcb). 
**          Need to do this after the call to FEningres where the CB is
**          allocated.
**      08-Jan-2009 (maspa05) Bug 123127
**          Moved declaration of IILQasGetThrAdfcb to libq.h
**/

# ifdef DGC_AOS
# define main IIMFrsmRingSevenMain
# endif


/*
**	MKMFIN Hints.
**
PROGRAM = itm
 
NEEDLIBS =	FSTMLIB QRLIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB MTLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB CUFLIB GCFLIB ADFLIB COMPATLIB MALLOCLIB

UNDEFS = II_copyright
*/

FUNC_EXTERN char *ITldopen();

GLOBALREF ADF_CB	*FS_adfscb;
GLOBALDEF	int	passwd_flag;
GLOBALDEF	int	role_flag;

#ifdef MSDOS
int	_stack = 30000;
#endif	/* MSDOS */

/*{
** Name:	main() -	main routine for interactive terminal monitor.
**
** Description:
**	The main control routine for the SQL and QUEL interactive
**	monitor (formerly Full Screen Terminal Monitor fstm).
**
** Inputs:
**	argc		- standard argument count.
**	argv		- standard argument vector.
**
** History:
**	8-aug-1985 (stevel)	written.
**	14-aug-89 (sylviap)	
**		Took out FSdiag and created FSDIAG.C.  Necessary so ReportWriter
**		could have access to it.
**	28-aug-1990 (Joe)
**	    Changed references to IIUIgdata to the UIDBDATA structure
**	    returned from IIUIdbdata().
**	09-nov-1990 (kathryn)
**	    Generate error message and exit if IIUIpasswd or IIUIroleID
**	    fail.  Both return NULL if password is included with startup flag.
**	10-dec-1990 (kathryn)
**	    Added variables role_flag and passwd_flag which will be set by
**	    FSargs if the "-P" or "-R" flag are issued at startup time.
**	    Prompt for password for "-P" flag will be done after all other
**	    flags have been processed.
**	02-jul-1991 (kathryn) Bug 36671
**	    Remove call to FEdml() - authorization will be checked by FEcichk().
**	    Always set "sql" variable TRUE when the "-sql" flag is issued.
**	    When the "-sql" flag is issued (ISQL) without authorization 
**	    (CI_SQL_TM is not set in authorization string) print an error 
**	    message and exit rather than attempting to startup IQUEL.
**	17-aug-91 (leighb) DeskTop Porting Change:
**	    Removed unnecessary func decl of FSbomb.
**      02-mar-1992 (kathryn) Bug 39902
**              Changed arguments to FEcopyright from messages  F_MO0030 and
**              F_MO0031 to literal strings.  Product Name and copyright year
**              remain the same across all languages so ERget of messages is not
**              needed.  Bug 39902 was ERget of messages failed and sent
**              bad arguments to FEcopyright() which assumes all arguments
**              are literals from FE's and therefore will always be valid.
**	31-may-1995 (harpa06) bug #68876
**		Since NO QUEL queries are currently supported by STAR, added
**		message E_MF2107 to tell the user that QUEL is currently not
**		supported bu STAR and gracefully exit.
**	11-Jul-2007 (kibro01) b118709
**	    Add in call to IItm_adfucolset so the unicode collation
**	    is set up, like happens in "tm".
*/

static BCB	*bcb = {0};

BCB *IIUFint_Init();
i4  FSoutput();

#define MAXARGS 12

GLOBALREF char	*fs_debug;
static char	*keystroke = NULL;
static char	*dump = NULL;
static char	*test = NULL;
static STATUS    FSbomb();
	
/* Parameter Description */

static ARG_DESC
	args[] = {
		/* Internal optional */
		    {ERx("sql"),DB_BOO_TYPE,	FARG_FAIL,	NULL},
		    {ERx("Z"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&test},
		    {ERx("D"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&dump},
		    {ERx("I"),	DB_CHR_TYPE,	FARG_FAIL,	(PTR)&keystroke},
		    {ERx("fsd"),DB_CHR_TYPE,	FARG_FAIL,	(PTR)&fs_debug},
		    NULL
};

i4
main(argc, argv)
i4    argc;
char **argv;
{
# ifdef	SQL_OPTION
	bool	sql = FALSE;
# else
# define	sql	TRUE
# endif
	register i4	j;
	char	*dbname;
	char	*dbargs[MAXARGS];
	STATUS	stat;
	i4	avail;
	EX_CONTEXT	ctx;

	i4	FSrecbe();
	VOID	FSdiag(char *str, ...);
	i4	FSnxtrow();
	ADF_CB	*FEadfcb();
	char	*IIUIpassword();
	char	*IIUIroleID();
	char	*fstmflag;

	/* Tell EX this is an ingres tool. */
	(void) EXsetclient(EX_INGRES_TOOL);

	/* Use the ingres allocator instead of malloc/free default (daveb) */
	MEadvise( ME_INGRES_ALLOC );


	/* Call IIUGinit to initialize character set attribute table. */
	if ( IIUGinit() != OK)
	{
	    PCexit(FAIL);
	}

	/*
	** Register diagnostic function name
	*/
	IIUFtfrTblFldRegister(FSdiag);

	if ( EXdeclare(FSbomb, &ctx) != OK )
	{
		EXdelete();
		PCexit(FAIL);
	}

# ifdef	SQL_OPTION
	args[0]._ref = (PTR)&sql;
#else
	args[0]._name = ERx("");
# endif	/* SQL_OPTION */

	FS_adfscb = FEadfcb();         /* initialize ADF control block */
	role_flag = -1;
	passwd_flag = -1;
	if ( FSargs(argc, argv, args, &dbname, dbargs, MAXARGS) != OK )
	{
		EXdelete();
		PCexit(FAIL);
	}

# ifdef DGC_AOS
	sql = TRUE;
# endif

#ifdef	PMFEWIN3				 
	{
		TEsetTitle(sql ? ERx("INGRES ISQL") : ERx("INGRES IQUEL"));
	}
#endif

	if ( test != NULL && *test != EOS && FEtest( test ) == FAIL ) /* -Z */
	{
		EXdelete();
		PCexit(FAIL);
	}

	fstest( dump, keystroke );

# ifdef DATAVIEW
	IIMWomwOverrideMW();
# endif /* DATAVIEW */

	/* initialize line draw character set */
	(void) ITldopen();

	if (FEchkenv(FENV_TWRITE) != OK)
	{
		IIUGerr(E_MF0025_TempNoWrite, UG_ERR_FATAL, 0);
	}

	if ( FEforms() != OK )
	{
		EXdelete();
		PCexit(FAIL);
	}

	FEcopyright(sql ? ERx("ISQL") : ERx("IQUEL"), ERx("1985"));

	/* initialize ADF session */
	_VOID_ IIAFdsDmlSet( sql ? DB_SQL : DB_QUEL );

	IIUIck1stAutoCom();

	/* Check if any password prompting needed for '-P' or '-R' dbms flags */

	if (role_flag >= 0)
	{
		if ((fstmflag = IIUIroleID(dbargs[role_flag])) != NULL)
			dbargs[role_flag] = fstmflag;
		else
		{
			IIUGerr(E_UG0014_BadArgument,0,1,dbargs[role_flag]);
			if (sql)
				IIUGerr(E_UG0011_ArgSyntax,0,2,ERx("isql"),
				STalloc(ERget(E_MF2006_R_FlagSyntax)));
			else
				IIUGerr(E_UG0011_ArgSyntax,0,2,ERx("iquel"),
				STalloc(ERget(E_MF2006_R_FlagSyntax)));
			FEendforms();
			EXdelete();
			PCexit(FAIL);
		}
	}
	if (passwd_flag >= 0)
	{
		if ((fstmflag = IIUIpassword(dbargs[passwd_flag])) != NULL)
			dbargs[passwd_flag] = fstmflag;
		else
		{
			IIUGerr(E_UG0014_BadArgument,0,1, dbargs[passwd_flag]);
			if (sql)
				IIUGerr(E_UG0011_ArgSyntax,0,2,ERx("isql"),
				STalloc(ERget(E_MF2005_P_FlagSyntax)));
			else
				IIUGerr(E_UG0011_ArgSyntax,0,2,ERx("iquel"),
				STalloc(ERget(E_MF2005_P_FlagSyntax)));
			FEendforms();
			EXdelete();
			PCexit(FAIL);
		}

	}
 
	/* NULL first arg to FEningres says to skip the FE dictionary check */
	if ( FEningres(NULL, (i4) 0, dbname, dbargs[0], dbargs[1],
		dbargs[2], dbargs[3],
		dbargs[4], dbargs[5], dbargs[6], dbargs[7], dbargs[8],
		dbargs[9], dbargs[10], dbargs[11] ) != OK )
	{
		IIUGerr( E_UG000F_NoDBconnect, 0, 1, (PTR)dbname );
		FEendforms();
		EXdelete();
		PCexit(FAIL);
	}

	/* initialize ADF control block - use dynamic version 
	   this needs to have been set up by IILQasAdfcbSetup()
	   IILQasAdfcbSetup() is called via IIdbconnect/IIsqConnect
	   (i.e. EXEC SQL CONNECT) in FEningres above
         */

	FS_adfscb = IILQasGetThrAdfcb(); 
	if (sql && (IIUIdbdata()->firstAutoCom == FALSE))
		IIUIautocommit(UI_AC_OFF);

	/* Set the unicode collation table and normalization
	** fields in Tm_adfscb (same as in tm) (kibro01) b118709
	*/
	IItm_adfucolset(FS_adfscb);

/* harpa06 */
        if (IIUIdcd_dist() && (!sql))
                IIUGerr(E_MF2107_STAR_not_supported, UG_ERR_FATAL, 0);

	bcb = IIUFint_Init( (char *)NULL, (char *)NULL );

	if (bcb == NULL)	/* IIUFint_Init failed */
	{
		FEendforms();
		EXdelete();
		PCexit(FAIL);
	}

	/*
	** Tell the browser subsystem where to find the routine that
	** will get the next record from the backend.
	*/
	bcb->nxrec = FSnxtrow;

	stat = FSrun( bcb, dbname, sql ? DB_SQL : DB_QUEL );

	IIUFdone(bcb);

	/* (peterk)
	**	deleted code placed here to fix bug 10693 --
	**	no longer needed
	*/

	FEing_exit();
	FEendforms();

	fstestexit();
	EXdelete();

	PCexit( stat );
}


/***********************************************************************
**								      **
**    FSbomb  --  Dump diagnostics and closeup shop after abend	      **
**								      **
**    Parameters:						      **
**	none							      **
**								      **
***********************************************************************/
static STATUS
FSbomb ( ex )
EX_ARGS *ex;
{
# ifdef	CMS
	EXautpsy(ex);
	if (ex->exarg_num == EXSNAP)
	{
		return EXCONTINUES;
	}
# endif

	return FEhandler(ex);
}
