/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <erglf.h>

#include    <ci.h>
#include    <cm.h>
#include    <cv.h>
#include    <er.h>
#include    <ex.h>
#include    <gcccl.h>
#include    <lo.h>
#include    <me.h>
#include    <nm.h>
#include    <pm.h>
#include    <qu.h>
#include    <sl.h>
#include    <st.h>

#include    <iicommon.h>
#include    <gca.h>
#include    <gcc.h>
#include    <gccer.h>
#include    <gccgref.h>
#include    <gcs.h>
#include    <gcu.h>
#include    <gcxdebug.h>

/*
**
**  Name: gccinit.c
**
**  Description:
**
**	Routines to initialize and terminate the Comm/Bridge Server.
**
**	    gcc_init - Comm Server initialization routine
**	    gcc_term - Comm Server termination routine
**	    gcc_call - Call a list of functions
**
**  History:
**	16-Oct-95 (gordy)
**	    Split out of iigcc.c to supported embedded Comm Server.
**	21-Nov-95 (gordy)
**	    Added standalone flag to gcc_init().
**	27-Feb-96 (rajus01)
**	    Added ci_cap parameter to gcc_init().
**	18-Apr-96 (rajus01)
**	    Added gcb_init_mib() for Bridge.
**	16-Sep-96 (gordy)
**	    Added GCC_MIB_PL_PROTOCOL.
**	25-Sep-96 (gordy)
**	    Make sure only the MIB information for the current server
**	    type gets initialized.
**	 1-Oct-96 (gordy)
**	   Added GCC_MIB_CONNECTION_PL_PROTOCOL.
**	23-May-97 (thoda04)
**	    Included sl.h header for function prototypes.
**	11-Aug-97 (gordy)
**	    Moved all general Comm/Bridge Server initialization
**	    into gcc_init().  Got rid of MIB stuff.  Init GCS.
**	10-Oct-97 (rajus01)
**	    Initialized random number generator for use in gcc_encode().
**	12-Nov-97 (rajus01)
**	    Removed PMlowerOn() in PMload() for bridge server.
**	 8-Sep-98 (gordy)
**	    Determine mode for installation registry.
**      28-apr-1999 (hanch04)
**          Replaced STlcopy with STncpy
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	19-may-2000 (mcgem01)
**	    The PM system is now initialized in CI, so the call to PMinit
**	    will fail, however we should still set the PM defaults here.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	28-jul-2000 (somsa01)
**	    Changed the usage of STcasecmp() for "-instance" back to
**	    STbcompare(), as the argument needs to be partially checked.
**	02-aug-2000 (somsa01)
**	    Do not check the return code from PMinit(), as it could have
**	    been run somewhere else.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      25-Jun-2001 (wansh01) 
**          replaced gcx_init with inline GLOBAL gcc_trace_level init
**	01-may-2003 (somsa01)
**	    In gcc_init(), in the case of EVALUATION_RELEASE, limit the GCC
**	    inbound/outbound connections to 5.
**      11-Jun-2004 (hanch04)
**          Removed reference to CI for the open source release.
**	21-Jul-09 (gordy)
**	    Remove string length restrictions.
**	22-Jan-10 (gordy)
**	    Use CM for common charset initialization.
*/

/*
** Forward references
*/

static	VOID	gcc_err_log( STATUS, i4, PTR );
static	VOID	gcc_msg_log( char * );



/*
** Name: gcc_init
**
** Description:
**	Performs the initialization functions required when the 
**	server begins execution.  If initialization is successful, 
**	OK is returned and execution may proceed.  Otherwise, FAIL 
**	is returned and execution must be aborted. 
**
** Inputs:
**	comsvr			TRUE for Comm Server, FALSE for Bridge Server.
**	ci_cap			CI capability bit to validate.
**	argc			Number of command line argument values.
**	argv			Array of command line argument values.
**
** Outputs:
**      generic_status		System-independent status code.
**	system_status		System-dependent error code.
**
** Returns:
**	STATUS			OK or FAIL.
**
** History:
**      07-Jan-88 (jbowers)
**          Initial routine creation.
**      12-Oct-88 (jbowers)
**          Added error logging initialization.
**      22-Mar-89 (jbowers)
**          Fix for bug # 5106:
**	    Check return code from CVan, and if not OK, substitute default value
**	    for inbound and outbound connection limits.
**      25-Oct-89 (cmorris)
**          If the user has specified a negative inbound or outbound
**          connection limit, substiture the default value.
**      16-Nov-89 (cmorris)
**          Allocate IIGCc_global first as it's referenced when we try to
**          log an error. 
**  	28-Oct-93 (cmorris)
**  	    Get rid of unused variables that should have been removed when
**  	    PM stuff was added.
**	21-Nov-95 (gordy)
**	    Added standalone flag to gcc_init().
**	27-Feb-96 (rajus01)
**	    Added ci_cap parameter to gcc_init().
**	22-May-96 (rajus01)
**	    Added #ifdef CI_INGRES_BRIDGE stmt. for Protocol Bridge MIB     
**	    initialization.
**	25-Sep-96 (gordy)
**	    Make sure only the MIB information for the current server
**	    type gets initialized.
**	11-Aug-97 (gordy)
**	    Moved all general Comm/Bridge Server initialization
**	    into gcc_init().
**	31-Mar-98 (gordy)
**	    Added configuration for default encryption mechanisms.
**	 8-Sep-98 (gordy)
**	    Determine mode for installation registry.
**	01-may-2003 (somsa01)
**	    In the case of EVALUATION_RELEASE, limit the GCC inbound/outbound
**	    connections to 5.
**	14-May-2003 (hanje04)
**	    Extend GCC connect limit of 5 to Linux SDK builds.
**	 6-Jun-03 (gordy)
**	    Initialize new MIB info: PIB queue and hostname.
**	17-Jun-2004 (schka24)
**	    Safe env variable handling.
**	21-Jul-09 (gordy)
**	    Use dynamic storage for mechanisms.
**	22-Jan-10 (gordy)
**	    Use CM for common charset initialization.
*/

STATUS
gcc_init
(
    bool		comsvr,
    i4			ci_cap,
    i4			argc,
    char		**argv,
    STATUS 		*generic_status,
    CL_ERR_DESC 	*system_status
)
{
    STATUS              status = OK;
    CL_ERR_DESC		cl_err;
    GCS_INIT_PARM	init;
    char		*instance_name = ERx("*");
    char		*deflt = ERx("default");
    char		*env;
    char		chname[MAX_LOC];
    i4			i;
    char                *level = NULL;

    /*
    ** Check to see if instance is specified in
    ** command line.  It may be specified as either
    ** -instance=<val> or -instance <val>
    */
    for( i = 1; i < argc; i++ )
	if (!STbcompare(argv[i], 9, ERx("-instance"), 9, TRUE))
	{
	    if ( argv[i][9] == '=' )
		instance_name = &argv[i][10];
	    else  if ( argv[i][9] == EOS  &&  (i + 1) < argc )
		instance_name = argv[++i];
	    break;
	}

    /*
    ** Allocate and initialize the comm server global data area.
    */
    IIGCc_global = (GCC_GLOBAL *)MEreqmem( 0, (u_i4)sizeof(GCC_GLOBAL), 
					   TRUE, &status );
    if ( ! IIGCc_global  ||  status != OK )
    {
	*generic_status = status;
	return( FAIL );
    }

    QUinit( &IIGCc_global->gcc_pib_q );
    QUinit( &IIGCc_global->gcc_ccb_q );
    for( i = 0; i < GCC_L_MDE_Q; i++ )
	QUinit( &IIGCc_global->gcc_mde_q[i] );

    GChostname( IIGCc_global->gcc_host, sizeof( IIGCc_global->gcc_host ) );

    /*
    ** Set up the error logging.
    */
    gcc_er_init( comsvr ? ERx("IIGCC") : ERx("IIGCB"), "" );
    if ( gcc_er_open( generic_status, system_status ) != OK )  return( FAIL );

    /* 
    ** Load environment and configuration information.
    */
    if ( CMset_charset( &cl_err ) != OK )
    {
	*generic_status = E_GC2009_CHAR_INIT;
	return( FAIL );
    }

    PMinit();
    switch( PMload( NULL, NULL ) )
    {
	case OK : break;

	case PM_FILE_BAD :
	    *generic_status = E_GC003D_BAD_PMFILE;
	    return( FAIL );
	
	default :
	    *generic_status = E_GC003E_PMLOAD_ERR;
	    return( FAIL );
    }

    PMsetDefault( 0, SystemCfgPrefix );
    PMsetDefault( 1, PMhost() );
    PMsetDefault( 2, comsvr ? ERx("gcc") : ERx("gcb") );
    PMsetDefault( 3, instance_name );

    /*   set up trace    */
    gcu_get_tracesym( "II_GCC_TRACE", "!.gcc_trace_level", &level);
    if ( level && *level) 
    {
      CVan(level,&i);
      IIGCc_global->gcc_trace_level = i;
    } 

#if defined(EVALUATION_RELEASE) || defined(conf_LinuxSDK)
    IIGCc_global->gcc_ib_max = IIGCc_global->gcc_ob_max = 5;
#else
    if ( PMget( ERx("!.inbound_limit"), &env ) != OK  || 
         CVan( env, &IIGCc_global->gcc_ib_max ) != OK )
	if ( comsvr )
	    IIGCc_global->gcc_ib_max = GCC_IB_DEFAULT;
	else
	{
	    IIGCc_global->gcc_ib_max = 1;
	    IIGCc_global->gcc_flags |= GCC_TRANSIENT;
	}

    if ( PMget( ERx("!.outbound_limit"), &env ) != OK  || 
         CVan( env, &IIGCc_global->gcc_ob_max ) != OK )
	IIGCc_global->gcc_ob_max = GCC_OB_DEFAULT;
#endif  /* EVALUATION_RELEASE */


    IIGCc_global->gcc_ib_mode = GCC_ENCRYPT_MODE;
    if ( PMget( ERx("!.ib_encrypt_mode"), &env ) == OK  &&
	 gcc_encrypt_mode( env, &IIGCc_global->gcc_ib_mode ) != OK )
    {
	GCC_ERLIST	erlist;

	status = E_GC200A_ENCRYPT_MODE;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = sizeof( env );
	erlist.gcc_erparm[0].value = env;
	gcc_er_log( &status, NULL, NULL, &erlist );
    }

    IIGCc_global->gcc_ob_mode = GCC_ENCRYPT_MODE;
    if ( PMget( ERx("!.ob_encrypt_mode"), &env ) == OK  &&
	 gcc_encrypt_mode( env, &IIGCc_global->gcc_ob_mode ) != OK )
    {
	GCC_ERLIST	erlist;

	status = E_GC200A_ENCRYPT_MODE;
	erlist.gcc_parm_cnt = 1;
	erlist.gcc_erparm[0].size = sizeof( env );
	erlist.gcc_erparm[0].value = env;
	gcc_er_log( &status, NULL, NULL, &erlist );
    }

    if ( PMget( ERx("!.ib_encrypt_mech"), &env ) == OK )
	IIGCc_global->gcc_ib_mech = STalloc( env );
    else
	IIGCc_global->gcc_ib_mech = GCC_ANY_MECH;

    if ( PMget( ERx("!.ob_encrypt_mech"), &env ) == OK )
	IIGCc_global->gcc_ob_mech = STalloc( env );
    else
	IIGCc_global->gcc_ob_mech = GCC_ANY_MECH;

    /*
    ** Determine the installation registry type.
    ** If not configured for GCC, or is "default",
    ** use the registry type of the Name Server.
    */
    if ( PMget( ERx("!.registry_type"), &env ) != OK  ||
	 ! STbcompare( env, 0, deflt, 0, TRUE ) )
	if ( PMget( ERx("$.$.gcn.registry_type"), &env ) != OK )
	    env = deflt;
    
    if ( ! STbcompare( env, 0, GCC_REG_MASTER_STR, 0, TRUE ) )
	IIGCc_global->gcc_reg_mode = GCC_REG_MASTER;
    else  if ( ! STbcompare( env, 0, GCC_REG_PEER_STR, 0, TRUE ) )
	IIGCc_global->gcc_reg_mode = GCC_REG_PEER;
    else  if ( ! STbcompare( env, 0, GCC_REG_SLAVE_STR, 0, TRUE ) )
	IIGCc_global->gcc_reg_mode = GCC_REG_SLAVE;
    else
	IIGCc_global->gcc_reg_mode = GCC_REG_NONE;

    /*
    ** Initialize the encryption system.
    */
    MEfill( sizeof( init ), '\0', (PTR)&init );
    init.version = GCS_API_VERSION_1;
    init.mem_alloc_func = (PTR)gcc_alloc;
    init.mem_free_func = (PTR)gcc_free;
    init.err_log_func = (PTR)gcc_err_log;
    init.msg_log_func = (PTR)gcc_msg_log;

    if ( (status = IIgcs_call( GCS_OP_INIT, GCS_NO_MECH, (PTR)&init )) != OK )
    {
	*generic_status = status;
	return( FAIL );
    }

    /* Initialize random number generator.
    ** It is used in gcc_encode() for username, password encryption.
    */
    MHsrand( TMsecs() );

    return( OK );
}  /* end gcc_init */



/*
** Name: gcc_term
**
** Description:
**	Performs the termination functions required when the server 
**	shuts down.  If termination is successful, OK is returned.  
**	Otherwise, FAIL is returned.
**
** Inputs:
**      None.
**
** Outputs:
**      generic_status			System-independent status code.
**	system_status			System-dependent error code.
**
** Returns:
**	STATUS			OK or FAIL.
**
** History:
**      07-Jan-88 (jbowers)
**          Initial routine creation.
**      12-Oct-88 (jbowers)
**          Added error logging termination.
*/

STATUS
gcc_term( STATUS *generic_status, CL_ERR_DESC *system_status )
{
    i4	i;

    /*
    ** Shutdown the encryption system.
    */
    IIgcs_call( GCS_OP_TERM, GCS_NO_MECH, NULL );

    /*
    ** Close the error log.
    */
    gcc_er_close( generic_status, system_status );

    /*
    ** Deallocate the Comm Server global data area.
    */
    for( i = 0; i < IIGCc_global->gcc_max_ccb; i++ )
	if ( IIGCc_global->gcc_tbl_ccb[ i ] )
	    MEfree( (PTR)IIGCc_global->gcc_tbl_ccb[ i ] );

    if ( IIGCc_global->gcc_tbl_ccb )
	MEfree( (PTR)IIGCc_global->gcc_tbl_ccb );

    MEfree( (PTR)IIGCc_global );
    IIGCc_global = NULL;

    return( OK );
}  /* end gcc_term */


/*
** Name: gcc_call
**
** Description:
**	Call a list of functions checking their status and returning
**	the number of successful calls.  
**
** Input:
**	call_count	Number of functions to call.
**	call_list	Array of function pointers.
**
** Output:
**	call_count	Number of successful calls.
**	generic_status	System-independent status of failing function.
**	system_status	System-dependent status of failing function.
**
** Returns:
**	STATUS		OK if all successful, FAIL otherwise.
**
** History:
*/

STATUS
gcc_call( i4  *call_count, STATUS (**call_list)(), 
	  STATUS *generic_status, CL_ERR_DESC *system_status )
{
    STATUS	status = OK;
    i4	i;

    for( i = *call_count; i; call_list++, i-- )
	if ( (status = (**call_list)( generic_status, system_status )) )
	    break;

    *call_count -= i;

    return( status );
}


/*
** Name: gcc_err_log
**
** Description:
**	GCS Callback function for error logging.
**
** Input:
**	error_code	Error code.
**	parm_count	Number of parameters.
**	parms		Parameters (ER_ARGUMENT array).
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	12-Aug-97 (gordy)
**	    Created.
*/

static VOID
gcc_err_log( STATUS error_code, i4  parm_count, PTR parms )
{
    gcu_erlog( 0, 1, error_code, NULL, parm_count, parms );
    return;
}


/*
** Name: gcc_msg_log
** 
** Description:
**	GCS callback function for logging messages.
**
** Input:
**	message		Message to be logged.
**
** Output:
**	None.
**
** Returns:
**	VOID
**
** History:
**	12-Aug-97 (gordy)
**	    Created.
*/

static VOID
gcc_msg_log( char *message )
{
    gcu_msglog( 0, message );
    return;
}

