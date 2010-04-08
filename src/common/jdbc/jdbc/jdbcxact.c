/*
** Copyright (c) 2004 Ingres Corporation
*/

#include <compat.h>
#include <gl.h>
#include <er.h>
#include <me.h>
#include <st.h>
#include <tr.h>

#include <iicommon.h>
#include <gcu.h>
#include <jdbc.h>
#include <jdbcapi.h>
#include <jdbcmsg.h>

/*
** Name: jdbcxact.c
**
** Description:
**	JDBC transaction message processing.
**
**	The following interactions between transaction requests
**	are enforced:  
**
**	    A commit or rollback request during autocommit, 
**	    or if no transaction is active, is ignored.
**
**	    A change in autocommit state is permitted at any
**	    time, but active statements are closed and the
**	    active transaction is committed.
**
**	    Enabling of autocommit is optimized by setting
**	    the autocommit flag but not actually enabling
**	    autocommit.  The function jdbc_xact_check()
**	    should be called prior to any DBMS requests
**	    to assure the correct transaction state.
**
**	    Additional autocommit simulations may be done, and
**	    are managed (and explained) by jdbc_xact_check().
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Extracted to this file.
**	 4-Nov-99 (gordy)
**	    Updated calling sequence of jdbc_send_done() to allow
**	    control of ownership of RCB parameter.
**	17-Jan-00 (gordy)
**	    Autocommit is no longer enabled when requested.
**	    Instead, the request is noted and enabled elsewhere
**	    if a query is to be made to the server.  Redundant
**	    enable/disable requests are eliminated.  Reset the
**	    autocommit request flag when autocommit is disabled.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Statements now
**	    purged using jdbc_purge_stmt().  PCB/RCB handling
**	    simplified.
**	12-Oct-00 (gordy)
**	    Added jdbc_xact_check() to establish correct state
**	    based on transaction mode and query operation (base
**	    for future autocommit simulation modes, but handles
**	    current autocommit enable optimization).
**	17-Oct-00 (gordy)
**	    Implemented the SINGLE autocommit mode.
**	19-Oct-00 (gordy)
**	    Implemented the MULTI autocommit mode.  Enforce
**	    interactions between transaction requests and the
**	    various transaction modes.
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions: begin and
**	    prepare transaction messages, XA transaction ID params.
**	 2-Apr-01 (gordy)
**	    Added support for Ingres distributed transaction IDs.
**	18-Apr-01 (gordy)
**	    Updated XA XID formatting.  Cosmetic changes in constants.
**      13-feb-03 (chash01) x-integrate change#461908
**	    In format_XID(): 
**          VMS compiler comaplains *(data++) << 24 ....  Change
**          it to an equivalent statement.
**       7-Jun-05 (gordy)
**          Added jdbc_xact_abort() to cleanup aborted transactions.
*/	

/*
** Transaction sequencing.
*/

static	GCULIST	xactMap[] = 
{
    { JDBC_XACT_COMMIT,	"COMMIT" },
    { JDBC_XACT_ROLLBACK, "ROLLBACK" },
    { JDBC_XACT_AC_ENABLE, "ENABLE AUTOCOMMIT" },
    { JDBC_XACT_AC_DISABLE, "DISABLE AUTOCOMMIT" },
    { JDBC_XACT_BEGIN, "BEGIN DISTRIBUTED" },
    { JDBC_XACT_PREPARE, "PREPARE" },
    { 0, NULL }
};

static	GCULIST	modeMap[] = 
{
    { JDBC_XACM_DBMS,	" [DBMS]" },
    { JDBC_XACM_SINGLE,	" [SINGLE]" },
    { JDBC_XACM_MULTI,	" [MULTI]" },
};

#define	XACT_COMMIT	0	/* Commit */
#define	XACT_ROLLBACK	10	/* Rollback */
#define	XACT_AC_ON	20	/* Autocommit enable */
#define	XACT_AC_OFF	30	/* Autocommit disable */
#define	XACT_BEGIN	40	/* Begin Distributed */
#define	XACT_PREPARE	50	/* Prepare */
#define XACT_GQI	80	/* Get Query Info */
#define	XACT_NEW_ID	81	/* New transaction ID */
#define	XACT_CANCEL	97	/* Cancel a query */
#define	XACT_CLOSE	98	/* Close a query */
#define	XACT_DONE	99	/* Completed */

static	GCULIST xactSeqMap[] =
{
    { XACT_COMMIT,	"COMMIT" },
    { XACT_ROLLBACK,	"ROLLBACK" },
    { XACT_AC_ON,	"AC_ON" },
    { XACT_AC_OFF,	"AC_OFF" },
    { XACT_BEGIN,	"BEGIN" },
    { XACT_PREPARE,	"PREPARE" },
    { XACT_GQI,		"GQI" },
    { XACT_NEW_ID,	"NEW_ID" },
    { XACT_CANCEL,	"CANCEL" },
    { XACT_CLOSE,	"CLOSE" },
    { XACT_DONE,	"DONE" },
    { 0, NULL }
};

/*
** Parameter flags and request required/optional parameters.
*/
#define	XP_II_XID	0x01
#define	XP_FRMT		0x02
#define	XP_GTID		0x04
#define	XP_BQUAL	0x08
#define	XP_XA_XID	(XP_FRMT | XP_GTID | XP_BQUAL)

static	u_i2	p_req_xa[] =
{
    0,
    0,
    0,
    0,
    0,
    XP_XA_XID,
    0,
};

static	u_i2	p_req_ing[] =
{
    0,
    0,
    0,
    0,
    0,
    XP_II_XID,
    0,
};

static	u_i2	p_opt[] = 
{
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};

static	char	*beginXA = "set session with xa_xid='%s'";
static	char	*beginII = "set session with description='%s'";

static	void	msg_xact_sm( PTR );
static  void    abort_cmpl( PTR );
static	void	formatXID( IIAPI_XA_DIS_TRAN_ID *, char * );


/*
** Name: jdbc_msg_xact
**
** Description:
**	Process a JDBC transaction message.
**
** Input:
**	ccb	Connection control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	18-Oct-99 (gordy)
**	    Renamed for external reference.
**	17-Jan-00 (gordy)
**	    Autocommit is no longer enabled when requested.
**	    Instead, the request is noted and enabled elsewhere
**	    if a query is to be made to the server.  Redundant
**	    enable/disable requests are eliminated.  Reset the
**	    autocommit request flag when autocommit is disabled.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Statements now
**	    purged using jdbc_purge_stmt().  PCB/RCB handling
**	    simplified.
**	19-Oct-00 (gordy)
**	    New autocommit mode which may turn off the autocommit
**	    flag and use a standard transaction (indicated by the
**	    new xacm_multi flag).  Ignore commit/rollback during
**	    autocommit.  If autocommit state is changed, close all
**	    active statments and commit the active transaction.
**	21-Mar-01 (gordy)
**	    Process new BEGIN and PREPARE transaction messages and
**	    XA transaction ID parameters.
**	 2-Apr-01 (gordy)
**	    Added support for Ingres distributed transaction IDs.
*/

void
jdbc_msg_xact( JDBC_CCB *ccb )
{
    JDBC_PCB		*pcb;
    IIAPI_II_TRAN_ID	iid;
    IIAPI_XA_TRAN_ID	xid;
    u_i2		*p_req = p_req_xa;
    u_i2		req_id, p_rec = 0;
    bool		done = FALSE;
    bool		incomplete = FALSE;

    if ( ! jdbc_get_i2( ccb, (u_i1 *)&req_id ) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid XACT message format\n", 
			ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC Transaction request: %s\n", 
		   ccb->id, gcu_lookup( xactMap, req_id ) );

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC %s %s %s\n", ccb->id, 
		   ccb->cib->autocommit ? "Autocommit" :
		       (ccb->xact.xacm_multi ? "Multi-cursor" : "Transaction"),
		   ccb->cib->tran ? "active" : "inactive",
		   ccb->cib->autocommit ? 
		       gcu_lookup( modeMap, ccb->xact.auto_mode ) : "" );

    MEfill( sizeof( xid ), 0, (PTR)&xid );

    /*
    ** Read the request parameters.
    */
    while( ccb->msg.msg_len )
    {
	JDBC_MSG_PM xp;

	incomplete = TRUE;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&xp.param_id ) )  break;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&xp.val_len ) )  break;

	switch( xp.param_id )
	{
	case JDBC_XP_II_XID :
	    if ( xp.val_len != (CV_N_I4_SZ * 2)  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&iid.it_lowTran )  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&iid.it_highTran ) )
		break;

	    p_rec |= XP_II_XID;
	    incomplete = FALSE;
	    p_req = p_req_ing;
	    break;

	case JDBC_XP_XA_FRMT :
	    if ( xp.val_len != CV_N_I4_SZ  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&xid.xt_formatID ) )
		break;

	    p_rec |= XP_FRMT;
	    incomplete = FALSE;
	    break;

	case JDBC_XP_XA_GTID :
	    if ( xid.xt_bqualLength  &&  xp.val_len <= IIAPI_XA_MAXGTRIDSIZE )
	    {
		/* BQUAL follows GTRID in data array */
		int i;

		for( i = xid.xt_bqualLength - 1; i >= 0; i-- )
		    xid.xt_data[ xp.val_len + i ] = xid.xt_data[ i ];
	    }

	    if ( xp.val_len > IIAPI_XA_MAXGTRIDSIZE  ||
		 ! jdbc_get_bytes( ccb, xp.val_len, (u_i1 *)&xid.xt_data[0] ) )
		break;
	    
	    xid.xt_gtridLength  = xp.val_len;
	    p_rec |= XP_GTID;
	    incomplete = FALSE;
	    break;

	case JDBC_XP_XA_BQUAL :
	    if ( xp.val_len > IIAPI_XA_MAXBQUALSIZE  ||
		 ! jdbc_get_bytes( ccb, xp.val_len, 
				   (u_i1 *)&xid.xt_data[xid.xt_gtridLength] ) )
		break;
	    
	    xid.xt_bqualLength = xp.val_len;
	    p_rec |= XP_BQUAL;
	    incomplete = FALSE;
	    break;

	default :
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC     unkown parameter ID %d\n",
			   ccb->id, xp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    if ( incomplete  ||  
	 (p_rec & p_req[ req_id ]) != p_req[ req_id ]  ||
	 (p_rec & ~(p_req[ req_id ] | p_opt[ req_id ])) )
    {
	if ( JDBC_global.trace_level >= 1 )
	    if ( incomplete )
		TRdisplay( "%4d    JDBC unable to read all request params %d\n",
			   ccb->id );
	    else
		TRdisplay( "%4d    JDBC invalid or missing request params %d\n",
			   ccb->id );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    /*
    ** Don't allocate the RCB at this time so that there
    ** is no client error message produced when purging
    ** statements.
    */
    if ( ! (pcb = jdbc_new_pcb( ccb, FALSE )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

    switch( req_id )
    {
    case JDBC_XACT_COMMIT :
	if ( 
	     ccb->cib->autocommit  ||  	/* Autocommit enabled */
	     ccb->xact.xacm_multi  ||	/* Simulated autocommit */
	     ! ccb->cib->tran 		/* No active transaction */
	   )  
	    done = TRUE;
	else
	    ccb->sequence = XACT_COMMIT;
	break;
    
    case JDBC_XACT_ROLLBACK :
	if ( 
	     ccb->cib->autocommit  ||	/* Autocommit enabled */
	     ccb->xact.xacm_multi  ||	/* Simulated autocommit */
	     ! ccb->cib->tran 		/* No active transaction */
	   )
	    done = TRUE;
	else
	    ccb->sequence = XACT_ROLLBACK;
	break;
    
    case JDBC_XACT_AC_ENABLE :
	if ( ccb->cib->autocommit  ||  ccb->xact.xacm_multi )
	    done = TRUE;	/* Already enabled */
	else  if ( ! ccb->cib->tran )
	{
	    /*
	    ** We don't actually turn on autocommit here,
	    ** rather we just flag autocommit requested 
	    ** (see jdbc_xact_check()).
	    */
	    ccb->cib->autocommit = TRUE;
	    done = TRUE;
	}
	else  if ( ccb->xact.distXID )
	{
	    jdbc_sess_error( ccb, &pcb->rcb, E_JD0110_XACT_AC_STATE );
	    done = TRUE;
	}
	else
	{
	    /*
	    ** A standard transaction is active: close all 
	    ** active statements and commit the transaction.
	    */
	    ccb->sequence = XACT_AC_ON;
	}
	break;
    
    case JDBC_XACT_AC_DISABLE :
	if ( ! ccb->cib->tran )
	{
	    ccb->cib->autocommit = FALSE;
	    ccb->xact.xacm_multi = FALSE;
	    done = TRUE;
	}
	else  if ( ccb->cib->autocommit )
	    ccb->sequence = XACT_AC_OFF;	/* Disable autocommit */
	else  if ( ccb->xact.distXID )
	{
	    jdbc_sess_error( ccb, &pcb->rcb, E_JD0110_XACT_AC_STATE );
	    done = TRUE;
	}
	else
	{
	    /*
	    ** There is a standard transaction active.
	    ** (possibly for autocommit simulation).
	    ** Commit the transaction (and disable
	    ** autocommit simulation).
	    */
	    ccb->xact.xacm_multi = FALSE;
	    ccb->sequence = XACT_COMMIT;
	}
	break;
    
    case JDBC_XACT_BEGIN :
	if ( ccb->cib->tran  ||  ccb->xact.distXID  ||
	     ccb->cib->autocommit  ||  ccb->xact.xacm_multi )
	{
	    jdbc_sess_error( ccb, &pcb->rcb, E_JD0114_XACT_BEGIN_STATE );
	    done = TRUE;
	}
	else
	{
	    char xid_str[ 512 ];

	    ccb->sequence = XACT_BEGIN;

	    if ( p_req == p_req_ing )
	    {
		pcb->data.tran.distXID.ti_type = IIAPI_TI_IIXID;
		STcopy( JDBC_XID_NAME, xid_str );
		STcopy( xid_str, 
		    pcb->data.tran.distXID.ti_value.iiXID.ii_tranName );
		pcb->data.tran.distXID.ti_value.iiXID.ii_tranID.it_highTran =
		    iid.it_highTran;
		pcb->data.tran.distXID.ti_value.iiXID.ii_tranID.it_lowTran =
		    iid.it_lowTran;

		if ( ! jdbc_expand_qtxt( ccb, STlength( beginII ) + 
					      STlength( xid_str ), FALSE ) )
		{
		    jdbc_del_pcb( pcb );
		    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
		    return;
		}

		STprintf( ccb->qry.qtxt, beginII, xid_str );
	    }
	    else
	    {
		pcb->data.tran.distXID.ti_type = IIAPI_TI_XAXID;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchSeqnum = 
		    JDBC_XID_SEQNUM;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchFlag = 
		    JDBC_XID_FLAGS;

		MEcopy( (PTR)&xid, sizeof( xid ),
			(PTR)&pcb->data.tran.distXID.ti_value.xaXID.xa_tranID );
		formatXID( &pcb->data.tran.distXID.ti_value.xaXID, xid_str );

		if ( ! jdbc_expand_qtxt( ccb, STlength( beginXA ) + 
					      STlength( xid_str ), FALSE ) )
		{
		    jdbc_del_pcb( pcb );
		    jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
		    return;
		}

		STprintf( ccb->qry.qtxt, beginXA, xid_str );
	    }
	}
	break;

    case JDBC_XACT_PREPARE :
	if ( ! ccb->cib->tran  ||  ! ccb->xact.distXID )
	{
	    jdbc_sess_error( ccb, &pcb->rcb, E_JD0115_XACT_PREP_STATE );
	    done = TRUE;
	}
	else
	    ccb->sequence = XACT_PREPARE;
	break;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid XACT operation: %d\n", 
			ccb->id, req_id );
	jdbc_del_pcb( pcb );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    if ( done )
    {
	jdbc_send_done( ccb, &pcb->rcb, NULL );
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
    }
    else
    {
	/*
	** Before changing the transaction state,
	** make sure all statements have been closed.
	*/
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_purge_stmt( pcb );
    }

    return;
}


/*
** Name: msg_xact_sm
**
** Description:
**	Process transaction requests.  Acts as a callback to API
**	transaction requests.  For commit and rollback, active
**	statements are first closed, and a new internal xact ID
**	is generated.
**
** Input:
**	arg	Parameter control block.
**
** Output:
**	None.
**
** Returns:
**	void.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Statements now
**	    purged using jdbc_purge_stmt() prior to entry into
**	    this routine.  PCB/RCB handling simplified.
**	19-Oct-00 (gordy)
**	    Disabling autocommit no longer generates a new ID.
**	    Set/reset the autocommit flag since not done in
**	    jdbc_api_autocommit() any more.  We don't actually
**	    enable autocommit here and only get called in the
**	    autocommit enable case when a standard transaction 
**	    is active, so commit the standard transaction and
**	    flag autocommit requested.
**	21-Mar-01 (gordy)
**	    Processing for BEGIN and PREPARE transactions.
*/

static void
msg_xact_sm( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;

    /*
    ** If not yet allocated, allocate the RCB.
    */
    if ( ! pcb->rcb  &&  ! (pcb->rcb = jdbc_new_rcb( ccb )) )
    {
	jdbc_del_pcb( pcb );
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	return;
    }

  top:

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC xact process seq %s\n",
		   ccb->id, gcu_lookup( xactSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case XACT_COMMIT :				/* Commit */
	ccb->sequence = XACT_NEW_ID;
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_commit( pcb );
	return;

    case XACT_ROLLBACK :			/* Rollback */
	ccb->sequence = XACT_NEW_ID;
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_rollback( pcb );
	return;
    
    case XACT_AC_ON :  				/* Enable autocommit */
	/*
	** We don't actually turn on autocommit here,
	** rather we just flag autocommit requested 
	** (see jdbc_xact_check()).
	**
	** We need to commit the existing standard
	** transaction (which requires a new ID).
	*/
	ccb->cib->autocommit = TRUE;
	ccb->sequence = XACT_NEW_ID;
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_commit( pcb );
	return;

    case XACT_AC_OFF : 				/* Disable autocommit */
	ccb->cib->autocommit = FALSE;
	ccb->sequence = XACT_DONE;
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_autocommit( pcb, FALSE );	
	return;

    case XACT_BEGIN :				/* Begin Distributed */
	/*
	** Register the distributed transaction ID.
	*/
	if ( ! jdbc_api_regXID( pcb->ccb, &pcb->data.tran.distXID ) )
	{
	    jdbc_sess_error( ccb, &pcb->rcb, E_JD0116_XACT_REG_XID );
	    ccb->sequence = XACT_DONE;
	    break;
	}

	/*
	** Issue a query to associate DBMS transaction with 
	** the XID.  This also has the effect of translating 
	** the OpenAPI Transaction Name Handle into an OpenAPI
	** Transaction Handle for the distributed transaction.
	*/
	ccb->cib->tran = ccb->xact.distXID;
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = FALSE;
	pcb->data.query.text = ccb->qry.qtxt;

	ccb->sequence = XACT_GQI;
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_query( pcb );
	return;

    case XACT_PREPARE :				/* Prepare (2PC) */
	ccb->sequence = XACT_DONE;
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_prepare( pcb );
	return;

    case XACT_GQI :				/* Get query info */
	if ( pcb->api_error )
	{
	    /*
	    ** The distributed XID is placed in the transaction 
	    ** handle to start a distributed transaction.  A real
	    ** real transaction handle should be returned.  If
	    ** not, clear the transaction handle.
	    */ 
	    if ( ccb->cib->tran == ccb->xact.distXID ) ccb->cib->tran = NULL;
	    ccb->sequence = XACT_CANCEL;
	    break;
	}

	ccb->sequence = XACT_CLOSE;
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_gqi( pcb );
	return;

    case XACT_NEW_ID :				/* New Transaction ID */
	ccb->sequence = XACT_DONE;
	ccb->xact.tran_id++;
	ccb->stmt.stmt_id = 0;
	break;

    case XACT_CANCEL :				/* Cancel query */
	jdbc_flush_msg( ccb );
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_cancel( pcb );
	return;

    case XACT_CLOSE :				/* Close query */
	jdbc_push_callback( pcb, msg_xact_sm );
	jdbc_api_close( pcb, NULL );
	return;
	
    case XACT_DONE :				/* Processing completed */
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	jdbc_msg_pause( ccb, TRUE );
	return;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid xact processing sequence: %d\n",
		       ccb->id, --ccb->sequence );
	jdbc_del_pcb( pcb );
	jdbc_gcc_abort( ccb, E_JD0109_INTERNAL_ERROR);
	return;
    }

    goto top;
}


/*
** Name: jdbc_xact_abort
**
** Description:
**      Cleanup session state after a transaction abort
**      is detected.
**
** Input:
**      pcb
**
** Output:
**      None.
**
** Returns:
**      void
**
** History:
**       3-Jun-05 (gordy)
**          Created.
*/

void
jdbc_xact_abort( JDBC_PCB *pcb )
{
    /*
    ** Begin by purging active statements.
    */
    jdbc_push_callback( pcb, abort_cmpl );
    jdbc_purge_stmt( pcb );
    return;
}

static void
abort_cmpl( PTR arg )
{
    JDBC_PCB     *pcb = (JDBC_PCB *)arg;

    pcb->ccb->cib->xact_abort = FALSE;

    /*
    ** If there is an active transaction handle, rollback
    ** to free resources (return here for final processing).
    ** Since the transaction abort may have close a cursor,
    ** check the transaction processing state for that event 
    ** (jdbc_xact_check() will pop our callback).
    */
    if ( ! pcb->ccb->cib->tran )
	jdbc_xact_check( pcb, JDBC_XQOP_CURSOR_CLOSE );
    else
    {
	jdbc_push_callback( pcb, abort_cmpl );
	jdbc_api_rollback( pcb );
    }

    return;
}


/*
** Name: jdbc_xact_check
**
** Description:
**	Makes sure that the correct connection and transaction
**	states exist for a given query operation and current
**	transaction mode.
**
**	Non-autocommit transactions are handled in the DBMS,
**	so this routine is primarily involved with handling
**	the autocommit state/mode.  The Ingres DBMS has the
**	convention that only one cursor may be open during
**	autocommit and only cursor related operations may
**	be done while the cursor is opened.  Multiple cursor
**	opens and non-cursor operations result in transaction
**	errors (the dreaded 'MST transaction' error).  The
**	JDBC server autocommit modes provide various forms
**	of support to avoid problems with autocommit and
**	open cursors.  The supported autocommit modes are 
**	as follows:
**
**   JDBC_XACM_DBMS
**
**	Autocommit is handled in DBMS.  Enabling of autocommit
**	is optimized so that it does not occur until needed.
**	The autocommit requested state is signaled by setting
**	ccb->cib->autocommit TRUE while no API transaction
**	handle exists (ccb->cib->tran is NULL).  This routine
**	will enable autocommit (create an API autocommit
**	transaction handle) given this transaction mode and 
**	state (independent of query operation).
**
**   JDBC_XACM_SINGLE
**
**	Autocommit is also handled by the DBMS, but transaction
**	errors are avoided by closing open cursors when a new 
**	cursor open or non-cursor request is made.  This mode 
**	also supports the optimization done by JDBC_XACM_DBMS.
**
**   JDBC_XACM_MULTI
**
**	Autocommit is handled by the DBMS only when there are
**	no open cursors.  If a cursor is opened, autocommit is
**	disabled and a flag is set to indicate that the active
**	transaction (which will be started by opening the cursor)
**	is really a part of a simulated autocommit transaction.
**	This mode permits multiple open cursors and non-cursor
**	operations with open cursors while simulating autocommit.
**
**	When a cursor is closed (if no other cursor is open)
**	the active transaction is committed and autocommit is
**	re-enabled.  As with the other autocommit modes, the
**	enabling of autocommit is optimized to only occur when
**	actually needed.
**
** Input:
**	pcb	Parameter control block.
**	xqop	Transaction query operation.
**		    JDBC_XQOP_NON_CURSOR
**		    JDBC_XQOP_CURSOR_OPEN
**		    JDBC_XQOP_CURSOR_UPDATE
**		    JDBC_XQOP_CURSOR_CLOSE
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	12-Oct-00 (gordy)
**	    Created.
**	17-Oct-00 (gordy)
**	    Implemented the SINGLE autocommit mode.
**	19-Oct-00 (gordy)
**	    Implemented the MULTI autocommit mode.
*/

void
jdbc_xact_check( JDBC_PCB *pcb, u_i1 xqop )
{
    JDBC_CCB	*ccb = pcb->ccb;

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC %s %s %s\n", ccb->id, 
		   ccb->cib->autocommit ? "Autocommit" :
		       (ccb->xact.xacm_multi ? "Multi-cursor" : "Transaction"),
		   ccb->cib->tran ? "active" : "inactive",
		   ccb->cib->autocommit ? 
		       gcu_lookup( modeMap, ccb->xact.auto_mode ) : "" );
    /*
    ** Non-autocommit transactions are handled entirely
    ** through the DBMS, so there is nothing to do if
    ** autocommit is disabled (watch out for multi-cursor
    ** simulated autocommit mode which temporarily disables
    ** autocommit).
    */
    if ( ! ccb->cib->autocommit  &&  ! ccb->xact.xacm_multi )
    {
	jdbc_pop_callback( pcb );
	return;
    }

    switch( ccb->xact.auto_mode )
    {
    case JDBC_XACM_DBMS :
	/*
	** Autocommit is being handled by the DBMS, but
	** autocommit may not yet have been enabled.
	** Start autocommit transaction if not yet started
	** (jdbc_api_autocommit() will make callback when
	** completed).  Otherwise, nothing to do.
	*/
	if ( ! ccb->cib->tran )
	    jdbc_api_autocommit( pcb, TRUE );
	else
	    jdbc_pop_callback( pcb );
	break;
    
    case JDBC_XACM_SINGLE :
	/*
	** Autocommit is being handled by the DBMS and
	** only a single cursor is permitted to be open.
	** An open cursor will be closed if an open 
	** cursor or non-cursor request is made to avoid 
	** an error when sending the request to the DBMS.
	**
	** The autocommit enable optimization is also
	** done for this mode, so turn on autocommit if
	** not enabled.
	*/
	if ( ! ccb->cib->tran )
	    jdbc_api_autocommit( pcb, TRUE );
	else  switch( xqop )
	{
	case JDBC_XQOP_NON_CURSOR :
	case JDBC_XQOP_CURSOR_OPEN :
	    jdbc_purge_stmt( pcb );
	    break;

	case JDBC_XQOP_CURSOR_UPDATE :
	case JDBC_XQOP_CURSOR_CLOSE :
	default : 
	    jdbc_pop_callback( pcb );
	    break;
	}
	break;

    case JDBC_XACM_MULTI :
	/*
	** Autocommit handled by DBMS only when cursors are
	** not open.  A normal transaction is used while
	** cursors are open and commited once all cursors
	** are closed.
	*/
	switch( xqop )
	{
	case JDBC_XQOP_NON_CURSOR :
	case JDBC_XQOP_CURSOR_UPDATE :
	    /*
	    ** Turn on autocommit if not enabled.
	    */
	    if ( ccb->cib->autocommit  &&  ! ccb->cib->tran )
		jdbc_api_autocommit( pcb, TRUE );
	    else
		jdbc_pop_callback( pcb );
	    break;

	case JDBC_XQOP_CURSOR_OPEN :
	    /*
	    ** Turn off autocommit to prepare for a normal 
	    ** transaction activated by the cursor open.
	    */
	    if ( ccb->xact.xacm_multi )
		jdbc_pop_callback( pcb );
	    else
	    {
		ccb->cib->autocommit = FALSE;
		ccb->xact.xacm_multi = TRUE;

		if ( ccb->cib->tran )
		    jdbc_api_autocommit( pcb, FALSE );
		else
		    jdbc_pop_callback( pcb );
	    }
	    break;

	case JDBC_XQOP_CURSOR_CLOSE :
	    /*
	    ** If no cursors are open, commit the
	    ** normal transaction activated by the
	    ** open cursor request and return our
	    ** state to autocommit.
	    **
	    ** We should not reach this point if
	    ** autocommit is actually enabled
	    ** (should be simulated autocommit),
	    ** but handle this condition just to
	    ** be safe.
	    */
	    if ( ccb->cib->autocommit )
	    {
		if ( ccb->cib->tran )
		    jdbc_pop_callback( pcb );
		else
		    jdbc_api_autocommit( pcb, TRUE );
	    }
	    else  if ( jdbc_active_stmt( ccb ) )
		jdbc_pop_callback( pcb );
	    else
	    {
		ccb->xact.xacm_multi = FALSE;
		ccb->cib->autocommit = TRUE;

		if ( ccb->cib->tran )
		    jdbc_api_commit( pcb );
		else
		    jdbc_pop_callback( pcb );
	    }
	    break;

	default : 
	    /*
	    ** Should not happen.
	    */
	    jdbc_pop_callback( pcb );
	    break;
	}
	break;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid xact autocommit mode: %d\n",
		       ccb->id, ccb->xact.auto_mode );
	jdbc_pop_callback( pcb );
	break;
    }

    return;
}


/*
** Name: formatXID
**
** Description:
**	Formats XA distributed transaction ID as text based
**	on algorithm implemented in IICXformat_xa_xid() and
**	IICXformat_xa_extd_xid() in libqxa.
**
** Input:
**	xid	XA Distributed Transaction ID.
**
** Output:
**	str	Formatted output (allow room for 512 bytes).
**
** Returns:
**	void.
**
** History:
**	12-Mar-01 (gordy)
**	    Created.
**	18-Apr-01 (gordy)
**	    Updated algorithm to match changes in libqxa.
**      03-jan-03 (chash01)
**          VMS compiler comaplains *(data++) << 24 ....  Change
**          it to an equivalent statement.
*/

static void
formatXID( IIAPI_XA_DIS_TRAN_ID *xid, char *str )
{
    u_i4	count, val;
    u_i1	*data;

    CVlx( xid->xa_tranID.xt_formatID, str );
    str += STlength( str );

    *(str++) = ':';
    CVla( xid->xa_tranID.xt_gtridLength, str );
    str += STlength( str );

    *(str++) = ':';
    CVla( xid->xa_tranID.xt_bqualLength, str );
    str += STlength( str );

    data = (u_i1 *)xid->xa_tranID.xt_data;
    count = (xid->xa_tranID.xt_gtridLength + xid->xa_tranID.xt_bqualLength + 
		sizeof( i4 ) - 1) / sizeof( i4 );

    while( count-- )
    {
	val = (*data << 24) | (*(data+1) << 16) |
                   (*(data+2) << 8) | *(data+3);
        data+= 4;
	*(str++) = ':';
	CVlx( val, str );
	str += STlength( str );
    }

    STcat( str, ERx( ":XA" ) );
    str += STlength( str );

    *(str++) = ':';
    CVna( xid->xa_branchSeqnum, str );
    str += STlength( str );

    *(str++) = ':';
    CVna( xid->xa_branchFlag, str );
    str += STlength( str );

    STcat( str, ERx( ":EX" ) );
    return;
}

