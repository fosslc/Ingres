/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: RQF.H - RQF data strcutures.
**
** Description:
**      RQF request control block and its substructural definitions.
**	RQF internal structures are deliberately hidden. See rqfprvt.h
**	for their definitions.
**
** History: $Log-for RCS$
**      may-25-1988 (alexh)
**          created.
**	jun-30-1988 (alexh)
**	    added tuple fetching interface. removed return format fields.
**	aug-2-1988 (alexh)
**	    spinned off the rqfprvt.h. This hides RQF internal data
**	    structures from RQF interface.
**	oct-11-1988 (alexh)
**	    add RQR_QUERY, RQR_GET, RQR_ENDRET interfaces
**	oct-20-1988 (alexh)
**	    added new table transfer and binary query data interface
**	jan-13-1989 (georgeg)
**	    added new field rqr_effect_user in RQR_CB.
**	jan-19-1989 (georgeg)
**	    added E_RQ0041 for effective user support (-u flag).
**	feb-02-1989 (georgeg)
**	    added RQR_SET_FUNC.
**	feb-13-1989 (georgeg)
**	    added constant RQF_MAX_ASSOC, Max number of associations per session
**	    added RQR_OPEN for open cursor
**	feb-19-1989 (georgeg)
**	    added E_RQ00043_CURSOR_UPDATE_FAILED
**	mar-26-1989 (georgeg)
**	    added E_RQ00044_CURSOR_OPEN_FAILED
**	14-jul-1990 (georgeg)
**		added RQR_CB.rqr_cluster_p, RQR_CB.rqr_nodes, RQR_CLUSTER_INFO.
**	14-dec-92 (teresa)
**		added rqr_dbp_param and rqr_dbp_rstat
**	03-feb-1993 (fpang)
**		Added RQR_CB.rqr_2pc_txn and RQR_04_XACT_MASK to support 
**		catching local commits/aborts when executing local database
**		procedures or executing direct execute immediate.
**	12-apr-1993 (fpang)
**	    Added RQR_CB.rqr_own_name to support owner.table and 
**	    owner.procedure.
**	22-jul-1993 (fpang)
**	    Prototyped rqf_call().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Added rqb_prec and rqb_r_prec to RQB_BIND.
**/


/*}
** Name: RQF_HEADER - RQF session control block header
**
** Description:
**	This is the standard facility interface control block header.
** History:
**      mar-3-88 (alexh)
**		created
**      09-nov-1989 (georgeg)
**		added RQR_CB.rqr_opcode, the request opcode to rqf_call()
**     05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
*/

typedef struct _RQF_HEADER
{
    /* generic struct prefix */
    struct _RQF_HEADER	*rqs_next;  /* next tpf control block */
    struct _RQF_HEADER	*rqs_prev;  /* previous control block */
    i4		rqs_length; /* length of control block */
    i2		rqs_type;   /* control block type */
#define	RQS_CB_TYPE 0x1956
    i2		rqs_s_reserved;
    PTR		rqs_l_reserved;
    PTR   	rqs_owner;
    i4		rqs_ascii_id;
}	RQF_HEADER;

/*}
** Name: RQB_BIND - RQF tuple column binding table
**
** Description:
**	RQB_BIND is used to specified the address and the size of
**	a return column. This specification must be done prior to
**	RQR_T_FETCH calls.
**
** History:
**      jun-30-88 (alexh)
**		created
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Added rqb_prec and rqb_r_prec to provide support for 
**          DB_DEC_TYPE in rqf_t_fetch().
*/
typedef  struct
{
  PTR	    rqb_addr;	    /* store address of a column */
  i4   rqb_length;	    /* expected length of column */
  DB_DT_ID  rqb_dt_id;	    /* expected data type of column */
  i2        rqb_prec;       /* expected precision of column */
  i4   rqb_r_length;   /* GCA length of column, for RQF's use */
  DB_DT_ID  rqb_r_dt_id;    /* GCA data type of column, for RQF's use */
  i2        rqb_r_prec;     /* GCA precision of column, for RQF's use */
}	RQB_BIND;

/*
** }
** Name: RQR_CB - RQ request parameter block.
**
** Description:
**	RQR_CB is used to store parameters of a RQF request.
**	RQR_CB must first be initialized by RQR_STARTUP request.
**
** History:
**      may-25-88 (alexh)
**		created
**      feb-21-89 (georgeg)
**		added *rqr_tabl_name, *rqr_mod_flag
**      07-nov-89 (georgeg)
**		added RQR_RESTART, rqr_cb.rqr_2pc_dx_id, rqr_cb.rqr_dx_status.
**      09-nov-1989 (georgeg)
**		added RQR_CB.rqr_opcode, the request opcode to rqf_call()
**	14-jul-1990 (georgeg)
**		added RQR_CB.rqr_cluster_p, RQR_CB.rqr_nodes.
**	13-aug-1991 (fpang)
**	        1. Added RQR_TRACE for rqf tracepoints.
**	        2. rqr_dbgcb_p to rqr_cb for server level debugging.
**		3. Constants to manage rqf trace points.
**	20-aug-1991 (fpang)
**		Added ';' after rqr_dbgcb_p.
**	08-sep-1992 (fpang)
**		Added E_RQ004A_LDB_ERROR_MSG and E_RQ004B_CHK_LCL_AUTHORIZATION.
**	03-feb-1993 (fpang)
**		Added RQR_CB.rqr_2pc_txn and RQR_04_XACT_MASK to support 
**		catching local commits/aborts when executing local database
**		procedures or executing direct execute immediate.
**	01-mar-93 (barbara)
**		Added rqr_own_name pointer to RQR_CB for DELETE cursor.
**		New message, GCA1_DELETE, takes ownername as well as tblname.
**      14-Oct-1993 (fred)
**              Added new field rqr_count to parallel qef_count.  This
**          	is used for handling large objects in determining the
**          	number of buffers (QEF_DATA) vs. the number of tuples.
**          	See QEF_RCB for details.
**	10-mar-94 (swm)
**		Bug #59883
**		Added rqr_trfmt and rqr_trsize to RQS_CB. This is to share
**		use of the QEF TRformat trace buffer. This buffer is used in
**		preference to an automatic buffer which could give rise to
**		stack overflow on deeply nested calls.
*/

/* RQF request code */
#define	RQR_STARTUP	1
#define	RQR_SHUTDOWN	2
#define	RQR_S_BEGIN	3
#define	RQR_S_END	4
#define RQR_READ        6
#define RQR_WRITE       7
#define RQR_INTERRUPT   8
#define RQR_NORMAL      9
#define RQR_XFR		10
#define RQR_BEGIN       11
#define RQR_COMMIT      12
#define RQR_SECURE      13
#define RQR_ABORT       14
#define RQR_DEFINE      15
#define RQR_EXEC        16
#define RQR_FETCH       17
#define RQR_DELETE      18
#define RQR_CLOSE       19
#define	RQR_T_FETCH	20
#define	RQR_T_FLUSH	21
#define	RQR_CONNECT	23
#define	RQR_DISCONNECT	24
#define	RQR_CONTINUE	25
#define	RQR_QUERY	26
#define	RQR_GET		27
#define	RQR_ENDRET	28
#define RQR_DATA_VALUES	29
#define RQR_SET_FUNC	30
#define RQR_UPDATE	31
#define RQR_OPEN	32
#define RQR_MODIFY	33
#define RQR_CLEANUP	34
#define RQR_TERM_ASSOC	35
#define RQR_LDB_ARCH	36
#define RQR_RESTART	37
#define RQR_CLUSTER_INFO 38
#define RQR_TRACE	39
#define RQR_INVPROC	40


typedef struct _RQR_CB
{
    RQF_HEADER	rqr_header;	    /* standard control block header */
    PTR		rqr_session;	    /* session control block - RQS_CB */
#define	RQRCB_ASCII_ID	CV_CONST_MACRO('R', 'Q', 'R', 'B')

    				    /* INPUT parameters */

    i4		rqr_timeout;	    /* timeout length */
    DD_LDB_DESC	*rqr_1_site;	    /* database server id */
    DB_LANG	rqr_q_language;	    /* DB_NOLANG, DB_QUEL or DB_SQL */
    i4		rqr_col_count;	    /* user's expected column count */
    DD_USR_DESC	*rqr_user;	    /* requesting user id, NULL if none */
    DD_COM_FLAGS *rqr_com_flags;    /* FE command flags */
				    /* RQR_XFER parameters start here */
    PTR		rqr_xfr;	    /* transfer parameters. RQR_XFR */
    DD_NAME	*rqr_tmp;	    /* transfer temprorary names. RQR_XFR */
    DD_NAME	*rqr_tabl_name;	    /* the name of the table to send */
				    /*  to the LBD for the cursor delete */

    DD_NAME	*rqr_own_name;	    /* owner name for cursor/procedure 
				    ** delete */
    PTR		rqr_dc_blkp;	    /* direct connect control block pointer */
    bool	rqr_dc_txn;	    /* direct connect transaction mode */
    PTR		rqr_tupdata;	    /* QEF_DATA data description array */
    i4		rqr_tupsize;	    /* tuple size being fetched */
    i4		rqr_dv_cnt;	    /* number of binary query data */
    DB_DATA_VALUE *rqr_dv_p;	    /* address of DB_DATA_VALUE array */
    RQB_BIND	*rqr_bind_addrs;    /* array of return column addresses,*/
				    /* NULL if none */
    PTR         rqr_inv_parms;      /* repeat paramertes. QEF_PARMS */

    				    /* INPUT and OUTPUT parameters */

    DB_CURSOR_ID   rqr_qid;         /* query or cursor id - GCA_ID */
    bool	rqr_qid_first;	    /* TRUE if QID goes before other parameters,*/
				    /* FALSE if after; valid for OPEN CURSOR,*/
				    /* REPLACE CURSOR and DEFINE REPEAT QUERY */
    DD_PACKET   rqr_msg;	    /* input or output message struct */
    i4	rqr_tupcount;	    /* number of tuples rqquested/returned */
    PTR		rqr_tupdesc_p;	    /* ptr to tuple descriptor returned by LDB*/
				    /* for SCF's use */

				    /* OUTPUT parameters */

    i4	rqr_txn_id;         /* STAR transaction id */
    bool        rqr_end_of_data;    /* more data may come */
    PTR		rqr_gca_status;     /* != NULL indicates a LDB status */
    				    /* GCA_RE_DATA */
    DD_MODIFY	*rqr_mod_flags;	    /* modify flags. +/-U, +/-Y, -A */
    bool	rqr_ldb_abort;	    /* TRUE if LDB has aborted its transaction */
				    /* unilaterally, FALSE otherwise; set to */
				    /* FALSE by QEF and tested by QEF if the */
				    /* request fails */
    i4	rqr_ldb_status;     /* status returned by LDB */
#define	RQF_00_NULL_STAT    0x0000L 
#define RQF_01_FAIL_MASK    0x0001L /* general failure */
#define RQF_02_STMT_MASK    0x0002L /* LDB cannot process the SQL statment */
#define RQF_03_TERM_MASK    0x0004L /* LDB terminates the transacation due to
				    ** a COMMIT, ROLLBACK, abort or subquery */
#define RQF_04_XACT_MASK    0x0008L /* LDB attempted a xact statement in
				    ** 2PC. */
    DB_ERROR	rqr_error;	    /* error reported by RQF */
    bool	rqr_diff_arch;	    /* architecture id of assoc partner */
    bool	rqr_rx_timeout;	    /* read timeout on GCA_RECEIVE */
    bool	rqr_no_attr_names;  /* TRUE for internal query that requires
				    ** no attributes names for display, FALSE
				    ** for user SELECT query */
    i4		rqr_tds_size;	    /* size of Tuple Descriptor Space allocated 
				    ** by QEF */
    i4	rqr_1ldb_severity;  /* Severity if the error reported by LDB */
    i4	rqr_2ldb_error_id;  /* Error id reported by LDB */
    i4	rqr_3ldb_local_id;  /* Local error reported by LDB */
    DD_DX_ID	*rqr_2pc_dx_id;	    /* Transaction id for SECURE message */
    i4	rqr_dx_status;	    /* transaction accepted rejected */
#define RQF_0DONE_DX	0x0000L	    /* Received a GCA_DONE */
#define RQF_1REFUSE_DX	0x0001L	    /* Received a GCA_REFUSE */
#define RQF_2ERROR_DX	0x0003L	    /* some other error occured */
    i4		rqr_opcode;	    /* the request opcode to rqf_call() */
    bool	rqr_crttmp_ok;	    /* did rqf create the tmp transer table */
    				    /*  or dit it fail TRUE/FALSE */
    PTR		rqr_cluster_p;	    /* pointer to the cluster node names */
    i4	rqr_nodes;	    /* number of nodes in the cluster */
    PTR		rqr_1_anything;
    PTR		rqr_2_anything;
    DB_DEBUG_CB *rqr_dbgcb_p;       /* trace point control block */
#define RQF_MIN_TRACE_120 120
#define RQF_MAX_TRACE_128 128
    QEF_USR_PARAM **rqr_dbp_param;  /* address of Procedure parameters array */
    i4	rqr_dbp_rstat;	    /* return status from Registered Procedure*/
    bool    	rqr_2pc_txn;	    /* TRUE if DX is in 2PC. */
    i4     rqr_count;          /* Number of buffers used */
    char	*rqr_trfmt;	    /* pointer to QEF_CB TRformat buffer */
    i4		rqr_trsize;	    /* size of QEF_CB TRformat buffer */
}   RQR_CB;


/*}
**
** Function prototypes
**
** History :
**	22-jul-1993 (fpang)
**	    Created, and prototyped rqf_call().
*/

FUNC_EXTERN DB_STATUS rqf_call (
	i4         rqr_type,
	RQR_CB      *rqr_cb
);


/* RQF Error status */
#define	E_RQ0000_OK			E_DB_OK
#define	E_RQ_MASK			(0x0d0000L)
#define	E_RQ0001_WRONG_COLUMN_COUNT	(E_RQ_MASK+0x1L)
#define	E_RQ0002_NO_TUPLE_DESCRIPTION	(E_RQ_MASK+0x2L)
#define	E_RQ0003_TOO_MANY_COLUMNS	(E_RQ_MASK+0x3L)
#define	E_RQ0004_BIND_BUFFER_TOO_SMALL	(E_RQ_MASK+0x4L)
#define E_RQ0005_CONVERSION_FAILED	(E_RQ_MASK+0x5L)
#define E_RQ0006_CANNOT_GET_ASSOCIATION	(E_RQ_MASK+0x6L)
#define E_RQ0007_BAD_REQUEST_CODE	(E_RQ_MASK+0x7L)
#define E_RQ0008_SCU_MALLOC_FAILED	(E_RQ_MASK+0x8L)
#define E_RQ0009_ULM_STARTUP_FAILED	(E_RQ_MASK+0x9L)
#define E_RQ0010_ULM_OPEN_FAILED	(E_RQ_MASK+0x10L)
#define	E_RQ0011_INVALID_READ		(E_RQ_MASK+0x11L)
#define	E_RQ0012_INVALID_WRITE		(E_RQ_MASK+0x12L)
#define	E_RQ0013_ULM_CLOSE_FAILED	(E_RQ_MASK+0x13L)
#define	E_RQ0014_QUERY_ERROR		(E_RQ_MASK+0x14L)
#define	E_RQ0015_UNEXPECTED_MESSAGE	(E_RQ_MASK+0x15L)
#define	E_RQ0016_CONVERSION_ERROR	(E_RQ_MASK+0x16L)
#define	E_RQ0017_NO_ACK			(E_RQ_MASK+0x17L)
#define	E_RQ0018_SHUTDOWN_FAILED	(E_RQ_MASK+0x18L)
#define	E_RQ0019_COMMIT_FAILED		(E_RQ_MASK+0x19L)
#define	E_RQ0020_ABORT_FAILED		(E_RQ_MASK+0x20L)
#define	E_RQ0021_BEGIN_FAILED		(E_RQ_MASK+0x21L)
#define	E_RQ0022_END_FAILED		(E_RQ_MASK+0x22L)
#define	E_RQ0023_COPY_FROM_EXPECTED	(E_RQ_MASK+0x23L)
#define	E_RQ0024_COPY_DEST_FAILED	(E_RQ_MASK+0x24L)
#define	E_RQ0025_COPY_SOURCE_FAILED	(E_RQ_MASK+0x25L)
#define	E_RQ0026_QID_EXPECTED		(E_RQ_MASK+0x26L)
#define	E_RQ0027_CURSOR_CLOSE_FAILED	(E_RQ_MASK+0x27L)
#define	E_RQ0028_CURSOR_FETCH_FAILED	(E_RQ_MASK+0x28L)
#define	E_RQ0029_CURSOR_EXEC_FAILED	(E_RQ_MASK+0x29L)
#define	E_RQ0030_CURSOR_DELETE_FAILED	(E_RQ_MASK+0x30L)
#define	E_RQ0031_INVALID_CONTINUE	(E_RQ_MASK+0x31L)
#define	E_RQ0032_DIFFERENT_TUPLE_SIZE	(E_RQ_MASK+0x32L)
#define	E_RQ0033_FETCH_FAILED		(E_RQ_MASK+0x33L)
#define	E_RQ0034_COPY_CREATE_FAILED	(E_RQ_MASK+0x34L)
#define	E_RQ0035_BAD_COL_DESC_FORMAT	(E_RQ_MASK+0x35L)
#define	E_RQ0036_COL_DESC_EXPECTED	(E_RQ_MASK+0x36L)
#define	E_RQ0037_II_LDB_NOT_DEFINED	(E_RQ_MASK+0x37L)
#define	E_RQ0038_ULM_ALLOC_FAILED	(E_RQ_MASK+0x38L)
#define	E_RQ0039_INTERRUPTED		(E_RQ_MASK+0x39L)
#define	E_RQ0040_UNKNOWN_REPEAT_Q	(E_RQ_MASK+0x40L)
#define	E_RQ0041_ERROR_MSG_FROM_LDB	(E_RQ_MASK+0x41L)   /* this message is */
		/* used to display to the client the message that the	*/
		/* LBD sends when an association cannot be modified	*/
#define	E_RQ0042_LDB_ERROR_MSG		(E_RQ_MASK+0x42L)
#define	E_RQ0043_CURSOR_UPDATE_FAILED	(E_RQ_MASK+0x43L)
#define	E_RQ0044_CURSOR_OPEN_FAILED	(E_RQ_MASK+0x44L)
#define	E_RQ0045_CONNECTION_LOST	(E_RQ_MASK+0x45L)
#define E_RQ0046_RECV_TIMEOUT		(E_RQ_MASK+0x46L)
#define E_RQ0047_UNAUTHORIZED_USER	(E_RQ_MASK+0x47L)
#define E_RQ0048_SECURE_FAILED		(E_RQ_MASK+0x48L)
#define E_RQ0049_RESTART_FAILED		(E_RQ_MASK+0x49L)
#define E_RQ004A_LDB_ERROR_MSG          (E_RQ_MASK+0x4AL)
#define E_RQ004B_CHK_LCL_AUTHORIZATION  (E_RQ_MASK+0x4BL)
#define E_RQ004C_RQF_LO_MISMATCH        (E_RQ_MASK+0x4CL)
