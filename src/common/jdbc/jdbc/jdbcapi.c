/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <st.h>
#include <tr.h>
#include <me.h>

#include <iicommon.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcapi.c
**
** Description:
**	JDBC server interface to API.
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
**	 7-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
*/	

static GCULIST	errMap[] = 
{
    { JDBC_ET_MSG, "USER_MESSAGE" },
    { JDBC_ET_WRN, "WARNING" },
    { JDBC_ET_ERR, "ERROR" },
    { 0, NULL }
};

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

static	STATUS	jdbc_api_status( char *, IIAPI_GENPARM *, 
				 JDBC_CCB *, JDBC_RCB ** );

II_FAR II_CALLBACK II_VOID	jdbc_sCon_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_conn_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_disc_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_abort_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_commit_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_roll_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_auto_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_qry_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_gDesc_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_gCol_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_gqi_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_cncl_cmpl( II_PTR, II_PTR );
II_FAR II_CALLBACK II_VOID	jdbc_gen_cmpl( II_PTR, II_PTR );


/*
** Name: jdbc_api_init
**
** Description:
**	Initialize the JDBC API interface.
**
** Input:
**	None.
**
** Output:
**	None.
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
*/

STATUS
jdbc_api_init( void )
{
    IIAPI_INITPARM	init;
    IIAPI_SETENVPRMPARM	set;
    bool		done;
    STATUS		status = OK;

    init.in_timeout = -1;
    init.in_version = IIAPI_VERSION_3;

    IIapi_initialize( &init );

    if ( init.in_status != IIAPI_ST_SUCCESS  &&
	 JDBC_global.trace_level >= 1 )
	TRdisplay( "%4d    JDBC Error initializing API: %s\n", 
		   -1, gcu_lookup( apiMap, init.in_status ) );

    switch( init.in_status )
    {
    case IIAPI_ST_SUCCESS :
	JDBC_global.api_env = init.in_envHandle;
	break;
    
    case IIAPI_ST_OUT_OF_MEMORY :
	status = E_JD0108_NO_MEMORY;
	break;
    
    default :
	status = E_JD0109_INTERNAL_ERROR;
	break;
    }

    if ( status == OK )
	for( done = FALSE; ! done; done = TRUE )
	{
	    II_LONG	value;

	    set.se_envHandle = JDBC_global.api_env;
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
	gcu_erlog( 0, JDBC_global.language, status, NULL, 0, NULL );

    return( status );
}


/*
** Name: jdbc_api_term
**
** Description:
**	Terminate the JDBC API interface.
**
** Input:
**	None.
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
jdbc_api_term( void )
{
    if ( JDBC_global.api_env )
    {
	IIAPI_RELENVPARM	rel;
	IIAPI_TERMPARM		term;

	rel.re_envHandle = JDBC_global.api_env;
	JDBC_global.api_env = NULL;

	IIapi_releaseEnv( &rel );

	if ( rel.re_status != IIAPI_ST_SUCCESS  &&
	     JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC Error releasing API environment: %d\n", 
		       -1, rel.re_status );

	IIapi_terminate( &term );

	if ( term.tm_status != IIAPI_ST_SUCCESS  &&
	     JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC Error shutting down API: %d\n", 
		       -1, rel.re_status );
    }

    return;
}


/*
** Name: jdbc_push_callback
**
** Description:
**	Push a callback function on a PCB callback stack.
**	The function will be called when a corresponding
**	call to jdbc_pop_callback() is made.  The function
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
jdbc_push_callback( JDBC_PCB *pcb, JDBC_PCB_CALLBACK callback )
{
    bool success = FALSE;

    if ( pcb->callbacks < JDBC_PCB_STACK )
    {
	pcb->callback[ pcb->callbacks++ ] = callback;
	success = TRUE;
    }
    else if ( JDBC_global.trace_level >= 1 )
	TRdisplay( "%4d    JDBC PCB stack overflow\n", pcb->ccb->id );

    return( success );
}


/*
** Name: jdbc_pop_callback
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
jdbc_pop_callback( JDBC_PCB *pcb )
{
    JDBC_PCB_CALLBACK	callback = NULL;

    if ( pcb->callbacks )  callback = pcb->callback[ --pcb->callbacks ];

    if ( callback )
	(*callback)( (PTR)pcb );
    else
	jdbc_del_pcb( pcb );
    
    return;
}


/*
** Name: jdbc_api_setConnParms
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
*/

void
jdbc_api_setConnParms( JDBC_PCB *pcb )
{
    u_i2 idx = --pcb->data.conn.param_cnt;

    pcb->api.name = "IIapi_setConnectParam";
    pcb->api.parm.scon.sc_genParm.gp_callback = jdbc_sCon_cmpl;
    pcb->api.parm.scon.sc_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.scon.sc_connHandle = pcb->ccb->cib->conn;
    pcb->api.parm.scon.sc_paramID = pcb->data.conn.parms[idx].id;
    pcb->api.parm.scon.sc_paramValue = pcb->data.conn.parms[idx].value;

    IIapi_setConnectParam( &pcb->api.parm.scon );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_sCon_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    IIAPI_SETCONPRMPARM	*setCon = (IIAPI_SETCONPRMPARM *)parm;

    pcb->ccb->cib->conn = setCon->sc_connHandle;
    jdbc_gen_cmpl( arg, parm );
    return;
}


/*
** Name: jdbc_api_connect
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
*/

void
jdbc_api_connect( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_connect()";
    pcb->api.parm.conn.co_genParm.gp_callback = jdbc_conn_cmpl;
    pcb->api.parm.conn.co_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.conn.co_type = IIAPI_CT_SQL;
    pcb->api.parm.conn.co_connHandle =  pcb->ccb->cib->conn;
    pcb->api.parm.conn.co_tranHandle = pcb->ccb->xact.distXID;
    pcb->api.parm.conn.co_target = pcb->data.conn.database;
    pcb->api.parm.conn.co_username = pcb->data.conn.username;
    pcb->api.parm.conn.co_password = pcb->data.conn.password;
    pcb->api.parm.conn.co_timeout = pcb->data.conn.timeout;

    IIapi_connect( &pcb->api.parm.conn );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_conn_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    IIAPI_CONNPARM	*conn = (IIAPI_CONNPARM *)parm;

    pcb->ccb->cib->conn = conn->co_connHandle;
    pcb->ccb->cib->tran = conn->co_tranHandle;
    pcb->ccb->cib->level = conn->co_apiLevel;
    jdbc_gen_cmpl( arg, parm );
    return;
}



/*
** Name: jdbc_api_disconnect
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
jdbc_api_disconnect( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_disconnect()";
    pcb->api.parm.disc.dc_genParm.gp_callback = jdbc_disc_cmpl;
    pcb->api.parm.disc.dc_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.disc.dc_connHandle = pcb->data.disc.conn;

    IIapi_disconnect( &pcb->api.parm.disc );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_disc_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    IIAPI_DISCONNPARM	*disc = (IIAPI_DISCONNPARM *)parm;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = jdbc_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );

    if ( pcb->api_error == E_AP0003_ACTIVE_TRANSACTIONS  ||
	 pcb->api_error == E_AP0004_ACTIVE_QUERIES  ||
	 pcb->api_error == E_AP0005_ACTIVE_EVENTS )
    {
	/*
	** Connection has not been cleaned up correctly.
	** Aborting the connection is a drastic action,
	** but we must free the resources to protect the
	** integrity of the JDBC server.
	*/
	jdbc_api_abort( pcb );
    }
    else
	jdbc_pop_callback( pcb );

    return;
}


/*
** Name: jdbc_api_abort
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
jdbc_api_abort( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_abort()";
    pcb->api.parm.abort.ab_genParm.gp_callback = jdbc_abort_cmpl;
    pcb->api.parm.abort.ab_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.abort.ab_connHandle = pcb->data.disc.conn;

    IIapi_abort( &pcb->api.parm.abort );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_abort_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;

    if ( pcb->ccb->xact.distXID )  jdbc_api_relXID( pcb->ccb );
    jdbc_gen_cmpl( arg, parm );

    return;
}


/*
** Name: jdbc_api_regXID
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
jdbc_api_regXID( JDBC_CCB *ccb, IIAPI_TRAN_ID *tranID )
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
    else  if ( JDBC_global.trace_level >= 1 )
    {
	TRdisplay( "%4d    JDBC IIapi_registerXID() status: %s\n", ccb->id, 
		   gcu_lookup( apiMap, parm.rg_status ) );
    }

    return( success );
}


/*
** Name: jdbc_api_relXID
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
jdbc_api_relXID( JDBC_CCB *ccb )
{
    IIAPI_RELXIDPARM	parm;

    parm.rl_tranIdHandle = ccb->xact.distXID;
    ccb->xact.distXID = NULL;
    IIapi_releaseXID( &parm );

    if ( API_ERROR( parm.rl_status )  &&  JDBC_global.trace_level >= 1 )
	TRdisplay( "%4d    JDBC IIapi_releaseXID() status: %s\n", ccb->id, 
		   gcu_lookup( apiMap, parm.rl_status ) );

    return( API_ERROR( parm.rl_status ) ? FALSE : TRUE );
}


/*
** Name: jdbc_api_prepare
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
jdbc_api_prepare( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_prepareCommit()";
    pcb->api.parm.prep.pr_genParm.gp_callback = jdbc_gen_cmpl;
    pcb->api.parm.prep.pr_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.prep.pr_tranHandle = pcb->ccb->cib->tran;

    IIapi_prepareCommit( &pcb->api.parm.prep );
    return;
}


/*
** Name: jdbc_api_commit
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
**	 7-Jun-05 (gordy)
**	    Only rollback after commit fails due to DBMS
**	    transaction problem.  In all other cases,
**	    the resources have been freed.  Also, don't
**	    signal transaction abort after commit since 
**	    it is just an informational indication. The 
**	    transaction resources have been freed.
**	31-Oct-05 (gordy)
**	    Transaction flag needs to be cleared after checking
**	    request status (where it is set).
*/

void
jdbc_api_commit( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_commit()";
    pcb->api.parm.commit.cm_genParm.gp_callback = jdbc_commit_cmpl;
    pcb->api.parm.commit.cm_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.commit.cm_tranHandle = pcb->ccb->cib->tran;
    pcb->ccb->cib->tran = NULL;
    pcb->result.flags |= PCB_RSLT_XACT_END;

    IIapi_commit( &pcb->api.parm.commit );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_commit_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    IIAPI_COMMITPARM	*commit = (IIAPI_COMMITPARM *)parm;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = jdbc_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );

    /*
    ** Transaction abort indicator is purely informational after a commit.
    */
    pcb->ccb->cib->xact_abort = FALSE;

    /*
    ** If commit failed due to DBMS error, transaction
    ** must be rolled back.  All other error conditions
    ** are unrecoverable.
    */
    if ( pcb->api_error != E_AP000B_COMMIT_FAILED )
    {
	if ( pcb->ccb->xact.distXID )  jdbc_api_relXID( pcb->ccb );
	jdbc_pop_callback( pcb );
    }
    else
    {
	pcb->ccb->cib->tran = commit->cm_tranHandle;
	jdbc_api_rollback( pcb );
    }

    return;
}


/*
** Name: jdbc_api_rollback
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
**	 7-Jun-05 (gordy)
**	    Don't signal transaction abort after rollback
**	    since it is just an informational indication.
**	    The transaction resources have been freed.
**	31-Oct-05 (gordy)
**	    Transaction flag needs to be cleared after checking
**	    request status (where it is set).
*/

void
jdbc_api_rollback( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_rollback()";
    pcb->api.parm.roll.rb_genParm.gp_callback = jdbc_roll_cmpl;
    pcb->api.parm.roll.rb_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.roll.rb_savePointHandle = NULL;
    pcb->api.parm.roll.rb_tranHandle = pcb->ccb->cib->tran;
    pcb->ccb->cib->tran = NULL;
    pcb->result.flags |= PCB_RSLT_XACT_END;

    IIapi_rollback( &pcb->api.parm.roll );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_roll_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    if ( pcb->ccb->xact.distXID )  jdbc_api_relXID( pcb->ccb );
    pcb->api_status = gp->gp_status;
    pcb->api_error = jdbc_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );

    /*
    ** Transaction abort indicator is purely informational after rollback.
    */
    pcb->ccb->cib->xact_abort = FALSE;

    jdbc_pop_callback( pcb );
    return;
}


/*
** Name: jdbc_api_autocommit
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
*/

void
jdbc_api_autocommit( JDBC_PCB *pcb, bool enable )
{
    pcb->api.name = "IIapi_autocommit()";
    pcb->api.parm.ac.ac_genParm.gp_callback = jdbc_auto_cmpl;
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
jdbc_auto_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    IIAPI_AUTOPARM	*ac = (IIAPI_AUTOPARM *)parm;

    if ( ac->ac_connHandle  &&  ! API_ERROR( ac->ac_genParm.gp_status ) )
	pcb->ccb->cib->tran = ac->ac_tranHandle;

    jdbc_gen_cmpl( arg, parm );
    return;
}


/*
** Name: jdbc_api_query
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
*/

void
jdbc_api_query( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_query()";
    pcb->api.parm.query.qy_genParm.gp_callback = jdbc_qry_cmpl;
    pcb->api.parm.query.qy_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.query.qy_connHandle = pcb->ccb->cib->conn;
    pcb->api.parm.query.qy_tranHandle = pcb->ccb->cib->tran;
    pcb->api.parm.query.qy_stmtHandle = NULL;
    pcb->api.parm.query.qy_queryType = pcb->data.query.type;
    pcb->api.parm.query.qy_queryText = pcb->data.query.text;
    pcb->api.parm.query.qy_parameters = pcb->data.query.params;

    IIapi_query( &pcb->api.parm.query );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_qry_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_QUERYPARM	*query = (IIAPI_QUERYPARM *)parm;
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;

    pcb->ccb->cib->tran = query->qy_tranHandle;
    pcb->ccb->api.stmt = query->qy_stmtHandle;
    jdbc_gen_cmpl( arg, parm );
    return;
}


/*
** Name: jdbc_api_setDesc
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
jdbc_api_setDesc( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_setDescriptor()";
    pcb->api.parm.sDesc.sd_genParm.gp_callback = jdbc_gen_cmpl;
    pcb->api.parm.sDesc.sd_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.sDesc.sd_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.sDesc.sd_descriptorCount = pcb->data.desc.count;
    pcb->api.parm.sDesc.sd_descriptor = pcb->data.desc.desc;

    IIapi_setDescriptor( &pcb->api.parm.sDesc );
    return;
}


/*
** Name: jdbc_api_putParm
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
jdbc_api_putParm( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_putParms()";
    pcb->api.parm.pParm.pp_genParm.gp_callback = jdbc_gen_cmpl;
    pcb->api.parm.pParm.pp_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.pParm.pp_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.pParm.pp_parmCount = pcb->data.data.col_cnt;
    pcb->api.parm.pParm.pp_parmData = pcb->data.data.data;
    pcb->api.parm.pParm.pp_moreSegments = pcb->data.data.more_segments;

    IIapi_putParms( &pcb->api.parm.pParm );
    return;
}


/*
** Name: jdbc_api_getDesc
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
jdbc_api_getDesc( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_getDescriptor()";
    pcb->api.parm.gDesc.gd_genParm.gp_callback = jdbc_gDesc_cmpl;
    pcb->api.parm.gDesc.gd_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.gDesc.gd_stmtHandle = pcb->ccb->api.stmt;

    IIapi_getDescriptor( &pcb->api.parm.gDesc );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_gDesc_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_GETDESCRPARM	*gDesc = (IIAPI_GETDESCRPARM *)parm;
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;

    if ( API_ERROR( gDesc->gd_genParm.gp_status )  ||
	 gDesc->gd_genParm.gp_status == IIAPI_ST_NO_DATA )
	pcb->data.desc.count = 0;
    else
    {
	pcb->data.desc.count = gDesc->gd_descriptorCount;
	pcb->data.desc.desc = gDesc->gd_descriptor;
    }

    jdbc_gen_cmpl( arg, parm );
    return;
}


/*
** Name: jdbc_api_gqi
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
*/

void
jdbc_api_gqi( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_getQueryInfo()";
    pcb->api.parm.gqi.gq_genParm.gp_callback = jdbc_gqi_cmpl;
    pcb->api.parm.gqi.gq_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.gqi.gq_stmtHandle = pcb->ccb->api.stmt;

    IIapi_getQueryInfo( &pcb->api.parm.gqi );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_gqi_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_GETQINFOPARM	*gqi = (IIAPI_GETQINFOPARM *)parm;
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;

    if ( ! API_ERROR( gqi->gq_genParm.gp_status ) )
    {
	if ( gqi->gq_mask & IIAPI_GQ_CURSOR  &&  gqi->gq_readonly )
	    pcb->result.flags |= PCB_RSLT_READ_ONLY;

	if ( gqi->gq_mask & IIAPI_GQ_ROW_COUNT )
	{
	    pcb->result.flags |= PCB_RSLT_ROWCOUNT;
	    pcb->result.rowcount = gqi->gq_rowCount;
	}

	if ( gqi->gq_mask & IIAPI_GQ_PROCEDURE_RET )
	{
	    pcb->result.flags |= PCB_RSLT_PROC_RET;
	    pcb->result.proc_ret = gqi->gq_procedureReturn;
	}
    }

    jdbc_gen_cmpl( arg, parm );
    return;
}


/*
** Name: jdbc_api_getCol
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
*/

void
jdbc_api_getCol( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_getColumns()";
    pcb->api.parm.gCol.gc_genParm.gp_callback = jdbc_gCol_cmpl;
    pcb->api.parm.gCol.gc_genParm.gp_closure = (PTR)pcb;
    pcb->api.parm.gCol.gc_stmtHandle = pcb->ccb->api.stmt;
    pcb->api.parm.gCol.gc_rowCount = 
	pcb->data.data.max_row - pcb->data.data.tot_row;
    pcb->api.parm.gCol.gc_columnCount = pcb->data.data.col_cnt;
    pcb->api.parm.gCol.gc_columnData = pcb->data.data.data;

    IIapi_getColumns( &pcb->api.parm.gCol );
    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_gCol_cmpl( II_PTR arg, II_PTR parm )
{
    IIAPI_GETCOLPARM	*gcol = (IIAPI_GETCOLPARM *)parm;
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;

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

    jdbc_gen_cmpl( arg, parm );
    return;
}


/*
** Name: jdbc_api_cancel
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
jdbc_api_cancel( JDBC_PCB *pcb )
{
    pcb->api.name = "IIapi_cancel()";
    pcb->api.parm.cancel.cn_genParm.gp_callback = jdbc_cncl_cmpl;
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
	jdbc_pop_callback( pcb );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_cncl_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = jdbc_api_status( pcb->api.name, gp, pcb->ccb, NULL );
    jdbc_pop_callback( pcb );

    return;
}


/*
** Name: jdbc_api_close
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
jdbc_api_close( JDBC_PCB *pcb, PTR stmtHandle )
{
    if ( ! stmtHandle )		
    {
	/* Close the current active statement. */
	stmtHandle = pcb->ccb->api.stmt;
	pcb->ccb->api.stmt = NULL;
    }

    pcb->api.name = "IIapi_close()";
    pcb->api.parm.close.cl_genParm.gp_callback = jdbc_gen_cmpl;
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
	jdbc_pop_callback( pcb );
    }

    return;
}

II_FAR II_CALLBACK II_VOID
jdbc_gen_cmpl( II_PTR arg, II_PTR parm )
{
    JDBC_PCB		*pcb = (JDBC_PCB *)arg;
    IIAPI_GENPARM	*gp = (IIAPI_GENPARM *)parm;

    pcb->api_status = gp->gp_status;
    pcb->api_error = jdbc_api_status( pcb->api.name, gp, pcb->ccb, 
				      pcb->rcb ? &pcb->rcb : NULL );
    jdbc_pop_callback( pcb );

    return;
}


/*
** Name: jdbc_api_format
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
jdbc_api_format
( 
    JDBC_CCB		*ccb,
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
** Name: jdbc_api_status
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
**	 7-Jun-05 (gordy)
**	    Check for connection and transaction abort.
*/

static STATUS
jdbc_api_status( char *func_name, IIAPI_GENPARM *genParm, 
		 JDBC_CCB *ccb, JDBC_RCB **rcb_ptr )
{
    IIAPI_STATUS	api_status = genParm->gp_status;
    bool		api_error = API_ERROR( genParm->gp_status );
    bool		dbms_error = FALSE;
    STATUS		status = OK;

    if ( (api_error  &&  JDBC_global.trace_level >= 1)  ||
	 JDBC_global.trace_level >= 2 )
	TRdisplay( "%4d    JDBC %s status: %s\n", ccb->id, func_name, 
		   gcu_lookup( apiMap, genParm->gp_status ) );

    if ( genParm->gp_errorHandle )
    {
	IIAPI_GETEINFOPARM	get;

	get.ge_errorHandle = genParm->gp_errorHandle;
	IIapi_getErrorInfo( &get );

	while( get.ge_status == IIAPI_ST_SUCCESS )
	{
	    JDBC_MSG_ERR    err;

	    err.msg_len = get.ge_message ? (u_i2)STlength(get.ge_message) : 0;

	    switch( get.ge_type )
	    {
	    case IIAPI_GE_ERROR   : 
		if ( get.ge_errorCode == E_AP0001_CONNECTION_ABORTED )
		    ccb->cib->conn_abort = TRUE;
		else  if ( get.ge_errorCode == E_AP0002_TRANSACTION_ABORTED )
		    ccb->cib->xact_abort = TRUE;

		if ( ! dbms_error )
		{
		    status = get.ge_errorCode;
		    dbms_error = TRUE;
		}

		err.msg_type = JDBC_ET_ERR;	
		break;


	    case IIAPI_GE_WARNING : err.msg_type = JDBC_ET_WRN;	break;
	    default :		    err.msg_type = JDBC_ET_MSG;	break;
	    }

	    if ( JDBC_global.trace_level >= 2 )
		TRdisplay( "%4d    JDBC Error 0x%x '%s' (%s)\n", ccb->id, 
			   get.ge_errorCode, get.ge_SQLSTATE,
			   gcu_lookup( errMap, err.msg_type ) );
	    if ( JDBC_global.trace_level >= 3  &&  get.ge_message )
		TRdisplay( "%4d    JDBC     %s\n", ccb->id, get.ge_message );

	    if ( rcb_ptr )
		jdbc_send_error( rcb_ptr, err.msg_type, get.ge_errorCode,
				 get.ge_SQLSTATE, err.msg_len, get.ge_message );

	    IIapi_getErrorInfo( &get );
	}
    }

    if ( dbms_error  &&  ! status )  
	status = E_JD0109_INTERNAL_ERROR;
    else  if ( api_error  &&  ! dbms_error )
    {
	status = (api_status == IIAPI_ST_OUT_OF_MEMORY)
		 ? E_JD0108_NO_MEMORY : E_JD0109_INTERNAL_ERROR;
	if ( rcb_ptr )  jdbc_sess_error( ccb, rcb_ptr, status );
    }

    return( status );
}

