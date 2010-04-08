/*
** Copyright (c) 1999, 2008 Ingres Corporation All Rights Reserved.
*/


#include <compat.h>
#include <gl.h>
#include <cm.h>
#include <cv.h>
#include <er.h>
#include <ex.h>
#include <gc.h>
#include <me.h>
#include <mh.h>
#include <nm.h>
#include <pc.h>
#include <pm.h>
#include <si.h>
#include <st.h>
#include <tm.h>
#include <tr.h>

#include <iicommon.h>
#include <gca.h>
#include <gcn.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>
#include <sp.h>
#include <mo.h>

/*
** Name: iigcd.c
**
** Description:
**	Ingres Data Access Server.  This server listens on a network
**	protocol port for network connection requests from Data Access
**	Messaging applications. 
**
**	This server also registers with the local Ingres Name
**	Server, but rejects GCA connection requests other than
**	server shutdown/quiesce requests.
**
** Ming hints
PROGRAM = (PROG1PRFX)gcd
NEEDLIBS= APILIB GCFLIB ADFLIB CUFLIB COMPATLIB 
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
**	    Replaced SIstd_write with SIfprintf for alternate products.
**	    (BACKED OUT NOT NEEDED IN MAIN. (hanje04)
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	26-Dec-02 (gordy)
**	    Load the local character-set ID.
**	15-Jan-03 (gordy)
**	    Add the TL Service Provider registry.
**	14-Mar-03 (gordy)
**	    Placed gcd objects in own library, adjusted ming hints.
**	18-Mar-2003 (wansh01)
**	    Moved GLOBALDEF GCD_global to gcddata.c. 
**	    Changed initilization of gcd_tl_services[] to initializie() 
**	    to avoid compiler error in w32 
**	 6-Jun-03 (gordy)
**	    Added GCD MIB information.  Protocol information saved in PIB.
**	17-Jul-03 (wansh01) 
**	    Added gcd_mib_init() from gcdmib.c.
**	08-aug-2003 (devjo01)
**	    Get number of class elements for gcd_classes from gcd_class_cnt,
**	    since we cannot calculate it from the sizeof(gcd_classes) as
**	    the number of array entries is not given in the GLOBALREF.
**      30-Mar-2004 (wansh01)
**          added call to gcd_adm_init and gcd_adm_term to support
**          gcadm interface.
**	16-Jun-04 (gordy)
**	    Seed the random number generator for session masks generation.
**	11-Jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
**	16-Jun-04 (gordy)
**	    Seed the random number generator for session masks generation.
**      24-Sept-04 (wansh01)
**          Added GCD_global.gcd_lcl_id (Server listen address) to support 
**          SHOW SERVER command.
**	29-Sep-2004 (drivi01)
**	    Removed MALLOCLIB from NEEDLIBS.  Added some dynamic libraries
**	    for windows build.
**      18-Nov-2004 (Gordon.Thorpe@ca.com and Ralph.Loen@ca.com)
**          Use CMgetDefCS() to retrieve the CMdefcs global reference, 
**          otherwise the Windows cannot resolve the reference to CMdefcs.
**	30-Nov-2004 (drivi01)
**	    Added ADFLIB library to NEEDLIBS hint.
**	10-Apr-2007 (fanra01)
**	    Bug 118097
**	    Use local server Id for output instead of uninitialized name string.
**	 5-Dec-07 (gordy)
**	    Moved client connection limit to GCC level.
**      05-Jun-08 (Ralph Loen)  SIR 120457
**          Moved startup message to gcd_gca_init(), to be consistent with
**          the format of the GCC.
**          
*/


/*
** Transport Layer Service Provider registry
*/

static TL_SRVC *gcd_tl_services[ 2 ]; 

GLOBALREF i4		gcd_class_cnt;
GLOBALREF MO_CLASS_DEF	gcd_classes[];

/*
**  Forward and/or External function references.
*/

FUNC_EXTERN	i4	GCX_exit_handler();
static		STATUS	initialize( i4, char ** );
static		bool	gcd_cset_id( char *, u_i4 );
static		VOID	gcd_exit();


/*
** Name: main
**
** Description:
**	GCD Server main entry routine.  Initialize the server,
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
**	 6-Jun-03 (gordy)
**	    Hostname saved in globals.  Initialize MIB.
**      26-Apr-2007 (hanal04) SIR 118196
**          Make sure PCpid will pick up the current process pid on Linux.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added support for maintaining API environment handle for each
**	    supported client protocol level.
*/	

int
main( int argc, char **argv )
{
    EX_CONTEXT		context;
    char		name[ GCC_L_PORT + 1 ];
    u_i2		apivers;
    PTR 		hndl;

#ifdef LNX
    PCsetpgrp();
#endif

    MEadvise( ME_INGRES_ALLOC );

    if ( EXdeclare( GCX_exit_handler, &context ) != OK )
    {
	gcu_erlog( 0, 1, E_GC480F_EXCEPTION, NULL, 0, NULL );
	PCexit( OK );
	return( 0 );
    }

    GChostname( GCD_global.host, sizeof( GCD_global.host ) );
    gcu_erinit( GCD_global.host, GCD_LOG_ID, "" );

    if ( initialize( argc, argv ) != OK )  gcd_exit();
    gcd_init_mib();
    if ( gcd_gca_init( ) != OK )  gcd_exit();
    gcu_erinit( GCD_global.host, GCD_LOG_ID, GCD_global.gcd_lcl_id );

    if ( gcd_adm_init() != OK ) gcd_exit();
    apivers = gcd_msg_version( MSG_DFLT_PROTO ); 
    if ( gcd_get_env( apivers, &hndl ) != OK ) gcd_exit();
    if ( gcd_pool_init() != OK )  gcd_exit();
    if ( gcd_gcc_init() != OK )  gcd_exit();

    /*
    ** Log Server startup.
    */
    {
	char		server_flavor[256], server_type[32];
	char		*tmpbuf = PMgetDefault(3);
	ER_ARGUMENT	erlist[2];

	SIstd_write(SI_STD_OUT, "PASS\n");

	if ( ! STbcompare( tmpbuf, 0, "*", 0, TRUE ) )
	    STcopy( "(DEFAULT)", server_flavor );
	else
	    STcopy( tmpbuf, server_flavor );

	STcopy(PMgetDefault(2), server_type);
	CVupper(server_type);

	erlist[0].er_value = server_flavor;
	erlist[0].er_size = STlength(server_flavor);
	erlist[1].er_value = server_type;
	erlist[1].er_size = STlength(server_type);

	gcu_erlog( 0, GCD_global.language, E_GC4802_LOAD_CONFIG, NULL, 2, 
		   (PTR)&erlist );
	gcu_erlog( 0, GCD_global.language, E_GC4800_STARTUP, NULL, 0, NULL );
    }

    GCexec();

    gcd_gcc_term();
    gcd_pool_term();
    gcd_rel_env(0);
    gcd_adm_term();
    gcd_gca_term();
    gcu_erlog( 0, GCD_global.language, E_GC4801_SHUTDOWN, NULL, 0, NULL );

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
**	26-Dec-02 (gordy)
**	    Load the local character-set ID.
**	15-Jan-03 (gordy)
**	    Initialize the TL Service Provider registry.
**	 6-Jun-03 (gordy)
**	    Initialize PIB queue.
**	16-Jun-04 (gordy)
**	    Seed the random number generator for session masks generation.
**	17-Jun-2004 (schka24)
**	    Length-check the charset name.
**	 5-Dec-07 (gordy)
**	    Moved client connection limit to GCC level.
*/ 

static STATUS
initialize( i4 argc, char **argv )
{
    CL_ERR_DESC	cl_err;
    char	*instance = ERx("*");
    char	*env;
    char	name[ 16 ];
    i4		i;

    GCD_global.language = 1;
    MHsrand( TMsecs() );

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
    STprintf( name, ERx("II_CHARSET%s"), (env  &&  *env) ? env : "" );
    NMgtAt( name, &env );

    if ( ! env  ||  ! *env || STlength(env) > CM_MAXATTRNAME)
    {
	switch( CMgetDefCS() )
	{
#if ( !defined(UNIX) && !defined(VMS) )
	case CM_IBM :	    env = "IBMPC850";	break;
#endif
	case CM_DECMULTI :  env = "DECMULTI";	break;
	default :	    env = "ISO88591";	break;
	}
    }

    GCD_global.charset = STalloc( env );
    CVupper( GCD_global.charset );
    gcu_read_cset( gcd_cset_id );

    if ( CMset_attr( GCD_global.charset, &cl_err) != OK )
    {
	gcu_erlog( 0, GCD_global.language, 
		    E_GC0105_GCN_CHAR_INIT, NULL, 0, NULL );
	return( E_GC0105_GCN_CHAR_INIT );
    }

    PMinit();

    switch( PMload( (LOCATION *)NULL, (PM_ERR_FUNC *)NULL ) )
    {
	case OK: break;

	case PM_FILE_BAD:
	    gcu_erlog( 0, GCD_global.language, 
			E_GC003D_BAD_PMFILE, NULL, 0, NULL );
	    return( E_GC003D_BAD_PMFILE );
	    break;

	default:
	    gcu_erlog( 0, GCD_global.language, 
			E_GC003E_PMLOAD_ERR, NULL, 0, NULL );
	    return( E_GC003E_PMLOAD_ERR );
	    break;
    }

    PMsetDefault( 0, SystemCfgPrefix );
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, ERx("gcd") );
    PMsetDefault( 3, instance );

    gcd_tl_services[0] = &gcd_dmtl_service;
    gcd_tl_services[1] = &gcd_jctl_service;
    GCD_global.tl_services = gcd_tl_services;
    GCD_global.tl_srvc_cnt = ARR_SIZE( gcd_tl_services );

    QUinit( &GCD_global.pib_q );
    QUinit( &GCD_global.ccb_q );
    QUinit( &GCD_global.ccb_free );
    for( i = 0; i < ARR_SIZE( GCD_global.cib_free ); i++ )  
	QUinit( &GCD_global.cib_free[ i ] );

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.client_max"), &env );
    if ( env  &&  *env )
    {
	i4 count;

	if ( CVal( env, &count ) == OK  &&  count > 0 )
	    GCD_global.client_max = count;
    }

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.client_timeout"), &env );
    if ( env  &&  *env )  
    {
	i4 timeout;

	if ( CVal( env, &timeout ) == OK  &&  timeout > 0 )
	    GCD_global.client_idle_limit = timeout * 60; /* Cnvt to seconds */
    }

    env = NULL;
    gcu_get_tracesym( NULL, ERx("!.connect_pool_status"), &env );
    if ( env  &&  *env )
    {
	if ( ! STbcompare( env, 0, "optional", 0, TRUE ) )
	    GCD_global.client_pooling = TRUE;

	if ( GCD_global.client_pooling  ||
	     ! STbcompare( env, 0, "on", 0, TRUE ) )
	{
	    env = NULL;
	    gcu_get_tracesym( NULL, ERx("!.connect_pool_size"), &env );
	    if ( env  &&  *env )  CVal( env, &GCD_global.pool_max );

	    env = NULL;
	    gcu_get_tracesym( NULL, ERx("!.connect_pool_expire"), &env );
	    if ( env  &&  *env )  
	    {
		i4 limit;

		if ( CVal( env, &limit ) == OK  &&  limit > 0 )
		    GCD_global.pool_idle_limit = limit * 60; /* Seconds */
	    }
	}
    }

    env = NULL;
    gcu_get_tracesym( "II_GCD_TRACE", ERx("!.gcd_trace_level"), &env );
    if ( env  &&  *env )  CVal( env, &GCD_global.gcd_trace_level );

    env = NULL;
    gcu_get_tracesym( "II_GCD_LOG", ERx("!.gcd_trace_log"), &env );
    if ( env  &&  *env ) 
	TRset_file( TR_F_OPEN, env, (i4)STlength( env ), &cl_err );

    return( OK );
}

/*
** Name: gcd_cset_id
**
** Description:
**	Callback routine for gcu_read_cset().  Checks each
**	character-set entry for match with installation
**	character-set.  Saves the character-set ID when found.
**
** Input:
**	name	Character-set name.
**	id	Character-set ID.
**
** Ouput:
**	None.
**
** Returns:
**	bool	TRUE to continue processing, FALSE to stop.
**
** History:
**	26-Dec-02 (gordy)
**	    Created.
*/

static bool
gcd_cset_id( char *name, u_i4 id )
{
    if ( ! STbcompare( name, 0, GCD_global.charset, 0, TRUE ) )
    {
	GCD_global.charset_id = id;
	return( FALSE );
    }
    
    return( TRUE );
}

/*
** Name: gcd_exit
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
gcd_exit()
{
    SIstd_write(SI_STD_OUT, "\nFAIL\n");
    PCexit(FAIL);
}

/*
** Name: gcd_init_mib()
**
** Description:
**      Issues MO calls to define all GCD MIB objects.
**
** Input:
**      None.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 6-Jun-03 (gordy)
**	    Created.
*/

VOID
gcd_init_mib( VOID )
{
    MOclassdef( gcd_class_cnt, gcd_classes );
    return;
}

