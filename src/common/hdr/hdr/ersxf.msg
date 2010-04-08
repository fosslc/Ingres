
SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _SX
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	13-apr-1993 (robf)
**	    Added SXM interface error messages:
**		E_SX0019_MAC_NOT_AVAILABLE
**		E_SX106C_SXM_VIEW_FAILURE
**		E_SX106D_SXM_WRITE_FAILURE
**		E_SX106E_SXM_WRITE_ACCESS
**		E_SX106F_SXM_ARBITRATE_ERR
**		E_SX1070_SXM_ARB_SUBJECT_LABEL
**		E_SX1071_SXM_ARB_OBJECT_LABEL
**		E_SX1072_SXM_DEF_SUBJECT_LABEL
**		E_SX1073_SXM_DEF_LABEL
**		E_SX1074_SXM_DEF_LABEL_PARM
**	7-may-1993 (robf)
**	   Added more SXM/L error messages
**		E_SX001A_SOP_ERROR
**		E_SX1075_SXM_DELDOWN_UNDEF
**		E_SX1076_PARM_ERROR
**		E_SX1077_SXC_GETDBCB
**		E_SX1078_SXL_SLCMP_ERROR
**		E_SX1079_SXL_FIND_ERROR
**		E_SX107A_SXC_ALTER_SESSION
**		E_SX107B_SXL_LI_EXPAND_TYPE
**		E_SX107C_SXL_COMPARE_ERROR
**		E_SX107D_B1_NEEDS_DELETE_DOWN
**		E_SX107E_INVALID_DELETE_DOWN   
**       	E_SX107F_SXM_ENCAPSULATE	
**      	E_SX1080_SXM_DISJ_ING_HIGH
**      	E_SX1081_SXM_DISJ_ING_LOW	
**      	E_SX1082_SXM_DISJ_DB_LABEL	
**		E_SX1083_B1_NEEDS_CA_HIGH
**		E_SX1084_B1_NEEDS_CA_LOW	
**		E_SX1085_BAD_CA_HIGH
**		E_SX1086_BAD_CA_LOW		
**		E_SX1087_BAD_PROC_SECLABEL		
**		E_SX1088_PROC_SECLABEL_CAHIGH		
**		E_SX1089_BAD_SECID_OPEN		
**		E_SX108A_BAD_SECID_CLOSE		
**		E_SX108B_SXL_GETID		
**		E_SX108C_BAD_SECID_POSITION
**		E_SX108D_BAD_SECID_FETCH
**		E_SX108E_BAD_SECID_INSERT
**		E_SX108F_BAD_SECID_DELETE
**		E_SX1090_SXL_GETLABEL
**		E_SX1091_SXT_UBEGIN
**		E_SX1092_SXT_UEND
**		E_SX1093_SXL_NEW_ID
**		E_SX1094_SXL_DELETE
**		E_SX1096_SXC_SOP_ADD
**		E_SX1097_SXL_COLLATE_ERROR
**		E_SX1098_SXL_SLCOLL_ERROR
**		E_SX1099_SXLC_FIND_LABINFO
**		E_SX109A_SXLC_ADD_LABINFO
**		E_SX109B_HASH_ERROR
**		E_SX109C_SXL_ID_NOT_FOUND
**		E_SX109D_SXL_INVALID_LABEL
**		E_SX109E_SXL_EXPAND_LABINFO
**		E_SX109F_SXL_LAB_TO_ID
**		E_SX10A0_SXL_ID_TO_LAB
**	   Changed SX106C_VIEW_FAILURE to SX106C_ACCESS_FAILURE
**	20-aug-93 (stephenb)
**	    Added new errors I_SX2037_RULE_CREATE, I_SX2038_RULE_DROP.
**	24-aug-93 (stephenb)
**	    Added new error I_SX2039_TBL_COMMENT, 
**	3-sep-93 (stephenb)
**	    Improve message I_SX200A_GROUP_MEM_DROP to reflect the fact that
**	    the user may be purging the whole group.
**	8-sep-93 (robf)
**	     Add I_SX273A_ALTER_DATABASE for alter database operation.
**	9-sep-93 (stephenb)
**	    Add messages I_SX203A_TBL_CONSTRAINT_ADD and 
**	    I_SX203B_TBL_CONSTRAINT_DROP for auditing of ALTER TABLE statement.
**	    Also change I_SX2032_EVENT_ACCESS to I_SX2032_EVENT_CREATE and add
**	    I_SX203C_EVENT_DROP to improve audit messages for database events.
**	23-sep-93 (stephenb)
**	    Add message I_SX203D_INTEGRITY_CREATE.
**	1-oct-93 (stephenb)
**	    Add messages I_SX203E_SCHEMA_CREATE, I_SX203F_SCHEMA_DROP.
**      6-oct-93 (stephenb)
**          Add messages I_SX2040_TMP_TBL_CREATE and I_SX2041_TMP_TBL_DROP.
**	7-oct-93 (robf)
**	    Add message E_SX10B3_INSERT_SECID_DUP_KEY
**	18-oct-93 (stephenb)
**	    Add message I_SX2042_REMOVE_TABLE.
**	20-oct-93 (robf)
**	    Add message E_SX002C_AUDIT_IS_ACTIVE
**	                I_SX273C_TABLE_STATS,
**	                I_SX273D_ENQUIRE_AUDIT_STATE
**	27-oct-93 (robf)  
**          Add message I_SX273E_SET_SESS_PRIORITY
**			I_SX273F_SET_SESSION_LIMIT
**	9-nov-93 (robf)
**          Add message I_SX2740_SET_QUERY_LIMIT, I_SX2741_SET_LOCKMODE
**	19-nov-93 (stephenb)
**	    Add messages I_SX2043_GW_REGISTER, I_SX2044_GW_REMOVE
**	24-nov-93 (stephenb)
**	    Alter protection messages to be more friendly, and to reflect the
**	    fact that all protections may be dropped in one statement.
**	03-dec-93 (dianeh)
**	    Refine text destined for messages.txt file.
**	8-dec-93 (robf)
**	    Add I_SX2743_DB_ALARM
**	30-dec-93 (robf)
**          Add I_SX2744_SESSION_IDLE_LIMIT,
**	        I_SX2745_SESSION_CONNECT_LIMIT
**	7-jan-94 (stephenb)
**	    Add following messages for SXAPO functionality:
**		E_SX002D_OSAUDIT_NO_POSITION
**		E_SX002E_OSAUDIT_NO_READ
**		E_SX10B4_OSAUDIT_WRITE_FAILED
**		E_SX10B5_OSAUDIT_FLUSH_FAILED
**		E_SX10B6_OSAUDIT_OPEN_FAILED
**	11-jan-94 (stephenb)
**	    Added E_SX10B7_BAD_AUDIT_METHOD
**	12-jan-94 (robf)
**	    Update text for E_SX102C_BAD_FILE_CONTEXT to match what it
**	    actually does.
**	4-feb-94 (stephenb)
**	    added E_SX10B8_OSAUDIT_NO_CHANGEFILE.
**	9-feb-94 (robf)
**          added E_SX10B9_BAD_MO_ATTACH
**	14-feb-94 (robf)
**	    added I_SX2835_CONNECT, I_SX2836_DISCONNECT
**	22-feb-94 (robf)
**          added I_SX2746_SESSION_ROLE
**	    change APPLICATION to ROLE for role auditing.
**	24-feb-94 (stephenb)
**	    added E_SX10BA_OSAUDIT_NOT_ENABLED.
**	30-feb-94 (stephenb)
**	    added I_SX2747_DUMPDB
**	5-mar-94 (robf)
**          added:
**		I_SX2748_SET_SERVER_CLOSED
**		I_SX2749_SET_SERVER_OPEN
**		I_SX274A_STOP_SERVER
**		I_SX274B_REMOVE_SESSION
**		I_SX274C_CRASH_SESSION
**		I_SX274D_SET_SERVER_SHUT
**	25-apr-94 (stephenb)
**	    Altered E_SX10B7 to display current correct audit mechanism options
**	    INGRES or SYSTEM.
**	7-mar-94 (robf)
** 	     added:
**	        I_SX274E_ROLE_GRANT
**	        I_SX274F_ROLE_REVOKE
**	15-apr-94 (robf)
**	     Added:
**		I_SX2750_SET_TRACE_POINT
**		I_SX2751_SET_NOTRACE_POINT
**		E_SX10BC_SXAP_SHM_BAD_VER
**		E_SX10BD_SXAP_SHM_BUF_COUNT
**		E_SX10BE_SXAP_SHM_LOCK_ERR
**		E_SX10BF_SXAP_SHM_UNLOCK_ERR
**		E_SX10C0_SXAP_QWRITE_ERR
**		E_SX10C1_SXAP_QFLUSH_ERR
**		E_SX10C2_SXAP_QFLUSH_BUF_FREE
**		E_SX10C3_SXAP_QALLFLUSH_ERR
**		E_SX10C4_SXAP_QALLFLUSH_BUF
**		E_SX10C5_AUDIT_WRITER_NO_SCB
**		E_SX10C6_AUDIT_WRITER_ERROR
**		E_SX10C7_AUDIT_WRITE_ERROR
**		E_SX10C8_AUDIT_WRITE_BUF_FREE
**	25-apr-94 (stephenb)
**	    Altered E_SX10B7 to display current correct audit mechanism options
**	    INGRES or SYSTEM.
**	29-apr-94 (robf)
**            Added:
**		E_SX10C9_INVALID_WRITE_FIXED
**		E_SX10CA_INVALID_UPDATE_FLOAT
**		E_SX10CB_SXAP_SHM_PG_SIZE
**	9-may-94 (robf)
**            Added:
**		E_SX10CC_SXL_LABEL_ID_INFO
**      4-jan-95 (alison)
**            Added:
**              E_SX2752_RELOCATE
**	19-jun-1996 (sweeney)
**		Fix typos/anglicisms.
**	10-oct-1996 (canor01)
**	    Changes INGRES_HIGH/LOW to CA_HIGH/LOW.  Made messages more
**	    generic.
**      26-feb-97 (dilma04)
**          Add I_SX2753_ROW_DEADLOCK. 
**	24-Apr-2003 (jenjo02)
**	    Added I_SX2755_KILL_QUERY, SIR 110141.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**      11-Oct-2006 (hanal04) Bug 115658
**          Remove Computer Associates references.
*/
#define _SX_CLASS  16

E_SX0000_OK:SS00000_SUCCESS
"SXF operation has been successfully completed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The requested SXF operation has completed without error.
**
** System Status:
**	The program continues.
**
** Recommendation:
**	None.
*/
E_SX0001_BAD_OP_CODE:SS5000B_INTERNAL_ERROR
"Operation code to sxf_call() was not recognized."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SXF call interface was called with an invalid operation
**	code.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0002_BAD_CB_TYPE:SS5000B_INTERNAL_ERROR
"A call made to SXF specified an invalid control block type."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is raised when SXF examines the control block type
**	passed with a request and finds it to be invalid.
**
** System Status:
**	The current operation will be rejected.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0003_BAD_CB_LENGTH:SS5000B_INTERNAL_ERROR
"A call to SXF specified an invalid control block (SXF_RCB) size"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is raised when SXF is called with a control block
**	with an invalid size.
**
** System Status:
**	The current operation will be rejected.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0004_BAD_PARAMETER:SS5000B_INTERNAL_ERROR
"A call made to SXF specified a bad parameter"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is raised when SXF is called with a parameter that it
**	does not recognize.
**
** System Status:
**	The current operation will be rejected.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0005_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"An internal error occurred within SXF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is caused by an internal error in the SXF facility.
**	Preceding error messages in the error log will describe the
**	nature of the error.
**
** System Status:
**	Depending on the cause of the error, the DBMS may shut down.
**
** Recommendation:
**	Examine preceding error messages to determine the cause of the error.
*/
E_SX0006_FACILITY_ACTIVE:SS5000B_INTERNAL_ERROR
"An attempt was made to start SXF when it was already running."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The routine used to start SXF was called more than once during
**	the execution of the program. This would be caused by an internal
**	coding error.
**
** System Status:
**	The program continues.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0007_NO_ROWS:SS5000B_INTERNAL_ERROR
"No more data satisfies the request."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Returned from an audit file read operation when there are no
**	more rows left to read in the audit file.
**
** System Status:
**	The program continues.
**
** Recommendation:
**	None.
*/
E_SX0008_BAD_STARTUP:SS5000B_INTERNAL_ERROR
"SXF failed to start up successfully."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error was encountered during the execution of the SXF startup
**	routines. An earlier error message should describe the nature of
**	the error.
**
** System Status:
**	SXF will not be started.
**
** Recommendation:
**	Examine the contents of earlier error messages in the error log.
*/
E_SX0009_BEGIN_SESSION:SS5000B_INTERNAL_ERROR
"A request to begin a SXF session failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error was encountered during the execution of the SXF begin session
**	routines. An earlier error message should describe the nature of
**	the error.
**
** System Status:
**	The session will not be registered with SXF.
**
** Recommendation:
**	Examine the contents of earlier error messages in the error log.
*/
E_SX000A_QUOTA_EXCEEDED:SS5000B_INTERNAL_ERROR
"The maximum number of SXF sessions has been exceeded."
/*%
** Parameters:
**	None.
**
** Explanation:
**	At SXF startup time, SCF will tell SXF the maximum number of sessions
**	that it is allowed to accept. If this number is exceeded, this error
**	is raised. Currently, the maximum number of sessions allowed is the
**	number of active DBMS sessions.
**
** System Status:
**	The current operation will be rejected.
**
** Recommendation:
**	Increase the maximum allowable number of DBMS sessions.
*/
E_SX000B_SESSIONS_ACTIVE:SS5000B_INTERNAL_ERROR
"An attempt was made to shutdown SXF when sessions were still active."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF was requested to shut down when there were still sessions
**	registered with the facility.
**
** System Status:
**	SXF will not shut down.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX000C_BAD_SHUTDOWN:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to shutdown SXF"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error was encountered during the execution of the SXF facility
**	shutdown routines. An earlier message should describe the nature of
**	the error.
**
** System Status:
**	SXF will not shut down cleanly.
**
** Recommendation:
**	Examine earlier error messages in the error log.
*/
E_SX000D_INVALID_SESSION:SS5000B_INTERNAL_ERROR
"A SXF request specified an invalid session identifier."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF clients are expected to pass the SCF session identifier of their
**	session when making requests from SCF. If this identifier is not
**	for a valid session that has previously been registered with SXF,
**	this message will be raised.
**
** System Status:
**	The current request will be rejected.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX000E_BAD_SESSION_END:SS5000B_INTERNAL_ERROR
"Error ending a SXF session."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error was encountered while executing the SXF session termination
**	routines. An earlier error message should describe the nature of the
**	error.
**
** System Status:
**	The SXF session will not terminate cleanly.
**
** Recommendation:
**	Examine earlier error messages in the error log.
*/
E_SX000F_SXF_NOT_ACTIVE:SS5000B_INTERNAL_ERROR
"A SXF request was made when the facility was not running."
/*%
** Parameters:
**	None.
**
** Explanation:
**	It is required that SXF initialization routines are run before
**	any other SXF request can be processed. If a request is made when
**	the facility has not been initialized correctly, this error will
**	be raised.
**
** System Status:
**	The request will not be processed.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0010_SXA_NOT_ENABLED:SS5000B_INTERNAL_ERROR
"This request cannot be processed because security auditing is not enabled."
/*%
** Parameters:
*       None.
**
** Explanation:
**	A request was made to the SXF system which cannot be processed,
**	because the SXF auditing system has not been enabled.
**
** System Status:
**	The current statement will be rejected.
**
** Recommendation:
**	Start the installation with auditing enabled.
*/
E_SX0011_INVALID_ACCESS_ID:SS5000B_INTERNAL_ERROR
"A call to SXF specified an invalid audit file access identifier."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When an SXF client opens an audit file for processing, a file access
**	identifier is returned to the caller. This identifier must be specified
**	for all subsequent accesses to the file. If a file access request
**	specifies a file identifier that is not known to SXF, this error
**	is returned. This is an internal DBMS problem.
**
** System Status:
**	The current statement will be rejected.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0012_FILE_NOT_OPEN:SS5000B_INTERNAL_ERROR
"Request to position an audit file failed because the file is not open."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to access an audit file which was not open.
**
** System Status:
**	The current statement will be rejected.
**
** Recommendation:
**	The most probable cause of this error is a bug in the audit
**	file access gateway. This should be reported to Ingres Corporation
**	Technical Support.
*/
E_SX0013_FILE_NOT_POSITIONED:SS5000B_INTERNAL_ERROR
"A request to read an audit record failed because the file has not 
been positioned for reading."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to read from an audit file which had not been
**	positioned for reading.
**
** System Status:
**	The current statement will be rejected.
**
** Recommendation:
**	The most probable cause of this error is a bug in the audit
**	file access gateway. This should be reported to Ingres Corporation
**	Technical Support.
*/
E_SX0014_BAD_POSITION_TYPE:SS5000B_INTERNAL_ERROR
"A request to position an audit file contained an invalid position type."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When the SXF caller wants to position a file for reading, the
**	request made must specify the type of position required (e.g.,
**	file scan, key position, etc.).  If the position request
**	specifies an unknown type, this message is returned. This
**	would be caused by an internal coding error. 
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0015_FILE_NOT_FOUND:SS5000B_INTERNAL_ERROR
"The specified security audit file was not found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to open an audit file which did not exist.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Make sure that you specify a valid audit file.
*/
E_SX0016_FILE_EXISTS:SS5000B_INTERNAL_ERROR
"The specified file could not be created because it already exists."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to create an audit file, but the request
**	could not be satisfied because the file already exists. This
**	error is used internally in SXF. The user should not see this
**	error message; if they do, it's a bug.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX0017_SXAS_ALTER:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to update the security audit configuration.
The exact cause of the problem is described by earlier messages"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request to update the audit configuration failed to complete
**	successfully. Earlier messages should show the cause of the failure.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Check preceding error messages in the error log.
*/
E_SX0018_TABLE_NOT_FOUND:SS5000B_INTERNAL_ERROR
" The iisecuritystate table was not found in the iidbdb."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt to open the security state catalog failed because the
**	table could not be found. This happens when the system catalogs were
**	created using a version of the product which doesn't support security 
**	auditing. This message is used for internal status management only; 
**	it should never be written to the error log, or returned to the user.
**
** System Status:
**	The current operation continues.
**
** Recommendation:
**	None.
*/
E_SX0019_MAC_NOT_AVAILABLE:SS5000B_INTERNAL_ERROR
"MAC arbitration is only available on B1 active servers. This server is
not configured for B1 security. Access to the object will be denied."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to use SXF to perform MAC arbitration, but the
**	DBMS is not configured with B1 security. 
**
** System Status:
**	Access to the object is rejected.
**
** Recommendation:
**	Reconfigure the system as B1, then try the operation again.
*/
E_SX001A_SOP_ERROR:SS5000B_INTERNAL_ERROR
"An error was returned by a SXF Security Operations extension request.
Check the error log (errlog.log) for more information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to SXF the SXF Security Operations (SOP)
**	extension, but an error occurred. 
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	Check the error log for more detailed reasons why this
**	error occurred.
*/
E_SX001B_LABELMAP_NOT_FOUND:SS5000B_INTERNAL_ERROR
"The iilabelmap catalog or one of its indexes does not exist or you do not
have privilege to access it."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF tried to access a system catalog or one of its indexes
**	but was unable to do so. 
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	Check the error log for more detailed reasons why this
**	error occurred.
*/
E_SX001C_BAD_DB_LABEL:SS5000B_INTERNAL_ERROR
"An invalid database security label was specified.\n
This could be caused by a security label unknown to the secure operating \n
system, or by a label which is not between CA_LOW and CA_HIGH."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An illegal security label for a database was specified.
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	Correct the database label or CA_HIGH/LOW to legal values.
*/
E_SX001D_BAD_USER_LABEL:SS5000B_INTERNAL_ERROR
"An invalid user session security label was specified.\n
This could be caused by a security label unknown to the secure operating
system, or by a label which is not between CA_LOW and CA_HIGH,
your limiting security label or the database security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An illegal security label for a user session was specified.
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	Correct the user session label or CA_HIGH/LOW to legal values.
*/
E_SX001E_TRACE_FLAG_ERR:SS5000B_INTERNAL_ERROR
"An invalid SXF trace flag was specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An illegal SXF trace flag was specified.
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	Specify a correct trace flag.
*/
E_SX001F_DEADLOCK:SS5000B_INTERNAL_ERROR
"Deadlock was detected in SXF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Deadlock was returned while processing a request.
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	None.
*/
E_SX0020_LOCK_ERROR:SS5000B_INTERNAL_ERROR
"A locking or lock resource/timeout error was reported to SXF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A lock or lock resource/timeout error was returned while processing 
**	an SXF request.
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	None.
*/
E_SX0021_ALREADY_SUSPENDED:SS5000B_INTERNAL_ERROR
"Security auditing is already suspended."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to suspend or stop security auditing, but auditing 
**	was already suspended.
**
** System Status:
**	The request is rejected.
**
** Recommendation:
**	Only suspend auditing when it is active.
*/
E_SX0022_NOT_SUSPENDED:SS5000B_INTERNAL_ERROR
"Security auditing is not suspended."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to resume security auditing, but it
**	is not suspended.
**
** System Status:
**	The request is rejected.
**
** Recommendation:
**	Only try to resume auditing when it is suspended.
*/
E_SX0023_SUSPEND_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to SUSPEND security auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to suspend security auditing.
**
** System Status:
**	The request is rejected.
**
** Recommendation:
**	Check the error log for more details on why the suspend
**	operation failed.
*/
E_SX0024_RESUME_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to RESUME security auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to resume security auditing.
**
** System Status:
**	The request is rejected; auditing is not resumed.
**
** Recommendation:
**	Check the error log for more details on why the resume
**	operation failed.
*/
E_SX0025_SET_LOG_ERR:SS5000B_INTERNAL_ERROR
"The requested file could not be used for the security audit log."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When trying to change security audit logs, the requested log file 
**	could not be used. Possibly either the file is not one of the 
** 	named files in the system configuration, the file already exists,
**	or the file name is invalid.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more details on why the operation
**	failed, then retry the operation.
*/
E_SX0026_USER_LOG_EXISTS:SS5000B_INTERNAL_ERROR
"The requested security audit log file already exists."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The requested security audit log already exists.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	You may only switch to a security audit log which doesn't already 
**	exist.
*/
E_SX0027_USER_LOG_UNKNOWN:SS5000B_INTERNAL_ERROR
"The requested security audit log file is not known to the audit system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The requested security audit log is not one of the files specified
**	in the system configuration.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	You may only switch to a security audit log which is known to
**	the audit system.
*/
E_SX0028_SET_LOG_NEEDS_AUDIT:SS5000B_INTERNAL_ERROR
"The security audit log file can only be changed when security auditing is
active."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to change the security audit log, but the 
**	security audit configuration is currently inactive.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Turn on security auditing, then try to switch to a new security audit
**	log.
*/
E_SX0029_STOP_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred attempting to STOP security auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to stop security auditing, but an error
**	occurred.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information on why this occurred.
*/
E_SX002A_RESTART_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred attempting to RESTART security auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to restart security auditing, but an error
**	occurred.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information why on this occurred.
*/
E_SX002B_AUDIT_NOT_ACTIVE:SS5000B_INTERNAL_ERROR
"Security auditing is not active."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to alter the security audit state in a way which
**	requires auditing to be active.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Restart security auditing, then issue the command again.
*/
E_SX002C_AUDIT_IS_ACTIVE:SS5000B_INTERNAL_ERROR
"Security auditing is already active.\n
You may not perform this operation when security auditing is active."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to alter the security audit state which is
**	only allowed when auditing is not active, but auditing is active.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Only alter the security audit state with security auditing is
**	not active.
*/
E_SX002D_OSAUDIT_NO_POSITION:SS5000B_INTERNAL_ERROR
"Positioning of operating system audit trails for reading is not supported."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to position an operating system audit trail for
**	reading. Read operations are not supported on operating system audit
**	trails.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Use the tools provided by the operating system to read the audit trail.
*/
E_SX002E_OSAUDIT_NO_READ:SS5000B_INTERNAL_ERROR
"Reading of operating system audit trails is not supported."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A request was made to read a record from the operating system audit
**	trail. Read operations are not supported on operating system audit
**	trails.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**      Use the tools provided by the operating system to read the audit trail.
*/
E_SX1001_UNKNOWN_EXCEPTION:SS5000B_INTERNAL_ERROR
"An unknown exception was signaled in SXF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When an exception is signaled in SXF, the exception handler will
**	try to determine the nature of the exception and log the relevant
**	exception details. If it is unable to determine the nature of the
**	exception, it will log this error message.
**
** System Status:
**	The DBMS will terminate.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX1002_NO_MEMORY:SS5000B_INTERNAL_ERROR
"The SXF memory pool is exhausted. Either use less memory, or increase the
size of the memory pool by changing the DBMS server parameter SXF.POOL_SIZE 
to a larger value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	There is no more memory available for use in the SXF memory pool.
**
** System Status:
**	The current request will be aborted.
**
** Recommendation:
**	Increase the memory available to SXF.
*/
E_SX1003_SEM_INIT:SS5000B_INTERNAL_ERROR
"Error initializing a semaphore."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while executing the SXF semaphore initialization
**	routine.
**
** System Status:
**	The current SXF request will be rejected.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX1004_LOCK_CREATE:SS5000B_INTERNAL_ERROR
"Error creating a lock list."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error occurred while SXF was attempting to create an LK
**	lock list. The details of the nature of the error should be in an
**	earlier message.
**
** System Status:
**	The current SXF request will be aborted.
**
** Recommendation:
**	Examine the contents of preceding error messages in the error log.
*/
E_SX1005_SESSION_INFO:SS5000B_INTERNAL_ERROR
"Error attempting to get session information from SCF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while SXF was making a scu_information() call to SCF.
**	An earlier message should describe the nature of the error.
**
** System Status:
**	The current SXF request will be aborted.
**
** Recommendation:
**	Examine the contents of preceding error messages in the error log.
*/
E_SX1006_SCB_BUILD:SS5000B_INTERNAL_ERROR
"Error building a SXF session control block."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while the SXF session control block build routines
**	were executing. An earlier message should describe the nature of the
**	error.
**
** System Status:
**	The current SXF request will be aborted.
**
** Recommendation:
**	Examine the contents of preceding error messages in the error log.
*/
E_SX1007_BAD_SCB_DESTROY:SS5000B_INTERNAL_ERROR
"Error destroying a SXF session control block."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while the SXF session control block destroy routines
**	were executing. An earlier message should describe the nature of the
**	error.
**
** System Status:
**	The current SXF request will be aborted.
**
** Recommendation:
**	Examine the contents of preceding error messages in the error log.
*/
E_SX1008_BAD_LOCK_RELEASE:SS5000B_INTERNAL_ERROR
"Error releasing a lock list."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error occurred while SXF was attempting to release an LK
**	lock list. The details of the nature of the error should be in an
**	earlier message.
**
** System Status:
**	The current SXF request will be aborted.
**
** Recommendation:
**	Examine the contents of preceding error messages in the error log.
*/
E_SX1009_SXA_STARTUP:SS5000B_INTERNAL_ERROR
"Error starting the auditing system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to start up the SXF auditing
**	system. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX100A_SXA_SHUTDOWN:SS5000B_INTERNAL_ERROR
"Error terminating the auditing system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to shut down the SXF auditing
**	system. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX100B_SXA_BGN_SESSION:SS5000B_INTERNAL_ERROR
"Error beginning a session in the auditing system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to begin a session in the
**	SXF auditing system. Previous messages should describe the 
**	nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX100C_SXA_END_SESSION:SS5000B_INTERNAL_ERROR
"Error terminating a session in the auditing system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to end a session in the SXF auditing
**	system. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX100D_SXA_OPEN:SS5000B_INTERNAL_ERROR
"Error opening a security audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to open an audit file.
**	Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX100E_SXA_CLOSE:SS5000B_INTERNAL_ERROR
"Error closing a security audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to close an audit file.
**	Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX100F_SXA_POSITION:SS5000B_INTERNAL_ERROR
"Error positioning a security audit file for reading."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to position an audit file.
**	Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1010_SXA_READ:SS5000B_INTERNAL_ERROR
"Error reading a record from a security audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to read a record from an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1011_SXA_WRITE:SS5000B_INTERNAL_ERROR
"Error writing a record to the current security audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to write a record to an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1012_SXA_FLUSH:SS5000B_INTERNAL_ERROR
"A request to flush an audit record to an audit file failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to flush records to an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1013_SXA_SHOW_STATE:SS5000B_INTERNAL_ERROR
"A request to retrieve the current security audit state failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to return the current state
**	of the auditing system. Previous messages should describe the 
**	nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1014_SXA_GET_SCB:SS5000B_INTERNAL_ERROR
"Error finding the SXF session control block for a session."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to locate the SXF session
**	control block for the current session. Previous messages should 
**	describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1015_SXA_RSCB_BUILD:SS5000B_INTERNAL_ERROR
"Error building a RSCB."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to build an SXF record stream
**	control block (RSCB) for an audit file. Previous messages should 
**	describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1016_SXAP_MEM_FREE:SS5000B_INTERNAL_ERROR
"Error attempting to free shared memory in the audit system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to free memory in the SXF 
**	auditing system. Previous messages should describe the nature 
**	of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1017_SXAP_SHM_INVALID:SS5000B_INTERNAL_ERROR
"The audit shared memory segment contained invalid data. This may be caused
by a server terminating abnormally. Restart the server to free the
problem."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When the auditing system starts up, it either creates or maps
**	a shared memory segment. This segment contains a buffered copy 
**	of the current page in the audit file. When a server starts,
**	if it finds that the shared segment already exists, it checks
**	that the data in the shared buffer is valid. If it finds that 
**	it is invalid, it will raise this error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Restarting the server should cure the problem. If the error
**	persists, or occurs frequently the user should report the
**	problem to Ingres Corporation Technical Support.
*/
E_SX1018_FILE_ACCESS_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to access an audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF uses the DI subsystem to access audit files. If DI returns
**	an error, this message is returned along with the DI error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine the error log for the associated DI error message to determine 
**	the cause of the failure.
*/
E_SX1019_BAD_LOCK_CONVERT:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to convert a lock."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to escalate a lock to a
**	higher mode. This message should be accompanied by a message
**	from the LK subsystem which shows the nature of the failure.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine the error log for the associated LK error message to determine 
**	the cause of the failure.
*/
E_SX101A_SXAP_STARTUP:SS5000B_INTERNAL_ERROR
"Because of previous errors, the low level auditing system could not start up."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to start up the SXF auditing
**	system. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX101B_SXAP_SHM_DESTROY:SS5000B_INTERNAL_ERROR
"Error destroying the audit system's shared memory segment."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to destroy the audit
**	shared memory segment. This message should be accompanied 
**	by an ME message which describes the nature of the problem.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine the error log for the associated ME error message to determine 
**	the cause of the failure.
*/
E_SX101C_SXAP_SHUTDOWN:SS5000B_INTERNAL_ERROR
"Because of previous errors the low level auditing system failed to
shutdown cleanly."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to shut down the SXF auditing
**	system. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX101D_SXAP_BGN_SESSION:SS5000B_INTERNAL_ERROR
"Because of previous errors, a session could not be registered with
the auditing system." 
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to begin a session in the SXF
**	auditing system. Previous messages should describe the nature of
**	the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX101E_INVALID_FILENAME:SS5000B_INTERNAL_ERROR
"The specified file name is not valid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The file name given to an open or create request was not valid.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	If the file name was specified by the user, it should be 
**	checked for validity and, if necessary, changed. 
*/
E_SX101F_SXAP_SHOW:SS5000B_INTERNAL_ERROR
"Error getting audit system configuration data."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to return information about
**	the current audit configuration. Previous messages should describe 
**	the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1020_SXAP_INIT_CNF:SS5000B_INTERNAL_ERROR
"Error initializing the auditing system configuration."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to initialize the auditing system's
**	configuration. Previous messages should describe the nature of the
**	error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1021_BAD_PAGE_TYPE:SS5000B_INTERNAL_ERROR
"An invalid page was found in the specified audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When the auditing system reads an audit file, it expects that
**	the first page of the file will be an audit file header page
**	and that all subsequent pages will be audit file data pages.
**	If a file does not conform to this ordering, or if the page
**	type of an audit page is neither DATA, nor HEADER, this error
**	will be returned.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Verify that the file being accessed is an audit file. If it is,
**	some form of file corruption may have taken place -- this should be
**	reported to Ingres Corporation Technical Support.
*/
E_SX1022_BAD_FILE_VERSION:SS5000B_INTERNAL_ERROR
"The audit file could not be read by this version of the
auditing system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Each audit file contains a version number which shows the
**	version of the auditing system that it was written with.
**	If a file has been written by an old version of the auditing
**	system that is no longer supported, this error will be raised.
**	Another possible cause of this error is corruption in the
**	audit file header page.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Check that the audit file was written by a supported version
**	of the auditing system.
*/
E_SX1023_SXAP_OPEN:SS5000B_INTERNAL_ERROR
"Error opening an audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to open an audit file.
**	Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1024_BAD_FILE_ALLOC:SS5000B_INTERNAL_ERROR
"A page allocated from the audit file contained an invalid page number."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is caused by an internal error in the auditing system.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX1025_SXAP_CREATE:SS5000B_INTERNAL_ERROR
"Error creating an audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to create an audit file.
**	Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1026_SXAP_CLOSE:SS5000B_INTERNAL_ERROR
"Error closing audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to close an audit file.
**	Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1027_BAD_FILE_MODE:SS5000B_INTERNAL_ERROR
"The audit file was open in an invalid mode for the operation requested."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is raised if an SXF sanity check fails. It is an 
**	internal error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX1028_SXAP_POSITION:SS5000B_INTERNAL_ERROR
"Error positioning audit file for reading."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to position an audit file for
**	reading. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1029_INVALID_PAGENO:SS5000B_INTERNAL_ERROR
"Bad audit file page number found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is raised if an SXF sanity check fails. It is an 
**	internal error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX102A_END_OF_FILE:SS5000B_INTERNAL_ERROR
"End of security audit file was reached unexpectedly."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is returned if, when reading from an audit file, the
**	end-of-file status is returned unexpectedly. 
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX102B_SXAP_READ:SS5000B_INTERNAL_ERROR
"Error reading a record from a security audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to read a record from an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX102C_BAD_FILE_CONTEXT:SS5000B_INTERNAL_ERROR
"Invalid audit file context, the requested write audit file does not match
the current write audit file, or the file context is empty."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is raised if an SXF sanity check fails: A request was
**	made to write an audit record to an audit file, but the specified
**	audit file context did not match the current audit file context,
**	or the provided audit file context is empty.
**	This is an internal error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX102D_SXAP_STOP_AUDIT:SS5000B_INTERNAL_ERROR
"Disable the auditing system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a SXF internal error; it should never be returned to a user.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX102E_SXAP_WRITE:SS5000B_INTERNAL_ERROR
"Error writing an audit record to the current audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to write a record to an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX102F_SXAP_FLUSH:SS5000B_INTERNAL_ERROR
"An error occurred flushing audit records to the audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to flush records to an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1030_SXAP_WRITE_PAGE:SS5000B_INTERNAL_ERROR
"An error occurred while writing an audit file page."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to write a page to an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1031_BAD_CHECKSUM:SS5000B_INTERNAL_ERROR
"An invalid checksum value was found for an audit file page."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Each age in an audit file has a checksum value calculated for
**	it when it gets written to the audit file. If, when the page
**	is read, the checksum value of the page does not match the
**	checksum when it was written, this error will be returned.
**	The main cause of this error would be corruption in the audit file.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SX1032_SXAP_READ_PAGE:SS5000B_INTERNAL_ERROR
"Error occurred while reading a page from an audit file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to read a page from an audit
**	file. Previous messages should describe the nature of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1033_SXAP_ALLOC_PAGE:SS5000B_INTERNAL_ERROR
"Error allocating new audit file page."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to allocate a new page in
**	an audit file. Previous messages should describe the nature
**	of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1034_SXAP_SHM_CREATE:SS5000B_INTERNAL_ERROR
"An error occurred when attempting to create the audit system's shared
memory segment."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when attempting to create the auditing system's
**	shared memory segment. This error should be accompanied by an ME
**	error message which describes the cause of the failure.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1035_BAD_LOCK_REQUEST:SS5000B_INTERNAL_ERROR
"An error occurred when requesting a lock."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when requesting a lock from the locking system.
**	This error should be accompanied by an LK error message which 
**	describes the cause of the failure.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1036_AUDIT_FILE_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred when processing a request for audit file '%0c'."
/*%
** Parameters:
**	%0c - audit file name
**
** Explanation:
**	An error occurred in a routine that manipulates audit files. This
**	message is used simply to show the user the name of the audit file
**	that is being accessed. A previous message should describe the 
**	cause of the error.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1037_ONERR_SERVER_SHUTDOWN:SS5000B_INTERNAL_ERROR
"An error occurred during security audit processing, and the ONERROR 
configuration parameter has been set to SHUTDOWN. 
In order to preserve the security of the system, the DBMS will now shutdown.
For more information on the cause of the error, consult the error log."
/*%
** Parameters:
**	None.
**
** Explanation:
**	If an error occurs during security audit processing, the value
**	of the PM resource, *C2.ONERROR, is consulted. This resource may
**	contain either the string "SHUTDOWN" or "STOPAUDIT". If the resource
**	contains the string "SHUTDOWN", the DBMS server is terminated, and
**	this error is written to the error log.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure, then restart the server.
*/
E_SX1038_BEGIN_XACT_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred attempting to begin a database transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to begin a database transaction
**	on behalf of the caller. Earlier messages in the error log should
**	describe the cause of the failure in more detail.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX1039_BAD_SECCAT_OPEN:SS5000B_INTERNAL_ERROR
"An error occurred while opening the iisecuritystate catalog in the iidbdb."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to open the security state
**	catalog. A previous message should show the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX103A_BAD_SECCAT_READ:SS5000B_INTERNAL_ERROR
"An error occurred while reading from the iisecuritystate catalog in the 
iidbdb."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to read from the security state
**	catalog. A previous message should show the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX103B_BAD_SECCAT_REPLACE:SS5000B_INTERNAL_ERROR
"An error occurred while updating a record from the iisecuritystate catalog 
in the iidbdb."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to update a tuple from the 
**	security state catalog. A previous message should show the cause 
**	of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX103C_XACT_COMMIT_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred attempting to commit a database transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to commit a database transaction
**	on behalf of the caller. Earlier messages in the error log should
**	describe the cause of the failure in more detail.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX103D_XACT_ABORT_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred attempting to abort a database transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to abort a database transaction
**	on behalf of the caller. Earlier messages in the error log should
**	describe the cause of the failure in more detail.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure.
*/
E_SX103E_BAD_SECCAT_CLOSE:SS5000B_INTERNAL_ERROR
"An error occurred while closing the iisecuritystate catalog in the iidbdb."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred when SXF attempted to close the security state 
**	catalog. A previous message should show the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX103F_AUDIT_THREAD_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred in the Security Audit Thread."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The Security Audit thread is used to maintain the consistency
**	of the SXF audit state cache (a cached copy of the security state
**	catalog). If, during normal processing, the audit state cache
**	is found to be invalid, the audit thread is signaled. It wakes
**	up and refreshes the cache from the underlying system catalog.
**	If the cache was not successfully updated, this message is returned.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX1040_SXAS_CHECK_STATE:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to determine the current audit state."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Each time a request to write an audit record is received by SXF, the
**	audit state cache (a cached copy of the security state catalog) is
**	consulted to determine if the audit record should be written to the
**	audit log. If an error occurs while this check is being made, this 
**	error is returned. A previous error message in the error log should 
**	describe the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX1041_INVALID_EVENT_TYPE:SS5000B_INTERNAL_ERROR
"The iisecuritystate catalog in the iidbdb contains an invalid audit event 
record."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Each tuple in the security state catalog shows if auditing is
**	enabled or disabled for a particular security event type. If
**	a tuple is found for an event type that is not known, this error
**	is raised.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine all rows in the security state catalog to determine which
**	is invalid. Contact Ingres Corporation Technical Support.
*/
E_SX1042_BAD_ROW_COUNT:SS5000B_INTERNAL_ERROR
"The iisecuritystate catalog in the iidbdb contains an invalid number of 
records."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Each tuple in the security state catalog shows if auditing is
**	enabled or disabled for a particular security event type. If
**	there are too many or too few tuples in the catalog, this error
**	is raised.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine all rows in the security state catalog to determine which
**	is invalid. Contact Ingres Corporation Technical Support.
*/
E_SX1043_SXAS_CACHE_INIT:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to initialize the audit state cache."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while SXF was attempting to initialize the audit
**	state cache. Previous messages should describe the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX1044_SXAS_STARTUP:SS5000B_INTERNAL_ERROR
"An error occurred starting up the Audit State Management Subsystem."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while SXF was attempting to start up the audit
**	state management subsystem. Previous messages should describe the 
**	cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX1045_BAD_EVENT_SET:SS5000B_INTERNAL_ERROR
"An error occurred initializing a lock event."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while SXF was attempting to initialize a lock
**	event. Previous messages should describe the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX1046_BAD_EVENT_WAIT:SS5000B_INTERNAL_ERROR
"An error occurred while waiting for a lock event."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while SXF was attempting to wait for a lock
**	event. Previous messages should describe the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX1047_BAD_EVENT_CLEAR:SS5000B_INTERNAL_ERROR
"An error occurred signaling a lock event."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while SXF was attempting to signal a lock event.
**	Previous messages should describe the cause of the error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error.
*/
E_SX1048_SERVER_TERMINATE:SS5000B_INTERNAL_ERROR
"Because of earlier unrecoverable errors in SXF, the DBMS server is shutting
down."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unrecoverable error occurred in SXF. In order to preserve the
**	security of the system, the server will be shut down. Previous messages 
**	should describe the cause of the error.
**
** System Status:
**	The server will be shut down.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause of
**	the error. If possible, fix the problem and restart the server;
**	otherwise, contact Ingres Corporation Technical Support.
*/
E_SX1049_AUDIT_DISABLED:SS5000B_INTERNAL_ERROR
"An error occurred during security audit processing, and the ONERROR
configuration parameter has been set to STOPAUDIT. In order to allow 
continued operation of the DBMS, security auditing will be disabled now. 
For more information on the cause of the error, consult the error log."
/*%
** Parameters:
**	None.
**
** Explanation:
**	If an error occurs while attempting to write an audit record to the
**	security audit log, the value of the PM resource, *C2.ONERROR, is
**	consulted. This resource may contain either the string "SHUTDOWN" or
**	"STOPAUDIT". If the resource contains the string "STOPAUDIT", security
**	auditing is disabled in the DBMS, and this message is written to the 
**	error log.
**
** System Status:
**	The server will continue normal operation but will no longer
**	attempt to write audit records.
**
** Recommendation:
**	Examine previous messages in the error log to determine the cause
**	of the failure, then restart the server to reenable auditing.
*/
E_SX104A_SXAP_BAD_MAXFILE:SS5000B_INTERNAL_ERROR
"The security audit MAXFILE configuration parameter has an illegal value.
\nThe maximum size of the Security Audit log should be 100K or more."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An invalid startup parameter was specified for the maximum size
**	of audit logs.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Change the startup configuration so the MAXFILE parameter
**	has a legal value.
*/
E_SX104B_SXAP_NO_LOGNAMES:SS5000B_INTERNAL_ERROR
"No logfile names (AUDITLOG) were found while processing the security audit
\nconfiguration. At least one log file must be specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	No security audit log names were found in the 
**	startup configuration file.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Change the startup configuration so some log file 
**	names are specified.
*/
E_SX104C_SXAP_NEED_LOGFILES:SS5000B_INTERNAL_ERROR
"All security audit logfiles are used."
/*%
** Parameters:
**	None.
**
** Explanation:
**	All the available security audit logfiles are used. Auditing
**	cannot continue.
**
** System Status:
**	Depending on the configuration value for ONLOGFULL, either
**	auditing will stop or the server will shut down.
**
** Recommendation:
**	Make more audit logs available, then retry.
*/
E_SX104D_SXAP_BAD_ONLOGFULL:SS5000B_INTERNAL_ERROR
"The security audit ONLOGFULL configuration parameter is missing or has an 
illegal value. The allowed values are SHUTDOWN or STOPAUDIT"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The security audit startup parameter ONLOGFULL had an illegal value.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Correct the ONLOGFULL startup parameter, then try again.
*/
E_SX104E_SXAP_BAD_ONSWITCHLOG:SS5000B_INTERNAL_ERROR
"The security audit ONSWITCHLOG configuration parameter is missing or has an 
illegal value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The security audit startup parameter ONSWITCHLOG had an illegal value.
**
** System Status:
**	The current operation will not complete.
**
** Recommendation:
**	Correct the ONSWITCHLOG startup parameter, then try again.
*/
E_SX104F_SXAP_FINDCUR_OPER:SS5000B_INTERNAL_ERROR
"An invalid operation was passed to the security audit SXAP_FINDCUR routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal routine was passed an invalid operation while
**	attempting to find the current audit file.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
E_SX1050_SXAP_FINDCUR_LOGCHK:SS5000B_INTERNAL_ERROR
"An unexpected error was returned while trying to check the Security Audit log
\n'%0c'. This security log will not be used."
/*%
** Parameters:
**	%0c - log file name
**
** Explanation:
**	An unexpected error was returned while trying to check the
**	current security log.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
E_SX1051_SXAP_FINDCUR_BADGET:SS5000B_INTERNAL_ERROR
"Unable to look up the name of the newly selected Security Audit log."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Although an internal routine was able to find a new security audit
**	log, an error occurred later while looking up the name.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
E_SX1052_SHUTDOWN_LOGFULL:SS5000F_TEXTUAL_INFO
"The DBMS server is being shutdown because all Security Audit logfiles are 
\nfull and the ONLOGFULL configuration parameter has been set to SHUTDOWN."
/*%
** Parameters:
**	None.
**
** Explanation:
**	All security audit logs are full, and the ONLOGFULL startup
**	parameter is set to SHUTDOWN, so the server is shutting down.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Make more audit files available, then restart the server.
*/
E_SX1053_SHUTDOWN_NOW:SS5000B_INTERNAL_ERROR
"The DBMS server is being shutdown now by the Security Extensions Facility."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS server is being shut down by the Security Extensions
**	Facility (SXF). An earlier message should indicate the reason why.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Review the error log and correct the reason for the problem.
*/
E_SX1054_SXAP_CHANGE_CLOSE:SS5000B_INTERNAL_ERROR
"An error occurred attempting to CLOSE the current full Security Audit Log."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When switching audit logs, an error occurred while trying to
**	close the old log. A previous message in the error log should
**	give more details on why the error occurred.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Check the error log for more information.
*/
E_SX1055_SXAP_CHANGE_OPEN:SS5000B_INTERNAL_ERROR
"An error occurred attempting to OPEN the new current Security Audit Log.
\nThe file causing the error is '%0c'."
/*%
** Parameters:
**	%0c - log file name
**
** Explanation:
**	When switching audit logs, an error occurred while trying to
**	open the new log. A previous message in the error log should
**	give more details on why the error occurred.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Check the error log for more information.
*/
I_SX1056_CHANGE_AUDIT_LOG:SS5000F_TEXTUAL_INFO
"The current Security Audit log has been set to '%0c'."
/*%
** Parameters:
**	%0c - log file name
**
** Explanation:
**	The security audit log has been set to the indicated file.
**
** System Status:
**	This message is informational only.
**
** Recommendation:
**	None.
*/
E_SX1057_SXAP_CHANGE_NEXT:SS5000B_INTERNAL_ERROR
"An error occurred while trying to find the NEXT Security Audit log to use."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When switching audit logs, an error occurred while trying to
**	find the next log to use. A previous message in the error log should
**	give more details on why the error occurred.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Check the error log for more information.
*/
E_SX1058_SXAP_CHANGE_LOG_ERR:SS5000B_INTERNAL_ERROR
"An error occurred trying to change Security Audit Logs."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while switching audit logs.  A previous message
**	in the error log should give more details on why the error occurred.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Check the error log for more information.
*/
E_SX1059_SXAP_CHANGE_CREATE:SS5000B_INTERNAL_ERROR
"An error occurred trying to CREATE the new current Security Audit Log file.
\nThe file name is '%0c'."
/*%
** Parameters:
**	%0c - log file name
**
** Explanation:
**	When switching audit logs, an error occurred while trying to
**	create the new log. A previous message in the error log should
**	give more details on why the error occurred.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Check the error log for more information.
*/
E_SX105A_SXAP_CHANGE_PARAM:SS5000B_INTERNAL_ERROR
"An invalid operation parameter was passed to the change audit log routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal routine was passed an invalid operation while
**	attempting to change the current audit file.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
E_SX105B_SXAP_CHK_CONF_ERR:SS5000B_INTERNAL_ERROR
"An error occurred while attempting to check if the current Security Audit Log
was still valid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When checking to see if the current security audit log needed
**	to change, an error was detected.  A previous message in the
**	error log should give more details on why the error occurred.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Check the error log for more information.
*/
E_SX105C_SXAP_CHK_PROTOCOL:SS5000B_INTERNAL_ERROR
"An internal protocol error occurred in the shared buffer manager portion of
\nSXF while changing security audit logs. Either the shared buffer stamp,
the internal DBMS stamp, or the global lock stamp is inconsistent."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal protocol error was detected while checking if
**	the current security audit log needed to change.
**
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
I_SX105D_RUN_ONSWITCHPROG:SS5000F_TEXTUAL_INFO
"The user exit '%0c' is being executed at change of Security Audit Log."
/*%
** Parameters:
**	%0c - user exit program
**
** Explanation:
**	An optional user exit program is being run at change of security
**	audit log.
**
** System Status:
**	This message is informational only.
**
** Recommendation:
**	None.
*/
E_SX105E_RUN_SWITCH_TOOMANY:SS5000B_INTERNAL_ERROR
"Excessive errors have occurred trying to run the Security Audit user exit.
\nDiscontinuing attempts to run the  user exit."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Repeated errors have been returned while trying to execute the
**	user exit associated with changing security audit logs.
**	Due to too many errors, the DBMS will discontinue trying to
**	run the exit.
**
** System Status:
**	Auditing continues, but the user exit program will not be called.
**
** Recommendation:
**	Correct the problem with the user exit program, then restart the
**	the DBMS server.
*/
E_SX105F_RUN_SWITCH_ERROR:SS5000B_INTERNAL_ERROR
"An error occurred trying to run the Security Audit user exit."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error has been returned while trying to execute the
**	user exit associated with changing security audit logs.
**
** System Status:
**	Auditing continues. If too many errors are detected, the DBMS
**	will stop trying to execute the user exit.
**
** Recommendation:
**	Correct the problem with the user exit program.
*/
E_SX1060_SXAP_BAD_ONERROR:SS5000B_INTERNAL_ERROR
"The security audit ONERROR configuration parameter is missing or has an 
illegal value. The allowed values are SHUTDOWN or STOPAUDIT."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The ONERROR security audit startup parameter had a missing or
**	illegal value.
**
** System Status:
**	The operation will not continue.
**
** Recommendation:
**	Correct the configuration value for ONERROR, and retry.
*/
E_SX1061_SXAP_BAD_MODE:SS5000B_INTERNAL_ERROR
"The security audit MODE configuration parameter is missing or has an 
illegal value. The allowed values are ON or OFF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The MODE security audit startup parameter had a missing or
**	illegal value.
**
** System Status:
**	The operation will not continue.
**
** Recommendation:
**	Correct the configuration value for MODE, and retry.
*/
E_SX1062_SCF_SET_ACTIVITY:SS5000B_INTERNAL_ERROR
"An error was returned by SCF while attempting to SET the session activity."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When trying to set the session activity, an error was returned
**	by SCF.
**
** System Status:
**	The operation continues, but the session activity (e.g., available
**	via the monitor process) will not be updated.
**
** Recommendation:
**	If the error occurs repeatedly, contact Ingres Corporation 
**	Technical Support.
*/
E_SX1063_SCF_CLR_ACTIVITY:SS5000B_INTERNAL_ERROR
"An error was returned by SCF while attempting to CLEAR the session activity."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When trying to clear the session activity, an error was returned
**	by SCF.
**
** System Status:
**	The operation continues, but the session activity will not be updated.
**
** Recommendation:
**	If the error occurs repeatedly, contact Ingres Corporation 
**	Technical Support.
*/
E_SX1064_CHK_CNF_NULL_LKVAL:SS5000B_INTERNAL_ERROR
"The SXF access lock value is empty when a file STAMP is expected. This
indicates a locking or protocol problem."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal routine detected an invalid (NULL) lock value for
**	the security audit access lock when a value was expected. 
**	
** System Status:
**	The current operation will not complete. 
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
E_SX1065_BAD_REC_LENGTH:SS5000B_INTERNAL_ERROR
"The record length field of an audit record contained a value which was not the
same as the actual length of the audit record. This means that the audit file
may contain corrupt data."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When written to an audit file, the first two bytes of each audit record
**	are set to show the total length of the audit record. If, when reading
**	the record, it is found that the actual length of the record is not 
**	the same as that stored in the first two bytes, this error is raised.
**	This may be caused either by a bug in the code, or by some form of
**	corruption of the audit file.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
E_SX1066_BUF_TOO_SHORT:SS5000B_INTERNAL_ERROR
"Unable to write an audit record to the audit buffer because there was 
insufficient space in the buffer."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF is attempting to write an audit record into an audit buffer,
**	but there is not enough space remaining in the buffer to contain 
**	the record. If this error is returned to the user, there has
**	been a serious internal error.
**
** System Status:
**	The current operation will be aborted.
**
** Recommendation:
**	If this error has been seen by the user, report the problem to
**	Ingres Corporation Technical Support.
*/
E_SX1067_AUD_THREAD_NO_SCB:SS5000B_INTERNAL_ERROR
"The Security Audit Thread is unable to find its Session Control Block."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The Security Audit thread tried to look up the internal session control
**	block (SCB), but was unable to do so. If this error is returned to the
**	user, there has been a serious internal error.
**
** System Status:
**	The DBMS will terminate.
**
** Recommendation:
**	If this error has been seen by the user, report the problem to
**	Ingres Corporation Technical Support.
*/
E_SX1068_AUD_THREAD_IIDBDB_ERR:SS5000B_INTERNAL_ERROR
"An unexpected error occurred when the Security Audit Thread tried to open 
the IIDBDB database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The Security Audit thread tried to open the database database,
**	but was unable to do so.  If this error is returned to the user, there
**	has been a serious internal error.
**
** System Status:
**	Depending on the setting of the C2 ONERROR configuration parameter,
**	the DBMS may stop auditing or shut down.
**
** Recommendation:
**	If this error has been seen by the user, report the problem to
**	Ingres Corporation Technical Support.
*/
W_SX1069_AUD_THREAD_NO_IIDBDB:SS5000B_INTERNAL_ERROR
"The Security Audit Thread could not open IIDBDB because it does not exist
or is currently unavailable."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The Security Audit thread tried to open the database database,
**	but the database did not exist or was otherwise inaccessible.
**	Because of this, the security audit state will not be refreshed.
**
** System Status:
**	Processing will continue, but the security audit state will be
**	unchanged.
**
** Recommendation:
**	Create the database database, if necessary, or verify that it is 
**	accessible (check permissions, filesystem is mounted, etc.).
*/
E_SX106A_BAD_POOL_SIZE:SS5000B_INTERNAL_ERROR
"The value specified for the SXF memory pool size is not valid. The 
minimum value that may be specified is 10000 bytes. Adjust this value
before attempting to start the server again."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user is allowed to specify a value for SXF memory pool
**	size in order to override the default value. The value specified
**	must be at least 10000 bytes. If the value specified is too
**	small, this error is raised.
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Increase the value specified for SXF memory pool size and
**	attempt to restart the server.
*/
E_SX106B_MEMORY_ERROR:SS5000B_INTERNAL_ERROR
"An internal memory management error has occurred in SXF. Examine the 
error log to determine the cause of the failure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF uses the ULM memory management routines for internal facility
**	memory management. If a failure occurs in the ULM routines, this error
**	is returned to the caller. An earlier ULM message should describe the
**	nature of the failure.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX106C_SXM_ACCESS_FAILURE:SS5000B_INTERNAL_ERROR
"An internal failure occurred in SXF while attempting to arbitrate MAC
access to ACCESS an object. Please check the error log to determine 
the cause of the failure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error occurred in SXF while attempting to arbitrate
**	MAC access to VIEW an object. An earlier SXF message in the error
**	log should give more details on why this occurred.
**
** System Status:
**	The current operation will fail to complete. Access to the
**	object is denied.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX106D_SXM_WRITE_FAILURE:SS5000B_INTERNAL_ERROR
"An internal failure occurred in SXF while attempting to arbitrate MAC
access to WRITE an object. Please check the error log to determine 
the cause of the failure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error occurred in SXF while attempting to arbitrate
**	MAC access to WRITE an object. An earlier SXF message in the error
**	log should give more details on why this occurred.
**
** System Status:
**	The current operation will fail to complete. Access to the
**	object is denied.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX106E_SXM_WRITE_ACCESS:SS5000B_INTERNAL_ERROR
"An invalid WRITE access method was passed in an SXM_WRITE request.
This is an internal error. Access to the object is denied."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error occurred while attempting to arbitrate the
**	MAC access to write an object. 
**
** System Status:
**	The current operation will fail to complete. Access to the
**	object is denied.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX106F_SXM_ARBITRATE_ERR:SS5000B_INTERNAL_ERROR
"An error occurred while performing MAC arbitration on an object.
Access to the object is denied."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error occurred while attempting to arbitrate the
**	MAC access to an object. 
**
** System Status:
**	The current operation will fail to complete. Access to the
**	object is denied.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX1070_SXM_ARB_SUBJECT_LABEL:SS5000B_INTERNAL_ERROR
"Unable to find the subject (session) security label while trying to
perform MAC arbitration.  Access to the object is denied."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The subject (session) security label could not be found when trying
**	to perform MAC arbitration.
**
** System Status:
**	The current operation will fail to complete. Access to the
**	object is denied.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX1071_SXM_ARB_OBJECT_LABEL:SS5000B_INTERNAL_ERROR
"Unable to find the object security label while trying to perform MAC 
arbitration.  Access to the object is denied."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The object security label could not be found when trying
**	to perform MAC arbitration.
**
** System Status:
**	The current operation will fail to complete. Access to the
**	object is denied.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX1072_SXM_DEF_SUBJECT_LABEL:SS5000B_INTERNAL_ERROR
"Unable to find the subject (session) security label while generating the
default label for an object."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The subject security label could not be found when trying
**	to create the default label for an object.
**
** System Status:
**	The current operation will fail to complete. 
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX1073_SXM_DEF_LABEL:SS5000B_INTERNAL_ERROR
"An error occurred while trying to create the default label for an
object."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while trying to create the default label for
** 	an object.
**
** System Status:
**	The current operation will fail to complete. 
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX1074_SXM_DEF_LABEL_PARM:SS5000B_INTERNAL_ERROR
"A request was made to determine the default security label for an object
but no storage was provided by the caller to return the value. This
is an internal error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF was called to determine the default security label for an object
**	but the output pointer was NULL.
**
** System Status:
**	The current operation will fail to complete. 
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX1075_SXM_DELDOWN_UNDEF:SS5000B_INTERNAL_ERROR
"Unable to determine the action required when a delete_down request is
arbitrated."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF was called to arbitrate a DeleteDown operation but was unable
**	to determine the configuration action specified for DeleteDown.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure. Also,
**	check the system configuration to ensure that the DeleteDown
**	behavior is correctly specified.
*/
E_SX1076_PARM_ERROR:SS5000B_INTERNAL_ERROR
"An invalid or missing parameter was passed to the SXF routine '%0c'"
/*%
** Parameters:
**	%0c - routine name
**
** Explanation:
**	An internal SXF routine was called with an invalid or missing
**	parameter. This indicates a severe coding or operational error.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support.
*/
E_SX1077_SXC_GETDBCB:SS5000B_INTERNAL_ERROR
"Unable to get SXF database control block."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to find an SXF database control block
**	This can be caused by lack of resources or by an internal error.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX1078_SXL_SLCMP_ERROR:SS5000B_INTERNAL_ERROR
"An error was returned by SLcompare when attempting to compare two 
security labels."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to compare two security labels.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX1079_SXL_FIND_ERROR:SS5000B_INTERNAL_ERROR
"An error was detected while attempting to find a security label during an
SXL_FIND operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to find a security label during
**	an SXL_FIND operation.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX107A_SXC_ALTER_SESSION:SS5000B_INTERNAL_ERROR
"An error was detected while attempting to alter session characteristics in
SXF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to alter session characteristics 
**	in SXF.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX107B_SXL_LI_EXPAND_TYPE:SS5000B_INTERNAL_ERROR
"An error was detected while attempting to expand a security label/label index
value to a full value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to expand a security label or
**	label index to a full label/index pair in SXF.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX107C_SXL_COMPARE_ERROR:SS5000B_INTERNAL_ERROR
"An error was detected while attempting to compare two security labels."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to compare two security labels.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX107D_B1_NEEDS_DELETE_DOWN:SS5000B_INTERNAL_ERROR
"A B1 secure DBMS needs to have the DeleteDown configuration parameter 
specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to start a B1-enabled DBMS server, but the 
**	DeleteDown installation security policy was undefined.
**
** System Status:
**	The DBMS will not start.
**
** Recommendation:
**	Correct the configuration parameter, then restart the DBMS.
*/
E_SX107E_INVALID_DELETE_DOWN:SS5000B_INTERNAL_ERROR
"An invalid DeleteDown configuration parameter value was detected. Allowed
values for the DeleteDown security policy are ANY, WRITEDOWN or NEVER."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DeleteDown configuration parameter is set to an invalid value.
**	This determines the security policy of the installation, and must
**	be set for a secure DBMS.
**
** System Status:
**	The DBMS will not start.
**
** Recommendation:
**	Check the configuration parameter and set it to a correct value,
**	then restart the DBMS server.
*/
E_SX107F_SXM_ENCAPSULATE:SS5000B_INTERNAL_ERROR
"An error was detected while attempting to encapsulate a security label 
during arbitration processing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF tried to encapsulate a label, but an unexpected error occurred.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the condition persists, contact
**	Ingres Technical Support.
*/
E_SX1080_SXM_DISJ_ING_HIGH:SS5000B_INTERNAL_ERROR
"A security label would be disjoint to the CA_HIGH label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to access a label which is disjoint with the
**	CA_HIGH label. All security labels must be dominated
**	by CA_HIGH.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation with a valid label, or alter CA_HIGH.
*/
E_SX1081_SXM_DISJ_ING_LOW:SS5000B_INTERNAL_ERROR
"A security label would be disjoint to the CA_LOW label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to access a label which is disjoint with the
**	CA_LOW label. All security labels must dominate
**	CA_LOW.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation with a valid label, or alter CA_LOW.
*/
E_SX1082_SXM_DISJ_DB_LABEL:SS5000B_INTERNAL_ERROR
"A security label would be disjoint to the database security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to access a label which is disjoint with the
**	database label. All security labels in a database must dominate
**	the database label.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation with a valid label.
*/
E_SX1083_B1_NEEDS_CA_HIGH:SS5000B_INTERNAL_ERROR
"CA_HIGH must be defined for a B1 secure installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to start an installation, but the value
**	for CA_HIGH was undefined.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Set the CA_HIGH configuration parameter, then try again.
*/
E_SX1084_B1_NEEDS_CA_LOW:SS5000B_INTERNAL_ERROR
"CA_LOW must be defined for a B1 secure installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to start an installation, but the value
**	for CA_LOW was undefined.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Set the CA_LOW configuration parameter, then try again.
*/
E_SX1085_BAD_CA_HIGH:SS5000B_INTERNAL_ERROR
"CA_HIGH set to invalid/illegal security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The CA_HIGH configuration parameter is set to an illegal
**	security label.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Set the CA_HIGH configuration parameter to a legal label,
**	then try again.
*/
E_SX1086_BAD_CA_LOW:SS5000B_INTERNAL_ERROR
"CA_LOW set to invalid/illegal security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The CA_LOW configuration parameter is set to an illegal
**	security label.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Set the CA_LOW configuration parameter to a legal label,
**	then try again.
*/
E_SX1087_BAD_PROC_SECLABEL:SS5000B_INTERNAL_ERROR
"Unable to determine process security label.\n
The security label of the current process could not be determined. Check
the installation and privileges of its executable programs and
ensure they are correct.  The Secure OS should also be checked for
errors which may have caused this condition."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF was unable to determine the process security label.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1088_PROC_SECLABEL_CAHIGH:SS5000B_INTERNAL_ERROR
"Process security label is not CA_HIGH.\n
This process must be started at CA_HIGH to ensure the integrity of the 
installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The process security label is not CA_HIGH. DBMS servers
**	must be executed at CA_HIGH.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1089_BAD_SECID_OPEN:SS5000B_INTERNAL_ERROR
"Error occurred opening SXF security index mapping catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred opening an SXF security index mapping catalog.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX108A_BAD_SECID_CLOSE:SS5000B_INTERNAL_ERROR
"Error occurred closing SXF security index mapping catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred closing an SXF security index mapping catalog.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX108B_SXL_GETID:SS5000B_INTERNAL_ERROR
"Error occurred getting the id for a security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while getting the ID for a security label.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX108C_BAD_SECID_POSITION:SS5000B_INTERNAL_ERROR
"Error occurred positioning a security label index catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while positioning a security label index catalog.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX108D_BAD_SECID_FETCH:SS5000B_INTERNAL_ERROR
"Error occurred fetching a row from a security label index catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while fetching a row from a security label 
**	index catalog.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX108E_BAD_SECID_INSERT:SS5000B_INTERNAL_ERROR
"Error occurred inserting a row into a security label index catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while inserting a row into a security label 
**	index catalog.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX108F_BAD_SECID_DELETE:SS5000B_INTERNAL_ERROR
"Error occurred deleting a row in a security label index catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while deleting a row in a security label 
**	index catalog.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1090_SXL_GETLABEL:SS5000B_INTERNAL_ERROR
"Error occurred getting the label for a security label id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while getting the label for a security label ID.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1091_SXT_UBEGIN:SS5000B_INTERNAL_ERROR
"Error starting SXF user transaction"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while beginning an SXF user transaction.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1092_SXT_UEND:SS5000B_INTERNAL_ERROR
"Error ending SXF user transaction"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while ending an SXF user transaction.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1093_SXL_NEW_ID:SS5000B_INTERNAL_ERROR
"Error occurred generating new security label id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while generating a new identifier for a security
**	label.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1094_SXL_DELETE:SS5000B_INTERNAL_ERROR
"Error occurred deleting old security label id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while deleting an old security label identifier.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1096_SXC_SOP_ADD:SS5000B_INTERNAL_ERROR
"Error occurred attempting to add Security Operations extension to ADF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to add the Security operations
**	extension to ADF.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1097_SXL_COLLATE_ERROR:SS5000B_INTERNAL_ERROR
"Error occurred while attempting to Collate two security labels."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to collate two security labels.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Technical Support.
*/
E_SX1098_SXL_SLCOLL_ERROR:SS5000B_INTERNAL_ERROR
"An error was returned by SLcollate when attempting to collate two 
security labels."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to collate two security labels.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX1099_SXLC_FIND_LABINFO:SS5000B_INTERNAL_ERROR
"The SXF security label cache detected an error while looking up label 
information in the cache."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to look up security label
**	information in the SXF label cache.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX109A_SXLC_ADD_LABINFO:SS5000B_INTERNAL_ERROR
"The SXF security label cache detected an error while adding new label 
information to the cache."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to add security label information
**	to the SXF label cache.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX109B_HASH_ERROR:SS5000B_INTERNAL_ERROR
"An internal hash table management error has occurred in SXF. Examine the 
error log to determine the cause of the failure."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF uses the ULH hash table management routines for internal facility
**	cache and hash table management. If a failure occurs in the ULH 
**	routines this error is returned to the caller. An earlier ULH message 
**	should describe the nature of the failure.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX109C_SXL_ID_NOT_FOUND:SS5000B_INTERNAL_ERROR
"A security label id could not be converted to a security label by SXF.
Check the error log (errlog.log) for information why this 
occurred. This could indicate a corrupted label/id mapping catalog or
an invalid id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF was called to convert a security label ID to a security label,
**	but the label was not found.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX109D_SXL_INVALID_LABEL:SS5000B_INTERNAL_ERROR
"An invalid security label was passed to an SXL internal routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SXL module of SXF was called with an invalid security label.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX109E_SXL_EXPAND_LABINFO:SS5000B_INTERNAL_ERROR
"Error expanding security label/id information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SXL module of SXF got an error trying to expand security 
**	or ID information.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX109F_SXL_LAB_TO_ID:SS5000B_INTERNAL_ERROR
"An error occurred while converting a security label to a label id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SXL module of SXF got an error trying to convert a security label
**	to an ID.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX10A0_SXL_ID_TO_LAB:SS5000B_INTERNAL_ERROR
"An error occurred while converting a security label id to a security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SXL module of SXF got an error trying to convert a security label
**	ID to a security label.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX10A1_SXF_EXCEPTION:SS5000B_INTERNAL_ERROR
"An SXF exception was signaled."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an advisory message.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Report this error to Ingres Corporation Technical Support.
*/
E_SX10A2_SXL_LUB_ERR:SS5000B_INTERNAL_ERROR
"An error occurred while generating the LUB of two security label values."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SXL module of SXF got an error trying to generate the LUB
**	of two security labels.
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX10A3_SXAP_BAD_SECLABEL:SS5000B_INTERNAL_ERROR
"An invalid security label was specified for a security audit record."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SXAP module of SXF detected an illegal security label for
**	a security audit record. 
**
** System Status:
**	The current operation will fail to complete.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/

E_SX10A4_SXS_AUD_LABEL_LIMIT:SS5000B_INTERNAL_ERROR
"The number of security levels with auditing enabled exceeds the current
limit. Please reconfigure your DBMS to allow more security levels to
be audited, then restart the DBMS."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error indicates that too many security levels are enabled for
**	audit, using ENABLE SECURITY_AUDIT. 
**
** System Status:
**	The operation is rejected. Depending on the ON_ERROR state, the
**	system may shut down or stop auditing.
**
** Recommendation:
**	Increase the DBMS configuration parameter determining the audit label
**	count limit, then restart the DBMS.
*/
E_SX10A5_SXLC_INIT_DBC:SS5000B_INTERNAL_ERROR
"An error occurred attempting to initialize the security label cache for
a database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	While initializing the security label cache for a database, an
**	error occurred. The error log should have more detailed
**	information on the exact error.
**
** System Status:
**	The operation is rejected.
**	
** Recommendation:
**	Examine the error log to determine the cause of the failure.
*/
E_SX10A6_STOPAUDIT_LOGFULL:SS5000B_INTERNAL_ERROR
"All security audit log files are full, and the ONLOGFULL configuration
parameter is set to STOPAUDIT so security auditing is being stopped."
/*%
** Parameters:
**	None.
**
** Explanation:
**	All security audit log files are full, and the ONLOGFULL configuration
**	parameter is set to STOPAUDIT, so security auditing is being disabled.
**
** System Status:
**	Security auditing stops.
**
** Recommendation:
**	None. This message is informational only.
*/
E_SX10A7_NO_CURRENT_FILE:SS5000B_INTERNAL_ERROR
"The CURRENT security audit log cannot be opened because security auditing
is disabled and there is no current audit log."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to open the CURRENT security audit log but
**	there is no such log because security auditing is currently disabled.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Re-enable security auditing to access the security audit log.
*/
E_SX10A8_SXLC_FIND_CMP:SS5000B_INTERNAL_ERROR
"The SXF security label cache detected an error while looking up a
label comparison in the cache."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to look up security label 
**	comparison information in the SXF label cache.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
E_SX10A9_SXLC_ADD_CMP:SS5000B_INTERNAL_ERROR
"The SXF security label cache detected an error while adding a
label comparison to the cache."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to add security label 
**	comparison information in the SXF label cache.
**
** System Status:
**	The operation will be rejected.
**
** Recommendation:
**	Examine the error log to determine the cause of the failure.
**	Contact Ingres Corporation Technical Support, if the error persists.
*/
I_SX10AA_AUDIT_SUSPEND:SS5000B_INTERNAL_ERROR
"Security Auditing has been suspended by %0c.
\nAttempts to write security audit records will wait until security auditing
\nis resumed by a privileged user."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	This message is provided to inform the user when security auditing
**	has been suspended.
**
** System Status:
**	Security auditing suspends.
**
** Recommendation:
**	Examine why security auditing was suspended, then resume auditing
**	when the condition has been resolved.
*/
I_SX10AB_AUDIT_USER_STOP:SS5000B_INTERNAL_ERROR
"Security Auditing has been stopped by %0c.
\nNo security audit records will be written until security auditing is
restarted."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	This message is provided to inform the user when security auditing
**	has been stopped.
**
** System Status:
**	Security auditing stops.
**
** Recommendation:
**	Examine why security auditing was stopped, then restart auditing
**	when the condition has been resolved.
*/
I_SX10AC_AUDIT_RESUME:SS5000B_INTERNAL_ERROR
"Security Auditing has been resumed by %0c."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	This message is provided to inform the user when security auditing
**	has been resumed.
**
** System Status:
**	Security auditing resumes.
**
** Recommendation:
**	None.
*/
I_SX10AD_AUDIT_RESTART:SS5000B_INTERNAL_ERROR
"Security Auditing has been restarted by %0c."
/*%
** Parameters:
**	%0c - user name
**
** Explanation:
**	This message is provided to inform the user when security auditing
**	has been restarted.
**
** System Status:
**	Security auditing restarts.
**
** Recommendation:
**	None.
*/
E_SX10AE_SXAP_AUDIT_SUSPEND:SS5000B_INTERNAL_ERROR
"Suspend security audit"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message should not normally be seen externally. It is used to
**	signal from the physical to logical SXF layers that security auditing
**	should be suspended.
**
** System Status:
**	Security audit suspends.
**
** Recommendation:
**	If this message is ever seen by the user, please contact 
**	Ingres Corporation Technical Support.
*/	
E_SX10AF_SXAP_STOP_CLOSE_ERR:SS5000B_INTERNAL_ERROR
"Error occurred attempting to close the current audit file while 
stopping security audit."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to close the current audit file
**	while stopping security auditing.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information on why this occurred.
**
*/	
E_SX10B0_SXAP_RESTART_INIT_ERR:SS5000B_INTERNAL_ERROR
"Error occurred attempting to find an initial logfile to restart security
auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to find an initial security audit
**	log file when restarting auditing.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information on why this occurred.
*/	
E_SX10B1_SXAP_RESTART_CREATE:SS5000B_INTERNAL_ERROR
"Error occurred attempting to find create new logfile 
when restarting security auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to create the new security
**	audit log when restarting security auditing.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information on why this occurred.
*/	
E_SX10B2_SXAP_RESTART_OPEN:SS5000B_INTERNAL_ERROR
"Error occurred attempting to find open new logfile when restarting 
security auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to open the new security
**	audit log when restarting security auditing.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Check the error log for more information on why this occurred.
*/	
E_SX10B3_INSERT_SECID_DUP_KEY:SS5000B_INTERNAL_ERROR
"Duplicate key error when inserting new security label id"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A duplicate key error was returned while inserting a new
**	security label ID.
**
** System Status:
**	The operation is retried, then rejected on continued failure.
**
** Recommendation:
**	Check the error log for more information on why this occurred.
*/	
E_SX10B4_OSAUDIT_WRITE_FAILED:SS5000B_INTERNAL_ERROR
"An attempt to write a record to the operating system audit trail failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was unable to write a record to the operating system
**	audit trail.
**
** System Status:
**	Either auditing will be stopped or the DBMS will be shut down,
**	this is dependent on how the parameter ON_ERROR is configured in
**	the C2 screen of CBF.
**
** Recommendation:
**	Look for further errors in the error log, or in
**	the operating system error log for more details.
*/
E_SX10B5_OSAUDIT_FLUSH_FAILED:SS5000B_INTERNAL_ERROR
"An attempt to flush records to the operating system audit trail failed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was unable to flush audit records to the operating system
**	audit trail.
**
** System Status:
**	Either auditing will be stopped or the DBMS will be shut down,
**	this is dependent on how the parameter ON_ERROR is configured in
**	the C2 screen of CBF.
**
** Recommendation:
**	Look for further errors in the error log, or in
**	the operating system error log for more details.
*/
E_SX10B6_OSAUDIT_OPEN_FAILED:SS5000B_INTERNAL_ERROR
"An attempt to open an operating system audit trail failed."
/*%
** Parameters:
**	None.
** Explanation:
**	The DBMS was unable to open an operating system audit trail.
**
** System Status:
**	Either auditing will be stopped or the DBMS will be shut down,
**	this is dependent on how the parameter ON_ERROR is configured in
**	the C2 screen of CBF.
**
** Recommendation:
**	Look for further errors in the error log, or in
**	the operating system error log for more details.
*/
E_SX10B7_BAD_AUDIT_METHOD:SS5000B_INTERNAL_ERROR
"The audit_mechanism configuration parameter has a bad value,
allowed values are CA or SYSTEM."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The AUDIT_METHOD configuration parameter had a bad value.
**
** System Status:
**	The operation will not continue.
**
** Recommendation:
**	Correct the configuration parameter for AUDIT_METHOD and try again.
*/
E_SX10B8_OSAUDIT_NO_CHANGEFILE:SS5000B_INTERNAL_ERROR
"An attempt was made to change the current audit logfile, this operation is
not allowed when using operating system auditing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user tried to change the the current audit logfile, this operation
**	is not allowed when using operating system auditing.
**
** System status:
**	The operation will fail.
**
** Recommendation:
**	Do not try to change the current audit logfile via database tools
**	when using operating system auditing, please use the operating 
**	system tools.
*/
E_SX10B9_BAD_MO_ATTACH:SS5000B_INTERNAL_ERROR
"Error attaching SXF managed objects to MO."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SXF tried to attach its managed objects to MO but an unexpected 
**	error occurred.
**
** System status:
**	The operation will fail.
**
** Recommendation:
**	Retry the operation. If condition persists contact your support
**	representative.
*/
E_SX10BA_OSAUDIT_NOT_ENABLED:SS5000B_INTERNAL_ERROR
"The operating system audit interface has not been enabled or is non functional
on this system."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user has tried to enable an audit mechanism that will write
**	to the operating system audit trail. Either the operating system 
**	interface has not been enabled on this system or it is not functional
**	for this version of the operating system.
**
** System status:
**	Auditing will not be enabled and may cause the server to shut down.
**
** Recommendation:
**	Either enable operating system auditing on your system, or use an
**	audit mechanism that will write to installation audit logs.
*/
E_SX10BB_SXAP_CHK_PROTO_INFO:SS5000B_INTERNAL_ERROR
"SXAP Stamp protocol information: \n
   Server stamp value:            [%0d, %1d],\n
   SXAP shared buffer stamp value:[%2d, %3d],\n
   Lock stamp value:              [%4d, %5d]"
/*%
** Parameters:
**	audit file protocol stamps
**
** Explanation:
**	This is a traceback message adding extra information on the SXAP
** 	log file stamps following a protocol error. 
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please preserve this information and provide to technical support.
*/
E_SX10BC_SXAP_SHM_BAD_VER:SS5000B_INTERNAL_ERROR
"Invalid version found for SXAP shared memory buffer.
The shared memory version is %0d, but the program version is %1d."
/*%
** Parameters:
**	Program and shared memory version numbers.
**
** Explanation:
**	This error indicates a mismatch in version between the 
** 	security audit shared memory buffer and the current SXAP version.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Ensure all executables in your installation are current.
*/
E_SX10BD_SXAP_SHM_BUF_COUNT:SS5000B_INTERNAL_ERROR
"Mismatch in number of SXAP shared memory buffers. 
The number in shared memory is %0d but the program is configured for %1d"
/*%
** Parameters:
**	Program and shared memory buffer counts numbers.
**
** Explanation:
**	This error indicates a mismatch in number of buffers between 
**	the security audit shared memory buffer and the currently
**	configured version. These must match exactly. This may be
**	caused by changing the value of *.*.c2.shared_buffer_count while
**	the installation is active.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Restart the installation to ensure all servers use the
**	same number of buffers.
*/
E_SX10BE_SXAP_SHM_LOCK_ERR:SS5000B_INTERNAL_ERROR
"SXAP error locking shared memory."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred while locking SXAP shared memory.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10BF_SXAP_SHM_UNLOCK_ERR:SS5000B_INTERNAL_ERROR
"SXAP error unlocking shared memory."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred while unlocking SXAP shared memory.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C0_SXAP_QWRITE_ERR:SS5000B_INTERNAL_ERROR
"Error writing an audit record into shared memory queue."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred while writing an audit record into
**	SXAP shared memory.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C1_SXAP_QFLUSH_ERR:SS5000B_INTERNAL_ERROR
"Error flushing auditing records from shared memory queue."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred while flushing records from the
**	SXAP shared memory queue to disk.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C2_SXAP_QFLUSH_BUF_FREE:SS5000B_INTERNAL_ERROR
"Empty shared record buffer detected while performing queue flush operation.
This is an internal protocol error."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred while flushing records from the
**	SXAP shared memory queue to disk, a record buffer was found which
**    	was empty when it had expected to be full.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C3_SXAP_QALLFLUSH_ERR:SS5000B_INTERNAL_ERROR
"Error flushing all auditing records from shared memory queue."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred while flushing all records from the
**	SXAP shared memory queue to disk.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C4_SXAP_QALLFLUSH_BUF:SS5000B_INTERNAL_ERROR
"Empty shared record buffer detected while performing queue all-flush operation.
This is an internal protocol error."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred while flushing all records from the
**	SXAP shared memory queue to disk, a record buffer was found which
**    	was empty when it had expected to be full.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C5_AUDIT_WRITER_NO_SCB:SS5000B_INTERNAL_ERROR
"The Audit Writer Thread was unable to find its session control block."
/*%
** Parameters:
**	none
**
** Explanation:
**	The Audit Writer Thread  was unable to find its session control
**	block.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C6_AUDIT_WRITER_ERROR:SS5000B_INTERNAL_ERROR
"An error was detected by the Audit Writer Thread in SXF."
/*%
** Parameters:
**	none
**
** Explanation:
**	The Audit Writer Thread  detected an error.  
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Check the error log for more details why this
**	condition occurred.
*/
E_SX10C7_AUDIT_WRITE_ERROR:SS5000B_INTERNAL_ERROR
"SXAP error writing audit records by audit writer"
/*%
** Parameters:
**	none
**
** Explanation:
**	SXAP error was found  writing audit records by the audit  writer.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Check the error log for more details why this
**	condition occurred.
*/
E_SX10C8_AUDIT_WRITE_BUF_FREE:SS5000B_INTERNAL_ERROR
"Empty shared record buffer detected by audit writer thread
This is an internal protocol error."
/*%
** Parameters:
**	none
**
** Explanation:
**	Internal error occurred in the audit writer thread while writing
**	SXAP shared memory queue to disk, a record buffer was found which
**    	was empty when it had expected to be full.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Please report this error to your support representative.
*/
E_SX10C9_INVALID_WRITE_FIXED:SS5000B_INTERNAL_ERROR
"An invalid WriteFixed configuration parameter value was detected. Allowed
values for the WriteFixed security policy are AUTOMATIC or REQUEST."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The WriteFixed configuration parameter is set to an invalid value.
**	This determines the security policy of the installation, and must
**	be set for a secure DBMS.
**
** System Status:
**	The DBMS will not start.
**
** Recommendation:
**	Check the configuration parameter and set it to a correct value,
**	then restart the DBMS server.
*/
E_SX10CA_INVALID_UPDATE_FLOAT:SS5000B_INTERNAL_ERROR
"An invalid UpdateFloat configuration parameter value was detected. Allowed
values for the UpdateFloat security policy are RECREATE or REPLACE."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The UpdateFloat configuration parameter is set to an invalid value.
**	This determines the security policy of the installation, and must
**	be set for a secure DBMS.
**
** System Status:
**	The DBMS will not start.
**
** Recommendation:
**	Check the configuration parameter and set it to a correct value,
**	then restart the DBMS server.
*/
E_SX10CB_SXAP_SHM_PG_SIZE:SS5000B_INTERNAL_ERROR
"Invalid page size found for SXAP shared memory.
The shared memory page size is %0d, but the current program page size is %1d."
/*%
** Parameters:
**	Program and shared memory page sizes.
**
** Explanation:
**	This error indicates a mismatch in page size between the 
** 	security audit shared memory buffer and the current SXAP version.
**	All servers attached to the SXAP shared memory segment must
**	have the same page size.
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Ensure all executables in your installation are current.
*/
E_SX10CC_SXL_LABEL_ID_INFO:SS5000B_INTERNAL_ERROR
"SXL was unable to process label id with internal values:
	 low_id=%0d 
	high_id=%1d."
/*
** Parameters:
**	 Low/High internal id values
**
** Explanation:
**	This error is a trace-back message giving more details of which
**	specific security label id (short security label) was unable
**	to be processed. 
**
** System status:
**	Operation is rejected.
**
** Recommendation:
**	Check the error log for more information.
*/
I_SX2001_ROLE_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define new ROLE."
I_SX2002_ROLE_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a ROLE."
I_SX2003_DBACCESS_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to grant database access."
I_SX2004_DBACCESS_DROP:SS5000F_TEXTUAL_INFO
"Attempt to revoke database access."
I_SX2005_DBPRIV_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to grant database privileges."
I_SX2006_DBPRIV_DROP:SS5000F_TEXTUAL_INFO
"Attempt to revoke database privileges."
I_SX2007_GROUP_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define new GROUP."
I_SX2008_GROUP_MEM_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to add new member to group."
I_SX2009_GROUP_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a GROUP."
I_SX200A_GROUP_MEM_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove member from, or purge GROUP."
I_SX200B_LOCATION_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a new LOCATION."
I_SX200C_USER_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a new USER."
I_SX200D_USER_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a USER."
I_SX2010_INDEX_DROP:SS5000F_TEXTUAL_INFO
"Attempt to DROP INDEX"
I_SX2011_INDEX_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to CREATE INDEX"
I_SX2012_DBPROC_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create DATABASE PROCEDURE."
I_SX2013_DBPROC_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a DATABASE PROCEDURE."
I_SX2014_VIEW_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a VIEW."
I_SX2015_VIEW_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a VIEW."
I_SX2016_PROT_TAB_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a PROTECTION for a TABLE."
I_SX2017_PROT_DBP_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a PROTECTION for a PROCEDURE."
I_SX2018_PROT_TAB_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a PROTECTION or ALL PROTECTIONS for a TABLE."
I_SX2019_PROT_DBP_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a PROTECTION or ALL PROTECTIONS for a PROCEDURE."
I_SX201A_DATABASE_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access a DATABASE."
I_SX201B_ROLE_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access a ROLE."
I_SX201C_SEC_CAT_UPDATE:SS5000F_TEXTUAL_INFO
"Attempt to directly update security system catalog."
I_SX201D_DBPROC_EXECUTE:SS5000F_TEXTUAL_INFO
"Attempt to access a DATABASE PROCEDURE."
I_SX201E_TABLE_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a TABLE."
I_SX201F_DATABASE_EXTEND:SS5000F_TEXTUAL_INFO
"Attempt to extend a DATABASE."
I_SX2020_PROT_REJECT:SS5000F_TEXTUAL_INFO
"Protection violation when accessing a TABLE."
I_SX2021_ROLE_ALTER:SS5000F_TEXTUAL_INFO
"Attempt to alter a ROLE."
I_SX2022_GROUP_ALTER:SS5000F_TEXTUAL_INFO
"Attempt to alter a GROUP."
I_SX2023_USER_ALTER:SS5000F_TEXTUAL_INFO
"Attempt to alter a USER."
I_SX2024_USER_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to authorize USER."
I_SX2025_TABLE_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a TABLE."
I_SX2026_TABLE_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access a TABLE."
I_SX2027_VIEW_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access a VIEW."
I_SX2028_ALARM_EVENT:SS5000F_TEXTUAL_INFO
"Alarm signaled for TABLE."
I_SX2029_DATABASE_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create a DATABASE."
I_SX202A_DATABASE_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a DATABASE."
I_SX202B_LOCATION_ALTER:SS5000F_TEXTUAL_INFO
"Attempt to alter a LOCATION."
I_SX202C_LOCATION_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a LOCATION."
I_SX202D_ALARM_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create a SECURITY_ALARM."
I_SX202E_ALARM_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a SECURITY_ALARM or ALL SECURITY_ALARMS."
I_SX202F_RULE_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access a RULE."
I_SX2030_PROT_EV_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a PROTECTION for a DBEVENT."
I_SX2031_PROT_EV_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a PROTECTION or ALL PROTECTIONS for a DBEVENT."
I_SX2032_EVENT_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create a DBEVENT."
I_SX2033_SET_USER_AUTH:SS5000F_TEXTUAL_INFO
"Attempt to SET SESSION AUTHORIZATION"
I_SX2034_SET_GROUP:SS5000F_TEXTUAL_INFO
"Attempt to SET GROUP"
I_SX2036_INTEGRITY_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop an INTEGRITY."
I_SX2037_RULE_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create a RULE."
I_SX2038_RULE_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a RULE."
I_SX2039_TBL_COMMENT:SS5000F_TEXTUAL_INFO
"Attempt to COMMENT on a table."
I_SX203A_TBL_CONSTRAINT_ADD:SS5000F_TEXTUAL_INFO
"Attempt to add a CONSTRAINT to a table."
I_SX203B_TBL_CONSTRAINT_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a CONSTRAINT from a table."
I_SX203C_EVENT_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a DBEVENT."
I_SX203D_INTEGRITY_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create an INTEGRITY."
I_SX203E_SCHEMA_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create a SCHEMA."
I_SX203F_SCHEMA_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a SCHEMA."
I_SX2040_TMP_TBL_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create a TEMPORARY TABLE."
I_SX2041_TMP_TBL_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop a TEMPORARY TABLE."
I_SX2042_REMOVE_TABLE:SS5000F_TEXTUAL_INFO
"Attempt to REMOVE an audit log registration."
I_SX2043_GW_REGISTER:SS5000F_TEXTUAL_INFO
"Attempt to REGISTER a gateway table."
I_SX2044_GW_REMOVE:SS5000F_TEXTUAL_INFO
"Attempt to REMOVE a gateway table."
I_SX2045_SEQUENCE_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to create SEQUENCE."
I_SX2046_SEQUENCE_ALTER:SS5000F_TEXTUAL_INFO
"Attempt to alter SEQUENCE."
I_SX2047_SEQUENCE_DROP:SS5000F_TEXTUAL_INFO
"Attempt to drop SEQUENCE."
I_SX2048_SEQUENCE_NEXT:SS5000F_TEXTUAL_INFO
"Attempt to execute NEXTVAL on a SEQUENCE."
I_SX2049_PROT_SEQ_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to define a PROTECTION for a SEQUENCE."
I_SX204A_PROT_SEQ_DROP:SS5000F_TEXTUAL_INFO
"Attempt to remove a PROTECTION or ALL PROTECTIONS for a SEQUENCE."
I_SX2501_CHANGE_AUDIT_PROFILE:SS5000F_TEXTUAL_INFO
"Attempt to CHANGE SECURITY_AUDIT PROFILE"
I_SX2502_ENAB_AUDIT_ALL:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT ALL"
I_SX2503_DISB_AUDIT_ALL:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT ALL"
I_SX2504_ENAB_AUDIT_TABLE:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT TABLE"
I_SX2505_DISB_AUDIT_TABLE:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT TABLE"
I_SX2506_ENAB_AUDIT_VIEW:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT VIEW"
I_SX2507_DISB_AUDIT_VIEW:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT VIEW"
I_SX2508_ENAB_AUDIT_DATABASE:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT DATABASE"
I_SX2509_DISB_AUDIT_DATABASE:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT DATABASE"
I_SX250A_ENAB_AUDIT_USER:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT USER"
I_SX250B_DISB_AUDIT_USER:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT USER"
I_SX250C_ENAB_AUDIT_PROCEDURE:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT PROCEDURE"
I_SX250D_DISB_AUDIT_PROCEDURE:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT PROCEDURE"
I_SX250E_ENAB_AUDIT_SECURITY:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT SECURITY"
I_SX250F_DISB_AUDIT_SECURITY:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT SECURITY"
I_SX2510_ENAB_AUDIT_ALARM:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT ALARM"
I_SX2511_DISB_AUDIT_ALARM:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT ALARM"
I_SX2512_ENAB_AUDIT_DBEVENT:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT DBEVENT"
I_SX2513_DISB_AUDIT_DBEVENT:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT DBEVENT"
I_SX2514_ENAB_AUDIT_RULE:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT RULE"
I_SX2515_DISB_AUDIT_RULE:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT RULE"
I_SX2516_ENAB_AUDIT_LOCATION:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT LOCATION"
I_SX2517_DISB_AUDIT_LOCATION:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT LOCATION"
I_SX2518_ENQUIRE_AUDIT_LOG:SS5000F_TEXTUAL_INFO
"Attempt to determine location of security audit log"
I_SX2519_ENAB_AUDIT_RESOURCE:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT RESOURCE"
I_SX251A_DISB_AUDIT_RESOURCE:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT RESOURCE"
I_SX251B_ENAB_AUDIT_QRYTEXT:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE SECURITY_AUDIT QUERY_TEXT"
I_SX251C_DISB_AUDIT_QRYTEXT:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE SECURITY_AUDIT QUERY_TEXT"
I_SX2701_REGISTER_AUDIT_SECURITY:SS5000F_TEXTUAL_INFO
"Attempt to Register Audit log without Security privilege"
I_SX2702_QUERY_AUDIT_SECURITY:SS5000F_TEXTUAL_INFO
"Attempt to Query an Audit log without Security privilege"
I_SX2703_REGISTER_AUDIT_LOG:SS5000F_TEXTUAL_INFO
"Security Audit Log Registered "
I_SX2704_RAISE_DBEVENT:SS5000F_TEXTUAL_INFO
"RAISE of DBEVENT"
I_SX2705_REGISTER_DBEVENT:SS5000F_TEXTUAL_INFO
"REGISTER for DBEVENT notification"
I_SX2706_REMOVE_DBEVENT:SS5000F_TEXTUAL_INFO
"REMOVE request for DBEVENT notification"
I_SX2707_USER_TERMINATE:SS5000F_TEXTUAL_INFO
"User connection to database terminated"
I_SX2708_USER_MONITOR:SS5000F_TEXTUAL_INFO
"Attempt to MONITOR dbms installation by User"
I_SX2709_AUDITDB:SS5000F_TEXTUAL_INFO
"Attempt to audit database with AUDITDB"
I_SX270A_AUDITDB_TABLE:SS5000F_TEXTUAL_INFO
"Attempt to view TABLE using AUDITDB"
I_SX270B_CKPDB:SS5000F_TEXTUAL_INFO
"Attempt to checkpoint a database using CKPDB"
I_SX270C_ROLLDB:SS5000F_TEXTUAL_INFO
"Attempt to rollforward a database using ROLLFORWARDDB"
I_SX270D_ALTERDB:SS5000F_TEXTUAL_INFO
"Attempt to alter status of a database using ALTERDB"
I_SX270E_LOCK_ESCALATE:SS5000F_TEXTUAL_INFO
"Attempt to ESCALATE to table level lock"
I_SX270F_TABLE_MODIFY:SS5000F_TEXTUAL_INFO
"Attempt to MODIFY a table"
I_SX2710_TABLE_RELOCATE:SS5000F_TEXTUAL_INFO
"Attempt to RELOCATE a table"
I_SX2711_INFODB:SS5000F_TEXTUAL_INFO
"Attempt to get database info using INFODB"
I_SX2712_CKPDB_ENABLE_JNL:SS5000F_TEXTUAL_INFO
"Attempt to ENABLE database journaling with CKPDB"
I_SX2713_CKPDB_DISABLE_JNL:SS5000F_TEXTUAL_INFO
"Attempt to DISABLE database journaling with CKPDB"
I_SX2714_ALTERDB_NOJNL:SS5000F_TEXTUAL_INFO
"Attempt to turn off database journaling with ALTERDB"
I_SX2715_CKPDB_TABLE_JNLON:SS5000F_TEXTUAL_INFO
"Attempt to turn ON journaling for table with CKPDB"
I_SX2716_CKPDB_TABLE_JNLOFF:SS5000F_TEXTUAL_INFO
"Attempt to turn OFF journaling for table with CKPDB"
I_SX2717_TABLE_JNLON:SS5000F_TEXTUAL_INFO
"Attempt to turn ON journaling for a table"
I_SX2718_TABLE_JNLOFF:SS5000F_TEXTUAL_INFO
"Attempt to turn OFF journaling for a table"
I_SX2719_TABLE_SAVE:SS5000F_TEXTUAL_INFO
"Attempt to change SAVE date for a table"
I_SX271A_TABLE_PATCH:SS5000F_TEXTUAL_INFO
"Attempt to PATCH the structure of a table."
I_SX271B_DELETE_DOWN:SS5000F_TEXTUAL_INFO
"Attempt to Delete Down"
I_SX271C_WRITE_DOWN:SS5000F_TEXTUAL_INFO
"Attempt to Write Down"
I_SX271D_WRITE_UP:SS5000F_TEXTUAL_INFO
"Attempt to Write Up"
I_SX271E_INSERT_UP:SS5000F_TEXTUAL_INFO
"Attempt to Insert Up"
I_SX271F_INSERT_DOWN:SS5000F_TEXTUAL_INFO
"Attempt to Insert Down"
I_SX2720_WRITE_FIXED:SS5000F_TEXTUAL_INFO
"Attempt to Write Fixed"
I_SX2721_WRITE_DISJOINT:SS5000F_TEXTUAL_INFO
"Attempt to Write Disjoint label"
I_SX2722_RECORD_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access record in a table"
I_SX2723_RECBYTID_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access record in a table by tid"
I_SX2724_RECBYKEY_ACCESS:SS5000F_TEXTUAL_INFO
"Attempt to access record in a table by key"
I_SX2725_SET_SESS_SECLABEL:SS5000F_TEXTUAL_INFO
"Attempt to alter session security label"
I_SX2726_SET_SESS_PRIV:SS5000F_TEXTUAL_INFO
"Attempt to alter session privileges"
I_SX2727_USER_EXPIRE:SS5000F_TEXTUAL_INFO
"User account is expired"
I_SX2728_LIMIT_SECLABEL:SS5000F_TEXTUAL_INFO
"Session security label not dominated by limiting security label"
I_SX2729_QUERY_TEXT:SS5000F_TEXTUAL_INFO
"Text of Query" 
I_SX272A_TBL_SYNONYM_CREATE:SS5000F_TEXTUAL_INFO
"Attempt to CREATE SYNONYM for table"
I_SX272B_TBL_SYNONYM_DROP:SS5000F_TEXTUAL_INFO
"Attempt to DROP SYNONYM on table"
I_SX272C_TBL_COMMENT:SS5000F_TEXTUAL_INFO
"Attempt to COMMENT on table"
I_SX272D_ALTER_SECAUDIT:SS5000F_TEXTUAL_INFO
"Attempt to ALTER SECURITY_AUDIT configuration"
I_SX272E_CREATE_PROFILE:SS5000F_TEXTUAL_INFO
"Attempt to CREATE PROFILE"
I_SX272F_ALTER_PROFILE:SS5000F_TEXTUAL_INFO
"Attempt to ALTER PROFILE"
I_SX2730_DROP_PROFILE:SS5000F_TEXTUAL_INFO
"Attempt to DROP PROFILE"
I_SX2731_QUERY_ROW_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed QUERY ROW LIMIT"
I_SX2732_QUERY_CPU_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed QUERY CPU LIMIT"
I_SX2733_QUERY_DIO_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed QUERY DIO LIMIT"
I_SX2734_QUERY_PAGE_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed QUERY PAGE LIMIT"
I_SX2735_QUERY_COST_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed QUERY COST LIMIT"
I_SX2736_TABLE_DEADLOCK:SS5000F_TEXTUAL_INFO
"DEADLOCK on TABLE access"
I_SX2737_PAGE_DEADLOCK:SS5000F_TEXTUAL_INFO
"DEADLOCK on PAGE access"
I_SX2738_LOCK_TIMEOUT:SS5000F_TEXTUAL_INFO
"TIMEOUT on LOCK"
I_SX2739_LOCK_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed available LOCKs"
I_SX273A_ALTER_DATABASE:SS5000F_TEXTUAL_INFO
"Attempt to alter a DATABASE"
I_SX273B_QUERY_SYSCAT:SS5000F_TEXTUAL_INFO
"Attempt to directly query system catalog"
I_SX273C_TABLE_STATS:SS5000F_TEXTUAL_INFO
"Attempt to access table statistics"
I_SX273D_ENQUIRE_AUDIT_STATE:SS5000F_TEXTUAL_INFO
"Attempt to determine security audit state"
I_SX273E_SET_SESS_PRIORITY:SS5000F_TEXTUAL_INFO
"Attempt to set session priority"
I_SX273F_SET_SESSION_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to set session limit"
I_SX2740_SET_QUERY_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to set query limit"
I_SX2741_SET_LOCKMODE:SS5000F_TEXTUAL_INFO
"Attempt to set lockmode values for session"
I_SX2742_EVENT_ALARM:SS5000F_TEXTUAL_INFO
"Attempt to link dbevent to security alarm"
I_SX2743_DB_ALARM:SS5000F_TEXTUAL_INFO
"Alarm signaled for DATABASE/INSTALLATION"
I_SX2744_SESSION_IDLE_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed session IDLE time limit"
I_SX2745_SESSION_CONNECT_LIMIT:SS5000F_TEXTUAL_INFO
"Attempt to exceed session CONNECT time limit"
I_SX2746_SESSION_ROLE:SS5000F_TEXTUAL_INFO
"Attempt to set session role"
I_SX2747_DUMPDB:SS5000F_TEXTUAL_INFO
"Attempt to audit a database dump file using DUMPDB."
I_SX2748_SET_SERVER_CLOSED:SS5000F_TEXTUAL_INFO
"Attempt to SET SERVER CLOSED in IIMONITOR"
I_SX2749_SET_SERVER_OPEN:SS5000F_TEXTUAL_INFO
"Attempt to SET SERVER OPEN in IIMONITOR"
I_SX274A_STOP_SERVER:SS5000F_TEXTUAL_INFO
"Attempt to STOP/CRASH SERVER in IIMONITOR"
I_SX274B_REMOVE_SESSION:SS5000F_TEXTUAL_INFO
"Attempt to REMOVE SESSION in IIMONITOR"
I_SX274C_CRASH_SESSION:SS5000F_TEXTUAL_INFO
"Attempt to CRASH SESSION in IIMONITOR"
I_SX274D_SET_SERVER_SHUT:SS5000F_TEXTUAL_INFO
"Attempt to SET SERVER SHUT in IIMONITOR"
I_SX274E_ROLE_GRANT:SS5000F_TEXTUAL_INFO
"Attempt to GRANT access to ROLE"
I_SX274F_ROLE_REVOKE:SS5000F_TEXTUAL_INFO
"Attempt to REVOKE access to ROLE"
I_SX2750_SET_TRACE_POINT:SS5000F_TEXTUAL_INFO
"Attempt to issue SET TRACE POINT"
I_SX2751_SET_NOTRACE_POINT:SS5000F_TEXTUAL_INFO
"Attempt to issue SET NOTRACE POINT"
I_SX2752_RELOCATE:SS5000F_TEXTUAL_INFO
"Attempt to relocate using RELOCATEDB"
I_SX2753_ROW_DEADLOCK:SS5000F_TEXTUAL_INFO
"DEADLOCK on ROW access"
I_SX2755_KILL_QUERY:SS5000F_TEXTUAL_INFO
"Attempt to KILL QUERY in IIMONITOR"

I_SX2800_BAD_ACCESS_TYPE:SS5000F_TEXTUAL_INFO
"BAD_ACCESS_TYPE"
I_SX2801_SELECT:SS5000F_TEXTUAL_INFO
"SELECT"
I_SX2802_INSERT:SS5000F_TEXTUAL_INFO
"INSERT"
I_SX2803_DELETE:SS5000F_TEXTUAL_INFO
"DELETE"
I_SX2804_UPDATE:SS5000F_TEXTUAL_INFO
"UPDATE"
I_SX2805_COPYINTO:SS5000F_TEXTUAL_INFO
"COPY INTO"
I_SX2806_COPYFROM:SS5000F_TEXTUAL_INFO
"COPY FROM"
I_SX2807_SCAN:SS5000F_TEXTUAL_INFO
"SCAN"
I_SX2808_ESCALATE:SS5000F_TEXTUAL_INFO
"ESCALATE"
I_SX2809_EXECUTE:SS5000F_TEXTUAL_INFO
"EXECUTE"
I_SX2810_EXTEND:SS5000F_TEXTUAL_INFO
"EXTEND"
I_SX2811_CREATE:SS5000F_TEXTUAL_INFO
"CREATE"
I_SX2812_MODIFY:SS5000F_TEXTUAL_INFO
"MODIFY"
I_SX2813_DROP:SS5000F_TEXTUAL_INFO
"DROP"
I_SX2814_INDEX:SS5000F_TEXTUAL_INFO
"INDEX"
I_SX2815_ALTER:SS5000F_TEXTUAL_INFO
"ALTER"
I_SX2816_RELOCATE:SS5000F_TEXTUAL_INFO
"RELOCATE"
I_SX2817_CONTROL:SS5000F_TEXTUAL_INFO
"CONTROL"
I_SX2818_AUTHENT:SS5000F_TEXTUAL_INFO
"AUTHENTICATE"
I_SX2819_READ:SS5000F_TEXTUAL_INFO
"READ"
I_SX2820_WRITE:SS5000F_TEXTUAL_INFO
"WRITE"
I_SX2821_READWRITE:SS5000F_TEXTUAL_INFO
"READ/WRITE"
I_SX2822_TERMINATE:SS5000F_TEXTUAL_INFO
"TERMINATE"
I_SX2823_DOWNGRADE:SS5000F_TEXTUAL_INFO
"DOWNGRADE"
I_SX2824_BYTID:SS5000F_TEXTUAL_INFO
"BY TID"
I_SX2825_BYKEY:SS5000F_TEXTUAL_INFO
"BY KEY"
I_SX2826_MOD_DAC:SS5000F_TEXTUAL_INFO
"MODIFY D.A.C"
I_SX2827_REGISTER:SS5000F_TEXTUAL_INFO
"REGISTER"
I_SX2828_REMOVE:SS5000F_TEXTUAL_INFO
"REMOVE"
I_SX2829_RAISE:SS5000F_TEXTUAL_INFO
"RAISE"
I_SX282A_SETUSER:SS5000F_TEXTUAL_INFO
"SET USER"
I_SX282B_REGRADE:SS5000F_TEXTUAL_INFO
"REGRADE"
I_SX282C_WRITEFIXED:SS5000F_TEXTUAL_INFO
"WRITEFIXED"
I_SX2830_QRYTEXT:SS5000F_TEXTUAL_INFO
"QUERYTEXT"
I_SX2831_SESSION:SS5000F_TEXTUAL_INFO
"SESSION"
I_SX2832_QUERY:SS5000F_TEXTUAL_INFO
"QUERY"
I_SX2833_LIMIT:SS5000F_TEXTUAL_INFO
"LIMIT"
I_SX2834_USERMESG:SS5000F_TEXTUAL_INFO
"USER MESSAGE"
I_SX2835_CONNECT:SS5000F_TEXTUAL_INFO
"CONNECT"
I_SX2836_DISCONNECT:SS5000F_TEXTUAL_INFO
"DISCONNECT"

I_SX2900_BAD_EVENT_TYPE:SS5000F_TEXTUAL_INFO
"BAD_EVENT_TYPE"
I_SX2901_DATABASE:SS5000F_TEXTUAL_INFO
"DATABASE"
I_SX2902_APPLICATION:SS5000F_TEXTUAL_INFO
"ROLE"
I_SX2903_PROCEDURE:SS5000F_TEXTUAL_INFO
"PROCEDURE"
I_SX2904_TABLE:SS5000F_TEXTUAL_INFO
"TABLE"
I_SX2905_LOCATION:SS5000F_TEXTUAL_INFO
"LOCATION"
I_SX2906_VIEW:SS5000F_TEXTUAL_INFO
"VIEW"
I_SX2907_RECORD:SS5000F_TEXTUAL_INFO
"ROW"
I_SX2908_SECURITY:SS5000F_TEXTUAL_INFO
"SECURITY"
I_SX2909_USER:SS5000F_TEXTUAL_INFO
"USER"
I_SX2910_LEVEL:SS5000F_TEXTUAL_INFO
"LABEL"
I_SX2911_ALARM:SS5000F_TEXTUAL_INFO
"ALARM"
I_SX2912_RULE:SS5000F_TEXTUAL_INFO
"RULE"
I_SX2913_EVENT:SS5000F_TEXTUAL_INFO
"DBEVENT"
I_SX2914_ALL:SS5000F_TEXTUAL_INFO
"ALL"
I_SX2915_INSTALLATION:SS5000F_TEXTUAL_INFO
"INSTALLATION"
I_SX2916_RESOURCE:SS5000F_TEXTUAL_INFO
"RESOURCE"
I_SX2917_QRYTEXT:SS5000F_TEXTUAL_INFO
"QUERYTEXT"

NO_SQLSTATE_MAPPING

