/*
** Copyright (c) 1992, 2010 Ingres Corporation
*/

# include <compat.h>
# include <si.h>
# include <st.h>
# include <me.h>
# include <lo.h>
# include <ex.h>
# include <er.h>
# include <gc.h>
# include <cm.h>
# include <nm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ui.h>
# include <te.h>
# include <iplist.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <pc.h>
# include <sys/stat.h>
# include <ip.h>
# include <ipcl.h>
# include <id.h>

/*
**  Name: buildrel -- build tool to convert a build area to a release area
**                    (Unix version!)
**
**  Entry points:
**	This file contains no externally visible functions.
**
**  Command-line options:
**
**	-a	Audit; just generate a list of bad files (missing or wrong
**		permissions), but don't move anything or update anything.
**		Also, don't compute checksums or sizes.
**
**      -b      Use alternate black box filename for release.dat
**
**	-B	Batch mode, no user prompting just report any errors and
**	        exit.
**
**	-f	Ignore bad permissions and/or missing files.
**
**      -p      Generate a picklist.dat in $ING_BUILD. Used with -a.
**   
**	-q	Run quiet, or mostly silently.  Generate very little
**		output and don't ask any questions.
**	
**	-r	Generate file attribute lists for RPM specfiles.
**
**	-d	Generate file attribute lists for DEB control files.
**
**	-S	Generate file attribute lists for SRPM spec files.
**
**      -x      Convert release data to XML format for external processing
**
**	-u	Unhide i18n, i18n64, and documentation packages
**
**	-l gpl|com|emb|eval Overide license type defined in rpmconfig (rpm only)
**
**	-s	Include Spatial datatype objects in the save set on Linux, 
**              generate Spatial datatype objects release on Unix. 
**
**  History:
**	xx-xxx-92 (jonb)
**		Created.
**      16-mar-93 (jonb)
**		Made comment style conform to standards.
**	13-jul-93 (tyler)
**		Rewritten to support portable manifest language.
**	14-jul-93 (tyler)
**		Replaced changes lost from r65 lire codeline and
**		fixed broken link creation.  Removed useless command 
**		line options.  Improved usage message.  Various other
**		finishing touches.
**	16-jul-93 (tyler)
**		Modified "cp" command to use the "-p" option to preserve 
**		file permissions.  In case that doesn't work (which
**		apparently doesn't on SunOS), use "chmod" to set the
**		correct permissions explictly.  Fixed bug which
**		caused CHECKSUM and SIZE entries to be created for
**		empty packages, which broke ingbuild.  Removed defunct 
**		sections of code.
**	09-aug-93 (fredv)
**		Add a check to the existence of the directory; creates the
**		the directory only if it doesn't exists.
**	24-sep-93 (tyler)
**		Changed RELDIR to II_RELEASE_DIR to be consistent
**		with II_MANIFEST_DIR.  Removed -l<logfile> option.
**		Don't try to create archive if all files are missing
**		for a package if -f option was passed.
**	19-oct-93 (tyler)
**		Beefed up usage message and improved argument parsing
**		to catch illegal flags of the form "-flkjsf".  Fixed -f
**		mode to ignore bad permissions.  Removed code which
**		silently disabled force mode when more than 10 files
**		were missing.  Added GVLIB to NEEDLIBS and added
**		GLOBALREF declaration of global variable Version[];.
**	11-nov-93 (tyler)
**		Ported to IP Compatibility Layer.
**	02-dec-93 (tyler)
**		Added argument to ip_parse().
**	05-jan-94 (tyler)
**		II_MANIFEST_DIR is now optional and the path of the
**		manifest directory is displayed.
**	07-feb-94 (tyler/fredv)
**		Use gtar instead of native tar, since tar fails to
**		preserve setuid on HP and possibly other platforms.
**	22-feb-94 (tyler)
**		Added support for the BUILD_FILE directive.
**	28-mar-95 (peeje01)
**		Crossintegration of doublebyte changes label 6500db_su4_us42
**      30-May-1995 (johnst)
**		For Unixware port (usl_us5) cannot use cp -p option, since 
**		this outputs warning message and non-zero exit code when 
**		copying files with suid bit set. Just do straight cp and 
**              let chmod fix up permissions afterwards.
**	17-may-95 (wadag01)
**		Changed the 'cp -p' command for odt_us5 (SCO's cp does not
**		have any options). SCO's equivalent is 'copy -m'.
**	08-aug-95 (morayf)
**		Made SCO OpenServer 5.0 (sos_us5) same as usl_us5, as same
**		logic re. cp -p applies.
**	11-nov-95 (hanch04)
**		Added eXpress
**	19-jan-96 (hanch04)
**		RELEASE_MANIFEST is now releaseManifest
**	        added -b flag for black box release.dat file
**	31-dec-96 (funka01)
**		Replaced hardcoded "ingres" with SystemAdminUser
**		and added 755 permission to directories created
**		because sometimes on install directories would
**		not be executable.
**	21-nov-97 (kinpa04)
**		SunOS doesn't support the '-m' option on mkdir
**	13-May-98 (merja01)
**		Added ing_src_bin to insure picking up correct version
**		of gtar.
**      19-jan-99 (matbe01)
**              Made NCR (nc4_us5) same as usl_us5 re. cp -p in STprintf
**              function.
**      11-mar-1999 (hanch04/kosma01/linke01)
**                   Added ignoreChecksum flag to ignore file mismatch errors.
**   26-apr-1999 (rosga02)
**       	Bug #96685. Add 1 byte string terminator for ing_src_bin to avoid
**		abends when the $ING_SRC/bin/ path length is multiple of 32.
**	10-may-1999 (walro03)
**		Remove obsolete version string odt_us5.
**      03-jul-1999 (podni01)
**              Added support for ReliantUNIX V 5.44 - rux_us5 (same as rmx_us5)
**      29-jul-1999 (hweho01)
**              Added statement 'return( OK )' at the end of routine 
**              write_manifest() if there is no error. Without the OK return 
**              code, the process will not continue on ris_u64 platform. 
**      10-Jan-2000 (hanal04) Bug 99965 INGSRV1090.
**              Correct rux_us5 changes that were applied using the rmx_us5
**              label.
**	28-apr-2000 (somsa01)
**		For other products, the user must be "ingres".
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      21-sep-2001 (gupsh01)
**              added GLOBALDEF for mkresponse mode.
**      29-oct-2001 (gupsh01)
**              added GLOBALDEF for exresponse mode.
**	20-nov-2001 (gupsh01)
**		added GLOBALDEF for respfilename.  
**	20-Jan-2004 (bonro01)
**		Add -pif option for new @PIF installer
**	02-Feb-2004 (bonro01)
**		Prevent SEGV when II_RELEASE_DIR is not set.
**	22-Jan-2004 (hanje04)
**		SIR 111659
**		Add check and get for II_RPM_PREFIX as we use it to generate 
**		the RPMs.
**	11-Feb-2004 (hanal04) Bug 111770
**          Detect buildrel use in a private path and use base
**          area files on any private path errors.
**	3-Mar-2004 (bonro01)
**		Display PIF build directory.
**		Allow gtar and compress command to return non-zero return code.
**		compress returns rc=2 when the resulting file is larger than
**		the original. While undesirable, this should be acceptable.
**	02-Apr-2004 (hweho01)
**              Use API_RELEASE_DIR to make the remote API package. 
**	11-Jun-2004 (somsa01)
**		Cleaned up code for Open Source.
**	16-Jun-2004 (bonro01)
**		Remove PIF install.
**	28-Jun-2004 (hanje04)
**	    Add check for II_RPM_EMB_PREFIX to allow correctl creation of
**	    ca-ingres-EI (embedded) package
**	30-Jul-2004 (hanje04)
**	    If II_RPM_EMB_PREFIX isn't set then don't generate the embedded
**  	    SPEC, but don't bail either.
**	02-Aug-2004 (hanje04)
**	    Replace use of tar and gtar with pax
**	17-sep-2004 (stephenb)
**	    Solaris pax doesn't support compression
**	20-Oct-2004 (bonro01)
**	    HP pax doesn't support compression.
**	19-Nov-2004 (bonro01)
**	    When tar'ing up packages, don't select the '.' directory
**	    because tar then trys to update the $II_SYSTEM directory
**	    which may not be owned by the install user.  The
**	    $II_SYSTEM/ingres directory is the only directory that
**	    is required to be owned by the install user.
**	01-Dec-2004 (hweho01)
**	    Setup the file mode creation mask to 022, so the  
**          intermediate directories in the release package will
**          have the appropriate permissions. Star #13792577.
**	06-Dec-2004 (hanje04)
** 	    In assemble_archives(), use STcopy() instead of STpolycat()
**	    to generate cmd.
**	10-Dec-2004 (hweho01)
**	    Modified the change on 01-Dec-2004, issue the umask command   
**	    when the package directory is created, so it will have    
**          effect on the intermediate directory.
**      13-Jan-2005 (hweho01)
**          Use /bin/tar to archive/de-archive the package for AIX.
**	13-Jan-2005 (lazro01)
**	    UnixWare pax doesn't support compression.
**	15-Mar-2005 (bonro01)
**	    Use pax for a64_sol with unix compress.
**	30-Mar-2005 (hweho01)
**          Fix error "Cannot set the time on ." if the directory of 
**          $II_SYSTEM is at the top level of the file system on AIX. 
**	20-May-22005 (hanje04)
**	    Instead of waiting to display incorrect file permissions before
**	    correcting them we should correct them on the fly and report it. 
**	    Only "error" when we cannot change the permssion.
**      04-Aug-2005 (zicdo01)
**          Remove check to build as ingres (replaced with a warning instead)
**	Aug-9-2005 (hweho01)
**          Added support for axp_osf platform. 
**	14-Sept-2005 (hanje04)
**	    On both Linux and Mac (and maybe others) PAX is reporting
**	    completion before it has finished writing the archive.
**	    This results in the wrong cksum and size info being recorded
**	    for most of the packages in release.dat. Add "sleep 1" to
**	    archive command line so to delay shell exit and allow pax 
**	    to finish archiving.
**	05-Jan-2006 (bonro01)
**	    Remove i18n, and documentation from default ingres.tar
**	31-Jan-2006 (hanje04)
**	    SIR 115688
**	    Add -L flag to buildrel so that the License dependencies in
**	    the RPM packages can be specified as something other than
**	    what is defined in rpmconfig.
**	    Update usage message and define license types to buildrel.
**	01-Feb-2006 (hanje04)
**	    SIR 115688
**	    Replace -L with -l to avoid problems on VMS.
**	02-Feb-2006 (hanje04)
**	    BUG 115239
**	    Add -dt to allow inclusion of spatial object package in
**	    saveset without having to rebuild.
**	03-Feb-2006 (hanje04)
**	    SIR 115688
**	    Add eval a license type for evaluation saveset.
**	06-Feb-2006 (hanje04)
**	    Make sure i18n, doc and spatial packages are hidden during 
**	    audit (-a) as well.
**	21-Feb-2006 (bonro01)
**	    Add LICENSE file back into install directory.
**	    LICENSE file needs to be variable for each type of licensing
**	    that we support.  Substitute %LICENSE% in manifest with the
**	    correct license file specified by "buildrel -l"
**      15-Sep-2005 (hanal04) Bug 115197
**          Added -p option to allow the generation of a picklist.dat
**          file in $ING_BUILD.
**      23-Feb-2006 (hweho01)
**          Use gtar (GNU tar version 1.15.1) to archive/de_archive
**          component package on AIX platform.  
**	01-Mar-2006 (hanje04)
**	    Replace -s for slient with -q for quiet and use -s for spatial
**	    instead of -p which is now used for picklists.
**	06-Mar-2006 (hanje04)
**	    -p can only be used with audit so set audit=TRUE with -p and
**	    remove need to pass -a too.
**    22-Mar-2006 (hanje04)
**        BUG 115796
**        After sucessfull completion under regular operation (not audit or
**        rpm etc) create build.timestamp under $ING_BUILD if it doesn't
**        already exist.
**        This is used by patch tools generate a "plist" for a patch.
**	29-Mar-2006 (hweho01)
**          Enable buildrel to process spatial release on Unix platforms. 
**      10-May-2006 (hweho01)
**          Enable buildrel to process spatial release on Linux.
**	13-Jun-2006 (bonro01)
**	    Make sure spatial, i18n, and documentation packages are 
**	    skipped for INCLUDE and NEED
**	15-Jun-2006 (hanje04)
**	    SOL_RELEASE_DIR doesn't need to be set when we're running with -a.
**      15-Jun-2006  (hweho01)
**          Use gtar (GNU tar version 1.15.1) to archive/de_archive
**          component package on HP Tru64 platform.  
**       9-Nov-2006 (hanal04) Bug 117044
**          Add -x for XML output used by IceBreaker build.
**       4-Apr-2006 (hanal04) Bug 118027
**          Ensure a picklist.dat generates the install file names so
**          that the manifest.dat in a delta patch can be used to backup
**          all installed files.
**          gtar and pax are now built in $ING_BUILD/bin so update calls
**          accordingly.
**	5-Apr-2007 (bonro01)
**	    Rearrange pax code to be more portable.
**	27-Apr-2007 (hweho01)
**	    Use OS archiver on AIX and Tru64.
**	16-May-2007 (bonro01)
**	    Fix pax location bug introduced by the last integration.
**	    Remove obsolete code from hanal04 change for bug 118027
**	11-Feb-2008 (bonro01)
**	    Remove separate Spatial object install package.
**	    Include Spatial object package into standard ingbuild and RPM
**	    install images instead of having a separate file.
**	08-OCt-2007 (hanje04)
**	    SIR 114907
**	    Pax writes to stdout by default so the "-f -" flag isn't needed
**	    and causes a file named '-' to be created on Mac OSX. Remove it
**	02-Aug-2007 (hanje04)
**	    SIR 118420
**	    Add support for build DEB package control files, entry point
**	    for new funtionality is gen_deb_cntrl().
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of id.h to eliminate gcc 4.3 warnings.
**	14-Feb-2010 (hanje04)
**	    SIR 123296
**	    Add support for LSB builds
**	04-Mar-2010 (hanje04)
**	    SIR 123296
**	    Add -S flag to generate source RPM file lists under
**	    ING_BUILD/files/srpmflists
**	    Add -B flag to allow batch operation. Perm errors or missing
**	    files result in listing and exit without further prompting.
**	14-July-2010 (bonro01)
**	    Add Beta LICENSE to ingbuild.
**	    
*/

/*
PROGRAM =	buildrel
**
NEEDLIBS =	INSTALLLIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB SQLCALIB GCFLIB ADFLIB CUFLIB \
		SQLCALIB COMPATLIB MALLOCLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

#define PLIST_NAME  "picklist.dat"
#define TIMESTAMP_FILE  "build.timestamp"
GLOBALDEF char *releaseDir;
GLOBALDEF char *pkgBuildroot;
GLOBALDEF char *rpmPrefix;
GLOBALDEF char *ingbuild_bin;
GLOBALDEF char *build_timestamp;
GLOBALDEF char *ing_vers = NULL;
GLOBALDEF char *base_vers = NULL;
GLOBALDEF char *buildhome = NULL;
GLOBALDEF char *license_file = NULL;
GLOBALDEF char *ingbuild = NULL;
GLOBALDEF LOCATION releaseLoc;
GLOBALDEF char releaseBuf[ MAX_LOC + 1 ];
GLOBALDEF char *distRelID, *instRelID;
GLOBALDEF char *releaseName, *instProductName;
GLOBALDEF char installDir[ MAX_LOC + 1 ];
GLOBALDEF char releaseManifest[ 32 ];
GLOBALDEF bool mkresponse = FALSE;
GLOBALDEF bool exresponse = FALSE;
GLOBALDEF char *respfilename = NULL; /* user supplied response file name */
GLOBALDEF bool privatePath = FALSE;
GLOBALDEF char *gpl_lic = "gpl" ;
GLOBALDEF char *com_lic = "com" ;
GLOBALDEF char *beta_lic = "beta" ;
GLOBALDEF char *eval_lic = "eval" ;
GLOBALDEF char *license_type = NULL ;
GLOBALDEF char manifestDir[ MAX_LOC + 1 ];

/* 
** dummy fields.  Some of the library functions we link in will refer
** to these fields.  They can be defined as dummies as long as we never
** actually call the functions.
*/

GLOBALDEF char distribDev[ 1 ];		/*** dummy ***/
GLOBALDEF char distribHost[ 1 ];	/*** dummy ***/
GLOBALDEF i4  ver_err_cnt;		/*** dummy ***/
GLOBALDEF bool batchMode;		/*** dummy ***/
GLOBALDEF bool ignoreChecksum=FALSE;	/*** dummy ***/
GLOBALDEF bool eXpress;			/*** dummy ***/
GLOBALDEF bool pmode;			/*** dummy ***/

/* Global References */
GLOBALREF bool hide;
GLOBALREF bool spatial;

/* global lists... */

GLOBALDEF LIST distPkgList;
LIST instPkgList;
LIST dirList;

/* local function declarations */
 
static STATUS create_subdirectories();
static STATUS assemble_archives();
static STATUS write_manifest();
static STATUS compute_file_info();
static STATUS compute_archive_info();
static void get_permission();
static bool getyorn();
static STATUS substitute_variables();

/* external function declarations */
STATUS gen_rpm_spec();
STATUS gen_srpm_file_lists();
STATUS gen_deb_cntrl();
static void dumpxml();

static LOCATION installLoc;
static char installBuf[ MAX_LOC + 1 ];
static bool force = FALSE;
static bool audit = FALSE;
static bool plist = FALSE;
static bool rpm = FALSE;
static bool srpm = FALSE;
static bool deb = FALSE;
static bool xml = FALSE;
static bool silent = FALSE;
static bool ask = TRUE;
static bool do_remote_api = FALSE;
static char api_manifest[] = {"relremoteapi.dat"}; 
static bool emb = TRUE;
static bool batch = FALSE;

# define COND( x ) { if( !silent ) {x;} }

/*
** usage() - display usage message and exit.
*/

void
usage()
{
	SIfprintf( stderr,
		ERx( "\n Usage: buildrel [ options ]\n" ) );
	SIfprintf( stderr, ERx( "\n Options:\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -a -- audit the build area (don't generate release area)\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -b filename -- alternate release.dat filename\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -B -- batch mode, report errors and exit without prompting user\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -f -- force build of release area (batch mode - ignore missing files)\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -p -- used with -a to generate a picklist.dat in $ING_BUILD\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -r -- build RPM spec files (req. II_RPM_BUILDROOT, e.g. /devsrc/rmainlnx/RPMS)\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -S -- build SRPM file lists (req. II_RPM_BUILDROOT, e.g. /devsrc/rmainlnx/RPMS)\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -d -- build DEB control files (req. II_DEB_BUILDROOT, e.g. /devsrc/rmainlnx/DEBS)\n" ) );
        SIfprintf( stderr,
                ERx( "\n    -x -- Convert release data to XML format for external processing\n\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -u -- Unhide i18n, i18n64, and documentation packages\n\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -l gpl|com|emb|eval -- Override default license file\n\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -s -- Include spatial objects package \n\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -q -- quiet operation (mostly)\n\n" ) );
	SIfprintf( stderr, ERx( " Description:\n\n" ) );
	SIfprintf( stderr,
		ERx( "  This program will assemble an %s release in the directory specified by\n" ), SystemDBMSName );
	SIfprintf( stderr,
		ERx( "  %s_RELEASE_DIR, using the release description source files located in the\n" ), SystemVarPrefix );
	SIfprintf( stderr,
		ERx( "  directory specified by %s_MANIFEST_DIR, or $%s/%s/manifest, if\n" ), SystemVarPrefix, SystemLocationVariable, SystemLocationSubdirectory );
	SIfprintf( stderr,
		ERx( "  %s_MANIFEST_DIR is undefined.  The path names of the files to be included\n" ), SystemVarPrefix ); 
	SIfprintf( stderr,
		ERx( "  in the release are specified in the release description source files in\n" ) );
	SIfprintf( stderr,
		ERx( "  terms of ING_BUILD, which must also be defined.\n\n" ) );
	SIfprintf( stderr,
		ERx( "  The end result of a successful buildrel will be a release directory\n" ) );
	SIfprintf( stderr,
		ERx( "  containing several sub-directories, each of which will contain a single\n" ) );
	SIfprintf( stderr,
		ERx( "  compressed tar archive.  The sole exception is the \"install\" sub-directory\n" ) );
	SIfprintf( stderr,
		ERx( "  which will contain additional files.\n\n" ) );
	SIfprintf( stderr,
		ERx( "  Because buildrel only assembles packages which have no existing compressed\n" ) );
	SIfprintf( stderr,
		ERx( "  tar archive, the entire contents of the release directory should be removed\n" ) );
	SIfprintf( stderr,
		ERx( "  before the program is run.  Or, if a change is made to the contents or a\n" ) );
	SIfprintf( stderr,
		ERx( "  particular package after the release has been built, buildrel will\n" ) );
	SIfprintf( stderr,
		ERx( "  reassemble only that package if its sub-directory has been removed.\n\n" ) );
	PCexit( FAIL );
}

/*
** buildrel's main()
*/

main(i4 argc, char *argv[])
{
	STATUS rtn;
	char *cp;
	i4 argvx;
	char cmd[ MAX_LOC * 2 ];
	char userName[ 64 ];
	LOCATION loc;
	char lobuf[ MAX_LOC + 1 ];

	MEadvise( ME_INGRES_ALLOC );

	/* set releaseManifest variable */ 
	STcopy( ERx("release.dat"), releaseManifest );

	license_type = gpl_lic;

	for( argvx = 1 ; argvx < argc ; argvx++ )
	{
		/* break if non-flag was encountered */
		if( argv[ argvx ][ 0 ] != '-' )
			break;

		/* display usage if flag is too long */
		if( STlength( argv[ argvx ] ) > 4 )
			usage();

		switch( argv[ argvx ][ 1 ] )
		{
			case 'f': /* -f -- create manifest only */
				force = TRUE;
				break;

			case 'a': /* -a -- generate list of errors only. */
				audit = TRUE;
				break;

			case 'B': /* -B -- batch mode */
				batch = TRUE;
				break;

			case 'p': /* -p -- generate a picklist.dat file . */
				plist = TRUE;
				audit = TRUE;
				break;

			case 'r': /* -r -- generate file list for RPM. */
				rpm = TRUE;
				break;

			case 'S': /* -r -- generate file list for RPM. */
				srpm = TRUE;
				break;

			case 'd': /* -d -- generate file list for DEB. */
				deb = TRUE;
				break;

			case 'q': /* -q -- quiet running, no output. */
				silent = TRUE;
				break;

			case 'b': /* -b filename -- alternate name */
				  /* for release.dat */
				STcopy( argv[ ++argvx ], releaseManifest );
                                if( STcompare( releaseManifest, 
                                               api_manifest ) == 0 ) 
                                   do_remote_api = TRUE; 
				break;
			case 'u': /* -u -- unhide packages i18n, documentation */
				hide = FALSE;
				break;
			case 'l':
				/* -l -- next argument should be license type */
				if ( ++argvx >= argc )
				    usage();	

				if (STcompare(argv[argvx], gpl_lic) == 0)
				{
				    license_type=gpl_lic;
				    break;
				}
				else if (STcompare(argv[argvx], com_lic) == 0)
				{
				    license_type=com_lic;
				    break;
				}
				else if (STcompare(argv[argvx], beta_lic) == 0)
				{
				    license_type=beta_lic;
				    break;
				}
				else if (STcompare(argv[argvx], eval_lic) == 0)
				{
				    license_type=eval_lic;
				    break;
				}
				else
				    license_type=NULL;

                        case 'x': /* -x -- generate release data in XML. */
                                xml = TRUE;
                                break;

			case 's': /* -s -- Spatial package */
				spatial = TRUE;
				break;
			default:
				usage();
		}
	}

	if( license_type == NULL )
	{
		SIfprintf( stderr,
			ERx( "\nbuildrel: the -l option value is invalid.\n\n" ) );
		usage();
		PCexit( FAIL );
	}

        if( plist && !audit )
        {
                SIfprintf( stderr,
                        ERx( "\nbuildrel: the -p option may only be used with the -a option.\n\n" ) );
                PCexit( FAIL );
        }

	if( audit && force )
	{
		SIfprintf( stderr,
			ERx( "\nbuildrel: the -a and -f options cannot be used together.\n\n" ) );
		PCexit( FAIL );
	}

	if((rpm && deb) || (deb && srpm) || (srpm && rpm))
	{
		SIfprintf( stderr,
			ERx( "\nbuildrel: the -r, -S and -d options cannot be used together.\n\n" ) );
		PCexit( FAIL );
	}

	/* make sure user is who we want */
	cp = userName;
	IDname( &cp );
	if( STcompare( cp, SystemAdminUser ) != 0 )
	{
		SIfprintf( stderr,
		ERx( "\nWARNING you are not running as '%s' "
		     "(this is okay if done on purpose)\n\n" ), SystemAdminUser );
	}

	if( argvx != argc )
		usage();

	/* set manifestDir variable */ 
	NMgtAt( ERx( "II_MANIFEST_DIR" ), &cp );
	if( cp != NULL && *cp != EOS )
		STcopy( cp, manifestDir );
	else
	{
		LOCATION loc;

		NMgtAt( SystemLocationVariable, &cp );
		STcopy( cp, manifestDir );
		IPCL_LOfroms( PATH, manifestDir, &loc );
		LOfaddpath( &loc, SystemAdminUser, &loc );
		LOfaddpath( &loc, ERx( "manifest" ), &loc );
	}

        /* get ING_VERS if set to determine whether we are in a private path */
        cp = NULL;
        NMgtAt(ERx("ING_VERS"), &cp);
        if( cp != NULL && *cp != EOS )
        {
            ing_vers = MEreqmem(0, (STlength(cp) * sizeof(char)) + 1, TRUE, &rtn);
            STprintf(ing_vers,"%s", cp);
            
            cp = NULL;
            NMgtAt(ERx("BASEPATH"), &cp);
            if( cp != NULL && *cp != EOS )
            {
                base_vers = MEreqmem(0, (STlength(cp) * sizeof(char)) + 1, TRUE, &rtn);
                STprintf(base_vers,"%s", cp);

		cp = NULL;
		NMgtAt(ERx("ING_BUILD"), &cp);
		if( cp != NULL && *cp != EOS )
		{
		    buildhome = MEreqmem(0, ((STlength(cp) - STlength(ing_vers) - 1) * sizeof(char)) + 1, TRUE, &rtn);
		    STlcopy(cp, buildhome, (STlength(cp) - STlength(ing_vers) - 1));

                    if( STcompare( ing_vers, base_vers ) != 0 )
                    {
                        privatePath = TRUE; 
                    }
		}
            } 
        }
             
        /* get II_RELEASE_DIR, API_RELEASE_DIR */
	if ( do_remote_api )
        {
             NMgtAt( ERx( "API_RELEASE_DIR" ), &releaseDir );
             if (!releaseDir || !*releaseDir)
              {
                SIfprintf(stderr,ERx("\nAPI_RELEASE_DIR must be defined.\n"));
                PCexit( FAIL );
              }
        }
	else
	  NMgtAt( ERx( "II_RELEASE_DIR" ), &releaseDir );
	if( (!releaseDir || !*releaseDir) && !audit && !deb && !rpm && !xml && !srpm )
	{
		SIfprintf( stderr, ERx( "\n%s_RELEASE_DIR must be defined. Use \"buildrel -help\" to get more information.\n\n" ), SystemVarPrefix );
		PCexit( FAIL );
	}

      /* use ING_BUILD to define build_timestamp */
      cp=NULL;
      NMgtAt( ERx( "ING_BUILD" ), &cp );
      if ( cp != NULL )
      {
          char bin [] = {"/bin/"};

          ingbuild = MEreqmem(0, (STlength(cp) + 1), TRUE, &rtn);
          STlcopy(cp, ingbuild, STlength(cp));
          ingbuild_bin = MEreqmem(0,((STlength(cp) + STlength(bin)) * sizeof(char)) + 1, TRUE, &rtn);
          STprintf(ingbuild_bin,"%s%s", cp, bin);

          /* allocate space for timestamp string */
            build_timestamp = MEreqmem(0,((STlength(cp) + 1 + /* add 1 for / */
                              STlength(TIMESTAMP_FILE)) * sizeof(char)) + 1,
                              TRUE, &rtn);
          /* copy file and location to buffer */
          if ( build_timestamp )
              STpolycat( 3, cp, "/", TIMESTAMP_FILE, build_timestamp );
          else
          {
              SIfprintf( stderr,
               "Failed to allocate memory for timestamp file.\n Aborting.\n\n" );
              PCexit(FAIL);
          }
      }
      else
      {
         SIfprintf( stderr, "ING_BUILD must be defined\n");
         PCexit(FAIL);
      }

	if( releaseDir && *releaseDir )
	{
	    STcopy( releaseDir, releaseBuf );
	    LOfroms( PATH, releaseBuf, &releaseLoc );
	    LOtos( &releaseLoc, &releaseDir );

	    LOcopy( &releaseLoc, installBuf, &installLoc );
	    LOfaddpath( &installLoc, ERx( "install" ), &installLoc );
	    LOtos( &installLoc, &cp );
	    STcopy( cp, installDir );
	}

	/*
	**  Read all data from the packing list file, and store it in internal
	**  data structures...
	*/

	if( ip_parse( DISTRIBUTION, &distPkgList, &distRelID, &releaseName,
		TRUE ) != OK )
	{
		PCexit(FAIL);
	}

        if( !xml )
        {
	    SIprintf( ERx( "\nRelease name: %s\n" ), releaseName );
	    SIprintf( ERx( "\nRelease Id #: %s\n" ), distRelID );
	    SIprintf( ERx( "\nManifest directory: %s\n" ), manifestDir );
	    SIprintf( ERx( "\nRelease directory: %s\n" ), (releaseDir && *releaseDir) ? releaseDir : "NULL" );
        }

	if( substitute_variables() != OK )
		PCexit(FAIL);

	if( audit ) 
	{
		SIfprintf(stderr,
			ERx( "\nAuditing deliverable files; this may take a few seconds...\n\n" ) );

		PCexit( compute_file_info() );
	}

        if( xml )
        {
                dumpxml();
                PCexit( OK );
        }

# ifdef conf_LSB_BUILD
	rpmPrefix="/usr/libexec/ingres";
# else
	NMgtAt( ERx( "II_RPM_EMB_PREFIX" ), &rpmPrefix );
	if ( !rpmPrefix || !*rpmPrefix ) emb = FALSE ; 
	NMgtAt( ERx( "II_RPM_PREFIX" ), &rpmPrefix );
# endif
	if( rpm || srpm )
	{
		NMgtAt( ERx( "II_RPM_BUILDROOT" ), &pkgBuildroot );
		if ( !pkgBuildroot || !*pkgBuildroot )
		{
            SIfprintf(stderr, 
                ERx("\nII_RPM_BUILDROOT must be defined. Use \"buildrel -help\" to get more information.\n\n") );
		    PCexit( FAIL ) ;
		}

		if ( !rpmPrefix || !*rpmPrefix )
		{
            SIfprintf(stderr, 
                ERx("\nII_RPM_PREFIX must be defined. Use \"buildrel -help\" to get more information.\n\n") );
		    PCexit( FAIL ) ;
		}
		audit=TRUE;
		if ( compute_file_info() )
		    PCexit( FAIL );
		SIfprintf(stderr,
			ERx( "\nGenerating RPM file lists...\n" ) );
		if ( srpm )
		    PCexit( gen_srpm_file_lists() );
		else
		    PCexit( gen_rpm_spec() );
	}
	else if ( deb )
	{
		NMgtAt( ERx( "II_DEB_BUILDROOT" ), &pkgBuildroot );
		if ( !pkgBuildroot || !*pkgBuildroot )
		{
            SIfprintf(stderr, 
                ERx("\nII_DEB_BUILDROOT must be defined. Use \"buildrel -help\" to get more information.\n\n") );
		    PCexit( FAIL ) ;
		}
		audit=TRUE;
		if ( compute_file_info() )
		    PCexit( FAIL );
		SIfprintf(stderr,
			ERx( "\nGenerating DEB file lists...\n" ) );
		PCexit( gen_deb_cntrl() );
	}

	SIprintf( ERx( "\nBuilding release area...\n" ) );

	/* 
	** If we're going to be moving any actual files, make sure the release
	** directory exists; if it doesn't create it.
	*/

	if( IPCLcreateDir( releaseDir ) != OK )
	{
		SIfprintf( stderr,
			ERx( "Possible ownership or permission problem with directory:\n" ) );
		SIfprintf( stderr, ERx( "\t%s\n\n" ), releaseDir );
		PCexit( FAIL );
	}

	if( assemble_archives() != OK )
		PCexit(FAIL);

	if( compute_file_info() != OK )
		PCexit( FAIL );

	if( compute_archive_info() != OK )
		PCexit( FAIL );

	SIprintf( ERx( "\r%-80s" ), ERx( " " ) );

	/*
	** create install directory if non-existent in order to write the
	** manifest file.
	*/
	STcopy(releaseDir, lobuf);
	LOfroms(PATH, lobuf, &loc);
	LOfaddpath(&loc, ERx( "install" ), &loc);
	LOtos(&loc, &cp);
	if( IPCLcreateDir( cp ) != OK)
	{	
		SIfprintf( stderr,
		ERx( "Possible permission/ownership problem on directory:\n" ) );
		SIfprintf( stderr, ERx( "\t%s\n\n" ), cp );
		PCexit(FAIL);
	}

	if( write_manifest() != OK )
		PCexit(FAIL);

	SIprintf( ERx( "\n" ) );

	if( releaseDir != NULL )
	{
		SIprintf( ERx( "Finished building release area: %s\n\n" ),
			releaseDir);
	}
	MEfree(ingbuild_bin);
	if(license_file)
	    MEfree(license_file);


      /*
      ** Create build.timestamp file to mark build completion time
      ** if it doesn't already exist.
      */
      if ( build_timestamp != NULL )
      {
          LOCATION bts_loc; /* timestamp file loc */

          LOfroms(PATH & FILENAME, build_timestamp, &bts_loc);
          if ( LOexist(&bts_loc) != 0 )
          {
              SIprintf( "Creating build timestamp file: %s\n\n",
                                              build_timestamp );
              if ( LOcreate(&bts_loc) )
                  SIprintf( "Failed to create %s\n", build_timestamp );
          }
	  MEfree(build_timestamp);
	 
      }

	PCexit( OK );
}

/*
** message display functions; called from various library routines.
*/

void ip_display_transient_msg( char *msg )
{
	COND( SIprintf( ERx( "%s\n" ), msg ) );
}

void ip_display_msg( char *msg )
{
	COND( SIprintf( ERx( "%s\n" ), msg ) );
}

/*
** substitute_variables() - substitute variables found in manifest
*/
static STATUS 
substitute_variables()
{
    STATUS rtn;
    PKGBLK *pkg;
    PRTBLK *prt;
    FILBLK *fil;

    SCAN_LIST(distPkgList, pkg)
    {
	SCAN_LIST(pkg->prtList, prt)
        {
	    SCAN_LIST(prt->filList, fil)
            {
		/* Replace %LICENSE% with proper license file */
		if( STequal(fil->name, ERx("%LICENSE%")) )
		{
			license_file = MEreqmem(0,((STlength(license_type) + 8) * sizeof(char)) + 1, TRUE, &rtn);
			STprintf(license_file,"%s.%s", ERx("LICENSE"), license_type);
			fil->name = ERx("LICENSE");
			fil->build_file = license_file;
		}
	    }
	}
    }
    return OK;
}

/*
** compute_file_info() - calculate file sizes and checksums.
*/

static STATUS 
compute_file_info()
{
    PKGBLK *pkg;
    PRTBLK *prt;
    FILBLK *fil;
    LOCATION loc;
    char lobuf[ MAX_LOC + 1 ];

    /* mikehan */
    char cmd[ 2 * MAX_LOC + MAX_MANIFEST_LINE ];
    STATUS status;

    char *filname;
    i4  notfound = 0;
    i4  badperms = 0;
    bool display=FALSE;
    FILE *outf = audit? stdout: stderr; /* no "erros" during audit */
    FILE *plistf = NULL;
    LOCATION plistloc;
    i4 count;
    char plistbuf[ MAX_LOC + 1 ];
    char *reldir;

    if( plist )
    {
        
        STprintf(plistbuf, ERx( "%s/%s" ), ingbuild, PLIST_NAME ); 
        status = LOfroms(PATH & FILENAME, plistbuf, &plistloc);
        if( status == OK )
        {
            status = SIopen( &plistloc, "w", &plistf);
        }
        if ( status != OK )
        {
            SIprintf( ERx( "Unable to open %s/%s.\n\n" ), ingbuild, PLIST_NAME);
            PCexit( FAIL );
        }
    }

    SCAN_LIST(distPkgList, pkg)
    {
	/*
	** Hide spatial, i18n and documentation packages
	*/ 
	if( ( hide && 
	      ( STequal( pkg->feature, ERx("i18n") ) ||
	    	STequal( pkg->feature, ERx("i18n64") ) ||
	    	STequal( pkg->feature, ERx("documentation")) ) ) ||
	  ( ( !spatial )  &&
	      ( STequal( pkg->feature, ERx("spatial") ) ||
		STequal( pkg->feature, ERx("spatial64")) ) ) )
	{
		/* Mark as hidden an skip */
		pkg->hide=TRUE;
		continue;
	}

	SCAN_LIST(pkg->prtList, prt)
        {
	    SCAN_LIST(prt->filList, fil)
            {
		/* compose file name */
		STcopy( fil->build_dir, lobuf );
	        LOfroms( PATH, lobuf, &loc );
		LOfstfile( fil->build_file != NULL ? fil->build_file :
			fil->name, &loc );
	        LOtos( &loc, &filname );

		/* skip already reported missing files */
		if( fil->size == -1 )
			continue;

		if( fil->size != 0 )
		{
		    /* size/checksum already computed - check permissions */
		    get_permission( filname, fil );
		    if( !force && fil->permission_sb != fil->permission_is )
			badperms++;
		    continue;
		}
		
		/* 
		** if this file is a link to another file or "-f" was
		** passed on the command line, don't compute checksum.  
		*/

		if( fil->link ) 
		{
		    if( audit )
		    {
			char path[ MAX_LOC + 1 ];

			COND( STprintf( path, ERx( "%s/%s" ),
			    fil->directory, fil->build_file != NULL ?
				fil->build_file : fil->name );
			    SIprintf( ERx( "\r%-80s" ), path );
			    SIflush( stdout ) );
		    }
		    fil->exists = TRUE;
		    fil->size = 1000;
		    fil->checksum = 1000;
		    fil->permission_is = fil->permission_sb;
		    continue;
		}

		if( audit )
		{
		    char path[ MAX_LOC + 1 ];

		    STprintf( path, ERx( "%s/%s" ), fil->directory,
			fil->build_file != NULL ? fil->build_file :
			fil->name );
		    COND( SIprintf( ERx( "\r%-80s" ), path );
			SIflush( stdout ) );

                    if(plist)
                    {
                        /* Strip ingbuild form the path to give
                        ** ./bin/iimerge format
                        */
                        reldir = STindex(fil->directory, "/", 0);
                        if(reldir)
                        {
                            reldir++;
                            STprintf(plistbuf, ERx( "./%s/%s\n" ), reldir,
                                 fil->name ); 
                        }
                        else
                        {
                            STprintf(plistbuf, ERx( "./%s\n" ),
                                 fil->name );
                        }
                        SIwrite(STlength(plistbuf), plistbuf, &count, plistf);
                    }
		}
		else
		{
		    COND( SIprintf( 
			ERx( "\rComputing file checksums... %-16s  %-16s  %-16s" ),
			pkg->feature, prt->name, fil->build_file != NULL ?
				fil->build_file : fil->name );
			SIflush(stdout) );
		}

		if( OK != ip_file_info_comp( &loc, &fil->size,
		    &fil->checksum, audit ) )
		{
		    /* 
		    ** since ip_file_info_comp() detected an error,
		    ** the file doesn't exist, so complain.
		    */
		    if( !force )
			notfound++;
		    else
		    {
			char buf[ 1024 ];

			STprintf( buf, ERx( "**Ignoring missing file: %s " ),
				filname );
    			COND( SIfprintf( outf, ERx( "\r%-80s\n" ), buf ) );
		    }
		    fil->exists = FALSE;
		}
		else
		{
		    /* 
		    ** since the file exists, verify its permissions. 
		    ** correct them if they're wrong 
		    */
		    get_permission( filname, fil );
		    if( fil->permission_sb != fil->permission_is )
		    {
                        SIfprintf( outf,
                            ERx( "\nPerm %o should be %o for %s\n" ), 
                            fil->permission_is, fil->permission_sb,
                            filname );

                          /* Fix permission as well - mikehan */
                        SIfprintf( outf,
                            ERx( "Permisson set to %o for %s\n\n" ),
                            fil->permission_sb, filname );

			STprintf( cmd,
			    ERx( "chmod %o %s" ),
			    fil->permission_sb, filname );

			if( (status = ip_cmdline( cmd,
				E_ST0205_chmod_failed )) == OK)
			    /* set perms as correct in fil */
			    fil->permission_is=fil->permission_sb;
			else
			    badperms++;
			

		    }
		}
	    }
        }
    }

    COND( SIfprintf( outf, ERx( "\r%-80s\r" ), ERx( " " ) ) );

    /* If we've found anything we'll have to complain about... */

    if( (notfound || badperms) )
    {
	i4 badfiles = notfound + badperms;

	/* Summarize the problem files... */

	if( notfound )
	{
	    SIfprintf( outf,
		ERx( "%d file%s not found%s" ), notfound,
		notfound==1? ERx( "" ): ERx( "s" ), badperms ?
		ERx( ", and " ) : ERx( "" ) );
	}
	if( badperms )
	{
	    SIfprintf( outf, ERx( "%d file%s had incorrect permissions" ),
		badperms, badperms == 1 ? ERx( "" ) : ERx( "s" ) );
	}

        /* 
	** If we're auditing, or if there aren't very many bad files,
	** or we're in no-questions mode, we'll just go ahead and display;
	** otherwise, we'll see what the user wants to do.  
	*/

	if( display && !audit )
	    SIfprintf( outf, ERx( ":\n" ) );
	else if ( batch )
	    display = TRUE ;
	else
	{
	    SIfprintf( outf, ERx( ".  Display %s? " ), badfiles == 1 ?
		ERx( "it" ) : ERx( "them" ) );
	    display = getyorn();
	}

	/* 
	** If we're going to display the bad files, do it.
	** NOTE: it would be nice to make this pause between screenfuls.  
	*/

	if( display )
	{
	    /* Do the non-existent files first. */

	    SIfprintf( outf, ERx( "\n" ) );
	    SCAN_LIST(distPkgList, pkg)
	    {
		/*
		** Hide i18n and documentation package
		*/ 
		if(pkg->hide) continue;

		SCAN_LIST(pkg->prtList, prt)
		{
		    SCAN_LIST(prt->filList, fil)
		    {
			if( fil->size == -1 )
			{
			    STcopy( fil->build_dir, lobuf );
			    LOfroms( PATH, lobuf, &loc );
			    LOfstfile( fil->build_file != NULL ?
				fil->build_file : fil->name, &loc );
			    LOtos( &loc, &filname );
			    if( !force )
			    {
				SIfprintf( outf, ERx( "Not found: %s\n" ),
				    filname );
			    }
			}
		    }
		}
	    }

            if( notfound && badperms )
		SIfprintf( outf, ERx( "\n" ) );

	    /* Then do the bad permission files. */

	    SCAN_LIST(distPkgList, pkg)
	    {
		/*
		** Hide i18n and documentation package
		*/ 
		if(pkg->hide) continue;

		SCAN_LIST(pkg->prtList, prt)
		{
		    SCAN_LIST(prt->filList, fil)
		    {
			if ( fil->permission_sb != fil->permission_is &&
			     fil->size != -1 )
			{
			    STcopy( fil->build_dir, lobuf );
			    LOfroms( PATH, lobuf, &loc );
			    LOfstfile( fil->build_file != NULL ?
				fil->build_file : fil->name, &loc );
			    LOtos( &loc, &filname );
			    SIfprintf( outf,
				ERx( "Perm could not be set to %o for %s\n" ),
				fil->permission_sb, filname );
			}
		    }
		}
	    }
	}

    	COND( SIprintf( ERx( "\n" ) ) );
    }

    if(plistf)
    {
        status = SIclose(plistf);
    }
    
    return( notfound > 0 || badperms > 0 ? FAIL : OK );
}

/*
** get_permission() - get permissions for a file.
**
** sets the "permission_is" field of a FILBLK structure to the current
** permission setting for the referenced file.  
****
    Heavily Unix-dependent!!
****
*/

static void
get_permission( char *filname, FILBLK *fil )
{
	struct stat statbuf;
	char basefile[ MAX_LOC + 1 ];
	char *subdir;

	fil->permission_is = 
		( fil->permission_sb == -1 || stat( filname, &statbuf ) ) ?
		-1 : statbuf.st_mode & 07777;
        if(fil->permission_is == -1 && privatePath)
	{
            /* Try the base area */
	    STcopy(buildhome, basefile);
            STcat(basefile, "/");
            STcat(basefile, base_vers); 
            subdir = (char *)filname +
                STlength(buildhome) + 1 +
                STlength(ing_vers);
            STcat(basefile, subdir);
	    
	    if(stat( basefile, &statbuf ) == OK )
	    {
		fil->permission_is = statbuf.st_mode & 07777;
	    }
	}
}

/*
** compute_archive_info() - compute package sizes and checksums.
*/

static STATUS
compute_archive_info()
{
	PKGBLK *pkg;
	LOCATION tarloc;
	char tarbuf[ MAX_LOC + 1 ];
	char *tarfil;
	LOINFORMATION inf;
	i4 infflag;

	STATUS rc;

	SCAN_LIST( distPkgList, pkg )
	{
		if( pkg->image_size != 0 )
			continue;

		/*
		** Hide i18n and documentation package
		*/ 
		if(pkg->hide) continue;

		STcopy(releaseDir, tarbuf);
		LOfroms(PATH, tarbuf, &tarloc);
		LOfaddpath(&tarloc, pkg->feature, &tarloc);
		LOfstfile( INSTALL_ARCHIVE, &tarloc);
		LOtos(&tarloc, &tarfil);

  		infflag = LO_I_ALL;

		if( OK == LOinfo( &tarloc, &infflag, &inf ) )
		{
			COND( SIprintf( ERx( "\rComputing archive checksums... %-32s" ),
				pkg->name); SIflush(stdout) );

			if( OK != ip_file_info_comp( &tarloc,
				&pkg->image_size, &pkg->image_cksum, audit ) )
			{
				SIfprintf( stderr,
					ERx( "\nProblem computing checksum for: %s\n" ),
					tarfil);
				pkg->image_size = pkg->image_cksum = -1;
			}
		}
	
	}
	return( OK );
}

/*
** assemble_archives() - move all files into each subdir.
*/

static STATUS
assemble_archives()
{
	PKGBLK *pkg;
	PRTBLK *prt;
	FILBLK *fil;
	LOCATION pkgloc, tmploc, tarloc;
	char pkgbuf[ MAX_LOC + 1 ];
	char tmpbuf[ MAX_LOC + 1 ];
	char tarbuf[ MAX_LOC + 1 ];
	char *pkgdir, *tmpdir, *tarfil;
	char path[ MAX_LOC + 1 ];
	char basedir[ MAX_LOC + 1 ];
	CL_ERR_DESC cl_err;
	char *subdir;
	char cmd[ 2 * MAX_LOC + MAX_MANIFEST_LINE ];
	STATUS status;
	char *dirp;

	batchMode = TRUE;

	ip_list_init( &dirList );

	SCAN_LIST( distPkgList, pkg )
	{
		i4 nfiles = 0;
		bool fail = FALSE;

		/*
		** Hide i18n and documentation package
		*/ 
		if(hide)
		{
			if( STequal( pkg->feature, ERx("i18n") ) ||
			    STequal( pkg->feature, ERx("i18n64") ) ||
			    STequal( pkg->feature, ERx("documentation") ))
			{
				COND( SIprintf( ERx( "\nHiding package: %s\n" ),
					 pkg->feature); );
				pkg->hide = TRUE;
				continue;
			}
		}

		/*
		** Hide spatial package
		*/
	 	if(!spatial)
		{
		    if( STequal( pkg->feature, ERx("spatial") ) ||
			STequal( pkg->feature, ERx("spatial64") ))
		    {
			COND( SIprintf( ERx( "\nHiding package: %s\n" ),
                                         pkg->feature); );
			pkg->hide = TRUE;
                                continue;
                    }
                }



		/*
		** compose path of directory into which package
		** files will be copied. 
		*/ 
		STcopy( releaseDir, pkgbuf );
		LOfroms( PATH, pkgbuf, &pkgloc );
		LOfaddpath( &pkgloc, pkg->feature, &pkgloc );
		LOtos( &pkgloc, &pkgdir );

		/*
		** compose path of temporary directory used to
		** hold files during package assembly.
		*/
		LOcopy( &pkgloc, tmpbuf, &tmploc );
		LOfaddpath( &tmploc, ERx( "tmp" ), &tmploc );
		LOtos( &tmploc, &tmpdir );

		/*
		** create tar file LOCATION relative to current
		** directory.
		*/ 
		STcopy( pkgdir, tarbuf );
		LOfroms( PATH, tarbuf, &tarloc );
	    	LOfstfile( INSTALL_ARCHIVE, &tarloc );
		LOtos( &tarloc, &tarfil );

		if( LOexist( &tarloc ) == OK &&
			LOexist( &tmploc ) != OK )
		{
			/*
			** archive exists but temporary directory
			** does not - therefore assume that the
			** archive was created successfully (i.e.
			** don't recreate it).
			*/
			COND ( SIprintf(
			ERx( "\n\"%s\" archive assembled.\n" ),
			pkg->name ) );
			continue;
		}

		COND( SIprintf( ERx( "\nAssembling \"%s\" archive:\n" ),
			pkg->name ) );

		COND( SIprintf( ERx( "  Computing file checksums" ) );
			SIflush(stdout) );

		/* Compute file sizes and checksums for package */
		SCAN_LIST( pkg->prtList, prt )
		{
			SCAN_LIST( prt->filList, fil )
			{
				LOCATION filloc;
				char filbuf[ MAX_LOC + 1 ];
				char *filnam;

				++nfiles;

				STcopy( fil->build_dir, filbuf );
				LOfroms( PATH, filbuf, &filloc );
	    				LOfstfile( fil->build_file != NULL ?
						fil->build_file : fil->name,
						&filloc );
	    				LOtos( &filloc, &filnam );
				status = ip_file_info_comp( &filloc,
					&fil->size, &fil->checksum, FALSE );

				if( (status != OK) && privatePath)
				{
				    /* Try the base area */
				    STcopy(buildhome, basedir);
				    STcat(basedir, "/");
				    STcat(basedir, base_vers); 
                                    subdir = (char *)fil->build_dir +
						STlength(buildhome) + 1 +
						STlength(ing_vers);
				    STcat(basedir, subdir);

				    STcopy( basedir, filbuf );
				    LOfroms( PATH, filbuf, &filloc );
	    				    LOfstfile( fil->build_file != NULL ?
						    fil->build_file : fil->name,
						    &filloc );
	    				    LOtos( &filloc, &filnam );
				    status = ip_file_info_comp( &filloc,
					    &fil->size, &fil->checksum, FALSE );
				}

				if( status != OK)
				{
					if( force )
					{
						fil->exists = FALSE;
						--nfiles;
						COND( SIprintf(
							ERx( "\n**Ignoring missing file: %s" ),
							filnam ) );
					}
					else
					{
						fail = TRUE;
						SIprintf(
							ERx( "\n**Missing file: %s\n" ),
							filnam );
					}
				}
				else
				{
					COND( SIprintf( ERx( "." ) );
						SIflush(stdout) );
				}
			}
		}

		if( nfiles == 0 )
		{
			COND( SIprintf( ERx( "\r  Empty package requires no archive.\n" ) ) );
			pkg->image_size = pkg->image_cksum = -1;
			continue;
		}

		if( fail )
		{
			SIprintf( ERx( "\nUnable to build release.\n" ) );
			SIprintf( ERx( "\nUse \"buildrel -a\" to generate a report of specific problems.  Once\n" ) );
			SIprintf( ERx( "they have been corrected, re-run buildrel to finish generating the release\n" ) );
			SIprintf( ERx( "area.  Use \"buildrel -f\" to ignore bad files if necessary.\n\n" ) );
			PCexit( FAIL );
		}

		COND( SIprintf( ERx( "\n  Copying files" ) );
			SIflush(stdout) );

		SCAN_LIST( pkg->prtList, prt )
		{
			SCAN_LIST( prt->filList, fil )
			{
				char dest[ MAX_LOC + 1 ];
				char cp_dest[ MAX_LOC + 1 ];
				struct stat statbuf;

				if( !fil->exists )
					continue;

				COND( SIprintf( ERx( "." ) );
					SIflush( stdout ) );

				/* create target path */
				STprintf( dest,
					ERx( "%s/%s" ),
					tmpdir,
					fil->directory );

				if( fil->link )
				{
					/* create symbolic link with 'ln' */
					STprintf( cmd,
					    ERx( "cd %s; ln -s ./%s %s" ),
					    dest, fil->build_file,
					    fil->name ); 

					/* execute 'ln' command */
					if( (status = ip_cmdline( cmd,
						E_ST0204_ln_failed ))
						!= OK )
					{
						SIprintf(
							ERx( "\nUnable to make symbolic link '%s' to\n\n\t%s/%s\n" ),
							fil->name, tmpdir,
							fil->build_file );
						goto cleanup;
					}
					continue;
				}	

				/*
				** Check whether the directory exists,
				** Create it if not.
				*/
				if( stat( dest, &statbuf ) ) {
				/* construct 'mkdir' command */
				STprintf( cmd,
# if defined(su4_u42)
					ERx( ERx( "mkdir -p %s" ) ),
# else
			ERx( ERx( "umask 022 ; mkdir -m 755 -p %s" ) ),
# endif
					dest ); 

				/* execute 'mkdir' command */
				(void) ip_cmdline( cmd,
					(ER_MSGID) NULL );
				}

				/* construct file path */
				STprintf( path,
					ERx( ERx( "%s/%s" ) ),
					fil->build_dir,
					fil->build_file != NULL ?
					fil->build_file : fil->name );

				/* deal with BUILD_FILE directive */
				if( fil->build_file != NULL )
				{
					STpolycat( 3, dest, ERx( "/" ),
						fil->name, cp_dest );
				}
				else
					STcopy( dest, cp_dest );

                                if(privatePath && 
				     (stat( path, &statbuf ) != OK ))
				{
                                    /* Try the base area */
                                    STcopy(buildhome, basedir);
                                    STcat(basedir, "/");
                                    STcat(basedir, base_vers);
                                    subdir = (char *)fil->build_dir +
                                                STlength(buildhome) + 1 +
                                                STlength(ing_vers);
                                    STcat(basedir, subdir);
 
                                    /* construct base file path */
                                    STprintf( path,
                                            ERx( ERx( "%s/%s" ) ),
                                            basedir,
                                            fil->build_file != NULL ?
                                            fil->build_file : fil->name );
				}

				/* construct 'cp' command */
				STprintf( cmd,
#if defined(usl_us5) || defined(sos_us5) || defined(nc4_us5)
					ERx( ERx( "cp  %s %s" ) ),
#else
					ERx( ERx( "cp -p %s %s" ) ),
#endif /* usl_us5 */
					path, cp_dest );

                                /* execute 'cp' command */
                                if( (status = ip_cmdline( cmd,
                                        E_ST0203_cp_failed ))
                                        != OK )
				{
					SIprintf(
						ERx( "\nUnable to copy %s to %s\n" ),
						path, cp_dest );
					goto cleanup;
				}

				/* construct 'chmod' command */
				STprintf( cmd,
					ERx( "chmod %o %s/%s" ),
					fil->permission_sb, dest, fil->name );

				/* execute 'chmod' command */
				if( (status = ip_cmdline( cmd,
					E_ST0205_chmod_failed ))
					!= OK )
				{
					SIprintf(
						ERx( "\nUnable to chmod %s to %o\n" ),
						dest, fil->permission_sb );
					goto cleanup;
				}

				if( STequal( pkg->feature,
					ERx( "install" ) ) )
				{
					/* deal with BUILD_FILE directive */
					if( fil->build_file != NULL )
					{
						STpolycat( 3, pkgdir, ERx( "/" ),
							fil->name, cp_dest );
					}
					else
						STcopy( pkgdir, cp_dest );
					/*
					** this is the
					** "install" package,
					** so copy file into
					** package directory.
					*/
					STprintf( cmd,
#if defined(usl_us5)
						ERx( "cp  %s %s" ),
#else
						ERx( "cp -p %s %s" ),
#endif /* usl_us5 */
						path, cp_dest );

					if( (status = ip_cmdline(
						cmd,
						E_ST0203_cp_failed ))
						!= OK )
					{
						SIprintf(
							ERx( "\nUnable to copy %s to %s\n" ),
							path, pkgdir );
						goto cleanup;
					}
				}

				/*
				** TARGET DIRECTORY PERMISSIONS SHOULD BE
				** SPECIFIED IN THE MANIFEST, NOT TAKEN
				** FROM THE BUILD DIRECTORY.
				*/

				/* get source dir perms */
				status = stat( fil->build_dir, &statbuf );

				if( (status != OK) && privatePath )
				{
				    /* Try the base area */
				    STcopy(buildhome, basedir);
				    STcat(basedir, "/");
				    STcat(basedir, base_vers); 
                                    subdir = (char *)fil->build_dir +
						STlength(buildhome) + 1 +
						STlength(ing_vers);
				    STcat(basedir, subdir);

				    status = stat(basedir, &statbuf );
				}

				if( status != OK )
				{
					SIfprintf( stderr,
						ERx( "\nCan't stat: %s\n" ),
						fil->build_dir );
					perror( ERx( "stat" ) );
					status = FAIL;
					goto cleanup;
				}

				/* set target dir perms */
				if( chmod( dest,
					statbuf.st_mode ) )
				{
					SIfprintf( stderr,
						ERx( "\nCan't chmod: %s\n" ),
						dest );
					perror( ERx( "chmod" ) );
					status = FAIL;
					goto cleanup;
				}
			}
		}

		/* create package tar file... */
		COND( SIprintf( ERx( "\n  Creating archive... " ) );
			SIflush( stdout ) );

		/*
		** compose tar file path (relative to "tmp"
		** subdirectory) necessary for tar command sequence.
		*/ 
		STprintf( tarbuf, ERx( "../%s" ), INSTALL_ARCHIVE );
		tarfil = tarbuf;

		/* compose tar command */
# if defined(LNX)
                STpolycat( 9,
                        ERx( " cd " ),
                        tmpdir,
                        ERx( " ; " ),
                        ingbuild_bin,
                        ERx( "pax -wzf " ),
                        tarfil,
# ifdef conf_LSB_BUILD
                        ERx( " *" ),
# else
                        ERx( " ingres" ),
# endif
                        ERx( " ;" ),
                        ERx( " sleep 1" ),
                        cmd );
# elif defined(any_aix)
                 STpolycat( 7,
                        ERx( " cd " ),
                        tmpdir,
                        ERx( " ; " ),
                        ERx( "/usr/bin/tar -cf - ingres" ),
                        ERx( " | compress " ),
                        ERx( " > " ),
                        tarfil,
                        cmd );
# else
                STpolycat( 7,
                        ERx( " cd " ),
                        tmpdir,
                        ERx( " ; " ),
			ERx( "pax -w ingres" ),
                        ERx( " | compress " ),
                        ERx( " > " ),
                        tarfil,
                        cmd );
# endif

		/* execute tar command */
		if( STcompare( pkg->feature, ERx( "install" ) ) != 0 )
		{
                    /* compress may return rc=2; this is acceptable */
		    status = ip_cmdline( cmd, (int)NULL );
		    if( status == OK || status == 2 )
		    {
			status = OK;

			/* compute size and checksum of tar file... */

			COND (
				SIprintf( ERx( "\n  Computing archive checksum... " ) );
					SIflush( stdout ) );

			/*
			** create tar file LOCATION relative to
			** current directory for checksum calculation.
			*/ 
			STcopy( pkgdir, tarbuf );
			LOfroms( PATH, tarbuf, &tarloc );
    			LOfstfile( INSTALL_ARCHIVE, &tarloc );
			LOtos( &tarloc, &tarfil );

			if( ip_file_info_comp( &tarloc,
				&pkg->image_size, &pkg->image_cksum,
				FALSE ) != OK )
			{
				SIfprintf(stderr,
					ERx( "\n\nProblems computing checksum for: %s\n" ),
					tarfil );
				status = FAIL;
			}
		    }
		    else
		    {
			SIfprintf(stderr, ERx( "\n%s\n" ), ERget(E_ST0201_tar_failed) );
		    }
		}

cleanup:	COND( SIprintf( ERx( "\n  Removing temporary files... " ) );
			SIflush( stdout ) );

		IPCLdeleteDir( tmpdir );

		COND( SIprintf( ERx( "\n" ) ) );

		if( status != OK )
		{
			/* remove package directory on error */ 
			SIprintf( ERx( "  Cleaning up...\n" ) );
			IPCLdeleteDir( pkgdir );
			return( status );
		}

		ip_add_inst_pkg( pkg );
	}

	COND( SIprintf( ERx( "\n" ) ) );

	return( OK );
}

/*  
** write_manifest() - write RELEASE_MANIFEST in the release area. 
*/

static STATUS
write_manifest()
{
	char *filename;

	COND( SIprintf( ERx( "\rWriting %s/%s...\n" ), installDir,
		RELEASE_MANIFEST ) );
	filename = RELEASE_MANIFEST;

	if ( OK != ip_write_manifest( DISTRIBUTION, installDir, &distPkgList,
		distRelID, releaseName ) )
	{
		SIfprintf( stderr, ERx( "Error writing %s!\n" ),
			RELEASE_MANIFEST );
		PCexit( FAIL );
	}
        return( OK );
}

/*
** ip_forms() - dummy stub
*/

void
ip_forms(bool yesno)
{
	return;
}

/*
** getyorn() - get "yes" or "no" response from the keyboard.
*/

static bool
getyorn()
{
	i4 chr;
	bool rtn;

	for (;;)
	{
		chr = SIgetc(stdin);
		switch (chr)
		{
			case 'n':
			case 'N':  rtn = FALSE; 
				   break;
			case 'y':
			case 'Y':  rtn = TRUE;  
				   break;
			default:   SIprintf( ERx( "\n(y/n) ? " ) );
				   continue;
		}
	break;
	}

	for ( ; SIgetc( stdin ) != '\n' ; );

	return( rtn );
}

/*
** ip_dep_popup() - dummy stub.
*/

bool
ip_dep_popup(PKGBLK *pkg1, PKGBLK *pkg2, char *oldvers)
{
	return( TRUE );
}

/*
** dumpxml() - dump release data in XML format for external processing
*/

static void
dumpxml()
{
        PKGBLK *pkg;
 
        SIprintf( ERx( "<RELEASE>\n" ) );
        SIprintf( ERx( "  <NAME>%s</NAME>\n" ), releaseName );
        SIprintf( ERx( "  <ID>%s</ID>\n" ), distRelID );
        SIprintf( ERx( "  <MANIFEST>%s</MANIFEST>\n" ), manifestDir );
        SCAN_LIST(distPkgList, pkg)
        {
                REFBLK *ref;
                PRTBLK *prt;
 
                if( ip_list_is_empty( &pkg->prtList ) )
                        continue;
 
                SIprintf( ERx( "  <PACKAGE>\n" ) );
                if( pkg->file != NULL )
                        SIprintf( ERx( "    <DEFFILE>%s</DEFFILE>\n" ), pkg->file );
                SIprintf( ERx( "    <NAME>%s</NAME>\n" ), pkg->name );
                SIprintf( ERx( "    <VERSION>%s</VERSION>\n" ), pkg->version );
                SIprintf( ERx( "    <FEATURE>%s</FEATURE>\n" ), pkg->feature );
                //SIprintf( ERx( "    <DESCRIPTION>%s</DESCRIPTION>\n" ), pkg->description );
                SIprintf( ERx( "    <TAG>%s</TAG>\n" ), pkg->tag );
 
                SCAN_LIST(pkg->prtList, prt)
                {
                        FILBLK *fil;
 
                        SIprintf( ERx( "    <PART>\n" ) );
                        if( prt->file != NULL )
                                SIprintf( ERx( "      <DEFFILE>%s</DEFFILE>\n" ), prt->file );
                        SIprintf( ERx( "      <NAME>%s</NAME>\n" ), prt->name );
                        if( prt->version != NULL )
                                SIprintf( ERx( "      <VERSION>%s</VERSION>\n" ), prt->version );
 
                        SCAN_LIST(prt->refList, ref)
                        {
                                SIprintf( ERx( "      <REFERENCE>\n" ) );
                                SIprintf( ERx( "        <NAME>%s</NAME>\n" ), ref->name );
                                if( ref->version != NULL )
                                        SIprintf( ERx( "        <VERSION>%s</VERSION>\n" ), ref->version );
                                SIprintf( ERx( "        <COMPARISON>%d</COMPARISON>\n" ), ref->comparison );
                                SIprintf( ERx( "        <TYPE>%d</TYPE>\n" ), ref->type );
                                SIprintf( ERx( "      </REFERENCE>\n" ) );
                        }
 
                        SCAN_LIST(prt->filList, fil)
                        {
                                SIprintf( ERx( "      <FILE>\n" ) );
                                SIprintf( ERx( "        <NAME>%s</NAME>\n" ), fil->name );
                                if( fil->build_file != NULL )
                                        SIprintf( ERx( "        <BUILD_FILE>%s</BUILD_FILE>\n" ), fil->build_file );
                                SIprintf( ERx( "        <BUILD_DIR>%s</BUILD_DIR>\n" ), fil->build_dir );
                                SIprintf( ERx( "        <DIR>%s</DIR>\n" ), fil->directory );
                                SIprintf( ERx( "        <GENDIR>%s</GENDIR>\n" ), fil->generic_dir );
                                SIprintf( ERx( "      </FILE>\n" ) );
                        }
 
                        SIprintf( ERx( "    </PART>\n" ) );
                }
 
                SCAN_LIST(pkg->refList, ref)
                {
                        SIprintf( ERx( "    <REFERENCE>\n" ) );
                        SIprintf( ERx( "      <NAME>%s</NAME>\n" ), ref->name );
                        if( ref->version != NULL )
                                SIprintf( ERx( "      <VERSION>%s</VERSION>\n" ), ref->version );
                        SIprintf( ERx( "      <COMPARISON>%d</COMPARISON>\n" ), ref->comparison );
                        SIprintf( ERx( "      <TYPE>%d</TYPE>\n" ), ref->type );
                        SIprintf( ERx( "    </REFERENCE>\n" ) );
                }
 
                SIprintf( ERx( "  </PACKAGE>\n" ) );
        }
        SIprintf( ERx( "</RELEASE>\n" ) );
 
        return;
}
