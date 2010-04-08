/*
**  Copyright (c) 1993, Ingres Corporation
**
**  ipcl.c -- Unix IP compatibility layer intended to isolate the platform
**	dependent code upon which front!st!install depends.
**
**  History:
**	17-nov-93 (tyler)
**		Created.
**	10-dec-93 (tyler)
**		Added IPCL_LOfroms().
**	13-dec-93 (tomm)
**		putenv needs ip_stash_string or it will do strange things
**		to the environement.
**	05-jan-94 (tyler)
**		IPCLtransfer() needed to clean up the tmp directory when
**		tar failed.
**	12-jan-94 (tyler)
**		Fixed broken remote tar device check in IPCLsetDevice().
**	31-jan-94 (tyler)
**		Fixed BUG which caused remote tape devices not to be
**		recognized.
**	07-feb-94 (tyler/fredv)
**		Use gtar instead of native tar, since tar fails to
**		preserve setuid on HP and possibly other platforms.
**	22-feb-94 (tyler)
**		Fixed bad message id.  Corrected bad parameters to 
**		message E_ST015B_verification_error.  Fixed BUG 58860:
**		if a package verification error occurs, explain that	
**		installation of selected products has been aborted
**		and clean up the temporary directory.  Fixed BUG 56799:
**		Report an error if the user doesn't enter an absolute
**		pathname for a local distribution device.
**	31-Mar-1994 (fredv)
**		Same as the previous change from tomm, we need ip_stash_...()
**		in putenv().
**	06-Apr-1994 (michael)
**		Fixed bad (uninitialized) pointer being passed to IDname in
**		IPCLsetDevice().
**	23-Jun-1994 (michael)
**		Added "-B" option to invocation of "gtar".
**	24-jun-1994 (tomm)
**		HP does not use rsh, but uses remsh instead.  Create a
**		#define RSH which contains the correct command and message
**		the code to use it.  Last change.
**      06-mar-95 (tutto01) BUG #67312  &  BUG #67316
**              Changed S_ST0185 to E_ST0185 to more accurately represent
**              the ingres message naming format.  Also added call to
**              IPCLsetEnv so that IPCLsetDevice's actions are seen by others.
**	11-nov-95 (hanch04)
**		Added checks for ip_is_visiblepkg
**	10-apr-96 (nick)
**	    Print error if we can't create or chmod the temporary directory 
**	    when in IPCLtransfer(). #62921
**	16-oct-96 (mcgem01)
**		Misc error and informational messages changed to take
**		the product name as a parameter.
**      16-dec-96 (reijo01)
**          Changed to use generic system variables.
**	24-jun-97 (hanch04)
**	    Updated call to ip_cmdline to use generic filename,
**	    SystemCfgPrefix"build.err" instead of ingbuild.err.
**	03-mar-98 (kinpa04)
**	    Added include me.h since MEcmp was not being converted
**	    properly for DoubleByte ports. For non-DoubleByte, me.h
**	    was being included through cmcl.h .
**	11-mar-1999 (hanch04)
**	    Added ignoreChecksum.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	11-Nov-2000 (hanje04)
**	    Changed command to uncompress and de-archive package archive
**	    on Linux, to use /bin/tar instead of gtar. This is to remove 
**	    the dependancy of ingbuild on uncompress.	    
**	11-Jul-2002 (hanje04)
**	    Use 'system' tar on all Linuxes.
**	02-Aug-2004 (hanje04)
**	    Replace use of tar and gtar with pax
**	17-sep-2004 (stephenb)
**	    Solris pax doesn't support compression
**	24-sep-2004 (gupsh01)
**	    Use zcat instead of uncompress for pax, so that the input for
**	    pax is from command line 
**	13-Oct-2004 (bonro01)
**	    HP uses zcat instead of uncompress for pax.
**	    Use -pe to preserve SETUID bit.
**	04-Nov-2004 (bonro01)
**	    Pax issues warnings during install when the install UID is 
**	    not 102, which is the UID used to create the archive.
**	    This is the result of the -pe flag.  The -pp flag must be
**	    used instead, but pax will drop the setuid bit from
**	    executables.  The setuid bit will now be set programatically
**	    in the installation scripts.
**	13-Jan-2005 (hweho01)
**	    Use /bin/tar to archive the package for AIX.
**	17-Jan-2005 (lazro01)
**	    UnixWare uses zcat instead of uncompress for pax.
**	15-Mar-2005 (bonro01)
**	    a64_sol users zcat to uncompress install image
**	30-Mar-2005 (hweho01)
**          Fix error "Cannot set the time on ." if the directory of 
**          $II_SYSTEM is at the top level of the file system on AIX.
**	Aug-9-2005 (hweho01)
**          Added support for axp_osf platform. 
**      13-Feb-2006 (hweho01)
**          Use gtar (GNU tar version 1.15.1) to archive/de-archive
**          component package on AIX platform.
**      31-May-2006 (hweho01)
**          Use gtar (GNU tar version 1.15.1) to archive/de-archive
**          component package on Tru64 platform platform.
**      20-Mar-2007 (hweho01)
**          Use OS archiver for Solaris platform.  
**	5-Apr-2007 (bonro01)
**	    Use OS archiver for all Unix platforms and rearrange code
**	    to be more portable.
**      26-Apr-2007 (hweho01)
**          Use OS archiver for AIX and Tru64 platforms.  
**	08-Oct-2007 (hanje04)
**	    SIR 114907
**	    pax is in /bin on Mac OSX not /usr/bin
**	12-Feb-2008 (hanje04)
**	    SIR S119978
**	    Replace mg5_osx with generic OSX
**	05-Jun-2008 (bonro01)
**	    Extracting files from the ingres.tar fails on OpenServer
**	    because of an apparent OS bug which causes umask to be
**	    set to 133 which causes new directories to not have the
**	    execute bit set and makes the directory unaccessible.
**	    Setting the umask to 022 eliminates the problem on
**	    OpenServer and should be good insurance on other unix
**	    platforms as well.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Use any_aix, sparc_sol, any_hpux symbols as needed.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added header files to eliminate gcc 4.3 warnings.
*/

# include <sys/stat.h>
# include <compat.h>
# include <me.h>
# include <cm.h>
# include <er.h>
# include <id.h>
# include <lo.h>
# include <st.h>
# include <iplist.h>
# include <ip.h>
# include <ipcl.h>
# include <erst.h>
# include <gl.h>
# include <iicommon.h>
# include <fe.h>
# include <ug.h>
# include <gc.h>
# include <nm.h>
# include <iicommon.h>

static char *destDir;		/* target directory for installation */ 
static char *devPath;		/* distribution media device name */
static char *devHost;		/* distribution device remote host */
static LOCATION tmpLoc;		/* temporary directory LOCATION */
static char tmpLocBuf[ MAX_LOC + 1 ]; /* tmpLoc LOCATION buffer */
static char *tmpDir;		/* temporary directory path */
static LOCATION tarLoc;		/* tar executable LOCATION */
static char tarLocBuf[ MAX_LOC + 1 ]; /* tar executable LOCATION buffer */ 
static char *tarPath;		/* tar executable path */ 
GLOBALREF bool ignoreChecksum;

#ifdef any_hpux
#define RSH "remsh"
#else
#define RSH "rsh"
#endif

#if defined(LNX)
#define OS_ARCHIVER_NAME  ERx( "pax" )
#define OS_ARCHIVER_OPTIONS_TRAN ERx( " -rf " )
#define OS_ARCHIVER_OPTIONS_INST ERx( " -pp -rzf " )

#else
static  LOCATION  archiver_loc;  
static	char name_str[LO_NM_LEN]; 
# ifdef OSX
#define OS_ARCHIVER_LOC   ERx( "/bin" )
# else
#define OS_ARCHIVER_LOC   ERx( "/usr/bin" )
# endif /* Mac OSX */

#if defined(any_aix)
#define OS_ARCHIVER_NAME  ERx( "tar" )
#define OS_ARCHIVER_OPTIONS_TRAN ERx( " -xpf " )
#define OS_ARCHIVER_OPTIONS_INST ERx( " -xpf - " )
#else
#define OS_ARCHIVER_NAME  ERx( "pax" )
#define OS_ARCHIVER_OPTIONS_TRAN ERx( " -rf " )
#define OS_ARCHIVER_OPTIONS_INST ERx( " -pp -r " )
#endif  /* if aix  */

#endif  /* if defined(LNX)      */

/*
** IPCLbuildPath() -- construct a native path given a generic path.
*/ 

char *IPCLbuildPath( char *p, IPCL_ERR_FUNC *error ) 
{
	char *locbuf = (char *) ip_alloc_blk( MAX_LOC + 1 );

	if( *p == '(' )
	{
		char *pp;
		char *nm_value;


		for( pp = p; *pp != ')' && *pp != EOS; CMnext( pp ) );
		if( *pp == EOS )
		{
			error( "Expecting ')' in path specification." );
			return( NULL );
		}

		CMnext( p );
		*pp = EOS;

		NMgtAt( p, &nm_value );
		STcopy( nm_value, locbuf );

		*pp = ')';
		p = pp;
		CMnext( p );
		if( *p == EOS )
			return( locbuf );

		if( *p != '!' )
		{
			error( "Expecting '!' in path specification." );
			return( NULL );
		}
	}
	else
		STcopy( ERx( "" ), locbuf );	

	STcat( locbuf, p );
	for( p = locbuf; *p != EOS; CMnext( p ) )
	{
		if( *p == '!' )
			*p = '/';
	}

	return( locbuf );
}

/*
** IPCLbuildPermission() -- construct a native PERMISSION given a
**	generic permission string from the release description
**	file (release.dat).
*/ 

PERMISSION IPCLbuildPermission( char *s, IPCL_ERR_FUNC *error )
{
	static PERMISSION permission;

# define EXECUTE_MASK	1
# define WRITE_MASK	2
# define READ_MASK	4

# define OWNER_SHIFT	6
# define GROUP_SHIFT	3
# define WORLD_SHIFT	0

# define SETUID_MASK	04000

	permission = 0;
	while( *s != EOS )
	{
		i4 shift;

		/* check for "setuid" */
		if( STbcompare( s, 0, ERx( "setuid" ), 0, FALSE ) == 0 ) 
		{
			permission |= SETUID_MASK;
			while( *s != ',' && *s != EOS )
				CMnext( s );	
			if( *s == ',' )
				CMnext( s );
			continue;
		}
		
		switch( *s )
		{
			case 'o':
				shift = OWNER_SHIFT;
				break;

			case 'g':
				shift = GROUP_SHIFT;
				break;

			case 'w':
				shift = WORLD_SHIFT;
				break;

			case 's':
				/* ignore system permissions */
				while( *s != ',' && *s != EOS )
					CMnext( s );	
				if( *s == ',' )
					CMnext( s );
				continue;

			default:
				error( "Unrecognized permission class." );
				while( *s != ',' && *s != EOS )
					CMnext( s );	
				if( *s == ',' )
					CMnext( s );
				continue;
		}
		CMnext( s );
		if( *s != ':' )
		{
			error( "Expecting ':' in permission string." );
			/*
			** we don't care what gets returned since the parse
			** will fail and exit.
			*/
			return( permission );
		}
		CMnext( s );
		while( *s != ',' && *s != EOS )
		{
			switch( *s )
			{
				case 'r':
					permission |= (READ_MASK << shift);
					break;
	
				case 'w':
					permission |= (WRITE_MASK << shift);
					break;
	
				case 'e':
					permission |= (EXECUTE_MASK << shift);
					break;
	
				case 'd':
					/* ignore delete permission */
					break;
			}	
			CMnext( s );
		}
		if( *s == ',' )
			CMnext( s );
	}	

	return( permission );
}

/*
** IPCLcreateDir() -- create a directory from a native path specification;
**	if the directory exists, return OK.
*/

STATUS
IPCLcreateDir( char *dirPath )
{
	STATUS status;
	char cmdline[ 256 ];
	char locbuf[ MAX_LOC + 1 ];
	LOCATION loc;
	LOINFORMATION locInfo;
	i4 flagword = LO_I_TYPE;

	STcopy( dirPath, locbuf );

	if( (status = LOfroms( PATH, locbuf, &loc )) != OK )
		return( status );
	
	if( LOinfo( &loc, &flagword, &locInfo ) == OK ) 
		return( chmod( dirPath, 0775 ) == 0 ? OK : FAIL );

	if( (status = LOcreate( &loc )) == OK ) 
		 status = (chmod( dirPath, 0775 ) == 0) ? OK : FAIL;

	return( status );
}

/*
** IPCLsetDevice() -- set and validate the distribution device name.
*/

bool
IPCLsetDevice( char *devName )
{
	char *cp;
	STATUS status;
	LOCATION tmpLoc;
	char tmpLocBuf[ MAX_LOC + 1 ];
	char msg[ 1024 ];

	/* check for remote host at beggining of device specification */
	if( (cp = STindex( devName, ERx( ":" ), 0 )) != NULL )
	{
		char cmdBuf[ 1024 ];
		char line[ SI_MAX_TXT_REC + 1 ];
		FILE *fp;
		bool response; 

		/* confirm remote host */
		ip_display_transient_msg( ERget( S_ST013D_confirming_host ) );

		devHost = STalloc( devName );
		cp = STindex( devHost, ERx( ":" ), 0 );
		*cp = EOS;
		devPath = CMnext( cp );

		STpolycat( 3, ERx( RSH " " ), devHost, ERx( " pwd" ),
			cmdBuf );

		if( (status = ip_cmdline( cmdBuf, ( ER_MSGID ) 0 )) != OK )
		{
			char hostName[ 100 ];
			char user[DB_MAXNAME];
			char *tmpptr; /* because apparently no pointer is on
				       * the stack for user[] can't take its
				       * address.
				       */

                        /*** note that IDname will bomb if passed an uninit'ed
			 *** pointer. why it needs a char ** is unapparent, but
	                 *** am passing it an initialized one.
		         ***/

                        tmpptr = user;

			IDname( &tmpptr );

			(void) GChostname( hostName, sizeof( hostName ) );
			IIUGerr( E_ST013B_no_such_host, 0, 3,
				devHost, user, hostName );
			return( FALSE );
		}

		/* confirm remote device */	
		ip_display_transient_msg( ERget( S_ST013E_confirming_dev ) );

		/*  
		** Since rsh won't reliably return the exit code of
		** the remote command, we use a command which results in
		** displaying a magic cookie to stdout if the remote device
		** exists, and then we check the output log file to see if
		** that happened.  Gotta do what works...  (jonb)
		*/

# define RSH_TEST_STR ERx( "eureka" )
# define RSH_TEST_CMD \
		ERx( RSH " %s \"echo '[ -f %s -o -c %s ] && echo %s' | /bin/sh\"" )

		STprintf( cmdBuf, RSH_TEST_CMD, devHost, devPath, devPath,
			RSH_TEST_STR );

		(void) ip_cmdline( cmdBuf, ( ER_MSGID ) 0 );

		*tmpLocBuf = EOS;
		(void) LOfroms( PATH & FILENAME, tmpLocBuf, &tmpLoc );
		STprintf(msg,ERx("%sbuild"),SystemCfgPrefix);
		(void) LOcompose( NULL, TEMP_DIR, msg,
			ERx( "err" ), NULL, &tmpLoc );

		if( (status = SIfopen( &tmpLoc, ERx( "r" ), SI_TXT,
			SI_MAX_TXT_REC, &fp )) != OK )
		{
			return( FALSE );
		}

		response = FALSE;
		while( SIgetrec( line, SI_MAX_TXT_REC, fp ) == OK )
		{
			char *cp;

			for( cp = line; *cp != EOS; CMnext( cp ) )
			{
				i4 len = STlength( RSH_TEST_STR );

				if( STlength( cp ) < len )
					break;
				if( STscompare( cp, len, RSH_TEST_STR, len )
					== 0 )
				{
					response = TRUE;
					break;
				}
			}
			if( response )
				break;
		}

		(void) SIclose( fp );

		if( !response ) 
		{
			IIUGerr( E_ST013C_no_such_remote_dev, 0, 2,
				devPath, devHost );
            		return( FALSE );
		}
	}
	else
	{
		LOCATION devLoc;
		char devLocBuf[ MAX_LOC + 1 ];	

		if( *devName != '/' )
		{
			IIUGerr( E_ST0185_not_absolute_path, 0, 1, 
					SystemProductName );
			return( FALSE );
		}
		devHost = NULL;
		devPath = devName;
		STcopy( devPath, devLocBuf );
		LOfroms( PATH & FILENAME, devLocBuf, &devLoc );
		if( LOexist( &devLoc ) != OK )
		{
			IIUGerr( E_ST0120_no_such_device, 0, 1, devPath );
			return( FALSE );
		}
                /* share with other programs called afterwards */
                IPCLsetEnv(ERx("II_DISTRIBUTION"),devPath,FALSE);
	}
	return( TRUE );
}

/*
** IPCLtransfer() --  transfer all packages in the specified LIST not 
**	marked NOT_SELECTED from the selected device into the file system
**	without performing transformations on the transferred files.
*/

STATUS
IPCLtransfer( LIST *pkgList, char *dir )
{
	STATUS status;
	bool noneFound = TRUE;
	char cmdBuf[ 1024 ];
	char *cp;
	PKGBLK *pkg;


	ip_display_transient_msg( ERget( S_ST0105_installing ) );

	destDir = dir;

	/* set up LOCATIONs of temporary directory and archiver executable */
	STcopy( dir, tmpLocBuf );
	LOfroms( PATH, tmpLocBuf, &tmpLoc );
	LOfaddpath( &tmpLoc, SystemLocationSubdirectory, &tmpLoc );
	LOfaddpath( &tmpLoc, ERx( "install" ), &tmpLoc );

#if defined(OS_ARCHIVER_LOC)
        STprintf( name_str, "%s/%s", OS_ARCHIVER_LOC, OS_ARCHIVER_NAME );
        LOfroms( PATH & FILENAME, name_str, &archiver_loc );
        if ( (status = LOexist(&archiver_loc)) != OK )
	  {
	   IIUGerr( E_ST0116_cant_verify_file, 0, 3, archiver_loc.string,
		    "OS utility", OS_ARCHIVER_LOC );  
	   return( status ); 
          }
	LOtos( &archiver_loc, &tarPath );
#else
	LOcopy( &tmpLoc, tarLocBuf, &tarLoc );
	LOfstfile( OS_ARCHIVER_NAME, &tarLoc );
	LOtos( &tarLoc, &tarPath );
#endif

	LOfaddpath( &tmpLoc, ERx( "tmp" ), &tmpLoc );
	LOtos( &tmpLoc, &tmpDir );
	if( (status = IPCLcreateDir( tmpDir )) != OK )
	{
		IIUGerr(E_ST020B_tmp_create_chmod, 0, 1, tmpDir);
		return( status );
	}

	STpolycat( 3, ERx( "cd " ), tmpDir, ERx( "; " ), cmdBuf );
	STpolycat( 2, cmdBuf, ERx( "umask 022 ; " ), cmdBuf );

	if( devHost == NULL )
	{
		/* distribution device is local */
                STpolycat( 4, cmdBuf, tarPath, OS_ARCHIVER_OPTIONS_TRAN,
                        devPath, cmdBuf );
	}
	else
	{
		/* distribution device is remote */
                STpolycat( 11, cmdBuf, ERx( " " RSH " " ), devHost,
                        ERx( " ' " ), ERx( " dd bs=20b if=" ), devPath,
                        ERx( " ' " ), ERx( " | " ), tarPath,
                        OS_ARCHIVER_OPTIONS_TRAN, ERx( " - " ), cmdBuf );
	}

	SCAN_LIST( *pkgList, pkg )
	{
	    if(( pkg->selected != NOT_SELECTED) && ( pkg->image_size != 0 ))
	    {
	        noneFound = FALSE;
	        STcat( cmdBuf, ERx( " " ) );
	        STcat( cmdBuf, pkg->feature );
	    }
	}

	if( noneFound )
	{
		IIUGerr( E_ST0104_no_pkgs_selected, 0, 0 );
		return( OK );
	}
	if( ip_cmdline( cmdBuf, E_ST0107_tar_failed ) != OK )
	{
		STpolycat( 2, ERx( " rm -rf " ), tmpDir, cmdBuf );
		(void) ip_cmdline( cmdBuf, E_ST0112_cant_delete_dir );
		return( FAIL );
	} 
	return( OK );
}

/*
** IPCLverify() -- verify that the file transfer phase for a specified
**	package was successful.
*/

STATUS
IPCLverify( PKGBLK *pkg )
{
	LOCATION pkgLoc;
	char pkgLocBuf[ MAX_LOC + 1 ];
	char *pkgDir;
	i4 size, cksum;
	char msgBuf[ 512 ];
	STATUS status;

	/* return OK if package has no image size information */
	if( pkg->image_size == 0 )
		return( OK );

	/* construct LOCATION for package archive */
	STcopy( tmpDir, pkgLocBuf );
	LOfroms( PATH, pkgLocBuf, &pkgLoc );
	LOfaddpath( &pkgLoc, pkg->feature, &pkgLoc );
	LOtos( &pkgLoc, &pkgDir );
	pkgDir = STalloc( pkgDir );
	LOfstfile( INSTALL_ARCHIVE, &pkgLoc );

	/* display status message */
	if( pkg->nfiles && ( ip_is_visible( pkg ) || ip_is_visiblepkg( pkg ) ) )
		STprintf( msgBuf, ERget( F_ST0157_verifying_pkg ), pkg->name );
	else
	{
		/* print a different message for a support package */
		STprintf( msgBuf, ERget( F_ST0151_verifying_supp_pkg ),
			pkg->name );
	}
	ip_display_transient_msg( msgBuf );

	/* compute size and checksum for package archive */
	if( ignoreChecksum )
	{
		MEfree( pkgDir );
		return( OK );
	}
	else if( ip_file_info_comp( &pkgLoc, &size, &cksum, FALSE ) != OK )
	{
		IIUGerr( E_ST0116_cant_verify_file, 0, 3,
			 INSTALL_ARCHIVE, pkg->name, pkgDir );
	}
	else if( pkg->image_cksum != cksum || pkg->image_size != size )
	{
		char cmdBuf[ 1024 ];

		IIUGerr( E_ST015B_verification_error, 0, 6,
			 INSTALL_ARCHIVE, pkgDir,
			 ( PTR ) &cksum, ( PTR ) &pkg->image_cksum, 
			 ( PTR ) &size, ( PTR ) &pkg->image_size,
			 SystemProductName );
		IIUGerr( E_ST0156_install_aborted, 0, 0 );
		STpolycat( 2, ERx( " rm -rf " ), tmpDir, cmdBuf );
		(void) ip_cmdline( cmdBuf, E_ST0112_cant_delete_dir );
	}
	else
	{
		MEfree( pkgDir );
		return( OK );
	}

	MEfree( pkgDir );
	return( FAIL );
}


/*
** IPCLinstall() -- move files for the specified package to their final
**	destinations.  Any transformations which need to be performed
**	on the file(s) as originally transferred from the distribution
**	media should be performed at this time.  If any file cannot be
**	successfully installed, all files which make up the package
**	should be removed before IPCLinstall() returns. 
**
**  History:
**      01-27-95 (forky01)
**          Fix bugs 63176 & 62359 - add package with no files to install.dat
**          so setup knows they need setting up, since they might include
**          other packages.
*/

STATUS
IPCLinstall( PKGBLK *pkg )
{
	LOCATION tarLoc;
	char tarLocBuf[ MAX_LOC+1 ];
	char *tarFile;
	char cmdBuf[ 1024 ];

	if( pkg->nfiles == 0 )
	{
		/* add top-level package to installation contents */
		ip_add_inst_pkg( pkg );
		return( OK );
	}
 
	if( ip_is_visible( pkg ) || ip_is_visiblepkg( pkg ) )
	{
		STprintf( cmdBuf,
			ERget( S_ST010E_moving_files ),pkg->name );
	}
        else
	{
		STprintf( cmdBuf, ERget( S_ST015A_moving_supp_files ),
			pkg->name );
	}
	ip_display_transient_msg( cmdBuf );

	if( pkg->image_size == 0 )
	{
		/* add degenerate package to installation contents */
		ip_add_inst_pkg( pkg );
		return( OK );
	}
 
	LOcopy( &tmpLoc, tarLocBuf, &tarLoc );
	LOfaddpath( &tarLoc, pkg->feature, &tarLoc );
	LOfstfile( INSTALL_ARCHIVE, &tarLoc );
	LOtos( &tarLoc, &tarFile );

	/* create command to uncompress and de-archive package archive */
#if defined(LNX)
	STpolycat( 6, ERx( " cd " ), destDir, ERx(  ";umask 022;" ),
		tarPath, OS_ARCHIVER_OPTIONS_INST, tarFile, cmdBuf );
#else
        STpolycat( 8, ERx( " cd " ), destDir, ERx(  ";umask 022;" ),
                ERx( "zcat " ), tarFile, ERx( " | "),
                tarPath, OS_ARCHIVER_OPTIONS_INST, cmdBuf );
#endif

	if( ip_cmdline( cmdBuf, E_ST0107_tar_failed ) == OK )
	{
		ip_add_inst_pkg( pkg );
		return( OK );
	}
	return( FAIL );
}

/*
** IPCL_LOaddPath() -- add a relative directory path to an INGRES LOCATION,
**	given a native relative directory specification.
*/

void
IPCL_LOaddPath( LOCATION *loc, char *dirPath )
{
	char *cp;
	i4 dirCount = MAX_LOC, i;
	char *dirNames[ 100 ];

	dirPath = STalloc( dirPath );
	for( cp = dirPath; *cp != EOS; cp++ )
	{
		if ( *cp == '/' )
			*cp = ' ';
	}

	/* parse whitespace delimited directories */ 
	STgetwords( dirPath, &dirCount, dirNames );

	/* Add to the directory path */
	for( i = 0; i < dirCount; i++ )
		LOfaddpath( loc, dirNames[ i ], loc );

	MEfree( dirPath );
}

/*
** IPCLcleanup() -- cleanup temporary files belonging to a package.
*/

void
IPCLcleanup( PKGBLK *pkg )
{
	LOCATION pkgLoc;
	char pkgLocBuf[ MAX_LOC+1 ];
	char cmdBuf[ 1024 ];
	char *pkgDir;

	LOcopy( &tmpLoc, pkgLocBuf, &pkgLoc );
	LOfaddpath( &pkgLoc, pkg->feature, &pkgLoc );
	LOtos( &pkgLoc, &pkgDir );

	STpolycat( 2, ERx( " rm -rf " ), pkgDir, cmdBuf );
	(void) ip_cmdline( cmdBuf, E_ST0112_cant_delete_dir );
}

/*
** Data structure used to store temporary environment settings.
*/

static struct {
	char *name;
	char *value;
} tmpEnv[ 100 ];

static i4  numEnv = -1;

/*
** IPCLsetEnv() -- set name = value in the current environment.
**  History:
**      16-dec-96 (reijo01)
**          Changed to use generic system variables.
*/

void
IPCLsetEnv( char *name, char *value, bool temp )
{
	i4 i;
	char buf[ 512 ];
	char newname[ 512 ];

	if ( MEcmp( name, "II", 2) == OK )
	    STpolycat( 2, SystemVarPrefix, name+2, newname );
	else
	    STcopy( name, newname );

        if ( STcompare( name, "TERM_INGRES" ) == OK ) 
	     STcopy( SystemTermType, newname );

	STpolycat( 3, newname, ERx( "=" ), value, buf );
	(void) putenv( ip_stash_string( buf ) );
	if( temp )
	{
		for( i = 0; i <= numEnv; i++ )
		{
			if( STcompare( newname, tmpEnv[ i ].name ) == 0 )
				break;
		}
		if( i > numEnv )
			i = ++numEnv;
		tmpEnv[ i ].name = ip_stash_string( newname ); 
		tmpEnv[ i ].value = ip_stash_string( value ); 
	}
}

/*
** IPCLclearEnv() - remove environment settings made with IPCLsetEnv().
*/

void
IPCLclearTmpEnv( void )
{
	i4 i;

	if( numEnv < 0 )
		return;
	for( i = 0; i <= numEnv; i++ )
	{
		char buf[ 512 ];

		STpolycat( 2, tmpEnv[ i ].name, ERx( "=" ), buf );
		(void) putenv( ip_stash_string(buf) );
	}
}

/*
** IPCLresetTmpEnv() - restore environment settings make with IPCLsetEnv(). 
*/

void
IPCLresetTmpEnv( void )
{
	i4 i;

	if( numEnv < 0 )
		return;
	for( i = 0; i <= numEnv; i++ )
	{
		char buf[ 512 ];

		STpolycat( 3, tmpEnv[ i ].name, ERx( "=" ),
			tmpEnv[ i ].value, buf );
		(void) putenv( ip_stash_string(buf) );
	}
}

/*
** IPCL_LOmove() -- move a file. 
*/

STATUS
IPCL_LOmove( char *path1, char *path2 )
{
	if( rename( path1, path2 ) == 0 )
		return( OK );
	return( FAIL );
}

/*
** IPCL_LOfroms() -- wrapper for LOfroms, which works fine on Unix.
*/
STATUS IPCL_LOfroms( LOCTYPE what, char *locBuf, LOCATION *loc )
{
	return( LOfroms( what, locBuf, loc ) );
}


