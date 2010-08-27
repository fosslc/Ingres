/* Copyright (c) 2010 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <bt.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cs.h>
#include    <lk.h>
#include    <tm.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <gca.h>
#include    <copy.h>
#include    <qsf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmscb.h>
#include    <dmucb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qefcopy.h>
#include    <qeuqcb.h>
#include    <qefscb.h>
#include    <rdf.h>
#include    <tm.h>
#include    <usererror.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>
#include    <cut.h>

/**
**
**  Name: QEUCOPY.C - COPY statement functions for QEF
**
**  Description:
**	These routines implement the QEF part of the COPY statement.
**	QEF's job is relatively simple, at least conceptually:
**	for a COPY INTO, get rows from the table and fire them off
**	to the outside world.  For a COPY FROM, receive rows from
**	the outside and send them into the table.  All translation
**	and formatting is done elsewhere (at the client, actually),
**	so we're just moving data around.
**
**	There are a couple complications to this simple process.
**	First, a COPY FROM might be a bulk copy.  This means that
**	we're calling DMR_LOAD instead of DMR_PUT to stash the rows.
**	There are setup complications, and an extra "end" call to
**	DMF is needed.  Bulk copy actually sends the rows directly
**	into the sorter (for sorted structures) and builds the table
**	directly instead of row-by-row.  DMF gets to worry about 99%
**	of this, but we need to do the proper setup and finish.
**
**	The other complication, and a fairly substantial one, is
**	partitioned tables.  Ideally we want to parallelize the
**	COPY process.  For COPY FROM in particular, where we have
**	to redirect the incoming row to potentially any partition,
**	it's necessary to be a little careful so as to not cause
**	unreasonable resource usage.
**
**	Routines defined in this file are:
**
**          qeu_b_copy  - begin copy
**          qeu_e_copy  - end copy
**          qeu_r_copy  - read a copy block
**          qeu_w_copy  - write a copy block
**
**
**
**  History:
**	8-Apr-2004 (schka24)
**	    Extract from qeus.c for partitioned COPY.  (See qeus.c for
**	    earlier module history relevant to COPY.)
**	    Rework to implement partitioned table COPY.  Use one CUT
**	    worker thread for COPY INTO and row-wise COPY FROM.  Let
**	    DMF handle the fancy stuff for bulk COPY FROM.
**	30-Apr-2004 (schka24)
**	    It seems we need shared transaction context after all,
**	    or else copy from doesn't catch force-abort.  And it
**	    flogs the transaction log right off the end...
**	5-May-2004 (schka24)
**	    Manage session temp flag properly in children.
**      18-jun-2004 (stial01)
**          Fix initialization of missing blob columns (b112521)
**	16-dec-04 (inkdo01)
**	    Added a few collID's.
**	14-Jun-2006 (kschendel)
**	    Implement sequence-valued column defaults.
**	13-Feb-2007 (kschendel)
**	    Replace CSswitch with better CScancelCheck.
**	4-Jun-2009 (kschendel) b122118
**	    Make sure dmt-cb doesn't have junk in it.
**	19-Mar-2010 (kschendel) SIR 123448
**	    Bulk-load partitioned tables if DMF allows it.
**	    Partitioned copy from will continue to use a child thread,
**	    as it makes it much easier to batch up the which-part'ed rows
**	    for DMF.  Split up row-reader loops for unpartitioned vs
**	    partitioned, do partitioned with DIRECT read and batching.
**	14-apr-2010 (toumi01) SIR 122403
**	    Add support for column encryption.
**/


/* Name: QEU_COPY_CTL - Copy control info
**
** Description:
**	The copy control info block contains stuff that's needed strictly
**	locally to the qeucopy.c module.  Therefore it's defined right
**	here, rather than attempting to wade into the morass of QEF header
**	files.
**
**	Basically, the QEU_COPY_CTL block contains whatever we need to
**	communicate what-to-do from the QEU_B_COPY begin operation,
**	to the copy worker thread(s), and back up to the QEU_E_COPY end
**	operation.
**
**	The QEU_COPY_CTL block is allocated from QEF (DSH) memory.
**	It's not all that big by itself, but it's likely that the
**	copy operation will require yet additional memory, so we might
**	as well open the stream and use it.
**
** History:
**	25-Apr-2004 (schka24)
**	    Partition-enabled COPY.
**	5-May-2004 (schka24)
**	    Add session-temp flag.
**	01-Jul-2004 (jenjo02)
**	    CUT_BUFFER renamed to CUT_RCB.
**	12-Apr-2010 (kschendel) SIR 123485
**	    dmt_open no longer harms the flags, delete is-session-temp bool.
*/

typedef struct _QEU_COPY_CTL
{
    QEU_COPY	*qeu_copy;		/* Outside-world COPY control block */

    /* copy BEGIN opens the (master) table with this DMT_CB, partly to
    ** set all the proper mode values, but mostly to verify the table
    ** timestamp to ensure that the copy is valid.
    */
    DMT_CB	dmtcb;			/* Table open/close request block */
    /* copy BEGIN does a (master) DMT SHOW to get table info. */
    DMT_TBL_ENTRY tbl_info;		/* (Master) table info from SHOW */

    /* TRUE if we are doing a bulk-load (COPY FROM only, obviously).
    ** For partitioned tables, the decision is on a per-partition basis,
    ** and the use-load flag just tells the child that copy startup
    ** thinks load might be possible.  (and opened the table with X lock.)
    ** Once the COPY is started (ie once qeu_b_copy is done), use_load
    ** is only set if it's a non-partitioned load.
    */
    bool	use_load;		/* Use LOAD instead of row-PUT */

    /* The parser creates a QEF_RCB for the copy and that is passed in
    ** to the COPY routines.
    */
    QEF_RCB	*qef_rcb;		/* Copy request control block */

    ULM_RCB	ulm;			/* Opened QEF memory stream */

    /* When CUT is being used (ie not bulk load), the CUT buffer is
    ** allocated by the parent, in QEU_B_COPY.  Its handle is stored here.
    */
    CUT_RCB	cut_rcb;		/* CUT buffer handle & req block */

    /* If special attribute processing is needed, tmptuple points to a
    ** "qeu_ext_length" sized area for holding a row while processing.
    */
    PTR		tmptuple;		/* Temp row pointer */

    /* Row estimate if given */
    i4		row_est;		/* COPY FROM row estimate */

    DB_STATUS	child_status;		/* Child completion status */
    DB_ERROR	child_error;		/* Child error info for parent */
    i4		state;			/* State flags CCF_xxxx */

#define CCF_ABORT	0x0001		/* Child or parent is in trouble */

} QEU_COPY_CTL;


/* Misc definitions */

/* Size of CUT buffer to shoot for.  Ideally this would be large enough
** to batch up bunches of rows for the partitioned bulk-load case.
** It should be larger than the GCA buffer (currently anywhere from 4k
** to 12k), otherwise it's not worthwhile.
*/
#define COPY_CUT_BUFSIZE (512*1024)	/* 512K target sounds reasonable */


/*
** forward declarations
*/

static DB_STATUS copy_from_child(SCF_FTX *ftx);

static void copy_from_unpartitioned(
	QEU_COPY_CTL *copy_ctl,
	CUT_RCB *cut_rcb,
	PTR tran_id);

static void copy_from_partitioned(
	QEU_COPY_CTL *copy_ctl,
	CUT_RCB *cut_rcb,
	PTR tran_id,
	bool use_load);

static DB_STATUS copy_into_child(SCF_FTX *ftx);

/*
**  Name: QEU_B_COPY     - begin the copy operation
**  
**  External QEF call:   status = qef_call(QEU_B_COPY, &qef_rcb);
**  
**  Description:
**      Initialize for COPY command so that subsequent calls to QEU_R_COPY
**      or QEU_W_COPY can be made.  A single-statement transaction is started
**      and the relation is opened for reading (if copy from) or writing 
**      (copy into). 
**  
**	The table (master if partitioned) is opened so that we can check
**	the timestamp against what the parser-built QEU_COPY block has.
**	If the table timestamp is newer, the COPY is rejected with a
**	"retry" error.  SCF will send the query through the parser again.
**
**	COPY INTO is relatively straightforward.  One worker thread is
**	started, which simply reads the table (one partition at a time,
**	if the table is partitioned).  Rows are stuffed through a CUT
**	buffer, where they will eventually be picked up by QEU_W_COPY
**	calls for transmission to the user.
**
**	COPY FROM is a little fancier because there are two styles:
**	row-by-row, using DMR_PUT's, and bulk load, using DMR_LOAD.
**	Partitioned tables worry about bulk-load in the child thread,
**	for reasons explained there.  For non-partitioned tables:
**	If there is any hope of using bulk-load, we'll ask DMF to
**	start it up.  Assuming DMF agrees to the bulk-load idea, no
**	child thread or CUT buffering is done here, as there will be
**	plenty of wild and crazy stuff happening in DMF.
**
**	If bulk-load is out of the question, or is refused by DMF,
**	we'll create a child thread to do the puts.  Rows coming from
**	the user will be sent to QEU_R_COPY, through a CUT buffer, and
**	into the table.  The child will take care of aiming the row at
**	the proper partition if need be.
**  
**      If an error is encountered opening the relation, the transaction begun
**      here will be aborted before returning.
**  
**  Inputs:
**       qef_rcb                      
**           .qef_sess_id            session id
**           .qef_db_id              database id
**           .qeu_copy               copy control block
**               .qeu_tab_id         relation table id
**               .qeu_tup_length     length of row in table
**               .qeu_direction      copy into or from
**                   QEU_INTO
**                   QEU_FROM
**		 .qeu_count	     Estimated rowcount from WITH clause.
**               .qeu_dmrcb          pointer to parser created DMR_CB
**				     (This DMR_CB is mostly empty and bogus
**				     and exists only to pass DMU_CHAR_ENTRY
**				     copy options through.  We probably
**				     should change this to a simple DM_DATA.)
**               
**  Outputs:
**       qef_rcb
**           .qeu_copy
**               .qeu_copy_ctl       initialized for calls to QEU_R_COPY or
**  				     QEU_W_COPY.
**           .error.err_code         One of the following
**                                   E_QE0000_OK
**                                   E_QE0002_INTERNAL_ERROR
**                                   E_QE0017_BAD_CB
**                                   E_QE0018_BAD_PARAM_IN_CB
**                                   E_QE0019_NON_INTERNAL_FAIL
**       Returns:
**           E_DB_OK
**           E_DB_ERROR                      caller error
**           E_DB_FATAL                      internal error
**       Exceptions:
**           none
**  
**  Side Effects:
**	Child thread may be created, any copy error after successful
**	return from this routine should call QEU_E_COPY to shut things
**	down cleanly.  (Even if the copy has failed.)
**  
**  History:
**	15-dec-86 (rogerk)
**	    written
**	14-mar-87 (daved)
**	    copy from must have writeable transaction
**	17-apr-87 (rogerk)
**	    Use qet_begin to start user transaction instead of qeu_btran.
**	19-nov-87 (puree)
**	    set savepoint pointer before using it.
**	24-mar-88 (puree)
**	    Add code to validate copy table timestamp.  Return
*	    E_QE0023_INVALID_QUERY if table is not found or if the 
**	    timestamp does not match.
**	08-aug-88 (puree)
**	    Watch for E_DM0114_FILE_NOT_FOUND after dmf call
**	20-sep-88 (puree)
**	    Fix bug checking error code from DMF.
**      01-aug-89 (jennifer)
**          Added code to audit copy operations.
**	05-oct-90 (davebf)
**	    Don't close all cursors for error in copy begin
**      16-may-90 (jennifer)
**          Add code to support empty table option.
**	16-oct-91 (bryanp)
**	    Part of 6.4 -> 6.5 merge changes. Must initialize new dmt_char_array
**	    field for DMT_SHOW calls.
**	8-Jan-1992 (rmuth)
**	    Before we do a fast-load we perform some checks to see if there
**	    is any data in the table. One of the checks is to make sure there
**	    is less than 16 pages in the table as we do not want to scan a
**	    large table to see if there is any data. The file allocation
**	    project added atleast two new pages to each table, a FHDR and one
**	    or more FMAPS depending on the size of the table. These need to
**	    be taken into account in the above check otherwise the code
**	    will reject a fast-load into an empty default hash table, which 
**	    has 18 pages. For all other table structures the number of pages
**	    is < 18 so increase the check size to 18 pages.
**	19-jan-93 (rickh)
**	    Corrected some merge problems caused by the Jan-18 integration.
**	26-apr-1993 (rmuth)
**	    COPY with ROW_ESTIMATE = n, place the estimated number of rows
**	    into the dmr_cb.
**	26-jul-1993 (rmuth)
**	    Enhance the COPY command to allow the following with clauses,
**	    ( FILLFACTOR, LEAFFILL, NONLEAFFILL, ALLOCATION, EXTEND, 
**	    MINPAGES and MAXPAGES). These are passed on in the 
**	    char_array.
**	8-nov-93 (stephenb)
**	    eliminated erroneous variable "err" and fixed error handling in
**	    call to qeu_secaudit().
**	20-jun-94 (jnash)
**	    When fast-loading heap tables, open table DM2T_A_SYNC_CLOSE.
**	    File synchronization will take place during DMT_CLOSE.  If DMF
**	    determines that fast load is inappropriate, close and reopen
**   	    table in normal write mode. 
**      19-dec-94 (sarjo01) from 22-Aug-94 (johna)
**          set dmt_cb.dmt_mustlock. Bug 58681
**      27-jun-96 (sarjo01)
**          Bug 75899:  qeu_b_copy(): make sure that qeu_copy->qeu_count is
**          reset to 0 before leaving here. If row_estimate was given in
**          the COPY FROM query, it is passed in qeu_copy->qeu_count, which
**          also serves as the running row count field. In the case of bulk
**          copy this field was getting cleared somewhere else, but not in
**          the case of non-bulk copy, causing the final rows added count
**          to include the row estimate.
**	28-oct-1998 (somsa01)
**	    In the case of Global Session Temporary Tables, make sure that
**	    its locks reside on the session lock list.  (Bug #94059)
**	12-nov-1998 (somsa01)
**	    Refined check for a Global Session Temporary Table. Also, removed
**	    unecessary extra code for using DMT_SHOW, as well as moving up
**	    the original call to it.  (Bug #94059)
**	06-aug-2003 (gupsh01)
**	    (Bug 110678) Copy statements, with clauses such as allocation 
**	    and minpages, fail during fast-load copy with error E_US14E4, 
**	    When row level locking is enabled as fast-load copy requires at 
**	    least table level locking. We will now escalate all lock levels 
**	    to LK_X which will cause lock level to be escalated to table level 
**	    when doing a fast-load copy.
**	25-Apr-2004 (schka24)
**	    Rework for partitioned COPY with CUT buffering.
**	11-May-2004 (schka24)
**	    Cleared row estimate before using it.  Oops.  Also, child threads
**	    need ADF, SXF.
**	16-Sep-2004 (schka24)
**	    Set parentage on CUT buffers, for cleanup.
**      15-Jan-2008 (hanal04) Bug 119736
**          COPY of IMA tables requies use of GWF. Make sure the factotum
**          thread creation flags its need for this facility.
**	19-Mar-2010 (kschendel) SIR 123488
**	    Allow bulk-load of partitioned tables.
**	    Allocate a row-temp (if needed) here, not in the parser.
**	26-jun-2010 (stephenb)
**	    Add intelligence to cope with small copies. If the copy is
**	    very small, we won't use the loader or create threads. Currently
**	    this flag is only set for the insert to copy optimization
**	04-aug-2010 (miket) 122403
**	    Trap attempt to copy a locked encrypted table.
**	05-aug-2010 (miket) SIR 122403
**	    Fix rookie error: dmt_enc_locked undefined unless E_DB_OK.
*/

DB_STATUS
qeu_b_copy(
QEF_RCB       *qef_rcb)
{
    bool		child_started = FALSE;
    CUT_RCB		*cut_rcb_ptr;
    DB_ERROR		cut_error;
    DB_STATUS		status = E_DB_OK;
    DMR_CB		dmr_cb;
    DMR_CB		*dmr_cb_ptr;
    DMS_CB		*dmscb;
    DMT_CHAR_ENTRY	ch_entry;
    DMT_SHW_CB		dmt_show;
    i4			error, error_code;
    i4			nattach;
    i4			row_est;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEU_COPY		*qeu_copy = qef_rcb->qeu_copy;
    QEU_COPY_CTL	*copy_ctl;
    SCF_CB		scfcb;
    SCF_FTC		ftc;
    ULM_RCB		ulm;

    /* Start by opening up a memory stream and allocating a copy control
    ** area to tie things together.
    ** NOTE: The memory stream opened is marked PRIVATE, but it's shared
    ** between the parent and worker threads.  To make this work, the
    ** parent thread is NOT ALLOWED to allocate from this stream as long
    ** as the child thread(s) may be alive.
    */
    STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
    ulm.ulm_flags |= ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
    ulm.ulm_psize = sizeof(QEU_COPY_CTL);
    status = qec_mopen(&ulm);
    if (DB_FAILURE_MACRO(status))
    {
	qef_error(ulm.ulm_error.err_code, 0, status, &error_code,
		&qef_rcb->error, 0);
 	return (status);
    }
    copy_ctl = (QEU_COPY_CTL *) ulm.ulm_pptr;

    /* Set up the copy control info */
    copy_ctl->qeu_copy = qeu_copy;
    copy_ctl->qef_rcb = qef_rcb;
    MEfill(sizeof(DMT_CB), 0, (PTR) &copy_ctl->dmtcb);
    copy_ctl->dmtcb.dmt_db_id = qef_rcb->qef_db_id;
    STRUCT_ASSIGN_MACRO(ulm, copy_ctl->ulm);
    copy_ctl->cut_rcb.cut_buf_handle = NULL;
    copy_ctl->child_status = E_DB_OK;
    copy_ctl->state = 0;
    qeu_copy->qeu_copy_ctl = copy_ctl;

    copy_ctl->use_load = FALSE;

    /* If a temp row is needed, allocate it now. */
    if (qeu_copy->qeu_att_info != NULL)
    {
	i4 len = qeu_copy->qeu_ext_length;

	if (len < qeu_copy->qeu_tup_length)
	    len = qeu_copy->qeu_tup_length;	/* Prevent overruns */
	copy_ctl->ulm.ulm_psize = len;
	status = qec_malloc(&copy_ctl->ulm);
	if (status != E_DB_OK)
	{
	    qef_error(copy_ctl->ulm.ulm_error.err_code, 0, status, &error_code,
		&qef_rcb->error, 0);
	    return (status);
	}
	copy_ctl->tmptuple = copy_ctl->ulm.ulm_pptr;
    }

    /* Pick up row estimate, and then clear qeu_count because it's
    ** used as the running total rows copied.
    */
    copy_ctl->row_est = row_est = qeu_copy->qeu_count;
    qeu_copy->qeu_count = 0;

    for (;;)
    {
	/*
	** Do a show on the table to determine if fastcopy should be used,
	** as well as to retrieve the table name and table owner.
	*/
	dmt_show.type = DMT_SH_CB;
	dmt_show.length = sizeof(DMT_SHW_CB);
	dmt_show.dmt_session_id = qef_rcb->qef_sess_id;
	dmt_show.dmt_db_id = qef_rcb->qef_db_id;
	dmt_show.dmt_char_array.data_in_size = 0;
	dmt_show.dmt_char_array.data_out_size  = 0;
	STRUCT_ASSIGN_MACRO(qeu_copy->qeu_tab_id, dmt_show.dmt_tab_id);
	dmt_show.dmt_flags_mask = DMT_M_TABLE;
	dmt_show.dmt_table.data_address = (PTR) &copy_ctl->tbl_info;
	dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	dmt_show.dmt_char_array.data_address = (PTR)NULL;
	dmt_show.dmt_char_array.data_in_size = 0;
	dmt_show.dmt_char_array.data_out_size  = 0;

	status = dmf_call(DMT_SHOW, &dmt_show);
	if (DB_FAILURE_MACRO(status))    
	{
	    if (dmt_show.error.err_code == E_DM0054_NONEXISTENT_TABLE ||
		dmt_show.error.err_code == E_DM0114_FILE_NOT_FOUND)
		qef_rcb->error.err_code = E_QE0023_INVALID_QUERY;
	    else
		qef_error(dmt_show.error.err_code, 0L, status, &error_code, 
		    &qef_rcb->error, 0);
	    break;
	}

	copy_ctl->dmtcb.dmt_char_array.data_in_size = 0;
	copy_ctl->dmtcb.dmt_char_array.data_address = NULL;
	if (qeu_copy->qeu_direction == CPY_INTO)
	{
	    copy_ctl->dmtcb.dmt_lock_mode = DMT_IS;
	    copy_ctl->dmtcb.dmt_access_mode = DMT_A_READ;
            copy_ctl->dmtcb.dmt_mustlock = FALSE;
	}
	else
	{
	    copy_ctl->dmtcb.dmt_lock_mode = DMT_IX;
	    copy_ctl->dmtcb.dmt_access_mode = DMT_A_WRITE;
            copy_ctl->dmtcb.dmt_mustlock = TRUE;

	    /* Attempt bulk-load if table is not journalled and not index and
	    ** not encrypted.  Later we might have to backtrack on this
	    ** idea, if DMF tells us that bulk load isn't going to work.
            */

            if ((copy_ctl->tbl_info.tbl_status_mask & (DMT_JNL | DMT_IDXD)) == 0
		&&
		(copy_ctl->tbl_info.tbl_encflags & DMT_ENCRYPTED) == 0 
		/* don't use bulk-load for small copies */
		&& (qeu_copy->qeu_stat & CPY_SMALL) == 0
		)
            {
                copy_ctl->use_load = TRUE;
		copy_ctl->dmtcb.dmt_lock_mode = DMT_X;
		/* bug 110678 force X lockmode for bulk-load */
		ch_entry.char_id = DMT_C_LOCKMODE;
		ch_entry.char_value = DMT_X;
		copy_ctl->dmtcb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
		copy_ctl->dmtcb.dmt_char_array.data_address = (PTR)&ch_entry;
            }
	}

	/* do we need a transaction */
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    qef_rcb->qef_modifier    = QEF_SSTRAN;
	    qef_rcb->qef_flag        = 0;
	    status = qet_begin(qef_cb);
	    if (DB_FAILURE_MACRO(status))
		break;
	}

	copy_ctl->dmtcb.type         = DMT_TABLE_CB;
	copy_ctl->dmtcb.length       = sizeof(DMT_CB);
	copy_ctl->dmtcb.dmt_db_id    = qef_rcb->qef_db_id;
	STRUCT_ASSIGN_MACRO(qeu_copy->qeu_tab_id, copy_ctl->dmtcb.dmt_id);
	copy_ctl->dmtcb.dmt_flags_mask = DMT_MULTI_ROW;
	if (qeu_copy->qeu_direction == CPY_FROM)
	    copy_ctl->dmtcb.dmt_flags_mask |= DMT_MANUAL_ENDOFROW;
	if ((copy_ctl->dmtcb.dmt_id.db_tab_base < 0) &&
	   (!STncmp("$Sess", copy_ctl->tbl_info.tbl_owner.db_own_name, 5)))
	{
	    copy_ctl->dmtcb.dmt_flags_mask |= DMT_SESSION_TEMP;
	}

	copy_ctl->dmtcb.dmt_update_mode = DMT_U_DIRECT;
	copy_ctl->dmtcb.dmt_sequence = qef_cb->qef_stmt;
	copy_ctl->dmtcb.dmt_tran_id = qef_cb->qef_dmt_id;

	/*
	** Open table "sync at close" if this is a fast load into a heap.
	*/
	if (copy_ctl->use_load &&
	    (copy_ctl->tbl_info.tbl_storage_type == DMT_HEAP_TYPE))
	{
	    copy_ctl->dmtcb.dmt_access_mode = DMT_A_SYNC_CLOSE;
	}

	status = dmf_call(DMT_OPEN, &copy_ctl->dmtcb);
	/* bail out if this is a locked encrypted table */
	if (status == E_DB_OK && copy_ctl->dmtcb.dmt_enc_locked)
	{
	    status = E_DB_ERROR;
	    copy_ctl->dmtcb.error.err_code = E_QE0190_ENCRYPT_LOCKED;
	}
	if (status == E_DB_OK)
	{
	    /* 
	    ** validate table timestamp against timestamp in the qeu_copy
	    ** block.
	    */
	    if (qeu_copy->qeu_timestamp.db_tab_high_time 
		    != copy_ctl->dmtcb.dmt_timestamp.db_tab_high_time
		|| qeu_copy->qeu_timestamp.db_tab_low_time
		    != copy_ctl->dmtcb.dmt_timestamp.db_tab_low_time)	    
	    {
		/* table out of date */

		/* Close the copy table */
		copy_ctl->dmtcb.dmt_flags_mask   = 0;
		status = dmf_call(DMT_CLOSE, &copy_ctl->dmtcb);
		if (DB_FAILURE_MACRO(status))
		{
		    qef_error(copy_ctl->dmtcb.error.err_code, 0L, status, 
				&error_code, &qef_rcb->error, 0);
		    break;
		}

		/* cause a retry */
		qef_rcb->error.err_code = E_QE0023_INVALID_QUERY;
		break;
	    }
	}
	else 
	{
	    if (copy_ctl->dmtcb.error.err_code == E_DM0054_NONEXISTENT_TABLE ||
		copy_ctl->dmtcb.error.err_code == E_DM0114_FILE_NOT_FOUND)
		qef_rcb->error.err_code = E_QE0023_INVALID_QUERY;
	    else
		qef_error(copy_ctl->dmtcb.error.err_code, 0L, status, &error_code, 
			&qef_rcb->error, 0);
	    break;
	}
	copy_ctl->dmtcb.dmt_char_array.data_in_size = 0;
	copy_ctl->dmtcb.dmt_char_array.data_address = NULL;

	if (copy_ctl->use_load
	  && (copy_ctl->tbl_info.tbl_status_mask & DMT_IS_PARTITIONED) == 0)
	{
	    /*
	    ** Try to start the load on this table.  If we can't load it
	    ** (because its not really empty), then reset the load flag to
	    ** do normal appends.
	    ** Partitioned table will start the load in the child, not here,
	    ** as the load or not-load decision is per-partition.
	    */

	    dmr_cb.type = DMR_RECORD_CB;
	    dmr_cb.length = sizeof(DMR_CB);
	    dmr_cb.dmr_access_id = copy_ctl->dmtcb.dmt_record_access_id;
	    dmr_cb.dmr_s_estimated_records = row_est;
	    dmr_cb.dmr_count = 0;
	    dmr_cb.dmr_mdata = (DM_MDATA *) 0;
	    dmr_cb.dmr_flags_mask = 0;

	    /*
	    ** If the user has specified information on how to
	    ** build the table pass this onto DMF
	    */
	    if ( qeu_copy->qeu_stat & CPY_DMR_CB )
	    {
		dmr_cb_ptr = (DMR_CB *)qeu_copy->qeu_dmrcb;
		if ( dmr_cb_ptr->dmr_char_array.data_in_size != 0)
		{
		    dmr_cb.dmr_flags_mask |= DMR_CHAR_ENTRIES;
		    dmr_cb.dmr_char_array = dmr_cb_ptr->dmr_char_array;
		}
	    }

	    dmr_cb.dmr_flags_mask |= DMR_TABLE_EXISTS;

	    status = dmf_call(DMR_LOAD, &dmr_cb);
	    if (status != E_DB_OK)
	    {
		i4 flags_mask;

		if (dmr_cb.error.err_code != E_DM0050_TABLE_NOT_LOADABLE)
		{
		    qef_error(dmr_cb.error.err_code, 0L, status, &error_code, 
			&qef_rcb->error, 0);
		    break;
		}
		copy_ctl->use_load = FALSE;

		/* 
		** Cannot use fast load.   Close and reopen the table in
		** normal sync-write mode with normal IX locking.
		*/
		flags_mask = copy_ctl->dmtcb.dmt_flags_mask;
		copy_ctl->dmtcb.dmt_flags_mask = 0;
		copy_ctl->dmtcb.dmt_access_mode = DMT_A_WRITE;

		status = dmf_call(DMT_CLOSE, &copy_ctl->dmtcb);
		if (DB_FAILURE_MACRO(status))
		{
		    qef_error(copy_ctl->dmtcb.error.err_code, 0L, status, 
			    &error_code, &qef_rcb->error, 0);
		    break;
		}

		copy_ctl->dmtcb.dmt_lock_mode = DMT_IX;
		copy_ctl->dmtcb.dmt_update_mode = DMT_U_DIRECT;
		copy_ctl->dmtcb.dmt_flags_mask = flags_mask;
		copy_ctl->dmtcb.dmt_char_array.data_in_size = 0;
		copy_ctl->dmtcb.dmt_char_array.data_address = NULL;

		status = dmf_call(DMT_OPEN, &copy_ctl->dmtcb);
		if (DB_FAILURE_MACRO(status))
		{
		    qef_error(copy_ctl->dmtcb.error.err_code, 0L, status, 
			&error_code, &qef_rcb->error, 0);
		    break;
		}

	    } /* if didn't work */
	} /* if bulk-load */

	/* Save the access ID cookie in case there are blobs */
	qeu_copy->qeu_access_id = copy_ctl->dmtcb.dmt_record_access_id;

	/*
	** Only bulk-load builds the table, if user has passed options
	** on how to build the table and not bulk-load issue an error
	*/
	dmr_cb_ptr = qeu_copy->qeu_dmrcb;
	if (!copy_ctl->use_load
	  && dmr_cb_ptr->dmr_char_array.data_in_size > 0 )
	{
          /*
          ** allow options on non-fast load copy for heap type table
          */
          if (copy_ctl->tbl_info.tbl_storage_type != DMT_HEAP_TYPE)
          {
              status = E_DB_ERROR;
              qef_error( E_US14E4_5348_NON_FAST_COPY, 0L, status,
                         &error_code, &qef_rcb->error, 0);
              break;
          }
	}

	/* If we have any sequence defaults, open up the sequences. */
	if (qeu_copy->qeu_dmscb != NULL)
	{
	    dmscb = qeu_copy->qeu_dmscb;
	    /* Fill in db-id, tran-id first.  The DMS_CB will be used
	    ** in the parent context (if using worker threads), so it's
	    ** OK to use the session's QEF_CB tran id.
	    */
	    dmscb->dms_db_id = qef_rcb->qef_db_id;
	    dmscb->dms_tran_id = qef_cb->qef_dmt_id;
	    status = dmf_call(DMS_OPEN_SEQ, dmscb);
	    if (status != E_DB_OK)
	    {
		qef_error(dmscb->error.err_code, 0L, status, 
			&error_code, &qef_rcb->error, 0);
		break;
	    }
	}

	/* Start up worker threads, CUT stuff, unless bulk-loading a
	** non-partitioned table.  Non-partitioned bulk-load stuffs rows
	** down inside DMF, and the extra CUT buffering would be a waste.
	** Partitioned bulk-loads benefit from the extra buffering.
	** Since we're only creating one extra thread, for now I will
	** insist that the CUT and thread machinery work -- no fallback
	** to pure serial copy.
	** It is not worth creating threads for a very small copy
	** so we call DMF directly for that case in qeu_r_copy
	*/
	if ((!copy_ctl->use_load && !(qeu_copy->qeu_stat & CPY_SMALL))
	  || (copy_ctl->tbl_info.tbl_status_mask & DMT_IS_PARTITIONED))
	{
	    if (qeu_copy->qeu_direction == CPY_FROM)
		STprintf(copy_ctl->cut_rcb.cut_buf_name, "COPY FROM %x",
			copy_ctl);
	    else
		STprintf(copy_ctl->cut_rcb.cut_buf_name, "COPY INTO %x",
			copy_ctl);
	    copy_ctl->cut_rcb.cut_cell_size = qeu_copy->qeu_tup_length;
	    copy_ctl->cut_rcb.cut_num_cells = COPY_CUT_BUFSIZE / qeu_copy->qeu_tup_length;
	    /* If row estimate is tiny, use small CUT buffer. */
	    if (row_est > 0
	      && row_est < copy_ctl->cut_rcb.cut_num_cells)
	    {
		copy_ctl->cut_rcb.cut_num_cells = row_est;
	    }

	    /* At least 10 if estimate is wrong or rows are large */
	    if (copy_ctl->cut_rcb.cut_num_cells < 10)
		copy_ctl->cut_rcb.cut_num_cells = 10;
	    copy_ctl->cut_rcb.cut_buf_use = CUT_BUF_READ | CUT_BUF_PARENT;
	    if (qeu_copy->qeu_direction == CPY_FROM)
		copy_ctl->cut_rcb.cut_buf_use = CUT_BUF_WRITE | CUT_BUF_PARENT;
	    cut_rcb_ptr = &copy_ctl->cut_rcb;
	    status = cut_create_buf(1, &cut_rcb_ptr, &cut_error);
	    if (DB_FAILURE_MACRO(status))
	    {
		qef_error(cut_error.err_code, 0, status,
			&error_code, &qef_rcb->error, 0);
		break;
	    }

	    /* Create a factotum thread to run the gets (COPY INTO) or
	    ** puts (COPY FROM).
	    ** Request DMF resources so that we can do shared txn things.
	    */
	    scfcb.scf_type = SCF_CB_TYPE;
	    scfcb.scf_length = sizeof(SCF_CB);
	    scfcb.scf_session = DB_NOSESSION;
	    scfcb.scf_facility = DB_QEF_ID;
	    scfcb.scf_ptr_union.scf_ftc = &ftc;

	    /* Private ADF, DMF, children use parent QEF CB's */
	    ftc.ftc_facilities = (1<<DB_ADF_ID) | (1<<DB_DMF_ID) | (1<<DB_SXF_ID) | (1<<DB_GWF_ID);
	    ftc.ftc_data = (PTR) copy_ctl;
	    ftc.ftc_data_length = 0;
	    ftc.ftc_priority = SCF_CURPRIORITY;
	    ftc.ftc_thread_exit = NULL;
	    ftc.ftc_thread_name = copy_ctl->cut_rcb.cut_buf_name;
	    if (qeu_copy->qeu_direction == CPY_FROM)
	    {
		ftc.ftc_thread_entry = copy_from_child;
	    }
	    else
	    {
		ftc.ftc_thread_entry = copy_into_child;
	    }
	    status = scf_call(SCS_ATFACTOT, &scfcb);
	    if (status != E_DB_OK)
	    {
		qef_error(scfcb.scf_error.err_code, 0, status,
			&error_code, &qef_rcb->error, 0);
		break;
	    }
	    child_started = TRUE;

	    /* Wait until the child thread initializes and connects.
	    ** A rendezvous is needed for COPY INTO, to make sure that
	    ** the child (a writer) is up and OK before we try reading.
	    ** (With a simple attach wait, the child could exit before we
	    ** get this far!)
	    */
	    nattach = 2;		/* Parent + child */
	    status = cut_event_wait(&copy_ctl->cut_rcb, CUT_SYNC,
			(PTR) &nattach, &cut_error);
	    if (DB_FAILURE_MACRO(status))
	    {
		qef_error(E_QE0002_INTERNAL_ERROR, 0, status,
			&error_code, &qef_rcb->error, 0);
		break;
	    }
	    /* Clear the bulk-load flag now that the child has synced up.
	    ** If bulk-load is on, it's a partitioned copy, and it's now
	    ** the business of the child to deal with it.  Don't attempt
	    ** dmr-load's in qeu_r_copy.
	    */
	    copy_ctl->use_load = FALSE;
	}

	/* Must audit that copy occurred. */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR e_error;
	    i4 access;

	    if (qeu_copy->qeu_direction == CPY_INTO)
		access = SXF_A_SUCCESS | SXF_A_COPYINTO;	
	    else
		access = SXF_A_SUCCESS | SXF_A_COPYFROM;

	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)&copy_ctl->tbl_info.tbl_name,
			    &copy_ctl->tbl_info.tbl_owner,
			    sizeof(copy_ctl->tbl_info.tbl_name), SXF_E_TABLE,
			    I_SX2026_TABLE_ACCESS, access, 
			    &e_error);

	    if (DB_FAILURE_MACRO(status))    
	    {
		error = e_error.err_code;
		qef_error(error, 0L, status, &error_code, 
			    &qef_rcb->error, 0);
		break;
	    }
	}

	return (E_DB_OK);
    }

    /*
    ** An error occurred in COPY BEGIN
    ** If a child started, kill it off.
    */
    if (child_started)
    {
	copy_ctl->state |= CCF_ABORT;
	(void) cut_signal_status(&copy_ctl->cut_rcb, E_DB_ERROR, &cut_error);
	/* Wait until we're the only one out */
	do
	{
	    /* Turn off any attn for ourselves that might be showing. */
	    (void) cut_signal_status(&copy_ctl->cut_rcb, E_DB_OK, &cut_error);
	    nattach = 1;
	    status = cut_event_wait(&copy_ctl->cut_rcb, CUT_DETACH,
			(PTR) &nattach, &cut_error);
	} while (status == E_DB_ERROR && cut_error.err_code == E_CU0204_ASYNC_STATUS);
    }

    /*
    ** If a transaction has been begun for this copy then back it out.
    */
    if (qef_cb->qef_stat != QEF_NOTRAN)
    {
	if(qef_rcb->error.err_code == E_QE0023_INVALID_QUERY   /* if retry */
	   && qef_cb->qef_open_count > 0)		/* and open cursors */
	      return(E_DB_ERROR);         /* return without aborting/closing */

	if (qef_cb->qef_stat == QEF_MSTRAN)
	{
	    qef_rcb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;
	}
	else
	{
	    qef_rcb->qef_spoint = (DB_SP_NAME *)NULL;
	}

	status = qet_abort(qef_cb);

	if ((DB_FAILURE_MACRO(status)) &&
	    (qef_rcb->error.err_code != E_QE0025_USER_ERROR))
	{
	    return (E_DB_FATAL);
	}

	/* clean up any cursor that might be opened */
	status = qee_cleanup(qef_rcb, (QEE_DSH **)NULL);
    }
    /* If we got as far as CUT, detach ourselves */
    if (copy_ctl->cut_rcb.cut_buf_handle != NULL)
    {
	(void) cut_signal_status(NULL, E_DB_OK, &cut_error);
	(void) cut_thread_term(TRUE, &cut_error);
    }

    /* Close the memory stream we opened */
    ulm_closestream(&ulm);

    return (E_DB_ERROR);
} /* qeu_b_copy */

/*
**  Name: QEU_E_COPY     - end the copy operation
**  
**  External QEF call:   status = qef_call(QEU_E_COPY, &qef_rcb);
**  
**  Description:
**      A copy command is ended.  The table is closed and the transaction
**      is ended.  If qeu_copy->qeu_stat specifies CPY_FAIL or an internal
**      error is encountered during qeu_e_copy, then the transaction is
**      aborted.
**
**	If we have a copy child, the action taken depends on which direction
**	we're going.
**	If this is COPY INTO, we got here because the child read all
**	the rows.  The child has sent through a completion status and
**	has exited (or is in the process of exiting).  All we need to
**	do is clean things up.
**
**	If this is COPY FROM, the user end is driving things.  The
**	child is doing CUT reads and DMR puts.  We need to tell the
**	child that there's nothing more coming, and make sure it's
**	awake to see that fact.  Then we wait for the child to
**	detach (meaning that it's processed the remaining rows) and
**	exit.
**
**	In either case, if we got here because of FAIL, we'll stuff
**	an asynchronous abort through CUT to make sure that the child
**	gives up and dies off expeditiously.
**
**	This routine should not return error if FAIL is set, unless
**	the error is significantly more interesting than the copy
**	failure.
**  
**  Inputs:
**       qef_rcb
**           .qef_sess_id	         session id
**           .qef_db_id	         database id
**           .qeu_copy               copy control block
**       	     .qeu_stat	         CPY_FAIL if we should abort copy
**		     .qeu_copy_ctl   Local copy controller info
**               
**  Outputs:
**       qef_rcb
**           .qeu_copy
**                .qeu_count         set to zero if copy is aborted
**           .error.err_code         One of the following
**                                   E_QE0000_OK
**                                   E_QE0002_INTERNAL_ERROR
**                                   E_QE0004_NO_TRANSACTION
**                                   E_QE0017_BAD_CB
**                                   E_QE0018_BAD_PARAM_IN_CB
**                                   E_QE0019_NON_INTERNAL_FAIL
**       Returns:
**           E_DB_OK
**           E_DB_ERROR                      caller error
**           E_DB_FATAL                      internal error
**       Exceptions:
**           none
**  
**  Side Effects:
**           none
**  
**  History:
**	15-dec-86 (rogerk)
**	     written
**	01-jun-88 (puree)
**	    change qeu_close to qet_commit/dmt_close-qet_savepoint to balance
**	    QEF house keeping initiated by qeu_b_copy.
**	26-Apr-2004 (schka24)
**	    Add child cleanup for partitioned COPY.
**	12-Apr-2010 (kschendel) SIR 123485
**	    Change adu-free-objects call to new "query ended" call to DMF.
**	    Use padded copy of standard savepoint name in the QEF SCB.
*/

DB_STATUS
qeu_e_copy(
QEF_RCB     *qef_rcb)
{
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEU_COPY		*qeu_copy = qef_rcb->qeu_copy;
    QEU_COPY_CTL	*copy_ctl;
    DMR_CB		dmr_cb;
    DB_STATUS		status = E_DB_OK;
    DB_ERROR		cut_error;
    i4			err;
    i4			nattached;
    ULM_RCB		ulm;

    copy_ctl = qeu_copy->qeu_copy_ctl;
    dmr_cb.type = DMR_RECORD_CB;
    dmr_cb.length = sizeof(DMR_CB);

    /*
    ** If we are doing fast copy and we are not aborting the copy, then
    ** tell the loader to sort and load the rows given to it.
    */
    if ((copy_ctl->use_load) && ((qeu_copy->qeu_stat & CPY_FAIL) == 0))
    {
	dmr_cb.dmr_access_id = copy_ctl->dmtcb.dmt_record_access_id;
	dmr_cb.dmr_count = 0;
	dmr_cb.dmr_mdata = (DM_MDATA *) 0;
	dmr_cb.dmr_flags_mask = DMR_TABLE_EXISTS | DMR_ENDLOAD;

	status = dmf_call(DMR_LOAD, &dmr_cb);
	if (status != E_DB_OK)
	{
	    qef_error(dmr_cb.error.err_code, 0L, status, &err, 
			&qef_rcb->error, 0);
	    qeu_copy->qeu_stat |= CPY_FAIL;
	}

	/*
	** Get actual number of rows added to table.  Sorter may have
	** removed duplicates.
	*/
	qeu_copy->qeu_count = dmr_cb.dmr_count;
    }

    /* If we are doing CUT stuff, shut down the child as described in the
    ** intro.
    */
    if (copy_ctl->cut_rcb.cut_buf_handle != NULL)
    {
	if (qeu_copy->qeu_stat & CPY_FAIL)
	{
	    /* This is an abort.  Signal everyone to blow chunks, unless
	    ** the child knows already.
	    */
	    if ((copy_ctl->state & CCF_ABORT) == 0)
	    {
		copy_ctl->state |= CCF_ABORT;
		/* Not much we can do with any error from this */
		status = cut_signal_status(&copy_ctl->cut_rcb, E_DB_ERROR,
				&cut_error);
	    }
	}
	else if (qeu_copy->qeu_direction == CPY_FROM)
	{
	    /* Child is doing read/put, tell it nothing more is coming. */
	    status = cut_writer_done(&copy_ctl->cut_rcb, &cut_error);
	    if (status != E_DB_OK)
	    {
		if (cut_error.err_code != E_CU0204_ASYNC_STATUS)
		    qef_error(cut_error.err_code, 0L, status, &err, 
			&qef_rcb->error, 0);
		qeu_copy->qeu_stat |= CPY_FAIL;
	    }
	}
	/* elseif COPY INTO, child saw no-more-rows and exited/is exiting */

	/* Wait for child to exit.  If this was COPY FROM, the child may
	** signal errors on the last few rows, just loop until it's
	** really gone and then we'll check the child status.
	*/
	do
	{
	    /* Turn off any attn for ourselves that might be showing. */
	    (void) cut_signal_status(&copy_ctl->cut_rcb, E_DB_OK, &cut_error);
	    nattached = 1;			/* Just us */
	    status = cut_event_wait(&copy_ctl->cut_rcb, CUT_DETACH,
			(PTR) &nattached, &cut_error);
	} while (status == E_DB_ERROR && cut_error.err_code == E_CU0204_ASYNC_STATUS);
	if (status != E_DB_OK)
	    TRdisplay("%@ [copy-end] Error %x from CUT detach\n",cut_error.err_code);

	/* Clean up CUT-ish things, detach */
	(void) cut_signal_status(NULL, E_DB_OK, &cut_error);
	(void) cut_thread_term(TRUE, &cut_error);

	/* Report any error we might have received from the child,
	** unless we know we're failing already.  (Extra errors seem
	** to confuse the sequencer.)
	*/
	if (copy_ctl->child_status != E_DB_OK
	  && (qeu_copy->qeu_stat & CPY_FAIL) == 0)
	{
	    if (copy_ctl->child_error.err_code != E_CU0204_ASYNC_STATUS)
		qef_error(copy_ctl->child_error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
	    qeu_copy->qeu_stat |= CPY_FAIL;
	}
    } /* if CUT */

    /* Note: no need to "close" sequences after doing only DML on them. */

    /* In case we were having fun with blobs, make sure they're cleaned up.
    ** The dmr-cb is only used to return error info.
    */
    dmr_cb.dmr_flags_mask = DMR_QEND_FREE_TEMPS;
    if (qeu_copy->qeu_stat & CPY_FAIL)
	dmr_cb.dmr_flags_mask |= DMR_QEND_ERROR;
    status = dmf_call(DMPE_QUERY_END, &dmr_cb);
    if (status != E_DB_OK && (qeu_copy->qeu_stat & CPY_FAIL) == 0)
    {
        qef_error(dmr_cb.error.err_code, 0L, status, &err,
		&qef_rcb->error, 0);
	qeu_copy->qeu_stat |= CPY_FAIL;
    }

    /* Close the copy table.  This is the master if partitioned. */
    copy_ctl->dmtcb.dmt_flags_mask   = 0;
    status = dmf_call(DMT_CLOSE, &copy_ctl->dmtcb);
    if (DB_FAILURE_MACRO(status)
      && (qeu_copy->qeu_stat & CPY_FAIL) == 0)
    {
        qef_error(copy_ctl->dmtcb.error.err_code, 0L, status, &err,
		&qef_rcb->error, 0);
	qeu_copy->qeu_stat |= CPY_FAIL;
    }
    qeu_copy->qeu_access_id = NULL;	/* Unnecessary, but be tidy */

    /*
    ** If copy has been successful, try to commit this transaction (or 
    ** create a new savepoint if we are in a MST).
    **
    ** If we can't commit, then mark copy to be aborted.
    */
    if ((qeu_copy->qeu_stat & CPY_FAIL) == 0)
    {
	if ((qef_cb->qef_stat == QEF_MSTRAN) && (qef_cb->qef_open_count == 0))
        {
	    qef_rcb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;
	    status = qet_savepoint(qef_cb);
        }
	else if ((qef_cb->qef_stat != QEF_NOTRAN) &&
	    (qef_cb->qef_open_count == 0))
	{
	    status = qet_commit(qef_cb);
	    /* clean up any cursor that might be opened */
	    status = qee_cleanup(qef_rcb, (QEE_DSH **)NULL);
	}
	if (DB_FAILURE_MACRO(status))
	    qeu_copy->qeu_stat |= CPY_FAIL;
    }


    /*
    ** If the copy has failed then abort to the transaction (or at least
    ** to the last savepoint.
    */
    if (qeu_copy->qeu_stat & CPY_FAIL)
    {
	qef_rcb->qeu_copy->qeu_count = 0;
	if (qef_cb->qef_stat == QEF_MSTRAN)
	{
	    qef_rcb->qef_spoint = &Qef_s_cb->qef_sp_savepoint;
	}
	else if (qef_cb->qef_stat != QEF_NOTRAN)
	{
	    qef_rcb->qef_spoint = (DB_SP_NAME *)NULL;
	}
	status = qet_abort(qef_cb);
	if ((DB_FAILURE_MACRO(status)) && 
	    (qef_rcb->error.err_code != E_QE0025_USER_ERROR))
	    return (E_DB_FATAL);

	/* clean up any cursor that might be opened */
	status = qee_cleanup(qef_rcb, (QEE_DSH **)NULL);
    }

    /* Close the COPY memory stream.  The ULM_RCB is in the memory being
    ** freed, which turns out to be a bad idea, so copy it...
    */
    STRUCT_ASSIGN_MACRO(copy_ctl->ulm, ulm);
    qeu_copy->qeu_copy_ctl = NULL;
    ulm_closestream(&ulm);

    return (E_DB_OK);
} /* qeu_e_copy */

/*
**  Name: QEU_R_COPY     - read tuples from file: append to relation
**  
**  External QEF call:   status = qef_call(QEU_R_COPY, &qef_rcb);
**  
**  Description:
**	Given one or more rows, send them to the target relation.
**	qeu_copy.qeu_cur_tups is the number of rows sent to us by SCF,
**	and .qeu_input is a pointer to the rows (formatted into a
**	QEF_DATA list).
**
**	If there is any preprocessing to do on the rows (datatype
**	conversion, missing value insertion), that is done first.
**
**	Then, the rows are either handed off to DMF (for bulk copy)
**	or stuffed through a CUT buffer for copy_from_child to insert
**	row-wise.
**  
**  Inputs:
**       qef_rcb
**           .qef_sess_id		 session id
**           .qef_db_id		 database id
**           .qeu_copy               copy control block
**       	     .qeu_cur_tups	 number of rows to insert
**               .qeu_input          list of QEF_DATA structs pointing to rows
**               
**  Outputs:
**       qef_rcb
**           .error.err_code         One of the following
**                                   E_QE0000_OK
**                                   E_QE0002_INTERNAL_ERROR
**                                   E_QE0004_NO_TRANSACTION
**                                   E_QE0017_BAD_CB
**                                   E_QE0018_BAD_PARAM_IN_CB
**       Returns:
**           E_DB_OK
**           E_DB_ERROR                      caller error
**           E_DB_FATAL                      internal error
**       Exceptions:
**           none
**  
**  Side Effects:
**           none
**  
**  History:
**	15-dec-86 (rogerk)
**	    written
**	17-apr-86 (rogerk)
**	    Don't error if all rows aren't appended, some may have been dups.
**	29-aug-88 (puree)
**	    clean up duplicate code.
**	14-jul-93 (robf)
**	    Do extra attribute processing if necessary prior to loading
**	10-Jan-2001 (jenjo02)
**	    We know this session's id; pass it to SCF.
**	    Deleted SCU_INFORMATION to get ADF_CB*; it's known
**	    and never null.
**      21-oct-2002 (horda03) Bug 77895
**          For all attributes of the table not specified in the COPY FROM
**          command, add the default value.
**	26-Apr-2004 (schka24)
**	    Stuff rows through CUT unless bulk-loading.
**	08-Sep-2004 (jenjo02)
**	    cut_write_buf prototype change for DIRECT mode.
**	23-Mar-2010 (kschendel) SIR 123448
**	    Put child-abort test in the right place, otherwise with bad
**	    luck child can abort and we hang in the cut write.
**	    Fix missing-value loop, went one too far.
**	    Cancel check got lost?  put one back in.
**	26-jun-2010 (stephenb)
**	    Add code to deal with varying sized and formatted input rows
**	    in the current copy buffer. This can occur when the client
**	    sends and describes each row individually, but we want to
**	    use "copy" (because it's faster). This is only currently
**	    the case for the batch insert to copy optimization.
*/  

DB_STATUS
qeu_r_copy(
QEF_RCB       *qef_rcb)
{
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEU_COPY		*qeu_copy = qef_rcb->qeu_copy;
    DB_ERROR		cut_error;
    DB_STATUS		status = E_DB_OK;
    GCA_TD_DATA		*sqlda=(GCA_TD_DATA*) qeu_copy->qeu_tdesc;
    i4			count;
    i4			err;
    i4			num_cells;
    QEF_DATA    	*dataptr;
    QEF_INS_DATA	*insptr;
    char		*tuple;
    char		*tmptuple;
    DB_DATA_VALUE   	fromdv;
    DB_DATA_VALUE   	intodv;
    i4			i;
    QEU_CPATTINFO	*cpatt;
    QEU_COPY_CTL	*copy_ctl;

    /* Check for async cancel from front-end occasionally. */
    CScancelCheck(qef_cb->qef_ses_id);

    copy_ctl = qeu_copy->qeu_copy_ctl;

    if(qeu_copy->qeu_att_info)
    {
	/*
	** We have some attributes which need special processing, so
	** loop over them in turn. We remember there may be multiple
	** tuples  to  process, so do this for each buffer.
	*/
	if (qeu_copy->qeu_stat & CPY_INS_OPTIM &&
		copy_ctl->ulm.ulm_psize < qeu_copy->qeu_ext_length)
	{
	    /* 
	    ** copy came from insert to copy optimization, in this case
	    ** the row size will vary with each insert, so there is an outside chance
	    ** that one of the rows is bigger than the temp buffer (which is currently
	    ** the bigger of the first row and the internal tuple length). In this case
	    ** we need to re-alloc the temp buffer before we use it. qeu_ext_length
	    ** will contain the size of the largest external row we saw in this group
	    */
	    (void)qec_mfree(&copy_ctl->ulm);
	    copy_ctl->ulm.ulm_psize = qeu_copy->qeu_ext_length;
	    status = qec_malloc(&copy_ctl->ulm);
	    if (status != E_DB_OK)
	    {
		qef_error(copy_ctl->ulm.ulm_error.err_code, 0, status, &err,
		    &qef_rcb->error, 0);
		return (status);
	    }
	    copy_ctl->tmptuple = copy_ctl->ulm.ulm_pptr;		
	}
	count = qeu_copy->qeu_cur_tups;
        dataptr = qeu_copy->qeu_input;    
        insptr = qeu_copy->qeu_insert_data;
	while (--count >= 0)
	{
	    bool first=TRUE;

	    tuple=dataptr->dt_data;
	    tmptuple=copy_ctl->tmptuple;
	
	    for(cpatt=qeu_copy->qeu_att_info; cpatt; cpatt=cpatt->cp_next)
	    {
	        if (cpatt->cp_flags & QEU_CPATT_EXP_INP)
	        {
		    /*
		    ** Special import/export processing, generally the
		    ** type coming from the frontend differs from the real type
		    ** and we have to do the conversion in the DBMS
		    */
		    intodv.db_datatype=cpatt->cp_type;
		    intodv.db_length=cpatt->cp_length;
		    intodv.db_prec=cpatt->cp_prec;
		    intodv.db_data = (PTR)(tmptuple+cpatt->cp_tup_offset);
		    intodv.db_collID = -1;

		    if (qeu_copy->qeu_stat & CPY_INS_OPTIM)
		    {
			/* 
			** for insert optimization, each row may contain
			** different sized and typed attributes which we store in the
			** insert pointer
			*/
			fromdv.db_datatype=
				insptr->ins_col[cpatt->cp_attseq].dtcol_value.db_datatype;
			fromdv.db_length=
				insptr->ins_col[cpatt->cp_attseq].dtcol_value.db_length;
			fromdv.db_prec=
				insptr->ins_col[cpatt->cp_attseq].dtcol_value.db_prec;
			fromdv.db_data = 
				(PTR)(tuple+insptr->ins_col[cpatt->cp_attseq].dtcol_offset);		
		    }
		    else
		    {
			fromdv.db_datatype=
			    sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_datatype;
			fromdv.db_length=
			    sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_length;
			fromdv.db_prec=
			    sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_prec;
			fromdv.db_data = (PTR)(tuple+cpatt->cp_ext_offset);
		    }
		    fromdv.db_collID = -1;
		
		    if ((status = adc_cvinto(qef_cb->qef_adf_cb,
				    &fromdv, &intodv)) != E_DB_OK)
		    {
			qef_rcb->error.err_code = E_QE000B_ADF_INTERNAL_FAILURE;
			return status;
		    }
		    /*
		    ** Now we need to move up the rest of the tuple to
		    ** fill up the  gap
		    */
		    if(first)
		    {
			/*
			** First attribute processed, so copy from start 
			** up to current position
			*/
			if(cpatt->cp_tup_offset>0)
				MEcopy ((PTR)tuple, cpatt->cp_tup_offset, 
					(PTR)tmptuple);
			first=FALSE;
		    }
		    /*
		    ** Copy from current attribute end to end of tuple
		    ** to make sure everything is in sync
		    */
		    if (qeu_copy->qeu_stat & CPY_INS_OPTIM)
		    {
			/* 
			** copy came from insert optimization. In this case the
			** external row size an layout may be different for each row so
			** we store it in the insert pointer
			*/
			MEcopy(
			  (PTR)(tuple+
				insptr->ins_col[cpatt->cp_attseq].dtcol_offset+
				insptr->ins_col[cpatt->cp_attseq].dtcol_value.db_length
				),
			  insptr->ins_ext_size-
			      insptr->ins_col[cpatt->cp_attseq].dtcol_offset-
			      insptr->ins_col[cpatt->cp_attseq].dtcol_value.db_length,
			 (PTR)(tmptuple+cpatt->cp_tup_offset+cpatt->cp_length));
		    }
		    else
		    {
			MEcopy(
			  (PTR)(tuple+
				cpatt->cp_ext_offset+
				sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_length
				),
			  qeu_copy->qeu_ext_length-
				cpatt->cp_ext_offset-
				sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_length,
			 (PTR)(tmptuple+cpatt->cp_tup_offset+cpatt->cp_length));
		    }
		}
	    }
	    /*
	    ** Now copy the temp tuple back into the buffer
	    */
	    MEcopy((PTR)tmptuple, qeu_copy->qeu_tup_length, (PTR)tuple);
            dataptr = dataptr->dt_next;
            if (qeu_copy->qeu_stat & CPY_INS_OPTIM)
        	insptr = insptr->ins_next;
	}
    }

    if (qeu_copy->qeu_missing)
    {
        /* User hasn't specified all the columns for the COPY FROM
        ** so need to insert the correct default values.
        */
	DMS_CB *dmscb = qeu_copy->qeu_dmscb;
        QEU_MISSING_COL *missing;

    	count = qeu_copy->qeu_cur_tups;
        dataptr = qeu_copy->qeu_input;    
	while (--count >= 0)
        {
	    if (dmscb != NULL)
	    {
		/* We have sequence defaults.  Ask DMF for new values for the
		** sequence(s).  Later, as we run thru the missing-value
		** structs, the sequence values will be coerced into the
		** column default value data slot.  (Missing-value columns
		** of exactly matching type simply point at the sequence
		** value, needing no coercion.)
		** Note that we arrange to generate a new set of sequence
		** values ONCE for each row.
		*/
		status = dmf_call(DMS_NEXT_SEQ, dmscb);
		if (status != E_DB_OK)
		{
		    qef_error(dmscb->error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
		    return (E_DB_ERROR);
		}
	    }

	    for( missing = qeu_copy->qeu_missing; missing; missing = missing->mc_next)
	    {
		if (missing->mc_seqdv.db_data != NULL)
		{
		    /* Coerce (numeric) sequence default */
		    status = adc_cvinto(qef_cb->qef_adf_cb,
				&missing->mc_seqdv, &missing->mc_dv);
		    if (status != E_DB_OK)
		    {
			qef_rcb->error.err_code = E_QE000B_ADF_INTERNAL_FAILURE;
			return status;
		    }
		}
		/* Use db_length in QEU_MISSING_COL
		** If this column has dt_bits & AD_PERIPHERAL
		** length in gca_attdbv is incorrect (-1)
		*/
		MEcopy( missing->mc_dv.db_data,
                      missing->mc_dv.db_length,
                      (PTR) dataptr->dt_data + missing->mc_tup_offset);
	    }
	    dataptr = dataptr->dt_next;
	}
    }

    if (copy_ctl->use_load)
    {
	DMR_CB dmrcb;

	dmrcb.type = DMR_RECORD_CB;
	dmrcb.length = sizeof(DMR_CB);
	dmrcb.dmr_access_id = copy_ctl->dmtcb.dmt_record_access_id;
	dmrcb.dmr_count = qeu_copy->qeu_cur_tups;
	/* QEF_DATA and DM_MDATA are the same ...!  FIXME get rid of one. */
	dmrcb.dmr_mdata = (DM_MDATA *) qeu_copy->qeu_input;
	dmrcb.dmr_flags_mask = DMR_TABLE_EXISTS;
	dmrcb.dmr_tid = 0;
	dmrcb.dmr_char_array.data_in_size = 0;
	dmrcb.dmr_s_estimated_records = 0;
	status = dmf_call(DMR_LOAD, &dmrcb);
	if (status != E_DB_OK)
	{
	    if (dmrcb.error.err_code == E_DM0046_DUPLICATE_RECORD
	      || dmrcb.error.err_code == E_DM0045_DUPLICATE_KEY)
		/* Ignore dups */
		status = E_DB_OK;
	    else
		qef_error(dmrcb.error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
	    return (status);
	}
	/* Notice that we don't count bulk-load rows incrementally.
	** DMF tells us the good news at END time.
	*/
    }
    else if 
	((qeu_copy->qeu_stat & CPY_SMALL) &&
	    (copy_ctl->tbl_info.tbl_status_mask & DMT_IS_PARTITIONED) == 0)
    {
	DMR_CB	dmrcb;
	/* 
	** Unpartitioned small copy doesn't use worker threads, 
	** just call DMF to add each row
	*/
	dmrcb.type = DMR_RECORD_CB;
	dmrcb.length = sizeof(DMR_CB);
	dmrcb.dmr_flags_mask = 0;
	dmrcb.dmr_access_id = copy_ctl->dmtcb.dmt_record_access_id;
	dmrcb.dmr_data.data_in_size = qeu_copy->qeu_tup_length;
	
    	count = qeu_copy->qeu_cur_tups;
        dataptr = qeu_copy->qeu_input;    
	while (--count >= 0)
	{
	    /* Do the PUT.  Ignore duplicate-key type errors. */
	    dmrcb.dmr_data.data_address = dataptr->dt_data;
	    dmrcb.dmr_tid = 0;
	    status = dmf_call(DMR_PUT, &dmrcb);
	    if (status == E_DB_OK)
	    {
		/* Count actual rows here */
		++ qeu_copy->qeu_count;
	    }
	    else
	    {
		if (dmrcb.error.err_code == E_DM0046_DUPLICATE_RECORD ||
		  dmrcb.error.err_code == E_DM0045_DUPLICATE_KEY ||
		  dmrcb.error.err_code == E_DM0048_SIDUPLICATE_KEY ||
		  dmrcb.error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
		  dmrcb.error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT)
		    status = E_DB_OK;
		else
		{
		    copy_ctl->child_error = dmrcb.error;
		    break;
		}
	    }
	    dataptr = dataptr->dt_next;
	}
    }
    else
    {
	/* Not loading, stuff rows at the child */

    	count = qeu_copy->qeu_cur_tups;
        dataptr = qeu_copy->qeu_input;    
	while (--count >= 0)
	{
	    if (copy_ctl->state & CCF_ABORT)
	    {
		/* Child puked, pass it on */
		qef_error(copy_ctl->child_error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
		return (E_DB_ERROR);	/* Child puked, pass it on */
	    }
	    num_cells = 1;
	    status = cut_write_buf(&num_cells, &copy_ctl->cut_rcb,
			(PTR*)&dataptr->dt_data, CUT_RW_WAIT, &cut_error);
	    if (status != E_DB_OK)
	    {
		/* Avoid confusing QEF messaging if interrupt */
		if (cut_error.err_code != E_CU0204_ASYNC_STATUS)
		    qef_error(cut_error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
		return (status);
	    }
	    dataptr = dataptr->dt_next;
	}
    }

    return (E_DB_OK);
}

/*  
**  Name: QEU_W_COPY     - read tuples from relation: append to file
**  
**  External QEF call:   status = qef_call(QEU_R_COPY, &qef_rcb);
**  
**  Description:
**	Fetch a bunch of rows for COPY INTO.
**
**	The rows are retrieved in the worker child thread, and stuffed into
**	a CUT buffer.  All we have to do is read the rows out of the buffer,
**	do any necessary post-processing (e.g. type conversion), and
**	hand the rows back to SCF.
**	When EOF on the table is reached, the CUT read will return with a
**	warning indicating that there aren't any more rows.  The number
**	of rows actually read is passed back in qeu_copy->qeu_cur_tups.
**  
**  Inputs:
**       qef_rcb
**           .qef_sess_id		 session id
**           .qef_db_id		 database id
**           .qeu_copy               copy control block
**       	     .qeu_cur_tups       number of rows to get
**               .qeu_output         points to QEF_DATA list of output buffers
**               
**  Outputs:
**       qef_rcb
**           .qeu_copy
**       	    .qeu_cur_tups        number of rows retrieved
**           .error.err_code         One of the following
**                                   E_QE0000_OK
**                                   E_QE0002_INTERNAL_ERROR
**                                   E_QE0004_NO_TRANSACTION
**                                   E_QE0017_BAD_CB
**                                   E_QE0018_BAD_PARAM_IN_CB
**                                   E_QE0015_NO_MORE_ROWS
**       Returns:
**           E_DB_OK
**           E_DB_ERROR                      caller error
**           E_DB_FATAL                      internal error
**       Exceptions:
**           none
**  
**  Side Effects:
**           none
**  
**  History:
**	15-dec-86 (rogerk)
**	     written
**	14-jul-93 (robf)
**	    Add attribute post processing
**	10-Jan-2001 (jenjo02)
**	    We know this session's id; pass it to SCF.
**	    Deleted SCU_INFORMATION to get ADF_CB*; it's known
**	    and never null.
**	26-Apr-2004 (schka24)
**	    Read rows out of CUT.
**	18-Jul-2004 (jenjo02)
**	    cut_read_buffer output buffer now a pointer to pointer
**	    to support LOCATE mode.
**	10-Dec-2004 (schka24)
**	    Since we're slinging rows at the client, check for interrupts
**	    every now and then, in case something went wrong.
**      25-Feb-2010 (hanal04) Bug 123291, 122938
**          Add an exception handler when dealing with attributes that need
**          special processing. These include i8->i4 and dec39->dec39
**          downgrades that may hit overflow exceptions. If we do hit
**          and ADF exception abort the COPY we can't materialise the
**          value on the old client.
*/  

DB_STATUS
qeu_w_copy(
QEF_RCB       *qef_rcb)
{
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QEU_COPY		*qeu_copy = qef_rcb->qeu_copy;
    QEU_COPY_CTL	*copy_ctl;
    DB_ERROR		cut_error;
    DB_STATUS		status = E_DB_OK;
    GCA_TD_DATA		*sqlda=(GCA_TD_DATA*) qeu_copy->qeu_tdesc;
    i4			count;
    i4			err;
    i4			num_cells;
    QEF_DATA    	*dataptr;
    char		*tuple;
    char		*tmptuple;
    DB_DATA_VALUE   	fromdv;
    DB_DATA_VALUE   	intodv;
    i4			i;
    QEU_CPATTINFO	*cpatt;

    copy_ctl = qeu_copy->qeu_copy_ctl;

    /* Check for child abort */
    if (copy_ctl->state & CCF_ABORT)
    {
	qeu_copy->qeu_cur_tups = 0;	/* Don't return any rows */
	return (E_DB_ERROR);
    }

    /* Check for async cancel from front-end occasionally. */
    CScancelCheck(qef_cb->qef_ses_id);

    count = qeu_copy->qeu_cur_tups;
    dataptr = qeu_copy->qeu_output;
    while (--count >= 0)
    {
	num_cells = 1;
	status = cut_read_buf(&num_cells, &copy_ctl->cut_rcb,
			(PTR*)&dataptr->dt_data, CUT_RW_WAIT, &cut_error);
	/* If the child reached EOF, a E_DB_WARN status can occur */
	if (status == E_DB_WARN && cut_error.err_code == W_CU0305_NO_CELLS_READ)
	    break;
	if (status != E_DB_OK)
	{
	    if (status == E_DB_WARN)
		status = E_DB_ERROR;
	    if (cut_error.err_code != E_CU0204_ASYNC_STATUS)
		qef_error(cut_error.err_code, 0L, status, &err,
			&qef_rcb->error, 0);
	    return (status);
	}
	dataptr = dataptr->dt_next;
    }
    /* Adjust qeu-cur-tups to be actual rows read */
    qeu_copy->qeu_cur_tups = qeu_copy->qeu_cur_tups - (count+1);

    if (qeu_copy->qeu_cur_tups == 0)
    {
	/* The only way we read nothing is if the child signalled EOF
	** (or error).  If the child didn't pass back any error, set up
	** an E_QE0015_NO_MORE_ROWS, because that's what SCF is expecting.
	*/
	if (status == E_DB_WARN && copy_ctl->child_status == E_DB_OK)
	{
	    status = E_DB_ERROR;
	    qef_rcb->error.err_code = E_QE0015_NO_MORE_ROWS;
	}
	return (status);
    }
	    
    if(qeu_copy->qeu_att_info)
    {
	/*
	** We have some attributes which need special processing, so
	** loop over them in turn. We remember there may be multiple
	** tuples  to  process, so do this for each buffer.
	*/
        EX_CONTEXT exc_context;		/* Exception trap context */

        if (EXdeclare(qef_handler, &exc_context) != 0)
        {
            EXdelete();
            if(qef_cb->qef_adf_cb->adf_errcb.ad_errcode != 0)
            {
                status = qef_adf_error(&qef_cb->qef_adf_cb->adf_errcb, 
                                       E_DB_ERROR, qef_cb, &qef_rcb->error);
                qef_rcb->error.err_code = E_QE0022_QUERY_ABORTED;
            }
            else
            {
                qef_rcb->error.err_code = E_QE000B_ADF_INTERNAL_FAILURE;
            }
            return(E_DB_ERROR);
        }

    	count = qeu_copy->qeu_cur_tups;
        dataptr = qeu_copy->qeu_output;    
	for(i=0; i<count;  i++)
	{
	    bool first=TRUE;

	    tuple=dataptr->dt_data;
	    tmptuple=copy_ctl->tmptuple;
	
	    for(cpatt=qeu_copy->qeu_att_info; cpatt; cpatt=cpatt->cp_next)
	    {
	        if (cpatt->cp_flags & QEU_CPATT_EXP_INP)
	        {
		    /*
		    ** Special import/export processing, generally the
		    ** type coming from the frontend differs from the real type
		    ** and we have to do the conversion in the DBMS
		    */
		    fromdv.db_datatype=cpatt->cp_type;
		    fromdv.db_length=cpatt->cp_length;
		    fromdv.db_prec=cpatt->cp_prec;
		    fromdv.db_data = (PTR)(tuple+cpatt->cp_tup_offset);
		    fromdv.db_collID=-1;

		    intodv.db_datatype=
			sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_datatype;
		    intodv.db_length=
			sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_length;
		    intodv.db_prec=
			sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_prec;
		    intodv.db_data = (PTR)(tmptuple+cpatt->cp_ext_offset);
		    intodv.db_collID= -1;
		
		    if ((status = adc_cvinto(qef_cb->qef_adf_cb,
				    &fromdv, &intodv)) != E_DB_OK)
		    {
                        EXdelete();
			qef_rcb->error.err_code = E_QE000B_ADF_INTERNAL_FAILURE;
			return status;
		    }

		    /*
		    ** Now we need to move up the rest of the tuple to
		    ** fill up the  gap
		    */
		    if(first)
		    {
			/*
			** First attribute processed, so copy from start 
			** up to current position
			*/
			if(cpatt->cp_tup_offset>0)
				MEcopy ((PTR)tuple, 
					cpatt->cp_tup_offset, 
					(PTR)tmptuple);
			first=FALSE;
		    }
		    /*
		    ** Copy from current attribute end to end of tuple
		    ** to make sure everything is in sync
		    */
		    MEcopy(
		      (PTR)(tuple+cpatt->cp_tup_offset+cpatt->cp_length),
		      qeu_copy->qeu_tup_length-
			cpatt->cp_tup_offset-
			cpatt->cp_length,
		     (PTR)(tmptuple+cpatt->cp_ext_offset+
			sqlda->gca_col_desc[cpatt->cp_attseq].gca_attdbv.db_length));
		}
	    }
	    /*
	    ** Now copy the temp tuple back into the buffer
	    */
	    MEcopy((PTR)tmptuple, qeu_copy->qeu_ext_length, (PTR)tuple);
            dataptr = dataptr->dt_next;
	}
        EXdelete();
    }
    return (status);
} /* qeu_w_copy */

/*
** Name: copy_from_child - Child thread for COPY FROM
**
** Description:
**
**	This routine is the child worker thread for row-at-a-time COPY
**	FROM.  We read rows from the CUT buffer and append them to
**	the table.  Since there are fairly significant differences
**	between partitioned and unpartitioned table handling, we'll
**	just handle the child thread and CUT boilerplate here, and
**	call separate routines to do the row looping.
**
** Inputs:
**	ftx		SCF_FTX data block with ftx_data
**			pointing to the copy_ctl copy controller block
**			which points to qeu_copy, qef_rcb, etc.
**
** Outputs:
**	Returns E_DB_xxx status.
**	Sets child_status and child_error in copy_ctl as appropriate.
**
** History:
**	26-Apr-2004 (schka24)
**	    Write for partitioned COPY.
**	30-Apr-2004 (schka24)
**	    Use shared transaction context.
**	01-Jul-2004 (jenjo02)
**	    Replaced direct DMX transaction calls with
**	    with (new) QEF-friendly qet_connect/disconnect
**	    functions.
**	18-Jul-2004 (jenjo02)
**	    cut_read_buffer output buffer now a pointer to pointer
**	    to support LOCATE mode.
**	22-Mar-2010 (kschendel) SIR 123448
**	    Split off row loops so that partitioned load can be
**	    expanded to include bulk copy.
**	    Make sure qeu-r-copy can't get stuck in a write wait if
**	    we're exiting abnormally.
*/

static DB_STATUS
copy_from_child(SCF_FTX *ftx)
{

    bool use_load;			/* Should we use bulk-load? */
    CUT_RCB cut_rcb;			/* CUT request block */
    DB_ERROR cut_error;			/* CUT error info */
    DB_STATUS status;			/* Usual status thing */
    PTR tran_id = NULL;			/* Thread's txn context */
    EX_CONTEXT exc_context;		/* Exception trap context */
    i4 num_threads;			/* Number of threads for sync */
    QEF_RCB *qef_rcb;			/* COPY caller request block */
    QEU_COPY_CTL *copy_ctl;		/* Copy controller data block */

    copy_ctl = (QEU_COPY_CTL *) ftx->ftx_data;
    qef_rcb = copy_ctl->qef_rcb;
    cut_rcb.cut_buf_handle = NULL;

    if (EXdeclare(qef_handler, &exc_context) != 0)
    {
	status = E_DB_ERROR;
	copy_ctl->child_error.err_code = E_QE0002_INTERNAL_ERROR;
	goto error_exit;
    }

    /* usual dummy loop... */
    do
    {
	/* Parent is going to turn this off in the copy_ctl block once
	** we sync, so make a copy for reference:
	*/
	use_load = copy_ctl->use_load;

	/* Attach to CUT buffer */
	cut_rcb.cut_buf_use = CUT_BUF_READ;
	cut_rcb.cut_buf_handle = copy_ctl->cut_rcb.cut_buf_handle;
	status = cut_attach_buf(&cut_rcb, &copy_ctl->child_error);
	if (status != E_DB_OK)
	    break;

	/* Don't go any farther until we're sure that the parent has
	** noticed our presence.  (The COPY INTO direction requires this
	** kind of thread-sync, so it's easiest to use the same mechanism.)
	*/
	num_threads = 2;			/* Us + parent */
	status = cut_event_wait(&cut_rcb, CUT_SYNC, (PTR) &num_threads,
			&copy_ctl->child_error);
	if (status != E_DB_OK)
	    break;

	/* This thread needs its own copy of transaction context, because
	** LG and LK things are going to do things like fetch the current
	** session SCB.  We're us, but the transaction was started by
	** the parent.  One symptom of not doing this right is that we
	** wouldn't see force-abort.
	*/
	status = qet_connect(qef_rcb, &tran_id, &copy_ctl->child_error);
	if (status != E_DB_OK)
	    break;

	if ((copy_ctl->tbl_info.tbl_status_mask & DMT_IS_PARTITIONED) == 0)
	{
	    copy_from_unpartitioned(copy_ctl, &cut_rcb, tran_id);
	}
	else
	{
	    copy_from_partitioned(copy_ctl, &cut_rcb, tran_id, use_load);
	}

    } while (0);

    /* We're done, one way or another.  Clean up and exit.
    ** Indicate abort if we got an error (tells parent that we're on
    ** our way out and it doesn't need to send signals, etc.)
    */
error_exit:
    if (copy_ctl->child_status != E_DB_OK)
    {
	i4 num_cells;
	PTR toss;

	/* Flag that we're aborting, and make sure that the parent
	** isn't stuck in a write wait.  Use DIRECT so we don't need
	** someplace to actually copy a row.
	*/
	copy_ctl->state |= CCF_ABORT;
	num_cells = 1;
	(void) cut_read_buf(&num_cells, &cut_rcb, &toss,
			CUT_RW_DIRECT | CUT_RW_FLUSH, &cut_error);
	(void) cut_read_complete(&cut_rcb, &cut_error);
    }

    /* Terminate the transaction.  Since this is a shared transaction
    ** we'll merely detach from it.
    */
    if ( tran_id )
    {
	DB_ERROR local_error;
	status = qet_disconnect(qef_rcb, &tran_id, &local_error);
	if (status != E_DB_OK)
	{
	    if (copy_ctl->child_status == E_DB_OK)
	    {
		copy_ctl->child_status = status;
		copy_ctl->child_error = local_error;
	    }
	    TRdisplay("%@ [copy-into] Xact disconnect error, status=%d, err=%x\n",
		    status, local_error.err_code);
	}
    }

    /* Detach from CUT.  If the parent is waiting for us to finish,
    ** the detach will signal the parent.
    */
    (void) cut_signal_status(NULL, E_DB_OK, &cut_error);
    (void) cut_thread_term(TRUE, &cut_error);

    EXdelete();

    return (E_DB_OK);

} /* copy_from_child */

/*
** Name: copy_from_unpartitioned - Copy loop for unpartitioned COPY FROM
**
** Description:
**
**	This routine runs the child thread loop for unpartitioned COPY
**	FROM.  We read rows from the CUT buffer and append them to
**	the table.   For simplicity, and to free up a CUT cell as
**	quickly as possible, standard move-mode CUT reads are done.
**	We're doing row-puts, which are likely to be slower than the
**	COPY parent anyway, so anything that lets the parent get more
**	rows sooner will keep the child (the bottleneck) running.
**
**	The caller has opened up the CUT buffer and connected to the
**	COPY transaction.  All we have to do is open the table here in
**	the child thread, read rows, and stuff them at DMF.  After
**	completion, the table is closed, and we return to let the caller
**	clean up the transaction and child thread.
**
** Inputs:
**	copy_ctl	Pointer to QEU_COPY_CTL copy control info block.
**	cut_rcb		CUT_RCB pointer to connected CUT buffer.
**	tran_id		DMF needs the transaction ID for child.
**
** Outputs:
**	Sets child_status and child_error in copy_ctl as appropriate.
**
** History:
**	22-Mar-2010 (kschendel) SIR 123448
**	    Split off unpartitioned copy-from child main loop.
*/

static void
copy_from_unpartitioned(QEU_COPY_CTL *copy_ctl,CUT_RCB *cut_rcb, PTR tran_id)
{

    DB_ERROR cut_error;			/* CUT error info */
    DB_STATUS status;			/* Usual status thing */
    DMR_CB dmrcb;			/* Row-putter request block */
    DMT_CB dmtcb;			/* Table open/close request block */
    i4 num_cells;			/* Number of cells to read (1) */
    PTR row_temp;			/* Area to hold a row */
    QEU_COPY *qeu_copy;			/* COPY info from parser */
    ULM_RCB ulm;			/* Thread copy of memory stream */

    qeu_copy = copy_ctl->qeu_copy;
    row_temp = NULL;
    dmtcb.dmt_record_access_id = NULL;
    MEfill(sizeof(DMR_CB), 0, &dmrcb);

    /* usual dummy loop... */
    do
    {
	/* Prepare by allocating a row holder */

	STRUCT_ASSIGN_MACRO(copy_ctl->ulm, ulm);
	ulm.ulm_psize = qeu_copy->qeu_tup_length;
	status = qec_malloc(&ulm);
	if (status != E_DB_OK)
	{
	    copy_ctl->child_error = ulm.ulm_error;
	    break;
	}
	row_temp = (PTR) ulm.ulm_pptr;

	/* We'll be doing puts against a table already opened by the parent.
	** We need our own open-table instance against our thread's
	** version of the shared transaction context.
	*/
	STRUCT_ASSIGN_MACRO(copy_ctl->dmtcb, dmtcb);
	dmtcb.dmt_tran_id = tran_id;
	status = dmf_call(DMT_OPEN, &dmtcb);
	if (status != E_DB_OK)
	{
	    copy_ctl->child_error = dmtcb.error;
	    break;
	}

	/* Set up the DMR_CB we'll need for writing rows.  For non-
	** partitioned tables, the access ID in our local DMTCB
	** (dup'ed from the copy_ctl DMTCB) is the right one.
	*/
	dmrcb.type = DMR_RECORD_CB;
	dmrcb.length = sizeof(DMR_CB);
	dmrcb.dmr_flags_mask = 0;
	dmrcb.dmr_access_id = dmtcb.dmt_record_access_id;
	dmrcb.dmr_data.data_address = row_temp;
	dmrcb.dmr_data.data_in_size = qeu_copy->qeu_tup_length;

	/* Loop here to read rows arriving from the user, and
	** write them to the relation.
	*/
	while ((copy_ctl->state & CCF_ABORT) == 0)
	{
	    num_cells = 1;
	    status = cut_read_buf(&num_cells, cut_rcb, &row_temp,
			CUT_RW_WAIT, &cut_error);
	    if (status == E_DB_WARN
	      && cut_error.err_code == W_CU0305_NO_CELLS_READ)
	    {
		/* This is normal EOF */
		status = E_DB_OK;
		break;
	    }
	    if (status != E_DB_OK)
	    {
		copy_ctl->child_error = cut_error;
		break;
	    }

	    /* Do the PUT.  Ignore duplicate-key type errors. */
	    dmrcb.dmr_tid = 0;
	    status = dmf_call(DMR_PUT, &dmrcb);
	    if (status == E_DB_OK)
	    {
		/* Count actual rows here */
		++ qeu_copy->qeu_count;
	    }
	    else
	    {
		if (dmrcb.error.err_code == E_DM0046_DUPLICATE_RECORD ||
		  dmrcb.error.err_code == E_DM0045_DUPLICATE_KEY ||
		  dmrcb.error.err_code == E_DM0048_SIDUPLICATE_KEY ||
		  dmrcb.error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
		  dmrcb.error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT)
		    status = E_DB_OK;
		else
		{
		    copy_ctl->child_error = dmrcb.error;
		    break;
		}
	    }
	} /* while read-loop */
    } while (0);

    /* We're done, one way or another.  Clean up and exit. */

    copy_ctl->child_status = status;

    /* Close the table, if we opened it */
    if (dmtcb.dmt_record_access_id != NULL)
    {
	dmtcb.dmt_flags_mask = 0;
	/* RCB still in dmt-record-access-id */
	status = dmf_call(DMT_CLOSE, &dmtcb);
	if (status != E_DB_OK && copy_ctl->child_status == E_DB_OK)
	{
	    copy_ctl->child_status = status;
	    copy_ctl->child_error = dmtcb.error;
	}
    }

} /* copy_from_unpartitioned */

/*
** Name: copy_from_partitioned - Copy loop for partitioned COPY FROM
**
** Description:
**
**	This routine runs the child thread loop for partitioned COPY
**	FROM.  Loading a partitioned table is a bit more exciting than
**	an unpartitioned table, for two reasons.  One, each row must
**	have a "which-partition" calculation applied, and the row must
**	be aimed at the proper partition (which must be opened as needed).
**	Two, partitioned tables can be using bulk-load.
**
**	Normally, bulk-loads don't use a child thread.  The reason for
**	feeding partitioned bulk-loads through CUT is for batching.
**	A part of the win associated with bulk-load is that the DMF
**	interface for LOAD is multi-row.  With partitioning, each
**	row has to be which-partition'ed first and then attached to
**	a DM_MDATA list for the appropriate partition.  In order to
**	get enough input rows for good batching of (say) a hash
**	partition, we need more buffering than the sequencer is
**	likely to provide.
**
**	Partitioned bulk-load also has to deal with DMF rejections of
**	bulk-load on a partition by partition basis.  So, we might be
**	bulk-loading some partitions, and row-loading others.  So as to
**	handle things in a reasonably uniform manner, we'll do the
**	which-part-ing and build one DM_MDATA chain for each partition
**	about to receive rows.  Then, while stuffing each DM_MDATA
**	chain at DMF, it's easy to select DMR_LOAD, or run a wee loop
**	to do DMR_PUT's.  (Eventually, DMR_PUT ought to get an MDATA
**	style alternate interface!)
**
**	Because we want to pull a whole bunch of rows from CUT, and
**	then submit them all to DMF, the CUT DIRECT mode is used.
**	DIRECT mode returns a pointer to a batch of CUT-buffer cells.
**	We can't free the cells until the whole batch is processed,
**	which is a bit of a down-side, but it means we can avoid
**	allocating memory and moving rows.
**
** Inputs:
**	copy_ctl	Pointer to QEU_COPY_CTL copy control info block.
**	cut_rcb		CUT_RCB pointer to connected CUT buffer.
**	tran_id		DMF needs the transaction ID for child.
**	use_load	TRUE if we should attempt bulk-loads.  NOTE:
**			the use-load flag in the copy_ctl CANNOT be
**			used here, as it means something different now
**			(for the parent thread).
**
** Outputs:
**	Sets child_error in copy_ctl as appropriate.
**
** History:
**	22-Mar-2010 (kschendel) SIR 123448
**	    Split off partitioned copy-from child main loop.
**	    Use DIRECT mode and do local DM_MDATA linking, to enable
**	    the bulk-loading of partitioned tables.
*/

static void
copy_from_partitioned(QEU_COPY_CTL *copy_ctl, CUT_RCB *cut_rcb,
	PTR tran_id, bool use_load)
{

    ADF_CB *adfcb;			/* Session ADF control block */
    char *p;				/* Memory layout assistant */
    DB_ERROR cut_error;			/* CUT error info */
    DB_STATUS status;			/* Usual status thing */
    DM_DATA *table_options;		/* List of bulk-load options */
    DM_MDATA *md;			/* general DM_MDATA pointer */
    DM_MDATA *mdata_base;		/* Base of DM_MDATA block array */
    DM_MDATA *mdata_next;		/* Next DM_MDATA to use */
    DM_MDATA **pp_mdata;		/* pp_mdata[partno] is base of mdata
					** chain for that partition
					*/
    DMR_CB dmrcb;			/* Row-putter request block */
    DMT_CB dmtcb;			/* Table open/close request block */
    DMT_PHYS_PART *pparray;		/* Physical partition info array */
    DMT_SHW_CB *dmtshow;		/* Table "show" request block */
    i4 cntsize;				/* Size of per-partition counters */
    i4 idsize;				/* Size (bytes) of access_ids array */
    i4 max_cells;			/* Total number of cells in buffer */
    i4 mdsize;				/* Size (bytes) of pp_mdata */
    i4 nparts;				/* Number of physical partitions */
    i4 num_cells;			/* Number of cells to read */
    i4 num_pps;				/* Number of entries in pps_with_rows */
    i4 partno;				/* Physical partition number */
    i4 pp_row_est;			/* Row estimate per-partition */
    i4 ppsize;				/* Size (bytes) DMT_PHYS_PART array */
    i4 *pps_with_rows;			/* pps_with_rows[0..num_pps-1] is a list
					** of partition numbers for which we
					** have received rows.  (Not ordered.)
					*/
    i4 *pp_mdcount;			/* pp_mdcount[partno] is the number
					** of rows being sent to that partition
					*/
    PTR read_cell;			/* Pointer to available read cells */
    PTR *access_ids;			/* Partition access ID's */
    QEF_RCB *qef_rcb;			/* COPY caller request block */
    QEU_COPY *qeu_copy;			/* COPY info from parser */
    ULM_RCB ulm;			/* Thread copy of memory stream */
    u_i2 upartno;			/* Partno the way whichpart likes it */

    qef_rcb = copy_ctl->qef_rcb;
    qeu_copy = copy_ctl->qeu_copy;
    access_ids = NULL;
    MEfill(sizeof(DMR_CB), 0, &dmrcb);
    dmrcb.type = DMR_RECORD_CB;
    dmrcb.length = sizeof(DMR_CB);
    table_options = NULL;
    if ( qeu_copy->qeu_stat & CPY_DMR_CB )
    {
	if ( qeu_copy->qeu_dmrcb->dmr_char_array.data_in_size != 0)
	    table_options = &qeu_copy->qeu_dmrcb->dmr_char_array;
    }

    /* usual dummy loop... */
    do
    {
	STRUCT_ASSIGN_MACRO(copy_ctl->ulm, ulm);
	adfcb = qef_rcb->qef_cb->qef_adf_cb;
	max_cells = copy_ctl->cut_rcb.cut_num_cells;
	nparts = copy_ctl->tbl_info.tbl_nparts;

	/* A cut read in DIRECT mode can potentially return up to
	** max-cells filled cells.  We need one DM_MDATA for each one.
	** Allocate memory for the DM_MDATA's.
	*/
	ulm.ulm_psize = max_cells * sizeof(DM_MDATA);
	status = qec_malloc(&ulm);
	if (status != E_DB_OK)
	{
	    copy_ctl->child_error = ulm.ulm_error;
	    break;
	}
	mdata_base = (DM_MDATA *) ulm.ulm_pptr;

	/* All the DM_MDATA data_size's are the same, go ahead and fill
	** them in now.
	*/
	for (md = mdata_base + max_cells - 1;
	     md >= mdata_base;
	     --md)
	{
	    md->data_size = qeu_copy->qeu_tup_length;
	}

	/* Since this is a partitioned table, we need more info about the
	** table.  Specifically, we need the physical partition table
	** ID's.  We also need an array of open-partition table access
	** ID's.  Allocate memory for these things.
	*/
	/* Allocate more memory, this time for per-partition stuff.
	** We need an array of physical partition table ID's, embedded
	** in DMT_PHYS_PART structures.
	** We need an array of open-partition table access ID's.
	** We need an array of DM_MDATA list headers, and an array of counts.
	** To avoid useless looping to find the next partition that we
	** received rows for, we need an array of partition numbers
	** for which we got rows.
	** And finally we need an array of can-we-bulk-load flags, but
	** since we're wasting most of the DMT_PHYS_PART, we'll repurpose
	** the pp_reltups member (which we otherwise ignore) as that flag.
	*/
	ppsize = nparts * sizeof(DMT_PHYS_PART);
	ppsize = DB_ALIGN_MACRO(ppsize);
	idsize = nparts * sizeof(PTR);
	idsize = DB_ALIGN_MACRO(idsize);
	mdsize = nparts * sizeof(DM_MDATA *);
	mdsize = DB_ALIGN_MACRO(mdsize);
	cntsize = nparts * sizeof(i4);
	cntsize = DB_ALIGN_MACRO(cntsize);
	ulm.ulm_psize = ppsize + idsize + mdsize + cntsize + cntsize + sizeof(DMT_SHW_CB);
	status = qec_malloc(&ulm);
	if (status != E_DB_OK)
	{
	    copy_ctl->child_error = ulm.ulm_error;
	    break;
	}
	access_ids = (PTR *) ulm.ulm_pptr;
	p = (char *) access_ids + idsize;
	p = (char *) ME_ALIGN_MACRO(p, DB_ALIGN_SZ);
	pparray = (DMT_PHYS_PART *) p;
	p = p + ppsize;
	p = (char *) ME_ALIGN_MACRO(p, DB_ALIGN_SZ);
	pp_mdata = (DM_MDATA **) p;
	p = p + mdsize;
	p = (char *) ME_ALIGN_MACRO(p, DB_ALIGN_SZ);
	pp_mdcount = (i4 *) p;
	p = p + cntsize;
	p = (char *) ME_ALIGN_MACRO(p, DB_ALIGN_SZ);
	pps_with_rows = (i4 *) p;
	p = p + cntsize;
	p = (char *) ME_ALIGN_MACRO(p, DB_ALIGN_SZ);
	dmtshow = (DMT_SHW_CB *) p;
	/* NULL access-id means not opened yet */
	MEfill(idsize, 0, access_ids);
	MEfill(sizeof(DMT_SHW_CB), 0, dmtshow);

	/* Ask DMF for physical partition details */
	dmtshow->type = DMT_SH_CB;
	dmtshow->length = sizeof(DMT_SHW_CB);
	dmtshow->dmt_session_id = qef_rcb->qef_sess_id;
	dmtshow->dmt_db_id = qef_rcb->qef_db_id;
	dmtshow->dmt_record_access_id = copy_ctl->dmtcb.dmt_record_access_id;
	dmtshow->dmt_flags_mask = DMT_M_PHYPART | DMT_M_ACCESS_ID;
	dmtshow->dmt_phypart_array.data_address = (PTR) pparray;
	dmtshow->dmt_phypart_array.data_in_size = ppsize;
	STRUCT_ASSIGN_MACRO(qeu_copy->qeu_tab_id, dmtshow->dmt_tab_id);
	status = dmf_call(DMT_SHOW, dmtshow);
	if (status != E_DB_OK)
	{
	    copy_ctl->child_error = dmtshow->error;
	    break;
	}

	/* If bulk-loading, we'll need a row estimate per partition. */
	pp_row_est = (copy_ctl->row_est + nparts - 1) / nparts;

	/* Loop here to read rows arriving from the user, and
	** write them to the relation.
	*/
	while ((copy_ctl->state & CCF_ABORT) == 0)
	{
	    /* DIRECT read returns pointer to the first cell in read_cell,
	    ** and the number of cells in num_cells.
	    */
	    status = cut_read_buf(&num_cells, cut_rcb, &read_cell,
			CUT_RW_DIRECT | CUT_RW_WAIT, &cut_error);
	    if (status == E_DB_WARN
	      && cut_error.err_code == W_CU0305_NO_CELLS_READ)
	    {
		/* This is normal EOF */
		status = E_DB_OK;
		break;
	    }
	    if (status != E_DB_OK)
	    {
		copy_ctl->child_error = cut_error;
		break;
	    }
	    /* Set the entire pp_mdata pointer array to NULL, and
	    ** reset the next DM_MDATA-to-use pointer.
	    */

	    MEfill(mdsize, 0, pp_mdata);
	    mdata_next = mdata_base;
	    MEfill(cntsize, 0, pp_mdcount);
	    num_pps = 0;

	    /* Process the cells, assigning a DM_MDATA to point to each one,
	    ** and applying which-partition in order to hook the DM_MDATA
	    ** to the proper list.
	    */
	    do
	    {
		md = mdata_next++;
		md->data_address = (char *) read_cell;

		status = adt_whichpart_no(adfcb, qeu_copy->qeu_part_def,
			    read_cell, &upartno);
		if (status != E_DB_OK)
		{
		    copy_ctl->child_error.err_code = adfcb->adf_errcb.ad_errcode;
		    break;
		}
		partno = upartno;
		/* Count this row, link into to proper chain */
		if (pp_mdcount[partno] == 0)
		    pps_with_rows[num_pps++] = partno;
		++pp_mdcount[partno];
		md->data_next = pp_mdata[partno];
		pp_mdata[partno] = md;

		/* Next cell if any */
		read_cell += md->data_size;
	    } while (--num_cells > 0);
	    if (status != E_DB_OK)
		break;

	    /* Go through the partitions for which we received data,
	    ** and send the rows to DMF.  This involves possibly opening
	    ** the partition, and possibly attempting a start-load.
	    */

	    while (--num_pps >= 0)
	    {
		partno = pps_with_rows[num_pps];

		dmrcb.dmr_tid = 0;
		dmrcb.dmr_val_logkey = 0;
		dmrcb.dmr_flags_mask = 0;
		dmrcb.dmr_s_estimated_records = 0;
		dmrcb.dmr_char_array.data_in_size = 0;
		dmrcb.dmr_access_id = access_ids[partno];
		if (dmrcb.dmr_access_id == NULL)
		{
		    /* We have to open this partition.  Use the same
		    ** flags, access mode, etc from the master.
		    */
		    STRUCT_ASSIGN_MACRO(copy_ctl->dmtcb, dmtcb);
		    STRUCT_ASSIGN_MACRO(pparray[partno].pp_tabid,
				dmtcb.dmt_id);
		    dmtcb.dmt_tran_id = tran_id;
		    status = dmf_call(DMT_OPEN, &dmtcb);
		    if (status != E_DB_OK)
		    {
			copy_ctl->child_error = dmtcb.error;
			break;
		    }
		    dmrcb.dmr_access_id =
			access_ids[partno] = dmtcb.dmt_record_access_id;
		    /* More setup if LOAD, pass estimated rows, set up
		    ** proper start-of-load flags.  Ask for non-parallel
		    ** sort, because sort threads per partition might be
		    ** too many threads.
		    ** Attempt to start the load, if it's rejected, then
		    ** note that fact.  Unlike an unpartitioned table load,
		    ** we don't close and re-open the partition;  the
		    ** reason for doing that is to turn off X locking, and
		    ** locking is not per-partition at this time.
		    */
		    if (use_load)
		    {
			dmrcb.dmr_flags_mask = DMR_TABLE_EXISTS | DMR_NOPARALLEL;
			dmrcb.dmr_s_estimated_records = pp_row_est;
			/* Steal pp_reltups as a bulk-load flag!
			** Assume no bulk-load, in case of failure.
			*/
			pparray[partno].pp_reltups = 0;
			dmrcb.dmr_mdata = NULL;
			dmrcb.dmr_count = 0;

			/* If the user has specified information on how to
			** build the table pass this onto DMF
			*/
			if (table_options != NULL)
			{
			    dmrcb.dmr_flags_mask |= DMR_CHAR_ENTRIES;
			    dmrcb.dmr_char_array = *table_options;
			}
			status = dmf_call(DMR_LOAD, &dmrcb);
			if (status != E_DB_OK && 
			  dmrcb.error.err_code != E_DM0050_TABLE_NOT_LOADABLE)
			{
			    /* Other errors are bad */
			    copy_ctl->child_error = dmrcb.error;
			    break;
			}
			else if (status == E_DB_OK)
			{
			    pparray[partno].pp_reltups = 1;
			}
			else
			{
			    /* "table not loadable", leave flag at zero.
			    ** Runtime error if non-heap and table options.
			    */
			    if (table_options != NULL
			      && copy_ctl->tbl_info.tbl_storage_type != DMT_HEAP_TYPE)
			    {
				status = E_DB_ERROR;
				SETDBERR(&copy_ctl->child_error, 0, E_US14E4_5348_NON_FAST_COPY);
				break;
			    }
			}
			/* Clean up DMR_CB for next usage, below */
			dmrcb.dmr_tid = 0;
			dmrcb.dmr_val_logkey = 0;
			dmrcb.dmr_flags_mask = 0;
			dmrcb.dmr_s_estimated_records = 0;
			dmrcb.dmr_char_array.data_in_size = 0;
		    } /* if use-load */
		} /* if needed to open */

		/* Attempt load, or series of row-puts.
		** Ignore duplicate-key type errors.  No need to check for
		** secondary-index errors from bulk-load, as SI's prevent
		** bulk-load.  (At least for 2010. Give me time.)
		*/
		if (use_load && pparray[partno].pp_reltups)
		{
		    dmrcb.dmr_flags_mask = DMR_TABLE_EXISTS;
		    dmrcb.dmr_mdata = pp_mdata[partno];
		    dmrcb.dmr_count = pp_mdcount[partno];
		    status = dmf_call(DMR_LOAD, &dmrcb);
		    if (status != E_DB_OK)
		    {
			if (dmrcb.error.err_code == E_DM0046_DUPLICATE_RECORD ||
			  dmrcb.error.err_code == E_DM0045_DUPLICATE_KEY ||
			  dmrcb.error.err_code == E_DM0150_DUPLICATE_KEY_STMT)
			    status = E_DB_OK;
			/* else bad status noticed below */
		    }
		    /* LOAD's don't count rows incrementally.  DMF gives
		    ** us the good news at end-load time.
		    */
		}
		else
		{
		    md = pp_mdata[partno];
		    do
		    {
			dmrcb.dmr_data.data_address = md->data_address;
			dmrcb.dmr_data.data_in_size = md->data_size;
			status = dmf_call(DMR_PUT, &dmrcb);
			if (status == E_DB_OK)
			{
			    /* Count actual rows here */
			    ++ qeu_copy->qeu_count;
			}
			else
			{
			    if (dmrcb.error.err_code == E_DM0046_DUPLICATE_RECORD ||
			      dmrcb.error.err_code == E_DM0045_DUPLICATE_KEY ||
			      dmrcb.error.err_code == E_DM0048_SIDUPLICATE_KEY ||
			      dmrcb.error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
			      dmrcb.error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT)
			    {
				status = E_DB_OK;
			    }
			    else
			    {
				break;
			    }
			}
			md = md->data_next;
		    } while (md != NULL);
		}
		if (status != E_DB_OK)
		{
		    copy_ctl->child_error = dmrcb.error;
		    break;
		}
	    } /* for partno loop */
	    if (status != E_DB_OK)
		break;

	    /* Cycle back for another read.  The initiation of a new
	    ** DIRECT read will also imply the completion of the previous
	    ** direct read.
	    */
	} /* while read-loop */
    } while (0);

    /* We're done, one way or another.  Clean up and exit.
    ** Indicate abort if we got an error (tells parent that we're on
    ** our way out and it doesn't need to send signals, etc.)
    **
    ** While it would be possible to call cut_read_complete here, there
    ** is no particular need to do so.  The detach that our caller will
    ** perform will complete the read too.
    */

    copy_ctl->child_status = status;

    if (access_ids != NULL)
    {
	/* Close any opened partitions */
	for (partno = copy_ctl->tbl_info.tbl_nparts-1; partno >= 0; --partno)
	{
	    if (access_ids[partno] != NULL)
	    {
		if (use_load && pparray[partno].pp_reltups
		  && copy_ctl->child_status == E_DB_OK)
		{
		    dmrcb.dmr_access_id = access_ids[partno];
		    dmrcb.dmr_count = 0;
		    dmrcb.dmr_mdata = (DM_MDATA *) 0;
		    dmrcb.dmr_flags_mask = DMR_TABLE_EXISTS | DMR_ENDLOAD;
		    status = dmf_call(DMR_LOAD, &dmrcb);
		    if (status != E_DB_OK && copy_ctl->child_status == E_DB_OK)
		    {
			copy_ctl->child_error = dmtcb.error;
			copy_ctl->child_status = status;
		    }
		    if (status == E_DB_OK)
		    {
			/*
			** Get actual number of rows added to partition.
			** Sorter may have removed duplicates.
			*/
			qeu_copy->qeu_count += dmrcb.dmr_count;
		    }
		}

		/* Close this table.  Use the local DMT_CB that we used
		** for opening tables.
		*/
		dmtcb.dmt_flags_mask = 0;
		dmtcb.dmt_record_access_id = access_ids[partno];
		status = dmf_call(DMT_CLOSE, &dmtcb);
		if (status != E_DB_OK && copy_ctl->child_status == E_DB_OK)
		{
		    copy_ctl->child_error = dmtcb.error;
		    copy_ctl->child_status = status;
		}
	    }
	} /* for */
    }

} /* copy_from_partitioned */

/*
** Name: copy_into_child - Child thread for COPY INTO
**
** Description:
**
**	This routine is the child worker thread for row-at-a-time COPY
**	INTO.  We read rows from the table and write them out through
**	the CUT buffer.
**
**	For a regular, non-partitioned table this is pretty simple.
**	All we have to do is (re)open the table in our thread, and
**	get rows from DMF.
**
**	For a partitioned table, things are a little bit more
**	interesting, but not much.  We open each partition in turn,
**	read it, and close it.
**
** Inputs:
**	ftx		SCF_FTX data block with ftx_data
**			pointing to the copy_ctl copy controller block
**
** Outputs:
**	Returns E_DB_xxx status.
**	Sets child_status and child_error in copy_ctl as appropriate.
**
** History:
**	26-Apr-2004 (schka24)
**	    Write for partitioned COPY.
**	30-Apr-2004 (schka24)
**	    Use shared transaction context.
**	01-Jul-2004 (jenjo02)
**	    Replaced direct DMX transaction calls with
**	    with (new) QEF-friendly qet_connect/disconnect
**	    functions.
**	08-Sep-2004 (jenjo02)
**	    cut_write_buf prototype change for DIRECT mode.
*/

static DB_STATUS
copy_into_child(SCF_FTX *ftx)
{

    CUT_RCB cut_rcb;			/* CUT request block */
    DB_ERROR cut_error;			/* CUT error info */
    DB_STATUS local_status;
    DB_STATUS status;			/* Usual status thing */
    DMR_CB dmrcb;			/* Row-putter request block */
    DMT_CB dmtcb;			/* Table open/close request block */
    DMT_PHYS_PART *pparray;		/* Physical partition info array */
    DMT_SHW_CB *dmtshow;		/* Table "show" request block */
    PTR		tran_id = NULL;		/* Thread's txn context */
    EX_CONTEXT exc_context;		/* Exception trap context */
    i4 num_cells;			/* Number of cells to read (1) */
    i4 num_partitions;			/* Number of physical partitions */
    i4 partno;				/* Physical partition number */
    i4 ppsize;				/* Size of DMT_PHYS_PART array */
    PTR row_temp;			/* Area to hold a row */
    QEF_RCB *qef_rcb;			/* COPY caller request block */
    QEF_CB  *qef_cb;
    QEU_COPY *qeu_copy;			/* COPY info from parser */
    QEU_COPY_CTL *copy_ctl;		/* Copy controller data block */
    ULM_RCB ulm;			/* Thread copy of memory stream */

    copy_ctl = (QEU_COPY_CTL *) ftx->ftx_data;
    qef_rcb = copy_ctl->qef_rcb;
    qef_cb = qef_rcb->qef_cb;
    qeu_copy = copy_ctl->qeu_copy;
    cut_rcb.cut_buf_handle = NULL;
    row_temp = NULL;
    pparray = NULL;
    dmtcb.dmt_record_access_id = NULL;
    MEfill(sizeof(DMR_CB), 0, &dmrcb);

    if (EXdeclare(qef_handler, &exc_context) != 0)
    {
	status = E_DB_ERROR;
	copy_ctl->child_error.err_code = E_QE0002_INTERNAL_ERROR;
	goto error_exit;
    }

    /* usual dummy loop... */
    do
    {
	/* Normal thread startup. */
	STRUCT_ASSIGN_MACRO(copy_ctl->ulm, ulm);

	/* Attach to CUT buffer */
	cut_rcb.cut_buf_use = CUT_BUF_WRITE;
	cut_rcb.cut_buf_handle = copy_ctl->cut_rcb.cut_buf_handle;
	status = cut_attach_buf(&cut_rcb, &copy_ctl->child_error);
	if (status != E_DB_OK)
	    break;

	/* Don't go any farther until we're sure that the parent has
	** noticed our presence.
	*/
	num_cells = 2;			/* Us + parent */
	status = cut_event_wait(&cut_rcb, CUT_SYNC, (PTR) &num_cells,
			&copy_ctl->child_error);
	if (status != E_DB_OK)
	    break;

	/* Unfortunately we need someplace for DMF to get rows into */
	ulm.ulm_psize = qeu_copy->qeu_tup_length;
	status = qec_malloc(&ulm);
	if (status != E_DB_OK)
	{
	    copy_ctl->child_error = ulm.ulm_error;
	    break;
	}
	row_temp = (PTR) ulm.ulm_pptr;

	/* This thread needs its own copy of transaction context, because
	** LG and LK things are going to do things like fetch the current
	** session SCB.  We're us, but the transaction was started by
	** the parent.
	*/
	status = qet_connect(qef_rcb, &tran_id, &copy_ctl->child_error);
	if (status != E_DB_OK)
	    break;

	/* if this is a partitioned table, we need more info about the
	** table.  Specifically, we need the physical partition table
	** ID's.  Allocate memory for them.
	**
	** If on the other hand it's not a partitioned table, we'll be
	** doing gets against a table already opened by the parent.
	** We need our own "copy" against our shared transaction context.
	*/
	num_partitions = 1;		/* For unpartitioned */
	if ((copy_ctl->tbl_info.tbl_status_mask & DMT_IS_PARTITIONED) == 0)
	{
	    /* Not partitioned, just (re)open the table */
	    STRUCT_ASSIGN_MACRO(copy_ctl->dmtcb, dmtcb);
	    dmtcb.dmt_tran_id = tran_id;
	    status = dmf_call(DMT_OPEN, &dmtcb);
	    if (status != E_DB_OK)
	    {
		STRUCT_ASSIGN_MACRO(dmtcb.error, copy_ctl->child_error);
		break;
	    }
	}
	else
	{
	    num_partitions = copy_ctl->tbl_info.tbl_nparts;
	    ppsize = num_partitions * sizeof(DMT_PHYS_PART);
	    ulm.ulm_psize = ppsize + sizeof(DMT_SHW_CB);
	    status = qec_malloc(&ulm);
	    if (status != E_DB_OK)
	    {
		copy_ctl->child_error = ulm.ulm_error;
		break;
	    }
	    pparray = (DMT_PHYS_PART *) ulm.ulm_pptr;
	    dmtshow = (DMT_SHW_CB *) ((PTR) pparray + ppsize);
	    MEfill(sizeof(DMT_SHW_CB), 0, dmtshow);

	    /* Ask DMF for physical partition details */
	    dmtshow->type = DMT_SH_CB;
	    dmtshow->length = sizeof(DMT_SHW_CB);
	    dmtshow->dmt_session_id = qef_rcb->qef_sess_id;
	    dmtshow->dmt_db_id = qef_rcb->qef_db_id;
	    dmtshow->dmt_record_access_id = copy_ctl->dmtcb.dmt_record_access_id;
	    dmtshow->dmt_flags_mask = DMT_M_PHYPART | DMT_M_ACCESS_ID;
	    dmtshow->dmt_phypart_array.data_address = (PTR) pparray;
	    dmtshow->dmt_phypart_array.data_in_size = ppsize;
	    STRUCT_ASSIGN_MACRO(qeu_copy->qeu_tab_id, dmtshow->dmt_tab_id);
	    status = dmf_call(DMT_SHOW, dmtshow);
	    if (status != E_DB_OK)
	    {
		STRUCT_ASSIGN_MACRO(dmtshow->error, copy_ctl->child_error);
		break;
	    }
	}

	/* Set up the DMR_CB we'll need for reading rows.
	** For partitioned tables, we'll be playing with the access ID
	** as we proceed.
	*/

	dmrcb.type = DMR_RECORD_CB;
	dmrcb.length = sizeof(DMR_CB);
	dmrcb.dmr_flags_mask = 0;
	dmrcb.dmr_access_id = dmtcb.dmt_record_access_id;

	/* Loop on partitions */
	for (partno = 0;
	     partno < num_partitions && (copy_ctl->state & CCF_ABORT) == 0;
	     ++ partno)
	{
	    /* If really partitioned, open the partition */
	    if (pparray != NULL)
	    {
		STRUCT_ASSIGN_MACRO(copy_ctl->dmtcb, dmtcb);
		STRUCT_ASSIGN_MACRO(pparray[partno].pp_tabid, dmtcb.dmt_id);
		dmtcb.dmt_tran_id = tran_id;
		/* Can't be session temp, don't check */
		status = dmf_call(DMT_OPEN, &dmtcb);
		if (status != E_DB_OK)
		{
		    STRUCT_ASSIGN_MACRO(dmtcb.error, copy_ctl->child_error);
		    break;
		}
		dmrcb.dmr_access_id = dmtcb.dmt_record_access_id;
	    }
	    /* Relation is open, we need to position to read ALL */
	    dmrcb.dmr_position_type = DMR_ALL;
	    dmrcb.dmr_q_fcn = 0;
	    dmrcb.dmr_q_arg = 0;
	    dmrcb.dmr_tid = 0;
	    status = dmf_call(DMR_POSITION, &dmrcb);
	    if (status != E_DB_OK)
	    {
		if (dmrcb.error.err_code == E_DM0055_NONEXT)
		    status = E_DB_OK;
		else
		{
		    STRUCT_ASSIGN_MACRO(dmrcb.error, copy_ctl->child_error);
		    break;
		}
	    }

	    /* Loop here to read rows from the relation, and
	    ** write them to the user via CUT.
	    */
	    dmrcb.dmr_data.data_address = row_temp;
	    dmrcb.dmr_data.data_in_size = qeu_copy->qeu_tup_physical;
	    while ((copy_ctl->state & CCF_ABORT) == 0)
	    {
		/* Read a row */
		dmrcb.dmr_flags_mask = DMR_NEXT;
		dmrcb.dmr_tid = 0;
		status = dmf_call(DMR_GET, &dmrcb);
		if (status != E_DB_OK)
		{
		    if (dmrcb.error.err_code == E_DM0055_NONEXT)
		    {
			/* Normal EOF */
			status = E_DB_OK;
		    }
		    else
		    {
			STRUCT_ASSIGN_MACRO(dmrcb.error, copy_ctl->child_error);
		    }
		    break;		/* whether error or EOF */
		}

		/* Stuff it at CUT */
		num_cells = 1;
		status = cut_write_buf(&num_cells, &cut_rcb, &row_temp,
			CUT_RW_WAIT, &cut_error);
		if (status != E_DB_OK)
		{
		    STRUCT_ASSIGN_MACRO(cut_error, copy_ctl->child_error);
		    break;
		}
	    } /* read-write loop */

	    /* Close the table or partition that we were reading */
	    dmtcb.dmt_flags_mask = 0;
	    local_status = dmf_call(DMT_CLOSE, &dmtcb);
	    dmtcb.dmt_record_access_id = NULL;
	    if (local_status != E_DB_OK && status == E_DB_OK)
	    {
		STRUCT_ASSIGN_MACRO(dmtcb.error, copy_ctl->child_error);
		status = local_status;
	    }
	    if (status != E_DB_OK)
		break;

	} /* for partition-loop */
    } while (0);

    /* We're done, one way or another.  Clean up and exit.
    ** Indicate abort if we got an error (tells parent that we're on
    ** our way out and it doesn't need to send signals, etc.)
    */
error_exit:
    if (status != E_DB_OK)
	copy_ctl->state |= CCF_ABORT;

    copy_ctl->child_status = status;

    /* If we were interrupted and a table is open, close it */
    if (dmtcb.dmt_record_access_id != NULL)
    {
	dmtcb.dmt_flags_mask = 0;
	/* RCB still in dmt-record-access-id */
	local_status = dmf_call(DMT_CLOSE, &dmtcb);
    }
    /* Terminate the transaction.  Since this is a shared transaction
    ** we'll merely detach from it.
    */
    if ( tran_id )
    {
	status = qet_disconnect(qef_rcb, &tran_id, &copy_ctl->child_error);
	if (status != E_DB_OK)
	    TRdisplay("%@ [copy-into] Xact disconnect error, status=%d, err=%x\n",
		    status, copy_ctl->child_error.err_code);
    }

    /* Tell reading end that there's no more coming */
    status = cut_writer_done(&cut_rcb, &cut_error);

    /* Detach from CUT.  If the parent is waiting for us to finish,
    ** the detach will signal the parent.  (For COPY INTO, though, it's
    ** more likely that we will be gone by the time the parent notices
    ** the EOF.)
    */
    (void) cut_signal_status(NULL, E_DB_OK, &cut_error);
    (void) cut_thread_term(TRUE, &cut_error);

    EXdelete();

    return (E_DB_OK);

} /* copy_into_child */
