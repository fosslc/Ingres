/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <st.h>
#include <tr.h>
#include <me.h>

#include <iicommon.h>
#include <gcu.h>
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdapi.c
**
** Description:
**	GCD server interface to API.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	29-Sep-99 (gordy)
**	    Set default BLOB buffer size to 8192 since this
**	    matches the NL max packet length.  Pass the
**	    more_segments flag through the API/MSG interface.
**	 4-Nov-99 (gordy)
**	    Added support for connect timeouts.
**	13-Dec-99 (gordy)
**	    Added support for cursor pre-fetching.
**	 6-Jan-00 (gordy)
**	    Connection data now stored in JDBC_CIB structure.
**	17-Jan-00 (gordy)
**	    Connection handle to disconnect now passed in PCB
**	    to avoid race conditions with the CCB.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	19-May-00 (gordy)
**	    Separate rows requested and rows received counters.
**	21-jul-2000 (somsa01)
**	    Added inclusion of st.h .
**	25-Aug-00 (gordy)
**	    Save procedure return values in query info.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack with new functions
**	    jdbc_push_callback(), jdbc_pop_callback().
**	19-Oct-00 (gordy)
**	    Rollback transaction if commit fails.  Don't set the
**	    autocommit flag (callers responsibility).
**	15-Nov-00 (gordy)
**	    Return the cursor readonly flag rather than pre-fetch limit.
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions: register and
**	    release XID, prepare (2PC) transaction.
**	18-Apr-01 (gordy)
**	    Added support for connecting to a distributed transaction.
**	29-apr-2001 (mcgem01)
**	    Add include of me.h
**	10-May-01 (gordy)
**	    Bumped API version for UCS2 datatypes.
**	13-Nov-01 (loera01) SIR 106366
**	    Made API version dependent on UCS2 (Unicode) support.
**	    (BACKED OUT NOT NEEDED IN MAIN. (hanje04)
**      28-Feb-02 (loera01) Bug 107238
**          Provide default environment values for money and decimal format 
**          settings.
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	14-Feb-03 (gordy)
**	    Added gcd_api_savepoint().  Return table/object keys.
**	15-Mar-04 (gordy)
**	    Bumped API version for BIGINT.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Clear transaction abort flag after status check 
**	    (which sets the flag).  Check for API logical 
**	    sequence errors.
**	21-Apr-06 (gordy)
**	    Support trace message with API callback function.
**	16-Jun-06 (gordy)
**	    Use API version 5 to support ANSI date/time.
**	14-Jul-06 (gordy)
**	    Added support for XA requests.
**	15-Nov-06 (gordy)
**	    Added support for LOB Locators.
**	 4-Apr-07 (gordy)
**	    Added support for scrollable cursors.
**	18-Jul-07 (gordy)
**	    Set date alias default.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added support for maintaining API environment handle for each
**	    supported client protocol level.
**	25-Mar-10 (gordy)
**	    Added support for batch processing.
*/	

static	GCULIST	apiMap[] = 
{
    { IIAPI_ST_SUCCESS, "SUCCESS" },
    { IIAPI_ST_MESSAGE, "USER_MESSAGE" },
    { IIAPI_ST_WARNING, "WARNING" },
    { IIAPI_ST_NO_DATA, "END-OF-DATA" },
    { IIAPI_ST_ERROR, "ERROR" },
    { IIAPI_ST_FAILURE, "FAILURE" },
    { IIAPI_ST_NOT_INITIALIZED, "NO_INIT" },
    { IIAPI_ST_INVALID_HANDLE, "INVALID_HANDLE" },
    { IIAPI_ST_OUT_OF_MEMORY, "OUT_OF_MEMORY" },
    { 0, NULL }
};

#define	API_ERROR( status )	( (status != IIAPI_ST_SUCCESS)  && \
				  (status != IIAPI_ST_MESSAGE)  && \
				  (status != IIAPI_ST_WARNING)	&& \
				  (status != IIAPI_ST_NO_DATA) )

static	STATUS	gcd_api_status( char *, IIAPI_GENPARM *, 
				 GCD_CCB *, GCD_RCB ** );
static	II_VOID	gcd_api_trace( IIAPI_TRACEPARM * );

II_FAR II_CALLBACK II_VOID	gcd_sCon_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_conn_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_disc_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_abort_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_sp_cmpl( II_PTR , II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_start_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_end_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_commit_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_roll_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_auto_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_qry_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_bat_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_gDesc_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_gCol_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_gqi_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_cncl_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	gcd_gen_cmpl( II_PTR, II_PTR );


/*
** Name: gcd_api_init
**
** Description:
**	Initialize the GCD API interface.
**
** Input:
**	api_vers	API version.
**	env	 	Pointer to environment handle.	 
**
** Output:
**	env		API environment handle.
**
** Returns:
**	STATUS		OK or error code.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Set default BLOB buffer size to 8192 since this
**	    matches the NL max packet length.
**	10-May-01 (gordy)
**	    Bumped API version for UCS2 datatypes.
**	15-Mar-04 (gordy)
**	    Bumped API version for BIGINT.
**	21-Apr-06 (gordy)
**	    Support trace message with API callback function.
**	16-Jun-06 (gordy)
**	    Use API version 5 to support ANSI date/time.
**	15-Nov-06 (gordy)
**	    Use API version 6 to support LOB Locators.
**	18-Jul-07 (gordy)
**	    Default date alias to ingresdate rather than allowing
**	    API to pick up the installation setting which isn't
**	    necessarily correct for non-installation oriented
**	    clients.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Initialize API environment handle for the given API version and
**	    set the environment parameters accordingly.
*/

STATUS
gcd_api_init( u_i2 api_ver, PTR *env )
{
    IIAPI_INITPARM	init;
    IIAPI_SETENVPRMPARM	set;
    bool		done;
    STATUS		status = OK;

    init.in_timeout = -1;
    init.in_version = api_ver;

    IIapi_initialize( &init );

    if ( init.in_status != IIAPI_ST_SUCCESS  &&
	 GCD_global.gcd_trace_level >= 1 )
	TRdisplay( "%4d    GCD Error initializing API: %s\n", 
		   -1, gcu_lookup( apiMap, init.in_status ) );

    switch( init.in_status )
    {
    case IIAPI_ST_SUCCESS :
	*env = init.in_envHandle;
	break;
    
    case IIAPI_ST_OUT_OF_MEMORY :
	status = E_GC4808_NO_MEMORY;
	break;
    
    default :
	status = E_GC4809_INTERNAL_ERROR;
	break;
    }

    for( done = (status != OK); ! done; done = TRUE )
    {
	II_LONG	value;

	set.se_envHandle = *env;
	set.se_paramID = IIAPI_EP_TRACE_FUNC;
	set.se_paramValue = (II_PTR)gcd_api_trace;

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;

	if( api_ver >= IIAPI_VERSION_6 )
	{
	    set.se_paramID = IIAPI_EP_DATE_ALIAS;
	    set.se_paramValue = IIAPI_EPV_INGDATE;

	    IIapi_setEnvParam( &set );
	    if ( (status = set.se_status) != OK )  break;
	}

	set.se_paramID = IIAPI_EP_DATE_FORMAT;
	value = IIAPI_EPV_DFRMT_FINNISH;
	set.se_paramValue = (II_PTR)&value;

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;

	set.se_paramID = IIAPI_EP_TIMEZONE;
	set.se_paramValue = "gmt";

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;

	set.se_paramID = IIAPI_EP_MAX_SEGMENT_LEN;
	value = 8192;
	set.se_paramValue = (II_PTR)&value;

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;
   
	set.se_paramID = IIAPI_EP_DECIMAL_CHAR;
	set.se_paramValue  = ".";

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;

	set.se_paramID = IIAPI_EP_MONEY_SIGN ;
	set.se_paramValue  = "$";

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;

	set.se_paramID = IIAPI_EP_MONEY_LORT ;
	value = IIAPI_EPV_MONEY_LEAD_SIGN;
	set.se_paramValue  =  (II_PTR)&value;

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;

	set.se_paramID = IIAPI_EP_MONEY_PRECISION ;
	value = 2;
	set.se_paramValue  =  (II_PTR)&value;

	IIapi_setEnvParam( &set );
	if ( (status = set.se_status) != OK )  break;
    }

    if ( status != OK )
	gcu_erlog( 0, GCD_global.language, status, NULL, 0, NULL );

    return( status );
}


/*
** Name: gcd_api_term
**
** Description:
**	Release the API environment handle. Terminiate GCD API interface.
**
** Input:
**	ehndl	API environment handle. 
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906
**	    Take environment handle as input.
*/

void
gcd_api_term( PTR ehndl )
{
    IIAPI_TERMPARM	term;

    if( ehndl )
    {
	IIAPI_RELENVPARM	rel;

	rel.re_envHandle = ehndl;
	IIapi_releaseEnv( &rel );

	if ( rel.re_status != IIAPI_ST_SUCCESS  &&
	     GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD Error releasing API environment: %d\n", 
		       -1, rel.re_status );
    }

    IIapi_terminate( &term );

    if ( term.tm_status != IIAPI_ST_SUCCESS && 
	 GCD_global.gcd_trace_level >= 1 &&
	 term.tm_status != IIAPI_ST_WARNING )
	TRdisplay( "%4d    GCD Error shutting down API: %d\n", 
		   -1, term.tm_status );

    return;
}


/*
** Name: gcd_push_callback
**
** Description:
**	Push a callback function on a PCB callback stack.
**	The function will be called when a corresponding
**	call to gcd_pop_callback() is made.  The function
**	should take a single PTR argument and return void.
**	The function will be passed the PCB when called.
**
** Input:
**	pcb		Parameter control block.
**	callback	Callback function.
**
** Output:
**	None.
**
** Returns:
**	bool		TRUE if successful, FALSE if stack overflow.
**
** History:
**	11-Oct-00 (gordy)
**	    Created.
*/

bool
gcd_push_callback( GCD_PCB *pcb, GCD_PCB_CALLBACK callback )
{
    bool success = FALSE;

    if ( pcb->callbacks < GCD_PCB_STACK )
    {
	pcb->callback[ pcb->callbacks++ ] = callback;
	success = TRUE;
    }
    else if ( GCD_global.gcd_trace_level >= 1 )
	TRdisplay( "%4d    GCD PCB stack overflow\n", pcb->ccb->id );

    return( success );
}


/*
** Name: gcd_pop_callback
**
** Description:
**	Execute the function at the top of a PCB callback stack.
**	The PCB is passed as the only argument to the callback function.
**	The PCB is deleted if no callback function is on the stack.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	11-Oct-00 (gordy)
**	    Created.
*/

void
gcd_pop_callback( GCD_PCB *pcb )
{
    GCD_PCB_CALLBACK	callback = NULL;

    if ( pcb->callbacks )  callback = pcb->callback[ --pcb->callbacks ];

    if ( callback )
	(*callback)( (PTR)pcb );
    else
	gcd_del_pcb( pcb );
    
    return;
}


/*
** Name: gcd_api_setConnParms
**
** Description:
**	Set connection parameters.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	16-Sep-99 (rajus01)
**	    Created.
**	 6-Jan-00 (gordy)
**	    Connection data now stored in JDBC_CIB structure.
**	25-Mar-10 (gordy)
**	    Initial connection handle now stored in API parms
**	    to separate from active connection handle in CIB.
*/

void
gcd_api_setConnParms( GCD_PCB *pcb )
{
    u_i2 idx = --pcb->data.conn.param_cnt;

    pcb->api.name = "IIapi_setConnectParam";
    pcb->api.parm.scon.sc_genParm.gp_callback = gcd_sCon_cmpl;
    pcb->api.parm.scon.sc_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.scon.sc_connHandle = pcb->ccb->api.conn;
    pcb->api.parm.scon.sc_paramID = pcb->data.conn.parms[idx].id;
    pcb->api.parm.scon.sc_paramValue = pcb->data.conn.parms[idx].value;

    IIapi_setConnectParam( &pcb->api.parm.scon );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_sCon_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    IIAPI_SETCONPRMPARM	*setCon = (IIAPI_SETCONPRMPARM *)parm;

    pcb->ccb->api.conn = setCon->sc_connHandle;
    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_connect
**
** Description:
**	Connect to the DBMS.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 4-Nov-99 (gordy)
**	    Added support for connect timeouts.
**	 6-Jan-00 (gordy)
**	    Connection data now stored in JDBC_CIB structure.
**	18-Apr-01 (gordy)
**	    Use distributed transaction handle as initial connection
**	    transaction handle.
**	20-Mar-10 (gordy)
**	    Initial connection handle now stored in API parms
**	    to separate from active connection handle in CIB.
*/

void
gcd_api_connect( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_connect()";
    pcb->api.parm.conn.co_genParm.gp_callback = gcd_conn_cmpl;
    pcb->api.parm.conn.co_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.conn.co_type = IIAPI_CT_SQL;
    pcb->api.parm.conn.co_connHandle =  pcb->ccb->api.conn;
    pcb->api.parm.conn.co_tranHandle = pcb->ccb->xact.distXID;
    pcb->api.parm.conn.co_target = pcb->data.conn.database;
    pcb->api.parm.conn.co_username = pcb->data.conn.username;
    pcb->api.parm.conn.co_password = pcb->data.conn.password;
    pcb->api.parm.conn.co_timeout = pcb->data.conn.timeout;
    pcb->ccb->api.conn = NULL;

    IIapi_connect( &pcb->api.parm.conn );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_conn_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_CONNPARM	*conn = (IIAPI_CONNPARM *)parm;

    pcb->ccb->cib->conn = conn->co_connHandle;
    pcb->ccb->cib->tran = conn->co_tranHandle;
    pcb->ccb->cib->level = conn->co_apiLevel;
    gcd_gen_cmpl( arg, parm );
    return;
}



/*
** Name: gcd_api_disconnect
**
** Description:
**	Disconnect from the DBMS.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Connection handle to disconnect now passed in PCB
**	    to avoid race conditions with the CCB.
**	11-Oct-00 (gordy)
**	    Callback handling replaced with jdbc_pop_callback();
*/

void
gcd_api_disconnect( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_disconnect()";
    pcb->api.parm.disc.dc_genParm.gp_callback = gcd_disc_cmpl;
    pcb->api.parm.disc.dc_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.disc.dc_connHandle = pcb->data.disc.conn;

    IIapi_disconnect( &pcb->api.parm.disc );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_disc_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_DISCONNPARM	*disc = (IIAPI_DISCONNPARM *)parm;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = gcd_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );

    if ( pcb->api_error == E_AP0003_ACTIVE_TRANSACTIONS  ||
	 pcb->api_error == E_AP0004_ACTIVE_QUERIES  ||
	 pcb->api_error == E_AP0005_ACTIVE_EVENTS )
    {
	/*
	** Connection has not been cleaned up correctly.
	** Aborting the connection is a drastic action,
	** but we must free the resources to protect the
	** integrity of the GCD server.
	*/
	gcd_api_abort( pcb );
    }
    else
	gcd_pop_callback( pcb );

    return;
}


/*
** Name: gcd_api_abort
**
** Description:
**	Shutdown a session.  If session shutdown has already
**	been initiated, nothing more is done.  A callback 
**	function is called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	17-Jan-00 (gordy)
**	    Connection handle to abort now passed in PCB
**	    to avoid race conditions with the CCB.
**	21-Mar-01 (gordy)
**	    Release distributed XID when present.
*/

void
gcd_api_abort( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_abort()";
    pcb->api.parm.abort.ab_genParm.gp_callback = gcd_abort_cmpl;
    pcb->api.parm.abort.ab_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.abort.ab_connHandle = pcb->data.disc.conn;

    IIapi_abort( &pcb->api.parm.abort );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_abort_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;

    if ( pcb->ccb->xact.distXID )  gcd_api_relXID( pcb->ccb );
    gcd_gen_cmpl( arg, parm );

    return;
}


/*
** Name: gcd_api_savepoint
**
** Description:
**	Create a savepoint.  A callback function is called from 
**	the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	14-Feb-03 (gordy)
**	    Created.
*/

void
gcd_api_savepoint( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_savePoint()";
    pcb->api.parm.sp.sp_genParm.gp_callback = gcd_sp_cmpl;
    pcb->api.parm.sp.sp_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.sp.sp_tranHandle = pcb->ccb->cib->tran;
    pcb->api.parm.sp.sp_savePoint = pcb->ccb->xact.savepoints->name;

    IIapi_savePoint( &pcb->api.parm.sp );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_sp_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_SAVEPTPARM	*sp = (IIAPI_SAVEPTPARM *)parm;

    if ( ! API_ERROR( sp->sp_genParm.gp_status ) )
	pcb->ccb->xact.savepoints->hndl = sp->sp_savePointHandle;

    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_regXID
**
** Description:
**	Register a distributed transaction ID.  This function
**	completes synchronously, no callback is made.
**
** Input:
**	ccb		Connection control block.
**	tranID		OpenAPI transaction ID.
**
** Output:
**	ccb
**	  xact
**	    distXID	Transaction ID handle.
**
** Returns:
**	bool		TRUE if successful, FALSE otherwise.
**
** History:
**	12-Mar-01 (gordy)
**	    Created.
*/

bool
gcd_api_regXID( GCD_CCB *ccb, IIAPI_TRAN_ID *tranID )
{
    IIAPI_REGXIDPARM	parm;
    bool		success = FALSE;

    MEcopy( (PTR)tranID, sizeof( *tranID ), (PTR)&parm.rg_tranID );
    IIapi_registerXID( &parm );

    if ( ! API_ERROR( parm.rg_status ) )
    {
	ccb->xact.distXID = parm.rg_tranIdHandle;
	success = TRUE;
    }  
    else  if ( GCD_global.gcd_trace_level >= 1 )
    {
	TRdisplay( "%4d    GCD IIapi_registerXID() status: %s\n", ccb->id, 
		   gcu_lookup( apiMap, parm.rg_status ) );
    }

    return( success );
}


/*
** Name: gcd_api_relXID
**
** Description:
**	Release a distributed transaction ID.  This function
**	completes synchronously, no callback is made.
**
** Input:
**	ccb		Connection control block.
**
** Output:
**	ccb
**	  xact
**	    distXID	Transaction ID handle (cleared).
**
** Returns:
**	void
**
** History:
**	12-Mar-01 (gordy)
**	    Created.
*/

bool
gcd_api_relXID( GCD_CCB *ccb )
{
    IIAPI_RELXIDPARM	parm;

    parm.rl_tranIdHandle = ccb->xact.distXID;
    ccb->xact.distXID = NULL;
    IIapi_releaseXID( &parm );

    if ( API_ERROR( parm.rl_status )  &&  GCD_global.gcd_trace_level >= 1 )
	TRdisplay( "%4d    GCD IIapi_releaseXID() status: %s\n", ccb->id, 
		   gcu_lookup( apiMap, parm.rl_status ) );

    return( API_ERROR( parm.rl_status ) ? FALSE : TRUE );
}


/*
** Name: gcd_api_prepare
**
** Description:
**	Prepare a distributed transaction.  A callback function
**	is called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	12-Mar-01 (gordy)
**	    Created.
*/

void
gcd_api_prepare( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_prepareCommit()";
    pcb->api.parm.prep.pr_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.prep.pr_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.prep.pr_tranHandle = pcb->ccb->cib->tran;

    IIapi_prepareCommit( &pcb->api.parm.prep );
    return;
}


/*
** Name: gcd_api_commit
**
** Description:
**	Commit a transaction.  If the commit fails, an attempt to
**	rollback the transaction will be made.  A callback function
**	is called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	19-Oct-00 (gordy)
**	    Added completion routine to rollback on error.
**	21-Mar-01 (gordy)
**	    Release distributed XID when present.
**	 3-Jun-05 (gordy)
**	    Don't signal transaction abort after commit.
**	11-Oct-05 (gordy)
**	    Clear transaction abort flag after status check 
**	    (which sets the flag).
*/

void
gcd_api_commit( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_commit()";
    pcb->api.parm.commit.cm_genParm.gp_callback = gcd_commit_cmpl;
    pcb->api.parm.commit.cm_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.commit.cm_tranHandle = pcb->ccb->cib->tran;
    pcb->ccb->cib->tran = NULL;
    pcb->result.flags |= PCB_RSLT_XACT_END;

    IIapi_commit( &pcb->api.parm.commit );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_commit_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = gcd_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );

    /*
    ** A transaction abort indication is purely
    ** information after commit, so clear flag.
    */
    pcb->ccb->cib->flags &= ~GCD_XACT_ABORT;

    /*
    ** If commit failed due to DBMS error, transaction
    ** must be rolled back.  All other error conditions
    ** are unrecoverable.
    */
    if ( pcb->api_error != E_AP000B_COMMIT_FAILED )
    {
	if ( pcb->ccb->xact.distXID )  gcd_api_relXID( pcb->ccb );
	gcd_pop_callback( pcb );
    }
    else
    {
	/*
	** Restore the transaction handle and rollback.
	*/
	pcb->ccb->cib->tran = ((IIAPI_COMMITPARM *)parm)->cm_tranHandle;
	pcb->data.tran.savepoint = NULL;
	gcd_api_rollback( pcb );
    }

    return;
}


/*
** Name: gcd_api_rollback
**
** Description:
**	Rollback a transaction.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	21-Mar-01 (gordy)
**	    Release distributed XID when present.
**	14-Feb-03 (gordy)
**	    Support rollback to savepoint.
**	 3-Jun-05 (gordy)
**	    Don't signal transaction abort after rollback.
**	11-Oct-05 (gordy)
**	    Clear transaction abort flag after status check 
**	    (which sets the flag).
*/

void
gcd_api_rollback( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_rollback()";
    pcb->api.parm.roll.rb_genParm.gp_callback = gcd_roll_cmpl;
    pcb->api.parm.roll.rb_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.roll.rb_tranHandle = pcb->ccb->cib->tran;

    if ( pcb->data.tran.savepoint != NULL )
	pcb->api.parm.roll.rb_savePointHandle = pcb->data.tran.savepoint;
    else
    {
	pcb->api.parm.roll.rb_savePointHandle = NULL;
	pcb->ccb->cib->tran = NULL;
	pcb->result.flags |= PCB_RSLT_XACT_END;
    }

    IIapi_rollback( &pcb->api.parm.roll );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_roll_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = gcd_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );
    /*
    ** A transaction abort indication is purely
    ** information after rollback, so clear flag.
    */
    if ( ! pcb->api.parm.roll.rb_savePointHandle )
	pcb->ccb->cib->flags &= ~GCD_XACT_ABORT;

    if ( pcb->ccb->xact.distXID )  gcd_api_relXID( pcb->ccb );
    gcd_pop_callback( pcb );
    return;
}


/*
** Name: gcd_api_autocommit
**
** Description:
**	Enable or disable autocommit transaction mode.
**	A callback function is called from the PCB stack 
**	when complete.
**
** Input:
**	pcb	Parameter control block.
**	enable	TRUE to enable autocommit, FALSE to disable.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	19-Oct-00 (gordy)
**	    Don't set/reset the autocommit flag.  This flag is
**	    used to signal the desire for autocommit when no
**	    transaction handle is active.  It is the callers
**	    responsibility to set the autocommit flag correctly.
**	27-Oct-05 (gordy)
**	    Clear transaction abort indicator if autocommit state
**	    is successfully changed.
*/

void
gcd_api_autocommit( GCD_PCB *pcb, bool enable )
{
    pcb->api.name = "IIapi_autocommit()";
    pcb->api.parm.ac.ac_genParm.gp_callback = gcd_auto_cmpl;
    pcb->api.parm.ac.ac_genParm.gp_closure = (PTR)pcb;

    if ( enable )
    {
	pcb->api.parm.ac.ac_connHandle = pcb->ccb->cib->conn;
	pcb->api.parm.ac.ac_tranHandle = NULL;
    }
    else
    {
	pcb->api.parm.ac.ac_connHandle = NULL;
	pcb->api.parm.ac.ac_tranHandle = pcb->ccb->cib->tran;
	pcb->ccb->cib->tran = NULL;
    }

    IIapi_autocommit( &pcb->api.parm.ac );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_auto_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_AUTOPARM	*ac = (IIAPI_AUTOPARM *)parm;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = gcd_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );

    if ( ! pcb->api_error )  
    {
	/*
	** Since we successfully changed the autocommit state,
	** a transaction abort indication is no longer relevant.
	*/
	pcb->ccb->cib->flags &= ~GCD_XACT_ABORT;

	/*
	** Save autocommit transaction handle
	** (if we enabled autocommit).
	*/
        if ( ac->ac_connHandle )   
	    pcb->ccb->cib->tran = ac->ac_tranHandle;
    }
    
    gcd_pop_callback( pcb );
    return;
}


/*
** Name: gcd_api_xaStart
**
** Description:
**	Start an XA transaction.  A callback function
**	is called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	14-Jul-06 (gordy)
**	    Created.
*/

void
gcd_api_xaStart( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_xaStart()";
    pcb->api.parm.start.xs_genParm.gp_callback = gcd_start_cmpl;
    pcb->api.parm.start.xs_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.start.xs_connHandle =  pcb->ccb->cib->conn;
    pcb->api.parm.start.xs_flags = pcb->data.tran.xa_flags;
    STRUCT_ASSIGN_MACRO(pcb->data.tran.distXID, pcb->api.parm.start.xs_tranID);

    IIapi_xaStart( &pcb->api.parm.start );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_start_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_XASTARTPARM	*start = (IIAPI_XASTARTPARM *)parm;
    GCD_PCB		*pcb = (GCD_PCB *)arg;

    if ( ! API_ERROR( start->xs_genParm.gp_status ) )
	pcb->ccb->cib->tran = start->xs_tranHandle;

    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_xaEnd
**
** Description:
**	End association with an XA transaction.  Even if an error
**	occurs, the transaction is dropped since there is nothing
**	else which can be done.  A callback function is called 
**	from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	14-Jul-06 (gordy)
**	    Created.
*/

void
gcd_api_xaEnd( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_xaEnd()";
    pcb->api.parm.end.xe_genParm.gp_callback = gcd_end_cmpl;
    pcb->api.parm.end.xe_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.end.xe_connHandle =  pcb->ccb->cib->conn;
    pcb->api.parm.end.xe_flags = pcb->data.tran.xa_flags;
    STRUCT_ASSIGN_MACRO(pcb->data.tran.distXID, pcb->api.parm.end.xe_tranID);

    pcb->ccb->cib->tran = NULL;
    pcb->result.flags |= PCB_RSLT_XACT_END;

    IIapi_xaEnd( &pcb->api.parm.end );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_end_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = gcd_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );
    /*
    ** A transaction abort indication is purely
    ** information after xaEnd, so clear flag.
    */
    pcb->ccb->cib->flags &= ~GCD_XACT_ABORT;

    gcd_pop_callback( pcb );
    return;
}


/*
** Name: gcd_api_xaPrepare
**
** Description:
**	Prepare XA transaction.  A callback function is called 
**	from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	14-Jul-06 (gordy)
**	    Created.
*/

void
gcd_api_xaPrepare( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_xaPrepare()";
    pcb->api.parm.xaprep.xp_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.xaprep.xp_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.xaprep.xp_connHandle =  pcb->ccb->cib->conn;
    pcb->api.parm.xaprep.xp_flags = pcb->data.tran.xa_flags;
    STRUCT_ASSIGN_MACRO(pcb->data.tran.distXID, pcb->api.parm.xaprep.xp_tranID);

    IIapi_xaPrepare( &pcb->api.parm.xaprep );
    return;
}


/*
** Name: gcd_api_xaCommit
**
** Description:
**	Commit XA transaction.  A callback function is called 
**	from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	14-Jul-06 (gordy)
**	    Created.
*/

void
gcd_api_xaCommit( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_xaCommit()";
    pcb->api.parm.xacomm.xc_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.xacomm.xc_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.xacomm.xc_connHandle =  pcb->ccb->cib->conn;
    pcb->api.parm.xacomm.xc_flags = pcb->data.tran.xa_flags;
    STRUCT_ASSIGN_MACRO(pcb->data.tran.distXID, pcb->api.parm.xacomm.xc_tranID);

    IIapi_xaCommit( &pcb->api.parm.xacomm );
    return;
}


/*
** Name: gcd_api_xaRollback
**
** Description:
**	Rollback XA transaction.  A callback function is called 
**	from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	14-Jul-06 (gordy)
**	    Created.
*/

void
gcd_api_xaRollback( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_xaRollback()";
    pcb->api.parm.xaroll.xr_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.xaroll.xr_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.xaroll.xr_connHandle =  pcb->ccb->cib->conn;
    pcb->api.parm.xaroll.xr_flags = pcb->data.tran.xa_flags;
    STRUCT_ASSIGN_MACRO(pcb->data.tran.distXID, pcb->api.parm.xaroll.xr_tranID);

    IIapi_xaRollback( &pcb->api.parm.xaroll );
    return;
}


/*
** Name: gcd_api_query
**
** Description:
**	Execute a query.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	 6-Jan-00 (gordy)
**	    Connection data now stored in JDBC_CIB structure.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	10-Nov-06 (gordy)
**	    Query flags required at API Vesion 6.
*/

void
gcd_api_query( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_query()";
    pcb->api.parm.query.qy_genParm.gp_callback = gcd_qry_cmpl;
    pcb->api.parm.query.qy_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.query.qy_connHandle = pcb->ccb->cib->conn;
    pcb->api.parm.query.qy_tranHandle = pcb->ccb->cib->tran;
    pcb->api.parm.query.qy_stmtHandle = NULL;
    pcb->api.parm.query.qy_queryType = pcb->data.query.type;
    pcb->api.parm.query.qy_queryText = pcb->data.query.text;
    pcb->api.parm.query.qy_parameters = pcb->data.query.params;
    pcb->api.parm.query.qy_flags = pcb->data.query.flags;

    IIapi_query( &pcb->api.parm.query );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_qry_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_QUERYPARM	*query = (IIAPI_QUERYPARM *)parm;
    GCD_PCB		*pcb = (GCD_PCB *)arg;

    pcb->ccb->cib->tran = query->qy_tranHandle;
    pcb->ccb->api.stmt = query->qy_stmtHandle;
    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_batch
**
** Description:
**	Execute a batch query.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	25-Mar-10 (gordy)
**	    Created.
*/

void
gcd_api_batch( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_batch()";
    pcb->api.parm.batch.ba_genParm.gp_callback = gcd_bat_cmpl;
    pcb->api.parm.batch.ba_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.batch.ba_connHandle = pcb->ccb->cib->conn;
    pcb->api.parm.batch.ba_tranHandle = pcb->ccb->cib->tran;
    pcb->api.parm.batch.ba_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.batch.ba_queryType = pcb->data.query.type;
    pcb->api.parm.batch.ba_queryText = pcb->data.query.text;
    pcb->api.parm.batch.ba_parameters = pcb->data.query.params;
    pcb->api.parm.batch.ba_flags = pcb->data.query.flags;

    IIapi_batch( &pcb->api.parm.batch );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_bat_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_BATCHPARM	*batch = (IIAPI_BATCHPARM *)parm;
    GCD_PCB		*pcb = (GCD_PCB *)arg;

    pcb->ccb->cib->tran = batch->ba_tranHandle;
    pcb->ccb->api.stmt = batch->ba_stmtHandle;
    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_setDesc
**
** Description:
**	Set query parameter descriptor.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

void
gcd_api_setDesc( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_setDescriptor()";
    pcb->api.parm.sDesc.sd_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.sDesc.sd_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.sDesc.sd_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.sDesc.sd_descriptorCount = pcb->data.desc.count;
    pcb->api.parm.sDesc.sd_descriptor = pcb->data.desc.desc;

    IIapi_setDescriptor( &pcb->api.parm.sDesc );
    return;
}


/*
** Name: gcd_api_putParm
**
** Description:
**	Put query parameters.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Pass the more_segments flag through the API/MSG interface.
*/

void
gcd_api_putParm( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_putParms()";
    pcb->api.parm.pParm.pp_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.pParm.pp_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.pParm.pp_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.pParm.pp_parmCount = pcb->data.data.col_cnt;
    pcb->api.parm.pParm.pp_parmData = pcb->data.data.data;
    pcb->api.parm.pParm.pp_moreSegments = pcb->data.data.more_segments;

    IIapi_putParms( &pcb->api.parm.pParm );
    return;
}


/*
** Name: gcd_api_getDesc
**
** Description:
**	Get query descriptor.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

void
gcd_api_getDesc( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_getDescriptor()";
    pcb->api.parm.gDesc.gd_genParm.gp_callback = gcd_gDesc_cmpl;
    pcb->api.parm.gDesc.gd_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.gDesc.gd_stmtHandle = pcb->ccb->api.stmt;

    IIapi_getDescriptor( &pcb->api.parm.gDesc );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_gDesc_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_GETDESCRPARM	*gDesc = (IIAPI_GETDESCRPARM *)parm;
    GCD_PCB		*pcb = (GCD_PCB *)arg;

    if ( API_ERROR( gDesc->gd_genParm.gp_status )  ||
	 gDesc->gd_genParm.gp_status == IIAPI_ST_NO_DATA )
	pcb->data.desc.count = 0;
    else
    {
	pcb->data.desc.count = gDesc->gd_descriptorCount;
	pcb->data.desc.desc = gDesc->gd_descriptor;
    }

    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_position
**
** Description:
**	Position a cursor and initiate row fetch .  A callback 
**	function is called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 4-Apr-07 (gordy)
**	    Created.
*/

void
gcd_api_position( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_position()";
    pcb->api.parm.pos.po_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.pos.po_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.pos.po_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.pos.po_reference = pcb->data.data.reference;
    pcb->api.parm.pos.po_offset = pcb->data.data.offset;
    pcb->api.parm.pos.po_rowCount = pcb->data.data.max_row;

    IIapi_position( &pcb->api.parm.pos );
    return;
}


/*
** Name: gcd_api_getCol
**
** Description:
**	Read the next block of columns.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	29-Sep-99 (gordy)
**	    Pass the more_segments flag through the API/MSG interface.
**	13-Dec-99 (gordy)
**	    Added support for fetching multiple rows.
**	19-May-00 (gordy)
**	    Separate rows requested and rows received counters.
**	 6-Oct-03 (gordy)
**	    Removed total row count.  We are either working on a single
**	    row or a block of rows.  With a block of rows, the entire 
**	    block is retrieved with one request, so total row count 
**	    is always be zero and max rows is sufficient.
*/

void
gcd_api_getCol( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_getColumns()";
    pcb->api.parm.gCol.gc_genParm.gp_callback = gcd_gCol_cmpl;
    pcb->api.parm.gCol.gc_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.gCol.gc_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.gCol.gc_rowCount = pcb->data.data.max_row;
    pcb->api.parm.gCol.gc_columnCount = pcb->data.data.col_cnt;
    pcb->api.parm.gCol.gc_columnData = pcb->data.data.data;

    IIapi_getColumns( &pcb->api.parm.gCol );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_gCol_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_GETCOLPARM	*gcol = (IIAPI_GETCOLPARM *)parm;
    GCD_PCB		*pcb = (GCD_PCB *)arg;

    if ( API_ERROR( gcol->gc_genParm.gp_status )  ||
	 gcol->gc_genParm.gp_status == IIAPI_ST_NO_DATA )
    {
	pcb->data.data.row_cnt = 0;
	pcb->data.data.col_cnt = 0;
	pcb->data.data.more_segments = FALSE;
    }
    else
    {
	pcb->data.data.row_cnt = gcol->gc_rowsReturned;
	pcb->data.data.more_segments = gcol->gc_moreSegments;
    }

    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_gqi
**
** Description:
**	Get query result information.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	13-Dec-99 (gordy)
**	    Check for and flag readonly cursors.
**	25-Aug-00 (gordy)
**	    Save procedure return values.
**	15-Nov-00 (gordy)
**	    Return the cursor readonly flag rather than pre-fetch limit.
**	24-Feb-03 (gordy)
**	    Return table and object keys.
**	 4-Apr-07 (gordy)
**	    Check for EOD, scrollable cursor, and row status/position.
*/

void
gcd_api_gqi( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_getQueryInfo()";
    pcb->api.parm.gqi.gq_genParm.gp_callback = gcd_gqi_cmpl;
    pcb->api.parm.gqi.gq_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.gqi.gq_stmtHandle = pcb->ccb->api.stmt;
    IIapi_getQueryInfo( &pcb->api.parm.gqi );
    return;
}

II_FAR II_CALLBACK II_VOID
gcd_gqi_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_GETQINFOPARM	*gqi = (IIAPI_GETQINFOPARM *)parm;
    GCD_PCB		*pcb = (GCD_PCB *)arg;

    if ( ! API_ERROR( gqi->gq_genParm.gp_status ) )
    {
	if ( gqi->gq_flags & IIAPI_GQF_END_OF_DATA )
	    pcb->result.flags |= PCB_RSLT_EOD;

	if ( gqi->gq_mask & IIAPI_GQ_CURSOR )
	{
	    if ( ! (gqi->gq_cursorType & IIAPI_CURSOR_UPDATE) )  
		pcb->result.flags |= PCB_RSLT_READ_ONLY;
	    if ( gqi->gq_cursorType & IIAPI_CURSOR_SCROLL )
	    	pcb->result.flags |= PCB_RSLT_SCROLL;
	}

	if ( gqi->gq_mask & IIAPI_GQ_ROW_STATUS )
	{
	    pcb->result.flags |= PCB_RSLT_ROW_STATUS;
	    pcb->result.row_status = (u_i2)gqi->gq_rowStatus;
	    pcb->result.row_position = gqi->gq_rowPosition;
	}

	if ( gqi->gq_mask & IIAPI_GQ_ROW_COUNT )
	{
	    pcb->result.flags |= PCB_RSLT_ROW_COUNT;
	    pcb->result.row_count = gqi->gq_rowCount;
	}

	if ( gqi->gq_mask & IIAPI_GQ_PROCEDURE_RET )
	{
	    pcb->result.flags |= PCB_RSLT_PROC_RET;
	    pcb->result.proc_ret = gqi->gq_procedureReturn;
	}

	if ( gqi->gq_mask & IIAPI_GQ_TABLE_KEY )
	{
	    pcb->result.flags |= PCB_RSLT_TBLKEY;
	    MEcopy( (PTR)gqi->gq_tableKey, sizeof( pcb->result.tblkey ), 
		    (PTR)pcb->result.tblkey );
	}

	if ( gqi->gq_mask & IIAPI_GQ_OBJECT_KEY )
	{
	    pcb->result.flags |= PCB_RSLT_OBJKEY;
	    MEcopy( (PTR)gqi->gq_objectKey, sizeof( pcb->result.objkey ),
		    (PTR)pcb->result.objkey );
	}
    }

    gcd_gen_cmpl( arg, parm );
    return;
}


/*
** Name: gcd_api_cancel
**
** Description:
**	Cancel the current query.  A callback function is
**	called from the PCB stack when complete.
**
** Input:
**	pcb		Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	11-Oct-00 (gordy)
**	    Callback handling replaced with jdbc_pop_callback();
*/

void
gcd_api_cancel( GCD_PCB *pcb )
{
    pcb->api.name = "IIapi_cancel()";
    pcb->api.parm.cancel.cn_genParm.gp_callback = gcd_cncl_cmpl;
    pcb->api.parm.cancel.cn_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.cancel.cn_stmtHandle = pcb->ccb->api.stmt;

    if ( pcb->api.parm.cancel.cn_stmtHandle )
	IIapi_cancel( &pcb->api.parm.cancel );
    else
    {
	/*
	** There was no statement to cancel.
	** No error is returned in this case.
	*/
	pcb->api_status = IIAPI_ST_SUCCESS;
	pcb->api_error = OK;
	gcd_pop_callback( pcb );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
gcd_cncl_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = gcd_api_status( pcb->api.name, gp, pcb->ccb, NULL );
    gcd_pop_callback( pcb );

    return;
}


/*
** Name: gcd_api_close
**
** Description:
**	Close an API statement handle.  A specific statement
**	handle may be provided.  If NULL is provided, the 
**	current query statement handle is closed.  A callback 
**	function is called from the PCB stack when complete.
**
** Input:
**	pcb		Parameter control block.
**	stmtHandle	API statement handle, may be NULL.
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	11-Oct-00 (gordy)
**	    Callback handling replaced with jdbc_pop_callback();
*/

void
gcd_api_close( GCD_PCB *pcb, PTR stmtHandle )
{
    if ( ! stmtHandle )		
    {
	/* Close the current active statement. */
	stmtHandle = pcb->ccb->api.stmt;
	pcb->ccb->api.stmt = NULL;
    }

    pcb->api.name = "IIapi_close()";
    pcb->api.parm.close.cl_genParm.gp_callback = gcd_gen_cmpl;
    pcb->api.parm.close.cl_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.close.cl_stmtHandle = stmtHandle;

    if ( pcb->api.parm.close.cl_stmtHandle )
	IIapi_close( &pcb->api.parm.close );
    else
    {
	/*
	** There was no statement to close.
	** No error is returned in this case.
	*/
	pcb->api_status = IIAPI_ST_SUCCESS;
	pcb->api_error = OK;
	gcd_pop_callback( pcb );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
gcd_gen_cmpl( II_PTR arg, II_PTR parm )
{
    GCD_PCB		*pcb = (GCD_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = gcd_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );
    gcd_pop_callback( pcb );
    return;
}


/*
** Name: gcd_api_format
**
** Description:
**	Format/convert data values based on connection parameters.
**
** Input:
**	ccb	Connection control block.
**	sdesc	Source descriptor.
**	sdata	Source data value.
**
** Output:
**	ddesc	Destination descriptor.
**	ddata	Destination data value.
**
** Returns:
**	STATUS	OK or error code.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
*/

STATUS
gcd_api_format
( 
    GCD_CCB		*ccb,
    IIAPI_DESCRIPTOR	*sdesc,
    IIAPI_DATAVALUE	*sdata,
    IIAPI_DESCRIPTOR	*ddesc,
    IIAPI_DATAVALUE	*ddata
)
{
    IIAPI_FORMATPARM	fd;

    fd.fd_envHandle = ccb->api.env;
    fd.fd_srcDesc = *sdesc;
    fd.fd_srcValue = *sdata;
    fd.fd_dstDesc = *ddesc;
    fd.fd_dstValue = *ddata;

    IIapi_formatData( &fd );
    return( fd.fd_status );
}


/*
** Name: gcd_api_status
**
** Description:
**
** Input:
**	func_name   Name of function producing status.
**	genParm	    API general parameters.
**	ccb	    Connection control block.
**	rcb_ptr	    Request control block (may be NULL).
**
** Output:
**	rcb_ptr	    New request control block.
**
** Returns:
**	STATUS	    OK or error code.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	30-Apr-01 (gordy)
**	    Make sure and error code is returned when a DBMS error
**	    is received.  Some older gateways (VSAM) don't return
**	    a host error code, only a generic error code.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Check for API logical sequence errors.
**	14-Jul-06 (gordy)
**	    Added support for XA errors.
*/

static STATUS
gcd_api_status( char *func_name, IIAPI_GENPARM *genParm, 
		 GCD_CCB *ccb, GCD_RCB **rcb_ptr )
{
    IIAPI_STATUS	api_status = genParm->gp_status;
    bool		api_error = API_ERROR( genParm->gp_status );
    bool		dbms_error = FALSE;
    STATUS		status = OK;

    if ( (api_error  &&  GCD_global.gcd_trace_level >= 1)  ||
	 GCD_global.gcd_trace_level >= 2 )
	TRdisplay( "%4d    GCD %s status: %s\n", ccb->id, func_name, 
		   gcu_lookup( apiMap, genParm->gp_status ) );

    if ( genParm->gp_errorHandle )
    {
	IIAPI_GETEINFOPARM	get;

	get.ge_errorHandle = genParm->gp_errorHandle;
	IIapi_getErrorInfo( &get );

	while( get.ge_status == IIAPI_ST_SUCCESS )
	{
	    u_i2 msg_len = get.ge_message ? (u_i2)STlength(get.ge_message) : 0;

	    switch( get.ge_type )
	    {
	    case IIAPI_GE_ERROR   : 
		switch( get.ge_errorCode )
		{
		case E_AP0001_CONNECTION_ABORTED :
		    ccb->cib->flags |= GCD_CONN_ABORT;
		    break;

		case E_AP0002_TRANSACTION_ABORTED : 
		    ccb->cib->flags |= GCD_XACT_ABORT;
		    break;

		case E_AP0003_ACTIVE_TRANSACTIONS : 
		case E_AP0004_ACTIVE_QUERIES : 
		case E_AP0006_INVALID_SEQUENCE : 
		    ccb->cib->flags |= GCD_LOGIC_ERR;
		    break;
		}

		if ( ! dbms_error )
		{
		    status = get.ge_errorCode;
		    dbms_error = TRUE;
		}

		if ( GCD_global.gcd_trace_level >= 2 )
		    TRdisplay( "%4d    GCD Error 0x%x '%s'\n", ccb->id, 
			       get.ge_errorCode, get.ge_SQLSTATE );
		if ( GCD_global.gcd_trace_level >= 3  &&  get.ge_message )
		    TRdisplay( "%4d    GCD     %s\n", ccb->id, get.ge_message );

		if ( rcb_ptr )
		    gcd_send_error( ccb, rcb_ptr, MSG_ET_ERR, get.ge_errorCode,
				    get.ge_SQLSTATE, msg_len, get.ge_message );
		break;


	    case IIAPI_GE_XAERR :
		if ( GCD_global.gcd_trace_level >= 2 )
		    TRdisplay( "%4d    GCD XA Error %d\n", ccb->id, 
			       get.ge_errorCode );

		if ( ! dbms_error )
		{
		    status = FAIL;
		    dbms_error = TRUE;
		}

		if ( rcb_ptr )
		    gcd_send_error( ccb, rcb_ptr, MSG_ET_XA, get.ge_errorCode,
				    get.ge_SQLSTATE, msg_len, get.ge_message );
	        break;

	    case IIAPI_GE_WARNING : 
		if ( GCD_global.gcd_trace_level >= 2 )
		    TRdisplay( "%4d    GCD Warning 0x%x '%s'\n", 
		    	       ccb->id, get.ge_errorCode, get.ge_SQLSTATE );
		if ( GCD_global.gcd_trace_level >= 3  &&  get.ge_message )
		    TRdisplay( "%4d    GCD     %s\n", ccb->id, get.ge_message );

		if ( rcb_ptr )
		    gcd_send_error( ccb, rcb_ptr, MSG_ET_WRN, get.ge_errorCode,
				    get.ge_SQLSTATE, msg_len, get.ge_message );
		break;

	    default :
		if ( GCD_global.gcd_trace_level >= 2 )
		    TRdisplay( "%4d    GCD User Message 0x%x\n", 
		    	       ccb->id, get.ge_errorCode );
		if ( GCD_global.gcd_trace_level >= 3  &&  get.ge_message )
		    TRdisplay( "%4d    GCD     %s\n", ccb->id, get.ge_message );

		if ( rcb_ptr )
		    gcd_send_error( ccb, rcb_ptr, MSG_ET_MSG, get.ge_errorCode,
				    get.ge_SQLSTATE, msg_len, get.ge_message );
		break;
	    }

	    IIapi_getErrorInfo( &get );
	}
    }

    if ( dbms_error  &&  ! status )  
	status = E_GC4809_INTERNAL_ERROR;
    else  if ( api_error  &&  ! dbms_error )
    {
	status = (api_status == IIAPI_ST_OUT_OF_MEMORY)
		 ? E_GC4808_NO_MEMORY : E_GC4809_INTERNAL_ERROR;
	if ( rcb_ptr )  gcd_sess_error( ccb, rcb_ptr, status );
    }

    return( status );
}


/*
** Name: gcd_api_trace
**
** Description:
**	API callback function for trace messages.
**
** Input:
**	parm	API trace parameter block.
**
** Output:
**	None.
**
** Returns:
**	II_VOID
**
** History:
**	21-Apr-06 (gordy)
**	    Support trace message with API callback function.
*/

static II_VOID
gcd_api_trace( IIAPI_TRACEPARM *parm )
{
    GCD_CCB	*ccb;

    if ( GCD_global.gcd_trace_level >= 2 )
	TRdisplay( "%4d    GCD trace message received\n", -1 );

    /*
    ** Remove trailing line terminators.
    */
    while( parm->tr_length  &&  
           ( parm->tr_message[ parm->tr_length - 1 ] == '\n'  ||
	     parm->tr_message[ parm->tr_length - 1 ] == '\r' ) )
	parm->tr_length--;

    if ( (ccb = gcd_sess_find( parm->tr_connHandle )) )
    	gcd_send_trace( ccb, parm->tr_length, parm->tr_message );

    return;
}



