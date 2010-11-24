/*
** Copyright (c) 1999, 2004 Ingres Corporation All Rights Reserved.
*/


#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <ex.h>
#include <gc.h>
#include <me.h>
#include <nm.h>
#include <pc.h>
#include <pm.h>
#include <si.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcapi.h>

/*
** Name: iijdbc.c
**
** Description:
**	Ingres JDBC server.  This server listens on a network
**	protocol port for connection requests from Java based
**	JDBC applications. 
**
**	This server also registers with the local Ingres Name
**	Server, but rejects GCA connection requests other than
**	server shutdown/quiesce requests.
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	22-Sep-99 (gordy)
**	    Save character set in global info.
**	21-Dec-99 (gordy)
**	    Added support for client idle timeouts.
**	17-Jan-00 (gordy)
**	    Added support for connection pooling.
**	22-Feb-00 (rajus01)
**	    Changed config strings for connection pooling to support 
**	    configuring using CBF.
**	 3-Mar-00 (gordy)
**	    Load max client limit config setting.
**	 9-Mar-00 (rajus01)
**	    Added JDBCLIB to NEEDLIBS.
**	27-Mar-00 (rajus01)
**	    Added INGNETLIB to NEEDLIBS
**	31-mar-2000 (somsa01)
**	    Added printout of E_JD0113_LOAD_CONFIG on successful startup.
**	24-apr-2000 (somsa01)
**	    Updated MING hint for program name to account for different
**	    products.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-sep-2000 (somsa01)
**	    Print a FAIL message to standard output upon startup failure.
**	13-nov-2001 (loera01) SIR 106366
**	    Replaced SIstd_write with SIfprintf for other products.
**	    (BACKED OUT NOT NEEDED IN MAIN. (hanje04)
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS
**      15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Changes to eliminate compiler prototype warnings.
*/

/*
NEEDLIBS =      JDBCLIB APILIB ADFLIB GCFLIB CUFLIB COMPATLIB INGNETLIB 
OWNER =         INGUSER
PROGRAM =       (PROG1PRFX)jdbc
*/

GLOBALDEF JDBC_GLOBAL	JDBC_global ZERO_FILL;

/*
** Kludge Alert: CMdefcs is not exposed through
** the CM interface, but this is what GCC uses
** to determine the default character set.  We
** simply propagate this mess.
*/

GLOBALREF	i4	CMdefcs;

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN	i4	GCX_exit_handler();
static		STATUS	initialize( i4 argc, char **argv );
static		VOID	jdbc_exit( void );


/*
** Name: main
**
** Description:
**	JDBC Server main entry routine.  Initialize the server,
**	GCA, and GCC interfaces.  Loops in GCexec() waiting for
**	shutdown request.
**	
** History:
**	 2-Jun-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Moved GCA initialization above API so listen info will
**	    be in first GCA control block.  Bit of a kludge until
**	    GCN is updated to handle multi-control block servers.
**	31-mar-2000 (somsa01)
**	    Printout E_JD0113_LOAD_CONFIG on successful startup.
*/	

int
main( int argc, char **argv )
{
    EX_CONTEXT		context;
    char		hostname[ 33 ];
    char		name[ MAX_LOC ];

    MEadvise( ME_INGRES_ALLOC );

    if ( EXdeclare( GCX_exit_handler, &context ) != OK )
    {
	gcu_erlog( 0, 1, E_JD0102_EXCEPTION, NULL, 0, NULL );
	PCexit( OK );
	return( 0 );
    }

    GChostname( hostname, sizeof( hostname ) );
    gcu_erinit( hostname, JDBC_LOG_ID, "" );

    if ( initialize( argc, argv ) != OK )  jdbc_exit();
    /* TODO: init MIB */
    if ( jdbc_gca_init( name ) != OK )
	jdbc_exit();
    else
    {
	char	buffer[512];

	STprintf(buffer, "\nJDBC Server = %s", name);
	SIstd_write(SI_STD_OUT, buffer);
	SIstd_write(SI_STD_OUT, "PASS\n");
    }
    if ( jdbc_pool_init() != OK )  PCexit( FAIL );
    if ( jdbc_api_init() != OK )  PCexit( FAIL );
    if ( jdbc_gcc_init() != OK )  PCexit( FAIL );

    gcu_erinit( hostname, JDBC_LOG_ID, name );

    {
	/*
	** Obtain and fix up the configuration name, then log it.
	*/
	char		server_flavor[256], server_type[32];
	char		*tmpbuf = PMgetDefault(3);
	ER_ARGUMENT	erlist[2];

	if (!STbcompare( tmpbuf, 0, "*", 0, TRUE ))
	    STcopy("(DEFAULT)", server_flavor);
	else
	    STcopy(tmpbuf, server_flavor);

	STcopy(PMgetDefault(2), server_type);
	CVupper(server_type);

	erlist[0].er_value = server_flavor;
	erlist[0].er_size = STlength(server_flavor);
	erlist[1].er_value = server_type;
	erlist[1].er_size = STlength(server_type);

	gcu_erlog( 0, JDBC_global.language, E_JD0113_LOAD_CONFIG, NULL, 2, 
		   (PTR)&erlist );
    }

    gcu_erlog( 0, JDBC_global.language, E_JD0100_STARTUP, NULL, 0, NULL );

    GCexec();

    jdbc_pool_term();
    jdbc_gcc_term();
    jdbc_api_term();
    jdbc_gca_term();
    gcu_erlog( 0, JDBC_global.language, E_JD0101_SHUTDOWN, NULL, 0, NULL );

    EXdelete();
    PCexit( OK );
    return( 0 );
}

/*
** Name: initialize
**
** Description:
**	General server initialization.
**
** Input:
**	argc		Number of command line arguments.
**	argv		Command line arguments.
**
** Output:
**	None.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 2-Jun-99 (gordy)
**	    Created.
**	22-Sep-99 (gordy)
**	    Save character set in global info.
**	21-Dec-99 (gordy)
**	    Load client idle limit.
**	17-Jan-00 (gordy)
**	    Added config params for connection pool.
**	22-Feb-00 (rajus01)
**	    Changed config strings for connection pooling to support 
**	    configuring using CBF.
**	 3-Mar-00 (gordy)
**	    Load max client limit config setting.
**	17-Jun-2004 (schka24)
**	    Safe env variable handling.
*/ 

static STATUS
initialize( i4 argc, char **argv )
{
    CL_ERR_DESC	cl_err;
    char	*instance = ERx("*");
    char	*env;
    char	name[ MAX_LOC ];
    i4		i;

    JDBC_global.language = 1;

    for( i = 1; i < argc; i++ )
	if ( ! STbcompare( argv[i], 0, ERx("-instance"), 9, TRUE ) )
	{
	    if ( argv[i][9] == ERx('=')  &&  argv[i][10] != EOS )
		instance = &argv[i][10];
	    else  if ( argv[i][9] == EOS  &&  (i + 1) < argc )
		instance = argv[++i];
	    break;
	}

    NMgtAt( ERx("II_INSTALLATION"), &env );
    if (env != NULL && *env != '\0')
	STlpolycat(2, sizeof(name)-1, "II_CHARSET", env, &name[0]);
    else
	STcopy("II_CHARSET", name);
    NMgtAt( name, &env );

    if ( ! env  ||  ! *env )
    {
	/*
	** Kludge Alert: CMdefcs is not exposed through
	** the CM interface, but this is what GCC uses
	** to determine the default character set.  We
	** simply propagate this mess.
	*/
	switch( CMdefcs )
	{
#if ( !defined(UNIX) && !defined(VMS) )
	case CM_IBM :	    env = "IBMPC850";	break;
#endif
	case CM_DECMULTI :  env = "DECMULTI";	break;
	default :	    env = "ISO88591";	break;
	}
    }

    STlcopy(env, name, CM_MAXATTRNAME);
    JDBC_global.charset = STalloc( name );
    CVupper( JDBC_global.charset );

    if ( CMset_attr( JDBC_global.charset, &cl_err) != OK )
    {
	gcu_erlog( 0, JDBC_global.language, 
		    E_GC0105_GCN_CHAR_INIT, NULL, 0, NULL );
	return( E_GC0105_GCN_CHAR_INIT );
    }

    PMinit();

    switch( PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)NULL ) )
    {
	case OK: break;

	case PM_FILE_BAD:
	    gcu_erlog( 0, JDBC_global.language, 
			E_GC003D_BAD_PMFILE, NULL, 0, NULL );
	    return( E_GC003D_BAD_PMFILE );
	    break;

	default:
	    gcu_erlog( 0, JDBC_global.language, 
			E_GC003E_PMLOAD_ERR, NULL, 0, NULL );
	    return( E_GC003E_PMLOAD_ERR );
	    break;
    }

    PMsetDefault( 0, SystemCfgPrefix );
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, ERx("jdbc") );
    PMsetDefault( 3, instance );

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.protocol"), &env );
    if ( ! env  ||  ! *env )  return( E_JD0104_NET_CONFIG );
    JDBC_global.protocol = STalloc( env );

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.port"), &env );
    if ( ! env  ||  ! *env )  return( E_JD0104_NET_CONFIG );
    JDBC_global.port = STalloc( env );

    QUinit( &JDBC_global.ccb_q );
    QUinit( &JDBC_global.ccb_free );
    for( i = 0; i < ARR_SIZE( JDBC_global.cib_free ); i++ )  
	QUinit( &JDBC_global.cib_free[ i ] );

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.client_max"), &env );
    if ( env  &&  *env )
    {
	i4 count;

	if ( CVal( env, &count ) == OK  &&  count > 0 )
	    JDBC_global.cib_max = count;
    }

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.client_timeout"), &env );
    if ( env  &&  *env )  
    {
	i4 timeout;

	if ( CVal( env, &timeout ) == OK  &&  timeout > 0 )
	    JDBC_global.client_idle_limit = timeout * 60; /* Cnvt to seconds */
    }

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.connect_pool_status"), &env );
    if ( env  &&  *env )
    {
	if ( ! STbcompare( env, 0, "optional", 0, TRUE ) )
	    JDBC_global.client_pooling = TRUE;

	if ( JDBC_global.client_pooling  ||
	     ! STbcompare( env, 0, "on", 0, TRUE ) )
	{
	    env = NULL;
	    gcu_get_tracesym( NULL, ERx("!.connect_pool_size"), &env );
	    if ( env  &&  *env )  CVal( env, &JDBC_global.pool_max );

	    env = NULL;
	    gcu_get_tracesym( NULL, ERx("!.connect_pool_expire"), &env );
	    if ( env  &&  *env )  
	    {
		i4 limit;

		if ( CVal( env, &limit ) == OK  &&  limit > 0 )
		    JDBC_global.pool_idle_limit = limit * 60; /* Seconds */
	    }
	}
    }

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.trace_level"), &env );
    if ( env  &&  *env )  CVal( env, &JDBC_global.trace_level );

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.trace_log"), &env );
    if ( env  &&  *env )
    {
	STlcopy( env, name, sizeof(name)-1);
	TRset_file( TR_F_OPEN, name, STlength( name ), &cl_err );
    }

    return( OK );
}

/*
** Name: jdbc_exit
**
** Description:
**	Called when we need to exit upon failure.
**
** Input:
**	None.
**
** Output:
**	None.
**
** Returns:
**	None.
**
** History:
**	04-sep-2000 (somsa01)
**	    Created.
*/

static VOID
jdbc_exit()
{
    SIstd_write(SI_STD_OUT, "\nFAIL\n");
    PCexit(FAIL);
}

