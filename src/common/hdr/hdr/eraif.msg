/*
** Copyright (c) 2004, 2010 Ingres Corporation
*/

/*
** Name: erapi.msg
**
** Description:
**	Message file for Facility code _AP
**
** History:
**	23-Jan-95 (gordy)
**	    Created.
**	13-Feb-95 (gordy)
**	    Added E_AP0014_INVALID_REPEAT_ID and E_AP0015_INVALID_TRANS_STMT.
**	16-Feb-95 (gordy)
**	    Added E_AP000D_PARAMETERS_REQUIRED.
**	13-Jun-95 (gordy)
**	    Added E_AP0016_ROW_DISCARDED and E_AP0017_SEGMENT_DISCARDED.
**	15-Sep-98 (rajus01)
**	    Added E_AP0018_INVALID_DISCONNECT.
**	22-Oct-04 (gordy)
**	    Added specific descriptor errors AP0020, AP0021, AP0024,
**	    AP0025, AP0028, AP0029, AP002A.
**	31-May-06 (gordy)
**	    Added AP002B for ANSI date/time datatypes.
**	11-Jul-06 (gordy)
**	    Added AP0022 for XA operations.
**	24-Oct-06 (gordy)
**	    Added AP002C for Blob/Clob locator datatypes.
**	 6-Mar-09 (gordy)
**	    Added AP0023_INVALID_NULL_DATA for non-nullable data.
**	26-Oct-09 (gordy)
**	    Added AP002D for Boolean data type.
**	25-Mar-10 (gordy)
**	    Added E_AP001F_BATCH_UNSUPPORTED.
**	30-Aug-10 (thich01)
**	    Added E_AP002E_LVL7_DATA_TYPE for spatial types.
*/

#define	_AP_CLASS 201

E_AP0000_OK
"Success."

E_AP0001_CONNECTION_ABORTED
"The connection to the server has been aborted."

E_AP0002_TRANSACTION_ABORTED
"The current transaction has been aborted."

E_AP0003_ACTIVE_TRANSACTIONS
"The requested operation cannot be performed with active transactions."

E_AP0004_ACTIVE_QUERIES
"The requested operation cannot be performed with active queries."

E_AP0005_ACTIVE_EVENTS
"The requested operation cannot be performed with active event processing"

E_AP0006_INVALID_SEQUENCE
"API function cannot be called in the current state."

E_AP0007_INCOMPLETE_QUERY
"Query cannot be closed in the current state, it may only be cancelled."

E_AP0008_QUERY_DONE
"Query has completed or is completing and may not be cancelled."

E_AP0009_QUERY_CANCELLED
"Query cancelled."

E_AP000A_QUERY_INTERRUPTED
"Operation interrupted."

E_AP000B_COMMIT_FAILED
"The transaction could not be committed."

E_AP000C_2PC_REFUSED
"Server unable to commit a 2PC transaction."

E_AP000D_PARAMETERS_REQUIRED
"The requested query requires parameters and none were specified."

E_AP000E_INVALID_CONNECT_PARM
"A connection parameter or value was rejected by the server."

E_AP000F_NO_CONNECT_PARMS
"No connection parameters have been set."

E_AP0010_INVALID_COLUMN_COUNT
"Invalid number of columns or parameters specified."

E_AP0011_INVALID_PARAM_VALUE
"Invalid parameter value."

E_AP0012_INVALID_DESCR_INFO
"Invalid column usage type."

E_AP0013_INVALID_POINTER
"A required pointer parameter was NULL."

E_AP0014_INVALID_REPEAT_ID
"The requested repeat query is not known to the server.  Please re-define."

E_AP0015_INVALID_TRANS_STMT
"Transaction commit or rollback attempted when such action not allowed."

E_AP0016_ROW_DISCARDED
"A row of data was received for which there was no room in the receive buffer."

E_AP0017_SEGMENT_DISCARDED
"All but first segment of a Binary Large OBject was discarded."

E_AP0018_INVALID_DISCONNECT
"Connection is being disconnected"

E_AP001F_BATCH_UNSUPPORTED
"Batch processing is not supported.\n
A connection level of IIAPI_LEVEL_6 is required."

E_AP0020_BYREF_UNSUPPORTED
"BYREF procedure parameters are not supported.\n
A connection level of IIAPI_LEVEL_1 is required."

E_AP0021_GTT_UNSUPPORTED
"Global Temporary Table procedure parameters are not supported.\n
A connection level of IIAPI_LEVEL_1 is required."

E_AP0022_XA_UNSUPPORTED
"XA operations are not supported.\n
A connection level of IIAPI_LEVEL_4 is required."

E_AP0023_INVALID_NULL_DATA
"A non-nullable data value was marked as null."

E_AP0024_INVALID_DATA_SIZE
"Invalid data size."

E_AP0025_SVC_DATA_TYPE
"Invalid data type for SVCPARM parameter."

E_AP0028_LVL1_DATA_TYPE
"Invalid data type.\n
A minimum connection level of IIAPI_LEVEL_1 is required."

E_AP0029_LVL2_DATA_TYPE
"Invalid data type.\n
A minimum connection level of IIAPI_LEVEL_2 is required.\n
IIapi_initialize() requires at least IIAPI_VERSION_3."

E_AP002A_LVL3_DATA_TYPE
"Invalid data type.\n
A minimum connection level of IIAPI_LEVEL_3 is required.\n
IIapi_initialize() requires at least IIAPI_VERSION_4."

E_AP002B_LVL4_DATA_TYPE
"Invalid data type.\n
A minimum connection level of IIAPI_LEVEL_4 is required.\n
IIapi_initialize() requires at least IIAPI_VERSION_5."

E_AP002C_LVL5_DATA_TYPE
"Invalid data type.\n
A minimum connection level of IIAPI_LEVEL_5 is required.\n
IIapi_initialize() requires at least IIAPI_VERSION_6."

E_AP002D_LVL6_DATA_TYPE
"Invalid data type.\n
A minimum connection level of IIAPI_LEVEL_6 is required.\n
IIapi_initialize() requires at least IIAPI_VERSION_7."

E_AP002E_LVL7_DATA_TYPE
"Invalid data type.\n
A minimum connection level of IIAPI_LEVEL_7 is required.\n
IIapi_initialize() requires at least IIAPI_VERSION_8."

