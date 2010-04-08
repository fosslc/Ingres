/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: TPF.H - TPF external interface data strcutures.
**
** Description:
**      TPF request control block definition.
[@comment_line@]...
**
** History: $Log-for RCS$
**      mar-3-88 (alexh)
**          created.
**      may-20-89 (carl)
**	    added TPF_1T_DDL_CC and TPF_0F_DDL_CC for turning a session's
**	    DDL_CONCURRENCY mode ON and OFF
**      oct-20-89 (carl)
**	    1)  added E_TP0020_INTERNAL, internal TPF error code
**	    2)  added TPF_SUBQRY_MODE to validate the mode of a subquery prior
**		to its execution by QEF
**      oct-29-89 (carl)
**	    added new fields to TPR_CB for 2PC
**      jan-28-90 (carl)
**	    added TPF_P1_RECOVER and TPF_P2_RECOVER
**      apr-24-90 (carl)
**	    added TPF_TRACE
**      sep-01-90 (carl)
**	    1)	added TPF_MAX_OP
**	    2)	retired obsolete TPF op codes
**	    3)	fixed CB typo 
**	dec-12-90 (fpang)
**	    Changed declaration of tpr_1_prev_p, and tpr_2_next_p
**	    from 'struct _TPW_LDB' to TPR_W_LDB. 
**	aug-13-91 (fpang)
**	    Changed MAX_TRACE from 127 to 119. 120 to 127 were given to RQF
**	feb-03-93 (fpang)
**	    Added tpr_22_flags to support detecting commits/aborts when
**	    executing database procedures and executing direct execute
**	    immediate.
**	22-jul-1993 (fpang)
**	    Prototyped tpf_call().
**      14-sep-93 (smc)
**          Made tpr_19_qef_sess a CS_SID.
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2001 (jenjo02)
**	    Changed E_TP_ERROR_OFF which caused TPF errors be
**	    incorrectly interpreted as (bad) E_USer errors to the
**	    correct E_TPF_MASK. Moved all errors in
**	    back/tpf/hdr/tpferr.h to here.
**      12-Apr-2004 (stial01)
**          Define tpf_length as SIZE_TYPE.
**/




/*}
** Name: TPF_HEADER - TPF control block header
**
** Description:
**	This is the standard server facility control block header.
** History:
**      mar-3-88 (alexh)
**		created
**	05-nov-1993 (smc)
**      	Bug #58635
**      	Made l_reserved & owner elements PTR for consistency with
**		DM_OBJECT.
**	10-mar-94 (swm)
**		Bug #59883
**		Added tpr_trfmt and tpr_trsize to TPR_CB. This is to share
**		use of the QEF TRformat trace buffer. This buffer is used in
**		preference to an automatic buffer which could give rise to
**		stack overflow on deeply nested calls.
*/

typedef struct _TPF_HEADER
{
    /* generic struct prefix */
    struct _TPF_HEADER	*tpf_next;  /* next tpf control block */
    struct _TPF_HEADER	*tpf_prev;  /* previous control block */
    SIZE_TYPE	tpf_length; /* length of control block */
    i2		tpf_type;   /* control block type */
#define	TPF_CB_TYPE 0x1956
    i2		tpf_s_reserved;
    PTR		tpf_l_reserved;
    PTR		tpf_owner;
    i4		tpf_ascii_id;
}	TPF_HEADER;


/*}
** Name: TPR_W_LDB - TPF control structure for constructing a list of LDB ptrs
**
** Description:
**	This is used by QEF to pass a list of updating LDBs involved in a
**	query plan.
**
** History:
**      nov-07-89 (carl)
**	    created
*/

typedef struct _TPR_W_LDB       TPR_W_LDB;

struct _TPR_W_LDB
{
    TPR_W_LDB		*tpr_1_prev_p;	/* previous such CB, NULL if none */
    TPR_W_LDB		*tpr_2_next_p;	/* next such CB, NULL if none */
    DD_LDB_DESC		*tpr_3_ldb_p;	/* ptr to the updating LDB's 
					** descriptor */
}   ;


/*}
** Name: TPR_CB - TP request parameter block.
**
** Description:
**	TPR_CB is used to store parameters of a TP request.
**	TPR_CB must be initialized by TPF_STARTUP reqeust.
**
** History:
**      mar-03-88 (alexh)
**	    created
**      may-20-89 (carl)
**	    added TPF_1T_DDL_CC and TPF_0F_DDL_CC for turning a session's
**	    DDL_CONCURRENCY mode ON and OFF
**      oct-29-89 (carl)
**	    extended for 2PC
**	10-mar-94 (swm)
**	    Bug #59883
**	    Added tpr_trfmt and tpr_trsize to TPR_CB. This is to share
**	    use of the QEF TRformat trace buffer. This buffer is used in
**	    preference to an automatic buffer which could give rise to
**	    stack overflow on deeply nested calls.
*/

/* 
    ** begin obsolete op codes **

#define	TPF_S_DISCNNCT	9	** slave disconnect **
#define	TPF_C_DISCNNCT	10	** client disconnect **
#define	TPF_S_DONE	11      ** slave completed the request **
#define	TPF_S_REFUSED	12	** slave did not complete the request **
#define	TPF_END_DX	16	** QUEL end transaction **
#define	TPF_AUTO	19	** set SQL autocommit mode. **
#define	TPF_NO_AUTO	20	** set SQL non-autocommit mode **
#define	TPF_EOQ		21	** end of query event **

    ** end obsolete op codes **
*/

/* TPF request code */

#define	TPF_STARTUP	1
#define	TPF_SHUTDOWN	2
#define	TPF_INITIALIZE	3
#define	TPF_TERMINATE	4
#define	TPF_SET_DDBINFO 5	/* used by QEF to provide the current DDB
				** information to TPF when first available */
#define	TPF_RECOVER	6	/* kept until TPF_P1_RECOVER and TPF_P2_RECOVER
				** take over */
#define	TPF_COMMIT	7	/* SQL commit directive */
#define	TPF_ABORT	8	/* SQL and QUEL abort directive */
#define	TPF_READ_DML	13	/* register a read dml */
#define	TPF_WRITE_DML	14	/* register an update dml */
#define TPF_BEGIN_DX	15	/* QUEL begin transaction */
#define	TPF_SAVEPOINT	17	/* set QUEL save point */
#define	TPF_SP_ABORT	18	/* QUEL save point abort */
#define	TPF_1T_DDL_CC	22	/* set DDL concurrency mode ON */
#define	TPF_0F_DDL_CC	23	/* set DDL concurrency mode OFF */
#define	TPF_OK_W_LDBS	24	/* validate updating LDBs for 1PC/2PC 
				** compatibility prior to its execution 
				** (by QEF) */
#define	TPF_P1_RECOVER	25	/* phase 1 of server restart, visit every CDB 
				** to find orphan DXs that must be recovered 
				** for reporting back to SCF to follow up in
				** a second phase */
#define	TPF_P2_RECOVER	26	/* phase 2 of server restart, recover all the
				** orphan DXs found in all CDBs in iidbdb, the
				** current installation */
#define	TPF_IS_DDB_OPEN	27	/* is it ok to allow a beginning session to
				** proceed to access the DDB in question */
#define	TPF_TRACE	28	/* SET TRACE POINT QE0{80..99}, called by
				** qec_trace() */
#define	TPF_MAX_OP  TPF_TRACE	/* maximum op code */

#define	TPF_MIN_TRACE_60    60	/* minimum trace point */
#define	TPF_MAX_TRACE_119   119	/* maximum trace point */


typedef struct _TPR_CB
{
    TPF_HEADER	tpr_header; 	/* standard control block header */
    DB_ERROR	tpr_error;
    PTR		tpr_session;	/* session control block TPT_CB */
    PTR		tpr_rqf;	/* RQF session control block */

#define	TPRCB_ASCII_ID	CV_CONST_MACRO('T', 'P', 'R', 'B')

    DB_LANG	tpr_lang_type;	/* transaction type DB_QUEL, DB_SQL */
    DD_LDB_DESC	*tpr_site;
    DB_SP_NAME	*tpr_save_name;	/* save point name */
    bool	tpr_need_recovery;
				/* TRUE if recovery is necessary, FALSE
				** otherwise */
    i4		tpr_11_max_sess_cnt;
				/* maximum number of sessions (used for pool
				** allocation at server startup time) */
    i4		tpr_12_max_ldbcnt_per_sess;
				/* maximum number of LDBs per session (for
				** furture use) */
    i4		tpr_13_max_spcnt_per_sess;
				/* maximum number of savepoints per session 
				** (for future use) */
    bool	tpr_14_w_ldbs_ok;	
				/* output TRUE if the following list of
				** updating LDBs is accepted for the DX, 
				** FALSE otherwise implying that 
				** QEF cannot execute the query plan and an 
				** error should be returned to the user */
    TPR_W_LDB	*tpr_15_w_ldb_p;/* ptr to first of LDB ptr list */
    DD_DDB_DESC	*tpr_16_ddb_p;	/* usd for TPF_SET_DDBINFO and TPF_IS_DDB_OPEN 
				*/
    bool	tpr_17_ddb_open;/* output: TRUE if the DDB in question is open,
				** FALSE if the DDB is still being recovered */
    DB_DEBUG_CB	*tpr_18_dbgcb_p;/* trace point value CB */
    CS_SID	tpr_19_qef_sess;	
				/* QEF session CB for calling SCC_TRACE until
				** TPF becomes a bona fide facility */
    char	*tpr_trfmt;	/* pointer to QEF_CB TRformat buffer */
    i4		tpr_trsize;	/* size of QEF_CB TRformat buffer */
    PTR		tpr_20_unused;	
    i4	tpr_21_unused;	
    u_i4	tpr_22_flags;	/* Various status bits returned. */
# define TPR_00FLAGS_NIL	    0x00000000	    /* Not set */
# define TPR_01FLAGS_2PC	    0x00000001	    /* XACTN in is 2pc if set */    
}   TPR_CB;


/*}
**
** Function prototypes
**
** History :
**      22-jul-1993 (fpang)
**          Created, and prototyped tpf_call().
*/

FUNC_EXTERN DB_STATUS tpf_call  (
        i4      i_request,
        TPR_CB  *v_tpr_p
);


#define E_TPF_MASK  ((i4)	(DB_TPF_ID << 16))

#define	E_TP0000_OK			0L
#define	E_TP0001_INVALID_REQUEST	(E_TPF_MASK + 0x0001L)
#define	E_TP0002_SCF_MALLOC_FAILED	(E_TPF_MASK + 0x0002L)
#define	E_TP0003_SCF_MFREE_FAILED	(E_TPF_MASK + 0x0003L)
#define	E_TP0004_MULTI_SITE_WRITE	(E_TPF_MASK + 0x0004L)
#define	E_TP0005_UNKNOWN_STATE		(E_TPF_MASK + 0x0005L)
#define	E_TP0006_INVALID_EVENT		(E_TPF_MASK + 0x0006L)
#define	E_TP0007_INVALID_TRANSITION	(E_TPF_MASK + 0x0007L)
#define	E_TP0008_TXN_BEGIN_FAILED	(E_TPF_MASK + 0x0008L)
#define	E_TP0009_TXN_FAILED		(E_TPF_MASK + 0x0009L)
#define	E_TP0010_SAVEPOINT_FAILED	(E_TPF_MASK + 0x0010L)
#define	E_TP0011_SP_ABORT_FAILED	(E_TPF_MASK + 0x0011L)
#define	E_TP0012_SP_NOT_EXIST		(E_TPF_MASK + 0x0012L)
#define	E_TP0013_AUTO_ON_NO_SP		(E_TPF_MASK + 0x0013L)
#define	E_TP0014_ULM_STARTUP_FAILED	(E_TPF_MASK + 0x0014L)
#define	E_TP0015_ULM_OPEN_FAILED	(E_TPF_MASK + 0x0015L)
#define	E_TP0016_ULM_ALLOC_FAILED	(E_TPF_MASK + 0x0016L)
#define	E_TP0017_ULM_CLOSE_FAILED	(E_TPF_MASK + 0x0017L)
#define	E_TP0020_INTERNAL		(E_TPF_MASK + 0x0020L)
#define	E_TP0021_LOGDX_FAILED		(E_TPF_MASK + 0x0021L)
#define	E_TP0022_SECURE_FAILED		(E_TPF_MASK + 0x0022L)
#define	E_TP0023_COMMIT_FAILED		(E_TPF_MASK + 0x0023L)
#define E_TP0024_DDB_IN_RECOVERY	(E_TPF_MASK + 0x0024L)
					/* DDB is being recovered and cannot
					** be accessed (not used) */

#define	I_TP0200_BEGIN_ROUND_REC	(E_TPF_MASK + 0x0200L)
#define	I_TP0201_END_ROUND_REC		(E_TPF_MASK + 0x0201L)
#define	E_TP0202_FAIL_FATAL_REC		(E_TPF_MASK + 0x0202L)
#define	E_TP0203_FAIL_REC		(E_TPF_MASK + 0x0203L)
#define	E_TP0204_BEGIN_DDB_REC		(E_TPF_MASK + 0x0204L)
#define	I_TP0205_DDB_FAIL_REC		(E_TPF_MASK + 0x0205L)
#define	I_TP0206_DDB_OK_REC		(E_TPF_MASK + 0x0206L)
#define	I_TP0207_BEGIN_CDB_REC		(E_TPF_MASK + 0x0207L)
#define	E_TP0208_FAIL_READ_CDBS		(E_TPF_MASK + 0x0208L)
#define	E_TP0209_FAIL_ALLOC_CDBS	(E_TPF_MASK + 0x0209L)
#define	I_TP020A_END_CDB_REC		(E_TPF_MASK + 0x020AL)
#define	E_TP020B_FAIL_CDB_REC		(E_TPF_MASK + 0x020BL)
#define	E_TP020C_ORPHAN_DX_FOUND	(E_TPF_MASK + 0x020CL)
#define	I_TP020D_NO_NEEDED_REC		(E_TPF_MASK + 0x020DL)
#define	I_TP020E_BEGIN_DX_LIST		(E_TPF_MASK + 0x020EL)
#define	I_TP020F_DX_LIST_EMPTY		(E_TPF_MASK + 0x020FL)
#define	I_TP0210_END_DX_LIST		(E_TPF_MASK + 0x0210L)
#define	E_TP0211_FAIL_END_DX_LIST	(E_TPF_MASK + 0x0211L)
#define	E_TP0212_FAIL_READ_DX		(E_TPF_MASK + 0x0212L)
#define	I_TP0213_CDB_NODE_NAME		(E_TPF_MASK + 0x0213L)
#define	E_TP0214_FAIL_END_LX_LIST	(E_TPF_MASK + 0x0214L)
#define	I_TP0215_BEGIN_LX_LIST		(E_TPF_MASK + 0x0215L)
#define	I_TP0216_RESTART_LDBS		(E_TPF_MASK + 0x0216L)
#define	E_TP0217_FAIL_RESTART_LDBS	(E_TPF_MASK + 0x0217L)
#define	I_TP0218_END_RESTART_LDBS	(E_TPF_MASK + 0x0218L)
#define	I_TP0219_END_RESTART_ALL_LDBS	(E_TPF_MASK + 0x0219L)
#define	E_TP021A_LDB_TERM		(E_TPF_MASK + 0x021AL)
#define	E_TP021B_DX_SECURE		(E_TPF_MASK + 0x021BL)
#define	E_TP021C_FAIL_DX_ABORT		(E_TPF_MASK + 0x021CL)
#define	I_TP021D_END_DX_ABORT		(E_TPF_MASK + 0x021DL)
#define	E_TP021E_FAIL_DX_COMMIT		(E_TPF_MASK + 0x021EL)
#define	I_TP021F_END_DX_COMMIT		(E_TPF_MASK + 0x021FL)
#define	E_TP0220_DX_STATE_COMMIT	(E_TPF_MASK + 0x0220L)
#define	E_TP0221_DX_STATE_ABORT		(E_TPF_MASK + 0x0221L)
#define	E_TP0222_DX_STATE_UNKNOWN	(E_TPF_MASK + 0x0222L)
#define	I_TP0223_DX_STATE_REC		(E_TPF_MASK + 0x0223L)
#define	E_TP0224_FAIL_DEL_DX		(E_TPF_MASK + 0x0224L)
#define	I_TP0225_END_DX_REC		(E_TPF_MASK + 0x0225L)
#define	E_TP0226_FAIL_DX_TERM		(E_TPF_MASK + 0x0226L)
#define	I_TP0227_COMMIT_UPDATE		(E_TPF_MASK + 0x0227L)
#define	E_TP0228_ABORT_UPDATE		(E_TPF_MASK + 0x0228L)
#define	I_TP0229_DX_OK			(E_TPF_MASK + 0x0229L)
#define	E_TP0230_DX_FAIL		(E_TPF_MASK + 0x0230L)
#define	I_TP0231_SKIP_PHASE1		(E_TPF_MASK + 0x0231L)
#define	I_TP0232_BEGIN_PHASE1		(E_TPF_MASK + 0x0232L)
#define	I_TP0233_NO_REC			(E_TPF_MASK + 0x0233L)
#define	E_TP0234_FAIL_REP_CDB		(E_TPF_MASK + 0x0234L)
#define	I_TP0235_END_PHASE1		(E_TPF_MASK + 0x0235L)
#define	I_TP0236_COMPLETE_PHASE1	(E_TPF_MASK + 0x0236L)
#define	E_TP0237_SKIP_PHASE2		(E_TPF_MASK + 0x0237L)
#define	I_TP0238_BEGIN_PHASE2		(E_TPF_MASK + 0x0238L)
#define	I_TP0239_END_PHASE2		(E_TPF_MASK + 0x0239L)
#define	I_TP023A_BEGIN_LDBS		(E_TPF_MASK + 0x023AL)
#define	E_TP023B_REFUSE_LDBS		(E_TPF_MASK + 0x023BL)
#define	I_TP023C_NODE_LDBS		(E_TPF_MASK + 0x023CL)
#define	E_TP023D_FAIL_LDBS		(E_TPF_MASK + 0x023DL)
#define	I_TP023E_END_LDBS		(E_TPF_MASK + 0x023EL)
#define	I_TP0240_COMPLETE_PHASE2	(E_TPF_MASK + 0x0240L)
#define	E_TP0241_FAIL_ACCESS_CDB	(E_TPF_MASK + 0x0241L)
#define	E_TP0242_FAIL_CATALOG_DDB	(E_TPF_MASK + 0x0242L)
#define	I_TP0243_BEGIN_SEARCH_DDB	(E_TPF_MASK + 0x0243L)
#define	I_TP0244_END_SEARCH_DDB		(E_TPF_MASK + 0x0244L)
#define	E_TP0245_FAIL_SEARCH_DDB	(E_TPF_MASK + 0x0245L)
#define	E_TP0246_REC_RQF_ERROR		(E_TPF_MASK + 0x0246L)
#define	I_TP0247_REC_QUERY_INFO		(E_TPF_MASK + 0x0247L)
#define	I_TP0248_REC_TARGET_LDB		(E_TPF_MASK + 0x0248L)
#define	I_TP0249_REC_QUERY_TEXT		(E_TPF_MASK + 0x0249L)
#define	E_TP024A_REC_NULL		(E_TPF_MASK + 0x024AL)
#define	I_TP024B_DDB_NAME		(E_TPF_MASK + 0x024BL)
#define	I_TP024C_DDB_OWNER		(E_TPF_MASK + 0x024CL)
#define	I_TP024D_CDB_NAME		(E_TPF_MASK + 0x024DL)
#define	I_TP024E_CDB_NODE		(E_TPF_MASK + 0x024EL)
#define	I_TP024F_CDB_DBMS		(E_TPF_MASK + 0x024FL)
#define	I_TP0250_NODE			(E_TPF_MASK + 0x0250L)
#define	I_TP0251_LDB			(E_TPF_MASK + 0x0251L)
#define	I_TP0252_DBMS			(E_TPF_MASK + 0x0252L)
#define	I_TP0253_LDB_ID			(E_TPF_MASK + 0x0253L)
#define	I_TP0254_CDB_ASSOC		(E_TPF_MASK + 0x0254L)

#define	E_TP_ERROR_MAX			(E_TPF_MASK + 0x0254L)
