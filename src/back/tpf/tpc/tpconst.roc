/*
** Copyright (c) 1990, 2008 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <tpf.h>

/**
**
**  Name: TPFCONS.ROC - Global readonly variables 
**
**  Description:
**      This file contains all the global readonly variables for TPF.
**
**	IITP_00_tpf_p		- ptr to string "TPF"
**	IITP_01_secure_p	- ptr to string "SECURE"
**	IITP_02_commit_p	- ptr to string "COMMIT"
**	IITP_03_abort_p		- ptr to string "ABORT"
**	IITP_04_unknown_p	- ptr to string "UNKNOWN"
**	IITP_05_reader_p	- ptr to string "reader"
**	IITP_06_updater_p	- ptr to string "updater"
**	IITP_07_traceing_p	- ptr to string "Tracing"
**	IITP_08_erred_calling_p	- ptr to string "occurred while calling"
**	IITP_09_3dots_p		- ptr to string "..."
**	IITP_10_savepoint_p	- ptr to string "savepoint"
**	IITP_11_abort_to_p	- ptr to string "abort to"
**	IITP_12_rollback_to_p	- ptr to string "rollback to"
**	IITP_13_allowed_in_dx_p	- ptr to string "allowed in DX"
**	IITP_14_aboring_p	- ptr to string "Aborting"
**	IITP_15_committing_p	- ptr to string "Committing"
**	IITP_17_failed_with_err_p	
**				- ptr to string "failed with ERROR"
**	IITP_18_fails_p		- ptr to string "fails"
**	IITP_19_succeeds_p	- ptr to string "succeeds"
**	IITP_20_2pc_recovery_p	- ptr to string "2PC/RECOVERY"
**	IITP_21_2pc_cats_p	- ptr to string "2PC catalogs"
**	IITP_22_aborted_ok_p	- ptr to string "aborted successfully"
**	IITP_23_committed_ok_p	- ptr to string "committed successfully"
**	IITP_24_ends_ok_p	- ptr to string "ends successfully"
**	IITP_25_78_dashes_p	- ptr to string of 78 dashes
**	IITP_26_completes_p	- ptr to string "completes"
**	IITP_27_begins_p	- ptr to string "begins"
**	IITP_28_unrecognized_p	- ptr to string "UNRECOGNIZED"
**	IITP_29_state_p		- ptr to string "state"
**	IITP_30_error_p		- ptr to string "ERROR"
**	IITP_31_phase_1_p	- ptr to string "Phase 1"
**	IITP_32_phase_2_p	- ptr to string "Phase 2"
**	IITP_33_unknown_rqfid_p	- ptr to string "an UNRECOGNIZED RQF operation"
**	IITP_34_unknown_tpfid_p	- ptr to string "an UNRECOGNIZED TPF operation"
**	IITP_35_qry_info_p	- ptr to string "query information"
**	IITP_36_qry_text_p	- ptr to string "query text"
**	IITP_37_target_ldb_p	- ptr to string "the target LDB"
**	IITP_38_null_ptr_p	- ptr to string "NULL pointer detected!"
**	IITP_39_ddb_name_p	- ptr to string "DDB name"
**	IITP_40_ddb_owner_p	- ptr to string "DDB owner"
**	IITP_41_cdb_name_p	- ptr to string "CDB name"
**	IITP_42_cdb_node_p	- ptr to string "CDB node"
**	IITP_43_cdb_dbms_p	- ptr to string "CDB DBMS"
**	IITP_44_node_p		- ptr to string "NODE"
**	IITP_45_ldb_p		- ptr to string "LDB"
**	IITP_46_dbms_p		- ptr to string "DBMS"
**	IITP_47_cdb_assoc_p	- ptr to string 
**				    "using a special CDB association"
**	IITP_48_reg_assoc_p	- ptr to string 
**				    "using a regular association"
**	IITP_50_rqf_call_tab[]  - table containing RQF operation names
**	IITP_51_tpf_call_tab[]  - table containing TPF operation names
**	IITP_52_dx_state_tab[]  - table containing DX state names
**
**  History:    $Log-for RCS$
**      25-aug-90 (carl)
**          created
**      24-oct-90 (carl)
**	    added more string constants
**	15-sep-93 (swm)
**	    Added cs.h include above other headers that use its definition
**	    of CS_SID in declarations.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPTPFLIBDATA
**	    in the Jamfile.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
**/

/*
**
LIBRARY = IMPTPFLIBDATA
**
*/


GLOBALDEF   char		*IITP_00_tpf_p = "TPF";

GLOBALDEF   char		*IITP_01_secure_p = "SECURE";

GLOBALDEF   char		*IITP_02_commit_p = "COMMIT";

GLOBALDEF   char		*IITP_03_abort_p = "ABORT";

GLOBALDEF   char		*IITP_04_unknown_p = "UNKNOWN";

GLOBALDEF   char		*IITP_05_reader_p = "reader";

GLOBALDEF   char		*IITP_06_updater_p = "updater";

GLOBALDEF   char		*IITP_07_tracing_p = "Tracing";

GLOBALDEF   char		*IITP_08_erred_calling_p = 
				    "occurred while calling";
GLOBALDEF   char		*IITP_09_3dots_p = "...";

GLOBALDEF   char		*IITP_10_savepoint_p = "savepoint";

GLOBALDEF   char		*IITP_11_abort_to_p = "abort to";

GLOBALDEF   char		*IITP_12_rollback_to_p = "rollback to";

GLOBALDEF   char		*IITP_13_allowed_in_dx_p = "allowed in DX";

GLOBALDEF   char		*IITP_14_aborting_p = "Aborting";

GLOBALDEF   char		*IITP_15_committing_p = "Committing";

GLOBALDEF   char		*IITP_16_securing_p = "Securing";

GLOBALDEF   char		*IITP_17_failed_with_err_p = 
				    "failed with ERROR";

GLOBALDEF   char		*IITP_18_fails_p = "fails";

GLOBALDEF   char		*IITP_19_succeeds_p = "succeeds";

GLOBALDEF   char		*IITP_20_2pc_recovery_p = "2PC/RECOVERY";

GLOBALDEF   char		*IITP_21_2pc_cats_p = "2PC catalogs";

GLOBALDEF   char		*IITP_22_aborted_ok_p = "aborted successfully";

GLOBALDEF   char		*IITP_23_committed_ok_p = 
				    "committed successfully";

GLOBALDEF   char		*IITP_24_ends_ok_p = 
				    "ends successfully";

GLOBALDEF   char		*IITP_25_78_dashes_p = 
"------------------------------------------------------------------------------"
;

GLOBALDEF   char		*IITP_26_completes_p = "completes";

GLOBALDEF   char		*IITP_27_begins_p = "begins";

GLOBALDEF   char		*IITP_28_unrecognized_p = "UNRECOGNIZED";

GLOBALDEF   char		*IITP_29_state_p = "state";

GLOBALDEF   char		*IITP_30_error_p = "ERROR";

GLOBALDEF   char		*IITP_31_phase_1_p = "Phase 1";

GLOBALDEF   char		*IITP_32_phase_2_p = "Phase 2";

GLOBALDEF   char		*IITP_33_unknown_rqfid_p = 
				    "an UNRECOGNIZED RQF operation";

GLOBALDEF   char		*IITP_34_unknown_tpfid_p = 
				    "an UNRECOGNIZED TPF operation";

GLOBALDEF   char		*IITP_35_qry_info_p = "query information";

GLOBALDEF   char		*IITP_36_qry_text_p = "query text";

GLOBALDEF   char		*IITP_37_target_ldb_p = "the target LDB";

GLOBALDEF   char		*IITP_38_null_ptr_p = 
				    "NULL pointer detected!";

GLOBALDEF   char		*IITP_39_ddb_name_p = "DDB name";

GLOBALDEF   char		*IITP_40_ddb_owner_p = "DDB owner";

GLOBALDEF   char		*IITP_41_cdb_name_p = "CDB name";

GLOBALDEF   char		*IITP_42_cdb_node_p = "CDB node";

GLOBALDEF   char		*IITP_43_cdb_dbms_p = "CDB DBMS";

GLOBALDEF   char		*IITP_44_node_p = "NODE";

GLOBALDEF   char		*IITP_45_ldb_p = "LDB";

GLOBALDEF   char		*IITP_46_dbms_p = "DBMS";

GLOBALDEF   char		*IITP_47_cdb_assoc_p = 
				    "using a special CDB association";

GLOBALDEF   char		*IITP_48_reg_assoc_p = 
				    "using a regular association";


/*}
** Name: IITP_50_rqf_call_tab - global table containing the RQF operation names
**
** Description:
**      This global table is used for outputting RQF operation names.
**
** History:
**      25-aug-90 (carl)
**          created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/

GLOBALDEF const  char        *IITP_50_rqf_call_tab[] =
{
    "",                             /* 0 */
    "RQR_STARTUP",                  /* 1 */
    "RQR_SHUTDOWN",                 /* 2 */
    "RQR_S_BEGIN",                  /* 3 */
    "RQR_S_END",                    /* 4 */
    "",                             /* 5 */
    "RQR_READ",                     /* 6 */
    "RQR_WRITE",                    /* 7 */
    "RQR_INTERRUPT",                /* 8 */
    "RQR_NORMAL",                   /* 9 */
    "RQR_XFR",                      /* 10 */
    "RQR_BEGIN",                    /* 11 */
    "RQR_COMMIT",                   /* 12 */
    "RQR_SECURE",                   /* 13 */
    "RQR_ABORT",                    /* 14 */
    "RQR_DEFINE",                   /* 15 */
    "RQR_EXEC",                     /* 16 */
    "RQR_FETCH",                    /* 17 */
    "RQR_DELETE",                   /* 18 */
    "RQR_CLOSE",                    /* 19 */
    "RQR_T_FETCH",                  /* 20 */
    "RQR_T_FLUSH",                  /* 21 */
    "",                             /* 22 */
    "RQR_CONNECT",                  /* 23 */
    "RQR_DISCONNECT",               /* 24 */
    "RQR_CONTINUE",                 /* 25 */
    "RQR_QUERY",                    /* 26 */
    "RQR_GET",                      /* 27 */
    "RQR_ENDRET",                   /* 28 */
    "RQR_DATA_VALUES",              /* 29 */
    "RQR_SET_FUNC",                 /* 30 */
    "RQR_UPDATE",                   /* 31 */
    "RQR_OPEN",                     /* 32 */
    "RQR_MODIFY",                   /* 33 */
    "RQR_CLEANUP",                  /* 34 */
    "RQR_TERM_ASSOC",               /* 35 */
    "RQR_LDB_ARCH",                 /* 36 */
    "RQR_RESTART",                  /* 37 */
    "RQR_CLUSTER_INFO"              /* 38 */
};



/*}
** Name: IITP_51_tpf_call_tab - global table containing the TPF operation names
**
** Description:
**      This global table is used for outputting TPF operation names.
**
** History:
**      25-aug-90 (carl)
**          created
*/

GLOBALDEF const  char        *IITP_51_tpf_call_tab[] =
{
    "",                             /* 0 */
    "TPF_STARTUP",                  /* 1 */
    "TPF_SHUTDOWN",                 /* 2 */
    "TPF_INITIALIZE",               /* 3 */
    "TPF_TERMINATE",                /* 4 */
    "TPF_SET_DDBINFO",              /* 5 */
    "TPF_RECOVER",                  /* 6 */
    "TPF_COMMIT",                   /* 7 */
    "TPF_ABORT",                    /* 8 */
    "TPF_S_DISCNNCT",               /* 9  (obsolete) */
    "TPF_C_DISCNNCT",               /* 10 (obsolete) */
    "TPF_S_DONE",                   /* 11 (obsolete) */
    "TPF_S_REFUSED",                /* 12 (obsolete) */
    "TPF_READ_DML",                 /* 13 */
    "TPF_WRITE_DML",                /* 14 */
    "TPF_BEGIN_DX",                 /* 15 */
    "TPF_END_DX",                   /* 16 (obsolete) */
    "TPF_SAVEPOINT",                /* 17 */
    "TPF_SP_ABORT",                 /* 18 */
    "TPF_AUTO",                     /* 19 (obsolete) */
    "TPF_NO_AUTO",                  /* 20 (obsolete) */
    "TPF_EOQ",                      /* 21 (obsolete) */
    "TPF_1T_DDL_CC",                /* 22 */
    "TPF_0F_DDL_CC",                /* 23 */
    "TPF_OK_W_LDBS",                /* 24 */
    "TPF_P1_RECOVER",               /* 25 */
    "TPF_P2_RECOVER",               /* 26 */
    "TPF_IS_DDB_OPEN",              /* 27 */
    "TPF_TRACE"                     /* 28 */
};


/*}
** Name: IITP_52_dx_state_tab - global table containing the DX state names
**
** Description:
**      This global table is used for outputting DX state names.
**
** History:
**      25-aug-90 (carl)
**          created
*/

GLOBALDEF const  char        *IITP_52_dx_state_tab[] =
{
    "INITIAL",			    /* 0 */
    "BEGIN",			    /* 1 */
    "SECURE",			    /* 2 */
    "ABORT",			    /* 3 */
    "COMMIT",			    /* 4 */
    "CLOSED"			    /* 5 */
};
