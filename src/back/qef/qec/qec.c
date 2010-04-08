/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cui.h>
#include    <cm.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <dudbms.h>
#include    <dmf.h> 
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ex.h>
#include    <scf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ulh.h>
#include    <qsf.h>
#include    <qefmain.h>
#include    <qefscb.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <sxf.h>
#include    <qefprotos.h>
#include    <rdf.h>

/**
**
**  Name: QEC.C - control routines for the QEF
**
**  Description:
**      These routines provide initialization and monitoring functions. 
**
**          qec_alter           - redefine performance constants
**          qec_begin_session   - initialize a QEF session control block
**          qec_debug           - print various QEF structures
**          qec_initialize      - initialize QEF for the server
**          qec_trace           - set trace flags for session or server
**	    qec_end_session	- end a qef session
**	    qec_shutdown	- end a qef server
**	    qec_info		- gather qef statistics
**	    qec_malloc		- QEF memory allocator.
**	    qec_mopen		- Open a memory stream for QEF
**	    qec_mfree		- Garbage collect QEF memory
**	    qec_tprintf		- Interface to SCC_TRACE.
**	    qec_trimwhite	- Trim trailing blanks for trace and errors.
**
**  History:
**      29-apr-86 (daved)    
**          written
**	30-nov-87 (puree)
**	    Convert SCF_SEMAPHORE to CS_SEMAPHORE.
**	10-dec-87 (puree)
**	    Implement qec_palloc
**	22-dec-87 (puree)
**	    Modify qec_initialize.
**	10-feb-88 (puree)
**	    Rename qec_palloc to qec_malloc.  Implement qec_mopen.
**	03-mar-88 (puree)
**	    clean up some lint errors.
**	12-may-88 (puree)
**	    Modify qec_end_session to always call qee_cleanup.
**	22-jul-88 (puree) 
**	    modify qec_malloc to fix infinite loop due to incorrect 
**	    else-if blocks
**	11-aug-88 (puree)
**	    Fix bug in qec_trace.
**	11-sep-88 (puree)
**	    Fix infinite loop in qec_mopen.
**	10-oct-88 (puree)
**	    Fix qec_info for transaction state and autocommit status.
**	30-jan-89 (carl)
**	    Modified to use new DB_DISTRIB definition
**	09-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added support for database privileges;
**	    qec_begin_session resets the session's dbprivs;
**	    qec_alter sets the session's dbprivs.
**	01-may-89 (ralph)
**	    GRANT Enhancements, Phase 2b:
**	    Revised dbpriv initialization.
**      22-may-89 (davebf)
**          Fix qec_info to return QEF_SSTRAN for QEF_SSTRAN
**	31-may-89 (neil)
**	    Cleaned up qec_tprintf and added qec_trimwhite.
**	03-jun-89 (carl)
**	    Fixed qec_begin_session to incorporate adf_cb.adf_constants
**	    initialization
**	23-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added QEC_IGNORE request code for qec_alter().
**	06-jul-89 (carl)
**	    Modified sequence of processing (in qec_end_session) to allow 
**	    STAR to reinitialize the states of used REPEAT QUERIES before
**	    space is released 
**
**	    Modified qec_begin_session to open a stream for the session's
**	    use and qec_end_session to close the stream
**      14-jul-89 (jennifer)
**          Added code to allow QEC_INITIALIZE to accept flag mask
**          indicating special states for server.
**	30-jan-90 (ralph)
**	    Added QEC_USTAT request code for qec_alter().
**	23-apr-90 (davebf)
**	    Return qef_rcb->qef_stat based on qef_cb->qef_ostat.
**	    qef_ostat is assigned from qef_stat at end of each action
**	    and at end of each statement.  Therefore it represents the
**	    transaction status at the beginning of the current action
**	    or statement.
**	24-apr-90 (davebf)
**	    init qef_ostat
**	24-apr-90 (carl)
**	    modified qec_trace to add 2PC trace points for TPF under QEF,
**	    temporarily qe60..qe99
**	21-aug-90 (carl)
**	    added comment to qec_trace for new trace points:
**
**		qe50 for skipping destroying temporary tables at the end of
**		     every DDB query, 
**		qe51 for validating every DDB QP, 
**
**		qe55 for tracing every DDB query/repeat plan, 
**		qe56 for tracing every QEF call,
**		qe57 for tracing every DDB DDL command,
**		qe58 for tracing every DDB cursor operation, 
**              qe59 for tracing QP when an error occurs
**	30-sep-90 (carl)
**	    integrated qec_tprintf and qec_trimwhite from mainline
**	07-oct-90 (carl)
**	    changed qec_trace to call qed_u17_tpf_call isntead of tpf_call
**	21-oct-90 (carl)
**	    changed qec_trace to call tpf for trace points only if they are 
**	    in range
**	25-feb-91 (rogerk)
**	    Added QEC_ONERROR_STATE mode to qec_alter.
**	    Added statement error mode (qef_stm_error) and savepoint name
**	    for QEF_SVPT_ROLLBACK statement error mode (qef_savepoint) to
**	    qec_info return information.
**	13-aug-91 (fpang)
**	    Changes to support rqf trace points.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added functions qec_interrupt_handler and
**	    qec_register_interrupt_handler. Added call to
**	    qec_register_interrupt_handler to qec_begin_session.
**	15-jun-92 (davebf)
**	    Sybil Merge
**      08-dec-92 (anitap)
**          Added #include <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	17-mar-93 (rickh)
**	    Hack to initialize case translation semantics.
**	25-mar-93 (rickh)
**	    Force case translation semantics to 0 since at session startup
**	    we haven't got a clue what they are.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Initialize QEF_CB.qef_dbxlate to address case translation mask.
**	    Initialize QEF_CB.qef_cat_owner to address catalog owner name.
**	11-may-93 (anitap)
**	    Added support for "SET UPDATE_ROWCOUNT" statement in
**          qec_begin_sesion().
**	    Return the value for qef_upd_rowcnt in qec_info() for dbmsinfo().
**          Fixed compiler warning in qec_debug(). 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	29-jun-93 (rickh)
**	    Added TR.H.
**	2-aug-93 (rickh)
**	    Removed 17-mar-93 hack that initialized case translation semantics.
**	    The 7-apr-93 fix superceded it.  Also, initialized the callback
**	    status.
**      10-sep-93 (smc)
**          Removed cast of qef_ses_id to i4 now seesion ids are CS_SID.
**          Moved <cs.h> for CS_SID. Fixed up cast in rdf_call.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	20-jan-94 (rickh)
**	    Initialize MO objects.  Also initialize the qen spill threshold.
**	21-jan-94 (iyer)
**	    In qec_info(), copy QEF_CB.qef_tran_id to QEF_RCB.qef_tran_id
**	    in support of dbmsinfo('db_tran_id').
**	14-mar-94 (swm)
**	    Bug #59883
**	    Allocate and release ULM stream for all QEF sessions, not just
**	    for RQF and TPF sessions associated with DDB sessions, in
**	    qec_begin_session() and qec_end_session() respectively. Allocate
**	    a TRformat buffer per QEF session rather than relying on automatic
**	    buffer allocation on the stack; this is to eliminate the
**	    probability of stack overrun owing to large buffers.
**	06-mar-96 (nanpr01)
**	    Initialize the field Qef_s_cb.qef_maxtup from qef_cb.qef_maxtup. 
**	23-sep-1996 (canor01)
**	    Move global data definitions to qecdata.c.
**	10-Jun-1997 (jenjo02)
**	    Completed 30-nov-87 (puree) conversion from SCF_SEMAPHORE to
**	    CS_SEMAPHORE by replacing SCU_SINIT call with CSw_semaphore()
**	    for qef_sem. Rearranged code so that qef_sem is held only as
**	    long as needed.
**	05-aug-1997 (canor01)
**	    Remove the Qef_s_cb semaphore before freeing the memory.
**	07-Aug-1997 (jenjo02)
**	    Changed *dsh to **dsh in qee_cleanup(), qee_destroy() prototypes.
**	    Pointer will be set to null if DSH is deallocated.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	1-apr-1998 (hanch04)
**	    Calling SCF every time has horendous performance implications,
**	    re-wrote routine to call CS directly with qef_session.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Replaced qef_session() function with GET_QEF_CB macro.
**	24-Jan-2001 (jenjo02)
**	    Added EXTERNAL qec_mfree() function to replace in-line ULH
**	    garbage collection.
**      28-feb-2003 (stial01)
**          Added qef-ulm memory diagnostics
**      06-may-2003 (stial01)
**          ifdef xDEBUG some qef-ulm memory diags
**	1-Dec-2003 (schka24)
**	    remove 25% allocation for sort/hash, instead allow user to
**	    define total sort+hash pool and per-session limits for each.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	20-Apr-2004 (jenjo02)
**	    Changed interrupt handler to return STATUS,
**	    consistent with SCF.
**      10-Jul-2006 (hanal04) Bug 116358
**          Added QEF_TRACE_IMMEDIATE_ULM_POOLMAP. User can now run
**          "set trace point qe41 2" to generate a pool map of QEF's
**          sort and DSH memory pools.
**/


/*
** Definition of all global variables owned by this file.
*/

GLOBALREF QEF_S_CB   *Qef_s_cb; /* The QEF server control block. */

/*
** Forward External References.
FUNC_EXTERN VOID	qec_mfunc();

static DB_STATUS
register_interrupt_handler();
*/

static STATUS qec_interrupt_handler(
i4		   eventid,
QEF_CB             *qef_cb );

static DB_STATUS
qec_register_interrupt_handler(
QEF_CB		*qef_cb );


static DB_STATUS
qec_trace_immediate(
QEF_CB *qef_cb, 
i4 val_count,
i4 val1,
i4 val2);


/*{
** Name: QEC_INITIALIZE - initialize the server
**
** External QEF call:   status = qef_call(QEC_INITIALIZE, &qef_s_cb);
**
** Description:
**      This operation prepares the QEF for operation. The server control
**  block is initialized, a memory pool is collected and the environment
**  queue is created. If this is a warm boot type operation, memory is freed
**  prior to the execution of the above steps. The server control block
**  is allocated in this file. 
**
**  Titan Description:
**      This operation complements initializing QEF for DDB operations. 
**  The DDB portion of the server control block is initialized.  RQF and
**  TPF are called to start up.
**
** Inputs:
**      qef_srcb
**	    .qef_eflag		    designate error handling semantis
**				    for user errors.
**		QEF_INTERNAL	    return error code.
**		QEF_EXTERNAL	    send message to user.
**	    .qef_sorthash_memory    max total memory for sort+hash buffers.
**	    .qef_sort_maxmem	    max session memory for sort buffers.
**	    .qef_hash_maxmem	    max session memory for hash buffers.
**	    .qef_sess_max	    maximum number of active sessions.
**          .qef_qpmax              maximum number of active query plans.
**          .qef_dsh_maxmem         maximum amount of memory for DSH.
**          .qef_flag_mask          Indicates special server state, must
**                                  be zero or QEF_F_C2SECURE.
**
** Outputs:
**      qef_srcb
**	    .qef_ssize		    size of session control block 
**	    .qef_server		    address of server control block
**	    .qef_ulhid		    ULH cache id for DSHs
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0017_BAD_CB
**                                  E_QE0018_BAD_PARAM_IN_CB
**                                  E_QE0019_NON_INTERNAL_FAIL
**                                  E_QE0001_INVALID_ALTER
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0003_INVALID_SPECIFICATION
**                                  E_QE001E_NO_MEM
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written
**	9-mar-87 (daved)
**	    initialize the facility for storing used DSH.
**	22-dec-87 (puree)
**	    change ulh_init call sequence and remove qpused.
**	22-aug-88 (carl)
**	    modified for Titan to start up RQF and TPF for DDB server,
**	    in that order
**      14-jul-89 (jennifer)
**          Added code to allow QEC_INITIALIZE to accept flag mask
**          indicating special states for server.
**	06-apr-90 (andre)
**	    interface to ulh_init() has changed somewhat.
**	03-feb-93 (andre)
**	    will call rdf_initialize() to get RDF's "facility control block"
**	    which can be used in subsequent calls to RDF
**	20-jan-94 (rickh)
**	    Initialize MO objects.
**	06-mar-96 (nanpr01)
**	    Initialize the field Qef_s_cb.qef_maxtup from qef_cb.qef_maxtup. 
**	10-Jun-1997 (jenjo02)
**	    Completed 30-nov-87 (puree) conversion from SCF_SEMAPHORE to
**	    CS_SEMAPHORE by replacing SCU_SINIT call with CSw_semaphore()
**	    for qef_sem.
**	28-jan-00 (inkdo01)
**	    Added qef_hash_maxmem for hash join implementation.
**	1-Dec-2003 (schka24)
**	    remove 25% allocation for sort/hash, instead allow user to
**	    define total sort+hash pool and per-session limits for each.
**	04-Dec-2004 (jenjo02)
**	    Set QEF_S_IS_MT if OS-threaded server.
**	24-Oct-2005 (schka24)
**	    Pass in max memory resource delay for QEF.
**      07-jun-2007 (huazh01)
**          add qef_no_dependency_chk for the config parameter which 
**          switches ON/OFF the fix to b112978.
**	5-Aug-2009 (kschendel) SIR 122512
**	    Pass in new hash related config items.
[@history_line@]...
*/
DB_STATUS
qec_initialize(
QEF_SRCB       *qef_srcb )
{
    DB_STATUS   status;
    i4     err;
    ULH_RCB	ulh_rcb;
    ULM_RCB	ulm_rcb;

    /* Validate request block */
    if (qef_srcb->qef_qpmax < 0 || qef_srcb->qef_dsh_maxmem < 0)
    {
        qef_error(E_QE0018_BAD_PARAM_IN_CB, 0L, E_DB_ERROR, &err,
            &qef_srcb->error, 0);
        return (E_DB_ERROR);
    }

    /* Allocate memory pool for DSH and work space */
    ulm_rcb.ulm_facility = DB_QEF_ID;
    ulm_rcb.ulm_sizepool = qef_srcb->qef_dsh_maxmem;
    ulm_rcb.ulm_blocksize = 0;
    status = ulm_startup(&ulm_rcb);
    if (status != E_DB_OK)
    {
        qef_error(E_QE001E_NO_MEM, 0L, E_DB_ERROR, &err,
            &qef_srcb->error, 0);
        return (status);
    }
    ulm_rcb.ulm_memleft = &ulm_rcb.ulm_sizepool;
    
    /* Open a SHARED memory stream and allocate the QEF server control block */
    ulm_rcb.ulm_flags = ULM_SHARED_STREAM | ULM_OPEN_AND_PALLOC;
    ulm_rcb.ulm_psize = sizeof (QEF_S_CB);

    status = ulm_openstream(&ulm_rcb);
    if (status != E_DB_OK)
    {
        qef_error(E_QE001E_NO_MEM, 0L, E_DB_ERROR, &err,
            &qef_srcb->error, 0);
        return (status);
    }

    /* Initialize QEF server control block */
    Qef_s_cb = (QEF_S_CB *)ulm_rcb.ulm_pptr;
    Qef_s_cb->qef_next = (QEF_S_CB *)NULL;
    Qef_s_cb->qef_prev = (QEF_S_CB *)NULL;
    Qef_s_cb->qef_length = sizeof(QEF_S_CB);
    Qef_s_cb->qef_type = QEFSCB_CB;
    Qef_s_cb->qef_owner = (PTR)DB_QEF_ID;
    Qef_s_cb->qef_ascii_id = QEFSCB_ASCII_ID;
    Qef_s_cb->qef_dsh_maxmem = qef_srcb->qef_dsh_maxmem;
    Qef_s_cb->qef_sort_maxmem = qef_srcb->qef_sort_maxmem;
    Qef_s_cb->qef_hash_maxmem = qef_srcb->qef_hash_maxmem;
    Qef_s_cb->qef_max_mem_nap = 1000 * qef_srcb->qef_max_mem_sleep;
    /* Let's go with 16K for write and 128K for read hash buffer sizes,
    ** if nothing was specified by the config.  Those are very conservative
    ** sizes suited to modest memory usage.
    */
    if (qef_srcb->qef_hash_rbsize == 0)
	qef_srcb->qef_hash_rbsize = 128 * 1024;
    if (qef_srcb->qef_hash_wbsize == 0)
	qef_srcb->qef_hash_wbsize = 16 * 1024;
    Qef_s_cb->qef_hash_rbsize = DB_ALIGNTO_MACRO(qef_srcb->qef_hash_rbsize, HASH_PGSIZE);
    Qef_s_cb->qef_hash_wbsize = DB_ALIGNTO_MACRO(qef_srcb->qef_hash_wbsize, HASH_PGSIZE);
    /* qeehash expects that the compression threshold is sane;  use at least
    ** twice the length multiple.  Anything less can't RLE-compress anyway.
    ** Zero means "don't compress", use a giant number.
    */
    if (qef_srcb->qef_hash_cmp_threshold == 0)
	qef_srcb->qef_hash_cmp_threshold = 1024 * 1024 * 1024;
    if (qef_srcb->qef_hash_cmp_threshold < 2*QEF_HASH_DUP_ALIGN)
	qef_srcb->qef_hash_cmp_threshold = 2*QEF_HASH_DUP_ALIGN;
    Qef_s_cb->qef_hash_cmp_threshold = DB_ALIGNTO_MACRO(qef_srcb->qef_hash_cmp_threshold, QEF_HASH_DUP_ALIGN);
    /* The hashjoin size limit hints don't need to be validated against
    ** anything (much);  just make sure that the min < max, and max > 500K.
    ** Zero hashjoin max means "big".
    */
    if (qef_srcb->qef_hashjoin_max == 0)
	qef_srcb->qef_hashjoin_max = Qef_s_cb->qef_hash_maxmem;
    if (qef_srcb->qef_hashjoin_min > qef_srcb->qef_hashjoin_max)
	qef_srcb->qef_hashjoin_max = qef_srcb->qef_hashjoin_min;
    if (qef_srcb->qef_hashjoin_max < 500 * 1024)
	qef_srcb->qef_hashjoin_max = 500 * 1024;
    Qef_s_cb->qef_hashjoin_min = qef_srcb->qef_hashjoin_min;
    Qef_s_cb->qef_hashjoin_max = qef_srcb->qef_hashjoin_max;
    Qef_s_cb->qef_sess_max = qef_srcb->qef_sess_max;
    Qef_s_cb->qef_qpmax = qef_srcb->qef_qpmax;
    Qef_s_cb->qef_maxtup = qef_srcb->qef_maxtup;
    Qef_s_cb->qef_v1_distrib = qef_srcb->qef_s1_distrib;
    Qef_s_cb->qef_state = 0;
    Qef_s_cb->qef_qescb_offset = qef_srcb->qef_qescb_offset;
    Qef_s_cb->qef_no_dependency_chk = qef_srcb->qef_nodep_chk; 

    if (qef_srcb->qef_flag_mask & QEF_F_C2SECURE)
	Qef_s_cb->qef_state |= QEF_S_C2SECURE;

    /* Let all know if running with OS threads */
    if ( CS_is_mt() )
	Qef_s_cb->qef_state |= QEF_S_IS_MT;

    /* Set up template for ULM control block */
    /* Note that this establishes default SHARED memory streams */
    STRUCT_ASSIGN_MACRO(ulm_rcb, Qef_s_cb->qef_d_ulmcb);
    Qef_s_cb->qef_d_ulmcb.ulm_memleft = &Qef_s_cb->qef_d_ulmcb.ulm_sizepool;

    /* set up the DSH cache */
    status = ulh_init(&ulh_rcb, (i4)DB_QEF_ID, ulm_rcb.ulm_poolid, 
	    Qef_s_cb->qef_d_ulmcb.ulm_memleft, qef_srcb->qef_qpmax, 
	    qef_srcb->qef_qpmax, (i4)1, (i4) 0, 0L);
    if (status != E_DB_OK)
    {
        qef_error(E_QE004E_ULH_INIT_FAILURE, 0L, E_DB_ERROR, &err,
            &qef_srcb->error, 0);
        return (status);
    }
    Qef_s_cb->qef_ulhid = ulh_rcb.ulh_hashid;


    /* Allocate the memory pool for sort+hash buffers */
    ulm_rcb.ulm_facility = DB_QEF_ID;
    ulm_rcb.ulm_sizepool = qef_srcb->qef_sorthash_memory;
    ulm_rcb.ulm_blocksize = 0;
    status = ulm_startup(&ulm_rcb);
    if (status != E_DB_OK)
    {
        qef_error(E_QE001E_NO_MEM, 0L, E_DB_ERROR, &err,
            &qef_srcb->error, 0);
        return (status);
    }

    /* Set up template for ULM control block */
    /* Make the sort buffer memory streams PRIVATE (thread-safe) */
    STRUCT_ASSIGN_MACRO(ulm_rcb, Qef_s_cb->qef_s_ulmcb);
    Qef_s_cb->qef_s_ulmcb.ulm_memleft = &Qef_s_cb->qef_s_ulmcb.ulm_sizepool;
    Qef_s_cb->qef_s_ulmcb.ulm_flags = ULM_PRIVATE_STREAM;

    /* set up the QEN vector table */
    qec_mfunc(Qef_s_cb->qef_func);
   
    /* Initialize trace vector: expect lint error message here */
    ult_init_macro(&Qef_s_cb->qef_trace, QEF_SNB, QEF_SNVP, QEF_SNVAO);

    /* Initialize QEF semaphore */
    CSw_semaphore(&Qef_s_cb->qef_sem, CS_SEM_SINGLE, "QEF sem");

    if (qef_srcb->qef_s1_distrib & DB_2_DISTRIB_SVR)
    {
	/* initializing a DDB server, hence RQF and TPF in that order */

	status = qed_q1_init_ddv(qef_srcb);
	if (status)
	    return(status);

    }

    /*
    ** get RDF "facility control block"
    */
    {
	RDF_CCB             rdf_cb;

	MEfill(sizeof(rdf_cb), (u_char) 0, (PTR) &rdf_cb);

	rdf_cb.rdf_fac_id  = DB_QEF_ID;
	rdf_cb.rdf_distrib = qef_srcb->qef_s1_distrib;

	status = rdf_call(RDF_INITIALIZE, (PTR)&rdf_cb);
	if (DB_FAILURE_MACRO(status))
	{
	    qef_error(rdf_cb.rdf_error.err_code, 0L, E_DB_ERROR, &err,
		&qef_srcb->error, 0);
	    return(status);
	}

	/* save the RDF facility control block in Qef_s_cb */
	Qef_s_cb->qef_rdffcb = rdf_cb.rdf_fcb;
    }
    
    /* Register QEF objects with the Management Objects facilities */
    /* ----------------------------------------------------------- */
    if (qef_mo_register() != OK)
    {
	qef_srcb->error.err_code = E_QE0307_INIT_MO_ERROR;
	status = E_DB_FATAL;
    }

    Qef_s_cb->qef_init = TRUE;
    qef_srcb->qef_server = (PTR)Qef_s_cb;
    qef_srcb->qef_ssize	= sizeof (QEF_CB);
    qef_srcb->error.err_code = E_QE0000_OK;

    return (E_DB_OK);
}


/*{
** Name: QEC_SHUTDOWN - end a server
**
** External QEF call:   status = qef_call(QEC_SHUTDOWN, &qef_srcb);
**
** Description:
**	This operation releases all DSH resources and returns system
**  resources to SCF. The current system resource is memory.
**  Server shutdown always returns E_DB_OK because there is not
**  enough information left to continue running after an aborted
**  shutdown.
**
** Inputs:
**      qef_srcb
**
** Outputs:
**      qef_srcb
**	    .error.err_code
**                                      E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written
**	22-aug-88 (carl)
**	    modified for Titan to shut down TPF and RQF for DDB server,
**	    in that order
**	03-feb-93 (andre)
**	    call rdf_terminate() to deallocate the RDF facility control block
**	    which was allocated on behalf of QEF
**	05-aug-1997 (canor01)
**	    Remove the Qef_s_cb semaphore before freeing the memory.
**	03-Sep-1997 (jenjo02)
**	    Do that with CSr_semaphore() directly instead of using SCF.
**	31-Mar-2010 (kschendel) SIR 123519
**	    xDEBUG the "before releasing DSH" printpools.  They will almost
**	    always contain stuff, and it's not useful to fill the shutdown
**	    report with pages of mostly-useless gibberish.  Eliminate
**	    the redundant print-pools before shutting the pools down,
**	    but do make sure ULM prints its usual report.
*/
DB_STATUS
qec_shutdown(
QEF_SRCB       *qef_srcb )
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    DB_STATUS		status = E_DB_OK;
    ULH_RCB		ulh_rcb;
    ULM_RCB		ulm_rcb;
    bool		ddb_svr_b;
    SCF_CB		scf_cb;
    i4     	err;

    if (Qef_s_cb->qef_v1_distrib & DB_2_DISTRIB_SVR)
	ddb_svr_b = TRUE;
    else
	ddb_svr_b = FALSE;

#ifdef xDEBUG
    /* print the qef memory pools prior to QEF shutdown */
    TRdisplay("%@ QEF_TRACE_IMMEDIATE_ULM sort memory before releasing DSH\n");
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
    ulm_print_pool(&ulm_rcb);
    TRdisplay("%@ QEF_TRACE_IMMEDIATE_ULM DSH memory before releasing DSH\n");
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm_rcb);
    ulm_print_pool(&ulm_rcb);
#endif

    /* Release/destroy all DSHs */
    qee_shutdown();

    /* Close the DSH cache */
    ulh_rcb.ulh_hashid = Qef_s_cb->qef_ulhid;
    ulh_close(&ulh_rcb);

    {
	/*
	** call rdf to deallocate the facility control block which was
	** allocated on behalf of QEF
	*/
	RDF_CCB             rdf_ccb;

	rdf_ccb.rdf_fcb = Qef_s_cb->qef_rdffcb;
	(void) rdf_call(RDF_TERMINATE, (PTR)&rdf_ccb);
    }

    /* Remove the semaphore before freeing the memory */
    CSr_semaphore(&Qef_s_cb->qef_sem);

    /* Close the memory pools */
    /* Make sure facility says QEF (it should, but be sure), so that we
    ** get the shutdown reports from ULM plus dumps if any mem leaks.
    */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
    ulm_rcb.ulm_facility = DB_QEF_ID;
    ulm_shutdown(&ulm_rcb);
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm_rcb);
    ulm_rcb.ulm_facility = DB_QEF_ID;
    ulm_shutdown(&ulm_rcb);

    qef_srcb->error.err_code	= E_QE0000_OK;

    if (ddb_svr_b)
    {
	/* shutting down DDB server, hence TPF and RQF in that order */

	qef_srcb->qef_server = (PTR) & Qef_s_cb;
	status = qed_q2_shut_ddv(qef_srcb);		
    }

    return (status);
}


/*{
** Name: QEC_BEGIN_SESSION      - begin QEF session
**
** External QEF call:   status = qef_call(QEC_BEGIN_SESSION, &qef_rcb);
**
** Description:
**      An empty control block is initialized. The operating characteristics
**  are established (and can be changed with the QEC_ALTER call). The control
**  initialized in this routine must be used on all subsequent calls to QEF
**  that specify the qef_cb as input.
**
** Inputs:
**	    qef_rcb
**              .qef_sess_id            session identifier
**		.qef_cb			session control block
**		.qef_eflag		designate error handling semantis
**					for user errors.
**		   QEF_INTERNAL		return error code.
**		   QEF_EXTERNAL		send message to user.
**		.qef_adf_cb		the adf session control block
**					this cb will be accessed for all future
**					QEF calls. Thus, it should be 
**					dynamically allocated.
**		.qef_auto		auto commit on end of query ?
**		    QEF_ON		should be default for QUEL
**		    QEF_OFF		should be default for SQL
**              .qef_asize              number of operating characteristics
**		.qef_server		address of server control block
**              .qef_alt                non-default operation characteristics
**                                      there are no non-default operations
**                                      at this time
**          <qef_alt> looks like
**              .alt_item           item to change
**              .alt_value          new value
**
** Outputs:
**	    qef_rcb
**              .qef_version            Version of QEF
**              .error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0017_BAD_CB
**                                      E_QE0018_BAD_PARAM_IN_CB
**                                      E_QE0019_NON_INTERNAL_FAIL
**                                      E_QE0001_INVALID_ALTER
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0003_INVALID_SPECIFICATION
**          Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written
**	30-nov-87 (puree)
**	    converted SCF_SEMAPHORE to CS_SEMAPHORE.
**	04-nov-87 (puree)
**	    removed qef_warn, added qef_ade_abort.
**	22-aug-88 (carl)
**	    modified to initialize associated RQF and TPF sessions for
**	    DDB session 
**	09-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added support for database privileges;
**	    qec_begin_session resets the session's dbprivs
**	01-may-89 (ralph)
**	    GRANT Enhancements, Phase 2b:
**	    Revised dbpriv initialization.
**	03-jun-89 (carl)
**	    incorporated a adf_cb.adf_constants initialization
**	06-jul-89 (carl)
**	    Modified to open a stream for the session's use 
**	24-apr-90 (davebf)
**	    init qef_ostat
**	25-feb-91 (rogerk)
**	    Added qef_stm_error field to the qef_cb to define the statement
**	    error handling action.  Initialize that field here.
**	11-mar-92 (jhahn)
**	    Added support for QEF interrupts.
**	    Added call to qec_register_interrupt_handler.
**	18-nov-92 (barbara)
**	    Initialize adf_cb.adf_constants.
**	17-mar-93 (rickh)
**	    Hack to initialize case translation semantics.
**	25-mar-93 (rickh)
**	    Force case translation semantics to 0 since at session startup
**	    we haven't got a clue what they are.
**	07-apr-93 (ralph)
**	    DELIM_IDENT:
**	    Initialize QEF_CB.qef_dbxlate to address case translation mask.
**	    Initialize QEF_CB.qef_cat_owner to address catalog owner name.
**      06-may-93 (anitap)
**          Initialize qef_cb->qef_upd_rowcnt.
**	2-aug-93 (rickh)
**	    Removed 17-mar-93 hack that initialized case translation semantics.
**	    The 7-apr-93 fix superceded it.  Also, initialized the callback
**	    status.
**	22-sep-93 (andre)
**	    translate QEF_UPGRADEDB_SESSION in qef_rcb->qef_sess_startup_flags
**	    into QEF_RUNNING_UPGRADEDB in qef_cb->qef_sess_flags
**      25-nov-93 (robf)
**           Save role/group values
**	20-jan-94 (rickh)
**	    Initialize the qen spill threshold.
**	14-mar-94 (swm)
**	    Bug #59883
**	    Allocate and release ULM stream for all QEF sessions, not just
**	    for RQF and TPF sessions associated with DDB sessions, in
**	    qec_begin_session() and qec_end_session() respectively. Allocate
**	    a TRformat buffer per QEF session rather than relying on automatic
**	    buffer allocation on the stack; this is to eliminate the
**	    probability of stack overrun owing to large buffers.
**	10-Jun-1997 (jenjo02)
**	    Initialize qef_cb, then grab qef_sem, put qef_cb on Qef_s_cb list,
**	    and free qef_sem instead of holding the semaphore while 
**	    initializing qef_cb.
**	28-jan-00 (inkdo01)
**	    Added qef_h_max, qef_h_used for hash join support. Also upped both
**	    sortmax and hashmax to be 25% of total server wide sort/hash 
**	    memory (like OPF and its maxmemf).
**	1-Dec-2003 (schka24)
**	    Reverse above 25% change, allow users to choose both total and
**	    per-session memory via parameters.
**	4-oct-2006 (dougi)
**	    Fix ult_init_macro() to allow multiple trace points.
*/
DB_STATUS
qec_begin_session(
QEF_CB         *qef_cb )
{
    QEF_S_CB		*qef_scb;
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    QEF_RCB		*qef_rcb = qef_cb->qef_rcb;
    ULM_RCB		ulm;
    DB_STATUS		status;
    i4		err;

    FUNC_EXTERN DB_STATUS scf_call();

    qef_scb = (QEF_S_CB *)qef_rcb->qef_server;
    qef_rcb->error.err_code = E_QE0000_OK;
    qef_rcb->error.err_data = E_QE0000_OK;

    do	/* to break off in case of error */
    {

	/* Initialize the currrent session control block */
	qef_cb->qef_length = sizeof(QEF_CB);
	qef_cb->qef_type = QEFCB_CB;
	qef_cb->qef_owner = (PTR)DB_SCF_ID;
	qef_cb->qef_ascii_id = QEFCB_ASCII_ID;
	qef_cb->qef_s_max   = qef_scb->qef_sort_maxmem;
	qef_cb->qef_h_max   = qef_scb->qef_hash_maxmem;
	qef_cb->qef_s_used  = 0;	    /* no sort buffer yet */
	qef_cb->qef_h_used  = 0;	    /* no hash buffer yet */
	qef_cb->qef_ses_id  = qef_rcb->qef_sess_id; /* session id */
	qef_cb->qef_tran_id.db_high_tran = 0;   /* no current tran id */
	qef_cb->qef_tran_id.db_low_tran = 0;    
	qef_cb->qef_stat    = QEF_NOTRAN;   /* no transaction */
	qef_cb->qef_ostat   = QEF_NOTRAN;   /* no transaction */
	qef_cb->qef_stmt    = 0;            /* statement number in tran */
	qef_cb->qef_open_count  = 0;        /* no open queries/cursors */
	qef_cb->qeu_cstat   = QEF_NOCOPY;   /* no copy in progress */
	qef_cb->qef_odsh    = (PTR)NULL;
	qef_cb->qef_dsh	    = (PTR)NULL;    
	qef_cb->qef_abort   = FALSE;
	qef_cb->qef_auto    = qef_rcb->qef_auto;
	qef_cb->qef_upd_rowcnt = QEF_ROWQLFD;
	qef_cb->qef_ade_abort   = FALSE;
	qef_cb->qef_adf_cb  = qef_rcb->qef_adf_cb;
						/* ptr to session ADF_CB */
	qef_cb->qef_adf_cb->adf_constants = (ADK_CONST_BLK *) NULL;
						/* no adk block exists yet */
	qef_cb->qef_c1_distrib = qef_rcb->qef_r1_distrib;
	qef_cb->qef_fl_dbpriv  = DBPR_BINARY;	/* All binary privileges */
	qef_cb->qef_row_limit  = -1;		/* Unrestricted rows */
	qef_cb->qef_dio_limit  = -1;		/* Unrestricted I/Os */
	qef_cb->qef_cpu_limit  = -1;		/* Unrestricted CPU time */
	qef_cb->qef_page_limit = -1;		/* Unrestricted pages */
	qef_cb->qef_cost_limit = -1;		/* Unrestricted cost  */

	/* initialize callback status */

	qef_cb->qef_callbackStatus = 0;

	/*
	** Initialize pointers to case translation semantics flags and
	** catalog owner name.  Although the case translation flags
	** are not known at session startup, SCF will change the flags
	** when they are determined.  Because a pointer to the flags
	** are passed in, QEF need not be notified when the flags
	** change, as long as we don't make a local copy of them.
	*/
	qef_cb->qef_dbxlate = qef_rcb->qef_dbxlate;
	qef_cb->qef_cat_owner = qef_rcb->qef_cat_owner;

	/*
	** Save group/role from SCF
	*/
	qef_cb->qef_group = qef_rcb->qef_group;
	qef_cb->qef_aplid = qef_rcb->qef_aplid;


	/*
	** Initialize statement error handling action.
	** Default is to abort just the current statement.
	*/
	qef_cb->qef_stm_error = QEF_STMT_ROLLBACK;

	/*
	** initialize session flags
	*/
	qef_cb->qef_sess_flags = 0;

	/*
	** initialize qen spill threshold:  spill when we reach the calculated
	** limit.
	*/

	qef_cb->qef_spillThreshold = QEF_NO_SPILL_THRESHOLD;

	if (qef_rcb->qef_sess_startup_flags & QEF_UPGRADEDB_SESSION)
	    qef_cb->qef_sess_flags |= QEF_RUNNING_UPGRADEDB;

	/* Initialize the session trace vector: expect a lint error message */
	/* Note: ult will think that every trace point below 128 can have
	** a value, allow enough room for multiple trace points set
	*/
	ult_init_macro(&qef_cb->qef_trace, QEF_SSNB, 128, QEF_SNVAO);

	/* Set the global variable Qef_s_cb to the value sent in the 
	** qef_rcb->qef_server.  However, since Qef_s_cb is a global
	** variable, we must lock the semaphore */
	status = CSp_semaphore(QEF_EXCLUSIVE, &qef_scb->qef_sem);
	if (status != E_DB_OK)
	{
	    qef_error(E_QE0098_SEM_FAILS, 0L, E_DB_ERROR, &err, 
		      &qef_rcb->error, 0);
	    status = E_DB_ERROR;
	    break;
	}
	Qef_s_cb = qef_scb;

	/* Prepend the current session control block to server-wide list */
	qef_cb->qef_prev = (QEF_CB *)NULL;
	if (Qef_s_cb->qef_qefcb == (PTR)NULL)
	{
	    qef_cb->qef_next = (QEF_CB *)NULL;
	}
	else
	{
	    ((QEF_CB *)(Qef_s_cb->qef_qefcb))->qef_prev = qef_cb;
	    qef_cb->qef_next = (QEF_CB *)(Qef_s_cb->qef_qefcb);
	}
	Qef_s_cb->qef_qefcb = (PTR)qef_cb;

	/* Release the semaphore */
	status = CSv_semaphore(&Qef_s_cb->qef_sem);
	if (status != E_DB_OK)
	{
	    qef_error(E_QE0098_SEM_FAILS, 0L, E_DB_ERROR, &err, 
			&qef_rcb->error, 0);
	    status = E_DB_ERROR;
	    break;
	}

	/* allocate stream for session's dedicated use */

	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
	/* Tell ULM where we want streamid returned */
	ulm.ulm_streamid_p = &qef_cb->qef_c3_streamid;
	/* Open and allocate TRformat buffer */
	ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm.ulm_psize = QEF_TRFMT_SIZE;

	status = qec_mopen(&ulm);
	if (status)
	{
		qef_rcb->error.err_code = ulm.ulm_error.err_code;
		return(status);
	}

	qef_cb->qef_c4_streamsize = 0;

	/* TRformat buffer is now allocated */

	qef_cb->qef_trfmt = ulm.ulm_pptr;
	qef_cb->qef_trsize = QEF_TRFMT_SIZE;

	/* Set session characteristics */
	status = qec_alter(qef_cb);
	if (status != E_DB_OK)
	    break;

	status = qec_register_interrupt_handler(qef_cb);
	if (status != E_DB_OK)
	    break;

	if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	{
	    /* a DDB session requires associated RQF and TPF sessions, 
	    ** established in that order */

	    status = qed_q3_beg_ses(qef_rcb);
	    if (status)
		return(status);

	}

    } while (0);

    return (status);
}

/*{
** Name: QEC_END_SESSION - end a session
**
** External QEF call:   status = qef_call(QEC_END_SESSION, &qef_rcb);
**
** Description:
**	This operation aborts any outstanding transactions
**  and then calls the environment manager to remove any DSHs
**  for this session. Only non-repeat query DSHs will be removed.
**
** Inputs:
**      qef_rcb
**		.qef_eflag		designate error handling semantis
**					for user errors.
**		   QEF_INTERNAL		return error code.
**		   QEF_EXTERNAL		send message to user.
**		.qef_cb			session control block
**
** Outputs:
**	    qef_rcb
**              .error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0017_BAD_CB
**                                      E_QE0018_BAD_PARAM_IN_CB
**                                      E_QE0019_NON_INTERNAL_FAIL
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0003_INVALID_SPECIFICATION
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written
**	12-may-88 (puree)
**	    Always call qee_cleanup to clean the environment.
**	22-aug-88 (carl)
**	    modified to terminate associated TPF and RQF sessions for
**	    DDB session 
**	06-jul-89 (carl)
**	    Modified sequence of processing to allow STAR to reinitialize
**	    the states of used REPEAT QUERIES (done by qed_q4_end_ses) 
**	    before all space is released
**  
**	    Modified to close the stream opened for this session's use
**	14-mar-94 (swm)
**	    Bug #59883
**	    Allocate and release ULM stream for all QEF sessions, not just
**	    for RQF and TPF sessions associated with DDB sessions, in
**	    qec_begin_session() and qec_end_session() respectively. Allocate
**	    a TRformat buffer per QEF session rather than relying on automatic
**	    buffer allocation on the stack; this is to eliminate the
**	    probability of stack overrun owing to large buffers.
**	30-jul-1997 (nanpr01)
**	    Set the streamid to NULL after memory deallocation.
*/
DB_STATUS
qec_end_session(
QEF_CB       *qef_cb )
{
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    QEF_RCB	*qef_rcb = qef_cb->qef_rcb;
    DB_STATUS   status;
    ULM_RCB	ulm;
    i4	err;

    /* if there is a transaction in progress, abort it */
    if (qef_cb->qef_stat != QEF_NOTRAN)
    {
	/* should tell user that transaction is being aborted */
	qef_rcb->qef_spoint = (DB_SP_NAME*) NULL;
	qef_rcb->qef_modifier = qef_cb->qef_stat;
	status = qet_abort(qef_cb);
    }

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
    {
	if (qef_cb->qef_c2_ddb_ses.qes_d1_ok_b)
	{
	    /* a DDB session requires the associated TPF and RQF sessions be
	    ** terminated, in that order */

	    status = qed_q4_end_ses(qef_rcb);
	    if (status)
		qef_error(qef_rcb->error.err_code, 0L, status, &err, 
		    &qef_rcb->error, 0);
	}
    }

    /* De-reference TRformat buffer from session control block */

    qef_cb->qef_trfmt = (char *)0;
    qef_cb->qef_trsize = 0;

    /* release session's dedicated stream */

    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_streamid_p = &qef_cb->qef_c3_streamid;
    status = ulm_closestream(&ulm);
    if (status != E_DB_OK)
    {
	qef_rcb->error.err_code = ulm.ulm_error.err_code;
	qef_error(qef_rcb->error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
    }
    /* ULM has nullified qef_c3_streamid */

    /* Remove dynamic DSH stuff */
    status = qee_cleanup(qef_rcb, (QEE_DSH **)NULL);
    if (status != E_DB_OK)
	qef_error(qef_rcb->error.err_code, 0L, status, &err, &qef_rcb->error, 0);

    /* Remove the session control block from the server link list.
    ** Must acquire the server semaphore */

    status = CSp_semaphore(QEF_EXCLUSIVE, &Qef_s_cb->qef_sem);
    if (status == E_DB_OK)
    {
	/* For the cb that is the 1st element of the list */
	if (Qef_s_cb->qef_qefcb == (PTR)qef_cb)    
	{
	    Qef_s_cb->qef_qefcb = (PTR)qef_cb->qef_next;
	    if(qef_cb->qef_next != (QEF_CB *)NULL)
		qef_cb->qef_next->qef_prev = (QEF_CB *)NULL;
	}
	/* For others */
	else 
	{
	    qef_cb->qef_prev->qef_next = qef_cb->qef_next;
	    if(qef_cb->qef_next != (QEF_CB *)NULL)
		qef_cb->qef_next->qef_prev = qef_cb->qef_prev;
	}

	/* Release the semaphore */
	status = CSv_semaphore(&Qef_s_cb->qef_sem);
	if (status != E_DB_OK)
	{
	    qef_error(E_QE0098_SEM_FAILS, 0L, E_DB_ERROR, &err, 
			&qef_rcb->error, 0);
	}
    }
    else
    {
	qef_error(E_QE0098_SEM_FAILS, 0L, E_DB_ERROR, &err, &qef_rcb->error, 0);
    }

#ifdef xDEBUG
    /* ASSERTION: The session must not hold any sort buffer */
    if (qef_cb->qef_s_used != 0)
    {
	qef_error(E_QE1010_HOLDING_SORT_BUFFER, 0L, E_DB_ERROR, 
		    &err, &qef_rcb->error, 0);
    }
#endif

    return (E_DB_OK);	/* Always */
}


/*{
** Name: QEC_ALTER      - change session operating characteristics
**
** External QEF call:   status = qef_call(QEC_ALTER, &qef_rcb);
**
** Description:
**      The session and QEF portion of the server are initialized 
**  with certain operating characteristics. In order to change server
**  level characteristics, it is necessary to re-init QEF. However,
**  changes to session characteristics can be made through this command.
**  The current set of options at the server level are:
**
**      none so far.
**      
**
** Inputs:
**	    qef_rcb
**		 .qef_cb		 session control block
**		 .qef_eflag		 designate error handling semantis
**					 for user errors.
**		    QEF_INTERNAL	 return error code.
**		    QEF_EXTERNAL	 send message to user.
**               .qef_asize              number of elements being altered
**               .qef_alt                item and new value array
**
**          <qef_alt> looks like
**		.alt_length	    length of item
**              .alt_code	    item to change
**              .alt_value          new value
**		.alt_rlength	    result length
**
** Outputs:
**
**          <qef_alt>
**		.alt_rlength	    result length (if not null)
**	    
**	    qef_rcb
**              .error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0017_BAD_CB
**                                      E_QE0018_BAD_PARAM_IN_CB
**                                      E_QE0019_NON_INTERNAL_FAIL
**                                      E_QE0001_INVALID_ALTER
**                                      E_QE0002_INTERNAL_ERROR
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written
**	09-apr-89 (ralph)
**	    GRANT Enhancements, Phase 2:
**	    Added support for database privileges;
**	    qec_alter sets the session's dbprivs.
**	23-jun-89 (ralph)
**	    GRANT Enhancements, Phase 2e:
**	    Added QEC_IGNORE request code for qec_alter().
**	30-jan-90 (ralph)
**	    Added QEC_USTAT request code for qec_alter().
**	25-feb-91 (rogerk)
**	    Added QEC_ONERROR_STATE mode to set error handling actions.
**	03-feb-93 (andre)
**	    added QEC_DBID and QEC_UDBID
**	05-feb-93 (andre)
**	    added QEC_EFF_USR_ID
*/
DB_STATUS
qec_alter(
QEF_CB             *qef_cb )
{
    i4	err;
    i4		asize = qef_cb->qef_rcb->qef_asize;
    QEF_ALT	*alt  = qef_cb->qef_rcb->qef_alt;
    QEF_RCB	*rcb  = qef_cb->qef_rcb;

    rcb->error.err_code = E_QE0000_OK;

    if (asize == 0)
	return (E_DB_OK);

    if ((asize < 0) ||
	(alt == NULL))
    {
        qef_error(E_QE0018_BAD_PARAM_IN_CB, 0L, E_DB_ERROR, &err,
            &rcb->error, 0);
        return (E_DB_WARN);
    }

    for (; asize > 0; asize--, alt++)
    {
	if ((alt->alt_code <= 0) ||
	    (alt->alt_code >  QEC_MAX_ALT))
	{
	    qef_error(E_QE0001_INVALID_ALTER, 0L, E_DB_ERROR, &err,
		&rcb->error, 0);
	    return (E_DB_WARN);
	}
	
	switch (alt->alt_code)
	{
	case QEC_DBPRIV:
	    qef_cb->qef_fl_dbpriv = alt->alt_value.alt_u_i4;
	    break;
	
	case QEC_QROW_LIMIT:
	    qef_cb->qef_row_limit = alt->alt_value.alt_i4;
	    break;

	case QEC_QDIO_LIMIT:
	    qef_cb->qef_dio_limit = alt->alt_value.alt_i4;
	    break;

	case QEC_QCPU_LIMIT:
	    qef_cb->qef_cpu_limit = alt->alt_value.alt_i4;
	    break;

	case QEC_QPAGE_LIMIT:
	    qef_cb->qef_page_limit = alt->alt_value.alt_i4;
	    break;

	case QEC_QCOST_LIMIT:
	    qef_cb->qef_cost_limit = alt->alt_value.alt_i4;
	    break;

	case QEC_ONERROR_STATE:
	    /*
	    ** Set statement error handling mode. Supported modes:
	    **
	    **     - rollback statement
	    **     - rollback transaction
	    **     - rollback savepoint
	    */
	    if ((alt->alt_value.alt_i4 != QEF_STMT_ROLLBACK) &&
	    	(alt->alt_value.alt_i4 != QEF_XACT_ROLLBACK) &&
	    	((alt->alt_value.alt_i4 != QEF_SVPT_ROLLBACK) ||
		     (qef_cb->qef_rcb->qef_spoint == NULL)))
	    {
		qef_error(E_QE0001_INVALID_ALTER, 0L, E_DB_ERROR, &err,
		    &rcb->error, 0);
		return (E_DB_WARN);
	    }
	    qef_cb->qef_stm_error = alt->alt_value.alt_i4;

	    /*
	    ** If statement error handling mode is abort-to-savepoint, then
	    ** record the savepoint name to which we should abort.
	    */
	    if (qef_cb->qef_stm_error == QEF_SVPT_ROLLBACK)
	    {
		MEcopy((PTR) qef_cb->qef_rcb->qef_spoint, 
		       sizeof(DB_SP_NAME), (PTR) &qef_cb->qef_savepoint);
	    }
	    break;

	case QEC_USTAT:
	    qef_cb->qef_ustat = alt->alt_value.alt_i4;
	    break;

	case QEC_DBID:
	    qef_cb->qef_dbid = alt->alt_value.alt_ptr;
	    break;
	    
	case QEC_UDBID:
	    qef_cb->qef_udbid = alt->alt_value.alt_i4;
	    break;

	case QEC_EFF_USR_ID:
	    STRUCT_ASSIGN_MACRO((*alt->alt_value.alt_own_name),
		qef_cb->qef_user);
	    break;

	case QEC_IGNORE:
	    break;

	default:
	    qef_error(E_QE0001_INVALID_ALTER, 0L, E_DB_ERROR, &err,
		&rcb->error, 0);
	    return (E_DB_WARN);
	}
    }

    return (E_DB_OK);
}


/*{
** Name: QEC_DBG        - dump various data structures
**
** External QEF call:   status = qef_call(QEC_DEBUG, &qef_dbg);
**
** Description:
**      Dump the specified data structures. The standard tracing mechanism
** will be in used so output can be redirected.
**
** Errors in call specification are ignored.
**
** Inputs:
**      qef_dbg
**          .dbg_operation          id of debug operation
**              QEF_PSES            print session control block
**              QEF_PSER            print server control block
**                                  (no address is needed)
**              QEF_PQP             print runtime environment
**                                  (address is of DSH)
**              QEF_PQEP            print a query execution plan
**          .dbg_addr               address of structure (if applicable)
**
** Outputs:
**      none.
**
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written.
**      06-may-93 (anitap)
**          Casting to get rid of compiler warning.
[@history_line@]...
*/
DB_STATUS
qec_debug(
QEF_DBG        *qef_dbg )
{
#ifdef xDEBUG
    switch (qef_dbg->dbg_operation)
    {
        case QEF_PSES:
            qec_pss((QEF_CB*) qef_dbg->dbg_addr);
            break;
        case QEF_PSER:
            qec_psr(Qef_s_cb);
            break;
        case QEF_PQP:
            qec_pqp((QEF_QP_CB*) qef_dbg->dbg_addr);
            break;
        case QEF_PQEP:
            qec_pqep((QEN_NODE *)qef_dbg->dbg_addr);
            break;
	case QEF_PDSH:
	    qec_pdsh((QEE_DSH*) qef_dbg->dbg_addr);
	    break;
    }
#endif
    return (E_DB_OK);
}

/*{
** Name: QEC_TRACE      - set trace flags
**
** External QEF call:   status = qec_trace(&db_debug_cb);
**
** Description:
**      Set and clear trace points. Numbers 0 - 127 are session trace
**  points, all others are server trace points. 
**
**
**  LDB Trace points			Meaning
**  ----------------			-------
**
**	1-39				(reserved)
**
**  Common Trace points			Meaning
**  -------------------			-------
**
**	40				Write the name of every QEF operation 
**					to the log.
**
**  DDB Trace points			Meaning
**  ----------------			-------
**
**	50				Skip destroying temporary tables 
**					when query plan is destroyed by
**					qee_destroy to allow OPF/OPC to
**					debug multi-site query problems.  
**					Such survived temporary tables
**					must be manually cleaned up.
**
**	51				Validate every query plan.
**	55				Log the every query or repeat plan.
**	56				Log every QEF call.
**	57				Log every DDL command.
**	58				Log every cursor operation.
**	59				Log query or repeat plan if error.
**
**  QEN/STAR Trace points		Meaning
**  ---------------------		-------
**
**	60-99				TPF's 2PC trace points under QEF, 
**					temporarily assigned to TPF until 
**	100-127				(temporarily reserved for RQF)
**
** Inputs:
**      debug_cb
**	    .db_trswitch                operation to perform
**              DB_TR_NOCHANGE          do nothing
**              DB_TR_ON                set trace point
**              DB_TR_OFF               clear trace point
**          .db_trace_point             trace point number
**          .db_vals[2]                 optional values
**          .db_value_count             number of optional values
**
** Outputs:
**	none
**
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written
**	11-aug-88 (puree)
**	    fix bug setting session/server trace flag.
**	24-apr-90 (carl)
**	    added 2PC trace points for TPF under QEF
**	21-aug-90 (carl)
**	    added comment to qec_trace for new trace points:
**		qe50 for skipping destroying temporary tables at the end of
**		     every DDB query, 
**		qe51 for validating every DDB QP, 
**
**		qe55 for tracing every DDB query/repeat plan, 
**		qe56 for tracing every QEF call,
**		qe57 for tracing every DDB DDL command,
**		qe58 for tracing every DDB cursor operation, 
**              qe59 for tracing DDB query/repeat plan when an error occurs
**	07-oct-90 (carl)
**	    changed to call qed_u17_tpf_call isntead of tpf_call
**	21-oct-90 (carl)
**	    changed qec_trace to call tpf for trace points only if they are 
**	    in range
*/
DB_STATUS
qec_trace(
DB_DEBUG_CB		*debug_cb )
{
    DB_STATUS		status;
    i4             tr_point;
    i4             val1;
    i4             val2;
    bool                sess_point = FALSE;
    bool		tr_immediate = FALSE;
    QEF_CB		*qef_cb;
    QEF_RCB		qer,
			*qer_p = & qer;
    QES_DDB_SES		*dds_p;
    QES_QRY_SES		*qss_p;
    TPR_CB		tpr_cb,
			*tpr_p = & tpr_cb;
    RQR_CB		rqr_cb,
			*rqr_p = & rqr_cb;
    CS_SID		sid;

    /* get the qef control block from SCF */
    CSget_sid(&sid);
    qef_cb = GET_QEF_CB(sid);

    MEfill(sizeof(qer), '\0', (PTR) & qer);
    qer_p->qef_cb = qef_cb;    /* qef_cb must be initialized */

    tr_point = debug_cb->db_trace_point;
    if (tr_point == QEF_TRACE_IMMEDIATE)
    {
       tr_immediate = TRUE;
    }
    else if (tr_point < QEF_SSNB)
    {
	/* session trace point */
        sess_point = TRUE;       
    }
    else if (tr_point < QEF_SNB + QEF_SSNB)
    {
        sess_point = FALSE;       
        tr_point -= QEF_SSNB;
    }
    else
    {
        return (E_DB_ERROR);
    }

    /* There can be UP TO two values, but maybe they weren't given */
    if (debug_cb->db_value_count > 0)
        val1 = debug_cb->db_vals[0];
    else
        val1 = 0L;

    if (debug_cb->db_value_count > 1)
        val2 = debug_cb->db_vals[1];
    else
        val2 = 0L;

    if (tr_immediate)
    {
	return (qec_trace_immediate(qef_cb, debug_cb->db_value_count, val1, val2));
    }

    /*
    ** Three possible actions: Turn on flag, turn it off, or do nothing.
    */
    switch (debug_cb->db_trswitch)
    {
    case DB_TR_ON:
        if (sess_point)
	{
	    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	    {
		if (tr_point > (TPF_MIN_TRACE_60 - 1)  
		    && 
		    tr_point < (TPF_MAX_TRACE_119 + 1))
		{
		    dds_p = & qef_cb->qef_c2_ddb_ses;
		    qss_p = & dds_p->qes_d8_union.u2_qry_ses;

		    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
		    tpr_p->tpr_session = qef_cb->qef_c2_ddb_ses.qes_d2_tps_p;
		    tpr_p->tpr_rqf = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
		    tpr_p->tpr_lang_type = qss_p->qes_q2_lang;
		    tpr_p->tpr_18_dbgcb_p = debug_cb;

		    status = qed_u17_tpf_call(TPF_TRACE, tpr_p, qer_p);

		    /* ignore error */
		}
		if (tr_point > (RQF_MIN_TRACE_120 - 1)  
		    && 
		    tr_point < (RQF_MAX_TRACE_128 + 1))
		{
		    dds_p = & qef_cb->qef_c2_ddb_ses;
		    qss_p = & dds_p->qes_d8_union.u2_qry_ses;

		    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);
		    rqr_p->rqr_session = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
		    rqr_p->rqr_dbgcb_p = debug_cb;

		    status = qed_u3_rqf_call(RQR_TRACE, rqr_p, qer_p);

		    /* ignore error */
		}
	    }
            ult_set_macro(&qef_cb->qef_trace, tr_point, val1, val2);
	}
        else
            ult_set_macro(&Qef_s_cb->qef_trace, tr_point, val1, val2);
        break;

    case DB_TR_OFF:
        if (sess_point)
	{
	    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	    {
		if (tr_point > (TPF_MIN_TRACE_60 - 1)
		    && 
		    tr_point < (TPF_MAX_TRACE_119 + 1))
		{
		    dds_p = & qef_cb->qef_c2_ddb_ses;
		    qss_p = & dds_p->qes_d8_union.u2_qry_ses;

		    MEfill(sizeof(tpr_cb), '\0', (PTR) & tpr_cb);
		    tpr_p->tpr_session = qef_cb->qef_c2_ddb_ses.qes_d2_tps_p;
		    tpr_p->tpr_rqf = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
		    tpr_p->tpr_lang_type = qss_p->qes_q2_lang;
		    tpr_p->tpr_18_dbgcb_p = debug_cb;

		    status = qed_u17_tpf_call(TPF_TRACE, tpr_p, qer_p);

		    /* ignore error */
		}
		if (tr_point > (RQF_MIN_TRACE_120 - 1)  
		    && 
		    tr_point < (RQF_MAX_TRACE_128 + 1))
		{
		    dds_p = & qef_cb->qef_c2_ddb_ses;
		    qss_p = & dds_p->qes_d8_union.u2_qry_ses;

		    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);
		    rqr_p->rqr_session = qef_cb->qef_c2_ddb_ses.qes_d3_rqs_p;
		    rqr_p->rqr_dbgcb_p = debug_cb;

		    status = qed_u3_rqf_call(RQR_TRACE, rqr_p, qer_p);

		    /* ignore error */
		}
	    }

            ult_clear_macro(&qef_cb->qef_trace, tr_point);
	}
        else
            ult_clear_macro(&Qef_s_cb->qef_trace, tr_point);
        break;

    case DB_TR_NOCHANGE:
        /* Do nothing */
        break;

    default:
        return (E_DB_ERROR);
    }
    return (E_DB_OK);
}

static DB_STATUS
qec_trace_immediate(QEF_CB *qef_cb, i4 val_count, i4 val1, i4 val2)
{
    i4	status = E_DB_OK;
    ULM_RCB     ulm_rcb;
    EX_CONTEXT	context;
    char	*cp;
    i4		i;

    if (val_count < 1)
	return(E_DB_ERROR);

    switch (val1)
    {
	case QEF_TRACE_IMMEDIATE_ULM_POOLMAP:
	{
	    TRdisplay("%@ QEF_TRACE_IMMEDIATE_ULM_POOLMAP sort memory\n");
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	    _VOID_ ulm_mappool(&ulm_rcb);
	    TRdisplay("%@ QEF_TRACE_IMMEDIATE_ULM_POOLMAP DSH memory\n");
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm_rcb);
	    _VOID_ ulm_mappool(&ulm_rcb);
	    break;
	}

	case QEF_TRACE_IMMEDIATE_ULM:
	{
	    TRdisplay("%@ QEF_TRACE_IMMEDIATE_ULM sort memory\n");
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	    ulm_print_pool(&ulm_rcb);
	    TRdisplay("%@ QEF_TRACE_IMMEDIATE_ULM DSH memory\n");
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm_rcb);
	    ulm_print_pool(&ulm_rcb);
	    break;
	}

	default:
	{
#ifdef xDEBUG
	    if (val_count != 2 || val2 > 4096)
	    {
		status = E_DB_ERROR;
		break;
	    }

	    /* Assume this is valid memory pointer, size */
	    TRdisplay("%@ QEF_TRACE_IMMEDIATE dump memory %x - %d bytes\n",
		val1, val2);

	    /* Check for pointer in QEF sort memory */
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_s_ulmcb, ulm_rcb);
	    status = ulm_print_mem(&ulm_rcb, (PTR)val1, val2);
	    if (status == E_DB_OK)
		break;
	    /* Check for pointer in QEF DSH memory */
	    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm_rcb);
	    status = ulm_print_mem(&ulm_rcb, (PTR)val1, val2);
	    break;
#endif
	}
    }
    return (status);
}


/*{
** Name: QEC_INFO - return state information to the caller
**
** External QEF call:   status = qef_call(QEC_INFO, &qef_rcb);
**
** Description:
**	This operation returns to the caller various pieces
**  of state information.
**
** Inputs:
**	qef_cb				    session control block
**	    .qef_rcb			    caller's request block
**
** Outputs:
**      .qef_rcb
**	    .qef_version		    version number
**	    .qef_stat			    is there a transaction
**			QEF_NOTRAN	    no transaction
**			QEF_SSTRAN	    a single statement transaction.
**			QEF_MSTRAN	    a multi statement transaction
**	    .qef_auto
**			QEF_ON		    auto commit enabled
**			QEF_OFF		    auto commit disabled
**			
**	    .qef_open_count		    number of open cursors
**	    .error.err_code
**                                      E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-apr-86 (daved)
**          written
**	22-aug-88 (carl)
**	    modified to return level of QEF service in qef_r2_qef_lvl:
**	    DD_QEF_DDB_1 for Titan-style of DDB server, DD_QEF_DDB_0
**	    for regular server
**	10-oct-88 (puree)
**	    fix for transaction state and autocommit flag.
**      22-may-89 (davebf)
**          Fix qec_info to return QEF_SSTRAN for QEF_SSTRAN
**	23-apr-90 (davebf)
**	    Return qef_rcb->qef_stat based on qef_cb->qef_ostat.
**	    qef_ostat is assigned from qef_stat at end of each action
**	    and at end of each statement.  Therefore it represents the
**	    transaction status at the beginning of the current action
**	    or statement.
**	25-feb-91 (rogerk)
**	    Added statement error mode (qef_stm_error) and savepoint name
**	    for QEF_SVPT_ROLLBACK statement error mode (qef_savepoint) to
**	    qec_info return information.
**	12-may-93 (anitap)
**	    Added qef_upd_rowcnt for "SET UPDATE_ROWCOUNT" statement. 
**	21-jan-94 (iyer)
**	    In qec_info(), copy QEF_CB.qef_tran_id to QEF_RCB.qef_tran_id
**	    in support of dbmsinfo('db_tran_id').
[@history_line@]...
*/
DB_STATUS
qec_info(
QEF_CB       *qef_cb )
{

    qef_cb->qef_rcb->qef_version 	= QEF_VERSION;
    qef_cb->qef_rcb->qef_open_count 	= qef_cb->qef_open_count;
    qef_cb->qef_rcb->qef_stm_error	= qef_cb->qef_stm_error;
    qef_cb->qef_rcb->qef_spoint		= &qef_cb->qef_savepoint;
    qef_cb->qef_rcb->qef_upd_rowcnt 	= qef_cb->qef_upd_rowcnt; 
    qef_cb->qef_rcb->error.err_code	= E_QE0000_OK;

    if (qef_cb->qef_stat != QEF_NOTRAN)
    {
	STRUCT_ASSIGN_MACRO(qef_cb->qef_tran_id, qef_cb->qef_rcb->qef_tran_id);
    }
    else
    {
	qef_cb->qef_rcb->qef_tran_id.db_high_tran = 0;
	qef_cb->qef_rcb->qef_tran_id.db_low_tran  = 0;
    }
    
    switch (qef_cb->qef_ostat)
    {	
	case QEF_NOTRAN:
	case QEF_ITRAN:
	    qef_cb->qef_rcb->qef_stat = QEF_NOTRAN;
	    qef_cb->qef_rcb->qef_auto = qef_cb->qef_auto;
	    break;

	case QEF_SSTRAN:
	case QEF_RO_SSTRAN:
	    qef_cb->qef_rcb->qef_stat = QEF_SSTRAN;
	    qef_cb->qef_rcb->qef_auto = QEF_ON;
	    break;

	case QEF_MSTRAN:
	case QEF_RO_MSTRAN:
	    qef_cb->qef_rcb->qef_stat = QEF_MSTRAN;
	    qef_cb->qef_rcb->qef_auto = QEF_OFF;
	    break;
    }

    if (qef_cb->qef_c1_distrib & DB_3_DDB_SESS)
	qef_cb->qef_rcb->qef_r2_qef_lvl = DD_1QEF_DDB;
    else
	qef_cb->qef_rcb->qef_r2_qef_lvl = DD_0QEF_DDB;

    return (E_DB_OK);
}


/*{
** Name: QEC_MALLOC - QEF memory allocator
**
** Description:
**	This routine allocates QEF memory from by calling the ulm_palloc.
**	If there is not enough memory for the request, however, this 
**	routine will issue a call to ULH to destroy an object at the head
**	of its free list.  If the there is no object to be destroy, the
**	routine returns "no memory to the caller".
**
** Inputs:
**      ulm_rcb				    ptr to formated ulm_rcb
**
** Outputs:
**	none.
**      Returns:
**          DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-dec-87 (puree)
**          written
**	10-feb-88 (puree)
**	    rename qec_palloc to qec_malloc.
**	22-jul-88 (puree) 
**	    fix infinite loop due to incorrect else-if blocks
**	12-sep-88 (puree)
**	    Clean up the loop. Replace return with break statements.
**	24-Jan-2001 (jenjo02)
**	    Replaced ULH garbage collection code with qec_mfree().
**	20-Apr-2004 (schka24)
**	    Log which pool is full.  This is a hack, it would be better to
**	    improve the actual error msg, but that's "too hard" for now.
**	19-May-2004 (schka24)
**	    If I had had any sense I would have printed at least the
**	    failing size too.  Also I see I screwed up the block structure.
*/
DB_STATUS
qec_malloc(
ULM_RCB	    *ulm_rcb )
{
    while ( ulm_palloc(ulm_rcb) )
    {
	if ( qec_mfree(ulm_rcb) != E_DB_OK )
	{
	    TRdisplay("%@ Allocation failure from QEF %s pool, psize=%d\n",
		ulm_rcb->ulm_poolid == Qef_s_cb->qef_d_ulmcb.ulm_poolid ? "DSH"
		: (ulm_rcb->ulm_poolid == Qef_s_cb->qef_s_ulmcb.ulm_poolid ? "sorthash" : "?"),
		ulm_rcb->ulm_psize);
	    return(E_DB_ERROR);
	}
    }
    return(E_DB_OK);
}



/*{
** Name: QEC_MOPEN - Open a private memory stream
**
** Description:
**	This routine opens a private QEF memory stream by calling the 
**	ulm_openstream.  If there is not enough memory to open a stream
**	this  routine will issue a call to ULH to destroy an object at 
**	the head of its free list.  If the there is no object to be destroy,
**	the routine returns "no memory to the caller".
**
** Inputs:
**      ulm_rcb				    ptr to formated ulm_rcb
**
** Outputs:
**	none.
**      Returns:
**          DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-feb-88 (puree)
**          written
**	12-aug-88 (puree)
**	    Prevent retry loop if error other than NOMEM is received from
**	    ULM.
**	24-Jan-2001 (jenjo02)
**	    Replaced ULH garbage collection code with qec_mfree().
**	19-May-2004 (schka24)
**	    In case we're open-and-palloc'ing, add failure trace from
**	    qec-malloc (above).
*/
DB_STATUS
qec_mopen(
ULM_RCB	    *ulm_rcb)
{
    /* Open a private, thread-specific stream */
    ulm_rcb->ulm_flags |= ULM_PRIVATE_STREAM;

    while ( ulm_openstream(ulm_rcb) )
    {
	if ( qec_mfree(ulm_rcb) != E_DB_OK )
	{
	    TRdisplay("%@ Allocation failure from QEF %s pool, psize=%d\n",
		ulm_rcb->ulm_poolid == Qef_s_cb->qef_d_ulmcb.ulm_poolid ? "DSH"
		: (ulm_rcb->ulm_poolid == Qef_s_cb->qef_s_ulmcb.ulm_poolid ? "sorthash" : "?"),
		ulm_rcb->ulm_psize);
	    return(E_DB_ERROR);
	}
    }
    return(E_DB_OK);
}

/*{
** Name: QEC_MFREE - Garbage collect QEF memory to free up space.
**
** Description:
**	Calls ULH to LRU-destroy a QEF object, thereby freeing
**	up memory from QEF's ULM pool.
**
** Inputs:
**      ulm_rcb				    ptr to formated ulm_rcb
**
** Outputs:
**	none.
**      Returns:
**          E_DB_OK	if an object was destroyed.
**	    E_DB_ERROR  if nothing left to destroy.
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	24-Jan-2001 (jenjo02)
**	    Replaced ULH garbage collection code with this function.
**	13-Jul-2006 (kschendel)
**	    Trace garbage collections.
*/
DB_STATUS
qec_mfree(
ULM_RCB	    *ulm_rcb)
{
    ULH_RCB	ulh_rcb;

    ulh_rcb.ulh_hashid = Qef_s_cb->qef_ulhid;	
    ulh_rcb.ulh_object = (ULH_OBJECT *)NULL;

    TRdisplay("%@ QEF DSH reclaim because of request size %u\n",
	ulm_rcb->ulm_psize);

    if ( ulh_destroy(&ulh_rcb, (unsigned char *) NULL, 0) )
    {
	if ( ulh_rcb.ulh_error.err_code != E_UL0109_NFND )
	    ulm_rcb->ulm_error.err_code = ulh_rcb.ulh_error.err_code;
	return(E_DB_ERROR);
    }
    return(E_DB_OK);
}

/*{
** Name: QEC_TPRINTF	- Perform a printf through SCC_TRACE
**
** Description:
**      This routine performs the printf function by returning the info to 
**      be displayed through SCC_TRACE. 
**
** Inputs:
**	qef_rcb -
**	    The current request control block to QEF. This is here for future
**	    expansion, since you can get most anywhere interesting from the
**	    rcb.
**	cbufsize -
**	    The formatted buffer size.
**
**	cbuf -
**	    The formatted buffer.
**
** Outputs:
**	none
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    The string goes to the FE.
**
** History:
**      17-may-89 (eric)
**          Created
**	24-may-89 (jrb)
**	    Renamed scf_enumber to scf_local_error.
**	31-may-89 (neil)
**	    Clean up - no local error number; session id passed; error change.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Made qec_tprintf portable by eliminating p1 .. p2 PTR paramters,
**	    instead requiring caller to pass a string already formatted by
**	    STprintf. This is necessary because STprintf is a varargs function
**	    whose parameters can vary in size; the current code breaks on
**	    any platform where the STprintf size types are not the same (these
**	    are usually integers and pointers).
**	    This approach has been taken because varargs functions outside CL
**	    have been outlawed and using STprintf in client code is less
**	    cumbersome than trying to emulate varargs with extra size or
**	    struct parameters.
*/
VOID
qec_tprintf(qef_rcb, cbufsize, cbuf)
QEF_RCB		*qef_rcb;
i4		cbufsize;
char		*cbuf;
{
    SCF_CB	scf_cb;
    DB_STATUS   scf_status;


    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_len_union.scf_blength = STnlength(cbufsize, cbuf);
    scf_cb.scf_ptr_union.scf_buffer = cbuf;
    scf_cb.scf_session = qef_rcb->qef_cb->qef_ses_id;
    scf_status = scf_call(SCC_TRACE, &scf_cb);
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error displaying trace data to user\n");
	TRdisplay("QEF trace message is:\n%s", cbuf);
    }
}

/*{
** Name: qec_trimwhite 	- Trim trailing white space.
**
** Description:
**	Utility to trim white space off of trace and error names.
**	It is assumed that no nested blanks are contained within text.
**
** Inputs:
**      len		Max length to be scanned for white space
**	buf		Buf holding char data (may not be null-terminated)
**
** Outputs:
**	Returns:
**	    Length of non-white part of the string
**	Exceptions:
**	    None.
**
** Side Effects:
**	    None.
**
** History:
**	30-may-89 (neil)
**	    Written for rules tracing and errors.
**	01-mar-93 (andre)
**	    rewrote to handle embedded blanks which can appear inside names 
**	    expressed as delimited identifiers.
*/
i4
qec_trimwhite(
i4	len,
char	*buf )
{
    register char   *ep;    /* pointer to end of buffer (one beyond) */
    register char   *p;
    register char   *save_p;

    if (!buf)
    {
       return (0);
    }

    for (save_p = p = buf, ep = p + len; (p < ep) && (*p != EOS); )
    {
        if (!CMwhite(p))
            save_p = CMnext(p);
        else
            CMnext(p);
    }

    return ((i4)(save_p - buf));
} /* qec_trimwhite */

/*{
** Name: qec_check_for_interrupt - Check for user interrupt
**
**
** Description:
** Check qef_interrupt in the QEF_CB to see if an interrupt has occured.
** If one has - clear qef_interrupt, set the error condition to
** E_QE0022_QUERY_ABORTED and return E_DB_ERROR.  Otherwise return
** E_DB_OK.
**
** Inputs:
**      qef_cb                       - pointer to QEF session control block
**
** Outputs:
**          DB_STATUS
** 
** Side Effects:
**	Will clear qef_interrupt.
**
** History:
**	15-jan-92 (jhahn)
**          initial creation
*/
/*DB_STATUS
qec_check_for_interrupt(qef_cb)
QEF_CB *qef_cb;
{
    if (qef_cb->qef_interrupt)
    {
	qef_cb->qef_rcb->error.err_code = E_QE0022_QUERY_ABORTED;
	qef_cb->qef_interrupt = FALSE;
	return (E_DB_ERROR);
    }
    return (E_DB_OK);
}*/

/*{
** Name: qec_interrupt_handler - Process a QEF asynchronous interrupt
**
** External call format:
**	qec_interrupt_handler((i4)eventid, (PTR)qef_cb);
**
** Description:
**      This entry point should only be called by SCF and it handles
**      an asynchronous interrupt of the QEF, such as a control-C
**      from a user.  The control-C interrupt will only affect the session
**      selected.  A local flag will be set by this routine in the respective
**      session control block and the routine will return.  This
**      session local flag will be monitored periodically by the QEF 
**      and if this flag  is set then an orderly stoppage of that session's 
**      operation will be made.  
**         Interrupts will be disabled when critical data structures
**      are being updated, so that these structures will not be
**      left in an inconsistent state.
**
** Inputs:
**      eventid                      - indication of event
**      ops_cb                       - pointer to OPF session control block
**                                     of session which was interrupted
** Outputs:
** 
** Side Effects:
**	The session control block interrupt flag will be updated to indicate
**      that an interrupt has occurred.
** History:
**	11-mar-92 (jhahn)
**          initial creation
*/

static STATUS qec_interrupt_handler(
i4		   eventid,
QEF_CB             *qef_cb )
{
    qef_cb->qef_user_interrupt = TRUE;	/* Set flag indicating interrupt has
                                        ** occurred.
					*/
    return(OK);
}

/*{
** Name: qec_register_interrupt_handler	- initialize the AIC and PAINE handlers
**
** Description:
**      This routine will inform SCF of the AIC and PAINE handler.
**	It will also initialize the qef_cb fields needed to support
**	the handler.
**
** Inputs:
**	qef_cb				    - ptr to caller's control block
**
** Outputs:
**	Returns:
**	    E_DB_OK
**          E_DB_ERROR - if initialization failed
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      11-mar-92 (jhahn)
**          initial creation
*/
static DB_STATUS
qec_register_interrupt_handler(
QEF_CB		*qef_cb )
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;
    i4		   err;

    qef_cb->qef_user_interrupt = FALSE;
    qef_cb->qef_user_interrupt_handled = FALSE;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_ptr_union.scf_afcn = qec_interrupt_handler;
    scf_cb.scf_session = qef_cb->qef_ses_id;
    scf_cb.scf_nbr_union.scf_amask = SCS_PAINE_MASK;
    scf_status = scf_call(SCS_DECLARE, &scf_cb);

    if (DB_FAILURE_MACRO(scf_status))
    {
	qef_error(scf_cb.scf_error.err_code, 0L, E_DB_ERROR, &err,
            &qef_cb->qef_rcb->error, 0);
        return(E_DB_ERROR);
    }

    scf_cb.scf_nbr_union.scf_amask = SCS_AIC_MASK;
    scf_status = scf_call(SCS_DECLARE, &scf_cb);

    if (DB_FAILURE_MACRO(scf_status))
    {
	qef_error(scf_cb.scf_error.err_code, 0L, E_DB_ERROR, &err,
            &qef_cb->qef_rcb->error, 0);
	return(E_DB_ERROR);
    }

    return(E_DB_OK);
}
