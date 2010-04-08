SQLSTATE_MAPPING_REQUIRED

/*
**    Message file for Facility code: _RQ
*/
/*
** History :
**
**    11-dec-1991 (fpang)
**      Added E_RQ004A_LDB_ERROR_MSG. This message will be used to log
**      local errors to the errlog file only. RQ0042 should no longer be used
**      to log errors to the errlog file.
**      Fixes B39727.
**    11-dec-1991 (fpang)
**      Added E_RQ004B. This message will be issued when a user attempts a
**      network connection w/o a local authorization. This message obsoletes
**      E_RQ0047.
**      Fixes B39731.
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**     18-Oct-1993 (fred)
**	Added E_RQ004C.  RQF issues this message when called upon to manage
**      a large object -- which it is not good at doing...
**	02-dec-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
*/


#define _RQ_CLASS 13


E_RQ0001_WRONG_COLUMN_COUNT:SS5000B_INTERNAL_ERROR
"LDB return column count differs from STAR's."
E_RQ0002_NO_TUPLE_DESCRIPTION:SSHZ000_RDA
"Missing GCA_TD_DATA. STAR or LDB protocol error."
E_RQ0003_TOO_MANY_COLUMNS:SS5000B_INTERNAL_ERROR
"LDB return column count differs from STAR's."
E_RQ0004_BIND_BUFFER_TOO_SMALL:SS5000B_INTERNAL_ERROR
"QEF column bind buffer to small. STAR internal error."
E_RQ0005_CONVERSION_FAILED:SS5000B_INTERNAL_ERROR
"Catalog column data conversion failed. STAR or LDB internal error."
E_RQ0006_CANNOT_GET_ASSOCIATION:SS08000_CONNECTION_EXCEPTION
"Cannot establish connection to LDB server for node=%0c, database=%1c, DBMS=%2c."
E_RQ0007_BAD_REQUEST_CODE:SS5000B_INTERNAL_ERROR
"Invalid RQF request code. STAR internal error."
E_RQ0008_SCU_MALLOC_FAILED:SS5000B_INTERNAL_ERROR
"Cannot allocate memory from SCF. STAR internal error."
E_RQ0009_ULM_STARTUP_FAILED:SS5000B_INTERNAL_ERROR
"Cannot start up ULM. STAR internal error."
E_RQ0010_ULM_OPEN_FAILED:SS5000B_INTERNAL_ERROR
"ULM stream open failed. STAR internal error."
E_RQ0011_INVALID_READ:SSHZ000_RDA
"GCA_RECEIVE failed."
E_RQ0012_INVALID_WRITE:SSHZ000_RDA
"GCA_SEND failed"
E_RQ0013_ULM_CLOSE_FAILED:SS5000B_INTERNAL_ERROR
"ULM stream close failed."
E_RQ0015_UNEXPECTED_MESSAGE:SSHZ000_RDA
"Unexpected GCA message."
E_RQ0017_NO_ACK:SSHZ000_RDA
"Missing acknowledgement for GCA_ATTENTION message."
E_RQ0018_SHUTDOWN_FAILED:SS5000B_INTERNAL_ERROR
"RQF shutdown failed"
E_RQ0019_COMMIT_FAILED:SS08007_XACT_RES_UNKNOWN
"RQF commit failed"
E_RQ0020_ABORT_FAILED:SS08007_XACT_RES_UNKNOWN
"Error occurred during RQF abort."
E_RQ0021_BEGIN_FAILED:SS5000B_INTERNAL_ERROR
"Cannot begin transaction."
E_RQ0022_END_FAILED:SS5000B_INTERNAL_ERROR
"Cannot end transaction."
E_RQ0023_COPY_FROM_EXPECTED:SSHZ000_RDA
"Expecting COPY FROM indication from LDB."
E_RQ0024_COPY_DEST_FAILED:SSHZ000_RDA
"Table transfer failed at destination site."
E_RQ0025_COPY_SOURCE_FAILED:SSHZ000_RDA
"Table transfer failed at source site."
E_RQ0026_QID_EXPECTED:SSHZ000_RDA
"Query ID expected from LDB."
E_RQ0027_CURSOR_CLOSE_FAILED:SS24000_INV_CURS_STATE
"Close cursor failed."
E_RQ0028_CURSOR_FETCH_FAILED:SS24000_INV_CURS_STATE
"Cursor fetch failed."
E_RQ0029_CURSOR_EXEC_FAILED:SS24000_INV_CURS_STATE
"Execute cursor failed."
E_RQ0030_CURSOR_DELETE_FAILED:SS24000_INV_CURS_STATE
"Cursor delete failed."
E_RQ0031_INVALID_CONTINUE:SS5000B_INTERNAL_ERROR
"Direct connection protocol error."
E_RQ0032_DIFFERENT_TUPLE_SIZE:SS5000B_INTERNAL_ERROR
"LDB and STAR differ on tuple size."
E_RQ0033_FETCH_FAILED:SSHZ000_RDA
"Data fetch from LDB message failed."
E_RQ0034_COPY_CREATE_FAILED:SSHZ000_RDA
"Temporary table creation for a transfer failed."
E_RQ0035_BAD_COL_DESC_FORMAT:SSHZ000_RDA
"Cannot process tuple description."
E_RQ0037_II_LDB_NOT_DEFINED:SS01000_WARNING
"II_LDB_SERVER not defined."
E_RQ0038_ULM_ALLOC_FAILED:SS5000B_INTERNAL_ERROR
"Memory allocation failed. Status %x."
E_RQ0039_INTERRUPTED:SS08000_CONNECTION_EXCEPTION
"LDB request was interrupted."
E_RQ0040_UNKNOWN_REPEAT_Q:SS26000_INV_SQL_STMT_NAME
"Invalid repeat query id used during repeat query execution."
E_RQ0041_ERROR_MSG_FROM_LDB:SS01000_WARNING
"LDB reports error:\n\"%0c\""
E_RQ0042_LDB_ERROR_MSG:SS01000_WARNING
"Preceding message is from LDB=%0c,  NODE=%1c, DBMS=%2c"
E_RQ0043_CURSOR_UPDATE_FAILED:SS24000_INV_CURS_STATE
"Cursor update failed."
E_RQ0044_CURSOR_OPEN_FAILED:SS24000_INV_CURS_STATE
"Cursor open failed."
E_RQ0045_CONNECTION_LOST:SS08000_CONNECTION_EXCEPTION
"RQF lost the connection to LDB"
E_RQ0046_RECV_TIMEOUT:SSHZ000_RDA
"RQF receiver timed out, LDB did not respond."
E_RQ0047_UNAUTHORIZE_USER:SS08000_CONNECTION_EXCEPTION
"Unauthorized user=%0c, connecting to node=%1c, database=%2c, DBMS=%3c"
E_RQ0048_SECURE_FAIL:SS08007_XACT_RES_UNKNOWN
"RQF failed to secure a distributed transaction"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an internal error; one of the local servers participating
**	in the distributed transaction refused to secure a transaction.
**
** System Status:
**	TPF will take further action.
**
** Recommendation:
**	None -- TPF will act accordingly.
*/
E_RQ0049_RESTART_FAILED:SS08007_XACT_RES_UNKNOWN
"RQF failed to restart a session of a failed distributed transaction \
for node=%0c, database=%1c, DBMS=%2c."
/*%
** Parameters:
**	%0c - node name
**	%1c - database name
**	%2c - DBMS type
**
** Explanation:
**	This is an internal error; RQF failed to establish a connection
**	with a database server to recover a failed distributed transaction.
**
** Recommendation:
**	This is an informational message logged to the error log file.  Other
**	error messages will indicate corrective actions, if any.
*/
E_RQ004A_LDB_ERROR_MSG:SS01000_WARNING
"Received LDB generic error (%0x (local error=%1x)) from %3c::%2c/%4c"
/*%
** Parameters:
**	%0x - LDB generic error number
**	%1x - LDB local error number
**	%2c - node name
**	%3c - database name
**	%4c - DBMS type
**
** Explanation:
**	This message should only be written to the error log file.
**	It displays the local error received.
**
** Recommendation:
**	None; the user should never see this message.
*/
E_RQ004B_CHK_LCL_AUTHORIZATION:SS08000_CONNECTION_EXCEPTION
"Authorization for USER=%0c to NODE=%1c is missing.
Use NETUTIL to add local authorization for USER=%0c to NODE=%1c."
/*%
** Parameters:
**	%0c - user name
**	%1c - node (should be local node)
**
** Explanation:
**	RQF detected a connection to a node without a password.
**	This happens when the user does not have a local authorization.
**
** Recommendation:
**	Use netutil to enter a local authorization.
*/
E_RQ004C_RQF_LO_MISMATCH:SS50008_UNSUPPORTED_STMT
"STAR does not yet allow large object parameters."
/*%
** Parameters:
**	None.
**
** Explanation:
**	RQF detected an attempt to use large objects.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Rewrite the query to avoid the large object, or use a direct connection.
*/


NO_SQLSTATE_MAPPING
