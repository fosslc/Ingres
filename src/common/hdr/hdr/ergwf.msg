SQLSTATE_MAPPING_REQUIRED
/*
**    Message file for Facility code: _GW
**
** History:
**      17-apr-90 (bryanp)
**          Added error messages for RMS Gateway errors.
**      11-jun-1990 (bryanp)
**          more RMS and GWF errors. RMS error messages probably belong in their
**          own RMS error message file...but for now they're here...
**      12-jun-1990 (bryanp)
**          E_GW050E_ALREADY_REPORTED for errors logged through scc_error().
**      20-jun-90 (edwin)
**          Add GW7017.
**      22-apr-92 (schang)
**          GW code merge
**          11-jul-91 (rickh)
**              Added E_GW5013_RMS_BAD_INDEX_VALUE.
**          7-aug-91 (rickh)
**              Added E_GW5014_RMS_RFA_TOO_LONG.
**      17-sep-92 (daveb)
**          Add missing E_GW0600_NO_MEM, and GWM errors in the block
**          starting at 8000.
**      22-sep-92 (daveb)
**          Add more GWM messages.
**	10-nov-92 (robf)
**	    Added back GWSXA messages lost from last time.
**	9-Dec-1992 (daveb)
**	    Redo GWM messages.
**	30-dec-1992 (robf)
**	    Added more GWF errors   
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**      09-apr-1993 (schang)
**          add E_GW0341_ error message
**	23-Aug-1993 (daveb)
**	    change wording of 81c3, and type of arg1.
**	10-oct-93 (robf)
**	    Fixed format for first param in E_GW407D (%s -> %0c)
**	22-oct-93 (dianeh)
**	    Refine text destined for messages.txt file.
**  	11-may-94 (daveb) 63344
**  	    Add messages 834B and 834C for more detectable register errs.
**      16-oct-96 (mcgem01)
**          MISC_ING_ERRORS changed to MISC_ERRORS for the Jasmine/Ingres
**          identity crisis.                                               
**      15-oct-03 (chash01)
**          modify certain rms gateway error messages to print file name,
**          not file internal identifier (ifi).
**	14-Apr-2008 (kschendel)
**	    Add GW0023 placeholder as pass-back thru DMF to QEF;  this
**	    is an indicator that the error msg was already issued.
*/


#define _GW_CLASS  15


E_GW0000_OK:SS50005_GW_ERROR
"Gateway Facility operation has been successfully completed."
E_GW0023_USER_ERROR:SS50005_GW_ERROR
"A user error has occurred."
E_GW0100_GWX_VTERM_ERROR:SS50005_GW_ERROR
"Termination error from Gateway exit."
E_GW0101_GWX_VINFO_ERROR:SS50005_GW_ERROR
"Error obtaining information from Gateway exit."
E_GW0102_GWX_VTABF_ERROR:SS50005_GW_ERROR
"Error formatting extended relation and attribute relations (Gateway exit)."
E_GW0103_GWX_VIDXF_ERROR:SS50005_GW_ERROR
"Error formatting extended index relation (Gateway exit)"
E_GW0104_GWX_VOPEN_ERROR:SS50005_GW_ERROR
"Error opening table (Gateway exit)."
E_GW0105_GWX_VCLOSE_ERROR:SS50005_GW_ERROR
"Error closing table (Gateway exit)."
E_GW0106_GWX_VPOSITION_ERROR:SS50005_GW_ERROR
"Error positioning table (Gateway exit)."
E_GW0107_GWX_VGET_ERROR:SS50005_GW_ERROR
"Error on tuple get (Gateway exit)."
E_GW0108_GWX_VPUT_ERROR:SS50005_GW_ERROR
"Error on tuple put (Gateway exit)."
E_GW0109_GWX_VREPLACE_ERROR:SS50005_GW_ERROR
"Error replacing tuple (Gateway exit)."
E_GW010A_GWX_VDELETE_ERROR:SS50005_GW_ERROR
"Error deleting tuple (Gateway exit)."
E_GW010B_GWX_VBEGIN_ERROR:SS50005_GW_ERROR
"Error on begin transaction (Gateway exit)."
E_GW010C_GWX_VABORT_ERROR:SS50005_GW_ERROR
"Error on abort transaction (Gateway exit)."
E_GW010D_GWX_VCOMMIT_ERROR:SS50005_GW_ERROR
"Error on commit transaction (Gateway exit)."
E_GW010E_GWX_VATCB_ERROR:SS50005_GW_ERROR
"Error allocating TCB (Gateway exit)."
E_GW010F_GWX_VDTCB_ERROR:SS50005_GW_ERROR
"Error deleting TCB (Gateway exit)."
E_GW0110_GWX_VINIT_ERROR:SS50005_GW_ERROR
"Error initializing Gateway (Gateway exit)."
E_GW0111_GWX_VCONNECT_ERROR:SS50005_GW_ERROR
"Error connecting to foreign file or table (Gateway exit)."
E_GW0200_GWF_INIT_ERROR:SS50005_GW_ERROR
"Error initializing Gateway Facility."
E_GW0201_GWF_TERM_ERROR:SS50005_GW_ERROR
"Error terminating Gateway Facility."
E_GW0202_GWS_INIT_ERROR:SS50005_GW_ERROR
"Error initializing session (Gateway Facility)"
E_GW0203_GWS_TERM_ERROR:SS50005_GW_ERROR
"Error terminating session (Gateway Facility)."
E_GW0204_GWT_REGISTER_ERROR:SS50005_GW_ERROR
"Error registering table (Gateway Facility)."
E_GW0205_GWT_REMOVE_ERROR:SS50005_GW_ERROR
"Error removing table (Gateway Facility)."
E_GW0206_GWT_OPEN_ERROR:SS50005_GW_ERROR
"Error opening table (Gateway Facility)."
E_GW0207_GWT_CLOSE_ERROR:SS50005_GW_ERROR
"Error closing table (Gateway Facility)."
E_GW0208_GWI_REGISTER_ERROR:SS50005_GW_ERROR
"Error registering index (Gateway Facility)."
E_GW0209_GWI_REMOVE_ERROR:SS50005_GW_ERROR
"Error removing index (Gateway Facility)."
E_GW020A_GWR_POSITION_ERROR:SS50005_GW_ERROR
"Error positioning table (Gateway Facility)."
E_GW020B_GWR_GET_ERROR:SS50005_GW_ERROR
"Error on tuple get (Gateway Facility)."
E_GW020C_GWR_PUT_ERROR:SS50005_GW_ERROR
"Error on tuple put (Gateway Facility)."
E_GW020D_GWR_REPLACE_ERROR:SS50005_GW_ERROR
"Error on tuple replace (Gateway Facility)."
E_GW020E_GWR_DELETE_ERROR:SS50005_GW_ERROR
"Error on tuple delete (Gateway Facility)."
E_GW020F_GWX_BEGIN_ERROR:SS50005_GW_ERROR
"Error on begin transaction (Gateway Facility)."
E_GW0210_GWX_ABORT_ERROR:SS50005_GW_ERROR
"Error on abort transaction (Gateway Facility)."
E_GW0211_GWX_COMMIT_ERROR:SS50005_GW_ERROR
"Error on commit transaction (Gateway Facility)."
E_GW0212_GWU_INFO_ERROR:SS50005_GW_ERROR
"Error getting information (Gateway Facility)."
E_GW0213_GWU_COPEN_ERROR:SS50005_GW_ERROR
"Error opening extended catalog (Gateway Facility)."
E_GW0214_GWU_DELTCB_ERROR:SS50005_GW_ERROR
"Error deleting TCB (Gateway Facility)."
E_GW0215_GWU_NEWRSB_ERROR:SS50005_GW_ERROR
"Error obtaining new RSB (Gateway Facility)."
E_GW0216_GWU_DELRSB_ERROR:SS50005_GW_ERROR
"Error deleting RSB (Gateway Facility)."
E_GW0217_GWU_ATXN_ERROR:SS50005_GW_ERROR
"Error ensuring transaction begin (Gateway Facility)."
E_GW0218_GWU_CDELETE_ERROR:SS50005_GW_ERROR
"Error deleting from extended catalog (Gateway Facility)."
E_GW0219_GWU_GETTCB_ERROR:SS50005_GW_ERROR
"Error obtaining new TCB (Gateway Facility)."
E_GW021A_GWU_XATTR_BLD_ERROR:SS50005_GW_ERROR
"Error building extended attribute catalog entries (Gateway Facility)."
E_GW021B_GWU_XIDX_BLD_ERROR:SS50005_GW_ERROR
"Error building extended index catalog entries (Gateway Facility)."
E_GW021C_GWF_SVR_INFO_ERROR:SS50005_GW_ERROR
"Error obtaining server information (Gateway Facility)."
E_GW021D_GWU_MAP_IDX_REL_ERROR:SS50005_GW_ERROR
"Error creating extended index catalog tuple (Gateway Facility)."
E_GW021E_GWU_MAP_IDX_ATT:SS50005_GW_ERROR
"Error creating extended attribute catalog tuples for gateway secondary index\n
(Gateway Facility)."
E_GW021F_GWU_FIND_ASSOC_ERROR:SS50005_GW_ERROR
"Error finding associated record stream when performing a join with a gateway
secondary index (Gateway Facility)."
E_GW0220_GWU_BAD_XATTR_OFFSET:SS50005_GW_ERROR
"Error in attid offset for extended catalog attribute - found %0d but
expected between 1 and %1d"
E_GW0221_GWS_ALTER_ERROR:SS50005_GW_ERROR
"Gateway error attempting to alter a session."
/*%
** Parameters:
**      None.
**
** Explanation:
**	This message is logged when attempting to alter the characteristics
**	of a gateway session.  Currently, this occurs when there is an 
**	internal error when translating the name of a gateway core catalog.
**	An additional message is issued that identifies the nature of the
**	translation error along with the name of the catalog.
**
** System Status:
**	The gateway session is terminated.
**
** Recommendation:
**      Inform your INGRES administrator, who should contact Ingres Technical
**      Support. The contents of the error log should be preserved for analysis
**      by Ingres Technical Support.
*/
/*
        (allows the VMS editor's 'section' key to work usefully)
*/

E_GW0300_SCU_MALLOC_ERROR:SS50005_GW_ERROR
"Gateway error requesting memory (SCU_MALLOC); size requested was %0d bytes."
E_GW0301_SCU_MFREE_ERROR:SS50005_GW_ERROR
"Gateway error freeing memory (SCU_MFREE)."
E_GW0302_SCU_INFORMATION_ERROR:SS50005_GW_ERROR
"Gateway error obtaining number of users for this server (SCU_INFORMATION).\n
This number is used to initialize memory for the Gateway Facility."
E_GW0303_SCC_ERROR_ERROR:SS50005_GW_ERROR
"An error occurred in the scc_error() internal function call."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This message is logged when an internal error occurs in the scc_error()
**      call. The scc_error() function is used to send error messages to the
**      user; this error indicates that the internal error occurred during the
**      handling of an error message that was to be sent to the user. The
**      error message in question was NOT sent to the user.
**
**      Error messages in the immediate vicinity of this message in the error
**      log may provide additional details about the operation that was in
**      progress when the error occurred. This information may be helpful to
**      Ingres Technical Support.
**
** System Status:
**      The current query is aborted, and message E_SC0206 is (usually) given
**      to the user who was running the query. Several messages in addition
**      to this message may have been logged in the error log.
**
** Recommendation:
**      Inform your INGRES administrator, who should contact Ingres Technical
**      Support. The contents of the error log should be preserved for analysis
**      by Ingres Technical Support. If the query which generated this error
**      can be determined, that information would be useful as well.
*/

E_GW0310_ULM_STARTUP_ERROR:SS50005_GW_ERROR
"Gateway error on call to ulm_startup().  Size of pool requested is %0d bytes."
E_GW0311_ULM_SHUTDOWN_ERROR:SS50005_GW_ERROR
"Gateway error on call to ulm_shutdown()."
E_GW0312_ULM_OPENSTREAM_ERROR:SS50005_GW_ERROR
"Gateway error opening memory stream (ulm_openstream()).\n
memleft = %0d, sizepool = %1d, blocksize = %2d."
E_GW0313_ULM_CLOSESTREAM_ERROR:SS50005_GW_ERROR
"Gateway error closing memory stream (ulm_closestream()).  memleft = %0d."
E_GW0314_ULM_PALLOC_ERROR:SS50005_GW_ERROR
"Gateway error allocating memory (ulm_palloc()).\n
memleft = %0d, sizepool = %1d, blocksize = %2d, psize = %3d."
E_GW0320_DMR_PUT_ERROR:SS50005_GW_ERROR
"Gateway error from DMR_PUT operation.  tab_base:  %0d, tab_index:  %1d."
E_GW0321_DMR_GET_ERROR:SS50005_GW_ERROR
"Gateway error from DMR_GET operation.  tab_base:  %0d, tab_index:  %1d."
E_GW0322_DMR_DELETE_ERROR:SS50005_GW_ERROR
"Gateway error from DMR_DELETE operation.  tab_base:  %0d, tab_index:  %1d."
E_GW0323_DMR_POSITION_ERROR:SS50005_GW_ERROR
"Gateway error from DMR_POSITION operation.  tab_base:  %0d, tab_index:  %1d."
E_GW0324_DMT_OPEN_ERROR:SS50005_GW_ERROR
"Gateway error from DMT_OPEN operation.  tab_base:  %0d, tab_index:  %1d."
E_GW0325_DMT_CLOSE_ERROR:SS50005_GW_ERROR
"Gateway error from DMT_CLOSE operation.  tab_base:  %0d, tab_index:  %1d."
E_GW0326_DMT_SHOW_ERROR:SS50005_GW_ERROR
"Gateway error from DMT_SHOW operation.\n
Table name:  '%0c'.  Table owner:  '%1c'."
E_GW0327_DMF_DEADLOCK:SS50005_GW_ERROR
"The Gateway received a deadlock error when updating extended system catalogs
during a REGISTER or REMOVE statement.  Retry the statement."
E_GW0330_CSP_SEMAPHORE_ERROR:SS50005_GW_ERROR
"Gateway error obtaining semaphore (CSp_semaphore())."
E_GW0331_CSV_SEMAPHORE_ERROR:SS50005_GW_ERROR
"Gateway error releasing semaphore (CSv_semaphore())."
E_GW0340_ADE_CX_SPACE_ERROR:SS50005_GW_ERROR
"Gateway error from ADE_CX_SPACE operation."
E_GW0341_USER_INTR:SS50005_GW_ERROR
"User issued interrupt detected, operation abort."

/*
        (allows the VMS editor's 'section' key to work usefully)
*/

E_GW0400_BAD_GWID:SS50005_GW_ERROR
"Gateway error:  bad gateway id detected.  Gateway id:  %0d."
E_GW0401_NULL_RSB:SS50005_GW_ERROR
"Gateway internal error:  null RSB detected."
E_GW0402_RECORD_ACCESS_CONFLICT:SS50005_GW_ERROR
"Gateway internal error:  gateway table was accessed illegally."
E_GW0403_NULL_SESSION_ID:SS50005_GW_ERROR
"Gateway internal error:  Null gateway session id detected."
E_GW0500_GW_TRANSACTIONS:SS50005_GW_ERROR
"Gateway informational status:  transaction notification requested."
E_GW0501_NO_GW_TRANSACTIONS:SS50005_GW_ERROR
"Gateway informational status:  no transaction notification required."
E_GW0502_NO_GATEWAY:SS50005_GW_ERROR
"Gateway informational status:  no gateways loaded."
E_GW0503_NONEXISTENT_TABLE:SS50005_GW_ERROR
"The foreign data manager was unable to open this gateway table because
an underlying foreign data file did not exist.\n
Check the syntax of the REGISTER TABLE command for typographical errors."
E_GW0504_LOCK_QUOTA_EXCEEDED:SS50005_GW_ERROR
"The foreign data manager exceeded its quota of system lock resources.\n
Reduce the complexity of the query or re-configure the foreign lock manager
to have additional lock resources."
E_GW0505_FILE_SECURITY_ERROR:SS50005_GW_ERROR
"The foreign data manager detected an file security error.\n
Check the file permissions and file protection for the underlying foreign
data file and the directory, if applicable."
E_GW0506_FILE_SYNTAX_ERROR:SS50005_GW_ERROR
"The foreign data manager detected a syntactic error in the file name or
path name provided with the REGISTER TABLE COMMAND.\n
Check the syntax of the REGISTER TABLE command for typographical errors."
E_GW0507_FILE_UNAVAILABLE:SS50005_GW_ERROR
"The underlying foreign data file is currently unavailable.\n
Retry the query later when other applications are not using the file."
E_GW0508_DEADLOCK:SS50005_GW_ERROR
"The foreign data manager detected a resource enqueuing deadlock and
aborted this query as a result."
E_GW0509_LOCK_TIMER_EXPIRED:SS50005_GW_ERROR
"The request timed out while waiting for the foreign data manager to lock
the required resources."
E_GW050A_RESOURCE_QUOTA_EXCEED:SS50005_GW_ERROR
"A resource quota was exceeded.\n
Gateway resource quotas which may have been exceeded include server memory
resources, foreign data manager memory resources, disk resources, and
other miscellaneous foreign data manager resources (such as number of open
files).\n
Re-configure the system with additional resources or run a less complex
query."
E_GW050B_DUPLICATE_KEY:SS50005_GW_ERROR
"The record which was added to the foreign data file contained a duplicate
key."
E_GW050C_NOT_POSITIONED:SS50005_GW_ERROR
"A request to the foreign data manager to replace or delete a record was
not preceded by a request to the foreign data manager to position the
record stream.\n
This may indicate an internal programming error in the Ingres Gateway
Server."

E_GW050D_DATA_CVT_ERROR:SS50005_GW_ERROR
"A data conversion error occurred while processing gateway data."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error message is used by GWF to pass back to DMF the fact that
**      a data conversion error occurred in the gateway. The data conversion
**      error was handled by taking the following actions:
**
**          1) Reporting a low-level, conversion-specific error message to the
**             user through a direct call to scc_error().
**          2) Aborting the processing of the record which was being accessed.
**          3) Returning without logging any error message text to the error
**	       log.
**
**      This error message indicates to DMF that no further work should be
**      performed on this query; the query should be aborted. However, no
**      error messages need be logged to the error log -- this is a "user"
**      error, not an "internal" error, and needs no traceback.
**
** System Status:
**      The current query will be aborted, if possible (not all gateways can
**      abort queries). A "friendly" error message has already been sent to
**      the user. No subsequent error messages need to be sent or logged for
**      this error.
**
** Recommendation:
**      Determine what caused the data conversion error and correct it. Then
**      re-execute the query. If possible, the gateway has aborted the query
**      and backed out the operations it performed; however, not all gateways
**      support aborting of query operations, so some work may have already
**      been performed by this query.
*/

E_GW050E_ALREADY_REPORTED:SS50005_GW_ERROR
"The Gateway Facility error has already been reported."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This message is used to notify DMF (the caller of GWF) that GWF has
**      encountered an error but has already reported the error. No additional
**      error reporting need be done. DMF should return to its caller (QEF),
**      and query cleanup and recovery should be performed normally.
**
** System Status:
**      The query will be aborted, but all other processing continues normally.
**
** Recommendation:
**      The user should never see this message. If so, it should be reported
**      to Ingres Technical Support as a bug.
*/

E_GW050F_DEFAULT_STATS_USED:SS50005_GW_ERROR
"The foreign file could not be opened.  If this is a 'register table' operation,
then default statistics will be used for the table; if a 'create view', then
the view will be defined successfully.  The table must be accessible before
any queries may be run on it."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This message is used to notify the user that the foreign data file
**      could not be found.  At registration time, this means that a default
**      value for the number of pages will be used.  When creating a view, the
**      view is successfully created based on the table definition.
**
** System Status:
**      Processing continues normally; for a register table operation, default
**      value is used for pages.
**
** Recommendation:
**      REGISTER TABLE:
**      If the user wants a more accurate size estimate for the file, the table
**      should be removed and re-registered when the underlying data file is
**      accessible.
**
**      CREATE VIEW:
**      No user action required.
**
**      The table must be accessible before any queries can be run on it.
*/

/*
        (allows the VMS editor's 'section' key to work usefully)
*/
E_GW0600_NO_MEM:SS50005_GW_ERROR
"Gateway internal error: no memory."
E_GW0610_BAD_TCB_CACHE:SS50005_GW_ERROR
"Gateway internal error:  no TCB found when one was expected."
E_GW0612_NULL_TCB_LIST:SS50005_GW_ERROR
"Gateway internal error:  null TCB list when a TCB was required."
E_GW0613_TCB_IN_USE:SS50005_GW_ERROR
"Gateway internal error:  TCB in use."
E_GW0614_BAD_TCB_ALLOCATE:SS50005_GW_ERROR
"Gateway internal error:  error allocating TCB."
E_GW0623_XCAT_PUT_FAILED:SS50005_GW_ERROR
"Gateway internal error:  extended catalog PUT failed."
E_GW062C_XREL_CLOSE_FAILED:SS50005_GW_ERROR
"Gateway internal error:  extended relation CLOSE failed."
E_GW062E_XATT_CLOSE_FAILED:SS50005_GW_ERROR
"Gateway internal error:  extended attribute CLOSE failed."
E_GW062F_WRONG_NUM_ATTRIBUTES:SS50005_GW_ERROR
"Gateway internal error:  wrong number of attribute entries in extended
attribute catalog."
E_GW0631_XIDX_CLOSE_FAILED:SS50005_GW_ERROR
"Gateway internal error:  extended index CLOSE failed."
E_GW0632_WRONG_NUM_INDEXES:SS50005_GW_ERROR
"Gateway internal error:  wrong number of index entries."
E_GW0633_CANT_FIND_INDEX:SS50005_GW_ERROR
"Gateway internal error:  can't find any index entries."
E_GW0634_NULL_RECORD_STREAM:SS50005_GW_ERROR
"Gateway internal error:  null record stream."
E_GW0641_END_OF_STREAM:SS50005_GW_ERROR
"Gateway information:  end of stream encountered."
E_GW0654_BAD_GW_VERSION:SS50005_GW_ERROR
"Gateway internal error:  this version of the RMS Gateway is incompatible with
current GWF level.  Error initializing gateway."
E_GW0660_BAD_OPERATION_CODE:SS50005_GW_ERROR
"Gateway error:  bad operation code sent to gwf_call()."
E_GW0661_BAD_CB_TYPE:SS50005_GW_ERROR
"Gateway error:  bad control block type sent to gwf_call()."
E_GW0662_BAD_CB_LENGTH:SS50005_GW_ERROR
"Gateway error:  bad control block length sent to gwf_call()."
E_GW0663_INTERNAL_ERROR:SS50005_GW_ERROR
"Gateway internal error:  exception occurred."
E_GW0664_UNKNOWN_OPERATION:SS50005_GW_ERROR
"Gateway error:  unknown operation code."
E_GW0665_UNKNOWN_EXCEPTION:SS50005_GW_ERROR
"Gateway error processing exception:  unknown exception occurred."
E_GW0666_BAD_FLAG_TO_GWU_DELTCB:SS50005_GW_ERROR
"Bad flag (%0d) sent to gwu_deltcb() routine."
E_GW0667_BAD_CHAR_VALUE:SS50005_GW_ERROR
"Bad characteristics value (%0d) sent to gwr_get() routine."
E_GW0668_UNEXPECTED_EXCEPTION:SS50005_GW_ERROR
"An unexpected exception was detected by GWF"
/*
**      C2 Audit gateway error messages
**
*/
E_GW4001_SXA_NO_INDEXES:SS0A000_FEATUR_NOT_SUPPORTED
"An audit table may not be registered as an Index through the SXA gateway."
/*%
** Parameters:
**      None.
**
** Explanation:
**      An attempt was made to use an SXA gateway table as a gateway
**	secondary index. SXA gateway tables are sequential files, so
**	they cannot be used as indexes.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Do not access SXA gateway tables as secondary indexes.
*/
E_GW4002_SXA_NO_UPDATE:SS0A000_FEATUR_NOT_SUPPORTED
"Audit records may not be updated through the SXA audit gateway."
/*%
** Parameters:
**      None.
**
** Explanation:
**	An attempt was made to insert/update/delete an SXA gateway table.
**	SXA gateway tables are read-only, so they cannot be updated.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Do not try to update an SXA gateway table.
*/
E_GW4003_SXA_SCAN_ONLY:SS0A000_FEATUR_NOT_SUPPORTED
"SXA audit gateway tables may only be positioned for scans (sequential
access). You may not perform random access on audit gateway tables."
/*%
** Parameters:
**      None.
**
** Explanation:
**	An attempt was made to access an SXA gateway table for direct
**	(keyed) access. SXA gateway tables are sequential, so they cannot
**	be positioned by key.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Do not attempt keyed operations on SXA gateway tables.
*/
E_GW4004_SXA_NO_REGISTER_UPDATE:SS0A000_FEATUR_NOT_SUPPORTED
"SXA Gateway tables may not be REGISTERED with UPDATE. The SXA Gateway
is read-only"
/*%
** Parameters:
**      None.
**
** Explanation:
**	An attempt was made to REGISTER an SXA gateway table WITH UPDATE.
**	SXA gateway tables are read-only, so they cannot be updated.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Retry the query without the WITH UPDATE clause.
*/
E_GW4005_SXA_NO_REGISTER_JNL:SS0A000_FEATUR_NOT_SUPPORTED
"SXA Gateway tables may not be REGISTERED with JOURNALING. The SXA Gateway
is read-only"
/*%
** Parameters:
**      None.
**
** Explanation:
**	An attempt was made to REGISTER an SXA gateway table WITH JOURNALING.
**	SXA gateway tables are read-only, so they do not have journaling.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Retry the query without the WITH JOURNALING clause.
*/
E_GW4006_SXA_REGISTER_NO_SEC:SS42503_INSUF_PRIV
"You need SECURITY privilege to REGISTER an SXA gateway table"
/*%
** Parameters:
**      %0c - SXA table name
**
** Explanation:
**	An attempt was made to REGISTER a security audit table, but the user
**	does not have sufficient privileges to do so.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Only users with SECURITY privilege may REGISTER a security audit log
**	with the SXA gateway.
*/
E_GW4007_SXA_REGISTER_BAD_TYPE:SS42000_SYN_OR_ACCESS_ERR
"An invalid external type name '%0c' was specified for column name\n
'%1c' when attempting to REGISTER table '%2c' with the SXA Gateway.\n
Please check the legal external types/names and try again."
/*%
** Parameters:
**	%0c - external name
**      %1c - column name
**	%2c - table name
**
** Explanation:
**      While attempting to REGISTER a table, an external (IS-clause) column 
**	name was used that was invalid. 
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Retry the statement using a correct column name.
*/
E_GW4008_SXA_REGISTER_MISMATCH:SS42000_SYN_OR_ACCESS_ERR
"The alternate name '%0c' used for internal attribute '%1c' while \n
attempting to REGISTER table '%2c' with the SXA Gateway is reserved\n
for use by internal attribute '%3c'. Please check the legal\n
attribute names and try again, either with both alternate and internal\n
names matching, or using a different alternate name."
/*%
** Parameters:
**      %0c - alternative column name
**	%1c - internal column name
**	%2c - table name
**	%3c - reserved name
**
** Explanation:
**      While attempting to REGISTER a table, an alternative column name
**	was specified that matches a reserved column name.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Retry the statement using a non-reserved column name.
*/
E_GW4009_SXA_REGISTER_BAD_COLUMN:SS42000_SYN_OR_ACCESS_ERR
"The column name '%0c', specified when trying to REGISTER the
table '%1c' with the SXA Gateway is invalid. Please check the valid
column names and try again"
/*%
** Parameters:
**      %0c - column name
**	%1c - table name
**
** Explanation:
**      While attempting to REGISTER a table, an invalid column name was used.
**	When no IS-clause is used, column names must match the name of the
**	corresponding audit record exactly.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Retry the statement using a correct column name.
*/
E_GW4010_SXA_OPEN_NO_SECURITY:SS42503_INSUF_PRIV
"You need SECURITY privilege to query an SXA audit gateway table '%0c'"
/*%
** Parameters:
**      %0c - SXA table name
**
** Explanation:
**	An attempt was made to query a security audit table, but the user
**	does not have sufficient privileges to do so.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Only users with SECURITY privilege may issue queries against the
**	security audit log.
*/
E_GW4011_SXA_OPEN_NO_INDEXES:SS50005_GW_ERROR
"Table '%0c' is a Security Audit Log. SALs may not be
\nused as secondary indexes through the SXA Gateway."
/*%
** Parameters:
**      %0c - INGRES table name for the log
**
** Explanation:
**      An attempt was made to use an SXA gateway table as a gateway
**	secondary index. SXA gateway tables are sequential files, so
**	they cannot be used as indexes.
**
** System Status:
**      The operation will be rejected..
**
** Recommendation:
**      Do not register SXA gateway tables as secondary indexes.
*/
E_GW4012_SXA_NO_AUDITING:SS50005_GW_ERROR
"Unable to access the Security Audit Log '%0c' for 
\ntable '%1c' because Security Auditing is disabled in the DBMS server. 
\nSecurity Auditing must be turned on before you can access a SAL."
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      The SXA gateway tried to access an SAL, but SXF reported that 
**	security auditing is disabled so no audit logs can be accessed.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Enable security auditing, then restart the DBMS server and reissue
**	the query.
*/
E_GW4050_SXA_NULL_RCB:SS50005_GW_ERROR
"The SXA Gateway routine %0c was passed a NULL RCB. This is an
internal error"
/*%
** Parameters:
**      %0c - internal gateway routine name
**
** Explanation:
**      This message indicates an illegal (NULL) control block; this is
**	illegal and should never occur.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to Ingres Technical Support.
*/
E_GW4051_SXA_BAD_GW_ID:SS50005_GW_ERROR
"The SXA Gateway routine %0c was passed an RCB referring to another
gateway (%1d). This is an internal error."
/*%
** Parameters:
**      %0c - internal gateway routine name
**	%1d - other gateway ID
**
** Explanation:
**      This message indicates that a control block was passed to
**	SXA that is identified as belonging to another gateway.
**	This is illegal and should never occur.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to your Ingres Technical Support.
*/
E_GW4052_SXA_BAD_XREL_LEN:SS50005_GW_ERROR
"The SXA Gateway routine %0c was passed an RCB which contained an invalid
xrelation identifier length. Expected %1d, but got %2d. This may indicate
invalid or mismatched system catalog tuple length for table iigw06_relation."
/*%
** Parameters:
**      %0c - internal gateway routine name
**	%1d - expected length
**	%2d - actual length
**
** Explanation:
**      This message indicates the size of an iigw06_relation catalog record
**	does not match that expected by the internal routine.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to your Ingres Technical Support.
*/
E_GW4053_SXA_BAD_XREL_LEN:SS50005_GW_ERROR
"The SXA Gateway routine %0c was passed an RCB which contained an invalid
xattribute identifier length. Expected %1d, but got %2d. This may indicate
invalid or mismatched system catalog tuple length for table iigw06_attribute."
/*%
** Parameters:
**      %0c - internal gateway routine name
**	%1d - expected length
**	%2d - actual length
**
** Explanation:
**      This message indicates the size of an iigw06_attribute catalog record
**	does not match that expected by the internal routine.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to your Ingres Technical Support.
*/
E_GW4054_SXA_BAD_NUM_COLUMNS:SS50005_GW_ERROR
"The SXA Gateway routine %0c was passed an RCB which contained an invalid
number of columns. Expected between 1 and %1d columns, but found %2d. "
/*%
** Parameters:
**      %0c - internal gateway routine name
**      %1d - maximum allowed number of columns
**      %2d - requested number of columns
**
** Explanation:
**	When registering an SAL with the SXA gateway, too many columns were
**	registered.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Re-try the REGISTER operation with fewer columns.
*/
E_GW4055_SXA_UNEXPECTED_GW_ATT:SS50005_GW_ERROR
"The SXA Gateway found an unexpected gateway attribute while looking for
the '%0c' attribute at offset '%1d'"
/*%
** Parameters:
**      %0c - attribute name
**	%1d - offset
**
** Explanation:
**      This message indicates an unexpected gateway attribute was found.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**	Check that the registration is correct.
*/
E_GW4056_SXA_CANT_GET_USER_PRIVS:SS50005_GW_ERROR
"The SXA Gateway detected an SCF error while trying to get the security
state information for the current user."
/*%
** Parameters:
**      None.
**
** Explanation:
**      The SXA gateway tried to get security state information for the
**	current user from SCF, but the request failed.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Check the INGRES error log for more information. If the problem 
**	persists, report it to Ingres Technical Support.
*/
E_GW4057_SXA_OPEN_WRITE:SS50005_GW_ERROR
"An attempt was made to open the SXA Gateway table '%0c' in WRITE mode.
SXA audit tables are read-only"
/*%
** Parameters:
**      %0c - INGRES gateway table name
**
** Explanation:
**      An attempt was made to open a security log for writing. Security
**	audit logs may not be updated through the SXA gateway, so the operation
**	was rejected.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Only query SXA gateway tables.
*/
E_GW4058_SXA_OPEN_ERROR:SS50005_GW_ERROR
"Unable to open SAL '%0c' for SXA Gateway table '%1c' due to an error
returned by SXF."
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      The SXA gateway tried to open an SAL, but this failed, due to an
**	SXF error. 
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Check the INGRES error log for more information. If the problem 
**	persists report it to Ingres Technical Support.
*/
E_GW4059_SXA_CLOSE_ERROR:SS50005_GW_ERROR
"Unable to close SAL '%0c' for SXA Gateway table '%1c' due to an error
returned by SXF."
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      The SXA gateway tried to close an SAL, but this failed, due to an
**	SXF error. 
**
** System Status:
**      The operation will continue.
**
** Recommendation:
**      Check the INGRES error log for more information. If the problem 
**	persists report it to Ingres Technical Support.
*/
E_GW405A_SXA_BAD_PALLOC:SS50005_GW_ERROR
"Unable to allocate memory from ULF in SXA Gateway, routine '%0c'."
/*%
** Parameters:
**      %0c - internal gateway routine name
**
** Explanation:
**      This message indicates no more memory is available to the SXA gateway.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Make more memory available, then try again.
*/
E_GW4060_SXA_CLOSE_NOT_OPEN:SS50005_GW_ERROR
"An attempt was made to close SAL '%0c' for SXA Gateway table '%1c'
when the GWSXA RSB describing the table indicates the SAL is not open."
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      The SXA gateway tried to close a gateway table which
**	had not been opened.  This is illegal and should never occur.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to Ingres Technical Support.
*/
E_GW4061_SXA_POSITION_NOT_OPEN:SS50005_GW_ERROR
"An attempt was made to position SAL '%0c' for SXA Gateway table '%1c'
when the GWSXA RSB describing the table indicates the SAL is not open."
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      The SXA gateway tried to position a gateway table which
**	had not been opened.  This is illegal and should never occur.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to Ingres Technical Support.
*/
E_GW4062_SXA_POSITION_ERROR:SS50005_GW_ERROR
"An error was returned by SXF while trying to position the SAL '%0c'
for the SXA audit gateway table '%1c'."
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      The SXA gateway tried to position a security audit log, but this
**	failed, due to an error from SXF.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Check the INGRES error log for more information. If the problem 
**	persists report it to Ingres Technical Support.
*/
E_GW4063_SXA_GET_NOT_OPEN:SS50005_GW_ERROR
"An attempt was made to get a record from SAL '%0c' for the\n
SXA Gateway table '%1c' when the GWSXA RSB describing the table\n
indicates the SAL is not open"
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      The SXA gateway tried to read a record from a gateway table which
**	had not been opened.  This is illegal and should never occur.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to Ingres Technical Support.
*/
E_GW4064_SXA_READ_ERROR:SS50005_GW_ERROR
"An error was returned by SXF while trying to read record %0d from
the SAL '%1c' for the SXA audit gateway table '%2c'"
/*%
** Parameters:
**	%0d - record number
**      %1c - security audit log
**      %2c - INGRES table name for the log
**
** Explanation:
**      The SXF facility returned an error while trying to read an SAL.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Correct the problem that prevented the record from being read.
*/
E_GW4065_SXA_TO_INGRES_ERROR:SS22000_DATA_EXCEPTION
"An error occurred converting SAL record information to Ingres data format."
/*%
** Parameters:
**      None.
**
** Explanation:
**      A security audit record could not be converted to INGRES format.
**	An earlier message should indicate why.
**
** System Status:
**      The data will not be retrieved.
**
** Recommendation:
**      Correct the problem reported in an earlier message that prevented
**	the log from being queried.
*/
E_GW4066_SXA_CVT_ERROR:SS22000_DATA_EXCEPTION
"An error occurred converting a SAL record to Ingres format.\n
The column which could not be converted was column %0d, name '%1c'.\n
The Ingres datatype is %2d, SAL datatype was %3d"
/*%
** Parameters:
**      %0d - attribute ID
**      %1c - attribute name
**	%2d - INGRES datatype
**	%3d - audit record datatype
**
** Explanation:
**      This message indicates the data from a security audit log column
**	cannot be converted to the registered INGRES datatype. A previous
**	message should indicate the exact cause of the error.
**
** System Status:
**      The operation will be rejected.
**
** Recommendation:
**      Re-issue the REGISTER statement with a compatible INGRES datatype.
*/
E_GW4067_SXA_NO_REGISTER_KEYED:SS50005_GW_ERROR
"You cannot specify a keyed storage structure when REGISTERing an\n
SXA Gateway table."
/*%
** Parameters:
**      None.
**
** Explanation:
**      When issuing a REGISTER TABLE statement, a storage structure other
**	than NONE was issued. Audit logs cannot be registered as a keyed
**	storage structure.
**
** System Status:
**      The REGISTER operation will be rejected.
**
** Recommendation:
**      Correct the REGISTER TABLE WITH clause, and retry the operation.
*/
E_GW4077_SXA_BAD_ATTID:SS50005_GW_ERROR
"The SXA Gateway detected an invalid internal attribute id %0d for
attribute '%1c' while performing the conversion to Ingres data."
/*%
** Parameters:
**      %0d - attribute ID
**	%1c - attribute name
**
** Explanation:
**      While converting an audit record to INGRES format, the SXA
**	gateway detected an illegal attribute ID; that is, an attribute
**	which does not correspond to any of the allowed audit record 
**	attributes. This could be caused by an internal error, or by
**	a corrupted iigw06_attribute table.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Try to REMOVE then REGISTER the table again. If the problem still
**	occurs, report this problem to Ingres Technical Support.
*/
E_GW4078_SXA_BAD_SCF_INFO:SS50005_GW_ERROR
"The SXA Gateway received an error return from a SCF INFORMATION request.
The information requested was '%0c'"
/*%
** Parameters:
**      %0c - information requested
**
** Explanation:
**      SXF made an information request to SCF which returned an error.
**	This should never occur.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to Ingres Technical Support.
*/
E_GW4079_SXA_NULL_EXTENDED_RSB:SS50005_GW_ERROR
"The SXA Gateway routine '%0c' was passed a NULL Extended RSB.\n
This is an internal error."
/*%
** Parameters:
**      %0c - internal gateway routine name
**
** Explanation:
**      This message indicates an illegal (NULL) control block was
**	passed to an SXA gateway routine. This is illegal and should never 
**	occur.
**
** System Status:
**      The operation will not complete.
**
** Recommendation:
**      Report this problem to Ingres Technical Support.
*/
E_GW407A_SXA_BAD_MO_ATTACH:SS50005_GW_ERROR
"The SXA Gateway was unable to attach to the INGRES MIB using MOattach.
\nSXA information will not be available in the MIB."
/*%
** Parameters:
**      None.
**
** Explanation:
**      At DBMS startup, the SXA gateway registers itself with the INGRES
**	MIB; this process failed.
**
** System Status:
**      Startup continues, but the SXA gateway will not be available in the
**	INGRES MIB.
**
** Recommendation:
**      Correct the problem that prevented the connection to the MIB.
*/
E_GW407B_SXA_LOG_NOT_EXIST:SS50005_GW_ERROR
"The Security Audit Log '%0c' 
\nfor SXA Gateway table '%1c' does not exist or is not available."
/*%
** Parameters:
**      %0c - security audit log
**      %1c - INGRES table name for the log
**
** Explanation:
**      This message indicates the security audit log either does not
**	exist or is unavailable.
**
** System Status:
**      The file (table) cannot be opened; access to the file will not be
**      allowed.
**
** Recommendation:
**      Correct the problem that prevented the file from being opened.
*/
E_GW407C_SXA_GET_BY_TID:SS50005_GW_ERROR
"An SXA Gateway SAL record was fetched by TID. Since SALS are sequential 
and cannot be randomly accessed by TID, this operation is emulated by 
scanning the file until the TID is found."
E_GW407D_SXA_NO_PRIV:SS42503_INSUF_PRIV
"%0c: This is a privileged operation for Security Audit Logs.\n
You do not have sufficient privilege to perform this operation on\n
SXA table %1c."
/*%
** Parameters:
**	%0c - attempted operation
**	%1c - audit table name
**
** Explanation:
**	A user who does not have the required privileges attempted to
**	access the security audit logs.
**
** System status:
**	The operation is rejected.
**
** User action:
**	Only try to access SAL tables if you have the required privileges.
*/
/*
** RMS Gateway errors. These error codes are generated by the RMS Gateway
** exits. Some of these error codes are checked for by GWF and mapped back
** into 'appropriate' GWF error codes for return to DMF. No error code
** requiring parameters is returned to GWF; all errors requiring parameter
** substitution are logged directly by the RMS Gateway exits.
**
** NOTE: RMS Gateway exits also may return one of the 'standard' exit errors
**       (e.g., E_GW0104_GWX_VOPEN_ERROR).
**
** Message numbers E_GW5000 through E_GW57FF are reserved for the RMS
** Gateway's use. Of these, messages E_GW5000 through E_GW53FF are reserved
** for internal RMS Gateway messages requiring full parameter substitution,
** while E_GW5400 through E_GW57FF are reserved for the so-called 'traceback'
** messages which are returned to GWF.
*/

E_GW5000_RMS_OPEN_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$OPEN RMS Service Call.\n
The RMS Gateway attempted to open file %0c with
file access %1x and file sharing %2x.\n
The RMS error code was %3c."
/*%
** Parameters:
**      %0c - file name
**      %1x - file access code (in hex)
**      %2x - file sharing code (in hex)
**      %3c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$OPEN service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The file (table) cannot be opened. Access to the file will not be
**      allowed, but the RMS gateway can still be used to access other
**      RMS files.
**
** Recommendation:
**      Correct the problem that prevented the file from being opened.
*/

E_GW5001_RMS_CLOSE_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$CLOSE RMS Service Call.\n
The RMS Gateway attempted to close file %oc
with file processing options %1x.\n
The RMS error code was %2c."
/*%
** Parameters:
**      %0c - file name.
**      %1x - file processing options (in hex)
**      %2c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$CLOSE service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The file (table) cannot be closed. Depending on the reason for the
**      problem, the file may have been damaged. Access to the file will
**      not be allowed, but the RMS gateway can still be used to access
**      other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the file from being closed.
*/

E_GW5002_RMS_CONNECT_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$CONNECT RMS Service Call.\n
The RMS Gateway attempted to connect to file %0c
with record options %1x.\n
The RMS error code was %2c."
/*%
** Parameters:
**      %0c - file name
**      %1x - record processing options
**      %2c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$CONNECT service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The file (table) cannot be used by this transaction. The transaction
**      will be aborted. Other transactions may or may not be able to connect
**      to this file, but the RMS gateway can still be used to access other
**      RMS files.
**
** Recommendation:
**      Correct the problem that prevented the file from being connected to.
*/

E_GW5003_RMS_DISCONNECT_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$DISCONNECT RMS Service Call.\n
The RMS error code was %0c."
/*%
** Parameters:
**      %0c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$DISCONNECT service call. The message provides the RMS error
**      code that was received by the SYS$DISCONNECT call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not have been damaged. If it has
**      not been damaged, access to the file by other transactions will still
**      be allowed, and the RMS gateway can still be used to access other
**      RMS files.
**
** Recommendation:
**      Correct the problem that prevented the file from being disconnected.
*/

E_GW5004_RMS_FIND_SEQ_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$FIND RMS Service Call.\n
The RMS Gateway attempted to position stream ID=%0x
for a sequential scan.\n
The RMS error code was %1c."
/*%
** Parameters:
**      %0x - internal stream identifier (in hex)
**      %1c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$FIND service call when an RMS file is being positioned for
**      a sequential scan. The message provides the RMS error code and the
**      internal record stream identifier which was being positioned.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the file from being positioned.
*/

E_GW5005_RMS_FIND_KEY_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$FIND RMS Service Call.\n
The RMS Gateway attempted to position stream ID=%0x
on key %1x with value %2c.\n
The RMS error code was %3c."
/*%
** Parameters:
**      %0x - internal stream identifier (in hex)
**      %1x - key reference number (in hex)
**      %2c - key value
**      %3c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$FIND service call when an RMS file is being positioned for
**      an indexed search (a search where a key value was used to select
**      a subset of the file). The message provides the RMS error code and
**      the key number and key value which were provided to the call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the file from being positioned.
*/

E_GW5006_RMS_GET_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$GET RMS Service Call.\n
The RMS Gateway attempted to get a record from file
%0c into the buffer at %1x whose
length is %2x with record access mode %3x.\n
The RMS error code was %4c."
/*%
** Parameters:
**      %0c - file name
**      %1x - record buffer address (in hex)
**      %2x - record buffer size (in hex)
**      %3x - record access code (in hex)
**      %4c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$GET service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the record from being retrieved.
*/

E_GW5007_RMS_PUT_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$PUT RMS Service Call.\n
The RMS Gateway attempted to put a record into file
%0c from the buffer at %1x whose
length is %2x with record access mode %3x.\n
The RMS error code was %4c."
/*%
** Parameters:
**      %0c - file name
**      %1x - record buffer address (in hex)
**      %2x - record buffer size (in hex)
**      %3x - record access code (in hex)
**      %4c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$PUT service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the record from being appended.
*/

E_GW5008_RMS_UPDATE_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$UPDATE RMS Service Call.\n
The RMS Gateway attempted to update a record in file
%0c from the buffer at %1x whose
length is %2x with record access mode %3x.\n
The RMS error code was %4c."
/*%
** Parameters:
**      %0c - file name
**      %1x - record buffer address (in hex)
**      %2x - record buffer size (in hex)
**      %3x - record access code (in hex)
**      %4c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$UPDATE service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the record from being updated.
*/

E_GW5009_RMS_DELETE_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$DELETE RMS Service Call.\n
The RMS Gateway attempted to delete a record in file
%0c with record processing options %1x.\n
The RMS error code was %2c."
/*%
** Parameters:
**      %0c - file name
**      %1x - record processing options
**      %2c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$DELETE service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the record from being deleted.
*/

E_GW500A_RMS_FLUSH_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$FLUSH RMS Service Call.\n
The RMS Gateway attempted to flush the I/O Buffers for
file %0c associated with stream ID=%1x
and with record processing options %2x.\n
The RMS error code was %3c."
/*%
** Parameters:
**      %0c - file name
**      %1x - internal stream ID
**      %2x - record processing options
**      %3c - RMS error code
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$FLUSH service call. The message provides the pertinent
**      parameters which were provided to the call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the buffers from being flushed.
*/

E_GW500B_RMS_FILE_NOT_INDEXED:SS50005_GW_ERROR
"The RMS Gateway was requested to perform an indexed search
on file %0c, but the file's organization (%1x)
does not indicate an indexed structure.\n
The search operation is rejected."
/*%
** Parameters:
**      %0c - file name
**      %1x - file organization (in hex)
**
** Explanation:
**      This message is issued when an indexed search request is made on
**      a non-indexed file. Such a search cannot be performed.
**
** System Status:
**      The transaction is aborted, but all other activity is unaffected.
**
** Recommendation:
**      Change the application to perform a different search.
*/

E_GW500C_RMS_CVT_TO_ING_ERROR:SS50005_GW_ERROR
"An error occurred converting an RMS record to Ingres format.\n
Record %0x of size %1x from file %2c could not be converted."
/*%
** Parameters:
**      %0x - record number (in hex)
**      %1x - record size, in bytes (in hex)
**      %2c - file name
**
** Explanation:
**      This message is issued when INGRES encounters problems while
**      attempting to convert a record from RMS datatypes into the
**      appropriate INGRES datatypes.
**
** System Status:
**      The transaction is aborted, but all other activity is unaffected.
**
** Recommendation:
**      Update the record to contain data which can be converted without
**      error.
*/


E_GW500D_CVT_TO_ING_ERROR:SS50005_GW_ERROR
"An error occurred converting an RMS record to Ingres format.\n
The column which could not be converted was
column %0d, with Ingres datatype %1d, Ingres precision %2d,
Ingres length %3d, Ingres record offset %4d.\n
The RMS specification was RMS datatype %5d, RMS precision %6d,
RMS length %7d, and RMS record offset %8d."
/*%
** Parameters:
**      %0d - column number of the bad column (from 0)
**      %1d - internal INGRES datatype ID
**      %2d - internal INGRES precision value
**      %3d - internal INGRES length, in bytes
**      %4d - offset of the INGRES column in the row, in bytes
**      %5d - internal INGRES datatype ID of the RMS field
**      %6d - internal INGRES precision of the RMS field
**      %7d - internal INGRES length of the RMS field
**      %8d - offset of the RMS field in the RMS record, in bytes
**
** Explanation:
**      This message is issued when INGRES encounters problems while
**      attempting to convert an RMS record into INGRES format. Each field
**      in the RMS record must be converted to a corresponding INGRES
**      column according to specifications presented in the REGISTER TABLE
**      command. If any of the RMS fields contains inconvertible data,
**      this error message is generated.
**
** System Status:
**      The transaction is aborted, but all other activity is unaffected.
**
** Recommendation:
**      Update the record to contain data which can be converted without
**      error.
*/

E_GW500E_CVT_TO_RMS_ERROR:SS50005_GW_ERROR
"An error occurred converting an Ingres record to RMS format.\n
The column which could not be converted was
column %0d, with Ingres datatype %1d, Ingres precision %2d,
Ingres length %3d, Ingres record offset %4d.\n
The RMS specification was RMS datatype %5d, RMS precision %6d,
RMS length %7d, and RMS record offset %8d."
/*%
** Parameters:
**      %0d - column number of the bad column (from 0)
**      %1d - internal INGRES datatype ID
**      %2d - internal INGRES precision value
**      %3d - internal INGRES length, in bytes
**      %4d - offset of the INGRES column in the row, in bytes
**      %5d - internal INGRES datatype ID of the RMS field
**      %6d - internal INGRES precision of the RMS field
**      %7d - internal INGRES length of the RMS field
**      %8d - offset of the RMS field in the RMS record, in bytes
**
** Explanation:
**      This message is issued when INGRES encounters problems while
**      attempting to convert an INGRES record into RMS format. Each column
**      in the INGRES row must be converted to a corresponding RMS
**      field according to specifications presented in the REGISTER TABLE
**      command. If any of the INGRES columns contains inconvertible data,
**      this error message is generated.
**
** System Status:
**      The transaction is aborted, but all other activity is unaffected.
**
** Recommendation:
**      Update the record to contain data which can be converted without
**      error.
*/

E_GW500F_CHECK_ACCESS_ERRPR:SS50005_GW_ERROR
"An error occurred in the SYS$CHECK_ACCESS System Service Call.  The RMS
Gateway attempted to open file %0c on behalf of user %1c.  The error code
was SS$_NOPRIV for access type %2c. "
/*%
** Parameters:
**      %0c - file name
**      %1c - user name
**      %2c - file access requested
**
** Explanation:
**      This message is issued when the user does not have permission to
**      access the file in the requested access mode.
**
** System Status:
**      The query is aborted, but all other activity is unaffected.
**
** Recommendation:
**      Contact your INGRES system administrator to arrange appropriate access
**      to the data file.
*/

E_GW5010_UNSUPPORTED_FILE_TYPE:SS50005_GW_ERROR
"Attempt was made to access an unsupported RMS file type.  Supported file types
are:  indexed, sequential and relative."

E_GW5011_CVT_KEY_TO_RMS_ERROR:SS50005_GW_ERROR
"An error occurred converting Ingres key columns to RMS format.  The column
which could not be converted was column %0d, with Ingres datatype %1d, Ingres
precision %2d, Ingres length %3d, Ingres record offset %4d.  The RMS
specification was RMS datatype %5d, RMS precision %6d, RMS length %7d, and RMS
record offset %8d."
/*%
** Parameters:
**      %0d - column number of the bad column (from 0)
**      %1d - internal INGRES datatype ID
**      %2d - internal INGRES precision value
**      %3d - internal INGRES length, in bytes
**      %4d - offset of the INGRES column in the row, in bytes
**      %5d - internal INGRES datatype ID of the RMS field
**      %6d - internal INGRES precision of the RMS field
**      %7d - internal INGRES length of the RMS field
**      %8d - offset of the RMS field in the RMS record, in bytes
**
** Explanation:
**      This message is issued when INGRES encounters problems while attempting
**      to convert an INGRES key column into RMS format. Each key column must
**      be converted to a corresponding RMS field according to specifications
**      presented in the REGISTER TABLE command. If any of the INGRES key
**      columns contains inconvertible data, this error message is generated.
**
** System Status:
**      The transaction is aborted, but all other activity is unaffected.
**
** Recommendation:
**      Update the record to contain data which can be converted without error.
*/


E_GW5012_RMS_REWIND_ERROR:SS50005_GW_ERROR
"An error occurred in the SYS$REWIND RMS Service Call.\n
The RMS Gateway attempted to position stream ID=%0x.\n
The RMS error code was %1c."
/*%
** Parameters:
**      %0x - internal stream identifier (in hex)
**      %1c - RMS error code (in hex)
**
** Explanation:
**      This is the standard error message used to log any error in the
**      RMS SYS$FIND service call when an RMS file is being positioned for
**      an indexed search (a search where a key value was used to select
**      a subset of the file). The message provides the RMS error code and
**      the key number and key value which were provided to the call.
**
** System Status:
**      The transaction will be aborted. Depending on the reason for the
**      error, the RMS file may or may not be usable by other transactions.
**      The RMS gateway can still be used to access other RMS files.
**
** Recommendation:
**      Correct the problem that prevented the file from being positioned.
*/

E_GW5013_RMS_BAD_INDEX_VALUE:SS50005_GW_ERROR
"The FROM field must be an RMS key reference number (ranging\n
between 1 and 254, inclusive).  '%0c' is not a valid key\n
reference number."
/*%
** Parameters:
**      %0c - user input
**
** Explanation:
**      An invalid key reference number in the "from" field of a
**      REGISTER INDEX statement for an RMS secondary index was
**      encountered.
**
** System Status:
**      The index registration will be aborted.
**
** Recommendation:
**      Re-enter the index registration with a valid key reference number.
*/


E_GW5014_RMS_RFA_TOO_LONG:SS50005_GW_ERROR
"RFA out of bounds.  The maximum block number supported\n
by the RMS Gateway is %0x and the maximum supported\n
record offset is %1x.  A record was fetched whose virtual\n
block number was %2x and whose offset into that\n
block was %3x."
/*%
** Parameters:
**      %0x - maximum block number supported by the gateway (in hex)
**      %1x - maximum record offset supported by the gateway (in hex)
**      %2x - actual block number of fetched record (in hex)
**      %3x - actual offset into that block of fetched record (in hex)
**
** Explanation:
**      We must map 6-byte RMS RFAs into 4-byte INGRES TIDs.  A
**      record was fetched whose virtual block number (as encoded
**      in its RFA) was too big to be mapped into a TID.
**
** System Status:
**      The query is aborted.
**
** Recommendation:
**      Break the file up into smaller files, none of which has more
**      virtual blocks than the gateway can support.
**
*/


/*

** RMS Gateway traceback messages, E_GW5400 through E_GW57FF:
*/
E_GW5400_RMS_FILE_ACCESS_ERROR:SS50005_GW_ERROR
"File access error (RMS$_ACC). A disk error or other hardware error
prevented the file from being accessed."

E_GW5401_RMS_FILE_ACT_ERROR:SS50005_GW_ERROR
"File activity precludes operation; operation attempted during other file
I/O. (RMS$_ACT)"

E_GW5402_RMS_BAD_DEVICE_TYPE:SS50005_GW_ERROR
"Bad device or inappropriate device type for operations (RMS$_DEV)"

E_GW5403_RMS_BAD_DIRECTORY_NAME:SS50005_GW_ERROR
"Error in directory name; the directory name was specified incorrectly.\n
For example, a directory or subdirectory name exceeded 39 characters in
length, or the use of a logical name resulted in more than one directory
name in the file specification.\n
Verify the syntax of either the directory name in the file specification
or the logical name, if applicable (RMS$_DIR)."

E_GW5404_RMS_OUT_OF_MEMORY:SS50005_GW_ERROR
"Dynamic memory exhausted. Occurs only if the reslated I/O segment in the
control region is full and the file is either a direct access
process-permanent file or the user has disallowed the use of the
program region for I/O buffers to RMS (RMS$_DME)."

E_GW5405_RMS_DIR_NOT_FOUND:SS50005_GW_ERROR
"Directory not found; specified directory name does not exist on the
specified device.\n
Verify that the device and directory are specified correctly.\n
Create the directory on that device or use a different directory.\n
Consider using a search list logical name if the same file exists in
multiple device/directory locations (RMS$_DNF)."

E_GW5406_RMS_FILE_LOCKED:SS50005_GW_ERROR
"File currently locked by another user; the program attempted to create or
open a file that is curently being accessed by another user, and the file
was not accessed for sharing.\n
Wait until the other user deaccesses the file and retry the operation
(RMS$_FLK)."

E_GW5407_RMS_FILE_NOT_FOUND:SS50005_GW_ERROR
"File not found. Check the file specification and verify that the device,
directory, file name, and file type are correct.\n
If a logical name was
specified, verify that the current equivalence name assigned to the logical
name is correct.\n
Verify that the correct volume is mounted and that the file was not
inadvertently deleted (RMS$_FNF)."

E_GW5408_RMS_BAD_FILE_NAME:SS50005_GW_ERROR
"Syntax error in file name (RMS$_FNM, RMS$_SYN)."

E_GW5409_RMS_LOGICAL_NAME_ERROR:SS50005_GW_ERROR
"Logical name error; an error was detected in file name processing.\n
Usually this is caused by exceeding the maximum logical name translation
count, typically when the equivalence string for a logical name is
identical to the name itself.\n
This error can also indicate a fatal error in logical name translation,
or invalid syntax for a process-permanent file equivalence string
(RMS$_LNE)."

E_GW540A_RMS_FILE_PROTECT_ERROR:SS50005_GW_ERROR
"Insufficient privilege or file protection violation; the current process
is denied access because of the file protection.\n
Check the access rights associated with the file and change them if
needed.\n
If you are not the file owner or have insufficient privilege, request that
the file owner change the protection (RMS$_PRV)."

E_GW540B_RMS_FILE_SUPERSEDED:SS50005_GW_ERROR
"Created file supersedes existing version of same file; no action is
required (success) (RMS$_SUPERSEDE)."

E_GW540C_RMS_ENQUEUE_LIMIT:SS50005_GW_ERROR
"Exceeded process enqueue limit. Reduce the number of locks held at one
time and retry, or have the system manager increase this limit for your
process. (RMS$_EXENQLM)."

E_GW540D_RMS_DEADLOCK:SS50005_GW_ERROR
"The VMS lock manager detected a deadlock. Modify the source program, if
necessary (RMS$_DEADLOCK)."

E_GW540E_RMS_ALREADY_LOCKED:SS50005_GW_ERROR
"Record is already locked; the program attempted to lock a previously
locked record.\n
No action is usually required, although this can depend on application
requirements (success) (RMS$_OK_ALK)."

E_GW540F_RMS_PREVIOUSLY_DELETED:SS50005_GW_ERROR
"Deleted record successfully accessed; a previously deleted record was read
successfully.\n
No action is usually required, although this can depend on application
requirements (success) (RMS$_OK_DEL)."

E_GW5410_RMS_READ_LOCKED_RECORD:SS50005_GW_ERROR
"Record locked but read anyway; the retrieved record was locked but was read
successfully. No action is required (success). OR, Record locked against
read, but read anyway; a record locked against reading was read
successfully. No action is required (success) (RMS$_OK_RLK, RMS$_OK_RRL)."

E_GW5411_RMS_RECORD_NOT_FOUND:SS50005_GW_ERROR
"Nonexistent record successfully accessed; a nonexistent record was read
successfully. No action is required (success) (RMS$_OK_RNF)."

E_GW5412_RMS_READ_AFTER_WAIT:SS50005_GW_ERROR
"Record locked after wait; the retrieved record was locked but has been
read successfully after a wait period.\n
No action is required (success) (RMS$_OK_WAT)."

E_GW5413_RMS_RECORD_LOCKED:SS50005_GW_ERROR
"Target record currently locked by another stream; the requested record
cannot be accessed. (RMS$_RLK)."

E_GW5414_RMS_TIMED_OUT:SS50005_GW_ERROR
"Timeout period expired; the Find or Get service did not complete. The
requested record was locked for the entire timeout period and was not
read. (RMS$_TMO)."

E_GW5415_RMS_RECORD_TOO_BIG:SS50005_GW_ERROR
"Record too large for user's buffer (warning); the record just returned was
too large for the user buffer provided. The returned record is truncated
to the size of the user buffer (RMS$_RTB)."

E_GW5416_RMS_INVALID_DUP_KEY:SS50005_GW_ERROR
"Duplicate key detected; key definition XAB key option flag not set to allow
duplicate key values (RMS$_DUP)."

E_GW5417_RMS_DEVICE_FULL:SS50005_GW_ERROR
"Device full; cannot create or extend file (RMS$_FUL)."

E_GW5418_RMS_DUPLICATE_KEY:SS50005_GW_ERROR
"Record inserted had duplicate key; the record was inserted successfully and
had one or more key values that duplicated the values of other records
(success) (RMS$_OK_DUP)."

E_GW5419_RMS_INDEX_UPDATE_ERROR:SS50005_GW_ERROR
"Index update error occurred; the record was inserted successfully, but an
error occurred on index update, which could cause slow access to that record
using that key path. File reorganization is recommended.\n
(success) (RMS$_OK_IDX)."

E_GW541A_RMS_NO_CURRENT_RECORD:SS50005_GW_ERROR
"No current record; operation not immediately preceded by a successful Get
or Find service (RMS$_CUR)."

E_GW541B_RMS_INTERNAL_MEM_ERROR:SS50005_GW_ERROR
"The RMS Gateway encountered an error allocating internal server memory.\n
Check the Ingres GWF Facility memory limits, and increase them if
necessary."

E_GW541C_RMS_DELETE_FROM_SEQ:SS50005_GW_ERROR
"An attempt was made to delete a record from a sequential RMS file.  RMS does
not allow this."

E_GW541D_RMS_INV_REC_SIZE:SS50005_GW_ERROR
"An insert operation failed because the record size was invalid.  On an RMS put
operation, the record(s) to be inserted cannot be larger than the maximum
record length that was specified when the file was created.  (RMS$_RSZ)"

E_GW541E_RMS_INV_UPD_SIZE:SS50005_GW_ERROR
"An update operation failed because the record size was invalid.  The record
length for sequential files cannot change.  For relative files with
variable-length or variable with fixed-length control records, the length of
the replacement record can be different from the length of the original record,
but cannot be larger than the maximum size you set when you created the file.
(RMS$_RSZ)"
/*
        (allows the VMS editor's 'section' key to work usefully)
*/

/*
** Message numbers starting with E_GW5480 are in rmsmap.c
*/

E_GW5480_RMSCX_TO_INGRES_ERROR:SS50005_GW_ERROR
"An error occurred building ADF Compiled Expression blocks for use in
converting RMS records to Ingres format (rmscx_to_ingres())."

E_GW5481_RMSCX_TCB_EST_ERROR:SS50005_GW_ERROR
"An error occurred while calculating internal memory space requirements
(rmscx_tcb_estimate())."

E_GW5482_RMS_TO_INGRES_ERROR:SS50005_GW_ERROR
"An error was encountered while converting an RMS record to Ingres format.\n
One of the fields in the RMS record could not be converted to a valid
Ingres column."

E_GW5483_RMS_FROM_INGRES_ERROR:SS50005_GW_ERROR
"An error was encountered while converting an Ingres record to RMS format.\n
One of the columns in the Ingres record could not be converted to a valid
RMS field."

E_GW5484_RMS_DATA_CVT_ERROR:SS50005_GW_ERROR
"A data conversion error occurred during processing of the RMS data."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error message indicates a recoverable data conversion error within
**      the RMS gateway exits. This error typically occurs when ADF reports
**      that it cannot properly perform the indicated conversion between RMS and
**      INGRES data formats. The conversion may have been from RMS to INGRES
**      or from INGRES to RMS; either type of conversion can produce this error.
**
**      The user does not see this error, nor is it logged to the error log. It
**      exists internally so that callers of the RMS exits can determine whether
**      an error which occurred was a data conversion error or an unexpected
**      internal error, and can modify their behavior accordingly.
**
**      Typically, this error should be remapped to a generic GWF error message
**      by the GWF layer.
**
** System Status:
**      The query is aborted, if possible. An ADF error message has already
**      been formatted and sent to the user at this point. This error code need
**      neither be formatted nor logged; it should just be returned back up to
**      the GWF layer.
**
** Recommendation:
**      None. The user should NEVER see this error; it is for internal program
**      use only. If this error does ever appear to the user, contact Ingres
**	Technical Support.
*/
E_GW5485_RMS_RECORD_TOO_SHORT:SS50005_GW_ERROR
"The table registration specified a field offset which was beyond the actual
length of the RMS record."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is typically caused by a mistake in table registration, such
**      that one or more fields has an offset past the end of the RMS record.
**
** System Status:
**      The query is aborted.
**
** Recommendation:
**      If the table has been incorrectly registered, the user should do a
**      "remove table", then re-register the table.  If the wrong file is
**      being accessed, this may be due to the wrong logical name being used,
**      or an incorrect logical name translation.  If the record is variable
**      length, all records must be long enough for the offsets specified.
*/

/*
        (allows the VMS editor's 'section' key to work usefully)
*/

/*
** Message numbers starting with E_GW54A0 are in gw02rms.c
*/

E_GW54A0_WRONG_NUM_PARMS:SS50005_GW_ERROR
"The wrong number of parameters (%0d) were provided for extended type '%1c',
which requires %2d parameters."

E_GW54A1_NO_SUCH_DATATYPE:SS50005_GW_ERROR
"Unknown datatype '%0c' was provided in an extended format specification."

E_GW54A2_INVALID_INTEGER:SS50005_GW_ERROR
"Invalid value provided for the offset, length, precision or scale parameter
for extended type '%0c'."

E_GW54A3_BAD_XFMT_PARAM:SS50005_GW_ERROR
"Out of range parameter ('%0d') provided for offset, length, precision or scale
for extended type '%1c'."

E_GW54A4_RMS_NO_DEFAULT:SS50005_GW_ERROR
"Ingres datatype '%0c' does not have a default conversion to an RMS datatype.
Use extended format syntax."

E_GW54A5_RMS_BAD_RECORD_LENGTH:SS50005_GW_ERROR
"A length ('%0d') + offset ('%1d') for extended type '%2c' exceeds the
maximum allowed record length of 32767."

E_GW54A6_BAD_FORMAT_STRING:SS50005_GW_ERROR
"An extended format string was not formatted correctly and cannot be
interpreted.  Check all extended format strings and make sure they have the
correct format, and the right number of parameters for the extended type
specified."

E_GW7000_INTEGER_OVF:SS22003_NUM_VAL_OUT_OF_RANGE
"Integer overflow occurred when attempting to convert value to a %0d byte\
 integer in the RMS gateway."

E_GW7001_FLOAT_OVF:SS22003_NUM_VAL_OUT_OF_RANGE
"Floating point overflow occurred when attempting to convert value to a %0d\
 byte float in the RMS gateway."

E_GW7002_FLOAT_UND:SS22003_NUM_VAL_OUT_OF_RANGE
"Floating point underflow occurred when attempting to convert value to a %0d\
 byte float in the RMS gateway."

E_GW7003_DATE_OUT_OF_RANGE:SS22008_DATETIME_FLD_OVFLOW
"The VAX date contains the year %0d which is out of the range of valid INGRES\
 dates."

E_GW7004_MONEY_OVF:SS22003_NUM_VAL_OUT_OF_RANGE
"Money overflow occurred in the RMS gateway."

E_GW7005_MONEY_OVF:SS22003_NUM_VAL_OUT_OF_RANGE
"Money underflow occurred in the RMS gateway."

E_GW7006_ING_TO_VAX_DATE:SS22008_DATETIME_FLD_OVFLOW
"Error occurred attempting to convert an INGRES date to a VAX date.  The INGRES\
 date value was not a valid VAX date (check range of valid VAX dates in VMS\
 documentation)."

E_GW7010_BAD_FLOAT_VAL:SS22003_NUM_VAL_OUT_OF_RANGE
"The VMS system routine OTS$CNVOUT_x returned an error when given an RMS\
 G_FLOAT or H_FLOAT value; be sure offsets are correctly specified in\
 REGISTERed table."

E_GW7011_BAD_VAXDATE_VAL:SS22008_DATETIME_FLD_OVFLOW
"A VAX date value was passed to a VMS system routine (SYS$ASCTIM or SYS$NUMTIM)\
 which in turn reported an error.  This indicates that the VAX date was\
 invalid. Be sure your offsets were correctly specified in the REGISTER\
 command."

E_GW7012_BAD_STR_TO_INT:SS50005_GW_ERROR
"Cannot convert '%0c' to integer in the RMS gateway."

E_GW7013_BAD_NUM_TO_INT:SS50005_GW_ERROR
"Error occurred attempting to convert a numeric string value (such as LEADING\
 NUMERIC, UNSIGNED NUMERIC, ZONED NUMERIC, or OVERPUNCHED numeric) to an\
 integer value in the RMS gateway.  Be sure your offsets were correctly\
 specified in the REGISTER command."

E_GW7014_BAD_STR_TO_VDATE:SS50005_GW_ERROR
"Error occurred attempting to convert '%0c' to a VAX date using the VMS system\
 routine LIB$CONVERT_DATE_STRING in the RMS gateway.  Make sure your date\
 format attributes are correctly set for the RMS data you are attempting to\
 convert, and make sure your offsets are correctly specified in the REGISTER\
 command."

E_GW7015_DECIMAL_OVF:SS22003_NUM_VAL_OUT_OF_RANGE
"Packed decimal overflow occurred when attempting to convert value to a\
 precision %0d scale cms %1d packed decimal value in the RMS gateway."

E_GW7016_BAD_STR_TO_DEC:SS50005_GW_ERROR
"Error occurred when attempting to convert '%0c' to a packed decimal value in\
 the RMS gateway."

E_GW7017_BAD_NUM_TO_STR:SS22003_NUM_VAL_OUT_OF_RANGE
"Error in %0c numeric string '%1c': invalid byte '%2c'."

E_GW7020_BAD_STR_TO_FLT:SS50005_GW_ERROR
"Error occurred when attempting to convert '%0c' to a floating point value in\
 the RMS gateway."

E_GW7021_BAD_NUM_TO_FLT:SS50005_GW_ERROR
"Error occurred attempting to convert a numeric string value (such as LEADING\
 NUMERIC, UNSIGNED NUMERIC, ZONED NUMERIC, or OVERPUNCHED numeric) to a\
 floating point value in the RMS gateway.  Be sure your offsets were correctly\
 specified in the REGISTER command."

E_GW7022_BAD_INT_TO_DATE:SS50005_GW_ERROR
"Error occurred attempting to convert the integer %0d to an INGRES date value\
 in the RMS gateway.  Check the current setting of II_DATE_FORMAT to make sure\
 it supports the correct ordering of days, months, and years which is intended;\
 also be sure your offsets were correctly specified in the REGISTER command."

E_GW7023_BAD_STR_TO_DATE:SS50005_GW_ERROR
"Error occurred attempting to convert '%0c' to an INGRES date value in the RMS\
 gateway. Check the current setting of II_DATE_FORMAT to make sure it supports\
 the correct date formats you are intending to use; also be sure your offsets\
 were correctly specified in the REGISTER command."

E_GW7024_BAD_STR_TO_MNY:SS50005_GW_ERROR
"Error occurred attempting to convert '%0c' to an INGRES money value in the RMS\
 gateway.  Check the current setting of the II_DECIMAL and II_MONEY_FORMAT\
 logicals to ensure you have the proper money formats you are intending to use;\
 also be sure your offsets were correctly specified in the REGISTER command."

E_GW7025_BAD_NUM_TO_MNY:SS50005_GW_ERROR
"Error occurred attempting to convert a numeric string value (such as LEADING\
 NUMERIC, UNSIGNED NUMERIC, ZONED NUMERIC, or OVERPUNCHED numeric) to a money\
 value in the RMS gateway.  Be sure your offsets were correctly specified in\
 the REGISTER command."

E_GW7026_BAD_FLT_TO_MNY:SS50005_GW_ERROR
"Internal error.  ADF returned an error when attempting to convert a float\
 value to a money value in the RMS gateway; this should never happen."

E_GW7027_MNY_STR_TOO_LONG:SS50005_GW_ERROR
"Attempt was made convert a string longer than 2000 characters into a money\
 value in the RMS gateway; this is not allowed."

E_GW7028_DATE_IS_INTERVAL:SS50005_GW_ERROR
"Attempt was made to convert an INGRES date interval into a VAX date in the RMS\
 gateway.  Only absolute dates may be converted to VAX dates."

E_GW7029_YEAR_NOT_1900:SS50005_GW_ERROR
"Attempt was made to convert an INGRES date with year %0d to an integer in the\
 RMS gateway.  Only dates in the 20th century (i.e. with year between 1900 and\
 1999 inclusive) may be converted to integer."

E_GW702A_FLT_STR_TOO_LONG:SS50005_GW_ERROR
"Attempt was made convert a string longer than 2000 characters into a float\
 value in the RMS gateway; this is not allowed."

E_GW702B_CANT_DO_G_OR_H:SS50005_GW_ERROR
"Could not convert a Vax G-float or H-float value to an Ingres float.  The\
 absolute value of the value to be converted was outside the range\
 1E+37 to 1E-37."

E_GW7050_BAD_ERLOOKUP:SS50005_GW_ERROR
"This is an internal error or an error in system configuration.  The datatype\
 conversion module of the RMS gateway was unable to look up an error message."

E_GW7100_NOCVT_AT_STARTUP:SS50005_GW_ERROR
"This is an internal error.  The datatype conversion module of the RMS gateway\
 was unable to cache all required static coercions at startup."

E_GW7999_INTERNAL_ERROR:SS50005_GW_ERROR
"The datatype conversion module of the RMS gateway incurred an internal error."

/*
** GWM errors are the 8000 block, loosely subdivided as follows:
**
**	gwmerr.c	8001-8040
**	gwmgcm.c	8041-8080
**	gwmgcn.c	8081-80C0
**	gwmima.c	80C1-8100
**	gwmkey.c	8141-8180
**	gwmmob.c	8181-81C0
**	gwmop.c		81C1-8200
**	gwmplace.c	8200-8240
**	gwmpmib.c	8241-8280
**	gwmrow.c	8281-82C0
**	gwmscb.c	82C1-8300
**	gwmutil.c	8301-8340
**	gwmxtab.c	8341-8380
**	gwmxutil.c	8381-93C0
*/

/*	gwmerr.c	8001-8040	*/

/*	gwmgcm.c	8041-8080	*/

E_GW8041_GCA_FORMAT:SS50000_MISC_ERRORS
"GWM error-- Error from GCA format"

E_GW8042_GCA_CALL:SS50000_MISC_ERRORS
"GWM error-- Error making gcm send to '%0c'."

E_GW8043_CS_SUSPEND:SS50000_MISC_ERRORS
"GWM error-- CSsuspend problem"

E_GW8044_NO_PEER:SS50000_MISC_ERRORS
"GWM error-- Got 'no peer' to '%0c' (class '%1c' flags x%2x)"

E_GW8045_GCA_COMPLETION:SS50000_MISC_ERRORS
"GWM error-- GCM completion error to '%0c' (class '%1c' flags x%2x)"

E_GW8046_BAD_OP:SS50000_MISC_ERRORS
"GWM error-- Unexpected OPBLK op %0d"

/*	gwmgcn.c	8081-80C0	*/

E_GW8081_GCN_ERROR:SS50000_MISC_ERRORS
"GWM error-- Error querying name server '%0c'"

E_GW8082_GCN_UNPACK:SS50000_MISC_ERRORS
"GWM error-- Mismatched instances '%0c' '%1c'"

/*	gwmima.c	80C1-8100	*/

E_GW80C1_BAD_FROM_CLAUSE:SS50000_MISC_ERRORS
"IMA Register error-- Invalid FROM clause\nvalue '%0c' for table %1c.\n
Valid choices are 'objects' or 'tables'"

E_GW80C2_IDX_BAD_FROM_CLAUSE:SS50000_MISC_ERRORS
"IMA Register index error-- Invalid FROM clause\nvalue '%0c' for table
'%1c'.\nValid choices are 'objects' or 'tables'"

E_GW80C3_INGRES_NOT_DBA:SS50000_MISC_ERRORS
"IMA: Tables must be in a database owned by ingres.\n
The current database '%0c' is owned by '%1c'."

E_GW80C4_NO_SUB_GATEWAY:SS50000_MISC_ERRORS
"IMA: Can't find gateway for table '%0c'\nwith saved flags x%1x"

E_GW80C5_INGRES_MUST_REGISTER:SS50000_MISC_ERRORS
"IMA: Only ingres may register IMA tables.\nYou are '%0c'."

E_GW80C6_ERRMAP_ERR_BUT_OK:SS50000_MISC_ERRORS
"GWM error-- Bad status %0d to GM_errmap, but OK cl_status."

E_GW80C7_SHOULD_HAVE_SEEN:SS50000_MISC_ERRORS
"GWM: You should already have seen the following error:"

E_GW80C8_ERRMAP_BAD_STATUS:SS50000_MISC_ERRORS
"GWM error-- E_DB_OK but bad status:"

/*	gwmkey.c	8141-8180	*/

E_GW8141_NULL_DMT_ATTR:SS50000_MISC_ERRORS
"GWM error-- NULL dmt_attribute setting key attr %0d (tup attr %1d)"

E_GW8142_BAD_INT_SIZE:SS50000_MISC_ERRORS
"GWM error-- Bad integer size %0d for column '%0c'."

E_GW8143_BAD_TYPE:SS50000_MISC_ERRORS
"GWM error-- Bad type %0d for column '%0c'"

E_GW8144_BAD_KEYATTR:SS50000_MISC_ERRORS
"GWM error-- Bad keyattr %0d for column '%0c'"

E_GW8145_BAD_ATTR_COUNT:SS50000_MISC_ERRORS
"GWM error-- Bad key attr count %0d, expecting %1d"

E_GW8146_GM_POS_ERROR:SS50000_MISC_ERRORS
"GWM error-- Table position failure."

/*	gwmmob.c	8181-81C0	*/

E_GW8181_OBJ_MISSING_IS_CLAUSE:SS50000_MISC_ERRORS
"IMA: Column '%0c' is missing the required IS clause.\n
Proper values are 'SERVER', 'VNODE', 'CLASSID', 'INSTANCE', 'VALUE',
or 'PERMISSIONS'"

E_GW8182_OBJ_BAD_IS_CLAUSE:SS50000_MISC_ERRORS
"IMA: Column '%0c' has an invalid IS clause\nvalue '%1c'.  Proper
values  are 'SERVER', 'VNODE', 'CLASSID', 'INSTANCE', 'VALUE', or
'PERMISSIONS'"

E_GW8183_OBJ_BAD_COL_TYPE:SS50000_MISC_ERRORS
"GWM error-- Column '%0c' has bad type '%1c'"

E_GW8184_BAD_XREL_BUF:SS50000_MISC_ERRORS
"GWM error-- Wrong width for iigw07_relation.\n
Got %0d, wanted %1d.  Maybe the SQL declaration is wrong?"

E_GW8185_BAD_NUMATTS:SS50000_MISC_ERRORS
"GWM error-- Caller gave wrong number of attributes.\n
Wanted %0d, got %1d." 

E_GW8186_BAD_ATTR_TYPE:SS50000_MISC_ERRORS
"GWM error-- Bad attr type %0d for column %1d (%2c)"

E_GW8187_BAD_XATT_BUF:SS50000_MISC_ERRORS
"GWM error-- Wrong width for iigw07_attribute.  Got %0d,
wanted %1d.  Maybe the SQL declaration is wrong?"

E_GW8188_NOT_PLACE:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The first key column, '%1c' is not 'VNODE' or 'SERVER'."

E_GW8189_NOT_CLASSID:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The first non-place key column, '%1c' is not 'CLASSID'."

E_GW818A_NOT_INSTANCE:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The last key column, '%1c' is not 'INSTANCE'." 

E_GW818B_NOT_UNIQUE:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The keys given are unique, but the structure is not." 

E_GW818C_IS_UNIQUE:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The keys given are not unique, but the structure given is unique." 

E_GW818D_NOT_PLACE_CLASSID:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The first key column, '%1c', isn't 'VNODE', 'SERVER', or 'CLASSID'." 

E_GW818E_NO_KEYS:SS50000_MISC_ERRORS
"IMA: No keys given for a structured table."

E_GW818F_TOO_MANY_KEYS:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly\n
with too many keys.\nFROM 'objects' tables may have two or three
key columns."

/*	gwmop.c		81C1-8200	*/

E_GW81C1_ROP_ERROR:SS50000_MISC_ERRORS
"GWM error-- Unexpected error in GM_rop:"

E_GW81C2_CVAL_ERR:SS50000_MISC_ERRORS
"GWM error-- Converting string '%0c'\n
to decimal from object '%1c' for column '%2c'" 

E_GW81C3_UNEXPECTED_ATTR_TYPE:SS50000_MISC_ERRORS
"GWM error-- Unsupported attribute type %0c\n
for column '%1c'"

/*	gwmplace.c	8201-8240	*/

E_GW8201_LOST_PLACE:SS50000_MISC_ERRORS
"GWM error-- No place '%0c' found in place cache."

E_GW8202_QUERY_VNODE_ERR:SS50000_MISC_ERRORS
"GWM error-- Querying name server got bad status:"

E_GW8203_VNODE_IN_SERVER:SS50000_MISC_ERRORS
"GWM error-- Looking for VNODE in SERVER domain!?"

E_GW8204_SELF_NOT_INSTALL:SS50000_MISC_ERRORS
"GWM error-- This process '%0c' isn't a vnode server!?"

E_GW8205_UNEXPECTED_DOM_TYPE:SS50000_MISC_ERRORS
"GWM error-- Unexpected domain type %0d in GM_place"

/*	gwmpmib.c	8241-8280	*/

/*	gwmrow.c	8281-82C0	*/

/*	gwmscb.c	82C1-8300	*/

E_GW82C1_NO_SCB:SS50000_MISC_ERRORS
"GWM error-- Didn't get domain for session"

E_GW82C2_NO_DOMAIN_START:SS50000_MISC_ERRORS
"GWM error-- Error starting domain for session x%0x, scb x%1x"

/*	gwmutil.c	8301-8340	*/

E_GW8301_NO_LISTEN_ADDR:SS50000_MISC_ERRORS
"GWM error-- Can't find my GCA listen address"

E_GW8302_ALLOC_ERROR:SS50000_MISC_ERRORS
"GWM error-- Couldn't allocate %0d bytes of memory"

E_GW8303_DBA_ERROR:SS50000_MISC_ERRORS
"GWM error-- Couldn't get DBA for the database."

E_GW8304_USER_ERROR:SS50000_MISC_ERRORS
"GWM error-- Couldn't get session user name"

E_GW8305_SESS_ALLOC_ERROR:SS50000_MISC_ERRORS
"GWM error-- Couldn't get %0d bytes out of session memory"

E_GW8306_USER_PRIV_ERR:SS50000_MISC_ERRORS
"GWM error-- Getting user privileges:"

E_GW8307_DBNAME_ERR:SS50000_MISC_ERRORS
"GWM error-- Getting database name:"

/*	gwmxtab.c	8341-8380	*/

E_GW8341_TAB_MISSING_IS_CLAUSE:SS50000_MISC_ERRORS
"IMA: Column '%0c' is missing the required IS clause.\n
Proper values are 'SERVER', 'VNODE' or a classid." 
				   
E_GW8342_TAB_BAD_IS_CLAUSE:SS50000_MISC_ERRORS
"IMA: Column '%0c' has an invalid IS clause value '%1c'.\n
Proper values are 'SERVER', 'VNODE' or a classid." 

/*
E_GW8343_TAB_BAD_COL_TYPE:SS50000_MISC_ERRORS
"GWM error-- Column '%0c' has bad type '%1c'"
*/

E_GW8344_GX_GET_PART_ERR:SS50000_MISC_ERRORS
"GWM error-- Partial row GET error:"

E_GW8345_GX_GETNEXT_ERR:SS50000_MISC_ERRORS
"GWM error-- Got db_error but OK status"

E_GW8346_NEXT_PLACE_ERR:SS50000_MISC_ERRORS
"GWM error-- Getting next place got unexpected status:"

E_GW8347_GX_GET_OK_STAT_ERR:SS50000_MISC_ERRORS
"GWM error-- Got db_error but OK status"

E_GW8348_NOT_USER:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The least significant key column, '%1c', is not a user-specified classid." 

E_GW8349_NOT_PLACE_USER:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly.\n
The most significant key column, '%1c', isn't 'VNODE', 'SERVER' or a
user-specified classid."

E_GW834A_TAB_TOO_MANY_KEYS:SS50000_MISC_ERRORS
"IMA: Table '%0c' was registered incorrectly\n
with too many keys.\nFROM 'tables' registrations may have one or two
key columns."

E_GW834B_INVALID_XTAB_IS_CLAUSE:SS50000_MISC_ERRORS
"IMA:  Column '%0c' has an invalid IS clause value '%1c'.\nImports
from 'tables' IS clause values must be 'VNODE', 'SERVER', or a
user-specified classid."

E_GW834C_XTAB_NO_USER_ATT:SS50000_MISC_ERRORS
"IMA:  Registered Table must have at least one user-defined classid.\n"

/*	gwmxutil.c	8381-83C0	*/

E_GW8381_INCONSISTENT_ROW:SS50000_MISC_ERRORS
"GWM: A 'tables' row being constructed had objects with inconsistent
instance values.  This can either be caused by a dirty read (objects
added or deleted during the query) or by registering objects as
columns with non-matching instance index objects."

NO_SQLSTATE_MAPPING
