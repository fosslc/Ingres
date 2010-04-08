SQLSTATE_MAPPING_REQUIRED

/*
**    Message file for Facility code: _TP
**
**    History:
**
**	16-feb-1993 (dianeh)
**	    Added NO_SQLSTATE_MAPPING to end of file.
**	03-dec-1993 (dianeh)
**	    Refine text destined for messages.txt file.
**      24-Sep-1999 (hanch04)
**          Created new set of TPF_MASK errors to help format error messages.
**	04-Oct-2001 (jenjo02)
**	    Added source file and line number to TP0020, changed
**	    incorrect "%s" to "%c" in lots of messages.
*/


#define _TP_CLASS 14

E_TP0001_INVALID_REQUEST:SS5000B_INTERNAL_ERROR
"Invalid TPF request code."
E_TP0002_SCF_MALLOC_FAILED:SS5000B_INTERNAL_ERROR
"SCF_MALLOC failed."
E_TP0003_SCF_MFREE_FAILED:SS5000B_INTERNAL_ERROR
"SCF_MFREE failed."
E_TP0004_MULTI_SITE_WRITE:SS40002_CONSTR_VIOLATION
"Multi-site update attempted."
E_TP0005_UNKNOWN_STATE:SS25000_INV_XACT_STATE
"Unknown TPF transaction state."
E_TP0006_INVALID_EVENT:SS25000_INV_XACT_STATE
"Unknown TPF event."
E_TP0007_INVALID_TRANSITION:SS25000_INV_XACT_STATE
"Invalid TPF event for the current state."
E_TP0008_TXN_BEGIN_FAILED:SS08007_XACT_RES_UNKNOWN
"Unable to perform BEGIN TRANSACTION request."
E_TP0009_TXN_FAILED:SS08007_XACT_RES_UNKNOWN
"Transaction failed."
E_TP0010_SAVEPOINT_FAILED:SS08007_XACT_RES_UNKNOWN
"Unable to complete savepoint request."
E_TP0011_SP_ABORT_FAILED:SS08007_XACT_RES_UNKNOWN
"Savepoint abort failed."
E_TP0012_SP_NOT_EXIST:SS08007_XACT_RES_UNKNOWN
"Unknown savepoint name."
E_TP0013_AUTO_ON_NO_SP:SS08007_XACT_RES_UNKNOWN
"Autocommit with savepoint is not permitted."
E_TP0014_ULM_STARTUP_FAILED:SS5000B_INTERNAL_ERROR
"ULM session startup failed."
E_TP0015_ULM_OPEN_FAILED:SS5000B_INTERNAL_ERROR
"ULM memory stream open failed."
E_TP0016_ULM_ALLOC_FAILED:SS5000B_INTERNAL_ERROR
"ULM memory stream allocation failed."
E_TP0017_ULM_CLOSE_FAILED:SS5000B_INTERNAL_ERROR
"ULM memory stream close failed."
E_TP0020_INTERNAL:SS5000B_INTERNAL_ERROR
"An internal TPF error has been detected at %0c:%1d"
/*%
** Parameters:
**	%0c	Name of the source file in which the error was 
**		detected.
**	%1d	Line number within that file.
**
** System Status:
**	This is an internal error.  TPF will act accordingly.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will indicate corrective actions, if any.
*/
E_TP0021_LOGDX_FAILED:SS5000B_INTERNAL_ERROR
"TPF failed to log the distributed transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**	TPF will abort the distributed transaction.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will indicate corrective actions, if any.
**
**	(A possible contributing problem: the CDB becomes inaccessible.)
*/
E_TP0022_SECURE_FAILED:SS08007_XACT_RES_UNKNOWN
"TPF failed to secure the distributed transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**	TPF will abort the distributed transaction.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will indicate corrective actions, if any.
**
**	(A possible contributing problem: An LDB becomes inaccessible.)
*/
E_TP0023_COMMIT_FAILED:SS08007_XACT_RES_UNKNOWN
"TPF failed to commit the distributed transaction."
/*%
** Parameter:
**	None.
**
** System Status:
**	TPF will abort the distributed transaction.
**
** Recommendation:
**	This is an informational message logged to the error log file; other
**	error messages will indicate corrective actions, if any.
**
**	(A possible contributing problem: An LDB refused to commit or becomes
**	inaccessible.)
*/
I_TP0200_BEGIN_ROUND_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY Round %0d recovery begins."
I_TP0201_END_ROUND_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY Round %0d recovery ends successfully."
E_TP0202_FAIL_FATAL_REC:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY Round %0d recovery ends with FATAL ERROR %1x"
E_TP0203_FAIL_REC:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY Round %0d recovery ends with ERROR %1x."
I_TP0204_BEGIN_DDB_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Recovery for DDB %0c begins."
E_TP0205_DDB_FAIL_REC:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Recovery of DDB %0c failed with ERROR %1x"
I_TP0206_DDB_OK_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Recovery of DDB %0c ends successfully."
I_TP0207_BEGIN_CDB_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building CDB list for recovery begins."
E_TP0208_FAIL_READ_CDBS:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : star_cdbs in iidbdb."
E_TP0209_FAIL_ALLOC_CDBS:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Cannot allocate CDB space."
I_TP020A_END_CDB_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building CDB list for recovery ends successfully."
E_TP020B_FAIL_CDB_REC:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Building CDB list for recovery failed."
E_TP020C_ORPHAN_DX_FOUND:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : %0d orphan DX(s) found in DDB %1c" 
I_TP020D_NO_NEEDED_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : No recovery is necessary."
I_TP020E_BEGIN_DX_LIST:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building the DX list for DDB %0c begins."
I_TP020F_DX_LIST_EMPTY:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building DX list for DDB %0c ends successfully with empty list."
I_TP0210_END_DX_LIST:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building DX list for DDB %0c ends successfully."
E_TP0211_FAIL_END_DX_LIST:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Building DX list for DDB %0c failed with ERROR %1x."
E_TP0212_FAIL_READ_DX:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Reading iidd_ddb_dxldbs for DX %0x %1x failed with ERROR %2x."
I_TP0213_CDB_NODE_NAME:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY :  CDB node %0c, CDB name %1c"
E_TP0214_FAIL_END_LX_LIST:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Building LX list for LX %0x %1x failed."
I_TP0215_BEGIN_LX_LIST:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building the LX list for DX %0x %1x begins."
I_TP0216_RESTART_LDBS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Restarting the LDBs for DX %0x %1x begins."
E_TP0217_FAIL_RESTART_LDBS:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Restarting the LDBs for DX %0x %1x failed."
I_TP0218_END_RESTART_LDBS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Restarting the LDBs for DX %0x %1x ends successfully."
I_TP0219_END_RESTART_ALL_LDBS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : All LDBs for DX %0x %1x have restarted."
E_TP021A_LDB_TERM:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : %0d LDB(s) of DX %1x %2x have terminated and dropped out."
E_TP021B_DX_SECURE:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : DX %0x %1x was left in SECURE state." 
E_TP021C_FAIL_DX_ABORT:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Logging new ABORT state for DX %0x %1x failed with ERROR %2x"
I_TP021D_END_DX_ABORT:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Logging new ABORT state for DX %0x %1x ends successfully."
E_TP021E_FAIL_DX_COMMIT:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Logging new COMMIT state for DX %0x %1x failed with ERROR %2x"
I_TP021F_END_DX_COMMIT:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Logging new COMMIT state for DX %0x %1x ends successfully."
E_TP0220_DX_STATE_COMMIT:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : DX %x %x was left in COMMIT state."
E_TP0221_DX_STATE_ABORT:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : DX %x %x was left in ABORT state."
E_TP0222_DX_STATE_UNKNOWN:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : DX %0x %1x was left in an UNKNOWN state %2d"
I_TP0223_DX_STATE_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Recovering DX %0x %1x from %2c state begins."
E_TP0224_FAIL_DEL_DX:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Deleting DX %0x %1x from iidd_ddb_dxlog failed with ERROR %2x"
I_TP0225_END_DX_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Recovering DX %0x %1x ends successfully."
E_TP0226_FAIL_DX_TERM:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Recovering DX %0x %1x failed to terminate all LXs."
I_TP0227_COMMIT_UPDATE:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Committing updater"
E_TP0228_ABORT_UPDATE:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Aborting updater"
I_TP0229_DX_OK:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : of DX %0x %1x succeeds."
E_TP0230_DX_FAIL:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : of DX %0x %1x failed with ERROR %2x."
I_TP0231_SKIP_PHASE1:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Phase 1 skipped."
I_TP0232_BEGIN_PHASE1:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Phase 1 begins."
I_TP0233_NO_REC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : No recovery necessary for any DDB."
E_TP0234_FAIL_REP_CDB:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Replicating CDB search list failed with ERROR %0x"
I_TP0235_END_PHASE1:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Phase 1 ends successfully."
I_TP0236_COMPLETE_PHASE1:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Phase 1 completes."
E_TP0237_SKIP_PHASE2:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Phase 2 skipped."
I_TP0238_BEGIN_PHASE2:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Phase 2 begins."
I_TP0239_END_PHASE2:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Phase 2 ends successfully."
I_TP023A_BEGIN_LDBS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Resecuring the (updating) LDBs of DX %0x %1x begins."
E_TP023B_REFUSE_LDBS:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Resecuring the (updating) LDBs of DX %0x %1x ends being refused."
I_TP023C_NODE_LDBS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Node %0c, LDB %1c, DBMS %2c"
E_TP023D_FAIL_LDBS:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Resecuring the (updating) LDBs of DX %0x %1x failed with ERROR %1x"
I_TP023E_END_LDBS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Resecuring the (updating) LDBs of DX %0x %1x ends successfully."
I_TP0240_COMPLETE_PHASE2:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Phase 2 completes."
E_TP0241_FAIL_ACCESS_CDB:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Cannot access CDB of DDB:"
E_TP0242_FAIL_CATALOG_DDB:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : 2PC catalogs NOT found for DDB:"
I_TP0243_BEGIN_SEARCH_DDB:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building search DDB (CDB) list for recovery begins."
I_TP0244_END_SEARCH_DDB:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Building search DDB (CDB) list for recovery ends successfully."
E_TP0245_FAIL_SEARCH_DDB:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : Building search DDB (CDB) list for recovery failed with ERROR %0x"
E_TP0246_REC_RQF_ERROR:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : ERROR %0x occurred while calling"
I_TP0247_REC_QUERY_INFO:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Query information"
I_TP0248_REC_TARGET_LDB:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : The target LDB query text"
I_TP0249_REC_QUERY_TEXT:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : Query text"
E_TP024A_REC_NULL:SS5000B_INTERNAL_ERROR
"2PC/RECOVERY : NULL pointer detected!"
I_TP024B_DDB_NAME:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 1) DDB name %0c"
I_TP024C_DDB_OWNER:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 2) DDB owner %0c"
I_TP024D_CDB_NAME:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 3) CDB name %0c"
I_TP024E_CDB_NODE:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 4) CDB node %0c"
I_TP024F_CDB_DBMS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 5) CDB DBMS %0c"
I_TP0250_NODE:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 1) NODE %0c"
I_TP0251_LDB:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 2) LDB %0c"
I_TP0252_DBMS:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 3) DBMS %0c"
I_TP0253_LDB_ID:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 4) LDB id %0d"
I_TP0254_CDB_ASSOC:SS5000F_TEXTUAL_INFO
"2PC/RECOVERY : 5) Using a special CDB association "
NO_SQLSTATE_MAPPING
