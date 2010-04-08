/*
** Copyright (c) 1992, 2005 Ingres Corporation
**
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
# include <dbms.h>
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
# include <ip.h>
# include <ipcl.h>
# include <pc.h>

/*
**  Name: iimaninf -- return information from a manifest-format file
**
**  Command-line options:
**
**	-all			List all packages described in the file.
**	-packages		List all VISIBLEPKG packages described.
**	-custom			List all VISIBLE packages described.
**	-version[=<list>]	Display versions of all packages in the
**				(comma-seperated) list, or the version
**				ID of the release if no list specified.
**	-output=<file>		Direct output to the named file.
**	-describe=<list>	Display descriptions of packages in the list.
**	-resolve=<list>		Display all files which need to be installed
**				for the specified list of packages.
**	-setup=<list>		Display all setup programs which need to
**				be run for the specified list of packages.
**	-preload=<list>		Display all pre-load programs which need
**				to be run for the specified list of packages.
**	-installation		Read the installation manifest.
**	-location=<path>	Specify the location of the file to be parsed.
**	-authorization=<str>	Validate the specified authorization string.
**	-directories=<list>	Display a list of all directories which will
**				be used by files associated with the specified
**				packages.
**	-unique=<list>		Display all files which need to be installed
**				for the specified list of packages. Similar to
**				the 'resolve' option except it also displays
**				the volume.
**	-permission=<list>	Displays directory, file, and file protection.
**
**  Command-line flags may be abbreviated to the smallest unique left
**  substring.  In the above descriptions, <list> represents a comma-separated
**  list of package featurenames, with no embedded whitespace.
**
**  History:
**	xx-xxx-92 (jonb)
**		Created.
**	19-mar-93 (jonb)
**		Added -directories flag.
**      27-may-93 (kellyp)
**		Corrected -authorization option to check the string passed
**		with the option rather than the value of II_AUTHORIZATION.
**	8-jun-93 (kellyp)
**		Added history for previous change.
**	13-jul-93 (tyler)
**		Sweeping changes to to make the manifest language portable.
**		Changed MAX_TXT to MAX_MANIFEST_LINE.
**	14-jul-93 (tyler)
**		Replaced changes lost from r65 lire codeline.
**	22-jul-93 (kellyp)
**		Corrected bracket mismatch in do_flags()
**	27-jul-93 (lauraw)
**		Removed asterisks from the middle of the NEEDLIBS hint.
**	19-oct-93 (tyler)	
**		Added GVLIB to NEEDLIBS and added GLOBALREL declaration
**		of global variable Version[]; 
**	01-nov-93 (kellyp)	
**		Added -unique option.
**	11-nov-93 (tyler)
**		Ported to IP Compatibility Layer.	
**	18-nov-93 (kellyp)
**		Added -permission option.
**	01-dec-93 (tyler)
**		Don't issue an error message when install.dat is not found
**		unless it's necessary.
**	10-dec-93 (tyler)
**		Call ip_init_installdir() before ip_init_authorization().
**	05-jan-94 (tyler)
**		Changed interface/shortened name of ip_resolve_dependencies().
**	22-feb-94 (joplin)
**		Set batchMode flag earlier to avoid errors in initialization steps.
**	13-jun-95 (forky01)
**		Change check for ip_licensed to use temp var ptr to allow recur-
**		sion rather than moving along real distribution list. (69294)
**	11-nov-95 (hanch04)
**		Added ip_is_visiblepkg and eXpress for new options to ingbuild.
**	19-jan-95 (hanch04)
**		RELEASE_MANIFEST is now releaseManifest
**	21-jan-96 (dougb)
**		To make a streamlined VMS installation work, rename "-packages"
**		option to "-all" and add "-packages" and "-custom" options
**		which display either VISIBLEPKG (-packages) or VISIBLE
**		(-custom) packages.
**	01-feb-96 (emmag)
**		Fix dougb's integration which didn't compile.
**	04-nov-1996 (kamal03)
**		The fix made into module ipparse.y (change 420188) is causing
**		the VAX C Compiler to finish with errors. To fix the problem,
**		reverse change 420188 and change this module: initialize
**		GLOBALDEF variable "batchOutf" if it's zero prior to use.
**	19-aug-1998 (kinte01)
**		Modified to use ip_init_calicense if INGRESII defined and
**		modifed the -all,-packages, & -custom options to check the
**		output of ip_verify_auth instead of relying on the value
**		of authStringEnv to ensure correct behavior with new
**		licensing
**      11-mar-199 (hanch04)
**              Added ignoreChecksum flag to ignore file mismatch errors.
**	24-apr-2000 (somsa01)
**		Updated MING hint for program name to account for different
**		products.
**	21-sep-2001 (gupsh01)
**		added GLOBALDEF for mkresponse mode.
**	19-Oct-2001 (gupsh01)
**		added GLOBALDEF for exresponse mode.
**	05-Nov-2001 (gupsh01)
**		added GLOBALDEF for respfilename.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	31-may-2005 (abbjo03)
**	    Eliminate the Licensed? column.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of ipcl.h and pc.h to eliminate gcc 4.3 warnings.
*/

/*
PROGRAM =	(PROG1PRFX)maninf
**
NEEDLIBS =	INSTALLLIB \
		UFLIB RUNSYSLIB RUNTIMELIB FDLIB FTLIB FEDSLIB \
		UILIB LIBQSYSLIB LIBQLIB UGLIB FMTLIB AFELIB \
		LIBQGCALIB SQLCALIB GCFLIB ADFLIB CUFLIB \
		SQLCALIB COMPATLIB MALLOCLIB GVLIB
**
UNDEFS =	II_copyright
**
DEST =		utility
*/

GLOBALDEF char *releaseDir;
GLOBALDEF char *relID;
GLOBALDEF char *productName;
GLOBALDEF char *instProductName;
GLOBALDEF char distribDev[1];
GLOBALDEF char distribHost[1];
GLOBALDEF i4  ver_err_cnt;
GLOBALDEF bool batchMode;
GLOBALDEF bool mkresponse = FALSE;
GLOBALDEF bool exresponse = FALSE;
GLOBALDEF char *respfilename = NULL;
GLOBALDEF bool ignoreChecksum;
GLOBALDEF bool eXpress;
GLOBALDEF bool noSetup;	
GLOBALDEF LIST distPkgList;
GLOBALDEF LIST instPkgList;
GLOBALDEF char installDir[ MAX_LOC + 1 ];
GLOBALDEF char releaseManifest[ 32 ];
GLOBALDEF char authString[ MAX_MANIFEST_LINE + 1 ];
GLOBALDEF bool authStringEnv;
GLOBALDEF bool pmode;
GLOBALREF FILE *batchOutf;

static void do_flags();
static char *argval, *featlist;
static bool apply_list( bool (*proc) ( char *, bool ) );
static bool proc_resolve( char *, bool );
static bool proc_version( char *, bool );
static bool proc_describe( char *, bool );
static char *manifestDir = NULL;
static bool installManifest = FALSE;

void
usage()
{
	SIfprintf( stderr, ERget( E_ST017F_usage_1 ) );
	SIfprintf( stderr, ERget( E_ST0180_usage_2 ) );
	PCexit( FAIL );
}

static void
do_parse()
{
	char *dummy;

	/*
	**  Read all data from the packing list file, and store it in internal
	**  data structures...
	*/

	if( installManifest )
	{
		if( ip_parse( INSTALLATION, &distPkgList, &relID,
			&productName, TRUE ) != OK )
		{
			PCexit( FAIL );
		}
	}
	else
	{
		if( ip_parse( DISTRIBUTION, &distPkgList, &relID,
			&productName, TRUE ) != OK )
		{
			PCexit( FAIL );
		}

		(void) ip_parse( INSTALLATION, &instPkgList, &dummy, &dummy,
			FALSE );
	}
}

main( i4  argc, char *argv[] )
{
	i4 argvx;

	MEadvise( ME_INGRES_ALLOC );

	batchMode = TRUE;
	batchOutf = stdout;

	if (argc < 2)
		usage();
	STcopy( ERx("release.dat") , releaseManifest );
	ip_init_installdir();
        ip_environment();

	do_flags( argc, argv );

	IPCLclearTmpEnv();

	PCexit( OK );
}


typedef struct _option
{
	char *op_text;
	uchar op_id;

} OPTION;

# define opNONE		0
# define opPACKAGES	1
# define opVERSION	2
# define opOUTPUT	3
# define opDESCRIBE	4
# define opRESOLVE	5
# define opSETUP	6
# define opINST		7
# define opLOCATION	8
# define opPREXFERP	9
# define opDIR		10
# define opUNIQUE	11
# define opPERMISSION	12
# define opALL		13
# define opCUSTOM	14

OPTION option[] =
{
	ERx("packages"), opPACKAGES,
	ERx("version"), opVERSION,
	ERx("output"), opOUTPUT,
	ERx("describe"), opDESCRIBE,
	ERx("resolve"), opRESOLVE,
	ERx("setup"), opSETUP,
	ERx("preload"), opPREXFERP,
	ERx("installation"), opINST,
	ERx("location"), opLOCATION,
	ERx("directories"), opDIR,
	ERx("unique"), opUNIQUE,
	ERx("permission"), opPERMISSION,
	ERx("all"), opALL,
	ERx("custom"), opCUSTOM
};

# define NOPTION ( sizeof( option ) / sizeof( option[ 0] ) )

static uchar
lookup( char *opname )
{
	i4 opn;
	uchar rtn = opNONE;

	if( NULL != (argval = STindex( opname, ERx( "=" ), 0 ) ) )
	{
		*argval = EOS;
		CMnext( argval );
		if( *argval == EOS )
			return opNONE;
	}

	for( opn = 0; opn < NOPTION; opn++ )
	{
		if( !STbcompare( opname, 99, option[ opn ].op_text, 99,
			FALSE) )
		{
			if( rtn == opNONE )
				rtn = option[ opn ].op_id;
			else
				return opNONE;  /* Not unique, apparently. */
		}
	}

	return rtn;
}

static void
do_flags( i4  argc, char *argv[] )
{
	i4 argvx;
	PKGBLK *pkg;
	PRTBLK *prt;
	FILBLK *fil;
	uchar opid;
	uchar option = opNONE;
	LOCATION outloc;
	LIST dirList;
	LISTELE *lp;
	char *dirp;

	if (argc < 2)
		return;

        if (batchOutf == 0)
		batchOutf = stdout;


	for( argvx = 1; argvx < argc; argvx++ )
	{
		if( argv[ argvx ][ 0 ] != '-' )
			break;

		switch (opid = lookup(&argv[argvx][1]))
		{
			case opOUTPUT:

				if (argval == NULL)
					usage();

   				if( LOfroms( FILENAME & PATH, argval,
					&outloc ) != OK || SIfopen( &outloc,
					ERx( "w" ), SI_TXT, MAX_MANIFEST_LINE,
					&batchOutf ) != OK )
				{
					IIUGerr( E_ST017D_cant_open_file,
						0, 1, argval );
					usage();
				}
				break;
	
			case opLOCATION:

				if (argval == NULL)
					usage();
				manifestDir = argval;
				break;

			case opINST:

				installManifest = TRUE;
				break;

			case opRESOLVE:
			case opUNIQUE:
			case opPERMISSION:
			case opDIR:
			case opSETUP:
			case opPREXFERP:
			case opVERSION:
			case opDESCRIBE:

				if (option != opNONE || argval == NULL)
					usage();
				option = opid;
				featlist = argval;
				break;

			case opALL:	/* Everything. */
			case opCUSTOM:	/* Custom visible packages. */
			case opPACKAGES:/* Express visible packages. */
				if (option == opNONE)
				{
					featlist = argval;
					option = opid;
					break;
				}
				/* Fall through */

			default:
				usage();
		}
	}

	/* Too many non-flags at the end of the line. */
	if( argvx < (argc - 1) ) 
		usage();

	switch (option)
	{
		case opNONE:

			usage();
	}

	do_parse();

	switch (option)
	{
		case opALL:	/* Used during VMS build process. */
		case opCUSTOM:	/* Used during old-style VMS installation. */
		case opPACKAGES:/* Used during streamlined/express VMS inst. */
			SIfprintf( batchOutf,
				ERget( S_ST0179_pkg_list_header ) );

			for( lp = distPkgList.head; lp != NULL; lp = lp->cdr )
			{
				pkg = (PKGBLK *) lp->car;

				if (( opPACKAGES != option
				     && ip_is_visible( pkg ))
				    || ( opCUSTOM != option
					&& ip_is_visiblepkg( pkg )))
				{
					SIfprintf( batchOutf,
						ERx("\t%-16s  %-30s\n"),
						pkg->feature,pkg->name );
				}
			}
			return;

		case opVERSION:

			if( relID == NULL )
			{
				IIUGerr( E_ST017B_not_present, 0, 1,
					ERx( "INGRES" ) );
				usage();
			}
			if( featlist == NULL )
			{
				SIfprintf( batchOutf, ERx( "%s\n" ), relID );
				return;
			}

			if ( !apply_list( proc_version ) )
				usage();
			return;

		case opRESOLVE:
		case opUNIQUE:
		case opPERMISSION:
		case opDIR:
		case opSETUP:
		case opPREXFERP:

			SCAN_LIST( distPkgList, pkg )
			{
				pkg->selected = NOT_SELECTED;
			}
			if( !apply_list( proc_resolve ) )
				usage();
			break;

		case opDESCRIBE:
			(void) apply_list( proc_describe );
			return;

		default:
			usage();
	}

	SCAN_LIST( distPkgList, pkg )
	{
		if( pkg->selected == SELECTED )
			break;
	}

	if( pkg == NULL )
		usage();

	if( option == opDIR )
		ip_list_init( &dirList );

	switch( option )
	{
		case opUNIQUE:
		case opPERMISSION:
		case opRESOLVE:
		case opDIR:
		case opSETUP:
		case opPREXFERP:

			if( !ip_resolve( NULL ) )
				usage();

			SCAN_LIST( distPkgList, pkg )
			{
				if (pkg->selected != NOT_SELECTED)
				{
					switch (option)
					{

					case opSETUP:

						(void) ip_setup( pkg,
							_DISPLAY);
						break;
				
					case opPREXFERP:

						(void) ip_preload(
						pkg, _DISPLAY );
						break;
		
					case opDIR:

						SCAN_LIST(pkg->prtList,
							prt)
						{

						SCAN_LIST(prt->filList,
							fil)
						{

						SCAN_LIST(dirList,
							dirp)
						{

						if( STequal( dirp,
							fil->directory)
							)
						{
							break;
						}

						}

						if( dirp == NULL )
						{
							ip_list_append(
								&dirList, 
						  	(PTR) ip_stash_string(
								fil->directory
								) );
						}

						}

						}
						break;

					case opRESOLVE:

						SCAN_LIST( pkg->prtList,
							prt )
						{

						SCAN_LIST(prt->filList,
							fil)
						{
							SIfprintf(
								batchOutf,
								ERx("%-40s  %s\n"),
						  		fil->directory,
								fil->name);
						}

						}

						break;

					case opUNIQUE:

						SCAN_LIST( pkg->prtList,
							prt )
						{

						SCAN_LIST(prt->filList,
							fil)
						{
							SIfprintf(
								batchOutf,
								ERx("%s %-40s  %s\n"),
						  		fil->volume,
						  		fil->directory,
								fil->name);
						}

						}

						break;

					case opPERMISSION:

						SCAN_LIST( pkg->prtList,
							prt )
						{

						SCAN_LIST(prt->filList,
							fil)
						{
						SIfprintf(
							batchOutf,
							ERx("%s %s %s\n"),
						  	fil->directory,
							fil->name,
							fil->permission_sb);
						}

						}

						break;
					}
				}
			}
	}

	if( option == opDIR )
	{
		SCAN_LIST( dirList, dirp )
		SIfprintf( batchOutf, ERx( "%s\n" ), dirp );
	}
}

static bool
apply_list( bool (*proc) ( char *, bool ) )
{
	char *featname, *featnext;
	bool only;
	char savechar;
	bool rtn = TRUE;

	only = (NULL == STindex( featlist, ERx( "," ), 0 ) );

	for( featname = featlist; *featname; )
	{
		featnext = STindex( featname, ERx( "," ), 0 );
		if( featnext != NULL )
		{
			savechar = *featnext;
			*featnext = EOS;
		}

		if( !(*proc)( featname, only ) )
			rtn = FALSE;

		if( featnext == NULL )
			break;

		*(featname = featnext) = savechar;
		CMnext( featname );
	}
	return rtn;
}

static bool
proc_version( char *featname, bool only )
{
	PKGBLK *pkg;

        if (batchOutf == 0)
		batchOutf = stdout;

	SCAN_LIST( distPkgList, pkg )
	{
		if( !STbcompare( featname, 0, pkg->feature, 0, TRUE ) )
		{
			if( !only )
				SIfprintf( batchOutf, ERx( "%s " ), pkg->feature );

			SIfprintf( batchOutf, ERx( "%s\n" ), pkg->version );
			return( TRUE );
		}
	}

	IIUGerr( E_ST017B_not_present, 0, 1, featname );
	return( FALSE );
}

static bool
proc_describe( char *featname, bool only )
{
	PKGBLK *pkg = ip_find_feature( featname, DISTRIBUTION );

	if( pkg != NULL )
	ip_describe( pkg->name );

	return( TRUE );
}

static bool
proc_resolve( char *featname, bool only )
{
	PKGBLK *pkg;

	SCAN_LIST( distPkgList, pkg )
	{
		if( !STbcompare( featname, 0, pkg->feature, 0, TRUE ) )
		{
			pkg->selected = SELECTED;
			return( TRUE );
		}
	}

	IIUGerr( E_ST017A_bad_featname, 0, 1, featname );
	return( FALSE );
}

/*  Dummy ip_forms ... we're never in forms mode so don't worry about it. */

void
ip_forms( bool yesno )
{
	return;
}

/*  Dummy message-display routines.  In forms mode, these use popups; here
	we just display 'em.
*/

void ip_display_msg( char *msg )
{

        if (batchOutf == 0)
		batchOutf = stdout;

	SIfprintf( batchOutf, ERx( "%s\n" ) , msg );
}

void ip_display_transient_msg( char *msg )
{
	ip_display_msg( msg );
}

/*
**  pkg1 has a dependency on pkg2, but pkg2 is not installed and has not been
**  selected for installation.  We'll just go ahead and install pkg2.
*/

bool
ip_dep_popup( PKGBLK *pkg1, PKGBLK *pkg2, char *oldvers )
{
	pkg2->selected = SELECTED;
	return( TRUE );
}
