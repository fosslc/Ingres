/*
** Copyright (c) 2004 Ingres Corporation
*/

/* Name: jdbc.h
**
** Description:
**	Global definitions for the JDBC server.
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
**	 18-Aug-04 (rajus01)  Bug #112848 ; Startrak Problem #EDJDBC92 
**	    It is noted that the u_i2 variables 'bb_max', 'db_max' of QDATA,
**	    'msg_len' of JDBC_CCB get overrun by the long parameter data 
**	    and resulted in JDBC server crashing. Changed the above u_i2 
**	    variables to u_i4.
**	 7-Jun-05 (gordy)
**	    Added connection and transaction abort flags to CIB.
**	25-Aug-2009 (kschendel) b121804
**	    Add jdbc-build-qdata.
*/

#ifndef _JDBC_INCLUDED_
#define _JDBC_INCLUDED_

#include <gcccl.h>
#include <qu.h>
#include <tm.h>
#include <iiapi.h>

#define	JDBC_SRV_CLASS		"JDBC"
#define	JDBC_GCA_PROTO_LVL	GCA_PROTOCOL_LEVEL_62
#define	JDBC_LOG_ID		"JDBC"

#define	JDBC_NAME_MAX		32

#define	ARR_SIZE( arr )		(sizeof(arr)/sizeof((arr)[0]))
#define	QUEUE_EMPTY( q )	(((QUEUE *)(q))->q_next == ((QUEUE *)(q)))

/*
** Transaction query operations
** (for jdbc_xact_check()).
*/

#define	JDBC_XQOP_NON_CURSOR	0
#define	JDBC_XQOP_CURSOR_OPEN	1
#define	JDBC_XQOP_CURSOR_UPDATE	2
#define	JDBC_XQOP_CURSOR_CLOSE	3

/*
** Name: JDBC_GLOBAL
**
** Description:
**	JDBC Server global information.
*/

typedef struct
{

    char	*charset;
    char	*protocol;
    char	*port;
    i4		language;
    i4		trace_level;
    PTR		api_env;

    i4		client_idle_limit;
    SYSTIME	client_check;

    u_i4	ccb_total;
    u_i4	ccb_active;
    QUEUE	ccb_free;
    QUEUE	ccb_q;

    u_i4	cib_max;
    u_i4	cib_total;
    u_i4	cib_active;
    QUEUE	cib_free[ 10 ];	/* Size based on connection parameter */
				/* count used to allocate CIBs */
    bool	client_pooling;
    i4		pool_max;
    i4		pool_cnt;
    i4		pool_idle_limit;
    SYSTIME	pool_check;
    
} JDBC_GLOBAL;

GLOBALREF JDBC_GLOBAL	JDBC_global;

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
** Name: JDBC_CIB
**
** Description:
**	Connection Information Block.  Contains the connection
**	data which uniquely identifies a given DBMS connection.
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

} JDBC_CAPS;

typedef struct
{
    QUEUE	q;
    u_i4	id;
    i4		level;		/* API level */
    PTR		conn;		/* API connection handle */

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
    bool	xact_abort;
    bool	conn_abort;

    bool	pool_off;	/* Connection pool disabled */
    bool	pool_on;	/* Connection pool requested */
    SYSTIME	expire;		/* Connection pool expiration limit */

    char	*database;
    char	*username;
    char	*password;

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
    bool	gateway;
    i1		db_name_case;
    
#define	DB_CASE_UPPER	1
#define	DB_CASE_LOWER	0
#define	DB_CASE_MIXED	-1

    QUEUE	caps;

    u_i2	parm_max;
    u_i2	parm_cnt;
    CONN_PARM	parms[1];	/* Variable length */

} JDBC_CIB;

/*
** Name: JDBC_CCB - Connection control block.
**
** Description:
**	Information describing a client connection.
**	Various sub-structures provide storage for
**	the processing layers in the JDBC server.
*/

typedef struct
{

    QUEUE	q;
    u_i4	id;
    u_i2	use_cnt;
    u_i1	sequence;	/* Processing state */
    JDBC_CIB	*cib;		/* DBMS Connection Information Block */

    u_i2	max_buff_len;	/* Communication buffer size */
    u_i2	max_mesg_len;	/* Buffer space available for messages */
    u_i2	max_data_len;	/* Buffer space available for message data */

    /*
    ** GCC interface info.
    */
    struct
    {
	u_i4	flags;

#define	JDBC_GCC_XOFF		0x01

	u_i1	state;
	u_i1	proto_lvl;
	GCC_PCE	*pce;
	PTR	pcb;
	PTR	xoff_rcb;
	PTR	abort_rcb;
	QUEUE	send_q;
	SYSTIME	idle_limit;
    } gcc;

    /*
    ** Client connection info.
    */
    struct
    {
	/*
	** Connection parameters
	*/
	u_i1	proto_lvl;	/* Protocol Level */
	bool	dtmc;		/* Dist Trans Mgmt Connection */

	/*
	** Parameter buffer.  Stores the parameters returned
	** in response to a new session request.  Each param
	** is composed of a single byte ID, a one or three
	** (if length >= 255) byte length, and the data.
	**
	** There are currently two possible parameters:
	**	Protocol Level: 1 byte of data.
	**	Dist Trans Mgmt Conn: 0 bytes of data.
	*/

#define	JDBC_MSG_PARAM_LEN	5

	u_i1	params[ JDBC_MSG_PARAM_LEN ];
    } conn_info;

    /*
    ** Message processing info
    */
    struct
    {
	u_i4	flags;

#define	JDBC_MSG_XOFF		0x01
#define	JDBC_MSG_ACTIVE		0x02
#define	JDBC_MSG_FLUSH		0x04

	u_i1	state;
	PTR	xoff_pcb;

	QUEUE	msg_q;
	u_i1	msg_id;
	u_i1	msg_flags;
	u_i4	msg_len;
    } msg;

    struct
    {
	u_i4	tran_id;
	PTR	distXID;

#define	JDBC_XID_NAME		ERx("JDBC-2PC")
#define	JDBC_XID_SEQNUM		0
#define	JDBC_XID_FLAGS		( IIAPI_XA_BRANCH_FLAG_FIRST | \
				  IIAPI_XA_BRANCH_FLAG_LAST  | \
				  IIAPI_XA_BRANCH_FLAG_2PC )

	u_i1	auto_mode;

#define	JDBC_XACM_DBMS		0
#define	JDBC_XACM_SINGLE	1
#define	JDBC_XACM_MULTI		2

	bool	xacm_multi;
    } xact;

    struct
    {
	u_i4	stmt_id;
	QUEUE	stmt_q;
    } stmt;

    struct
    {
	u_i2	flags;		/* Query execution flags */

#define	QRY_TUPLES	0x01	/* Query may produce a tuple stream */
#define	QRY_CURSOR	0x02	/* Query is 'open cursor' */
#define	QRY_PROCEDURE	0x04	/* Query is 'execute procedure' */
#define	QRY_NEED_DESC	0x10	/* Expect parameter descriptor */
#define	QRY_HAVE_DESC	0x20	/* Paremeter descriptor received */
#define	QRY_NEED_DATA	0x40	/* Expect parameter data */
#define	QRY_HAVE_DATA	0x80	/* Paremeter data received */

	QDATA	svc_parms;	/* API service parameters */
	QDATA	qry_parms;	/* Query parameters */
	QDATA	all_parms;	/* Combined API server and query parameters */
	QDATA	*api_parms;	/* Which parameter QDATA is used */

	PTR	handle;		/* API statement handle */
	char	crsr_name[ JDBC_NAME_MAX + 1 ];

	u_i2	qtxt_max;	/* Size of query text buffer */
	char	*qtxt;		/* Query text buffer */
    } qry;

    struct
    {
	QUEUE	rqst_q;		/* Request processing queue */
	char	rqst_id[ JDBC_NAME_MAX + 1 ];
	char	rqst_val[ JDBC_NAME_MAX + 1 ];
    } rqst;

    /*
    ** OpenAPI interface info.
    */
    struct
    {
	PTR	env;
	PTR	stmt;
    } api;

} JDBC_CCB;

/*
** Name: JDBC_RCB - Request control block.
**
** Description:
**	Information describing a single client
**	request or response message group.
*/

typedef struct
{

    QUEUE	q;
    JDBC_CCB	*ccb;
    u_i1	request;

    u_i1	*buffer;
    u_i4	buf_max;	/* Size of buffer */
    u_i4	buf_ptr;	/* Start of data */
    u_i4	buf_len;	/* Length of data */

#define	RCB_AVAIL( rcb )	(max((rcb)->buf_max - (rcb)->buf_len,0))

    /*
    ** GCC interface info.
    */
    struct
    {
	GCC_P_PLIST	p_list;
    } gcc;

    struct
    {
	u_i1	msg_id;
	u_i1	msg_flags;
	u_i2	msg_len;
	u_i4	msg_hdr;
    } msg;

} JDBC_RCB;

/*
** Name: JDBC_SCB - Statement control block
**
** Description:
**	Information for active client statements (cursors).
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
    char	crsr_name[ JDBC_NAME_MAX + 1 ];	/* Cursor name (if cursor) */
    PTR		handle;				/* API handle */
    QDATA	column;				/* Result data-set */
    u_i2	seg_max;			/* Max segment length */
    u_i2	seg_len;			/* Current segment length */
    u_i1	*seg_buff;			/* Segment buffer */

} JDBC_SCB;


/*
** Error codes
*/

#define	E_JD_MASK	(237 * 0x10000)

#define	E_JD0100_STARTUP		(E_JD_MASK | 0x0100)
#define E_JD0101_SHUTDOWN		(E_JD_MASK | 0x0101)
#define E_JD0102_EXCEPTION		(E_JD_MASK | 0x0102)
#define E_JD0103_TL_FSM_STATE		(E_JD_MASK | 0x0103)
#define E_JD0104_NET_CONFIG		(E_JD_MASK | 0x0104)
#define E_JD0105_NTWK_RQST_FAIL		(E_JD_MASK | 0x0105)
#define E_JD0106_NTWK_OPEN		(E_JD_MASK | 0x0106)
#define E_JD0107_CONN_ABORT		(E_JD_MASK | 0x0107)
#define E_JD0108_NO_MEMORY		(E_JD_MASK | 0x0108)
#define E_JD0109_INTERNAL_ERROR		(E_JD_MASK | 0x0109)
#define E_JD010A_PROTOCOL_ERROR		(E_JD_MASK | 0x010A)
#define E_JD010B_NO_CLIENTS		(E_JD_MASK | 0x010B)
#define	E_JD010C_NO_AUTH		(E_JD_MASK | 0x010C)
#define	E_JD010D_IDLE_LIMIT		(E_JD_MASK | 0x010D)
#define	E_JD010E_CLIENT_MAX		(E_JD_MASK | 0x010E)
#define E_JD0110_XACT_AC_STATE		(E_JD_MASK | 0x0110)
#define E_JD0111_NO_STMT		(E_JD_MASK | 0x0111)
#define E_JD0112_UNSUPP_SQL_TYPE	(E_JD_MASK | 0x0112)
#define E_JD0113_LOAD_CONFIG		(E_JD_MASK | 0x0113)
#define E_JD0114_XACT_BEGIN_STATE	(E_JD_MASK | 0x0114)
#define E_JD0115_XACT_PREP_STATE	(E_JD_MASK | 0x0115)
#define E_JD0116_XACT_REG_XID		(E_JD_MASK | 0x0116)


/*
** Global functions.
*/

FUNC_EXTERN STATUS	jdbc_gca_init( char * );
FUNC_EXTERN void	jdbc_gca_term( void );

FUNC_EXTERN STATUS	jdbc_gcc_init( void );
FUNC_EXTERN void	jdbc_gcc_term( void );
FUNC_EXTERN void	jdbc_gcc_send( JDBC_RCB * );
FUNC_EXTERN void	jdbc_idle_check( void );
FUNC_EXTERN void	jdbc_gcc_xoff( JDBC_CCB *, bool );
FUNC_EXTERN void	jdbc_gcc_abort( JDBC_CCB *, STATUS );

FUNC_EXTERN STATUS	jdbc_pool_init( void );
FUNC_EXTERN void	jdbc_pool_term( void );
FUNC_EXTERN bool	jdbc_pool_save( JDBC_CIB * );
FUNC_EXTERN JDBC_CIB	*jdbc_pool_find( JDBC_CIB * );
FUNC_EXTERN bool	jdbc_pool_flush( u_i4 );
FUNC_EXTERN void	jdbc_pool_check( void );

FUNC_EXTERN STATUS	jdbc_sess_begin( JDBC_CCB *, u_i1 **, u_i2 * );
FUNC_EXTERN void	jdbc_sess_process( JDBC_RCB * );
FUNC_EXTERN void	jdbc_sess_interrupt( JDBC_CCB * );
FUNC_EXTERN void	jdbc_sess_end( JDBC_CCB * );
FUNC_EXTERN void	jdbc_sess_error( JDBC_CCB *, JDBC_RCB **, STATUS );

FUNC_EXTERN JDBC_CCB	*jdbc_new_ccb( void );
FUNC_EXTERN void	jdbc_del_ccb( JDBC_CCB * );
FUNC_EXTERN JDBC_CIB	*jdbc_new_cib( u_i2 );
FUNC_EXTERN void	jdbc_del_cib( JDBC_CIB * );
FUNC_EXTERN JDBC_RCB	*jdbc_new_rcb( JDBC_CCB * );
FUNC_EXTERN void	jdbc_del_rcb( JDBC_RCB * );
FUNC_EXTERN bool	jdbc_alloc_qdesc( QDATA *, u_i2, PTR );
FUNC_EXTERN bool	jdbc_alloc_qdata( QDATA *, u_i2 );
FUNC_EXTERN bool	jdbc_build_qdata( QDATA *, QDATA *, QDATA * );
FUNC_EXTERN void	jdbc_free_qdata( QDATA * );
FUNC_EXTERN void	jdbc_decode( char *, u_i1 *, u_i2, char * );

#endif /* _JDBC_INCLUDED_ */
