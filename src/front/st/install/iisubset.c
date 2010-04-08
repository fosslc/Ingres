/*
** Copyright (c) 2001, 2005 Ingres Corporation
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
# include <pc.h>
# include <ipcl.h>

/*
**  Name: iisubset -- return information from a manifest-format file
**
**  Command-line options:
**
**	-all			List all packages described in the file.
**      -packages               List all VISIBLEPKG packages described.
**      -custom                 List all VISIBLE packages described.
**      -output=<file>          Direct output to the named file.
**      -describe=<list>        Display descriptions of packages in the list.
**	-resolve=<list>		Display all files which need to be installed
**				for the specified list of packages.
**                              Files for all NEEDed packages will also
**                              be displayed.
**	-nolinks                Stops -resolve from listing link files.
**
**  Command-line flags may be abbreviated to the smallest unique left
**  substring.  In the above descriptions, <list> represents a comma-separated
**  list of package featurenames, with no embedded whitespace.
**
**  History:
**	12-Jan-2001 (hanal04) SIR 102895
**		Created using iimaninf as a template.
**	30-Jan-2001 (hanch04)
**	    Replace nat with i4
**	13-Feb-2001 (ahaal01)
**	    Changed "iisubset" to "(PROG1PRFX)subset".
**	05-Nov-2001 (gupsh01)
**	    Added GLOBALDEF for mkresponse, exresponse, respfilename.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	31-may-2005 (abbjo03)
**	    Eliminate the Licensed? column.
**	12-Sep-2007 (kiria01) b119056
**	    Add -nolinks option to drop LINK files from the
**	    resolved list.
**	24-Nov-2009 (frima01) Bug 122490
**	    Added include of pc.h and ipcl.h to eliminate gcc 4.3 warnings.
*/

/*
PROGRAM =	(PROG1PRFX)subset
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
GLOBALDEF i4 ver_err_cnt;
GLOBALDEF bool batchMode;
GLOBALDEF bool mkresponse = FALSE;
GLOBALDEF bool exresponse = FALSE;
GLOBALDEF char *respfilename = NULL;
GLOBALDEF bool ignoreChecksum;
GLOBALDEF bool eXpress;
GLOBALDEF bool noSetup;	
GLOBALDEF LIST distPkgList;
GLOBALDEF LIST needPkgList;
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
static void resolve_needed(PKGBLK *);
static char *manifestDir = NULL;
static bool installManifest = FALSE;
static bool droplinks = FALSE;

void
usage()
{
        SIfprintf( stderr, ERget( E_ST0187_usage_1 ) );
        SIfprintf( stderr, ERget( E_ST0188_usage_2 ) );
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

static void
resolve_needed(PKGBLK *pkg)
{
    PKGBLK *sub_pkg;
    PRTBLK *prt;
    FILBLK *fil;
    LISTELE *lp;
    REFBLK *ref;

    /* Rrecursively resolve NEEDED packages */
    for( lp = pkg->refList.head; lp != NULL; lp = lp->cdr )
    {
        ref = (REFBLK *) lp->car;
 
        if( ((sub_pkg = ip_find_package( ref->name, DISTRIBUTION ))
                != NULL ) && (ref->type != PREFER) )
        {
            resolve_needed(sub_pkg);
        }
    }


    SIfprintf(batchOutf, ERx("PACKAGE %s\n"), pkg->feature);

    SCAN_LIST( pkg->prtList, prt)
    {
        SCAN_LIST(prt->filList, fil)
        {
	    if (!fil->link || !droplinks)
                SIfprintf(batchOutf, ERx("%-40s  %s\n"),
                                 fil->directory, fil->name);
        }
    }
}

main( i4 argc, char *argv[] )
{
	i4 argvx;

	MEadvise( ME_INGRES_ALLOC );

	batchMode = TRUE;
	batchOutf = stdout;

	STcopy( ERx("release.dat") , releaseManifest );
	ip_init_installdir();
  
        ip_environment();

	if (argc < 2)
		usage();

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
# define opOUTPUT	2
# define opDESCRIBE	3
# define opRESOLVE	4
# define opLOCATION	5
# define opPREXFERP	6
# define opALL		7
# define opCUSTOM	8
# define opNOLINKS	9

OPTION option[] =
{
	ERx("packages"), opPACKAGES,
	ERx("output"), opOUTPUT,
	ERx("describe"), opDESCRIBE,
	ERx("resolve"), opRESOLVE,
	ERx("preload"), opPREXFERP,
	ERx("location"), opLOCATION,
	ERx("all"), opALL,
	ERx("custom"), opCUSTOM,
	ERx("nolinks"), opNOLINKS
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
do_flags( i4 argc, char *argv[] )
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
			case opNOLINKS:	/* Don't include links */
				droplinks = TRUE;
				break;

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

			case opRESOLVE:
			case opPREXFERP:
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

		case opRESOLVE:
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

	switch( option )
	{
		case opPREXFERP:

			if( !ip_resolve( NULL ) )
				usage();

			SCAN_LIST( distPkgList, pkg )
			{
				if (pkg->selected != NOT_SELECTED)
				{
					(void) ip_preload(
					pkg, _DISPLAY );
					break;
                                }
			}
                        break;

		case opRESOLVE:

                        /* The list pointers in distPkgList will
                        ** be reset in ip_find_package()
                        ** scan a copy list, for the packages requested.
                        */
			needPkgList = distPkgList;
			SCAN_LIST( needPkgList, pkg )
			{
                                if (pkg->selected == SELECTED)
                                {
                                        /* Called for each selected pkg */
                                        resolve_needed(pkg);
				}
			}
                        break;
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
