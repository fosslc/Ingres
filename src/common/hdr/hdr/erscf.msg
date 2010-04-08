SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _SC
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	4-may-1993 (robf)
**	    Added security label errors:
**		E_SC0335_NO_PROC_SEC_LABEL
**		E_SC0336_BAD_SESS_SEC_LABEL
**		E_SC0337_SEC_LABEL_TOO_LOW
**		E_SC0338_SEC_LABEL_TOO_HIGH
**     13-Aug-1993 (fred)
**          Added E_SC0130_LO_DBP_MISMATCH.
**     16-Aug-1993 (fred)
**          Fixed wording of that message :-).
**     13-sep-1993 (stevet)
**          Added E_SC033D_TZNAME_INVALID.
**     18-Oct-1993 (fred)
**          Added E_SC0131_LO_STAR_MISMATCH.
**	22-oct-93 (robf)
**	    Added E_SC033F_INVLD_PRIORITY, E_SC033E_SET_PRIORITY_ERR
**	25-oct-93 (robf)
**	    Added E_SC0340_TERM_THREAD_ADD, E_SC0341_TERM_THREAD_EXIT
**	27-oct-93 (robf)
**	    Added E_SC0342_IDLE_LIMIT_INVALID, E_SC0343_CONNECT_LIMIT_INVALID,
**	          I_SC0344_SESSION_DISCONNECT
**	28-Oct-1993 (daveb)
**	    Added E_SC0345_SERVER_CLOSED
**	5-nov-1993 (bryanp)
**	    Added E_SC0346_IIDBMS_PARAMETER_ERROR.
**	17-nov-1993 (pearl)
**	    Add W_SC0347_CRASH_SESSION and W_SC0347_CRASH_SERVER.
**	02-dec-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	8-dec-93 (robf)
**          Add E_SC0349_GET_DB_ALARMS, E_SC034A_FIRE_DB_ALARMS
**      17-Jan-1994 (daveb)
**          Add E_SC0110_ALLOC_TOO_BIG
**      14-feb-1994 (rblumer)
**          Add E_SC026B_INVALID_CURSOR_MODE
**	22-feb-94 (robf)
**	    Add E_SC034E_PRIV_PROPAGATE_ERROR
**	        E_SC034D_ROLE_NOT_AUTHORIZED
**	        E_SC034C_GET_SESSION_ROLE
**	1-mar-94 (robf)
**          Add E_SC034F_NO_FLAG_PRIV so we can tell what flag was bad.
**	3-mar-94 (robf)
**          Add E_SC0350_B1_NOT_LICENCED.
**	9-mar-94 (robf)
**          Add E_SC0354_EXTPWD_GCA_COMPLETION
**		E_SC0355_EXTPWD_GCA_NOPEER
**		E_SC0356_EXTPWD_GCA_CSSUSPEND
**		E_SC0357_EXTPWD_GCA_FASTSELECT
**		E_SC0358_EXTPWD_GCA_FORMAT
**	11-mar-94 (robf)
**          Add E_SC035B_ROLE_CHECK_ERROR
**	15-mar-94 (robf)
**	    Add E_SC035C_USER_CHECK_EXT_PWD
**	18-mar-94 (robf)
**          Add E_SC035D_SET_ROLE_IN_XACT
**              E_SC035E_LOAD_ROLE_DBPRIV
**		E_SC035F_SET_ROLE_NODBACCESS
**	15-apr-94 (robf)
**          Add E_SC0360_SEC_WRITER_THREAD_ADD
**		E_SC0361_SEC_WRITER_THREAD_EXIT
**      11-April-1994 (jpk)
**	    Change II_AUTHORIZATION references to II_AUTH_STRING.
**	19-apr-94 (robf)
**	    Add E_SC0362_SECLABEL_PARAM_SLTYPE
**  	15-jun-1994 (fred)
**  	    Added E_SC0363_DBP_PARAM_INCONSISTENCY.
**	2-feb-94 (stephenb)
**	    Added E_SC0031_SET_SESS_AUTH_ID_INVALID.
**	24-apr-95 (cohmi01)
**	    Added msgs for IOMASTER server.
**	14-nov-95 (nick)
**	    Add E_SC037B_BAD_DATE_CUTOFF.
**	23-may-96 (stephenb)
**	    Add E_SC037B_REP_QMAN_ADD and E_SC037C_REP_QMAN_EXIT
**	19-jun-1996 (sweeney)
**	    Fix typos.
**	12-sep-96 (nick)
**	    Add back in my change of 14-nov-95 which somebody lost and
**	    alter E_SC037B_REP_QMAN_ADD to E_SC037D_REP_QMAN_ADD to 
**	    resolve the conflict caused by this disappearance.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Added errors for LK callback thread.
**      27-feb-1997 (stial01)
**          Added E_SC0380_SET_LOCKMODE_ROW
**      21-may-1997 (stial01)
**          Changed text of E_SC0380_SET_LOCKMODE_ROW.
**	17-jun-1997 (wonst02)
** 	    Added E_SC0381_SETLG_READONLY_DB for readonly databases.
**	11-aug-1997 (canor01)
**	    Corrected parameters for SC0129.
**      20-aug-1997 (stial01)
**          Updated text of E_SC0380_SET_LOCKMODE_ROW
**	23-sep-1997 (canor01)
**	    Add additional SCF messages for compatibility with Jasmine
**	    and Frontier.
**      18-dec-1997 (stial01)
**          Added E_SC0132_LO_REPEAT_MISMATCH.
**	3-feb-98 (inkdo01)
**	    Added E_SC0256_SORT_THREAD_EXIT.
**	06-may-1998 (canor01)
**	    Add E_SC0382_LICENSE_THREAD_ADD and E_SC0383_LICENSE_THREAD_EXIT.
**      04-Feb-1999 (fanra01)
**	    Add E_SC0384_CLEANUP_THREAD_ADD and E_SC0385_CLEANUP_THREAD_EXIT.
**      25-Feb-1999 (fanra01)
**          Add E_SC0386_LOGTRC_THREAD_ADD and E_SC0387_LOGTRC_THREAD_EXIT.
**	09-mar-1999 (mcgem01)
**	    Add E_SC0388_NO_LICENSE for the case where a server-side
**	    license check fails.  Also cleaned up E_SC034F_NO_FLAG_PRIV
**	    - found in the course of testing the licensing change.
**	09-jun-1999 (somsa01)
**	    Added E_SC051D_LOAD_CONFIG.
**	01-jul-1999 (somsa01)
**	    For E_SC051D_LOAD_CONFIG, print out the server type also.
**      27-May-1999 (hanal04)
**          Added E_SC0387, E_SC0388, E_SC0389 for reporting lock related
**          errlors in SCF. b97083.
**	13-aug-1999 (mcgem01)
**	    Renumbered E_SC0388 to avoid a conflict with 2.5
**      27-May-1999 (hanal04)
**          Added E_SC0387, E_SC0388, E_SC0389 for reporting lock related
**          errlors in SCF. b97083.
**      30-Sep-2002 (hanch04)
**          Added RAAT_NOT_SUPPORTED error message.
**      22-Nov-2002 (hanal04) Bug 107159 INGSRV 1696
**          Added E_SC0390 to log a failure to acquire DDB info.
**	25-jan-2004 (devjo01)
**	    Added E_SC0394_CLUSTER_NOT_SUPPORTED.
**      02-Feb-2004 (horda03) Bug 111047/ INGSRV 2541
**          Added E_SC039A, SET SESSION in an MST.
**	30-Aug-2004 (sheco02)
**	    Cross-integrate 466017 to main and change E_SC0390 to E_SC0393.
**	31-Aug-2004 (schka24)
**	    Try SC0395...
**      05-may-2005 (horda03) Bug 11453/INGSRV 3290
**          Added I_SC051E_IIMONITOR_CMD.
**	09-may-2005 (devjo01)
**	    Add E_SC0396_DIST_DEADLOCK_THR_ADD and
**	    E_SC0397_DIST_DEADLOCK_THR_EXIT
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	24-jul-2005 (devjo01)
**	    Add E_SC0398_GLC_SUPPORT_THR_ADD, E_SC0399_GLC_SUPPORT_THR_EXIT.
**      11-Oct-2006 (hanal04) Bug 115658
**          Remove Computer Associates references.
**      27-dec-06 (kibro01)
**          Change incorrect '%s' to '%<n>c' in error messages
**	25-Feb-2008 (kiria01) b119976
**	    Added E_SC039B_VAR_LEN_INCORRECT
**	Jan 2010 (stephenb)
**	    Batch processing add E_SC02A0_NO_QUERY_TEXT.
*/

#define _SC_CLASS  9


E_SC0000_OK:SS00000_SUCCESS
"System Control Facility operation has been successfully completed."
E_SC0001_OP_CODE:SS5000B_INTERNAL_ERROR
"Operation code to scf_call() was not recognized."
E_SC0002_BAD_SEMAPHORE:SS5000B_INTERNAL_ERROR
"Semaphore has not been initialized or has been corrupted."
E_SC0003_INVALID_SEMAPHORE:SS5000B_INTERNAL_ERROR
"Indicated semaphore has not been initialized or has been corrupted."
E_SC0004_NO_MORE_MEMORY:SS50003_EXHAUSTED_RESOURCE
"No more available to the server."
E_SC0005_LESS_THAN_REQUESTED:SS50003_EXHAUSTED_RESOURCE
"Not all memory requested has been allocated."
E_SC0006_NOT_ALLOCATED:SS5000B_INTERNAL_ERROR
"Attempt to free memory which has not been allocated."
E_SC0007_INFORMATION:SS5000B_INTERNAL_ERROR
"Request for an invalid item of information."
E_SC0008_INFO_TRUNCATED:SS5000B_INTERNAL_ERROR
"Insufficient space for requested information.
The result has been truncated."
E_SC0009_BUFFER:SS5000B_INTERNAL_ERROR
"Buffer provided for information is not addressable."
E_SC000A_NO_BUFFER:SS5000B_INTERNAL_ERROR
"Request for information with no result buffer."
E_SC000B_NO_REQUEST:SS5000B_INTERNAL_ERROR
"Pointer to/portion of SCI blk not addressable."
E_SC000C_BUFFER_LENGTH:SS5000B_INTERNAL_ERROR
"Length of error message is invalid."
E_SC000D_BUFFER_ADDR:SS5000B_INTERNAL_ERROR
"Specified scf_buffer address not accessible."
E_SC000E_AFCN_ADDR:SS5000B_INTERNAL_ERROR
"AIC/PAINE address provided is not accessible."
E_SC000F_IN_USE:SS5000B_INTERNAL_ERROR
"Memory/semaphore not freed by allocator."
E_SC0010_NO_PERM:SS5000B_INTERNAL_ERROR
"Facility not authorized to use SCS_ALWAYS_MASK."
E_SC0011_NESTED_DISABLE:SS5000B_INTERNAL_ERROR
"Attempt to disable AIC's while AIC's are disabled."
E_SC0012_NOT_SET:SS5000B_INTERNAL_ERROR
"Attempt to release a semaphore which is not set."
E_SC0013_TIMEOUT:SS5000B_INTERNAL_ERROR
"Out of time before operation completed."
E_SC0014_INTERRUPTED:SS5000B_INTERNAL_ERROR
"User interrupt while awaiting completion."
E_SC0015_BAD_PAGE_COUNT:SS5000B_INTERNAL_ERROR
"Invalid (<= 0) number of pages requested."
E_SC0016_BAD_PAGE_ADDRESS:SS5000B_INTERNAL_ERROR
"Page address is improperly aligned."
E_SC0017_BAD_SESSION:SS5000B_INTERNAL_ERROR
"Session id to SCF request is neither the current session
nor the DB_NOSESSION value."
E_SC0018_BAD_FACILITY:SS5000B_INTERNAL_ERROR
"Facility id to SCF request is invalid."
E_SC0019_BAD_FUNCTION_CODE:SS5000B_INTERNAL_ERROR
"Function code is invalid."
E_SC0020_WRONG_PAGE_COUNT:SS5000B_INTERNAL_ERROR
"Page count to be freed does not match allocated quantity."
E_SC0021_BAD_ADDRESS:SS5000B_INTERNAL_ERROR
"Attemp to free memory which had not been allocated."
E_SC0022_BAD_REQUEST_BLOCK:SS5000B_INTERNAL_ERROR
"Request block to scf_call() was incorrect format."
E_SC0023_INTERNAL_MEMORY_ERROR:SS5000B_INTERNAL_ERROR
"SCF memory arena has been corrupted."
E_SC0024_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"Internal SCF coding error."
E_SC0025_NOT_YET_AVAILABLE:SS0A000_FEATUR_NOT_SUPPORTED
"Function requested has not yet been implemented.
Contact the development team for scheduling information."
E_SC0026_SCB_ALLOCATE:SS50003_EXHAUSTED_RESOURCE
"Unable to allocate SCB."
E_SC0027_BUFFER_LENGTH:SS5000B_INTERNAL_ERROR
"Invalid buffer length on SCU_INFORMATION call."
E_SC0028_SEMAPHORE_DUPLICATE:SS5000B_INTERNAL_ERROR
"Attempt to set a semaphore which is currently set by requester."
E_SC0029_HOLDING_SEMAPHORE:SS5000B_INTERNAL_ERROR
"Attempt to scd_suspend while holding a semaphore."
E_SC002B_EVENT_REGISTERED:SS50000_MISC_ERRORS
"Already registered to receive dbevent %0c.
You cannot register to receive a dbevent for which you are already registered."
E_SC002C_EVENT_NOT_REGISTERED:SS50000_MISC_ERRORS
"Dbevent registration %0c cannot be removed because it does not exist.
You cannot remove a dbevent registration for a dbevent that is not currently
registered. Check that your REGISTER DBEVENT statement succeeded."
E_SC002D_SET_SESS_AUTH_IN_XACT:SS25000_INV_XACT_STATE
"SET SESSION AUTHORIZATION: This statement is not allowed in a
\nmulti-statement transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to execute a SET SESSION AUTHORIZATION statement
**	while in a multi-statement transaction.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	The current transaction must be completed before this statement may be
**	issued.
*/
E_SC002E_SETROLE_IN_XACT:SS25000_INV_XACT_STATE
"SET [NO]ROLE: This statement is not allowed in a multi-statement transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to execute a SET [NO]ROLE statement while in a
**	multi-statement transaction.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	The current transaction must be completed before this statement may be
**	issued.
*/
E_SC002F_SETGROUP_IN_XACT:SS25000_INV_XACT_STATE
"SET [NO]GROUP: This statement is not allowed in a multi-statement transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to execute a SET [NO]GROUP statement while in a
**	multi-statement transaction.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	The current transaction must be completed before this statement may be
**	issued.
*/
E_SC0030_CHANGESECAUDIT_IN_XACT:SS25000_INV_XACT_STATE
"The security audit state may not be changed while in a multi-statement transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to change the security audit state while in a
**	multi-statement transaction.
**
** System Status:
**	The program continues; the current statement is aborted.
**
** Recommendation:
**	The current transaction must be completed before this statement may be
**	issued.
*/
E_SC0031_SET_SESS_AUTH_ID_INVALID:SS50000_MISC_ERRORS
"Attempt to SET SESSION AUTHORIZATION to a non-existent user."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to use the SET SESSION AUTHORIZATION statement
**	to switch to an invalid user.
**
** System Status:
**	The session continues, but the current query is aborted.
**
** Recommendation:
**	Check in the system catalogs for valid users.
*/
E_SC0100_MULTIPLE_MEM_INIT:SS5000B_INTERNAL_ERROR
"Attempt to initialize memory more than once."
E_SC0101_NO_MEM_INIT:SS5000B_INTERNAL_ERROR
"Attempt to allocate memory before initializing it."
E_SC0102_MEM_CORRUPT:SS5000B_INTERNAL_ERROR
"SCF memory arena has been corrupted."
E_SC0103_MEM_PROTECTION:SS5000B_INTERNAL_ERROR
"Error changing memory protection."
E_SC0104_CORRUPT_POOL:SS5000B_INTERNAL_ERROR
"Corrupted pool block found at %0x."
E_SC0105_CORRUPT_FREE:SS5000B_INTERNAL_ERROR
"Corrupted free block found at %0x."
E_SC0106_BAD_SIZE_REDUCE:SS5000B_INTERNAL_ERROR
"Error reducing virtual size of server."
E_SC0107_BAD_SIZE_EXPAND:SS5000B_INTERNAL_ERROR
"Error expanding virtual size of server."
E_SC0108_UNEXPECTED_EXCEPTION:SS5000B_INTERNAL_ERROR
"Unexpected exception (%0x) trapped."
E_SC0109_MULTI_DB_ADD:SS5000B_INTERNAL_ERROR
"Attempt to add database %0c to the server by session %1x.
That database has already been added to the server."
E_SC010A_RETRY:SS50000_MISC_ERRORS
"Operation should be attempted again."
E_SC010B_SCB_DBCB_LINK:SS5000B_INTERNAL_ERROR
"The linkage between the database control block and the session control block
has been corrupted."
E_SC010C_DB_ADD:SS5000B_INTERNAL_ERROR
"Error adding database.  Name: %0c Owner: %1c Access Mode %2x Flags %3x"
E_SC010D_DB_LOCATION:SS5000B_INTERNAL_ERROR
"Database Location Name: %0c Physical Specification: %1c Flags: %2x"
E_SC010E_DB_DELETE:SS5000B_INTERNAL_ERROR
"Error deleting database.  Name: %0c Owner: %1c Added Id %2x"
E_SC010F_INCOMPLETE_DB_DELETE:SS5000B_INTERNAL_ERROR
"SCF Database deletion incomplete.  Name: %0c Owner: %1c"
E_SC0110_ALLOC_TOO_BIG:SS5000B_INTERNAL_ERROR
"Request to allocate an object bigger than supported by sc0m."
/*%
** Parameters:
**      None
**
** Description:
**      Internal error.  A requested object was bigger than 
**	the maximum supported by the sc0m allocator.
**
** System Status:
**      Allocation will fail.
**
** Recommendation:
**      none.
*/
E_SC0120_DBLIST_ERROR:SS5000B_INTERNAL_ERROR
"SCF Error while attempting known database listing."
E_SC0121_DB_OPEN:SS5000B_INTERNAL_ERROR
"Error opening database.  Name: %0c Owner: %1c Access Mode: %2x Flags %3x"
E_SC0122_DB_CLOSE:SS5000B_INTERNAL_ERROR
"Error closing database.  Name: %0c Owner: %1c"
E_SC0123_SESSION_INITIATE:SS08001_CANT_GET_CONNECTION
"Error initiating session."
E_SC0124_SERVER_INITIATE:SS5000B_INTERNAL_ERROR
"Error initiating Server."
E_SC0125_POOL_ADDITION:SS5000B_INTERNAL_ERROR
"SCF Error adding memory pool."
E_SC0126_SC0M_CHECK:SS5000B_INTERNAL_ERROR
"Error discovered in SCF Memory."
E_SC0127_SERVER_TERMINATE:SS5000B_INTERNAL_ERROR
"Error terminating Server."
E_SC0128_SERVER_DOWN:SS00000_SUCCESS
"Server Normal Shutdown."
E_SC0129_SERVER_UP:SS00000_SUCCESS
"%1c Release %0c Server -- Normal Startup."
E_SC012A_BAD_MESSAGE_FOUND:SS50000_MISC_ERRORS
"Unable to format message number %0x."
E_SC012B_SESSION_TERMINATE:SS5000B_INTERNAL_ERROR
"Error terminating Session."
E_SC012C_ADF_ERROR:SS5000B_INTERNAL_ERROR
"Error encountered initiating ADF."
E_SC012D_DB_NOT_FOUND:SS50000_MISC_ERRORS
"Requested Database not found in SCF's known db list. "
E_SC012E_QEP_INVALID:SS50000_MISC_ERRORS
"Database reorganization activity has rendered the
query plan invalid.  Try again after the db activity is complete.  In case
of dynamic SQL, the statement must be prepared again. "
E_SC012F_RETRY:SS50000_MISC_ERRORS
"Query plan has been rendered invalid.  Retry."
E_SC0130_LO_DBP_MISMATCH:SS50008_UNSUPPORTED_STMT
"Database Procedure Parameters may not be Large Objects."
/*%
** Parameters:
**	None.
**
** System Status:
**	The server continues to operate normally.
**
** Recommendation:
**	Alter the database procedure invocation so that it does not use large
**	objects.
*/
E_SC0131_LO_STAR_MISMATCH:SS50008_UNSUPPORTED_STMT
"STAR does not yet support Large Object query parameters."
/*%
** Parameters:
**	None.
**
** System Status:
**	The server continues to operate normally.
**
** Recommendation:
**	Either alter the application to avoid passing large objects as
**	parameters to STAR, or use DIRECT CONNECT to bypass
**	the STAR database.
*/
E_SC0132_LO_REPEAT_MISMATCH:SS50008_UNSUPPORTED_STMT
"Large Objects may not be used in REPEAT queries."
/*%
** Parameters:
**	None.
**
** System Status:
**	The server continues to operate normally.
**
** Recommendation:
**      Do not use large objects with REPEAT queries.
*/
E_SC0200_SEM_INIT:SS5000B_INTERNAL_ERROR
"Error initializing semaphore."
E_SC0201_SEM_WAIT:SS5000B_INTERNAL_ERROR
"Error waiting for semaphore."
E_SC0202_SEM_RELEASE:SS5000B_INTERNAL_ERROR
"Error releasing semaphore."
E_SC0203_SEM_FREE:SS5000B_INTERNAL_ERROR
"Error freeing semaphore."
E_SC0204_MEMORY_ALLOC:SS5000B_INTERNAL_ERROR
"Error allocating memory."
E_SC0205_BAD_SCS_OPERATION:SS5000B_INTERNAL_ERROR
"Sequencer called incorrectly."
E_SC0206_CANNOT_PROCESS:SS50000_MISC_ERRORS
"An internal error prevents further processing of this query.
\n Associated error messages which provide more detailed information about the
problem can be found in the error log, II_CONFIG:errlog.log"
E_SC0207_UNEXPECTED_ERROR:SS5000B_INTERNAL_ERROR
"Facility returned an undocumented error."
E_SC0208_BAD_PSF_HANDLE:SS5000B_INTERNAL_ERROR
"QSF Handle returned was unusable."
E_SC0209_PSF_REQ_DISTRIBUTED:SS5000B_INTERNAL_ERROR
"PSF requested a distributed query mode."
E_SC020A_UNRECOGNIZED_QMODE:SS50008_UNSUPPORTED_STMT
"The Query Mode %0d is unknown."
E_SC020B_UNRECOGNIZED_TEXT:SS50000_MISC_ERRORS
"Text is '%0c'."
E_SC020C_INPUT_OUT_OF_SEQUENCE:SS50000_MISC_ERRORS
"Sequencer received unrequested input."
E_SC020D_NO_DISTRIBUTED:SS50008_UNSUPPORTED_STMT
"Distributed operations are not supported in this server."
E_SC020E_UNUSED_MSG:SS5000B_INTERNAL_ERROR
"unused"
E_SC020F_LOST_LOCK:SS5000B_INTERNAL_ERROR
"The object lock for the input text is unavailable."
E_SC0210_SCS_SQNCR_ERROR:SS5000B_INTERNAL_ERROR
"Sequencer Internal Error."
E_SC0211_CLF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by CLF."
E_SC0212_ADF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by ADF."
E_SC0213_DMF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by DMF."
E_SC0214_OPF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by OPF."
E_SC0215_PSF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by PSF."
E_SC0216_QEF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by QEF."
E_SC0217_QSF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by QSF."
E_SC0218_RDF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by RDF."
E_SC0219_SCF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by SCF."
E_SC021A_ULF_ERROR:SS5000B_INTERNAL_ERROR
"Error returned by ULF."
E_SC021B_BAD_COPY_STATE:SS5000B_INTERNAL_ERROR
"Saved COPY state is invalid."
E_SC021C_SCS_INPUT_ERROR:SS5000B_INTERNAL_ERROR
"SCS_INPUT internal error."
E_SC021D_TOO_MANY_CURSORS:SS50003_EXHAUSTED_RESOURCE
"Number of open cursors exceeds the local limit."
E_SC021E_CURSOR_NOT_FOUND:SS24000_INV_CURS_STATE
"Specified cursor is not known to the server."
E_SC021F_BAD_SQNCR_CALL:SS5000B_INTERNAL_ERROR
"Internal Error:  Call to sequencer is incorrect."
E_SC0220_SESSION_ERROR_MAX:SS50000_MISC_ERRORS
"Too many errors for this session. "
E_SC0221_SERVER_ERROR_MAX:SS50006_FATAL_ERROR
"Error count for server has been exceeded."
E_SC0222_BAD_SCDNOTE_STATUS:SS5000B_INTERNAL_ERROR
"Bad status passed to SCD_NOTE() --\n
        Status was %0x (%0d), facility was %1x (%1x)."
E_SC0223_BAD_SCDNOTE_FACILITY:SS5000B_INTERNAL_ERROR
"Invalid facility code was passed to scd_note() --\n
        Status was %0x (%0d), facility was %1x (%1x)."
E_SC0224_MEMORY_FREE:SS5000B_INTERNAL_ERROR
"Unable to deallocate SCF memory."
E_SC0225_BAD_GCA_INFO:SS5000B_INTERNAL_ERROR
"Unable to get information about a GCA block."
E_SC0226_BAD_GCA_TYPE:SS50000_MISC_ERRORS
"GCA block has an invalid block type -- %0x (%0d.)."
E_SC0227_MEM_NOT_FREE:SS5000B_INTERNAL_ERROR
"Facility %0x had not freed session memory at end of session."
E_SC0228_INVALID_TRACE:SS50000_MISC_ERRORS
"Invalid SCF trace flag specified."
E_SC0229_INVALID_ATTN_CALL:SS5000B_INTERNAL_ERROR
"Invalid AIC delivered to SCF session."
E_SC022A_SERVER_LOAD:SS50000_MISC_ERRORS
"Insufficient system/server resources to complete the current query.
Consult the system administrator and/or error log."
E_SC022B_PROTOCOL_VIOLATION:SS50000_MISC_ERRORS
"Improper GCA block type (or connection block) was sent."
E_SC022C_PREMATURE_MSG_END:SS50000_MISC_ERRORS
"Internal Protocol Error: Message type %0x (%0d.) ended with a dmm value of %1d."
E_SC022D_BLOCK_NOT_MD_ASSOC:SS50000_MISC_ERRORS
"Internal Protocol Error: First communication block from a new session was type %0x (%0d.).  Must be GCA_MD_ASSOC (2)."
E_SC022E_WRONG_BLOCK_TYPE:SS50000_MISC_ERRORS
"Internal Protocol Error: SCF received block type %0x (%0d.) when expecting type %1x (%1d.)."
E_SC022F_BAD_GCA_READ:SS50000_MISC_ERRORS
"SCF was unable to read a GCF block."
E_SC0230_INVALID_CHG_PROT_LEN:SS50000_MISC_ERRORS
"A protection block was sent with a length (%0d.) not equal to 1."
E_SC0231_INVALID_CHG_PROT_CODE:SS50000_MISC_ERRORS
"A protection block was sent with an invalid code of %0x (%0d.).  This code must be GCA_SUPSYS."
E_SC0232_INVALID_STARTUP_CODE:SS50000_MISC_ERRORS
"A startup code(index %0x (%0d.), value %1x (%1d.)) was sent which is
unknown to the server.  The server will not start."
E_SC0233_SESSION_START:SS50000_MISC_ERRORS
"Session created for %0c (real: %1c) on database %2c for %3c%4c access."
E_SC0234_SESSION_END:SS50000_MISC_ERRORS
"Session ended."
E_SC0235_AVERAGE_ROWS:SS50000_MISC_ERRORS
"On %0d. select/retrieve statements, the average row count returned was %1d."
E_SC0236_FAST_COMMIT_INVALID:SS50000_MISC_ERRORS
"Server must be started with SOLE_SERVER option in order to use Fast Commit."
E_SC0237_FAST_COMMIT_ADD:SS50000_MISC_ERRORS
"Error adding Fast Commit thread to the server."
E_SC0238_FAST_COMMIT_EXIT:SS50000_MISC_ERRORS
"Fast Commit Thread terminated abnormally."
E_SC0239_DBMS_TASK_ERROR:SS50000_MISC_ERRORS
"Internal error initializing a DBMS server task - task type (%0d.) not recognized."
E_SC023A_GCA_LISTEN_FAIL:SS50000_MISC_ERRORS
"Unable to start sessions due to excessive consecutive GCA_LISTEN failures.\n
This server will no longer accept new session requests,\n
and will shut down when all user sessions disconnect."
/*%
** Parameters:
**	None.
**
** System Status:
**	The server enters normal shutdown processing.  No new sessions will
**	be accepted, and the server will terminate when all active user
**	sessions end.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC023B_INCONSISTENT_MSG_QUEUE:SS50000_MISC_ERRORS
"The GCA message queue for this session was inconsistent.\n
All messages for this session will be discarded."
/*%
** Parameters:
**	None.
**
** System Status:
**	The GCA message queue for the thread was found to be inconsistent.
**	All messages on the queue are discarded.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC023C_SXF_ERROR:SS50000_MISC_ERRORS
"An error was returned by SXF (Security eXtensions Facility)."
/*%
** Parameters:
**	None.
**
** System Status:
**	A call to the SXF facility failed.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC023D_SXF_BAD_FLUSH:SS50000_MISC_ERRORS
"An error occurred when flushing security audit data through SXF."
/*%
** Parameters:
**	None.
**
** System Status:
**	A call to flush security audit information through SXF failed.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC023E_SXF_BAD_WRITE:SS50000_MISC_ERRORS
"An error occurred while writing a Security Audit record to SXF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A call to write security audit information through SXF failed.
**
** System Status:
**	Depending on the setting of the SXF error handling, the server might
**	turn off auditing, or it might shut down.
**
** Recommendation:
**	Check the error log file for additional information on why 
**	the write failed.  If the problem persists, contact
**	Technical support.
*/
E_SC023F_QRYTEXT_WRITE:SS50000_MISC_ERRORS
"An error occurred while writing a Query Text security audit record"
/*%
** Parameters:
**	None.
**
** Explanation:
**	A call to write query text security audit information through 
**	SXF failed.
**
** System Status:
**	Depending on the setting of the SXF error handling, the server might
**	turn off auditing, or it might shut down.
**
** Recommendation:
**	Check the error log file for additional information on why 
**	the write failed.  If the problem persists, contact
**	Technical support.
*/
E_SC0240_DBMS_TASK_INITIATE:SS50000_MISC_ERRORS
"Internal error initializing a session for a DBMS server task - Server Task
name '%0c'."
E_SC0241_VITAL_TASK_FAILURE:SS50000_MISC_ERRORS
"A Server Task thread necessary to the server has terminated forcing the
shutdown of the DBMS server - Server Task name '%0c'."
E_SC0242_ALTER_MAX_SESSIONS:SS50000_MISC_ERRORS
"Error altering Max Sessions count to allow for DBMS tasks - old count %0d, new
count %1d."
E_SC0243_WRITE_BEHIND_ADD:SS50000_MISC_ERRORS
"Error adding Write Behind thread to the server."
E_SC0244_WRITE_BEHIND_EXIT:SS50000_MISC_ERRORS
"Write Behind Thread terminated abnormally."
E_SC0245_CANNOT_REGISTER:SS50000_MISC_ERRORS
"Unable to start server, failed to register with the nameserver."
E_SC0246_SCA_SHOW_ERROR:SS50000_MISC_ERRORS
 "Error performing DMF show operation on table %0c."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	This error occurs when the system is unable to correctly find all the
**	user-defined abstract datatype catalogs. The actual error will
**	be contained in the (possible sequence of) DMF error(s) logged just
**	before this message (for this server).
**
** System Status:
**	The server will come up, but with no user-defined datatypes defined.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC0247_SCA_OPEN_ERROR:SS50000_MISC_ERRORS
 "Error performing DMF open operation on table %0c."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	This error occurs when the system is unable to correctly open all the
**	user-defined abstract datatype catalogs. The actual error will
**	be contained in the (possible sequence of) DMF error(s) logged just
**	before this message (for this server).
**
** System Status:
**	The server will come up, but with no user-defined datatypes defined.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC0248_SCA_POSITION_ERROR:SS50000_MISC_ERRORS
 "Error performing DMF position operation on table %0c."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	This error occurs when the system is unable to correctly position
**	itself within the named table.  The actual error will be contained
**	in the (possible sequence of) DMF error(s) logged just before this
**	message (for this server).
**
** System Status:
**	The server will come up, but with no user-defined datatypes defined.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC0249_SCA_READ_ERROR:SS50000_MISC_ERRORS
 "Error performing DMF read operation on table %0c."
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	This error occurs when the system is unable to correctly read all
**	entries in the user-defined abstract datatype catalogs.
**	The actual error will be contained in the (possible sequence of) DMF
**	error(s) logged just before this message (for this server).
**
** System Status:
**	The server will come up, but with no user-defined datatypes defined.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC024A_SCA_CLOSE_ERROR:SS50000_MISC_ERRORS
 "Error performing DMF close operation on table %0c."
/*%
** Parameters:
**	%0c Table on which the error occurred.
**
** Explanation:
**	This error occurs when the system is unable to correctly close the
**	named user-defined abstract datatype catalog.  The
**	actual error will be contained in the (possible sequence of) DMF
**	error(s) logged just before this message (for this server).
**
** System Status:
**	The server will come up, but with no user-defined datatypes defined.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC024B_SCA_ALLOCATE:SS50000_MISC_ERRORS
 "Error allocating memory for user defined datatype initialization."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error occurs as a result of failures while attempting to
**	allocate memory from the System Control Facility (SCF) memory
**	management system.  More detailed error messages concerning the
**	precise failure precede this message in the error log.
**
** System Status:
**	The DBMS server will start up, but no user-defined datatypes will be
**	accessible.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC024C_SCA_DEALLOCATE:SS50000_MISC_ERRORS
 "Error deallocating memory for user defined datatype initialization"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error occurs as a result of failures attempting to deallocate
**	memory which is no longer needed after datatype initialization.  Coding
**	errors are the probable cause. More detailed error messages concerning
**	the precise failure precede this message in the error log.
**
** System Status:
**	The DBMS Server will start up, but other errors may follow.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC024D_SCA_ADDING:SS50000_MISC_ERRORS
 "User Defined Datatype Initialization:\n
       Adding %0d. new datatype entries,\n
              %1d. new datatypes,\n
              %2d. new operators,\n
         and  %3d. new function instances."
/*%
** Parameters:
**	%0d - number of user-defined additions
**	%1d - number of datatype additions
**	%2d - number of operator additions
**	%3d - number of function instances additions
**
** Explanation:
**	This is a status message only, informing the system operator of the
**	new additions to the system.
**
** System Status:
**	The system continues operating normally.
**
** Recommendation:
**	None.
*/
E_SC024E_SCA_NOT_ADDED:SS50000_MISC_ERRORS
 "User Defined Datatypes not added. See previous errors."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This message provides status information only.  Other error messages
**	will have preceded this message in the error log.  These contain the
**	detailed cause of the problem.
**
** System Status:
**	The DBMS server will start up, but no user-defined datatype operations
**	will be available.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC024F_SCA_IMP_ARGUMENTS:SS50000_MISC_ERRORS
 "Number of arguments in IIFI_ARGUMENTS (%0d.)\n
does not agree with the number found in IIFI_ARGUMENTS (%1d.)\n
for function instance %2d.."
/*%
** Parameters:
**	%0d - number of arguments found in the iifi_arguments table
**	%1d - number specified as the correct number in iifunction_instances
**	%2d - function instance ID number
**
** Explanation:
**	This message indicates that the datatype catalogs are inconsistent,
**	either from errors at definition time, or from other, unknown causes.
**
** System Status:
**	The DBMS Server will start up.  No user-defined datatypes will be
**	defined, since the catalogs are inconsistent.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support to determine further 
**	action. Any other out-of-the-ordinary events in the database should 
**	be noted, as well as hardware or other machine problems (disk 
**	problems, etc.).
**
**	The function instance in question should be dropped and recreated using
**	the standard SQL statement for this purpose.
*/
E_SC0250_COPY_OUT_OF_SEQUENCE:SS50000_MISC_ERRORS
"A COPY data block was received when one was not expected (or not received when expected)."
E_SC0251_COPY_CLOSE_ERROR:SS50000_MISC_ERRORS
"Error ending the COPY statement."
E_SC0252_COPY_SYNC_ERROR:SS50000_MISC_ERRORS
"Synchronization error with GCA COPY protocol."
E_SC0253_BAD_COPY_ERRMSG:SS50000_MISC_ERRORS
"Error message for COPY statement not found."
E_SC0254_SERVER_INIT_ADD:SS50000_MISC_ERRORS
 "Error adding the DBMS server initialization thread."
/*%
** Parameters:
**	None.
**
** System Status:
**	The DBMS server will not start.  The server initialization task is
**	necessary to get the server fully operational.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC0255_INCOMPLETE_SEND:SS50000_MISC_ERRORS
"SCC_SEND:  Found terminating message type %0d. when type %1d. was necessary to
complete an incomplete message."
/*%
** Parameters:
**	%0d - terminating message type
**	%1d - expected message type
**
** Explanation:
**	The system has detected an internal protocol error.
**
** System Status:
**	The session may continue.  However, the current query will fail,
**	probably with a frontend protocol failure.
**
** Recommendation:
**	If this has happened with no interrupts or force-abort conditions, it
**	should be reported to Ingres Corporation Technical Support.
*/
E_SC0256_SORT_THREAD_EXIT:SS50000_MISC_ERRORS
"Sort Thread terminated abnormally."
E_SC0260_XENCODE_ERROR:SS50000_MISC_ERRORS
"Error encountered during encryption."
/*%
** Parameters:
**	None.
**
** Explanation:
**	scs_dbdb_info() got a bad return code from scu_xencode()
**	when attempting to encode a role password.
**
** System Status:
**	For superusers, a message is logged, and the session
**	continues with the role in effect.
**
**	For non-superusers, a message is logged and returned,
**	and the session is terminated.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC0261_XENCODE_BAD_PARM:SS50000_MISC_ERRORS
"Invalid parameter received by scu_xencode."
/*%
** Parameters:
**
** Explanation:
**	scu_xencode() received an invalid password as input.
**	Either the pointer to it was null, it was too short,
**	too long, or it contained all blanks.
**
** System Status:
**	This status is returned to either psy_[ca]aplid() or scs_dbdb_info().
**	The caller will issue a traceback message and determine disposition.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support; scu_xencode() should 
**	never see blank passwords.
*/
E_SC0262_XENCODE_BAD_RESULT:SS50000_MISC_ERRORS
"Encryption did not function properly"
/*%
** Parameters:
**	None.
**
** Explanation:
**	After encoding a password, scu_encode() detected that the resultant
**	string had not been altered.
**
** System Status:
**	This status is returned to either psy_[ca]aplid() or scs_dbdb_info().
**	The caller will issue a traceback message and determine disposition.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC0263_SCA_RISK_CONSISTENCY:SS50000_MISC_ERRORS
 "User datatype registration risks consistency.
Databases accessed in this installation may become inconsistent and/or lose data."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The datatypes being registered for use by this server may not be known
**	throughout the installation.  Normally, this is illegal.  However, the
**	user-supplied exit has requested that this check be overridden; this
**	means the recovery system may not understand the format of the data
**	in use. Consequently, it may not be able to correctly recover any data
**	for this installation (in the cases where the data formats disagree
**	enough to cause a major failure).
**
** System Status:
**	The server may start up (barring other errors).  However, all databases
**	in this installation now risk a lack of proper recovery.
**
** Recommendation:
**	None -- this is an informational message only. This request (for risking
**	consistency) is intended for use during the test of datatype code, and
**	then only in a test installation.  As soon as the system moves to
**	production status, this request should be changed.
*/
E_SC0264_SCA_ILLEGAL_MAJOR:SS50000_MISC_ERRORS
 "Illegal User Datatype Major id (%0d./%0x). Major ID
must be greater than zero (0)."
/*%
** Parameters:
**	%0d,x - user-defined datatype ID
**
** Explanation:
**	The system found an illegal value for a major ID. It is displayed as
**	both a decimal (followed by a ".") and as a hexadecimal. The only
**	correct values are greater than 0.
**
** System Status:
**	The system will start up, ignoring the user-defined datatypes.
**
** Recommendation:
**	Correct the value and retry.
*/
E_SC0265_SCA_STATE:SS50000_MISC_ERRORS
 "User define datatype identity: Major %0d./%0x -- Minor
%1d./%1x -- <%2c>"
/*%
** Parameters:
**	%0d,x - user-defined major ID
**	%1d,x - user-defined minor ID
**	%2c   - user-defined string
**
** Explanation:
**	The system is attempting to add the specified user datatypes.  This
**	message is informational -- providing the user-supplied information.
**
** System Status:
**	No change in system status -- this is an informational message only.
**
** Recommendation:
**	None - this is an informational message only.
*/
E_SC0266_USER_SHUTDOWN:SS50000_MISC_ERRORS
 "User requested shutdown on datatype initialization
failure.  Server shutting down."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system has failed to correctly add the user-supplied datatypes (for
**	reasons listed previously in the log file).  The user-supplied
**	information requests that the server not start up if this occurs.
**
** System Status:
**	The server will shut down (that is, not complete the startup).
**
** Recommendation:
**	Correct the problem and retry.
*/
E_SC0267_SCA_ID_UNKNOWN:SS50000_MISC_ERRORS
 "System cannot determine the known ADF identification.
Original ADF is assumed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system cannot determine the user-defined datatype level.
**
** System Status:
**	The system will assume that no user-defined datatypes have been added.
**	Further action will be based upon that assumption.
**
** Recommendation:
**	Correct the problem.  This is probably some sort of installation
**	problem; perhaps a portion of the recovery system has been killed.
*/
E_SC0268_SCA_RISK_INVOKED:SS50000_MISC_ERRORS
 "User datatypes are now in use which may compromise
installation consistency.  Data may be lost."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is approving the use of datatypes based upon the request to
**	override the major and minor IDs in use by the installation.  See
**	message E_SC0263_RISK_CONSISTENCY for more details on the risk.
**
** System Status:
**	Data in this installation may be lost and/or databases may be marked
**	inconsistent.
**
** Recommendation:
**	This is an informational message only; no action is required.  However,
**	it is advisable to remedy this situation as soon as possible, making the
**	installation aware of the datatypes in use and consistent.
*/
E_SC0269_SCA_INCOMPATIBLE:SS50000_MISC_ERRORS
 "The proposed datatype id is incompatible with the
installation level <%0d./%0x,%1d./%1x>.  The server will shutdown."
/*%
** Parameters:
**	%0d,x - major ID of datatypes in use in the installation
**	%1d,x - minor ID of datatypes in use in the installation
**
**	NOTE: <0x80000000,0> is the default.
**
** Explanation:
**	The system cannot use the datatypes being proposed, as they are
**	incompatible with the installation level.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Correct and restart the installation.
**
**	Alternatively, the add_risk_consistency flag can be set, but only if
**	this is a test installation.  SETTING THIS FLAG RISKS THE CONSISTENCY OF
**	DATA THROUGHOUT THE INSTALLATION.
*/
E_SC026A_SCA_REGISTER_ERROR:SS50000_MISC_ERRORS
 "The IIUDADT_REGISTER routine returned a non-zero:NO_GE
status."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to define user datatypes.  In doing so, it
**	calls the iiudadt_register() routine to obtain the definitions.  This
**	routine has returned a non-zero status, which is viewed as an error.
**
** System Status:
**	New datatypes will not be defined.  The system will come up with the
**	vendor-supplied defaults.
**
** Recommendation:
**	Correct the error in the iiudadt_register() routine.
*/
E_SC026B_INVALID_CURSOR_MODE:SS50000_MISC_ERRORS
 "Invalid cursor_update_mode specified: '%0c'.
Valid values are DEFERRED and DIRECT."
/*%
** Parameters:
**	%0c	Invalid mode that was specified.
**
** Explanation:
**	When the server is reading in values from the CONFIG.DAT file,
**	it validates the settings.  CURSOR_UPDATE_MODE has only 2 possible
**	settings:  DEFERRED and DIRECT.
**	The user specified a value different than either of these.
**
** System Status:
**	The server will not start up.  
**
** Recommendation:
**	Either replace the value with a valid one, or remove the line from the
**	config.dat file (and then will default to DEFERRED).
**	Note that ANSI compatibility requires a value of DIRECT (so you can
**	have 2 or more FOR UPDATE cursors opened at the same time using 
**	the default cursor mode).
*/

E_SC0270_NO_EVENT_MESSAGE:SS50000_MISC_ERRORS
"Invalid GCA protocol level for dbevent registration."
/* Mapped to E_QE019A_EVENT_MESSAGE for users */

E_SC0271_EVENT_THREAD:SS50000_MISC_ERRORS
"The SCF alert subsystem event thread has been altered.
The operation code is %0d (0 = REMOVE, 1 = ADD, 2 = MODIFY)."
/*%
** Parameters:
**	%0 - operation code corresponding to action
**
** System Status:
**	A system command has been given to alter the state of the event
**	thread.  This message is not an error, but may be used to resolve
**	dbevent-related errors that are encountered later on.
**
** Recommendation:
**	This is not an error.
*/
E_SC0272_EVENT_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding event thread to SCF alert system."
/*%
** Parameters:
**	None.
**
** System Status:
**	SCF could not initialize the event thread.
**
** Recommendation:
**	If your application does not require dbevent processing you may ignore
**	this error message.
*/
E_SC0273_EVENT_RETRY:SS50000_MISC_ERRORS
"The SCF alert system has failed to connect to the Event Subsystem.
After %0d connect attempts SCF has given up."
/*%
** Parameters:
**	%0d - number of connect attempts
**
** Explanation:
**	The event subsystem (EV), when connected to, must allow multiple
**	servers to "attach" themselves to the shared memory segment
**	associated with the EV.  The current server was not able to attach
**	to the segment because the EV has locked out any other servers.
**	This may be because the shared EV segment is currently being taken
**	down or initialized.
**
** Recommendation:
**	If there are many running servers on your machine, find out if any
**	of them are currently just starting or going down.  A server that
**	could not attach to the EV will not allow dbevent processing.
*/
E_SC0274_NOTIFY_EVENT:SS50000_MISC_ERRORS
"An internal error occurred while trying to notify the application of
a dbevent.  The DBMS alert system attempted to notify you, but found
that you had no queued alert instances."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The alert system attempted to notify the client that a dbevent for
**	which the client is registered was raised.  This failure was due to
**	some form of internal flow failure, where an alert resource was
**	lost when the client was about to receive it.
**
** Recommendation:
**	Apply normal error-handling procedures, as the dbevent notification has
**	been discarded.
*/
E_SC0275_IGNORE_ASTATE:SS50000_MISC_ERRORS
"An internal error occurred while trying to read more data from the
application.  The state of alerts was set to SCS_ASIGNORE.  This state
indicates to ignore GCA completion routines, which will not enable the program
to continue."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When SCF was about to read more data from the client, it noticed that
**	the alert state was set to SCS_ASIGNORE.  This state means that when
**	the read operation would complete it would not have resumed the
**	current session.  Consequently, the current session is terminated.
**
** System Status:
**	The current session is terminated.
**
** Recommendation:
**	Report this problem to the system administrator.  If the DBMS
**	event subsystem was taken down while your application was running,
**	then ignore this error.  Otherwise, report this error to
**	Technical Support.
*/
E_SC0276_CX_RAISE_FAIL:SS50000_MISC_ERRORS
"Event Subsystem: An attempt to raise an alert in a clustered installation
failed due to resource exhaustion in the CX facility."
/*%
** Parameters:
**	none.
**
** Explanation:
**	The event subsystem (EV) was using the CX facility to broadcast
**	an event across the cluster, but the CX facility returned an
**	error code.
**
** System Status:
**	The event raised is not set to either local or remote servers.
**
** Recommendation:
**	Make sure CX facility was properly configured/initialized.
**
**	Make sure events are being efficently processed, so that the number
**	of unprocessed events does not exceed to capacity of the CX
**	sub-system.
*/
E_SC0280_NO_ALERT_INIT:SS50000_MISC_ERRORS
"The SCF alert subsystem has not been initialized.
This is most likely caused by a server initiation error.
Check the server event log for initialization errors."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SCF is attempting to register, remove, or raise an alert (dbevent),
**	but the SCF event subsystem has not been initialized. This can only
**	occur if SCF initialization encountered a memory allocation error
**	or an EV connect error.
**
** Recommendation:
**	All initialization errors are detailed in the server error log.
**	Determine which server the user is currently using and check that
**	server's error log for initialization errors. Any action to take
**	will depend on the error discovered.
*/
E_SC0281_ALERT_ALLOCATE_FAIL:SS50000_MISC_ERRORS
"Memory could not be allocated to register the user dbevent.
This error is caused by too many alerts currently being registered.
Check that users are properly using dbevents or reconfigure the server
with more SCF memory."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SCF could not allocate an SCE_ALERT block to contain the dbevent
**	registration requested by the user. This will occur if the users
**	have allocated too many dbevent registrations or have raised a
**	sufficiently large number of dbevents that have not been
**	transmitted to the target clients. Both types of operations require
**	SCF memory. It is also possible that other SCF memory allocations
**	have reduced the amount of SCF memory available.
**
** Recommendation:
**	Check your application programs to be sure they are not registering
**	an inordinate number of dbevents or raising a large number of dbevents
**	that are not being sent to the client applications. Either correct
**	these applications or allocate more SCF memory.
*/
E_SC0282_RSES_ALLOCATE_FAIL:SS50000_MISC_ERRORS
"Memory could not be allocated to register the user dbevent.
This error is caused by too many alerts currently being registered.
Check that users are properly using dbevents or reconfigure the server
with more SCF memory."
/*%
** Parameters:
**	None.
**
** Explanation:
**	SCF could not allocate an SCE_RSESSION block to contain the dbevent
**	registration requested by the user. This will occur if the users
**	have allocated too many dbevent registrations or have raised a
**	sufficiently large number of dbevents that have not been
**	transmitted to the target clients. Both types of operations require
**	SCF memory. It is also possible that other SCF memory allocations
**	have reduced the amount of SCF memory available.
**
** Recommendation:
**	Check your application programs to be sure they are not registering
**	an inordinate number of dbevents or raising a large number of dbevents
**	that are not being sent to the client applications. Either correct
**	these applications or allocate more SCF memory.
*/
E_SC0283_AINST_ALLOCATE_FAIL:SS50000_MISC_ERRORS
"Memory could not be allocated to raise a dbevent.
The registered client (%0x) will not be notified of dbevent '%1c'.
This error is caused by exhausting SCF memory.
Check that users are properly using dbevents or reconfigure the server
with more SCF memory."
/*%
** Parameters:
**	%0x - session ID
**	%1c - dbevent name
**
** Explanation:
**	SCF could not allocate an SCE_AINST block to contain the dbevent
**	notification requested by the user. This will occur if the users
**	have allocated too many dbevent registrations or have raised a
**	sufficiently large number of dbevents that have not been
**	transmitted to the target clients. Both types of operations require
**	SCF memory. It is also possible that other SCF memory allocations
**	have reduced the amount of SCF memory available.
**
** Recommendation:
**	Check your application programs to be sure they are not registering
**	an inordinate number of dbevents or raising a large number of dbevents
**	that are not being sent to the client applications. Either correct
**	these applications or allocate more SCF memory.
*/
E_SC0284_BUCKET_INIT_ERROR:SS50000_MISC_ERRORS
"The SCF event handling data structures cannot be allocated because the
user specified %0d event buckets; this value is out of bounds.
Restart the server changing the invocation parameter 'events' to a valid value."
/*%
** Parameters:
**	%0d - number of buckets requested by the user
**
** Explanation:
**	The server event thread would not start, because the specified parameter
**	for the number of event buckets is out of bounds. This value must be
**	larger than zero and less than some large but environment-specific
**	value. This parameter only affects the number of hash buckets into
**	which dbevent registrations are placed, so reducing this value should
**	have little effect on the operation of the system.
**
** Recommendation:
**	Either remove this option from the server initialization command,
**	make the value positive (if < 0), or make the value smaller
**	(if very large). NOTE: Values larger than 10,000 for this
**	parameter are thought to make little sense.
*/
E_SC0285_EVENT_MEMORY_ALLOC:SS50000_MISC_ERRORS
"The SCF event handling data structures could not be allocated.
A request was made to allocate %0d bytes (%1d alert buckets).
This error is caused by exhausting SCF memory.
Restart the server with fewer event buckets (or users) or with more SCF memory."
/*%
** Parameters:
**	%0d - total memory (in bytes) required for alert structures
**	%1d - number of buckets requested for server (or number of users)
**
** Explanation:
**	The server event thread would not start, because the SCF alert data
**	structures could not be allocated.
**
** Recommendation:
**	Check what other threads are running and that they are not (already)
**	consuming a lot of SCF memory.  Try to reconfigure your server with
**	more SCF memory.
*/
E_SC0286_EVENT_SEM_INIT:SS50000_MISC_ERRORS
"The SCF event handling data structures could not be initialized.\
A request to initialize semaphore '%0c' failed.
Try to restart the server if dbevent processing is required."
/*%
** Parameters:
**	%0c - internal name of semaphore for SCF alert structures
**
** Explanation:
**	The server event thread would not start, because the SCF alert
**	semaphore could not be initialized.
**
** Recommendation:
**	Check what other threads are running and that they are not (already)
**	consuming a lot of system locks. If dbevent processing is not required,
**	then this error can be ignored.  Otherwise, try to restart the server.
*/
E_SC0287_AINST_SESSION_ID:SS50000_MISC_ERRORS
"The SCF event thread found a non-existing session, %0x, registered for a dbevent."
/*%
** Parameters:
**	%0x - session ID
**
** Explanation:
**	SCF is attempting to notify a client of the occurrence of a dbevent.
**	The client's session ID, however, indicates that the client is no
**	longer active.  This could occur if a user thread was removed through
**	unusual methods and consequently did not remove all of its
**	registrations.
**
** System Status:
**	The system continues processing the dbevent for other registered
**	sessions.
**
** Recommendation:
**	This error is entered into the log file and does not affect user
**	processing.
*/
E_SC0288_XSEV_SM_DESTROY:SS50000_MISC_ERRORS
"Event Subsystem: Shared memory segment '%0c' could not be destroyed.
During server initialization or shutdown an attempt to destroy the
shared event segment failed."
/*%
** Parameters:
**	%0c - internal name of shared event segment
**
** Explanation:
**	The event subsystem (EV) may attempt to destroy the EV shared segment
**	if an EV initialization failed or when the last server disconnects
**	from the EV. If the segment cannot be destroyed other servers will not
**	be able to connect to a refreshed EV.
**
** Recommendation:
**	Check if other servers are starting up or shutting down at the exact
**	same moment.  If so, try to start a new server (to refresh the EV).
**	This error may be ignored if your server will not be processing
**	dbevents on behalf of applications.  Otherwise, report this problem to
**	Ingres Corporation Technical Support.
*/
E_SC0289_XSEV_ALLOC:SS50000_MISC_ERRORS
"Event Subsystem: Shared memory segment could not be allocated.
An attempt to allocate %0d shared pages failed."
/*%
** Parameters:
**	%0d - number of pages
**
** Explanation:
**	The event subsystem (EV), when initialized, attempts to allocate
**	a number of shared memory pages.  The number of pages is a function
**	of the number of users and event buckets of the first server that
**	connects to the EV.
**
** Recommendation:
**	If the number of pages is too high, try to reduce the event-related
**	parameters to the "starting server".  This error may be ignored if
**	your server will not be processing dbevents on behalf of applications.
**	Otherwise, report this problem to Ingres Corporation Technical Support.
*/
E_SC028A_XSEV_CRLOCKLIST:SS50000_MISC_ERRORS
"Event Subsystem: Event lock list could not be created (LKcreate_list).
The server could not be connected to the event subsystem."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The event subsystem (EV), when initialized and when connected to,
**	creates a "lock list" on behalf of the connecting server.  This
**	list allows multiple servers to be notified (through locks) of dbevents
**	that they should process from other servers (through the EV). If the
**	lock list could not be created, then the server cannot be notified.
**	The system, therefore, does not allow any interaction between this
**	server and dbevents.
**
** Recommendation:
**	This error may be ignored if your server will not be processing
**	dbevents on behalf of applications.  Otherwise, report this problem to
**	Ingres Corporation Technical Support.
*/
E_SC028B_XSEV_ATTACH:SS50000_MISC_ERRORS
"Event Subsystem: Unsuccessful attempt to attach to established subsystem.
The server could not attached to shared memory segment '%0c'."
/*%
** Parameters:
**	%0c - internal name of shared event subsystem segment
**
** Explanation:
**	The event subsystem (EV), when connected to, must allow multiple
**	servers to "attach" themselves to the shared memory segment
**	associated with the EV.  The current server was not able to attach
**	to the segment.  The system, therefore, does not allow any interaction
**	between this server and dbevents.
**
** Recommendation:
**	If there are many running servers on your machine, find out if they
**	are all using dbevents.  Maybe some of them can be initialized without
**	dbevents (see the "events" startup qualifier).  This error may be
**	ignored if your server will not be processing dbevents on behalf of
**	applications.  Otherwise, report this problem to Ingres Corporation 
**	Technical Support.
*/
E_SC028C_XSEV_EALLOCATE:SS50000_MISC_ERRORS
"Event Subsystem: Unsuccessful attempt to allocate data structure.
The data structure '%0c' (requiring %1d bytes) could not be allocated.
There are only %2d available bytes left in the subsystem memory pool."
/*%
** Parameters:
**	%0c - internal name of data structures being allocated
**	%1d - data structure size
**	%2d - available space (in bytes) of shared memory pool
**
** Explanation:
**	The event subsystem (EV) could not execute an operation, because
**	all the shared EV space was exhausted. If there are many applications
**	running with many dbevents, then the EV subsystem may have been
**	initialized with too small an amount (determined as a function of
**	the number of users and event buckets of the first server that
**	connected to the EV). Depending on when this error was encountered,
**	it may affect one application or the whole server.
**
** Recommendation:
**	If there are many running servers on your machine, find out if they
**	are all using dbevents.  Maybe some of them can be initialized without
**	dbevents (see the "events" startup qualifier).  This error may be
**	ignored if your server will not be (or is not already) processing
**	dbevents on behalf of applications.  Otherwise, report this problem to
**	Ingres Corporation Technical Support.
*/
E_SC028D_XSEV_REGISTERED:SS50000_MISC_ERRORS
"Event Subsystem: Invalid attempt by server to re-register a dbevent.
This server is already registered for dbevent '%0c' or for ALL dbevents."
/*%
** Parameters:
**	%0c - full internal name of dbevent (with blanks)
**
** Explanation:
**	The event subsystem (EV) was asked to register a server for a dbevent
**	for which the server is already registered.  It is an error to register
**	twice. If the server has registered for all dbevents, then it may not
**	register for a specific dbevent.
**
** Recommendation:
**	If the request to register the dbevent within the server works (i.e.,
**	an application will be notified of the dbevent), then you can ignore
**	this error.  Otherwise, this will impact the behavior of the application
**	and should be reported to Ingres Corporation Technical Support.
*/
E_SC028E_XSEV_NOT_REGISTERED:SS50000_MISC_ERRORS
"Event Subsystem: Invalid attempt by server to de-register a dbevent.
This server is not registered for dbevent '%0c'."
/*%
** Parameters:
**	%0c - full internal name of dbevent (with blanks)
**
** Explanation:
**	The event subsystem (EV) was asked to de-register a server for a dbevent
**	for which the server is not (yet) registered.  It is an error to
**	de-register a nonexisting dbevent.
**
** Recommendation:
**	If the request to remove the dbevent within the server works (i.e.,
**	an application will not be notified of the dbevent), then you can ignore
**	this error.  Otherwise, this will impact the behavior of the application
**	and should be reported to Ingres Corporation Technical Support.
*/
E_SC0290_XSEV_SIGNAL:SS50000_MISC_ERRORS
"Event Subsystem: A dbevent could not be broadcast to server %0x.
An LKevent call failed to broadcast dbevent '%1c' to the registered server."
/*%
** Parameters:
**	%0x - process ID of server registered for the dbevent
**	%1c - full internal name of dbevent (includes blanks)
**
** Explanation:
**	The event subsystem (EV) attempted to raise a dbevent to all servers
**	registered to receive it.  When signaling a specific registered
**	server, the EV encountered an LKevent() error. This is usually an error
**	on the sender's side and not the recipient's side.  This dbevent will
**	not be added to the registered server's event instance queue, with
**	the consequence that registered applications will not receive this
**	dbevent.
**
** Recommendation:
**	This error may be ignored if your server will not be processing
**	dbevents on behalf of applications.  Otherwise, report this problem to
**	Ingres Corporation Technical Support.
*/
E_SC0291_XSEV_NOT_CONNECT:SS50000_MISC_ERRORS
"Event Subsystem: Invalid dbevent request from server %0x.
An attempt to execute the '%1c' operation failed because the server
is not connected to the subsystem."
/*%
** Parameters:
**	%0x - process ID of server executing the operation
**	%1c - internal name of requested operation
**
** Explanation:
**	The event subsystem (EV) may only interact with servers that have
**	formally connected to the EV.  A server that did not formally (or
**	successfully) connect may not execute any operations against the EV
**	(even disconnecting -- though the disconnect error may be ignored).
**
** Recommendation:
**	If this same server had event-related initialization errors (earlier
**	in the error log file), then ignore this subsequent error.  Otherwise,
**	report this problem to Ingres Corporation Technical Support.
*/
E_SC0292_XSEV_SEM_INIT:SS50000_MISC_ERRORS
"Event Subsystem: Unsuccessful attempt to initialize the subsystem.
A request to initialize the cross-process semaphore '%0c' failed.
Try to restart the server if dbevent processing is required."
/*%
** Parameters:
**	%0c - internal name of semaphore
**
** Explanation:
**	The event subsystem (EV) could not be initialized, because an attempt
**	to allocate/clear cross-process semaphores (that control shared memory
**	data structures) failed.
**
** Recommendation:
**	This error may be ignored if your server will not be processing
**	dbevents on behalf of applications.  Otherwise, report this problem to
**	Ingres Corporation Technical Support.
*/
E_SC0293_XSEV_VERSION:SS50000_MISC_ERRORS
"Event Subsystem: Unsuccessful attempt to attach to the subsystem.
Event subsystem '%0c' was established with structural version %1d.
The current server is attempting to connect with version %2d.
All event subsystems in a single installation must be the same version."
/*%
** Parameters:
**	%0c - internal name of shared event subsystem segment
**	%1d - version of already-established event subsystem
**	%2d - event-connection version of server attempting to connect
**
** Explanation:
**	The current server could not connect to the event subsystem (EV),
**	because the server's EV version did not match the already-established
**	version.  Version numbers are modified when internal changes are
**	made to the EV data structures (all allocated out of memory shared
**	among multiple servers).  Normally, all servers in an installation are
**	of the same level.  Internal developers and VARS may have installations
**	constructed with multiple servers of different levels.
**
** Recommendation:
**	This error may be ignored if your server will not be processing
**	dbevents on behalf of applications.  Otherwise, report this problem to
**	Ingres Corporation Technical Support.
*/
E_SC0294_XSEV_CONFIG_LOC:SS50000_MISC_ERRORS
"The Event System config file could not be opened as the system could not
build a path to the config file (events.cnf) in the II_DBMS_CONFIG
directory (error status %0x).  This server will use default values for
event configuration parameters rather than values contained in a config file.
Check the environment variable II_DBMS_CONFIG to correct this warning."
/*%
** Parameters:
**	%0x - CL status returned from NMloc() call
**
** Explanation:
**	The system attempted an NMloc() call to find a pointer to the event
**	config file, events.cnf.  This call returned an error status.
**	The NMloc() call should succeed even if the file does not exist.
**	This error is probably the result of a missing or incorrectly
**	defined environment variable.
**
** System Status:
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
**	If this error is actually evidence of more serious environment setup
**	problems, then it is likely that other portions of the installation 
**	will also encounter problems and the system may not start up.
**
** Recommendation:
**	Check the setting of environment variables.  Check for the existence of 
**	the configuration directory and the events.cnf file.  Check for
**	file and directory access permissions.
*/
E_SC0295_XSEV_CONFIG_OPEN:SS50000_MISC_ERRORS
"The Event System config file (events.cnf) could not be opened and probably
does not exist (error status %0x).  This server will continue to start up,
but will use default values for event configuration parameters.  To create a
config file with non-default parameters, use the Event Utility.  If this has
already been done, check file and directory access permissions on the event
config file.  The event config file is located in the II_DBMS_CONFIG directory."
/*%
** Parameters:
**	%0x - CL status returned from TRset_file() call
**
** Explanation:
**	The server was unable to open the event config file.  This file
**	should exist in the configuration directory and is named events.cnf.
**	This problem is probably caused by the nonexistence of the event config
**	file.  If the file exists, the problem could be caused by file
**	access permissions or an incorrect setting of the configuration
**	directory.
**
** System Status:
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
**	If this error is actually evidence of more serious environment setup
**	problems, then it is likely that other portions of the installation 
**	will also encounter problems and the system may not start up.
**
** Recommendation:
**	Check the setting of environment variables.  Check for the existence of 
**	the configuration directory and the events.cnf file.  Check for
**	file and directory access permissions.
*/
E_SC0296_XSEV_CONFIG_READ:SS50000_MISC_ERRORS
"The server encountered an error reading the event config file (error
status %0x).  The server will start up using default values for event
configuration parameters that it did not find in the config file."
/*%
** Parameters:
**	%0x - CL status returned from TRrequest() call
**
** Explanation:
**	The server encountered an error attempting to read a record from
**	the event config file.  This may indicate that a record of the
**	config file has a bad format.
**
**	A new config file may be created using the Event Utility.
**
**	When this error is encountered, the server will continue starting
**	up, but will use default values for any configuration parameters
**	it did not find in the config file.  Parameters found before the
**	read error will be used.
**
**	The configuration parameters used by the system are written to
**	the server trace file if one is defined at startup time. 
**
** System Status:
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
** Recommendation:
**	Use the Event Utility to examine the event config file and create
**	a new one if necessary.
*/
E_SC0297_XSEV_CONFIG_VALUE:SS50000_MISC_ERRORS
"The server encountered an error reading the event config file - the value
for the parameter %0c (value = %1d) is out of range for this parameter.
The minimum value %2d will be used instead."
/*%
** Parameters:
**	%0c - parameter name
**	%1d - parameter value
**	%2d - substituted value
**
** Explanation:
**	The server encountered an error attempting to read a record from
**	the event config file.  A value for one of the parameters was out
**	of range.  The minimum value for that parameter is substituted
**	instead.
**
** System Status:
**	The server/system will continue to start up.
**
** Recommendation:
**	This error message can be avoided by changing the value in the
**	event config file.
*/
E_SC0298_EVENT_CONFIG:SS50000_MISC_ERRORS
"The SCF alert subsystem could not be initialized due to an error attempting
to read the event system config file.  Check the server error log for
initialization errors."
/*%
** Parameters:
**	None.
**
** Explanation:
**	At server startup time, the server connects to the installation's
**	Event System.  In order to do this, it must open and read the event
**	system's config file.  This action is normally tolerant of errors
**	such as a nonexistent config file, read errors, or data format errors.
**	The reporting of this error probably means that some error condition
**	involving the event config file was overlooked.
**
** System Status:
**	When this error is logged, the server will fail to start up.
**
** Recommendation:
**	The user should look in the installation error log for further
**	detail on why the event config file could not be read.  If this
**	condition cannot be remedied, the user can create a new event
**	config file or start up the server specifying to not use the
**	event system.
*/
E_SC0299_EVENTS_DISABLED:SS50000_MISC_ERRORS
"Warning: The server startup parameters specified to use the %0c Event
System, but the event system is disabled in this installation.  This server
will run without dbevent processing (as if the EVENTS=0 startup option was
specified).  To enable the %1c Event System, use the Event Utility."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The server startup options specified to use dbevents.  The event
**	system config file currently specifies that the event system is
**	disabled in this installation.  The server will start up but will
**	not connect to the event system.  Applications connected to this
**	server will not be able to use alerts or other features which rely
**	on the Event System.  To enable the Event System, use the
**	Event Utility.  After enabling dbevents, the installation must be
**	restarted to bring up the Event System.
**
** System Status:
**	The server will start up, but will not connect to the Event System.
**	Applications connected to this sever will not be able to use alerts
**	or other features which rely on the Event System.
**
** Recommendation:
**	If the user wants to run without dbevents, no action is necessary.
**	Since dbevents were disabled in the event config file, this is probably
**	what was desired.  The user can get rid of this error message by
**	using the EVENTS=0 server startup option.  If the user wants to
**	use dbevents, then the Event System should be enabled using the Event
**	Utility.
*/
E_SC029A_EVENT_CLUSTER_INIT:SS50000_MISC_ERRORS
"This server cannot connect to the Event System because it is not yet
initialized.  In a Cluster Installation, the cluster process (CSP)
must initialize the Event System so that dbevents can be properly signaled
to other nodes.  In this installation, the CSP is not connected to the
Event System.  This probably means that the Event System was not enabled
at the time that the installation was started, but has since been modified
to indicate that dbevents should be used.  To enable the Event System, the
installation on this node must be stopped and restarted.  To start a server
without dbevent connecting to the Event System, use the EVENTS=0 startup option."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The Event System is initialized by the first process which
**	connects to it.  In a cluster installation, this must be the CSP
**	process.  If a server on a cluster installation finds that it is
**	the first connector to the Event System, then it will fail to
**	start up.  This is because it will be unable to properly communicate
**	dbevents to other nodes of the installation.
**
**	This error will most likely occur if the system is initialized while
**	dbevents are not enabled in the installation, and then later the
**	system is modified to specify that dbevents should be used.  Servers
**	that start up after dbevents have been enabled will try to connect
**	to the Event System, generating this error.
**
**	After enabling dbevents in an installation in which they were previously
**	disabled, the entire installation should be brought down and restarted.
**
** System Status:
**	The server will not start up until either dbevents are explicitly
**	turned off, or until the installation is restarted so that the CSP
**	can be connected to the Event System.
**
** Recommendation:
**	To start a server with dbevent handling, restart the installation to
**	connect the CSP to the Event System.
**
**	In order to start a server without restarting the installation, the
**	server must be started with options specifying to not do dbevent
**	processing.  This is done by using the EVENTS=0 startup option, or
**	by turning off dbevents in the installation with the Event Utility.
*/
E_SC029B_XSEV_REQLOCK:SS50000_MISC_ERRORS
"Event Subsystem: Registered server %0x has been disabled from receiving
database events due to errors attempting to probe the status of that server.
This was detected while trying to deliver dbevent '%1c' to that server."
/*%
** Parameters:
**	%0x - process ID of server registered for the dbevent
**	%1c - full internal name of dbevent (includes blanks)
**
** Explanation:
**	The event subsystem (EV) attempted to raise a dbevent to all servers
**	registered to receive it.  Prior to broadcasting to a registered
**	server, the EV encountered an LKrequest() error.  This usually means
**	either the receiving server was abnormally terminated, or the event
**	thread in that server abnormally terminated.  This dbevent will
**	not be added to the registered server's event instance queue, with
**	the consequence that registered applications will not receive this
**	dbevent.  No future attempts will be made to deliver events to
**	the receiving server.
**
** Recommendation:
**	Check the installation error log to determine why the identified server
**	terminated, or why the event thread in that server terminated.  If the
**	identified server still exists, use the sceadmin utility to stop and
**	restart the event thread for that server.  If the server no longer
**	exists, use sceadmin to remove that server's registration from the
**	Event Subsystem.
*/
E_SC029C_XSEV_RLSLOCK:SS50000_MISC_ERRORS
"Event Subsystem: Unable to release connect lock for server %0.
Please report this problem to the product vendor."
/*%
** Parameters:
**	%0 - process ID of server affected by the error
**
** Explanation:
**	The event subsystem encountered an error while attempting to
**	release (LKrelease()) a server connect lock (LK_EVCONNECT).
**	This internal error indicates potential corruption of
**	the event (EV) or locking (LK) subsystem.
**
** Recommendation:
**	Report this problem to Ingres Corporation Technical Support.
*/
E_SC029D_XSEV_PREVERR:SS50000_MISC_ERRORS
"Event Subsystem: Registered server %0x has been disabled from receiving
database events due to previous errors.
This prevents broadcast of dbevent '%1c' to that server."
/*%
** Parameters:
**	%0x - process ID of server registered for the dbevent
**	%1c - full internal name of dbevent (includes blanks)
**
** Explanation:
**	The event subsystem (EV) encountered previous errors while attempting
**	to broadcast events to the indicated server.  As a result, the event
**	subsystem has marked the indicated server ineligible to receive further
**	events.
**
** Recommendation:
**	Check the installation error log to determine why the identified server
**	was disqualified from receiving events. If the identified server still
**	exists, use the sceadmin utility to stop and restart the event thread
**	for that server.  If the server no longer exists, use sceadmin to
**	remove that server's registration from the Event Subsystem.
*/
E_SC029E_XSEV_TSTLOCK:SS50000_MISC_ERRORS
"Event Subsystem: Registered server %0x has been disabled from receiving
database events due to errors attempting to probe the status of that server.
This was detected while trying to deliver dbevent '%1c' to that server."
/*%
** Parameters:
**	%0x - process ID of server registered for the dbevent
**	%1c - full internal name of dbevent (includes blanks)
**
** Explanation:
**	The event subsystem (EV) encountered previous errors while attempting
**	to determine if the indicated server was still active.  As a result,
**	the Event Subsystem has marked the indicated server ineligible to
**	receive further events.
**
** Recommendation:
**	If the identified server still exists, use the sceadmin utility
**	to stop and restart the event thread for that server.  If the
**	server no longer exists, use sceadmin to remove that server's
**	registration from the Event Subsystem.
*/
E_SC029F_XSEV_EVCONNECT:SS50000_MISC_ERRORS
"Event Subsystem: Failed to get the LK_EVCONNECT lock.
The server could not be connected to the event subsystem."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The event subsystem (EV), when initialized and when connected to,
**	obtains an LK_EVCONNECT lock on behalf of the connecting server.
**	This lock allows other servers to detect if this server terminates
**	without formally disconnecting from the EV.
**
** Recommendation:
**	This error may be ignored if your server will not be processing
**	dbevents on behalf of applications.  If events are required,
**	attempt to restart the event thread for this server via the
**	sceadmin utility.  If that fails, ensure sufficient locking
**	resources are available.
*/
E_SC02A0_NO_QUERY_TEXT:SS50000_MISC_ERRORS
"A query has been sent without query text and either there is 
no previous usable query text or the GCA_REPEAT_MASK has not
been set. Please inform your technical support representative."
/*%
** Parameters: None
**
** Explanation:
**	The client has sent a query without query text. In this case
**	the GCA_REPEAT_MASK must be set by the client to indicate that
**	this was intentional; if the mask is set the DBMS will attempt
**	to use the most recently executed query for this session. If
**	no valid query can be found, or if the GCA_REPEAT_MASK has not
**	been set by the client, this message appears and query execution
**	will be aborted.
**
** Reccomendation:
**	This will generally not be an end-user programming error. The
**	Ingres drivers usually keep track of the current state and will
**	only send queries without text when they believe it is valid to
**	do so. This is likely to be a driver logic or GCA communication
**	error. Please inform your technical support representative.
*/


E_SC02C0_CSP_MAIN_ADD:SS50000_MISC_ERRORS
"Fail to start main CSP thread."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the recovery server
**	attempted to start the main CSP thread.
**
** Recommendation:
**	This message should only be possible when running in a
**	clustered configuration, and startup of the local node 
**	should fail if seen.  Contact technical support.
*/

E_SC02C1_CSP_MAIN_EXIT:SS50000_MISC_ERRORS
"Main CSP thread has abended."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Unexpected error within main CSP thread has led to
**	premature thread termination.
**
** Recommendation:
**	This error should result in the local node crashing.  Check
**	any preceding errors for thread failure cause, and contact
**	technical support.
*/

E_SC02C2_CSP_MSGMON_ADD:SS50000_MISC_ERRORS
"Fail to start cluster message monitor thread."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the recovery server
**	attempted to start the cluster message monitor thread.
**
** Recommendation:
**	This message should only be possible when running in a
**	clustered configuration, and startup of the local node 
**	should fail if seen.  Contact technical support.
*/

E_SC02C3_CSP_MSGMON_EXIT:SS50000_MISC_ERRORS
"Cluster support message monitor thread has abended."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Unexpected error within cluster support message monitor thread
**	has led to premature thread termination.
**
** Recommendation:
**	This error should result in the local node crashing.  Check
**	any preceding errors for thread failure cause, and contact
**	technical support.
*/

E_SC02C4_CSP_MSGTHR_ADD:SS50000_MISC_ERRORS
"Fail to start cluster support message thread."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the recovery server
**	attempted to start the cluster message thread.
**
** Recommendation:
**	This message should only be possible when running in a
**	clustered configuration, and startup of the local node 
**	should fail if seen.  Contact technical support.
*/

E_SC02C5_CSP_MSGTHR_EXIT:SS50000_MISC_ERRORS
"Cluster support message thread has abended."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An unexpected error within the cluster support message thread
**	led to premature thread termination.
**
** Recommendation:
**	This error should result in the local node crashing.  Check
**	any preceding errors for thread failure cause, and contact
**	technical support.
*/

E_SC02C6_BAD_RAD_PARAM:SS50000_MISC_ERRORS
"Invalid RAD specification was passed as a server argument. (%c)"
/*%
** Parameters:
**	Bad parameter or "missing" if no parameter was provided.
**
** Explanation:
**	In a NUMA cluster configuration, Target RAD (Resource Affinity
**	Domain) is specified as part of the installation code
**	parameter in the form II.n, where II is the installation
**	code, and 'n' is a positive integer id in the set of Ingres
**	RAD idents which are valid for this system.
**
** Recommendation:
**	This parameter is generated by ingstart, so the only time
**	this might happen would be if the server process is being
**	started by hand for some reason, or there is a configuration
**	problem at either the OS or Ingres level.
**
**	Contact technical support if not user error.
*/

E_SC0300_EXDB_LOCK_REQ:SS50000_MISC_ERRORS
"This operation requires an exclusive database lock."
E_SC0301_LOC_EXISTS:SS50000_MISC_ERRORS
"This location already exists."
E_SC0302_AUDIT_INIT:SS50000_MISC_ERRORS
"Error initializing auditing state.  This server will be terminated."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS has encountered an error when initializing the auditing
**	state for the server.  Either the security state catalog could
**	not be processed, or DMF returned an error when initializing
**	internal control blocks.
**
** System Status:
**	The DBMS server is terminated.
**
** Recommendation:
**	Contact Ingres Corporation Technical Support.
*/
E_SC0303_SCA_CREATE:SS50000_MISC_ERRORS
 "Error creating user defined datatype catalog <%0c>"
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The DBMS has encountered an error creating the system catalog noted.
**
** System Status:
**	The DBMS server will start up, but the system catalogs will not have
**	been converted to support user-defined datatypes.
**
** Recommendation:
**	Correct the error and restart the server.  This is a summary message --
**	the real problem will precede this message in the error log.
*/
E_SC0304_SCA_MODIFY:SS50000_MISC_ERRORS
 "Error modifying user defined datatype catalog <%0c>"
/*%
** Parameters:
**	%0c - table name
**
** Explanation:
**	The DBMS has encountered an error modifying the system catalog noted.
**
** System Status:
**	The DBMS server will start up, but the system catalogs will not have
**	been converted to support user-defined datatypes.
**
** Recommendation:
**	Correct the error and restart the server.  This is a summary message --
**	the real problem will precede this message in the error log.
*/
E_SC0305_SCA_QTXT_CREATE:SS50000_MISC_ERRORS
 "Error adding database query text to the IIQRYTEXT catalog."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to add query text for a system database
**	procedure used to maintain the system.  These procedures are necessary
**	for the correct maintenance of the user-defined datatypes.
**
**	The actual error precedes this message in the error log.
**
** System Status:
**	The DBMS server will start, but no system catalog conversion will have
**	taken place.
**
** Recommendation:
**	Correct the problem and restart the server.
*/
E_SC0306_SCA_DBP_CREATE:SS50000_MISC_ERRORS
 "Error adding database procedure to the IIPROCEDURES catalog"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to add query text for a system database
**	procedure used to maintain the system.  These procedures are necessary
**	for the correct maintenance of the user-defined datatypes.
**
**	The actual error precedes this message in the error log.
**
** System Status:
**	The DBMS server will start, but no system catalog conversion will have
**	taken place.
**
** Recommendation:
**	Correct the problem and restart the server.
*/
E_SC0307_SCA_CONVERTING:SS50000_MISC_ERRORS
 "Beginning conversion of IIDBDB database for User
Defined Datatype Support."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS is attempting the conversion of the system catalogs for
**	user-defined datatype support.  This is done by the first server to 
**	start up after user-defined datatype support has been authorized 
**	(purchased).
**
** Recommendation:
**	This is a status message only.
*/
E_SC0308_SCA_CONVERTED:SS50000_MISC_ERRORS
 "IIDBDB User Defined Datatype Conversion successfully completed."
/*%
** Parameters:
**	None.
**
** System Status:
**	The DBMS has completed the conversion.
**
** Recommendation:
**	This is a status message only.
*/
E_SC0309_SCA_NOT_CONVERTED:SS50000_MISC_ERRORS
 "IIDBDB User Defined Datatype Conversion unsuccessful."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was unsuccessful at converting the system catalogs.  Any
**	problems encountered are logged before this message.  The server will
**	start; new datatype support will be unavailable until a successful 
**	conversion has taken place.  Partial conversion should not occur -- 
**	either the conversion is successful or it is aborted.
**
** Recommendation:
**	Fix the previous problem and retry.  Previous messages in the error log
**	contain the detailed problem information.
*/
E_SC030A_SCA_ALTER:SS50000_MISC_ERRORS
 "Error altering DMF to allow system catalog updates."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS is attempting to convert the system catalogs; to do so, it
**	must create and modify a set of system catalogs.  This altering could 
**	not occur, so the conversion will be aborted.
**
** Recommendation:
**	Correct the problem and retry.  Previous messages in the error log
**	contain the detailed problem information.
*/
E_SC030B_SCA_BEGIN_XACT:SS50000_MISC_ERRORS
 "Error beginning a transaction"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS cannot start a transaction; conversion or reading of datatype
**	catalogs will not occur.
**
** System Status:
**	The server will start without completing this activity.
**
** Recommendation:
**	Correct the problem and retry.  Previous messages in the error log
**	contain the detailed problem information.
*/
E_SC030C_SCA_COMMIT:SS50000_MISC_ERRORS
 "Error committing transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS cannot commit the transaction after converting or reading the
**	system catalogs.
**
** System Status:
**	The server will start; extended datatype support will be unavailable.
**
** Recommendation:
**	Correct the problem and retry.  Previous messages in the error log
**	contain the detailed problem information.
*/
E_SC030D_SCA_ABORT:SS50000_MISC_ERRORS
 "Error aborting transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS cannot abort the transaction after converting or reading the
**	system catalogs.
**
** System Status:
**	The server will start; extended datatype support will be unavailable.
**
** Recommendation:
**	Correct the problem and retry.  Previous messages in the error log
**	contain the detailed problem information.
*/
E_SC030E_SCA_ADD_DB:SS50000_MISC_ERRORS
 "Error performing DMF Add database operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was in the process of looking for user-defined datatype
**	information and was unable to add the system catalogs.
**
** System Status:
**	The DBMS server will start up, but  with no additional datatype
**	information.
**
** Recommendation:
**	Previous messages in the error log file contain the reason for the
**	error. Correct the problem and restart the server.
*/
E_SC030F_SCA_OPEN_DB:SS50000_MISC_ERRORS
 "Error performing DMF open database operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was in the process of looking for user-defined datatype
**	information and was unable to open the system catalogs.
**
** System Status:
**	The DBMS server will start up, but with no additional datatype
**	information.
**
** Recommendation:
**	Previous messages in the error log file contain the reason for the
**	error. Correct the problem and restart the server.
*/
E_SC0310_SCA_CLOSE_DB:SS50000_MISC_ERRORS
 "Error performing DMF Close database operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was in the process of looking for user-defined datatype
**	information and was unable to close the system catalogs.
**
** System Status:
**	The DBMS server will start up with the database left open, assuming
**	the DMF returned a non-fatal error.  The server is left up as an aid
**	in diagnosing the problem.
**
** Recommendation:
**	Previous messages in the error log file contain the reason for the
**	error. Correct the problem and restart the server.
*/
E_SC0311_SCA_DELETE_DB:SS50000_MISC_ERRORS
 "Error performing DMF Delete database operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was in the process of looking for user-defined datatype
**	information and was unable to delete the system catalogs.
**
** System Status:
**	The DBMS server will start up with the database still added, assuming
**	the DMF returned a non-fatal error.  The server is left up as an aid
**	in diagnosing the problem.
**
** Recommendation:
**	Previous messages in the error log file contain the reason for the
**	error. Correct the problem and restart the server.
*/
E_SC0312_SCA_NO_LICENSE:SS50000_MISC_ERRORS
"An attempt was made to add User Defined Datatypes, Operators, or Functions,\
 but this node is not currently licensed to do so."
/*%
** Parameters:
**	None.
**
** Explanation:
**	When the DBMS server attempted to start, the IIudadt_register()
**	function was called, and it returned a non-null pointer, indicating
**	that this node is attempting to use UserADT enhancements. Upon checking
**	the authorization string, the server discovered that this node
**	is not licensed for this functionality.
**
** Recommendation:
**	If you believe this node is licensed for this functionality, check your
**	authorization string to ensure it is current and correctly entered.  If
**	you are not licensed for this functionality, either change the
**	IIudadt_register() function in your IIUSERADT sharable image to return
**	NULL, or contact Ingres Corporation to obtain licensing for the
**	UserADT extension.
*/
E_SC0313_NOT_LICENSED:SS50000_MISC_ERRORS
"An attempt was made to use the `%0c' statement.  This functionality is part of\
 the `%1c' extension which this node is not licensed to use."
/*%
** Parameters:
**	%0c - statement whose execution was attempted
**	%1c - name of DBMS extension containing this feature
**
** System Status:
**	An attempt was made to execute a statement which is listed as an
**	optional feature.  Upon checking the authorization string,
**	the server discovered that this node is not licensed for this
**	feature.
**
** Recommendation:
**	If you believe this node is licensed for this functionality, check your
**	authorization string to ensure it is current and correctly entered.  If
**	you are not licensed for this functionality, either change the
**	application which caused this statement to be issued, or contact
**	Ingres Corporation to obtain licensing for the extension listed in the
**	error message.
*/
E_SC0314_RECOVER_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding the DX recovery thread to the server"
/*%
** Parameters:
**	None.
**
** Explanation:
**	CS failed to add a new thread to perform the DX recovery.
**	This error may show up during STAR server startup.
**
** System Status:
**	This is a fatal error; the server will die.
**
** Recommendation:
**	Make sure the server has enough resources and is configured to
**	start at least the DX recovery thread and a monitor thread,
**	in case someone needs to monitor the server from the DBMS 
**	monitoring process.
*/
E_SC0315_RECOVERING_ORPHAN_DX:SS50000_MISC_ERRORS
"STAR server is not available yet, it is recovering orphan distributed\
 transactions, try again latter."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The STAR server is performing recovery on orphan distributed
**	transactions.
**
** System Status:
**	The server will not be available until the recovery is complete.
**	Only the DBMS monitor can connect to the server and monitor server 
**	activity.
**
** Recommendation:
**	Wait until the TPF recovers all the orphan distributed transactions
**	of the installation.
*/
E_SC0316_LOC_NOT_FOUND:SS50000_MISC_ERRORS
"This database is not extended to location '%0c'."
/*%
** Parameters:
**	%0c - location name
**
** Explanation:
**	The SET WORK LOCATIONS command was issued with at least one work
**	location which doesn't exist in this database's list of locations.
**	The name of this location is given in the error text.  SET WORK
**	LOCATIONS can be used only with locations which the database has
**	been extended to.
**
** System Status:
**	The command is disregarded.
**
** Recommendation:
**	Use accessdb to extend the location to the database if you want to
**	use this location for work in this database, or remove this location
**	from your SET WORK LOCATIONS statement.
*/
E_SC0317_LOC_NOT_AUTHORIZED:SS50000_MISC_ERRORS
"Either you are not authorized to use location '%0c' or it is not a WORK
location."
/*%
** Parameters:
**	%0c - location name
**
** Explanation:
**	An attempt was made to use the SET WORK LOCATIONS command to begin
**	using a location whose name is given in the error text.  Either you are
**	not allowed to use this location because of a GRANT NOALLOCATE,
**	or you have specified a location which is not a work location.  In
**	either case, you are not authorized to use the location specified.
**
** System Status:
**	The command is disregarded.
**
** Recommendation:
**	Either gain access through the proper channels or do not attempt to use
**	this location as a work location.
*/
E_SC0318_SCA_USER_TRACE:SS50000_MISC_ERRORS
"User Trace Message: '%0c'"
/*%
** Parameters:
**	%0c - message provided by the user
**
** Explanation:
**	Some user-defined datatype code has requested that a trace message
**	be placed into the system error log.
**
** System Status:
**	The system is operating normally.
**
** Recommendation:
**	Check with the author of the user-defined datatype.  Presumably,
**	that person is interested in the contents of the message.
*/
E_SC0319_SERVER_CRASH_TEST:SS50000_MISC_ERRORS
"The Server has been shutdown as part of a server crash test.  The
shutdown was triggered by the server crash trace flag.  This is used to
simulate actual server crashes during product validation."
/*%
** Parameters:
**	None.
**
** System Status:
**	The server will crash immediately.
**
** Recommendation:
**	None - presumably the user has requested the server crash by
**	issuing the server crash trace flag.  Recovery should be done
**	automatically by the Recovery System (RCP).
*/
E_SC031A_LO_PARAM_ERROR:SS50000_MISC_ERRORS
"Error processing Large Object Parameter:  See previous error(s) in error log
for details."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a summary error, which indicates the presence of other, more
**	specific errors in the error log.
**
** System Status:
**	The current query will be aborted.
**
** Recommendation:
**	See the more specific messages in the error log which precede this
**	error.  Follow the actions indicated for those errors.
*/
E_SC031B_SETLG_XCT_INPROG:SS50000_MISC_ERRORS
"You cannot issue the SET [NO]LOGGING statement when you are in a
multi-query transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SET [NO]LOGGING statement allows transactions to run with logging
**	disabled, thus bypassing recovery processing.  If a NOLOGGING
**	transaction fails, the database will be left in an unknown state.
**
**	You cannot toggle the logging state while already executing a
**	transaction.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**	Commit the current transaction and reissue the SET statement.
*/
E_SC031C_ONERR_XCT_INPROG:SS50000_MISC_ERRORS
"You cannot issue the SET SESSION WITH ON_ERROR statement when you are in a
multi-query transaction."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SET SESSION WITH ON_ERROR statement allows users to define the
**	error-handling actions to take on statement errors -- to abort the
**	current statement, or abort the entire transaction.
**
**	You cannot change the statement error-handling mode while already
**	executing a transaction.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**	Commit the current transaction and reissue the SET statement.
*/
E_SC031D_RECOVERY_ADD:SS50000_MISC_ERRORS
"Error adding Recovery thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start
**	the Recovery thread. The message immediately preceding this message in
**	the error log should have additional information about the precise
**	reason that the thread could not be added.
**
** System Status:
**	The server does not start.
**
** Recommendation:
**	Repair the condition causing the Recovery thread to fail to start. If
**	you cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC031E_RECOVERY_EXIT:SS50000_MISC_ERRORS
"Recovery Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Recovery thread to terminate abnormally. The message immediately
**	preceding this message in the error log should have additional
**	information about the precise reason that the thread terminated
**	abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Recovery thread to fail. If you
**	cannot repair the error condition, contact Ingres Corporation 
**	Technical Support for assistance.
*/
E_SC031F_LOGWRITER_ADD:SS50000_MISC_ERRORS
"Error adding Logwriter thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start
**	the Logwriter thread. The message immediately preceding this message
**	in the error log should have additional information about the precise
**	reason that the thread could not be added.
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Logwriter thread to fail to start. If
**	you cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0320_LOGWRITER_EXIT:SS50000_MISC_ERRORS
"Logwriter Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Logwriter thread to terminate abnormally. The message immediately
**	preceding this message in the error log should have additional
**	information about the precise reason that the thread terminated
**	abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Logwriter thread to fail. If you
**	cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0321_CHECK_DEAD_ADD:SS50000_MISC_ERRORS
"Error adding Check Dead thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start
**	the Check Dead thread. The message immediately preceding this message
**	in the error log should have additional information about the precise
**	reason that the thread could not be added.
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Check Dead thread to fail to start. If
**	you cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0322_CHECK_DEAD_EXIT:SS50000_MISC_ERRORS
"Check Dead Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Check Dead thread to terminate abnormally. The message immediately
**	preceding this message in the error log should have additional
**	information about the precise reason that the thread terminated
**	abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Check Dead thread to fail. If you
**	cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0323_GROUP_COMMIT_ADD:SS50000_MISC_ERRORS
"Error adding Group Commit thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Group Commit thread. The message immediately preceding this message
**	in the error log should have additional information about the precise
**	reason that the thread could not be added.
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Group Commit thread to fail to start.
**	If you cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0324_GROUP_COMMIT_EXIT:SS50000_MISC_ERRORS
"Group Commit Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Group Commit thread to terminate abnormally. The message immediately
**	preceding this message in the error log should have additional
**	information about the precise reason that the thread terminated
**	abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Group Commit thread to fail. If you
**	cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0325_FORCE_ABORT_ADD:SS50000_MISC_ERRORS
"Error adding Force Abort thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Force Abort thread. The message immediately preceding this message
**	in the error log should have additional information about the precise
**	reason that the thread could not be added.
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Force Abort thread to fail to start.
**	If you cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0326_FORCE_ABORT_EXIT:SS50000_MISC_ERRORS
"Force Abort Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Force Abort thread to terminate abnormally. The message immediately
**	preceding this message in the error log should have additional
**	information about the precise reason that the thread terminated
**	abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Force Abort thread to fail. If you
**	cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0327_RECOVERY_SERVER_NOCNCT:SS50000_MISC_ERRORS
"User connections to the Recovery Server are not supported."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A user session has attempted to connect to the recovery server, but
**	the recovery server does not currently support processing of user
**	connections.
**
** System Status:
**	The connection attempt is rejected.
**
** Recommendation:
**	Connect to an ordinary server.
*/
W_SC0328_REMOVE_SESSION:SS5000B_INTERNAL_ERROR
"Warning: Session being removed via iimonitor or IPM"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Log warning to the error log if a session is being removed;
**	scs_iformat() will be called to dump out the SCB.
**
** System Status:
**	This is a warning message only
**
** Recommendation:
**	None.
*/
E_SC0329_AUDIT_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding Security Audit Thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Audit thread. The message immediately preceding this message
**	in the error log should have additional information about the precise
**	reason that the thread could not be added. The audit thread will only
**	be present in servers which have security auditing enabled.
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Audit thread to fail to start.
**	If you cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance. 
*/
E_SC032A_AUDIT_THREAD_EXIT:SS50000_MISC_ERRORS
"Security Audit Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Audit thread to terminate abnormally. The message immediately
**	preceding this message in the error log should have additional
**	information about the precise reason that the thread terminated
**	abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Audit thread to fail. If you
**	cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC032B_BAD_PM_FILE:SS50000_MISC_ERRORS
"The default Parameter Management (PM) resource data files (config.dat and
protect.dat) contains syntax errors"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt to load Parameter Management (PM) resource files failed
**	due to syntax errors being present.
**	
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Check that the PM files are present and are not corrupt; cbf may
**	need to be run.
*/
E_SC032C_NO_PM_FILE:SS50000_MISC_ERRORS
"The default Parameter Management (PM) resource data files (config.dat and
protect.dat) are unreadable or do not exist"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt to load Parameter Management (PM) resource files failed
**	due the files not being present.
**	
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Check that the PM files are present and are accessible.
*/
E_SC032D_C2_AUDITING_ON:SS00000_SUCCESS
"Server will operate with C2 Security Auditing Enabled"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The server has been configured (via cbf) to operate with
**	C2 security auditing.
**
** System Status:
**	The installation may perform security auditing.
**
** Recommendation:
**	This message is informational only.
*/
E_SC032E_C2_NEEDS_KME:SS50000_MISC_ERRORS
"C2 Security Auditing will NOT operate in this server due 
to the lack of the Knowledge Management Extension.  
Run iiconfig to turn off C2 Security Auditing, or obtain 
a new authorization string."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The installation has C2 security enabled (in config.dat), but
**	lacks the KME option required to allow it to operate.
**	
** System Status:
**	The server will continue without security auditing.
**
** Recommendation:
**	The problem should be corrected, either by obtaining KME or
**	turning off security auditing.
*/
E_SC032F_CP_TIMER_ADD:SS50000_MISC_ERRORS
"Error adding Consistency Point Timer thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Consistency Point Timer thread. The message immediately preceding
**	this message in the error log should have additional information about
**	the precise reason that the thread could not be added.
**
** System Status:
**	The server does not start.
**
** Recommendation:
**	Repair the condition causing the Consistency Point Timer thread to fail
**	to start. If you cannot repair the error condition, contact
**	Technical Support for assistance.
*/
E_SC0330_CP_TIMER_EXIT:SS50000_MISC_ERRORS
"Consistency Point Timer Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Consistency Point Timer thread to terminate abnormally. The message
**	immediately preceding this message in the error log should have
**	additional information about the precise reason that the thread
**	terminated abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Consistency Point Timer thread to
**	fail. If you cannot repair the error condition, contact
**	Technical Support for assistance.
*/
E_SC0331_C2_INVALID_MODE:SS50000_MISC_ERRORS
"The C2 Security Mode had an illegal value. 
Allowed values are 'ON' or 'OFF'."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The C2 Security Mode had an illegal value in the resource parameter
**	file. Legal values are "ON" or "OFF".
**	
** System Status:
**	The server will not start.
**
** Recommendation:
**	Correct the C2 Security Mode parameter, then restart the server.
*/
E_SC0332_B1_NEEDS_C2:SS50000_MISC_ERRORS
"C2 security must be enabled  for a B1 secure DBMS. Set the C2 mode to ON
then try again."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was started in B1 mode but C2 security was disabled;
**	B1 security requires C2.
**	
** System Status:
**	The server will not start.
**
** Recommendation:
**	Correct the C2 Security Mode, then restart the server.
*/
E_SC0333_B1_NO_SECLABELS:SS50000_MISC_ERRORS
"B1 security requires security label support, which is not available on this
installation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS was started in B1 mode, but security label support is not
**	available.
**	
** System Status:
**	The server will not start.
**
** Recommendation:
**	Correct the B1 security mode, then restart the server.
*/
E_SC0334_BAD_SECURE_LEVEL:SS50000_MISC_ERRORS
"An invalid or missing installation security level was found. Legal values for 
the security level are B1 or C2.  Correct the configuration secure level then 
restart the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The Secure Level configuration parameter was undefined or set to an
**	invalid level.
**	
** System Status:
**	The server will not start.
**
** Recommendation:
**	Correct the security level configuration parameter, then restart the 
**	server.
*/

E_SC0335_NO_PROC_SEC_LABEL:SS50000_MISC_ERRORS
"Unable to determine the process security label for a B1 server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A B1 server tried to get its own process security label but failed.
**	
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the problem persists, contact
**	Technical Support.
*/
E_SC0336_BAD_SESS_SEC_LABEL:SS50000_MISC_ERRORS
"Invalid session security label detected while trying to connect to a B1
secure DBMS. This connection attempt is rejected."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session tried to connect to a DBMS but specified a security
**	label which is misformatted or illegal for the underlying
**	security system.
**	
** System Status:
**	The connection is rejected.
**
** Recommendation:
**	Retry the connection. If the problem persists, contact
**	Technical Support.
*/
E_SC0337_SEC_LABEL_TOO_LOW:SS50000_MISC_ERRORS
"A security label was specified which does not dominate CA_LOW,
the lowest allowed security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session tried to specify a security label which did not dominate
**	CA_LOW, the lowest legal security label.
**	
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the problem persists, contact
**	Technical Support.
*/
E_SC0338_SEC_LABEL_TOO_HIGH:SS50000_MISC_ERRORS
"A security label was specified which is not dominated by CA_HIGH,
the highest allowed security label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A session tried to specify a security label which was not dominated
**	by CA_HIGH, the highest legal security label.
**	
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the problem persists, contact
**	Technical Support.
*/
E_SC0339_SECURE_INVALID_PARM:SS50000_MISC_ERRORS
"The server configuration parameter '%0c' is set to an invalid or illegal
value."
/*%
** Parameters:
**	%0c - parameter name
**
** Explanation:
**	An attempt was made to start a DBMS server with an illegal 
**	security-related configuration parameter.
**	
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Correct the parameter value in the configuration file,
**	then try again.
*/
E_SC033A_DB_SEC_LABEL:SS50000_MISC_ERRORS
"A security label was specified which does not dominate the current 
database label."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A security label was specified which does not dominate the
**	current database label.
**	
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the problem persists, contact
**	Technical Support.
*/
E_SC033B_SET_LABEL_INVLD:SS50000_MISC_ERRORS
"The requested security label is invalid for your session. Either the label
is invalid, or out of the range of legal security labels for your session."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A security label was specified which is not legal for the current
**	session.
**	
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the problem persists, contact
**	Technical Support.
*/
E_SC033C_SET_SESSION_ERR:SS50000_MISC_ERRORS
"An error occurred in SCF while setting new session values."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred setting session values, e.g, with the SET SESSION
**	statement.
**
** System Status:
**	The operation is rejected. All or some of the changes may not have
**	been made.
**
** Recommendation:
**	Retry the operation. If the problem persists, contact Ingres
**	Technical Support.
*/
E_SC033D_TZNAME_INVALID:SS50000_MISC_ERRORS
"Unable to load timezone file for timezone name: %0c.  Timezone 
name, which is specified by %1c_TIMEZONE_NAME, should map to a valid 
timezone file.  Timezone names to timezone files mapping are managed in 
the configuration file (config.dat) and timezone files are located 
in the $%2c_CONFIG/zoneinfo directory."
/*%
** Parameters:
**	%0c - timezone name
**	%1c - system prefix for environment variables
**	%2c - system prefix for environment variables
**
** Explanation:
**	An attempt was made to load a time zone file, but the file
**	couldn't be read.
**
** System Status:
**	If this error occurs during server startup, the server startup will 
**	fail.  If encountered during session startup, the session will be
**	terminated.
**
** Recommendation:
**	Use the timezone check program "*zck" to further diagnosis this problem
**	and take the appropriate corrective action (such as using a different
**	timezone name).
*/
E_SC033E_SET_PRIORITY_ERR:SS50000_MISC_ERRORS
"Error occurred setting session priority to %0d. (internal priority value %1d)."
/*%
** Parameters:
**	%0d - requested priority
**	%1d - internal priority
**
** Explanation:
**	An error occurred setting session priority, e.g., with the SET SESSION
**	statement. 
**
** System Status:
**	The operation continues; the session priority is unchanged.
**
** Recommendation:
**	Retry the operation with a valid priority.
*/
E_SC033F_INVLD_PRIORITY:SS50000_MISC_ERRORS
"You may not set your session priority to %0d."
/*%
** Parameters:
**	%0d - requested priority
**
** Explanation:
**	The user tried to set their session priority to an invalid value.
**	
** System Status:
**	The operation continues; the session priority is unchanged.
**
** Recommendation:
**	Retry the operation with a valid priority.
*/
E_SC0340_TERM_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding Session Terminator Thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Session Terminator thread. The message immediately preceding this 
**	message in the error log should have additional information about 
**	the precise reason that the thread could not be added. 
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Session Terminator thread to fail to
**	start. If you cannot repair the error condition, contact
**	Technical Support for assistance. 
*/
E_SC0341_TERM_THREAD_EXIT:SS50000_MISC_ERRORS
"Session Terminator Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Session Terminator thread to abnormally terminate. The message
**	immediately preceding this message in the error log should have
**	additional information about the precise reason that the thread
**	terminated abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Session Terminator thread to fail. If
**	you cannot repair the error condition, contact Ingres Corporation
**	Technical Support for assistance.
*/
E_SC0342_IDLE_LIMIT_INVALID:SS50000_MISC_ERRORS
"You may not set the session idle time limit to %0d."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An invalid or out-of-range value was requested for the session
**	idle time limit.
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	Specify a valid idle time limit.
*/
E_SC0343_CONNECT_LIMIT_INVALID:SS50000_MISC_ERRORS
"You may not set the session connect time limit to %0d."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An invalid or out-of-range value was requested for the session
**	connect time limit.
**
** System Status:
**	The action is rejected.
**
** Recommendation:
**	Specify a valid connect time limit.
*/
I_SC0344_SESSION_DISCONNECT:SS50000_MISC_ERRORS
"Session %0c (user %1c) has exceeded its allowed session limits. This
session will be disconnected. The limit exceeded was '%2c'."
/*%
** Parameters:
**	%0c - session identifier
**	%1c - user name
**	%2c - limit name
**
** Explanation:
**	This is an advisory message. A session has exceeded its allowed
**	session limits (for example, connect or idle time), so it will be
**	disconnected.
**
** System Status:
**	The session will be disconnected.
**
** Recommendation:
**	None.
*/
E_SC0345_SERVER_CLOSED:SS08004_CONNECTION_REJECTED
"Server not accepting connections because of maintenance or shut down.\n
Try another server, have the administrator re-open this one, or start a
new one."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An attempt was made to connect to a server that is already in
**	the process of either be shut down or having maintenance done
**	on it.
**
** System Status:
**	The server is not accepting connections by non-privileged
**	users.  This is a result of "set server shut" or "set server
**	closed".
**
** Recommendation:
**	Attempt to connect to another server, or arrange to have the
**	affected server re-opened by having the administrator use
**	"set server open".
*/
E_SC0346_DBMS_PARAMETER_ERROR:SS5000B_INTERNAL_ERROR
"An invalid parameter (%0c) was passed to the iidbms executable. The acceptable
parameters are: dbms, rms, star, or recovery, but the actual parameter was
%0c. Please correct the parameter and restart the server process."
/*%
** Parameters:
**	%0c - parameter
**
** System Status:
**	The server process refuses to start. This message is followed by
**	message E_SC0124, indicating a problem initializing the server.
**
** Recommendation:
**	Change the argument which was passed to the server process.
*/
W_SC0347_CRASH_SESSION:SS5000B_INTERNAL_ERROR
"Warning: Session being killed via iimonitor or IPM"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Log warning to the error log if a session is being killed;
**	scs_iformat() will be called to dump out the SCB.
**
** System Status:
**	This is a warning message only.
**
** Recommendation:
**	None.
*/
W_SC0348_CRASH_SERVER:SS5000B_INTERNAL_ERROR
"Warning: Server being stopped or crashed via iimonitor or IPM"
/*%
** Parameters:
**	None.
** Explanation:
**	Log warning to the error log if a server is being killed.
**
** System Status:
**	This is a warning message only.
**
** Recommendation:
**	None.
*/
E_SC0349_GET_DB_ALARMS:SS50000_MISC_ERRORS
"An error occurred in SCF while getting database security alarms."
/*%
** Parameters:
**      None
**
** Description:
**      An error occurred in SCF while getting database security alarm
**	information from the system catalogs.
**
** System Status:
**      Session will be disconnected.
**
** Recommendation:
**      None
*/
E_SC034A_FIRE_DB_ALARMS:SS50000_MISC_ERRORS
"An error occurred in SCF while firing database security alarms."
/*%
** Parameters:
**      None
**
** Description:
**      An error occurred in SCF while firing database security alarms
**
** System Status:
**      Session will be disconnected.
**
** Recommendation:
**      None
*/
E_SC034B_ALARM_QRY_IIEVENT:SS50000_MISC_ERRORS
"An error occurred accessing the ii[x]event catalog while looking
up dbevents associated with security alarms."
/*%
** Parameters:
**      None
**
** Description:
**      An error occurred in SCF while looking up dbevent information
**	in one of the event-related catalogs while building security alarm 
**	information.
**
** System Status:
**      Session will be disconnected.
**
** Recommendation:
**      If condition continues destroy then recreate the security alarm
**	and/or dbevent.
*/
E_SC034C_GET_SESSION_ROLE:SS50000_MISC_ERRORS
"An error occurred in SCF while getting session roles."
/*%
** Parameters:
**      None
**
** Description:
**      An error occurred in SCF while getting session role 
**	information from the system catalogs.
**
** System Status:
**      Session will be disconnected.
**
** Recommendation:
**      None
*/
E_SC034D_ROLE_NOT_AUTHORIZED:SS42000_SYN_OR_ACCESS_ERR
"The requested role does not exist, or you are not authorized to use it,
or you supplied an invalid role password."
/*%
** Parameters:
**      None
**
** Description:
**      The requested role could not be used for the session due to
**	one of the indicated reasons.
**
** System Status:
**      Processing continues;
**
** Recommendation:
**      Specify a legal role name which you are authorized to use.
*/
E_SC034E_PRIV_PROPAGATE_ERROR:SS50000_MISC_ERRORS
"An error occurred propagating privilege changes from SCF to other DBMS
facilities."
/*%
** Parameters:
**      None
**
** Description:
**      An error occurred while propagating privileges changes from SCF
**	to other DBMS facilities.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      This is an internal error, please contact your support
**	representative if this error occurs.
*/
E_SC034F_NO_FLAG_PRIV:SS42000_SYN_OR_ACCESS_ERR
"User '%0c' attempted to specify invalid flag/option '%1c' while connecting to %2c."
/*%
** Parameters:
**      %0c - User name
**	%1c - Flag
**
** Description:
**      This is a detail message providing supplemental information
**	to the generic E_US0002 error message.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Retry the operation specifying appropriate flags.
*/
E_SC0350_B1_NOT_LICENCED:SS42000_SYN_OR_ACCESS_ERR
"This installation is not licensed to use B1 mode."
/*%
** Parameters:
**      none
**
** Description:
**      An attempt was made to start the installation in B1 mode but the 
**	installation is not licensed to use B1.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Disable B1 mode, or ensure your authorization string is correct.
*/
E_SC0351_IIROLE_GET_ERROR:SS50000_MISC_ERRORS
"An error occurred querying the IIROLE catalog to look up information
about role '%0c'."
/*%
** Parameters:
**      %0c - Role name
**
** Description:
**      An error occurred while SCF was querying the system
**	catalog to look up information about the indicated role.
**	This may indicate an inconsistent or corrupted system catalog,
**	a missing role identifier or some other error.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Check the error log for more information why this
**	occurred.
*/
E_SC0352_IIROLE_ERROR:SS50000_MISC_ERRORS
"An error occurred processing the IIROLE catalog" 
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error occurred while SCF was processing the
**	system catalog while looking up a role.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Check the error log for more information why this
**	occurred.
*/
E_SC0353_SESSION_ROLE_ERR:SS50000_MISC_ERRORS
"An error occurred in SCF while getting information about 
roles associated with the session." 
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error occurred in SCF while getting information
**	about roles associated with the session.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Check the error log for more information why this
**	occurred.
*/
E_SC0354_EXTPWD_GCA_COMPLETION:SS50000_MISC_ERRORS
"SCF got unexpected GCF error in completion routine while requesting 
external password information."
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error occurred in SCF while getting external 
**	password information.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Check the error log for more information why this
**	occurred.
*/
E_SC0355_EXTPWD_GCA_NOPEER:SS50000_MISC_ERRORS
"No peer was found while attempting to authenticate external password for
user/role '%0c'. The target was '%1c'."
/*%
** Parameters:
**      %0c - user/role name
**	%1c - target name
**
** Description:
**      No peer was available to authenticate an external password for
**	a user/role.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Ensure the role authentication mechanism is correctly configured
**	and the authentication service is operational.
*/
E_SC0356_EXTPWD_GCA_CSSUSPEND:SS50000_MISC_ERRORS
"Unexpected error returned from CSsuspend while requesting
external password information."
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error was returned from CSsuspend while requesting
**	external password information.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Ensure the role authentication mechanism is correctly configured
**	and the authentication service is operational.
*/
E_SC0357_EXTPWD_GCA_FASTSELECT:SS50000_MISC_ERRORS
"Unexpected error returned from GCA_FASTSELECT while requesting
external password information."
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error was returned from GCA_FASTSELECT while requesting
**	external password information.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Ensure the role authentication mechanism is correctly configured
**	and the authentication service is operational.
*/
E_SC0358_EXTPWD_GCA_FORMAT:SS50000_MISC_ERRORS
"Unexpected error returned from GCA_FORMAT while requesting
external password information."
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error was returned from GCA_FORMAT while requesting
**	external password information.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Ensure the role authentication mechanism is correctly configured
**	and the authentication service is operational.
*/
E_SC0359_EXTPWD_GCM_ERROR:SS50000_MISC_ERRORS
"Unexpected error returned from GCM while requesting
external password information."
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error was returned from GCM while requesting
**	external password information.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Ensure the role authentication mechanism is correctly configured
**	and the authentication service is operational.
*/
E_SC035A_EXTPWD_NO_AUTH_MECH:SS50000_MISC_ERRORS
"No authentication mechanism was found for user/role '%0c' while checking 
an external password"
/*%
** Parameters:
**      %0c - username/rolename
**
** Description:
**      The user/role authentication mechanism could not be found while
**	checking an external password.
**
** System Status:
**      Operation is aborted.
**
** Recommendation:
**      Ensure the user/role authentication mechanism is correctly configured
**	and the authentication service is operational.
*/
E_SC035B_ROLE_CHECK_ERROR:SS50000_MISC_ERRORS
"An error occurred checking access to the requested role.
Access to the role is denied.
Please check the error log (errlog.log) for more details
why this condition occurred."
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error occurred while checking access to the 
**	requested role. 
**
** System Status:
**      Access  is denied
**
** Recommendation:
**      Check the error log for further details why this condition
**	occurred.
*/
E_SC035C_USER_CHECK_EXT_PWD:SS50000_MISC_ERRORS
"Error occurred checking an external user password. Access is denied.
Please check the error log (errlog.log) for more details
why this condition occurred."
/*%
** Parameters:
**      none
**
** Description:
**      An unexpected error occurred while checking access to an
**	external user password.
**
** System Status:
**      Access  is denied
**
** Recommendation:
**      Check the error log for further details why this condition
**	occurred.
*/
E_SC035D_SET_ROLE_IN_XACT:SS42000_SYN_OR_ACCESS_ERR
"SET ROLE: This statement is not allowed in a multi-query transaction."
/*%
** Parameters:
**      none
**
** Description:
**      An attempt was made to issue a SET ROLE statement while a
**	transaction was active. This is not allowed.
**
** System Status:
**      Operation is rejected.
**
** Recommendation:
**      Try issuing SET ROLE when no transaction is active.
*/
E_SC035E_LOAD_ROLE_DBPRIV:SS50000_MISC_ERRORS
"SCF detected error loading role dbprivilege information."
/*%
** Parameters:
**      none
**
** Description:
**      SCF detected an error loading dbprivilege information for
**	roles associated with a session.
**
** System Status:
**      Operation is rejected.
**
** Recommendation:
**      Check error log for more details why this occurred.
*/
E_SC035F_SET_ROLE_NODBACCESS:SS42000_SYN_OR_ACCESS_ERR
"SET ROLE: You may not set role to '%0c' because access to the current
database would be denied."
/*%
** Parameters:
**      %0c - requested role.
**
** Description:
**      An attempt was made to change role using SET ROLE which would have
**	resulted in the session loosing the ability to access the current
**	database, which is not allowed.
**
** System Status:
**      Operation is rejected.
**
** Recommendation:
**      Check role access to the current database and correct this
**	if appropriate.
*/
E_SC0360_SEC_WRITER_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding Security Audit Writer Thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Security Audit Writer thread. The message immediately preceding 
**	this message in the error log should have additional information about 
**	the precise reason that the thread could not be added. 
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Security Audit Writer thread to fail to
**	start. If you cannot repair the error condition, contact
**	Technical Support for assistance. 
*/
E_SC0361_SEC_WRITER_THREAD_EXIT:SS50000_MISC_ERRORS
"Security Audit Writer Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	Security Audit Writer thread to abnormally terminate. The message
**	immediately preceding this message in the error log should have
**	additional information about the precise reason that the thread
**	terminated abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the Security Audit Writer thread to fail. 
**	If you cannot repair the error condition, contact
**	Technical Support for assistance.
*/
E_SC0362_SECLABEL_PARAM_SLTYPE:SS50000_MISC_ERRORS
"A security label query parameter was passed to the DBMS from a 
frontend with an incompatible security label type. Internal security label
values can only be processed from frontends with the same security label type."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS received an internal security label data value
**	from a frontend application which supports an incompatible
**	security label type. The incompatible security label value cannot
**	be processed by the DBMS so the query is rejected.
**
** System Status:
**	The query is rejected.
**
** Recommendation:
**	Only attempt to pass internal security labels between compatible
**	frontends and DBMS servers. 
*/
E_SC0363_DBP_PARAM_INCONSISTENCY:SS50000_MISC_ERRORS
"An inconsistency in the setup of database parameters was detected.
Please report this problem to product support."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS has detected an internal inconsistency related
**      to the processing of database procedure parameters.
**  	
**
** System Status:
**	The query is rejected.
**
** Recommendation:
**	Contact your product support representative.
*/
E_SC0364_READ_AHEAD_EXIT:SS50000_MISC_ERRORS
"Read Ahead Thread terminated abnormally."
E_SC0365_IOMASTER_OPENDB:SS50000_MISC_ERRORS
"IOMASTER server encountered error opening database from database list."
E_SC0366_WRITE_ALONG_EXIT:SS50000_MISC_ERRORS
"Write Along Thread terminated abnormally."
E_SC0367_WRITE_ALONG_ADD:SS50000_MISC_ERRORS
"Error adding Write Along thread to the server."
E_SC0368_READ_AHEAD_ADD:SS50000_MISC_ERRORS
"Error adding Read Ahead thread to the server."
E_SC0369_IOMASTER_BADCPU:SS50000_MISC_ERRORS
"IOMASTER server requires a multi-CPU environment."
E_SC0370_IOMASTER_SERVER_NOCNCT:SS50000_MISC_ERRORS
"User connections to the I/O Master Server are not supported."
E_SC0371_NO_TX_FLAG:SS50000_MISC_ERRORS
"No transaction flag has been provided, you must use either
RAAT_TX_READ or RAAT_TX_WRITE."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user attempted to use a RAAT call to open a transaction,
**	but did not provide either RAAT_TX_READ or RAAT_TX_WRITE in the flags 
**	field to open the transaction read or write, this is a mandatory flag.
**
** System status:
**	The query will be aborted.
**
** Recommendation:
**	Fill out the flags field with either RAAT_TX_READ or RAAT_TX_WRITE.
*/
E_SC0372_NO_MESSAGE_AREA:SS50000_MISC_ERRORS
"Unable to allocate memory to store return GCA message for RAAT call."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The DBMS is unable to allocate memory to contain the return 
**	GCA message for a RAAT operation.
**
** System status:
**	The query will be aborted.
**
** Recommendation:
**	Check other errors in the error log, and report the error to your
**	system administrator.
*/
E_SC0373_PSF_RESOLVE_ERROR:SS50000_MISC_ERRORS
"Error calling PSF to resolve user access to a table."
/*%
** Parameters:
**	None
** Explanation:
**	An error was encountered whilst calling the Parser Facility to resolve
**	user access to a table.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Check other errors in the error log and report the error to your
**	system administrator.
*/
W_SC0374_NO_ACCESS_TO_TABLE:SS50000_MISC_ERRORS
"You do not have any privilege to access this table."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user does not have privilege to access the table
**
** System status:
**	The query will be aborted.
**
** Recommendation:
**	Talk to your database or system administrator, or use the GRANT
**	statement to grant access to the table.
*/
W_SC0375_INVALID_LOCK_MODE:SS50000_MISC_ERRORS
"An invalid lock mode has beed specified in the flags field, use one of:
	RAAT_LK_XT
	RAAT_LK_ST
	RAAT_LK_XP
	RAAT_LK_SP
	RAAT_LK_NL"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user specified an invalid lockmode flag when attempting to
**	change the current lock mode using the RAAT interface.
**
** System status:
**	The current lock mode will not be altered.
**
** Recommendation:
**	Read the RAAT documentation and re-code with the correct lockmode flag.
*/
E_SC0376_INVALID_GET_FLAG:SS50000_MISC_ERRORS
"An invalid flag was passed to a record get operation, use one of:
	RAAT_REC_CURR
	RAAT_REC_NEXT
	RAAT_REC_PREV
	RAAT_REC_BYNUM"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user specified an invalid flag to a RAAT record get operation.
**
** System status:
**	The query will be aborted.
**
** Recommendation:
**	Read the RAAT documentation and re-code using the correct flag.
*/
E_SC0377_NULL_RECORD_ID:SS50000_MISC_ERRORS
"A zero record access identifier has been passed in a position operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	the DBMS was passed a zero record access identifier when attempting
**	to perform a RAAT record position operation.
**
** System status:
**	The query will be aborted.
**
** Recommendation:
**	Contact your system administrator
*/
E_SC0378_NO_KEY_VALUES:SS50000_MISC_ERRORS
"No key values were passed to a RAAT position by key operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	the DBMS was not passed any key values when attempting to perform
**	a RAAT record position by key operation, there no position can be
**	performed.
**
** System status:
**	The query will be aborted.
**
** Recommendation:
**	Check your code to ensure you have passed key values for a key
**	position, then contact your system administrator.
*/
E_SC0379_INVALID_KEYSIZE:SS50000_MISC_ERRORS
"An invalid key size was passed when attemping to perform a RAAT get by key
operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	One of the key values passed to the DBMS for a RAAT get record by
**	key operation had an invalid key size.
**
** System status:
**	The query will be aborted.
**
** Recommendation:
**	Check your code to ensure that you have filled in all of the key
**	fields correctly for a position by key operation, then contact your
**	system administrator.
*/
E_SC037A_INVALID_OPCODE:SS50000_MISC_ERRORS
"An invalid operation code has been passed to the RAAT interface."
/*%
** Parameters: 
**	None.
**
** Explanation:
**	The user passed an invalid operation code the the RAAT interface
**
** System status:
**	No operation will be performed.
**
** Recommendation:
**	Check the RAAT documentation and ensure that you code passed
**	a valid operation code.
*/
E_SC037B_BAD_DATE_CUTOFF:SS50000_MISC_ERRORS
"The value specified by %0c_DATE_CENTURY_BOUNDARY (%1d) is invalid."
/*%
** Parameters:
**	%0c - the system prefix
**	%1d - the invalid value specified.
**
** Explanation:
**	The value provided for the date century boundary is outside
**	the valid range.
**
** System Status:
**	The connection is rejected.
**
** Recommendation:
**	Alter or undefine the variable.
*/
E_SC037C_REP_QMAN_EXIT:SS50000_MISC_ERRORS
"Replicator queue management thread terminated abnormally."
E_SC037D_REP_QMAN_ADD:SS50000_MISC_ERRORS
"Error adding a replicator queue management thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered which has caused the server's
**	LK Blocking Callback thread to abnormally terminate. The message
**	immediately preceding this message in the error log should have
**	additional information about the precise reason that the thread
**	terminated abnormally.
**
** System Status:
**	The server will shut down.
**
** Recommendation:
**	Repair the condition causing the LK Blocking Callback thread to fail. 
**	If you cannot repair the error condition, contact Ingres Technical 
**	Support for assistance.
*/
E_SC037E_LK_CALLBACK_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding LK Blocking Callback Thread to the server."
E_SC037F_LK_CALLBACK_THREAD_EXIT:SS50000_MISC_ERRORS
"LK Blocking Callback Thread terminated abnormally."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the LK Blocking Callback thread. The message immediately preceding 
**	this message in the error log should have additional information about 
**	the precise reason that the thread could not be added. 
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the LK Blocking Callback thread to fail to
**	start. If you cannot repair the error condition, contact Ingres
**	Technical Support for assistance. 
*/
E_SC0380_SET_LOCKMODE_ROW:SS50000_MISC_ERRORS
"SET LOCKMODE LEVEL=ROW is allowed only for databases accessed via a fast 
commit server."
/*%
** Parameters:
**	None.
**
** Explanation:
**      You cannot issue the SET LOCKMODE where LEVEL=ROW when the 
**      database is served by multiple servers using different data caches.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**      Reconfigure your server.
*/
E_SC0381_SETLG_READONLY_DB:SS50000_MISC_ERRORS
"You cannot issue the SET [NO]LOGGING statement for a readonly database."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The SET [NO]LOGGING statement allows transactions to run with logging
**	disabled, thus bypassing recovery processing.  If a NOLOGGING
**	transaction fails for a readonly database, the database will NOT be 
** 	left in an unknown state.
**
** 	For a readonly database, SET NOLOGGING is used and cannot be changed
** 	by the user.
**
** System Status:
**	The current statement is rejected.
**
** Recommendation:
**	None.
*/
E_SC0382_LICENSE_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding License Checking Thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the License Checking thread. The message immediately preceding this 
**	message in the error log should have additional information about 
**	the precise reason that the thread could not be added. 
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the License Checking thread to fail to
**	start. If you cannot repair the error condition, contact
**	Technical Support for assistance. 
*/
E_SC0383_LICENSE_THREAD_EXIT:SS50000_MISC_ERRORS
"License Checking Thread terminated abnormally."

/*%
** Parameters:
**      None.
**
** Explanation:
**      An internal error was encountered which has caused the server's
**      License Checking thread to abnormally terminate. The message
**      immediately preceding this message in the error log should have
**      additional information about the precise reason that the thread
**      terminated abnormally.
**
** System Status:
**      The server will shut down.
**
** Recommendation:
**      Repair the condition causing the License Checking thread to fail. If
**      you cannot repair the error condition, contact Ingres Corporation
**      Technical Support for assistance.
*/
E_SC0384_CLEANUP_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding Cleanup Checking Thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Cleanup Checking thread. The message immediately preceding this
**	message in the error log should have additional information about 
**	the precise reason that the thread could not be added. 
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Cleanup Checking thread to fail to
**	start. If you cannot repair the error condition, contact
**	Technical Support for assistance. 
*/
E_SC0385_CLEANUP_THREAD_EXIT:SS50000_MISC_ERRORS
"Cleanup Checking Thread terminated abnormally."
/*%
** Parameters:
**      None.
**
** Explanation:
**      An internal error was encountered which has caused the server's
**      Cleanup Checking thread to abnormally terminate. The message
**      immediately preceding this message in the error log should have
**      additional information about the precise reason that the thread
**      terminated abnormally.
**
** System Status:
**      The server will shut down.
**
** Recommendation:
**      Repair the condition causing the Cleanup Checking thread to fail. If
**      you cannot repair the error condition, contact Ingres Corporation
**      Technical Support for assistance.
*/
E_SC0386_LOGTRC_THREAD_ADD:SS50000_MISC_ERRORS
"Error adding Logging/Tracing Thread to the server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An internal error was encountered when the server attempted to start up
**	the Logging/Tracing thread. The message immediately preceding this
**	message in the error log should have additional information about 
**	the precise reason that the thread could not be added. 
**
** System Status:
**	The server will not start.
**
** Recommendation:
**	Repair the condition causing the Logging/Tracing thread to
**      fail to start. If you cannot repair the error condition, contact
**      Ingres Corporation Technical Support for assistance.
*/
E_SC0387_LOGTRC_THREAD_EXIT:SS50000_MISC_ERRORS
"Logging/Tracing Thread terminated abnormally."
/*%
** Parameters:
**      None.
**
** Explanation:
**      An internal error was encountered which has caused the server's
**      Logging/Tracing thread to abnormally terminate. The message
**      immediately preceding this message in the error log should have
**      additional information about the precise reason that the thread
**      terminated abnormally.
**
** System Status:
**      The server will shut down.
**
** Recommendation:
**      Repair the condition causing the Logging/Tracing thread to fail. If
**      you cannot repair the error condition, contact Ingres Corporation
**      Technical Support for assistance.
*/

E_SC0388_NO_LICENSE:SS50000_MISC_ERRORS
"An attempt was made to validate the license for a runtime component,\
 but this node is not currently licensed for that component.\
\nAssociated error messages which provide more detailed information about the
problem can be found in the error log."
/*%
** Parameters:
**      None.
**
** Explanation:
**      The license validation for a client component which depends upon the
**      server to perform its license validation failed.
**
** System Status:
**      The server will be unaffected, but the client will fail to connect.
**
** Recommendation:
**      Please contact Ingres Corporation to obtain licensing for the
**      specified component.
*/

E_SC0389_LOCK_CREATE:SS50000_MISC_ERRORS
"Error creating a lock list."
/*%
** Parameters:
**      None.
**
** Explanation:
**      A failure occurred whilst trying to create a lock list.
**
** System Status:
**      The server will be unaffected, but the client sessions will abort.
**
** Recommendation:
**      Please contact Ingres Corporation to obtain licensing for the
**      specified component.
*/

E_SC0390_BAD_LOCK_REQUEST:SS50000_MISC_ERRORS
"An error occurred when requesting a lock."
/*%
** Parameters:
**      None.
**
** Explanation:
**      A lock request failed in SCF.
**
** System Status:
**      The server will be unaffected, but the client sessions will abort.
**
** Recommendation:
**      Please contact Ingres Corporation to obtain licensing for the
**      specified component.
*/

E_SC0391_BAD_LOCK_RELEASE:SS50000_MISC_ERRORS
"An error occurred when releasing a lock."
/*%
** Parameters:
**      None.
**
** Explanation:
**      A lock release failed in SCF.
**
** System Status:
**      The server will be unaffected, but the client sessions will abort.
**
** Recommendation:
**      Please contact Ingres Corporation to obtain licensing for the
**      specified component.
*/

E_SC0392_CUT_INIT:SS50000_MISC_ERRORS
"An internal error occurred when attempting to initialize CUT, please see 
errlog.log in II_CONFIG for more details."

E_SC0393_RAAT_NOT_SUPPORTED:SS5000B_INTERNAL_ERROR
"RAAT access is not supported."

E_SC0394_CLUSTER_NOT_SUPPORTED:SS5000B_INTERNAL_ERROR
"Installation is configured for clusters, but server binary cannot support
clustering."
E_SC0395_RDF_DDB_FAILURE:SS50000_MISC_ERRORS
"Error attempting to obtain DDB details. Name: %0c."
/*
** Parameters:
**      %0c - database name
**
** Explanation:
**      A failure occurred whilst trying to obtain DDB details.
**
** System Status:
**      The server will be unaffected, but the client sessions will abort.
**
** Recommendation:
**      Please contact Ingres Corporation to obtain licensing for the
**      specified component.
*/
E_SC0396_DIST_DEADLOCK_THR_ADD:SS5000B_INTERNAL_ERROR
"Fail to start distributed deadlock thread."
E_SC0397_DIST_DEADLOCK_THR_EXIT:SS5000B_INTERNAL_ERROR
"Unexpected exit from distributed deadlock thread."
E_SC0398_GLC_SUPPORT_THR_ADD:SS5000B_INTERNAL_ERROR
"Fail to start GLC support thread."
E_SC0399_GLC_SUPPORT_THR_EXIT:SS5000B_INTERNAL_ERROR
"Unexpected exit from GLC support thread."

E_SC039A_SET_SESSION_READWRITE_ERR:SS50000_MISC_ERRORS
"The SET SESSION statement is not allowed in a multi-query transaction."
/*%
** Parameters:
**      None.
**
** Explanation:
**      SET SESSION command specified which is not allowed in a 
**      Multi-Query transaction.
**
** System Status:
**      None
**
** Recommendation:
**      Issue the command following a COMMIT.
*/

E_SC039B_VAR_LEN_INCORRECT:SS50000_MISC_ERRORS
"Data in column %0c has a length of %1d (0x%1x) exceeding its
maximum of %2d - excess length retracted."
/*%
** Parameters:
**      %c - column name.
**      %d - length in error
**      %d - max valid length
**
** Explanation:
**      A VARCHAR, NVARCHAR, TEXT or VARBYTE column contained a
**      length field too large for the column definition. This could
**      indicate a data curruption or a programming error.
**      The length field will have been reset to match the column
**      length.
**
** System Status:
**      None
**
** Recommendation:
**      Identify the offending data. A comparison between LENGTH(x)
**      and SIZE(x) of a column should show LENGTH <= SIZE for varying
**      columns. If this affects every row of a column it could
**      indicate a corrupt catalog. If limited to an isolated column
**      it could indicate a local date corruption or programming error.
*/

NO_SQLSTATE_MAPPING

E_SC0500_INVALID_LANGCODE
"Invalid language code supplied in the client request."
E_SC0501_INTERNAL_ERROR
"Internal error occurred in the sequencer. File: '%0c'. Line: '%1d'"
E_SC0502_GCA_ERROR
"Sequencer communication error. File: '%0c'. Line: '%1d'"
E_SC0503_MKSYSCF_THREAD_ADD
"Error starting the mksyscf thread."
E_SC0504_CLEANUP_THREAD_ADD
"Error starting the cleanup thread."
E_SC0505_CLEANUP_THREAD_INIT
"Error initializing the clean up thread."
E_SC0506_COMPILE_THREAD_ADD
"Error starting the compiler thread."
E_SC0507_STALONE_UTIL_START
"Error starting the utility thread."
E_SC0508_LICENSE_OBTAIN_FAIL
"Error obtaining a valid license for this server."
E_SC0509_LICENSE_RELEASE_FAIL
"Error releasing the license held by this server."
E_SC050A_LICENSE_VERIFY_FAIL
"Error verifying the license held by this server."
E_SC050C_PROBELOG_OPEN_FAIL
"Error openning the probe log '%0c'."
E_SC050D_INVALID_PROBENUM
"Attempt to activate an invalid probe number '%0d'."
E_SC050E_PROBES_INTERNAL_ERROR
"Probes internal error in '%0c'at '%1d'."
E_SC050F_PROBES_MISPLACED_END
"Misplaced 'end' event for probe '%0d'."
E_SC0510_PROBES_STARTUP
"Server probes initialization failure."
E_SC0511_PROBES_SHUTDOWN
"Server probes shut down failure."
E_SC0514_BAD_SYSTEM_STORE
"System store can not be verified. It does not exist or can not be read."
E_SC0515_NO_SYSTEM_STORE
"System store does not exist."
E_SC0516_BAD_SERVER_TYPE
"Server type is incorrect."
E_SC0517_SYSTEM_STORE_EXISTS
"System store exists already. Delete the system store before recreating it."
E_SC0519_CONN_LIMIT_EXCEEDED
"Connection request was rejected because number of sessions has reached the limit."
E_SC051A_SERVER_IS_RUNNING
"Shared memory segments already exist.\n
Please run jasstop to shut down running servers\n
and/or clean up shared memory segments."
E_SC051B_LOGTRC_THREAD_ADD
"Error starting the cleanup thread."
E_SC051C_LOGTRC_THREAD_INIT
"Error initializing the clean up thread."
E_SC051D_LOAD_CONFIG
"Finished loading configuration parameters for configuration '%0c' of server
type '%1c'."
I_SC051E_IIMONITOR_CMD
"IIMONITOR: '%0c' issued by '%1c' from Terminal: %2c"
