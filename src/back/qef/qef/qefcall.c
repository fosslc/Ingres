/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <er.h>
#include    <cs.h>
#include    <ex.h>
#include    <tm.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <qefkon.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ulx.h>
#include    <adf.h>
#include    <ade.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <qefmain.h>
#include    <qefscb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <sxf.h>
#include    <qefqp.h>
#include    <dudbms.h>
#include    <qefdsh.h>
#include    <qefprotos.h>	
/**
**
**  Name: QEFCALL.C - top level interface to QEF
**
**  Description:
**      QEF provides query execution support to the INGRES DBMS.
**	QEF generally receives input from OPF in the form of query plans
**	and returns rows to SCF for final processing. 
**
**	Functions included in this file.
**
**          qef_call	    - top level interface to all of QEF
**	    qef_handler	    - QEF exception handler
**	    qef_2handler    - Second level exception handler
**	    qe1_chk_error   - Debug code to check if the legal error is
**			      being returned to the caller.
**	    qe2_chk_qp	    - Check for QP pointer in a DSH.
**
**
**  History:
**      28-may-86 (daved)    
**      20-aug-86 (jennifer)
**	    integrated qeu query code
**	05-dec-86 (rogerk)
**	    put in QEU_B_COPY, QEU_E_COPY, QEU_R_COPY and QEU_W_COPY
**	17-sep-87 (puree)
**          error handling clean up
**	27-oct-87 (puree)
**	    change ULE_FORMAT flag in qef_handler
**	10-nov-87 (puree)
**	    modified qef_hander not to call functions if the request
**	    blocks has not been acquired.
**	24-nov-87 (puree)
**	    changed EXaccvio to EXsys_report in qef_handler.
**	04-dec-87 (puree)
**          fixed math exception handling when -xf is set.
**	23-dec-87 (puree)
**	    modify qef_call for transaction processing flag for qeu_dbu calls.
**	06-jan-88 (puree)
**	    modify qef_call for QEU_DBU function again.  PSY wants to handle
**	    transaction by itself but wants QEF to handle error mapping.
**	07-jan-88 (puree)
**	    modify qef call to add debug code to check for invalid error 
**	    codes before returning to the caller.  Added qef_chk_error
**	    function.
**	11-mar-88 (puree)
**	    added qe2_chk_qp. Rename qef_chk_error to qe1_chk_error.
**	02-may-88 (puree)
**	    Added op code QEU_CREDBP and QEU_DRPDBP to qef_call.
**	12-may-88 (puree)
**	    Modify qef_call and qef_handler to call qee_cleanup function.
**	11-aug-88 (puree)
**	    Clean up lint error
**	    Modify QEFCALL to return E_DB_ERROR rather than E_DB_FATAL
**	    in AV cases when compiled with xDEBUG.
**	15-aug-88 (puree)
**	    Issue EXdelete before returning to the caller.  Move all EXdelete
**	    calls near the return statements to make it obvious.
**	21-aug-88 (carl)
**	    1.  Extracted from qef_call code to form qef_c1_fatal.
**	    2.  Modified qef_call to process DDB operations.
**	28-oct-88 (puree)
**	    Move cleaning up logic from qef_handler into qef_call.  Qef_handler
**	    is shared by qen_dmr_cx in which the DSH cannot be cleaned up.
**	15-jan-89 (paul)
**	    Added additional comments to describe new return values for
**	    rules support in qef_call().
**	8-Mar-89 (ac)
**	    Added 2PC support.
**	12-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Add support for QEU_GROUP and QEU_APLID opcodes.
**	19-mar-89 (neil)
**	    Added rule querymod support for QEU_CRULE and QEU_DRULE.
**	17-apr-89 (paul)
**	    Dispatch cursor update requests to qeq_query.
**	25-apr-89 (carl)
**	    Modified qef_call:
**		1.  Changed QED_DDL_CONCURRENCY to QED_CONCURRENCY
**		2.  Added new call QED_7RDF_DIFF_ARCH
**	10-may-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add support for QEU_DBPRIV opcode.
**	14-may-89 (carl)
**	    Modified qef_call to maintain the QES_02CTL_COMMIT_CDB_TRAFFIC 
**	    bit of qes_d9_ctl_info in the DDB session control block for 
**	    committing any and all catalog traffic via the privileged CDB
**	    association if QES_01CTL_DDL_CONCURRENCY is ON
**	22-may-89 (neil)
**	    "Find" lost cursor.  Cursor was lost due to calling qee_cleanup
**	    before validating the xact state on a call to QET_COMMIT/ABORT.
**	23-may-89 (jrb)
**	    Changed calls to scc_error to first set generic error code.  Renamed
**	    scf_enumber to scf_local_error.  Changed ule_format calls for new
**	    interface.
**	23-may-89 (carl)
**	    Fixed qef_call to do DDL_CONCURRENCY commit when a rerouted SELECT 
**	    query is finished reading all the tuples 
**	30-may-89 (carl)
**	    Changed qef_call to precommit the privileged CDB association 
**	    to release RDF's catalog locks when the internal transaction
**	    ends or aborts.
**	06-jun-89 (carl)
**	    Moved above functions from qef_call to qeu_etran and qeu_atran
**	22-jun-89 (carl)
**	    Changed qef_call to process QED_DESCPTR 
**      27-jun-89 (jennifer)
**          Added routines for user, location, dbaccess, and security.
**	29-sep-89 (ralph)
**	    Added calls to qeu_aloc() and qeu_dloc().
**	07-oct-89 (carl)
**	    Modified qef_call to call qel_u3_lock_objbase for CREATE LINK,
**	    CREATE VIEW, DROP VIEW to avoid STAR deadlocks
**	25-oct-89 (neil)
**	    Added event qrymod support for QEU_CEVENT and QEU_DEVENT.
**	11-nov-89 (carl)
**	    Added code for QED_RECOVER
**	28-jan-90 (carl)
**	    Replaced QED_RECOVER with QED_P1_RECOVER and QED_P2_RECOVER 
**	    Added QED_IS_DDB_OPEN
**	15-feb-90 (carol)
**	    Added comment support for QEU_CCOMMENT.
**      22-mar-90 (carl)
**          changed qef_call to call qeu_d9_a_copy for QED_A_COPY
**	19-apr-90 (andre)
**	    Added support for QEU_CSYNONYM and QEU_DSYNONYM.
**	14-jun-90 (andre)
**	    Added support for QEU_CTIMESTAMP.
**      12-jul-90 (carl)
**	    replaced all calls to qef_c1_fatal in qef_call with in-line code
**	12-jul-90 (georgeg-carl) 
**	    added qed_3scf_cluster_info() to pass the cluster node names from 
**	    SCF-RDF to RQF.
**      21-aug-90 (carl)
**	    modified qef_call to set QES_04CTL_OK_TO_OUTPUT_QP in the DDB 
**	    portion of the session CB;
**	    added trace point processing code
**	20-nov-90 (davebf)
**	    Don't reset qef_ostat if qef_id is QEC_INFO
**	18-apr-91 (fpang)
**	    In qef_call, pass in actual error cb, not a dummy one to qef_error.
**	    Qef_error will remap rqf/tpf errors to qef error.s
**	    Fixes B36913.
**	24-may-91 (andre)
**	    added support for QEC_RESET_EFF_USER_ID
**	07-aug-91 (fpang)
**	    If request is QEU_{C,D}VIEW, don't assume a valid qer_p when
**	    initilizing qer_p->error.err_code, qer_p->qef_cb, and
**	    qer_p->qef_r3_ddb_req.qer_d13_ctl_info.
**	    Fixes B38882.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts in qef_call.
**	15-jun-92 (davebf)
**	    Sybil Merge
**	26-jun-92 (andre)
**	    added support for QEU_DBP_STATUS
**	02-aug-92 (andre)
**	    added support for QEU_REVOKE
**	08-sep-92 (fpang)
**	    Temporarily define rqf_call() and tpf_call().
**	    Distributed will have to define STAR to not have
**	    the stubs.
**	03-nov-92 (fpang)
**	    Eliminated rqf_call() and tpf_call() stubs. It's one
**	    one big happer server now.
**	20-nov-92 (rickh)
**	    New arguments to qeu_dbu.
**	08-dec-92 (anitap)
**	    Added #include of qsf.h and psfparse.h for CREATE SCHEMA.
**	21-dec-92 (anitap)
**	    Prototyped qef_handler(), qef_2handler(), qe1_chk_error() and
**	    qe2_chk_qp().
**	05-feb-93 (andre)
**	    remove support for QEC_RESET_EFF_USER_ID; resetting the effective
**	    user id will be handled by QEC_ALTER
**	20-apr-93 (jhahn)
**	    Added code to check to see if transaction statements should be
**	    allowed.
**      06-may-93 (anitap)
**          Added support for "SET UPDATE_ROWCOUNT" statement.
**	09-jun-93 (rickh)
**	    New arguments to qeu_dbu.
**	16-jun-93 (jhahn)
**	    Fixed problem with returning error when an illegal transaction
**	    statement is attempted.
**	29-jun-93 (rickh)
**	    Added TR.H.
**	07-jul-93 (rickh)
**	    Prototyped qef_call itself.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jul-1993 (rog)
**	    Exception handler cleanup.
**	26-aug-93 (robf)
**	    Add profile handling (QEU_C/A/DPROFILE)
**	4-mar-94 (robf)
**          Add role grant handling (QEU_ROLEGRANT)
**      5-dec-96 (stephenb)
**          Add performance profiling code
**	21-may-1997 (shero03)
**	    Added facility tracing diagnostic
**	07-Aug-1997 (jenjo02)
**	    Changed *dsh to **dsh in qee_cleanup(), qee_destroy() prototypes.
**	    Pointer will be set to null if DSH is deallocated.
**	1-apr-1998 (hanch04)
**	    Calling SCF every time has horendous performance implications,
**	    re-wrote routine to call CS directly with qef_session.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      05-Aug-1999 (hweho01)
**          1) Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**              redefinition on AIX 4.3, it is used in <sys/context.h>.
**          2) Moved include of cs.h before ex.h to avoid compile errors
**             from replacement of jmpbuf in ex_context structure. 
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	19-Oct-2000 (jenjo02)
**	    Added "cb_check" static array to get rid of several
**	    lengthy switch statements.
**	    Replaced calls to qef_session() with GET_QEF_CB macro.
**	16-jul-2001 (somsa01)
**	    In qef_call(), added the missing setup of the qef_cb if
**	    EXdeclare() fails. This was accidentally removed by a previous
**	    change.
**	17-Aug-2001 (devjo01)
**	    Force compiler to not put qef_cb into a register.  This
**	    fixes b105551 in which user numeric overflows would
**	    become FATAL errors on Tru64.
**	01-nov-2001 (somsa01)
**	    In qef_call(), initialize "status" and "local_status" to OK.
**	05-Mar-2002 (jenjo02)
**	    Added Sequence Generator operations, QEU_CSEQ, QEU_ASEQ,
**	    QEU_DSEQ.
**      02-oct-2002 (stial01)
**          Added op code QET_XA_ABORT to qef_call.
**	17-Jan-2004 (schka24)
**	    Redo prototype so that 2nd arg is pointer to void.
**	19-Oct-2005 (schka24)
**	    Remove unused alter-timestamp call.
**	26-Jun-2006 (jonj & stial01)
**	    Deleted QET_XA_ABORT, added QET_XA_PREPARE,
**	    QET_XA_COMMIT, QET_XA_ROLLBACK, QET_XA_START, QET_XA_END.
**      09-feb-2007 (stial01)
**          Free locators at explicit commit/abort.
**	01-May-2007 (toumi01)
**	    Don't try to free (NULL) locators for STAR.
**      17-May-2007 (stial01)
**          Call adu_free_locator with DB_DATA_VALUE
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**      17-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
*/

GLOBALREF   QEF_S_CB	*Qef_s_cb;
GLOBALREF   char	*IIQE_53_lo_qef_tab[];
GLOBALREF   char	*IIQE_54_hi_qef_tab[];

static	const	struct
{
	i2	cb_type;
} cb_check[QEF_LO_MAX_OP] = 
{
	{-1},			/* 0	Not used */
	{-1},			/* 1	Not used */
	{QESRCB_CB},		/* 2	QEC_INITIALIZE */
	{QEFRCB_CB},		/* 3	QEC_BEGIN_SESSION */
	{-1},			/* 4	QEC_DEBUG */
	{QEFRCB_CB},		/* 5	QEC_ALTER */
	{-1},			/* 6	QEC_TRACE */
	{QEFRCB_CB},		/* 7	QET_BEGIN */
	{QEFRCB_CB},		/* 8	QET_SAVEPOINT */
	{QEFRCB_CB},		/* 9	QET_ABORT */
	{QEFRCB_CB},		/* 10	QET_COMMIT */
	{QEFRCB_CB},		/* 11	QEQ_OPEN */
	{QEFRCB_CB},		/* 12	QEQ_FETCH */
	{QEFRCB_CB},		/* 13	QEQ_REPLACE */
	{QEFRCB_CB},		/* 14	QEQ_DELETE */
	{QEFRCB_CB},		/* 15	QEQ_CLOSE */
	{QEFRCB_CB},		/* 16	QEQ_QUERY */
	{QEFRCB_CB},		/* 17	QEQ_B_COPY */
	{QEFRCB_CB},		/* 18	QEQ_E_COPY */
	{QEFRCB_CB},		/* 19	QEQ_R_COPY */
	{QEFRCB_CB},		/* 20	QEQ_W_COPY */
	{QEFRCB_CB},		/* 21	QEC_INFO */
	{QESRCB_CB},		/* 22	QEC_SHUTDOWN */
	{QEFRCB_CB},		/* 23	QEC_END_SESSION */
	{QEUCB_CB},		/* 24	QEU_BTRAN */
	{QEUCB_CB},		/* 25	QEU_ETRAN */
	{QEUCB_CB},		/* 26	QEU_APPEND */
	{QEUCB_CB},		/* 27	QEU_GET */
	{QEUCB_CB},		/* 28	QEU_DELETE */
	{QEUCB_CB},		/* 29	QEU_OPEN */
	{QEUCB_CB},		/* 30	QEU_CLOSE */
	{QEUCB_CB},		/* 31	QEU_DBU */
	{QEUCB_CB},		/* 32	QEU_ATRAN */
	{QEUCB_CB},		/* 33	QEU_QUERY */
	{QEUQCB_CB},		/* 34	QEU_DBP_STATUS */
	{QEUQCB_CB},		/* 35	QEU_DVIEW */
	{QEUQCB_CB},		/* 36	QEU_CPROT */
	{QEUQCB_CB},		/* 37	QEU_DPROT */
	{QEUQCB_CB},		/* 38	QEU_CINTG */
	{QEUQCB_CB},		/* 39	QEU_DINTG */
	{QEUQCB_CB},		/* 40	QEU_DSTAT */
	{QEFRCB_CB},		/* 41	QET_SCOMMIT */
	{QEFRCB_CB},		/* 42	QET_AUTOCOMMIT */
	{QEFRCB_CB},		/* 43	QET_ROLLBACK */
	{QEFRCB_CB},		/* 44	QEQ_ENDRET */
	{-1},			/* 45	QEQ_EXECUTE */
	{QEUQCB_CB},		/* 46	QEU_CREDBP */
	{QEUQCB_CB},		/* 47	QEU_DRPDBP */
	{QEFRCB_CB},		/* 48	QET_SECURE */
	{QEFRCB_CB},		/* 49	QET_RESUME */
	{QEUQCB_CB},		/* 50	QEU_GROUP */
	{QEUQCB_CB},		/* 51	QEU_APLID */
	{QEUQCB_CB},		/* 52	QEU_CRULE */
	{QEUQCB_CB},		/* 53	QEU_DRULE */
	{QEUQCB_CB},		/* 54	QEU_DBPRIV */
	{QEUQCB_CB},		/* 55	QEU_CUSER */
	{QEUQCB_CB},		/* 56	QEU_DUSER */
	{QEUQCB_CB},		/* 57	QEU_CLOC */
	{QEUQCB_CB},		/* 58	QEU_CDBACC */
	{QEUQCB_CB},		/* 59	QEU_DDBACC */
	{QEUQCB_CB},		/* 60	QEU_MSEC */
	{QEUQCB_CB},		/* 61	QEU_AUSER */
	{QEUQCB_CB},		/* 62	QEU_CEVENT */
	{QEUQCB_CB},		/* 63	QEU_DEVENT */
	{QEUQCB_CB},		/* 64	QEU_ALOC */
	{QEUQCB_CB},		/* 65	QEU_DLOC */
	{QEUQCB_CB},		/* 66	QEU_CCOMMENT */
	{QEUQCB_CB},		/* 67	QEU_CSYNONYM */
	{QEUQCB_CB},		/* 68	QEU_DSYNONYM */
	{-1},			/* 69	Not used */
	{QEUQCB_CB},		/* 70	QEU_REVOKE */
	{QEUQCB_CB},		/* 71	QEU_DSCHEMA */
	{QEFRCB_CB},		/* 72	QEQ_UPD_ROWCNT */
	{-1},			/* 73	Not used */
	{-1},			/* 74	Not used */
	{-1},			/* 75	Not used */
	{-1},			/* 76	Not used */
	{-1},			/* 77	Not used */
	{-1},			/* 78	Not used */
	{-1},			/* 79	Not used */
	{-1},			/* 80	Not used */
	{-1},			/* 81	Not used */
	{-1},			/* 82	Not used */
	{-1},			/* 83	Not used */
	{-1},			/* 84	Not used */
	{-1},			/* 85	Not used */
	{-1},			/* 86	Not used */
	{-1},			/* 87	Not used */
	{-1},			/* 88	Not used */
	{-1},			/* 89	Not used */
	{-1},			/* 90	Not used */
	{-1},			/* 91	Not used */
	{-1},			/* 92	Not used */
	{-1},			/* 93	Not used */
	{-1},			/* 94	Not used */
	{-1},			/* 95	Not used */
	{-1},			/* 96	Not used */
	{-1},			/* 97	Not used */
	{-1},			/* 98	Not used */
	{-1},			/* 99	Not used */
	{QEUQCB_CB},		/* 100	QEU_CPROFILE */
	{QEUQCB_CB},		/* 101	QEU_APROFILE */
	{QEUQCB_CB},		/* 102	QEU_DPROFILE */
	{QEFRCB_CB},		/* 103	QEU_RAISE_EVENT */
	{QEUQCB_CB},		/* 104	QEU_CSECALM */
	{QEUQCB_CB},		/* 105	QEU_DSECALM */
	{QEUQCB_CB},		/* 106	QEU_ROLEGRANT */
	{QEFRCB_CB},		/* 107	Not used */
	{QEFRCB_CB},		/* 108	QEU_IPROC_INFO */
	{QEFRCB_CB},		/* 109	QEU_EXEC_IPROC */
	{QEUQCB_CB},		/* 110	QEU_CSEQ */
	{QEUQCB_CB},		/* 111	QEU_ASEQ */
	{QEUQCB_CB}, 		/* 112	QEU_DSEQ */
	{QEFRCB_CB},		/* 113	QET_XA_PREPARE */
	{QEFRCB_CB},		/* 114	QET_XA_COMMIT */
	{QEFRCB_CB},		/* 115	QET_XA_ROLLBACK */
	{QEFRCB_CB},		/* 116	QET_XA_START */
	{QEFRCB_CB}		/* 117	QET_XA_END */
};

/*{
** Name: qef_call	- dispatch QEF calls
**
** Description:
**      Input validation checking is performed and then
**	a call to the appropriate routine is made. Error
**	handling is handled in each of the routines. 
**
**	Support for rules processing requires the following constraints
**	be met by the caller of QEF.
**
**	    qef_call() may return E_QE0023_INVALID_QUERY or
**	    E_QE0119_LOAD_QP. Each of these is a request to load a QP
**	    that QEF could not find. The first is a request to load the
**	    QP which QEF was originally called to execute. The second
**	    is a request to load a QP that has been invoked by a nested
**	    procedure call in a QP. Both situations may arise in a
**	    nested procedure invocation. When these errors occur, the
**	    caller of QEF is constrained to return to QEF with a valid QP
**	    with the returned QEF_RCB unchanged.
**	    
** Inputs:
**	qef_id				qef operation request id
**      rcb		                appropriate control block
**
** Outputs:
**      none
**
**	Returns:
**	    E_DB_{OK ERROR WARN FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	The request is satisfied and error messages are either
**	logged or spooled.
**
** History:
**	29-may-86 (daved)
**          written
**      20-aug-86 (jennifer)
**	    integrated qeu query code
**	05-dec-86 (rogerk)
**	    put in QEU_B_COPY, QEU_E_COPY, QEU_R_COPY and QEU_W_COPY
**	17-sep-87 (puree)
**          error handling clean up.
**	04-dec-87 (puree)
**	    when -xf specified, qef_handler will return EX_DECLARE value
**	    with qef_ade_abort set.  Qef_call has to return E_DB_ERROR, not
**	    E_DB_FATAL, to SCF.
**	23-dec-87 (puree)
**	    set transaction processing flag for qeu_dbu based on the
**	    setting of qef_eflag.
**	06-jan-88 (puree)
**	    modify call sequence to qeu_dbu.  Set caller flag to external.
**	07-jan-88 (puree)
**	    modify qef call to add debug code to check for invalid error 
**	    codes before returning to the caller.
**	02-may-88 (puree)
**	    Added opcode QEU_CREDBP and QEU_DRPDBP for DB procedure support.
**	12-may-88 (puree)
**	    Call qee_cleanup after qet_abort to clean up the environment.
**	    The call to qee_cleanup in qet_abort has been removed.
**	11-aug-88 (puree)
**	    Modify QEFCALL to return E_DB_ERROR rather than E_DB_FATAL
**	    in AV cases when compiled with xDEBUG.
**	21-aug-88 (carl)
**	    Modified to 
**		1) process new DDB operations, and 
**		2) downgrade existing functions
**	28-oct-88 (puree)
**	    Move cleaning up logic from qef_handler to this routine.
**	15-jan-89 (paul)
**	    Added comments to procedure header to indicate new return
**	    information for qef_call().
**	8-Mar-89 (ac)
**	    Added 2PC support.
**	12-mar-89 (ralph)
**	    GRANT Enhancements, Phase 1:
**	    Add support for QEU_GROUP and QEU_APLID opcodes.
**	19-mar-89 (neil)
**	    Added opcode QEU_CRULE and QEU_DRULE for rule creation/destruction.
**	17-apr-89 (paul)
**	    Dispatch cursor update requests to qeq_query.
**	25-apr-89 (carl)
**	    Changed QED_DDL_CONCURRENCY to QED_CONCURRENCY
**	    Added new call QED_7RDF_DIFF_ARCH
**	10-may-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Add support for QEU_DBPRIV opcode.
**	14-may-89 (carl)
**	    Modified to maintain the QES_02CTL_COMMIT_CDB_TRAFFIC 
**	    bit of qes_d9_ctl_info in the DDB session control block
**	    for committing any and all catalog traffic via the privileged
**	    CDB association if QES_01CTL_DDL_CONCURRENCY is ON
**	21-may-89 (jrb)
**	    Changed calls to scc_error to first set generic error code.  Renamed
**	    scf_enumber to scf_local_error.
**	22-may-89 (neil)
**	    "Find" lost cursor.  Cursor was lost due to calling qee_cleanup
**	    BEFORE validating the xact state on call to QET_COMMIT, QET_ABORT,
**	    QET_ROLLBACK & QET_SECURE.  Solution is to use those routines'
**	    transaction status to indicate if we should clean up afterwards.
**	23-may-89 (carl)
**	    Fixed to do DDL_CONCURRENCY commit when a rerouted SELECT query
**	    is finished reading all the tuples 
**	30-may-89 (carl)
**	    Modified to precommit the privileged CDB association to
**	    release RDF's catalog locks when the internal transaction
**	    ends or aborts.
**	06-jun-89 (carl)
**	    Moved above functions to qeu_etran and qeu_atran
**	22-jun-89 (carl)
**	    1)  Added a defensive measure to catch NULL DDB descriptor 
**	        pointer in the session control block (QEF_CB)
**	    2)  Added code to process QED_DESCPTR to set the DDB descriptor
**	        pointer in the session control block (QEF_CB)
**	23-jun-89 (seputis)
**	    removed above check for NULL DDB ptr since it occurs for a iimonitor session
**      27-jun-89 (jennifer)
**          Added routines for user, location, dbaccess, and security.
**	29-sep-89 (ralph)
**	    Added calls to qeu_aloc() and qeu_dloc().
**	07-oct-89 (carl)
**	    Modified qef_call to call qel_u3_lock_objbase for CREATE LINK,
**	    CREATE VIEW, DROP VIEW to avoid STAR deadlocks
**	25-oct-89 (neil)
**	    Added event qrymod support for QEU_CEVENT and QEU_DEVENT.
**	15-feb-90 (carol)
**	    Added comment support for QEU_CCOMMENT.
**      22-mar-90 (carl)
**          changed to call qeu_d9_a_copy for QED_A_COPY
**	19-apr-90 (andre)
**	    Added support for QEU_CSYNONYM and QEU_DSYNONYM.
**      06-jun-90 (carl)
**	    changed to introduce QED_SESSCDB
**	14-jun-90 (andre)
**	    Added support for QEU_CTIMESTAMP.
**      12-jul-90 (carl)
**	    replaced all calls to qef_c1_fatal with in-line code
**	12-jul-90 (georgeg-carl) QED_3SCF_CLUSTER_INFO to pass 
**	    the cluster node names from SCF-RDF to RQF.
**      21-aug-90 (carl)
**	    modified qef_call to set QES_04CTL_OK_TO_OUTPUT_QP in the DDB 
**	    portion of the session CB;
**	    added trace point processing code
**      05-oct-90 (carl)
**	    added test to avoid AV when trace vector is unitialized at
**	    session start-up time
**	20-nov-90 (davebf)
**	    Don't reset qef_ostat if qef_id is QEC_INFO
**	24-may-91 (andre)
**	    added support for QEC_RESET_EFF_USER_ID
**	07-aug-91 (fpang)
**	    If request is QEU_{C,D}VIEW, don't assume a valid qer_p when
**	    initilizing qer_p->error.err_code, qer_p->qef_cb, and
**	    qer_p->qef_r3_ddb_req.qer_d13_ctl_info.
**	    Fixes B38882.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added initialization of qef_user_interrupt and
**	    qef_user_interrupt_handled.
**	26-mar-92 (davebf)
**	    Merged for Sybil
**	26-jun-92 (andre)
**	    added support for QEU_DBP_STATUS
**	02-aug-92 (andre)
**	    added support for QEU_REVOKE
**	24-oct-92 (andre)
**	    both ADF_ERROR and SCF_CB have been changed to contain an SQLSTATE
**	    (DB_SQLSTATE) instead of generic_error (i4)
**	20-nov-92 (rickh)
**	    New arguments to qeu_dbu.
**	05-feb-93 (andre)
**	    remove support for QEC_RESET_EFF_USER_ID; resetting the effective
**	    user id will be handled by QEC_ALTER
**	20-feb-93 (andre)
**	    added support for QEU_DSCHEMA (DROP SCHEMA)
**	01-mar-93 (andre)
**	    QEU_CVIEW opcode will no longer be supported.
**	21-mar-93 (andre)
**	    interface of qeu_dintg() has changed - we need to tell it to drop
**	    only "old style" INGRES integrity(ies)
**	25-mar-93 (barbara)
**	    Get qef_cb for QED_IIDBDB.  This opcode now causes Star
**	    to query iidbdb and for this we need a QEF session control block.
**	14-apr-93 (andre)
**	    tell qeu_drule() that it is being called as a part of DROP RULE
**	    processing
**	20-apr-93 (jhahn)
**	    Added code to check to see if transaction statements should be
**	    allowed.
**      06-may-93 (anitap)
**          Added support for QEQ_UPD_ROWCNT.
**	09-jun-93 (rickh)
**	    New arguments to qeu_dbu.
**	16-jun-93 (jhahn)
**	    Fixed problem with returning error when an illegal transaction
**	    statement is attempted.
**	07-jul-93 (rickh)
**	    Prototyped qef_call itself.
**	21-jul-1993 (rog)
**	    EXdeclare() returns STATUS, so test against OK.  Remove unnecessary
**	    FUNC_EXTERNS for functions defined in qefprotos.h.
**	12-oct-1993 (fpang)
**	    If qer_d14_ses_timout is 0 (eq. user sepecified no timeout), set
**	    qes_d10_timeout to -1.
**	    Fixes B53486, "set lockmode session where timout = 0" does not work.
**	26-nov-93 (robf)
**          Add QEU_RAISE_EVENT, QEU_CSECALM, QEU_DSECALM
**	4-mar-94 (robf)
**          Add QEU_ROLEGRANT.
**	10-sep-1997 (nanpr01)
**	    Remove duplicate initialization.
**	19-Oct-2000 (jenjo02)
**	    Added "cb_check" static array to get rid of several
**	    lengthy switch statements.
**	    Replaced calls to qef_session() with GET_QEF_CB macro.
**	21-mar-01 (inkdo01)
**	    Removed ad_errcode check inserted by jenjo in his zeal to remove
**	    all function calls from Ingres. ADF warning counters were adversely
**	    affected.
**	16-jul-2001 (somsa01)
**	    Added the missing setup of the qef_cb if EXdeclare() fails. This
**	    was accidentally removed by a previous change.
**	17-Aug-2001 (devjo01)
**	    Force compiler to not put qef_cb into a register.  This
**	    fixes b105551 in which user numeric overflows would
**	    become FATAL errors on Tru64.
**	01-nov-2001 (somsa01)
**	    Initialize "status" and "local_status" to OK.
**	11-Feb-2004 (schka24)
**	    Revision to dbu call.
**	21-Sep-2009 (thaju02) B122534
**	    Check qef_id rather than qef_cb->qef_rcb->qef_operation,
**	    to determine if adu_free_locator() should be called in 
**	    the case of qef_call(QET_SCOMMIT).
*/
DB_STATUS
qef_call(
i4	    qef_id,
void *	    rcb )
{
    DB_STATUS	    status = OK;
    DB_STATUS	    loc_status = OK;	/* Secondary status */
    i4		    was_xact;		/* There was a transaction */
    i4	    err;
    DB_ERROR	    err_blk;
    ERR_CB	    err_cb;
    PTR		    save1;
    PTR		    save2;
    EX_CONTEXT	    ex_context;

    bool	    ddb_b,
                    trace_qef_58,
                    trace_err_59;
    QES_DDB_SES	    *dds_p;
    QEF_RCB	    lock_rcb;	
    i4	    	    i4_1, i4_2;

    QEF_CB	    *qef_cb = (QEF_CB*)NULL;
    QEF_CB	    **hqef_cb = &qef_cb;
    QEU_CB	    *qeu_cb;
    QEF_RCB	    *qef_rcb;
    QEUQ_CB	    *qeuq_cb;
    QEF_SRCB	    *qef_srcb;
#ifdef PERF_TEST
    CSfac_stats(TRUE, CS_QEF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(TRUE, CS_QEF_ID, qef_id, rcb);
#endif

    if (EXdeclare(qef_handler, &ex_context) != OK)
    {
	EXdelete();

	/* get the QEF session control block (QEF_CB) from SCF */
	qef_rcb = (QEF_RCB*)rcb;
	qef_cb = qef_rcb->qef_cb = GET_QEF_CB(qef_rcb->qef_sess_id);

#ifdef xDEBUG
	status = E_DB_ERROR;
#else
	status = E_DB_FATAL;
#endif

	/* Do we have a session control block? */
	if (qef_cb = *hqef_cb)
	{
	    /* abort the query/transaction */
	    if (qef_cb->qef_stat != QEF_NOTRAN)
	    {
		DB_SP_NAME		spoint;

		qef_rcb = (QEF_RCB*)rcb;
		save1 = (PTR)qef_cb->qef_rcb;
		save2 = qef_cb->qef2_rcb;
		qef_cb->qef_rcb = qef_rcb;
		qef_cb->qef2_rcb = (PTR)&err_cb;

		if (qef_cb->qef_stat == QEF_MSTRAN)
		{
		    qef_rcb->qef_spoint = &spoint;
		    MEmove(QEF_PS_SZ, (PTR) QEF_SP_SAVEPOINT, (char) ' ',
			sizeof(DB_SP_NAME),
			(PTR) qef_rcb->qef_spoint->db_sp_name);
		}
		else
		{
		    qef_rcb->qef_spoint = (DB_SP_NAME *)NULL;
		}

		(VOID) qet_abort(qef_cb);
		qef_cb->qef_abort = FALSE;

		/* clean up */
		(VOID)qee_cleanup(qef_rcb, (QEE_DSH **) NULL);

		qef_cb->qef_rcb = (QEF_RCB*)save1;
		qef_cb->qef2_rcb = save2;
	    }

	    if (qef_cb->qef_ade_abort)
	    {
		qef_cb->qef_ade_abort = FALSE;
		status = E_DB_ERROR;
	    }

	}
	return(status);
    }

    /*
    ** Get QEF session control block (qef_cb).
    */
    if ( qef_id < QEF_LO_MAX_OP )
    {
	if ( cb_check[qef_id].cb_type == QEFRCB_CB )
	{
	    qef_rcb = (QEF_RCB*)rcb;
	    qef_cb = qef_rcb->qef_cb = GET_QEF_CB(qef_rcb->qef_sess_id);
	    err_cb.err_eflag = qef_rcb->qef_eflag;
	    err_cb.err_block = &qef_rcb->error;
	}
	else if ( cb_check[qef_id].cb_type == QEUCB_CB )
	{
	    qeu_cb = (QEU_CB*)rcb;
	    qef_cb = GET_QEF_CB(qeu_cb->qeu_d_id);
	    err_cb.err_eflag = qeu_cb->qeu_eflag;
	    err_cb.err_block = &qeu_cb->error;
	}
	else if ( cb_check[qef_id].cb_type == QEUQCB_CB )
	{
	    qeuq_cb = (QEUQ_CB*)rcb;
	    qef_cb = GET_QEF_CB(qeuq_cb->qeuq_d_id);
	    err_cb.err_eflag = qeuq_cb->qeuq_eflag;
	    err_cb.err_block = &qeuq_cb->error;
	}
	else if ( cb_check[qef_id].cb_type == QESRCB_CB )
	{
	    qef_srcb = (QEF_SRCB*)rcb;
	    err_cb.err_eflag = qef_srcb->qef_eflag;
	    err_cb.err_block = &qef_srcb->error;
	}
	else if ( qef_id == QEC_DEBUG )
	{
	    err_cb.err_block = NULL;
	}
	else
	{
	    qef_error(E_QE001F_INVALID_REQUEST, 0L, E_DB_FATAL, 
		&err, &err_blk, 0);
	    EXdelete();
	    return(E_DB_FATAL);
	}
    }
    else if ( qef_id >= QED_IIDBDB && qef_id <= QEF_HI_MAX_OP )
    {
	/* all DDB operations use RCB */
	qef_rcb = (QEF_RCB*)rcb;
	qef_cb = GET_QEF_CB(qef_rcb->qef_sess_id);
	err_cb.err_eflag = qef_rcb->qef_eflag;
	err_cb.err_block = &qef_rcb->error;
        qef_rcb->error.err_code = 0;
        qef_rcb->qef_cb = qef_cb;		/* must set */
        qef_rcb->qef_r3_ddb_req.qer_d13_ctl_info = QEF_00DD_NIL_INFO;
    }
    else
    {
	qef_error(E_QE001F_INVALID_REQUEST, 0L, E_DB_FATAL, 
	    &err, &err_blk, 0);
	EXdelete();
	return(E_DB_FATAL);
    }

    /* Only QEC_DEBUG will be missing a pointer to some DB_ERROR */
    if ( err_cb.err_block )
	CLRDBERR(err_cb.err_block);

    /*
    ** Replace the session request blocks with the caller's request blocks.
    ** Save the original ones in the stack.  Also, set the operation id
    ** for error mapping.
    */
    if (qef_cb)
    {
	save1 = (PTR)qef_cb->qef_rcb;
	save2 = qef_cb->qef2_rcb;
	qef_cb->qef_rcb = (QEF_RCB *) rcb;
	qef_cb->qef2_rcb	= (PTR)&err_cb;
	qef_cb->qef_operation   = qef_id;

	/*initialize user interrupt variables. */
	qef_cb->qef_user_interrupt = FALSE;
	qef_cb->qef_user_interrupt_handled = FALSE;

	if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	{
	    ddb_b = TRUE;
	    qef_cb->qef_c2_ddb_ses.qes_d9_ctl_info |= 
		QES_04CTL_OK_TO_OUTPUT_QP;

	    if (qef_id > QEC_BEGIN_SESSION)
	    {
		/* extract trace points if set */

		if (ult_check_macro(& qef_cb->qef_trace, 
		    QEF_TRACE_DDB_LOG_QEF_58,
		    & i4_1, & i4_2))
		    trace_qef_58 = TRUE;
		else
		    trace_qef_58 = FALSE;

		if (ult_check_macro(& qef_cb->qef_trace,    
		    QEF_TRACE_DDB_LOG_ERR_59,
		    & i4_1, & i4_2))
		    trace_err_59 = TRUE;
		else
		    trace_err_59 = FALSE;

		if (trace_qef_58)
		    qed_u15_trace_qefcall(qef_cb, qef_id);

	    }
	}
	else
	    ddb_b = FALSE;


#ifdef xDEBUG
	if(ddb_b)
	{
	    switch (qef_id)	/* valid for distributed */
	    {
		case QEC_BEGIN_SESSION:
		case QEC_ALTER:		
		case QEC_END_SESSION:	
		case QEC_INFO:
		case QET_ROLLBACK:
		case QET_ABORT:		
		case QET_BEGIN:		
		case QET_SCOMMIT:
		case QET_COMMIT:
		case QET_SAVEPOINT:
		case QET_AUTOCOMMIT:
		case QEU_BTRAN:	
		case QEU_ETRAN:	
		case QEU_ATRAN:	
		case QEU_DBU:	
		case QEU_DVIEW:	
		case QEQ_ENDRET:
		case QEQ_QUERY:	
		case QEQ_OPEN:	
		case QEQ_CLOSE:	
		case QEQ_FETCH:	
		case QEQ_DELETE: 
		case QEQ_REPLACE:
		case QED_IIDBDB:
		case QED_DDBINFO:
		case QED_LDBPLUS:
		case QED_USRSTAT:
		case QED_DESCPTR:
		case QED_SESSCDB:
		case QED_C1_CONN:
		case QED_C2_CONT:
		case QED_C3_DISC:
		case QED_1RDF_QUERY:
		case QED_2RDF_FETCH:
		case QED_3RDF_FLUSH:
		case QED_4RDF_VDATA:
		case QED_5RDF_BEGIN:
		case QED_6RDF_END:
		case QED_7RDF_DIFF_ARCH:
		case QED_EXEC_IMM:
		case QED_A_COPY:
		case QED_CLINK:
		case QED_1PSF_RANGE:
		case QED_SET_FOR_RQF:
		case QED_1SCF_MODIFY:
		case QED_2SCF_TERM_ASSOC:
		case QED_3SCF_CLUSTER_INFO:
		case QED_B_COPY:
		case QED_E_COPY:
		case QED_CONCURRENCY:
		case QED_SES_TIMEOUT:
		case QED_QRY_FOR_LDB:
		case QED_RECOVER:
		case QED_P1_RECOVER:
		case QED_P2_RECOVER:
		case QED_IS_DDB_OPEN:
		case QET_XA_PREPARE:
		case QET_XA_COMMIT:
		case QET_XA_ROLLBACK:
		case QET_XA_START:
		case QET_XA_END:
		    break;
		default:
		    qef_error(E_QE001F_INVALID_REQUEST, 0L, E_DB_FATAL, 
				&err, &err_blk, 0);
		    EXdelete();
		    return(E_DB_FATAL);
		    break;
	    }
	}
	else
	{

/* be sure any new local request codes get added to this list */

	    switch (qef_id)	/* valid for local */
	    {
		case QEC_BEGIN_SESSION:
		case QEC_ALTER:
		case QEC_END_SESSION:
		case QEC_INFO:
		case QET_ROLLBACK:
		case QET_ABORT:
		case QET_BEGIN:
		case QET_RESUME:
		case QET_SECURE:
		case QET_SCOMMIT:
		case QET_COMMIT:
		case QET_SAVEPOINT:
		case QET_AUTOCOMMIT:
		case QEU_ETRAN:
		case QEU_ATRAN:
		case QEU_BTRAN:
		case QEU_OPEN:
		case QEU_CLOSE:
		case QEU_APPEND:
		case QEU_GET:
		case QEU_DELETE:
		case QEU_DBU:
		case QEU_QUERY:
		case QEU_DVIEW:
		case QEU_DPROT:
		case QEU_REVOKE:
		case QEU_CPROT:
		case QEU_DINTG:
		case QEU_CINTG:
		case QEU_DSTAT:
		case QEU_DRPDBP:
		case QEU_CREDBP:
		case QEU_DBP_STATUS:
		case QEU_CRULE:
		case QEU_DRULE:
		case QEU_CEVENT:
		case QEU_DEVENT:
		case QEU_CCOMMENT:
		case QEU_CSYNONYM:
		case QEU_DSYNONYM:
		case QEU_DSCHEMA:
		case QEU_B_COPY:
		case QEU_E_COPY:
		case QEU_R_COPY:
		case QEU_W_COPY:
		case QEQ_ENDRET:
		case QEQ_REPLACE:
		case QEQ_DELETE:
		case QEQ_QUERY:
		case QEQ_OPEN:
		case QEQ_CLOSE:
		case QEQ_FETCH:
		case QEQ_UPD_ROWCNT:
		case QEU_GROUP:
		case QEU_APLID:
		case QEU_DBPRIV:
		case QEU_CUSER:
		case QEU_AUSER:
		case QEU_DUSER:
		case QEU_CLOC:
		case QEU_ALOC:
		case QEU_DLOC:
		case QEU_CDBACC:
		case QEU_DDBACC:
		case QEU_MSEC:
		case QEU_CPROFILE:
		case QEU_APROFILE:
		case QEU_DPROFILE:
		case QEU_ROLEGRANT:
		case QEU_CSECALM:
		case QEU_DSECALM:
		case QEU_RAISE_EVENT:
		    break;
		default:
		    qef_error(E_QE001F_INVALID_REQUEST, 0L, E_DB_FATAL, 
				&err, &err_blk, 0);
		    EXdelete();
		    return(E_DB_FATAL);
		    break;
	    }
	}
#endif


    }	/* end if qef_cb */
    
    /*
    ** Dispatch QEF requests
    */
    switch (qef_id)
    {
	case QEC_INITIALIZE:
	    status = qec_initialize(qef_srcb);
	    break;

	case QEC_BEGIN_SESSION:
	    status = qec_begin_session(qef_cb);
	    break;

	case QEC_DEBUG:
	    status = qec_debug((QEF_DBG*) rcb);
	    break;

	case QEC_ALTER:
	    status = qec_alter(qef_cb);
	    break;

	case QEC_END_SESSION:
	    status = qec_end_session(qef_cb);
	    break;

	case QEC_SHUTDOWN:
	    status = qec_shutdown(qef_srcb);
	    break;

	case QEC_INFO:
	    status = qec_info(qef_cb);
	    break;

	case QET_ROLLBACK:
	case QET_ABORT:
	    /* Disallow rollbacks when qef_noxact_stmt is set and a savepoint
	    ** is not specified*/
	    if ((!qef_cb->qef_rcb->qef_spoint) &&
		qef_cb->qef_rcb->qef_no_xact_stmt)
	    {
		qef_cb->qef_rcb->qef_illegal_xact_stmt = TRUE;
		status = E_DB_ERROR;
		qef_error(E_QE0258_ILLEGAL_XACT_STMT, 0L, status, 
			  &err, &qef_cb->qef_rcb->error, 0);
		break;
	    }

	    /* Free locators at explicit rollback */
	    if (!(qef_cb->qef_c1_distrib & DB_3_DDB_SESS))
		adu_free_locator(qef_cb->qef_rcb->qef_sess_id, (DB_DATA_VALUE *)NULL);

	    was_xact = qef_cb->qef_stat != QEF_NOTRAN;
	    qef_cb->qef_rcb->qef_operation = qef_id;
	    status = qet_abort(qef_cb);
	    if (was_xact && qef_cb->qef_stat == QEF_NOTRAN) /* Close cursors */
	    {
		loc_status = qee_cleanup(qef_cb->qef_rcb, (QEE_DSH **)NULL);
		if (loc_status > status)
		    status = loc_status;
	    }
	    break;

	case QET_BEGIN:
	    status = qet_begin(qef_cb);
	    break;

	case QET_RESUME:
	    status = qet_resume(qef_cb);
	    break;

	case QET_SECURE:
	    if (qef_cb->qef_rcb->qef_no_xact_stmt)
	    {
		qef_cb->qef_rcb->qef_illegal_xact_stmt = TRUE;
		status = E_DB_ERROR;
		qef_error(E_QE0258_ILLEGAL_XACT_STMT, 0L, status, 
			  &err, &qef_cb->qef_rcb->error, 0);
		break;
	    }
	    was_xact = qef_cb->qef_stat != QEF_NOTRAN;
	    status = qet_secure(qef_cb);
	    if (was_xact && qef_cb->qef_stat == QEF_NOTRAN) /* Close cursors */
	    {
		loc_status = qee_cleanup(qef_cb->qef_rcb, (QEE_DSH **)NULL);
		if (loc_status > status)
		    status = loc_status;
	    }
	    break;

	case QET_SCOMMIT:
	case QET_COMMIT:
	    if (qef_cb->qef_rcb->qef_no_xact_stmt)
	    {
		qef_cb->qef_rcb->qef_illegal_xact_stmt = TRUE;
		status = E_DB_ERROR;
		qef_error(E_QE0258_ILLEGAL_XACT_STMT, 0L, status, 
			  &err, &qef_cb->qef_rcb->error, 0);
		break;
	    }

	    /* Free locators at explicit commit */
	    if ((qef_id == QET_SCOMMIT) &&
		!(qef_cb->qef_c1_distrib & DB_3_DDB_SESS))
	    {
		adu_free_locator(qef_cb->qef_rcb->qef_sess_id, (DB_DATA_VALUE *)NULL);
	    }

	    was_xact = qef_cb->qef_stat != QEF_NOTRAN;
	    qef_cb->qef_rcb->qef_operation = qef_id;
	    status = qet_commit(qef_cb);
	    if (was_xact && qef_cb->qef_stat == QEF_NOTRAN) /* Close cursors */
	    {
		loc_status = qee_cleanup(qef_cb->qef_rcb, (QEE_DSH **)NULL);
		if (loc_status > status)
		    status = loc_status;
	    }
	    break;

	case QET_SAVEPOINT:
	    status = qet_savepoint(qef_cb);
	    break;

	case QET_AUTOCOMMIT:
	    if (qef_cb->qef_rcb->qef_no_xact_stmt)
	    {
		qef_cb->qef_rcb->qef_illegal_xact_stmt = TRUE;
		status = E_DB_ERROR;
		qef_error(E_QE0258_ILLEGAL_XACT_STMT, 0L, status, 
			  &err, &qef_cb->qef_rcb->error, 0);
		break;
	    }
	    status = qet_autocommit(qef_cb);
	    break;

	case QEU_ETRAN:
	    status = qeu_etran(qef_cb, qeu_cb);
	    break;

	case QEU_ATRAN:
	    status = qeu_atran(qef_cb, qeu_cb);
	    break;

	case QEU_BTRAN:
	    status = qeu_btran(qef_cb, qeu_cb);
	    break;

	case QEU_OPEN:
	    status = qeu_open(qef_cb, qeu_cb);
	    break;

	case QEU_CLOSE:
	    status = qeu_close(qef_cb, qeu_cb);
	    break;

	case QEU_APPEND:
	    status = qeu_append(qef_cb, qeu_cb);
	    break;

	case QEU_GET:
	    status = qeu_get(qef_cb, qeu_cb);
	    break;

	case QEU_DELETE:
	    status = qeu_delete(qef_cb, qeu_cb);
	    break;

	case QEU_DBU:
	    /* perform the dbu with transaction support */
	    status = qeu_dbu( qef_cb, qeu_cb, 0, ( DB_TAB_ID * ) NULL,
		&qeu_cb->qeu_count, 
		err_cb.err_block, ( i4  ) QEF_EXTERNAL );
	    break;

	case QEU_QUERY:
	    status = qeu_query(qef_cb, qeu_cb);
	    break;

	case QEU_DVIEW:
	    if(ddb_b)
	    {
		MEfill(sizeof(lock_rcb), '\0', (PTR) & lock_rcb);
		lock_rcb.qef_cb = qef_cb;

		/* lock to synchronize */

		status = qel_u3_lock_objbase(& lock_rcb);
		if (status)
		{
	    	    STRUCT_ASSIGN_MACRO(lock_rcb.error, 
			qeuq_cb->error);
		    break;
		}
		status = qeu_d4_des_view(qef_cb, qeuq_cb);
	    }
	    else
		status = qeu_dview(qef_cb, qeuq_cb);
	    break;

	case QEU_DPROT:
	    status = qeu_dprot(qef_cb, qeuq_cb);
	    break;

	case QEU_REVOKE:
	    status = qeu_revoke(qef_cb, qeuq_cb);
	    break;

	case QEU_CPROT:
	    status = qeu_cprot(qef_cb, qeuq_cb);
	    break;

	case QEU_DINTG:
	    status = qeu_dintg(qef_cb, qeuq_cb, (bool) TRUE,
		(bool) FALSE);
	    break;

	case QEU_CINTG:
	    status = qeu_cintg(qef_cb, qeuq_cb);
	    break;

	case QEU_DSTAT:
	    status = qeu_dstat(qef_cb, qeuq_cb);
	    break;

	case QEU_DRPDBP:
	    status = qeu_drpdbp(qef_cb, qeuq_cb);
	    break;

	case QEU_CREDBP:
	    status = qeu_credbp(qef_cb, qeuq_cb);
	    break;

	case QEU_DBP_STATUS:
	    status = qeu_dbp_status(qef_cb, qeuq_cb);
	    break;

	case QEU_CRULE:		/* Cruel */
	    status = qeu_crule(qef_cb, qeuq_cb);
	    break;

	case QEU_DRULE:		/* Drool */
	    status = qeu_drule(qef_cb, qeuq_cb, (bool) TRUE);
	    break;

	case QEU_CEVENT:
	    status = qeu_cevent(qef_cb, qeuq_cb);
	    break;

	case QEU_DEVENT:
	    status = qeu_devent(qef_cb, qeuq_cb);
	    break;

	case QEU_CCOMMENT:
	    status = qeu_ccomment(qef_cb, qeuq_cb);
	    break;

	case QEU_CSYNONYM:
	    status = qeu_create_synonym(qef_cb, qeuq_cb);
	    break;

	case QEU_DSYNONYM:
	    status = qeu_drop_synonym(qef_cb, qeuq_cb);
	    break;

	case QEU_DSCHEMA:
	    status = qeu_dschema(qef_cb, qeuq_cb);
	    break;

	case QEU_B_COPY:
	    status = qeu_b_copy(qef_rcb);
	    break;

	case QEU_E_COPY:
	    status = qeu_e_copy(qef_rcb);
	    break;

	case QEU_R_COPY:
	    status = qeu_r_copy(qef_rcb);
	    break;

	case QEU_W_COPY:
	    status = qeu_w_copy(qef_rcb);
	    break;

	case QEQ_ENDRET:
	    status = qeq_endret(qef_rcb);
	    break;

	case QEQ_REPLACE:
	case QEQ_DELETE:
	case QEQ_QUERY:
	    status = qeq_query(qef_rcb, qef_id);
	    if(ddb_b &&
		qef_id == QEQ_QUERY &&
	        status == E_DB_WARN &&
		qef_rcb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/* end of GET condition */

		dds_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;/* initialize */
		dds_p->qes_d9_ctl_info &= ~QES_03CTL_READING_TUPLES;
	    }
	    break;

	case QEQ_OPEN:
	    status = qeq_open(qef_rcb);
	    break;

	case QEQ_CLOSE:
	    status = qeq_close(qef_rcb);
	    break;

	case QEQ_FETCH:
	    status = qeq_fetch(qef_rcb);
	    break;

	case QEQ_UPD_ROWCNT:
            status = qeq_updrow(qef_cb);
            break;

	case QEU_GROUP:
	    status = qeu_group(qef_cb, qeuq_cb);
	    break;

	case QEU_APLID:
	    status = qeu_aplid(qef_cb, qeuq_cb);
	    break;

	case QEU_DBPRIV:
	    status = qeu_dbpriv(qef_cb, qeuq_cb);
	    break;

	case QEU_ROLEGRANT:
	    status = qeu_rolegrant(qef_cb, qeuq_cb);
	    break;

	case QEU_CUSER:
	    status = qeu_cuser(qef_cb, qeuq_cb);
	    break;

	case QEU_AUSER:
	    status = qeu_auser(qef_cb, qeuq_cb);
	    break;

	case QEU_DUSER:
	    status = qeu_duser(qef_cb, qeuq_cb);
	    break;

	case QEU_CLOC:
	    status = qeu_cloc(qef_cb, qeuq_cb);
	    break;

	case QEU_ALOC:
	    status = qeu_aloc(qef_cb, qeuq_cb);
	    break;

	case QEU_DLOC:
	    status = qeu_dloc(qef_cb, qeuq_cb);
	    break;

	case QEU_CDBACC:
	    status = qeu_cdbacc(qef_cb, qeuq_cb);
	    break;

	case QEU_DDBACC:
	    status = qeu_ddbacc(qef_cb, qeuq_cb);
	    break;

	case QEU_MSEC:
	    status = qeu_msec(qef_cb, qeuq_cb);
	    break;

	case QEU_CPROFILE:
	    status = qeu_cprofile(qef_cb, qeuq_cb);
	    break;

	case QEU_APROFILE:
	    status = qeu_aprofile(qef_cb, qeuq_cb);
	    break;

	case QEU_DPROFILE:
	    status = qeu_dprofile(qef_cb, qeuq_cb);
	    break;

	case QEU_CSECALM:
	    status = qeu_csecalarm(qef_cb, qeuq_cb);
	    break;

	case QEU_DSECALM:
	    status = qeu_dsecalarm(qef_cb, qeuq_cb, TRUE);
	    break;

	case QEU_RAISE_EVENT:
	    status = qeu_evraise(qef_cb, qef_rcb);
	    break;

	case QEU_CSEQ:
	    status = qeu_cseq(qef_cb, qeuq_cb);
	    break;

	case QEU_ASEQ:
	    status = qeu_aseq(qef_cb, qeuq_cb);
	    break;

	case QEU_DSEQ:
	    status = qeu_dseq(qef_cb, qeuq_cb);
	    break;

	/* pure DDB operations start here */

	case QED_IIDBDB:
	    status = qed_s1_iidbdb(qef_rcb);
	    break;
	case QED_DDBINFO:
	    status = qed_s2_ddbinfo(qef_rcb);
	    break;
	case QED_LDBPLUS:
	    status = qed_s3_ldbplus(qef_rcb);
	    break;
	case QED_USRSTAT:
	    status = qed_s4_usrstat(qef_rcb);
	    break;
	case QED_DESCPTR:
	    qef_cb->qef_c2_ddb_ses.qes_d4_ddb_p = 
		qef_rcb->qef_r3_ddb_req.qer_d1_ddb_p;
	    status = qed_s0_sesscdb(qef_rcb);
	    break;
	case QED_SESSCDB:
	    status = qed_s0_sesscdb(qef_rcb);
	    break;

	case QED_C1_CONN:
	    status = qed_c1_conn(qef_rcb);
	    break;
	case QED_C2_CONT:
	    status = qed_c2_cont(qef_rcb);
	    break;
	case QED_C3_DISC:
	    status = qed_c3_disc(qef_rcb);
	    break;
	case QED_1RDF_QUERY:
	    status = qed_r1_query(qef_rcb);
	    break;
	case QED_2RDF_FETCH:
	    status = qed_r2_fetch(qef_rcb);
	    break;
	case QED_3RDF_FLUSH:
	    status = qed_r3_flush(qef_rcb);
	    break;
	case QED_4RDF_VDATA:
	    status = qed_r4_vdata(qef_rcb);
	    break;
	case QED_5RDF_BEGIN:
	    status = qed_r5_begin(qef_rcb);
	    break;
	case QED_6RDF_END:
	    status = qed_r6_commit(qef_rcb);
	    break;
	case QED_7RDF_DIFF_ARCH:
	    status = qed_r8_diff_arch(qef_rcb);
	    break;
	case QED_EXEC_IMM:
	    status = qed_e1_exec_imm(qef_rcb);
	    break;
	case QED_A_COPY:			/* abort due to COPY error */
	    status = qeu_d9_a_copy(qef_rcb);
	    break;
	case QED_CLINK:
	    /* lock to synchronize */

	    status = qel_u3_lock_objbase(qef_rcb);
	    if (status)
	    {
		break;
	    }
	    status = qel_e0_cre_lnk(qef_rcb);
	    break;
	case QED_1PSF_RANGE:
	    status = qed_p1_range(qef_rcb);
	    break;
	case QED_SET_FOR_RQF:
	    status = qed_p2_set(qef_rcb);
	    break;
	case QED_1SCF_MODIFY:
	    status = qed_s6_modify(qef_rcb);
	    break;
	case QED_2SCF_TERM_ASSOC:
	    status = qed_s7_term_assoc(qef_rcb);
	    break;
	case QED_3SCF_CLUSTER_INFO:
	    status = qed_s8_cluster_info(qef_rcb);
	    break;
	case QED_B_COPY:
	    status = qeu_d7_b_copy(qef_rcb);
	    break;
	case QED_E_COPY:
	    status = qeu_d8_e_copy(qef_rcb);
	    break;
	case QED_CONCURRENCY:
	    status = qet_t6_ddl_cc(qef_rcb);
	    break;
	case QED_SES_TIMEOUT:
	    {
		QEF_DDB_REQ *ddr2_p = & qef_rcb->qef_r3_ddb_req;
		QES_DDB_SES *dds2_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;


		dds2_p->qes_d10_timeout = 
		    (ddr2_p->qer_d14_ses_timeout == 0) ? -1 :
						    ddr2_p->qer_d14_ses_timeout;
	    }
	    break;
	case QED_QRY_FOR_LDB:
	    status = qed_e4_exec_qry(qef_rcb);
	    break;
	case QED_RECOVER:
	    status = qet_t20_recover(qef_rcb);
	    break;
	case QED_P1_RECOVER:
	    status = qet_t7_p1_recover(qef_rcb);
	    break;
	case QED_P2_RECOVER:
	    status = qet_t8_p2_recover(qef_rcb);
	    break;
	case QED_IS_DDB_OPEN:
	    status = qet_t10_is_ddb_open(qef_rcb);
	    break;

	case QET_XA_START:
	    status = qet_xa_start(qef_cb);
	    break;

	case QET_XA_END:
	    status = qet_xa_end(qef_cb);
	    break;

	case QET_XA_PREPARE:
	    status = qet_xa_prepare(qef_cb);
	    break;

	case QET_XA_COMMIT:
	    status = qet_xa_commit(qef_cb);
	    break;

	case QET_XA_ROLLBACK:
	    status = qet_xa_rollback(qef_cb);
	    break;

	/* DDB operations end here */

	default:
	    qef_error(E_QE001F_INVALID_REQUEST, 0L, E_DB_FATAL, 
		&err, &err_blk, 0);
	    status = E_DB_FATAL;
	    break;
    }

    if ( qef_cb && qef_id != QEC_INFO )
	/* preserve XACT status as of statement end */
	qef_cb->qef_ostat = qef_cb->qef_stat;

    if (ddb_b)
    {
	/*  handle DDB termination condition here */

	if (DB_SUCCESS_MACRO(status))
	{
	    bool	qer_safe = FALSE;   /* assume not safe to use qef_rcb */

	    switch (qef_id)			/* operations common to DDB */
	    {
	    case QEQ_ENDRET:
	    case QEQ_OPEN:
	    case QEQ_CLOSE:
	    case QEQ_FETCH:
	    case QEQ_DELETE:
	    case QEQ_REPLACE:
		dds_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;	/* initialize */
		dds_p->qes_d9_ctl_info &= ~QES_03CTL_READING_TUPLES;

		/* fall thru */

	    case QEQ_QUERY:
		dds_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;	/* initialize */
		qer_safe = TRUE;
		break;

	    default:
		break;
	    }

	    if (qer_safe)				
	    {
		if (qef_rcb->qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
		{
		    /* a DDB operation */
	    
		    if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON 
			&&
			dds_p->qes_d9_ctl_info & QES_02CTL_COMMIT_CDB_TRAFFIC
			&&
			qef_rcb->qef_cb->qef_auto != QEF_ON
			&&
			! (dds_p->qes_d9_ctl_info & QES_03CTL_READING_TUPLES))
		    {
			/* commit the privileged CDB association if necessary */

			DB_STATUS	tmp_sts;
			DB_ERROR	tmp_err;
			i4		rowcnt = qef_rcb->qef_rowcount;


			/* save error code */

			tmp_err.err_code = qef_rcb->error.err_code;
		    
			tmp_sts = qed_u11_ddl_commit(qef_rcb);
			if (tmp_sts)
			{
			    qed_u10_trap();
			    status = tmp_sts;
			}
			else
			{
			    /* must restore state */

			    qef_rcb->error.err_code = tmp_err.err_code;
			    qef_rcb->qef_rowcount = rowcnt;			
			}
		    }
		}
	    }
	}

	/* a DDB operation error ? */
	if ( status && qef_id >= QED_IIDBDB )
	{
	    i4	    ignore;


	    dds_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;/* initialize */
	    dds_p->qes_d6_rdf_state = QES_0ST_NONE;	/* reinitialize */

	    /* yes, call qef_error to generate error message */

	    qef_error(qef_rcb->error.err_code, 
			  0L, E_DB_ERROR, 
			  & ignore, & qef_rcb->error, 0);
	}
    }	/* end if ddb_b */

    /*
    ** Report warnings for successful operations.  If we are not doing
    ** a QEQ_QUERY, just report the warnings.  If we are doing QEQ_QUERY,
    ** don't report the warnings until we know that the query is done.
    ** This is necessary because QEF may be called several times for the
    ** same retrieve query.  If there is no data segment header pointer,
    ** then a QEQ_QUERY operation is finished.
    */
    else if ( qef_cb && DB_SUCCESS_MACRO(status) &&
	    (qef_id != QEQ_QUERY || qef_cb->qef_dsh == (PTR)NULL) )
    {
	/* Call ADX_CHKWARN to see if there are any warnings 
	** from query processing
	*/
	while (adx_chkwarn(qef_cb->qef_adf_cb) != E_DB_OK)
	{
	    if (err_cb.err_eflag == QEF_EXTERNAL)
	    {
		SCF_CB		scf_cb;

		scf_cb.scf_facility = DB_QEF_ID;
		scf_cb.scf_session = qef_cb->qef_ses_id;
		scf_cb.scf_length = sizeof(SCF_CB);
		scf_cb.scf_type = SCF_CB_TYPE;
		scf_cb.scf_ptr_union.scf_buffer  = 
		    qef_cb->qef_adf_cb->adf_errcb.ad_errmsgp;
		scf_cb.scf_len_union.scf_blength = 
		    qef_cb->qef_adf_cb->adf_errcb.ad_emsglen;
		scf_cb.scf_nbr_union.scf_local_error = 
		    qef_cb->qef_adf_cb->adf_errcb.ad_usererr;
		STRUCT_ASSIGN_MACRO(qef_cb->qef_adf_cb->adf_errcb.ad_sqlstate,
				    scf_cb.scf_aux_union.scf_sqlstate);
		    
		(VOID) scf_call(SCC_ERROR, &scf_cb);
		/* if we already have a warning message, don't change it */
		if (status == E_DB_OK)
		    err_cb.err_block->err_code = E_QE0025_USER_ERROR;
	    }
	    else
	    {
		err_cb.err_block->err_code = 
		    qef_cb->qef_adf_cb->adf_errcb.ad_usererr;
	    }
	    status = E_DB_WARN;
	} 
    }	/* end else ddb_b */


    if (status == E_DB_OK)
	err_cb.err_block->err_code = E_QE0000_OK;

#ifdef xDEBUG
    if (DB_FAILURE_MACRO(status))
    {
	i4	facility_code;

	facility_code = (err_cb.err_block->err_code >> 16);
	if (facility_code != DB_QEF_ID && facility_code != DB_DMF_ID)
	{
	    TRdisplay("QEF debug: Illegal error code detected in QEF\n");
	    TRdisplay(" error code = %x, QEF operation ",
		err_cb.err_block->err_code);
	    switch (qef_id)
	    {
		case QEC_INITIALIZE:
		    TRdisplay("QEC_INITIALIZE\n");
		    break;
		case QEC_BEGIN_SESSION:
		    TRdisplay("QEC_BEGIN_SESSION\n");
		    break;
		case QEC_DEBUG:
		    TRdisplay("QEC_DEBUG\n");
		    break;
		case QEC_ALTER:
		    TRdisplay("QEC_ALTER\n");
		    break;
		case QEC_END_SESSION:
		    TRdisplay("QEC_END_SESSION\n");
		    break;
		case QEC_SHUTDOWN:
		    TRdisplay("QEC_SHUTDOWN\n");
		    break;
		case QEC_INFO:
		    TRdisplay("QEC_INFO\n");
		    break;
		case QET_ROLLBACK:
		    TRdisplay("QET_ROLLBACK\n");
		    break;
		case QET_ABORT:
		    TRdisplay("QET_ABORT\n");
		    break;
		case QET_BEGIN:
		    TRdisplay("QET_BEGIN\n");
		    break;
		case QET_SCOMMIT:
		    TRdisplay("QET_SCOMMIT\n");
		    break;
		case QET_COMMIT:
		    TRdisplay("QET_COMMIT\n");
		    break;
		case QET_SECURE:
		    TRdisplay("QET_SECURE\n");
		    break;
		case QET_RESUME:
		    TRdisplay("QET_RESUME\n");
		    break;
		case QET_SAVEPOINT:
		    TRdisplay("QET_SAVEPOINT\n");
		    break;
		case QET_AUTOCOMMIT:
		    TRdisplay("QET_AUTOCOMMIT\n");
		    break;
		case QEU_ETRAN:
		    TRdisplay("QEU_ETRAN\n");
		    break;
		case QEU_ATRAN:
		    TRdisplay("QEU_ATRAN\n");
		    break;
		case QEU_BTRAN:
		    TRdisplay("QEU_BTRAN\n");
		    break;
		case QEU_OPEN:
		    TRdisplay("QEU_OPEN\n");
		    break;
		case QEU_CLOSE:
		    TRdisplay("QEU_CLOSE\n");
		    break;
		case QEU_APPEND:
		    TRdisplay("QEU_APPEND\n");
		    break;
		case QEU_GET:
		    TRdisplay("QEU_GET\n");
		    break;
		case QEU_DELETE:
		    TRdisplay("QEU_DELETE\n");
		    break;
		case QEU_DBU:
		    TRdisplay("QEU_DBU\n");
		    break;
		case QEU_QUERY:
		    TRdisplay("QEY_QUERY\n");
		    break;
		case QEU_DVIEW:
		    TRdisplay("QEU_DVIEW\n");
		    break;
		case QEU_DPROT:
		    TRdisplay("QEU_DPROT\n");
		    break;
		case QEU_CPROT:
		    TRdisplay("QEU_CPROT\n");
		    break;
		case QEU_REVOKE:
		    TRdisplay("QEU_REVOKE\n");
		    break;
		case QEU_DINTG:
		    TRdisplay("QEU_DINTG\n");
		    break;
		case QEU_CINTG:
		    TRdisplay("QEU_CINTG\n");
		    break;
		case QEU_DBP_STATUS:
		    TRdisplay("QEU_DBP_STATUS\n");
		    break;
		case QEU_CRULE:
		    TRdisplay("QEU_CRULE\n");
		    break;
		case QEU_DRULE:
		    TRdisplay("QEU_DRULE\n");
		    break;
		case QEU_CEVENT:
		    TRdisplay("QEU_CEVENT\n");
		    break;
		case QEU_DEVENT:
		    TRdisplay("QEU_DEVENT\n");
		    break;
		case QEU_CCOMMENT:
		    TRdisplay("QEU_CCOMMENT\n");
		    break;
		case QEU_CSYNONYM:
		    TRdisplay("QEU_CSYNONYM\n");
		    break;
		case QEU_DSYNONYM:
		    TRdisplay("QEU_DSYNONYM\n");
		    break;
		case QEU_DSCHEMA:
		    TRdisplay("QEU_DSCHEMA\n");
		    break;
		case QEU_DSTAT:
		    TRdisplay("QEU_DSTAT\n");
		    break;
		case QEU_B_COPY:
		    TRdisplay("QEU_B_COPY\n");
		    break;
		case QEU_E_COPY:
		    TRdisplay("QEU_E_COPY\n");
		    break;
		case QEU_R_COPY:
		    TRdisplay("QEU_R_COPY\n");
		    break;
		case QEU_W_COPY:
		    TRdisplay("QEU_W_COPY\n");
		    break;
		case QEQ_ENDRET:
		    TRdisplay("QEQ_ENDRET\n");
		    break;
		case QEQ_QUERY:
		    TRdisplay("QEQ_QUERY\n");
		    break;
		case QEQ_OPEN:
		    TRdisplay("QEQ_OPEN\n");
		    break;
		case QEQ_CLOSE:
		    TRdisplay("QEQ_CLOSE\n");
		    break;
		case QEQ_FETCH:
		    TRdisplay("QEQ_FETCH\n");
		    break;
		case QEQ_DELETE:
		    TRdisplay("QEQ_DELETE\n");
		    break;
		case QEQ_REPLACE:
		    TRdisplay("QEQ_REPLACE\n");
		    break;
		case QEQ_UPD_ROWCNT:
                    TRdisplay("QEQ_UPD_ROWCNT\n");
                    break;
		case QEU_GROUP:
		    TRdisplay("QEU_GROUP\n");
		    break;
		case QEU_APLID:
		    TRdisplay("QEU_APLID\n");
		    break;
		case QEU_DBPRIV:
		    TRdisplay("QEU_DBPRIV\n");
		    break;
		case QEU_CUSER:
		    TRdisplay("QEU_CUSER\n");
		    break;
		case QEU_AUSER:
		    TRdisplay("QEU_AUSER\n");
		    break;
		case QEU_DUSER:
		    TRdisplay("QEU_DUSER\n");
		    break;
		case QEU_CLOC:
		    TRdisplay("QEU_CLOC\n");
		    break;
		case QEU_ALOC:
		    TRdisplay("QEU_ALOC\n");
		    break;
		case QEU_DLOC:
		    TRdisplay("QEU_DLOC\n");
		    break;
		case QEU_CSEQ:
		    TRdisplay("QEU_CSEQ\n");
		    break;
		case QEU_ASEQ:
		    TRdisplay("QEU_ASEQ\n");
		    break;
		case QEU_DSEQ:
		    TRdisplay("QEU_DSEQ\n");
		    break;
		case QEU_CDBACC:
		    TRdisplay("QEU_CDBACC\n");
		    break;
		case QEU_DDBACC:
		    TRdisplay("QEU_DDBACC\n");
		    break;
		case QEU_MSEC:
		    TRdisplay("QEU_MSEC\n");
		    break;
		case QEU_CPROFILE:
		    TRdisplay("QEU_CPROFILE\n");
		    break;
		case QEU_APROFILE:
		    TRdisplay("QEU_APROFILE\n");
		    break;
		case QEU_DPROFILE:
		    TRdisplay("QEU_DPROFILE\n");
		    break;
		case QEU_ROLEGRANT:
		    TRdisplay("QEU_ROLEGRANT\n");
		    break;
		case QEU_RAISE_EVENT:
		    TRdisplay("QEU_RAISE_EVENT\n");
		    break;
		case QEU_CSECALM:
		    TRdisplay("QEU_CSECALM\n");
		    break;
		case QEU_DSECALM:
		    TRdisplay("QEU_DSECALM\n");
		    break;

		/* DDB operations start here */

		case QED_IIDBDB:
		    TRdisplay("QED_IIDBDB\n");
		    break;
		case QED_SESSCDB:
		    TRdisplay("QED_SESSCDB\n");
		    break;
		case QED_DDBINFO:
		    TRdisplay("QED_DBINFO\n");
		    break;
		case QED_LDBPLUS:
		    TRdisplay("QED_LDBPLUS\n");
		    break;
		case QED_USRSTAT:
		    TRdisplay("QED_USRSTAT\n");
		    break;
		case QED_1RDF_QUERY:
		    TRdisplay("QED_1RDF_QUERY\n");
		    break;
		case QED_2RDF_FETCH:
		    TRdisplay("QED_2RDF_FETCH\n");
		    break;
		case QED_3RDF_FLUSH:
		    TRdisplay("QED_3RDF_FLUSH\n");
		    break;
		case QED_4RDF_VDATA:
		    TRdisplay("QED_4RDF_VDATA\n");
		    break;
		case QED_5RDF_BEGIN:
		    TRdisplay("QED_5RDF_BEGIN\n");
		    break;
		case QED_6RDF_END:
		    TRdisplay("QED_6RDF_END\n");
		    break;
		case QED_7RDF_DIFF_ARCH:
		    TRdisplay("QED_7RDF_DIFF_ARCH\n");
		    break;
		case QED_EXEC_IMM:
		    TRdisplay("QED_EXEC_IMM\n");
		    break;
		case QED_C1_CONN:
		    TRdisplay("QED_C1_CONN\n");
		    break;
		case QED_C2_CONT:
		    TRdisplay("QED_C2_CONT\n");
		    break;
		case QED_C3_DISC:
		    TRdisplay("QED_C3_DISC\n");
		    break;
		case QED_A_COPY:
		    TRdisplay("QED_A_COPY\n");
		    break;
		case QED_B_COPY:
		    TRdisplay("QED_B_COPY\n");
		    break;
		case QED_E_COPY:
		    TRdisplay("QED_E_COPY\n");
		    break;
		case QED_CLINK:
		    TRdisplay("QED_CLINK\n");
		    break;
		case QED_1PSF_RANGE:
		    TRdisplay("QED_1PSF_RANGE\n");
		    break;
		case QED_1SCF_MODIFY:
		    TRdisplay("QED_1SCF_MODIFY\n");
		    break;
		case QED_2SCF_TERM_ASSOC:
		    TRdisplay("QED_2SCF_TERM_ASSOC\n");
		    break;
		case QED_CONCURRENCY:
		    TRdisplay("QED_DDL_CONCURRENCY\n");
		    break;
		case QED_SET_FOR_RQF:
		    TRdisplay("QED_SET_FOR_RQF\n");
		    break;
		case QED_QRY_FOR_LDB:
		    TRdisplay("QED_QRY_FOR_LDB\n");
		    break;
		case QED_SES_TIMEOUT:
		    TRdisplay("QED_SES_TIMEOUT\n");
		    break;
		case QED_RECOVER:
		    TRdisplay("QED_RECOVER\n");
		    break;
		case QED_P1_RECOVER:
		    TRdisplay("QED_P1_RECOVER\n");
		    break;
		case QED_P2_RECOVER:
		    TRdisplay("QED_P2_RECOVER\n");
		    break;
		case QED_IS_DDB_OPEN:
		    TRdisplay("QED_IS_DDB_OPEN\n");
		    break;

		/* DDB operations end here */
	    }
	
	}
    }
#endif
    if (qef_cb)
    {
	qef_cb->qef_rcb = (QEF_RCB*)save1;
	qef_cb->qef2_rcb = save2;
    }

    EXdelete();
#ifdef PERF_TEST
    CSfac_stats(FALSE, CS_QEF_ID);
#endif
#ifdef xDEV_TRACE
    CSfac_trace(FALSE, CS_QEF_ID, qef_id, rcb);
#endif

    return (status);
}


/**
** Name: qef_handler	- Exception handler to trap errors
**
** Description:
**      This routine traps any unexpected exceptions which occur 
**      within the boundaries of a qef_call(), logs the error, 
**      and returns.  qef_call() will interpret this as a fatal error.
**
** Inputs:
**      ex_args			Arguments provided by the exception mechanism
**
** Outputs:
**      (none)
**
**	Returns:
**	    EXDECLARE
**
**	Exceptions:
**	    (in response)
**
** Side Effects:
**	Logs an error message.
**
** History:
**	28-apr-86 (thurston)
**          Initial creation.
**	14-jul-86 (daved)
**	    Modified for use in QEF.
**	22-sep-86 (daved)
**	    add use of ADF exception handler
**	27-oct-87 (puree)
**	    set ule flag to ULE_MESSAGE when calling for AV message
**	10-nov-87 (puree)
**	    modified qef_hander not to call functions if their primary
**	    control blocks are not acquired.
**	24-nov-87 (puree)
**	    changed EXaccvio to EXsys_report.
**	04-dec-87 (puree)
**	    when -xf set, adx_handler will return E_DB_ERROR.  Need to
**	    set qef_ade_abort flag and return EX_DECLARE to qef_call.  The
**	    qef_ade_abort flag let qef_call know that this is not a fatal 
**	    error.
**	12-may-88 (puree)
**	    Call qee_cleanup after qet_abort to clean up the environment.
**	    The call to qee_cleanup in qet_abort has been removed.
**	28-oct-88 (puree)
**	    Move cleaning up logic to qef_call.
**	08-nov-88 (puree)
**	    Mark transtion to be aborted if an AV occurs.
**	13-dec-88 (puree)
**	    Fix casting error (qef_cb->qef_dsh).
**	26-jan-1989 (roger)
**	    An ex_args->exarg_num is not a CL_ERR_DESC (formerly CL_SYS_ERR).
**	    Therefore, don't pass the former where the latter is required.
**	23-may-89 (jrb)
**	    Changed calls to scc_error to first set generic error code.  Renamed
**	    scf_enumber to scf_local_error.  Changed ule_format calls for new
**	    interface.
**	24-oct-92 (andre)
**	    ule_format() now expects a (DB_SQLSTATE *) sqlstate instead of
**	    (i4 *) generic_error +
**	    both ADF_ERROR and SCF_CB have been changed to contain an SQLSTATE
**	    (DB_SQLSTATE) instead of generic_error (i4)
**	21-dec-92 (anitap)
**	    Prototyped.
**	21-jul-1993 (rog)
**	    General cleanup including the use of ulx_exception().
**	22-june-06 (dougi)
**	    Replicate Karl's fix of June 7 to qeferror.c to assure there's
**	    an action header before calling qef_procerr().
**	26-july-06 (dougi)
**	    Remove above change - we now tolerate lack of action header.
*/

STATUS
qef_handler(
	EX_ARGS            *ex_args)
{
    i4             error;
    i4             err;
    QEE_DSH		*dsh;
    ERR_CB		*err_cb;
    QEF_CB		*qef_cb;
    DB_STATUS		status = E_DB_OK;
    STATUS		ret_val = EXDECLARE;
    DB_ERROR		err_blk;
    EX_CONTEXT		context;
    bool		adf_error;
    CS_SID		sid;

    /*
    ** Handle all exceptions here the same way: internal DB_ERROR.
    */
    if (EXdeclare(qef_2handler, &context) != OK)
    {
	EXdelete();
	return (EXDECLARE);
    }
    
    /* get the qef control block */
    CSget_sid(&sid);
    qef_cb = GET_QEF_CB(sid);

    err_cb		= (ERR_CB*) qef_cb->qef2_rcb;
    qef_cb->qef_ade_abort = FALSE;

    /* determine if ADF math exception. */

    adf_error = FALSE;

    if (qef_cb->qef_adf_cb != (ADF_CB *) NULL)
    {
	status = adx_handler(qef_cb->qef_adf_cb, ex_args);
	error  = qef_cb->qef_adf_cb->adf_errcb.ad_errcode;
	if (status == E_DB_OK || error == E_AD0001_EX_IGN_CONT
	    || error == E_AD0115_EX_WRN_CONT) /* handle warning messages */
	{
	    EXdelete();
	    return(EXCONTINUES);
	}

	/* If the error happened in a database procedure, display
	** a message that locates the statement at fault. */
        {
            QEE_DSH     *currdsh;

            currdsh = (QEE_DSH *)qef_cb->qef_dsh;
            if (currdsh && currdsh->dsh_qp_ptr &&
                currdsh->dsh_qp_ptr->qp_status & QEQP_ISDBP)
		qef_procerr(qef_cb, currdsh);
        }

	/* handle other errors adf knows about */
	if (qef_cb->qef_adf_cb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	{
	    if (err_cb->err_eflag == QEF_EXTERNAL)
	    {
		SCF_CB		scf_cb;

		scf_cb.scf_facility = DB_QEF_ID;
		scf_cb.scf_session = qef_cb->qef_ses_id;
		scf_cb.scf_length = sizeof(SCF_CB);
		scf_cb.scf_type = SCF_CB_TYPE;

		scf_cb.scf_ptr_union.scf_buffer  = 
			    qef_cb->qef_adf_cb->adf_errcb.ad_errmsgp;
		scf_cb.scf_len_union.scf_blength = 
			    qef_cb->qef_adf_cb->adf_errcb.ad_emsglen;
		scf_cb.scf_nbr_union.scf_local_error = 
			    qef_cb->qef_adf_cb->adf_errcb.ad_usererr;
		STRUCT_ASSIGN_MACRO(
		    qef_cb->qef_adf_cb->adf_errcb.ad_sqlstate,
		    scf_cb.scf_aux_union.scf_sqlstate);
			    
		(VOID) scf_call(SCC_ERROR, &scf_cb);
		err_cb->err_block->err_code = E_QE0025_USER_ERROR;
	    }
	    else
	    {
		err_cb->err_block->err_code = 
			    qef_cb->qef_adf_cb->adf_errcb.ad_usererr;
	    }
	    qef_cb->qef_ade_abort = TRUE;
	    adf_error = TRUE;
	}
    }

    if (adf_error == FALSE)
    {
	/* Report all other errors */
	ulx_exception( ex_args, DB_QEF_ID, E_QE0002_INTERNAL_ERROR, TRUE );

	qef_cb->qef_abort = TRUE;	/* mark transaction to be aborted */
	err_cb->err_block->err_code = E_QE0002_INTERNAL_ERROR;
	ret_val = EXDECLARE;
    }

    /* now mark the QP obsolete */
    if ((qef_cb->qef_rcb != (QEF_RCB *)NULL) && 
	((dsh = (QEE_DSH *)(qef_cb->qef_dsh)) != (QEE_DSH *)NULL))
    {
	dsh->dsh_qp_status |= DSH_QP_OBSOLETE;
    }

    EXdelete();
    return(ret_val);
}

/**
** Name: qef_2handler	- Exception handler to trap errors
**
** Description:
**      This routine traps any unexpected exceptions which occur 
**	while processing an exception.
**
** Inputs:
**      ex_args			Arguments provided by the exception mechanism
**
** Outputs:
**      (none)
**
**	Returns:
**	    EXDECLARE
**
**	Exceptions:
**	    (in response)
**
** Side Effects:
**	Logs an error message.
**
** History:
**	17-dec-86 (daved)
**	    written
**	21-dec-92 (anitap)
**	    Prototyped.
*/

STATUS
qef_2handler(
	EX_ARGS            *ex_args)
{
    return (EXDECLARE);
}

#ifdef xDEBUG
bool
qe1_chk_error(
	char	    *caller,
	char	    *callee,
	i4	    status,
	DB_ERROR    *err_blk)
{
    if (DB_FAILURE_MACRO(status))
    {
	i4	facility_code;

	facility_code = (err_blk->err_code >> 16);
	if (facility_code != DB_QEF_ID && facility_code != DB_DMF_ID)
	{
	    TRdisplay("QEF debug: Illegal error code detected in %s\n", caller);
	    TRdisplay("           Last subroutine called was %s\n", callee);
	}
	return (TRUE);
    }
    else
    {
	return (FALSE);
    }
}

#define	QE_DBG_MACRO	    null_ptr = 0; null_char = *null_ptr;

bool
qe2_chk_qp(
	QEE_DSH	    *dsh)
{
    char	*null_ptr;
    char	null_char;
    QEF_QP_CB	*qp;

    for (;;)
    {
	if (dsh == (QEE_DSH *)NULL)
	{
	    QE_DBG_MACRO;
	    break;
	}

	if (dsh->dsh_length != sizeof(QEE_DSH) ||
	    dsh->dsh_type != QEDSH_CB ||
	    dsh->dsh_owner != DB_QEF_ID)
	{
	    QE_DBG_MACRO;
	    break;
	}
	break;	    /* until problem with associated QP fixed */

	if ((qp = dsh->dsh_qp_ptr) == (QEF_QP_CB *)NULL)
	{
	    QE_DBG_MACRO;
	    break;
	}

	if (qp->qp_length != sizeof(QEF_QP_CB) ||
	    qp->qp_type != QEQP_CB)
	{
	    QE_DBG_MACRO;
	    break;
	}

	break;
    }
    return(FALSE);    
}
#endif
