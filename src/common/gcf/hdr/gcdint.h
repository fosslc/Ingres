/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/* Name: gcdint.h
**
** Description:
**	Global definitions for the Data Access Server (IIGCD).
**
** History:
**	 3-Jun-99 (gordy)
**	    Created.
**	10-Sep-99 (gordy)
**	    Parameterized initial TL connection info.
**	14-Sep-99 (gordy)
**	    Implemented JDBC error messages.
**	22-Sep-99 (gordy)
**	    Added charset to global info.
**	29-Sep-99 (gordy)
**	    Implemented support for BLOB data.
**	26-Oct-99 (gordy)
**	    Added cursor handle to CCB to support positioned DELETE/UPDATE.
**	 2-Nov-99 (gordy)
**	    User ID and password are required.
**	 4-Nov-99 (gordy)
**	    Added jdbc_sess_interrupt().
**	13-Dec-99 (gordy)
**	    Replace GCC private buffer length with global values for
**	    buffer, message and message data lengths.  Added info to
**	    Query Data for handling cursor pre-fetching.
**	21-Dec-99 (gordy)
**	    Added info to track/check for clients which have been idle
**	    for an excessive perioud (configurable).
**	17-Jan-00 (gordy)
**	    Adding data structures to increase performance of connect
**	    requests, including connection pooling and query cacheing.
**	 1-Mar-00 (gordy)
**	    Transaction info moved to CIB so that autocommit transactions
**	    (JDBC default) may be carried throught the connection pool.
**	 3-Mar-00 (gordy)
**	    Add free queue for CIB and related counters.
**	31-mar-2000 (somsa01)
**	    Added E_JD0113_LOAD_CONFIG.
**	19-May-00 (gordy)
**	    Added a PCB pointer to the SCB to support nested requests.
**	25-Aug-00 (gordy)
**	    Added system constants to saved connection info.  Added
**	    flags and parameter data to clean up query processing.
**	11-Oct-00 (gordy)
**	    Removed PCB pointer from SCB.  Nested requests are now
**	    handled through a callback stack in the PCB.
**	12-Oct-00 (gordy)
**	    Added sub-structures to CCB for transaction and statement
**	    info.  Added autocommit mode indication and transaction
**	    query operation defines for current (default) and future
**	    handling of autocommit.
**	17-Oct-00 (gordy)
**	    Added autocommit mode single (also added define for mode
**	    multi which isn't yet implemented).
**	19-Oct-00 (gordy)
**	    Implemented multi-cursor autocommit mode.  Added flag to
**	    signal when autocommit has been temporarily disabled and
**	    a normal transaction is being used to simulate autocommit.
**	    The autocommit transaction handle flag must be set FALSE
**	    during this mode, so an additional flag is needed to know
**	    that we want to return to a regular autocommit transaction.
**	24-Oct-00 (gordy)
**	    Added gateway flag to CIB so that server operations can be
**	    adjusted for non-ingres DBMS.
**	21-Mar-01 (gordy)
**	    Added support for distributed transactions: error messages
**	    and OpenAPI transaction ID handle.
**	18-Apr-01 (gordy)
**	    Added support for distributed transaction management connections.
**	    Added dtmc flag and request result queue in CCB.  Moved some
**	    transaction constants here from jdbcxact.
**	10-May-01 (gordy)
**	    Add storage for info request ID and value in CCB.
**	01-Nov-2002 (wansh01)
**	    Add local connection indicator in the CCB. 
**	01-Dec-2002 (wansh01)
**	    Add local connection indicator in CIB. 
**	18-Dec-02 (gordy)
**	    Converted for Data Access Server (GCD).
**	20-Dec-02 (gordy)
**	    Added charset_id to globals.  Added msg_hdr_id to CCB.
**	15-Jan-03 (gordy)
**	    Abstracted interfaces between the GCC protocol driver
**	    state machine, Transport Layer packet processing and
**	    Message Layer by adding Service Provider interfaces.
**	14-Feb-03 (gordy)
**	    Added support for savepoints.
**	 6-Jun-03 (gordy)
**	    Added GCD MIB information.  Added protocol information block
**	    (PIB) for listen addresses.
**	 7-Feb-03 (gordy)
**	    Added symbol for max API connection parameters.
**	15-Jul-03 (gordy)
**	    Added E_GC4817_CONN_PARM.
**	 4-Aug-03 (gordy)
**	    Added query execution flags to CCB and statement type flags
**	    to SCB.
**	20-Aug-03 (gordy)
**	    Added statement auto close flags.
**	 1-Oct-03 (gordy)
**	    Added flags for fetching rows with initial query request.
**	 6-Jun-04 (wansh01)
**	    Added gcd admin error messages.
**	16-Jun-04 (gordy)
**	    Added ML Session Mask to CCB.
**	13-Aug-04 (gordy)
**	    Allow for messages exceeding 64K in total length.
**	24-Aug-04 (wansh01)
**	    Added support for SHOW command.
**	24-Sept-04 (wansh01)
**	    Added Global variable gcd_lcl_id ito support SHOW SERVER command.
**	    Added gcd_adm_init() gcd_adm_term() prototype.  
**	20-Apr-05 (wansh01)
**	    Changed GCD_SESS_INFO to GCD_POOL_INFO. 
**	10-May-05 (gordy)
**	    Added error messages E_GC4818_XACT_SAVEPOINT and
**	    E_GC4819_XACT_ABORT_STATE.
**	11-Oct-05 (gordy)
**	    Added flag for API logical sequence error.
**	14-Jul-06 (gordy)
**	    Added E_GC481A_XACT_END_STATE, GCD_XACT_RESET.
**	17-Aug-06 (rajus01)
**	    Added GCD_SESS_STATS and other related variables.
**	31-Aug-06 (gordy)
**	    Generalized and extended request data for parameter info.
**	 6-Nov-06 (gordy)
**	    Allow query text to be longer than 64K.
**	23-Jul-07 (gordy)
**	    Added connection parameter for date alias.
**	 5-Dec-07 (gordy)
**	    Added accept() entry point to GCC_SERVICE to provide a
**	    mechanism allowing the GCC layer to reject a connection
**	    and provide an error code to be returned to the client.
**	23-Mar-09 (gordy)
**	    Support long database object names.
**	21-Jul-09 (gordy)
**	    Remove restrictions on length of user and cursor names.
**	25-Aug-2009 (kschendel) 121804
**	    Add gcd-build-qdata prototype.
**	25-Mar-10 (gordy)
**	    Add support for batch processing.
**	 31-Mar-10 (gordy & rajus01) SD issue 142688, Bug 123270
**	    Add gcd_gca_activate().
**	13-May-10 (gordy)
**	    Added free RCB queues.
**       15-Nov-2010 (stial01) SIR 124685 Prototype Cleanup
**          Changes to eliminate compiler prototype warnings.
*/

#ifndef _GCD_INCLUDED_
#define _GCD_INCLUDED_

#include <gcccl.h>
#include <qu.h>
#include <tm.h>
#include <iiapi.h>

#define	GCD_SRV_CLASS		"DASVR"
#define	GCD_GCA_PROTO_LVL	GCA_PROTOCOL_LEVEL_62
#define	GCD_LOG_ID		"IIGCD"
#define GCD_L_LCL_ID        	18      /* Length of the local server ID */

#define	GCD_NAME_MAX		32
#define	GCD_MSG_BUFF_LEN	1024

#define	ARR_SIZE( arr )		(sizeof(arr)/sizeof((arr)[0]))
#define	QUEUE_EMPTY( q )	(((QUEUE *)(q))->q_next == ((QUEUE *)(q)))

/*
** The maxinum number of API connection parameters 
** which may be associated with a connection.
*/

#define	GCD_API_CONN_PARM_MAX	12

/*
** Transaction query operations
** (for gcd_xact_check()).
*/

#define	GCD_XQOP_NON_CURSOR	0
#define	GCD_XQOP_CURSOR_OPEN	1
#define	GCD_XQOP_CURSOR_UPDATE	2
#define	GCD_XQOP_CURSOR_CLOSE	3

#define MSG_S_IDLE      0
#define MSG_S_READY     1
#define MSG_S_PROC      2
#define	MSG_S_BATCH	3

#define MSG_S_CNT       4

/*
** Forward structure references
*/

typedef	struct GCD_GCCTL_SERVICE	GCC_SRVC;	/* GCC Service */
typedef	struct GCD_TLGCC_SERVICE	TL_SRVC;	/* TL Service for GCC */
typedef	struct GCD_TLMSG_SERVICE	TL_MSG_SRVC;	/* TL Service for MSG */
typedef struct GCD_MSGTL_SERVICE	MSG_SRVC;	/* MSG Service */

/*
** Name: GCD_GLOBAL
**
** Description:
**	GCD Server global information.
**
** History:
**	26-Dec-02 (gordy)
**	    Added charset_id.
**	15-Jan-03 (gordy)
**	    Added TL services and TL header size.
**	 6-Jun-03 (gordy)
**	    Added MIB information: PIB queue, hostname, connection count.
**	 7-Jul-03 (gordy)
**	    CIB free queue size expanded.
**	20-Nov-07 (rajus01) Bug 119505, SD Issue: 122906
**	    Removed API environment handle from GCD_GLOBAL. Added gcd_get_env()
**	    gcd_rel_env().
**	 5-Dec-07 (gordy)
**	    Changed connections to client_max and client_cnt to enforce 
**	    connection limit at GCC level.
**	13-May-10 (gordy)
**	    Add free RCB queue for zero length buffers.
*/

typedef struct
{

    char	host[ GCC_L_NODE + 1 ];
    char	*charset;
    u_i4	charset_id;
    i4		language;
    i4		gcd_trace_level;

    u_i4	client_max;
    u_i4	client_cnt;
    i4		client_idle_limit;
    SYSTIME	client_check;

    QUEUE	pib_q;
    QUEUE	rcb_q;

    u_i4	ccb_total;
    u_i4	ccb_active;
    QUEUE	ccb_free;
    QUEUE	ccb_q;

    u_i4	cib_total;
    u_i4	cib_active;
    /* Queue for each param count value */
    QUEUE	cib_free[ GCD_API_CONN_PARM_MAX + 1 ];

    bool	client_pooling;
    i4		pool_max;
    i4		pool_cnt;
    i4		pool_idle_limit;
    SYSTIME	pool_check;
    
    u_i4	nl_hdr_sz;		/* Max NL header size */
    u_i4	tl_hdr_sz;		/* Max TL header size */

    u_i4	tl_srvc_cnt;		/* Number of TL services */
    TL_SRVC	**tl_services;		/* Array of TL service */
    u_i4	flags;			/* flags  */

#define GCD_SVR_SHUT    0x0001          /* mionitor: server shutdown  */
#define GCD_SVR_QUIESCE 0x0002          /* mionitor: server quiesce  */
#define GCD_SVR_CLOSED  0x0004          /* mionitor: server closed  */

    u_i4	adm_sess_count;		/* Number of adm sessesion  */
    char        gcd_lcl_id[GCD_L_LCL_ID];

} GCD_GLOBAL;

GLOBALREF GCD_GLOBAL	GCD_global;

/*
** Name: GCD_PIB
**
** Description:
**	Control block representing a network protocol.
*/

typedef struct _gcd_pib GCD_PIB;

struct _gcd_pib
{
    QUEUE	q;
    u_i4	flags;				/* PCT status flags */
    char	pid[ GCC_L_PROTOCOL + 1 ];	/* Protocol identifier */
    char	host[ GCC_L_NODE + 1 ];		/* Host identifier */
    char	addr[ GCC_L_PORT + 1 ];		/* Listen address */
    char	port[ GCC_L_PORT + 1 ];		/* Actual listen port */
};

/*
** Name: GCD_SPCB
**
** Description:
**	Control block representing a transaction savepoint.
**	This is a variable length structure.  Allocate as
**	much additional space as necessary to hold the name.
*/

typedef struct _gcd_spcb_ GCD_SPCB;

struct _gcd_spcb_
{
    GCD_SPCB	*next;
    PTR		hndl;
    char	name[1];
};

/*
** Name: QDATA
**
** Description:
**	Query data descriptors and values.  This structure is
**	used for both input parameters and output columns.
**	Data items may be processed in groups and counters
**	are provided to track the processing progression.
**
**	Maintains arrays of API descriptors and data values.
**	Since column descriptors are owned by the API, there
**	is a separate descriptor pointer which is either the
**	API descriptor or the QDATA parameter descriptor.
**
**	A single data buffer holds all non-BLOB data and is
**	referrenced by the data value array.  BLOB data is
**	processed a segment at a time, so there is a single
**	BLOB segment buffer sized to hold the largest BLOB
**	segment.
**
** History:
**	18-Aug-04 (gordy)
**	    Make the length fields i4's to avoid overflow.
*/

typedef struct
{

    u_i2	max_rows;	/* Number of rows buffer can hold */
    u_i2	max_cols;	/* Number of data items */
    u_i2	col_cnt;	/* Number of available items */
    u_i2	cur_col;	/* Current item */
    bool	more_segments;	/* BLOB processing */
    PTR		desc;		/* Data descriptors */

    u_i2	desc_max;	/* Number of descriptors */
    PTR		param_desc;	/* IIAPI_DESCRIPTOR data descriptors */
    u_i2	data_max;	/* Number of data values */
    PTR		data;		/* IIAPI_DATAVALUE data values */
    u_i4	db_max;		/* Length of data buffer */
    PTR		data_buff;	/* Data value buffer */
    u_i4	bb_max;		/* Length of BLOB buffer */
    PTR		blob_buff;	/* BLOB segment buffer */

} QDATA;

/*
** Name: GCD_CIB
**
** Description:
**	Connection Information Block.  Contains the connection
**	data which uniquely identifies a given DBMS connection.
**
** History:
**	11-Oct-05 (gordy)
**	    Added flag for API logical sequence error.
*/

typedef struct
{
    i4	id;
    PTR	value;
} CONN_PARM;

typedef struct
{
    char *name;
    char *value;
} CAPS_INFO;

typedef struct
{
    QUEUE	q;
    PTR		data;		/* Buffer for caps info */
    u_i2	cap_cnt;
    CAPS_INFO	caps[1];	/* Variable length */

} GCD_CAPS;

typedef struct
{
    QUEUE	q;
    u_i4	id;
    i4		level;		/* API level */
    PTR		conn;		/* API connection handle */
    u_i2	flags;		/* Status flags */

#define	GCD_CONN_ABORT		0x01
#define	GCD_XACT_ABORT		0x02
#define	GCD_LOGIC_ERR		0x04
#define	GCD_XACT_RESET		0x08
#define	GCD_IS_INGRES		0x10
#define	GCD_CASE_LOWER		0x20
#define	GCD_CASE_UPPER		0x40
#define	GCD_CASE_MIXED		(GCD_CASE_LOWER | GCD_CASE_UPPER)

#define	GCD_IS_LOWER( flags )	(((flags) & GCD_CASE_MIXED) == GCD_CASE_LOWER)
#define	GCD_IS_UPPER( flags )	(((flags) & GCD_CASE_MIXED) == GCD_CASE_UPPER)

    /*
    ** The following members are used to determine
    ** the transaction state as follows:
    **
    **	autocommit	 tran
    **	----------	------
    **	   FALSE	  NULL	No transaction.
    **	   FALSE	! NULL	Multi-statement transaction.
    **	   TRUE		! NULL	Autocommit transaction.
    **	   TRUE		  NULL	Autocommit requested but not active.
    */
    bool	autocommit;
    PTR		tran;

    bool	pool_off;	/* Connection pool disabled */
    bool	pool_on;	/* Connection pool requested */
    bool	isLocal;	/* Connection local/remote indicator */
    SYSTIME	expire;		/* Connection pool expiration limit */

    char	*database;
    char	*username;
    char	*password;
    u_i2	api_ver;	/* API version for pooled connection */

    union
    {
	IIAPI_AUTOPARM		ac;
	IIAPI_DISCONNPARM	disc;
	IIAPI_ABORTPARM		abrt;
    } api;

    /*
    ** Request info.
    */
    char	*usr;
    char	*dba;
    char	*sys;

    QUEUE	caps;

    u_i2	parm_max;
    u_i2	parm_cnt;
    CONN_PARM	parms[1];	/* Variable length */

} GCD_CIB;

/*
** Name: GCD_CCB - Connection control block.
**
** Description:
**	Information describing a client connection.
**	Various sub-structures provide storage for
**	the processing layers in the GCD server.
**
** History:
**	20-Dec-02 (gordy)
**	    Added msg_hdr_id as this is now protocol level depedent.
**	15-Jan-03 (gordy)
**	    Don't need max_mesg_len when global tl_hdr_sz is available.
**	    Removed xoff_rcb, XOFF/XON handled via state machine.  Moved
**	    conn_info members to more relevant structures.  Added tl
**	    structure and moved proto_lvl from GCC.  Added services.
**	14-Feb-03 (gordy)
**	    Added savepoints.
**	 6-Jun-03 (gordy)
**	    Added MIB information: local and remote addresses.
**	 4-Aug-03 (gordy)
**	    Removed max_data_len.  Added query execution flags.
**	20-Aug-03 (gordy)
**	    Added auto close query flag.
**	 1-Oct-03 (gordy)
**	    Added fetch query flags.
**	 6-Jun-04 (wansh01)
**	    Added gcd admin error messages.
**	16-Jun-04 (gordy)
**	    Added ML Session Mask.
**	13-Aug-04 (gordy)
**	    While message segments cannot exceed 64K, the combined length
**	    of all segments may do so.  Increase message length to u_i4.
**	21-Apr-06 (gordy)
**	    Added INFO message queue.
**	31-Aug-06 (gordy)
**	    Added req_id and generalized parameter values to rqst_id0
**	    and rqst_id1 to support procedure parameter information.
**	 6-Nov-06 (gordy)
**	    Allow query text to be longer than 64K.
**	 5-Dec-07 (gordy)
**	    Added TL packet ID.
**	23-Mar-09 (gordy)
**	    Made request parameters dynamic length rather than fixed
**	    to support long database object names.
**	21-Jul-09 (gordy)
**	    Cursor name dynamically allocated.
**	25-Mar-10 (gordy)
**	    Add query processing function to share parameter processing
**	    between standard queries and batch query sets.  Store initial 
**	    connection handle (prior to IIapi_connect() completion) in 
**	    API parms.  Add storage for batch procedures.
**	13-May-10 (gordy)
**	    Add free RCB queue for default sized buffers.
*/

typedef struct
{
    char	protocol[ GCC_L_PROTOCOL + 1 ];
    char	node_id[ GCC_L_NODE + 1 ];
    char	port_id[ GCC_L_PORT + 1 ];
} GCD_ADDR;

typedef struct
{

    QUEUE	q;
    u_i4	id;
    u_i2	use_cnt;
    u_i1	sequence;	/* Processing state */
    GCD_CIB	*cib;		/* DBMS Connection Information Block */
    bool	isLocal;        /* local connection indicator */
    char	*client_user;	/* Client user ID */
    char	*client_host;	/* Client host name */
    char	*client_addr;	/* Client host addr */

    u_i2	max_buff_len;	/* Communication buffer size */
    QUEUE	rcb_q;		/* Free RCB - default buffer size */

    /*
    ** GCC interface info.
    */
    struct
    {
	GCD_PIB		*pib;			/* Protocol Information */
	GCC_PCE		*pce;			/* Protocol Driver */
	GCD_ADDR	lcl_addr;		/* Local Address */
	GCD_ADDR	rmt_addr;		/* Remote Address */
	PTR		pcb;			/* Protocol request */
	TL_SRVC		*tl_service;		/* TL Service */
	PTR		abort_rcb;		/* RCB for abort processing */
	QUEUE		send_q;			/* Send RCB queue */
	SYSTIME		idle_limit;		/* Time idle client expires */
	u_i1		state;			/* GCC FSM state */
	u_i4		flags;			/* Processing flags */

#define	GCD_GCC_CONN		0x01	/* CCB is connected */
#define	GCD_GCC_XOFF		0x02	/* XOFF requested */
#define	GCD_GCC_SHUT		0x04	/* Disconnect in progress */

	u_i4            gcd_msgs_in;            /* stats for admin interface */
	u_i4            gcd_msgs_out;           /* stats for admin interface */
	u_i4            gcd_data_in;            /* stats for admin interface */
	u_i4            gcd_data_out;           /* stats for admin interface */
    } gcc;

    struct 
    {
	GCC_SRVC	*gcc_service;		/* GCC Service */
	MSG_SRVC	*msg_service;		/* MSG Service */
	u_i4		packet_id;		/* TL packet ID */
	u_i1		proto_lvl;		/* Protocol level */
    } tl;

    /*
    ** Message processing info
    */
    struct
    {
	TL_MSG_SRVC	*tl_service;	/* TL Service Provider */
	PTR		xoff_pcb;	/* Active request when XOFF */
	u_i1		proto_lvl;	/* Protocol Level */
	bool		dtmc;		/* Dist Trans Mgmt Connection */

	/*
	** The session mask is used to modify the encryption keys.
	** It is currently defined to match the fixed key length
	** used by CIdecode().  Mask length should be set to 0
	** if not provided by client.
	*/

#define GCD_SESS_MASK_MAX	8

	u_i1		mask_len;
	u_i1		mask[ GCD_SESS_MASK_MAX ];

	/*
	** Parameter buffer.  Stores the parameters returned
	** in response to a new session request.  Each param
	** is composed of a single byte ID, a one or three
	** (if length >= 255) byte length, and the data.
	**
	** There are currently three possible parameters:
	**	Protocol Level: 1 byte of data.
	**	Dist Trans Mgmt Conn: 0 bytes of data.
	**	Session Mask: 8 bytes of data.
	*/

#define	GCD_MSG_PARAM_MAX	15

	u_i1		params[ GCD_MSG_PARAM_MAX ];

	u_i1		state;		/* Message processing state */
	u_i1		flags;		/* Processing flags */

#define	GCD_MSG_XOFF		0x01
#define	GCD_MSG_ACTIVE		0x02
#define	GCD_MSG_FLUSH		0x04

	QUEUE		msg_q;		/* Incomming message queue */
	u_i4		msg_hdr_id;	/* Message header ID */
	u_i1		msg_id;		/* Current message ID */
	u_i1		msg_flags;	/* Current message flags */
	u_i4		msg_len;	/* Current message length */

	bool		msg_active;	/* Outbound message initiated */
    	QUEUE		info_q;		/* Outbound INFO queue */
    } msg;

    struct
    {
	u_i4		tran_id;
	GCD_SPCB	*savepoints;
	bool		xacm_multi;
	u_i1		auto_mode;

#define	GCD_XACM_DBMS		0
#define	GCD_XACM_SINGLE		1
#define	GCD_XACM_MULTI		2

	PTR		distXID;

#define	GCD_XID_NAME		ERx("IIGCD-2PC")
#define	GCD_XID_SEQNUM		0
#define	GCD_XID_FLAGS		( IIAPI_XA_BRANCH_FLAG_FIRST | \
				  IIAPI_XA_BRANCH_FLAG_LAST  | \
				  IIAPI_XA_BRANCH_FLAG_2PC )
    } xact;

    struct
    {
	u_i4	stmt_id;
	QUEUE	stmt_q;
    } stmt;

    struct
    {
	u_i2	flags;		/* Query execution flags */

#define	QRY_TUPLES	0x0001	/* Query may produce a tuple stream */
#define	QRY_BLOB	0x0002	/* Tuple contains a BLOB column */
#define	QRY_CURSOR	0x0004	/* Query is 'open cursor' */
#define	QRY_UPDATE	0x0008	/* Cursor is updatable */
#define	QRY_PROCEDURE	0x0010	/* Query is 'execute procedure' */
#define	QRY_BYREF	0x0020	/* BYREF procedure parameter */
#define	QRY_FETCH_FIRST	0x0100	/* Fetch first block of rows */
#define	QRY_FETCH_ALL	0x0200	/* Fetch all rows */
#define	QRY_AUTO_CLOSE	0x0400	/* Close statement at EOD */
#define	QRY_NEED_DESC	0x1000	/* Expect parameter descriptor */
#define	QRY_HAVE_DESC	0x2000	/* Paremeter descriptor received */
#define	QRY_NEED_DATA	0x4000	/* Expect parameter data */
#define	QRY_HAVE_DATA	0x8000	/* Paremeter data received */

	QDATA	svc_parms;	/* API service parameters */
	QDATA	qry_parms;	/* Query parameters */
	QDATA	all_parms;	/* Combined API server and query parameters */
	QDATA	*api_parms;	/* Which parameter QDATA is used */
	void	(*qry_sm)(PTR);	/* Query processing routine */
	u_i4	qtxt_max;	/* Size of query text buffer */
	char	*qtxt;		/* Query text buffer */
	char	*crsr_name;	/* Cursor name */
	char	*schema_name;	/* Schema name */
	char	*proc_name;	/* Procedure name */
    } qry;

    struct
    {
	QUEUE	rqst_q;		/* Request processing queue */
	u_i2	req_id;
	char	*rqst_id0;
	char	*rqst_id1;
    } rqst;

    /*
    ** OpenAPI interface info.
    */
    struct
    {
	PTR	env;
	PTR	conn;
	PTR	stmt;
    } api;

} GCD_CCB;

/*
** Name: GCD_RCB - Request control block.
**
** Description:
**	Information describing a single client request or 
**	response message group.
**
** History:
**	15-Jan-03 (gordy)
**	    Changed buf_ptr to a pointer to simplify processing.
**	    buf_ptr always points into buffer, buf_len indicates
**	    amount of data following buf_ptr.
*/

typedef struct
{

    QUEUE	q;
    GCD_CCB	*ccb;
    u_i1	request;

    u_i1	*buffer;	/* Data buffer */
    u_i4	buf_max;	/* Size of buffer */
    u_i1	*buf_ptr;	/* Start of data */
    u_i4	buf_len;	/* Length of data */

#define	RCB_AVAIL( rcb )	(max((rcb)->buf_max - \
				     ((rcb)->buf_ptr - (rcb)->buffer) - \
				     (rcb)->buf_len, 0))
    /*
    ** GCC protocol driver info.
    */
    struct
    {
	GCC_P_PLIST	p_list;
    } gcc;

    /*
    ** MSG info.
    */
    struct
    {
	u_i1	msg_id;		/* Message type */
	u_i1	msg_flags;	/* EOD/EOG */
	u_i2	msg_len;	/* Message length */
	u_i4	msg_hdr_pos;	/* Buffer position of message header */
    } msg;

} GCD_RCB;

/*
** Name: GCD_SCB - Statement control block
**
** Description:
**	Information for active client statements (cursors).
**
** History:
**	 4-Aug-03 (gordy)
**	    Added flags.
**	20-Aug-03 (gordy)
**	    Added AUTO_CLOSE flag.
**	 1-Oct-03 (gordy)
**	    Added fetch flags.
**	21-Jul-09 (gordy)
**	    Cursor name dynamically allocated.
*/

typedef struct
{

    u_i4	id_high;
    u_i4	id_low;

} STMT_ID;

typedef struct
{

    QUEUE	q;				/* Statement queue */
    STMT_ID	stmt_id;			/* Unique statement ID */
    PTR		handle;				/* API handle */
    u_i2	flags;				/* Statement attributes */

/*
** The following flag values indicate the nature 
** of the tuple stream being returned by the DBMS.  
** These flags are exclusive: one and only one 
** should be set.
*/

#define	GCD_STMT_SELECT		0x01	/* Select statement */
#define	GCD_STMT_CURSOR		0x02	/* Cursor open */
#define	GCD_STMT_PROCEDURE	0x04	/* Row returning procedure */
#define	GCD_STMT_BYREF		0x08	/* BYREF parameters */

/*
** The following flag values may be set in any combination
** (including the exclusive flags above).
*/

#define	GCD_STMT_FETCH_ALL	0x10	/* Fetch all rows in single request */
#define	GCD_STMT_AUTO_CLOSE	0x20	/* Close statment at EOD */

    char	*crsr_name;		/* Cursor name (if cursor) */
    QDATA	column;			/* Result data-set */
    u_i2	seg_max;		/* Max segment length */
    u_i2	seg_len;		/* Current segment length */
    u_i1	*seg_buff;		/* Segment buffer */

} GCD_SCB;


/*
** Service Interfaces.
**
** The following structures define the entry points
** required in service providers at the various
** layer interfaces and provider entity.
*/

struct GCD_GCCTL_SERVICE	/* GCC  Service provider for TL access */
{
    /*
    ** Name: accept
    **
    ** Description:
    **     Indicates that the TL Service is accepting the connection.
    **     Provides the GCC Service the opportunity to reject the
    **     connection with a specific error condition.  If a status
    **     other than OK is returned, the TL Service should reject
    **     the new connection and return the status to the client
    **     if possible.
    **
    **     During connection establishment, the GCC Service provider
    **     does not have the capability of returning an error code by
    **     itself.  Many conditions related to physical communications 
    **     may cause a connection to be rejected and preclude returning
    **     any type of error status.  Other conditions unrelated to
    **     the physical connection may require rejection and which
    **     need to be reported to the client.
    **
    **     This entry point must be called from TL_SRVC->accept() if 
    **     the TL Service accepts the connection.  It does not need to 
    **     be called if the TL Service is rejecting the connection.  
    **     It must not be called if the TL Service does not service 
    **     the request passed to TL_SRVC->accept().
    **
    ** Parameters:
    **     TL_SRVC *	The TL Service servicing the connection.
    **     GCD_CCB *	Connection control block.
    **
    ** Returns:
    **     STATUS	OK or error code.
    */
    STATUS (*accept)( TL_SRVC *, GCD_CCB * );

    /*
    ** Name: xoff
    **
    ** Description: 
    **     Control data flow being received on a connection.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block
    **     bool		TRUE to pause receiving data, FALSE to resume
    */
    void (*xoff)( GCD_CCB *, bool );

    /*
    ** Name: send
    **
    ** Description: 
    **     Send data on a connection.
    **
    ** Parameters:
    **     GCD_RCB *	Request control block.
    */
    void (*send)( GCD_RCB * );

    /*
    ** Name: disc
    **
    ** Description: 
    **     Disconnect connection.  No further requests
    **     (in either direction) are allowed.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block
    */
    void (*disc)( GCD_CCB * );

};

struct GCD_TLGCC_SERVICE	/* TL Service provider for GCC access */
{
    /*
    ** Name: accept
    **
    ** Description:
    **     Called for first data packet received on a connection 
    **     to determine which TL Service will service the connection.
    **     Returns FALSE if provider does not service the request.
    **     Returns TRUE otherwise.  Provider may accept or reject
    **     connections which it services.
    **
    **     If TRUE is returned, the RCB is consumed.  Otherwise,
    **     the RCB is not changed.
    **
    **     If the Service Provider would normally accept the
    **     connection but an error occurs, the provider still
    **     indicates that the connection is serviced by this
    **     provider and initiates whatever actions are needed to
    **     abort the connection.  This must ultimately result in
    **     a call to the GCC Service Provider disc() function.
    **
    ** Parameters:
    **     GCC_SRVC *	The GCC Service servicing the connection.
    **     GCD_RCB *	Request control block containing TL packet data.
    **
    ** Returns:
    **     bool		TRUE if Service Provider accepts connection.
    */
    bool (*accept)( GCC_SRVC *, GCD_RCB * );

    /*
    ** Name: xoff
    **
    ** Description:
    **     Control data flow being sent on a connection.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block.
    **     bool		TRUE to pause sending data, FALSE to resume.
    */
    void (*xoff)( GCD_CCB *, bool );

    /*
    ** Name: data
    **
    ** Description:
    **     Called for each data packet received on a 
    **     connection (except the first, see accept()).
    **
    ** Parameters:
    **     GCD_RCB *	Request control block containing TL packet data.
    */
    void (*data)( GCD_RCB * );

    /*
    ** Name: abort
    **
    ** Description:
    **     Called to indicate that the GCC Service has 
    **     aborted the connection.  No further requests
    **     (in either direction) are allowed.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block.
    */
    void (*abort)( GCD_CCB * );

};

struct GCD_TLMSG_SERVICE	/* TL Service provider for MSG access */
{
    /*
    ** Name: xoff
    **
    ** Description:
    **     Control data flow being received on a connection.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block
    **     bool		TRUE to pause receiving data, FALSE to resume
    */
    void (*xoff)( GCD_CCB *, bool );

    /*
    ** Name: send
    **
    ** Description:
    **     Send data on a connection.
    **
    ** Parameters:
    **     GCD_RCB *	Request control block.
    */
    void (*send)( GCD_RCB * );

    /*
    ** Name: disc
    **
    ** Description:
    **     Disconnect a connection.  An error code other
    **     than OK indicates an abort condition.  No 
    **     further requests (in either direction) are 
    **     allowed.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block.
    **     STATUS	OK or Error code.
    */
    void (*disc)( GCD_CCB *, STATUS );

};

struct GCD_MSGTL_SERVICE	/* MSG Service provider for TL access */
{
    /*
    ** Name: init
    **
    ** Description:
    **     Initialize MSG Service for new connection.
    **     The MSG initialization parameter block is
    **	   provided as input and a response parameter
    **     block may be returned as output.
    **
    ** Parameters:
    **     TL_MSG_SRVC*	TL Service servicing the connection.
    **     GCD_CCB *	Connection control block.
    **     u_i1 **	MSG initialization parameter block.
    **     u_i2 *	Length of MSG parameter block.
    **
    ** Returns:
    **     STATUS	OK to accept connection, error code otherwise.
    */
    STATUS (*init)( TL_MSG_SRVC *, GCD_CCB *, u_i1 **, u_i2 * );

    /*
    ** Name: xoff
    **
    ** Description:
    **     Control data flow being sent on a connection.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block.
    **     bool		TRUE to pause sending data, FALSE to resume.
    */
    void (*xoff)( GCD_CCB *, bool );

    /*
    ** Name: data
    **
    ** Description:
    **     Called for each data packet received on a connection.
    **
    ** Parameters:
    **     GCD_RCB *	Request control block containing TL packet data.
    */
    void (*data)( GCD_RCB * );

    /*
    ** Name: intr
    **
    ** Description:
    **     Indicates that the current operation should be interrupted.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block.
    */
    void (*intr)( GCD_CCB * );

    /*
    ** Name: abort
    **
    ** Description:
    **     Called to indicate that the connection has been aborted.
    **     No further requests (in either direction) are allowed.
    **
    ** Parameters:
    **     GCD_CCB *	Connection control block.
    */
    void (*abort)( GCD_CCB * );

};

/*
** Session information for admin SHOW SESSIONS.
*/

typedef struct
{
    PTR         sess_id;
    i4          assoc_id;
    bool	is_system;
    bool	isLocal;
    bool	autocommit;
    char        state[ GCD_NAME_MAX + 1 ];
    char        *user_id;
    char        *database;
    char        srv_class[ GCD_NAME_MAX + 1 ];
    char        gca_addr[ 257 ];

} GCD_SESS_INFO;

#define GCD_USER_COUNT		1
#define GCD_SYS_COUNT		2
#define GCD_POOL_COUNT		3
#define GCD_ALL_COUNT		4
#define GCD_USER_INFO		5
#define GCD_SYS_INFO		6
#define GCD_POOL_INFO		7
#define GCD_ALL_INFO		8
#define GCD_SESS_USER		9
#define GCD_SESS_SYS		10
#define GCD_SESS_POOL		11
#define GCD_SESS_ADMIN		12



GLOBALREF TL_SRVC	gcd_dmtl_service;
GLOBALREF TL_SRVC	gcd_jctl_service;
GLOBALREF MSG_SRVC	gcd_dam_service;
GLOBALREF MSG_SRVC	gcd_jdbc_service;


/*
** Error codes
*/

#define	E_GCF_MASK	(12 * 0x10000)

#define	E_GC4800_STARTUP		(E_GCF_MASK | 0x4800)
#define E_GC4801_SHUTDOWN		(E_GCF_MASK | 0x4801)
#define E_GC4802_LOAD_CONFIG		(E_GCF_MASK | 0x4802)
#define E_GC4803_NTWK_OPEN		(E_GCF_MASK | 0x4803)
#define E_GC4804_NTWK_CONFIG		(E_GCF_MASK | 0x4804)
#define E_GC4805_NTWK_RQST_FAIL		(E_GCF_MASK | 0x4805)
#define	E_GC4806_NTWK_INIT_FAIL		(E_GCF_MASK | 0x4806)
#define E_GC4807_CONN_ABORT		(E_GCF_MASK | 0x4807)
#define E_GC4808_NO_MEMORY		(E_GCF_MASK | 0x4808)
#define E_GC4809_INTERNAL_ERROR		(E_GCF_MASK | 0x4809)
#define E_GC480A_PROTOCOL_ERROR		(E_GCF_MASK | 0x480A)
#define E_GC480B_NO_CLIENTS		(E_GCF_MASK | 0x480B)
#define	E_GC480C_NO_AUTH		(E_GCF_MASK | 0x480C)
#define	E_GC480D_IDLE_LIMIT		(E_GCF_MASK | 0x480D)
#define	E_GC480E_CLIENT_MAX		(E_GCF_MASK | 0x480E)
#define E_GC480F_EXCEPTION		(E_GCF_MASK | 0x480F)
#define E_GC4810_TL_FSM_STATE		(E_GCF_MASK | 0x4810)
#define E_GC4811_NO_STMT		(E_GCF_MASK | 0x4811)
#define E_GC4812_UNSUPP_SQL_TYPE	(E_GCF_MASK | 0x4812)
#define E_GC4813_XACT_AC_STATE		(E_GCF_MASK | 0x4813)
#define E_GC4814_XACT_BEGIN_STATE	(E_GCF_MASK | 0x4814)
#define E_GC4815_XACT_PREP_STATE	(E_GCF_MASK | 0x4815)
#define E_GC4816_XACT_REG_XID		(E_GCF_MASK | 0x4816)
#define E_GC4817_CONN_PARM		(E_GCF_MASK | 0x4817)
#define	E_GC4818_XACT_SAVEPOINT		(E_GCF_MASK | 0x4818)
#define	E_GC4819_XACT_ABORT_STATE	(E_GCF_MASK | 0x4819)
#define	E_GC481A_XACT_END_STATE		(E_GCF_MASK | 0x481A)
#define E_GC481E_GCD_SESS_ABORT 	(E_GCF_MASK | 0x481E)
#define E_GC481F_GCD_SVR_CLOSED 	(E_GCF_MASK | 0x481F)
#define E_GC4820_GCD_ADM_INIT_ERR	(E_GCF_MASK | 0x4820)
#define E_GC4821_GCD_ADM_SESS_ERR	(E_GCF_MASK | 0x4821)


/*
** Global functions.
*/

FUNC_EXTERN void	gcd_init_mib( void );
FUNC_EXTERN STATUS	gcd_adm_init( void );
FUNC_EXTERN STATUS	gcd_adm_term( void );
FUNC_EXTERN STATUS	gcd_adm_session( i4, PTR, char * );

FUNC_EXTERN bool        gcd_gca_activate( void );
FUNC_EXTERN STATUS	gcd_gca_init( void );
FUNC_EXTERN void	gcd_gca_term( void );
FUNC_EXTERN STATUS	gcd_get_env( u_i2, PTR * );
FUNC_EXTERN void	gcd_rel_env( u_i2 );

FUNC_EXTERN STATUS	gcd_gcc_init( void );
FUNC_EXTERN void	gcd_gcc_term( void );
FUNC_EXTERN void	gcd_idle_check( void );

FUNC_EXTERN STATUS	gcd_pool_init( void );
FUNC_EXTERN void	gcd_pool_term( void );
FUNC_EXTERN bool	gcd_pool_save( GCD_CIB * );
FUNC_EXTERN GCD_CIB	*gcd_pool_find( GCD_CIB * );
FUNC_EXTERN bool	gcd_pool_flush( u_i4 );
FUNC_EXTERN void	gcd_pool_check( void );
FUNC_EXTERN i4          gcd_pool_info( PTR, bool );
FUNC_EXTERN bool     	gcd_pool_remove( GCD_CIB * );


FUNC_EXTERN void	gcd_sess_term( GCD_CCB * );
FUNC_EXTERN void	gcd_sess_abort( GCD_CCB *, STATUS );
FUNC_EXTERN void	gcd_sess_error( GCD_CCB *, GCD_RCB **, STATUS );
FUNC_EXTERN i4          gcd_sess_count( i4 );
FUNC_EXTERN GCD_CCB	*gcd_sess_find( PTR );

FUNC_EXTERN GCD_CCB	*gcd_new_ccb( void );
FUNC_EXTERN void	gcd_del_ccb( GCD_CCB * );
FUNC_EXTERN GCD_CIB	*gcd_new_cib( u_i2 );
FUNC_EXTERN void	gcd_del_cib( GCD_CIB * );
FUNC_EXTERN void	gcd_release_sp( GCD_CCB *, GCD_SPCB * );
FUNC_EXTERN GCD_RCB	*gcd_new_rcb( GCD_CCB *, i2 );
FUNC_EXTERN void	gcd_init_rcb( GCD_RCB * );
FUNC_EXTERN void	gcd_del_rcb( GCD_RCB * );
FUNC_EXTERN bool	gcd_alloc_qdesc( QDATA *, u_i2, PTR );
FUNC_EXTERN bool	gcd_alloc_qdata( QDATA *, u_i2 );
FUNC_EXTERN bool	gcd_build_qdata( QDATA *, QDATA *, QDATA * );
FUNC_EXTERN void	gcd_combine_qdata( QDATA *, QDATA *, QDATA * );
FUNC_EXTERN void	gcd_free_qdata( QDATA * );
FUNC_EXTERN void	gcd_decode( char *, u_i1 *, u_i1 *, u_i2, char * );


FUNC_EXTERN void	gca_sm ( i4 ); 
#endif /* _GCD_INCLUDED_ */
