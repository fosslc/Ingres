/*
** Copyright (c) 1993, 2004 Ingres Corporation
**
# define DEBUG_SETUP ON 
# define DEBUG_DELETE ON 
# define DEBUG_RESOLVE ON
*/

# include <compat.h>
# include <si.h>
# include <st.h>
# include <me.h>
# include <gc.h>
# include <ex.h>
# include <er.h>
# include <cm.h>
# include <gl.h>
# include <sl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <ui.h>
# include <pc.h>
# include <lo.h>
# include <iplist.h>
# include <erst.h>
# include <uigdata.h>
# include <stdprmpt.h>
# include <ip.h>
# include <ipcl.h>
# include <nm.h>
# include <cv.h>
# include "ipparse.h"
# include <iphash.h>
# include <sys/stat.h>

/*
**  Name: ipfile.sc -- file-handling functions for programs which generate
**	and install INGRES distributions.
**
**  Entry points:
**
**	ip_add_inst_pkg -- add a package to the installation list
**	ip_cmdline -- execute a command line via a shell
**	ip_comp_version -- compare two version identifiers
**	ip_compare_name -- compare two package names
**	IPCLdeleteDir -- delete a directory and its contents
**	ip_delete_dist_pkg -- remove the image associated with a package
**	ip_delete_inst_pkg -- remove a package from the installation
**	ip_describe -- display description information for a package
**	ip_file_info -- compute file size and checksum
**	ip_file_info_comp -- alternate entry into ip_file_info
**	ip_find_dir -- return the directory associated with a file
**	ip_find_feature -- find the package structure for a feature name
**	ip_find_package -- find the package structure for a package name
**	ip_init_installdir -- create the private install directory if needed
**	ip_install_file -- figure out full path name of an installation file
**	ip_install_loc -- return path name of a installation location
**	ip_is_visible -- figure out if a package is visible
**	ip_is_visiblepkg -- figure out if a package is visiblepkg
**	ip_listpick -- general 2-choice list pick handler
**	ip_listpick3 -- general 3-choice list pick handler
**	ip_oemstring -- get licensing information from special OEM file
**	ip_overlay -- overlay file size/checksum info (for patch installation)
**	ip_preserve_cust_files -- preserve user customized files
**	ip_preload -- call pre-transfer program( s ) for a package
**	ip_resolve -- make sure all required packages are selected
**	ip_setup -- run setup program( s ), or see if they exist
**	ip_sysstat -- check to see if INGRES system is running
**	ip_tape_dir -- add a directory to the list to be extracted
**	ip_tape_init -- initialize list of directories to be extracted
**	ip_tape_restore -- extract selected directories
**	ip_verify_files -- verify checksums and sizes of files
**	ip_write_manifest -- write the manifest or installation descriptor file
**	ip_xfer_package -- transfer a package onto the installation area
** 	ip_open_response - open a response file for writing.
** 	ip_write_response - writes out a response file.
** 	ip_close_response - close a response file if open.
** 	ip_chk_response_pkg - add a package to the response file.
**
**  History:
**	12-aug-92 ( jonb )
**		Created.
**	17-feb-93 ( jonb )
**		Always write state information to the installation file.
**	19-feb-93 ( jonb )
**		Make ip_create_dir available to VMS version.
**	11-mar-93 ( kellyp )
**		Generate a different message when installing support packages
**	16-mar-93 ( jonb )
**		Brought comment style into conformance with standards.
**	23-mar-93 ( kellyp )
**		Support preservation of user customized files
**	31-mar-93 ( kellyp )
**		Removes all references to deleted packages unless they
**		are referenced by some other package.
**	2-apr-93  ( kellyp )
**		Do not verify files with the keyword DYNAMIC associated to it
**	13-jul-93 ( tyler )
**		Changes to support the portable manifest language.
**	14-jul-93 ( tyler )
**		Replaced changes lost from r65 lire codeline.
**	04-aug-93 ( tyler )
**		Stopped using "volume" field in PKGBLK and PRTBLK structures.
**		Fixed bug which caused installation tool to attempt to
**		verify degenerate ( empty ) package.  Reformatted file.
**	13-aug-93 ( kellyp )
**		Made necessary changes to make it work with modified 
**		GENERATE_VERSION_TOOL.
**	23-aug-93 (dianeh)
**		Changed instances of Ingres to INGRES in error messages,
**		where appropriate.
**	19-sep-93 (kellyp)
**		Added setup to the file structure
**	19-sep-93 (kellyp)
**		Another revision simply to add previous History comment.
**	20-sep-93 (kellyp)
**		Modified for the new way in which SETUP works.
**		Also fixed the bug which did not account for the limitation of
**		LOfaddpath which can only contain a single directory name.
**	24-sep-93 (tyler)
**		Modified the way SETUP keyword is emitted in generated
**		release/installation description files.  Eliminated
**		blank lines from generated files.  Stopped generating
**		default (value = 0) WEIGHT directive.
**	19-oct-93 (tyler)
**		Stopped using incredibly hokey method of using defined
**		symbols GENERATE_VERSION and GENERATE_VERSION_TOOL to call
**		a program which generates the version string.  ipparse.y
**		now includes <gv.h> and refers directly to GV_VER.  Removed
**		defunct function ip_fix_version().  Fixed bad call to 
**		STgetwords() found by vijay.  Changed install.log to
**		install.err since it's not a general log of installation
**		activity.
**	02-nov-93 (tyler)
**		Added support for the PREFER directive which can be used
**		to indicate that certain package(s) should be set up before
**		others when installed together.  Removed support for the
**		WEIGHT directive which wasn't really used and didn't make
**		sense.  Modified indentation of CAPABILITY lines in
**		generated files.
**	04-nov-93 (tyler)
**		Fixed bug which prevented the PREFER and NEED directives
**		from being included in install.dat.  This caused packages
**		in the installation contents list to be sorted incorrectly,
**		which their setup programs to be executed in the wrong order.
**		Clear the screen before running each setup program in forms
**		mode.  This used to work, but for some mysterious reason
**		it was broken.
**	09-nov-93 (kellyp)
**		Fixed a bug in ip_file_info_comp() which does not recognize 
**		files that are of 'Relative' file organization vs the normal
**		which is 'Sequential' when trying to do SIfopen.(VMS only)
**		On UNIX, this problem doesn't come up since all files are
**		treated as text.
**	11-nov-93 (tyler)
**		Made previous change apply to both VMS and Unix since 
**		the added calls to fopen() which specify SI_RACC and
**		SI_VAR file types are harmless on Unix, thereby eliminating
**		platform dependent code.  Ported to IP Compatibility
**		Layer.
**	18-nov-93 (kellyp)
**		Write permission to the release.dat file for VMS.
**	22-nov-93 (tyler)
**		Removed unused code.
**	02-dec-93 (kellyp)
**		Corrected printing of CUSTOM, DYNAMIC, and PERMISSION.
**	10-dec-93 (tyler)
**		Replaced LOfroms() with calls to IPCL_LOfroms().
**	05-jan-94 (tyler)
**		Removed support for SUBSUME keyword.  Removed dead code.
**		Fixed broken estimation of disk space requirements.  Fixed
**		broken ip_delete_inst_pkg().  Modified ip_describe() to use
**		message stored in "description" field, rather than reading
**		from a file the name of which is stored in "description"
**		field. 
**	12-jan-94 (tyler)
**		Make sure all installed packages are marked NOT_SELECTED
**		when ip_resolve returns.  Fixed BUG in is_needed_by()
**		which prevented removal of VisionPro.
**	22-feb-94 (tyler)
**		Addressed SIR 58920: check Unix $PATH by calling iibintst
**		and iiutltst before trying to run program(s).  NOTE:
**		this should really be handled by an IPCL function.  Fixed
**		a couple of other unreported BUGS in resolve(): size
**		estimates for packages with PREFER dependecies are now
**		correct, and resolve() now checks the version number
**		when deciding whether a package is already installed.
**	01-mar-94 (tyler)
**		Make sure Unix $PATH check only happens for Unix.
**      16-feb-95 (lawst01)  bug 66788
**              before forking the logstat executable - reset the original
**              environment by calling function 'IPCLclearTmpEnv()' and
**              after returning from logstat restore the temporary environment
**              by calling function 'IPCLresetTmpEnv()'.
**	01-jun-95 (forky01)
**		Now check all INCLUDED packages for licensing by CI bits for a
**		a package recursively.  This required rewriting ip_licensed.
**		Also fix a problem to allow any of a set of CI bits for a PART
**		to license the part.  For example, the ABF package has the
**		CI_ABF and CI_VISION specified.  The package is licensed when
**		either of these two are set.  Currently ip_licensed would
**		have returned FAIL unless both bits were set.  All this is
**		for bug 68693.
**	19-07-95 (harpa06)
**		Added code to run "iistar -rmpkg" and "iisunet -rmpkg" when
**		the packages are being removed from INGBUILD. The "-rmpkg"
**		flag deletes all references to a particular server in the
**		config.dat file.
**	11-nov-95 (hanch04)
**		Added new function ip_is_visiblepkg to check if 
**		pgk->visible is VISIBLEPKG
**	29-nov-95 (hanch04)
**		Running express install will not display install scripts.
**	19-jan-96 (hanch04)
**		RELEASE_MANIFEST is now releaseManifest
**	15-mar-1996 (angusm)
**		SIR 75443 - various changes to allow 'ingbuild' to be used
**		for installation of patches. 
**		a) Alter validation of PRELOAD and DELETE to validate first 
**		word of string only, to allow specification of one or 
**		more arguments to PRELOAD/DELETE programs.
**		b) add ip_overlay, to update in-memory copy of installation
**		description file (for patch installation)
**    	01-Mar-96 (rajus01)
**		Added progname "iisubr" for Protocol Bridge.
**	 2-jul-96 (nick)
**		Don't use logstat to determine availability / state of the 
**		installation ; it writes ( harmless ) messages to the error log
**		when it fails ( which means the installation is down ) which
**		customers get worried about.  Use csreport instead. #69565
**	09-jul-96 (hanch04)
**		Don't prompt for return after setup program fails in express.
**	17-oct-96 (mcgem01)
**		E_ST015B now takes SystemProductName as a parameter.
**		Also cleaned up indentation of change histories.
**      22-nov-96 (reijo01)
**              Changed to use system generic variables.
**              The calls to files iibinitst and iiutltst have been          
**              updated to use SystemCfgPrefix instead of hard coded
**              ii.
**      23-dec-96 (reijo01)
**              Updated call to ip_cmdline to use generic filename, 
**              SystemCfgPrefix"build.err" instead of ingbuild.err.
**    19-feb-1997 (boama01)
**        Bug 80559: chgd ip_licensed() behavior for new AGGREGATE attribute
**        of pkgs.  An AGGREGATE pkg is a "bundle" of individually-licensable
**        parts whose capabilities must be OR-ed, rather than the default
**        behavior of AND-ed capabilties.  This allows pkg such as CUSTOMINST
**        to be "licensed" when any of its parts is.
**        Also reference new mbr in ip_add_inst_pkg(), ip_write_manifest().
**      17-jun-97 (funka01)
**              Updated to not call jasutltst for JASMINE since the
**              utility and bin directories on JASMINE have been 
**              consolidated to just one bin directory
**      10-Sep-1998 (wanya01)
**              Bug 77504: After complete custom installation. Removing 
**              components from current frame always gives error message
**              "full installation" must be removed first. 
**              Add condition to skip AGGREGATE package when checking 
**              package dependency in ip_delete_inst_pkg().
**	11-mar-1999 (hanch04)
**	    Added ignoreChecksum.
**	03-may-1999 (hanch04)
**	    SIputrec returns EOF on error.
**	28-apr-2000 (somsa01)
**	    Changed II_SYSTEM to SystemLocationVariable.
**	20-sep-2001 (gupsh01)
**	    Added functions ip_close_response, ip_chk_response_pkg 
**	    ip_close_response, ip_write_response to support the
**	    mkresponse mode.
**	19-oct-2001 (gupsh01)
**	    Added exresponse mode for response file based execution.
**	31-oct-2001 (gupsh01)
**	    Incorrect Return true removed from path_ok. 
**	02-nov-2001 (gupsh01)
**	    Added respfilename so that the users can use their own name
**	    for the response file.
**	24-jan-2002 (abbjo03)
**	    In ip_init_installdir, add checking of II_SYSTEM for disallowed
**	    characters.
**	11-apr-2002 (kinte01)
**	    In ip_init_installdir, add an ifdef for VMS to check for 
**	    characters allowed on VMS for II_SYSTEM that are not allowed
**	    on Unix. Added :,.,[,],<,>,_,- and $ in the VMS check as these 
**	    characters can all be found in an II_SYSTEM definition on VMS
**	03-jun-2002 (abbjo03)
**	    In ip_init_installdir, also allow period in II_SYSTEM path.
**	11-jun-2002 (toumi01)
**	    Correct parameter count for message E_ST015B_verification_error
**	    from 6 to 7, else we die on an internal error in ERlookup when
**	    we verify an install that has checksum or file size differences.
**	13-dec-2002 (devjo01) b109096, 12325659;1
**	    Added '-' to the characters allowed for UNIX in ip_init_installdir.
**	21-May-2003 (bonro01)
**	    Added LOdelete() of iibuild.err file after ip_cmdline() completes
**	    to clean up tmp directory.  This file would cause ingbuild to
**	    fail if it's permissions or ownership were changed between
**	    runs of ingbuild.
**	11-jun-2003 (hayke02)
**	    Call iisuice with the new -rmpkg flag when removing ICE in order
**	    to remove config.dat entries. This change fixes bug 110379.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	14-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	17-Jun-2004 (schka24)
**	    Safer environment variable handling.
**	23-Nov-2004 (bonro01)
**	    Fix bug introduced by removing reference to CI.
**	    ingbuild was showing components that should have been invisible
**	    sub-components, and it was also showing Packages on the
**	    component selection screen.
**	19-Sep-2005 (hanje04)
**	    Buildrel (or ingbuild) will fail mysteriously if iibuild.err cannot
**	    be removed. Check that iibuild.err does actually get deleted. Warn 
**	    appropriately if it doesn't.
**	19-Sep-2005 (hanje04)
**	    Ooops. If LOdelete will fail if iibuild.err doesn't exist. Only
**	    delete if it exists.
**	05-Jan-2006 (bonro01)
**	    Remove i18n, and documentation from default ingres.tar
**	17-Apr-2006 (hweho01)
**          When install manifest file (install.dat) is created, not to 
**          skip any package record.
**	13-Jun-2006 (bonro01)
**	    Make sure spatial, i18n, and documentation packages are 
**	    skipped for INCLUDE and NEED
**      13-Nov-2006 (hanal04) Bug 117100
**          ip_write_response() outputs II_DISTRIBUTION=value. Ensure the
**          buffer in ip_write_response() is as big as the one used for
**          distribDev in ipmain.sc
**	12-Feb-2007 (bonro01)
**	    Remove JDBC package.
**       9-May-2007 (hanal04) Bug 117075
**          Modify the open and close routines to handle "r" open and
**          close-without-rename so we can read a response file
**          in exresponse mode.
**	23-Oct-2007 (bonro01)
**	    Execute iisudas -rmpkg when removing the DAS server.
**      28-Feb-2008 (upake01)
**          In "ip_cmdline", add logic to not report an error if the
**          LOdelete returns "LO_NO_SUCH" as the file that existed
**          in the beginning may not exist when we try to delete it
**          (as it could have been deleted by other process).
**	26-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added header files to eliminate gcc 4.3 warnings.
*/

static void	add_inst_prt();
static void	add_inst_fil();

GLOBALREF char installDir[];
GLOBALREF char releaseManifest[];
GLOBALREF char distribDev[];
GLOBALREF char distribHost[];
GLOBALREF LIST instPkgList;
GLOBALREF LIST distPkgList;
GLOBALREF i4  ver_err_cnt;
GLOBALREF bool batchMode;
GLOBALREF bool ignoreChecksum;
GLOBALREF bool eXpress;
GLOBALREF bool mkresponse;
GLOBALREF bool exresponse;
GLOBALREF char *respfilename;
GLOBALREF FILE *batchOutf;
GLOBALREF FILE *rSTREAM;
GLOBALREF char Version[];
GLOBALREF bool pmode;

/* Global Definititions */
GLOBALDEF bool hide = TRUE;
GLOBALDEF bool spatial = FALSE;
/*
** The static LOCATION, and the routines that maintain it.  There are a
** whole lot of directories that we're going to want to refer to for a
** number of different reasons, and the following set of functions know
** how to set up the static location structure in all of the important
** ways.  In general, what's needed is a loc_setup_<something> call to
** set the structure, and a loc_get() call to return the resulting file
** or directory name.
*/

static LOCATION loc;
static char lobuf[ MAX_LOC + 1 ];

/* loc_setup -- set the "loc" structure to the installation directory */

static void
loc_setup()
{
	STcopy( installDir, lobuf );
	IPCL_LOfroms( PATH, lobuf, &loc );
}

/*
** loc_setup_private -- set the "loc" structure to the private subdirectory
**	of the installation area used by the installation process.
*/

static void
loc_setup_private()
{
	loc_setup();
	LOfaddpath( &loc, SystemLocationSubdirectory, &loc );
	LOfaddpath( &loc, ERx( "install" ), &loc );
}

/*
** loc_setup_private_file -- set the "loc" structure to a file that lives
**	in the private installation subdirectory.
*/

static void
loc_setup_private_file( char *file )
{
	loc_setup_private();
	LOfstfile( file, &loc );
}

/*
** loc_get -- return the file or directory currently pointed at by the
**	"loc" structure.
*/

static char *
loc_get()
{
	char *rtn;

	LOtos( &loc, &rtn );
	return( rtn );
}

/* ip_install_loc -- externally visible interface to loc_setup_private */

char *
ip_install_loc( void )
{
	loc_setup_private();
	return( loc_get() );
}

/* ip_install_file -- externally visible interface to loc_setup_private_file */

char *
ip_install_file( char *file )
{
	loc_setup_private_file( file );
	return( loc_get() );
}

/*
** loc_exists -- does the file or directory currently pointed at by the
**	"loc" structure actually exist?
*/

static bool
loc_exists()
{
	LOINFORMATION loinfo;
	i4 flagword = LO_I_TYPE;

	return( OK == LOinfo( &loc, &flagword, &loinfo ) );
}

/*
** ip_oemstring -- deal with OEM authorization files.
**
** A file with a special magic name may be included in the distribution.
** This file has the following property: if it exists, an authorization
** string will be read from this file, and will be used automatically by
** all components of INGRES.  This allows OEMs ( who are now known as VARs;
** same deal ) to pre-authorize various pieces of INGRES without the user
** having to know anything about authorization strings.
**
** This function just checks for the existence of the file, and, if it
** exists, attempts to read the authorization string into the global
** string "oemauth".
*/

STATUS
ip_oemstring( char *oemauth )
{
	FILE *oemf;

	loc_setup_private_file( OEM_AUTH_FILE );

	if( loc_exists() && OK == SIfopen( &loc, ERx( "r" ), SI_TXT,
		MAX_MANIFEST_LINE, &oemf ) &&
		OK == SIgetrec( oemauth, MAX_MANIFEST_LINE, oemf ) )
	{
		return( OK );
	}
	return( FAIL );
}

# ifdef UNIX

/*
** ip_preserve_cust_files - preserve customized files to prevent overwriting
*/

STATUS
ip_preserve_cust_files( void )
{
	PKGBLK *pkg;
	PRTBLK *prt;
	FILBLK *fil;
	STATUS stat;

	SCAN_LIST( distPkgList, pkg )
	{
		/* if package is empty or not selected, continue */
		if( !pkg->nfiles || pkg->selected == NOT_SELECTED )
			continue;

		SCAN_LIST( pkg->prtList, prt )
		{
			SCAN_LIST( prt->filList, fil )
			{
				LOCATION newLoc;
				LOINFORMATION loinfo;
				char newLocBuf[ MAX_LOC + 1 ];
				char *newPath;
				bool init = TRUE;
				i4 flags;

				/* rename only if file is customizable */
				if( !fil->custom )
					continue;

				/* continue if customizable file not found */
				loc_setup();
				IPCL_LOaddPath( &loc, fil->directory );
				LOfstfile( fil->name, &loc );
				if( !loc_exists() )
					continue;

				/* construct custom LOCATION */
				STcopy( installDir, newLocBuf );
				(void) IPCL_LOfroms( PATH, newLocBuf,
					&newLoc );
				LOfaddpath( &newLoc, 
					  SystemLocationSubdirectory,
					  &newLoc );
				LOfaddpath( &newLoc, ERx( "custom" ),
					&newLoc );

				if( init )
				{
					ip_display_transient_msg(
						ERget(
						S_ST0182_preserving_files ) );

					/* create "custom" directory */
					init = FALSE;
					LOtos( &newLoc, &newPath );
					if( IPCLcreateDir( newPath ) != OK )
						return( FAIL );
				}

				LOfstfile( fil->name, &newLoc );
				/* does file already exist? */
				flags = LO_I_TYPE;
				LOinfo( &newLoc, &flags, &loinfo );
				if (!(flags & LO_I_TYPE)) /* not there, good */
				{
					LOtos( &newLoc, &newPath );
				
					if( IPCL_LOmove( loc_get(), newPath ) 
					    != OK )
						return( FAIL );
				}
			}
		}
	}
	return( OK );
}

# endif /* UNIX */

/*
** ip_cmdline -- execute a shell command via PCcmdline.
**
** This is a wrapper around PCcmdline.  It executes the specified command,
** and, if it fails, creates an error message from an error message
** identified as a parameter and the output from the command; if the
** error message ID is NULL, then an error just pops up the output from
** the command.  It returns OK if the command succeeded, and the return
** status from PCcmdline otherwise.
*/

STATUS
ip_cmdline( char *cmdline, ER_MSGID errmsg )
{
	CL_ERR_DESC cl_err;
	char line[ MAX_MANIFEST_LINE + 1 ];
	STATUS stat, rtn;
	FILE *rfile;
	LOCATION loc;
	char locBuf[ MAX_LOC + 1 ];
	char err[ 1024 ];
	char msg[ 1024 ];

	/* set up LOCATION for error log file */
	*locBuf = EOS;
	(void) IPCL_LOfroms( PATH & FILENAME, locBuf, &loc );
	STprintf(msg,ERx("%sbuild"),SystemCfgPrefix);
	if( OK != ( stat = LOcompose( TEMP_DEV, TEMP_DIR, msg,
		ERx( "err" ), NULL, &loc ) ) )
	{
		return( stat );
	}

	/* remove existing errorfile (SystemCfgPrefix"build.err" if it exists */
	if ( !LOexist( &loc ) && ( stat = LOdelete( &loc ) ) )
	{
            /* It's ok if file doesn't exist, but return other errors */
            if ( stat != LO_NO_SUCH )
            {
	       char    *fname;
	       LOtos( &loc, &fname );
	       SIprintf("Error: cannot remove %s\n", fname);
	       return(stat);
	    }
	}

	if( (rtn = PCcmdline( NULL, cmdline, PC_WAIT, &loc, &cl_err ))
		!= OK )
	{
		if( errmsg != ( ER_MSGID ) 0 )
			STprintf( err, ERget( errmsg ), rtn );
		else
			*err = EOS;

		if( OK == (stat = SIfopen( &loc, ERx( "r" ), SI_TXT,
			MAX_MANIFEST_LINE, &rfile )) )
		{
			while( SIgetrec( line, MAX_MANIFEST_LINE, rfile )
				== OK )
			{
				if( STlength( line ) + STlength( err ) >
					sizeof( err ) - 1 )
				{
					break;
				}

				STtrmwhite( line );
				STcat( err, ERx( "\n" ) );
				STcat( err, line );
			}
			(void) SIclose( rfile );
		}

		if( !batchMode )
		{
			exec frs clear screen;
			exec frs redisplay;
		}
		ip_display_msg( err );
	}
	/* remove existing errorfile (SystemCfgPrefix"build.err" if it exists */
	(void) LOdelete( &loc );

	return( rtn );
}

# ifdef UNIX

/*
** ip_sysstat -- Verify that the system is not running.
**
** For now we do it like this: if $II_SYSTEM/ingres/utility/csreport doesn't
** exist or isn't readable, there's no system there at all.  If it exists,
** we run it; if it succeeds, the system's up, otherwise it's down.  This
** is pretty primitive.
*/

i4
ip_sysstat( void )
{
	CL_ERR_DESC cl_err;
	FILE *rfile;
	STATUS stat;
	char buf[MAX_LOC];
        STATUS rtrn;

	loc_setup();
	LOfaddpath( &loc, SystemLocationSubdirectory, &loc );
	LOfaddpath( &loc, ERx( "utility" ), &loc );
	LOfstfile( ERx( "csreport" ), &loc );
	
	if( !loc_exists() )
		return( IP_NOSYS );
	
	stat = SIfopen( &loc, ERx( "r" ), SI_RACC, MAX_MANIFEST_LINE, &rfile );
	if( stat != OK )
		return( IP_NOSYS );
	SIclose( rfile );

	STpolycat( 2, loc_get(), ERx( " > /dev/null " ), buf );
        IPCLclearTmpEnv () ;
	rtrn = PCcmdline( NULL, buf, PC_WAIT, NULL, &cl_err ) == OK ? 
		IP_SYSUP: IP_SYSDOWN ;
        IPCLresetTmpEnv () ;
        return (rtrn );
}

/*
** IPCLdeleteDir() -- delete a named directory by executing a shell command.
*/

void
IPCLdeleteDir( char *dir )
{
	char cmd[ 1024 ];

	STpolycat( 2, ERx( " rm -rf " ), dir, cmd );
	(void) ip_cmdline( cmd, E_ST0112_cant_delete_dir );
}

/*
** ip_delete_dist_pkg() 
**
** Delete a package from the distribution temporary staging area.  Simple;
** all we have to do is recursively delete the directory containing the
** package.
*/

void
ip_delete_dist_pkg( PKGBLK *pkg )
{
	loc_setup_private();
	LOfaddpath( &loc, pkg->feature, &loc );
	IPCLdeleteDir( loc_get() );
}

# endif /* UNIX */


/*
** ip_file_info_comp -- compute checksum and size for a location.
**
** This is a pretty lame checksum algorithm, but it's easy to code.
*/

STATUS
ip_file_info_comp( LOCATION *floc, i4  *bytecnt, i4  *checksum, bool audit )
{
	STATUS stat;
	FILE *rfile;
	i4 chr, cks, cnt;

	if( OK != (stat = SIfopen( floc, ERx( "r" ), SI_TXT, SI_MAX_TXT_REC,
		&rfile )) )
	if( (stat = SIfopen( floc, ERx( "r" ), SI_TXT, SI_MAX_TXT_REC,
		&rfile ) != OK) && (stat = SIfopen( floc, ERx( "r" ),
		SI_VAR, SI_MAX_TXT_REC, &rfile )) != OK && (stat =
		SIfopen( floc, ERx( "r" ), SI_RACC, SI_MAX_TXT_REC,
		&rfile )) != OK )
	{
		/* mark file not found by setting bytecnt to -1 */
		*bytecnt = -1;
		return( stat );
	}

	if( audit )
	{
		SIclose( rfile );
		return( OK );
	}

	for( cnt = cks = 0 ; EOF != ( chr = SIgetc( rfile ) ) ; )
	{
		cnt++;
		cks ^= chr << ( cnt & 3 );
	}

	SIclose( rfile );

	*bytecnt = cnt;
	*checksum = cks;
	return( OK );
}

/*
** ip_file_info -- compute checksum and size for named file.
**
** Just a wrapper that sets up a LOCATION struct and calls ip_file_info_comp
** to do the real work.
*/

STATUS
ip_file_info( LOCATION *floc, char *file, i4  *bytecnt, i4  *checksum )
{
	LOCATION infoloc;

	LOfstfile( file, floc );

	return( ip_file_info_comp( floc, bytecnt, checksum, FALSE ) );
}

/*
** ip_find_dir -- find the directory where a specified file goes.
**
** Find a specified file within a package, and return a string
** corresponding to the directory where the file will be installed.
*/

char *
ip_find_dir( PKGBLK *pkg, char *filename )
{
	PRTBLK *prt;
	FILBLK *fil;

	SCAN_LIST( pkg->prtList, prt )
	{
		SCAN_LIST( prt->filList, fil )
		{
			if( STequal( filename,fil->name ) && 
				(fil->directory && *fil->directory))
			{
				return( fil->directory );
			}
		}
	}

	return( ERx( "" ) );  /* Never found the file??? */
}

static void
press_return()
{
	SIprintf( ERget( S_ST014A_press_return ) );
	(void) SIgetc( stdin );
	ip_forms( TRUE );
	if( batchMode )
		SIprintf( ERx( "\n" ) );
}

/*
** do_setup() -- actually execute a setup program.
**
** Note that this leaves the forms mode in an indeterminate state.  If the
** setup program succeeds, it's left in no-forms mode, because we might be
** about to do another setup program, and we don't want the screen to flash.
** If anything goes wrong, the screen is put back into forms mode so the
** error message can be displayed in the One True Error Message Format.
**
** If the "action" parameter has the value _DISPLAY, then we just display
** the name of the setup program on the batch output file.  This is a
** special feature for the benefit of iimaninf.
**
** History:
**
**	11-dec-2001 (gupsh01)
**	    Added check that if the response file is open then it should
**	    closed before any entries are added to the response file
**	    by the setup programs in mkresponse mode.
*/

static bool
do_setup( PKGBLK *pkg, PRTBLK *prt, char *refname, i4  action )
{
	CL_ERR_DESC cl_err;
	char *dir_path, *dir_path_copy, *p;
	char *path_names[MAX_LOC+1];
	i4 db_count=MAX_LOC, i;
        char msgBuf[ 512 ];
        char progname[LO_NM_LEN];

	STcopy(refname, progname);

	loc_setup();

	dir_path=ip_find_dir(pkg, progname);

	IPCL_LOaddPath( &loc, dir_path );

	LOfstfile( progname, &loc );

	if( action == _DISPLAY )  /* just display the setup program name */
	{
		SIfprintf( batchOutf, ERx( "%s\n" ), loc_get() );
		return( TRUE );
	}

	if( !loc_exists() ) /* Is the setup program really there? */
	{
		ip_forms( TRUE );
		IIUGerr( E_ST012D_no_setup_program, 0, 3, loc_get(),
			prt->name, pkg->name );
		return( FALSE );
	}
	else 
	{
		if (eXpress)
  		{
                	if( pkg->nfiles && ( ip_is_visible( pkg )
			   || ip_is_visiblepkg( pkg ) ) )
                		STprintf( msgBuf, ERget(
					  F_ST0207_setting_up_pkg ), 
					  pkg->name );
        		else
        		{
                	/* print a different message for a support package */
                		STprintf( msgBuf, ERget(
					  F_ST0208_setting_up_supp_pkg ),
                        		  pkg->name );
        		}

                  	ip_display_transient_msg( msgBuf );

		  /* add -batch flag to setup programs */
			STcat( progname, " -batch" );
			LOfstfile( progname, &loc );
		}
		else
		{
			if (mkresponse)
			{

			        /* If we are now doing setup then obviously
				** we are through with writing all parameters
				** to the response file from forms mode hence
				** close the response file now.
				*/	
				
				if (rSTREAM)
				{
			            ip_close_response (rSTREAM);	
				    rSTREAM = NULL;
				}

				/* add -mkresponse flag to setup programs */
                        	STcat( progname, " -mkresponse" );
                        	LOfstfile( progname, &loc );
			}
			else if (exresponse)
			{
				/* add -exresponse flag to setup programs */
                        	STcat( progname, " -exresponse" );
                        	LOfstfile( progname, &loc );
			}
			if (respfilename != NULL)
			{
				/* 
				** Added the response filename if the mode 
				** is -exresponse or -mkresponse 
				*/
				if (mkresponse || exresponse) 
				{
				  STpolycat( 3, progname, " ", respfilename, 
						progname );
			          LOfstfile( progname, &loc );	
				}
			}
		  /* clear the screen before running a setup program */
			if( !batchMode )
				exec frs clear screen; 
			ip_forms( FALSE );
		}
		
		IPCLclearTmpEnv();

		if( PCcmdline( NULL, loc_get(), PC_WAIT, NULL, &cl_err ) == OK )
			return( TRUE );

		IPCLresetTmpEnv();
		
		/* Setup program exited with failure status */

		if (!eXpress)
		{ 
		    press_return();
		    IIUGerr( E_ST012E_setup_program_error, 0, 3, loc_get(),
			prt->name, pkg->name );
		}

		return( FALSE );
	}
}

# ifdef UNIX

/*
** path_ok -- verify that path contains $II_SYSTEM/ingres/{utility,bin}
**
** NOTE: This is specific to Unix and should be moved to an IPCL abstaction
** when ingbuild is ported to non-Unix environments.
*/

static bool 
path_ok( void )
{
	static bool path_ok, init = TRUE;
	if( init ) 
	{
		CL_ERR_DESC cl_err;
		char cmd1[ 100 ], cmd2[ 100 ]; 
		STprintf( cmd1, ERx( "%sbintst \"%s\"" ),
			SystemCfgPrefix, Version );
		STprintf( cmd2, ERx( "%sutltst \"%s\"" ),
			SystemCfgPrefix, Version );
 		
		if( PCcmdline( NULL, cmd1, PC_WAIT, (LOCATION *) NULL,
			&cl_err ) == OK && PCcmdline( NULL, cmd2,
			PC_WAIT, (LOCATION *) NULL, &cl_err ) == OK )
		{
			path_ok = TRUE;
		}
		else
		{
			/* redraw screen if forms mode */
			if( !batchMode )
			{
				exec frs clear screen;
				exec frs redisplay;
			}
			path_ok = FALSE;
		}
		init = FALSE;
	}
	return( path_ok );
}

# endif /* UNIX */

/*
** ip_setup -- execute all setup programs for a package.
**
** This can be called in two different modes, depending on the value of the
** "action" parameter.  If action==_CHECK, we just want to determine if there
** are _any_ setup programs to be run for the specified package; we should
** return( TRUE ) if there are and FALSE if there aren't.  If action==_PERFORM,
** we want to actually run all the setup programs, in the order they were
** specified, which, for our purposes, means in the order that they appear
** in the reference lists for each part in the package.  In this case, we
** return( TRUE ) if the setup completed successfully, FALSE otherwise.
*/

bool
ip_setup( PKGBLK *pkg, i4  action )
{
	PKGBLK *ipkg;
	PRTBLK *prt, *iprt;
	REFBLK *ref;
	LISTELE *lp1, *lp2, *lp3;
        char mesg[ 500 ];
# ifdef DEBUG_SETUP 
exec sql begin declare section;
char msg[ 100 ];
exec sql end declare section;
# endif /* DEBUG_SETUP */

# ifdef DEBUG_SETUP 
STprintf( msg, "ip_setup( %s, %s ) called.", pkg->feature,
	action == _CHECK ? "CHECK" : "PERFORM" ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */

# ifdef UNIX
	/* make sure path is set correctly */
	if( action != _CHECK && !path_ok() )
	{
		STprintf( mesg, ERget( E_ST0141_bad_search_path ), SystemExecName); 
		ip_display_msg( mesg );
		return( FALSE );
	}
# endif /* UNIX */

	/*
	** Call ip_setup() recursively on INCLUDED packages.
	*/
	for( lp1 = pkg->refList.head; lp1 != NULL; lp1 = lp1->cdr )
	{
		ref = (REFBLK *) lp1->car;
		if( ref->type == INCLUDE )
		{
			if( (ipkg = ip_find_package( ref->name,
				INSTALLATION )) != NULL )
			{
				if( ip_setup( ipkg, action ) )
				{
					if( action == _CHECK )
# ifdef DEBUG_SETUP 
{
STprintf( msg, "Returning TRUE from ip_setup( %s, %s )", pkg->feature,
	action == _CHECK ? "CHECK" : "PERFORM" ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */
						return( TRUE );
# ifdef DEBUG_SETUP 
}
# endif /* DEBUG_SETUP */
				}
				else
				{
					if( action == _PERFORM )
# ifdef DEBUG_SETUP 
{
STprintf( msg, "Returning FALSE from ip_setup( %s, %s )", pkg->feature,
	action == _CHECK ? "CHECK" : "PERFORM" ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */
						return( FALSE );
# ifdef DEBUG_SETUP 
}
# endif /* DEBUG_SETUP */
				}
			}
		}
	}

	if( action == _CHECK && pkg->state == INSTALLED )
# ifdef DEBUG_SETUP 
{
STprintf( msg, "Returning FALSE from ip_setup( %s, %s )", pkg->feature,
	action == _CHECK ? "CHECK" : "PERFORM" ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */
		return( FALSE );
# ifdef DEBUG_SETUP 
}
# endif /* DEBUG_SETUP */

	for( lp1 = pkg->prtList.head; lp1 != NULL; lp1 = lp1->cdr )
	{
		prt = (PRTBLK *) lp1->car;

		for( lp2 = prt->refList.head; lp2 != NULL; lp2 = lp2->cdr )
		{
			ref = (REFBLK *) lp2->car;

			if( ref->type == SETUP )
			{
				if( ref->version != NULL )
				{

					/* 
					** We've encountered a conditional
					** setup program; i.e. one which is
					** to be run only if the version of
					** the package currently installed
					** on the system bears some specified
					** relationship to the version we're
					** installing.  We need to find the
					** version on the current installation
					** and do the comparison before we
					** know whether to run this.  
					*/
					  
					if( (ipkg = ip_find_package( pkg->name,
						INSTALLATION )) == NULL )
					{
						continue;
					}

					for( lp3 = ipkg->prtList.head;
						lp3 != NULL; lp3 = lp3->cdr )
					{
						iprt = (PRTBLK *) lp3->car;

						if( !STbcompare( ref->name,
							99, iprt->name, 99,
							TRUE ) )
						{
							break;
						}
					}

					if( iprt == NULL ||
						!ip_comp_version (
						iprt->version, ref->comparison,
						ref->version ) )
					{
						continue;
					}
				}

				/* 
				** At this point we know we have a setup
				** program to run.  If that's all we wanted
				** to know, return. 
				*/
				if( action == _CHECK )  
# ifdef DEBUG_SETUP 
{
STprintf( msg, "Returning TRUE from ip_setup( %s, %s )", pkg->feature,
	action == _CHECK ? "CHECK" : "PERFORM" ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */
					return( TRUE );
# ifdef DEBUG_SETUP 
}
# endif /* DEBUG_SETUP */

				if( pkg->state != INSTALLED && !do_setup(
					pkg, prt, ref->name, action ) )
				{
					/*
					** Setup failed; kill further setup
					** for this part.
					*/
# ifdef DEBUG_SETUP 
STprintf( msg, "Returning FALSE from ip_setup( %s, PERFORM )", pkg->feature );
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */
					return( FALSE );
				}
			}
		}
	}
	if ( mkresponse  == FALSE ) 
	pkg->state = INSTALLED;

	if( action == _CHECK )
		/* no setup program found, so exit */ 
# ifdef DEBUG_SETUP 
{
STprintf( msg, "Returning FALSE from ip_setup( %s, %s )", pkg->feature,
	action == _CHECK ? "CHECK" : "PERFORM" ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */
		return( FALSE );
# ifdef DEBUG_SETUP 
}
# endif /* DEBUG_SETUP */

# ifdef DEBUG_SETUP 
STprintf( msg, "Returning TRUE from ip_setup( %s, %s )",
	pkg->feature, action == _CHECK ? "CHECK" : "PERFORM" ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_SETUP */
	return( TRUE );
}


/*
** ip_preload -- execute all pre-transfer programs for a package.
**
** If action==_CHECK, we're just checking to see if there are any; the return
** code will be TRUE if there are and FALSE if not.  If action==_PERFORM, we
** actually run the programs, and return( TRUE ) if they all returned success
** codes, FALSE otherwise.  If action==_DISPLAY, we just want to display
** the names of all the pre-transfer programs on the batch output file.
**	
** When validating that program exists, only validate the first word 
** of the preload program string. This is to allow PRELOAD programs
** which take arguments.
*/

bool
ip_preload( PKGBLK *pkg, i4  action )
{
	PRTBLK *prt;
	CL_ERR_DESC cl_err;
	bool rtn = TRUE;
	char *t;
	char rwd[DB_MAXNAME];
	i4	l=0;

	SCAN_LIST( pkg->prtList, prt )
	{
		if( prt->preload != NULL )
		{
			if( action == _CHECK )
				return( TRUE );
			if (pmode)
			{



			}
			if( action == _DISPLAY )
			{
				loc_setup_private_file( prt->preload );
				SIfprintf( batchOutf, ERx( "%s\n" ),
					loc_get() );
				return( TRUE );
			}

			t = STindex(prt->preload, (PTR)" ", 0);
			if (t == NULL)
				loc_setup_private_file( prt->preload );
			else
			{
				l = t - prt->preload;
				STlcopy(prt->preload, rwd, l);
				loc_setup_private_file( rwd );
			}
			
			/* Check whether preload program actually exist? */
			if( !loc_exists() )
			{
				ip_forms( TRUE );
				IIUGerr( E_ST0134_no_preload_prog, 0, 2,
					loc_get(), prt->name );
				rtn = FALSE;
			}
			else 
			{
				if (l != 0)
					loc_setup_private_file( prt->preload );
				/* Assume it's a non-forms prog. */
				ip_forms( FALSE );
				if( OK != PCcmdline( NULL, loc_get(),
					PC_WAIT, NULL, &cl_err ) )
				{
					press_return();
					IIUGerr( E_ST0135_preload_prog_err,
						0, 2, loc_get(), prt->name );
					rtn = FALSE;
				}
			}
		}
	}

	/* 
	** If we get this far, haven't found one, and are here to see if there
	** are any, well, there aren't.  
	*/

	return( action == _CHECK ? FALSE : rtn );
}

/*
** remove_inst_pkg() --  Remove the package block from the list it's in,
**	remove all subsidiary lists, and remove the block itself.
*/

static void
remove_inst_pkg( PKGBLK *pkg )
{
	PRTBLK *prt;

	SCAN_LIST( pkg->prtList, prt )
	{
		ip_list_destroy( &prt->filList );
		ip_list_destroy( &prt->capList );
		ip_list_destroy( &prt->refList );
	}

	ip_list_destroy( &pkg->prtList );
	ip_list_destroy( &pkg->refList );
	ip_list_remove( &instPkgList );
}

/*
** ip_add_inst_pkg()
**
** Add the data for a package to the internal list that represents the
** current installation.  This basically involves making a copy of
** the package block, which recursively involves making copies of all
** lists which are headed in the package block or any block underneath it.
*/

void
ip_add_inst_pkg( dpkg )
PKGBLK *dpkg;
{
	FILBLK *dfil, *ifil;
	PRTBLK *dprt, *iprt;
	REFBLK *dref, *iref;

	PKGBLK *ipkg;

	/* If there's an old one, kill it first. */
	if( NULL != ( ipkg = ip_find_feature( dpkg->feature, INSTALLATION ) ) )
		remove_inst_pkg( ipkg );

	/*  Create a new package block.  */
	ipkg = ( PKGBLK * ) ip_alloc_blk( sizeof( PKGBLK ) );
	ipkg->file = ip_stash_string( dpkg->file );
	ipkg->name = ip_stash_string( dpkg->name );
	ipkg->version = ip_stash_string( dpkg->version );
	ipkg->feature = ip_stash_string( dpkg->feature );
	ipkg->description = ip_stash_string( dpkg->description );
	ipkg->actual_size = dpkg->actual_size;
	ipkg->apparent_size = dpkg->apparent_size;
	ipkg->nfiles = dpkg->nfiles;
	ipkg->state = dpkg->state;
	ipkg->visible = dpkg->visible;
	ipkg->image_size = dpkg->image_size;
	ipkg->image_cksum = dpkg->image_cksum;
        ipkg->aggregate = dpkg->aggregate;
	ip_list_append( &instPkgList, ( PTR )ipkg );

	/*  
	** Copy the list of package references ( i.e. package-to-package
	** dependencies ) into the new package block.  
	*/

	ip_list_init( &ipkg->refList );
	SCAN_LIST( dpkg->refList, dref )
	{
		iref = ( REFBLK * ) ip_alloc_blk( sizeof( REFBLK ) );
		iref->name = ip_stash_string( dref->name );
		iref->version = ip_stash_string( dref->version );
		iref->comparison = dref->comparison;
		iref->type = dref->type;
		ip_list_append( &ipkg->refList, ( PTR )iref );
	}

	/*  Copy the list of parts into the new package block.  */
	ip_list_init( &ipkg->prtList );
	SCAN_LIST( dpkg->prtList, dprt )
	{
		i4 *ibitp, *dbitp;

		/*  Create a new part block.  */

		iprt = ( PRTBLK * ) ip_alloc_blk( sizeof( PRTBLK ) );
		iprt->file = ip_stash_string( dprt->file );
		iprt->name = ip_stash_string( dprt->name );
		iprt->version = ip_stash_string( dprt->version );
		iprt->preload = NULL; 
		iprt->delete = ip_stash_string( dprt->delete );
		iprt->size = dprt->size;
		ip_list_append( &ipkg->prtList, ( PTR )iprt );

		/*  Duplicate the file list in the new part block.  */

		ip_list_init( &iprt->filList );
		SCAN_LIST( dprt->filList, dfil )
		{
			/*  Create a new file block.  */

			ifil = ( FILBLK * ) ip_alloc_blk( sizeof( FILBLK ) );
			ifil->name = dfil->name;
			ifil->directory = dfil->directory;
			ifil->build_dir = dfil->build_dir;
			ifil->generic_dir = dfil->generic_dir;
			ifil->volume = dfil->volume;
			ifil->size = dfil->size;
			ifil->checksum = dfil->checksum;
			ifil->custom = dfil->custom;
			ifil->setup = dfil->setup;
			ifil->dynamic = dfil->dynamic;
			ifil->exists = dfil->exists;
			ifil->link = dfil->link;
			ip_list_append( &iprt->filList, ( PTR )ifil );
		}

		/*  Duplicate the capability list in the new part block.  */

		ip_list_init( &iprt->capList );
		SCAN_LIST( dprt->capList, dbitp )
		{
			*( ibitp = ( i4  * ) ip_alloc_blk( sizeof( i4  ) ) )
				= *dbitp;
			ip_list_append( &iprt->capList, ( PTR )ibitp );
		}

		/*
		** Duplicate the part reference ( i.e. setup ) list in the
		** new block.
		*/
		ip_list_init( &iprt->refList );
		SCAN_LIST( dprt->refList, dref )
		{
			iref = ( REFBLK * ) ip_alloc_blk( sizeof( REFBLK ) );
			iref->name = ip_stash_string( dref->name );
			iref->version = ip_stash_string( dref->version );
			iref->comparison = dref->comparison;
			iref->type = dref->type;
			ip_list_append( &iprt->refList, ( PTR )iref );
		}

	}
}

/*
** delete_utility -- execute the delete program( s ) for a package.
**
** Delete programs may be defined on a part-by-part basis, just like
** setup programs; a delete program is to be run before a part is deleted
** from an installation.  The difference between delete and setup programs,
** at the moment, is that you can only specify one delete program per
** part.  The delete program is specified in the "delete" field of the
** part block; setup programs are defined in the part reference list.
**
** If the STAR package is being deleted, run "iisustar -rmpkg" to remove
** all references to STAR in the config.dat file.
**
** If INGRES/Net is being deleted, run "iisunet -rmpkg" to remove all 
** references to Ingres/Net in the config.dat file.
**	
** When validating that program exists, only validate the first word 
** of the delete program string. This is to allow DELETE programs
** which take arguments to be specified in manifests
*/

static bool
delete_utility( pkg )
PKGBLK *pkg;
{
	PRTBLK *prt;
	CL_ERR_DESC cl_err;
        char *dir_path;
	char *temp;
        char progname [20];
	char *t;
	char rwd[DB_MAXNAME];
	i4  result;
	i4 l;
	bool rtn = TRUE;

	SCAN_LIST( pkg->prtList, prt )
	{
		if( prt->delete != NULL )
		{
			loc_setup_private_file( prt->delete );
			t = STindex(prt->delete, (PTR)" ", 0);
			if (t == NULL)
				loc_setup_private_file( prt->delete );
			else
			{
				l = t - prt->delete;
				STlcopy(prt->delete, rwd, l);
				loc_setup_private_file( rwd );
			}

			if( !loc_exists() )
			{
				ip_forms( TRUE );
				IIUGerr( E_ST014F_no_delete_prog, 0, 2,
					loc_get(), prt->name );
				rtn = FALSE;
			}
			else 
			{
				loc_setup_private_file( prt->delete );
				ip_forms( FALSE );
				if( OK != PCcmdline( NULL, loc_get(),
					PC_WAIT, NULL, &cl_err ) )
				{
					press_return();
					IIUGerr( E_ST0150_delete_prog_err,
						0, 2, loc_get(), prt->name );
					rtn = FALSE;
				}
			}
		}
	}

        if ( ( (!(STcompare(pkg->feature,ERx("star")))) ||
	(!(STcompare(pkg->feature,ERx("net")))) || 
	(!(STcompare(pkg->feature,ERx("bridge")))) ||
	(!(STcompare(pkg->feature,ERx("das")))) ||
	(!(STcompare(pkg->feature,ERx("ice")))) ) && (rtn != FALSE))
	{
		if (!(STcompare(pkg->feature,ERx("star"))))
			STcopy (ERx("iisustar"),progname);
		else if (!(STcompare(pkg->feature,ERx("net"))))
		        STcopy (ERx("iisunet"),progname);
		else if (!(STcompare(pkg->feature,ERx("bridge"))))
		        STcopy (ERx("iisubr"),progname);
		else if (!(STcompare(pkg->feature,ERx("das"))))
		        STcopy (ERx("iisudas"),progname);
		else
			STcopy (ERx("iisuice"),progname);
                loc_setup();
                dir_path=ip_find_dir(pkg,progname);
                IPCL_LOaddPath(&loc,dir_path);
                LOfstfile(progname,&loc);
                if (loc_exists())
                {
			temp=loc_get();
			STcat (temp,ERx(" -rmpkg"));
			ip_forms(FALSE);
                        result=PCcmdline (NULL, temp, PC_WAIT, NULL,
			       &cl_err);
			press_return();
			if (result!=OK)
			{
                              IIUGerr( E_ST0150_delete_prog_err,
                                       0, 2, loc_get(), pkg->name );
			      rtn = FALSE;    
			}
                }
		else
		{
			IIUGerr( E_ST0150_delete_prog_err,
				 0, 2, loc_get(), pkg->name );
			rtn = FALSE;
		}
        }

	ip_forms( TRUE );
	return( rtn );
}

/*
** Data structure used to produce a list of unique INVISIBLE packages
** INCLUDED directly or indirectly by VISIBLE packages in an installation. 
** This list is used by garbage collection algorithm which cleans up
** orphan packages.
*/

typedef struct flist {
	char *feature;
	struct flist *next;
} FLIST; 

static void
get_inc_inv_pkgs( FLIST **fl, PKGBLK *pkg )
{
	PKGBLK *ipkg;
	REFBLK *ref;
	FLIST *fp;

	SCAN_LIST( pkg->refList, ref )
	{
		if( ref->type == INCLUDE && (ipkg =
			ip_find_package( ref->name, INSTALLATION )) != NULL )
		{
			get_inc_inv_pkgs( fl, ipkg );
		}
	}

	/*
	** Don't add VISIBLE packages to the list.
	*/
	if( ip_is_visible( pkg ) || ip_is_visiblepkg( pkg ) )
		return;

	/*
	** Return if package is already in the list.
	*/
	for( fp = *fl; fp != (FLIST *) NULL; fp = fp->next )
	{
		if( STequal( pkg->feature, fp->feature ) )
			return;
	}

	/*
	** Allocate new record and make it the head of the list.
	*/
	fp = (FLIST *) MEreqmem( 0, sizeof( FLIST ), FALSE, NULL );
	fp->feature = pkg->feature;
	fp->next = *fl;
	*fl = fp;
}

/*
** is_needed_by()
**
** Recursively determine if a pkg1 or anything it includes is NEEDED by
** pkg2.
*/
static PKGBLK *
is_needed_by( PKGBLK *pkg1, PKGBLK *pkg2 )
{
	LISTELE *lp;
	PKGBLK *ipkg;
	REFBLK *ref;

	if( pkg2 == NULL )
	{
		/*
		** Call is_needed_by() on all packages.
		*/
		for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
		{
			pkg2 = (PKGBLK *) lp->car;	
			if( pkg1 == pkg2 )
				continue;
			if( (ipkg = is_needed_by( pkg1, pkg2 )) != NULL )
				return( ipkg );
		}
		return( NULL );
	}

	/*
	** Only got here if( pkg2 != NULL )
	**
	** First check whether pkg2 NEEDS pkg1.
	*/
	for( lp = pkg2->refList.head; lp != NULL; lp = lp->cdr )
	{
		ref = (REFBLK *) lp->car;	

		if( ref->type == NEED && STequal( ref->name, pkg1->feature ) )
			return( pkg2 );
	}

	/*
	** Now check whether pkg2 NEEDS anything included by pkg1.
	*/
	for( lp = pkg1->refList.head; lp != NULL; lp = lp->cdr )
	{
		ref = (REFBLK *) lp->car;	

		if( ref->type == INCLUDE && (ipkg = ip_find_package(
			ref->name, INSTALLATION )) != NULL &&
			( ip_is_visible( ipkg ) || ip_is_visiblepkg (ipkg) ) &&
			(ipkg = is_needed_by( ipkg, pkg2 ))
			!= NULL )
		{
			return( pkg2 );
		}
	}

	/*
	** If here, pkg1 is not needed by pkg2.
	*/
	return( NULL );
} 

/*
** includes()
**
** Recursively determine if a pkg1 includes pkg2. 
*/
static bool 
includes( PKGBLK *pkg1, PKGBLK *pkg2 )
{
	LISTELE *lp;
	PKGBLK *ipkg;
	REFBLK *ref;

	/*
	** Call includes() recursively for all INCLUDED packages.
	*/
	for( lp = pkg1->refList.head; lp != NULL; lp = lp->cdr )
	{
		ref = (REFBLK *) lp->car;	

		if( ref->type == INCLUDE && (ipkg = ip_find_package(
			ref->name, DISTRIBUTION )) != NULL &&
			includes( ipkg, pkg2 ) )
		{	
			return( TRUE );
		}
	}

	/*
	** If feature names match, pkg1 does include pkg2.
	*/
	if( STequal( pkg1->feature, pkg2->feature ) )
		return( TRUE );

	/*
	** If here, pkg1 does not INCLUDE pkg2. 
	*/
	return( FALSE );
} 

/*
** ip_delete_inst_pkg()
**
** Delete a package from the installation, by which we mean, at the moment,
** delete all the files that were installed with that package.
**
**	18-march-1996 (angusm)
**		Changes to allow patch deinstall. Patches are expected
**		to have a DELETE action of 'pvrestore <patchid>', but
**		we need to do this AFTER deleting the files, not before,
**		so that we can then read the new SIZE/CHECKSUM values
**		from disk and update the current installation desription
**		in memory.
**	5-july-1996 (abowler)
**              Bug 77473: Increase message buffer to 512 bytes, as per
**              others used for install, as it was overflowing.
*/

bool
ip_delete_inst_pkg( PKGBLK *pkg, char *parent )
{
exec sql begin declare section;
	char msg[ 512 ];
exec sql end declare section;
	PKGBLK *ipkg;
	PRTBLK *prt;
	FILBLK *fil;
	REFBLK *ref;
	PTR pkgp;
	bool delete_ok;
	LISTELE *lp;

	if( (ipkg = is_needed_by( pkg, NULL )) != NULL )
	{
		if( parent == NULL )
		{
			IIUGerr( E_ST0007_cannot_delete, 0, 2, pkg->name,
				ipkg->name );
		}
		return( FALSE );
	}

	/*
	** Call ip_delete_inst_pkg() recursively on all INCLUDED
	** packages.  If INCLUDE dependencies are recursive, this 
	** won't terminate.
	*/
	for( lp = pkg->refList.head; lp != NULL; lp = lp->cdr )
	{
		ref = (REFBLK *) lp->car;

		if( ref->type == INCLUDE && (ipkg =
			ip_find_package( ref->name, INSTALLATION )) != NULL )
		{
			(void) ip_delete_inst_pkg( ipkg, pkg->feature );
		}
	}

	/*
	** Don't delete INCLUDED packages which are VISIBLE.
	*/
	if( parent != NULL && ( ip_is_visible( pkg ) || 
				    ip_is_visiblepkg( pkg ) ) )
		return( FALSE );

	/*
	** Don't delete package if it is INCLUDED by any package other
	** than the parent to this call.
	*/
	for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
	{
		ipkg = (PKGBLK *) lp->car;

		/* ignore self-inclusion */
		if( ipkg == pkg )
			continue;

		/* skip parent to this call */
		if( parent != NULL && STequal( ipkg->feature, parent ) )
			continue;

		/* skip INVISIBLE package */
		if( !ip_is_visible( ipkg ) && !ip_is_visiblepkg( ipkg ) )
			continue;

                /* skip aggregate package since it is a "bundle" of   */
                /* individually-licensable product                    */
                if ( ipkg->aggregate == TRUE )
                        continue;

		if( includes( ipkg, pkg ) )
		{
			if( parent == NULL )
			{
STprintf( msg, "'%s' must be removed before '%s'.", ipkg->name, pkg->name );
exec frs message :msg with style = popup;
			}
			return( FALSE );
		}
	}

# ifdef BROKEN 
	/*
	** Don't delete package if it is INCLUDED by any package other
	** than the parent to this call.
	*/
	SCAN_LIST( instPkgList, ipkg )
	{
		/* ignore self-inclusion */
		if( ipkg == pkg )
			continue;

		/* skip parent to this call */
		if( parent != NULL && STequal( ipkg->feature, parent ) )
			continue;

                /* skip aggregate package since it is a "bundle" of   */
                /* individually-licensable product                    */
                if ( ipkg->aggregate == TRUE )
                        continue;

		/*
		** Check refList of INCLUDED file.
		*/
		SCAN_LIST( ipkg->refList, ref )
		{
			if( ref->type == INCLUDE &&
				STequal( ref->name, pkg->feature ) )
			{
				if( parent == NULL )
				{
STprintf( msg, "'%s' must be removed before '%s'.", ipkg->name, pkg->name );
exec frs message :msg with style = popup;
				}
				return( FALSE );
			}
		}
	}
# endif /* BROKEN */

	/*  
	** Run deletion scripts, but not for patch:
	** we do them lower down.
	*/
	if ( (STbcompare(pkg->name, 5, "patch", 5, TRUE)) != 0)
		if( !delete_utility( pkg ) )
			return( FALSE );

# ifdef DEBUG_DELETE
STprintf( msg, "Removing package %s...", pkg->feature );
exec frs message :msg with style = popup;
# endif /* DEBUG_DELETE */
	/*
	** Delete files for this package.
	*/
	delete_ok = TRUE;
	SCAN_LIST( pkg->prtList, prt )
	{
		SCAN_LIST( prt->filList, fil )
		{
			loc_setup();
			LOfaddpath( &loc, fil->directory, &loc );
			LOfstfile( fil->name, &loc );

			if( loc_exists() )
			{
				if( OK != LOdelete( &loc ) )
					delete_ok = FALSE;
			}
		}
	}
	if( !delete_ok )
	{
		/* LOdelete() failed at least once */
		pkg->state = UNDEFINED;
		return( FALSE );
	}

	/*
	** for patch: overlay SIZE/CHECKSUM values
	** with values generated from restored
	** originals
	*/
	if ( (pmode == TRUE) && (STbcompare(pkg->name, 5, "patch", 5, TRUE)) == 0)
	{
		if( !delete_utility( pkg ) )
			return( FALSE );
		ip_overlay(pkg, PATCHOUT);
	}
	/*
	** Delete package from instPkgList.
	*/
	SCAN_LIST( instPkgList, ipkg )
	{
		if( !STbcompare( ipkg->feature, 0, pkg->feature,
			0, TRUE ) )
		{
			break;
		}
	}
	remove_inst_pkg( pkg );

			


	/*
	** Clean up garbage.
	*/
	if( parent == NULL )
	{
		FLIST *fl = (FLIST *) NULL;
		LISTELE *lp, *next;

		/*
		** Recursively build complete list of INVISIBLE packages 
		** included (directory or indirectly) by VISIBLE packages.
		** NOTE: we can't use the SCAN_LIST() macro here, since
		** SCAN_LIST() is used within outer loops to scan same
		** list.  It would work if the macro were re-entrant.
		*/
		for( lp = instPkgList.head; lp != NULL; lp = lp->cdr )
		{
			pkg = (PKGBLK *) lp->car;
			if( !ip_is_visible( pkg ) && !ip_is_visiblepkg( pkg ) )
				continue;
			get_inc_inv_pkgs( &fl, pkg );
		}
	
		/*
		** Remove all INVISIBLE packages which are not in the list
		** and which do not INCLUDE any VISIBLE packages.
		*/
		for( lp = instPkgList.head; lp != NULL; lp = next )
		{
			FLIST *fp;
			bool found;

			/*
			** Save "next" here, since lp->next may be set 
			** to NULL at the end of this loop.
			*/
			next = lp->cdr;

			pkg = (PKGBLK *) lp->car;

			if( ip_is_visible( pkg ) || ip_is_visiblepkg( pkg ) )
				continue;

			/* Do NOT remove the "install" package! */
			if( STequal( pkg->feature, ERx( "install" ) ) )
				continue;

			for( found = FALSE, fp = fl; fp != (FLIST *) NULL;
				fp = fp->next )
			{
				if( STequal( fp->feature, pkg->feature ) )
				{
					found = TRUE;
					break;
				}
			}
			if( found )
				continue;

			/*
			** Make sure INVISIBLE package does not INCLUDE
			** VISIBLE ones.
			*/
			found = FALSE;
			SCAN_LIST( pkg->refList, ref )
			{
				if( ref->type == INCLUDE && (ipkg =
					ip_find_package( ref->name,
					INSTALLATION )) != NULL )
				{
					if( ip_is_visible( ipkg ) ||
						ip_is_visiblepkg( pkg ) )
					{
						found = TRUE;
						break;
					}
				}
			}
			if( found )
				continue;

# ifdef DEBUG_DELETE
STprintf( msg, "Removing GARBAGE package %s...", pkg->feature );
exec frs message :msg with style = popup;
# endif /* DEBUG_DELETE */
			/*
			** If here, package is INVISIBLE and not included
			** (directly or indirectly) by any VISIBLE package,
			** nor does it INCLUDE and VISIBLE packages, so
			** it can be safely removed.
			*/
			delete_ok = TRUE;
			SCAN_LIST( pkg->prtList, prt )
			{
				SCAN_LIST( prt->filList, fil )
				{
					loc_setup();
					LOfaddpath( &loc, fil->directory, &loc );
					LOfstfile( fil->name, &loc );
		
					if( loc_exists() )
					{
						if( OK != LOdelete( &loc ) )
							delete_ok = FALSE;
					}
				}
			}
			if( !delete_ok )
			{
				/* LOdelete() failed at least once */
				pkg->state = UNDEFINED;
				return( FALSE );
			}

			/*
			** Finally, remove it from instPkgList.
			*/
			SCAN_LIST( instPkgList, ipkg )
			{
				if( STequal( ipkg->feature, pkg->feature ) )
					break;
			}
			remove_inst_pkg( pkg );
		}

		/*
		** Free list.
		*/
		while( fl != (FLIST *) NULL )
		{
			FLIST *next;

			next = fl->next;
			MEfree( (PTR)fl );
			fl = next;
		}
	}
	return( TRUE );
}


/* ip_compare_name -- test whether a package has a specified name.  */

bool 
ip_compare_name( char *name, PKGBLK *pkg )
{
/*
exec sql begin declare section;
char msg[ 100 ];
exec sql end declare section;
STprintf( msg, "ip_compare_name comparing <%s> and <%s>", name, pkg->name );
exec frs message :msg with style = popup;
*/
	return( STequal( name, pkg->name ) || STequal( name, pkg->feature ) );
}

/*
** ip_find_package()
**
** locate the package block corresponding to a named package.  Returns
** NULL if the package is not found, otherwise returns a pointer to the
** corresponding package block.  The "which" parameter indicates
** whether to look for the package on the INSTALLATION or the DISTRIBUTION
** list.
*/

PKGBLK *
ip_find_package( char *name, i4  which )
{
	PKGBLK *pkg;
	LIST *listp = ( which == INSTALLATION ) ? &instPkgList :
		&distPkgList;
	LISTELE *lp;

	for( ip_list_rewind( listp ); NULL != ip_list_scan( listp,
		( PTR * )( &pkg ) ); )
	{
		if( ip_compare_name( name, pkg ) )
			return( pkg );
	}

	return( NULL );  /* Didn't find a package with that name. */
}


/*
** ip_find_feature()
**
** locate the package block corresponding to a specified feature name.
** Returns NULL if the package is not found, otherwise returns a pointer
** to the corresponding package block.  The "which" parameter
** indicates whether to look for the package on the INSTALLATION or
** the DISTRIBUTION list.
*/

PKGBLK *
ip_find_feature( char *name, i4  which )
{
	PKGBLK *pkg;
	LIST *listp = ( which==INSTALLATION )? &instPkgList: &distPkgList;

	for( ip_list_rewind( listp ); NULL != ip_list_scan( listp,
		( PTR * )( &pkg ) ); )
	{
		if( !STbcompare( name, 0, pkg->feature, 0, TRUE ) )
			return( pkg );
	}
	return( NULL );
}

/*
** ip_verify_files -- verify that the sizes and checksums of all files
**	in the specified package are what they should be.  Return OK if
**	so, FAIL if not.
**
** The global "ver_err_cnt" just keeps track of the number of verify
** errors.  If the number gets higher than a built-in maximum, we pop
** up a prompt which allows the user to stop the verification process
** or continue it.  If the user chooses to continue, we don't complain
** about subsequent errors, but we'll display a summary at the end.
*/

STATUS
ip_verify_files( pkg )
PKGBLK *pkg;
{
	PRTBLK *prt;
	FILBLK *fil;
	LISTELE *lp1, *lp2;

	i4 cnt, cks;
	char msg[512];

	for( lp1 = pkg->prtList.head; lp1 != NULL; lp1 = lp1->cdr )
	{
		prt = (PRTBLK *) lp1->car;

		for( lp2 = prt->filList.head; lp2 != NULL; lp2 = lp2->cdr )
		{
			char *path;
			LOCATION tmpLoc;
			char tmpLocBuf[ MAX_LOC + 1 ];

			fil = (FILBLK *) lp2->car;

			/*
			** Don't need to verify links, dynamic, or custom
			** files.
			*/
			if( fil->link || fil->dynamic || fil->custom )
				continue;

			loc_setup();
			IPCL_LOaddPath( &loc, fil->directory );

			/*
			** Display full path of file being verified.
			*/
			LOcopy( &loc, tmpLocBuf, &tmpLoc );
			LOfstfile( fil->name, &tmpLoc );
			LOtos( &tmpLoc, &path );
			STprintf( msg, ERget( S_ST0115_verifying_file ),
				path );
			ip_display_transient_msg( msg );
			
			if( ignoreChecksum )
				continue;
			else if( OK != ip_file_info( &loc, fil->name,
				&cnt, &cks ) )
			{
				IIUGerr( E_ST0116_cant_verify_file, 0, 3,
					fil->name, pkg->feature,
					fil->directory );
			}
			else if( ( fil->size != -1 && cnt != fil->size )
				|| ( fil->checksum != -1 &&
				cks != fil->checksum ) )
			{
				IIUGerr( E_ST015B_verification_error, 0, 7, 
					fil->name, fil->directory,
					( PTR ) &cks, ( PTR ) &fil->checksum, 
					( PTR ) &cnt, ( PTR ) &fil->size,
					SystemProductName );
			}
			else
				continue;

			/* We've encountered a verification error... */

			if( ver_err_cnt >= 0 && ++ver_err_cnt >= VER_ERR_MAX )
			{
				STcopy( ERget( S_ST0118_many_verify_errors ),
					msg );
				if( 1 == ip_listpick( msg, S_ST010B_abort,
					S_ST011C_abort_ver, S_ST010A_continue,
					S_ST011B_continue_ver ) )
				{
					ver_err_cnt = -1;
				}
				else
					return( FAIL );
				break;
			}
		}
	}
	return( OK );
}

/*
** ip_listpick -- Display a two-branch listpick.
**
** Display a listpick with a header as specified by the "msg" parameter,
** and two lines, each of which has a short title ( like "Yes" ), and a
** long explanation ( like "Go ahead and delete all my files" ).  Returns
** whatever IIFDlpListPick returns, which is basically 0 if the first
** option is chosen, 1 if the second option is chosen, and -1 if the
** "Cancel" menuitem is chosen.
*/

i4
ip_listpick( msg, m1, m2, m3, m4 )
char *msg;
ER_MSGID m1, m2, m3, m4;
{
	char lpstr[1024];
	i4 rtn;

	STcopy( ERget( m1 ),lpstr );
	STcat( lpstr, ERx( ";" ) );   /* Can't do it with STprintf... */
	STcat( lpstr, ERget( m2 ) );  /* ...cuz ERget returns ptr to static */
	STcat( lpstr, ERx( "\n" ) );  /* Grr. */
	STcat( lpstr, ERget( m3 ) );
	STcat( lpstr, ERx( ";" ) );
	STcat( lpstr, ERget( m4 ) );

	rtn = IIFDlpListPick( msg, lpstr, 0, -1, -1, NULL, NULL, NULL, NULL );
	exec frs redisplay;
	return( rtn );
}

/*
** ip_listpick3 -- display a three-branch listpick.
**
** Same idea as ip_listpick, except that there are three lines, and
** each line contains only a single field.
*/

i4
ip_listpick3( msg, m1, m2, m3 )
char *msg;
ER_MSGID m1, m2, m3;
{
	char lpstr[1024];
	i4 rtn;

	STcopy( ERget( m1 ),lpstr );
	STcat( lpstr, ERx( "\n" ) );
	STcat( lpstr, ERget( m2 ) );
	STcat( lpstr, ERx( "\n" ) );  
	STcat( lpstr, ERget( m3 ) );

	rtn = IIFDlpListPick( msg, lpstr, 0, -1, -1, NULL, NULL, NULL, NULL );
	return( rtn );
}

/*
** ip_describe -- display the description file for a specified package.
*/

void
ip_describe( char *pkgname )
{
	PKGBLK *pkg;

	if( NULL == ( pkg = ip_find_package( pkgname, DISTRIBUTION ) ) )
		return;

	if( pkg->description == NULL )
	{
		char msg[ 512 ];

		STprintf( msg, ERget( S_ST0166_no_description ), pkg->name );
		ip_display_msg( msg );
	}
	else
		ip_display_msg( pkg->description );
}


/*
**  Rewriting the packing list...
*/

GLOBALREF MAP ip_tokenid[];	 /* Tables in the parser... */
GLOBALREF MAP ip_comparison[];  /* ...which we use for translation. */

static i4  max_toklen;
static bool write_err;

# define tNULL	-1	/* NULL token indicator */

/* wlinec -- write a line to the output file with a character data field. 
**
**  ofile   the output file
**  indent  integer; the indentation level of the line
**  token1   integer corresponding to the keyword token
**  token2   integer corresponding to the keyword token
**  data	the data field
*/

static void
wlinec( FILE *ofile, i4  indent, i4  token1, i4  token2,
	char *data )
{
	char line[MAX_MANIFEST_LINE + 1];
	i4 toklen;
	MAP *tkid;
	STATUS rtn;

	if( data == NULL )
		return;

	*line = EOS;

	/* Do the indentation... */

	for( ; indent ; indent-- )
		STcat( line, ERx( "	" ) );

	/* 
	** Add token1 to the line, and pad it out to the length of the
	** longest keyword... 
	*/
	for( tkid = ip_tokenid; token1 > -1 && *tkid->token; tkid++ )
	{
		if( token1 == tkid->id )
		{
			STcat( line, tkid->token );
			for( indent = max_toklen -
				STlength( tkid->token ); indent; indent-- )
			{
				STcat( line, ERx( " " ) );
			}
			break;
		}
	}

	/* 
	** Add token2 to the line, and pad it out to the length of the
	** longest keyword... 
	*/
	for( tkid = ip_tokenid; token2 > -1 && *tkid->token; tkid++ )
	{
		if( token2 == tkid->id )
		{
			STcat( line, tkid->token );
			for( indent = max_toklen -
				STlength( tkid->token ); indent; indent-- )
			{
				STcat( line, ERx( " " ) );
			}
			break;
		}
	}

	/* Space over to where the data field is gonna go... */

	for( indent = 8 - ( ( STlength( line )+1 ) & 7 ) ; indent; indent-- )
		STcat( line,ERx( " " ) );

	/* Put it there... */

	STcat( line, data );
	STcat( line, ERx( "\n" ) );

	if( SIputrec( line,ofile ) == EOF )
		write_err = TRUE;
}


/*
** wlinen -- write a data line with a numeric data field.  Wrapper for
**	"wlinec", above.
*/

static void
wlinen( FILE *ofile, i4  indent, i4  token1, i4  token2, i4  num )
{
	char nval[10];

	if( num >= 0 )
	{
		STprintf( nval, "%d", num );
		wlinec( ofile, indent, token1, token2, nval );
	}
}


/*
** wref -- write a data line corresponding to a reference block.  If it's
**	a reference that includes a comparison, also write out the version
**	line that contains the comparison.
*/

static void
wref( FILE *pklf, REFBLK *ref, bool blank, i4  lvl )
{
	switch ( ref->type )
	{
		case INCLUDE:
			wlinec( pklf, lvl, tINCLUDE, tNULL, ref->name );
			break;
		case NEED:
			wlinec( pklf, lvl, tNEED, tNULL, ref->name );
			break;
		case PREFER:
			wlinec( pklf, lvl, tPREFER, tNULL, ref->name );
			break;
	}

	if( ref->version != NULL )
	{
		MAP *compid;
		char line[MAX_MANIFEST_LINE];

		for( compid = ip_comparison; *compid->token; compid++ )
		{
			if( ref->comparison == compid->id )
			{
				STpolycat( 3, compid->token, ERx( " " ),
					ref->version, line );
				wlinec( pklf, lvl + 1, tVERSION,
					tNULL, line );
				break;
			}
		}
	}
}

/*
** open_file -- open a file for output, preparatory to copying an internal
**	list to the file in manifest format.
**
** Note that we don't really open the specified file; we prepend the letter
** "T" onto the filename and open that.  When we "close" the file after
** it's been written, we also rename it to what it should be.  This prevents
** us from blowing away the old version of the file if we run into an
** error or run out of disk space or something.
*/

static STATUS
open_file( char *dir, char *filename, FILE **ofile, char *mode )
{
	char tfile[MAX_LOC+1];
	char lobuf[MAX_LOC+1];
	LOCATION fileloc;
	STATUS rtn;

	if( filename == NULL )
	{
		*ofile = stdout;
		return( OK );
	}
	STcopy( dir,lobuf );
	IPCL_LOfroms( PATH, lobuf, &fileloc );
        if(STcompare(mode, "r"))
        {
	    STpolycat( 2, ERx( "T" ), filename, tfile );
	    LOfstfile( tfile, &fileloc );
        }
        else
        {
            /* mode is read, must be an exresponse rsp file */
            LOfstfile( filename, &fileloc );
        }
	return( SIfopen( &fileloc, mode, SI_TXT, MAX_MANIFEST_LINE,
		ofile ) );
}


# ifdef UNIX

/*
** change_mode() -- change the permission on a file.
**
** Warning: heavy Unix dependency here.
*/

static STATUS
change_mode( char *filename, i4  mode )
{
	return( ( chmod( filename, mode ) ) ? FAIL : OK );
}

# endif /* UNIX */

/*
** close_file -- close an output file, and rename it.  See note about
**	renaming above, with the "open_file" function.
** History:
**
**	11-dec-2001 (gupsh01)
**	    Added support for handling response file closure.
*/

static STATUS
close_file( char *dir, char *filename, FILE *file )
{
	char obuf[MAX_LOC+1], nbuf[MAX_LOC+1];
	LOCATION oloc, nloc;
	char *nname;
	STATUS stat;

	if( filename == NULL )
		return( OK );

	(void) SIclose( file );

        if((dir == NULL) && (filename == NULL))
        {
            /* Caller doe snot want a rename */
            return(OK);
        }
        else
        {
	    STcopy( dir,obuf );
	    IPCL_LOfroms( PATH, obuf, &oloc );
	    STpolycat( 2, ERx( "T" ), filename, nbuf );
	    LOfstfile( nbuf, &oloc );

	    STcopy( dir,nbuf );
	    IPCL_LOfroms( PATH, nbuf, &nloc );
	    LOfstfile( filename, &nloc );
    
	    if( OK != ( stat = LOrename( &oloc, &nloc ) ) )
		    return( stat );

# ifdef UNIX	
	    LOtos( &nloc, &nname );
	    return( change_mode( nname, 0644 ) );
# else /* UNIX */
	    return( OK );
# endif /* UNIX */
        }
}

/*
** ip_open_response - open a response file for writing.
**
** History:
**
**	11-dec-2001 (gupsh01)
**	    Added support for handling response file existing. 
*/
STATUS
ip_open_response( char *dir, char *filename, FILE **resp_file, char *mode) 
{
    STATUS rc;
    char testlobuf[MAX_LOC+1];
    LOCATION testfileloc;
    LOINFORMATION loinf;
    i4 flagword;

    if (!filename) filename = "ingrsp.rsp"; 
    STcopy( dir,testlobuf );

    /* In mkresponse mode then we should actually
    ** check if the file exists.
    */

    IPCL_LOfroms( PATH, testlobuf, &testfileloc );
    LOfstfile( filename, &testfileloc ) ;
    flagword = LO_I_TYPE;
    if (LOinfo( &testfileloc, &flagword, &loinf ) == OK)
    {
         if(mkresponse)
         {
             /* location exists, report error and exit */
             IIUGerr( E_ST0191_response_exists, 0, 1, filename );
             PCexit (FAIL);
         }
    }
    else
    {
        if(exresponse)
        {
            /* location does not exist, report error and exit */
            IIUGerr( E_ST0192_response_not_found, 0, 1, filename );
            PCexit (FAIL);
        }
    }

    if( OK != ( rc = open_file( ERx(""), filename, resp_file, mode) ) )
    {
    	IIUGerr( E_ST0155_file_open_error, 0, 2, filename,
    		dir );
    }
    return (rc);
}
/*
** ip_write_response - writes out a response file.
*/
STATUS
ip_write_response( char *var, char *value, FILE *resp_file) 
{
    char line[MAX_LOC + 1];
    STATUS rc;

    /* default rtn = OK */
    rc = OK; 

    STlpolycat(4, sizeof(line)-1, var, ERx("="), value, ERx("\n"), line);

    if( SIputrec (line, resp_file ) == EOF )
    {
       IIUGerr( E_ST0189_response_write, 0, 0);
       (void) SIclose( resp_file );
       rc = FAIL;
    }

    return (rc);
}

/*
** ip_close_response - close a response file if open.
**
** History:
**      02-11-01 (gupsh01)
**          Added user specified file handling. 
*/
STATUS
ip_close_response( FILE *resp_file)
{
   STATUS rc = OK;
   char *filename;
   char *def_file ="ingrsp.rsp";
   
   if (respfilename != NULL )
	filename=respfilename;
   else 
	filename=def_file;

   if(exresponse)
   {
       if( (rc = close_file( NULL, NULL, resp_file )) != OK );
       {
           rc = FAIL;
       }
   }
   else
   {
       if( (rc = close_file( ERx(""), filename, resp_file )) != OK )
       {
             IIUGerr( E_ST0152_rename_error, 0, 2,
            		    filename, "");
             rc = FAIL;
       }
   }
   return (rc);
 
}
/*
** check_pkg_response(pkg->feature) - add a package to the response file.
*/
STATUS 
ip_chk_response_pkg(char *pkg_name)
{
   i2 i=0;
   STATUS rc=OK;
   char* set_yes = ERx("YES");
   char value_string[MAX_MANIFEST_LINE];
   bool validated = FALSE;

   char *values[] =
   { "esql","dbms","abf", "c2audit", "net","ome","secure",
     "spatial","star","rep","tm","oldmsg", "tuxedo",
     "qr_run", "qr_tools","vispro","ice","bridge","mwvisualtools",
     "custominst", "ingbase","netclient", "dbmsnet", "dbmsstar",   
     "standalone", "END" };

   STcopy (pkg_name, value_string);
   STcat (value_string, ERx("_set"));

   while (STcompare (values[i], ERx("END")) != 0)
   {
       if (STcompare (values[i], pkg_name) == 0); 
       { 	
           validated = TRUE;
      	   break;
	}
	i++;
   } 

   if (validated)
   { 
       ip_write_response (ERx(value_string),set_yes, rSTREAM); 
       rc = OK;
   }
   return rc; 
} 


/*
** ip_write_manifest -- write an internal list to a manifest text file.
**
** Arguments:
**
**	which		INSTALLATION or DISTRIBUTION.
**	dir		Directory where file is to be written.
**	pkg_list	Pointer to the list to be copied out.
**	release_id	Revision level of the entire product.
**	product_name	Name of the entire product.
**
** Returns OK for success, FAIL for failure.
*/

STATUS
ip_write_manifest( i4  which, char *dir, LIST *pkg_list, char *release_id,
	char *product_name )
{
	FILE *pklf;
	PKGBLK *pkg, *cpkg;
	PRTBLK *prt;
	FILBLK *fil;
	REFBLK *ref;
	STATUS rc;
	MAP *tkid;
	bool blank, need_prt_hdr;
	char *prevname;
	char *file;

	if( which == INSTALLATION )
		file = INSTALLATION_MANIFEST;
	else
		file = RELEASE_MANIFEST;

	/* Figure out the length of the longest keyword; for formatting... */

	for( max_toklen = 0, tkid = ip_tokenid; *tkid->token; tkid++ )
	{
		if( STlength( tkid->token ) > max_toklen )
			max_toklen = STlength( tkid->token );
	}

	/* Open the output file... */

	if( OK != ( rc = open_file( dir, file, &pklf, ERx( "w" ) ) ) )
	{
		IIUGerr( E_ST0155_file_open_error, 0, 2, file,
			dir );
		return( rc );
	}

	write_err = FALSE;   /* Initially. */

	/* Write out the product header... */
					
	wlinec( pklf, 0, tRELEASE, tNULL, product_name );

	/* 
	** Sleazy sorting.  We're going to make multiple passes through
	** the list, picking entries in alphabetical order by the "name"
	** field.  We just run the list from the beginning every time,
	** looking for the lowest-sorting name that's greater than the
	** name we picked on the previous pass.  Would Knuth be proud
	** of me, or what?  
	*/

	for( prevname = ERx( " " ) ; ; )
	{
		for( pkg = NULL, ip_list_rewind( pkg_list ); 
			NULL != ip_list_scan( pkg_list, ( PTR * )
			( &cpkg ) ); )
		{
			/*
			** Hide i18n and documentation package
			*/
			if( which == DISTRIBUTION && cpkg->hide) 
			   continue;

			if( STbcompare( prevname, 0, cpkg->name, 0, TRUE )
				< 0 )
			{
				if( pkg == NULL || STbcompare( cpkg->name,
					0, pkg->name, 0, TRUE ) < 0 )
				{
					pkg = cpkg;
				}
			}
		}

		/* Didn't find anything after last one?  We're done! */
		if( pkg == NULL )
			break;

		prevname = pkg->name;

		wlinec( pklf, 0, tPACKAGE, tNULL, pkg->feature );
		wlinec( pklf, 1, tVERSION, tNULL, pkg->version );
		if( pkg->state )
			wlinen( pklf, 1, tSTATE, tNULL, pkg->state );
		else if( which == INSTALLATION )
			wlinen( pklf, 1, tSTATE, tNULL, UNDEFINED );

		/* 
		** The following is stuff which we want if we're copying a
		** distribution to build a release, but which we don't want
		** if we're making a descriptor file for the installation.  
		*/
		if( which == DISTRIBUTION )
		{
			wlinen( pklf, 1, tCHECKSUM, tNULL,
				pkg->image_cksum );
			wlinen( pklf, 1, tSIZE, tNULL,
				pkg->image_size );
		}

		switch ( pkg->visible )
		{
			case VISIBLE:
				break;

			case VISIBLEPKG:
				wlinec( pklf, 1, tVISIBLEPKG, tNULL,
					ERx( "" ) );
				break;

			case INVISIBLE:
				wlinec( pklf, 1, tINVISIBLE, tNULL,
					ERx( "" ) );
				break;
		
			default:
				wlinec( pklf, 1, tVISIBLE, tNULL,
				    ip_bit2ci( pkg->visible ) );
				break;
		}

                /*
                ** B80559: new aggregate mbr, should be written out for both
                ** DISTRIBUTION and INSTALLATION.
                */
                if (pkg->aggregate == TRUE)
                        wlinec( pklf, 1, tAGGREGATE, tNULL, ERx( "" ) );

		blank = TRUE;
		SCAN_LIST( pkg->refList, ref )
		{
		    if( ( hide &&
			  ( STequal( ref->name, ERx("i18n") ) ||
			    STequal( ref->name, ERx("i18n64") ) ||
			    STequal( ref->name, ERx("documentation")) ) ) ||
		      ( ( !spatial )  &&
			  ( STequal( ref->name, ERx("spatial") ) ||
			    STequal( ref->name, ERx("spatial64")) ) ) )
		    {
			    /* skip hidden package */
			    continue;
		    }
		    wref( pklf, ref, blank, 1 );
		    blank = FALSE;
		}

		SCAN_LIST( pkg->prtList, prt )
		{
			char *previous_dir;
			char *previous_vol;

			previous_dir = NULL;
			previous_vol = NULL;

			need_prt_hdr = TRUE;  /* Delay - might be no files */

			SCAN_LIST( prt->filList, fil )
			{
				if( fil->exists )
				{
					if( need_prt_hdr )
					{
						i4 *bitp;

						wlinec( pklf, 1, tPART, tNULL,
							prt->name );

						SCAN_LIST( prt->capList, bitp )
						{
							wlinec( pklf, 1,
								tCAPABILITY, 
								tNULL,
                                                                ip_bit2ci(
                                                                *bitp ) );
						}

						if( STcompare( pkg->version,
							prt->version ) )
						{
							wlinec( pklf, 2,
								tVERSION,
								tNULL,
								prt->version );
						}

						if( prt->preload != NULL )
						{
							wlinec( pklf, 2,
								tPRELOAD,
								tNULL,
								prt->preload );
						}

						if( prt->delete != NULL )
						{
							wlinec( pklf, 2,
								tDELETE,
								tNULL,
								prt->delete );
						}

						blank = TRUE;
						SCAN_LIST( prt->refList, ref )
						{
							if( ( hide &&
							      ( STequal( ref->name, ERx("i18n") ) ||
								STequal( ref->name, ERx("i18n64") ) ||
								STequal( ref->name, ERx("documentation")) ) ) ||
							  ( ( !spatial )  &&
							      ( STequal( ref->name, ERx("spatial") ) ||
								STequal( ref->name, ERx("spatial64")) ) ) )
							{
								/* skip hidden package */
								continue;
							}
							wref( pklf, ref,
								blank, 2 );
							blank = FALSE;
						}

						need_prt_hdr = FALSE;
					}

					if( previous_dir == NULL ||
						STequal( fil->generic_dir,
						previous_dir ) == 0 )
					{
						wlinec( pklf, 1, tDIRECTORY,
							tNULL,
							fil->generic_dir );
					}
					previous_dir = fil->generic_dir;

					if( previous_vol == NULL ||
						STequal( fil->volume,
						previous_vol ) == 0 )
					{
						wlinec( pklf, 1, tVOLUME,
							tNULL, fil->volume );
					}
					previous_vol = fil->volume;

#ifdef VMS
					wlinec( pklf, 2, tPERMISSION, tNULL,
							fil->permission_sb );
#endif
					wlinec( pklf, 2, fil->link ? tLINK :
						tFILE, fil->setup ? tSETUP :
						tNULL, fil->name );

					/* no checksums or sizes for links */
					if( !fil->link )
					{
						wlinen( pklf, 3, tCHECKSUM,
							tNULL, fil->checksum );

						wlinen( pklf, 3, tSIZE, tNULL,
							fil->size );

						if( fil->custom )
						{
							wlinec( pklf, 3, tCUSTOM,
								tNULL, ERx( "" ) );
						}
	
						if( fil->dynamic )
						{
							wlinec( pklf, 3, tDYNAMIC,
								tNULL, ERx( "" ) );
						}
					}
				}
			}
		}
	}

	if( write_err )
	{
		IIUGerr( E_ST0153_write_error, 0, 2, INSTALLATION_MANIFEST,
				installDir );
		(void) SIclose( pklf );
		rc = FAIL;
	}
	else
	{
		if( (rc = close_file( dir, file, pklf )) != OK )
		{
			IIUGerr( E_ST0152_rename_error, 0, 2,
				INSTALLATION_MANIFEST, installDir );
		}
	}

	return( rc );
}


/*
** ip_comp_version() -- compare two INGRES version strings.
**
** Here's what we think a version string looks like:
**
**	a.b/c ( ---.---/d )
**
** ...where a, b, c, and d are one- or two-digit numbers, the ---'s
** represent noise that we aren't interested in, and other punctuation
** is as shown.  The numbers have left-to-right precedence; in other words,
** we only need to compare the "b" fields if the "a" fields are identical,
** and so forth like that.
*/

static i4  get_num();
static bool cvers();

bool
ip_comp_version( char *v1, uchar comparison, char *v2 )
{
	i4 comp;

	/* If all fields were "don't-care" -- weird */
	if( v1 == NULL || v2 == NULL || !comparison ||
		!cvers( v1, v2, &comp ) )
	{
		return( TRUE );
	}

	switch ( comparison )
	{
		case 0:
			return( TRUE );
		case COMP_EQ:
			return( comp == 0 );
		case COMP_LT:
			return( comp <  0 );
		case COMP_LE:
			return( comp <= 0 );
		case COMP_GT:
			return( comp >  0 );
		case COMP_GE:
			return( comp >= 0 );
	}
}

/*
** cvers -- compare two version numbers.
**
** Returns:
**	diff < 0  ==>  v1 < v2
**	diff == 0 ==>  v1 == v2
**	diff > 0  ==>  v1 > v2
**
** The return code will be TRUE if there were any do-care fields.  A
** do-care field is the opposite of a don't-care field, and a don't-care
** field is one which isn't part of the comparison because it isn't there.
** For instance, a version ID like "6.5/00 ( su4.u42 )" has a don't-care
** build-level field.  Don't-care fields can also be coded explicitly,
** for example...
**
**	 Version  == 6.5/? ( su4.u42/00 )
**
** ...will match any release level of 6.5.
*/

static bool
cvers( char *v1, char *v2, i4  *diff )
{
	i4 n1, n2;
	bool rtn = FALSE;

	if( 0 <= ( n1 = get_num( &v1 ) ) && 0 <= ( n2 = get_num( &v2 ) ) )
	{
		if( 0 != ( *diff = n1 - n2 ) )  /* Major release number */
			return( TRUE );
		rtn = TRUE;
	}

	if( 0 <= ( n1 = get_num( &v1 ) ) && 0 <= ( n2 = get_num( &v2 ) ) )
	{
		if( 0 != ( *diff = n1 - n2 ) )  /* Minor release number */
			return( TRUE );
		rtn = TRUE;
	}

	if( 0 <= ( n1 = get_num( &v1 ) ) && 0 <= ( n2 = get_num( &v2 ) ) )
	{
		if( 0 != ( *diff = n1 - n2 ) )  /* Maintenance release number */
			return( TRUE );
		rtn = TRUE;
	}

	/* 
	** Now we have to scan ahead to find the next thing that follows
	** a slash, which I believe is called the build or patch number. 
	*/

	while( *v1 && CMcmpcase( v1, ERx( "/" ) ) )
		CMnext( v1 );
	while( *v2 && CMcmpcase( v2, ERx( "/" ) ) )
		CMnext( v2 );


	if( 0 <= ( n1 = get_num( &v1 ) ) && 0 <= ( n2 = get_num( &v2 ) ) )
	{
		*diff = n1 - n2;	/* Build number */
		return( TRUE );
	}

	return( rtn );
}

/*
** get_num -- get the next number in a version string.  Used by cvers.
**	Returns the next numeric value, or -1 for a don't-care field.
**
** History:
**	01-27-95 (forky01)
**	    CVan was being passed RValue of num instead of &num, changed
**	    to &num.
*/

static i4
get_num( char **v )
{
	i4 num;
	char anum[MAX_MANIFEST_LINE];
	char *anump;

	while( **v && !CMdigit( *v ) && CMcmpcase( *v, ERx( "?" ) ) )
		CMnext( *v );

	if( !CMdigit( *v ) )
	{
		/* Either at end of string, or a don't care field */
		while( **v && !CMcmpcase( *v, ERx( "?" ) ) )
			CMnext( *v ); /* Skip over don't-care */
		return( -1 );
	}

	for( anump = anum; CMdigit( *v ); CMnext( *v ), CMnext( anump ) )
		CMcpychar( *v,anump );

	*anump = EOS;
	(void) CVan( anum,&num );
	return( num );
}

/*
** ip_is_visible -- determine whether a package is visible or not.
**
** Packages may be visible, invisible, or conditionally visible.  The
** "conditional" case means that they're visible if some capability
** bit is in some particular state; if the "visible" field in the package
** block is less than NOT_BIAS, the bit has to be on for the package to
** be visible; otherwise, the bit corresponding to the value of the "visible"
** field minus the constant NOT_BIAS has to be off for the package to be
** visible.  I think I'm making this sound more complicated than it is...
*/

bool 
ip_is_visible( pkg )
PKGBLK *pkg;
{
	switch ( pkg->visible )
	{
		case VISIBLE:
			return( TRUE );

		case INVISIBLE:
			return( FALSE );

		case VISIBLEPKG:
			return( FALSE );

		default:
			return( TRUE );
	}
}


/*
** ip_is_visiblepkg -- determine whether a package is visiblepkg or not.
**
** Packages may be visible, invisible, or conditionally visible.  The
** VISBLEPKG flag is used to do group installs.
*/
 
bool
ip_is_visiblepkg( pkg )
PKGBLK *pkg;
{
        switch ( pkg->visible )
        {
                case VISIBLEPKG:
                        return( TRUE );
 
                default:
                        return( FALSE );
        }
}

/*
** resolve_need()
**
** Resolve a NEED dependency against the installation description.
** Loop through the list that describes the current installation and
** see if there's a version of the required package that satisfies
** the condition expressed in the dependency.  If it's satisfied, return
** TRUE.  If it's not satisfied, return( FALSE ).  If it's not satisfied
** because a version constraint is not satisfied, return a pointer to
** the installed version number.
*/

static bool
resolve_need( PKGBLK *pkg, char *version, uchar comparison, char **vers )
{
	PKGBLK *ipkg;
	
	if( NULL == ( ipkg = ip_find_feature( pkg->feature, INSTALLATION ) ) )
	{
		*vers = NULL;
		return( FALSE );
	}
	*vers = ipkg->version;
	return( ip_comp_version( ipkg->version, comparison, version ) );
}

/*
** resolve()
**
** Recursively resolve package dependencies.  Automatically select
** INCLUDED packages and prompt prompt the user first to resolve NEED
** dependencies.  Packages required but already installed should be 
** not be prompted for, nor should they be selected.
*/

static STATUS 
resolve( PKGBLK *parent, uchar type, char* version, uchar comparison,
	PKGBLK *pkg )
{
	PKGBLK *ipkg;
	REFBLK *ref;
	char *vers;
	LISTELE *lp;
# ifdef DEBUG_RESOLVE
exec sql begin declare section;
char msg[ 100 ];
exec sql end declare section;
# endif /* DEBUG_RESOLVE */

	ipkg = ip_find_package( pkg->feature, INSTALLATION );

	if( parent == NULL )
	{
		/*
		** At the top level, ignore packages which are NOT_SELECTED.
		*/
		if( pkg->selected == NOT_SELECTED )
			return( OK );

		/*
		** Deselect installed packages.
		*/
		if( ipkg != NULL && STequal( pkg->version, ipkg->version ) )
		{
			pkg->selected = NOT_SELECTED; 
			return( OK );
		}
	}
	else if( ipkg != NULL && STequal( pkg->version, ipkg->version ) )
	{
# ifdef DEBUG_RESOLVE
STprintf( msg, "resolve(): %s already installed", pkg->feature ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_RESOLVE */
		/*
		** Deselect installed packages.
		*/
		pkg->selected = NOT_SELECTED; 
		return( OK );
	}

	/*
	** Call resolve() recursively.
	*/
	for( lp = pkg->refList.head; lp != NULL; lp = lp->cdr )
	{
		PKGBLK *dpkg;

		ref = (REFBLK *) lp->car;

		if( (dpkg = ip_find_package( ref->name, DISTRIBUTION ))
			== NULL )
		{
			return( FAIL );	
		}

		if( ref->type != PREFER && resolve( pkg, ref->type,
			ref->version, ref->comparison, dpkg ) != OK )
		{
			return( FAIL );
		}
	}

	/*
	** Handle dependencies.
	*/
	switch( type )
	{
		case INCLUDE:
# ifdef DEBUG_RESOLVE
STprintf( msg, "resolve(): selected %s since it is INCLUDED", pkg->feature ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_RESOLVE */
			pkg->selected = INCLUDED; 
			break;

		case NEED:	  
			if( pkg->selected == NOT_SELECTED )
			{
				if( !ip_is_visible( pkg ) &&
					 !ip_is_visiblepkg( pkg ) )
				{
					PKGBLK *dpkg;
					bool found = FALSE;
/*
** BUG: If an INVISIBLE PACKAGE is NEEDED, but a VISIBLE package INCLUDES
** it, then that VISIBLE package needs to be prompted for, and resolved if
** selected. 
*/
# ifdef DEBUG_RESOLVE
STprintf( msg, "resolve(): INVISIBLE package %s is NEEDED but not VISIBLE", pkg->feature ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_RESOLVE */
					/*
					** Try to find a VISIBLE package
					** which INCLUDES this one.  If
					** there is one, it needs to be
					** prompted for as the NEED and
					** if selected, it needs to be
					** resolved.
					*/
					for( lp = distPkgList.head;
						lp != NULL;
						lp = lp->cdr )
					{
						dpkg = (PKGBLK *) lp->car;
						if( !ip_is_visible( dpkg ) && 
						     !ip_is_visiblepkg( dpkg ) )
							continue;
# ifdef DEBUG_RESOLVE
STprintf( msg, "resolve(): Checking whether %s INCLUDES %s",
	dpkg->feature, pkg->feature ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_RESOLVE */
						if( includes( dpkg, pkg ) )
						{
# ifdef DEBUG_RESOLVE
STprintf( msg, "resolve(): %s INCLUDED by VISIBLE package %s",
	pkg->feature, dpkg->feature ); 
exec frs message :msg with style = popup;
STprintf( msg, "%s->selected=%d", dpkg->feature, dpkg->selected ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_RESOLVE */
							found = TRUE;
							break;	
						}
					}
					if( found )
					{
						if( dpkg->selected ==
							NOT_SELECTED &&
							!resolve_need( dpkg,
							version, comparison,
							&vers ) )
						{
							if( ip_dep_popup(
								parent, dpkg,
								vers ) )
							{
# ifdef DEBUG_RESOLVE
STprintf( msg, "resolve(): calling ip_resolve( %s )...", dpkg->feature ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_RESOLVE */
								if( ip_resolve( dpkg->feature ) != OK )
									return( FALSE );	
								pkg->selected = INCLUDED; 
							}
							else
								return( FALSE );
						}
					}
					else
# ifdef DEBUG_RESOLVE
{
STprintf( msg, "resolve(): NEEDED package %s selected silently",
	pkg->feature ); 
exec frs message :msg with style = popup;
# endif /* DEBUG_RESOLVE */
						pkg->selected = INCLUDED;
# ifdef DEBUG_RESOLVE
}
# endif /* DEBUG_RESOLVE */
				}
				else if( pkg->selected == NOT_SELECTED &&
					!resolve_need( pkg, version,
					comparison, &vers ) && 
					!ip_dep_popup( parent, pkg, vers ) )
				{
					return( FAIL );
				}
			}
			break;
	} 
	return( OK );
}

/*
** ip_resolve()
**
** Scan through the list of packages, and deal with any dependencies
** in the set.  For INCLUDE dependencies, automatically select the
** requisite package; for NEED dependencies, prompt the user first.
** Packages required but already installed should not be prompted for.
*/

# define NO_TYPE 10
# define NO_COMP 10

bool
ip_resolve( char *pkgname )
{
	PKGBLK *pkg;
	LISTELE *lp;

	if( pkgname == NULL )
	{
		for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
		{
			pkg = (PKGBLK *) lp->car;
			if( resolve( NULL, NO_TYPE, NULL, NO_COMP, pkg ) != OK )
				return( FALSE );
		} 
		return( TRUE );
	}
	/*
	** If here, called for a specific packge.
	*/
	if( (pkg = ip_find_package( pkgname, DISTRIBUTION )) == NULL )
		return( FALSE );
	return( resolve( NULL, NO_TYPE, NULL, NO_COMP, pkg ) == OK ?
		TRUE : FALSE );
}


/*
** ip_init_installdir -- set up the global "installDir" field, which is
**	a path consisting of the value of the II_SYSTEM variable suffixed
**	with "ingres".
*/

void
ip_init_installdir( void )
{
	char *cp, *p;

	NMgtAt( SystemLocationVariable, &cp );
	if( !cp || !*cp )
	{

		/* 
		** Right, I'm not using the standard error message system
		** here.  If the user hasn't even so much as set II_SYSTEM,
		** it's highly unlikely that we'd be able to find a message
		** file, so let's not even try. 
		*/

		SIprintf( ERx( "\nThe environment variable %s must be set to" ), SystemLocationVariable );
		SIprintf( ERx( " the full path name\nof the directory where %s is" ), SystemDBMSName );
		SIprintf( ERx( " to be installed.  %s executables\nwill be placed" ), SystemDBMSName );
		SIprintf( ERx( " in subdirectories of the path specified by the value" ) );
		SIprintf( ERx( " of %s.\n\nPlease assign a value" ), SystemLocationVariable );
		SIprintf( ERx( " to %s and re-run this program.\n\n" ), SystemLocationVariable );
		PCexit( FAIL );
	}

	for (p = cp; *p != EOS; CMnext(p))
	{
# ifndef VMS
	    if (CMalpha(p) || CMdigit(p) ||
		*p == '_' || *p == '/' || *p == '.' || *p == '-' )
		continue;
	    SIprintf(ERx("\nThe environment variable %s contains characters \
that are not\n"),
		SystemLocationVariable);
	    SIprintf(ERx("allowed. The path should only contain alphabetic, \
digit, period,\n"));
	    SIprintf(ERx("underscore and hyphen characters.\n"));
# else
	    if (CMalpha(p) || CMdigit(p) || *p == '_' || *p == '.' ||  
		*p == ':' || *p == '$' || *p == '[' || *p == ']' || 
		*p == '<' || *p == '>' || *p == '-' )
		continue;
	    SIprintf(ERx("\nThe logical %s contains characters \
that are not allowed.\n"),
		SystemLocationVariable);
	    SIprintf(ERx("The path should only contain alphabetic, \
digit, underscore,\n"));
	    SIprintf(ERx("colon, dollar sign, [], <> \
and hyphen characters\n"));

# endif /* VMS */
	    PCexit(FAIL);
	}


	/* set up path for target installation directory */
	STlcopy( cp, installDir, MAX_LOC-1 );
	loc_setup();
}
/* 
** Name: ip_overlay
**
** Description:
**		Walk prt list of patch package:
**		find corresponding FILBLK in installation description,
**		INSTALLING PATCH:
**			copy across values from patch's FILBLK
**		REMOVING PATCH:
**			get values anew from disk
**		(Files starting '[Pp]atch' are ignored, to bypass
**		checking of patch.doc, patchxxx.hlp)
**
**Input:
**		ipkg	-	patch package being installed/removed
**		mode	-	PATCHIN		-> patch installation
**					PATCHOUT	-> patch removal
**Output:
** History :
**	18-march-1996 (angusm)
**		Initial creation
**
*/
VOID
ip_overlay(PKGBLK *ipkg, i4  mode)
{
	PRTBLK *prt;
	FILBLK *fil;
	FILBLK *ifil;
	LOCATION floc;
	char msgBuf[ 512 ];
	i4	size=0;
	i4 csum=0;

	if (pmode == FALSE)
		return;
	
	SCAN_LIST(ipkg->prtList, prt)
	{
		SCAN_LIST(prt->filList, fil)
		{
			if ((fil != NULL) && ( fil->link ) )
				continue;
			if ( (STbcompare(fil->name, 5, "patch", 5, TRUE)) == 0)	
				continue;
			ifil = (FILBLK *)ip_hash_file_get(fil->directory, fil->name);
			if ((ifil != NULL) && ( !ifil->link ) )
			{
				if (mode == PATCHIN)
				{
					ifil->size=fil->size;
					ifil->checksum=fil->checksum;
				}
				else if (mode == PATCHOUT)
				{
					loc_setup();
					IPCL_LOaddPath( &loc, ifil->directory );
					ip_file_info( &loc, ifil->name, &size, &csum );
					ifil->size=size;
					ifil->checksum=csum;
				}
				else
				{
					ifil->size=0;
					ifil->checksum=0;
				}
				STprintf( msgBuf, ERget( S_ST0209_overlay_size_checksum), 
				ipkg->name );
				ip_display_transient_msg( msgBuf );
			}
			else
			{
				STprintf( msgBuf, ERget( S_ST020A_overlay_not_found ), 
				fil->name);
				ip_display_transient_msg( msgBuf );
			}
		} /*part file list*/
	}/*package part list*/
}

