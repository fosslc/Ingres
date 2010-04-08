/*
** Copyright (c) 2004 Ingres Corporation
*/

package ca.edbc.util;

/*
** Name: MsgConst.java
**
** Description:
**	Defines the global constants used in the driver-server
**	messaging communication system.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	17-Sep-99 (rajus01)
**	    Added connection parameters.
**	26-Oct-99 (gordy)
**	    Added query types for cursor positioned DELETE/UPDATE.
**	 4-Nov-99 (gordy)
**	    Added timeout connection parameter.
**	13-Dec-99 (gordy)
**	    Added fetch limit result parameter, pre-fetch cursor parameter.
**	14-Jan-00 (gordy)
**	    Added Connection Pool connection parameter and Request message
**	    for pre-defined queries and statements (the first being the
**	    DBMS capabilities query).
**	21-Apr-00 (gordy)
**	    Moved to package util.  Removed non-messaging constants.
**	19-May-00 (gordy)
**	    Added internal connection parameter for select loops and
**	    result parameter for end-of-data (tuples).
**	15-Jun-00 (gordy)
**	    Added query parameters and descriptor flags for procedures.
**	17-Oct-00 (gordy)
**	    Added autocommit mode connection parameter and values.
**	31-Oct-00 (gordy)
**	    Added read-only cursor return parameter.
**	20-Jan-01 (gordy)
**	    Added protocol level 2.
**	 7-Mar-01 (gordy)
**	    Added REQUEST for dbmsinfo().
**	21-Mar-01 (gordy)
**	    Added support for Distributed Transactions.
**	 2-Apr-01 (gordy)
**	    Added JDBC_XP_ING_XID.
**	18-Apr-01 (gordy)
**	    Added support for Distributed Transaction Management Connections.
**      13-Mon-01 (loera01) SIR 105521
**          Added JDBC_DSC_GTT constant to support global temp table parameters
**          in the CallableStatement interface.
**	20-Aug-01 (gordy)
**	    Added internal connection parameter JDBC_CP_CURSOR_MODE, query
**	    flags parameter JDBC_QP_FLAGS, and query flag JDBC_QF_READONLY.
*/


/*
** Name: MsgConst
**
** Description:
**	Interface defining the global constants used in driver-
**	server messages.
**
**  Protocol Levels
**
**	JDBC_MSG_PROTO_1
**
**	    Base protocol level for Beta testing and initial release.
**
**	JDBC_MSG_PROTO_2
**
**	    Messaging parameters:
**		JDBC_MSG_P_DTMC
**
**	    Connection parameters: 
**		JDBC_CP_AUTO_MODE, JDBC_CP_II_XID, JDBC_CP_XA_FRMT,
**		JDBC_CP_XA_GTID, JDBC_CP_XA_BQUAL
**
**	    Transaction operations and parameters:
**		JDBC_XACT_BEGIN, JDBC_XACT_PREPARE, JDBC_XP_II_XID,
**		JDBC_XP_XA_FRMT, JDBC_XP_XA_GTID, JDBC_ZP_XA_BQUAL
**
**	    Query parameters:
**		JDBC_QP_SCHEMA_NAME, JDBC_QP_PROC_NAME
**
**	    Request operations and parameters:
**		JDBC_REQ_PARAM_NAMES, JDBC_RQP_SCHEMA_NAME, JDBC_RQP_PROC_NAME
**		JDBC_REQ_DBMSINFO, JDBC_RQP_INFO_ID
**		JDBC_REQ_2PC_XIDS, JDBC_RQP_DB_NAME
**
**	    Result parameters: 
**		JDBC_RP_EOD, JDBC_RP_PROC_RESULT, JDBC_RP_READ_ONLY
**	    
**  Constants
**
**	JDBC_MSG_P_PROTO	Init parameter: protocol level.
**	JDBC_MSG_P_DTMC		Init parameter: Dist Transact Mgmt Connect.
**	JDBC_MSG_PROTO_1	Init param value: protocol level 1.
**	JDBC_MSG_PROTO_2	Init param value: protocol level 2.
**	JDBC_MSG_DFLT_PROTO	Supported message protocol level.
**
**	JDBC_MSGHDR_ID		Message Header ID.
**	JDBC_HDRFLG_EOD		Message Flag: end-of-data.
**	JDBC_HDRFLG_EOG		Message Flag: end-of-group.
**	JDBC_MSG_CONNECT	Message Type: connect request.
**	JDBC_MSG_DISCONN	Message Type: disconnect request.
**	JDBC_MSG_XACT		Message Type: transaction request.
**	JDBC_MSG_QUERY		Message Type: query request.
**	JDBC_MSG_CURSOR		Message Type: cursor request.
**	JDBC_MSG_DESC		Message Type: data descriptor.
**	JDBC_MSG_DATA		Message Type: data.
**	JDBC_MSG_ERROR		Message Type: error message.
**	JDBC_MSG_DONE		Message Type: request completed.
**	JDBC_MSG_REQUEST	Message Type: Pre-defined request.
**
**	JDBC_CP_DATABASE	Connect parameter: database specification
**	JDBC_CP_USERNAME	Connect parameter: user ID.
**	JDBC_CP_PASSWORD	Connect parameter: password.
**	JDBC_CP_DBUSERNAME	Connect parameter: effective user ID
**	JDBC_CP_GROUP_ID	Connect parameter: group ID
**	JDBC_CP_ROLE_ID		Connect parameter: role ID
**	JDBC_CP_DBPASSWORD	Connect parameter: DBMS password.
**	JDBC_CP_TIMEOUT		Connect parameter: timeout.
**	JDBC_CP_CONNECT_POOL	Connect parameter: connection pool.
**	JDBC_CP_AUTO_MODE	Connect parameter: autocommit mode.
**	JDBC_CP_II_XID		Connect parameter: Ingres XID.
**	JDBC_CP_XA_FRMT		Connect parameter: XA Format ID.
**	JDBC_CP_XA_GTID		Connect parameter: XA Globat Trans ID.
**	JDBC_CP_XA_BQUAL	Connect parameter: XA Branch qualifier.
**	JDBC_CP_SELECT_LOOP	Connect parameter: select loops (internal).
**	JDBC_CP_CURSOR_MODE	Connect parameter: cursor mode (internal).
**	JDBC_CPV_POOL_OFF	Connect param value: connect pool off.
**	JDBC_CPV_POOL_ON	Connect param value: connect pool on.
**	JDBC_CPV_XACM_DBMS	Connect param value: auto mode DBMS.
**	JDBC_CPV_XACM_SINGLE	Connect param value: auto mode single.
**	JDBC_CPV_XACM_MULTI	Connect param value: auto mode multi.
**	JDBC_XACT_COMMIT	Transaction action: commit transaction.
**	JDBC_XACT_ROLLBACK	Transaction action: rollback transaction.
**	JDBC_XACT_AC_ENABLE	Transaction action: enable autocommit.
**	JDBC_XACT_AC_DISABLE	Transaction action: disable autocommit.
**	JDBC_XACT_BEGIN		Transaction action: begin distributed.
**	JDBC_XACT_PREPARE	Transaction action: prepare distributed.
**	JDBC_XP_II_XID		Transaction param value: Ingres XID.
**	JDBC_XP_XA_FRMT		Transaction param value: XA Format ID.
**	JDBC_XP_XA_GTID		Transaction param value: XA Global Trans ID.
**	JDBC_XP_XA_BQUAL	Transaction param value: XA Branch Qualifier.
**	JDBC_QRY_EXQ		Query type: Execute query.
**	JDBC_QRY_OCQ		Query type: Open cursor for query.
**	JDBC_QRY_PREP		Query type: Prepare statement.
**	JDBC_QRY_EXS		Query type: Execute statement.
**	JDBC_QRY_OCS		Query type: Open cursor for statement.
**	JDBC_QRY_EXP		Query type: Execute procedure.
**	JDBC_QRY_CDEL		Query type: cursor delete.
**	JDBC_QRY_CUPD		Query type: cursor update.
**	JDBC_QP_QTXT		Query parameter: Query text.
**	JDBC_QP_CRSR_NAME	Query parameter: Cursor name.
**	JDBC_QP_STMT_NAME	Query parameter: Statement name.
**	JDBC_QP_SCHEMA_NAME	Query parameter: Schema name.
**	JDBC_QP_PROC_NAME	Query parameter: Procedure name.
**	JDBC_QP_FLAGS		Query parameter: Flags.
**	JDBC_QF_READONLY	Query flag: Readonly query.
**	JDBC_CUR_CLOSE		Cursor action: close.
**	JDBC_CUR_FETCH		Cursor action: fetch.
**	JDBC_CUR_STMT_ID	Cursor parameter: Cursor ID.
**	JDBC_CUR_PRE_FETCH	Cursor parameter: pre-fetch rows.
**	JDBC_DSC_NULL		Nullable.
**	JDBC_DSC_PIN		Procedure param: input.
**	JDBC_DSC_POUT		Procedure param: output.
**	JDBC_DSC_GTT		Procedure param: global temp table.
**	JDBC_ET_MSG		Error type: user message.
**	JDBC_ET_WRN		Error type: warning.
**	JDBC_ET_ERR		Error type: error.
**	JDBC_RP_ROWCOUNT	Result parameter: row count.
**	JDBC_RP_XACT_END	Result parameter: end-of-transaction.
**	JDBC_RP_STMT_ID		Result parameter: Statement or Cursor ID.
**	JDBC_RP_EOD		Result parameter: End-of-data.
**	JDBC_RP_PROC_RESULT	Result parameter: Procedure result.
**	JDBC_RP_READ_ONLY	Result parameter: Read-only cursor.
**	JDBC_REQ_CAPABILITY	Request type: DBMS capabilities.
**	JDBC_REQ_PARAM_NAMES	Request type: Parameter names.
**	JDBC_REQ_DBMSINFO	Request type: dbmsinfo().
**	JDBC_RQP_SCHEMA_NAME	Request parameter: Schema name.
**	JDBC_RQP_PROC_NAME	Request parameter: Procedure name.
**	JDBC_RQP_INFO_ID	Request parameter: dbmsinfo() ID.
**
**  Public Data:
**
**	msgMap			Mapping table for message ID.
**	cpMap			Mapping table for connection parameters.
**
** History:
**	 8-Sep-99 (gordy)
**	    Created.
**	17-Sep-99 (rajus01)
**	    Added connection parameters.
**	26-Oct-99 (gordy)
**	    Added query types for cursor positioned DELETE/UPDATE.
**	 4-Nov-99 (gordy)
**	    Added timeout connection parameter.
**	11-Nov-99 (rajus01)
**	    Implement DatabaseMetaData: Add DBMS_NAME_LEN, DBMS_TBLS_INSEL. 
**	13-Dec-99 (gordy)
**	    Added fetch limit result parameter, cursor pre-fetch parameter.
**	14-Jan-00 (gordy)
**	    Added Connection Pool connection parameter and Request message
**	    for pre-defined queries and statements (the first being the
**	    DBMS capabilities query).
**	19-May-00 (gordy)
**	    Added internal connection parameter for select loops and
**	    result parameter for end-of-data (tuples).
**	15-Jun-00 (gordy)
**	    Added query parameters and descriptor flags for procedures.
**	17-Oct-00 (gordy)
**	    Added autocommit mode connection parameter and values.
**	31-Oct-00 (gordy)
**	    Added read-only cursor return parameter.
**	20-Jan-01 (gordy)
**	    Added protocol level 2 for changes since 19-May-00.
**	 7-Mar-01 (gordy)
**	    Added REQUEST for dbmsinfo().
**	21-Mar-01 (gordy)
**	    Added JDBC_XACT_BEGIN, JDBC_XACT_PREPARE, JDBC_XP_XA_FRMT,
**	    JDBC_XP_XA_GTID, JDBC_XP_XA_BQUAL.
**	 2-Apr-01 (gordy)
**	    Added JDBC_XP_II_XID.
**	18-Apr-01 (gordy)
**	    Added JDBC_MSG_P_DTMC, JDBC_CP_II_XID, JDBC_CP_XA_FRMT,
**	    JDBC_CP_XA_GTID, JDBC_CP_XA_BQUAL, JDBC_REQ_2PC_XIDS,
**	    JDBC_RP_DB_NAME to support Distributed Transaction
**	    Management Connections.  Renamed JDBC_XP_ING_XID to
**	    JDBC_XP_II_XID and JDBC_CP_EUSERNAME to JDBC_CP_DBUSERNAME.
**	20-Aug-01 (gordy)
**	    Added internal connection parameter JDBC_CP_CURSOR_MODE, query
**	    flags parameter JDBC_QP_FLAGS, and query flag JDBC_QF_READONLY.
*/

public interface
MsgConst
{

    byte	JDBC_MSG_P_PROTO    = 0x01;	// Parameter: protocol
    byte	JDBC_MSG_P_DTMC	    = 0x02;
    
    byte	JDBC_MSG_PROTO_1    = 0x01;	// Protocol level 1
    byte	JDBC_MSG_PROTO_2    = 0x02;	// Protocol level 2
    byte	JDBC_MSG_DFLT_PROTO = JDBC_MSG_PROTO_2;

    int		JDBC_MSGHDR_ID = 0x4342444A;	// Message ID: "JDBC"
    byte	JDBC_HDRFLG_EOD	    = 0x01;	// end-of-data
    byte	JDBC_HDRFLG_EOG	    = 0x02;	// end-of-group

    /*
    ** Message types.
    */
    byte	JDBC_MSG_CONNECT = 0x01;	// Connect
    byte	JDBC_MSG_DISCONN = 0x02;	// Disconnect
    byte	JDBC_MSG_XACT	 = 0x03;	// Transaction
    byte	JDBC_MSG_QUERY   = 0x04;	// Query
    byte	JDBC_MSG_CURSOR  = 0x05;	// Cursor
    byte	JDBC_MSG_DESC	 = 0x06;	// Descriptor
    byte	JDBC_MSG_DATA    = 0x07;	// Data
    byte	JDBC_MSG_ERROR   = 0x08;	// Error
    byte	JDBC_MSG_DONE    = 0x09;	// Done
    byte	JDBC_MSG_REQUEST = 0x0A;	// Request

    IdMap	msgMap[] =
    {
	new IdMap( JDBC_MSG_CONNECT,	"CONNECT" ),
	new IdMap( JDBC_MSG_DISCONN,	"DISCONN" ),
	new IdMap( JDBC_MSG_XACT,	"XACT" ),
	new IdMap( JDBC_MSG_QUERY,	"QUERY" ),
	new IdMap( JDBC_MSG_CURSOR,	"CURSOR" ),
	new IdMap( JDBC_MSG_DESC,	"DESC" ),
	new IdMap( JDBC_MSG_DATA,	"DATA" ),
	new IdMap( JDBC_MSG_ERROR,	"ERROR" ),
	new IdMap( JDBC_MSG_DONE,	"DONE" ),
	new IdMap( JDBC_MSG_REQUEST,	"REQUEST" ),
    };

    /*
    ** Connection parameters.
    */
    short	JDBC_CP_DATABASE 	= 0x0001;	// Database name
    short	JDBC_CP_USERNAME 	= 0x0002;	// User ID
    short	JDBC_CP_PASSWORD 	= 0x0003;	// Password
    short	JDBC_CP_DBUSERNAME	= 0x0004;	// Effective user
    short	JDBC_CP_GROUP_ID	= 0x0005;	// Group ID
    short	JDBC_CP_ROLE_ID		= 0x0006;	// Role ID
    short	JDBC_CP_DBPASSWORD 	= 0x0007;	// DBMS password
    short	JDBC_CP_TIMEOUT		= 0x0008;	// Timeout
    short	JDBC_CP_CONNECT_POOL	= 0x0009;	// Connection pooling
    short	JDBC_CP_AUTO_MODE	= 0x000A;	// Autocommit mode
    short	JDBC_CP_II_XID		= 0x000B;	// Ingres XID.
    short	JDBC_CP_XA_FRMT		= 0x000C;	// XA Format ID.
    short	JDBC_CP_XA_GTID		= 0x000D;	// XA Globat xact ID.
    short	JDBC_CP_XA_BQUAL	= 0x000E;	// XA Branch qualifier.

    /*
    ** The following connection properties are used
    ** locally and are not sent to the server.
    */
    short	JDBC_CP_SELECT_LOOP	= -1;
    short	JDBC_CP_CURSOR_MODE	= -2;

    IdMap	cpMap[] =
    {
	new IdMap( JDBC_CP_DATABASE,	"Database" ),
	new IdMap( JDBC_CP_USERNAME,	"Username" ),
	new IdMap( JDBC_CP_PASSWORD,	"Password" ),
 	new IdMap( JDBC_CP_DBUSERNAME,	"DBMS Username" ),
 	new IdMap( JDBC_CP_GROUP_ID,	"Group ID" ),
 	new IdMap( JDBC_CP_ROLE_ID,	"Role ID" ),
 	new IdMap( JDBC_CP_DBPASSWORD,	"DBMS Password" ),
	new IdMap( JDBC_CP_TIMEOUT,	"Timeout" ),
	new IdMap( JDBC_CP_CONNECT_POOL,"Connection Pool" ),
	new IdMap( JDBC_CP_AUTO_MODE,	"Autocommit Mode" ),
	new IdMap( JDBC_CP_II_XID,	"Distributed XID" ),
	new IdMap( JDBC_CP_SELECT_LOOP,	"Select Loops" ),
	new IdMap( JDBC_CP_CURSOR_MODE,	"Cursor Mode" ),
    };

    /*
    ** Connection parameter values.
    */
    byte	JDBC_CPV_POOL_OFF	= 0;	// Connect pool: off
    byte	JDBC_CPV_POOL_ON	= 1;	// Connect pool: on
    byte	JDBC_CPV_XACM_DBMS	= 0;	// Autocommit mode: DBMS
    byte	JDBC_CPV_XACM_SINGLE	= 1;	// Autocommit mode: single
    byte	JDBC_CPV_XACM_MULTI	= 2;	// Autocommit mode: multi

    /*
    ** Transaction operations.
    */
    short	JDBC_XACT_COMMIT    = 0x01;	// Commit transaction.
    short	JDBC_XACT_ROLLBACK  = 0x02;	// Rollback transaction.
    short	JDBC_XACT_AC_ENABLE = 0x03;	// Enable autocommit.
    short	JDBC_XACT_AC_DISABLE= 0x04;	// Disable autocommit.
    short	JDBC_XACT_BEGIN	    = 0x05;	// Start distributed.
    short	JDBC_XACT_PREPARE   = 0x06;	// Prepare distributed.

    /*
    ** Transaction parameters.
    */
    short	JDBC_XP_II_XID	    = 0x01;	// Ingres XID.
    short	JDBC_XP_XA_FRMT	    = 0x02;	// XA XID Format ID.
    short	JDBC_XP_XA_GTID	    = 0x03;	// XA XID Globat xact ID.
    short	JDBC_XP_XA_BQUAL    = 0x04;	// XA XID Branch qualifier.

    /*
    ** Request operations.
    */
    short	JDBC_REQ_CAPABILITY = 0x01;	// DBMS Capabilities.
    short	JDBC_REQ_PARAM_NAMES= 0x02;	// Procedure Parameter Names.
    short	JDBC_REQ_DBMSINFO   = 0x03;	// dbmsinfo().
    short	JDBC_REQ_2PC_XIDS   = 0x04;	// Two-phase Commit Xact Ids.

    /*
    ** Request parameters.
    */
    short	JDBC_RQP_SCHEMA_NAME= 0x01;	// Schema name.
    short	JDBC_RQP_PROC_NAME  = 0x02;	// Procedure name.
    short	JDBC_RQP_INFO_ID    = 0x03;	// dbmsinfo( INFO_ID ).
    short	JDBC_RQP_DB_NAME    = 0x04;	// Database name.

    /*
    ** Query operations.
    */
    short	JDBC_QRY_EXQ	    = 0x01;	// Execute query.
    short	JDBC_QRY_OCQ	    = 0x02;	// Open cursor for query.
    short	JDBC_QRY_PREP	    = 0x03;	// Prepare statement.
    short	JDBC_QRY_EXS	    = 0x04;	// Execute statement.
    short	JDBC_QRY_OCS	    = 0x05;	// Open cursor for statement.
    short	JDBC_QRY_EXP	    = 0x06;	// Execute procedure.
    short	JDBC_QRY_CDEL	    = 0x07;	// Cursor delete.
    short	JDBC_QRY_CUPD	    = 0x08;	// Cursor update.

    /*
    ** Query parameters.
    */
    short	JDBC_QP_QTXT	    = 0x01;	// Query text.
    short	JDBC_QP_CRSR_NAME   = 0x02;	// Cursor name.
    short	JDBC_QP_STMT_NAME   = 0x03;	// Statement name.
    short	JDBC_QP_SCHEMA_NAME = 0x04;	// Schema name.
    short	JDBC_QP_PROC_NAME   = 0x05;	// Procedure name.
    short	JDBC_QP_FLAGS	    = 0x06;	// Query flags.

    /*
    ** Query flags
    */
    short	JDBC_QF_READONLY    = 0x0001;	// READONLY cursor.

    /*
    ** Cursor operations.
    */
    short	JDBC_CUR_CLOSE	    = 0x01;	// Cursor close.
    short	JDBC_CUR_FETCH	    = 0x02;	// Cursor fetch.

    /*
    ** Cursor parameters
    */
    short	JDBC_CUR_STMT_ID    = 0x01;	// Cursor ID.
    short	JDBC_CUR_PRE_FETCH  = 0x02;	// Pre-fetch rows.

    /*
    ** Descriptor parameters.
    */
    byte	JDBC_DSC_NULL	    = 0x01;	// Nullable.
    byte	JDBC_DSC_PIN	    = 0x02;	// Procedure param: input.
    byte	JDBC_DSC_POUT	    = 0x04;	// Procedure param: output.
    byte	JDBC_DSC_GTT	    = 0x08;	// Proc param: global tmp table.

    /*
    ** Error parameters.
    */
    byte	JDBC_ET_MSG	    = 0x00;	// User message.
    byte	JDBC_ET_WRN	    = 0x01;	// Warning message.
    byte	JDBC_ET_ERR	    = 0x02;	// Error message.

    /*
    ** Result parameters.
    */
    short	JDBC_RP_ROWCOUNT    = 0x01;	// Row count.
    short	JDBC_RP_XACT_END    = 0x02;	// End of transaction.
    short	JDBC_RP_STMT_ID	    = 0x03;	// Statement ID.
    short	JDBC_RP_FETCH_LIMIT = 0x04;	// Fetch row limit.
    short	JDBC_RP_EOD	    = 0x05;	// End-of-data.
    short	JDBC_RP_PROC_RESULT = 0x06;	// Procedure return value.
    short	JDBC_RP_READ_ONLY   = 0x07;	// Read-only cursor.

} // interface MsgConst
