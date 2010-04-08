/*
** Copyright (c) 1999, 2010 Ingres Corporation All Rights Reserved.
*/

package com.ingres.gcf.dam;

/*
** Name: MsgConst.java
**
** Description:
**	Defines the global constants used in the Data Access Messaging
**	communications protocol Message Layer (DAM-ML).
**
**  Interfaces
**
**	MsgConst
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
**	27-Sep-2002 (wansh01)
**          Added connection parameter MSG_CP_LOGIN_TYPE, MSG_CPV_LOGIN_LOCAL, 
**	    MSG_CPV_LOGIN_REMOTE.
**	 1-Oct-02 (gordy)
**	    Moved to GCF dam package which implements the Data Access
**	    Messaging Network and Transport Layers protocols and builds
**	    Message Layer messages.  Changed JDBC references to MSG.
**	05-Nov-2002 (wansh01)
**          Added internal connection parameter value MSG_CPV_LOGIN_USER.
**	20-Dec-02 (gordy)
**	    Added protocol level 3, new header ID and autocommit state
**	    connection parameter.
**	15-Jan-03 (gordy)
**	    Added connection parameters for client info.
**	14-Feb-03 (gordy)
**	    Added savepoint support and table/object keys.
**	 7-Jul-03 (gordy)
**	    Added connection parameters for Ingres configuration settings.
**	20-Aug-03 (gordy)
**	    Added query and result flags.
**	 6-Oct-03 (gordy)
**	    Renamed MSG_DONE to MSG_RESULT.  Added query flags for fetches.
**	16-Jun-04 (gordy)
**	    Added Session Mask parameter.
**	10-May-05 (gordy)
**	    Added protocol level 4, transaction action abort.
**	21-Apr-06 (gordy)
**	    Added protocol level 5, message type INFO.
**	16-Jun-06 (gordy)
**	    ANSI Date/Time data type support.
**	28-Jun-06 (gordy)
**	    Output procedure parameter support.
**	30-Jun-06 (gordy)
**	    Describe Input support.
**	20-Jul-06 (gordy)
**	    Enhanced server XA support.
**	25-Aug-06 (gordy)
**	    Procedure parameter info.
**	15-Nov-06 (gordy)
**	    Support LOB Locators.
**	 6-Apr-07 (gordy)
**	    Support scrollable cursors.
**	23-Jul-07 (gordy)
**	    Added connection parameter for date alias.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	26-Oct-09 (gordy)
**	    Support Boolean data type.
**	25-Mar-10 (gordy)
**	    Support batch processing.
*/

import	com.ingres.gcf.util.IdMap;


/*
** Name: MsgConst
**
** Description:
**	Interface defining the global constants used in the Data
**	Access Messaging communications protocol Message Layer.
**
**  Protocol Levels
**
**	MSG_PROTO_1
**
**	    Base protocol level for Beta testing and initial release.
**
**	MSG_PROTO_2
**
**	    Messaging parameters:
**		MSG_P_DTMC
**
**	    Connection parameters: 
**		MSG_CP_AUTO_MODE, MSG_CP_II_XID, MSG_CP_XA_FRMT,
**		MSG_CP_XA_GTID, MSG_CP_XA_BQUAL
**
**	    Transaction operations and parameters:
**		MSG_XACT_BEGIN, MSG_XACT_PREPARE, MSG_XP_II_XID,
**		MSG_XP_XA_FRMT, MSG_XP_XA_GTID, MSG_XP_XA_BQUAL
**
**	    Query parameters:
**		MSG_QP_SCHEMA_NAME, MSG_QP_PROC_NAME
**
**	    Request operations and parameters:
**		MSG_REQ_PARAM_NAMES, MSG_RQP_SCHEMA_NAME, MSG_RQP_PROC_NAME
**		MSG_REQ_DBMSINFO, MSG_RQP_INFO_ID
**		MSG_REQ_2PC_XIDS, MSG_RQP_DB_NAME
**
**	    Result parameters: 
**		MSG_RP_EOD, MSG_RP_PROC_RESULT, MSG_RP_READ_ONLY
**
**	MSG_PROTO_3
**
**	    Messaging header:
**		MSG_HDR_ID_2
**
**	    Connection parameters:
**		MSG_CP_LOGIN_TYPE, MSG_CP_AUTO_STATE, MSG_CP_CLIENT_USER,
**		MSG_CP_CLIENT_HOST, MSG_CP_CLIENT_ADDR, MSG_CP_TIMEZONE,
**		MSG_CP_DECIMAL, MSG_CP_DATE_FRMT, MSG_CP_MNY_FRMT,
**		MSG_CP_MNY_PREC.
**
**	    Transaction operations and parameters:
**		MSG_XACT_SAVEPOINT, MSG_XP_SAVEPOINT.
**
**	    Query parameter flags:
**		MSG_QF_AUTO_CLOSE, MSG_QF_FETCH_FIRST, MSG_QF_FETCH_ALL.
**
**	    Result parameters and flags:
**		MSG_RP_TBLKEY, MSG_RP_OBJKEY, MSG_RP_FLAGS (MSG_RF_XACT_END,
**		MSG_RF_READONLY, MSG_RF_EOD, MSG_RF_CLOSED).
**
**	MSG_PROTO_4
**
**	    Transaction operations:
**		MSG_XACT_ABORT.
**	    
**	MSG_PROTO_5
**
**	    Message type: 
**		MSG_INFO
**
**	    Transaction operation, parameter, and flags:
**		MSG_XACT_END, MSG_XP_XA_FLAGS, MSG_XA_JOIN,
**		MSG_XA_FAIL, MSG_XA_1PC.
**
**	    Query operation and flags:
**		MSG_QRY_DESC, MSG_QF_DESC_OUTPUT.
**
**	    Request operation:
**		MSG_REQ_PARAM_INFO.
**
**	    Data Type:
**		SQL_INTERVAL (ANSI Date/Time data types).
**
**	    Descriptor flag:
**		MSG_DSC_OUT:
**
**	    Error flag:
**		MSG_ET_XA
**
**	MSG_PROTO_6
**
**	    Connection parameters:
**		MSG_CP_DATE_ALIAS.
**
**	    Query flag:
**		MSG_QF_LOCATORS.
**		MSG_QF_SCROLL.
**
**	    Cursor parameters and values:
**		MSG_CUR_POS_ANCHOR, MSG_CUR_POS_OFFSET,
**		MSG_CPV_POS_BEG, MSG_CPV_POS_CUR, MSG_CPV_POS_END.
**
**	    Data Type:
**		LOB Locators.
**
**	    Result parameters, flags, and values:
**		MSG_RP_ROW_STATUS, MSG_RP_ROW_POS, MSG_RF_SCROLL,
**		MSG_RPV_ROW_BEFORE, MSG_RPV_ROW_FIRST, 
**		MSG_RPV_ROW_LAST, MSG_RPV_ROW_AFTER, 
**		MSG_RPV_ROW_INSERTED, MSG_RPV_ROW_UPDATED,
**		MSG_RPV_ROW_DELETED.
**
**	MSG_PROTO_7
**
**	    Message Header:
**		MSG_HDR_EOB.
**		
**	    Message Type:
**		MSG_BATCH.
**
**	    Data Type:
**		SQL_BOOLEAN.
**
**  Constants
**
**	MSG_P_PROTO		Init parameter: protocol level.
**	MSG_P_DTMC		Init parameter: Dist Transact Mgmt Connect.
**	MSG_P_MASK		Init parameter: session mask.
**	MSG_PROTO_1		Init param value: protocol level 1.
**	MSG_PROTO_2		Init param value: protocol level 2.
**	MSG_PROTO_3		Init param value: protocol level 3.
**	MSG_PROTO_4		Init param value: protocol level 4.
**	MSG_PROTO_5		Init param value: protocol level 5.
**	MSG_PROTO_6		Init param value: protocol level 6.
**	MSG_PROTO_7		Init param value: protocol level 7.
**	MSG_PROTO_DFLT		Supported message protocol level.
**
**	MSG_HDR_ID_1		Message Header ID (upto proto level 2).
**	MSG_HDR_ID_2		Message Header ID (beginning at proto level 3).
**	MSG_HDR_EOD		Header Flag: end-of-data.
**	MSG_HDR_EOG		Header Flag: end-of-group.
**	MSG_HDR_EOB		Header Falg: end-of-batch.
**
**	MSG_CONNECT		Message Type: connect request.
**	MSG_DISCONN		Message Type: disconnect request.
**	MSG_XACT		Message Type: transaction request.
**	MSG_QUERY		Message Type: query request.
**	MSG_CURSOR		Message Type: cursor request.
**	MSG_DESC		Message Type: data descriptor.
**	MSG_DATA		Message Type: data.
**	MSG_ERROR		Message Type: error message.
**	MSG_RESULT		Message Type: result message.
**	MSG_REQUEST		Message Type: pre-defined request.
**	MSG_INFO		Message Type: information.
**	MSG_BATCH		Message Type: batch request.
**
**	MSG_STR_CONNECT		Message Type Names
**	MSG_STR_DISCONN
**	MSG_STR_XACT
**	MSG_STR_QUERY
**	MSG_STR_CURSOR
**	MSG_STR_DESC
**	MSG_STR_DATA
**	MSG_STR_ERROR
**	MSG_STR_RESULT
**	MSG_STR_REQUEST
**	MSG_STR_INFO
**	MSG_STR_BATCH
**
**	MSG_CP_DATABASE		Connect parameter: database specification
**	MSG_CP_USERNAME		Connect parameter: user ID.
**	MSG_CP_PASSWORD		Connect parameter: password.
**	MSG_CP_DBUSERNAME	Connect parameter: effective user ID
**	MSG_CP_GROUP_ID		Connect parameter: group ID
**	MSG_CP_ROLE_ID		Connect parameter: role ID
**	MSG_CP_DBPASSWORD	Connect parameter: DBMS password.
**	MSG_CP_TIMEOUT		Connect parameter: timeout.
**	MSG_CP_CONNECT_POOL	Connect parameter: connection pool.
**	MSG_CP_AUTO_MODE	Connect parameter: autocommit mode.
**	MSG_CP_II_XID		Connect parameter: Ingres XID.
**	MSG_CP_XA_FRMT		Connect parameter: XA Format ID.
**	MSG_CP_XA_GTID		Connect parameter: XA Globat Trans ID.
**	MSG_CP_XA_BQUAL		Connect parameter: XA Branch qualifier.
**	MSG_CP_LOGIN_TYPE 	Connect parameter: login type (local/remote).
**	MSG_CP_AUTO_STATE	Connect parameter: autocommit state.
**	MSG_CP_CLIENT_USER	Connect parameter: Local user ID.
**	MSG_CP_CLIENT_HOST	Connect parameter: Local host name.
**	MSG_CP_CLIENT_ADDR	Connect parameter: Local host address.
**	MSG_CP_TIMEZONE		Connect parameter: Ingres timezone.
**	MSG_CP_DECIMAL		Connect parameter: Decimal character.
**	MSG_CP_DATE_FRMT	Connect parameter: Ingres date format.
**	MSG_CP_MNY_FRMT		Connect parameter: Ingres money format.
**	MSG_CP_MNY_PREC		Connect parameter: Ingres money precision.
**	MSG_CP_DATE_ALIAS	Connect parameter: Ingres date alias.
**
**	MSG_CPV_AUTO_OFF	Connect param value: autocommit disabled.
**	MSG_CPV_AUTO_ON		Connect param value: autocommit enabled.
**	MSG_CPV_LOGIN_LOCAL	Connect param value: login local.
**	MSG_CPV_LOGIN_REMOTE	Connect param value: login remote.
**	MSG_CPV_LOGIN_USER  	Connect param value: login local userID.
**	MSG_CPV_POOL_OFF	Connect param value: connect pool off.
**	MSG_CPV_POOL_ON		Connect param value: connect pool on.
**	MSG_CPV_XACM_DBMS	Connect param value: auto mode DBMS.
**	MSG_CPV_XACM_SINGLE	Connect param value: auto mode single.
**	MSG_CPV_XACM_MULTI	Connect param value: auto mode multi.
**
**	MSG_XACT_COMMIT		Transaction action: commit transaction.
**	MSG_XACT_ROLLBACK	Transaction action: rollback transaction.
**	MSG_XACT_AC_ENABLE	Transaction action: enable autocommit.
**	MSG_XACT_AC_DISABLE	Transaction action: disable autocommit.
**	MSG_XACT_BEGIN		Transaction action: begin distributed.
**	MSG_XACT_PREPARE	Transaction action: prepare distributed.
**	MSG_XACT_SAVEPOINT	Transaction action: savepoint.
**	MSG_XACT_ABORT		Transaction action: abort distributed.
**	MSG_XACT_END		Transaction action: end distributed.
**	MSG_XP_II_XID		Transaction param value: Ingres XID.
**	MSG_XP_XA_FRMT		Transaction param value: XA Format ID.
**	MSG_XP_XA_GTID		Transaction param value: XA Global Trans ID.
**	MSG_XP_XA_BQUAL		Transaction param value: XA Branch Qualifier.
**	MSG_XP_SAVEPOINT	Transaction param value: Savepoint Name.
**	MSG_XP_XA_FLAGS		Transaction param value: XA Flags.
**	MSG_XA_JOIN		Transaction XA flags: Join existing xact.
**	MSG_XA_FAIL		Transaction XA flags: Fail transaction.
**	MSG_XA_1PC		Transaction XA flags: One phase commit.
**
**	MSG_REQ_CAPABILITY	Request type: DBMS capabilities.
**	MSG_REQ_PARAM_NAMES	Request type: Parameter names.
**	MSG_REQ_DBMSINFO	Request type: dbmsinfo().
**	MSG_REQ_2PC_XIDS	Request type: Two-phase Commit Xact Ids.
**	MSG_REQ_PARAM_INFO	Request type: Parameter Info.
**
**	MSG_RQP_SCHEMA_NAME	Request parameter: Schema name.
**	MSG_RQP_PROC_NAME	Request parameter: Procedure name.
**	MSG_RQP_INFO_ID		Request parameter: dbmsinfo() ID.
**	MSG_RQP_DB_NAME		Request parameter: Database name.
**
**	MSG_QRY_EXQ		Query type: Execute query.
**	MSG_QRY_OCQ		Query type: Open cursor for query.
**	MSG_QRY_PREP		Query type: Prepare statement.
**	MSG_QRY_EXS		Query type: Execute statement.
**	MSG_QRY_OCS		Query type: Open cursor for statement.
**	MSG_QRY_EXP		Query type: Execute procedure.
**	MSG_QRY_CDEL		Query type: Cursor delete.
**	MSG_QRY_CUPD		Query type: Cursor update.
**	MSG_QRY_DESC		Qerry type: Describe statement.
**
**	MSG_QP_QTXT		Query parameter: Query text.
**	MSG_QP_CRSR_NAME	Query parameter: Cursor name.
**	MSG_QP_STMT_NAME	Query parameter: Statement name.
**	MSG_QP_SCHEMA_NAME	Query parameter: Schema name.
**	MSG_QP_PROC_NAME	Query parameter: Procedure name.
**	MSG_QP_FLAGS		Query parameter: Flags.
**
**	MSG_QF_READONLY		Query flag: Readonly query.
**	MSG_QF_AUTO_CLOSE	Query flag: Close statement at EOD.
**	MSG_QF_FETCH_FIRST	Query flag: Fetch first rows.
**	MSG_QF_FETCH_ALL	Query flag: Fetch all rows.
**	MSG_QF_DESC_OUTPUT	Query flag: Describe output.
**	MSG_QF_LOCATORS		Query flag: LOB Locators.
**	MSG_QF_SCROLL		Query flag: Scrollable cursor.
**
**	MSG_BAT_EXQ		Batch type: Execute query.
**	MSG_BAT_EXS		Batch type: Execute statement.
**	MSG_BAT_EXP		Batch type: Execute procedure.
**
**	MSG_BP_QTXT		Batch parameter: Query text.
**	MSG_BP_STMT_NAME	Batch parameter: Statement name.
**	MSG_BP_SCHEMA_NAME	Batch parameter: Schema name.
**	MSG_BP_PROC_NAME	Batch parameter: Procedure name.
**	MSG_BP_FLAGS		Batch parameter: Flags.
**
**	MSG_BF_REPEAT		Batch flag: Repeat statement.
**
**	MSG_CUR_CLOSE		Cursor action: close.
**	MSG_CUR_FETCH		Cursor action: fetch.
**
**	MSG_CUR_STMT_ID		Cursor parameter: Cursor ID.
**	MSG_CUR_PRE_FETCH	Cursor parameter: pre-fetch rows.
**	MSG_CUR_POS_ANCHOR	Cursor parameter: position anchor.
**	MSG_CUR_POS_OFFSET	Cursor parameter: position offset.
**
**	MSG_CPV_POS_BEG		Cursor param value: BEGIN anchor.
**	MSG_CPV_POS_CUR		Cursor param value: CURRENT anchor.
**	MSG_CPV_POS_END		Cursor param value: END anchor.
**
**	MSG_DSC_NULL		Nullable.
**	MSG_DSC_PIN		Procedure param: IN.
**	MSG_DSC_POUT		Procedure param: OUT.
**	MSG_DSC_PIO		Procedure param: INOUT.
**	MSG_DSC_GTT		Procedure param: global temp table.
**
**	SQL_INTERVAL		Extended type Interval.
**
**	MSG_ET_MSG		Error type: user message.
**	MSG_ET_WRN		Error type: warning.
**	MSG_ET_ERR		Error type: error.
**	MSG_ET_XA		Error type: XA error.
**
**	MSG_RP_ROWCOUNT		Result parameter: Row count.
**	MSG_RP_XACT_END		Result parameter: End-of-transaction.
**	MSG_RP_STMT_ID		Result parameter: Statement or Cursor ID.
**	MSG_RP_EOD		Result parameter: End-of-data.
**	MSG_RP_PROC_RESULT	Result parameter: Procedure result.
**	MSG_RP_READ_ONLY	Result parameter: Read-only cursor.
**	MSG_RP_TBLKEY		Result parameter: Table key.
**	MSG_RP_OBJKEY		Result parameter: Object key.
**	MSG_RP_FLAGS		Result parameter: Result flags.
**	MSG_RP_ROW_STATUS	Result parameter: Row status.
**	MSG_RP_ROW_POS		Result parameter: Row position.
**
**	MSG_RF_XACT_END		Result flag: End-of-transaction.
**	MSG_RF_READ_ONLY	Result flag: Read-only cursor.
**	MSG_RF_EOD		Result flag: End-of-data.
**	MSG_RF_CLOSED		Result flag: Statement closed.
**	MSG_RF_SCROLL		Result flag: Scrollable cursor.
**
**	MSG_RPV_TBLKEY_LEN	Result param value: Table key length.
**	MSG_RPV_OBJKEY_LEN	Result param value: Object key length.
**	MSG_RPV_ROW_BEFORE	Result param value: Row before first.
**	MSG_RPV_ROW_FIRST	Result param value: Row is first.
**	MSG_RPV_ROW_LAST	Result param value: Row is last.
**	MSG_RPV_ROW_AFTER	Result param value: Row after last.
**	MSG_RPV_ROW_INSERTED	Result param value: Row inserted.
**	MSG_RPV_ROW_UPDATED	Result param value: Row updated.
**	MSG_RPV_ROW_DELETED	Result param value: Row deleted.
**
**	MSG_IP_TRACE		Info parameter: Trace text.
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
**	27-Sep-2002 (wansh01)
**          Added connection parameter MSG_CP_LOGIN_TYPE, MSG_CPV_LOGIN_LOCAL
**          MSG_CPV_LOGIN_REMOTE.
**	 1-Oct-02 (gordy)
**	    Changed JDBC references to MSG.
**	05-Nov-2002 (wansh01)
**          Added internal connection parameter value MSG_CPV_LOGIN_USER.
**	20-Dec-02 (gordy)
**	    Added protocol level 3, new header ID, connection parameter for
**	    initial autocommit state, MSG_CP_AUTO_STATE, and parameter values:
**	    MSG_CPV_AUTO_OFF, MSG_CPV_AUTO_ON.  Removed internal connection
**	    parameters and ID map since not used in the messaging system.
**	15-Jan-03 (gordy)
**	    Added connection parameters MSG_CP_CLIENT_{USER,HOST,ADDR}.
**	14-Feb-03 (gordy)
**	    Added savepoint support: MSG_XACT_SAVEPOINT, MSG_XP_SAVEPOINT.
**	    Added table/object keys: MSG_RP_TBLKEY, MSG_RP_OBJKEY,
**	    MSG_RVP_TBLKEY_LEN, MSG_RVP_OBJKEY_LEN.
**	 7-Jul-03 (gordy)
**	    Added connection parameters MSG_CP_TIMEZONE, MSG_CP_DECIMAL,
**	    MSG_CP_DATE_FRMT, MSG_CP_MNY_FRMT, MSG_CP_MNY_PREC.
**	20-Aug-03 (gordy)
**	    Added MSG_RP_FLAGS and flags MSG_RF_XACT_END, MSG_RF_READ_ONLY,
**	    MSG_RF_EOD, MSG_RF_CLOSED.  Added query flag MSG_QF_AUTO_CLOSE.
**	 6-Oct-03 (gordy)
**	    Rename MSG_DONE to MSG_RESULT.  Added query flags
**	    MSG_QF_FETCH_FIRST and MSG_QF_FETCH_ALL.
**	16-Jun-04 (gordy)
**	    Added Session Mask parameter.
**	10-May-05 (gordy)
**	    Added protocol level 4, transaction operation MSG_XACT_ABORT.
**	21-Apr-06 (gordy)
**	    Added protocol level 5, message type MSG_INFO and info
**	    parameter MSG_IP_TRACE.
**	16-Jun-06 (gordy)
**	    Added extended data type SQL_INTERVAL (most types defined by
**	    java.sql.Types).
**	28-Jun-06 (gordy)
**	    Descriptor parameter MSG_DSC_OUT.
**	30-Jun-06 (gordy)
**	    Query operation MSG_QRY_DESC and flag MSG_QF_DESC_OUTPUT.
**	20-Jul-06 (gordy)
**	    Enhanced server XA support: MSG_XACT_END, MSG_XP_XA_FLAGS,
**	    MSG_XA_JOIN, MSG_XA_FAIL, MSG_XA_1PC, MSG_ET_XA.
**	25-Aug-06 (gordy)
**	    Request operation MSG_REQ_PARAM_INFO.
**	15-Nov-06 (gordy)
**	    Added protocol level 6, query flag MSG_QF_LOCATORS.
**	 6-Apr-07 (gordy)
**	    Added scrollable cursor parameters/flags.
**	23-Jul-07 (gordy)
**	    Added connection parameter MSG_CP_DATE_ALIAS.
**	 9-Jan-09 (gordy)
**	    Cleanup related to problems reported by the findbugs utility.
**	    Removed publicly mutable ID maps and replaced with string
**	    constants.  Users will need to provide their own maps.
**	26-Oct-09 (gordy)
**	    Added protocol level 7.
**	25-Mar-10 (gordy)
**	    Added end-of-batch header flag and batch message with
**	    associated parameters.
*/

public interface
MsgConst
{

    byte	MSG_P_PROTO		= 0x01;		// Parameter: protocol
    byte	MSG_P_DTMC		= 0x02;		// Parameter: DMTC
    byte	MSG_P_MASK		= 0x03;		// Parameter: sess mask
    
    byte	MSG_PROTO_1		= 0x01;		// Protocol level 1
    byte	MSG_PROTO_2		= 0x02;		// Protocol level 2
    byte	MSG_PROTO_3		= 0x03;		// Protocol level 3
    byte	MSG_PROTO_4		= 0x04;		// Protocol level 4
    byte	MSG_PROTO_5		= 0x05;		// Protocol level 5
    byte	MSG_PROTO_6		= 0x06;		// Protocol level 6
    byte	MSG_PROTO_7		= 0x07;		// Protocol level 7
    byte	MSG_PROTO_DFLT		= MSG_PROTO_7;

    int		MSG_HDR_ID_1		= 0x4342444A;	// Message ID: "JDBC"
    int		MSG_HDR_ID_2		= 0x4C4D4D44;	// Message ID: "DMML"
    byte	MSG_HDR_EOD		= 0x01;		// end-of-data
    byte	MSG_HDR_EOG		= 0x02;		// end-of-group
    byte	MSG_HDR_EOB		= 0x04;		// end-of-batch

    /*
    ** Message types.
    */
    byte	MSG_CONNECT		= 0x01;		// Connect
    byte	MSG_DISCONN		= 0x02;		// Disconnect
    byte	MSG_XACT		= 0x03;		// Transaction
    byte	MSG_QUERY		= 0x04;		// Query
    byte	MSG_CURSOR		= 0x05;		// Cursor
    byte	MSG_DESC		= 0x06;		// Descriptor
    byte	MSG_DATA		= 0x07;		// Data
    byte	MSG_ERROR		= 0x08;		// Error
    byte	MSG_RESULT		= 0x09;		// Result
    byte	MSG_REQUEST		= 0x0A;		// Request
    byte	MSG_INFO		= 0x0B;		// Info
    byte	MSG_BATCH		= 0x0C;		// Batch

    String	MSG_STR_CONNECT		= "CONNECT";
    String	MSG_STR_DISCONN		= "DISCONN";
    String	MSG_STR_XACT		= "XACT";
    String	MSG_STR_QUERY		= "QUERY";
    String	MSG_STR_CURSOR		= "CURSOR";
    String	MSG_STR_DESC		= "DESC";
    String	MSG_STR_DATA		= "DATA";
    String	MSG_STR_ERROR		= "ERROR";
    String	MSG_STR_RESULT		= "RESULT";
    String	MSG_STR_REQUEST		= "REQUEST";
    String	MSG_STR_INFO		= "INFO";
    String	MSG_STR_BATCH		= "BATCH";

    /*
    ** Connection parameters.
    */
    short	MSG_CP_DATABASE 	= 0x0001;	// Database name
    short	MSG_CP_USERNAME 	= 0x0002;	// User ID
    short	MSG_CP_PASSWORD 	= 0x0003;	// Password
    short	MSG_CP_DBUSERNAME	= 0x0004;	// Effective user
    short	MSG_CP_GROUP_ID		= 0x0005;	// Group ID
    short	MSG_CP_ROLE_ID		= 0x0006;	// Role ID
    short	MSG_CP_DBPASSWORD 	= 0x0007;	// DBMS password
    short	MSG_CP_TIMEOUT		= 0x0008;	// Timeout
    short	MSG_CP_CONNECT_POOL	= 0x0009;	// Connection pooling
    short	MSG_CP_AUTO_MODE	= 0x000A;	// Autocommit mode
    short	MSG_CP_II_XID		= 0x000B;	// Ingres XID.
    short	MSG_CP_XA_FRMT		= 0x000C;	// XA Format ID.
    short	MSG_CP_XA_GTID		= 0x000D;	// XA Globat xact ID.
    short	MSG_CP_XA_BQUAL		= 0x000E;	// XA Branch qualifier.
    short	MSG_CP_LOGIN_TYPE	= 0x000F;	// login type.
    short	MSG_CP_AUTO_STATE	= 0x0010;	// Autocommit state.
    short	MSG_CP_CLIENT_USER	= 0x0011;	// Local user ID.
    short	MSG_CP_CLIENT_HOST	= 0x0012;	// Local host name.
    short	MSG_CP_CLIENT_ADDR	= 0x0013;	// Local host address.
    short	MSG_CP_TIMEZONE		= 0x0014;	// Ingres Timezone.
    short	MSG_CP_DECIMAL		= 0x0015;	// Decimal Character.
    short	MSG_CP_DATE_FRMT	= 0x0016;	// Ingres Date Format.
    short	MSG_CP_MNY_FRMT		= 0x0017;	// Ingres Money Format.
    short	MSG_CP_MNY_PREC		= 0x0018;	// Ingres Money prec.
    short	MSG_CP_DATE_ALIAS	= 0x0019;	// Ingres Date Alias.

    /*
    ** Connection parameter values.
    */
    byte	MSG_CPV_AUTO_OFF	= 0;	// Autocommit disabled.
    byte	MSG_CPV_AUTO_ON		= 1;	// Autocommit enabled.
    byte	MSG_CPV_LOGIN_LOCAL	= 0;	// Login local
    byte	MSG_CPV_LOGIN_REMOTE	= 1;	// Login remote
    byte	MSG_CPV_LOGIN_USER  	= 2;	// Login local user
    byte	MSG_CPV_POOL_OFF	= 0;	// Connect pool: off
    byte	MSG_CPV_POOL_ON		= 1;	// Connect pool: on
    byte	MSG_CPV_XACM_DBMS	= 0;	// Autocommit mode: DBMS
    byte	MSG_CPV_XACM_SINGLE	= 1;	// Autocommit mode: single
    byte	MSG_CPV_XACM_MULTI	= 2;	// Autocommit mode: multi

    /*
    ** Transaction operations.
    */
    short	MSG_XACT_COMMIT		= 0x01;	// Commit transaction.
    short	MSG_XACT_ROLLBACK	= 0x02;	// Rollback transaction.
    short	MSG_XACT_AC_ENABLE	= 0x03;	// Enable autocommit.
    short	MSG_XACT_AC_DISABLE	= 0x04;	// Disable autocommit.
    short	MSG_XACT_BEGIN		= 0x05;	// Start distributed.
    short	MSG_XACT_PREPARE	= 0x06;	// Prepare distributed.
    short	MSG_XACT_SAVEPOINT	= 0x07;	// Create savepoint.
    short	MSG_XACT_ABORT		= 0x08;	// Abort distributed.
    short	MSG_XACT_END		= 0x09; // End distributed.

    /*
    ** Transaction parameters.
    */
    short	MSG_XP_II_XID		= 0x01;	// Ingres XID.
    short	MSG_XP_XA_FRMT		= 0x02;	// XA XID Format ID.
    short	MSG_XP_XA_GTID		= 0x03;	// XA XID Globat xact ID.
    short	MSG_XP_XA_BQUAL		= 0x04;	// XA XID Branch qualifier.
    short	MSG_XP_SAVEPOINT	= 0x05;	// Savepoint name.
    short	MSG_XP_XA_FLAGS		= 0x06;	// XA Flags.

    /*
    ** XA Flags.
    */
    int		MSG_XA_JOIN		= 0x00200000;	// Join existing xact.
    int		MSG_XA_FAIL		= 0x20000000;	// Fail transaction.
    int		MSG_XA_1PC		= 0x40000000;	// One phase commit.

    /*
    ** Request operations.
    */
    short	MSG_REQ_CAPABILITY	= 0x01;	// DBMS Capabilities.
    short	MSG_REQ_PARAM_NAMES	= 0x02;	// Procedure Parameter Names.
    short	MSG_REQ_DBMSINFO	= 0x03;	// dbmsinfo().
    short	MSG_REQ_2PC_XIDS	= 0x04;	// Two-phase Commit Xact Ids.
    short	MSG_REQ_PARAM_INFO	= 0x05;	// Procedure Parameter Info.

    /*
    ** Request parameters.
    */
    short	MSG_RQP_SCHEMA_NAME	= 0x01;	// Schema name.
    short	MSG_RQP_PROC_NAME	= 0x02;	// Procedure name.
    short	MSG_RQP_INFO_ID		= 0x03;	// dbmsinfo( INFO_ID ).
    short	MSG_RQP_DB_NAME		= 0x04;	// Database name.

    /*
    ** Query operations.
    */
    short	MSG_QRY_EXQ		= 0x01;	// Execute query.
    short	MSG_QRY_OCQ		= 0x02;	// Open cursor for query.
    short	MSG_QRY_PREP		= 0x03;	// Prepare statement.
    short	MSG_QRY_EXS		= 0x04;	// Execute statement.
    short	MSG_QRY_OCS		= 0x05;	// Open cursor for statement.
    short	MSG_QRY_EXP		= 0x06;	// Execute procedure.
    short	MSG_QRY_CDEL		= 0x07;	// Cursor delete.
    short	MSG_QRY_CUPD		= 0x08;	// Cursor update.
    short	MSG_QRY_DESC		= 0x09;	// Describe statement.

    /*
    ** Query parameters.
    */
    short	MSG_QP_QTXT		= 0x01;	// Query text.
    short	MSG_QP_CRSR_NAME	= 0x02;	// Cursor name.
    short	MSG_QP_STMT_NAME	= 0x03;	// Statement name.
    short	MSG_QP_SCHEMA_NAME	= 0x04;	// Schema name.
    short	MSG_QP_PROC_NAME	= 0x05;	// Procedure name.
    short	MSG_QP_FLAGS		= 0x06;	// Query flags.

    /*
    ** Query flags
    */
    short	MSG_QF_READONLY		= 0x0001;	// READONLY cursor.
    short	MSG_QF_AUTO_CLOSE	= 0x0002;	// Close statement.
    short	MSG_QF_FETCH_FIRST	= 0x0004;	// Fetch first rows.
    short	MSG_QF_FETCH_ALL	= 0x0008;	// Fetch all rows.
    short	MSG_QF_DESC_OUTPUT	= 0x0010;	// Describe Output.
    short	MSG_QF_LOCATORS		= 0x0020;	// LOB Locators.
    short	MSG_QF_SCROLL		= 0x0040;	// Scrollable cursor.

    /*
    ** Batch operations.
    */
    short	MSG_BAT_EXQ		= 0x01;	// Execute query.
    short	MSG_BAT_EXS		= 0x04;	// Execute statement.
    short	MSG_BAT_EXP		= 0x06;	// Execute procedure.

    /*
    ** Batch parameters.
    */
    short	MSG_BP_QTXT		= 0x01;	// Query text.
    short	MSG_BP_STMT_NAME	= 0x03;	// Statement name.
    short	MSG_BP_SCHEMA_NAME	= 0x04;	// Schema name.
    short	MSG_BP_PROC_NAME	= 0x05;	// Procedure name.
    short	MSG_BP_FLAGS		= 0x06;	// Query flags.

    /*
    ** Batch flags
    */
    short	MSG_BF_REPEAT		= 0x0080;	// Repeat statement.

    /*
    ** Cursor operations.
    */
    short	MSG_CUR_CLOSE		= 0x01;	// Cursor close.
    short	MSG_CUR_FETCH		= 0x02;	// Cursor fetch.

    /*
    ** Cursor parameters
    */
    short	MSG_CUR_STMT_ID		= 0x01;	// Cursor ID.
    short	MSG_CUR_PRE_FETCH	= 0x02;	// Pre-fetch rows.
    short	MSG_CUR_POS_ANCHOR	= 0x03;	// Position anchor.
    short	MSG_CUR_POS_OFFSET	= 0x04;	// Position offset.

    /*
    ** Cursor parameter values:
    */
    short	MSG_POS_BEG		= 0x01;	// Begin position.
    short	MSG_POS_END		= 0x02;	// End position.
    short	MSG_POS_CUR		= 0x03;	// Current position.

    /*
    ** Descriptor parameters.
    */
    byte	MSG_DSC_NULL		= 0x01;	// Nullable.
    byte	MSG_DSC_PIN		= 0x02;	// Procedure param: IN.
    byte	MSG_DSC_PIO		= 0x04;	// Procedure param: INOUT.
    byte	MSG_DSC_GTT		= 0x08;	// Proc param: global tmp table.
    byte	MSG_DSC_POUT		= 0x10;	// Procedure param: OUT.

    /*
    ** Extended Data Types
    */
    int		SQL_INTERVAL		= 10;	// Interval.

    /*
    ** Error parameters.
    */
    byte	MSG_ET_MSG		= 0x00;	// User message.
    byte	MSG_ET_WRN		= 0x01;	// Warning message.
    byte	MSG_ET_ERR		= 0x02;	// Error message.
    byte	MSG_ET_XA		= 0x03;	// XA Error message.

    /*
    ** Result parameters.
    */
    short	MSG_RP_ROWCOUNT		= 0x01;	// Row count.
    short	MSG_RP_XACT_END		= 0x02;	// -- deprecated (see flags) --
    short	MSG_RP_STMT_ID		= 0x03;	// Statement ID.
    short	MSG_RP_FETCH_LIMIT	= 0x04;	// Fetch row limit.
    short	MSG_RP_EOD		= 0x05;	// -- deprecated (see flags) --
    short	MSG_RP_PROC_RESULT	= 0x06;	// Procedure return value.
    short	MSG_RP_READ_ONLY	= 0x07;	// -- deprecated (see flags) --
    short	MSG_RP_TBLKEY		= 0x08;	// Table key.
    short	MSG_RP_OBJKEY		= 0x09; // Object key.
    short	MSG_RP_FLAGS		= 0x0A; // Result flags.
    short	MSG_RP_ROW_STATUS	= 0x0B;	// Row status.
    short	MSG_RP_ROW_POS		= 0x0C;	// Row position.
    
    byte	MSG_RF_XACT_END		= 0x01;	// End-of-Transaction.
    byte	MSG_RF_READ_ONLY	= 0x02;	// READONLY Cursor.
    byte	MSG_RF_EOD		= 0x04;	// End-of-Data
    byte	MSG_RF_CLOSED		= 0x08;	// Statement closed.
    byte	MSG_RF_SCROLL		= 0x10;	// Scrollable cursor.

    byte	MSG_RPV_TBLKEY_LEN	= 8;	// Table key value length.
    byte	MSG_RPV_OBJKEY_LEN	= 16;	// Object key value length.
    byte	MSG_RPV_ROW_BEFORE	= 0x01;	// Row before first.
    byte	MSG_RPV_ROW_FIRST	= 0x02;	// Row is first.
    byte	MSG_RPV_ROW_LAST	= 0x04;	// Row is last.
    byte	MSG_RPV_ROW_AFTER	= 0x08;	// Row after last.
    byte	MSG_RPV_ROW_INSERTED	= 0x10;	// Row inserted.
    byte	MSG_RPV_ROW_UPDATED	= 0x20;	// Row updated.
    byte	MSG_RPV_ROW_DELETED	= 0x40;	// Row deleted.

    byte	MSG_IP_TRACE		= 0x01; // Trace text.


} // interface MsgConst
