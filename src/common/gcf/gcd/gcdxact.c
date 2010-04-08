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
#include <gcdint.h>
#include <gcdapi.h>
#include <gcdmsg.h>

/*
** Name: gcdxact.c
**
** Description:
**	GCD transaction message processing.
**
**	The following interactions between transaction requests
**	are enforced:  
**
**	    A commit or rollback request during autocommit, 
**	    or if no transaction is active, is ignored.
**
**	    Savepoint requests during autocommit are ignored.
**	    If no transaction is active, a savepoint will
**	    represent the entire transaction and a rollback
**	    request will result in a full transaction rollback.
**
**	    A change in autocommit state is permitted at any
**	    time, but active statements are closed and the
**	    active transaction is committed.
**
**	    Enabling of autocommit is optimized by setting
**	    the autocommit flag but not actually enabling
**	    autocommit.  The function gcd_xact_check()
**	    should be called prior to any DBMS requests
**	    to assure the correct transaction state.
**
**	    Additional autocommit simulations may be done, and
**	    are managed (and explained) by gcd_xact_check().
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
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**      13-feb-03 (chash01) 
**          In format_XID(): VMS compiler complains *(data++) << 24 ....  
**	    Change it to an equivalent statement.
**	15-Jan-03 (gordy)
**	    Added gcd_sess_abort().  Rename gcd_flush_msg() to gcd_msg_flush().
**	14-Feb-03 (gordy)
**	    Added SAVEPOINT and ROLLBACK TO SAVEPOINT support.
**	13-May-05 (gordy)
**	    Added ABORT to allow DTMC rollback of a non-prepared transaction.
**	 3-Jun-05 (gordy)
**	    Added gcd_xact_abort() to cleanup aborted transactions.
**	    Check for connection and transaction aborts.
**	21-Apr-06 (gordy)
**	    Simplified PCB/RCB handling.
**	28-Jun-06 (gordy)
**	    Pre-allocate RCB so error messages will be returned to client.
**	29-Jun-06 (lunbr01)  Bug 115970
**	    Skip calling API cancel or close on a failed request when
**	    the connection has been aborted.  This will prevent potential
**	    access violation or logic error due to cancel/close accessing 
**	    deleted API handles for the session.
**	14-Jul-06 (gordy)
**	    Added support for XA requests.
*/	

/*
** Transaction sequencing.
*/

static	GCULIST	xactMap[] = 
{
    { MSG_XACT_COMMIT,		"COMMIT" },
    { MSG_XACT_ROLLBACK,	"ROLLBACK" },
    { MSG_XACT_AC_ENABLE,	"ENABLE AUTOCOMMIT" },
    { MSG_XACT_AC_DISABLE,	"DISABLE AUTOCOMMIT" },
    { MSG_XACT_BEGIN,		"START" },
    { MSG_XACT_PREPARE,		"PREPARE" },
    { MSG_XACT_SAVEPOINT,	"SAVEPOINT" },
    { MSG_XACT_ABORT,		"ABORT" },
    { MSG_XACT_END,		"END" },
    { 0, NULL }
};

static	GCULIST	modeMap[] = 
{
    { GCD_XACM_DBMS,	" [DBMS]" },
    { GCD_XACM_SINGLE,	" [SINGLE]" },
    { GCD_XACM_MULTI,	" [MULTI]" },
};

#define	XACT_COMMIT	10	/* Commit */
#define	XACT_ROLLBACK	15	/* Rollback */
#define	XACT_SAVEPOINT	20	/* Savepoint */
#define	XACT_SP_CHK	21	/* Check savepoint status */
#define	XACT_ROLL_SP	25	/* Rollback to savepoint */
#define	XACT_AC_OFF	30	/* Autocommit disable */
#define	XACT_BEGIN	35	/* Begin Distributed */
#define XACT_BEG_GQI	36	/* Get Query Info */
#define	XACT_BEG_CHK	37	/* Check begin status */
#define	XACT_PREPARE	40	/* Prepare distributed transaction */
#define	XACT_START	45	/* Start XA transaction */
#define	XACT_END	50	/* End XA association */
#define	XACT_XA_PREP	55	/* Prepare XA transaction */
#define	XACT_XA_COMM	60	/* Commit XA transaction */
#define	XACT_XA_ROLL	65	/* Rollback XA transaction */
#define	XACT_ABORT_1	80	/* Abort: stmt 1 */
#define	XACT_AB1_GQI	81	/* Abort: query info */
#define	XACT_AB1_CLOSE	82	/* Abort: close */
#define	XACT_ABORT_2	83	/* Abort: stmt 2*/
#define	XACT_AB2_GQI	84	/* Abort: query info */
#define	XACT_AB2_CLOSE	85	/* Abort: close */
#define	XACT_NEW_ID	90	/* New transaction ID */
#define	XACT_CANCEL	95	/* Cancel a query */
#define	XACT_CLOSE	96	/* Close a query */
#define	XACT_CHECK	97	/* Check transaction state */
#define	XACT_DONE	98	/* Completed */

static	GCULIST xactSeqMap[] =
{
    { XACT_COMMIT,	"COMMIT" },
    { XACT_ROLLBACK,	"ROLLBACK" },
    { XACT_SAVEPOINT,	"SAVEPOINT" },
    { XACT_SP_CHK,	"SAVEPOINT: Check" },
    { XACT_ROLL_SP,	"ROLLBACK: Savepoint" },
    { XACT_AC_OFF,	"AUTOCOMMIT: Off" },
    { XACT_BEGIN,	"BEGIN" },
    { XACT_BEG_GQI,	"BEGIN: GQI" },
    { XACT_BEG_CHK,	"BEGIN: CHK" },
    { XACT_PREPARE,	"PREPARE" },
    { XACT_START,	"START" },
    { XACT_END,		"END" },
    { XACT_XA_PREP,	"XA PREPARE" },
    { XACT_XA_COMM,	"XA COMMIT" },
    { XACT_XA_ROLL,	"XA ROLLBACK" },
    { XACT_ABORT_1,	"ABORT: enable force abort" },
    { XACT_AB1_GQI,	"ABORT: GQI-1" },
    { XACT_AB1_CLOSE,	"ABORT: Close-1" },
    { XACT_ABORT_2,	"ABORT: force abort" },
    { XACT_AB2_GQI,	"ABORT: GQI-2" },
    { XACT_AB2_CLOSE,	"ABORT: Close-2" },
    { XACT_NEW_ID,	"NEW_ID" },
    { XACT_CANCEL,	"CANCEL" },
    { XACT_CLOSE,	"CLOSE" },
    { XACT_CHECK,	"CHECK" },
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
#define	XP_SP		0x10
#define	XP_FLAGS	0x20

#define	XP_XA_XID	(XP_FRMT | XP_GTID | XP_BQUAL)

static	u_i2	p_req_xa[] =
{
    0,
    0,
    0,
    0,
    0,
    XP_XA_XID,	/* Start */
    0,
    XP_SP,	/* Savepoint */
    XP_XA_XID,	/* Abort */
    XP_XA_XID,	/* End */
};

static	u_i2	p_req_ing[] =
{
    0,
    0,
    0,
    0,
    0,
    XP_II_XID,	/* Begin */
    0,
    XP_SP,	/* Savepoint */
    0,
    0,
};

static	u_i2	p_opt[] = 
{
    0,
    XP_XA_XID | XP_FLAGS,
    XP_XA_XID | XP_FLAGS | XP_SP,
    0,
    0,
    XP_FLAGS,
    XP_XA_XID | XP_FLAGS,
    0,
    0,
    XP_FLAGS,
};


static	char	*beginII = "set session with description='%s'";
static	char	*beginXA = "set session with xa_xid='%s'";
static	char	*abortXA = "rollback with xa_xid='%s'";
static	char	*forceAbort = "set trace point qe40";

static	void	msg_xact_sm( PTR );
static	void	abort_cmpl( PTR );
static	void	formatXID( IIAPI_XA_DIS_TRAN_ID *, char * );


/*
** Name: gcd_msg_xact
**
** Description:
**	Process a GCD transaction message.
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
**	14-Feb-03 (gordy)
**	    Added support for savepoints and rollback to savepoint.
**	13-May-05 (gordy)
**	    Added support for abort transactions from different session.
**	 3-Jun-05 (gordy)
**	    Separated rollback and rollback-to-savepoint actions.
**	14-Jul-06 (gordy)
**	    Added support for XA Start, End, Prepare, Commit, and Rollback.
*/

void
gcd_msg_xact( GCD_CCB *ccb )
{
    GCD_PCB		*pcb;
    GCD_SPCB		*sp = NULL;
    IIAPI_II_TRAN_ID	iid;
    IIAPI_XA_TRAN_ID	xid;
    u_i4		xa_flags = 0;
    u_i2		*p_req = p_req_xa;
    u_i2		req_id, p_rec = 0;
    bool		done = FALSE;
    bool		purge = FALSE;
    bool		incomplete = FALSE;
    STATUS		status = OK;

    if ( ! gcd_get_i2( ccb, (u_i1 *)&req_id ) )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid XACT message format\n", 
			ccb->id );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD Transaction operation: %s\n", 
		   ccb->id, gcu_lookup( xactMap, req_id ) );

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD %s %s %s\n", ccb->id, 
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
	DAM_ML_PM xp;

	incomplete = TRUE;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&xp.param_id ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&xp.val_len ) )  break;

	switch( xp.param_id )
	{
	case MSG_XP_II_XID :
	    if ( xp.val_len != (CV_N_I4_SZ * 2)  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&iid.it_lowTran )  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&iid.it_highTran ) )
		break;

	    p_rec |= XP_II_XID;
	    incomplete = FALSE;
	    p_req = p_req_ing;
	    break;

	case MSG_XP_XA_FRMT :
	    if ( xp.val_len != CV_N_I4_SZ  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&xid.xt_formatID ) )
		break;

	    p_rec |= XP_FRMT;
	    incomplete = FALSE;
	    break;

	case MSG_XP_XA_GTID :
	    if ( xid.xt_bqualLength  &&  xp.val_len <= IIAPI_XA_MAXGTRIDSIZE )
	    {
		/* BQUAL follows GTRID in data array */
		int i;

		for( i = xid.xt_bqualLength - 1; i >= 0; i-- )
		    xid.xt_data[ xp.val_len + i ] = xid.xt_data[ i ];
	    }

	    if ( xp.val_len > IIAPI_XA_MAXGTRIDSIZE  ||
		 ! gcd_get_bytes( ccb, xp.val_len, (u_i1 *)&xid.xt_data[0] ) )
		break;
	    
	    xid.xt_gtridLength  = xp.val_len;
	    p_rec |= XP_GTID;
	    incomplete = FALSE;
	    break;

	case MSG_XP_XA_BQUAL :
	    if ( xp.val_len > IIAPI_XA_MAXBQUALSIZE  ||
		 ! gcd_get_bytes( ccb, xp.val_len, 
				   (u_i1 *)&xid.xt_data[xid.xt_gtridLength] ) )
		break;
	    
	    xid.xt_bqualLength = xp.val_len;
	    p_rec |= XP_BQUAL;
	    incomplete = FALSE;
	    break;

	case MSG_XP_SAVEPOINT :
	    if ( ! (sp = (GCD_SPCB *)MEreqmem( 0, sizeof( GCD_SPCB ) + 
						  xp.val_len, TRUE, NULL ))  ||
		 ! gcd_get_bytes( ccb, xp.val_len, (u_i1 *)sp->name ) )
		break;

	    sp->name[ xp.val_len ] = EOS;
	    p_rec |= XP_SP;
	    incomplete = FALSE;
	    break;

	case MSG_XP_XA_FLAGS :
	    if ( xp.val_len != CV_N_I4_SZ  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&xa_flags ) )
		break;

	    p_rec |= XP_FLAGS;
	    incomplete = FALSE;
	    break;

	default :
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     unkown parameter ID %d\n",
			   ccb->id, xp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    for( ; status == OK; )
    {
	if ( 
	     incomplete  ||  
	     (p_rec & p_req[ req_id ]) != p_req[ req_id ]  ||
	     (p_rec & ~(p_req[ req_id ] | p_opt[ req_id ]))  ||
	     (p_rec & XP_XA_XID  &&  (p_rec & XP_XA_XID) != XP_XA_XID)
	   )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		if ( incomplete )
		    TRdisplay( "%4d    GCD unable to read all xact params\n",
			       ccb->id );
		else
		    TRdisplay( "%4d    GCD invalid or missing xact params\n",
			       ccb->id );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    break;
	}

	/*
	** Don't allocate the RCB at this time so that there
	** is no client error message produced when purging
	** statements.
	*/
	if ( ! (pcb = gcd_new_pcb( ccb )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    break;
	}

	switch( req_id )
	{
	case MSG_XACT_COMMIT :
	    if ( p_rec & XP_XA_XID )		/* XA Commit */
	    {
		if ( ccb->cib->level < IIAPI_LEVEL_4  ||
		     ccb->msg.proto_lvl < MSG_PROTO_5 )
		{
		    gcd_sess_error( ccb, &pcb->rcb, E_GC480A_PROTOCOL_ERROR );
		    done = TRUE;
		    break;
		}

		ccb->sequence = XACT_XA_COMM;
		pcb->data.tran.xa_flags = xa_flags;
		pcb->data.tran.distXID.ti_type = IIAPI_TI_XAXID;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchSeqnum = 
		    GCD_XID_SEQNUM;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchFlag = 
		    GCD_XID_FLAGS;
		STRUCT_ASSIGN_MACRO( xid, 
			pcb->data.tran.distXID.ti_value.xaXID.xa_tranID );
	    }
	    else  if ( ccb->cib->autocommit  ||  ccb->xact.xacm_multi )  
		done = TRUE;			/* Autocommit */
	    else  if ( ! ccb->cib->tran )	/* No active transaction */
	    {
		gcd_release_sp( ccb, NULL );
		done = TRUE;
	    }
	    else
	    {
		ccb->sequence = XACT_COMMIT;
		purge = TRUE;
	    }
	    break;
    
	case MSG_XACT_ROLLBACK :
	    if ( p_rec & XP_XA_XID )		/* XA Rollback */
	    {
		if ( ccb->cib->level < IIAPI_LEVEL_4  ||
		     ccb->msg.proto_lvl < MSG_PROTO_5 )
		{
		    gcd_sess_error( ccb, &pcb->rcb, E_GC480A_PROTOCOL_ERROR );
		    done = TRUE;
		    break;
		}

		ccb->sequence = XACT_XA_ROLL;
		pcb->data.tran.xa_flags = xa_flags;
		pcb->data.tran.distXID.ti_type = IIAPI_TI_XAXID;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchSeqnum = 
		    GCD_XID_SEQNUM;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchFlag = 
		    GCD_XID_FLAGS;
		STRUCT_ASSIGN_MACRO( xid, 
			pcb->data.tran.distXID.ti_value.xaXID.xa_tranID );
	    }
	    else  if ( ccb->cib->autocommit  ||	 ccb->xact.xacm_multi )
		done = TRUE;			/* Autocommit */
	    else  if ( ! ccb->cib->tran )	/* No active transaction */
	    {
		gcd_release_sp( ccb, NULL );
		done = TRUE;
	    }
	    else  if ( sp )			/* Rollback to savepoint */
	    {
		GCD_SPCB *savepoint;

		for( 
		     savepoint = ccb->xact.savepoints; 
		     savepoint;
		     savepoint = savepoint->next
		   )
		   if ( STcompare( sp->name, savepoint->name ) == 0 )
		       break;

		if ( ! savepoint )
		{
		    gcd_sess_error( ccb, &pcb->rcb, E_GC4818_XACT_SAVEPOINT );
		    done = TRUE;
		}
		else
		{
		    ccb->sequence = XACT_ROLL_SP;
		    pcb->data.tran.savepoint = savepoint->hndl;
		    gcd_release_sp( ccb, savepoint );
		    purge = TRUE;
		}
	    }
	    else				/* Rollback transaction */
	    {
		ccb->sequence = XACT_ROLLBACK;
		pcb->data.tran.savepoint = NULL;
		purge = TRUE;
	    }
	    break;
    
	case MSG_XACT_AC_ENABLE :
	    if ( ccb->cib->autocommit  ||  ccb->xact.xacm_multi )
		done = TRUE;	/* Already enabled */
	    else  if ( ! ccb->cib->tran )
	    {
		/*
		** We don't actually turn on autocommit here,
		** rather we just flag autocommit requested 
		** (see gcd_xact_check()).
		*/
		ccb->cib->autocommit = TRUE;
		gcd_release_sp( ccb, NULL );
		done = TRUE;
	    }
	    else  if ( ccb->xact.distXID )
	    {
		gcd_sess_error( ccb, &pcb->rcb, E_GC4813_XACT_AC_STATE );
		done = TRUE;
	    }
	    else
	    {
		/*
		** A standard transaction is active: close all 
		** active statements and commit the transaction.
		**
		** We don't actually turn on autocommit here,
		** rather we just flag autocommit requested 
		** (see gcd_xact_check()).
		*/
		ccb->cib->autocommit = TRUE;
		ccb->sequence = XACT_COMMIT;
		purge = TRUE;
	    }
	    break;
    
	case MSG_XACT_AC_DISABLE :
	    if ( ! ccb->cib->tran )
	    {
		ccb->cib->autocommit = FALSE;
		ccb->xact.xacm_multi = FALSE;
		done = TRUE;
	    }
	    else  if ( ccb->cib->autocommit )
	    {
		ccb->sequence = XACT_AC_OFF;	/* Disable autocommit */
		purge = TRUE;
	    }
	    else  if ( ccb->xact.distXID )
	    {
		gcd_sess_error( ccb, &pcb->rcb, E_GC4813_XACT_AC_STATE );
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
		purge = TRUE;
	    }
	    break;
    
	case MSG_XACT_BEGIN :
	{
	    char xid_str[ 512 ];

	    if ( ccb->cib->tran  ||  ccb->xact.distXID  ||
		 ccb->cib->autocommit  ||  ccb->xact.xacm_multi )
	    {
		gcd_sess_error( ccb, &pcb->rcb, E_GC4814_XACT_BEGIN_STATE );
		done = TRUE;
		break;
	    }

	    ccb->sequence = XACT_BEGIN;
	    purge = TRUE;

	    if ( p_req == p_req_ing )
	    {
		pcb->data.tran.distXID.ti_type = IIAPI_TI_IIXID;
		STcopy( GCD_XID_NAME, xid_str );
		STcopy( xid_str, 
			pcb->data.tran.distXID.ti_value.iiXID.ii_tranName );
		pcb->data.tran.distXID.ti_value.iiXID.ii_tranID.it_highTran =
		    iid.it_highTran;
		pcb->data.tran.distXID.ti_value.iiXID.ii_tranID.it_lowTran =
		    iid.it_lowTran;

		if ( ! gcd_expand_qtxt( ccb, STlength( beginII ) + 
					      STlength( xid_str ), FALSE ) )
		    status = E_GC4808_NO_MEMORY;
		else
		    STprintf( ccb->qry.qtxt, beginII, xid_str );
	    }
	    else
	    {
		pcb->data.tran.distXID.ti_type = IIAPI_TI_XAXID;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchSeqnum = 
		    GCD_XID_SEQNUM;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchFlag = 
		    GCD_XID_FLAGS;
		STRUCT_ASSIGN_MACRO( xid, 
			pcb->data.tran.distXID.ti_value.xaXID.xa_tranID );

		/*
		** If supported, use the API to start an XA transaction,
		** Otherwise, set session transaction ID.  Note: Older 
		** clients expect the latter functionality.
		*/
		if ( ccb->cib->level >= IIAPI_LEVEL_4  &&
		     ccb->msg.proto_lvl >= MSG_PROTO_5 )
		{
		    ccb->sequence = XACT_START;
		    pcb->data.tran.xa_flags = xa_flags;
		}
		else
		{
		    formatXID( &pcb->data.tran.distXID.ti_value.xaXID, 
		    		xid_str );

		    if ( ! gcd_expand_qtxt( ccb, STlength( beginXA ) + 
						 STlength( xid_str ), FALSE ) )
			status = E_GC4808_NO_MEMORY;
		    else
			STprintf( ccb->qry.qtxt, beginXA, xid_str );
	        }
	    }
	    break;
	}

	case MSG_XACT_PREPARE :
	    if ( p_rec & XP_XA_XID )		/* XA Prepare */
	    {
		if ( ccb->cib->level < IIAPI_LEVEL_4  ||
		     ccb->msg.proto_lvl < MSG_PROTO_5 )
		{
		    gcd_sess_error( ccb, &pcb->rcb, E_GC480A_PROTOCOL_ERROR );
		    done = TRUE;
		    break;
		}

		ccb->sequence = XACT_XA_PREP;
		pcb->data.tran.xa_flags = xa_flags;
		pcb->data.tran.distXID.ti_type = IIAPI_TI_XAXID;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchSeqnum = 
		    GCD_XID_SEQNUM;
		pcb->data.tran.distXID.ti_value.xaXID.xa_branchFlag = 
		    GCD_XID_FLAGS;
		STRUCT_ASSIGN_MACRO( xid, 
			pcb->data.tran.distXID.ti_value.xaXID.xa_tranID );
	    }
	    else  if ( ! ccb->cib->tran  ||  ! ccb->xact.distXID )
	    {
		gcd_sess_error( ccb, &pcb->rcb, E_GC4815_XACT_PREP_STATE );
		done = TRUE;
	    }
	    else
	    {
		ccb->sequence = XACT_PREPARE;
		purge = TRUE;
	    }
	    break;

	case MSG_XACT_SAVEPOINT :
	    if ( ccb->cib->autocommit  ||  ccb->xact.xacm_multi )
		done = TRUE;	/* Ignore during autocommit */
	    else
	    {
		/* Save savepoint control block */
		sp->next = ccb->xact.savepoints;
		ccb->xact.savepoints = sp;
		sp = NULL;

		/*
		** If there is no current transaction, an actual
		** savepoint cannot be created.  The NULL handle 
		** will indicate that the savepoint represents
		** the entire transaction.  Otherwise, create the
		** savepoint.
		*/
		if ( ! ccb->cib->tran )
		    done = TRUE;
		else
		    ccb->sequence = XACT_SAVEPOINT;
	    }
	    break;

	case MSG_XACT_ABORT :
	{
	    char xid_str[ 512 ];

	    if ( ccb->cib->tran  ||  ccb->xact.distXID  ||
		 ccb->cib->autocommit  ||  ccb->xact.xacm_multi )
	    {
		gcd_sess_error( ccb, &pcb->rcb, E_GC4819_XACT_ABORT_STATE );
		done = TRUE;
		break;
	    }

	    ccb->sequence = XACT_ABORT_1;
	    purge = TRUE;

	    pcb->data.tran.distXID.ti_type = IIAPI_TI_XAXID;
	    pcb->data.tran.distXID.ti_value.xaXID.xa_branchSeqnum = 
		GCD_XID_SEQNUM;
	    pcb->data.tran.distXID.ti_value.xaXID.xa_branchFlag = 
		GCD_XID_FLAGS;
	    STRUCT_ASSIGN_MACRO( xid,
	    		pcb->data.tran.distXID.ti_value.xaXID.xa_tranID );

	    formatXID( &pcb->data.tran.distXID.ti_value.xaXID, xid_str );

	    if ( ! gcd_expand_qtxt( ccb, STlength( abortXA ) + 
					 STlength( xid_str ), FALSE ) )
		status = E_GC4808_NO_MEMORY;
	    else
		STprintf( ccb->qry.qtxt, abortXA, xid_str );
	    break;
	}

	case MSG_XACT_END :
	    if ( 
		 ccb->cib->autocommit  ||  	/* Autocommit enabled */
		 ccb->xact.xacm_multi  ||	/* Simulated autocommit */
	         ! ccb->cib->tran 		/* No transaction */
	       )
	    {
		gcd_sess_error( ccb, &pcb->rcb, E_GC481A_XACT_END_STATE );
		done = TRUE;
		break;
	    }

	    /*
	    ** Prior to full XA support, the END 
	    ** request was never sent to the DBMS.
	    */
	    if ( ccb->cib->level < IIAPI_LEVEL_4  ||
	         ccb->msg.proto_lvl < MSG_PROTO_5 )
	    {
		gcd_release_sp( ccb, NULL );
	    	done = TRUE;
		break;
	    }

	    ccb->sequence = XACT_END;
	    purge = TRUE;

	    pcb->data.tran.xa_flags = xa_flags;
	    pcb->data.tran.distXID.ti_type = IIAPI_TI_XAXID;
	    pcb->data.tran.distXID.ti_value.xaXID.xa_branchSeqnum = 
		GCD_XID_SEQNUM;
	    pcb->data.tran.distXID.ti_value.xaXID.xa_branchFlag = 
		GCD_XID_FLAGS;
	    STRUCT_ASSIGN_MACRO( xid, 
			pcb->data.tran.distXID.ti_value.xaXID.xa_tranID );
	    break;

	default :
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD invalid XACT operation: %d\n", 
			    ccb->id, req_id );
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	if ( status != OK )
	{
	    /*
	    ** Fatal error, abort connection.
	    */
	    gcd_del_pcb( pcb );
	    gcd_sess_abort( ccb, status );
	    break;
	}

	if ( done )
	{
	    /*
	    ** No further processing required.
	    */
	    gcd_send_done( ccb, &pcb->rcb, NULL );
	    gcd_del_pcb( pcb );
	    gcd_msg_pause( ccb, TRUE );
	    break;
	}

	if ( purge )
	{
	    /*
	    ** Make sure all statements have been closed
	    ** prior to transaction processing.
	    */
	    gcd_push_callback( pcb, msg_xact_sm );
	    gcd_purge_stmt( pcb );
	    break;
	}

	/*
	** Begin transaction processing.
	*/
	msg_xact_sm( (PTR)pcb );
	break;
    }

    /*
    ** Free resources allocated in this routine.
    */
    if ( sp )  MEfree( (PTR)sp );

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
**	14-Feb-03 (gordy)
**	    Added support for savepoints.
**	13-May-05 (gordy)
**	    Added support for aborting transactions from other sessions.
**	 3-Jun-05 (gordy)
**	    Check for connection and transaction aborts.
**	11-Oct-05 (gordy)
**	    Also check for logical processing errors.
**	21-Apr-06 (gordy)
**	    Don't need to pre-allocate RCB.
**	28-Jun-06 (gordy)
**	    OK, I was wrong.  Pre-allocate RCB so error messages will
**	    be returned to client.
**	14-Jul-06 (gordy)
**	    Added support for XA requests.
*/

static void
msg_xact_sm( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;

    /*
    ** If not yet allocated, allocate the RCB.
    */
    if ( ! pcb->rcb  &&  ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	return;
    }

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Xact: %s\n",
		   ccb->id, gcu_lookup( xactSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case XACT_COMMIT :				/* Commit */
	gcd_release_sp( ccb, NULL );
	ccb->sequence = XACT_NEW_ID;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_commit( pcb );
	return;

    case XACT_ROLLBACK :			/* Rollback */
	gcd_release_sp( ccb, NULL );
	ccb->sequence = XACT_NEW_ID;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_rollback( pcb );
	return;
    
    case XACT_SAVEPOINT :			/* Savepoint */
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_savepoint( pcb );
	return;

    case XACT_SP_CHK :				/* Check savepoint status */
	/*
	** If an error occured, we need to free the savepoint
	** control block.  It is at the top of the list, so
	** we can simply release the first savepoint.
	*/
	if ( pcb->api_error )  gcd_release_sp( ccb, ccb->xact.savepoints );
	ccb->sequence = XACT_CHECK;
	break;

    case XACT_ROLL_SP :				/* Rollback to Savepoint */
	ccb->sequence = XACT_CHECK;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_rollback( pcb );
	return;
    
    case XACT_AC_OFF : 				/* Disable autocommit */
	ccb->cib->autocommit = FALSE;
	ccb->sequence = XACT_NEW_ID;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_autocommit( pcb, FALSE );	
	return;

    case XACT_BEGIN :				/* Begin Distributed */
	/*
	** Register the distributed transaction ID.
	*/
	if ( ! gcd_api_regXID( pcb->ccb, &pcb->data.tran.distXID ) )
	{
	    gcd_sess_error( ccb, &pcb->rcb, E_GC4816_XACT_REG_XID );
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

	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_query( pcb );
	return;

    case XACT_BEG_GQI :				/* Get query info */
	if ( pcb->api_error )
	{
	    ccb->sequence = XACT_CANCEL;

	    /*
	    ** The distributed XID is placed in the transaction 
	    ** handle to start a distributed transaction.  A 
	    ** real transaction handle should be returned and
	    ** will need to be aborted.  If no new transaction, 
	    ** clear the transaction handle and release XID.
	    */ 
	    if ( ccb->cib->tran != ccb->xact.distXID ) 
		ccb->cib->flags |= GCD_XACT_ABORT;
	    else
	    {
		ccb->cib->tran = NULL;
		gcd_api_relXID( ccb );
	    }
	    break;
	}

	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_gqi( pcb );
	return;

    case XACT_BEG_CHK :				/* Get query info */
	if ( pcb->api_error )  ccb->cib->flags |= GCD_XACT_ABORT;
	ccb->sequence = XACT_CLOSE;
	break;

    case XACT_PREPARE :				/* Prepare (2PC) */
	gcd_release_sp( ccb, NULL );
	ccb->sequence = XACT_CHECK;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_prepare( pcb );
	return;

    case XACT_START :				/* Start XA transaction */
	ccb->sequence = XACT_CHECK;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_xaStart( pcb );
        return;

    case XACT_END :				/* End XA association */
	gcd_release_sp( ccb, NULL );
	ccb->sequence = XACT_NEW_ID;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_xaEnd( pcb );
	return;

    case XACT_XA_PREP :				/* Prepare XA transaction */
	ccb->sequence = XACT_DONE;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_xaPrepare( pcb );
        return;

    case XACT_XA_COMM :				/* Commit XA transaction */
	ccb->sequence = XACT_DONE;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_xaCommit( pcb );
    	return;

    case XACT_XA_ROLL :				/* Rollback XA transaction */
	ccb->sequence = XACT_DONE;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_xaRollback( pcb );
    	return;

    case XACT_ABORT_1 :				/* Abort Distributed */
	/*
	** First issue statement required to permit
	** a forcefully abort of a transaction in 
	** another session.
	*/
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = FALSE;
	pcb->data.query.text = forceAbort;

	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_query( pcb );
	return;

    case XACT_AB1_GQI :				/* Get query info */
	/*
	** Cleanup query resources.
	*/
	if ( pcb->api_error )
	{
	    ccb->sequence = XACT_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_gqi( pcb );
	return;

    case XACT_AB1_CLOSE :			/* Close query */
	if ( pcb->api_error )  ccb->sequence = XACT_CHECK;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_close( pcb, NULL );
	return;
	
    case XACT_ABORT_2 :				/* Abort Distributed */
	/*
	** Now abort the distributed transaction.
	*/
	pcb->data.query.type = IIAPI_QT_QUERY;
	pcb->data.query.params = FALSE;
	pcb->data.query.text = ccb->qry.qtxt;

	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_query( pcb );
	return;

    case XACT_AB2_GQI :				/* Get query info */
	/*
	** Cleanup query resources.
	*/
	if ( pcb->api_error )
	{
	    ccb->sequence = XACT_CANCEL;
	    break;
	}

	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_gqi( pcb );
	return;

    case XACT_AB2_CLOSE :			/* Close query */
	/*
	** Our actions should not have started a transaction,
	** but an API transaction handle was created.  The
	** handle is freed by issuing a local rollback once
	** the statement resources are freed.
	*/
	ccb->sequence = XACT_ROLLBACK;
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_close( pcb, NULL );
	return;
	
    case XACT_NEW_ID :				/* New Transaction ID */
	ccb->sequence = XACT_CHECK;
	ccb->xact.tran_id++;
	ccb->stmt.stmt_id = 0;
	break;

    case XACT_CANCEL :				/* Cancel query */
	gcd_msg_flush( ccb );
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = XACT_DONE;
	    break;
	}
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_cancel( pcb );
	return;

    case XACT_CLOSE :				/* Close query */
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	{
	    ccb->sequence = XACT_DONE;
	    break;
	}
	gcd_push_callback( pcb, msg_xact_sm );
	gcd_api_close( pcb, NULL );
	return;
	
    case XACT_CHECK :
	if ( ccb->cib->flags & (GCD_CONN_ABORT | GCD_LOGIC_ERR) )
	    break;	/* Handled below */

	if ( ccb->cib->flags & GCD_XACT_ABORT )
	{
	    gcd_push_callback( pcb, msg_xact_sm );
	    gcd_xact_abort( pcb );
	    return;
	}
	break;

    case XACT_DONE :				/* Processing completed */
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_del_pcb( pcb );

	if ( ccb->cib->flags & GCD_CONN_ABORT )
	    gcd_sess_abort( ccb, FAIL );
	else  if ( ccb->cib->flags & GCD_LOGIC_ERR )
	    gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	else
	    gcd_msg_pause( ccb, TRUE );
	return;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid xact processing sequence: %d\n",
		       ccb->id, --ccb->sequence );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR);
	return;
    }

    goto top;
}


/*
** Name: gcd_xact_abort
**
** Description:
**	Cleanup session state after a transaction abort
**	is detected.
**
** Input:
**	pcb
**
** Output:
**	None.
**
** Returns:
**	void
**
** History:
**	 3-Jun-05 (gordy)
**	    Created.
**	11-Oct-05 (gordy)
**	    Check for autocommit transactions.
*/

void
gcd_xact_abort( GCD_PCB *pcb )
{
    /*
    ** Begin by purging active statements.
    */
    gcd_push_callback( pcb, abort_cmpl );
    gcd_purge_stmt( pcb );
    return;
}

static void
abort_cmpl( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;

    pcb->ccb->cib->flags &= ~GCD_XACT_ABORT;

    /*
    ** If there is an active transaction handle, rollback 
    ** to free resources.  The completion callback returns
    ** to this routine for clean-up (no transaction handle).
    **
    ** We should not be in autocommit mode, but check that
    ** state just in case since an autocommit transaction
    ** handle cannot be rolled back.
    **
    ** Aborting a transaction may close a cursor, so check 
    ** the transaction processing state for that event 
    ** (gcd_xact_check() will pop our callback).
    */
    if ( ! pcb->ccb->cib->tran )
	gcd_xact_check( pcb, GCD_XQOP_CURSOR_CLOSE );
    else  if ( pcb->ccb->cib->autocommit )
    {
	pcb->ccb->cib->autocommit = FALSE;
	pcb->ccb->xact.tran_id++;
	pcb->ccb->stmt.stmt_id = 0;
	gcd_push_callback( pcb, abort_cmpl );
	gcd_api_autocommit( pcb, FALSE );	
    }
    else
    {
	pcb->data.tran.savepoint = NULL;
	pcb->ccb->xact.tran_id++;
	pcb->ccb->stmt.stmt_id = 0;
	gcd_push_callback( pcb, abort_cmpl );
	gcd_api_rollback( pcb );
    }

    return;
}


/*
** Name: gcd_xact_check
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
**	GCD server autocommit modes provide various forms
**	of support to avoid problems with autocommit and
**	open cursors.  The supported autocommit modes are 
**	as follows:
**
**   GCD_XACM_DBMS
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
**   GCD_XACM_SINGLE
**
**	Autocommit is also handled by the DBMS, but transaction
**	errors are avoided by closing open cursors when a new 
**	cursor open or non-cursor request is made.  This mode 
**	also supports the optimization done by GCD_XACM_DBMS.
**
**   GCD_XACM_MULTI
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
**		    GCD_XQOP_NON_CURSOR
**		    GCD_XQOP_CURSOR_OPEN
**		    GCD_XQOP_CURSOR_UPDATE
**		    GCD_XQOP_CURSOR_CLOSE
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
gcd_xact_check( GCD_PCB *pcb, u_i1 xqop )
{
    GCD_CCB	*ccb = pcb->ccb;

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD %s %s %s\n", ccb->id, 
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
    if ( ! (ccb->cib->autocommit  ||  ccb->xact.xacm_multi) )
    {
	gcd_pop_callback( pcb );
	return;
    }

    switch( ccb->xact.auto_mode )
    {
    case GCD_XACM_DBMS :
	/*
	** Autocommit is being handled by the DBMS, but we don't
	** enable autocommit until actually needed.  If the current 
	** operation is initiating a query and autocommit is not 
	** enabled, then enable autocommit.
	**
	** Otherwise, nothing to do.
	*/
	switch( xqop )
	{
	case GCD_XQOP_NON_CURSOR :
	case GCD_XQOP_CURSOR_OPEN :
	    if ( ! ccb->cib->tran )  
	    {
		gcd_api_autocommit( pcb, TRUE );
	    	return;
	    }
	    break;
	}
	break;
    
    case GCD_XACM_SINGLE :
	/*
	** Autocommit is being handled by the DBMS and
	** only a single cursor is permitted to be open.
	** An open cursor will be closed if an open 
	** cursor or non-cursor request is made to avoid 
	** an error when sending the request to the DBMS.
	**
	** Updating or closing a cursor has no effect in
	** this mode.
	**
	** Also, autocommit is not enabled until needed.
	** Enable autocommit if not enabled and operation
	** is initiating a query.
	*/
	switch( xqop )
	{
	case GCD_XQOP_NON_CURSOR :
	case GCD_XQOP_CURSOR_OPEN :
	    if ( ccb->cib->tran )
		gcd_purge_stmt( pcb );
	    else
		gcd_api_autocommit( pcb, TRUE );
	    return;
	}
	break;

    case GCD_XACM_MULTI :
	/*
	** Autocommit handled by DBMS only when cursors are
	** not open.  A normal transaction is used while
	** cursors are open and commited once all cursors
	** are closed.
	*/
	switch( xqop )
	{
	case GCD_XQOP_NON_CURSOR :
	    /*
	    ** Turn on autocommit if not enabled.
	    */
	    if ( ccb->cib->autocommit  &&  ! ccb->cib->tran )
	    {
		gcd_api_autocommit( pcb, TRUE );
	    	return;
	    }
	    break;

	case GCD_XQOP_CURSOR_OPEN :
	    /*
	    ** Turn off autocommit to prepare for a normal 
	    ** transaction activated by the cursor open.
	    */
	    if ( ! ccb->xact.xacm_multi )
	    {
		ccb->cib->autocommit = FALSE;
		ccb->xact.xacm_multi = TRUE;

		if ( ccb->cib->tran )
		{
		    gcd_api_autocommit( pcb, FALSE );
	    	    return;
		}
	    }
	    break;

	case GCD_XQOP_CURSOR_CLOSE :
	    /*
	    ** If no cursors are open, commit the
	    ** normal transaction activated by the
	    ** open cursor request and return our
	    ** state to autocommit.
	    **
	    ** We should not reach this point if
	    ** autocommit is actually enabled
	    ** (should be simulated autocommit),
	    ** but check for non-autocommit mode
	    ** just to be safe.
	    */
	    if ( ccb->xact.xacm_multi  &&  ! gcd_active_stmt( ccb ) )
	    {
		ccb->xact.xacm_multi = FALSE;
		ccb->cib->autocommit = TRUE;

		if ( ccb->cib->tran )
		{
		    gcd_api_commit( pcb );
		    return;
	        }
	    }
	    break;
	}
	break;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid xact autocommit mode: %d\n",
		       ccb->id, ccb->xact.auto_mode );
	break;
    }

    gcd_pop_callback( pcb );
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
**          VMS compiler complains *(data++) << 24 ....  
**	    Change it to an equivalent statement.
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

