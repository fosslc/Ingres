/*
** Copyright (c) 2004, 2010 Ingres Corporation
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
** Name: gcdconn.c
**
** Description:
**	GCD connection message processing.
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
**	19-Aug-2002 (wansh01)
**	    Added support for login type(Local/Remote) as a connection 
**  	    parameter.
**	01-Nov-2002 (wansh01)
**          Added support for optional Userid for local connection.
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	26-Dec-02 (gordy)
**	    Add AUTO_STATE connection parameter.
**	15-Jan-03 (gordy)
**	    Added client info connection parameters.  Added gcd_sess_term()
**	    and gcd_sess_abort().
**	 7-Jul-03 (gordy)
**	    Added new connection parameters, consolidated API parameter
**	    processing in read_parms().
**	16-Jun-04 (gordy)
**	    Added session mask to decryption key of password.
**	11-Oct-05 (gordy)
**	    Check for disconnect error.
**	21-Apr-06 (gordy)
**	    Simplified PCB/RCB handling.
**	29-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	23-Jul-07 (gordy)
**	    Added connection parameter for date alias.
**	 5-Dec-07 (gordy)
**	    Client limit is now checked at GCC level.
**	25-Mar-10 (gordy)
**	    Initial connection handle now stored in API parms.
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
#define	DISC_CHK	4
#define	DISC_ABORT	10
#define	DISC_DONE	11

static	GCULIST	discSeqMap[] =
{
    { DISC_COMMIT,	"COMMIT" },
    { DISC_POOL,	"POOL" },
    { DISC_AUTO,	"AUTOCOMMIT" },
    { DISC_DISC,	"DISCONNECT" },
    { DISC_CHK,		"CHECK" },
    { DISC_ABORT,	"ABORT" },
    { DISC_DONE,	"DONE" },
};

static  void	msg_connect_sm( PTR );
static	STATUS	read_parms( GCD_CCB *, DAM_ML_PM *, CONN_PARM *, u_i2 * );
static	STATUS	read_string( GCD_CCB *, u_i2, char ** );
static	void	msg_disc_sm( PTR );


/*
** Name: gcd_msg_connect
**
** Description:
**	Process a GCD connection message.
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
**	19-Aug-2002 (wansh01)
**	    Added support for JDBC_CP_LOGIN_TYPE as a connection parameter.
**	01-Nov-2002 (wansh01)
** 	    Added support for optional userid for local connection,
**	    if JDBC_CP_LOGIN_TYPE is JDBC_CPV_LOGIN_USER, (optional
** 	    userid indicator), make sure it is a local connection and 
**	    userid and password are the same.
**	26-Dec-02 (gordy)
**	    Add AUTO_STATE connection parameter.
**	15-Jan-03 (gordy)
**	    Added client info connection parameters.
**	 7-Jul-03 (gordy)
**	    Extracted API connection parameter processing to read_parms().
**	16-Jun-04 (gordy)
**	    Added session mask to decryption key.
**	29-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
**	23-Jul-07 (gordy)
**	    Added connection parameter for date alias.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906.
**	    Added support for maintaining API environment handle for each
**	    supported client protocol level.
**	 5-Dec-07 (gordy)
**	    Client limit is now checked at GCC level.
**	04-Nov-08 (rajus01) Bug 121169, SD Issue: 131912.
**	    The API environment handle happended to be NULL when connections
**	    come out of the connection pool causing failure of API routine
**	    which in this case is IIapi_formatData. Reset the API environment
**	    for the connections from the pool.
**	25-Mar-10 (gordy)
**	    Initial connection handle (prior to IIapi_connect() completing)
**	    now stored in API parms (initialized with environment handle).
*/

void
gcd_msg_connect( GCD_CCB *ccb )
{
    GCD_PCB		*pcb;
    GCD_CIB		*cib;
    GCD_CIB		*pool;
    IIAPI_II_TRAN_ID	iid;
    IIAPI_XA_TRAN_ID	xid;
    STATUS		status = OK;
    char		*db = NULL;
    char		*usr = NULL;
    char		*pwd = NULL;
    char		*uid = NULL;
    char		*host = NULL;
    char		*addr = NULL;
    bool		pool_on = FALSE;
    bool		pool_off = FALSE;
    bool		auto_on = (! ccb->msg.dtmc);
    bool		login_user = FALSE;
    u_i4		ccb_id = ccb->id;
    u_i2		i, pwd_len;
    u_i2		xact_2pc = 0;
    u_i2		count = 0;
    CONN_PARM		parms[ GCD_API_CONN_PARM_MAX ];

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD Connect request\n", ccb_id );

    if ( ! (pcb = gcd_new_pcb( ccb ))  ||
         ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	if ( pcb )  gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	return;
    }

    MEfill( sizeof( xid ), 0, (PTR)&xid );
    pcb->data.conn.timeout = -1;	/* Default full blocking */

    while( ccb->msg.msg_len )
    {
	DAM_ML_PM cp;

	if ( ! gcd_get_i2( ccb, (u_i1 *)&cp.param_id ) )  break;
	if ( ! gcd_get_i2( ccb, (u_i1 *)&cp.val_len ) )  break;

	switch( cp.param_id )
	{
	case MSG_CP_DATABASE :
	    if ( (status = read_string( ccb, cp.val_len, &db )) != OK )
		break;

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Database: '%s'\n", ccb_id, db );
	    break;
	
	case MSG_CP_USERNAME :
	    if ( (status = read_string( ccb, cp.val_len, &usr )) != OK )
		break;

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Username: '%s'\n", ccb_id, usr );
	    break;
	
	case MSG_CP_PASSWORD :
	    if ( (status = read_string( ccb, cp.val_len, &pwd )) != OK )
		break;

	    pwd_len = cp.val_len;
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Password: *****\n", ccb_id );
	    break;

	case MSG_CP_TIMEOUT :
	    if ( cp.val_len != 4  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&pcb->data.conn.timeout ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	    
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Timeout: %d\n", 
			   ccb_id, pcb->data.conn.timeout );
	    break;

	case MSG_CP_CONNECT_POOL :
	{
	    u_i1 val;
	    char *str;

	    if ( cp.val_len != 1  ||  ! gcd_get_i1( ccb, (u_i1 *)&val ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	    
	    switch( val )
	    {
	    case MSG_CPV_POOL_OFF : 
		pool_off = TRUE;
		str = "off";
		break;

	    case MSG_CPV_POOL_ON : 
		pool_on = TRUE;
		str = "on";
		break;

	    default :
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		str = "invalid";
		break;
	    }

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Connect Pool: %s\n", ccb_id, str );
	    break;
	}

	case MSG_CP_AUTO_MODE :
	{
	    u_i1 val;
	    char *str;

	    if ( cp.val_len != 1  ||  ! gcd_get_i1( ccb, (u_i1 *)&val ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	    
	    switch( val )
	    {
	    case MSG_CPV_XACM_DBMS :   
		ccb->xact.auto_mode = GCD_XACM_DBMS;
		str = "DBMS";
		break;

	    case MSG_CPV_XACM_SINGLE :
		ccb->xact.auto_mode = GCD_XACM_SINGLE;
		str = "SINGLE";
		break;

	    case MSG_CPV_XACM_MULTI :
		ccb->xact.auto_mode = GCD_XACM_MULTI;
		str = "MULTI";
		break;

	    default :
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		str = "INVALID";
		break;
	    }

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Autocommit Mode: %s\n", 
			    ccb_id, str );
	    break;
	}

	case MSG_CP_AUTO_STATE:
	{
	    u_i1 val;
	    char *str;

	    if ( cp.val_len != 1  ||  ! gcd_get_i1( ccb, (u_i1 *)&val ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	    
	    switch( val )
	    {
	    case MSG_CPV_AUTO_OFF : 
		auto_on = FALSE;
		str = "off";
		break;

	    case MSG_CPV_AUTO_ON :
		auto_on = TRUE;
		str = "on";
		break;

	    default :
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		str = "invalid";
		break;
	    }

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay("%4d    GCD     Autocommit State: %s\n",ccb_id,str);
	    break;
	}

	case MSG_CP_II_XID :
	    if ( cp.val_len != (CV_N_I4_SZ * 2)  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&iid.it_lowTran )  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&iid.it_highTran ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }

	    xact_2pc |= CP_II_XID;
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Ingres XID: 0x%x, 0x%x\n", 
			    ccb_id, iid.it_highTran, iid.it_lowTran );
	    break;

	case MSG_CP_XA_FRMT :
	    if ( cp.val_len != CV_N_I4_SZ  ||
		 ! gcd_get_i4( ccb, (u_i1 *)&xid.xt_formatID ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }

	    xact_2pc |= CP_FRMT;
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     XA FormatID: 0x%x\n", 
			    ccb_id, xid.xt_formatID );
	    break;

	case MSG_CP_XA_GTID :
	    if ( cp.val_len > IIAPI_XA_MAXGTRIDSIZE )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }

	    if ( xid.xt_bqualLength )
	    {
		/* BQUAL follows GTRID in data array */
		int i;

		for( i = xid.xt_bqualLength - 1; i >= 0; i-- )
		    xid.xt_data[ cp.val_len + i ] = xid.xt_data[ i ];
	    }

	    if ( ! gcd_get_bytes( ccb, cp.val_len, (u_i1 *)&xid.xt_data[0] ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	    
	    xid.xt_gtridLength  = cp.val_len;
	    xact_2pc |= CP_GTID;
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     XA GTID: len = %d\n", 
			    ccb_id, cp.val_len );
	    break;

	case MSG_CP_XA_BQUAL :
	    if ( cp.val_len > IIAPI_XA_MAXBQUALSIZE  ||
		 ! gcd_get_bytes( ccb, cp.val_len, 
				   (u_i1 *)&xid.xt_data[xid.xt_gtridLength] ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	    
	    xid.xt_bqualLength = cp.val_len;
	    xact_2pc |= CP_BQUAL;
	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     XA BQual: len = %d\n", 
			    ccb_id, cp.val_len );
	    break;

	case MSG_CP_CLIENT_USER :
	    if ( (status = read_string( ccb, cp.val_len, &uid )) != OK )
		break;

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay( "%4d    GCD     Client UID: '%s'\n", ccb_id, uid );
	    break;
	
	case MSG_CP_CLIENT_HOST :
	    if ( (status = read_string( ccb, cp.val_len, &host )) != OK )
		break;

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay("%4d    GCD     Client Host: '%s'\n", ccb_id, host);
	    break;
	
	case MSG_CP_CLIENT_ADDR :
	    if ( (status = read_string( ccb, cp.val_len, &addr )) != OK )
		break;

	    if ( GCD_global.gcd_trace_level >= 3 )
		TRdisplay("%4d    GCD     Client Addr: '%s'\n", ccb_id, addr);
	    break;
	
	case MSG_CP_LOGIN_TYPE :
	{
	    /*
	    ** Login local/remote is handled as an API connection
	    ** parameter.  Login user is noted for later processing.
	    */
	    u_i1 val;

	    if ( cp.val_len != 1  ||  ! gcd_get_i1( ccb, (u_i1 *)&val ) )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	    
	    switch( val )
	    {
	    case MSG_CPV_LOGIN_LOCAL :
		/*
		** Use val_len to pass TRUE/FALSE for login local/remote.
		*/
		cp.val_len = TRUE;
		status = read_parms( ccb, &cp, parms, &count );
		break;
		     
	    case MSG_CPV_LOGIN_REMOTE : 
		/*
		** Use val_len to pass TRUE/FALSE for login local/remote.
		*/
		cp.val_len = FALSE;
		status = read_parms( ccb, &cp, parms, &count );
		break; 

	    case MSG_CPV_LOGIN_USER : 
		if ( GCD_global.gcd_trace_level >= 3 )
		    TRdisplay( "%4d    GCD     login: user\n", ccb_id ); 
		login_user = TRUE;
		break; 

	    default :
		if ( GCD_global.gcd_trace_level >= 1 )
		    TRdisplay( "%4d    GCD     unknown login type: %d\n",
			       ccb_id, val );
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break; 
	    }

	    break;
	}

	case MSG_CP_DBUSERNAME :
	case MSG_CP_DBPASSWORD :
	case MSG_CP_GROUP_ID :
	case MSG_CP_ROLE_ID :
	case MSG_CP_TIMEZONE :
	case MSG_CP_DECIMAL :
	case MSG_CP_DATE_FRMT :
	case MSG_CP_MNY_FRMT :
	case MSG_CP_MNY_PREC :
	case MSG_CP_DATE_ALIAS :
	    /*
	    ** These are all API connection parameters.
	    */
	    status = read_parms( ccb, &cp, parms, &count );
	    break;

	default :
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     unknown parameter ID: %d\n",
			   ccb_id, cp.param_id );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    status = FAIL;
	    break;
	}

	if ( status != OK )  break;
    }

    for( ; status == OK; )
    {
	/*
	** Username and password are required for a standard
	** connection, and transaction IDs are not permitted.
	*/
	if ( ! ccb->msg.dtmc )
	{
	    if ( ! usr  ||  ! pwd )
	    {
		status = E_GC480C_NO_AUTH;
		break;
	    }
	    else 
	    {
	        gcd_decode( usr, (ccb->msg.mask_len ? ccb->msg.mask : NULL),
			    (u_i1 *)pwd, pwd_len, pwd);

	        if ( login_user )
	            if ( STcompare( usr, pwd ) == 0  &&  ccb->isLocal )
			pwd = NULL;
	            else
	            {
			status = E_GC480C_NO_AUTH;
			break;
		    }    
	    }

	    if ( xact_2pc )
	    {
		gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
		status = FAIL;
		break;
	    }
	}

	/*
	** Make sure a valid distributed transaction ID
	** (Ingres or XA) has been received.
	*/
	if ( xact_2pc  &&  xact_2pc != CP_II_XID  &&  xact_2pc != CP_XA_XID )
	{
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    status = FAIL;
	    break;
	}

	if ( ! (cib = gcd_new_cib( count )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    status = FAIL;
	    break;
	}

	/*
	** Save CIB information.
	*/
	ccb->cib = cib;
	if ( db )  cib->database = db;
	if ( usr )  cib->username = usr;
	if ( pwd )  cib->password = pwd;
	if ( uid )  ccb->client_user = uid;
	if ( host )  ccb->client_host = host;
	if ( addr )  ccb->client_addr = addr;
	cib->isLocal = ccb->isLocal;
	cib->autocommit = auto_on;

	if ( ccb->msg.dtmc )
	{
	    /*
	    ** Distributed transaction management connections
	    ** require a new connection, so force pooling off.
	    */
	    cib->pool_on = FALSE;
	    cib->pool_off = TRUE;
	}
	else
	{
	    /*
	    ** Pooling may be controlled by the client.
	    */
	    cib->pool_on = pool_on;
	    cib->pool_off = pool_off;
	}

	for( cib->parm_cnt = 0; cib->parm_cnt < count; cib->parm_cnt++ )
	     STRUCT_ASSIGN_MACRO( parms[ cib->parm_cnt ],
	 		         cib->parms[ cib->parm_cnt ] );
	/*
	** Since saved in CIB, don't free if error occurs.
	*/
	db = usr = pwd = uid = host = addr = NULL;
	count = 0;
	cib->api_ver = gcd_msg_version(ccb->msg.proto_lvl); 

	/*
	** See if there is a connection in the pool
	** which matches the request.  If so, use the
	** pool CIB and free the CIB allocated above.
	*/
	if ( (pool = gcd_pool_find( cib )) )
	{
	    /*
	    ** Pool matching ensures a compatible transaction
	    ** state, but not necessarily the requested state.
	    ** If a transaction is active, then it will be of 
	    ** the correct type.  If no transaction, then we 
	    ** must be sure to request the correct initial 
	    ** state for autocommit.
	    */
	    if ( ! pool->tran )  pool->autocommit = cib->autocommit;
	    ccb->cib = pool;
	    gcd_del_cib( cib );

	    /*
	    ** use ccb->cib->api_ver to set ccb->api_env. In case
	    ** of failure to initialize environment handle for
	    ** the pooled connection set the status to protocol error.
	    */
	    if( gcd_get_env(ccb->cib->api_ver, &ccb->api.env) != OK )
	    {
	        status = E_GC480A_PROTOCOL_ERROR;
	        break;
	    }

	    ccb->sequence = CONN_DONE;
	    msg_connect_sm( (PTR)pcb );
	    return;
	}

	/*
	** No connection in the pool.  Create a new connection.
	*/
	pcb->data.conn.database = cib->database;
	pcb->data.conn.username = cib->username;
	pcb->data.conn.password = cib->password;
	pcb->data.conn.param_cnt = cib->parm_cnt;
	pcb->data.conn.parms = cib->parms;

	switch( xact_2pc )
	{
	case CP_II_XID :
	    pcb->data.conn.distXID.ti_type = IIAPI_TI_IIXID;
	    STcopy( GCD_XID_NAME,
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
		GCD_XID_SEQNUM;
	    pcb->data.conn.distXID.ti_value.xaXID.xa_branchFlag = 
		GCD_XID_FLAGS;

	    MEcopy( (PTR)&xid, sizeof( xid ),
		    (PTR)&pcb->data.conn.distXID.ti_value.xaXID.xa_tranID );
	    ccb->sequence = CONN_XID;
	    break;
	    
	default :
	    ccb->sequence = CONN_PARMS;
	    break;
	}

	if( gcd_get_env(cib->api_ver, &ccb->api.env ) != OK )
	{
	    status = E_GC480A_PROTOCOL_ERROR;
	    break;
	}

	ccb->api.conn = ccb->api.env;
	msg_connect_sm( (PTR)pcb );
	return;
    }

    if ( status != FAIL )
    {
	/*
	** Connection rejected (rather than aborted).
	** Send connection request status to client.
	*/
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD Error processing connection request: 0x%x\n",
		       ccb_id, status );

	gcd_sess_error( ccb, &pcb->rcb, status );
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_sess_term( ccb );
    }

    /*
    ** Free resources allocated in this routine.
    */
    if ( pcb )  gcd_del_pcb( pcb );
    if ( db )   MEfree( (PTR)db );
    if ( usr )  MEfree( (PTR)usr );
    if ( pwd )  MEfree( (PTR)pwd );
    if ( uid )  MEfree( (PTR)uid );
    if ( host ) MEfree( (PTR)host );
    if ( addr ) MEfree( (PTR)addr );
    for( i = 0; i < count; i++ )  MEfree( (PTR)parms[i].value );

    return;
}

static void
msg_connect_sm( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Connect: %s\n",
		   ccb->id, gcu_lookup( connSeqMap, ccb->sequence ) );

    switch( ccb->sequence++ )
    {
    case CONN_XID :
	if ( ! gcd_api_regXID( ccb, &pcb->data.conn.distXID ) )
	{
	    gcd_sess_error( ccb, &pcb->rcb, E_GC4816_XACT_REG_XID );
	    ccb->sequence = CONN_ABORT;
	}
	break;

    case CONN_PARMS :			/* Connection parameters */
	if ( pcb->api_error )
	{
	    ccb->sequence = CONN_ABORT;
	    break;
	}

	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD connect param count: %d\n",
		       ccb->id, pcb->data.conn.param_cnt );

	if ( pcb->data.conn.param_cnt )
	{
	    ccb->sequence = CONN_PARMS;
	    gcd_push_callback( pcb, msg_connect_sm );
	    gcd_api_setConnParms( pcb );
	    return;
	}
	break;

    case CONN_CONN :			/* Connect */
	gcd_push_callback( pcb, msg_connect_sm );
	gcd_api_connect( pcb ); 
	return;

    case CONN_DONE :			/* Done */
	if ( pcb->api_error )
	{
	    ccb->sequence = CONN_ABORT;
	    break;
	}

	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_msg_pause( ccb, TRUE );
	gcd_del_pcb( pcb );
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
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_msg_pause( ccb, TRUE );
	if ( ! pcb->data.disc.conn )  break;
	gcd_push_callback( pcb, msg_connect_sm );
	gcd_api_abort( pcb ); 
	return;
    
    case CONN_ERROR :			/* Clean-up */
	gcd_del_pcb( pcb );
	gcd_sess_term( ccb );
	return;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid connect sequence: %d\n",
		       ccb->id, --ccb->sequence );
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	return;
    }

    goto top;
}


/*
** Name: read_parms
**
** Description:
**	Processes the following MSG layer connection parameters
**	and populates a CONN_PARM array with the corresponding
**	API connection parameters.
**
**	    MSG_CP_DBUSERNAME
**	    MSG_CP_DBPASSWORD
**	    MSG_CP_GROUP_ID
**	    MSG_CP_ROLE_ID
**	    MSG_CP_TIMEZONE
**	    MSG_CP_DECIMAL
**	    MSG_CP_DATE_FRMT
**	    MSG_CP_MNY_FRMT
**	    MSG_CP_MNY_PREC
**	    MSG_CP_DATE_ALIAS
**	    MSG_CP_LOGIN_TYPE
**
**	In the case of MSG_CP_LOGIN_TYPE, the parameter value
**	should already have been read.  This routine should only
**	be called if the parameter value is MSG_CPV_LOGIN_LOCAL
**	or MSG_CPV_LOGIN_REMOTE.  Instead of passing the param
**	length (in cp), pass TRUE for local and FALSE for remote.
**
**	A critical error will result in a session abort and a
**	status of FAIL will be returned.  Other errors will
**	simply return the appropriate error status.
**
** Input:
**	ccb	Connection Control Block.
**	cp	Connection Parameter info: id & length.
**	parms	Array of API connection parameters.
**	count	Current number of API connection parameters.
**
** Output:
**	parms	New API connection parameter.
**	count	Updated number of API connection parameters.
**
** Returns:
**	STATUS	OK, FAIL or error code.
**
** History:
**	 7-Jul-03 (gordy)
**	    Created.
**	18-Oct-2007 (kibro01) b119318
**	    Use a common function in adu to determine valid date formats
**	23-Jul-07 (gordy)
**	    Added date alias parameter.
*/

static STATUS
read_parms( GCD_CCB *ccb, DAM_ML_PM *cp, CONN_PARM *parms, u_i2 *count )
{
    char	*name, *display, str_val[33];
    u_i1	*val, i1_val;
    i4		id;
    i4		date_fmt;

    if ( *count >= GCD_API_CONN_PARM_MAX )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD     too many connection parameters: %d\n",
		       ccb->id, *count + 1 );
	gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	return( FAIL );
    }

    /*
    ** Determine the API parameter id and value
    ** along with the tracing name and value.
    */
    switch( cp->param_id )
    {
    case MSG_CP_LOGIN_TYPE :
	/*
	** The parameter value should already have been read
	** and cp->val_len used as a boolean to indicate login
	** local/remote (TRUE/FALSE).
	*/
	if ( ! (val = (u_i1 *)MEreqmem( 0, sizeof(II_BOOL), TRUE, NULL )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return( FAIL );
	}

	*(II_BOOL *)val = (II_BOOL)cp->val_len; 
	id = IIAPI_CP_LOGIN_LOCAL;
	name = "login";
	display = (cp->val_len ? "local" : "remote"); 
	break; 

    case MSG_CP_DATE_FRMT :
	/*
	** Must convert string format name into API numeric value.
	*/
	if ( ! (val = (u_i1 *)MEreqmem( 0, sizeof(II_LONG), TRUE, NULL )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return( FAIL );
	}
	else  if ( cp->val_len >= sizeof( str_val )  ||
	           ! gcd_get_bytes( ccb, cp->val_len, (u_i1 *)str_val ) )
	{
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return( FAIL );
	}

	str_val[ cp->val_len ] = EOS;
	CVlower( str_val );

	/* Use common function to determine date formats (kibro01) b119318 */
	date_fmt = adu_date_format(str_val);
	if (date_fmt == -1)
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     invalid date format: '%s'\n",
			   ccb->id, str_val );
	    return( E_GC4817_CONN_PARM );
	}
	*(II_LONG *)val = (II_LONG)date_fmt;

	id = IIAPI_CP_DATE_FORMAT;
	name = "Date Format";
	display = str_val;
	break;

    case MSG_CP_MNY_FRMT :
	/*
	** Money format actually produces two API parameters.
	** First, extract the format string.
	*/
	if ( cp->val_len >= sizeof( str_val )  ||
	     ! gcd_get_bytes( ccb, cp->val_len, (u_i1 *)str_val ) )
	{
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return( FAIL );
	}

	str_val[ cp->val_len ] = EOS;

	/*
	** Now, determine leading/trailing sign and create
	** the API parameter.
	*/
	if ( ! (val = (u_i1 *)MEreqmem( 0, sizeof(II_LONG), TRUE, NULL )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return( FAIL );
	}

	if ( str_val[1] != ':' )
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     invalid money format: '%s'\n",
			   ccb->id, str_val );
	    return( E_GC4817_CONN_PARM );
	}
	else  if ( str_val[0] == 'L'  ||  str_val[0] == 'l' )
	    *(II_LONG *)val = (II_LONG)IIAPI_CPV_MONEY_LEAD_SIGN;
	else  if ( str_val[0] == 'T'  ||  str_val[0] == 't' )
	    *(II_LONG *)val = (II_LONG)IIAPI_CPV_MONEY_TRAIL_SIGN;
	else
	{
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     invalid money format: '%s'\n",
			   ccb->id, str_val );
	    return( E_GC4817_CONN_PARM );
	}

	parms[*count].id = IIAPI_CP_MONEY_LORT;
	parms[*count].value = (PTR)val;
	(*count)++;

	/*
	** Finally, save the money symbol.
	*/
	if ( ! (val = (u_i1 *)MEreqmem( 0, cp->val_len - 1, TRUE, NULL )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return( FAIL );
	}

	STcopy( &str_val[2], (char *)val );
	id = IIAPI_CP_MONEY_SIGN;
	name = "Money Format";
	display = str_val;
	break;

    case MSG_CP_MNY_PREC :
	/*
	** Numeric value
	*/
	if ( ! (val = (u_i1 *)MEreqmem( 0, sizeof(II_LONG), TRUE, NULL )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return( FAIL );
	}
	else  if ( cp->val_len != 1  ||  ! gcd_get_i1( ccb, &i1_val ) )
	{
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return( FAIL );
	}

	*(II_LONG *)val = (II_LONG)i1_val;
	CVla( (i4)i1_val, str_val );

	id = IIAPI_CP_MONEY_PRECISION;
	name = "Money Precision";
	display = str_val;
	break;

    default :
	/*
	** The remaining parameters all have a string value
	** which is passed directly as the API parameter value.
	*/
	if ( ! (val = (u_i1 *)MEreqmem( 0, cp->val_len + 1, TRUE, NULL )) )
	{
	    gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	    return( FAIL );
	}
	else  if ( ! gcd_get_bytes(ccb, cp->val_len, val ) )
	{
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return( FAIL );
	}

	display = (char *)val;

	switch( cp->param_id )
	{
	case MSG_CP_DBUSERNAME :
	    id = IIAPI_CP_EFFECTIVE_USER;
	    name = "DBMS username";
	    break;

	case MSG_CP_DBPASSWORD :
	    id = IIAPI_CP_DBMS_PASSWORD;
	    name = "DB Password";
	    display = "*****";
	    break;

	case MSG_CP_GROUP_ID :
	    id = IIAPI_CP_GROUP_ID;
	    name = "Group ID";
	    break;

	case MSG_CP_ROLE_ID :
	    id = IIAPI_CP_APP_ID;
	    name = "Role ID";
	    break;

	case MSG_CP_TIMEZONE :
	    id = IIAPI_CP_TIMEZONE;
	    name = "Timezone";
	    break;

	case MSG_CP_DECIMAL :
	    id = IIAPI_CP_DECIMAL_CHAR;
	    name = "Decimal";
	    break;

	case MSG_CP_DATE_ALIAS :
	    id = IIAPI_CP_DATE_ALIAS;
	    name = "Date Alias";
	    break;

	default :				/* Shouldn't happen! */
	    if ( GCD_global.gcd_trace_level >= 1 )
		TRdisplay( "%4d    GCD     unknown parameter ID: %d\n",
			   ccb->id, cp->param_id );
	    gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	    return( FAIL );
	}
	break;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD     %s: '%s'\n", ccb->id, name, display );

    parms[*count].id = id;
    parms[*count].value = (PTR)val;
    (*count)++;
    return( OK );
}


/*
** Name: read_string
**
** Description:
**	Allocates memory for a string parameter value and reads
**	the value from the current message.  Session is aborted
**	if any error occurs and FAIL is returned.
**
** Input:
**	ccb	Connection control block.
**	len	String length.
**
** Output:
**	value	Parameter value.
**
** Returns:
**	STATUS	OK or FAIL.
**
** History:
**	15-Jul-03 (gordy)
**	    Created.
*/

static STATUS
read_string( GCD_CCB *ccb, u_i2 len, char **value )
{
    if ( ! (*value = (char *)MEreqmem( 0, len + 1, TRUE, NULL )) )
    {
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
	return( FAIL );
    }
    
    if ( ! gcd_get_bytes( ccb, len, (u_i1 *)*value ) )
    {
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return( FAIL );
    }

    return( OK );
}


/*
** Name: gcd_msg_disconnect
**
** Description:
**	Process a GCD disconnect message.
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
**	11-Oct-05 (gordy)
**	    Check for disconnect error.
**	29-Jun-06 (gordy)
**	    RCB no longer allocated along with PCB.  Allocate explicitly
**	    to ensure error messages returned to client.
*/

void
gcd_msg_disconnect( GCD_CCB *ccb )
{
    GCD_RCB	*rcb;
    GCD_PCB	*pcb;

    if ( ccb->msg.msg_len )
    {
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD unexpected data with disconnect: %d\n", 
			ccb->id, ccb->msg.msg_len );
	gcd_sess_abort( ccb, E_GC480A_PROTOCOL_ERROR );
	return;
    }

    /*
    ** Check to see if the DBMS connection 
    ** shutdown has already been initiated.
    */
    if ( ! ccb->cib  ||  ! ccb->cib->conn )
    {
	if ( GCD_global.gcd_trace_level >= 3 )
	    TRdisplay( "%4d    GCD Disconnect (completed)\n", ccb->id );
	gcd_send_done( ccb, NULL, NULL );
	return;
    }

    if ( GCD_global.gcd_trace_level >= 3 )
	TRdisplay( "%4d    GCD Disconnect request\n", ccb->id );

    if ( ! (pcb = gcd_new_pcb( ccb ))  ||
         ! (pcb->rcb = gcd_new_rcb( ccb, -1 )) )
    {
	/*
	** We can't disconnect or abort the connection
	** if we can't allocate memory.  The memory
	** allocation error has been logged, so just
	** try and clean-up the CCB.
	*/
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD can't alloc pcb to disconnect/abort\n",
			ccb->id );
	if ( pcb )  gcd_del_pcb( pcb );
	ccb->cib->conn = ccb->cib->tran = NULL;
	gcd_sess_abort( ccb, E_GC4808_NO_MEMORY );
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
    gcd_push_callback( pcb, msg_disc_sm );
    gcd_purge_stmt( pcb );
    return;
}

static void
msg_disc_sm( PTR arg )
{
    GCD_PCB	*pcb = (GCD_PCB *)arg;
    GCD_CCB	*ccb = pcb->ccb;
    GCD_CIB	*cib;

  top:

    if ( GCD_global.gcd_trace_level >= 4 )
	TRdisplay( "%4d    GCD Disconnect: %s\n",
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

	gcd_push_callback( pcb, msg_disc_sm );
	gcd_api_commit( pcb );
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

	if ( gcd_pool_save( cib ) )
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
		gcd_push_callback( pcb, msg_disc_sm );
		gcd_api_autocommit( pcb, FALSE );
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

	gcd_push_callback( pcb, msg_disc_sm );
	gcd_api_disconnect( pcb );
	return;
    
    case DISC_CHK :			/* Check disconnect status */
	if ( pcb->api_error )
	    ccb->sequence = DISC_ABORT;
	else
	    ccb->sequence = DISC_DONE;
    	break;

    case DISC_ABORT :			/* Abort connection */
	ccb->sequence = DISC_DONE;
	ccb->cib->tran = NULL;
	gcd_push_callback( pcb, msg_disc_sm );
	gcd_api_abort( pcb );
	return;
    
    case DISC_DONE :			/* Free connection resources */
	gcd_send_done( ccb, &pcb->rcb, pcb );
	gcd_del_pcb( pcb );
	gcd_sess_term( ccb );
	return;

    default :
	if ( GCD_global.gcd_trace_level >= 1 )
	    TRdisplay( "%4d    GCD invalid disconnect sequence: %d\n",
		       ccb->id, --ccb->sequence );
	ccb->cib->conn = ccb->cib->tran = NULL;
	gcd_del_pcb( pcb );
	gcd_sess_abort( ccb, E_GC4809_INTERNAL_ERROR );
	return;
    }

    goto top;
}

