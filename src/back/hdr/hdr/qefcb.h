/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: QEFCB.H - The QEF control block 
** 
**
** Description:
**      The QEF control block is used to initiate all communication
**  with the Query Execution Facility. All session specific information
**  is provided through this mechanism. It is assumed that anyone wanting
**  QEF information or service can gain access to the proper control block
**  for the session and pass it to QEF.
**
** History: $Log-for RCS$
**      19-mar-86 (daved)
**          written
**	11-jan-88 (puree)
**	    change QEF special savepoint name to lower case.
**	03-aug-88 (puree)
**	    add define for QEF_RO_MSTRAN.
**	24-aug-88 (puree)
**	    add define for QEF_RO_SSTRAN.
**	26-sep-88 (puree)
**	    modified QEF_CB to support QEF in-memory sorter.
**	23-dec-88 (carl)
**	    modified QES_QRY_SES to support Titan's hold-and-execute strategy
**	15-jan-89 (paul)
**	    add support for rules. qef_dsh is now the top of stack pointer
**	    for the DSH stack.
**	08-Mar-89 (ac)
**	    add 2PC support.
**	30-mar-89 (paul)
**	    Added session query cost limits for resource control.
**	    Also added qef_max_stack to allow maximum procedure nesting
**	    depth to be set dynamically.
**	07-apr-89 (carl)
**	    added define QES_4ST_REFRESH to flag a REGISTER WITH REFRESH command
**	12-apr-89 (ralph)
**	    Changed qef_fl_dbpriv to u_i4
**	22-may-89 (carl)
**	    added QES_RPT_HANDLE structure
**	23-may-89 (carl)
**	    added QES_03CTL_READING_TUPLES for QES_DDB_SES
**	05-jul-89 (carl)
**	    added qef_c3_streamid and qef_c4_streamsize for allocating 
**	    dedicated stream for a session's use
**	27-oct-89 (ralph)
**	    Added qef_user and qef_ustat to QEF_CB.
**	03-jun-90 (carl)
**	    added defines for STAR trace points qe50, qe51
**	04-aug-90 (carl)
**	    added:
**		1) define for STAR trace point qe55, 
**		2) define QES_04CTL_OK_TO_OUTPUT_QP to QES_DDB_SES to avoid 
**		   writing the current QP info to the log multiple times,
**		3) define QES_05CTL_SYSCAT_USER to QES_DDB_SES to indicate 
**		   that the session is a system user
**	21-aug-90 (carl)
**	    1)  added defines for STAR trace points:
**              qe40 for tracing every QEF operation,
**              qe50 for skipping destroying temporary tables at the end of
**              every DDB query,
**              qe51 for validating DDB QPs,
**              qe55 for tracing queries including their text,
**              qe56 for tracing DDL commands,
**              qe57 for tracing stream allocation and deallocation,
**              qe56 for tracing QEF calls,
**              qe59 for tracing queries in error situations
**	    2)  added memory operation defines:
**		QEF_MEM0_NIL, QEF_MEM1_AFT_OPEN, QEF_MEM2_AFT_CLOSE,
**		QEF_MEM3_AFT_ALLOC
**	05-oct-90 (davebf)
**	    Added qef_ostat, qef_tran_seq
**	25-feb-91 (rogerk)
**	    Added qef_stm_error to the qef_cb to specify transaction error
**	    handling actions for the session.  Added qef_savepoint to the
**	    qef_cb to hold user savepoint name for statement aborts.
**	    These were added as part of the SET SESSION WITH ON_ERROR changes.
**	    Added comments on the use of the Per-Statement Savepoint name.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added qef_user_interrupt, qef_user_interrupt_handled to qef_cb.
**	    Added QEF_CHECK_FOR_INTERRUPT macro.
**      26-jun-92 (anitap)
**          Added support for backward compatibility for update rowcount.
**      14-jul-92 (fpang)
**	    Fixed compiler warnings, QES_RPT_HANDLE.
**	17-mar-93 (rickh)
**	    Added field to qef_cb to describe case translation semantics.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added qef_dbxlate to QEF_CB, which points to case translation mask.
**	    Added qef_cat_own to QEF_CB, which points to catalog owner name.
**	2-aug-93 (rickh)
**	    Removed 17-mar-93 case translation field, which was superceded by
**	    7-apr-93 field.  Replaced the removed field with
**	    qef_callbackStatus.  This new field is used to preserve context
**	    over nested qef_calls.  For instance, during the creation of
**	    a CONSTRAINT, qef cooks up text (like "create index"), which
**	    psf parses and then callsback qef to execute.  It's useful to
**	    remember some simple facts across these callbacks.  Again for
**	    example, if we're in the middle of creating a UNIQUE CONSTRAINT
**	    and qef has called itself back to cook up an index, it's useful
**	    for the nexted index-creation to know that it's operating in
**	    the context of a "create constraint" so it can handle errors
**	    differently.  Simple.
**      10-sep-93 (smc)
**          Made qef_ses_id a CS_SID.
**	9-aug-93 (rickh)
**	    Added another flag to qef_callbackStatus.  This flags whether
**	    a SELECT ANY( 1 ) statement returned 1 or 0.  The REFERENTIAL
**	    CONSTRAINT state machine issues a SELECT statement verifying
**	    whether the constraint holds.  This new flag allows that SELECT
**	    to tell the next REFERENTIAL state whether the SELECT succeeded.
**	    If it didn't, we roll back the ADD CONSTRAINT DDL and don't bother
**	    cooking up all the tiresome objects which support a REFERENTIAL
**	    CONSTRAINT.
**	25-aug-93 (anitap)
**	    Added another flag to qef_callbackStatus in QEF_CB. This flags 
**	    whether a SELECT ANY(1) statement has to be executed by QEF after 
**	    the query text for the statement has gone through execute immediate
**	    parsing.
**	20-jan-94 (rickh)
**	    Added field to QEF_CB to hold the spill threshold for QEN hold
**	    files.  So far, this is only used for testing the spill logic.
**	14-mar-94 (swm)
**	    Bug #59883
**	    Added qef_trfmt buffer pointer and qef_trsize, QEF_TRFMT_SIZE
**	    definitions to QEF_CB. This is to enable the allocation of a
**	    TRformat buffer per QEF session rather than relying on automatic
**	    (TRformat) buffer allocation on the stack to eliminate the
**	    probability of stack overrun owing to large buffers.
**	 13-aug-2002(stial01 for angusm)
**	    Add trace point qe40 (QEF_XAFABORT) for XA transaction force-abort.
**      28-feb-2003 (stial01)
**          Added qef-ulm memory diagnostics (set trace point qe41 #)
**	8-apr-04 (inkdo01)
**	    Removed QEF_CHECK_FOR_INTERRUPT macro (moved to qef!hdr!qefdsh.h).
**      12-Apr-2004 (stial01)
**          Define qef_length as SIZE_TYPE.
**      28-dec-2004 (huazh01)
**          Add trace point qe61, which allows user to modify table/index
**          with constraint dependency.
**          INGSRV3004, b112978.
**      10-Jul-2006 (hanal04) Bug 116358
**          Added QEF_TRACE_IMMEDIATE_ULM_POOLMAP option to trace point qe41.
**	02-Jun-2009 (hanje04)
**	    Change qef_auto in QEF_CB to be i4 rather than bool, to match
**	    the same field in QEF_RCB. The miss-match causes problems on OSX
**	    because it stores values of 1 & 2
[@history_line@]...
**/

/*
**  Defines of other constants.
*/

/*
** Per Statement Savepoint
**
**    This savepoint name is used by QEF to partition the transaction work
**    associated with individual statements within a Multi-Statement 
**    Transaction.  These internal savepoints are used by the system to 
**    allow individual statements to be rolled-back upon statement errors
**    while preserving the rest of the work done by the transaction.
**
**    Note that the QET transaction routines will ignore requests to mark
**    or abort to this savepoint name when not running in the statement error
**    mode of "ON_ERROR ROLLBACK STATEMENT".
*/
#define QEF_SP_SAVEPOINT    "$qef_pssp" /* special savepoint name for QEF */
#define QEF_PS_SZ   9

/* return FALSE if no transaction, else true */
#define QEF_X_MACRO(qef_cb) (((QEF_CB*)qef_cb)->qef_stat == QEF_NOTRAN ? 0 : 1)

#define	QEF_MEM0_NIL		(i4) 0	    /* used by QECMEM.C */
#define	QEF_MEM1_AFT_OPEN	(i4) 1	    /* used by QECMEM.C */
#define	QEF_MEM2_AFT_CLOSE	(i4) 2	    /* used by QECMEM.C */
#define	QEF_MEM3_AFT_ALLOC	(i4) 3	    /* used by QECMEM.C */


/*}
** Name: QES_CONN_SES - CONNECT (local session) control block.
**
** Description:
**      This data structure is used to store complete control information
**  for direct connection with an LDB, i.e., for a established local session.
**
** History:
**      10-may-88 (carl)
**          written
*/

typedef struct _QES_CONN_SES
{
    PTR		    qes_c1_rqs_p;	    /* RQF's CONNECT session id */
    QED_CONN_INFO   qes_c2_con_info;	    /* CONNECT information */
}   QES_CONN_SES;


/*}
** Name: QES_RPT_HANDLE - handle to a repeat query's DSH
**
** Description:
**      This data structure is used to save the handle to a REPEAT QUERY's
**  DSH for reinitialization when the session terminates.
**
** History:
**	22-may-89 (carl)
**          written
**      14-jul-92 (fpang)
**          Removed typedef, it's forward declared above.
*/

typedef struct _QES_RPT_HANDLE	QES_RPT_HANDLE;

struct _QES_RPT_HANDLE
{
    QES_RPT_HANDLE  *qes_1_prev;	    /* ptr to predecessor in list */
    QES_RPT_HANDLE  *qes_2_next;	    /* ptr to successor in list */
    PTR		    qes_3_qso_handle;	    /* handle to the REPEAT QUERY's
					    ** DSH */
}   ;


/*}
** Name: QES_QRY_SES - Control block for QEQ_QUERY, QEQ_GET and QEQ_ENDRET.
**
** Description:
**      This data structure is used to store the current control information of 
**  a DDB query plan.
**
** History:
**      12-oct-88 (carl)
**          written
**	23-dec-88 (carl)
**	    modified to support Titan's hold-and-execute strategy
**	9-sep-98 (stephenb)
**	    Add QES_05Q_DSH to indicate that we already have the dsh from the
**	    last pass.
*/

typedef struct _QES_QRY_SES
{
    i4	     qes_q1_qry_status;	    /* status of query plan execution */

#define	    QES_00Q_NIL		0x0000L	    /* initial state */
#define	    QES_01Q_READ	0x0001L	    /* set if current subquery is 
					    ** a retrieval */
#define	    QES_02Q_EOD		0x0002L	    /* set if there is no more data to
					    ** read for current/previous read
					    ** subquery */
#define	    QES_03Q_PHASE2	0x0004L	    /* set if execution is in phase 2,
					    ** i.e., one or more LDBs have been
					    ** updated */
#define	    QES_04Q_HOLD	0x0008L	    
					    /* set if query execution is still 
					    ** in progress but SCF is owed a
					    ** return of:
					    **	
					    **	status = E_DB_WARN and
					    **	err_code = E_QE0015_NO_MORE_ROWS
					    **
					    ** as soon as execution comes 
					    ** to: 1) completion when temporary 
					    ** tables are dropped or 2) a 
					    ** juncture when the next read
					    ** subquery is executed if the
					    ** current query plan consists 
					    ** of multiple read subqueries */
#define    QES_05Q_DSH		0x0010L     /* set if we already have the DSH
					    ** from the last pass. This prevents
					    ** QEF from asking QSF for the QP
					    ** again, which increments the 
					    ** lock count of the QP object a
					    ** second time and leads to un-freed
					    ** QSF objects */

    DB_LANG	     qes_q2_lang;	    /* language of current subquery */
    DD_LDB_DESC	    *qes_q3_ldb_p;	    /* LDB executing current subquery */
    i4	     qes_q4_hold_cnt;	    /* # output rows on hold from SCF,
					    ** valid if QES_03Q_HOLD is set */
    i4	     qes_q5_ldb_rowcnt;	    /* # output rows from LDB, to be
					    ** returned to SCF at completion */
}   QES_QRY_SES;


/*}
** Name: QES_DDB_SES - DDB session control block
**
** Description:
**      This data structure is contained in QEF_CB and is used to supply 
**  session-specific control information for the current DDB session.
**
** History:
**      10-may-88 (carl)
**          written
**	07-apr-89 (carl)
**	    added define QES_4ST_REFRESH to flag a REGISTER WITH REFRESH command
**	23-may-89 (carl)
**	    added QES_03CTL_READING_TUPLES to indicate a SELECT is going on
**	04-aug-90 (carl)
**	    added defines: 
**		1) QES_04CTL_OK_TO_OUTPUT_QP to avoid writing QP info to the 
**		   log multiple times
**		2) QES_05CTL_SYSCAT_USER to indicate that the session can
**		   access system catalogs
*/

typedef struct _QES_DDB_SES
{
    bool	    qes_d1_ok_b;	/* TRUE if initialized, FALSE otherwise
					*/
    PTR		    qes_d2_tps_p;	/* generic ptr to associated TPF
					** session cb, initialized by TPF
					** when QEF is called to initialize
					** a session, must be used to call
					** TPF */
    PTR		    qes_d3_rqs_p;	/* generic ptr to associated RQF
					** session cb, initialized by RQF
					** when QEF is called to initialize
					** a session, must be used to call
					** RQF */
    DD_DDB_DESC	    *qes_d4_ddb_p;      /* DDB descriptor */
    DD_USR_DESC	    qes_d5_usr_desc;	/* session user descriptor */
    i4		    qes_d6_rdf_state;	/* used to track RDF state:
					** 0ST and 1ST  */
    i4		    qes_d7_ses_state;	/* tracks session state: 0ST, 1ST, 
					** 2ST */

#define	QES_0ST_NONE	    0		/* no state */
#define	QES_1ST_READ	    1		/* READ state */
#define	QES_2ST_CONNECT	    2		/* CONNECT state */
#define	QES_3ST_EXCHANGE    3		/* EXCHANGE state */
#define	QES_4ST_REFRESH	    4		/* REFRESH state */

    union
    {
	QES_CONN_SES    u1_con_ses;	/* control information for direct
					** connection, valid if above 
					** is QES_2ST_CONNECT */
	QES_QRY_SES	u2_qry_ses;	/* control information for QEQ_QUERY,
					** and QEQ_ENDRETR, valid if
					** above state is QES_1ST_READ */
    }		    qes_d8_union;
    i4	    qes_d9_ctl_info;	/* session control information */

#define	QES_00CTL_NIL_INFO		    0x0000L	

					/* place holder */

#define QES_01CTL_DDL_CONCURRENCY_ON	0x0001L	

					/* ON if CATALOG CONCURRENCY is desired,
					** i.e., the privileged CDB association
					** to be committed at the end of each 
					** user query, OFF otherwise */

#define QES_02CTL_COMMIT_CDB_TRAFFIC	0x0002L	

					/* ON if there has been traffic to
					** the CDB via the privileged 
					** association to be committed 
					** in accordance with the above 
					** condition), OFF otherwise */

#define QES_03CTL_READING_TUPLES	0x0004L	

					/* ON if a SELECT if still going on,
					** OFF otherwise implying ok to commit
					** if DDL_CONCURRENCY is ON; set by
					** QEQ_QUERY, tested and reset by
					** QEF_CALL if qeq_query returns
					** a status */

#define QES_04CTL_OK_TO_OUTPUT_QP	0x0008L	

					/* ON if current query plan NOT
					** written to the log, i.e., it is OK
					** for the display routines to output
					** the information without incurring
					** redundancy; OFF otherwise meaning
					** that it is UNNECESSARY to write
					** the QP information a second time;
					** set by qef_call every time it is
					** called, reset and tested against
					** by the top display routine 
					** qeq_p90_prt_qp */

#define QES_05CTL_SYSCAT_USER	    0x0010L	

					/* ON if the session is system catalog
					** user, OFF otherwise */

    i4	    qes_d10_timeout;	/* default session timeout ticks for 
					** each subquery */
    i4	    qes_d11_handle_cnt;	/* number of REPEAT QUERY handles */
    QES_RPT_HANDLE  *qes_d12_handle_p;	/* ptr handle list */
}   QES_DDB_SES;


/*}
** Name: QEF_CB - The QEF session control block
**
** Description:
**	This control block is used internally by QEF to keep information
**	specific to a session.  This control block is required as an input
**	to certain QEF functions listed below.  These functions are reserved 
**	to be called by SCF only (since only SCF knows how to get the 
**	session's  qef_cb).
**
**		qec_begin_session
**		qec_end_session
**		qec_alter
**		qec_info
**		all qet_xxxx
**
** History:
**      19-mar-86 (daved)
**          written
**	13-may-88 (puree)
**	    remove qef_spstat, qef_cursor.
**	05-aug-88 (puree)
**	    add define for QEF_RO_MSTRAN.
**	24-aug-88 (puree)
**	    add define for QEF_RO_SSTRAN.
**	26-sep-88 (puree)
**	    add qef_s_max and qef_s_used for QEF in-memory sorter.
**	08-Mar-89 (ac)
**	    add 2PC support.
**	30-mar-89 (paul)
**	    Added session query cost limits for resource control.
**	    Also added qef_max_stack to allow procedure nesting limit to be
**	    set dynamically.
**	12-apr-89 (ralph)
**	    Changed qef_fl_dbpriv to u_i4
**	05-jul-89 (carl)
**	    added qef_c3_streamid and qef_c4_streamsize for allocating 
**	    dedicated stream for a session's use
**	27-oct-89 (ralph)
**	    Added qef_user and qef_ustat to QEF_CB.
**	03-jun-90 (carl)
**	    added defines for STAR trace points qe50, qe51
**	04-aug-90 (carl)
**	    added:
**		1) define for STAR trace point qe55, 
**		2) define QES_04CTL_OK_TO_OUTPUT_QP to QES_DDB_SES to avoid 
**		   writing the current QP info to the log multiple times,
**		3) define QES_05CTL_SYSCAT_USER to QES_DDB_SES to indicate 
**		   that the session is a system user
**	21-aug-90 (carl)
**	    added defines for STAR trace points:
**              qe50 for skipping destroying temporary tables at the end of
**              every DDB query,
**              qe51 for validating every DDB QP,
**              qe55 for tracing every DDB query plan,
**
**              qe51 for validating DDB QPs,
**              qe55 for tracing queries including their text,
**              qe56 for tracing DDL commands,
**              qe57 for tracing stream allocation and deallocation,
**              qe56 for tracing QEF calls,
**              qe59 for tracing queries in error situations
**	05-oct-90 (davebf)
**	    Added qef_ostat, qef_tran_seq
**	25-feb-91 (rogerk)
**	    Added qef_stm_error field to specify transaction error handling
**	    actions.  If QEF_STMT_ROLLBACK then abort statement on error:
**	    if QEF_XACT_ROLLBACK then abort the entire transaction.  
**	    Added qef_savepoint to hold user defined savepoint name to which
**	    to abort if qef_stm_error is QEF_SVPT_ROLLBACK.  These were
**	    added as part of the SET SESSION WITH ON_ERROR changes.
**	30-jan-1992 (bonobo)
**	    Removed the redundant typedef to quiesce the ANSI C 
**	    compiler warnings.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added qef_user_interrupt, qef_user_interrupt_handled.
**      26-jun-92 (anitap)
**          Added support for backward compatibility for update rowcount. 5.0
**          gives the rowcount for update as the number of rows changed where
**          as in 6.0, it is the number of rows which qualified for the update.
**	03-feb-93 (andre)
**	    defined qef_dbid and qef_udbid
**	17-mar-93 (rickh)
**	    Added field to qef_cb to describe case translation semantics.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Added qef_dbxlate to QEF_CB, which points to case translation mask.
**	    Added qef_cat_own to QEF_CB, which points to catalog owner name.
**	2-aug-93 (rickh)
**	    Removed 17-mar-93 case translation field, which was superceded by
**	    7-apr-93 field.  Replaced the removed field with
**	    qef_callbackStatus.  This new field is used to preserve context
**	    over nested qef_calls.  For instance, during the creation of
**	    a CONSTRAINT, qef cooks up text (like "create index"), which
**	    psf parses and then callsback qef to execute.  It's useful to
**	    remember some simple facts across these callbacks.  Again for
**	    example, if we're in the middle of creating a UNIQUE CONSTRAINT
**	    and qef has called itself back to cook up an index, it's useful
**	    for the nexted index-creation to know that it's operating in
**	    the context of a "create constraint" so it can handle errors
**	    differently.  Simple.
**	9-aug-93 (rickh)
**	    Added another flag to qef_callbackStatus.  This flags whether
**	    a SELECT ANY( 1 ) statement returned 1 or 0.  The REFERENTIAL
**	    CONSTRAINT state machine issues a SELECT statement verifying
**	    whether the constraint holds.  This new flag allows that SELECT
**	    to tell the next REFERENTIAL state whether the SELECT succeeded.
**	    If it didn't, we roll back the ADD CONSTRAINT DDL and don't bother
**	    cooking up all the tiresome objects which support a REFERENTIAL
**	    CONSTRAINT.
**	25-aug-93 (anitap)
**	    Added another flag to qef_callbackStatus. This flags whether a
**	    SELECT ANY(1) statement has to be executed by QEF after the 
**	    query text for the statement has gone through execute immediate
**	    parsing.
**	22-sep-93 (andre)
**	    added qef_sess_flags and defined QEF_RUNNING_UPGRADEDB
**	05-nov-1993 (smc)
**          Bug #58635
**          Made l_reserved & owner elements PTR for consistency with DM_SVCB.
**	24-nov-93 (robf)
**          Add qef_group, qef_aplid to allow security alarms by group/role.
**	20-jan-94 (rickh)
**	    Added field to QEF_CB to hold the spill threshold for QEN hold
**	    files.  So far, this is only used for testing the spill logic.
**	14-mar-94 (swm)
**	    Bug #59883
**	    Added qef_trfmt buffer pointer and qef_trsize, QEF_TRFMT_SIZE
**	    definitions to QEF_CB. This is to enable the allocation of a
**	    TRformat buffer per QEF session rather than relying on automatic
**	    (TRformat) buffer allocation on the stack to eliminate the
**	    probability of stack overrun owing to large buffers.
**	28-jan-00 (inkdo01)
**	    Added qef_h_max & qef_h_used to define max hash join memory per query.
**	28-Jul-2006 (jonj)
**	    Added qef_error_errorno, qef_error_qef_code, set by qef_error(),
**	    used by the sequencer to figure out XA return codes.
**	4-oct-2006 (dougi)
**	    Enlarged qef_debug to accommodate Karl's tp.
**	4-Jun-2009 (kschendel) b122118
**	    Add QE11 trace point (for the nth time, I keep losing it!)
**	    Add QE89 hash debug trace point.
**	24-Sep-09 (smeke01) b122635
**	    Add trace point QE71 to enable/disable diagnostic function 
**	    qen_exchange_serial(). 
**      13-Sep-2010 (kschendel) Bug 123720
**          Add qef_trsem for valid parallel-query output.
*/
/* FIXME:  This structure should have been reorganized for better alignment */
struct _QEF_CB
{
    /* standard stuff */
    QEF_CB     *qef_next;       /* The next QEF control block */
    QEF_CB     *qef_prev;       /* The previous one */
    SIZE_TYPE   qef_length;     /* size of the control block */
    i2          qef_type;       /* type of control block */
#define QEFCB_CB    0
    i2          qef_s_reserved;
    PTR         qef_l_reserved;
    PTR         qef_owner;
    i4          qef_ascii_id;   /* to view in a dump */
#define QEFCB_ASCII_ID  CV_C_CONST_MACRO('Q', 'E', 'C', 'B')
    /* user specific stuff */
    i4		qef_operation;	/* requested QEF operation */
    PTR		qef2_rcb;	/* generic request block pointer */
    QEF_RCB	*qef_rcb;	/* user's request block */
    /* transaction information */
    CS_SID	qef_ses_id;     /* name of session */
    PTR		qef_dbid;	/* database id */
    i4		qef_udbid;	/* unique database id */
    DB_TRAN_ID  qef_tran_id;    /* what transaction are we running */
    PTR		qef_dmt_id;     /* dmf transaction id */
    i4          qef_stat;       /* what type of transaction is occurring */
#define QEF_NOTRAN  1           /* no transaction in progress   */
#define QEF_SSTRAN  2           /* single statment single cursor */
#define QEF_MSTRAN  3           /* multiple statement transaction */
#define QEF_ITRAN   4           /* internal read only transaction, used
				** only during query translation and
				** compilation */
#define QEF_RO_SSTRAN 10	/* SST being used temporary in read-only mode,
				** used only during query translation */
#define QEF_RO_MSTRAN 11	/* MST being used temporary in read-only mode,
				** used only during query translation */
    i4		qef_ostat;      /* qef_stat at end of last action/statement */
    bool	qef_abort;	/* TRUE - current tran must be aborted */
    i4		qef_auto;	/* TRUE - auto commit is on */
    bool	qef_defr_curs;	/* TRUE - a deferred cursor is opened */
    i4          qef_open_count; /* number of open cursors in transaction */
    i4          qef_stmt;       /* Next statement number.  This nunber is
				** reset to 0 at the beginning of the 
				** transaction. When a QP is opened, this
				** number is used as the DSH statement # */
    i4		qef_tran_seq;   /* statement sequence within transaction */
				/* (this will eventually replace qef_stmt) */
    /* query information */
    PTR		qef_dsh;	/* Top of Stack for the DSH stack. Contains
				** pointer to current query plan's
                                ** data segment header (DSH).
				** The stack is represented as a list of
				** DSH's (see dsh_stack).
                                */
    PTR		qef_odsh;	/* pointer to list of open DSHs */
    /* utility routines */
    i4              qeu_cstat;  /* copy status */
#define QEF_NOCOPY 0            /* no copy in progress */
#define QEF_R_COPY 1            /* copy from in progress */
#define QEF_W_COPY 2            /* copy into in progress */
    QEU_COPY	    *qeu_copy;  /* QEU_COPY control block for 
                                ** copy commands
                                */
    /* general purpose stuff */
    ADF_CB	    *qef_adf_cb;/* define datatype semantics */
    ULT_VECTOR_MACRO(QEF_SSNB, QEF_SNVAO) qef_trace; /* session trace flag */
#define QEF_TRACE_TOSS_ROWS	    11	/* (qe11 NN) Return first NN rows,
					** toss the rest.  Unlike FIRST N,
					** isn't known to the query plan.
					*/
#define QEF_XAFABORT		    40  /* (qe40) XA_ROLLBACK force abort */

/* qef trace immediate flags - print to II_DBMS_LOG */
#define QEF_TRACE_IMMEDIATE	    	41  /* (qe41) trace immediate */
#define QEF_TRACE_IMMEDIATE_ULM	    	 1
#define QEF_TRACE_IMMEDIATE_ULM_POOLMAP	 2

/* trace points used by STAR */
#define QEF_TRACE_DDB_NO_DROP_50    50	/* (qe50) suppress dropping temporary 
					** table(s) at end of query */
#define QEF_TRACE_DDB_VAL_QP_51	    51	/* (qe51) validate QP */

#define QEF_TRACE_DDB_LOG_QRY_55    55	/* (qe55) trace queries, including text
					*/
#define QEF_TRACE_DDB_LOG_DDL_56    56	/* (qe56) trace DDL commands */
#define QEF_TRACE_DDB_LOG_MEM_57    57	/* (qe57) trace stream (de)allocation */
#define QEF_TRACE_DDB_LOG_QEF_58    58	/* (qe58) trace QEF calls */
#define QEF_TRACE_DDB_LOG_ERR_59    59	/* (qe59) trace queries in error 
					** situations */
#define QEF_TRACE_NO_AGGF_OPTIM     60  /* No simple aggregate optim     */
#define QEF_TRACE_NO_DEPENDENCY_CHK 61  /* trace point qe61
					** Using this trace point allows server
					** to skip checking table/index constrain
					** dependency for 'modify table' stmt.
					** This may cause b109682,INGSRV2155 and
   			         	** b109564, INGSRV2237.
					*/
/* Trace point parallel index building */
#define QEF_TRACE_NO_PAR_INDEX_70   70  /* No parallel index building */
#define QEF_TRACE_EXCH_SERIAL_71  71	/* Enable use of serial non-CUT exchange node
					** code instead of the normal exchange node code.
					** Useful for analysing problems with parallel 
					** query plans */
#define QEF_TRACE_HASH_DEBUG	    89	/* Massive debug output from hash
					** join, hash aggregation */

    char    *qef_trfmt;			/* buffer for TRformat, initialised
					** in qec_begin_session() */
    i4	    qef_trsize;			/* size of TRformat buffer */
#define QEF_TRFMT_SIZE 4096		/* allocated buffer size, must be >=
					** ULD_PRNODE_LINE_WIDTH + 1
					** DB_MAXTUP * 2 */
    CS_SEMAPHORE qef_trsem;		/* Mutex the trfmt buffer for valid
					** parallel-query output
					*/

    bool	    qef_ade_abort;	/* ADF fatal error flag */
    /* small sort */
    i4	    qef_s_max;	/* max size of sort buffers per session */
    i4	    qef_s_used;	/* amount currently used for sort buffers */
    i4	    qef_h_max;	/* max size of hash join buffers per query */
    i4	    qef_h_used;	/* amount currently used for hash join buffers */
    DB_DIS_TRAN_ID  qef_dis_tran_id; /* The distributed transaction ID of 
				     ** the local transaction. 
				     */
    /* The following fields contain the resource limits for executing	    */
    /* queries in this session. See the QEF_QEP description for additional  */
    /* details. */
    i4	    qef_dio_limit;  /* Physical IO estimate limit */
    i4	    qef_row_limit;  /* Max estimated rows returned */
    i4	    qef_cpu_limit;  /* Max estimated cpu limit */
    i4	    qef_page_limit; /* Max estimated pages touched limit */
    i4	    qef_cost_limit; /* Max estimated query cost limit */

    /* The following field contains the database-wide privileges for	    */
    /* this session */
    u_i4	    qef_fl_dbpriv;  /* Database privilege flags for session.*/

    i4		    qef_max_stack;  /* Maximum nesting depth for database   */
				    /* procedure's in this session. */
    i4		    qef_stm_error;  /* Defines error handling action for
				    ** statement errors. */
#define	QEF_STMT_ROLLBACK    1      /* Abort current statement on error */
#define	QEF_XACT_ROLLBACK    2      /* Abort entire transaction on error */
#define	QEF_SVPT_ROLLBACK    3      /* Abort to specified savepoint on error */
    DB_SP_NAME	    qef_savepoint;  /* User Savepoint name to which to abort 
				    ** when a statement error occurs. */
    i4		    qef_ustat;	    /* User status flags from SCS_ICS */
    DB_OWN_NAME	    qef_user;	    /* User name */
    bool            qef_user_interrupt; /* Flag to indicate that user interrupt
					** has occured.
					*/
    bool            qef_user_interrupt_handled; /* Flag to indicate that user
						** interrupt has been reported.
						*/
    i4              qef_upd_rowcnt;  /* Update rowcount value flag.
                                    ** Values for this are defined in QEF_RCB
                                    ** for CHANGED and QUALIFIED.
                                    */
    
    DB_DISTRIB	    qef_c1_distrib;	/* a DDB session if DB_DSTYES, 
					** a regular one otherwise */
    QES_DDB_SES	    qef_c2_ddb_ses;	/* additional DDB session information, 
					** valid only for Titan */
    PTR		    qef_c3_streamid;	/* dedicated stream for session's use,
					** opened by qec_beg_session and
					** closed by qec_end_session */
    i4		    qef_c4_streamsize;	/* size of stream currently in use */

/*
**	    qef_callbackStatus preserves context
**	    over nested qef_calls.  For instance, during the creation of
**	    a CONSTRAINT, qef cooks up text (like "create index"), which
**	    psf parses and then callsback qef to execute.  It's useful to
**	    remember some simple facts across these callbacks.  Again for
**	    example, if we're in the middle of creating a UNIQUE CONSTRAINT
**	    and qef has called itself back to cook up an index, it's useful
**	    for the nexted index-creation to know that it's operating in
**	    the context of a "create constraint" so it can handle errors
**	    differently.  Simple.
*/

    u_i4	    qef_callbackStatus;

#define	QEF_MAKE_UNIQUE_CONS_INDEX	0x00000001
					/* we're in the middle of making an
					** index for a unique constraint.
					*/
#define	QEF_SELECT_RETURNED_ROWS	0x00000002
/*
**	    This flags whether
**	    a SELECT ANY( 1 ) statement returned 1 or 0.  The REFERENTIAL
**	    CONSTRAINT state machine issues a SELECT statement verifying
**	    whether the constraint holds.  This flag allows that SELECT
**	    to tell the next REFERENTIAL state whether the SELECT succeeded.
**	    If it didn't, we roll back the ADD CONSTRAINT DDL and don't bother
**	    cooking up all the tiresome objects which support a REFERENTIAL
**	    CONSTRAINT.
*/
#define QEF_EXECUTE_SELECT_STMT		0x00000004
/*
**	    This flags whether a SELECT ANY(1) statement has be to be 
**	    executed. This is in execute immediate mode. We have finished
**	    parsing the SELECT ANY(1) statement cooked up by QEF.
*/

    u_i4	    *qef_dbxlate;	/* Case translation semantics flags
					** See <cuid.h> for masks.
					*/
    DB_OWN_NAME	    *qef_cat_owner;	/* Catalog owner name.  Will be
					** "$ingres" if regular ids are lower;
					** "$INGRES" if regular ids are upper.
					*/
    i4		    qef_sess_flags;
					/*
					** will be set if the current session 
					** is running UPGRADEDB; initially, 
					** this will cause qeu_v_ubt_id() and
					** qeu_p_ubt_id() to behave as if id 
					** of a base table used in the 
					** definition of a view or a dbproc
					** could not be located (we don't want
					** to look for them because in case of 
					** views, RDF may be unable to handle 
					** older versions of trees and in case 
					** of dbprocs, db_dbp_ubt_id will 
					** contain 0's anyway 
					*/
#define	QEF_RUNNING_UPGRADEDB		0x01

    i4         qef_spillThreshold;	/*
					** For testing only.  When set to a
					** non-negative number, that's the
					** number of tuples we allow in qen hold
					** files before we spill them to
					** disk.
					*/
#define	QEF_NO_SPILL_THRESHOLD		( -1L )
					/* means this field doesn't contain
					** a spill threshold.  spill when we
					** reach the calculated limit. */
    DB_OWN_NAME	    *qef_group;		/* Current ingres group */

    DB_OWN_NAME	    *qef_aplid;		/* Current ingres role */

    /* Tracks of qef_error(errorno) to qef_code transformation: */
    i4		    qef_error_errorno;	/* errorno that started qef_error() */
    i4		    qef_error_qef_code;	/* errorno=>qef_code translation */
};

/*}
** Name: QEF_DBG - debugging control block
**
** Description:
**      This structure is used to specify which functions to call
** in order to produce some debugging information.
**
** History:
**     29-apr-86 (daved)
**          written
*/
typedef struct _QEF_DBG
{
    i4          dbg_operation;          /* debugging operation */
#define QEF_PSES    0                   /* print session control block */
#define QEF_PSER    1                   /* print server control block */
#define QEF_PDSH    2                   /* print runtime environment */
#define QEF_PQEP    3                   /* print query execution plan */
#define	QEF_PQP	    4			/* print query plan */
    PTR         dbg_addr;               /* address of control structure */
}   QEF_DBG;

/*}
** Name: ERR_CB - error control block
**
** Description:
**      This structure is used to keep track of error handling info
**  and a pointer to the error block
**
** History:
**	24-mar-87 (daved)
**	    written
*/
typedef struct _ERR_CB
{
    i4		err_eflag;		/* send errors to user (qef_exteral)
					** or just return (qef_internal)
					*/
    DB_ERROR	*err_block;		/* pointer to users error block */
}   ERR_CB;

