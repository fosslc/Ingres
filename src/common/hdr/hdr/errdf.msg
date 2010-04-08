SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _RD
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	02-nov-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**	29-nov-93 (robf)
**	    Added E_RD0185/6/7 for rdf_update processing of security alarms.
**	07-dec-1993 (dianeh)
**	    Refine newly added text destined for messages.txt file.
**	18-feb-94 (teresa)
**	    Added E_RD0101_FLUSH_CACHE and E_RD010D_BAD_CHECKSUM for bug 59336.
**	22-feb-94 (robf)
**          Add SQLSTATE to teresa's 18-feb change to let file compile
**	19-jun-1996 (sweeney)
**	    Fix typos.
**	10-oct-1996 (canor01)
**	    Make messages more generic.
**	4-Jan-2004 (schka24)
**	    Add partitioning errors, message strings
*/


#define _RD_CLASS 8


E_RD0000_OK:SS00000_SUCCESS
"successful completion."
E_RD0001_NO_MORE_MEM:SS50003_EXHAUSTED_RESOURCE
"Not enough memory for RDF temporary work area."
E_RD0002_UNKNOWN_TBL:SS42500_TBL_NOT_FOUND
"Requested table is not known to DMF."
E_RD0003_BAD_PARAMETER:SS50000_MISC_ERRORS
"RDF function was called with bad parameters."
E_RD0006_MEM_CORRUPT:SS50000_MISC_ERRORS
"Memory allocation control structure was corrupted."
E_RD0007_MEM_NOT_OWNED:SS50003_EXHAUSTED_RESOURCE
"Not the owner of the memory block."
E_RD0008_MEM_SEMWAIT:SS50000_MISC_ERRORS
"Error in ULM waiting for a memory semaphore."
E_RD0009_MEM_SEMRELEASE:SS50000_MISC_ERRORS
"Error in ULM releasing a memory semaphore."
E_RD000A_MEM_NOT_FREE:SS50003_EXHAUSTED_RESOURCE
"Memory allocation control structure was corrupted, cannot reclaim memory."
E_RD000B_MEM_ERROR:SS50003_EXHAUSTED_RESOURCE
"General error from ULM."
E_RD000C_USER_INTR:SS40000_XACT_ROLLBACK
"User interrupts while requesting DMF function."
E_RD000D_USER_ABORT:SS40000_XACT_ROLLBACK
"User's abort detected."
E_RD000E_DMF_ERROR:SS50000_MISC_ERRORS
"General error from DMF."
E_RD000F_INFO_OUT_OF_DATE:SS50000_MISC_ERRORS
"The information block is out of date."
E_RD0010_QEF_ERROR:SS50000_MISC_ERRORS
"Internal error from QEF."
E_RD0011_NO_MORE_ROWS:SS02000_NO_DATA
"No more tuples in the current table."
E_RD0012_BAD_HISTO_DATA:SS5000J_INCONSISTENT_DBMS_CAT
"Histogram tuples are out of sequence."
E_RD0013_NO_TUPLE_FOUND:SS42505_OBJ_NOT_FOUND
"No tuples found in the current table."
E_RD0014_BAD_QUERYTEXT_ID:SS5000J_INCONSISTENT_DBMS_CAT
"The requested query text is not found in the system catalog."
E_RD0015_AMBIGUOUS_REPLACE:SS50000_MISC_ERRORS
"Attempted to update the same row twice."
E_RD0016_KEY_SEQ:SS50000_MISC_ERRORS
"An illegal key sequence was specified."
E_RD0017_TABLE_EXISTS:SS42502_DUPL_OBJECT
"The operation specified tried to create a table that already exists."
E_RD0018_NONEXISTENT_TABLE:SS42500_TBL_NOT_FOUND
"The operation required the use of a table that does not exist."
E_RD0019_TABLE_ACCESS_CONFLICT:SS50000_MISC_ERRORS
"Table access conflict, table opened twice."
E_RD001A_RESOURCE_ERROR:SS50003_EXHAUSTED_RESOURCE
"RDF received out-of-resource error from DMF"
E_RD001B_HISTOGRAM_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"RDF could not found histogram associated with statistics tuple"
E_RD001C_CANT_READ_COL_DEF:SS50000_MISC_ERRORS
"Concurrent build of an attribute default failed.\n
The failure was detected by another session.\n
Check the errlog.log to see why the failure occurred."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Two threads tried to build the same cache object at the same time. The
**	other thread ran into a resource problem or was unable to read the
**	default from the defaults system catalog for a variety of reasons.  This
**	thread was counting on using the cache object the other thread was
**	unable to build.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Check the error log to see why the other thread failed to build
**	the default cache object, and take any appropriate corrective action.
*/
E_RD001D_NO_SUCH_DEFAULT:SS50000_MISC_ERRORS
"The specified attribute default does not exist in iidefaults."
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF was asked to retrieve an attribute default for a default ID that
**	is not in the system catalog.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Ensure that the default is created before referencing it.
*/
E_RD0020_INTERNAL_ERR:SS5000B_INTERNAL_ERROR
"RDF internal error."
E_RD0021_FILE_NOT_FOUND:SS50000_MISC_ERRORS
"Underlying physical file for table not found"
E_RD0025_USER_ERROR:SS5000F_TEXTUAL_INFO
"error by caller, error has been reported or logged by RDF"
E_RD002A_DEADLOCK:SS50003_EXHAUSTED_RESOURCE
"Deadlock has occurred on system catalogs"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Due to concurrent activity on system catalogs, a deadlock situation
**	has occurred.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Reduce the amount of concurrent DDL activity.
*/
E_RD002B_LOCK_TIMER_EXPIRED:SS50003_EXHAUSTED_RESOURCE
"Timeout has occurred on a system catalog"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Due to concurrent activity on system catalogs, a timeout situation
**	has occurred.
**
** System Status:
**	The query will be aborted.
**
** Recommendation:
**	Reduce the amount of concurrent DDL activity.
*/
E_RD0030_SCF_ERROR:SS50000_MISC_ERRORS
"General error from SCU."
E_RD0031_SEM_INIT_ERR:SS50002_LIMIT_EXCEEDED
"Error in SCU initializing a semaphore."
E_RD0032_NO_SEMAPHORE:SS50000_MISC_ERRORS
"The semaphore is not known to SCU."
E_RD0034_RELEASE_SEMAPHORE:SS50000_MISC_ERRORS
"Error releasing ULH semaphore"
E_RD0040_ULH_ERROR:SS50003_EXHAUSTED_RESOURCE
"General error from ULH."
E_RD0041_BAD_FCB:SS50000_MISC_ERRORS
"RDF facility control block contains bad cache id."
E_RD0042_CACHE_NOT_EMPTY:SS50000_MISC_ERRORS
"Error detected in closing a non-empty cache."
E_RD0043_CACHE_FULL:SS50003_EXHAUSTED_RESOURCE
"No more space in cache to build or extend a descriptor block."
E_RD0044_BAD_INFO_BLK:SS50000_MISC_ERRORS
"User's information block is corrupted."
E_RD0045_ULH_ACCESS:SS50002_LIMIT_EXCEEDED
"Failed to gain access to a ULH object due to ULH error"
E_RD0050_TRANSACTION_ABORTED:SS40000_XACT_ROLLBACK
"The transaction has been aborted."
E_RD0060_DMT_SHOW:SS42500_TBL_NOT_FOUND
"Cannot access table information due to a non-recoverable DMT_SHOW error"
E_RD0071_NO_PROT_INT:SS50000_MISC_ERRORS
"error by caller
\n    asking for QTUPLES but no request for protect, rule or integrity tuples"
E_RD0072_NOT_OPEN:SS50000_MISC_ERRORS
"error by caller
\n    attempt to close PROTECTION, INTEGRITY or RULE tuple stream which was not open"
E_RD0073_TUPLE_COUNT:SS40002_CONSTR_VIOLATION
"error by caller
\n    bad tuple count for qrymod retrieve"
E_RD0076_OPEN:SS50000_MISC_ERRORS
"integrity, protection or rule table already opened, but record access id not
\n    available"
E_RD0077_QEU_OPEN:SS50000_MISC_ERRORS
"QEU could not open query mod table"
E_RD0078_QEU_GET:SS50000_MISC_ERRORS
"QEU could not get data from query mod table"
E_RD0079_QEU_CLOSE:SS50000_MISC_ERRORS
"QEU could not close query mod table"
/* FIXME need to spiff up these two errmsgs later, eg with catalog name */
E_RD007A_QEU_DELETE:SS50000_MISC_ERRORS
"RDF could not delete old partition catalog rows"
E_RD007B_QEU_APPEND:SS50000_MISC_ERRORS
"RDF could not insert new partition catalog rows"
E_RD0101_FLUSH_CACHE:SS50000_MISC_ERRORS
"consistency check\n
     - memory corruption detected via multiple bad checksums.\n
       RDF is flushing its entire cache."
E_RD0102_SYS_CATALOG:SS50000_MISC_ERRORS
"consistency check
\n    - more than one extended system catalog was opened at
\n    the same time by RDF within the same thread"
E_RD0103_NOT_OPENED:SS50000_MISC_ERRORS
"consistency check
\n    - attempt to read system catalog which was not opened"
E_RD0104_BUFFER:SS50000_MISC_ERRORS
"consistency check
\n    - internal buffer for reading extended system catalogs not large enough"
E_RD0105_NO_QDATA:SS50000_MISC_ERRORS
"consistency check
\n    - QEF_QDATA structure not initialized"
E_RD0106_CELL_COUNT:SS50000_MISC_ERRORS
"consistency check
\n    - iistatistics tuple contains invalid cell count"
E_RD0107_QT_SEQUENCE:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check
\n    - query tree tuple out of sequence"
E_RD0108_ADI_FIDESC:SS50003_EXHAUSTED_RESOURCE
"consistency check
\n    - error calling the ADI_FIDESC routine"
E_RD0109_NOT_QTUPLE_ONLY:SS50000_MISC_ERRORS
"consistency check - rdu_qget is called for multiple reads,\n
but bitmask RDR_QTUPLE_ONLY is not set"
/*%
** Parameters:
**	None.
**
** Explanation:
**	There is an unreproducible concurrency bug (35513 & 30694) where
**	an E_RD0103_NOT_OPENED consistency check is hit.  This appears to
**	occur on Unix but not on VMS.  Since this bug is not reproducible,
**	additional consistency checks (E_RD0109_NOT_QTUPLE_ONLY,
**	E_RD010A_RQEU_NOT_QTUPLE and E_RD010B_NO_QEU_ACC_ID) are being added to
**	further define what is causing E_RD0103_NOT_OPENED to occur.  This may
**	help to debug/fix this unreproducible bug.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	This is a traceback message to attempt to solve an elusive bug.  If
**	you see this message in your log file on a repeated basis, please call
**	Ingres Corporation Technical Support and report it.
*/
E_RD010A_RQEU_NOT_QTUPLE:SS50000_MISC_ERRORS
"consistency check - rdu_qclose is called to close a table open for multiple\n
reads, but RDR_QTUPLE_ONLY request bitmask is NOT set."
/*%
** Parameters:
**	None.
**
** Explanation:
**	There is an unreproducible concurrency bug (35513 & 30694) where
**	an E_RD0103_NOT_OPENED consistency check is hit.  This appears to
**	occur on Unix but not on VMS.  Since this bug is not reproducible,
**	additional consistency checks (E_RD0109_NOT_QTUPLE_ONLY,
**	E_RD010A_RQEU_NOT_QTUPLE and E_RD010B_NO_QEU_ACC_ID) are being added to
**	further define what is causing E_RD0103_NOT_OPENED to occur.  This may
**	help to debug/fix this unreproducible bug.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	This is a traceback message to attempt to solve an elusive bug.  If
**	you see this message in your log file on a repeated basis, please call
**	Ingres Corporation Technical Support and report it.
*/
E_RD010B_NO_QEU_ACC_ID:SS50000_MISC_ERRORS
"consistency check - rdf was called to close a table.  However that table is\n
not open. This was detected because qeucb.qeu_acc_id is null."
/*%
** Parameters:
**	None.
**
** Explanation:
**	There is an unreproducible concurrency bug (35513 & 30694) where
**	an E_RD0103_NOT_OPENED consistency check is hit.  This appears to
**	occur on Unix but not on VMS.  Since this bug is not reproducible,
**	additional consistency checks (E_RD0109_NOT_QTUPLE_ONLY,
**	E_RD010A_RQEU_NOT_QTUPLE and E_RD010B_NO_QEU_ACC_ID) are being added to
**	further define what is causing E_RD0103_NOT_OPENED to occur.  This may
**	help to debug/fix this unreproducible bug.
**
** System Status:
**	Processing continues.
**
** Recommendation:
**	This is a traceback message to attempt to solve an elusive bug.  If
**	you see this message in your log file on a repeated basis, please call
**	Ingres Corporation Technical Support and report it.
*/
E_RD010C_HIST_LENGTH:SS50000_MISC_ERRORS
"consistency check
\n    - iistatistics tuple contains invalid histogram value length"
/*%
** Parameters:
**	None.
**
** Explanation:
**	The sversion field of the statistics catalog contains a version (it's
**	not all blanks), and the shistlength field contains a length <= 0.
**
** System Status:
**	The default histogram is used for this column in the optimizer.
**
** Recommendation:
**	Run optimizedb on the column.
*/
E_RD010D_BAD_CKSUM:SS50000_MISC_ERRORS
"consistency check\n
      - checksum error on infoblk indicates potential memory corruption"
/*%
** Parameters:
**      None.
**
** Explanation:
** 	RDF found a checksum error on one of its cache objects.  This usually
**	indicates memory corruption.  Checksum checking can be turned off by
**	setting trace point rd22.
**
** System Status:
**	The statement is aborted and the faulty cache object is invalidated.
**
**  Recommendation:
**      checksum errors should be reported to Ingres Corporation Tech Support.
*/
E_RD0110_QRYMOD:SS50000_MISC_ERRORS
"consistency check
\n    - internal error processing qrymod operation"
E_RD0111_VIEW:SS50000_MISC_ERRORS
"consistency check
\n    - internal error processing view operation"
E_RD0112_ATTRIBUTE:SS50000_MISC_ERRORS
"consistency check
\n    - inconsistent attribute cache"
E_RD0113_KEYS:SS50000_MISC_ERRORS
"consistency check
\n    - inconsistent keying info in attribute cache"
E_RD0114_HASH:SS50000_MISC_ERRORS
"consistency check
\n    - inconsistent attribute hash info"
E_RD0116_0INDEX:SS50000_MISC_ERRORS
"consistency check - request for 0 index tuples"
E_RD0117_STREAMID:SS50003_EXHAUSTED_RESOURCE
"consistency check
\n    - cannot find stream id for memory allocation"
E_RD0118_ULMOPEN:SS50000_MISC_ERRORS
"consistency check
\n    - error calling ULM open"
E_RD0119_QOPEN_INFO:SS50000_MISC_ERRORS
"consistency check
\n    - incorrect self-index in qopen_info array"
E_RD011A_ULH_RELEASE:SS50000_MISC_ERRORS
"consistency check
\n    - error releasing ULH object"
E_RD011B_PRIVATE:SS50000_MISC_ERRORS
"consistency check
\n    - private memory stream expected"
E_RD011C_ULM_CLOSE:SS50000_MISC_ERRORS
"consistency check
\n    - error closing private memory stream"
E_RD011D_DAPERMIT:SS50000_MISC_ERRORS
"consistency check
\n    - error defining permit all to all"
E_RD011E_DRPERMIT:SS50000_MISC_ERRORS
"consistency check
\n    - error defining retrieve permit to all"
E_RD011F_DINTEGRITY:SS50000_MISC_ERRORS
"consistency check
\n    - error defining integrity"
E_RD0120_DPERMIT:SS50000_MISC_ERRORS
"consistency check
\n    - error defining permit or security alarm"
/*%
** Parameters:
**	None.
**
** Explanation:
**	qeu_cprot() returned an error status when attempting to create a
**	permit or a security alarm.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	This is a traceback message.  See other related messages in the
**	error log for the exact nature of the problem.
*/
E_RD0121_DESTROY_VIEW:SS50000_MISC_ERRORS
"consistency check
\n    - error destroying view"
E_RD0122_DESTROY_PERMIT:SS50000_MISC_ERRORS
"consistency check
\n    - error destroying permit or security alarm"
/*%
** Parameters:
**	None.
**
** Explanation:
**	qeu_dprot() returned an error status when attempting to drop a
**	permit or a security alarm.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	This is a traceback message.  See other related messages in the
**	error log for the exact nature of the problem.
*/
E_RD0123_CREATE_VIEW:SS50000_MISC_ERRORS
"consistency check
\n    - error creating view"
E_RD0124_DESTROY_INTEGRITY:SS50000_MISC_ERRORS
"consistency check
\n    - error destroying integrity"
E_RD0125_EXCEPTION:SS22003_NUM_VAL_OUT_OF_RANGE
"consistency check
\n    - unexpected exception in RDF"
E_RD0126_EXEXCEPTION:SS22003_NUM_VAL_OUT_OF_RANGE
"consistency check
\n    - unexpected exception processing exception in RDF"
E_RD0127_ULM_PALLOC:SS50002_LIMIT_EXCEEDED
"consistency check
\n    - error calling ulm_palloc"
E_RD0128_SERVER_MEMORY:SS50003_EXHAUSTED_RESOURCE
"consistency check
\n    - error releasing server memory to SCF"
E_RD0129_RDF_STARTUP:SS50003_EXHAUSTED_RESOURCE
"consistency check
\n    - error obtaining memory on RDF startup"
E_RD012A_ULH_CLOSE:SS50000_MISC_ERRORS
"consistency check
\n    - error shutting down ULH"
E_RD012B_ULM_CLOSE:SS50000_MISC_ERRORS
"consistency check
\n    - error closing down ULM memory stream for facility"
E_RD012C_ULH_SEMAPHORE:SS50002_LIMIT_EXCEEDED
"consistency check
\n    - cannot get access to ULH semaphore"
E_RD012D_ULH_ACCESS:SS50003_EXHAUSTED_RESOURCE
"consistency check - error from ULH ALIAS command"
E_RD012E_QUERY_TREE:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check - bad query tree"
E_RD012F_QUERY_TREE:SS50000_MISC_ERRORS
"attempt to write out an invalid query tree symbol type"
E_RD0130_PST_LEN:SS50000_MISC_ERRORS
"attempt to write out an invalid query tree symbol type - pst_len incorrect"
E_RD0131_QUERY_TREE:SS50000_MISC_ERRORS
"attempt to write out an invalid query tree header
\n    - an uninitialized field was found"
E_RD0132_QUERY_TREE:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check
\n    - invalid tree node was read in from catalogs"
E_RD0133_QUERY_TREE:SS5000J_INCONSISTENT_DBMS_CAT
"user error
\n    - old version of query tree, update views, integrities and permits"
E_RD0134_INVALID_SECONDARY:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check
\n    - iirelation secondary index is inconsistent"
E_RD0135_CREATE_PROCEDURE:SS50000_MISC_ERRORS
"consistency check
\n    - error writing procedure object to catalogs"
E_RD0136_CONCURRENT_PROC:SS50000_MISC_ERRORS
"consistency check
\n    - invalid concurrent access to procedure object"
E_RD0137_DUPLICATE_PROCS:SS42502_DUPL_OBJECT
"consistency check
\n    - more than one procedure defined with same name"
E_RD0138_QUERYTEXT:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check
\n    - error reading procedure text from iiqrytext"
E_RD0139_DROP_PROCEDURE:SS50000_MISC_ERRORS
"consistency check
\n    - cannot drop procedure due to QEF error"
E_RD013A_UNFIX_PROCEDURE:SS50000_MISC_ERRORS
"consistency check
\n    - cannot unfix procedure"
E_RD013B_BAD_ATTR_COUNT:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check
\n    - bad attribute count of zero for relation"
E_RD013C_BAD_QRYTRE_RNGSIZ:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check
\n    - too many entries in range table for version 3 trees"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Version 3 query trees should never have more than PST_3NUMVARS [30]
**	range table entries.  However, this version 3 query tree has more than
**	30 range entries.  This indicates that the DBMS catalogs are damaged
**	or inconsistent.  Either the tree version is incorrect or the data
**	contained in the tree is corrupted.  RDF cannot guess what is wrong and
**	attempt a recovery.  There is no way to determine what the correct
**	number of range entries is, and reading the rest of the tree depends on
**	knowning this.
**
** System Status:
**	The statement is aborted, and RDF cannot read the query tree.
**
** Recommendation:
**	The DBMS system catalogs are corrupted.  It may not be possible to
**	fix the query tree.  However, the user should be able to find the
**	corresponding query text and recreate the query using tm.
*/
E_RD013D_CREATE_COMMENT:SS50000_MISC_ERRORS
"RDF interface to QEF (QEU) encountered an error while creating a comment"
/*%
** Parameters:
**	None.
**
** Explanation:
**	QEF does most of the work of adding the comment to the DBMS catalogs.
**	QEF returned an error, indicating that it did not successfully add the
**	comment to the DBMS catalog.
**
** System Status:
**	The "CREATE COMMENT" statement is aborted.
**
** Recommendation:
**	The user should examine the QEF error message in the error log
**	and take any corrective action based on that message. The user may also
**	run verifydb to see if the DBMS catalogs are corrupted.
*/
E_RD013E_DROP_COMMENT:SS50000_MISC_ERRORS
"RDF interface to QEF (QEU) encountered an error while dropping a comment"
/*%
** Parameters:
**	None.
**
** Explanation:
**	QEF does most of the work of dropping the comment from the DBMS
**	catalogs.  QEF returned an error, indicating that it did not
**	successfully drop the comment from the DBMS catalog.
**
**	NOTE: Currently, the "DROP COMMENT" statement is implemented by
**	creating a comment with null short and long text.  However, this
**	message is included for future growth.
**
** System Status:
**	The "DROP COMMENT" statement is aborted.
**
** Recommendation:
**	The user should examine the QEF error message in the error log
**	and take any corrective action based on that message. The user may also
**	run verifydb to see if the DBMS catalogs are corrupted.
*/
E_RD013F_MISSING_RELINFO:SS50000_MISC_ERRORS
"Consistency Check
\n    - a request was made for attribute, key, index information for an
\n      attribute name table without first obtaining relation info."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Either PSF or OPF has called rdu_gdesc() and asked for attribute,
**	index, key, or attribute name information without first obtaining
**	relation information.
**
**	NOTE: This is not an RDF error, but a severe logic error of
**	the calling routine.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	This is an internal error; contact Ingres Corporation Technical Support.
*/
E_RD0140_BAD_RELKEY_REQ:SS50000_MISC_ERRORS
"Consistency Check
\n    - a request was made for key information without first obtaining
\n      relation info."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Either PSF or OPF has called rdu_gdesc() and asked for key information
**	without first obtaining attribute information.
**
**	NOTE: This is not an RDF error, but a severe logic error of the
**	calling routine.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	This is an internal error; contact Ingres Corporation Technical Support.
*/
E_RD0141_BAD_ATTHASH_REQ:SS50000_MISC_ERRORS
"Consistency Check
\n    - a request was made for an attribute name table without first
\n      obtaining relation info."
/*%
** Parameters:
**	None.
**
** Explanation:
**	PSF has called rdu_gdesc() and asked for an attribute name hash table
**	without first obtaining attribute information.
**
**	NOTE: This is not an RDF error, but a severe logic error of the
**	calling routine.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	This is an internal error; contact Ingres Corporation Technical Support.
*/
E_RD0142_ULH_ACCESS:SS50000_MISC_ERRORS
"consistency check - error from ULH ACCESS command"
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF has found a synonym and determined what the table ID is for the
**	table that the synonym resolves to.  It is calling ulh_access() to see
**	if the underlying table is already on the cache.  The "not found"
**	error, E_UL0109_NFND, is the only error that is expected here, and that
**	does not cause this error message.  This is an unexpected error.
**
** System Status:
**	The program continues, but the current "CREATE" statement is aborted.
**
** Recommendation:
**	Try stopping the server and restarting it.  There is some sort of
**	cache problem, and restarting the server should clear it.
*/
E_RD0143_CREATE_SYNONYM:SS50000_MISC_ERRORS
"RDF interface to QEF (QEU) encountered an error while creating a synonym"
/*%
** Parameters:
**	None.
**
** Explanation:
**	QEF does most of the work of adding the synonym to the DBMS catalogs.
**	QEF returned an error, indicating that it did not successfully add the
**	synonym to the DBMS catalog.
**
** System Status:
**	The "CREATE SYNONYM" statement is aborted.
**
** Recommendation:
**	The user should examine the QEF error message in the error log
**	and take any corrective action based on that message. The user may also
**	run verifydb to see if the DBMS catalogs are corrupted.
*/
E_RD0144_DROP_SYNONYM:SS50000_MISC_ERRORS
"RDF interface to QEF (QEU) encountered an error while dropping a synonym"
/*%
** Parameters:
**	None.
**
** Explanation:
**	QEF does most of the work of deleting a synonym from the DBMS catalogs.
**	QEF returned an error, indicating that it did not successfully drop the
**	synonym from the DBMS catalog.
**
** System Status:
**	The "DROP SYNONYM" statement is aborted.
**
** Recommendation:
**	The user should examine the QEF error message in the error log
**	and take any corrective action based on that message. The user may also
**	run verifydb to see if the DBMS catalogs are corrupted.
*/
E_RD0145_ALIAS_MEM:SS50003_EXHAUSTED_RESOURCE
"RDF was attempting to define an alias, but ULH returned an error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF did a ulh_define_alias() call, and ULH returned error
**	E_UL0119_ALIAS_MEM.  This is viewed as an error if RDF is attempting
**	to create an alias for a table name.  However, if RDF is attempting to
**	create an alias for a synonym, then this is just a warning.
**
**	In this case, RDF was attempting to create an alias to a table name,
**	so this is an error.  It is probably due to the system not being
**	configured with enough synonym memory space or enough tables in the
**	RDF cache.
**
** System Status:
**	The program continues, and the current statement is aborted.
**
** Recommendation:
**	Use cbf to modify the RDF startup parameters. Specify larger numbers
**	(defaults are tbl_synonyms=1 and max_tbls=300), restart the server,
**	and try again.
*/
E_RD0146_ILLEGAL_TREEVSN:SS5000J_INCONSISTENT_DBMS_CAT
"Consistency check - invalid tree version.\n
This tree version is not supported for this server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The server has encountered a tuple which has a version ID
**	not supported in this server.  For example:
**
**	    - attempting to read a 6.5 database tree with a 6.3 DBMS server
**	    - attempting to read a non-Star database tree with a
**	      Star server
**
**	This will only occur if the installation is set up to illegally
**	access a database that is not compatible with the current release of
**	the DBMS or Star server.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the user is attempting to use the correct server on this
**	database.
*/
E_RD0147_BAD_QTREE_CLASS:SS5000J_INCONSISTENT_DBMS_CAT
"Consistency check - invalid tree class. Tree must be integrity, permit or rule"
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF was asked to invalidate a QTREE cache class, but the caller did not
**	specify what kind of tree it was.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the user is attempting to use the correct server on this
**	database.
*/
E_RD0148_BAD_QTREE_ALIAS:SS5000J_INCONSISTENT_DBMS_CAT
"Consistency check - invalid tree alias. Tree must be integrity, permit or rule"
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF was asked to invalidate a QTREE cache alias, but the caller did not
**	specify what kind of tree it was.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Ensure that the user is attempting to use the correct server on this
**	database.
*/
E_RD0149_INVALID_TABLETYPE:SS50000_MISC_ERRORS
"Consistency Check - invalid tree type requested."
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF has been requested to open a DBMS catalog and retrieve tuples for
**	the calling program (usually PSF in qrymod).  RDF keeps track of which
**	catalog to open by using an internal table type code.  This table type
**	must be specified for any open catalog.  (See rdfint.h typedef for
**	RDF_TABLE for possible valid values.)
**
** System Status:
**	RDF refuses the catalog read request, which should generate an
**	internal error to the user and abort the statement.
**
** Recommendation:
**	The user should never see this consistency check.  If they do, they
**	should contact Ingres Corporation Technical Support.
*/
E_RD014A_NONEXISTENT_SYNONYM:SS42505_OBJ_NOT_FOUND
"DROP SYNONYM: Specified synonym does not exist."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The synonym specified in the "DROP SYNONYM" statement does not exist.
**	This message will never be returned to the user -- it is used for
**	inter-facility communication.
**
** System Status:
**	The DBMS will provide the user with a message describing the synonym
**	that could not be dropped and proceed to drop other synonyms (if any)
**	named in the statement.
**
** Recommendation:
** 	None.
*/
E_RD014B_BAD_PARTCAT:SS50000_MISC_ERRORS
"RDF found an inconsistency in the table partitioning catalogs.
\nCatalog: %0c, problem: %1c, table: %2c.%3c"
/*%
** Parameters:
**	%2c.%3c - the partitioned table
**
** Explanation:
**	RDF is trying to read the partitioning catalogs for the
**	table, and has encountered an inconsistency.
**	This indicates incorrect data in the catalogs.  Either
**	the catalogs were improperly manually updated by a
**	privileged user, or there was an Ingres error when constructing
**	the catalog entries.
**
** System Status:
**	The current query will be aborted.  Access to the partitioned
**	table will be impossible until the catalogs are fixed.
**
** Recommendation:
**	The inconsistency in the catalogs will have to be manually
**	fixed.  Contact Ingres Corporation Technical Support.
*/
E_RD014C_PCOPY_ERROR:SS50000_MISC_ERRORS
"RDF_PART_COPY internal error: pointer / allocation mismatch."
/*%
** Parameters:
**	None
**
** Explanation:
**	An internal check within the partition definition copy function
**	failed.  This indicates an internal coding error.
**
** System Status:
**	The current query will be aborted.
**
** Recommendation:
**	This message is an internal error;  contact Technical Support.
*/
E_RD0150_CREATE_RULE:SS50000_MISC_ERRORS
"RDF interface to QEF (QEU) encountered error while creating a rule."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon return from qeu_crule(), QEF indicated a failure.  If this is
**	not a user error (such as a duplicate rule name), this error message
**	will be logged.  Parameters to qeu_crule() should be checked for in
**	rdf_update(), rdr_append(), and rdr_rule().
**
** System Status:
**	The program continues, and the current "CREATE" statement is aborted.
**
** Recommendation:
**	If a duplicate rule name was entered, try the statement again without
**	the duplicated rule name.
*/
E_RD0151_DROP_RULE:SS50000_MISC_ERRORS
"RDF interface to QEF (QEU) encountered error while dropping a rule."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon return from qeu_drule(), QEF indicated a failure.  If this is
**	not a user error (such as an invalid rule name), this error message
**	will be logged.  Parameters to qeu_drule() should be checked for in
**	rdf_update(), rdr_delete(), and rdr_rule().
**
** System Status:
**	The program continues, and the current "DROP" statement is aborted.
**
** Recommendation:
**	If an invalid rule name was entered, try again with a different
**	rule name.
*/
E_RD0154_DROP_SCHEMA:SS50000_MISC_ERRORS
"RDF interface to QEF (QEU) encountered error while dropping a schema."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon return from qeu_dschema(), QEF indicated a failure.  If this is
**	not a user error (such as non-existent schema or an attempt to perform
**	a restricted destruction of a schema with objects still in it), this
**	error message will be logged.  Parameters to qeu_dschema() should be
**	checked for in rdf_update(), rdr_delete(), and rdr2_schema().
**
** System Status:
**	The program continues, and the current "DROP SCHEMA" statement is
**	aborted.
**
** Recommendation:
**	If an invalid schema name was entered, try again with a different
**	schema name.
*/
E_RD0160_AGROUP:SS50000_MISC_ERRORS
"Internal QEF error detected while appending tuples to iiusergroup"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when appending tuples to the user group catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0161_AAPLID:SS50000_MISC_ERRORS
"Internal QEF error detected while appending tuples to iirole"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when appending tuples to the role catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0162_DGROUP:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting tuples from iiusergroup"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when deleting tuples from the user group catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0163_DAPLID:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting tuples from iirole"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when purging tuples from the role catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0164_PGROUP:SS50000_MISC_ERRORS
"Internal QEF error detected while purging tuples in iiusergroup"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when purging tuples from the user group catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0165_RAPLID:SS50000_MISC_ERRORS
"Internal QEF error detected while replacing tuples in iirole"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when replacing tuples in the role catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0166_GDBPRIV:SS50000_MISC_ERRORS
"Internal QEF error detected while granting database privileges."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected in QEF while granting database privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0167_RDBPRIV:SS50000_MISC_ERRORS
"Internal QEF error detected while revoking database privileges."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected in QEF while revoking database privileges.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0168_AUSER:SS50000_MISC_ERRORS
"Internal QEF error detected while appending tuples to iiuser"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when appending tuples to the user catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0169_DUSER:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting tuples from iiuser"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when deleting tuples from the user catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD016A_RUSER:SS50000_MISC_ERRORS
"Internal QEF error detected while replacing tuples in iiuser"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when replacing tuples in the user catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD016B_ALOCATION:SS50000_MISC_ERRORS
"Internal QEF error detected while appending tuples to iilocation"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when appending tuples to the location catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD016C_DLOCATION:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting tuples from iilocation"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when deleting tuples from the location catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD016D_RLOCATION:SS50000_MISC_ERRORS
"Internal QEF error detected while replacing tuples in iilocation"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when replacing tuples in the location catalog.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD016E_RSECSTATE:SS50000_MISC_ERRORS
"Internal QEF error detected while replacing tuples in iisecuritystate"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when replacing tuples.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD016F_R_DBP_STATUS:SS50000_MISC_ERRORS
"Internal QEF error detected while updating status of a database procedure in \n
iiproceure, iidbdepends or iipriv"
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if an
**	error is detected when updating the status of a database procedure
**	that involves updating a tuple in a system catalog to indicate that a
**	dbproc is active or grantable and possibly inserting a list of
**	objects and privileges on which it depends.  This operation may be 
**	attempted if the dbproc is marked as dormant and an attempt is made 
**	to execute it, or if a dbproc is not marked as grantable and an 
**	attempt is made to grant privileges on it.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0180_ALABAUDIT:SS50000_MISC_ERRORS
"Internal QEF error detected while adding security label auditing in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when updating the status of security-label
**	auditing in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0181_DLABAUDIT:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting security label auditing in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when updating the status of security-label
**	auditing in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0182_APROFILE:SS50000_MISC_ERRORS
"Internal QEF error detected while appending user profile in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when appending a user profile in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0183_DPROFILE:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting user profile in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when deleting a user profile in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0184_RPROFILE:SS50000_MISC_ERRORS
"Internal QEF error detected while updating user profile in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when updating a user profile in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages.  If none, contact Ingres Corporation 
**	Technical Support.
*/
E_RD0185_ASECALARM:SS50000_MISC_ERRORS
"Internal QEF error detected while appending security alarm in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update() issued if an error
**	is detected when appending a security alarm in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages.  If none, contact Ingres Corporation
**	Technical Support.
*/
E_RD0186_DSECALARM:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting security alarm in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update() issued if and error
**	is detected when deleting a security alarm in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0187_RSECALARM:SS50000_MISC_ERRORS
"Internal QEF error detected while updating security alarm in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update() issued if an error
**	is detected when updating a security alarm in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0188_AROLEGRANT:SS50000_MISC_ERRORS
"Internal QEF error detected while appending role grant in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when appending a role grant in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD0189_DROLEGRANT:SS50000_MISC_ERRORS
"Internal QEF error detected while deleting role grant in RDF."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a traceback message from rdf_update(), issued if
**	an error is detected when deleting a role grant in RDF.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	See the related messages in the error log.  If no related
**	messages are found, contact Ingres Corporation Technical Support.
*/
E_RD018A_EVENT_FAIL:SS50002_LIMIT_EXCEEDED
"Not enough space in event queue to synchronize RDF caches on\n
multiple servers in installation.  RDF caches may become out\n
of sync.  Increase event_limit with CBF to prevent this condition."
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF uses the server event system to synchronize the RDF
**	caches in multiple servers in the same installation.  If
**	event_limit is not set high enough, some of the events will
*	not be delivered, making it possible for the caches in 
**	multiple servers to become out of sync.
**
** System Status:
**	The statement continues running.
**
** Recommendation:
**	Increase event_limit using CBF, and restart the servers
**	in the installation.
*/
E_RD0200_NO_TUPLES:SS42505_OBJ_NOT_FOUND
"user error - attempt to read query tree tuples but none were found
\n for the query ID, probable cause is associated table has been deleted"
E_RD0201_PROC_NOT_FOUND:SS42505_OBJ_NOT_FOUND
"procedure does not exist"
E_RD0202_ALIAS_DEGRADED:SS50002_LIMIT_EXCEEDED
"WARNING: RDF is running at degraded performance\n
on some queries using synonyms.\n
This can be caused by too much concurrent activity, or by an improperly\n
configured RDF.  The server will still find correct answers, but may\n
take longer to do so."
/*%
** Parameters:
**	None.
**
** Explanation:
**	RDF accepts the startup parameters from config.dat configuration file.
**	One of those parameters, rdf_max_tbls, indicates the maximum number of
**	tables RDF may have on its cache (the default is 300).  Another
**	parameter, tbl_synonyms, indicates how many synonyms may be defined
**	(the default is 1).
**
**	You may have your system configured reasonably, but a lot of concurrent
**	activity is keeping RDF memory usage too high to add another synonym to
**	RDF's memory cache.  Or, your system may not be configured correctly
**	for RDF.
**
**	In either case, you can improve the situation by increasing the
**	number of synonyms permitted, by increasing the values for
**	max_tbls and tbl_synonyms.  The tradeoff is that the DBMS server will
**	use more memory when you increase these values.
**
** System Status:
**	The statement continues running, but will be a bit slower, because the
**	server will have to go to disk next time it needs this information
**	instead of getting it from memory.
**
** Recommendation:
**	The user may reconfigure RDF startup parameters and restart the server.
*/
E_RD0210_ABANDONED_OBJECTS:SS50000_MISC_ERRORS
"Restricted revocation of privileges could not proceed as it would result
\nin some objects and/or privilege descriptors becoming abandoned."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user issued a "DROP PERMIT" or "REVOKE ... RESTRICT" statement,
**	but destruction of the specified privileges would result in some
**	object(s) and/or privilege descriptors becoming abandoned (which is
**	illegal when the implicit or explicit <drop behavior> is "RESTRICT").
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Check the error log for a description of an object or a permit
**	that would be rendered abandoned by the operation.
*/
E_RD0211_NONEXISTENT_SCHEMA:SS50000_MISC_ERRORS
"Schema specified in DROP SCHEMA statement does not exist."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The schema named in the "DROP SCHEMA" statement does not exist.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Verify that the schema was specified correctly; if not, correct the
**	query and reenter.
*/
E_RD0212_NONEMPTY_SCHEMA:SS50000_MISC_ERRORS
"Restricted destruction of a schema could not proceed because the specified
\nschema contained tables, views, synonyms, dbevents, or database procedures."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user issued a "DROP SCHEMA ... RESTRICT", but the named schema
**	still contains tables, views, synonyms, dbevents, or database
**	procedures. For restricted revocation to succeed, the schema must
**	not contain any objects.
**
** System Status:
**	The statement is aborted.
**
** Recommendation:
**	Check the error log for a description of an object still
**	contains in the schema.  To destroy the schema and all the objects
**	therein, specify a "DROP SCHEMA ... CASCADE" statement.
*/
/*
** Reserve E_RD0250 to E_RD0300 for Titan
*/
E_RD0250_DD_QEFQUERY:SS50000_MISC_ERRORS
"error occurs when calling QEF to process SQL query"
E_RD0251_DD_QEFFETCH:SS50000_MISC_ERRORS
"error occurs when calling QEF to fetch tuple"
E_RD0252_DD_QEFFLUSH:SS50000_MISC_ERRORS
"error occurs when calling QEF to flush buffer"
E_RD0253_DD_NOOBJECT:SS42500_TBL_NOT_FOUND
"no object was found in system catalog"
E_RD0254_DD_NOTABLE:SS42500_TBL_NOT_FOUND
"local table not found in local db"
E_RD0255_DD_COLCOUNT:SS50000_MISC_ERRORS
"error occurs when retrieving column count information"
E_RD0256_DD_COLMISMATCH:SS5000I_UNEXP_LDB_SCHEMA_CHNG
"column count mismatch"
E_RD0257_DD_NOLDB:SS08500_LDB_UNAVAILABLE
"local database was no defined in system catalog"
E_RD0258_DD_NOINGTBL:SS5000J_INCONSISTENT_DBMS_CAT
"object was not defined in system catalog"
E_RD0259_DD_NOPHYTBL:SS5000J_INCONSISTENT_DBMS_CAT
"object was not defined in system catalog"
E_RD025A_ULMSTART:SS50000_MISC_ERRORS
"ulm error"
E_RD025B_ULMSHUT:SS50000_MISC_ERRORS
"ulm error"
E_RD025C_INVALIDDATE:SS22015_INTERVAL_FLD_OVFLOW
"error occurs when converting date"
E_RD025D_INVALID_DATATYPE:SS5000J_INCONSISTENT_DBMS_CAT
"invalid attribute data type"
E_RD025E_QEF_LDBINFO:SS5000J_INCONSISTENT_DBMS_CAT
"error occurs when calling QEF for ldb information"
E_RD025F_NO_LDBINFO:SS5000J_INCONSISTENT_DBMS_CAT
"ldb description was not defined in system catalog"
E_RD0260_QEF_DDBINFO:SS50000_MISC_ERRORS
"error occurs when calling QEF for ddb information"
E_RD0261_QEF_USRSTAT:SS50000_MISC_ERRORS
"error occurs when calling QEF for user stats"
E_RD0262_QEF_IIDBDB:SS50000_MISC_ERRORS
"error occurs when calling QEF for iidbdb information"
E_RD0263_QEF_LDBPLUS:SS50000_MISC_ERRORS
"error occurs when calling QEF for ldb information"
E_RD0264_UNKOWN_OBJECT:SS5000I_UNEXP_LDB_SCHEMA_CHNG
"undefined object type"
E_RD0265_STORAGE_TYPE_ERROR:SS5000J_INCONSISTENT_DBMS_CAT
"undefined storage type"
E_RD0266_NO_INDEXTUPLE:SS5000J_INCONSISTENT_DBMS_CAT
"index tuple not found"
E_RD0267_NO_STATSTUPLE:SS42505_OBJ_NOT_FOUND
"statistics not found"
E_RD0268_LOCALTABLE_NOTFOUND:SS5000I_UNEXP_LDB_SCHEMA_CHNG
"local table not found in LDB"
E_RD0269_OBJ_DATEMISMATCH:SS5000I_UNEXP_LDB_SCHEMA_CHNG
"alter date in the local table mismatches alter date in CDB"
E_RD026A_OBJ_INDEXCOUNT:SS5000J_INCONSISTENT_DBMS_CAT
"consistency check error on index count"
E_RD026B_UNKNOWN_UPDATEMODE:SS50000_MISC_ERRORS
"unknown update mode"
E_RD026C_UNKNOWN_USER:SS28000_INV_AUTH_SPEC
"unauthorized user"
E_RD026D_USER_NOT_OWNER:SS28000_INV_AUTH_SPEC
"user does not own the local table"
E_RD026E_RDFQUERY:SS50000_MISC_ERRORS
"already in tuple fetching mode"
E_RD026F_RDFFETCH:SS50000_MISC_ERRORS
"fetch tuple before sending query"
E_RD0270_RDFFLUSH:SS50000_MISC_ERRORS
"invalid buffer flush request"
E_RD0271_UNKNOWN_STORAGETYPE:SS5000J_INCONSISTENT_DBMS_CAT
"unknown storage type for an object"
E_RD0272_UNKNOWN_DDREQUESTS:SS50000_MISC_ERRORS
"unknown distributed information requests"
E_RD0273_LDBLONGNAME_NOTFOUND:SS5000J_INCONSISTENT_DBMS_CAT
"unable to find ldb long name"
E_RD0274_LDBDESC_NOTFOUND:SS5000J_INCONSISTENT_DBMS_CAT
"unable to find ldb descriptor"
E_RD0275_UNKNOWN_LDBCAPABILITY:SS5000J_INCONSISTENT_DBMS_CAT
"unknown ldb capability"
E_RD0276_INCONSISTENT_CATINFO:SS5000J_INCONSISTENT_DBMS_CAT
"error detected in distributed catalog"
E_RD0277_CANNOT_GET_ASSOCIATION:SS08500_LDB_UNAVAILABLE
"cannot get ldb association"
E_RD0278_MISMATCH_ARCHITECT_ID:SS50000_MISC_ERRORS
"qtree was created by different architecture"
E_RD0279_NO_SESSION_CONTROL_BLK:SS5000B_INTERNAL_ERROR
"unable to retrieve rdf session control block from scf"
E_RD027A_GET_ARCHINFO_ERROR:SS50000_MISC_ERRORS
"error detected when calling qef for architecture info"
E_RD027B_IITABLES_NOTFOUND:SS5000J_INCONSISTENT_DBMS_CAT
"iitables not found in local database"

NO_SQLSTATE_MAPPING

/* Generic message strings, not complete error messages */

S_RDF000_LIST_DEFAULT
"LIST distributions must include a DEFAULT entry"
S_RDF001_ONE_DEFAULT
"LIST distributions may only have one DEFAULT entry"
S_RDF002_LIST_EQ
"LIST distributions may only have equality comparisons"
S_RDF003_LIST_UNIQUE
"Duplicate LIST entries are not allowed"
S_RDF004_RNG_FIRST_LT
"The smallest RANGE value's operator must be < or <="
S_RDF005_RNG_LAST_GT
"The largest RANGE value's operator must be > or >="
S_RDF006_RANGE_OPER
"RANGE distributions must use >, >=, <, or <= operators"
S_RDF007_RANGE_GAP_OVERLAP
"RANGE distributions may not have gaps or overlaps"
