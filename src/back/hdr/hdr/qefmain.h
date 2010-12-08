/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/**
** Name:QEFMAIN.H - General QEF data structures
**
** Description:
**      This file contains general data structures necessary for the
**  execution of QEF routines.
**
** History:
**      2-apr-86 (daved)
**          written
**      29-jul-86 (jennifer)
**          Updated to add query mod routines.
**	30-nov-87 (puree)
**	    added definitions for CS_SEMAPHORE.
**	04-dec-87 (puree)
**	    added E_QE0099_DB_INCONSISTENT.
**	02-feb-88 (puree)
**	    added E_QE009B_LOCATION_LIST ERROR
**	23-feb-88 (puree)
**	    added E_QE0100_NO_ROWS_QUALIFIED.
**	27-apr-88 (puree)
**	    added opcode for QEU_CREDBP and QEU_DRPDBP.
**	02-aug-88 (puree)
**	    added E_QE0116_CANT_ALLOCATE_DBP_ID.
**	17-aug-88 (puree)
**	    added E_QE0117_NONEXISTENT_DBP
**	20-aug-88 (carl)
**	    added Titan changes: request ids, error codes and new field
**	    qef_s1_distrib in QEF_SRCB
**	15-jan-89 (paul)
**	    added support for rules processing in the form of
**	    QEF_MAX_STACK, QEF_MAX_ACT_DEPTH, E_QE0119_LOAD_QP,
**	    E_QE0120_RULE_ACTION_LIST.
**	15-feb-89 (paul)
**	    Corrected error made by the previous integrator.
**	    Code inserted between a comment and the code it described.
**	23-feb-89 (carl)
**	    added E_QE1995_BAD_AGGREGATE_ACTION
**	04-mar-89 (carl)
**	    added QED_1SCF_MODIFY
**	21-mar-89 (carl)
**	    added QED_2SCF_TERM_ASSOC
**	10-Mar-89 (ac)
**	    Added 2PC support.
**	14-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Added new QEU operation codes QEU_GROUP and QEU_APLID
**	    Added new internal QEF error codes for groups/applids
**	16-mar-89 (neil)
**	    Added QEU_CRULE and QEU_DRULE operation codes.
**	28-mar-89 (carl)
**	    added QED_RLINK
**	30-mar-89 (paul)
**	    Added error codes for resource limits exceeded and maximum
**	    procedure call nesting level exceeded.
**	31-mar-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added new QEU operation code, QEU_DBPRIV;
**	    Added error codes for obtaining dbprivs.
**	09-apr-89 (carl)
**	    added QED_DDL_CONCURRENCY
**	25-apr-89 (carl)
**	    renamed QED_DDL_CONCURRENCY to QED_CONCURRENCY
**	    added QED_7RDF_DIFF_ARCH
**      18-apr-89 (teg)
**          added error codes for internal procedures (E_QE0130 - E_QE0135)
**	06-may-89 (ralph)
**	    added more error codes (E_QE0210-22F)
**	10-may-89 (neil)
**	    Added trace point QEF_T_RULES for rule firing tracing.
**	31-may-89 (neil)
**	    Added error QE0123-124.
**	06-jun-89 (carl)
**	    added E_QE0547_UNAUTHORIZED_USER	
**	22-jun-89 (carl)
**	    added QED_DESCPTR for setting the DDB descriptor pointer in QEF's
**	    session control block (QEF_CB)
**      28-jun-89 (jennifer)
**          Added opcodes for user, location dbaccess and security
**          routines.
**      14-jul-89 (jennifer)
**          Added a flag mask field to QEF_SRCB to allow operation 
**          to be altered.  In this case allowed server to be intialized
**          to C2 secure.
**      20-jul-89 (jennifer)
**          Added two new structures used for DAC security checking.
**          One (QEF_ART)contains a list of table.owner or view.owner names
**          for each table along with the query mode.  This information 
**          will be used to generate audit records when the query 
**          is successfully executed.  Could not use the information 
**          currently in the QP since views are already expanded.
**          The other (QEF_APR) contains a list of permits of type
**          alarm which must be checked each time the query is executed.
**	07-aug-89 (neil)
**	    Added error QE0105
**	14-nov-89 (carl)
**	    added QED_RECOVER and new error codes for 2PC
**	07-sep-89 (neil)
**	    Alerters: Added new errors, QEU_CEVENT & QEU_DEVENT op codes,
**	    and trace flags.
**	26-sep-89 (neil)
**	    Rules: Added QEF_T_NORULES and corresponding error.
**	29-sep-89 (ralph)
**	    Added new QEU operation codes QEU_ALOC and QEU_DLOC for ALTER
**	    and DROP LOCATION.
**	    Added messages E_QE0230-233.
**	30-oct-89 (ralph)
**	    Added messages E_QE0234-235
**	09-nov-89 (sandyh)
**	    Added E_QE0230 for >255 dml ops/xact.
**	29-dec-89 (neil)
**	    Added error QE0199, QE0200.
**	28-jan-90 (carl)
**	    added QED_RECOVER_P1 and QED_RECOVER_P2 to replace QED_RECOVER
**	09-feb-90 (neil)
**	    Alerters/auditing: added new info messages QE0230 - QE0232.
**	12-feb-90 (carol)
**	    Added request id QEU_CCOMMENT for comment on statement.
**	19-apr-90 (andre)
**	    Added request id QEU_CSYNONYM and QEU_DSYNONYM for CREATE/DROP
**	    SYNONYM.
**	14-jun-90 (andre)
**	    defined QEU_CTIMESTAMP.
**	14-jul-90 (carl)
**	    added QED_3SCR_CLUSTER_INFO
**	08-aug-90 (ralph)
**	    Changed qef_apr.qef_popset from i2 to i4.
**	    Added qef_apr.qef_popctl.
**	30-sep-90 (carl)
**	    removed the obsolete QED_EXCHANGE define
**	14-nov-90 (ralph)
**	    Added E_QE0237_DROP_DBA
**	10-dec-90 (neil)
**	    Alerters: Added new errors, QEU_CEVENT & QEU_DEVENT op codes,
**	    and trace flags.
**	31-dec-90 (ralph)
**	    Added E_QE0229_DBPRIV_NOT_GRANTED, E_QE0233_ALLDBS_NOT_AUTH
**	25-jan-91 (teresa)
**	    Added E_QE0136 and E_QE0137
**	04-feb-91 (neil)
**	    Alerters: added new error QE019A
**	03-mar-91 (ralph)
**	    Added E_QE0300_USER_MSG
**	24-may-91 (andre)
**	    defined QEC_RESET_EFF_USER_ID
**      26-jun-91 (stevet)
**          Added E_QE0238_NO_MSTRAN
**	15-aug-91 (markg)
**	    Added E_QE11B_PARAM_MISSING, this message replaces 
**	    E_QE114_PARAM_MISSING which has now become obselete. Bug 38765.
**	12-feb-92 (nancy)
**	    Added E_QE0301_TBL_SIZE_CHANGE for bug fix 38565.  This is a 
**	    status (not an error) indicating that qep needs recompilation due
**	    to table size change.  It is returned from qeq_validate() and 
**	    checked in scs_sequencer().
**	19-feb-92 (bryanp)
**	    Enhanced the TEMP_TABLE_MACRO so that it can tell the difference
**	    between internal temporary tables (owned by $ingres) and
**	    user-defined temporary tables (owned by specific sessions).
**	26-jun-92 (andre)
**	    define QEU_DBP_STATUS for GRANT/REVOKE project
**      26-jun-92 (anitap)
**          Added QEQ_UPD_ROWCNT for backward compatibility in the value of
**          rowcount for update.
**	02-aug-92 (andre)
**	    defined QEU_REVOKE
**	24-sep-92 (robf)
**	    Added E_QE0400_GATEWAY_ERROR and E_QE0401_NO_GATEWAY
**	    to handle errors coming back from GWF/DMF that otherwise
**	    cascaded to a generic SCF "internal error" message.
**	02-nov-92 (jrb)
**	    Multi-location sorts: added QE0240-245.
**	23-oct-1992 (bryanp)
**	    Added E_QE019B_DMM_CREATE_DB_ERROR for qeaiproc.c
**      03-nov-92 (anitap)
**          Added E_QE0302_SCHEMA_EXISTS to handle errors when an user has
**          specified CREATE SCHEMA AUTHORIZATION twice. Added
**          E_QE0303_PARSE_EXECIMM_QTEXT to handle the case when the 
**	    QEA_EXEC_IMM action will need to be processed by QEF. 
**	    QEF will call back SCF with this message.
**	29-dec-92 (robf)
**	    Added E_QE019C_SEC_WRITE_ERROR to handle security audit write 
**	    errors.
**	4-jan-92 (rickh)
**	    Added E_QE0165_NONEXISTENT_RULE.
**	05-feb-93 (andre)
**	    undefined QEC_RESET_EFF_USER_ID; redefined QEU_REVOKE to fill space
**	    left vacant by QEC_RESET_EFF_USER_ID
**	03-feb-93 (fpang)
**	    Added E_QE0991_LDB_ALTERED_XACT which qef returns when an execute
**	    procedure or execute immediate on a local tried to commit/abort.
**	    QEF will abort the current 2PC transaction.
**	20-feb-93 (andre)
**	    defined QEU_DSCHEMA for DROP SCHEMA processing
**	01-mar-93 (andre)
**	    undefined QEU_CVIEW since we will be building a query plan to create
**	    a view and it will be created as a part of processing a 
**	    QEA_CREATE_VIEW action
**	3-MAR-93 (RICKH)
**	    Added E_QE0304_SCHEMA_DOES_NOT_EXIST.
**	14-mar-93 (andre)
**	    added E_QE0166_NO_CONSTRAINT_IN_SCHEMA and
**	    E_QE0167_DROP_CONS_WRONG_TABLE
**	15-mar-93 (rickh)
**	    Added E_QE0305_UDT_DEFAULT_TOO_BIG.
**	16-mar-93 (andre)
**	    added E_QE0168_ABANDONED_CONSTRAINT and
**	    E_QE0169_UNKNOWN_OBJTYPE_IN_DROP
**	15-apr-93 (jhahn)
**	    Added E_QE0258_ILLEGAL_XACT_STMT
**	08-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Revised TEMP_TABLE_MACRO to use qef_cat_owner rather than "$ingres"
**	07-jun-93 (andre)
**	    defined new tracepoint values"
**		QEF_T_CHECK_OPT_ROW_LVL_RULE and
**		QEF_T_CHECK_OPT_TIDOJ_SET_DBP
**	24-jun-93 (robf)
**	    Added E_QE0280_MAC_ERROR, QEF_F_B1SECURE,
**	7-jul-93 (rickh)
**	    Prototyped qef_call.
**	9-jul-93 (rickh)
**	    Fixed casting in the MEcmp call inside TEMP_TABLE_MACRO.
**	22-jul-93 (rickh)
**	    Added a new trace point to dump execute immediate text generated
**	    by CONSTRAINTS.
**      26-july-1993 (jhahn)
**          Added E_QE0306_ALLOCATE_BYREF_TUPLE.
**	28-jul-93 (rblumer)
**	    added E_QE016A_NOT_DROPPABLE_CONS error.
**	3-aug-93 (rickh)
**	    Added E_QE0138_CANT_CREATE_UNIQUE_CONSTRAINT.
**	9-aug-93 (rickh)
**	    Added E_QE0139_CANT_ADD_REF_CONSTRAINT and
**	    E_QE0140_CANT_EXLOCK_REF_TABLE.
**	26-aug-93 (robf)
**	    Added QEU_CPROFILE/APROFILE/DPROFILE
**	07-sep-93 (andre)
**	    defined E_QE0141_V_UBTID_GETINFO_ERR,
**	    E_QE0142_V_UBTID_UNFIX_ERR, and E_QE0143_V_UBTID_GETDESC_ERR
**	9-sep-93 (rickh)
**	    Added E_QE0144_CANT_ADD_CHECK_CONSTRAINT and
**	    E_QE0145_CANT_EXLOCK_CHECK_TABLE.
**	22-oct-93 (andre)
**	    added E_QE011C_NONEXISTENT_SYNONYM
**	26-nov-93 (robf)
** 	    Add QEU_RAISE_DBEVENT, QEU_CSECALM, QEU_DSECALM
**	6-dec-93 (robf)
**	   Added  E_QE028C_DROP_ALARM_ERR,
**                E_QE028B_CREATE_ALARM_ERR
**	28-jan-94 (andre)
**	    removed QEF_T_CHECK_OPT_ROW_LVL_RULE and defined 
**	    QEF_T_CHECK_OPT_STMT_LVL_RULE
**	20-jan-94 (rickh)
**	    Added E_QE0307_INIT_MO_ERROR.
**	27-jan-94 (rickh)
**	    Added E_QE0308_COL_SIZE_MISCOMPILED.
**	04-mar-94 (andre)
**	    defined E_QE0146_QSO_JUST_TRANS_ERROR, and 
**	    E_QE0147_QSO_DESTROY_ERROR
**	4-mar-94 (robf)
**          Add QEU_ROLEGRANT
**      17-may-95 (dilma04)
**          Added I_QE2034_READONLY_TABLE_ERR
**      20-aug-95 (ramra01)
**          New function type QE_ORIGAGG
**      21-aug-95 (tchdi01)
**          Added
**              E_QE7610_SPROD_UPD_IIDB
**              E_QE7612_PROD_MODE_ERR
**          for the production mode projec
**      06-mar-96 (nanpr01)
**          Added new field qef_maxtup in QEF_SRCB for incresed tuple size
**	    project.
**      19-apr-96 (stial01)
**          Added E_QE0309_NO_BMCACHE_BUFFERS 
**      22-jul-96 (ramra01 for bryanp)
**          Added E_QE009D_ALTER_TABLE_SUPP
**          Added E_QE016B_COLUMN_HAS_OBJECTS
**      24-jul-96 (inkdo01)
**          Added E_QE030A_TTAB_PARM_ERROR
**	14-nov-1996 (nanpr01)
**	    Added error code E_QE009E_CANT_MOD_TEMP_TABLE.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Added E_QE005B_TRAN_ACCESS_CONFLICT.
**	4-mar-97 (inkdo01)
**	    Added E_QE030B_RULE_PROC_MISMATCH
**	3-Apr-98 (thaju02)
**	    Added E_QE011D_PERIPH_CONVERSION.
**	1-apr-98 (inkdo01)
**	    Added E_QE013A_CANT_CREATE_CONSTRAINT.
**	1-apr-1998 (hanch04)
**	    Added qef_qescb_offset
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	3-feb-99 (inkdo01)
**	    Added 30C, 30D, 30E for row-producing procedures.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	01-oct-2001 (devjo01)
**	    Renamed QEN_HASH_DUP to QEF_HASH_DUP and moved it from qefdsh.h 
**	    to qefmain.h so that it will be visible to OPF. 
**	05-Mar-2002 (jenjo02)
**	    For sequence generator support, added QEU_CSEQ,
**	    QEU_ASEQ, QEU_DSEQ.
**	18-mar-02 (inkdo01)
**	    Added E_QE00A0_SEQ_LIMIT_EXCEEDED for sequences.
**      22-oct-2002 (stial01)
**          Added QET_XA_ABORT
**	14-feb-03 (inkdo01)
**	    Added QE00A1_IISEQ_NOTDEFINED for sequences.
**	5-mar-03 (inkdo01)
**	    Added QE00A2_CURRVAL_NONEXTVAL for sequences.
**      29-Apr-2003 (hanal04) Bug 109682 INGSRV 2155, Bug 109564 INGSRV 2237
**          Added I_QE2037_CONSTRAINT_TAB_IDX_ERR.
**	1-Dec-2003 (schka24)
**	    Add total sort-hash memory parameter holder.
**	2-jan-04 (inkdo01)
**	    Added QE00A5_END_OF_PARTITION to signal no more rows in 
**	    partition grouping.
**	12-Jan-2004 (schka24)
**	    revise qef-call prototype to void *.
**	09-Apr-2004 (jenjo02)
**	    Added E_QE007F_ERROR_UNFIXING_PAGES.
**      12-Apr-2004 (stial01)
**          Define length as SIZE_TYPE.
**      7-oct-2004 (thaju02)
**          Replace i4 with SIZE_TYPE for memory pool > 2Gig.
**	11-Nov-2004 (gorvi01)
**	    Added new error message entry of E_QE028E for unextenddb.
**	10-may-2005 (raodi01)
**	    Added error message E_QE00A3_MOD_TEMP_TABLE_KEY 
**	19-Oct-2005 (schka24)
**	    Remove unused alter-timestamp function.
**      22-mar-2006 (toumi01)
**	    Add I_QE2038_CONSTRAINT_TAB_IDX_ERR for MODIFY with error
**	    message advice for NODEPENDENCY_CHECK. Unlike DROP, not
**	    all MODIFY commands are destructive.
**	25-may-06 (dougi)
**	    Add QEF_T_DUMPTABS server tracepoint to trigger dump of table
**	    names for any QEF error.
**	26-Jun-2006 (jonj)
**	    Add QE005C, QET_XA_? functions.
**	26-july-06 (dougi)
**	    Add QEF_T_PROCERR to enable the QE00C0 message for errors in
**	    procedure execution.
**	2-feb-2007 (dougi)
**	    Add E_QE00A6_NO_DATA for scrollable cursors.
**	9-apr-2007 (dougi)
**	    Add E_QE00A7_ROW_DELETED for keyset scrollable cursors.
**	18-dec-2007 (dougi)
**	    Add E_QE00A8_BAD_FIRST_OFFSETN for first/offset "n".
**	30-jan-2008 (dougi)
**	    Add E_QE00A9_BAD_OFFSETN and fix previous.
**      09-Sep-2008 (gefei01)
**          Add E_QE030F_LOAD_TPROC_QP and E_QE0310_INVALID_TPROC_ACT
**      21-Jan-2009 (horda03) Bug 121519
**          Add E_QE00AA_ERROR_CREATING_TABLE.
**	Feb-24-2010 (troal01)
**	    Add E_QE5423_SRID_MISMATCH
**	Mar-15-2010 (troal01)
**	    Added E_QE5424_INVALID_SRID and E_QE5425_SRS_NONEXISTENT errors
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	18-Apr-2010 (gupsh01) Sir 123444
**	    Added E_QE016C_RENAME_TAB_HAS_OBJS.
**	21-apr-2010 (toumi01) SIR 122403
**	    Add messages for column encryption.
**	04-Aug-2010 (miket) SIR 122403
**	    Change encryption activation terminology from
**	    enabled/disabled to unlock/locked.
**	25-aug-2010 (miket) SIR 122403 SD 145781
**	    Better msg for alter table not valid for encrypted tables.
**      06-sep-2010 (maspa05) SIR 124363
**          Added I_QE3000_LONGRUNNING_QUERY
**      07-sep-2010 (maspa05) SIR 124363
**          Added I_QE3001_LONGRUNNING_QUERY - similar message to the above
**          but with an extra parameter
**/

/*
**  Forward and/or External typedef/struct references.
*/

typedef struct _QEF_FUNC QEF_FUNC;

/*
**  Defines of other constants.
*/
/*
**      General constants
*/
#define QEF_MAX_FUNC    32      /* number of functions in the QEF_FUNC table */
#define QEF_VERSION     1       /* version of QEF */
#define QEF_SNB         256     /* number of server trace flags */
#define QEF_SNVP        50      /* number of server flags with values */
#define QEF_SNVAO       50      /* number of values at once */
#define QEF_SSNB        128     /* number of session trace flags */
#define QEF_MEM         512000  /* number of bytes in QEF mem pool */
#define QEF_EXCLUSIVE	1	/* exclusive flag for CSp_semaphore calls */
#define	QEF_STACK_MAX	20	/* Maximum procedure nesting depth. Also    */
				/* doubles as maximum rule nesting depth    */
				/* since rule nesting is determined by	    */
				/* stack depth. */
#define	QEF_ACT_DEPTH_MAX   2	/* Maximum nesting depth of action lists in */
				/* a QP. */
#if 0
/* remove #if if you do not want to use ADF */
#define QEF_NO_ADF		/* some code has been dummied so that
				** ADF will not be used. This is not
				** consistent throughout the code.
				*/
#endif
/*
**
**	Trace flags
**
** Session trace flags: If < QEF_SSNB then apply to session only.
**
**  100-1nn node trace flags. If 100+n is set then node n will print a
**  row. Printing can be turned off for the first val1 rows by setting
**  the value when setting the trace flag.
*/
#define	QEF_T_RULES	2	/* Trace rule firing */
#define	QEF_T_EVENTS	3	/* User trace of RAISE EVENT - PRINTEVENTS */
#define	QEF_T_LGEVENTS	4	/* Log trace of RAISE EVENT - LOGEVENTS */
#define	QEF_T_NORULES	5	/* SET NORULES is on */
#define	QEF_T_RETURN	6	/* Display return value */

				/*
				** IF enforcing CHECK OPTION using a
				** statement-level rule, have the dbproc check
				** for CHECK OPTION violation by issuing a query
				** involving an outer join on TID attribute of
				** the view's underlying base table and the
				** sole attribute of the temp table
				** NOTE: if QEF_T_CHECK_OPT_TIDOJ_SET_DBP is 
				**       set without 
				**	 QEF_T_CHECK_OPT_STMT_LVL_RULE being 
				**	 set, the former will be disregarded and
				**	 row-level rule will be used to enforce
				**	 CHECK OPTION
				*/
#define	QEF_T_CHECK_OPT_TIDOJ_SET_DBP	8

				/*
				** If creating a CONSTRAINT, dump the
				** execute immediate text to the terminal.
				*/
#define QEF_T_DUMP_EXEC_IMMEDIATE_TEXT	9

				/*
				** enforce CHECK OPTION using a statement
				** level rule with dbproc NOT using an outer
				** join on TID
				** NOTE: if QEF_T_CHECK_OPT_TIDOJ_SET_DBP is 
				**       set without 
				**	 QEF_T_CHECK_OPT_STMT_LVL_RULE being 
				**	 set, the former will be disregarded and
				**	 row-level rule will be used to enforce
				**	 CHECK OPTION
				*/
#define QEF_T_CHECK_OPT_STMT_LVL_RULE	10

#define QEF_T_DUMPTABS	130	/* 
				** server flag that dumps names of open tables
				** whenever QEF error is encountered
				*/
#define	QEF_T_PROCERR	131	/*
				** server flag that enables display of error
				** E_QE00C0 whenever an error occurs during
				** procedure execution. It names the proc and
				** roughly identifies the failing statement
				*/

/*
**      Error messages
*/
#define E_QEF_MASK  ((i4) (DB_QEF_ID << 16))

#define E_QE0000_OK                     (0x0000L)
#define E_QE0001_INVALID_ALTER          (E_QEF_MASK + 0x0001L)
#define E_QE0002_INTERNAL_ERROR         (E_QEF_MASK + 0x0002L)
#define E_QE0003_INVALID_SPECIFICATION  (E_QEF_MASK + 0x0003L)
#define E_QE0004_NO_TRANSACTION         (E_QEF_MASK + 0x0004L)
#define E_QE0005_CANNOT_ESCALATE	(E_QEF_MASK + 0x0005L)
#define E_QE0006_TRANSACTION_EXISTS     (E_QEF_MASK + 0x0006L)
#define E_QE0007_NO_CURSOR              (E_QEF_MASK + 0x0007L)
#define E_QE0008_CURSOR_NOT_OPENED      (E_QEF_MASK + 0x0008L)
#define E_QE0009_NO_PERMISSION          (E_QEF_MASK + 0x0009L)
#define E_QE000A_READ_ONLY              (E_QEF_MASK + 0x000AL)
#define E_QE000B_ADF_INTERNAL_FAILURE	(E_QEF_MASK + 0x000BL)
#define E_QE000C_CURSOR_ALREADY_OPENED  (E_QEF_MASK + 0x000CL)
#define E_QE000D_NO_MEMORY_LEFT         (E_QEF_MASK + 0x000DL)
#define E_QE000E_ACTIVE_COUNT_EXCEEDED  (E_QEF_MASK + 0x000EL)
#define E_QE000F_OUT_OF_OTHER_RESOURCES (E_QEF_MASK + 0x000FL)
#define E_QE0010_DUPLICATE_ROW          (E_QEF_MASK + 0x0010L)
#define E_QE0011_AMBIGUOUS_REPLACE      (E_QEF_MASK + 0x0011L)
#define E_QE0012_DUPLICATE_KEY_INSERT   (E_QEF_MASK + 0x0012L)
#define E_QE0013_INTEGRITY_FAILED       (E_QEF_MASK + 0x0013L)
#define E_QE0014_NO_QUERY_PLAN          (E_QEF_MASK + 0x0014L)
#define E_QE0015_NO_MORE_ROWS           (E_QEF_MASK + 0x0015L)
#define E_QE0016_INVALID_SAVEPOINT      (E_QEF_MASK + 0x0016L)
#define E_QE0017_BAD_CB                 (E_QEF_MASK + 0x0017L) 
#define E_QE0018_BAD_PARAM_IN_CB        (E_QEF_MASK + 0x0018L)
#define E_QE0019_NON_INTERNAL_FAILURE   (E_QEF_MASK + 0x0019L)
#define E_QE001A_DATATYPE_OVERFLOW      (E_QEF_MASK + 0x001AL)
#define E_QE001B_COPY_IN_PROGRESS       (E_QEF_MASK + 0x001BL)
#define E_QE001C_NO_COPY                (E_QEF_MASK + 0x001CL)
#define E_QE001D_FAIL                   (E_QEF_MASK + 0x001DL)
#define E_QE001E_NO_MEM                 (E_QEF_MASK + 0x001EL)
#define E_QE001F_INVALID_REQUEST        (E_QEF_MASK + 0x001FL)
#define E_QE0020_OPEN_CURSOR            (E_QEF_MASK + 0x0020L)
#define E_QE0021_NO_ROW                 (E_QEF_MASK + 0x0021L)
#define E_QE0022_QUERY_ABORTED          (E_QEF_MASK + 0x0022L)
#define E_QE0023_INVALID_QUERY          (E_QEF_MASK + 0x0023L)
#define E_QE0024_TRANSACTION_ABORTED    (E_QEF_MASK + 0x0024L)
#define E_QE0025_USER_ERROR		(E_QEF_MASK + 0x0025L)
#define E_QE0026_DEFERRED_EXISTS	(E_QEF_MASK + 0x0026L)
#define E_QE0027_BAD_QUERY_ID           (E_QEF_MASK + 0x0027L)
#define E_QE0028_BAD_TABLE_NAME		(E_QEF_MASK + 0x0028L)
#define E_QE0029_BAD_OWNER_NAME		(E_QEF_MASK + 0x0029L)
#define E_QE002A_DEADLOCK		(E_QEF_MASK + 0x002AL)
#define E_QE002B_TABLE_EXISTS		(E_QEF_MASK + 0x002BL)
#define	E_QE002C_BAD_ATTR_TYPE		(E_QEF_MASK + 0x002CL)
#define E_QE002D_BAD_ATTR_NAME		(E_QEF_MASK + 0x002DL)
#define	E_QE002E_BAD_ATTR_SIZE		(E_QEF_MASK + 0x002EL)
#define E_QE002F_BAD_ATTR_PRECISION	(E_QEF_MASK + 0x002FL)
#define E_QE0030_BAD_LOCATION_NAME	(E_QEF_MASK + 0x0030L)
#define	E_QE0031_NONEXISTENT_TABLE	(E_QEF_MASK + 0x0031L)
#define E_QE0032_BAD_SAVEPOINT_NAME	(E_QEF_MASK + 0x0032L)
#define E_QE0033_BAD_TID		(E_QEF_MASK + 0x0033L)
#define E_QE0034_LOCK_QUOTA_EXCEEDED	(E_QEF_MASK + 0x0034L)
#define E_QE0035_LOCK_RESOURCE_BUSY	(E_QEF_MASK + 0x0035L)
#define E_QE0036_LOCK_TIMER_EXPIRED	(E_QEF_MASK + 0x0036L)
#define E_QE0037_ERROR_DELETING_RECORD	(E_QEF_MASK + 0x0037L)
#define E_QE0038_DUPLICATE_RECORD	(E_QEF_MASK + 0x0038L)
#define E_QE0039_DUPLICATE_KEY		(E_QEF_MASK + 0x0039L)
#define E_QE003A_FOPEN_FROM		(E_QEF_MASK + 0x003AL)
#define E_QE003B_FOPEN_TO		(E_QEF_MASK + 0x003BL)
#define E_QE003C_COLUMN_TOO_SMALL	(E_QEF_MASK + 0x003CL)
#define E_QE003D_BAD_INPUT_STRING	(E_QEF_MASK + 0x003DL)
#define E_QE003E_UNEXPECTED_EOF		(E_QEF_MASK + 0x003EL)
#define	E_QE003F_UNTERMINATED_C0	(E_QEF_MASK + 0x003FL)
#define E_QE0040_NO_FULL_PATH		(E_QEF_MASK + 0x0040L)
#define E_QE0041_DUPLICATES_IGNORED	(E_QEF_MASK + 0x0041L)
#define	E_QE0042_CONTROL_TO_BLANK	(E_QEF_MASK + 0x0042L)
#define E_QE0043_TRUNC_C0		(E_QEF_MASK + 0x0043L)
#define	E_QE0044_ILLEGAL_FTYPE		(E_QEF_MASK + 0x0044L)
#define	E_QE0045_BINARY_F_T0		(E_QEF_MASK + 0x0045L)
#define	E_QE0046_ERROR_ON_ROW		(E_QEF_MASK + 0x0046L)
#define E_QE0047_INVALID_CHAR		(E_QEF_MASK + 0x0047L)
#define E_QE0048_TUP_TOO_WIDE		(E_QEF_MASK + 0x0048L)
#define E_QE0049_ALL_KEY		(E_QEF_MASK + 0x0049L)
#define E_QE004A_KEY_SEQ		(E_QEF_MASK + 0x004AL)
/* not all rows matched in select all */
#define E_QE004B_NOT_ALL_ROWS		(E_QEF_MASK + 0x004BL)
/* multiple rows returned in select */
#define E_QE004C_NOT_ZEROONE_ROWS	(E_QEF_MASK + 0x004CL)
/* duplicate key on update */
#define E_QE004D_DUPLICATE_KEY_UPDATE	(E_QEF_MASK + 0x004DL)
/* ulh initialize failure */
#define E_QE004E_ULH_INIT_FAILURE	(E_QEF_MASK + 0x004EL)
/* a subselect has returned a row that no inner rows qualified */
#define E_QE004F_UNQUAL_ROW		(E_QEF_MASK + 0x004FL)
/* tried to create a temp table that already exists */
#define E_QE0050_TEMP_TABLE_EXISTS	(E_QEF_MASK + 0x0050L)
/* table access conflict. table opened twice */
#define E_QE0051_TABLE_ACCESS_CONFLICT	(E_QEF_MASK + 0x0051L)
/* out of disk space, disk quota, or open file quota */
#define E_QE0052_RESOURCE_QUOTA_EXCEED	(E_QEF_MASK + 0x0052L)
#define E_QE0053_TRAN_ID_NOT_UNIQUE	(E_QEF_MASK + 0x0053L)
#define E_QE0054_DIS_TRAN_UNKNOWN	(E_QEF_MASK + 0x0054L)
#define E_QE0055_DIS_TRAN_OWNER		(E_QEF_MASK + 0x0055L)
#define E_QE0056_CANT_SECURE_IN_CLUSTER	(E_QEF_MASK + 0x0056L)
#define E_QE0057_ERROR_RESUME_TRAN	(E_QEF_MASK + 0x0057L)
#define E_QE0058_ERROR_SECURE_TRAN	(E_QEF_MASK + 0x0058L)
#define E_QE0059_ILLEGAL_STMT		(E_QEF_MASK + 0x0059L)
#define E_QE005A_DIS_DB_UNKNOWN		(E_QEF_MASK + 0x005AL)
#define E_QE005B_TRAN_ACCESS_CONFLICT	(E_QEF_MASK + 0x005BL)
#define E_QE005C_INVALID_TRAN_STATE	(E_QEF_MASK + 0x005CL)

#define	E_QE005F_BAD_DB_ACCESS_MODE	(E_QEF_MASK + 0x005FL)
#define E_QE0060_BAD_DB_ID		(E_QEF_MASK + 0x0060L)
#define	E_QE0061_BAD_DB_NAME		(E_QEF_MASK + 0x0061L)
#define	E_QE0062_BAD_INDEX		(E_QEF_MASK + 0x0062L)
#define	E_QE0063_BAD_TABLE_OWNER	(E_QEF_MASK + 0x0063L)
#define	E_QE0064_BAD_TRAN_ID		(E_QEF_MASK + 0x0064L)
#define	E_QE0065_DB_ACCESS_CONFLICT	(E_QEF_MASK + 0x0065L)
#define	E_QE0066_DB_OPEN		(E_QEF_MASK + 0x0066L)
#define	E_QE0067_DB_OPEN_QUOTA_EXCEEDED	(E_QEF_MASK + 0x0067L)
#define	E_QE0068_DB_QUOTA_EXCEEDED	(E_QEF_MASK + 0x0068L)
#define	E_QE0069_DELETED_TID		(E_QEF_MASK + 0x0069L)
#define	E_QE006A_NONEXISTENT_DB		(E_QEF_MASK + 0x006AL)
#define	E_QE006B_SESSION_QUOTA_EXCEEDED	(E_QEF_MASK + 0x006BL)
#define	E_QE006C_CANT_MOD_CORE_SYSCAT	(E_QEF_MASK + 0x006CL)
#define	E_QE006D_CANT_INDEX_CORE_SYSCAT	(E_QEF_MASK + 0x006DL)
#define	E_QE006E_TRAN_IN_PROGRESS	(E_QEF_MASK + 0x006EL)
#define	E_QE006F_TRAN_NOT_IN_PROGRESS	(E_QEF_MASK + 0x006FL)
#define	E_QE0070_TRAN_QUOTA_EXCEEDED	(E_QEF_MASK + 0x0070L)
#define	E_QE0071_TRAN_TABLE_OPEN	(E_QEF_MASK + 0x0071L)
#define	E_QE0072_LOCATIONS_TOO_MANY	(E_QEF_MASK + 0x0072L)
#define	E_QE0073_RECORD_ACCESS_CONFLICT	(E_QEF_MASK + 0x0073L)
#define	E_QE0074_BAD_TABLE_CREATE	(E_QEF_MASK + 0x0074L)
#define	E_QE0075_BAD_KEY_DIRECTION	(E_QEF_MASK + 0x0075L)
#define	E_QE0076_BTREE_BAD_KEY		(E_QEF_MASK + 0x0076L)
#define	E_QE0077_LOCATION_EXISTS	(E_QEF_MASK + 0x0077L)
#define	E_QE0078_ERROR_ADDING_DB	(E_QEF_MASK + 0x0078L)
#define	E_QE0079_ERROR_DELETING_DB	(E_QEF_MASK + 0x0079L)
#define	E_QE007A_ERROR_OPENING_DB	(E_QEF_MASK + 0x007AL)
#define	E_QE007B_ERROR_CLOSING_DB	(E_QEF_MASK + 0x007BL)
#define	E_QE007C_ERROR_GETTING_RECORD	(E_QEF_MASK + 0x007CL)
#define	E_QE007D_ERROR_PUTTING_RECORD	(E_QEF_MASK + 0x007DL)
#define	E_QE007E_ERROR_REPLACING_RECORD	(E_QEF_MASK + 0x007EL)
#define	E_QE007F_ERROR_UNFIXING_PAGES	(E_QEF_MASK + 0x007FL)
#define	E_QE0080_ERROR_POSITIONING	(E_QEF_MASK + 0x0080L)
#define	E_QE0081_ERROR_OPENING_TABLE	(E_QEF_MASK + 0x0081L)
#define	E_QE0082_ERROR_CLOSING_TABLE	(E_QEF_MASK + 0x0082L)
#define	E_QE0083_ERROR_MODIFYING_TABLE	(E_QEF_MASK + 0x0083L)
#define	E_QE0084_ERROR_INDEXING_TABLE	(E_QEF_MASK + 0x0084L)
#define	E_QE0085_ERROR_SORTING		(E_QEF_MASK + 0x0085L)
#define	E_QE0086_ERROR_BEGINNING_TRAN	(E_QEF_MASK + 0x0086L)
#define	E_QE0087_ERROR_COMMITING_TRAN	(E_QEF_MASK + 0x0087L)
#define	E_QE0088_ERROR_ABORTING_TRAN	(E_QEF_MASK + 0x0088L)
#define	E_QE0089_ERROR_SAVEPOINT	(E_QEF_MASK + 0x0089L)
#define	E_QE008A_BAD_TABLE_DESTROY	(E_QEF_MASK + 0x008AL)
#define	E_QE008B_CANT_OPEN_VIEW		(E_QEF_MASK + 0x008BL)
#define	E_QE008C_LOCK_MANAGER_ERROR	(E_QEF_MASK + 0x008CL)
#define	E_QE008D_BAD_SYSCAT_MOD		(E_QEF_MASK + 0x008DL)
#define	E_QE008E_ERROR_RELOCATING_TABLE	(E_QEF_MASK + 0x008EL)
#define	E_QE008F_ERROR_DUMPING_DATA	(E_QEF_MASK + 0x008FL)
#define	E_QE0090_ERROR_LOADING_DATA	(E_QEF_MASK + 0x0090L)
#define	E_QE0091_ERROR_ALTERING_TABLE	(E_QEF_MASK + 0x0091L)
#define	E_QE0092_ERROR_SHOWING_TABLE	(E_QEF_MASK + 0x0092L)
#define	E_QE0093_ERROR_ALTERING_DB	(E_QEF_MASK + 0x0093L)
#define	E_QE0094_ISAM_BAD_KEY		(E_QEF_MASK + 0x0094L)
#define	E_QE0095_COMP_BAD_KEY		(E_QEF_MASK + 0x0095L)
#define	E_QE0096_MOD_IDX_UNIQUE		(E_QEF_MASK + 0x0096L)
#define E_QE0097_NOT_A_VIEW		(E_QEF_MASK + 0x0097L)
#define E_QE0098_SEM_FAILS		(E_QEF_MASK + 0x0098L)
#define E_QE0099_DB_INCONSISTENT	(E_QEF_MASK + 0x0099L)
#define E_QE009A_DUP_LOCATION_NAME	(E_QEF_MASK + 0x009AL)
#define E_QE009B_LOCATION_LIST_ERROR	(E_QEF_MASK + 0x009BL)
#define E_QE009C_UNKNOWN_ERROR		(E_QEF_MASK + 0x009CL)
#define E_QE009D_ALTER_TABLE_SUPP	(E_QEF_MASK + 0x009DL)
#define E_QE009E_CANT_MOD_TEMP_TABLE	(E_QEF_MASK + 0x009EL)
#define E_QE00A0_SEQ_LIMIT_EXCEEDED	(E_QEF_MASK + 0x00A0L)
#define E_QE00A1_IISEQ_NOTDEFINED	(E_QEF_MASK + 0x00A1L)
#define E_QE00A2_CURRVAL_NONEXTVAL	(E_QEF_MASK + 0x00A2L)
#define E_QE00A3_MOD_TEMP_TABLE_KEY     (E_QEF_MASK + 0x00A3L)
#define	E_QE00A5_END_OF_PARTITION	(E_QEF_MASK + 0x00A5L)
#define	E_QE00A6_NO_DATA		(E_QEF_MASK + 0x00A6L)
#define	E_QE00A7_ROW_DELETED		(E_QEF_MASK + 0x00A7L)
#define	E_QE00A8_BAD_FIRSTN		(E_QEF_MASK + 0x00A8L)
#define	E_QE00A9_BAD_OFFSETN		(E_QEF_MASK + 0x00A9L)
#define E_QE00AA_ERROR_CREATING_TABLE   (E_QEF_MASK + 0x00AAL)
#define	E_QE00C0_PROC_STMTNO		(E_QEF_MASK + 0X00C0L)
#define E_QE0100_CREATE_DSH_OBJECT	(E_QEF_MASK + 0x0100L)
#define E_QE0101_DESTROY_DSH_OBJECT	(E_QEF_MASK + 0x0101L)
#define E_QE0102_CATALOG_DSH_OBJECT	(E_QEF_MASK + 0x0102L)
#define E_QE0103_ACCESS_DSH_OBJECT	(E_QEF_MASK + 0x0103L)
#define E_QE0104_RELEASE_DSH_OBJECT	(E_QEF_MASK + 0x0104L)
#define E_QE0105_CORRUPTED_CB		(E_QEF_MASK + 0x0105L)
#define E_QE0110_NO_ROWS_QUALIFIED	(E_QEF_MASK + 0x0110L)
#define E_QE0111_ERROR_SENDING_MESSAGE	(E_QEF_MASK + 0x0111L)
#define E_QE0112_DBP_RETURN		(E_QEF_MASK + 0x0112L)
#define E_QE0113_PARAM_CONVERSION	(E_QEF_MASK + 0x0113L)
#define E_QE0114_PARAM_MISSING		(E_QEF_MASK + 0x0114L)
#define E_QE0115_TABLES_MODIFIED	(E_QEF_MASK + 0x0115L)
#define E_QE0116_CANT_ALLOCATE_DBP_ID	(E_QEF_MASK + 0x0116L)
#define E_QE0117_NONEXISTENT_DBP	(E_QEF_MASK + 0x0117L)
#define E_QE0118_COMPRESSION_CONFLICT	(E_QEF_MASK + 0x0118L)
#define	E_QE0119_LOAD_QP		(E_QEF_MASK + 0x0119L)
#define	E_QE0120_RULE_ACTION_LIST	(E_QEF_MASK + 0x0120L)
#define E_QE011B_PARAM_MISSING		(E_QEF_MASK + 0x011BL)
#define E_QE011C_NONEXISTENT_SYNONYM	(E_QEF_MASK + 0x011CL)
#define E_QE011D_PERIPH_CONVERSION	(E_QEF_MASK + 0x011DL)
#define E_QE0121_USER_RAISE_ERROR	(E_QEF_MASK + 0x0121L)
#define	E_QE0122_ALREADY_REPORTED	(E_QEF_MASK + 0x0122L)
#define	E_QE0123_PARAM_INVALID		(E_QEF_MASK + 0x0123L)
#define	E_QE0124_PARAM_MANY		(E_QEF_MASK + 0x0124L)
#define	E_QE0125_RULES_INHIBIT		(E_QEF_MASK + 0x0125L)
#define E_QE0130_NONEXISTENT_IPROC      (E_QEF_MASK + 0x0130L)
#define E_QE0131_MISSING_IPROC_PARAM    (E_QEF_MASK + 0x0131L)
#define E_QE0132_WRONG_PARAM_NAME       (E_QEF_MASK + 0x0132L)
#define E_QE0133_WRONG_PARAM_TYPE       (E_QEF_MASK + 0x0133L)
#define E_QE0134_LISTFILE_ERROR         (E_QEF_MASK + 0x0134L)
#define E_QE0135_DROPFILE_ERROR         (E_QEF_MASK + 0x0135L)
#define E_QE0136_FINDDBS_ERROR	    	(E_QEF_MASK + 0x0136L)
#define E_QE0137_SEVERE_FINDDBS_ERR	(E_QEF_MASK + 0x0137L)
#define E_QE0138_CANT_CREATE_UNIQUE_CONSTRAINT	(E_QEF_MASK + 0x0138L)
#define E_QE0139_CANT_ADD_REF_CONSTRAINT	(E_QEF_MASK + 0x0139L)
#define E_QE013A_CANT_CREATE_CONSTRAINT	(E_QEF_MASK + 0x013AL)
#define E_QE0140_CANT_EXLOCK_REF_TABLE	(E_QEF_MASK + 0x0140L)
#define E_QE0141_V_UBTID_GETINFO_ERR	(E_QEF_MASK + 0x0141L)
#define E_QE0142_V_UBTID_UNFIX_ERR	(E_QEF_MASK + 0x0142L)
#define	E_QE0143_V_UBTID_GETDESC_ERR	(E_QEF_MASK + 0x0143L)
#define E_QE0144_CANT_ADD_CHECK_CONSTRAINT	(E_QEF_MASK + 0x0144L)
#define E_QE0145_CANT_EXLOCK_CHECK_TABLE	(E_QEF_MASK + 0x0145L)
#define E_QE0146_QSO_JUST_TRANS_ERROR	(E_QEF_MASK + 0x0146L)
#define	E_QE0147_QSO_DESTROY_ERROR	(E_QEF_MASK + 0x0147L)

#define E_QE0160_INVALID_GRPID_OP	(E_QEF_MASK + 0x0160L)
#define E_QE0161_INVALID_APLID_OP	(E_QEF_MASK + 0x0161L)
#define E_QE0162_GRPID_ANCHOR_MISSING	(E_QEF_MASK + 0x0162L)
#define E_QE0163_GRPMEM_PURGE		(E_QEF_MASK + 0x0163L)
#define E_QE0164_GRPID_PURGE		(E_QEF_MASK + 0x0164L)
#define E_QE0165_NONEXISTENT_RULE	(E_QEF_MASK + 0x0165L)
#define	E_QE0166_NO_CONSTRAINT_IN_SCHEMA (E_QEF_MASK + 0x0166L)
#define	E_QE0167_DROP_CONS_WRONG_TABLE	(E_QEF_MASK + 0x0167L)
#define	E_QE0168_ABANDONED_CONSTRAINT	(E_QEF_MASK + 0x0168L)
#define	E_QE0169_UNKNOWN_OBJTYPE_IN_DROP (E_QEF_MASK + 0x0169L)
#define	E_QE016A_NOT_DROPPABLE_CONS	(E_QEF_MASK + 0x016AL)
#define	E_QE016B_COLUMN_HAS_OBJECTS	(E_QEF_MASK + 0x016BL)
#define	E_QE016C_RENAME_TAB_HAS_OBJS	(E_QEF_MASK + 0x016CL)
#define	E_QE016D_RENAME_TAB_HAS_PROC	(E_QEF_MASK + 0x016DL)
#define	E_QE016E_RENAME_TAB_HAS_VIEW	(E_QEF_MASK + 0x016EL)
#define	E_QE016F_RENAME_TAB_HAS_RULE	(E_QEF_MASK + 0x016FL)
#define	E_QE0170_RENAME_TAB_HAS_NGPERM	(E_QEF_MASK + 0x0170L)
#define	E_QE0171_RENAME_TAB_HAS_CONS	(E_QEF_MASK + 0x0171L)
#define	E_QE0172_RENAME_TAB_HAS_SECALARM	(E_QEF_MASK + 0x0172L)
#define	E_QE0173_RENAME_TAB_IS_CATALOG	(E_QEF_MASK + 0x0173L)
#define	E_QE0174_RENAME_TAB_IS_SYSGEN	(E_QEF_MASK + 0x0174L)
#define	E_QE0175_RENAME_TAB_IS_ETAB	(E_QEF_MASK + 0x0175L)
#define	E_QE0176_RENAME_TAB_IS_GATEWAY	(E_QEF_MASK + 0x0176L)
#define	E_QE0177_RENAME_TAB_IS_PARTITION	(E_QEF_MASK + 0x0177L)
#define	E_QE0178_RENAME_TAB_IS_INCONSIST	(E_QEF_MASK + 0x0178L)
#define	E_QE0179_RENAME_TAB_IS_READONLY	(E_QEF_MASK + 0x0179L)
#define	E_QE0180_RENAME_TAB_HAS_INTEGRITY	(E_QEF_MASK + 0x0180L)
#define	E_QE0181_RENAME_COL_HAS_OBJS	(E_QEF_MASK + 0x0181L)

#define	E_QE0190_ENCRYPT_LOCKED		(E_QEF_MASK + 0x0190L)
#define	E_QE0191_RECORD_NOT_ENCRYPTED	(E_QEF_MASK + 0x0191L)
#define	E_QE0192_PASSPHRASE_FAILED_CRC	(E_QEF_MASK + 0x0192L)
#define	E_QE0193_ENCRYPT_NO_ALTER_TABLE	(E_QEF_MASK + 0x0193L)

#define E_QE0199_CALL_ALLOC		(E_QEF_MASK + 0x0199L)
#define E_QE019A_EVENT_MESSAGE		(E_QEF_MASK + 0x019AL)
#define E_QE019B_DMM_CREATE_DB_ERROR	(E_QEF_MASK + 0x019BL)
#define E_QE019C_SEC_WRITE_ERROR        (E_QEF_MASK + 0x019CL)
#define E_QE019D_INVALID_ROLEGRANT_OP   (E_QEF_MASK + 0x019DL)

#define E_QE0200_NO_EVENTS		(E_QEF_MASK + 0x0200L)
#define E_QE0201_BAD_SORT_KEY		(E_QEF_MASK + 0x0201L)
#define	E_QE0202_RESOURCE_CHCK_INVALID	(E_QEF_MASK + 0x0202L)
#define	E_QE0203_DIO_LIMIT_EXCEEDED	(E_QEF_MASK + 0x0203L)
#define	E_QE0204_ROW_LIMIT_EXCEEDED	(E_QEF_MASK + 0x0204L)
#define	E_QE0205_CPU_LIMIT_EXCEEDED	(E_QEF_MASK + 0x0205L)
#define	E_QE0206_PAGE_LIMIT_EXCEEDED	(E_QEF_MASK + 0x0206L)
#define	E_QE0207_COST_LIMIT_EXCEEDED	(E_QEF_MASK + 0x0207L)
#define	E_QE0208_EXCEED_MAX_CALL_DEPTH	(E_QEF_MASK + 0x0208L)
#define	E_QE0209_BAD_RULE_STMT		(E_QEF_MASK + 0x0209L)

#define	E_QE020A_EVENT_EXISTS		(E_QEF_MASK + 0x020AL)
#define	E_QE020B_EVENT_ABSENT		(E_QEF_MASK + 0x020BL)
#define	E_QE020C_EV_REGISTER_EXISTS	(E_QEF_MASK + 0x020CL)
#define	E_QE020D_EV_REGISTER_ABSENT	(E_QEF_MASK + 0x020DL)
#define	E_QE020E_EV_PERMIT_ABSENT	(E_QEF_MASK + 0x020EL)
#define	E_QE020F_EVENT_SCF_FAIL		(E_QEF_MASK + 0x020FL)

#define E_QE0210_QEC_ALTER		(E_QEF_MASK + 0x0210L)
#define E_QE0211_DEFAULT_GROUP		(E_QEF_MASK + 0x0211L)
#define E_QE0212_DEFAULT_MEMBER		(E_QEF_MASK + 0x0212L)
#define E_QE0213_GRPID_EXISTS		(E_QEF_MASK + 0x0213L)
#define E_QE0214_GRPID_NOT_EXIST	(E_QEF_MASK + 0x0214L)
#define E_QE0215_GRPMEM_EXISTS		(E_QEF_MASK + 0x0215L)
#define E_QE0216_GRPMEM_NOT_EXIST	(E_QEF_MASK + 0x0216L)
#define E_QE0217_GRPID_NOT_EMPTY	(E_QEF_MASK + 0x0217L)
#define E_QE0218_APLID_EXISTS		(E_QEF_MASK + 0x0218L)
#define E_QE0219_APLID_NOT_EXIST	(E_QEF_MASK + 0x0219L)
#define E_QE021A_DBPRIV_NOT_AUTH	(E_QEF_MASK + 0x021AL)
#define E_QE021B_DBPRIV_NOT_DBDB	(E_QEF_MASK + 0x021BL)
#define E_QE021C_DBPRIV_DB_UNKNOWN	(E_QEF_MASK + 0x021CL)
#define E_QE021D_DBPRIV_NOT_GRANTED	(E_QEF_MASK + 0x021DL)
#define E_QE021E_DBPRIV_USER_UNKNOWN	(E_QEF_MASK + 0x021EL)
#define E_QE021F_DBPRIV_GROUP_UNKNOWN	(E_QEF_MASK + 0x021FL)
#define E_QE0220_DBPRIV_ROLE_UNKNOWN	(E_QEF_MASK + 0x0220L)
#define E_QE0221_GROUP_USER		(E_QEF_MASK + 0x0221L)
#define E_QE0222_GROUP_ROLE		(E_QEF_MASK + 0x0222L)
#define E_QE0223_ROLE_USER		(E_QEF_MASK + 0x0223L)
#define E_QE0224_ROLE_GROUP		(E_QEF_MASK + 0x0224L)
#define E_QE0225_USER_GROUP		(E_QEF_MASK + 0x0225L)
#define E_QE0226_USER_ROLE		(E_QEF_MASK + 0x0226L)
#define E_QE0227_GROUP_BAD_USER		(E_QEF_MASK + 0x0227L)
#define E_QE0228_DBPRIVS_REMOVED	(E_QEF_MASK + 0x0228L)
#define E_QE0229_DBPRIV_NOT_GRANTED     (E_QEF_MASK + 0x0229L)
#define	E_QE022A_IIUSERGROUP		(E_QEF_MASK + 0x022AL)
#define	E_QE022B_IIAPPLICATION_ID	(E_QEF_MASK + 0x022BL)
#define	E_QE022C_IIDBPRIV		(E_QEF_MASK + 0x022CL)
#define E_QE022D_IIDATABASE		(E_QEF_MASK + 0x022DL)
#define E_QE022E_IIUSER			(E_QEF_MASK + 0x022EL)
#define E_QE022F_SCU_INFO_ERROR		(E_QEF_MASK + 0x022FL)
#define E_QE0230_USRGRP_NOT_EXIST	(E_QEF_MASK + 0x0230L)
#define E_QE0231_USRMEM_ADDED		(E_QEF_MASK + 0x0231L)
#define E_QE0232_USRMEM_DROPPED		(E_QEF_MASK + 0x0232L)
#define E_QE0233_ALLDBS_NOT_AUTH	(E_QEF_MASK + 0x0233L)
#define E_QE0234_USER_ROLE		(E_QEF_MASK + 0x0234L)
#define E_QE0235_USER_GROUP		(E_QEF_MASK + 0x0235L)
/* unused E_QE0236 unused */
#define E_QE0237_DROP_DBA		(E_QEF_MASK + 0x0237L)
#define E_QE0238_NO_MSTRAN              (E_QEF_MASK + 0x0238L)
#define	E_QE0239_SEQ_PERMIT_ABSENT	(E_QEF_MASK + 0x0239L)
#define E_QE0240_BAD_LOC_TYPE		(E_QEF_MASK + 0x0240L)
#define E_QE0241_DB_NOT_EXTENDED	(E_QEF_MASK + 0x0241L)
#define E_QE0242_BAD_EXT_TYPE		(E_QEF_MASK + 0x0242L)
#define E_QE0243_CNF_INCONSIST		(E_QEF_MASK + 0x0243L)
#define E_QE0244_TOO_FEW_LOCS		(E_QEF_MASK + 0x0244L)
#define E_QE0245_TOO_MANY_LOCS		(E_QEF_MASK + 0x0245L)
#define	E_QE0250_ABANDONED_PERMIT	(E_QEF_MASK + 0x0250L)
#define E_QE0251_ABANDONED_VIEW		(E_QEF_MASK + 0x0251L)
#define E_QE0252_ABANDONED_DBPROC	(E_QEF_MASK + 0x0252L)
#define	E_QE0253_ALL_PRIV_NONE_REVOKED	(E_QEF_MASK + 0x0253L)
#define E_QE0254_SOME_PRIV_NOT_REVOKED	(E_QEF_MASK + 0x0254L)
#define	E_QE0255_NO_SCHEMA_TO_DROP	(E_QEF_MASK + 0x0255L)
#define	E_QE0256_NONEMPTY_SCHEMA	(E_QEF_MASK + 0x0256L)
#define	E_QE0257_ABANDONED_CONSTRAINT	(E_QEF_MASK + 0x0257L)
#define	E_QE0258_ILLEGAL_XACT_STMT	(E_QEF_MASK + 0x0258L)
#define E_QE0280_MAC_ERROR		(E_QEF_MASK + 0x0280L)
#define E_QE0281_DEF_LBL_ERROR		(E_QEF_MASK + 0x0281L)
#define E_QE0282_IIPROFILE		(E_QEF_MASK + 0x0282L)
#define E_QE0283_DUP_OBJ_NAME		(E_QEF_MASK + 0x0283L)
#define E_QE0284_OBJECT_NOT_EXISTS	(E_QEF_MASK + 0x0284L)
#define	E_QE0285_PROFILE_USER_MRG_ERR   (E_QEF_MASK + 0x0285L)
#define	E_QE0286_PROFILE_ABSENT   	(E_QEF_MASK + 0x0286L)
#define E_QE0287_PROFILE_IN_USE 	(E_QEF_MASK + 0x0287L)
#define E_QE0288_DROP_DEFAULT_PROFILE 	(E_QEF_MASK + 0x0288L)
#define E_QE0289_EXTEND_DB_LOC_LABEL 	(E_QEF_MASK + 0x0289L)
#define E_QE028A_EXTEND_DB_LOC_ERR 	(E_QEF_MASK + 0x028AL)
#define E_QE028B_CREATE_ALARM_ERR 	(E_QEF_MASK + 0x028BL)
#define E_QE028C_DROP_ALARM_ERR 	(E_QEF_MASK + 0x028CL)
#define E_QE028D_ALARM_DBEVENT_ERR	(E_QEF_MASK + 0x028DL)
#define E_QE028E_DB_NOT_UNEXTENDED	(E_QEF_MASK + 0x028EL)

#define E_QE0300_USER_MSG		(E_QEF_MASK + 0x0300L)
#define E_QE0301_TBL_SIZE_CHANGE        (E_QEF_MASK + 0x0301L)
#define E_QE0302_SCHEMA_EXISTS          (E_QEF_MASK + 0x0302L)
#define E_QE0303_PARSE_EXECIMM_QTEXT            (E_QEF_MASK + 0x0303L)
#define E_QE0304_SCHEMA_DOES_NOT_EXIST  (E_QEF_MASK + 0x0304L)
#define E_QE0305_UDT_DEFAULT_TOO_BIG    (E_QEF_MASK + 0x0305L)
#define E_QE0306_ALLOCATE_BYREF_TUPLE	(E_QEF_MASK + 0x0306L)
#define	E_QE0307_INIT_MO_ERROR		(E_QEF_MASK + 0x0307L)
#define	E_QE0308_COL_SIZE_MISCOMPILED	(E_QEF_MASK + 0x0308L)
#define E_QE0309_NO_BMCACHE_BUFFERS     (E_QEF_MASK + 0x0309L)
#define E_QE030A_TTAB_PARM_ERROR	(E_QEF_MASK + 0x030AL)
#define E_QE030B_RULE_PROC_MISMATCH	(E_QEF_MASK + 0x030BL)
#define E_QE030C_BYREF_IN_ROWPROC	(E_QEF_MASK + 0x030CL)
#define E_QE030D_NESTED_ROWPROCS	(E_QEF_MASK + 0x030DL)
#define E_QE030E_RULE_ROWPROCS		(E_QEF_MASK + 0x030EL)
#define E_QE030F_LOAD_TPROC_QP          (E_QEF_MASK + 0x030FL)
#define E_QE0310_INVALID_TPROC_ACT      (E_QEF_MASK + 0x0310L)

/*  Gateway error codes start here */

#define E_QE0400_GATEWAY_ERROR          (E_QEF_MASK + 0x0400L)
#define E_QE0401_GATEWAY_ACCESS_ERROR   (E_QEF_MASK + 0x0401L)

/*  STAR error codes start here  */

/*  RQF errors: 500 to 6FF */

#define E_QE0500_RQF_GENERIC_ERR	(E_QEF_MASK + 0x0500L)
#define	E_QE0501_WRONG_COLUMN_COUNT	(E_QEF_MASK + 0x0501L)
#define	E_QE0502_NO_TUPLE_DESCRIPTION	(E_QEF_MASK + 0x0502L)
#define	E_QE0503_TOO_MANY_COLUMNS	(E_QEF_MASK + 0x0503L)
#define	E_QE0504_BIND_BUFFER_TOO_SMALL	(E_QEF_MASK + 0x0504L)
#define E_QE0505_CONVERSION_FAILED	(E_QEF_MASK + 0x0505L)
#define E_QE0506_CANNOT_GET_ASSOCIATION	(E_QEF_MASK + 0x0506L)
#define E_QE0507_BAD_REQUEST_CODE	(E_QEF_MASK + 0x0507L)
#define E_QE0508_SCU_MALLOC_FAILED	(E_QEF_MASK + 0x0508L)
#define E_QE0509_ULM_STARTUP_FAILED	(E_QEF_MASK + 0x0509L)
#define E_QE0510_ULM_OPEN_FAILED	(E_QEF_MASK + 0x0510L)
#define	E_QE0511_INVALID_READ		(E_QEF_MASK + 0x0511L)
#define	E_QE0512_INVALID_WRITE		(E_QEF_MASK + 0x0512L)
#define	E_QE0513_ULM_CLOSE_FAILED	(E_QEF_MASK + 0x0513L)
#define	E_QE0514_QUERY_ERROR		(E_QEF_MASK + 0x0514L)
#define	E_QE0515_UNEXPECTED_MESSAGE	(E_QEF_MASK + 0x0515L)
#define	E_QE0516_CONVERSION_ERROR	(E_QEF_MASK + 0x0516L)
#define	E_QE0517_NO_ACK			(E_QEF_MASK + 0x0517L)
#define	E_QE0518_SHUTDOWN_FAILED	(E_QEF_MASK + 0x0518L)
#define	E_QE0519_COMMIT_FAILED		(E_QEF_MASK + 0x0519L)
#define	E_QE0520_ABORT_FAILED		(E_QEF_MASK + 0x0520L)
#define	E_QE0521_BEGIN_FAILED		(E_QEF_MASK + 0x0521L)
#define	E_QE0522_END_FAILED		(E_QEF_MASK + 0x0522L)
#define	E_QE0523_COPY_FROM_EXPECTED	(E_QEF_MASK + 0x0523L)
#define	E_QE0524_COPY_DEST_FAILED	(E_QEF_MASK + 0x0524L)
#define	E_QE0525_COPY_SOURCE_FAILED	(E_QEF_MASK + 0x0525L)
#define	E_QE0526_QID_EXPECTED		(E_QEF_MASK + 0x0526L)
#define	E_QE0527_CURSOR_CLOSE_FAILED	(E_QEF_MASK + 0x0527L)
#define	E_QE0528_CURSOR_FETCH_FAILED	(E_QEF_MASK + 0x0528L)
#define	E_QE0529_CURSOR_EXEC_FAILED	(E_QEF_MASK + 0x0529L)
#define	E_QE0530_CURSOR_DELETE_FAILED	(E_QEF_MASK + 0x0530L)
#define	E_QE0531_INVALID_CONTINUE	(E_QEF_MASK + 0x0531L)
#define	E_QE0532_DIFFERENT_TUPLE_SIZE	(E_QEF_MASK + 0x0532L)
#define	E_QE0533_FETCH_FAILED		(E_QEF_MASK + 0x0533L)
#define	E_QE0534_COPY_CREATE_FAILED	(E_QEF_MASK + 0x0534L)
#define	E_QE0535_BAD_COL_DESC_FORMAT	(E_QEF_MASK + 0x0535L)
#define	E_QE0536_COL_DESC_EXPECTED	(E_QEF_MASK + 0x0536L)
#define	E_QE0537_II_LDB_NOT_DEFINED	(E_QEF_MASK + 0x0537L)
#define	E_QE0538_ULM_ALLOC_FAILED	(E_QEF_MASK + 0x0538L)
#define	E_QE0539_INTERRUPTED		(E_QEF_MASK + 0x0539L)
#define	E_QE0540_UNKNOWN_REPEAT_Q	(E_QEF_MASK + 0x0540L)
#define	E_QE0541_ERROR_MSG_FROM_LDB	(E_QEF_MASK + 0x0541L)
#define	E_QE0542_LDB_ERROR_MSG		(E_QEF_MASK + 0x0542L)
#define	E_QE0543_CURSOR_UPDATE_FAILED	(E_QEF_MASK + 0x0543L)
#define	E_QE0544_CURSOR_OPEN_FAILED	(E_QEF_MASK + 0x0544L)
#define	E_QE0545_CONNECTION_LOST	(E_QEF_MASK + 0x0545L)
#define	E_QE0546_RECV_TIMEOUT		(E_QEF_MASK + 0x0546L)
#define	E_QE0547_UNAUTHORIZED_USER	(E_QEF_MASK + 0x0547L)
#define E_QE0548_SECURE_FAILED		(E_QEF_MASK + 0x0548L)
#define E_QE0549_RESTART_FAILED		(E_QEF_MASK + 0x0549L)



#define E_QE7800_LDB_GENERIC_ERR	(E_QEF_MASK + 0x7800L)
#define E_QE7801_LDB_BAD_SQL_QRY	(E_QEF_MASK + 0x7801L)
#define E_QE7802_LDB_XACT_ENDED		(E_QEF_MASK + 0x7802L)

/*  TPF errors: 700 to 8FF */

#define E_QE0700_TPF_GENERIC_ERR	(E_QEF_MASK + 0x0700L)
#define	E_QE0701_INVALID_REQUEST	(E_QEF_MASK + 0x0701L)
#define	E_QE0702_SCF_MALLOC_FAILED	(E_QEF_MASK + 0x0702L)
#define	E_QE0703_SCF_MFREE_FAILED	(E_QEF_MASK + 0x0703L)
#define	E_QE0704_MULTI_SITE_WRITE	(E_QEF_MASK + 0x0704L)
#define	E_QE0705_UNKNOWN_STATE		(E_QEF_MASK + 0x0705L)
#define	E_QE0706_INVALID_EVENT		(E_QEF_MASK + 0x0706L)
#define	E_QE0707_INVALID_TRANSITION	(E_QEF_MASK + 0x0707L)
#define	E_QE0708_TXN_BEGIN_FAILED	(E_QEF_MASK + 0x0708L)
#define	E_QE0709_TXN_FAILED		(E_QEF_MASK + 0x0709L)
#define	E_QE0710_SAVEPOINT_FAILED	(E_QEF_MASK + 0x0710L)
#define	E_QE0711_SP_ABORT_FAILED	(E_QEF_MASK + 0x0711L)
#define	E_QE0712_SP_NOT_EXIST		(E_QEF_MASK + 0x0712L)
#define	E_QE0713_AUTO_ON_NO_SP		(E_QEF_MASK + 0x0713L)
#define	E_QE0714_ULM_STARTUP_FAILED	(E_QEF_MASK + 0x0714L)
#define	E_QE0715_ULM_OPEN_FAILED	(E_QEF_MASK + 0x0715L)
#define	E_QE0716_ULM_ALLOC_FAILED	(E_QEF_MASK + 0x0716L)
#define	E_QE0717_ULM_CLOSE_FAILED	(E_QEF_MASK + 0x0717L)
#define	E_QE0720_TPF_INTERNAL		(E_QEF_MASK + 0x0720L)
#define	E_QE0721_LOGDX_FAILED		(E_QEF_MASK + 0x0721L)
#define	E_QE0722_SECURE_FAILED		(E_QEF_MASK + 0x0722L)
#define	E_QE0723_COMMIT_FAILED		(E_QEF_MASK + 0x0723L)
#define E_QE0724_DDB_IN_RECOVERY	(E_QEF_MASK + 0x0724L)
					/* DDB is being recovered and cannot
					** therefore be accessed (not used ) */



/*  INTERNAL QUERY errors: 900 to 919  */

#define E_QE0900_BAD_LOCAL_TABLE_NAME	(E_QEF_MASK + 0x0900L)
#define E_QE0901_BAD_LOCAL_COLUMN_COUNT	(E_QEF_MASK + 0x0901L)
#define E_QE0902_NO_LDB_TAB_ON_CRE_LINK	(E_QEF_MASK + 0x0902L)


/*  CREATE LINK errors: 940 to 94E  */


/*  DROP/DESTROY LINK errors: 950 to 95E  */

#define E_QE0950_INVALID_DROP_LINK	(E_QEF_MASK + 0x0950L)


/*  DIRECT CONNECT/DISCONNECT errors: 960 to 96E  */

#define E_QE0960_CONNECT_ON_CONNECT	(E_QEF_MASK + 0x0960L)
#define E_QE0961_CONNECT_ON_XACTION	(E_QEF_MASK + 0x0961L)
#define E_QE0962_DDL_CC_ON_XACTION	(E_QEF_MASK + 0x0962L)

#define E_QE096E_INVALID_DISCONNECT	(E_QEF_MASK + 0x096EL)


/*  OPEN DDB errors: 970 to 97E  */

#define E_QE0970_BAD_USER_NAME		(E_QEF_MASK + 0x0970L)


/*  Generic classes of errors: 980 to 99E  */

#define E_QE0980_LDB_REPORTED_ERROR	(E_QEF_MASK + 0x0980L)


/*  2PC related errors: 990..? */

#define E_QE0990_2PC_BAD_LDB_MIX	(E_QEF_MASK + 0x0990L)
					/* current write query contains LDB(s) 
					** that would cause 1PC/2PC conflict 
					** with others already in the DX */
#define E_QE0991_LDB_ALTERED_XACT	(E_QEF_MASK + 0x0991L)
					/* LDB execute procedure or execute
					** immediate tried to alter xact.
					** QEF will abort 2PC xact. */



/*  Titan error codes end here  */


/*
**  Diagnostic Error Codes.  Used when QEF is compiled with "xDEBUG" defined.
*/
#define E_QE1001_NO_ACTIONS_IN_QP	(E_QEF_MASK + 0x1001L)
#define E_QE1002_BAD_ACTION_COUNT	(E_QEF_MASK + 0x1002L)
#define E_QE1003_NO_VALID_LIST		(E_QEF_MASK + 0x1003L)
#define E_QE1004_TABLE_NOT_OPENED	(E_QEF_MASK + 0x1004L)
#define E_QE1005_WRONG_JOIN_NODE	(E_QEF_MASK + 0x1005L)
#define E_QE1006_BAD_SORT_DESCRIPTOR	(E_QEF_MASK + 0x1006L)
#define E_QE1007_BAD_SORT_COUNT		(E_QEF_MASK + 0x1007L)
#define E_QE1009_DSH_NOTMATCH_QP	(E_QEF_MASK + 0x1009L)
#define E_QE1010_HOLDING_SORT_BUFFER	(E_QEF_MASK + 0x1010L)

/*  Titan diagnostic error codes: 1999 and lower */

#define E_QE1995_BAD_AGGREGATE_ACTION	(E_QEF_MASK + 0x1995L)
#define E_QE1996_BAD_ACTION		(E_QEF_MASK + 0x1996L)
#define E_QE1997_NO_ACTION		(E_QEF_MASK + 0x1997L)
#define E_QE1998_BAD_QMODE		(E_QEF_MASK + 0x1998L)
#define E_QE1999_BAD_QP			(E_QEF_MASK + 0x1999L)

/* Production mode error messages */
# define E_QE7610_SPROD_UPD_IIDB        (E_QEF_MASK + 0x7610L)
# define E_QE7612_PROD_MODE_ERR         (E_QEF_MASK + 0x7612L)

/*
**  Informational Error Codes.  Used for building international 
**  audit text messages, etc.
*/
#define I_QE2001_ROLE_CREATE		(E_QEF_MASK + 0x2001L)
#define I_QE2002_ROLE_DROP		(E_QEF_MASK + 0x2002L)
#define I_QE2003_DBACCESS_CREATE	(E_QEF_MASK + 0x2003L)
#define I_QE2004_DBACCESS_DROP		(E_QEF_MASK + 0x2004L)
#define I_QE2005_DBPRIV_CREATE		(E_QEF_MASK + 0x2005L)
#define I_QE2006_DBPRIV_DROP		(E_QEF_MASK + 0x2006L)
#define I_QE2007_GROUP_CREATE		(E_QEF_MASK + 0x2007L)
#define I_QE2008_GROUP_MEM_CREATE	(E_QEF_MASK + 0x2008L)
#define I_QE2009_GROUP_DROP		(E_QEF_MASK + 0x2009L)
#define I_QE200A_GROUP_MEM_DROP		(E_QEF_MASK + 0x200AL)
#define I_QE200B_LOCATION_CREATE	(E_QEF_MASK + 0x200BL)
#define I_QE200C_USER_CREATE		(E_QEF_MASK + 0x200CL)
#define I_QE200D_USER_DROP		(E_QEF_MASK + 0x200DL)
#define I_QE200E_SEC_EVENT_CREATE	(E_QEF_MASK + 0x200EL)
#define I_QE200F_SEC_EVENT_DROP	        (E_QEF_MASK + 0x200FL)
#define I_QE2010_SEC_LEVEL_CREATE	(E_QEF_MASK + 0x2010L)
#define I_QE2011_SEC_LEVEL_DROP	        (E_QEF_MASK + 0x2011L)
#define I_QE2012_DBPROC_CREATE		(E_QEF_MASK + 0x2012L)
#define I_QE2013_DBPROC_DROP		(E_QEF_MASK + 0x2013L)
#define I_QE2014_VIEW_CREATE		(E_QEF_MASK + 0x2014L)
#define I_QE2015_VIEW_DROP		(E_QEF_MASK + 0x2015L)
#define I_QE2016_PROT_TAB_CREATE	(E_QEF_MASK + 0x2016L)
#define I_QE2017_PROT_DBP_CREATE	(E_QEF_MASK + 0x2017L)
#define I_QE2018_PROT_TAB_DROP		(E_QEF_MASK + 0x2018L)
#define I_QE2019_PROT_DBP_DROP		(E_QEF_MASK + 0x2019L)
#define I_QE201A_DATABASE_ACCESS	(E_QEF_MASK + 0x201AL)
#define I_QE201B_ROLE_ACCESS		(E_QEF_MASK + 0x201BL)
#define I_QE201C_SEC_CAT_UPDATE 	(E_QEF_MASK + 0x201CL)
#define I_QE201D_DBPROC_EXECUTE 	(E_QEF_MASK + 0x201DL)
#define I_QE201E_TABLE_CREATE		(E_QEF_MASK + 0x201EL)
#define I_QE201F_DATABASE_EXTEND	(E_QEF_MASK + 0x201FL)
#define I_QE2020_PROT_REJECT            (E_QEF_MASK + 0x2020L)
#define I_QE2021_ROLE_ALTER		(E_QEF_MASK + 0x2021L)
#define I_QE2022_GROUP_ALTER		(E_QEF_MASK + 0x2022L)
#define I_QE2023_USER_ALTER		(E_QEF_MASK + 0x2023L)
#define I_QE2024_USER_ACCESS	        (E_QEF_MASK + 0x2024L)
#define I_QE2025_TABLE_DROP	        (E_QEF_MASK + 0x2025L)
#define I_QE2026_TABLE_ACCESS	        (E_QEF_MASK + 0x2026L)
#define I_QE2027_VIEW_ACCESS	        (E_QEF_MASK + 0x2027L)
#define I_QE2028_ALARM_EVENT            (E_QEF_MASK + 0x2028L)
#define I_QE2029_DATABASE_CREATE        (E_QEF_MASK + 0x2029L)
#define I_QE202A_DATABASE_DROP	        (E_QEF_MASK + 0x202AL)
#define I_QE202B_LOCATION_ALTER		(E_QEF_MASK + 0x202BL)
#define I_QE202C_LOCATION_DROP		(E_QEF_MASK + 0x202CL)

#define I_QE202D_ALARM_CREATE		(E_QEF_MASK + 0x202DL)
#define I_QE202E_ALARM_DROP		(E_QEF_MASK + 0x202EL)
#define I_QE202F_RULE_ACCESS		(E_QEF_MASK + 0x202FL)
#define I_QE2030_PROT_EV_CREATE         (E_QEF_MASK + 0x2030L)
#define I_QE2031_PROT_EV_DROP           (E_QEF_MASK + 0x2031L)
#define I_QE2032_EVENT_ACCESS           (E_QEF_MASK + 0x2032L)
#define I_QE2033_SEC_VIOLATION          (E_QEF_MASK + 0x2033L)
#define I_QE2034_READONLY_TABLE_ERR     (E_QEF_MASK + 0x2034L)
#define I_QE2035_SET_ID			(E_QEF_MASK + 0x2035L)
#define I_QE2036_INTEGRITY_DROP		(E_QEF_MASK + 0x2036L)
#define I_QE2037_CONSTRAINT_TAB_IDX_ERR (E_QEF_MASK + 0x2037L)
#define I_QE2038_CONSTRAINT_TAB_IDX_ERR (E_QEF_MASK + 0x2038L)

/* informational messages NOT for auditing */
#define I_QE3000_LONGRUNNING_QUERY      (E_QEF_MASK + 0x3000L)
#define I_QE3001_LONGRUNNING_QUERY      (E_QEF_MASK + 0x3001L)

/*
 * Geospatial error codes
 */
#define E_QE5423_SRID_MISMATCH	        (E_QEF_MASK + 0x5423L)
#define E_QE5424_INVALID_SRID           (E_QEF_MASK + 0x5424L)
#define E_QE5425_SRS_NONEXISTENT        (E_QEF_MASK + 0x5425L)

#define TEMP_TABLE_MACRO(dmt_cb)    ((i4)(dmt_cb)->dmt_id.db_tab_base < 0 &&\
					MEcmp( ( PTR) &(dmt_cb)->dmt_owner,\
					( PTR ) qef_cb->qef_cat_owner,\
					( u_i2 ) sizeof(DB_OWN_NAME)) == 0)

/*
**	Entry point for the QUERY EXECUTION FACILITY
*/

FUNC_EXTERN	DB_STATUS
qef_call(
i4	    qef_id,	/* request ids documented immediately below */
void *	    rcb );


/*
**  Define QEF request ids.  These are the request codes that go into
**  the first argument (qef_id) of a qef_call invocation.
*/

#define QEC_INITIALIZE	    2
#define QEC_BEGIN_SESSION   3
#define QEC_DEBUG	    4
#define QEC_ALTER	    5
#define QEC_TRACE	    6
#define QET_BEGIN	    7
#define QET_SAVEPOINT	    8
#define QET_ABORT	    9
#define QET_COMMIT	    10
#define QEQ_OPEN	    11
#define QEQ_FETCH	    12
#define QEQ_REPLACE	    13
#define QEQ_DELETE	    14
#define QEQ_CLOSE	    15
#define QEQ_QUERY	    16
#define QEU_B_COPY	    17
#define QEU_E_COPY	    18
#define QEU_R_COPY	    19
#define QEU_W_COPY	    20
#define QEC_INFO	    21
#define QEC_SHUTDOWN	    22
#define QEC_END_SESSION	    23
#define QEU_BTRAN	    24
#define QEU_ETRAN	    25
#define QEU_APPEND	    26
#define QEU_GET		    27
#define QEU_DELETE	    28
#define QEU_OPEN	    29
#define QEU_CLOSE	    30
#define QEU_DBU		    31
#define QEU_ATRAN	    32
#define	QEU_QUERY	    33
#define QEU_DBP_STATUS	    34
#define QEU_DVIEW	    35
#define QEU_CPROT	    36
#define QEU_DPROT	    37
#define QEU_CINTG	    38
#define QEU_DINTG	    39
#define QEU_DSTAT	    40
#define QET_SCOMMIT	    41
#define QET_AUTOCOMMIT	    42
#define QET_ROLLBACK	    43
#define QEQ_ENDRET	    44
#define QEQ_EXECUTE	    45
#define QEU_CREDBP	    46
#define QEU_DRPDBP	    47
#define QET_SECURE	    48
#define QET_RESUME	    49
#define	QEU_GROUP	    50
#define QEU_APLID	    51
#define QEU_CRULE	    52
#define QEU_DRULE	    53
#define QEU_DBPRIV	    54
#define QEU_CUSER	    55
#define QEU_DUSER	    56
#define QEU_CLOC	    57
#define QEU_CDBACC	    58
#define QEU_DDBACC	    59
#define QEU_MSEC	    60
#define QEU_AUSER	    61
#define QEU_CEVENT	    62      /* Create an event */
#define QEU_DEVENT          63      /* Drop an event */
#define QEU_ALOC	    64
#define QEU_DLOC	    65
#define QEU_CCOMMENT	    66	    /* Create a comment */
#define QEU_CSYNONYM	    67	    /* CREATE SYNONYM   */
#define QEU_DSYNONYM	    68	    /* DROP SYNONYM   */
/* unused		    69	    Available */
#define	QEU_REVOKE	    70
#define QEU_DSCHEMA	    71
#define QEQ_UPD_ROWCNT      72      /* Support for SET UPDATE_ROWCOUNT */
#define QEU_CPROFILE        100     /* CREATE PROFILE */
#define QEU_APROFILE        101     /* ALTER PROFILE */
#define QEU_DPROFILE        102     /* DROP PROFILE */
#define QEU_RAISE_EVENT     103	    /* Raise a dbevent */
#define QEU_CSECALM         104	    /* CREATE SECURITY_ALARM */
#define QEU_DSECALM         105	    /* DROP SECURITY_ALARM */
#define QEU_ROLEGRANT	    106     /* GRANT/REVOKE ROLE */
/* unused		    107	    Available */
#define QEU_IPROC_INFO	    108	    /* get info about internal DB procs */
#define QEU_EXEC_IPROC      109     /* EXECUTE an internal procedure */
#define QEU_CSEQ      	    110     /* CREATE SEQUENCE */
#define QEU_ASEQ      	    111     /* ALTER SEQUENCE */
#define QEU_DSEQ      	    112     /* DROP SEQUENCE */
#define QET_XA_PREPARE	    113     /* xa_prepare() disassociated XA txn */
#define QET_XA_COMMIT	    114	    /* xa_commit() disassociated XA txn */
#define QET_XA_ROLLBACK	    115	    /* xa_rollback() XA txn */
#define QET_XA_START	    116	    /* xa_start(flags) */
#define QET_XA_END	    117	    /* xa_end(flags) */
#define QEU_OP_MAX          118

#define QEF_LO_MAX_OP       QEU_OP_MAX

/*  Titan operation codes start here  */

#define QEF_HI_MIN_OP	500		/* all users of this define must follow
					** any changes */
#define QED_IIDBDB	500		/* retrieve control info for 
					** accessing IIDBDB */
#define QED_DDBINFO	501		/* retrieve DDB's info */
#define QED_LDBPLUS	502		/* retrieve LDB's plus info */
#define QED_USRSTAT	503		/* retrieve user's DB access status */
#define QED_DESCPTR	504		/* set 
					** QEF_CB.qef_c2_ddb_ses.qes_d4_ddb_p
					*/
#define QED_SESSCDB	505		/* pass CDB info to TPF via QEF for
					** new session on DDB known to SCF */
#define QED_C1_CONN	507		/* DIRECT CONNECT */
#define QED_C2_CONT	508		/* continue within a CONNECT state,
					** called by SCF */
#define QED_C3_DISC	509		/* DIRECT DISCONNECT */
#define QED_EXEC_IMM	510		/* send a query to target LDB
					** for direct execution */
#define QED_CLINK	512		/* CREATE/DEFINE LINK */
#define QED_DLINK	513		/* DROP/DESTROY LINK */
#define QED_RLINK	514		/* REREGISTER LINK command */

#define QED_1RDF_QUERY	515		/* RDF support operation 1: send a
					** query to an LDB for execution */
#define QED_2RDF_FETCH	516		/* RDF support operation 2: read the
					** next tuple from a sending LDB */
#define QED_3RDF_FLUSH	517		/* RDF support operation 3: flush all
					** all the tuples from a sending LDB */
#define QED_4RDF_VDATA	518		/* RDF support operation 4: send a
					** query with data values to an LDB */
#define QED_5RDF_BEGIN	519		/* RDF support operation 5: indicate
					** beginning of a series of purely read 
					** or update queries on an LDB; this 
					** causes QEF to inform TPF once and
					** only once for all the queries, an
					** optimization for minimizing the
					** number of TPF calls */
#define QED_6RDF_END	520		/* order QEF to send a commit message 
					** over the CDB association */
#define QED_7RDF_DIFF_ARCH	521	/* send a QUEL RANGE statement to LDB */
#define QED_IS_DDB_OPEN		522	/* (NOT used, saved for reference)
					** query if a DDB is in the process of 
					** being recovered */
#define QED_1SCF_MODIFY		523	/* send a SET statement to RQF */
#define QED_2SCF_TERM_ASSOC	524	/* close with RQF the open IIDBDB
					** association */
#define QED_A_COPY	525		/* SCF aborts COPY command for error */
#define QED_B_COPY	526		/* SCF begins COPY command */
#define QED_E_COPY	527		/* SCF ends COPY command */
#define QED_CONCURRENCY	528		/* SET DDL CONCURRENCY command */
#define QED_SET_FOR_RQF	529		/* deliver a SET statement to RQF */
#define QED_SES_TIMEOUT	530		/* a SET SESSION TIMEOUT for QEF */
#define QED_QRY_FOR_LDB	531		/* a query for an LDB */
#define QED_1PSF_RANGE	532		/* send a QUEL RANGE statement to LDB */
#define QED_P1_RECOVER	533		/* phase 1 of server startup recovery,
					** TPF is called to find out if there
					** is any recovery work necessary */
#define QED_P2_RECOVER	534		/* phase 2 of server startup recovery,
					** TPF is called to finish off the
					** necessary recovery */
#define QED_RECOVER	535		/* (NOT used, saved for reference)
					** superseded by QED_RECOVER_P1 and
					** QED_RECOVER_P2 */
#define QED_3SCF_CLUSTER_INFO	536	/* send cluster node names to RQF */

#define QEF_HI_MAX_OP	536		/* must be set to the highest request
					** code in order for qef_call to work;
					** all users of this define must follow
					** any changes */

/*  Titan operation codes end here  */

typedef struct _QEF_SRCB QEF_SRCB;


/*}
** Name: QEF_FUNC - This is an entry in the function map
**
** Description:
**      Given a function id, this map will give the function location.
**  This is provided to improve the flexibility of the facility, to make
**  it easier to make use of new functions, and to improve performance.
**
** History:
**     2-apr-86 (daved)
**          written
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedefs to quiesce the ANSI C
**	    compiler warnings.
**	26-feb-99 (inkdo01)
**	    Added QE_HJOIN for hash join.
**	7-nov-03 (inkdo01)
**	    Added QE_EXCHANGE for exchange node.
**	4-Jun-2009 (kschendel) b122118
**	    Properly prototype the function, they are all the same now.
[@history_line@]...
*/
/* Forward pointer typedefs, gcc complains if these are inside the
** parameter list...
*/
typedef struct _QEN_NODE *QEN_NODE_P;
typedef struct _QEF_RCB *QEF_RCB_P;
typedef struct _QEE_DSH *QEE_DSH_P;

struct _QEF_FUNC
{
    i4              func_id;            /* function id */
#define QE_TJOIN   0                    /* tid join */
#define QE_KJOIN   1                    /* key join */
#define QE_ORIG    2                    /* orig node */
#define QE_FSMJOIN 3			/* full sort merge join node */	       
#define QE_ISJOIN  4			/* inner sorted join node */	       
#define QE_CPJOIN  5			/* cart-prod join node */	       
#define QE_SORT    6                    /* sort node */
#define QE_TSORT   7                    /* top sort node */
#define QE_QP      8                    /* query plan node */
#define QE_SEJOIN  9			/* select join */
#define QE_ORIGAGG 10			/* Aggf Optimization */
#define QE_HJOIN   11			/* hash join */
#define QE_EXCHANGE 12			/* exchange */
#define QE_TPROC   13                   /* table procedure */
    DB_STATUS       (*func_name)(QEN_NODE_P, QEF_RCB_P, QEE_DSH_P, i4);
};


/*}
** Name: QEF_SRCB - server request block
**
** Description:
**      This control block is used to request QEF to start up.
**
** History:
**     29-may-86 (daved)
**          written
**     26-sep-88 (puree)
**	    Modified for QEF in-memory sorter.  Added qef_sort_maxmem and
**	    qef_sess_max.  Renamed qef_memmax to qef_dsh_maxmem.
**      14-jul-89 (jennifer)
**          Added a flag mask field to QEF_SRCB to allow operation 
**          to be altered.  In this case allowed server to be intialized
**          to C2 secure.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**     06-mar-96 (nanpr01)
**          Added new field qef_maxtup in QEF_SRCB for incresed tuple size
**	    project.
**	9-sep-99 (stephenb)
**	    Add def for qef_star_gca_handle to be passed to RQF for new GCA 
**	    interface.
**	28-jan-00 (inkdo01)
**	    Added qef_hash_max for hash join implementation. 
**	1-Dec-2003 (schka24)
**	    Add total sort-hash memory parameter holder.
**	24-Oct-2005 (schka24)
**	    Add memory-delay parameter holder.
**	6-Mar-2007 (kschendel) SIR 122512
**	    Add hash file read/write buffer size suggestions.
**      7-jun-2007 (huazh01)
**          Add qef_nodep_chk for the config parameter which switches
**          ON/OFF the fix to b112978.
[@history_line@]...
*/
struct _QEF_SRCB
{
    /* standard stuff */
    QEF_SRCB    *qef_next;              /* The next control block */
    QEF_SRCB    *qef_prev;              /* The previous one */
    SIZE_TYPE   qef_length;             /* size of the control block */
    i2          qef_type;               /* type of control block */
#define QESRCB_CB    3
    i2          qef_s_reserved;
    PTR         qef_l_reserved;
    PTR         qef_owner;
    i4          qef_ascii_id;           /* to view in a dump */
#define QESRCB_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'S', 'R')
    i4	qef_ssize;		/* size of session control block */
    SIZE_TYPE	qef_dsh_maxmem;		/* max memory for DSH (total) */
    SIZE_TYPE	qef_sorthash_memory;	/* max memory for sort+hash (total) */
    /* qef_sort_maxmem and qef_hash_maxmem are per-session limits */
    i4		qef_sort_maxmem;	/* max memory for sort buffer */
    i4		qef_hash_maxmem;	/* max memory for hash join bufs */
    i4		qef_sess_max;		/* max number of active sessions */
    i4          qef_qpmax;              /* maximum number of queries in QEF */
    i4     qef_maxtup;             /* maximum tuple size in installation */
    i4		qef_max_mem_sleep;	/* Max for-memory delay in seconds */
    u_i4	qef_hash_rbsize;	/* Hash file read size suggestion */
    u_i4	qef_hash_wbsize;	/* Hash file write size suggestion */
    u_i4	qef_hashjoin_min;	/* Minimum hash join reservation */
    u_i4	qef_hashjoin_max;	/* Maximum hash join reservation */
    u_i4	qef_hash_cmp_threshold;	/* Minimum row size for considering
					** compression for hashops */
    i4		qef_eflag;		/* type of error handling semantics */
#define	QEF_EXTERNAL	0		/* send error to user */
#define QEF_INTERNAL	1		/* return error code to caller */
    PTR		qef_server;		/* address of server control block */
    i4     qef_flag_mask;          /* Flag indicating special server
					** startup indicators. */
#define         QEF_F_C2SECURE          0x0001 
                                        /* Indicates server is C2 secure. */
    DB_ERROR    error;
    DB_DISTRIB	qef_s1_distrib;		/* a DDB server operation if DB_DSTYES,
					** backend server otherwise */
    i4		qef_qescb_offset;	/* offset from SCB of qescb (used
					** to get session's QEF_CB in
					** qef_session().
					*/

    BITFLD      qef_nodep_chk:1;        /* used for the config parameter
                                        **  which switches ON/OFF the fix 
                                        ** to b112978 */
};

/*}
** Name: QEF_ART - Audit Range Table.
**
** Description:
**      This control block is used to access the audit range table
**      built by PSF and attached to the QP.
**
** History:
**     20-jul-89 (jennifer)
**          written
**	5-jul-93  (robf)
**	    Changed tabtype to use QEF definitions instead  of DMF ones,
**	    also store whether its a system catalog or not.
**	    Added security label for object.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
[@history_line@]...
*/
typedef struct _QEF_ART
{
    /* standard stuff */
    struct _QEF_ART     *qef_next;              /* The next control block */
    struct _QEF_ART     *qef_prev;              /* The previous one */
    SIZE_TYPE   qef_length;             /* size of the control block */
    i2          qef_type;               /* type of control block */
#define QEFART_CB    200
    i2          qef_s_reserved;
    PTR         qef_l_reserved;
    PTR         qef_owner;
    i4          qef_ascii_id;           /* to view in a dump */
#define QEFART_ASCII_ID  CV_C_CONST_MACRO('Q', 'A', 'R', 'T')
    DB_TAB_ID       qef_tabid;          /* Table id of table */
    i4              qef_tabtype;        /* Table type, base or view. */
# define QEFART_TABLE	0x01
# define QEFART_VIEW	0x02
# define QEFART_SYSCAT  0x04
# define QEFART_SECURE  0x08
    DB_TAB_NAME	    qef_tabname;	/* Name of corresponding table */
    DB_OWN_NAME	    qef_ownname;	/* Owner of table */
    i4              qef_mode;           /* access mode of query*/
} QEF_ART;

/*}
** Name: QEF_APR - Audit Permit List.
**
** Description:
**      This control block is used to access the audit permit list
**      built by PSF and attached to the QP.
**
** History:
**     20-jul-89 (jennifer)
**          written
**	08-aug-90 (ralph)
**	    Changed qef_apr.qef_popset from i2 to i4.
**	    Added qef_apr.qef_popctl.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	24-nov-93 (robf)
**	    Added qef_flags, qef_evname, qef_evowner, qef_ev_l_text,
**	         qef_evtext fields for Secure 2.0.
[@history_line@]...
*/
typedef struct _QEF_APR
{
    /* standard stuff */
    struct _QEF_APR *qef_next;              /* The next control block */
    struct _QEF_APR *qef_prev;              /* The previous one */
    SIZE_TYPE   qef_length;             /* size of the control block */
    i2          qef_type;               /* type of control block */
#define QEFAPR_CB    201
    i2          qef_s_reserved;
    PTR         qef_l_reserved;
    PTR         qef_owner;
    i4          qef_ascii_id;           /* to view in a dump */
#define QEFAPR_ASCII_ID  CV_C_CONST_MACRO('Q', 'A', 'P', 'R')
    DB_TAB_ID       qef_tabid;          /* Table id of table */
    DB_OWN_NAME	    qef_user;		/* Name of user getting permission */
    i4              qef_mode;           /* access mode of query */
    i4		    qef_popctl;		/* Bit map of defined   operations */
    i4		    qef_popset;		/* Bit map of permitted operations */
					/* See dbms.h for settings, for 
                                        ** example DB_EXECUTE. */
    i2		    qef_gtype;		/* Grantee type:	    */
#define		    QEFA_DEFAULT    -1	/*  Installation default    */
#define		    QEFA_USER	     0	/*  User		    */
#define		    QEFA_GROUP	     1	/*  Group		    */
#define		    QEFA_APLID	     2	/*  Application identifier  */
#define		    QEFA_PUBLIC	     3	/*  Public		    */
     i4	    qef_flags;		/* Various flags */
# define	    QEFA_F_EVENT      0x01	/* Alarm has associate event */
     DB_EVENT_NAME  qef_evname;		/* Event name */
     DB_OWN_NAME    qef_evowner;	/* Event owner */
     i2		    qef_ev_l_text;	/* Length of qef_evtext */
     char	    qef_evtext[DB_EVDATA_MAX]; /* Event text */
} QEF_APR;

/*}
** Name: QEF_AUD - Audit control block.
**
** Description:
**      This control block is used to point to the QEF_ART and 
**      QEF_APR arrays.
**
** History:
**     20-jul-89 (jennifer)
**          written
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
[@history_line@]...
*/
typedef struct _QEF_AUD
{
    /* standard stuff */
    struct    _QEF_AUD *qef_next;              /* The next control block */
    struct    _QEF_AUD *qef_prev;              /* The previous one */
    SIZE_TYPE   qef_length;             /* size of the control block */
    i2          qef_type;               /* type of control block */
#define QEFAUD_CB    202
    i2          qef_s_reserved;
    PTR         qef_l_reserved;
    PTR         qef_owner;
    i4          qef_ascii_id;           /* to view in a dump */
#define QEFAUD_ASCII_ID  CV_C_CONST_MACRO('Q', 'A', 'U', 'D')
    QEF_ART         *qef_art;           /* Pointer to QEF_ART */
    i4              qef_cart;           /* Count of entries in qef_art. */
    QEF_APR	    *qef_apr;		/* Pointer to QEF_APR */
    i4              qef_capr;           /* Count of entries in qef_apr. */
} QEF_AUD;

/*}
** Name: QEF_HASH_DUP	- hash table duplicate chain header
**
** Description:
**	describes structure representing chain of rows assigned to the 
**	same hash bucket
**
** History:
[@history_line@]...
**	26-jan-01 (inkdo01)
**	    Written.
**	01-oct-2001 (devjo01)
**	    Renamed QEN_HASH_DUP to QEF_HASH_DUP and moved it from qefdsh.h 
**	    to qefmain.h so that it will be visible to OPF. 
**	6-May-2005 (schka24)
**	    Hash values must be unsigned.
**	7-May-2008 (kschendel) SIR 122512
**	    Add same-as-next flag.
**	    Move OJ indicator into this header.
**	    Add a length and a compressed-row flag to this header.
*/

typedef struct _QEF_HASH_DUP
{
    u_i4	hash_number;	/* original computed hash number */

    /* Since every row has this header, space is important.  Fortunately
    ** modern compilers on modern platforms manage to generate decent
    ** code for bit-fields.  At present the bitfields total 32 bits,
    ** i.e. another u_i4's worth.
    */
    BITFLD	hash_same_key:1;  /* If 1, this row has the same join key
				** as the next row in the hash chain.  Not
				** meaningful unless the row is in the hash
				** table, for hash joins only.
				*/
    BITFLD	hash_oj_ind:1;	/* For outer join only: 0 if row hasn't been
				** used for an inner join, 1 if it has. Rows
				** with 0 indicator are OJ candidates.
				*/
    BITFLD	hash_null_key:1;  /* *NOTUSED* but save for future extension
				** of null-join to hash joins.
				*/
    BITFLD	hash_compressed:1;  /* If 1, this row has been rle-compressed.
				** (RLE compression affects non-key columns
				** only, keys remain uncompressed.  Fortunately
				** opc arranges to put all the keys first in
				** the row.)
				*/
    BITFLD	hash_filler:12;	/* Notused, available */
    BITFLD	hash_row_length:16;  /* The length of the row, divided by 8.
				** Includes this header.  Avoid using directly,
				** use get/set macros defined below.  The div
				** by 8 allows row lengths up to 512K to be
				** held in 16 bits.
				** Note: the length could expand a few bits
				** into the filler instead of the div by 8,
				** but it's probably a performance wash:
				** i2 load and shift vs i4 load and mask.
				** Doing it this way preserves filler for
				** future needs.
				*/

    struct _QEF_HASH_DUP   *hash_next;	/* ptr to next row on chain */

    /* *** Important ***  *** Readme ***
    ** You MUST arrange to align hash_key at a 4-byte boundary, or better.
    ** The simplest way to do this is to arrange for an i4 or a pointer
    ** to precede the hash key.  (pointers are >=32 bits on all current
    ** architectures.)  This does not explicitly force alignment, but
    ** will in fact produce alignment on all rational platforms.
    ** qen_hash_value will puke on alignment-enforcing architectures
    ** if the hash key is not properly aligned.
    */
    char	hash_key[1];	/* 1st byte of hash key (in row buffer) */
}	QEF_HASH_DUP;

/* We frequently need the size of the header, which does NOT include
** the first hash key.  Define a macro to do this properly.
** (This is essentially CL_OFFSETOF although done a little differently.)
*/
#define QEF_HASH_DUP_SIZE ( &(((QEF_HASH_DUP *)0)->hash_key[0]) - (char*)0 )

/* DO NOT hardcode the divide-by-8 of the length into code, instead use
** these macros which get fed a QEF_HASH_DUP *.
** Typical usages:
** length_in_bytes = QEFH_GET_ROW_LENGTH_MACRO(rowptr);
** QEFH_SET_ROW_LENGTH_MACRO(rowptr,length_in_bytes);
*/
#define QEFH_GET_ROW_LENGTH_MACRO(rowptr) ((u_i4)((rowptr)->hash_row_length) << 3)
#define QEFH_SET_ROW_LENGTH_MACRO(rowptr,length) (rowptr)->hash_row_length = ((u_i4)(length))>>3
#define QEF_HASH_DUP_ALIGN 8	/* For opc to force alignment */
