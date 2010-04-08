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
** Name: jdbcconn.c
**
** Description:
**	JDBC connection message processing.
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	18-Oct-99 (gordy)
**	    Extracted to this file.
**	 2-Nov-99 (gordy)
**	    User ID and password are required.
**	 4-Nov-99 (gordy)
**	    Added connection timeout.
**	 4-Nov-99 (gordy)
**	    Updated calling sequence of jdbc_send_done() to allow
**	    control of ownership of RCB parameter.
**	 6-Jan-00 (gordy)
**	    Connection info now saved in JDBC_CIB structure.  Mark 
**	    autocommit on (requested) as initial state and disable
**	    autocommit if on at disconnect time.  Added a connection
**	    parameter to configure connection pooling.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	 3-Mar-00 (gordy)
**	    Check for client limit and abort connection when exceeded.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Statements purged
**	    using jdbc_purge_stmt().
**	13-Oct-00 (gordy)
**	    Simplify organization by combining completion routines
**	    into state machines.  Abort connection if error while
**	    establishing connection.  If a non-autocommit transaction
**	    is active when disconnect requested, commit the transaction
**	    to duplicate the behaviour of ESQL.
**	17-Oct-00 (gordy)
**	    Added autocommit mode connection parameter and values.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	    Added support for XID parameters and connecting to an existing
**	    distributed transaction.
**	10-May-01 (gordy)
**	    When processing connection parameters, need to return to same
**	    sequence point to check for additional parameters.
*/	

#define	CONN_XID	0
#define	CONN_PARMS	1
#define	CONN_CONN	2
#define	CONN_DONE	3
#define	CONN_ABORT	10
#define	CONN_ERROR	11

static	GCULIST	connSeqMap[] =
{
    { CONN_XID,		"XID" },
    { CONN_PARMS,	"PARAMS" },
    { CONN_CONN,	"CONNECT" },
    { CONN_DONE,	"DONE" },
    { CONN_ABORT,	"ABORT" },
    { CONN_ERROR,	"ERROR" },
};

/*
** Parameter flags.
*/
#define	CP_II_XID	0x01
#define	CP_FRMT		0x02
#define	CP_GTID		0x04
#define	CP_BQUAL	0x08
#define	CP_XA_XID	(CP_FRMT | CP_GTID | CP_BQUAL)

#define	DISC_COMMIT	0
#define	DISC_POOL	1
#define	DISC_AUTO	2
#define	DISC_DISC	3
#define	DISC_ABORT	10
#define	DISC_DONE	11

static	GCULIST	discSeqMap[] =
{
    { DISC_COMMIT,	"COMMIT" },
    { DISC_POOL,	"POOL" },
    { DISC_AUTO,	"AUTOCOMMIT" },
    { DISC_DISC,	"DISCONNECT" },
    { DISC_ABORT,	"ABORT" },
    { DISC_DONE,	"DONE" },
};

static  void	msg_connect_sm( PTR );
static	void	msg_disc_sm( PTR );


/*
** Name: jdbc_msg_connect
**
** Description:
**	Process a JDBC connection message.
**
** Input:
**	ccb	Connection control block.
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
**	17-Sep-99 (rajus01)
**	    Added more connection parameters.
**	18-Oct-99 (gordy)
**	    Renamed for external reference.
**	 2-Nov-99 (gordy)
**	    User ID and password are required.
**	 4-Nov-99 (gordy)
**	    Added connection timeout.
**	 6-Jan-00 (gordy)
**	    Connection info now saved in JDBC_CIB structure.
**	    Added connection parameter to control connection
**	    pooling.  Autocommit is on (requested) by default.
**	27-Jan-00 (gordy)
**	    Do not free CIB at end of routine with other resources.
**	    If the CIB is allocated, execution never reaches the
**	    end of the routine.  If the end of the routine is
**	    reached, the CIB was never allocated.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	 3-Mar-00 (gordy)
**	    Check for client limit and abort connection when exceeded.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.
**	13-Oct-00 (gordy)
**	    Completion routines combined into state machine.
**	    Make sure ccb->cib->conn is only set when calling
**	    into state machine so that any abort condition
**	    will not mess with the (non-existent) connection.
**	17-Oct-00 (gordy)
**	    Added autocommit mode connection parameter and values.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**	    Added support for XID parameters and connecting to an existing
**	    distributed transaction.
**	10-May-01 (gordy)
**	    When processing connection parameters, need to return to same
**	    sequence point to check for additional parameters.
*/

void
jdbc_msg_connect( JDBC_CCB *ccb )
{
    JDBC_PCB		*pcb;
    JDBC_CIB		*cib;
    JDBC_CIB		*pool;
    STATUS		status = OK;
    bool		incomplete = FALSE;
    char		*db = NULL;
    char		*usr = NULL;
    char		*pwd = NULL;
    bool		pool_on = FALSE;
    bool		pool_off = FALSE;
    IIAPI_II_TRAN_ID	iid;
    IIAPI_XA_TRAN_ID	xid;
    u_i2		i, pwd_len;
    u_i2		xact_2pc = 0;
    u_i2		count = 0;
    CONN_PARM		parms[ JDBC_CP_MAX ];

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC DBMS connect request\n", ccb->id );

    if ( ! (pcb = jdbc_new_pcb( ccb, TRUE )) )
    {
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	if ( pcb )  jdbc_del_pcb( pcb );
	return;
    }

    MEfill( sizeof( xid ), 0, (PTR)&xid );
    pcb->data.conn.timeout = -1;	/* Default full blocking */

    while( ccb->msg.msg_len )
    {
	JDBC_MSG_PM cp;

	incomplete = TRUE;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&cp.param_id ) )  break;
	if ( ! jdbc_get_i2( ccb, (u_i1 *)&cp.val_len ) )  break;

	switch( cp.param_id )
	{
	case JDBC_CP_DATABASE :
	    if ( ! (db = (char *)MEreqmem(0, cp.val_len + 1, TRUE, NULL))  ||
	         ! jdbc_get_bytes( ccb, cp.val_len, (u_i1 *)db ) )
		break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     Database: '%s'\n", ccb->id, db );

	    incomplete = FALSE;
	    break;
	
	case JDBC_CP_USERNAME :
	    if ( ! (usr = (char *)MEreqmem(0, cp.val_len + 1, TRUE, NULL))  ||
		 ! jdbc_get_bytes( ccb, cp.val_len, (u_i1 *)usr ) )
		break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     Username: '%s'\n", ccb->id, usr );

	    incomplete = FALSE;
	    break;
	
	case JDBC_CP_PASSWORD :
	    if ( ! (pwd = (char *)MEreqmem(0, cp.val_len + 1, TRUE, NULL))  ||
		 ! jdbc_get_bytes( ccb, cp.val_len, (u_i1 *)pwd ) )
		break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     Password: *****\n", ccb->id );

	    pwd_len = cp.val_len;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_DBUSERNAME :
	    if ( count >= JDBC_CP_MAX  || 
		 ! (parms[count].value = 
			 (char *)MEreqmem( 0, cp.val_len + 1, TRUE, NULL ))  ||
		 ! jdbc_get_bytes(ccb, cp.val_len, (u_i1 *)parms[count].value) )
	        break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     DBMS username: '%s'\n",
			   ccb->id, parms[count].value );

	    parms[count++].id = IIAPI_CP_EFFECTIVE_USER;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_GROUP_ID :
	    if ( count >= JDBC_CP_MAX  || 
		 ! (parms[count].value = 
			(char *)MEreqmem( 0, cp.val_len + 1, TRUE, NULL ))  ||
		 ! jdbc_get_bytes(ccb, cp.val_len, (u_i1 *)parms[count].value) )
	        break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     Group ID: '%s'\n",
			   ccb->id, parms[count].value );

	    parms[count++].id = IIAPI_CP_GROUP_ID;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_ROLE_ID :
	    if ( count >= JDBC_CP_MAX  ||
		 ! (parms[count].value = 
			(char *)MEreqmem( 0, cp.val_len + 1, TRUE, NULL ))  ||
		 ! jdbc_get_bytes(ccb, cp.val_len, (u_i1 *)parms[count].value) )
	        break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     Role ID: '%s'\n",
			   ccb->id, parms[count].value );

	    parms[count++].id = IIAPI_CP_APP_ID;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_DBPASSWORD :
	    if ( count >= JDBC_CP_MAX  ||
		 ! (parms[count].value =
			(char *)MEreqmem( 0, cp.val_len + 1, TRUE, NULL ))  ||
		 ! jdbc_get_bytes(ccb, cp.val_len, (u_i1 *)parms[count].value) )
	        break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     DB Password: *****\n", ccb->id);

	    parms[count++].id = IIAPI_CP_DBMS_PASSWORD;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_TIMEOUT :
	    if ( cp.val_len != 4  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&pcb->data.conn.timeout ) )
		break;
	    
	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     Timeout: %d\n", 
			   ccb->id, pcb->data.conn.timeout );

	    incomplete = FALSE;
	    break;

	case JDBC_CP_CONNECT_POOL :
	    {
		u_i1 val;

		if ( cp.val_len != 1  ||  ! jdbc_get_i1( ccb, (u_i1 *)&val ) )
		    break;
	    
		switch( val )
		{
		case JDBC_CPV_POOL_OFF	: pool_off = TRUE;	break;
		case JDBC_CPV_POOL_ON	: pool_on = TRUE;	break;
		}

		if ( JDBC_global.trace_level >= 3 )
		    TRdisplay( "%4d    JDBC     Connect Pool: %s\n", ccb->id, 
				pool_on ? "on" : (pool_off ? "off" : "?") );

		incomplete = FALSE;
	    }
	    break;

	case JDBC_CP_AUTO_MODE :
	    {
		u_i1 val;
		char *str = "?";

		if ( cp.val_len != 1  ||  ! jdbc_get_i1( ccb, (u_i1 *)&val ) )
		    break;
	    
		switch( val )
		{
		case JDBC_CPV_XACM_DBMS :   
		    ccb->xact.auto_mode = JDBC_XACM_DBMS;
		    str = "DBMS";
		    break;

		case JDBC_CPV_XACM_SINGLE :
		    ccb->xact.auto_mode = JDBC_XACM_SINGLE;
		    str = "SINGLE";
		    break;

		case JDBC_CPV_XACM_MULTI :
		    ccb->xact.auto_mode = JDBC_XACM_MULTI;
		    str = "MULTI";
		    break;
		}

		if ( JDBC_global.trace_level >= 3 )
		    TRdisplay( "%4d    JDBC     Autocommit Mode: %s\n", 
				ccb->id, str );

		incomplete = FALSE;
	    }
	    break;

	case JDBC_CP_II_XID :
	    if ( cp.val_len != (CV_N_I4_SZ * 2)  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&iid.it_lowTran )  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&iid.it_highTran ) )
		break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     Ingres XID: 0x%x, 0x%x\n", 
			    ccb->id, iid.it_highTran, iid.it_lowTran );

	    xact_2pc |= CP_II_XID;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_XA_FRMT :
	    if ( cp.val_len != CV_N_I4_SZ  ||
		 ! jdbc_get_i4( ccb, (u_i1 *)&xid.xt_formatID ) )
		break;

	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     XA FormatID: 0x%x\n", 
			    ccb->id, xid.xt_formatID );

	    xact_2pc |= CP_FRMT;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_XA_GTID :
	    if ( cp.val_len > IIAPI_XA_MAXGTRIDSIZE )  break;

	    if ( xid.xt_bqualLength )
	    {
		/* BQUAL follows GTRID in data array */
		int i;

		for( i = xid.xt_bqualLength - 1; i >= 0; i-- )
		    xid.xt_data[ cp.val_len + i ] = xid.xt_data[ i ];
	    }

	    if ( ! jdbc_get_bytes( ccb, cp.val_len, (u_i1 *)&xid.xt_data[0] ) )
		break;
	    
	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     XA GTID: len = %d\n", 
			    ccb->id, cp.val_len );

	    xid.xt_gtridLength  = cp.val_len;
	    xact_2pc |= CP_GTID;
	    incomplete = FALSE;
	    break;

	case JDBC_CP_XA_BQUAL :
	    if ( cp.val_len > IIAPI_XA_MAXBQUALSIZE  ||
		 ! jdbc_get_bytes( ccb, cp.val_len, 
				   (u_i1 *)&xid.xt_data[xid.xt_gtridLength] ) )
		break;
	    
	    if ( JDBC_global.trace_level >= 3 )
		TRdisplay( "%4d    JDBC     XA BQual: len = %d\n", 
			    ccb->id, cp.val_len );

	    xid.xt_bqualLength = cp.val_len;
	    xact_2pc |= CP_BQUAL;
	    incomplete = FALSE;
	    break;

	default :
	    if ( JDBC_global.trace_level >= 1 )
		TRdisplay( "%4d    JDBC     unkown parameter ID %d\n",
			   ccb->id, cp.param_id );
	    break;
	}

	if ( incomplete )  break;
    }

    for( ; status == OK; )
    {
	/*
	** Handle invalid message parameters.
	*/
	if ( incomplete )  
	{
	    status = E_JD010A_PROTOCOL_ERROR;
	    break;
	}

	/*
	** Username and password are required for a standard
	** connection, and transaction IDs are not permitted.
	*/
	if ( ! ccb->conn_info.dtmc )
	    if ( ! usr  ||  ! pwd )
	    {
		status = E_JD010C_NO_AUTH;
		break;
	    }
	    else  if ( xact_2pc )
	    {
		status = E_JD010A_PROTOCOL_ERROR;
		break;
	    }

	/*
	** Make sure a valid distributed transaction ID
	** (Ingres or XA) has been received.
	*/
	if ( xact_2pc  &&  xact_2pc != CP_II_XID  &&  xact_2pc != CP_XA_XID )
	{
	    status = E_JD010A_PROTOCOL_ERROR;
	    break;
	}

	if ( ! (cib = jdbc_new_cib( count )) )
	{
	    status = E_JD0108_NO_MEMORY;
	    break;
	}

	/*
	** The messaging sub-system needs the connection control block
	** to remain active during the lifetime of a connection handle.
	** The connection handle is initiated here, so we bump the use
	** count in the CCB.  The CCB will be released/freed when the
	** connection is gone (either a disconnect request, or session
	** terminated).
	*/
	ccb->use_cnt++;
	ccb->cib = cib;

	if ( ccb->conn_info.dtmc )
	{
	    /*
	    ** Distributed transaction management connections
	    ** require a new connection and disable autocommit.
	    */
	    cib->autocommit = FALSE;
	    cib->pool_on = FALSE;
	    cib->pool_off = TRUE;
	}
	else
	{
	    /*
	    ** Autocommit is the JDBC default.  This state is flagged 
	    ** but not actually enabled until the first request is made.
	    */
	    cib->autocommit = TRUE;
	    cib->pool_on = pool_on;
	    cib->pool_off = pool_off;
	}

	/*
	** Save allocated resources in the CIB.
	*/
	if ( db )  cib->database = db;
	if ( usr ) 
	{
	    cib->username = usr;

	    if ( pwd )
	    {
		jdbc_decode( usr, (u_i1 *)pwd, pwd_len, pwd );
		cib->password = pwd;
	    }
	}

	for( cib->parm_cnt = 0; cib->parm_cnt < count; cib->parm_cnt++ )  
	    STRUCT_ASSIGN_MACRO( parms[ cib->parm_cnt ], 
				 cib->parms[ cib->parm_cnt ] );
	/*
	** Since saved in CIB, don't free if error occurs.
	*/
	db = usr = pwd = NULL;
	count = 0;

	/*
	** See if there is a connection in the pool
	** which matches the request.  If so, use the
	** pool CIB and free the CIB allocated above.
	*/
	if ( (pool = jdbc_pool_find( cib )) )
	{
	    /*
	    ** Autocommit is the JDBC default.  This state is flagged 
	    ** but not actually enabled until the first request is made
	    ** (only if no carried over active transaction).
	    */
	    if ( ! pool->tran )  pool->autocommit = TRUE;
	    ccb->cib = pool;
	    jdbc_del_cib( cib );
	    ccb->sequence = CONN_DONE;
	    msg_connect_sm( (PTR)pcb );
	    return;
	}

	/*
	** No connection in the pool.  Create a new connection
	** if we have not reached our limit or we can bump a
	** connection from the pool.
	*/
	if ( 
	     JDBC_global.cib_max <= 0  ||
	     JDBC_global.cib_active <= JDBC_global.cib_max  ||  
	     jdbc_pool_flush( JDBC_global.cib_active - JDBC_global.cib_max ) 
	   )
	{
	    pcb->data.conn.database = cib->database;
	    pcb->data.conn.username = cib->username;
	    pcb->data.conn.password = cib->password;
	    pcb->data.conn.param_cnt = cib->parm_cnt;
	    pcb->data.conn.parms = cib->parms;

	    switch( xact_2pc )
	    {
	    case CP_II_XID :
		pcb->data.conn.distXID.ti_type = IIAPI_TI_IIXID;
		STcopy( JDBC_XID_NAME,
			pcb->data.conn.distXID.ti_value.iiXID.ii_tranName );
		pcb->data.conn.distXID.ti_value.iiXID.ii_tranID.it_highTran =
		    iid.it_highTran;
		pcb->data.conn.distXID.ti_value.iiXID.ii_tranID.it_lowTran =
		    iid.it_lowTran;
		ccb->sequence = CONN_XID;
		break;
	    
	    case CP_XA_XID :
		pcb->data.conn.distXID.ti_type = IIAPI_TI_XAXID;
		pcb->data.conn.distXID.ti_value.xaXID.xa_branchSeqnum = 
		    JDBC_XID_SEQNUM;
		pcb->data.conn.distXID.ti_value.xaXID.xa_branchFlag = 
		    JDBC_XID_FLAGS;

		MEcopy( (PTR)&xid, sizeof( xid ),
			(PTR)&pcb->data.conn.distXID.ti_value.xaXID.xa_tranID );
		ccb->sequence = CONN_XID;
		break;
	    
	    default :
		ccb->sequence = CONN_PARMS;
		break;
	    }

	    ccb->cib->conn = ccb->api.env;
	    msg_connect_sm( (PTR)pcb );
	    return;
	}

	/*
	** Server has reached its connection limit.
	*/
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC client limit reached: %d of %d\n",
		       ccb->id, JDBC_global.cib_active, JDBC_global.cib_max );

	status = E_JD010E_CLIENT_MAX;
	break;
    }

    if ( JDBC_global.trace_level >= 1 )
	TRdisplay( "%4d    JDBC Error processing connection request: 0x%x\n",
		   ccb->id, status );

    jdbc_sess_error( ccb, &pcb->rcb, status );
    jdbc_send_done( ccb, &pcb->rcb, pcb );
    jdbc_gcc_abort( ccb, status );

    /*
    ** Free resources allocated in this routine.
    */
    if ( pcb )  jdbc_del_pcb( pcb );
    if ( db )   MEfree( (PTR)db );
    if ( usr )  MEfree( (PTR)usr );
    if ( pwd )  MEfree( (PTR)pwd );
    for( i = 0; i < count; i++ )  MEfree( (PTR)parms[i].value );

    return;
}

static void
msg_connect_sm( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;

  top:

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC connect seq %s\n",
		   ccb->id, gcu_lookup( connSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case CONN_XID :
	if ( ! jdbc_api_regXID( ccb, &pcb->data.conn.distXID ) )
	{
	    jdbc_sess_error( ccb, &pcb->rcb, E_JD0116_XACT_REG_XID );
	    ccb->sequence = CONN_ABORT;
	}
	break;

    case CONN_PARMS :			/* Connection parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = CONN_ABORT;
	    break;
	}

	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC connect SM: parameter count %d\n",
		       ccb->id, pcb->data.conn.param_cnt );

	if ( pcb->data.conn.param_cnt )
	{
	    ccb->sequence = CONN_PARMS;
	    jdbc_push_callback( pcb, msg_connect_sm );
	    jdbc_api_setConnParms( pcb );
	    return;
	}
	break;

    case CONN_CONN :			/* Connect */
	jdbc_push_callback( pcb, msg_connect_sm );
	jdbc_api_connect( pcb ); 
	return;

    case CONN_DONE :			/* Done */
	if ( pcb->api_error )
	{
	    ccb->sequence = CONN_ABORT;
	    break;
	}

	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_msg_pause( ccb, TRUE );
	jdbc_del_pcb( pcb );
	return;

    case CONN_ABORT :			/* Abort connection */
	/*
	** Remove connection handle from CIB
	** to avoid any other attempt to drop
	** the connection.  Send DONE message
	** to client and abort connection.
	*/
	pcb->data.disc.conn = ccb->cib->conn;
	ccb->cib->conn = NULL;
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_msg_pause( ccb, TRUE );
	if ( ! pcb->data.disc.conn )  break;
	jdbc_push_callback( pcb, msg_connect_sm );
	jdbc_api_abort( pcb ); 
	return;
    
    case CONN_ERROR :			/* Clean-up */
	jdbc_del_pcb( pcb );
	jdbc_del_ccb( ccb );
	return;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid connect sequence: %d\n",
		       ccb->id, --ccb->sequence );
	jdbc_del_pcb( pcb );
	jdbc_gcc_abort( ccb, E_JD0109_INTERNAL_ERROR );
	return;
    }

    goto top;
}


/*
** Name: jdbc_msg_disconnect
**
** Description:
**	Process a JDBC disconnect message.
**
** Input:
**	ccb	Connection control block.
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
**	18-Oct-99 (gordy)
**	    Renamed for external reference.
**	 6-Jan-00 (gordy)
**	    Connection info now saved in JDBC_CIB structure.
**	    Disable autocommit if enabled.  A race condition
**	    can exist between disconnecting the client and the
**	    DBMS server.  If the client completes before we do,
**	    it can cause an additional attempt to disconnect.
**	    To avoid this race condition, the connection handle
**	    is removed from global access once we begin the
**	    disconnect process.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB.
**	11-Oct-00 (gordy)
**	    Callbacks now maintained on stack.  Statements purged
**	    using jdbc_purge_stmt().
**	13-Oct-00 (gordy)
**	    Completion routines combined into state machine.
**	    Always purge statements.  Commit non-autocommit
**	    transactions.  Errors are now returned to client.
*/

void
jdbc_msg_disconnect( JDBC_CCB *ccb )
{
    JDBC_RCB	*rcb;
    JDBC_PCB	*pcb;

    if ( ccb->msg.msg_len )
    {
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC unexpected data with disconnect: %d\n", 
			ccb->id, ccb->msg.msg_len );
	jdbc_gcc_abort( ccb, E_JD010A_PROTOCOL_ERROR );
	return;
    }

    /*
    ** Check to see if the DBMS connection 
    ** shutdown has already been initiated.
    */
    if ( ! ccb->cib  ||  ! ccb->cib->conn )
    {
	if ( JDBC_global.trace_level >= 3 )
	    TRdisplay( "%4d    JDBC Disconnect (completed)\n", ccb->id );
	jdbc_send_done( ccb, NULL, NULL );
	return;
    }

    if ( JDBC_global.trace_level >= 3 )
	TRdisplay( "%4d    JDBC Disconnect\n", ccb->id );

    if ( ! (pcb = jdbc_new_pcb( ccb, TRUE )) )
    {
	/*
	** We can't disconnect or abort the connection
	** if we can't allocate memory.  The memory
	** allocation error has been logged, so just
	** try and clean-up the CCB.
	*/
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC can't alloc pcb to disconnect/abort\n",
			ccb->id );
	ccb->cib->conn = ccb->cib->tran = NULL;
	jdbc_gcc_abort( ccb, E_JD0108_NO_MEMORY );
	jdbc_del_ccb( ccb );
	return;
    }

    /*
    ** Musical pointers (part 1): Stash the 
    ** connection handle in the PCB so that 
    ** client disconnect code will not attempt 
    ** to abort the connection if the client 
    ** disconnect completes before the DBMS.
    */
    pcb->data.disc.conn = ccb->cib->conn;
    ccb->cib->conn = NULL;

    /*
    ** We begin the disconnect process by
    ** closing any active statements.
    */
    ccb->sequence = DISC_COMMIT;
    jdbc_push_callback( pcb, msg_disc_sm );
    jdbc_purge_stmt( pcb );
    return;
}

static void
msg_disc_sm( PTR arg )
{
    JDBC_PCB	*pcb = (JDBC_PCB *)arg;
    JDBC_CCB	*ccb = pcb->ccb;
    JDBC_CIB	*cib;

  top:

    if ( JDBC_global.trace_level >= 4 )
	TRdisplay( "%4d    JDBC disconnect seq %s\n",
		   ccb->id, gcu_lookup( discSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case DISC_COMMIT :			/* Commit transaction */
	if ( pcb->api_error )
	{
	    ccb->sequence = DISC_ABORT;
	    break;
	}

	/*
	** Skip ahead to connection pooling if
	** there is no transaction or it is an
	** autocommit transaction.
	*/
	if ( ! ccb->cib->tran  ||  ccb->cib->autocommit )  break;

	jdbc_push_callback( pcb, msg_disc_sm );
	jdbc_api_commit( pcb );
	return;

    case DISC_POOL :			/* Attempt to pool the connection */
	if ( pcb->api_error )
	{
	    ccb->sequence = DISC_ABORT;
	    break;
	}

	/*
	** Musical pointers (part 2): the connection 
	** handle was removed from the CIB to handle 
	** race conditions in the shutdown code.  We 
	** now want to attempt to save the CIB in the 
	** connection pool, so we need to restore the 
	** connection handle.  To handle the race 
	** conditions, we remove the CIB from the CCB
	** before restoring the connection handle.
	*/
	cib = ccb->cib;
	ccb->cib = NULL;
	cib->conn = pcb->data.disc.conn;

	if ( jdbc_pool_save( cib ) )
	{
	    ccb->sequence = DISC_DONE;
	    break;
	}

	/*
	** Musical pointers (part 3): since the connection
	** could not be saved, we must disconnect and ensure
	** that the CIB is released.  Normal disconnect code
	** will release the CIB if it is in the CCB, so we
	** must restore the CIB.  The connection handle must
	** be removed to avoid the afore mentioned race 
	** condition (note that the connection handle has
	** already been placed in the PCB).
	*/
	cib->conn = NULL;
	ccb->cib = cib;
	break;
    
    case DISC_AUTO :			/* Disable autocommit */
	/*
	** We committed any non-autocommit transaction
	** earlier.  Autocommit must be disabled (if
	** enabled) before disconnecting.
	*/
	if ( ccb->cib->tran )
	    if ( ccb->cib->autocommit )
	    {
		jdbc_push_callback( pcb, msg_disc_sm );
		jdbc_api_autocommit( pcb, FALSE );
		return;
	    }
	    else
	    {
		/* Shouldn't happen! */
		ccb->sequence = DISC_ABORT;
		break;
	    }
	break;
    
    case DISC_DISC :			/* Disconnect */
	if ( pcb->api_error )
	{
	    ccb->sequence = DISC_ABORT;
	    break;
	}

	ccb->sequence = DISC_DONE;
	jdbc_push_callback( pcb, msg_disc_sm );
	jdbc_api_disconnect( pcb );
	return;
    
    case DISC_ABORT :			/* Abort connection */
	ccb->sequence = DISC_DONE;
	ccb->cib->tran = NULL;
	jdbc_push_callback( pcb, msg_disc_sm );
	jdbc_api_abort( pcb );
	return;
    
    case DISC_DONE :			/* Free connection resources */
	jdbc_send_done( ccb, &pcb->rcb, pcb );
	jdbc_del_pcb( pcb );
	jdbc_del_ccb( ccb );
	return;

    default :
	if ( JDBC_global.trace_level >= 1 )
	    TRdisplay( "%4d    JDBC invalid disconnect sequence: %d\n",
		       ccb->id, --ccb->sequence );
	ccb->cib->conn = ccb->cib->tran = NULL;
	jdbc_gcc_abort( ccb, E_JD0109_INTERNAL_ERROR );
	jdbc_del_pcb( pcb );
	jdbc_del_ccb( ccb );
	return;
    }

    goto top;
}

