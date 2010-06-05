/*
**  Copyright (c) 2004 Ingres Corporation
**
**  Name: iimklog.c -- contains main() and other function(s) for a program
**	which creates or erases an INGRES transaction log file.
**
**  Usage:
**	iimklog [ -erase ] [ -dual ] [ -setres ]
**
**  Description:
**	iimklog make DI calls to create or erase (if -erase was passed)
**	the primary or secondary (if -dual was passed) transaction log.
**	Maintains a primitive completion "thermometer" on standard output
**	during the operation. 
**
**	The -setres option tells iimklog to set the ii.$.rcp.file.kbytes
**	resource to the (total) transaction log size.  An upgrade
**	install might do this, since it's inconvenient for the calling
**	script to figure out the size total, and we know it here.
**	(Obviously, -setres would be used with -erase.)
**
**  Exit Status:
**	OK	transaction log successfully created/erased.
**	FAIL	unable to create/erase transaction log.
**
**  History:
**	14-dec-92 (tyler)
**		Created.
**	16-feb-93 (tyler)
**		Added call to CSset_sid() required since the last DBMS
**		integration.  Changed usage of DIalloc() and added call
**		to DIforce() to make it work on VMS.
**	1-mar-93 (tyler)
**		Minor changes to displayed output.
**	5-apr-93 (tyler)
**		Added call to libc printf() (naughty, naughty) on VMS
**		to work around limitation of VMS implementation of SI
**		which prevents characters not followed by a newline
**		from being flushed to stdout.  Without this change, the
**		completion thermometer for operations on transaction log
**		files can't be displayed. 
**	24-may-93 (tyler)
**		Improved error messages.
**	21-jun-93 (tyler)
**		Changed call to SIfprintf() which was missing the FILE
**		pointer argument to SIprintf().
**	23-jul-93 (tyler)
**		PMerror() is now passed to PMload() for better error
**		reporting.  Added UTILLIB to NEEDSLIBS.  Changed -erase
**		option to use LOinfo() to determine file size.  Moved
**		code which manipulates transaction logs to util.c;
**		inserted calls to write_transaction_log().  Replaced
**		embedded strings with message lookups.
**	19-oct-93 (tyler)
**		Check whether csinstall (Unix only) succeeds and issue
**		an error if it doesn't.  Call PMhost() instead of
**		GChostname().
**	22-feb-94 (tyler)
**		Fixed previous history which had botched date. 
**	18-jul-95 (duursma)
**		On VMS, now use the new NMgtIngAt() to translate II_DUAL_LOG
**		and II_DUAL_LOG_NAME.  For II_LOG_FILE and II_LOG_FILE_NAME,
**		still use NMgtAt(), so we can't remove the #ifdef VMS.
**		This is part of fix for bug #69872
**	26-sep-1995 (sweeney)
**		Call TRset_file to allow TRdisplay from called functions.
**	09-apr-1997 (hanch04)
**		Delete space at the end of ming hint
**	03-nov-1997 (canor01)
**		Use kbytes instead of bytes for file size so we can support
**		files greater than 2GB in size.
**	27-Jan-1998 (jenjo02)
**	    Partitioned log files project:
**	    Integrated balst01's changes to cope with multiple log 
**	    partitions gleaned from the PM file.
**	24-aug-1998 (hanch04)
**	    Partitioned log files project:
**	    Change from log file names to log file locations.
**	31-aug-1998 (hanch04)
**	    Removed code for using symbol table.  Log symbols hould only be
**	    defined in config.dat
**	11-sep-1998 (hanch04)
**	    fix typo.
**	05-sep-2002 (somsa01)
**	    For the 32/64 bit build,
**	    call PCexeclp64 to execute the 64 bit version
**	    if II_LP64_ENABLED is true.
**	23-jul-2001 (devjo01)
**	    Re-redo way log file name is decorated when using clusters.
**	11-jun-2002 (devjo01)
**	    Limit log file name to 36 characters.
**	11-sep-2002 (somsa01)
**	    Prior change is not for Windows.
**      25-Sep-2002 (hanch04)
**          PCexeclp64 needs to only be called from the 32-bit version of
**          the exe only in a 32/64 bit build environment.
**	25-sep-2002 (devjo01)
**	    NUMA clusters support.
**	2-Sep-2004 (schka24)
**	    Add -print so that we can set config file.kbytes properly.
**	25-Jul-2007 (drivi01)
**	    Added routines for testing if the program is being
**	    executed on Vista and if user has sufficient
**	    privileges to run this program.  Program will
**	    result in error if elevation is required.
**	22-Jun-2009 (kschendel) SIR 122138
**	    Hybrid add-on symbol changed, fix here.
**      14-Jan-2010 (horda03) Bug 123153
**          Allow iimklog to be used to create TX log files on any node
**          in a clustered Ingres installation (whether the cluster be
**          NUMA or otherwise).
*/

/*
PROGRAM = 	iimklog
**
NEEDLIBS = 	UTILLIB COMPATLIB MALLOCLIB
**
UNDEFS = 	II_copyright
**
DEST =		utility
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <nm.h>
# include <si.h>
# include <st.h>
# include <di.h>
# include <lo.h>
# include <ep.h>
# include <er.h>
# include <me.h>
# include <pc.h>
# include <cv.h>
# include <gc.h>
# include <cs.h>
# include <pm.h>
# include <tr.h>
# include <lk.h>
# include <cx.h>
# include <lg.h>
# include <util.h>
# include <simacro.h>
# include <erst.h>

static  char	Affine_Arg[16] = { '\0', };
static char bad_usage[] = "Usage: iimklog [-dual] [-erase] [-setres] [-rad <radid> | -node <nodename>]";

static	void	call_ipcclean()
{
# ifdef UNIX
    CL_ERR_DESC cl_err;
    char	cmd[128];

    STprintf( cmd, ERx( "ipcclean%s" ), Affine_Arg );

    (void) PCcmdline((LOCATION *) NULL, cmd, PC_WAIT,
	    (LOCATION *) NULL, &cl_err );
# endif /* UNIX */
}


static void call_iisetres(i4 size, PM_CONTEXT *context)
{
    CL_ERR_DESC cl_err;
    char cmd[256];

    STprintf(cmd, "iisetres %s %u",
		PMmExpandRequest(context,"$.$.rcp.file.kbytes"), size);
    (void) PCcmdline((LOCATION *) NULL, cmd, PC_WAIT,
	    (LOCATION *) NULL, &cl_err );
}


static void
cleanup( void )
{
    call_ipcclean();
    PCexit( FAIL );
}

static void
init_graph( bool create, i4 size )
{
    if ( create )
    {
	PRINT( ERx( "\n" ) );
	F_PRINT( ERget( S_ST0446_creating_log ), size );
	PRINT( ERx( "\n" ) );
    }
    else
    {
	PRINT( ERx( "\n" ) );
	F_PRINT( ERget( S_ST0447_erasing_log ), size );
	PRINT( ERx( "\n" ) );
    }

    PRINT( ERx( "\n0%%          25%%         50%%         75%%         100%%\n" ) );

    SIflush( stdout );

}

static void
update_graph( void )
{
# ifdef VMS
    printf( "|" );
# else /* VMS */
    PRINT( "|" );
# endif /* VMS */
}

static void
message( char *msg )
{
    TRdisplay( "\niimklog: %s\n\n", msg );
}

main( argc, argv )
int argc;
char **argv;
{
    PM_CONTEXT		*pmcontext;
    char 		*path_sym;
    char		*log_dir[LG_MAX_FILE];
    char		*log_file[LG_MAX_FILE];
    char		*path_resource;
    char		*file_resource;
    char		*basefilename;
    char		resourcebuf[MAX_LOC + 1];
    char		fileext[CX_MAX_NODE_NAME_LEN+2];
    i4			i;
    i4			size;
    LOCATION 		loc;
    char 		locbuf[ MAX_LOC + 1 ];
    bool 		create = TRUE;
    bool		dual = FALSE;
    bool		setres = FALSE;
    CL_ERR_DESC 	cl_err;
#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)
    char		*lp64enabled;
#endif
    char		node_name_buf[CX_MAX_NODE_NAME_LEN + 2]; 
    char		cmd[64];

    MEadvise( ME_INGRES_ALLOC );

#if defined(conf_BUILD_ARCH_32_64) && defined(BUILD_ARCH32)

    /*
    ** Try to exec the 64-bit version
    */
    NMgtAt("II_LP64_ENABLED", &lp64enabled);
    if ( (lp64enabled && *lp64enabled) &&
       ( !(STbcompare(lp64enabled, 0, "ON", 0, TRUE)) ||
	 !(STbcompare(lp64enabled, 0, "TRUE", 0, TRUE))))
    {
	PCexeclp64(argc, argv);
    }
#endif  /* hybrid */

    /* for TRdisplay from the CL DIO routines */

    TRset_file(TR_T_OPEN, TR_OUTPUT, TR_L_OUTPUT, &cl_err);
    /* XXX do something if this call just failed */

    /* 
    ** Check if this is Vista, and if user has sufficient privileges 
    ** to execute.
    */
    ELEVATION_REQUIRED();

    if ( OK != CXget_context( &argc, argv, CX_NSC_STD_CLUSTER, 
			      node_name_buf, CX_MAX_NODE_NAME_LEN + 1 ) )
    {
	ERROR( bad_usage );
    }
    else
    {
	/* argc, argv was adjusted to consume CX args */
	for ( i = 1; i < argc; i++ )
	{
	    if ( STcompare( argv[i], ERx( "-dual" ) ) == 0 )
		dual = TRUE;
	    else
	    if ( STcompare( argv[i], ERx( "-erase" ) ) == 0 )
		create = FALSE;
	    else
	    if ( STcompare( argv[i], ERx( "-setres" ) ) == 0 )
		setres = TRUE;
	    else
		ERROR( bad_usage );
	}
    }

    PMmInit( &pmcontext );

    if ( PMmLoad( pmcontext, NULL, PMerror ) != OK )
	PCexit( FAIL );

    PMmSetDefault( pmcontext, 0, SystemCfgPrefix );
    PMmSetDefault( pmcontext, 1, node_name_buf );

# ifdef UNIX
    {
	i4	rad;

	if ( 0 != ( rad = CXnuma_cluster_rad() ) )
	{
	    STprintf( Affine_Arg, ERx( " -rad %d" ), rad );
	}
	STprintf( cmd, ERx( "csinstall%s >/dev/null" ), Affine_Arg );	
	if ( PCcmdline((LOCATION *) NULL, cmd,
		PC_WAIT, (LOCATION *) NULL, &cl_err ) != OK )
	{
	    PRINT( ERget( S_ST039F_csinstall_failed_1 ) );
	    PCexit( FAIL );
	}
    }
# endif /* UNIX */

    /* Extract log file names/paths from the PM file */
    if (dual)
    {
	path_resource = ERx( "dual_log" );
	file_resource = ERx( "$.$.rcp.log.dual_log_name" );
	path_sym = ERx( "II_DUAL_LOG" );
    }
    else
    {
	path_resource = ERx( "log_file" );
	file_resource = ERx( "$.$.rcp.log.log_file_name" );
	path_sym = ERx( "II_LOG_FILE" );
    }

    if ( OK != PMmGet( pmcontext, file_resource, &basefilename) ||
	 '\0' == *basefilename )
	basefilename = NULL;

    /* If running in a cluster, log name gets "decorated" with node name. */
    fileext[0] = '\0';
    if ( CXcluster_configured() )
    {
	(void)CXdecorate_file_name( fileext, node_name_buf );
    }

    i = 0;
    if ( basefilename )
    {
        while ( i < LG_MAX_FILE )
	{
	    STprintf(locbuf, ERx( "%s.l%02d%s" ),
	     basefilename, i + 1, fileext);	

	    /* Make sure filename is not too long. */
	    locbuf[DI_FILENAME_MAX] = '\0';
	    log_file[i] = STalloc( locbuf );

	    STprintf( resourcebuf, ERx( "$.$.rcp.log.%s_%d" ),
                path_resource, i + 1);

	    if (PMmGet( pmcontext, resourcebuf, &log_dir[i]) != OK ||
		!log_dir[i] )
	    {
		break;
	    }

	    /* do LOCATION manipulation to get full path of tx log */
	    STcopy( log_dir[i], locbuf );
	    LOfroms( PATH, locbuf, &loc );
	    LOfaddpath( &loc, ERx( "ingres" ), &loc );
	    LOfaddpath( &loc, ERx( "log" ), &loc );
	    LOtos( &loc, &log_dir[i] );
	    log_dir[i] = STalloc( log_dir[i] );
	    i++;
	}
    }

    /*
    ** If PM resources were not defined, don't fall back to
    ** determining log file and path via NM symbols.
    */
    if (i == 0)
    {
	PRINT( ERx( "\niimklog: " ) );
	F_PRINT( ERget( S_ST0423_must_be_defined ), path_sym );
	PRINT( ERx( "\n\n" ) );
	cleanup();
    }

    /* Null out last entry so we know how many we have */
    if (i < LG_MAX_FILE)
    {
	log_file[i] = log_dir[i] = 0;
    }
    
    size = write_transaction_log( create, pmcontext, log_dir,
	    log_file, message, init_graph, 51, update_graph );
    if (size == 0)
    {
	cleanup();
    }
    if (setres)
    {
	/* Set ii.$.rcp.file.kbytes since w know it.
	** This is done for cooked logs where it's inconvenient for
	** the calling script to add up the total log file sizes.
	*/
	call_iisetres(size, pmcontext);
    }

    PRINT( ERx( "\n" ) );
    SIflush( stdout );

    call_ipcclean();

    PCexit( OK );
}
