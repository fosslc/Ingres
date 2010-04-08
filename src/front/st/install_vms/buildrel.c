/*
** Copyright (c) 1992, 2005 Ingres Corporation
**
*/

#include <compat.h>
#include <si.h>
#include <st.h>
#include <me.h>
#include <lo.h>
#include <ex.h>
#include <er.h>
#include <gc.h>
#include <cm.h>
#include <nm.h>
#include <dbms.h>
#include <fe.h>
#include <ug.h>
#include <ui.h>
#include <te.h>
#include <iplist.h>
#include <erst.h>
#include <uigdata.h>
#include <stdprmpt.h>
#include <ip.h>

/*
**  Name: buildrel -- build tool to convert a build area to a release area
**                    ( VMS version! )
**
**  Entry points:
**      This file contains no externally visible functions.
** 
**  Command-line Options:
**	-a 	Simply generate a listing of missing files without computing
**		or moving files
**	-p	Run buildrel against a specified product
**      -b      Use alternate black box filename for release.dat
**	-f	Ignore missing files.
**	-q 	Run silently.
**	-m 	Generate release.dat file only without actually moving files.
**	-l gpl|com|emb|eval Override license type
**      -u      Unhide i18n, i18n64, and documentation packages
**      -s      include spatial data objects in the saveset
**
**
**  History:
**	12-aug-92 (jonb)
**		Created.
**	18-feb-93 (jonb)
**		Add volume level to the output file.  Write manifest to "A"
**		volume.  Display missing files in -n mode.
**	23-march-93 (jonb)
**		Removed some useless Unix-only code.  Added code to set
**   		the correct permission on every file moved.
**	26-mar-93 (jonb)
**		Add support for symbol names in source directory names.  A
**		symbol may be included in a directory name by surrounding
**		it with single quote marks.
**	3-aug-93 (kellyp)
**		Backed out last two changes. Made necessary changes to
**		accomodate the new data structure for the combined prt
**		files. Also, wrapped all strings with ERx.
**	26-oct-93 (kellyp)
**		Changed installation dir from [install] to [ingres.install]
**	11-nov-93 (tyler)
**		Ported to IP Compatibility Layer.
**	19-nov-93 (tyler)
**		Modified to just verify file existence during audit.
**	30-nov-93 (tyler)
**		Write release.dat in the [.A.INSTALL] directory.
**	01-dec-93 (tyler)
**		Added argument to ip_parse() and added check to make
**		sure volume field is never NULL.
**	02-dec-93 (kellyp)
**		Corrected printf to be SIprintf
**	10-dec-93 (tyler)
**		Replaced LOfroms() with calls to IPCL_LOfroms().
**		Changed SIprintf() back to printf().
**	10-dec-93 (tyler)
**		Previous change was botched.  This change required to
**		submit correct version.
**	15-nov-95 (hanch04)
**		added eXpress
**	05-mar-96 (albany)
**		Cross-integrated Chris Hane's Unix change 422801:
**		Also added some potentially useful Unix code.
**		19-jan-96 (hanch04)
**		    RELEASE_MANIFEST is now releaseManifest
**	            added -b flag for black box release.dat file
**	09-16-96(rosga02)
**		added dummy pmode to avoid linking error
**	28-jun-1999 (kinte01)
**		Added dummy ignoreChecksum for Unix change 442196 to
**		avoid linking error.
**	29-jul-1999 (hweho01)
**	    Added statement 'return( OK )' at the end of routine
**	    write_manifest() if there is no error. Without the OK return
**	    code, the process will not continue on ris_u64 platform.
**	15-feb-2001	(kinte01)
**	    Bug 103393 - removed nat, longnat, u_nat, & u_longnat
**	    from VMS CL as the use is no longer allowed
**	19-jun-2001 (kinte01)
**	    update for multiple product builds
**	17-oct-2001 (kinte01)
**	    added GLOBALDEF for mkresponse mode per change 453278 for 
**	    bug 105848.
**	02-nov-2001 (kinte01)
**	    add GLOBALDEF for exresponse 
**	31-jan-2002 (kinte01)
**	    added GLOBALDEF for respfilename.
**	18-feb-2005 (abbjo03)
**	    Change to use II_RELEASE_DIR instead of FRONT_STAGE.
**	24-feb-2005 (abbjo03)
**	    The releaseDir pointer variable was expected to retain the value
**	    of II_RELEASE_DIR during execution.  Since NM/LO use a static
**	    buffer, that was an invalid assumption.
**	21-Feb-2006 (bonro01)
**	    Add LICENSE file back into install directory.
**	    LICENSE file needs to be variable for each type of licensing
**	    that we support.  Substitute %LICENSE% in manifest with the
**	    correct license file specified by "buildrel -l"
**      25-Jan-2007 (upake01)
**           - Remove i18n and documentation from default zip file
**           - Make sure that i18n, documentation and spatial packages
**             are hidden during audit (-a) as well
**           - Replase -s for silent with -q and use -s for spatial objects
**      28-Oct-2008 (horda03)
**           Mimic Unix's BUILDREL and create a Build Timestamp file.
**      11-Nov-2008 (horda03)
**           Obtain the value of ING_BUILD after II_RELEASE_DIR value has
**           been stored.
**      22-dec-2008 (stegr01)
**           Itanium VMS port.
**      07-oct-2009 (stegr01)
**           report success for .obj files on Itanium - SIfopen assumes that
**           the record format is variable (which it is on Alpha) - however
**           on Itanium the record format is undefined (since it's an ELF format)
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
*/

/*
**  INSTALL_FEATNAME defines a directory which gets special handling.
**  It is created explictly even if it doesn't appear in the manifest,
**  and it goes into the release area unarchived and uncompressed.
*/

#define INSTALL_FEATNAME "install"

GLOBALDEF char *distRelID;
GLOBALDEF char *instRelID;
GLOBALDEF char *productName;
GLOBALDEF char *instProductName;
GLOBALDEF char distribDev[1]; /*** Dummy ***/
GLOBALDEF char distribHost[1];/*** Dummy ***/
GLOBALDEF i4   ver_err_cnt;    /*** Dummy ***/
GLOBALDEF bool batchMode = TRUE;     /*** Dummy ***/
GLOBALDEF bool eXpress;       /*** Dummy ***/
GLOBALDEF bool pmode;       /*** Dummy ***/
GLOBALDEF bool ignoreChecksum=FALSE;  /*** dummy ***/
GLOBALDEF char userName[64];
GLOBALDEF LIST distPkgList;
GLOBALDEF LIST instPkgList;
GLOBALDEF LIST dirList;
GLOBALDEF char installBuf[ MAX_LOC + 1 ];
GLOBALDEF char installDir[ MAX_LOC + 1 ];
GLOBALDEF char releaseManifest[ 32 ];
GLOBALDEF bool mkresponse = FALSE;
GLOBALDEF bool exresponse = FALSE; 
GLOBALDEF char *respfilename = NULL; /* user supplied response file name */
GLOBALDEF char *license_file = NULL;
GLOBALDEF char *gpl_lic = "gpl" ;
GLOBALDEF char *com_lic = "com" ;
GLOBALDEF char *emb_lic = "emb" ;
GLOBALDEF char *eval_lic = "eval" ;
GLOBALDEF char *license_type = NULL ;
GLOBALDEF char *build_timestamp = NULL;

/* Global References */
GLOBALREF bool hide;
GLOBALREF bool spatial;

static char releaseBuf[MAX_LOC+1];
static STATUS create_subdirectories();
static STATUS move_files();
static STATUS write_manifest();
static STATUS compute_file_info();
static STATUS audit_files();
static bool getyorn();
static STATUS substitute_variables();
static LOCATION installLoc;
static bool manifestOnly = FALSE;
static char *onlyPkg = NULL;
static bool force = FALSE;
static bool audit=FALSE;
static bool silent = FALSE;

# define COND(x) if (!silent) {x;}

static VOID usage( VOID );


static VOID
usage( VOID )
{
	SIfprintf( stderr,
		ERx( "\n Usage: buildrel [ options ]\n" ) );
	SIfprintf( stderr, ERx( "\n Options:\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -a -- audit the build area (don't generate release area)\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -p product -- build a specific product\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -b filename -- alternate release.dat filename\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -f -- force build of release area (batch mode - ignore missing files)\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -m -- generate the release file without actually moving the files\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -u -- unhide i18n, i18n64, and documentation packages\n\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -l gpl|com|emb|eval -- Override default license file\n\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -s -- Generate spatial objects package \n\n" ) );
	SIfprintf( stderr,
		ERx( "\n    -q -- quiet operation (mostly)\n" ) );
	SIfprintf( stderr, ERx( " Description:\n\n" ) );
	SIfprintf( stderr,
		ERx( "  This program will assemble an %s release in the directory specified by\n" ), SystemDBMSName );
	SIfprintf( stderr,
		ERx( "  FRONT_STAGE, using the release description source files located in the\n" ) );
	SIfprintf( stderr,
		ERx( "  directory specified by %s_MANIFEST_DIR,   The path names of the files to be included\n" ), SystemVarPrefix ); 
	SIfprintf( stderr,
		ERx( "  in the release are specified in the release description source files in\n" ) );
	SIfprintf( stderr,
		ERx( "  terms of ING_SRC, which must also be defined.\n\n" ) );
	SIfprintf( stderr,
		ERx( "  The end result of a successful buildrel will be a release directory\n" ) );
	SIfprintf( stderr,
		ERx( "  containing several sub-directories, which will contain the files to be made\n" ) );
	SIfprintf( stderr,
		ERx( "  into the saveset.\n\n" ) );
	SIfprintf( stderr,
		ERx( "  Because buildrel only assembles packages which have no existing files,\n" ) );
	SIfprintf( stderr,
		ERx( "  the entire contents of the release directory should be removed before\n" ) );
	SIfprintf( stderr,
		ERx( "  the program is run.  Or, if a change is made to the contents or a\n" ) );
	SIfprintf( stderr,
		ERx( "  particular package after the release has been built, buildrel will\n" ) );
	SIfprintf( stderr,
		ERx( "  reassemble only that package if its sub-directory has been removed.\n\n" ) );
	PCexit( FAIL );
}

main(i4 argc, char *argv[])
{
    STATUS rtn;
    char *cp;
    i4 argvx;
    char *onlyPkgName = NULL;
    char *releaseDir;
    LOCATION loc;
    char lobuf[MAX_LOC+1];

    MEadvise( ME_INGRES_ALLOC );  /* We'll use the Ingres malloc library */

    /* set releaseManifest variable */ 
    STcopy( ERx("release.dat"), releaseManifest );

# ifdef NOT_DEFINED

    cp = userName;
    IDname(&cp);		  /* Get current user's ID */

    if (STcompare(cp, REQD_USER_ID))
    {
	SIfprintf(stderr,ERx("You must run this as user 'ingres'\n"));
	PCexit(FAIL);
    }

#endif
    license_type = gpl_lic;

    for (argvx = 1 ; argvx < argc ; argvx++)
    {
	/* break if non-flag was encountered */
	if (argv[argvx][0] != '-')
	    break;

	switch (argv[argvx][1])
	{
	    case 'p':
		if (onlyPkg)
		{
		    SIfprintf(stderr,
			      ERx("Only one -p specification is permitted.\n"));
		    PCexit(FAIL);
		}
		
		switch (argv[argvx][2])
                {
                    case EOS:
                        onlyPkg = argv[++argvx];
                        break;
                    case '=':
                        onlyPkg = &argv[argvx][3];
                        break;
                    default:
                        onlyPkg = &argv[argvx][2];
                        break;
                }
 
		break;

	    case 'u': /* -u -- unhide packages i18n, documentation */
		hide = FALSE;
		break;

	    case 'l':
		/* -l -- next argument should be license type */
		if ( ++argvx >= argc )
		    usage();	

		if (STcompare(argv[argvx], gpl_lic) == 0)
		    license_type = gpl_lic;
		else if (STcompare(argv[argvx], com_lic) == 0)
		    license_type = com_lic;
		else if (STcompare(argv[argvx], emb_lic) == 0)
		    license_type = emb_lic;
		else if (STcompare(argv[argvx], eval_lic) == 0)
		    license_type = eval_lic;
		else
		    license_type = NULL;
		break;

	    case 'f': /* -f -- ignore missing files */
		force = TRUE;
		break;

	    case 'a': /* -a -- generate listing of missing files */
		audit = TRUE;
		break;

	    case 'm': /* -m -- generate manifest file only */
		manifestOnly = TRUE;
		break;

	    case 'q': /* -q -- quiet running, no output */
		silent = TRUE;
		break;

	    case 'b': /* -b filename -- alternate name */
		/* for release.dat */
		STcopy( argv[ ++argvx ], releaseManifest );
		break;

	    case 's': /* -s -- Spatial package */
		spatial = TRUE;
		break;

	    case '?': /* don't give an error on this! */
		/* fall through... */

	    default:
		SIfprintf(stderr,ERx("Invalid switch: %s\n"), argv[argvx]);
		usage();
	}
    }

    if ( license_type == NULL )
    {
	SIfprintf( stderr,
		ERx( "\nbuildrel: the -l option value is invalid.\n\n" ) );
	usage();
	PCexit( FAIL );
    }

    if( audit && force )
    {
	SIfprintf( stderr,
		  ERx( "\nbuildrel: the -a and -f options cannot be used together.\n\n" ) );
	PCexit( FAIL );
    }

    if (manifestOnly && onlyPkg)
    {
	SIfprintf(stderr,ERx("Only one of -m and -p is permitted.\n"));
	PCexit(FAIL);
    }

    NMgtAt(ERx("II_RELEASE_DIR"), &releaseDir);
    if ((!releaseDir || !*releaseDir) && !audit)
    {
	SIfprintf(stderr, ERx("\n%s_RELEASE_DIR must be defined. Use \"buildrel -help\" to get more information.\n\n"),
	    SystemVarPrefix);
	PCexit(FAIL);
    }

    if (releaseDir && *releaseDir)
    {
	STcopy( releaseDir, releaseBuf );
	STcopy( releaseDir, installBuf );
	IPCL_LOfroms( PATH, installBuf, &installLoc  );
	LOfaddpath( &installLoc, ERx( "A" ), &installLoc );
	LOfaddpath( &installLoc, SystemDBMSName, &installLoc );
	LOfaddpath( &installLoc, ERx( "install" ), &installLoc );
	LOtos( &installLoc, &cp );
	STcopy( cp, installDir );

	if( OK != IPCLcreateDir( installDir ) )
	{
	    SIfprintf( stderr, 
		ERx( "Unable to create directory: %s\n" ), installDir );
	    PCexit(FAIL);
	}
    }
    else
    {
	STcopy("NULL", releaseBuf);
    }

    /* Use ING_BUILD to define build_timestamp */
    cp=NULL;
    NMgtAt( ERx( "ING_BUILD" ), &cp );
    if ( cp != NULL )
    {
#define TIMESTAMP_FILE "ING_BUILD:[000000]build.timestamp"
       build_timestamp = MEreqmem(0, STlength( TIMESTAMP_FILE ) + 1, TRUE, &rtn);

       if ( build_timestamp )
          STcopy( TIMESTAMP_FILE, build_timestamp);
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

    /*
    **  Read all data from the packing list file, and store it in internal
    **  data structures...
    */
    SIprintf( ERx( "\nParsing MANIFEST source...\n\n" ) );
    SIflush(stdout);
    rtn = ip_parse( DISTRIBUTION, &distPkgList, &distRelID, &productName,
	TRUE );
    if (rtn != OK)
    {
	IIUGerr(E_ST0110_parse_failed,0,1, ERx("distribution"));
	PCexit(FAIL);
    }

    if (onlyPkg)
    {
	PKGBLK *pkg;

        SCAN_LIST(distPkgList, pkg)
	{
	    if (!STbcompare(onlyPkg, 0, pkg->feature, 0, TRUE))
	    {
		onlyPkgName = pkg->name;
		break;
	    }
	}
	if (onlyPkgName == NULL)
	{
	    SIfprintf(stderr,ERx("Invalid package specified with -p: %s\n"),
		      onlyPkg);
	    PCexit(FAIL);
	}

	rtn = ip_parse( INSTALLATION, &instPkgList, &instRelID,
		&instProductName, TRUE );
	if (rtn != OK)
	{
	    if (rtn == PARSE_FAIL)
		IIUGerr(E_ST0110_parse_failed,0,1, ERx("release"));
	    else
		SIfprintf( stderr, ERx( "Unable to open %s/%s\n" ),
		    installDir, releaseManifest);
	    PCexit(FAIL);
	}
	if ( !STequal(instRelID, distRelID) || 
	     !STequal(productName, instProductName) )
	{
	    SIfprintf(stderr,ERx("Product IDs do not match:\n"));
	    SIfprintf(stderr,
		ERx("  Build area: %s %s\n"),productName,distRelID);
	    SIfprintf(stderr,
		ERx("Release area: %s %s\n\n"),instProductName,instRelID);
	    PCexit(FAIL);
	}

	SIprintf(ERx("**** Updating '%s' package only ****\n\n"),onlyPkgName);
    }
    else
    {
	ip_list_init(&instPkgList);
    }

    SIprintf( ERx( "Release name: %s\n" ), productName );
    SIprintf( ERx( "Release ID: %s\n" ), distRelID );
    SIprintf( ERx( "Release directory: %s\n\n" ), releaseBuf);

    if ( substitute_variables() != OK )
	PCexit(FAIL);

    if (audit)
    {
	SIprintf(
	    ERx("Auditing manifest file; this may take a few seconds...\n\n"));
	SIflush( stdout );
	PCexit( audit_files() );
    }


    if (manifestOnly)
	SIprintf(ERx("**** Updating %s only ****\n\n"), releaseManifest);

    /*
    ** We are going to move any actual files.  Make sure the release
    ** directory exists. If it does not, create it.  We need both the
    ** directory itself and the "install" subdirectory underneath it.
    */
    if (!manifestOnly && !onlyPkg)
    {
	SIprintf(ERx("**** Current contents of the release area will"));
	SIprintf(ERx(" be overwritten ****\n\n"));

	STcopy(releaseBuf, lobuf);
	IPCL_LOfroms(PATH, lobuf, &loc);
	LOtos(&loc, &cp);
	if (OK != LOexist(&loc))
	{
	    SIfprintf(stderr,ERx("Directory does not exist: %s\n"),cp);
	    PCexit(FAIL);
	}
    }

    if (compute_file_info() != OK)
	PCexit(FAIL);

    if (!manifestOnly)
    {
        if (move_files() != OK)
	    PCexit(FAIL);
    }

    if (write_manifest() != OK)
	PCexit(FAIL);

    if (*releaseBuf)
        SIprintf(ERx("\nFinished building release area: %s\n"), releaseBuf);

    if (license_file)
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


VOID ip_display_transient_msg(char *msg)
{
    SIprintf(ERx("%s\n"),msg);
    SIflush(stdout);
}

VOID ip_display_msg(char *msg)
{
    SIprintf(ERx("%s\n"),msg);
    SIflush(stdout);
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
		if ( STequal(fil->name, ERx("%LICENSE%.")) )
		{
		    license_file = MEreqmem(0,
			((STlength(license_type) + 8) * sizeof(char)) + 1,
			TRUE, &rtn);
		    STprintf(license_file, "%s.%s", ERx("LICENSE"),
		        license_type);
		    fil->name = ERx("LICENSE.");
		    fil->build_file = license_file;
		}
	    }
	}
    }
    return OK;
}

static VOID
source_loc(FILBLK *fil, char *sbuf, LOCATION *sloc)
{
    char tbuf[MAX_LOC+1];
    char *cpl, *cpr, *cpx, *sptr, *val;

    if (fil->build_dir && *fil->build_dir)
    {
        if (*fil->build_dir == '-')
            *tbuf = EOS;
        else
	    STcopy(fil->build_dir, tbuf);
    }
    else
	STprintf(tbuf, ERx("II_SYSTEM:%s"), fil->directory);

    /*
    **  Replace any term in the directory name that's surrounded by
    **  single quotes with the value of the symbol.
    */

    for (cpl=tbuf, sptr=sbuf; *cpl; )
    {
        if (!CMcmpcase(cpl,ERx("'")))
        {

            CMnext(cpl);
            if (NULL != (cpx = cpr = STindex(cpl, ERx("'"), 0)))
            {
                CMnext(cpr);   /* Point at what comes after it */
                *cpx = EOS;    /* Terminate string where right quote was */
                
                NMgtAt(cpl, &val);  /* Get the value of the symbol */
                cpl = cpr;     /* We'll restart scan after where rquote was */
                
                if (val)
                {
                    STcopy(val, sptr);
                    sptr += STlength(val);
                }
                
                continue;
	    }
        }
        
        CMcopy(cpl, 1, sptr);
        CMnext(cpl);
        CMnext(sptr);
        continue;
    }

    *sptr = EOS;

    if (fil->build_file && *fil->build_file)
        STcat(sbuf, fil->build_file);
    else
        STcat(sbuf, fil->name);

    if (OK != IPCL_LOfroms(PATH & FILENAME, sbuf, sloc))
    {
	/* NOTE: the following printf() is intentional */
        COND( printf( ERx( "\nIllegal file name: %s\n" ) , sbuf ) );
        SIflush( stdout );
    }
}

/*
**  Note: the following is necessary either because LOfroms doesn't
**  work right, or because I don't understand VMS filename conventions
**  well enough.  Same difference.  Here's the deal: if I have a filename
**  like "foo:[bar]blurfl.zot" and hand that to LOfroms, what I end up
**  with is "exp:[ansion.of.foo][bar]blurfl.zot", which gets handled just
**  fine by all the LO stuff, but backup doesn't seem to like it.  This
**  is probably a flame-thrower approach, but this routine fixes these
**  glitches.
*/

static VOID
source_tweak(char *sname)
{
    char *p;
    bool prevdot = FALSE;

    for (p=sname; *p; p++)
    {
        if (*p == '.')
        {
            prevdot = TRUE;
            continue;
        }
        
	if (*p == ']' && p[1] == '[')
	{
	    if (prevdot)
	    {
	        STcopy(&p[2], p);
	    }
	    else
	    {
                *p = '.';
	        STcopy(&p[2], &p[1]);
	    }
	    
	    break;
	}
	
        prevdot = FALSE;

    }
}
	
static STATUS 
audit_files()
{
    PKGBLK *pkg;
    PRTBLK *prt;
    FILBLK *fil;
    LOCATION loc;
    char lobuf[MAX_LOC+1];
    char *filname;
    i4 notfound = 0;
    LOINFORMATION loinf;
    i4 flagword;
    bool display;

    SCAN_LIST(distPkgList, pkg)
    {
	if( onlyPkg && STbcompare( onlyPkg, 0, pkg->feature, 0, TRUE ) )
	    continue;

	SCAN_LIST(pkg->prtList, prt)
        {
	    SCAN_LIST(prt->filList, fil)
            {
		source_loc(fil, lobuf, &loc);

		LOtos( &loc, &filname );

		/* NOTE: the following call to printf() is intentional. */
		printf( ERx( "\r%-79s" ), filname );

		if( LOexist( &loc ) != OK )
		{
			notfound++;
			fil->exists = FALSE;
		}
	    }
        }
    }

    if( notfound )
    {
	SIprintf( ERx( "\n%d file%s not found") , notfound, notfound==1 ?
		ERx( "" ) : ERx( "s" ) );

	SIprintf( ERx( ".  Display %s? "), (notfound == 1) ? ERx( "it" ) :
	    ERx( "them" ) );
	display = getyorn();

	if (display)
	{
	    SIprintf(ERx("\n"));
	    SCAN_LIST(distPkgList, pkg)
	    {
		if (onlyPkg && STbcompare(onlyPkg, 0, pkg->feature, 0, TRUE))
		    continue;

		SCAN_LIST(pkg->prtList, prt)
		{
		    SCAN_LIST(prt->filList, fil)
		    {
			if ( ! fil->exists )
			{
			    source_loc(fil, lobuf, &loc);
			    LOtos(&loc, &filname);
			    if (!force )
			    	SIprintf(ERx("Not found: %s\n"),filname);
			}
		    }
		}
	    }
	}

	COND(SIprintf(ERx("\n")));
    }
    return (notfound >0 && !force ? FAIL : OK );
}



/*  For each file, calculate the byte size and checksum...  */

static STATUS 
compute_file_info()
{
    PKGBLK *pkg;
    PRTBLK *prt;
    FILBLK *fil;
    LOCATION loc;
    char lobuf[MAX_LOC+1];
    char *filname;
    i4 notfound = 0;
    bool newpkg;
    bool display=FALSE;

    LOINFORMATION loinf;
    i4 flagword;

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

	if (onlyPkg && STbcompare(onlyPkg, 0, pkg->feature, 0, TRUE))
	    continue;

        newpkg = TRUE;

	SCAN_LIST(pkg->prtList, prt)
        {
	    SCAN_LIST(prt->filList, fil)
            {
		if (force)
		{
		    fil->exists = TRUE;
		    fil->size = 1000;
		    fil->checksum = 1000;
		    continue;
		}

                if (newpkg)
		{
		    newpkg = FALSE;
		    COND(SIprintf(ERx("Computing file sizes for package: %s\n"),
                              pkg->name); SIflush(stdout));
		}

		source_loc(fil, lobuf, &loc);

		if ( OK != ip_file_info_comp (&loc, &fil->size,
		    &fil->checksum, FALSE))
		{
		    /* 
		    ** Since ip_file_info_comp() detected an error,
		    ** the file does not exists. So, complain.
		    */

#ifdef i64_vms
                    /*
                    ** BUILDREL attempts to include .obj files in the release
                    ** whilst ip_file_info_comp() calls SIfopen treating the file
                    ** as a text source file. This is fine on an Alpha, where the record format
                    ** is "variable length"; however Itanium object files are in ELF format
                    ** and the record format is "undefined". This results in SIfopen
                    ** returning a fail status. In order to overcome this problem
                    ** we'll just check if the file exists here and if it does then 
                    ** treat it as though we have done a force
                    */

                    if (LOexist( &loc) == OK)
                    {
                        fil->exists = TRUE;
                        fil->size = 1000;
                        fil->checksum = 1000;
                        continue;
                    }

#endif

		    if (force)
		    {
			COND(SIprintf(
				ERx("Ignoring missing file: %s"),fil->name));
		    }
		    else
			notfound++;

		    fil->exists = FALSE;
		}

	    }
        }
    }

    /* 
    **  If there were files that were in the manifest list, but weren't
    **  found, complain now...
    */
    if( notfound )
    {
	if( notfound )
	{
	    SIprintf( ERx( "\n%d file%s not found" ), notfound,
		(notfound == 1) ? ERx( "" ) : ERx( "s" ) );
	}

	/*
	** If we are auditing or it there aren't very many bad files,
	** or we are in no-questions mode, we'll just go ahead & display.
	*/
	display = (notfound <= 10);
	if (!audit && display)
	{
	    SIprintf(ERx(":\n"));
	}
	else
	{
	    SIprintf(ERx(".  Display %s? "), notfound==1?"it":"them");
	    display = getyorn();
	}

	/*
	** If we are going to display the bad files, do it.
	*/
	if (display)
	{
	    SIprintf(ERx("\n"));
	    SCAN_LIST(distPkgList, pkg)
	    {
		/*
		** Hide i18n and documentation package
		*/ 
		if(pkg->hide) continue;

		if (onlyPkg && STbcompare(onlyPkg, 0, pkg->feature, 0, TRUE))
		    continue;

		SCAN_LIST(pkg->prtList, prt)
		{
		    SCAN_LIST(prt->filList, fil)
		    {
			if ( fil->size == -1 )
			{
			    source_loc(fil, lobuf, &loc);
			    LOtos(&loc, &filname);
			    if (!force )
			    	SIprintf(ERx("Not found: %s\n"),filname);
			}
		    }
		}
	    }
	}

	COND(SIprintf(ERx("\n")));
    }

    return (notfound >0 && !force ? FAIL : OK );
}


/*  move_files() -- move all files into the release area.  */

static STATUS
move_files()
{
    PKGBLK *pkg;
    PRTBLK *prt;
    FILBLK *fil;

    bool newpkg;

    LOCATION sourceloc, destloc;
    char sourcebuf[MAX_LOC+1], destbuf[MAX_LOC+1], tempbuf[MAX_LOC+1];
    char *sourcename, *destname;

    char cmd[3 * MAX_LOC];

    i4 nfiles = 0;

    SCAN_LIST(distPkgList, pkg)
    {
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

        if (onlyPkg && STbcompare(onlyPkg, 0, pkg->feature, 0, TRUE))
            continue;

        newpkg = TRUE;

	SCAN_LIST(pkg->prtList, prt)
	{
	    SCAN_LIST(prt->filList, fil)
	    {
		if (fil->exists)
		{
                    if (newpkg)
		    {
		        newpkg = FALSE;
		        SIprintf( ERx( "Moving files for package: %s\n" ),
			    pkg->name );
		        SIflush(stdout);
		    }

		    source_loc(fil, sourcebuf, &sourceloc);
		    LOtos(&sourceloc, &sourcename);
		    source_tweak(sourcename);

		    STcopy( releaseBuf, destbuf );
		    IPCL_LOfroms( PATH, destbuf, &destloc );
		    if( fil->volume == NULL || *fil->volume == EOS )
		    {	
			SIfprintf( stderr, ERx( "\nFatal error: NULL VOLUME for \"%s\"\n\n" ), fil->name );
			PCexit( FAIL );
		    }	
		    LOfaddpath( &destloc, fil->volume, &destloc );
		    if( *fil->directory == '[' )
			    STcopy( &fil->directory[1], tempbuf );
		    else
			    STcopy( fil->directory, tempbuf );
		    if( tempbuf[ STlength( tempbuf ) - 1 ] == ']' )
		            tempbuf[ STlength( tempbuf ) - 1 ] = EOS;
		    LOfaddpath( &destloc, tempbuf, &destloc );
		    LOfstfile( fil->name, &destloc );
		    LOtos( &destloc, &destname );

		    /* Construct the copy command... */

		    STpolycat(4, ERx("BACKUP/REPLACE/IGNORE=INTERLOCK "), 
                                 sourcename, ERx("; "), destname, cmd);

		    COND(SIprintf(ERx("%s\n"),cmd); SIflush(stdout));

	            /*
		    ** Execute the command; if that works, execute another
		    ** command to set the permission on the file.
		    */
                    
		    if (OK != ip_cmdline(cmd, NULL))
		    {
                        fil->exists == FALSE;
		    }
		    else
		    {
			STpolycat(4, ERx("SET PROT=("), fil->permission_sb,
			             ERx(") "), destname, cmd);
			COND(SIprintf(ERx("%s\n"),cmd); SIflush(stdout));
			       
		        _VOID_ ip_cmdline(cmd, NULL);
                    }
                    
		    /* Make some noise occasionally so we don't look dead. */

		    nfiles++;
		    if (!(nfiles % 50))
	            {
		        SIprintf(ERx("  (Number of files moved: %d)\n"),nfiles);
			SIflush(stdout);
	            }
		}
	    }
	}

        /*
	**  Add the package we just finished moving to the "installation"
	**  list.  This is the list which will eventually be converted
	**  into the deliverable manifest file.
	*/

	ip_add_inst_pkg(pkg);
    }

    SIprintf(ERx("\n       Total files moved: %d\n"), nfiles);
    return OK;
}


/*
**  write_manifest() -- write the data structure out to Release.dat in
**  the release area.
*/

static STATUS
write_manifest()
{
    COND( SIprintf( ERx( "\nWriting %s%s...\n" ), installDir,
	releaseManifest ) );

    if ( OK != ip_write_manifest( DISTRIBUTION, installDir, &distPkgList,
		distRelID, productName ) )
    {
	SIfprintf(stderr,ERx("Error writing %s!\n"), releaseManifest);
	PCexit(FAIL);
    }
    return OK;
}


/*  Dummy ip_forms ... we're never in forms mode so don't worry about it. */

VOID
ip_forms(bool yesno)
{
    return;
}

/* getyorn() -- accept a 'yes' or 'no' response from the keyboard.  */

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
	    default:   SIprintf(ERx(" ?? "));
		       continue;
	}
	break;
    }

    for ( ; SIgetc(stdin) != '\n' ; ) 
        ;

    return rtn;
}

/*
**  Dummy...
*/

bool
ip_dep_popup(PKGBLK *pkg1, PKGBLK *pkg2, char *oldvers)
{
    return TRUE;
}

