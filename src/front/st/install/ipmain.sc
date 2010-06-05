/*
** Copyright (c) 1992, 2004 Ingres Corporation
**
# define DEBUG_ESTIMATE_SIZE
*/

# include <compat.h>
# include <si.h>
# include <ci.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ui.h>
# include <te.h>
# include <gc.h>
# include <gcccl.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <pc.h>
# include <iplist.h>
# include <ip.h>
# include <ipcl.h>
# include <ft.h>
# include <menu.h>
# include <help.h>
# include <pm.h>
# include <iphash.h>
# include <id.h>
#if defined(axp_osf)
#include <sys/stat.h>
#endif


/*
**  Name: ipmain -- main program for INGRES/Install
**
**  Entry points:
**	This file contains no externally visible functions.
**
**  History:
**	12-aug-92 (jonb)
**		Created.
**	17-feb-93 (jonb)
**		Allow the user to kill the setup phase if a setup prog fails.
**      	Also, fix logic to allow the "Setup" menuitem to be run against
** 		a package that's already been set up.
**	9-mar-93 (jonb)
**		Don't display packages on the "Current" screen if they're
**   		invisible according to the authorization string. (b50010)
**	10-mar-93 (kellyp)
**		Fixed SIR 49847 - highlighting the package currently being
**		installed.
**	16-mar-93 (jonb)
**  		Removed feature where install form is called automatically.
**		Made comment style consistent with coding standards.
**	22-mar-93 (jonb)
** 		Display package name when complaining that the system is up
**		after a delete command.  (b49922)
**	23-mar-93 (kellyp)
**		Support preservation of user customized files
**	12-apr-93 (kellyp)
**		Prevent execution of FRS statements in batch mode
**	14-apr-93 (kellyp)
**		Corrected ip_display_transient_msg to be able to handle
**		-output option in batch mode.
**	19-apr-93 (kellyp)
**		Execution setup scripts in the order of the parent package's
**		weight. Weight is a measure of how long it takes to execute
**		the setup scripts in a particular package. The idea is to 
**		start from the package with least weight.
**	28-apr-93 (vijay)
**		Reintegrate a previous change which went into the bit bucket:
**		**    13-apr-93 (vijay)
**		Check validity of dist_dev before giving an error.
**	13-jul-93 (tyler)
**		Changes to support the portable manifest language.
**	14-jul-93 (tyler)
**		Replaced changes lost from r65 lire codeline.
**	05-aug-93 (tyler)
**		Error message now displayed if no distribution
**		device specified. 
**	19-oct-93 (tyler)
**		Fixed bugs reported by ICL Goldrush team.  Removed
**		release_id field from distribution contents form.
**		Added GVLIB to NEEDLIBS and added GLOBALREL declaration
**		of global variable Version[];
**	02-nov-93 (tyler)
**		Removed defunct GVLIB NEEDLIBS entry (there is no
**		GVLIB) and removed references to now defunct WEIGHT
**		attribute, which was removed since it conflicted with
**		correct processing of package setup dependencies.
**	11-nov-93 (tyler)
**		Ported to IP Compatibility Layer and moved from
**		front!st!install_unix path.	
**	22-nov-93 (tyler)
**		Removed duplicate definition of <ex.h>.  Fixed bug
**		which broke package size display routine.
**	01-dec-93 (tyler)
**		Don't issue an error message when install.dat is not found
**		unless it's necessary.
**	10-dec-93 (tyler)
**		Replaced LOfroms() with calls to IPCL_LOfroms().
**	05-jan-94 (tyler)
**		Write install.dat to the "files" directory since it will
**		be overwritten during upgrades otherwise.  Include temporary
**		disk space in required disk space for install confirmation
**		popup.
**	12-jan-94 (tyler)
**		Don't validate distribution device until "Install"
**		menu item is selected.
**	22-feb-94 (tyler)
**		Minor fixes.  Removed Help menu items for now since we
**		have no help text.
**	03/25/94 (dkh) - Added call to EXsetclient().
**      08-Apr-94 (michael)
**              Changed references of II_AUTHORIZATION to II_AUTH_STRING where
**              appropriate.
**      26-Jan-1995 (canor01)
**              B62358: Add Help back to menu and add help file.
**              B60369: If having all dependents set up means no setup
**              needs to be done, change screen to show setup done.
**	27-Jan-95 (wonst02)
**		Only allow "Current" selection if auth string is OK (#60746).
**	30-Jan-95 (wonst02)
**		Save II_AUTH_STRING in symbol.tbl if no setup run (#60744)
**      15-Feb-1995 (canor01)
**		Explicitly tell help runtime where to find the files,
**		since II_CONFIG is not what it seems.
**      16-Feb-1995 (canor01)
**		B63192: if a package is installed as part of another
**		package's installation, we don't mark it NEWLY_INSTALLED
**		so we must check all INSTALLED as well before we update
**		the screen.
**	22-mar-95 (athda01)
**		B63639:  add menuitem 'help' and associated .hlp files for
**		ingbuild/install and ingbuild/current frames.  Also add 
**		'getinfo' menuitem for ingbuild/current.
**      23-Mar-1995 (tutto01) BUG 67539
**              Added check for valid setting of II_TERMCAP_FILE, which may
**              reference a deleted termcap file during an upgrade install.
**              If invalid, set to file in "install" directory.
**	29-Mar-1995 (brogr02)
**		BUG 67243: segv dump when ERget return is copied to too-short 
**		vars, when II_LANGUAGE is set to an invalid value.
**		Also added function prototype for ip_lochelp function and 
**		changed it to static.
**	30-mar-95 (brogr02)
**		Removed func prototype of ip_lochelp()--it is in ip.h
**      31-mar-95 (brogr02)
**		Fix for bug 67243 did not work well.  New approach.
**	11-nov-95 (hanch04)
**		Added new ingbuild menus for InstallCustom, and
**		InstallExpress.
**	29-nov-95 (hanch04)
**		change express to F_ST010E_package_menuitem
**	01-dec-95 (hanch04)
**		fix bug 72961, at least one package will be yes to install
**	08-dec-95 (hanch04)
**		Star shows that the install is running.
**		added please_wait
**	20-dec-95 (hanch04)
**	        fix bug 73303, install will not be shown if no products 
**		are to be installed.
**	19-jan-95 (hanch04)
**		RELEASE_MANIFEST does not have to be release.dat
**	18-match-1996 (angusm)
**		SIR 75433: various changes to allow ingbuild to be used
**		for patch installation/deinstallation.
**	08-jul-1996 (hanch04)
**		eXpress variable needs to be false after setup is done.
**	16-oct-96 (mcgem01)
**		Misc error and informational messages changed to take the
**		product name and user as a parameter.
**	04-nov-1996 (kamal03)
**		The fix made into module ipparse.y (change 420188) is causing
**		the VAX C Compiler to finish with errors. To fix the problem,
**		reverse change 420188 and change this module: initialize
**		GLOBALDEF variable "batchOutf" at the main entry point.
**	22-nov-96 (reijo01)
**		Remove auth check if Jasmine. Use generic system variables.
**	17-dec-96 (reijo01)
**		Introduced bool ingres_menu. Used to include parts not used
**		for Jasmine. Also changed calls to use jasmainform and     
**		jasmainform1 for Jasmine.
**	18-apr-97 (funka01)
**		Move copy of package_menuitem button to top of if ingres_menu
**		check in install_frame
**	12-jun-97 (funka01)
**		Remove express_menuitem button from install menu for jasmine 
**	08-jul-97 (funka01)
**		Remove leftover debug statements that somehow got submitted. 
**	09-jul-97 (funka01)
**		Change to package install for jasmine installation.
**	16-oct-97 (funka01)
**		Modify E_ST0535 to E_ST053B to correspond to emma's change
**		to avoid duplicate messaging.
**	28-May-1998 (hanch04)
**		Do not use II_AUTH_STRING for INGRESII
**	05-jan-98 (funka01)
**		Submitting changes for jasmine installation made for ga release
**	08-Jun-1998 (allmi01)
**		Added initialization of variable a4 for INGRESII builds.
**	11-Jun-1998 (hanch04)
**		FRS env must be setup before print errors.
**	22-Jul-1998 (hanch04)
**		Show user license.txt before starting forms.
**	28-Jul-1998 (hanch04)
**		User doesn't need to agree to license.  Left code
**		in case we need it later.
**	30-Nov-1998 (toumi01)
**		Skip ip_init_calicense for the free Linux version.
**		INGRESII is overloaded to mean that CA licensing is in
**		effect (not necessarily true).  Define, locally,
**		INGRESII_CALICENSE to have this meaning and override
**		it for Linux (lnx_us5).
**      11-mar-199 (hanch04)
**              Added ignoreChecksum flag to ignore file mismatch errors.
**	04-oct-1999 (toumi01)
**		Modify Linux test for CA licensing to check VERS setting,
**		since at least one build configuration of the Linux version
**		now _does_ include CA licensing.
**	06-oct-1999 (toumi01)
**		Change Linux config string from lnx_us5 to int_lnx.
**	24-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products. Also, incorporated specific changes for EDBC.
**	19-may-2000 (somsa01)
**		The previous change accidentally renamed the regular Ingres
**		form to the EDBC one, even in the case when EDBC was not
**		defined.
**	20-may-2000 (somsa01)
**		The HELP screen should display the product-specific install
**		name.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	07-Sep-2000 (hanch04)
**	    If II_EMBED_USER is set in the environment, then the checking
**	    for the ingres id is bypassed.
**	01-aug-2001 (toumi01)
**	    for i64_aix there is no CA licensing
**	09-Sep-2001 (gups01)
**	    Added mkresponse. 
**	19-Oct-2001 (gupsh01)
**	    Added -exresponse option. Modified static function 
**	    ip_mkresponse to ip_response. Added Global exresponse.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Avoid some potential buffer overruns.
**	20-Jul-2004 (hanje04)
**	    Remove call to check authstring as function has been removed.
**	16-Nov-2004 (bonro01)
**	    Allow ingbuild to install Ingres using any userid.
**	03-Dec-2004 (sheco02)
**	    Enable license mechanism for CATOSL agreement for Open Source.
**	20-Dec-2004 (sheco02)
**	    It is now decided to disable the license mechanism for CATOSL.
**      31-Mar-2005 (kodse01)
**	    Changed 0 to NULL in IDsetsys call.
**	09-May-2005 (bonro01)
**	    Ingbuild unintentionally kills itself, because 'batchMode' was
**	    not being checked before calling PCforce_exit() which was meant
**	    to kill the spinning character star "process" which is only
**	    created in forms mode.
**	21-Sep-2005 (hweho01)
**	    For Tru64 platform, fixed the permission denied error during 
**          a upgrade from 2.6/SP2 installation. Star #14394628.
**	17-Feb-2006 (bonro01)
**	    Enable license prompt
**	23-Mar-2006 (bonro01)
**	    The install has been changed to display the LICENSE for all
**	    installs including interactive, express, and batch modes.
**	    Embedded installs will also be prompted for a LICENSE unless
**	    the -acceptlicense option is passed to ingbuild.
**	10-Apr-2006 (hweho01)
**          Check the existence of DBMS server and its version before 
**          install the spatial package. 
**       6-Nov-2006 (hanal04) SIR 117044
**          Add int.rpl for Intel Rpath Linux build.
**	11-Dec-2006 (bonro01)
**	    Disable the license prompt yet again. (third time).
**       9-May-2007 (hanal04) Bug 117075
**          Add new mode parameter to ip_open_response() call.
**	11-Feb-2008 (bonro01)
**	    Remove separate Spatial object install package.
**	    Include Spatial object package into standard ingbuild and RPM
**	    install images instead of having a separate file.
**	6-Mar-2008 (bonro01)
**	    Enable upgrade install of netu utility so that it does not fail
**	    while extracting a new directory on top of the old netu exe.
**      20-Oct-2008 (hweho01)
**          For 9.2, the ice and ice64 packages can only be installed      
**          in these two situations : 
**          1) Specify the package name in option -install in command line. 
**          2) In a upgrade install, the package was installed previously.  
**       7-Jan-2009 (hweho01)
**          Remove ice and ice64 packages from full installation which is    
**          listed in the package installation panel.
**	23-Aug-2009 (kschendel) 121804
**	    Need more .h's to satisfy gcc 4.3.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of iphash.h and id.h to eliminate gcc 4.3 warnings.
**	10-Dec-2009 (bonro01)
**	    Enable the license prompt yet again. (fourth time).
**	12-Feb-2010 (shust01)
**	    Changed text for license acceptance.  Also only accept 'y', 'yes', 
**	    'n' or 'no' for valid answers for acceptance. b123285.
**	20-Apr-2010 (frima01) B 123610
**	    Set II_ADMIN to II_SYSTEM/ingres/install cause it is needed
**	    to launch the front end part of ingbuild.
*/

/*
PROGRAM =	(PROG2PRFX)build
**
NEEDLIBS =	INSTALLLIB UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB \
		FEDSLIB UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB SQLCALIB GCFLIB ADFLIB CUFLIB SQLCALIB \
		COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

/* Not in any .h's yet (but should be) */
FUNC_EXTERN bool	IIRTfrshelp(char *, char *, i4, void(*)());

# ifdef INGRESII
# define INGRESII_CALICENSE
# endif /* INGRESII */
# if defined(int_lnx) || defined(int_rpl) || defined(i64_aix)
# if !defined(conf_CA_LICENSE)
# undef INGRESII_CALICENSE
# endif /* conf_CA_LICENSE */
# endif /* int_lnx || int_rpl || i64_aix */
 
exec sql include sqlca;
exec sql include sqlda;

#define LICENSE_PROMPT 1

/* Way too many global variables... */ 
 
exec sql begin declare section;
char authString[ MAX_MANIFEST_LINE + 1 ]; /* Current authorization string. */
bool authStringEnv;		   /* TRUE if II_AUTH_STRING from environ. */
bool batchMode = FALSE;		   /* TRUE if we're running in batch mode. */
bool ignoreChecksum= FALSE;	   /* TRUE if we're running we don't check */
bool noSetup = FALSE;		   /* TRUE if skipping setup. */
bool eXpress = FALSE;		   /* TRUE if batch mode setup. */
bool custChoice = FALSE;	   /* TRUE if cust choices menu */
GLOBALDEF bool pmode = FALSE;   /* TRUE if in 'patch install' mode */
GLOBALDEF bool mkresponse = FALSE;   /* TRUE if in 'make response file' mode */
GLOBALDEF bool exresponse = FALSE;   /* TRUE if in 'execute response file' mode */
GLOBALDEF char *respfilename = NULL; /* user supplied response file name */
char userName[ MAX_MANIFEST_LINE ]; /* Name of current user. */
static char localHost[ GCC_L_NODE + 1 ]; /* Name of the local host. */
GLOBALDEF char installDir[ MAX_LOC + 1 ]; /* INGRES root directory */
GLOBALDEF char releaseManifest[ 32 ]; /* release.dat filename */
char distribDev[MAX_LOC];   	   /* Name of distribution device */
bool distribDevEnv;		   /* TRUE if II_DISTRIBUTION from environ. */
char distribHost[ MAX_MANIFEST_LINE ]; /* Host for distrib device, if any */
static char topFormName[ MAX_MANIFEST_LINE ]; /* Form used by top frame */
static char instFormName[ MAX_MANIFEST_LINE ]; /* Form used by install frame */
static char *productName;	   /* Name of the product, from parse */
static char *distRelID;		   /* Major release ID of distrib, from parse */
char *instRelID;		   /* Major release ID of curr. installation */
char yesWord[10], noWord[10];
bool ingres_menu=TRUE;		   /* Set to TRUE if starting INGRES menu */
GLOBALDEF char          *ii_system_name;
GLOBALDEF char          *ii_build_name;
GLOBALDEF bool          Install_Fullpkg = FALSE;
exec sql end declare section;

i4  ver_err_cnt;		   /* Number of verification errors so far */

static bool forms_state = FALSE;   /* Remember whether forms are on or off */

GLOBALREF FILE *batchOutf;	   /* Output text file for batch mode. */
GLOBALREF FILE *rSTREAM;	   /* Output text file for response file */
GLOBALREF bool batch_opALL; 	   /* flag for selecting all in batch mode */
GLOBALREF bool Install_Ice; 	   /* flag for selecting ice in batch mode */

#if LICENSE_PROMPT
bool acceptLicense = FALSE;	   /* TRUE if license accepted with -acceptlicense*/
#endif

/*
** The lists.  Each is structured much like the manifest file.  They're
** lists of packages; each package has a list of parts; each part has
**  a list of files.  
*/

LIST distPkgList;   /* Data list for distribution medium */
LIST instPkgList;   /* Data list for current installation */

/*
**  Local functions...
*/

static bool	top_frame();
static void	install_frame();
static void	current_frame();
static STATUS	verify_installdir();
static void	format_size();
static void	refresh_currform();
static void     please_wait();
static void	write_inst_descr();
static void	do_install();
static bool	install_popup();
static void	compose_auth();
static void	decompose_auth();
static void	ip_init_distribdev();
static VOID 	ip_patchmode();
static VOID 	ip_response();
static VOID	ip_response_init();

#if LICENSE_PROMPT
void		license_prompt();
#endif

/* 
**  Entry points to compiled forms...
*/

exec sql begin declare section;
globalref int *installform;
globalref int *custform;
globalref int *currform;
globalref int *ipmainform;
globalref int *ipmainform1;
globalref int *waitform;
exec sql end declare section;

/*
**  Initial setup to get us into forms mode.  This includes informing
**  the FRS about the compiled forms.
*/

static void
enable_frs()
{
	ip_forminit();

	exec frs addform :installform;
	exec frs addform :custform;
	exec frs addform :currform;
	exec frs addform :ipmainform;
	exec frs addform :ipmainform1;
	exec frs addform :waitform;

	exec frs set_frs frs ( outofdatamessage = 0 );

	forms_state = TRUE;
}

/*
**  Leave forms mode.
*/

static void
end_frs()
{
	if (forms_state)
	{
		exec frs clear screen;
		exec frs endforms;
	}
}

/*
**  Main program.
**
** History:
**
**	11-dec-2001 (gups01)
**	    If we are normally quitting the program make sure that in 
**	    mkresponse mode, response file is closed.
*/

main(i4 argc, char *argv[])
{
    EX_CONTEXT	context;
    EX		FEhandler();
    STATUS 	rtn;
    char	*adminuser=NULL;
    char	*cp;
    char	*cp2;
    LOCATION    tcloc;
    char        tcbuf[MAX_LOC + 1];
    i4		tempnat;
    CL_ERR_DESC clerrdesc;
    bool	install;

    char admin_env[MAX_LOC + 1];
    STcpy(admin_env,getenv( SystemLocationVariable ));
    STcat(admin_env,"/ingres/install");
    IPCLsetEnv( ERx( "II_ADMIN" ), admin_env, TRUE );

    /*
    ** Load PM resource values from the standard location.
    */
    if( PMinit() == OK )
	PMload((LOCATION *)NULL, (PM_ERR_FUNC *)NULL );

    /*  Initialize batchOutf (if it's not up to this point)    */
    if (batchOutf == 0)
	batchOutf = stdout;

    /* Tell EX this is an ingres tool. */
    (void) EXsetclient(EX_INGRES_TOOL);

    MEadvise( ME_INGRES_ALLOC ); /* We'll use the Ingres malloc library */

    if( EXdeclare( FEhandler, &context ) != OK ) /* Any failure here... */
    {
        EXdelete();
        PCexit( FAIL );
    }

    (void) GChostname( localHost, sizeof( localHost ) );

    ip_init_installdir();	/* Figure out real value of $II_SYSTEM/ingres */
    ii_build_name = STalloc (SystemBuildName);
    ii_system_name = STalloc (SystemLocationVariable);

    ip_init_distribdev();	/* Get and verify II_DISTRIBUTION */

    /* 
    **  Check that the installation directory exists.
    */
        IPCL_LOfroms( PATH, ip_install_loc(), &tcloc );
        if (LOexist(&tcloc) != OK)
        {
 	   SIfprintf( stderr,"The installation directory <%s> does not exist.\n", ip_install_loc()); 
	   exit(1);
        } 
    /*
    **  Check II_TERMCAP_FILE for validity, in case of update install which
    **  may remove the current file.
    */

    NMgtAt(ERx("II_TERMCAP_FILE"), &cp);
    if (cp && *cp)
    {
        STlcopy(cp, tcbuf, sizeof(tcbuf)-1);
        IPCL_LOfroms( PATH & FILENAME, tcbuf, &tcloc );
        if (LOexist(&tcloc) != OK)
        {
            IPCL_LOfroms( PATH, ip_install_loc(), &tcloc );
            LOfstfile( "termcap", &tcloc );
            LOtos( &tcloc, &cp );
            IPCLsetEnv( ERx("II_TERMCAP_FILE"), cp, FALSE );
        } 
    }

    ip_environment();		/* Make sure environment supports FRS */

    cp = userName;
    IDname( &cp );		/* Get current user's ID */

    /* Set SystemAdminUser from config.dat unless this is an initial */
    /* install then set it to the current user.  This allows ingres  */
    /* to be installed as any user but prevents other users from     */
    /* running ingbuild after the installation is complete.          */
    rtn = PMsetDefault(1,PMhost());
    rtn = PMget("ii.$.setup.owner.user", &adminuser );
    if ((rtn == OK) && (adminuser))
	IDsetsys(NULL,NULL,NULL,NULL,NULL,NULL,NULL, adminuser,NULL);
    else
	IDsetsys(NULL,NULL,NULL,NULL,NULL,NULL,NULL, cp,NULL);

    cp2 = NULL;
    NMgtAt(ERx("II_EMBED_USER"), &cp2);
    if (!(cp2 && *cp2))
    {
	if (STcompare( cp, SystemAdminUser ) )
	{
	    IIUGerr( E_ST0159_bad_user_id, 0, 2, cp, SystemAdminUser );
	    EXdelete();
	    PCexit( FAIL );
	}
    }

    /* Validate the language to use.  If II_LANGUAGE has been set and
       it is set to a non-supported value, we give an error message in 
       English and we stop.  
       If you wait and let ERget() error-out, it gives a cryptic 
       error message and it doesn't return a status code so we 
       can stop processing.  It also overwrites the buffer you pass it
       with a too-long error string which causes segv's (bug 67243). */

    if (iiuglcd_check() != OK)
    {
	/* error message already output by iiuglcd_check() */
        EXdelete();
	PCexit( FAIL );
    }

    STcopy(ERget(F_ST0103_yes),yesWord);
    STcopy(ERget(F_ST0104_no), noWord);

    /* set releaseManifest variable */
    STcopy( ERx("release.dat") , releaseManifest );

    /*
    ** Read all data from the release description file and store it in
    ** internal data structures...
    */
    if( ip_parse( DISTRIBUTION, &distPkgList, &distRelID, &productName,
	TRUE ) != OK )
    {
	PCexit(FAIL);
    }

    /* 
    ** Read in the installation descriptor file if there is one...
    */
	ip_patchmode(argc, argv);

	ip_response(argc, argv); 

    if( ip_parse( INSTALLATION, &instPkgList, &instRelID, NULL, FALSE )
	!= OK )
    {
	PCexit( FALSE );
    }

    rtn = ip_batch(argc,argv);  /* See if we're running in batch mode */
#if LICENSE_PROMPT
    /* Check that the license agreement file is readable */
    if ( !batchMode && !acceptLicense )
    { 
	license_prompt();
    }
#endif
    if (!batchMode)		/* If not... */
    {
	if ( IIUGinit() != OK)   /* ...fire up the forms system... */
	    PCexit(FAIL);

	for (enable_frs() ; top_frame(); )  /* ... and start running forms. */
	    ;
	     
	end_frs();

    }

    /* we are quitting out and if the response mode is on and response file is
    ** we should close the response file now.
    */
    if ( mkresponse && rSTREAM )
    {
	ip_close_response(rSTREAM); 
	rSTREAM = NULL;
    }

    EXdelete();
    PCexit( rtn );
}

#if LICENSE_PROMPT
/*
**  license_prompt() -- display license prompt
*/
void
license_prompt()
{
    char	*cp2;
    char        cp2buf[MAX_LOC + 1];
    LOCATION    textloc;
    char 	*textfilename;
    i4  	flagword;
    LOINFORMATION loinfo;
    char	cmd[1024];
    char 	answer[80],ans[80];

	NMgtAt(SystemLocationVariable, &cp2);
	STlcopy(cp2, cp2buf, sizeof(cp2buf)-20-1);
	LOfroms(PATH, cp2buf, &textloc);
	LOfaddpath(&textloc, SystemLocationSubdirectory, &textloc);
	LOfaddpath(&textloc, ERx("install"), &textloc);
	LOfstfile("LICENSE", &textloc);
	LOtos(&textloc, &textfilename);
	flagword = (LO_I_TYPE | LO_I_PERMS);
	if (LOinfo(&textloc, &flagword, &loinfo) == OK)
	{
	    if((flagword & LO_I_TYPE) == 0 || (loinfo.li_type !=LO_IS_FILE))
	    {
		SIfprintf(stdout, "License text file is not accessible\n");
		exit(1);
	    }
	    if((flagword & LO_I_PERMS) == 0 || 
		(loinfo.li_perms & LO_P_READ) == 0)
	    {
		SIfprintf(stdout, "License text file is not accessible\n");
		exit(1);
	    }
	}
	else
	{
	    SIfprintf(stdout, "License text file is not accessible\n");
	    exit(1);
	}
	STprintf(cmd,"more %s", textfilename);
	system(cmd);
	for(;;)
	{
	    SIfprintf(stdout, "Do you accept this license agreement? (y or n): ");
	    SIgetrec(answer,80,stdin);
	    if( (!STxcompare(answer,80,"no\n",3,1,1)) ||
		(!STxcompare(answer,80,"n\n",2,1,1)))
	    {
		SIfprintf(stdout, "Installation Cancelled\n");
		exit(1);
	    } 
	    else
		if( (!STxcompare(answer,80,"yes\n",4,1,1)) ||
		    (!STxcompare(answer,80,"y\n",2,1,1)))
	    {
		   break;
	    }
	}
}
#endif

/*
**  install_frame() -- display and process the installation form.
*/

static void
install_frame()
{
	exec sql begin declare section;
	char name[ MAX_MANIFEST_LINE ];
	char msg[ 1024 ];
        char express_menuitem[20];
	char yorn[ 20 ];
	char *tagp;
	exec sql end declare section;
	PKGBLK *pkg, *ipkg;
	LISTELE *lp;


       if (ingres_menu)
         STcopy(ERget(F_ST010D_express_menuitem), express_menuitem);
       else       
         *express_menuitem = EOS;

	/*
	**  Various housekeeping to get the form displayed...
	*/

	exec frs inittable :instFormName dist_tbl update;
	exec frs display :instFormName;
	exec frs initialize (
                ii_build_name = :ii_build_name,
                ii_system_name = :ii_system_name
        );


	exec frs begin;

	if( *distribHost == EOS )
		/* display distrib dev in standard format */
		STcopy( distribDev, msg );
	else
		STpolycat( 3, distribHost, ERx(":"), distribDev, msg );

	STpolycat(3, productName, ERx(" "), distRelID, name);

	/* display header stuff on form... */
	exec frs putform :instFormName (
		installation_area = :installDir, 
		dist_medium = :msg
	);
        
        /*
        ** The packages ice and ice64 are invisible in 9.2 by default,   
        ** will not be displayed in the panel for selection. 
        ** However if the packages were installed previously, they will 
        ** be showned for upgrade.
        */
        if( ( ip_find_package( "ice64", INSTALLATION ) == NULL ) && 
 	    ( ip_find_package( "ice", INSTALLATION ) == NULL ) )
        {
          if( (pkg = ip_find_package( "ice", DISTRIBUTION )) != NULL )  
        	     pkg->visible = INVISIBLE;  
          if( (pkg = ip_find_package( "ice64", DISTRIBUTION )) != NULL )  
        	     pkg->visible = INVISIBLE;  
        } 

	/* scan distribution list and display everything that's visible. */
	for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
	{
		exec sql begin declare section;
		char size[10], lic_col[10];
		char *rev;
		exec sql end declare section;

		pkg = (PKGBLK *) lp->car;

# ifdef OMIT
		if( pkg->nfiles && ip_is_visible( pkg ) )
# endif /* OMIT */
		if( ( ip_is_visible( pkg ) && custChoice )  ||
		    ( ip_is_visiblepkg( pkg )  && !custChoice ) )
		{
			/* see if this version is already installed... */
			i4 state = ( NULL != (ipkg = ip_find_package(
				pkg->name, INSTALLATION )) &&
				!STcompare( pkg->version, ipkg->version ) )
				? ipkg->state : NOT_INSTALLED;

			/* set up some fields to display on the form... */
			rev = pkg->version;
			STcopy( pkg->name, name );
			STcopy( yesWord, lic_col );
			if( custChoice )
			    STcopy( yesWord, pkg->tag );
			else
			    STcopy( noWord, pkg->tag );

			if( state == UNDEFINED ) 
				STcopy( ERx( "?" ), pkg->tag );
			else if( state != NOT_INSTALLED && ipkg != NULL
				&& STequal( ipkg->version, pkg->version ) )
			{
				if( ip_setup( ipkg, _CHECK ) )
				{
					STcopy( ERget( F_ST0137_no_setup_tag ),
						pkg->tag );
				}
				else
				{
					STcopy( ERget( F_ST0136_installed_tag ),
						pkg->tag );
				}
			}
		
			format_size( size, pkg->apparent_size );

			tagp = pkg->tag;

#ifdef INGRESII
			exec frs loadtable :instFormName dist_tbl (
				package = :name,
				size = :size,
				inst_yorn = :tagp
			);
#else /* INGRESII */
			exec frs loadtable :instFormName dist_tbl (
				package = :name,
				size = :size,
				auth_yorn = :lic_col,
				inst_yorn = :tagp
			);
#endif /* INGRESII */
		}
	}
	if( !custChoice )
	{
        	exec sql begin declare section;
        	i4 rowend;
        	char yorntmp[ 20 ];
        	exec sql end declare section;

  		exec frs inquire_frs table :instFormName
                                        ( :rowend = lastrow );
		exec frs getrow :instFormName dist_tbl :rowend
                                ( :yorntmp = inst_yorn );
 		if( !CMcmpnocase( yorntmp, noWord ) )
	       		exec frs putrow :instFormName dist_tbl
					 :rowend ( inst_yorn = :yesWord ); 
	}
	exec frs end;

	exec frs activate menuitem :ERget( F_ST0105_setup_all_menuitem );
	exec frs begin;
	{
		bool write_needed, did_setup;
		PKGBLK *pkg;

		exec sql begin declare section;
		i4 nrow, firstrow;
		exec sql end declare section;

		/* Scan tablefield for anything that requires setup... */
		did_setup = FALSE;
		exec frs unloadtable :instFormName dist_tbl ( :name = package );
		exec frs begin;

			if ( NULL != (ipkg = ip_find_package( name,
				INSTALLATION)) && ip_setup( ipkg, _CHECK ) )
			{
				did_setup = TRUE;
				if( ip_setup( ipkg, _PERFORM ) )
				{
					/* Mark stuff we did */
					if( (pkg = ip_find_package( name,
						DISTRIBUTION )) != NULL )
					{
						/* this shouldn't fail */
						pkg->state = NEWLY_INSTALLED;
					}
				}
				else
				{
					STcopy( ERget( S_ST0125_continue_setup_prompt),
						msg);
	   				if( ip_listpick( msg,
						F_ST0103_yes,
						S_ST0183_continue_setup,
						F_ST0104_no,
						S_ST0184_abort_setup ) )
					{
						/* no more setup */
						break;
					}
		    		}
			}
		exec frs end;

		ip_forms( TRUE );

		/* Position tablefield cursor on the first thing set up... */

		write_needed = FALSE;
		firstrow = -1;
		/*for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )*/
		for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
		{
			pkg = (PKGBLK *) lp->car;

			if( ( ip_is_visible( pkg ) || ip_is_visiblepkg ( pkg ) )
			      && (pkg->state == NEWLY_INSTALLED ||
			     pkg->state == INSTALLED) ) /* B63192 */
			{
				pkg->state = INSTALLED;
				write_needed = TRUE;
				exec frs unloadtable :instFormName dist_tbl (
					:name = package, :nrow = _record
				);
				exec frs begin;

					if( ip_compare_name( name, pkg ) )
					{
						exec frs putrow :instFormName 
							dist_tbl (
							inst_yorn = :ERget(
							F_ST0136_installed_tag ) );
						if( firstrow == -1 )
							firstrow = nrow;
						exec frs endloop;
					}

				exec frs end;
			}
		}

		if( firstrow != -1 )
			exec frs scroll :instFormName dist_tbl to :firstrow;

		if( !did_setup )
			IIUGerr( E_ST0143_no_pkgs_req_setup, 0, 0 );
		else if( write_needed )
			/* Status changed; rewrite inst. descriptor */
			write_inst_descr();

		exec frs resume column dist_tbl inst_yorn;
	}
	exec frs end;

	exec frs activate menuitem :ERget( F_ST0102_setup_menuitem );
	exec frs begin;
	{
		bool setup_ok;
		PKGBLK *pkg;

		/* get package record for versin comparision */
		if( (pkg = ip_find_package( name, DISTRIBUTION )) == NULL )
			exec frs resume menu;

		exec frs getrow :instFormName dist_tbl ( :name = package );

		if( (ipkg = ip_find_package( name, INSTALLATION ) ) == NULL ||
			!STequal( ipkg->version, pkg->version ) )
		{
			exec frs resume menu;
		}

		if( !ip_setup( ipkg, _CHECK ) )
		{
			STprintf( msg,
				ERget( S_ST0146_no_setup_reqd ), ipkg->name );
			ip_display_msg( msg );
		}
		else 
		{
			setup_ok = ip_setup( ipkg, _PERFORM );
			ip_forms( TRUE );
			if( setup_ok ) 
			{
				exec frs putrow :instFormName dist_tbl (
					inst_yorn =
						:ERget( F_ST0136_installed_tag )
				);
				write_inst_descr();
			}
		}
		exec frs resume column dist_tbl inst_yorn;
	}
	exec frs end;

	exec frs activate menuitem :ERget( F_ST0100_install_menuitem );
	exec frs begin;
		eXpress = FALSE;
		do_install( NULL );
		exec frs resume column dist_tbl inst_yorn;
	exec frs end;

	exec frs activate menuitem :express_menuitem;
	exec frs begin;
		eXpress = TRUE;
		do_install( NULL );
		eXpress = FALSE;
		exec frs resume column dist_tbl inst_yorn;
	exec frs end;

	exec frs activate menuitem :ERget( F_ST0106_describe_menuitem );
	exec frs begin;
	{
		exec sql begin declare section;
		char pkgname[ MAX_MANIFEST_LINE ];
		exec sql end declare section;

		exec frs getrow :instFormName dist_tbl ( :pkgname = package );

		ip_describe(pkgname);

		exec frs resume column dist_tbl inst_yorn;
	}
	exec frs end;

   	exec frs activate menuitem :ERget( F_ST0108_help_menuitem )
		( activate = 0 ), frskey1 ( activate = 0 );
    	exec frs begin;
	if(ingres_menu)
	{
		ip_lochelp( "installf.hlp", "Ingbuild/Install" );
	}
	else
		ip_lochelp( "installf.hlp", "Jasbuild/Install" );
	exec frs end;


	exec frs activate menuitem :ERget( F_ST0109_end_menuitem )
		( activate = 0 ), frskey3 ( activate = 0 );
	exec frs begin;
		exec frs breakdisplay;
		return;
	exec frs end;

	exec frs activate before column dist_tbl all;
	exec frs begin;
	{
		bool test; 

		/*
		** Conditionally display "Install" menu choice. 
		*/
		test = FALSE;
		exec frs unloadtable :instFormName dist_tbl ( :yorn = inst_yorn );
		exec frs begin;
			if( !STcompare( yorn, yesWord ) )
			{
				test = TRUE;
				break;
			}
		exec frs end;

		if( test )
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0100_install_menuitem ) )
					= 1
			);
			if (ingres_menu) {
			exec frs set_frs menu '' (
				active( :ERget( F_ST010D_express_menuitem ) )
					= 1
			);
			}
		}
		else
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0100_install_menuitem ) )
					= 0
			);
			if (ingres_menu) {
			exec frs set_frs menu '' (
				active( :ERget( F_ST010D_express_menuitem ) )
					= 0
			);
			}
		}

		/*
		** Conditionally display "SetupAll" menu choice. 
		*/
		test = FALSE;
		for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
		{
			pkg = (PKGBLK *) lp->car;
	
			if( ( ip_is_visible( pkg ) || ip_is_visiblepkg( pkg ) )
				&& ip_setup( pkg, _CHECK ) )
			{
				test = TRUE;
				break;
			}
		}
		if( test )
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0105_setup_all_menuitem ) )
					= 1
			);
		}
		else
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0105_setup_all_menuitem ) )
					= 0
			);
		}

		exec frs getrow :instFormName dist_tbl ( :name = package );

		/*
		** Conditionally display "GetInfo" menu choice. 
		*/
		if( (pkg = ip_find_package( name, DISTRIBUTION ) ) == NULL )
			exec frs resume menu;
		if( pkg->description == NULL )
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0106_describe_menuitem ) )
					= 0
			);
		}
		else
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0106_describe_menuitem ) )
					= 1
			);
		}

		/*
		** Conditionally display "Setup" menu choice. 
		*/
		if( (pkg = ip_find_package( name, INSTALLATION ) ) == NULL ||
			!ip_setup( pkg, _CHECK ) )
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0102_setup_menuitem ) ) = 0
			);
		}
		else
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0102_setup_menuitem ) ) = 1
			);
		}
	}
	exec frs end;

	exec frs activate after column dist_tbl inst_yorn;
	exec frs begin;
	{
		exec sql begin declare section;
		i4 chflg;
		i4 maxrows;
		i4 rownum;
		char yorntmp[ 20 ];
		exec sql end declare section;
		bool resumeflag = TRUE;
                bool test;

		exec frs inquire_frs row :instFormName dist_tbl (
			:chflg = change( inst_yorn )
		);

		if (!chflg) 
			exec frs resume next;

		exec frs getrow :instFormName dist_tbl (
			:name = package,
			:yorn = inst_yorn
		);

		if( (pkg = ip_find_package( name, DISTRIBUTION )) == NULL )
			exec frs resume;

		if( (ipkg = ip_find_package( name, INSTALLATION )) == NULL ||
			!STequal( ipkg->version, pkg->version ) )
		{
			goto yes_or_no; 
		}

		if( ipkg->state == UNDEFINED )
		{
yes_or_no:		/* "Install?" field must contain "y" or "n"... */
			if( !CMcmpnocase( yorn, yesWord ) )
			{
				STcopy( yesWord, yorn );
				if ( !custChoice)
				{
				exec frs inquire_frs table :instFormName
					( :maxrows = lastrow );
				for ( rownum = 1 ; rownum <= maxrows; rownum++ )
				{
				exec frs getrow :instFormName dist_tbl :rownum
				( :yorntmp = inst_yorn );
                                if( !CMcmpnocase( yorntmp, yesWord ) )
                		exec frs putrow :instFormName dist_tbl 
				    :rownum ( inst_yorn = :noWord ); 
				}
				}
			}
			else
			{
				if( !CMcmpnocase( yorn, noWord ) )
					STcopy( noWord, yorn );
				else
				{
					IIUGerr( E_ST0130_enter_y_or_n, 0, 0 );
					/* save the response */
					STcopy( pkg->tag, yorn );
					resumeflag = FALSE;
				}
			}
		}
		else if( ip_setup( ipkg, _CHECK ) )
			STcopy( ERget( F_ST0137_no_setup_tag ), yorn );
		else
			STcopy( ERget( F_ST0136_installed_tag ), yorn );

		if( pkg != NULL )
			/* save the response */
			STcopy( yorn, pkg->tag );

		exec frs putrow :instFormName dist_tbl ( inst_yorn = :yorn );

                /*
                ** Conditionally display "Install" menu choice.
                */
                test = FALSE;
                exec frs unloadtable :instFormName dist_tbl ( :yorn = inst_yorn);
                exec frs begin;
                        if( !STcompare( yorn, yesWord ) )
                        {
                                test = TRUE;
                                break;
                        }
                exec frs end;
                if( test )
                {
                        exec frs set_frs menu '' (
                                active( :ERget( F_ST0100_install_menuitem ) )
                                        = 1
                        );
			if (ingres_menu) {
                        exec frs set_frs menu '' (
                                active( :ERget( F_ST010D_express_menuitem ) )
                                        = 0
                        );
			}
                }
                else
                {
                        exec frs set_frs menu '' (
                                active( :ERget( F_ST0100_install_menuitem ) )
                                        = 0
                        );
			if (ingres_menu) {
                        exec frs set_frs menu '' (
                                active( :ERget( F_ST010D_express_menuitem ) )
                                        = 0
                        );
			}
                }
 

		exec frs redisplay;
	
		if( resumeflag )
			exec frs resume next;
		else
			exec frs resume;
	}
	exec frs end;
}


/*
** Resolve a "need" dependency.  pkg1 has a dependency on pkg2, but pkg2
** is not installed and has not been selected for installation.  Give the
** user the options.
*/

bool
ip_dep_popup( PKGBLK *pkg1, PKGBLK *pkg2, char *oldvers )
{
    char lpstr[256];
    char msg[1024];
    char option[ MAX_MANIFEST_LINE ];
    i4  pick;

    bool rtn = TRUE;

    if (batchMode)
    {
	SIprintf(ERget(S_ST017C_dep_prompt), pkg2->name, pkg1->name);
	pick = 0;
    }
    else
    {
	if (oldvers == NULL)
	    STprintf(msg,ERget(S_ST0162_dep_prompt),pkg1->name,pkg2->name);
	else
	    STprintf(msg,ERget(S_ST0176_dep_prompt),pkg1->name,pkg2->name,
		     oldvers);

	STprintf(lpstr, ERget(S_ST0163_select), pkg2->name);
	STcat(lpstr, ERx("\n"));  

	STprintf(option, ERget(S_ST0164_deselect), pkg1->name);
	STcat(lpstr, option);
	STcat(lpstr, ERx("\n"));  

	pick = IIFDlpListPick( msg, lpstr, 0, -1, -1, NULL, NULL, NULL, NULL );
	exec frs redisplay;
    }

    switch (pick)
    {
	case 0:
	    pkg2->selected = SELECTED;
	    break;
	case 1: 
	    pkg1->selected = NOT_SELECTED;
	    break;
	default:
	    rtn = FALSE;
	    break;
    }

    return rtn;
}

/*
**  Popup to inform user what's about to happen in the installation, and
**  get confirmation that s/he wants to continue the process.  Shows the
**  number of requested packages, the number of extra packages being loaded
**  to resolve dependencies, and the total disk footprint.
**
**  This functionality will need to go into ipcl.c and be ported for
**  other environments.
*/

static bool
install_popup()
{
	PKGBLK *pkg;
	char msg[ 1920 ];
	char size[ 6 ], isize[ 6 ], tsize[ 6 ];
	bool rtn;
	i4 n_pkg = 0, n_incl_pkg = 0, pkg_size = 0, incl_size = 0;
	i4 tmp_size = 0;
	LISTELE *lp;
	char 	*chkspccom,*cp;
	char	cpbuf[MAX_LOC +1];
	LOCATION	loc;
	char command[200];
	CL_ERR_DESC	cl_err;
	

	for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
	{
		pkg = (PKGBLK *) lp->car;

		if( pkg->selected && pkg->image_size > tmp_size )
			tmp_size = pkg->image_size;

		switch( pkg->selected )
		{
# ifdef DEBUG_ESTIMATE_SIZE
exec sql begin declare section;
char msg[ 100 ];
exec sql end declare section;
# endif /* DEBUG_ESTIMATE_SIZE */
			case SELECTED:
# ifdef DEBUG_ESTIMATE_SIZE
STprintf( msg, "Adding %s (selected) size (%d).\n", pkg->feature,
	pkg->actual_size );
exec frs message :msg with style = popup;
# endif /* DEBUG_ESTIMATE_SIZE */
				n_pkg++;
				pkg_size += pkg->actual_size;
				break;
			
			case INCLUDED:
# ifdef DEBUG_ESTIMATE_SIZE
STprintf( msg, "Adding %s (included) size (%d).\n", pkg->feature,
	pkg->actual_size );
exec frs message :msg with style = popup;
# endif /* DEBUG_ESTIMATE_SIZE */
				n_incl_pkg++;
				incl_size += pkg->actual_size;
				break;
		}
	}

# ifdef DEBUG_ESTIMATE_SIZE
STprintf( msg, "Temporary space needed: (%d)\n", tmp_size );
exec frs message :msg with style = popup;
# endif /* DEBUG_ESTIMATE_SIZE */
	/* if no included packages, include scratch in pkg_size */ 
	if( n_incl_pkg == 0 )
		pkg_size += tmp_size;

	format_size( size, pkg_size );
	STprintf( msg, ERget(S_ST0167_install_popup_1 ), n_pkg, size );

	if( n_incl_pkg > 0 )
	{
		pkg_size += incl_size;
		format_size( size,  incl_size );
		format_size( isize, pkg_size + tmp_size );
		format_size( tsize, tmp_size );
		STprintf( &msg[ STlength( msg ) ],
			ERget( S_ST0168_install_popup_2 ), 
			n_incl_pkg, size, tsize, n_pkg+n_incl_pkg, isize );
	}

	if( batchMode )   /* ...just dump it to the terminal. */
	{
		SIfprintf( batchOutf, ERx( "\n%s\n" ), msg );
		return TRUE;
	}

	STcat( msg, ERget( S_ST0169_install_popup_3 ) );

	rtn = (0 == ip_listpick( msg, F_ST0103_yes, S_ST0119_continue_inst,
		     F_ST0104_no,  S_ST011A_abort_inst ) );

	exec frs redisplay;
	return rtn;
}


/*
**  do_preload()
**
**  Run all pre-transfer programs that are required, and return a flag
**  indicating whether they all ran ok or not.
*/

static bool
do_preload()
{
    bool flag = FALSE;
    PKGBLK *pkg;
    LISTELE *lp;

    exec sql begin declare section;
    char name[ MAX_MANIFEST_LINE ];
    char msg[ 1024 ];
    exec sql end declare section;

    /*  First, let's see if there's anything we need to do here... */

    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
    {
	pkg = (PKGBLK *) lp->car;

        if (pkg->selected != NOT_SELECTED && ip_preload(pkg,FALSE))
	{
	    flag = TRUE;
	    break;
	}
    }

    if (!flag)  /* There aren't any preloads, so everything's just fine */
	return TRUE;

    /*  Ok, there's a reason to be here.  Scan the list again... */

    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
    {
	pkg = (PKGBLK *) lp->car;

	if (pkg->selected != NOT_SELECTED && !ip_preload(pkg,TRUE))
	{
	    ip_forms( TRUE );
	    STprintf( msg, ERget( S_ST0131_preload_failed ), pkg->name );
	    if ( batchMode || 1 > ip_listpick( msg, F_ST0104_no, S_ST0133_dont_transfer,
				     F_ST0103_yes, S_ST0132_do_transfer ) )
	    {
		pkg->state = DESELECTED;
		IPCLcleanup( pkg );
		flag = FALSE;
	    }
	}
    }

    ip_forms( TRUE );  /* We may have gone non-forms, so fix that. */
    if( flag )	     /* Everything go ok? */
	return TRUE; /* If so, we're done. */

    /*  
    **  Apparently something went wrong, so let's deselect things that we won't
    **  be able to finish transferring... 
    */

    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
    {
	pkg = (PKGBLK *) lp->car;

        if (pkg->selected == DESELECTED)
	{
	    pkg->selected = NOT_SELECTED;
	    exec frs unloadtable :instFormName dist_tbl (:name = package);
	    exec frs begin;
		if( ip_compare_name( name, pkg ) )
		{
		    exec frs putrow :instFormName dist_tbl (inst_yorn = :noWord);
		    exec frs endloop;
		}
	    exec frs end;
	}
    }

    exec frs redisplay;
    return FALSE;
}

/*
**  do_install() -- install a single package, or all selected packages.
**
**  This is the top-level handler for the "Install" menuitem in forms mode.
*/

static void
do_install( char *pkgname )
{
    exec sql begin declare section;
    char name[ MAX_MANIFEST_LINE ], yorn[10];
    char msg[ 1024 ];
    exec sql end declare section;
    PKGBLK *pkg, *ipkg;
    LISTELE *lp1, *lp2;

    /*  If Ingres appears to be running right now, complain... */

    if (ip_sysstat() == IP_SYSUP) 
    {
	STprintf(msg,ERget(S_ST0109_installation_is_up),
				SystemProductName, installDir);
	if ( 1 > ip_listpick(msg, S_ST010B_abort, S_ST011A_abort_inst,
			       S_ST010A_continue, S_ST0119_continue_inst) )
	    return;
    }

    /*  
    **  Mark everything we're going to install as "selected", or at any rate
    **  as something other than NOT_SELECTED.
    */

    for( lp1 = distPkgList.head; lp1 != NULL; lp1 = lp1->cdr )
    {
	pkg = (PKGBLK *) lp1->car;
	pkg->selected = NOT_SELECTED;
    }

    exec frs unloadtable :instFormName dist_tbl (
	:name = package, :yorn = inst_yorn
    );
    exec frs begin;
	if( NULL != (pkg = ip_find_package( name, DISTRIBUTION )) )
	{
	    if( (pkgname == NULL && !STcompare( yorn, yesWord ) ) ||
		 (pkgname != NULL && !STcompare( name, pkgname )) )
	    {
		pkg->selected = SELECTED;
                if( !STbcompare( pkg->name, STlength( pkg->name),   
                                 "Full Installation", 17, TRUE ) )
                     Install_Fullpkg = TRUE; 
	    }
	}
    exec frs end;

    /*  
    **  If there are things that we've just been told to install which we
    **  think might already be installed, give the user a chance to bail out
    **  before re-installing.
    */
    for( lp1 = instPkgList.head; lp1 != NULL; lp1 = lp1->cdr )
    {
	ipkg = (PKGBLK *) lp1->car;

	for( lp2 = distPkgList.head; lp2 != NULL; lp2 = lp2->cdr )
	{
	    pkg = (PKGBLK *) lp2->car;

	    if ( pkg->selected != NOT_SELECTED &&
		 !STbcompare(ipkg->feature, 0, pkg->feature, 0, TRUE) &&
		 !STcompare(ipkg->version, pkg->version) &&
		 ( ipkg->state == INSTALLED || ipkg->state == NOT_SETUP) )
	    {
		STprintf( msg, ERget( S_ST015F_pkg_is_installed ),
			 pkg->name, pkg->version );
		if( 1 > ip_listpick( msg, F_ST0104_no, S_ST0161_dont_install,
				      F_ST0103_yes, S_ST0160_do_install ) )
		    pkg->selected = NOT_SELECTED;
		exec frs redisplay;
	    }
	}
    }

    /*  Call ip_install to do the real work... */

    ip_install( pkgname );
}

/*
**  ip_install() -- REALLY install a single package, or all selected packages.
**
**  This is the top level of the installation process for both forms and
**  batch mode.  By the time we get here, we've marked all packages the
**  user has requested as SELECTED, but we haven't dealt with dependencies
**  yet.  This routine deals with everything from moving the packages 
**  through running setup routines.
**	18-mar-1996 (angusm)
**		Add call to ip_overlay for 'patch' packages: will walk
**		thru list for package and update in-memory list
**		for installation with new values for SIZE/CHECKSUM
**	11-dec-2001 (gupsh01)
**		For mkresponse mode close rSTREAM and set to NULL.
*/

void
ip_install( char *pkgname )
{
    PKGBLK *pkg, *ipkg;
    bool flag, preload_errs;
    i4  setup_state, i;
    char msg[ 1024 ];
    LISTELE *lp;
    PID pid;
    CL_ERR_DESC cl_err;
    STATUS status;
#if defined(axp_osf)
    char  name_buf[MAX_LOC+1], *ing_loc ; 
    char  cmd[MAX_LOC+1]  ; 
    struct stat stats; 
#endif

    ver_err_cnt = 0;  /* No errors yet */

    /* resolve dependencies */
    if( !ip_resolve( pkgname ) )
	return;
    /* 
    ** If option "-all" is specified in batch mode, and it   
    ** is not a upgrade install, the ice and ice64 packages
    ** will not be selected.
    */
    if( ( batch_opALL && batchMode  && (!Install_Ice ) ) || Install_Fullpkg ) 
     {
      if( (ipkg = ip_find_package( "ice64", INSTALLATION ) ) == NULL )
      {
         if( (pkg = ip_find_package( "ice64", DISTRIBUTION ) ) != NULL )
	      pkg->selected = NOT_SELECTED ;  
      }	 
	 
      if( (ipkg = ip_find_package( "ice", INSTALLATION ) ) == NULL )
      {
         if( (pkg = ip_find_package( "ice", DISTRIBUTION ) ) != NULL )
	      pkg->selected = NOT_SELECTED ;  
      }	 
    }

    /* give the user one last chance to bail out... */
    if( !install_popup() )
	return;

#if defined(axp_osf)
  /* Need to change the read only permission 
  ** for $II_SYSTEM/ingres/file/raat.h file if it is
  ** a upgrade from 2.6/SP2 installation. Star #14394628. 
  */
    status =  NMpathIng( &ing_loc );
    if( ing_loc && status == OK  )
     {
       STprintf( name_buf, ERx("%s/files/%s"), ing_loc, "raat.h" ); 
       if ((stat(name_buf, &stats) == 0) && !(stats.st_mode & S_IWUSR))
        {
          STprintf( cmd, ERx("chmod 644 %s" ), name_buf );   
          system( cmd ); 
        }
      }
#endif

    if ( (!batchMode) &&  ( ( pid = PCfork( &status ) ) == 0 ) )
    {
	please_wait();
	PCexit( TRUE );
    }

    /**********  File transfer **********/

    /* 
    ** In case this is response mode then we just 
    ** write the information about selected packages
    ** to the response file 
    */

    if (mkresponse)
    {
       SCAN_LIST( distPkgList, pkg )
       {
          if( pkg->selected != NOT_SELECTED)
          {  
	       /* check and add the package to the response file */
 	       ip_chk_response_pkg (pkg->feature);
           }
        }
    }

    if( IPCLtransfer( &distPkgList, installDir ) )
    {
    	if( !batchMode )
		PCforce_exit( pid,  &cl_err );
	IIUGerr( E_ST0108_install_failed, 0, 1, SystemAdminUser);
	return;
    }

    /**********  Archive verification **********/


    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
    {
	pkg = (PKGBLK *) lp->car;

        if (pkg->selected != NOT_SELECTED)
	{
            exec sql begin declare section;
            i4  nrow;
            char name[ MAX_MANIFEST_LINE ];
            exec sql end declare section;
  
            /* highlight the package currently being installed*/
	    if (!batchMode)
	    {
            	exec frs unloadtable :instFormName dist_tbl 
                    (:name = package, :nrow = _record);
            	exec frs begin;
                    if( ip_compare_name( name, pkg ) )
                    {
                    	exec frs scroll :instFormName dist_tbl to :nrow;
		    	exec frs endloop;
                    };
            	exec frs end;
	    	exec frs redisplay;
	    }
  
	    /* verify package archive */
	    if( IPCLverify( pkg ) )
	    {
		++ver_err_cnt;;
		if (ver_err_cnt > 0)   /* Does the user want to bail out? */
    	        {
			if( !batchMode )
				PCforce_exit( pid,  &cl_err );
		    	return;
		}
		break;
	    }
	}
    }

    if (ver_err_cnt)  /* Were there any verification errors? */
    {
	if (batchMode)/* In batch mode, that's all she wrote. */
	    return;

	/* In forms mode, see what the user wants to do about it... */

	STcopy(ERget(S_ST012F_ver_error_prompt), msg);
	if ( !eXpress )
		PCforce_exit( pid,  &cl_err );
	if (  1 > ip_listpick(msg, S_ST010B_abort, S_ST011A_abort_inst,
	    		S_ST010A_continue, S_ST0119_continue_inst) )
	{
	    return;
	}

    }

    /********** Run pre-transfer programs **********/

    /* 
    ** Run any pre-transfer programs, and keep track of whether there
    ** were any problems running them... 
    */

    preload_errs = !do_preload();

    /********** Preservation of customizable files **********/

    if( ip_preserve_cust_files() != OK )
    {
	IIUGerr( E_ST0181_preserve_failed, 0, 0 );
	for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
	{
	    pkg = (PKGBLK *) lp->car;
	    if (pkg->selected != NOT_SELECTED)
		IPCLcleanup( pkg );
	}
	if( !batchMode )
		PCforce_exit( pid,  &cl_err );
	return;
    }

    /********** File installation **********/

    /*
    ** if (rSTREAM)
    **    (void) SIclose( rSTREAM ); 
    */

    if (mkresponse && rSTREAM)
    {
      ip_close_response( rSTREAM);
      rSTREAM = NULL;
    }

    /* If the system is running, complain... */

    if (batchMode) 
    {
	flag = (ip_sysstat() == IP_SYSUP);
	if (flag)
	    IIUGerr(E_ST0178_installation_is_up, 0, 1, installDir);
    }
    else
    {
	/*  In forms mode, let the user say what to do if it's running... */
	switch (ip_sysstat())
	{
	    case IP_NOSYS:
		flag = FALSE;
		break;
	    case IP_SYSUP:
		PCforce_exit( pid,  &cl_err );
		STprintf(msg,ERget(S_ST010C_installation_still_up),
				SystemProductName, installDir, 
				SystemProductName);
		flag = (1 > ip_listpick(msg,S_ST010B_abort,S_ST011A_abort_inst,
				    S_ST010A_continue,S_ST0119_continue_inst));
    		if ( (!batchMode) &&  ( ( pid = PCfork( &status ) ) == 0 ) )
    		{
        		please_wait();
        		PCexit( TRUE );
    		}
		break;
	    case IP_SYSDOWN:
		PCforce_exit( pid,  &cl_err );
		STprintf(msg,ERget(S_ST010D_installation_is_down),
			SystemProductName, installDir);
		flag = (0 != ip_listpick(msg, 
				    S_ST010A_continue, S_ST0119_continue_inst,
				    S_ST010B_abort, S_ST011A_abort_inst));
    		if ( (!batchMode) &&  ( ( pid = PCfork( &status ) ) == 0 ) )
    		{
        		please_wait();
        		PCexit( TRUE );
    		}
		break;
	}
    }

    if ( flag )
    {
	if( !batchMode )
		PCforce_exit( pid,  &cl_err );
 		exec frs clear screen;
 		exec frs redisplay;
	return;
    }

    /* 
    ** Move all the packages to the installation area.  This is the
    ** definitive act that makes a package "current".  
    */

    for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
    {
	pkg = (PKGBLK *) lp->car;

        if( pkg->selected != NOT_SELECTED )
	{
            exec sql begin declare section;
            i4  nrow;
            char name[ MAX_MANIFEST_LINE ];
            exec sql end declare section;
  
            /* highlight the package that is currently being installed */
	    if (!batchMode)
	    {
            	exec frs unloadtable :instFormName dist_tbl 
                    (:name = package, :nrow = _record);
            	exec frs begin;
                    if( ip_compare_name( name, pkg ) )
                    {
                    	exec frs scroll :instFormName dist_tbl to :nrow;
		    	exec frs endloop;
                    };
            	exec frs end;
	    	exec frs redisplay;
	    }

	    /* Special case for Upgrade installs to Ingres 2006 r3 */
	    /* netu was moved to a directory of the same name and  */
	    /* some pax's will not overwrite a file with a         */
	    /* directory, so the old netu file must be deleted     */
	    /* before the new netu/netu dir and file is created.   */
	    /* The netu file is always assumed to be in the net package. */
	    /* This code can be removed when it is no longer       */
	    /* possible to upgrade from a release older than       */
	    /* Ingres 2006 r3.                                     */
	    if( STcompare(pkg->feature, "net") == 0 )
	    {
		char	*cp2;
		char        cp2buf[MAX_LOC + 1];
		LOCATION    loc;
		i4  	flagword;
		LOINFORMATION loinfo;

		NMgtAt(SystemLocationVariable, &cp2);
		STlcopy(cp2, cp2buf, sizeof(cp2buf)-20-1);
		LOfroms(PATH, cp2buf, &loc);
		LOfaddpath(&loc, SystemLocationSubdirectory, &loc);
		LOfaddpath(&loc, ERx("sig"), &loc);
		LOfstfile("netu", &loc);
		flagword = LO_I_TYPE;
		if (LOinfo(&loc, &flagword, &loinfo) == OK)
		{
		    if( (flagword & LO_I_TYPE)  &&
		        (loinfo.li_type == LO_IS_FILE) )
		    {
			/* Detecting an error here is not important because */
			/* when extracting the new file this may not be a   */
			/* problem on some platforms. */
			LOdelete(&loc);
		    }
		}
	    }

	    if( IPCLinstall( pkg ) == OK )
	    {
		/* The following should NOT return NULL! */ 
		if( (ipkg = ip_find_package( pkg->feature, INSTALLATION ) )
		    != NULL )
	    	{
 		    if( ip_setup( ipkg, _CHECK ) )
		    	ipkg->state = NOT_SETUP;
	    	}

		/*
		** for patch: overlay SIZE/CHECKSUM values
		** with values from new files 
		*/
		if ( (pmode == TRUE) && 
			 (STbcompare(pkg->name, 5, "patch", 5, TRUE)) == 0)
			ip_overlay(pkg, PATCHIN);
# ifdef OMIT
		else
		    pkg->state = NOT_SETUP;
# endif /* OMIT */
	    }
	    else
	    {
		IIUGerr( E_ST013F_cant_transfer_pkg, 0, 1, pkg->name );
		pkg->state = NOT_INSTALLED; /* De-select it for installation */
		if( !batchMode )
		{
		    exec frs putrow :instFormName dist_tbl (
			inst_yorn = :noWord
		    );
		    exec frs redisplay;
		}
	    }
	    IPCLcleanup( pkg );
	}
    }

    ip_list_assign_string( &instRelID, distRelID );

    if (!batchMode)
 	exec frs redisplay;
    write_inst_descr();
    if (!batchMode)
    	exec frs redisplay;

    /**********  Setup phase  **********/

    /* 
    ** See if there are any setup or upgrade programs to be run.  If not, we
    ** won't prompt the user to see if he wants to run the setup phase. 
    */

/* States of the "state machine"... */

# define INIT_SETUP 0
# define SKIPPING_SETUP 1
# define DOING_SETUP 2
# define ABORT_PENDING 3
# define ERRORS_IN_SETUP 4

    if( noSetup )
	setup_state = SKIPPING_SETUP;
    else
	setup_state = INIT_SETUP;

    /*
    ** Setup all products which need it.
    */
    for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
    {
	pkg = (PKGBLK *) lp->car;

	if (setup_state == SKIPPING_SETUP)
	    break;  /* If we're blowing off setup, just exit the loop */

        if( pkg->state == NEWLY_INSTALLED || pkg->state == NOT_SETUP )
        {   	
	    switch (setup_state)
	    {
		case INIT_SETUP:
		    /* First candidate for setup */
		    if (!eXpress)
            	    {
		        if ( !batchMode )
			    PCforce_exit( pid,  &cl_err );
		    	STcopy(ERget(S_ST0127_ver_ok_prompt), msg);
		        setup_state = batchMode ? DOING_SETUP :
			  ip_listpick( msg, F_ST0103_yes, F_ST0128_do_setup,
			  F_ST0104_no, F_ST0129_dont_do_setup ) ?
			  SKIPPING_SETUP : DOING_SETUP;
		    }
		    else
			setup_state = DOING_SETUP;
		    break;

		case ABORT_PENDING:
		    /* Prompt whether to continue setting up */ 
		    if( instPkgList.curr->cdr != NULL )
		    {
		        if ( !batchMode )
			    PCforce_exit( pid,  &cl_err );
			STcopy(ERget(S_ST0125_continue_setup_prompt), msg);
			setup_state = batchMode ? ERRORS_IN_SETUP :
		            ip_listpick( msg, F_ST0103_yes,
			    S_ST0183_continue_setup, F_ST0104_no,
			    S_ST0184_abort_setup ) ? SKIPPING_SETUP :
			    ERRORS_IN_SETUP;
		    }
		    else
			setup_state = SKIPPING_SETUP;
		    break;
	    }

	    if( setup_state != SKIPPING_SETUP )  /* Still gonna do it?*/
	    {
		if( ( !batchMode ) && ( !eXpress ) )
				PCforce_exit( pid,  &cl_err );
		pkg->state = ip_setup( pkg, _PERFORM ) ? INSTALLED: NOT_SETUP;

                /* Mark the new state on the "current installation" list */
		if( NULL != (ipkg = ip_find_package( pkg->name,
		    INSTALLATION )) )
		{
		    ipkg->state = pkg->state;
		}

		if( pkg->state == NOT_SETUP )    /* Anything go wrong? */
		    setup_state = ABORT_PENDING;/* Then we might want to quit */
	    }
	}
    }

    ip_forms( TRUE );  /* Make sure we end up in forms mode. */

    /*  Update the "state" field on the form for everything we've installed.  */

    if (!batchMode)
    {
	exec sql begin declare section;
	i4 nrow, firstrow;
	char name[ MAX_MANIFEST_LINE ], *yorn;
	exec sql end declare section;

	firstrow = -1;
	exec frs unloadtable :instFormName dist_tbl (
		:name = package, :nrow = _record
	);
	exec frs begin;

	    if( (pkg = ip_find_package( name, DISTRIBUTION )) == NULL ) 
		continue; /* this shouldn't happen */

	    if( (ipkg = ip_find_package( name, INSTALLATION )) != NULL &&
		STequal( ipkg->version, pkg->version ) )
	    {
		if( ipkg->state == UNDEFINED )
			yorn = ERx( "?" );
		else if( ip_setup( ipkg, _CHECK ) )
			yorn = ERget( F_ST0137_no_setup_tag );
		else
			yorn = ERget( F_ST0136_installed_tag );

		exec frs putrow :instFormName dist_tbl ( inst_yorn = :yorn );

		if (firstrow == -1)
		    firstrow = nrow;
	    }

	exec frs end;

	if (firstrow != -1)
	{
	    exec frs scroll :instFormName dist_tbl to :firstrow;
	    exec frs redisplay;
	}
    }

    /*  End of setup phase; let the user know how things went. */

    if( !batchMode )
	PCforce_exit( pid,  &cl_err );

    switch( setup_state )
    {
	case SKIPPING_SETUP:
	    if ( *authString != EOS )
	    {
		/* No setup was done, so run the "iicnffiles" script 	*/
		/* to create symbol.tbl & config.dat and store   	*/
		/* the authorization string as an ING attribute 	*/
		char 	 *cp;
		LOCATION loc;
		char 	 locBuf[ MAX_LOC + 1 ];

		NMgtAt( SystemLocationVariable, &cp );
		STlcopy( cp, locBuf, sizeof(locBuf)-30-1 );
		IPCL_LOfroms( PATH, locBuf, &loc );
		LOfaddpath(&loc, SystemLocationSubdirectory, &loc);
		LOfaddpath(&loc, ERx("utility"), &loc);
		LOfaddpath(&loc, ERx("iicnffiles"), &loc);
		LOtos( &loc, &cp );
		(void) ip_cmdline( cp, S_ST0314_bad_config_write);
		NMstIngAt( ERx( "II_AUTH_STRING" ), authString );
	    }
	    if( batchMode )
		SIprintf( "\n" );
	    ip_display_msg(ERget(S_ST012A_no_setup_now));
	    if( batchMode )
		SIprintf( "\n" );
	    break;

	case DOING_SETUP:
	case INIT_SETUP:
	    /* (Never found anything that needed it) */
	    ip_display_msg( preload_errs ? ERget( S_ST011E_setup_ok ) :
		ERget( S_ST012B_setup_ok )
	    );
	    break;

	case ERRORS_IN_SETUP:
	case ABORT_PENDING:
	    IIUGerr(E_ST012C_setup_failed,0,0);
	    break;
    }

    write_inst_descr();  /* Write out the installation descriptor file. */

}


/*  
**  current_frame() -- top level of the "Current installation" form.
**
**  History:
**	01-27-95 (forky01)
**	    Fix bug 63176 - Make sure all packages which need setup are
**	    correctly marked for readiness in current frame.
*/

static void
current_frame()
{
	PKGBLK *ipkg;
	char msg[ 1024 ];
	LISTELE *lp;

	exec sql begin declare section;
	char name[ MAX_MANIFEST_LINE ];
	exec sql end declare section;

	exec frs inittable currform inst_tbl read;
	exec frs display currform;

   	exec frs initialize (
                ii_build_name = :ii_build_name,
                ii_system_name = :ii_system_name
        );

	exec frs begin;
	{
		exec frs putform currform ( curr_inst = :installDir );
		refresh_currform();
	}
	exec frs end;

	exec frs activate menuitem :ERget( F_ST0105_setup_all_menuitem );
	exec frs begin;
	{
		bool write_needed, did_setup;

		exec sql begin declare section;
		i4 nrow, firstrow;
		exec sql end declare section;

		/* Scan tablefield for anything that requires setup... */

		did_setup = FALSE;
		exec frs unloadtable currform inst_tbl ( :name = package );
		exec frs begin;
			if ( NULL != (ipkg = ip_find_package( name,
				INSTALLATION )) && ip_setup( ipkg, _CHECK ) )
			{
				did_setup = TRUE;
				if( ip_setup( ipkg, _PERFORM ) )
					ipkg->state = NEWLY_INSTALLED;
				else
				{
					STcopy( ERget( S_ST0125_continue_setup_prompt),
						msg);
	   				if( ip_listpick( msg,
						F_ST0103_yes,
						S_ST0183_continue_setup,
						F_ST0104_no,
						S_ST0184_abort_setup ) )
					{
						/* no more setup */
						break;
					}
				}
			}
		exec frs end;

		ip_forms( TRUE );

		write_needed = FALSE;
		for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
		{
			ipkg = (PKGBLK *) lp->car;

			if( ipkg->state == NEWLY_INSTALLED )
			{
				ipkg->state = INSTALLED;
				write_needed = TRUE;
			}
		}
		if ( write_needed )
		{
			exec frs clear field inst_tbl;
			refresh_currform();
			write_inst_descr();
		}

		if( !did_setup )
			IIUGerr( E_ST0143_no_pkgs_req_setup, 0, 0 );

		exec frs resume column inst_tbl status;
	}
	exec frs end;

	exec frs activate menuitem :ERget( F_ST0102_setup_menuitem );
	exec frs begin;
	{
		bool setup_ok;

		exec frs getrow currform inst_tbl ( :name = package );
		if (NULL == (ipkg = ip_find_package( name, INSTALLATION )) )
			exec frs resume menu;

		if( !ip_setup( ipkg, _CHECK ) )
		{
			STprintf( msg, ERget( S_ST0146_no_setup_reqd ),
				ipkg->name );
			ip_display_msg( msg );
		}
		else 
		{
			setup_ok = ip_setup( ipkg, _PERFORM );
			ip_forms( TRUE );
			if( setup_ok ) 
			{
				exec frs putrow currform inst_tbl (
					status = :ERget( F_ST0103_yes )
				);
				write_inst_descr();
			}
		}

		exec frs resume column inst_tbl status;
	}
	exec frs end;

	exec frs activate menuitem :ERget(F_ST0107_verify_menuitem);
	exec frs begin;
	{
		ver_err_cnt = 0;

		for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
		{
			ipkg = (PKGBLK *) lp->car;
			if( (ipkg->state == INSTALLED ||
				ipkg->state == NOT_SETUP) &&
				( !ignoreChecksum ) &&
				ip_verify_files( ipkg ) != OK )
			{
				break;	
			}
		}
		ip_display_msg( ver_err_cnt ?
			ERget( S_ST015C_verification_failed ) :
			ERget( S_ST015D_verification_ok ) );
	}
	exec frs end;


	exec frs activate menuitem :ERget( F_ST0106_describe_menuitem );
	exec frs begin;
	{
		exec sql begin declare section;
		char pkgname[ MAX_MANIFEST_LINE ];
		exec sql end declare section;

		exec frs getrow currform inst_tbl ( :pkgname = package );

		ip_describe(pkgname);

		exec frs resume column inst_tbl status;
	}
	exec frs end;


	exec frs activate menuitem :ERget( F_ST010B_delete_menuitem );
	exec frs begin;
	{
		bool rtn;

		exec frs getrow currform inst_tbl (:name = package);

		if (NULL != (ipkg = ip_find_package(name, INSTALLATION)))
		{
			STprintf(msg, ERget(S_ST0147_really_delete),
				ipkg->name);
			if( 1 > ip_listpick( msg, F_ST0104_no,
				F_ST0148_dont_delete, F_ST0103_yes,
				F_ST0149_delete) )
			{
				exec frs resume column inst_tbl status;
			}

			if( ip_sysstat() == IP_SYSUP ) 
			{
				STprintf( msg,
					ERget( S_ST014B_installation_is_up ),
					SystemProductName, installDir, 
					ipkg->name );
				if( 1 > ip_listpick( msg, F_ST0104_no,
					F_ST0148_dont_delete, F_ST0103_yes,
					F_ST0149_delete) )
				{
					exec frs resume column inst_tbl status;
				}
			}

			ip_display_transient_msg(
				ERget( S_ST014C_deleting_package ) );
			
			rtn = ip_delete_inst_pkg( ipkg, NULL );

			if( rtn || ipkg->state == UNDEFINED )
			{
				exec frs clear field inst_tbl;
				refresh_currform();
				write_inst_descr();
				if( !rtn )
				{
					IIUGerr( E_ST014D_delete_errs, 0, 3, 
				 		ipkg->name, userName,
						installDir );
				}
			}

			if (ip_list_is_empty(&instPkgList))
			{
				ip_display_msg( ERget( S_ST0165_no_more_pkgs ) );
				exec frs breakdisplay;
			}

		}

		exec frs resume column inst_tbl status;
	}
	exec frs end;


    	exec frs activate menuitem :ERget( F_ST0108_help_menuitem )
		( activate = 0 ), frskey1 ( activate = 0 );
    	exec frs begin;
        {
		if(ingres_menu)
		{
			ip_lochelp( "currform.hlp", "Ingbuild/Current" );
		}
		else
		ip_lochelp( "currform.hlp", "Jasbuild/Current" );
	}
	exec frs end;


	exec frs activate menuitem :ERget( F_ST0109_end_menuitem )
		( activate = 0 ), frskey3 ( activate = 0 );
	exec frs begin;
		exec frs breakdisplay;
	exec frs end;

	exec frs activate before column inst_tbl all;
	exec frs begin;
	{
		bool setup_needed = FALSE;
		exec sql begin declare section;
		i4 rows;
		exec sql end declare section;

		exec frs inquire_frs table currform (
			:rows = lastrow
		);

		if( rows < 1 )
			exec frs resume;

		for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
		{
			ipkg = (PKGBLK *) lp->car;
			if( ip_is_visible( ipkg ) && ip_setup( ipkg, _CHECK ) )
			{
				setup_needed = TRUE;
				break;
			}
		}
		if( setup_needed )
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0105_setup_all_menuitem ) )
					= 1
			);
		}
		else
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0105_setup_all_menuitem ) )
					= 0
			);
		}

		exec frs getrow currform inst_tbl ( :name = package );

		if( (ipkg = ip_find_package( name, INSTALLATION ) ) == NULL )
			exec frs resume menu;

		/* disable 'Setup' menu item if not relevant */
		if( ip_setup( ipkg, _CHECK ) )
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0102_setup_menuitem ) ) = 1
			);
		}
		else
		{
			exec frs set_frs menu '' (
				active( :ERget( F_ST0102_setup_menuitem ) ) = 0
			);
		}
	}
	exec frs end;
}

/*  ip_init_distribdev -- Initialize global distribution device field.
**
**  Initialize the distribution device to II_DISTRIBUTION if one's
**  available.  If the authorization is predefined, we'll set the global 
**  "distribDevEnv" boolean to indicate distribution device was provided 
**  in the environment.  If there's either no distribution device provided
**  or an invalid distribution device is provided, we'll set distribDevEnv
**  to FALSE so the user will be required to enter a valid distribution 
**  device before doing the installation.
*/

void
ip_init_distribdev()
{
	char *cp;

	NMgtAt( ERx( "II_DISTRIBUTION" ), &cp );
	if( cp && *cp )
	{
		STlcopy( cp, distribDev, sizeof(distribDev)-1 );
		distribDevEnv = IPCLsetDevice( distribDev );
	}
	else
	{
		*distribDev = EOS;
		distribDevEnv = FALSE;
	}
}

#ifndef INGRESII
/*
**  auth_str_ok() -- check validity of authstr_* fields on top form
*/

static bool
auth_str_ok(bool emptyflag)
{
    exec sql begin declare section;
    char auth1[10], auth2[10], auth3[10], auth4[10];

    exec sql end declare section;

    exec frs getform :topFormName ( :auth1 = authstr_1, :auth2 = authstr_2,
		                    :auth3 = authstr_3, :auth4 = authstr_4 );

    if (*auth1 == EOS)
    {
	if (emptyflag)
	    IIUGerr(E_ST0173_authstr_required,0,1,SystemProductName);
	return FALSE;
    }

    compose_auth(auth1, auth2, auth3, auth4);

    if( !ip_verify_auth() )
    {
	IIUGerr(E_ST0174_authstr_bad,0,1,SystemProductName);
	return FALSE;
    }

    return TRUE;
}
#endif /* INGRESII */

/*
**  top_frame()
**
**  Top-level processing for the main form.  
**
**  The main form can actually be one of two forms, depending on whether
**  we're going to have a "Current" option or not, which in turn depends
**  on whether an RELEASE_MANIFEST file is found, describing the current 
**  installation.  The value returned from this function tells the caller
**  whether to recall the frame, which allows us to exit the display loop
**  and re-enter it with another form transparently to the user.
*/

static bool
top_frame()
{
    bool recall = FALSE;
    STATUS status = FAIL;

    exec sql begin declare section;
    char auth1[10], auth2[10], auth3[10], auth4[10];
    char current_menuitem[20];
    char custom_menuitem[20];
    i4  cmd;
    char a4[20];
    exec sql end declare section;
 

    /*
    **  Display main form...
    */
  
    if (ingres_menu)
    {
#ifndef INGRESII
      STcopy("authstr_4", a4);
#else
      *a4=EOS;
#endif /* INGRESII */
      STcopy(ERget(F_ST010C_custom_menuitem), custom_menuitem);
      if (ip_list_is_empty(&instPkgList))
      {
	    STcopy(ERx("ipmainform1"), topFormName);
	*current_menuitem = EOS;
      }
      else
      {
	    STcopy(ERx("ipmainform"), topFormName);
	STcopy(ERget(F_ST0101_current_menuitem), current_menuitem);
      }
    }

    exec frs set_frs frs
	( validate(keys)=0, validate(menu)=0, validate(menuitem)=0,
	  validate(nextfield)=0, validate(previousfield)=0 );
    exec frs set_frs frs
	( activate(keys)=1, activate(menu)=1, activate(menuitem)=1,
	  activate(nextfield)=1, activate(previousfield)=1 );
        
    /*
    **  Beginning of display loop.
    */

    exec frs display :topFormName;

    exec frs initialize;
    exec frs begin;
    {
	exec sql begin declare section;
	char mkprompt_1[ MAX_MANIFEST_LINE ], mkprompt_2[ MAX_MANIFEST_LINE ];
	char keyname[20];
	exec sql end declare section;

	char *p;

	/*
	** Depending on the capabilities of the terminal, we're going
	** to display one of two messages: either "To select a function,
	** press <keyname> and type the name of the function" or "To
	** select a function, press the key whose label appears after the
	** name in parentheses, or press <keyname>...", where <keyname>
	** is the name of the key that moves the cursor to the menu line.
	** This is for the benefit of users who might not understand how
	** FRS screens work.
	*/

        STcopy(ERget(S_ST016C_menu_prompt_a), mkprompt_1);
        exec frs inquire_frs frs (:keyname = label(menu1));
	if (*keyname)
	{
	    STcat(mkprompt_1, ERget(S_ST016D_menu_prompt_b));
	    STcopy(ERget(S_ST016E_menu_prompt_c), mkprompt_2);
	    p = mkprompt_2;
	}
	else
	{
	    *mkprompt_2 = EOS;
	    p = mkprompt_1;
	}

        exec frs inquire_frs frs (:keyname = label(menu));
	STprintf(p+STlength(p), ERget(S_ST016F_menu_prompt_d), keyname);

	exec frs putform (
		dist_dev = :distribDev, root_dir = :installDir, 
		menu_prompt_1 = :mkprompt_1, menu_prompt_2 = :mkprompt_2
	);

	if( *distribDev != EOS )
	    exec frs putform ( dist_dev = :distribDev );

#ifndef INGRESII
        if( ingres_menu )
	{
	  if( *authString == EOS )
	  {
	    /* user must enter an authorization string */
	    exec frs set_frs field :topFormName (
		displayonly( authstr_1 ) = 0, underline( authstr_1 ) = 1,
		displayonly( authstr_2 ) = 0, underline( authstr_2 ) = 1,
		displayonly( authstr_3 ) = 0, underline( authstr_3 ) = 1,
		displayonly( authstr_4 ) = 0, underline( authstr_4 ) = 1 );
	    exec frs resume field authstr_1;
	  }
	  else
	  {
	    /* II_AUTH_STRING is set */ 
	    decompose_auth( auth1, auth2, auth3, auth4 );

	    exec frs putform (
		authstr_1 = :auth1, authstr_2 = :auth2,
		authstr_3 = :auth3, authstr_4 = :auth4
	    );

	    if( ip_verify_auth() )
	    {
	        exec frs set_frs field :topFormName (
		    displayonly( authstr_1 ) = 1, underline( authstr_1 ) = 0,
		    displayonly( authstr_2 ) = 1, underline( authstr_2 ) = 0,
		    displayonly( authstr_3 ) = 1, underline( authstr_3 ) = 0,
		    displayonly( authstr_4 ) = 1, underline( authstr_4 ) = 0
	        );
	    }
	    else
	    {
		exec frs redisplay;
		IIUGerr(E_ST0174_authstr_bad,0,1,SystemProductName);
		*auth1 = *auth2 = *auth3 = *auth4 = EOS;
	    	exec frs putform (
		    authstr_1 = :auth1, authstr_2 = :auth2,
		    authstr_3 = :auth3, authstr_4 = :auth4
	    	);
		exec frs redisplay;
		exec frs resume field authstr_1;
	    }
	  }
        }
#endif /* INGRESII */

	if( *distribDev != EOS )
	{
	    exec frs putform ( dist_dev = :distribDev );
	    if( IPCLsetDevice( distribDev ) )
	    {
		exec frs set_frs field :topFormName ( 
		    displayonly( dist_dev ) = 1,
		    underline( dist_dev ) = 0
		);
	    }
	}
    }
    exec frs end;

    exec frs activate menuitem :custom_menuitem;
    exec frs begin;

        if( verify_installdir( TRUE ) != OK )
            exec frs resume menu;
 
        if( *distribDev == EOS )
        {
            IIUGerr(E_ST0101_device_required,0,0);
            exec frs resume field dist_dev;
        }
        else
        {
            exec frs getform :topFormName ( :distribDev = dist_dev );
            if( !IPCLsetDevice( distribDev ) )
                exec frs resume field dist_dev;
        }
 
#ifndef INGRESII
  	if( ingres_menu )
        {
          if( !auth_str_ok( TRUE ) )
          {
                exec frs resume field authstr_1;
                exec frs resume field dist_dev;
          }
        }
#endif /* INGRESII */
 
                STcopy(ERx("custform"), instFormName);
		custChoice = TRUE;
                install_frame();
        /*  If we now have something in the installation list, but didn't
            before, we now want to display the other form, so we'll break
            out of the display loop after setting the return value to
            something that will get us re-entered.
        */
 
        if( *current_menuitem == EOS && !ip_list_is_empty( &instPkgList ) )
        {
            recall = TRUE;
            exec frs breakdisplay;
        }
 
        exec frs resume menu;
    exec frs end;

    exec frs activate menuitem :ERget(F_ST010E_package_menuitem);
    exec frs begin;

	if( verify_installdir( TRUE ) != OK )
	    exec frs resume menu;

	if( *distribDev == EOS )
	{
	    IIUGerr(E_ST0101_device_required,0,0);
	    exec frs resume field dist_dev;
	}
	else
	{
	    exec frs getform :topFormName ( :distribDev = dist_dev );
	    if( !IPCLsetDevice( distribDev ) )
		exec frs resume field dist_dev;
	}

#ifndef INGRESII
 	if( ingres_menu )
        {
          if( !auth_str_ok( TRUE ) )
          {
                exec frs resume field authstr_1;
                exec frs resume field dist_dev;
          }
        }
#endif /* INGRESII */

		STcopy(ERx("installform"), instFormName);
		custChoice = FALSE;
		install_frame();
	/*  If we now have something in the installation list, but didn't
	    before, we now want to display the other form, so we'll break
	    out of the display loop after setting the return value to
	    something that will get us re-entered.  
        */

	if( *current_menuitem == EOS && !ip_list_is_empty( &instPkgList ) )
	{
	    recall = TRUE;
	    exec frs breakdisplay;
	}

	exec frs resume menu;
    exec frs end;

    exec frs activate menuitem :current_menuitem;
    exec frs begin;
	if (OK != verify_installdir(FALSE))
	    exec frs resume menu;
	     
#ifndef INGRESII
 	if( ingres_menu )
        {
          if( !auth_str_ok( TRUE ) )    /* make sure auth string is OK */
          {
                exec frs resume field authstr_1;
                exec frs resume field dist_dev;
          }
        }
#endif /* INGRESII */

	current_frame();

	if (ip_list_is_empty(&instPkgList))
	{
	    recall = TRUE;
	    exec frs breakdisplay;
	}

	exec frs resume menu;
    exec frs end;

    exec frs activate menuitem :ERget( F_ST0108_help_menuitem )
	( activate = 0 ), frskey1 ( activate = 0 );
    exec frs begin;
        {

	    /* the following code has been removed from inline and placed
	    ** as a new function (ip_lochelp) at the end of this file  **
	    */

            /* LOCATION loc;						*/
            /* char     locBuf[ MAX_LOC + 1 ];				*/
            /* char     *cp;						*/

            /* 
            ** The help files are in a non-standard place during install :
            ** II_FORMFILE is the formfile with the "Keys" help form
            ** II_HELPDIR is for Help on Help 
            */
            /* IPCL_LOfroms( PATH, ip_install_loc(), &loc );		*/
            /* LOfstfile( "rtiforms.fnx", &loc );			*/
            /* LOtos( &loc, &cp );					*/
            /* IPCLsetEnv( ERx("II_FORMFILE"), cp, TRUE );		*/
            /* IPCLsetEnv( ERx("II_HELPDIR"), ip_install_loc(), TRUE ); */

            /* STcopy( ip_install_loc(), locBuf );			*/
  
            /* IPCL_LOfroms( PATH, locBuf, &loc );			*/
            /* LOfstfile( "ingbuild.hlp", &loc );			*/
	    /* LOtos( &loc, &cp );					*/
            /* IIRTfrshelp( cp, "Ingbuild", H_FE, (VOID (*)())NULL);	*/

	    if(ingres_menu)
	    {
		    ip_lochelp( "ingbuild.hlp", "Ingbuild");
	    }

	}
    exec frs end;

    exec frs activate menuitem :ERget( F_ST010A_quit_menuitem )
	( activate = 0 ), frskey2 ( activate = 0 ), frskey3 ( activate = 0 );
    exec frs begin;
	recall = FALSE;
        exec frs breakdisplay;
    exec frs end;
    exec frs activate after field :a4;
    exec frs begin;

      exec frs inquire_frs frs (:cmd = command);
      if (cmd == 5)  /* Previous field */
          exec frs resume next;
      
#ifndef INGRESII
      if (!auth_str_ok(FALSE))
          exec frs resume field authstr_1;
#endif /* INGRESII */

      exec frs resume next;
    exec frs end;

    exec frs activate after field dist_dev;
    exec frs begin;
    {
	exec frs getform :topFormName ( :distribDev = dist_dev );
	exec frs resume next;
    }
    exec frs end;

    return recall;
}


/*
**  refresh_currform() -- display all the information from the "current
**  installation" data list on the "Current" form.
**
**	Make 'patch' packages :
**		always visible in patch mode
**		always invisible other than in 'patch' mode.
*/

static void
refresh_currform()
{
	PKGBLK *ipkg;
	exec sql begin declare section;
	char size[10], state_txt[ MAX_MANIFEST_LINE ];
	char name[ MAX_MANIFEST_LINE ], lic_col[10];
	char *rev;
	exec sql end declare section;
	LISTELE *lp;
	bool	viz;

	for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
	{
		ipkg = (PKGBLK *) lp->car;
	
		viz = ip_is_visible( ipkg );

		/*
		** if invisible, ignore if 
		**		- not in patch mode
		**		- in patch mode && not a patch
		*/
		if ( !viz && !pmode )
			continue;

		if (!viz && pmode &&
		   ((STbcompare(ipkg->name, 5, "patch", 5, TRUE) != 0)) )
			continue;

		/*
		** if visible, and a patch, but not in patch mode
		**	ignore
		*/
		if (viz && !pmode &&
		   ((STbcompare(ipkg->name, 5, "patch", 5, TRUE) == 0)) )
			continue;

		rev = ipkg->version;
		STcopy( ipkg->name, name );

		STcopy( yesWord, lic_col );

		format_size( size, ipkg->apparent_size );

		if( ipkg->state != NOT_SETUP && ipkg->state != INSTALLED &&
			ipkg->state != UNDEFINED )
		{
			STprintf( state_txt, ERx( "??????" ) );
		}
		else if( ip_setup( ipkg, _CHECK ) )
			STcopy( ERget(F_ST0104_no ), state_txt );
		else
			STcopy( ERget(F_ST0103_yes ), state_txt );

#ifdef INGRESII
		exec frs loadtable currform inst_tbl (
			package = :name, rev_level = :rev, size = :size,
			status = :state_txt
		);
#else /* INGRESII */
		exec frs loadtable currform inst_tbl (
			package = :name, rev_level = :rev, size = :size,
			auth_yorn = :lic_col, status = :state_txt
		);
#endif /* INGRESII */
	}
}

/*
**  please_wait() -- puts up the waitform and spins the star forever.
*/
static void
please_wait()
{
 
    exec frs display waitform read with style=popup (startcolumn=2, startrow=22,
		border=none );
 
    exec frs initialize;
    exec frs begin;
        for(;;)
        {
                exec frs putform waitform (star = '-');
                exec frs redisplay;
                exec frs sleep 1;
                exec frs putform waitform (star = '\');
                exec frs redisplay;
                exec frs sleep 1;
                exec frs putform waitform (star = '|');
                exec frs redisplay;
                exec frs sleep 1;
                exec frs putform waitform (star = '/');
                exec frs redisplay;
                exec frs sleep 1;
                exec frs putform waitform (star = '-');
                exec frs redisplay;
                exec frs sleep 1;
                exec frs putform waitform (star = '\');
                exec frs redisplay;
                exec frs sleep 1;
                exec frs putform waitform (star = '|');
                exec frs redisplay;
                exec frs sleep 1;
                exec frs putform waitform (star = '/');
                exec frs redisplay;
                exec frs sleep 1;
        }
    exec frs end;
}

/*
**  write_inst_descr() -- write out an installation descriptor file, based
**  on the current contents of the "current installation" data list.
**
** History:
**
**	11-dec-2001 (gupsh01)
**	    Add check that if we are in response mode then rSTREAM is closed.
**
*/

static void
write_inst_descr()
{
	LOCATION loc;
	char locBuf[ MAX_LOC + 1 ], *cp;

	ip_display_transient_msg( ERget( S_ST0139_writing_pkl ) );

	NMgtAt( SystemLocationVariable, &cp );
	STlcopy( cp, locBuf, sizeof(locBuf)-20-1 );
	IPCL_LOfroms( PATH, locBuf, &loc );
	LOfaddpath( &loc, SystemLocationSubdirectory, &loc );
	LOfaddpath( &loc, ERx( "files" ), &loc );
			
	if( ip_write_manifest( INSTALLATION, locBuf, &instPkgList,
		instRelID, productName ) != OK )
	{
		IIUGerr( E_ST013A_write_error, 0, 0 );
	}
	
	if (mkresponse)
	{
	/* 
        ** if we are running in the mkresponse mode then we should 
	** close the response file if it is still open.
        */
	  if (rSTREAM)
          {
              ip_close_response( rSTREAM);
              rSTREAM=NULL;
          }
	}
}

/*
**  ip_display_transient_msg -- display a one-line message at the bottom
**  of the screen.
*/

void
ip_display_transient_msg(msg)
exec sql begin declare section;
char *msg;
exec sql end declare section;
{
	if( !forms_state )
		SIfprintf( batchOutf, ERx( "%s\n" ), msg );
	else
		exec frs message :msg;
}

/*  ip_display_msg -- display a popup message, which the user must take
**  explicit action to dismiss.
*/

void
ip_display_msg( msg )
exec sql begin declare section;
char *msg;
exec sql end declare section;
{
	bool save_forms_state = forms_state;

	if (batchMode)
		SIfprintf( batchOutf, ERx( "%s\n" ), msg );
	else
	{
		ip_forms( TRUE );
		exec frs message :msg with style = popup;
		ip_forms( save_forms_state );
	}
}


/*  verifiy_installdir() -- make sure the installation directory actually
**  exists; complain, and offer to create it, if it doesn't.
*/

static STATUS
verify_installdir(create)
bool create;
{
    LOCATION install_loc;
    STATUS stat;
    char msg[1024];

    if ( OK == (stat = IPCL_LOfroms( PATH, installDir, &install_loc )) )
    {
	if ( OK != (stat = LOexist(&install_loc)) )
	{
	    if (create)
	    {
		STprintf(msg,ERget(S_ST0121_no_install_dir),installDir);
		if (ip_listpick(msg, F_ST0103_yes, S_ST0122_yes_create,
			             F_ST0104_no, S_ST0123_dont_create))
		    return FAIL;
		if ( OK != (stat = IPCLcreateDir( installDir )) )
		    IIUGerr(E_ST0124_cant_create_instdir,0,2, 
			    installDir, userName);
	    }
	    else
		IIUGerr(E_ST0142_no_inst_directory, 0, 2, installDir, SystemLocationVariable);
	}
    }

    return stat;
}

/*
** ip_forms -- enter or leave forms mode.  Try real hard not to do 
** unnecessary screen clears.
*/

void
ip_forms( bool yesno )
{
	if( forms_state == yesno )
		return;

	if( !batchMode && forms_state != yesno )
	{
		forms_state = yesno;

		TErestore( TE_FORMS );

		exec frs clear screen;

		if( yesno )
			exec frs redisplay;
		else
			TErestore( TE_NORMAL );
	}
}

/*  format_size -- format a size of something (a package, probably) into
**  easily readable form.  Decide whether it's best expressed in bytes,
**  kbytes, megs, gigs, or (heaven forfend) terabytes, and append the
**  appropriate suffix.
*/

static void
format_size( char *s, i4  n )
{
	i4 rem;
	char *tag = ERx(" KMGT");
   	   
	for( ; *tag != EOS; CMnext( tag ) )
	{
		if( n == 0 )
		{
			STcopy( ERx( "   0"), s );
			return;
		}
		if( n < 1024 )
		{
			STprintf( s, ERx("%4d%c"), n, *tag );
			return;
		}

		rem = n % 1024;
		n /= 1024;
		n += (rem >= 512);
	}
	STcopy( ERx( "huge " ), s );  /* something wrong! */
}

/* Break an authorization string down into the customary four-group format. */

static void
decompose_auth(a1, a2, a3, a4)
char *a1, *a2, *a3, *a4;
{
    char *from, *to;
    char *a[4];
    i4  ax, cnt;

    a[0] = a1;
    a[1] = a2;
    a[2] = a3;
    a[3] = a4;

    for ( from = authString, ax = 0; ax < 4; ax++)
    {
	to = a[ax];
	for (cnt = 0; *from && cnt < 8; (void) CMnext(from))
	{
	    if (!CMwhite(from))
	    {
		CMcpychar(from, to);
		(void) CMnext(to);
		cnt++;
	    }
	}

	*to = EOS;
    }
}

/*  Smoosh an authorization string together from the four-group format. */

static void
compose_auth(a1, a2, a3, a4)
char *a1, *a2, *a3, *a4;
{
    STprintf(authString, ERx("%s %s %s %s"), a1, a2, a3, a4);
}

/*
**	ip_lochelp()  -- locate specified .hlp file for the active frame
**	
**	this function used to be inline code within top_frame() as it was
**	only used originally for the initial ingbuild frame.
**	it has been moved here as a function as a result of both the 
**	subsequent install and current frames also requiring menuitem 'help'.
**
**	22-mar-95	(athda01) b67639
**			code moved during fix for bug b67639 to provide 
**			menuitem 'help' for ingbuild/install and current.
**			
*/
void  
ip_lochelp( char *hlpfile, char *hlpname )

{

            LOCATION loc;
            char     locBuf[ MAX_LOC + 1 ];
            char     *cp;

            /* 
            ** The help files are in a non-standard place during install :
            ** II_FORMFILE is the formfile with the "Keys" help form
            ** II_HELPDIR is for Help on Help 
            */
            IPCL_LOfroms( PATH, ip_install_loc(), &loc );
            LOfstfile( "rtiforms.fnx", &loc );
            LOtos( &loc, &cp );
            IPCLsetEnv( ERx("II_FORMFILE"), cp, TRUE );
            IPCLsetEnv( ERx("II_HELPDIR"), ip_install_loc(), TRUE );

            STcopy( ip_install_loc(), locBuf );
  
            IPCL_LOfroms( PATH, locBuf, &loc );
            LOfstfile( hlpfile, &loc );
	    LOtos( &loc, &cp );
            IIRTfrshelp( cp, hlpname, H_FE, (VOID (*)())NULL);

}

static VOID 
ip_patchmode(i4 argc, char **argv)
{
	i4 c;

	for (c = argc; c; c--, *argv++)	
	{
		if (STcompare(*argv, "-patch") == 0)
		{
			pmode = TRUE;
			ip_hash_init();
			break;
		}
	}
}

/*
** ip_response : This compares the input flags for -mkresponse 
**		 or -exresponse option 
**  History:
**      02-nov-2001 (gupsh01)
**              Added handling of user defined response file name.
*/
static VOID
ip_response(i4 argc, char **argv)
{
        i4 c;
	STATUS status; 
	char *filename;
	char *def_file = "ingrsp.rsp";

        for (c = argc; c; c--, *argv++)
        {
           if (STcompare(*argv, "-mkresponse") == 0)
           {
	            mkresponse = TRUE;
	            filename=def_file;	
           }
	   else if (STcompare(*argv, "-exresponse") == 0)
           {
		if (mkresponse)
		{
		    /* both mkresponse and exresponse given */
		    IIUGerr( E_ST0190_bad_resp_op, 0, 1);
                    EXdelete();
                    PCexit( FAIL );
	        }	
                exresponse = TRUE;
	   }
	   else if (MEcmp(*argv + 1, "file=", 5) == 0)
           {
		filename=*argv + 6;
		respfilename=filename;
	   }
        }
	if (mkresponse)
	    ip_response_init();
}

/*
** response_init : This function initializes the stream for response file.
** writes the II_AUTHSTRING, II_DISTRIBUTION, II_TERMCAP_FILE, 
** II_EMBED_USER, II_LANGUAGE variables.
**
** History: 
**	11-dec-2001 (gupsh01)
**	    Added code so that if error in opening the response file 
**	    terminate the program.
*/

static VOID
ip_response_init()
{
    FILE        *respfile;
    char        *value;
    char 	*filename;
    char 	*def_file="ingrsp.rsp";
    char 	*cp2;
    STATUS      stat;

    if (respfilename != NULL)
	filename = respfilename;
    else
	filename = def_file;

    /* 
    ** response filename can be an input parameter. 
    ** By default we set it as ingrsp.rsp
    ** Opened in the current directory
    */

    if (OK == (stat = ip_open_response (ERx(""), filename, 
               &respfile, ERx( "w" )) )) 
    { 
	rSTREAM = respfile;

	/* write value for II_SYSTEM */
        cp2 = NULL;
        NMgtAt("II_SYSTEM",&cp2);
        if ((cp2 && *cp2))
            ip_write_response( ERx("II_SYSTEM"), cp2, rSTREAM);

    	/* write the initial variables to the response file. */  
        if( *authString != EOS )
            stat = ip_write_response( ERx("II_AUTH_STRING"), authString, rSTREAM);

	if (*distribDev != EOS )
            stat = ip_write_response( ERx("II_DISTRIBUTION"), distribDev, rSTREAM);

	cp2 = NULL;
        NMgtAt(ERx("II_TERMCAP_FILE"), &cp2);
	if ((cp2 && *cp2))
            stat = ip_write_response( ERx("II_TERMCAP_FILE"), cp2, rSTREAM);

        cp2 = NULL;
        NMgtAt(ERx("II_EMBED_USER"), &cp2);
        if ((cp2 && *cp2))
	    stat = ip_write_response( ERx("II_EMBED_USER"), cp2, rSTREAM);

	cp2 = NULL;
	NMgtAt("II_LANGUAGE",&cp2);
	if ((cp2 && *cp2))
            stat = ip_write_response( ERx("II_LANGUAGE"), cp2, rSTREAM);

	/* II_MSGDIR is already set. If it is then write to the response file */
	cp2 = NULL;
	NMgtAt("II_MSGDIR",&cp2);
	if ((cp2 && *cp2))
            stat = ip_write_response( ERx("II_MSGDIR"), cp2, rSTREAM);	

	/* II_CONFIG is already set. If it is then write to the response file */
        cp2 = NULL;
        NMgtAt("II_CONFIG",&cp2);
        if ((cp2 && *cp2))
            stat = ip_write_response( ERx("II_CONFIG"), cp2, rSTREAM); 

	cp2 = NULL;
        NMgtAt("TERM_INGRES",&cp2);
        if ((cp2 && *cp2))
            stat = ip_write_response( ERx("TERM_INGRES"), cp2, rSTREAM);
    }
    else 
	PCexit ( FAIL );
}
