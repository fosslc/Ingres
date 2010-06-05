/*
**Copyright (c) 2010 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <bt.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <tm.h>
#include    <pc.h>
#include    <sr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <scf.h>
#include    <ulf.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmccb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmucb.h>
#include    <dmxcb.h>
#include    <dm.h>
#include    <dml.h>
#include    <dm0m.h>
#include    <dmp.h>
#include    <dm2f.h>
#include    <dm2r.h>
#include    <dm2t.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm1h.h>
#include    <dmpp.h>
#include    <dm1p.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dmse.h>
#include    <dm2umct.h>
#include    <dm2umxcb.h>
#include    <dm2u.h>
#include    <dm2up.h>
#include    <dma.h>
#include    <dmpbuild.h>
#include    <cm.h>
#include    <cui.h>

static STATUS create_index_threads(
				DM2U_MXCB       *mxcbs,
				CHILD_CB        *ccb,
				EXCH_BUF_HDR    *exch_hdr,
				EXCH_BUF        *exch_buf,
				i4         no_of_ind,
				DB_ERROR        *dberr);
static STATUS check_child_status(
				CHILD_CB *ccb,
				i4  no_of_ind,
				DB_ERROR  *dberr);
static i4 count_indexes(
				DM2U_MXCB	*mxcbs);
static VOID wait_for_index_finish(EXCH_BUF_HDR *exch_hdr);
static STATUS get_record(
			  EXCH_BUF_HDR *exch_buf_hdr,
			  EXCH_BUF     *cur_buf,
			  DM2U_TPCB    *tp,
			  i4      nth,
			  i4      reclen,
			  DM_TID       *tid);
static STATUS put_record(
			  EXCH_BUF_HDR  *exch_buf_hdr,
			  EXCH_BUF 	**cur_buf_ptr,
			  DM_TID 	*tid,
			  char 		*record,
			  i4 	reclen,
			  i4 	noofchild,
			  i4 	*end_of_file,
			  i4 	local_status,
			  DB_ERROR 	*dberr);
static STATUS switch_buffer(
			  EXCH_BUF_HDR  *exch_buf_hdr, 
			  EXCH_BUF 	**cur_buf_ptr,
			  i4 	noofchild);
static STATUS setup_buffers(
			  EXCH_BUF_HDR 	*exch_hdr,
			  char 		*exch_buf,
			  i4 	no_of_ind,
			  i4 	no_of_buffers,
			  i4 	no_of_records,
			  i4 	size_of_buffer);

static VOID cleanup_buffers(
			  char	     **buf_p,
			  EXCH_BUF_HDR *exch_hdr);

static STATUS load_sorter(
			  DM2U_TPCB    *tp,
			  DM_TID     *tid,
			  DB_ERROR    *dberr);
static STATUS init_child_thread(
			  DM2U_TPCB    *tp,
			  DM_OBJECT  **wloc_mask,
			  DB_ERROR    *dberr);

/*
**
**  Name: DM2UPUTI.C - Parallel Index creation utility routines.
**
**  Description:
**      This file contains routines that perform creation of thread,
**      reading from exchange buffers, putting record on exchange
**	buffers, child thread synchronization.
**
**  History:
**      10-april-1998 (nanpr01)
**	    created.
**      05-may-2000 (stial01, gupsh01)
**	    added check for the child thread getting ahead of parent thread.
**      16-may-2000 (stial01)
**          put_record() fixed code added 05-may-2000 for B101448
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	16-feb-2000 (stial01)
**	    init_child_thread() Init mct_ver_number (b104008)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      24-apr-2003 (stial01)
**          Don't zero fill DM0M_ZERO exchange buffers. 
**          Use reclen of projected columns only (b110061)
**	22-Jan-2004 (jenjo02)
**	    Preliminary support of Global indexes, TID8s.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**	28-Feb-2004 (jenjo02)
**	    Modified to new form (one MXCB, potential for
**	    multiple sources/targets), but this code
**	    isn't yet run for Global indexes.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      25-jun-2004 (stial01)
**          Init mct_segbuf to be different than mct_crecord
**	18-dec-2004 (thaju02)
**	    In load_index, replaced m->mx_dupchk_rcb with tp->tpcb_dupchk_rcb.
**      01-dec-2005 (stial01)
**          Increase number of bits in exch_visit so we can create > 32 indices
**          concurrently (b115572)
**	22-Feb-2008 (kschendel) SIR 122739
**	    Various changes for the new rowaccessor structure.
**	04-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_?, dmse_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1?b? functions converted to DB_ERROR *
**      21-may-2009 (huazh01)
**          Add "include <dm1h.h>". (b122075)
**	25-Aug-2009 (kschendel) 121804
**	    Need dmpbuild.h to satisfy gcc 4.3.
*/


/*{
** Name: dm2u_pload_table - This is the main routine. This is the parent
**	                    thread which creates children threads and manages
**	                    them.
**
** Description:
**	This function creates the threads and exchange buffer and then
**	reads the buffer and puts it the exchange buffer for reading.
** Inputs:
**	Index control blocks and number of index control blocks.
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**      11-jan-2000 (stial01)
**          Missed this file when updated dm2uuti.c for change 443920
**          We did not add tidp to key for ANY unique secondary indexes
**          No need to decrement sort_kcount anymore
**      05-may-2000 (stial01, gupsh01)
**          added check for the child thread getting ahead of parent thread.
**	25-Jan-2002 (jenjo02)
**	    Added cleanup_buffers() function so that mutexes get
**	    destroyed.
**	15-Apr-2003 (jenjo02)
**	    Added checks of rcb_uiptr interrupts after each get.
**	25-aug-2004 (thaju02)
**	    For online modify with persistent indices, if 
**	    MX_ONLINE_INDEX_BUILD, parent to wait for children 
**	    to setup mct block only, cleanup and return.  
**	27-Jul-2005 (toumi01) BUG 114933
**	    If we are projecting key columns into the exchange buffer
**	    don't init non-existent coupon fields and trash memory.
**	11-Nov-2005 (jenjo02)
**	    Check for external interrupts via XCB when available.
**      15-Jan-2007 (horda03) Bug 117471
**          If a child thread is waiting for its parent to populate a buffer
**          deschedule the child to give the parent a chance to do the work.
**          This is esential if using Internal threads.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Don't need to set no-coupon, done by access mode now.
*/
DB_STATUS
dm2u_pload_table(
DM2U_MXCB           *mxcbs,
i4  		    *rec_cnt,
DB_ERROR            *dberr)
{
    DM_SVCB		*svcb = dmf_svcb;
    DM2U_MXCB           *m = mxcbs;
    DM_TID              tid;
    DB_STATUS           status, local_status;
    CHILD_CB		*ccb;
    EXCH_BUF		*cur_buf;
    EXCH_BUF_HDR	*exch_hdr;
    char		*buf_p = NULL, *exch_buf;
    char 		*temp_p = NULL;
    i4		no_of_ind, no_of_buffers, no_of_records, size_of_buffer;
    i4		exch_tot_size;
    i4 		end_of_file = FALSE;
    i4		reclen;
    char		*dummy_record = NULL;
    i4			error;

    /* Check if we are projecting key columns into exchange buffer */
    if (m->mx_rcb->rcb_proj_misc)
    {
	reclen = m->mx_rcb->rcb_proj_relwid;
    }
    else
	reclen = m->mx_rcb->rcb_tcb_ptr->tcb_rel.relwid;

    /* Init number of records */
    *rec_cnt = 0;

    /* 
    ** count number of indexes 
    */
    no_of_ind = count_indexes(mxcbs);

    /* Validate number of indexes */
    if (no_of_ind > sizeof(cur_buf->exch_visit) * BITSPERBYTE)
    {
	TRdisplay("create_index_threads %d current max allowed %d\n",
		no_of_ind, sizeof(cur_buf->exch_visit) * BITSPERBYTE);
	SETDBERR(dberr, 0, E_DM0188_PINDEX_CRERR);
	return (E_DB_ERROR);
    }

    if (svcb) 
    {
	no_of_buffers = svcb->svcb_pind_nbuffers;
	no_of_records = svcb->svcb_pind_bsize; 
    }
    else
    {
	no_of_buffers = DM_DEFAULT_EXCH_NBUFF;
	no_of_records = DM_DEFAULT_EXCH_BSIZE;
    }
    /* 
    ** calculate the size of the buffers to allocate for parent-child
    ** communication and exchange buffers 
    */
    size_of_buffer = (sizeof(DM_TID) + reclen) * no_of_records;
    exch_tot_size = sizeof(EXCH_BUF_HDR) +
			no_of_buffers * (size_of_buffer + sizeof(EXCH_BUF)) +
			(sizeof(CHILD_CB) * no_of_ind);
 
    /* TRdisplay if we need more than 50 meg memory */
    if (sizeof(DMP_MISC) + exch_tot_size + reclen > 50 * 1024 * 1024)
	TRdisplay("PINDEX %~t reclen %d recs %d bufs %d ind %d mem %d \n", 
		sizeof(DB_TAB_NAME), 
		m->mx_rcb->rcb_tcb_ptr->tcb_rel.relid.db_tab_name,
		(sizeof(DM_TID) + reclen), no_of_records, no_of_buffers,
		no_of_ind, sizeof(DMP_MISC) + exch_tot_size + reclen);

    status = dm0m_allocate((i4)sizeof(DMP_MISC) + exch_tot_size + reclen,
                0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
                (char *)0, (DM_OBJECT **)&buf_p, dberr);
    if (status != E_DB_OK)
	return(status); 

    ccb = (CHILD_CB *)(buf_p + sizeof(DMP_MISC));
    ((DMP_MISC*)buf_p)->misc_data = (char*)ccb;
    MEfill(sizeof(CHILD_CB) * no_of_ind, '\0', ccb);

    exch_hdr = (EXCH_BUF_HDR *)((char *)ccb + 
					sizeof(CHILD_CB) * no_of_ind);
    MEfill(sizeof(EXCH_BUF_HDR), '\0', exch_hdr);

    exch_buf = (char *)exch_hdr + sizeof(EXCH_BUF_HDR);
    dummy_record = (char *)exch_buf + 
			no_of_buffers * (size_of_buffer + sizeof(EXCH_BUF));

#ifdef xDEBUG
    TRdisplay("PINDEX Mefill %d bytes instead of %d bytes \n",
	(sizeof(CHILD_CB) * no_of_ind) + 
	sizeof(EXCH_BUF_HDR) + 
	(no_of_buffers * sizeof(EXCH_BUF)),
	exch_tot_size + reclen);
#endif

    /* Initialize exchange buffers */
    status = setup_buffers(exch_hdr, exch_buf, no_of_ind, no_of_buffers,
				no_of_records, size_of_buffer);
    if (status != E_DB_OK)
    {
	cleanup_buffers(&buf_p, exch_hdr);
	return(E_DB_ERROR);
    }

    /* set the parent's current buffer to first buffer */
    cur_buf = exch_hdr->ehdr_exch_buffers;
    /* Hold the mutex to block the kids */
    CSp_semaphore(TRUE, &cur_buf->exch_buf_mutex);

    /* Create the factotum threads for parallel index creation */
    status = create_index_threads(mxcbs, ccb, exch_hdr, 
			(EXCH_BUF *)exch_buf, no_of_ind, dberr);
    if ( (status == E_DB_OK) && 
	 !(m->mx_flags & MX_ONLINE_INDEX_BUILD) )
    {
	/*
	**  Position for a full scan of table or index. 
	*/
	status = dm2r_position(m->mx_rcb, DM2R_ALL, (DM2R_KEY_DESC *)0,
			       (i4)0, (DM_TID *)0,
			       dberr);
    }

    if (status != E_DB_OK)
    {
	/* Flag parent - Error let the children go */
	exch_hdr->ehdr_error_flag = PARENT_ERROR;
	CSv_semaphore(&exch_hdr->ehdr_exch_buffers->exch_buf_mutex);
	wait_for_index_finish(exch_hdr);
	cleanup_buffers(&buf_p, exch_hdr);
	return (status);
    }

    while ( status == E_DB_OK && !end_of_file &&
		!(m->mx_flags & MX_ONLINE_INDEX_BUILD) )
    {
	/* get a record from the base table */
	local_status = dm2r_get(m->mx_rcb, &tid, DM2R_GETNEXT, 
				dummy_record, dberr);
	/* check to see if user interrupt occurred. */
	if ( local_status == E_DB_OK && *(m->mx_rcb->rcb_uiptr) )
	{
	    /* If XCB, check via SCB */
	    if ( m->mx_rcb->rcb_xcb_ptr )
	    {
		dmxCheckForInterrupt(m->mx_rcb->rcb_xcb_ptr, &error);
		if ( error )
		    SETDBERR(dberr, 0, error);
	    }
	    else if (*(m->mx_rcb->rcb_uiptr) & RCB_USER_INTR)
		SETDBERR(dberr, 0, E_DM0065_USER_INTR);
	    /* check to see if force abort occurred. */
	    else if (*(m->mx_rcb->rcb_uiptr) & RCB_FORCE_ABORT)
		SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);

	    if ( dberr->err_code )
		local_status = E_DB_ERROR;
	}

	if (local_status == E_DB_OK)
	    *rec_cnt = *rec_cnt + 1;

	/* put record in exchange buffer */
	status = put_record(exch_hdr, &cur_buf, &tid, dummy_record, 
				    reclen, no_of_ind, &end_of_file,
				    local_status, dberr);
    }

    /* 
    ** We have finished loading the data to the child threads now
    ** and child threads can go concurrent
    ** We have to wait for all the children to finish.
    ** If one of them finishes with error, we have to tell others
    ** to stop at the earliest opportunities and return error
    ** when everyone is done.
    */
    wait_for_index_finish(exch_hdr);
    
    /* If parent is ok, check status of kids */
    if ( status == E_DB_OK )
	status = check_child_status(ccb, no_of_ind, dberr);

    cleanup_buffers(&buf_p, exch_hdr);
    return(status);
}

/*{
** Name: create_index_threads
**
** Description:
**	 This routine creates factotum thread and executes the routine
**	 to get data from the parent thread.
**
* Inputs:
**	
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01) 
**          Created.  
*/
static STATUS
create_index_threads(
DM2U_MXCB	*mxcbs,
CHILD_CB	*ccb,
EXCH_BUF_HDR	*exch_hdr,
EXCH_BUF	*exch_buf,
i4		no_of_ind,
DB_ERROR	*dberr)
{
    SCF_CB      scf_cb;
    SCF_FTC     ftc;
    DM2U_MXCB	*mxcb;
    DB_STATUS   status = E_DB_OK;
    i4	ix;
    i4		error;

    /* Initialize the child cb's */
    MEfill(sizeof(CHILD_CB) * no_of_ind, 0, ccb);

    /* Create the control blocks and initialize it */

    /* For now assume there is no limit for such threads  */
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_facility = DB_DMF_ID;
    CSget_sid(&scf_cb.scf_session);
    scf_cb.scf_ptr_union.scf_ftc = &ftc;
 
    ftc.ftc_facilities = 0;
    ftc.ftc_priority = SCF_CURPRIORITY;
    ftc.ftc_thread_name = "<Parallel Index Clone>";
    ftc.ftc_thread_entry = build_pindex;
    ftc.ftc_thread_exit = build_pindex_exit;

    for (ix = 0,  mxcb = mxcbs;
         ix < no_of_ind && status == E_DB_OK;
         ix++, mxcb = (DM2U_MXCB *)mxcb->mx_next, ccb++)
    {
        ccb->ccb_noofchild = no_of_ind;
	ccb->ccb_exch_hdr = exch_hdr;
	ccb->ccb_mxcb = mxcb;
	ccb->ccb_childno = ix+1;
	ccb->ccb_status = 0;
	CLRDBERR(&ccb->ccb_error);
        ftc.ftc_data = (PTR)ccb;
	/* 
	** This was done intentionally so that factotum threads donot copy
	** the control block since control block structure is pointers 
	*/
        ftc.ftc_data_length = 0; 
        status = scf_call(SCS_ATFACTOT, &scf_cb);
        if (status != E_DB_OK)
        {
            uleFormat(&scf_cb.scf_error, 0, (CL_ERR_DESC *)NULL,
                           ULE_LOG, NULL, (char * )NULL,
                           (i4)0, (i4 *)NULL, &error, 0);
	    SETDBERR(dberr, 0, E_DM0188_PINDEX_CRERR);
        }
	else
	{
	    ccb->ccb_thread_id = ftc.ftc_thread_id;
	    /* Count another successfully attached offspring */
	    exch_hdr->ehdr_cwait_count++;
	}
    }
    return(status);
}
/*{
** Name: build_pindex
**
** Description:
**	This routine gets executed by each child creating index. This reads
**	the exchange buffer and passes it to sorter. At end, this routine
**	calls the load routine to build the index in parallel.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**	25-Jan-2002 (jenjo02)
**	    Must hold exch_buf_mutex while checking parent_status,
**	    and parent_status must be cleared before CSresume-ing
**	    parent to ensure that only one child resumes the
**	    parent.
**	    Set/return error status directly in child's CHILD_CB
**	    where it can be viewed by the parent.
**      15-Jan-2007 (horda03) Bug 117471
**          If a child thread is waiting for its parent to populate a buffer
**         (or other children to catch up deschedule the child to give them
**          a chance to do the work.
**          This is esential if using Internal threads.
*/
STATUS
build_pindex(SCF_FTX *ftx)
{
    CHILD_CB            *ccb;
    EXCH_BUF		*cur_buf;
    EXCH_BUF_HDR	*exch_hdr;
    i4			allread = FALSE, noofrecord, reclen;
    DM_OBJECT           *wloc_mask = NULL;
    DM2U_MXCB		*m;
    DM2U_TPCB		*tp;
    DM_TID		tid;
    i4			local_err_code;
    i4			error;
    DB_ERROR		local_dberr;

    ccb = (CHILD_CB *)ftx->ftx_data;
    exch_hdr = ccb->ccb_exch_hdr;
    m = ccb->ccb_mxcb;
    tp = m->mx_tpcb_next;

    /* round robin buffer for reading and that is why always start at 1 */
    cur_buf = exch_hdr->ehdr_exch_buffers;
    reclen = (exch_hdr->ehdr_size/
		(exch_hdr->ehdr_rec_capacity) - sizeof(DM_TID));

    if ( (ccb->ccb_status = init_child_thread(tp, &wloc_mask, &ccb->ccb_error)) )
	return(ccb->ccb_status);

    if (m->mx_flags & MX_ONLINE_INDEX_BUILD)
	return(ccb->ccb_status);

    /* 
    ** This routine waits for parents to send data in exchange buffer
    ** until nomore data
    */

    while ( ccb->ccb_status == E_DB_OK && !allread )
    {
#ifdef xDEBUG
	TRdisplay("child %d Going to wait on buffer %d\n", 
		ccb->ccb_childno, cur_buf->exch_bufno);
#endif
	/* Wait to get a buffer using the shared mutex */
	CSp_semaphore(FALSE, &cur_buf->exch_buf_mutex);
	/* check for errors before reading data */
	if (exch_hdr->ehdr_error_flag)
	{
	    ccb->ccb_status = E_DB_WARN;
	    /* if parent is waiting on this buffer .. wake him up */
            if (exch_hdr->ehdr_parent_status == cur_buf->exch_bufno)
	    {
		exch_hdr->ehdr_parent_status = 0;
	        CSresume(ftx->ftx_thread_id);
	    }
	    CSv_semaphore(&cur_buf->exch_buf_mutex);
	    break;
	}
#ifdef xDEBUG
	TRdisplay("child %d Got buffer mutex for buffer %d\n", 
			ccb->ccb_childno, cur_buf->exch_bufno);
#endif


	/* Mutex the buffer */
	CSp_semaphore(TRUE, &cur_buf->exch_cnt_mutex);

	if (!cur_buf->exch_usr_cnt)
	{
	    /* Child switched to this buffer before the parent filled it */
#ifdef xDEBUG
	    TRdisplay(" CHILD %d buffer %x %d has no users no_of_records %d\n",
		ccb->ccb_childno, cur_buf, cur_buf->exch_bufno, 
		cur_buf->exch_noofrec );
#endif
	    /* if parent is waiting on this buffer .. wake him up */
            if (exch_hdr->ehdr_parent_status == cur_buf->exch_bufno)
	    {
		exch_hdr->ehdr_parent_status = 0;
		CSresume(ftx->ftx_thread_id);
	    }
	    CSv_semaphore(&cur_buf->exch_cnt_mutex);
	    CSv_semaphore(&cur_buf->exch_buf_mutex);

            /* Let the parent have a chance to run */
            CSswitch();
	    continue;
	}

	/* Have we visited this buffer already */
	if (BTtest(ccb->ccb_childno, (char *)&cur_buf->exch_visit))
        {
#ifdef xDEBUG
        TRdisplay("child %d already visited this buffer %d\n", 
                        ccb->ccb_childno, cur_buf->exch_bufno);
#endif

	    /* Wait for other children to catch up */
	    CSv_semaphore(&cur_buf->exch_cnt_mutex);
	    CSv_semaphore(&cur_buf->exch_buf_mutex);

            /* Let the other children have a chance to run */
            CSswitch();
	    continue;
        }
	CSv_semaphore(&cur_buf->exch_cnt_mutex);

        if (cur_buf->exch_noofrec != exch_hdr->ehdr_rec_capacity)
	{
	    /* This is the last buffer to read */
	    allread = TRUE;
	} 
#ifdef xDEBUG
        TRdisplay("child %d reading buffer %d end %d\n", 
                        ccb->ccb_childno, cur_buf->exch_bufno, allread);
#endif
	if ( noofrecord = cur_buf->exch_noofrec )
	{
	    i4	nth;

	    while ( noofrecord > 0 && ccb->ccb_status == E_DB_OK )
	    {
		nth = cur_buf->exch_noofrec - noofrecord;
            	get_record(exch_hdr, cur_buf, 
					   tp, nth, reclen, &tid);
 	    	ccb->ccb_status = load_sorter(tp, &tid, &ccb->ccb_error);		
	    	noofrecord--;
	    }

	    CSp_semaphore(TRUE, &cur_buf->exch_cnt_mutex);
	    cur_buf->exch_usr_cnt--;
	    BTset(ccb->ccb_childno, (char *)&cur_buf->exch_visit);
            if (!cur_buf->exch_usr_cnt)
            {
		/* if parent is waiting on this buffer .. wake him up */
                if (exch_hdr->ehdr_parent_status == cur_buf->exch_bufno)
		{
		    exch_hdr->ehdr_parent_status = 0;
		    CSresume(ftx->ftx_thread_id);
		}
            }
	    CSv_semaphore(&cur_buf->exch_cnt_mutex);
	}
	CSv_semaphore(&cur_buf->exch_buf_mutex);
#ifdef xDEBUG
        TRdisplay("child %d finished reading this buffer %d\n", 
                        ccb->ccb_childno, cur_buf->exch_bufno);
#endif
	if ( ccb->ccb_status == E_DB_OK )
	{
	    if (allread)
	    {
		ccb->ccb_status = dmse_input_end(tp->tpcb_srt, &ccb->ccb_error);
	    }
	    else if (cur_buf->exch_bufno == exch_hdr->ehdr_noofbufs)
		/* reset back to 1 */
		cur_buf = exch_hdr->ehdr_exch_buffers;
	    else
		cur_buf++;
	}
    }
    if ( ccb->ccb_status == E_DB_OK )
        ccb->ccb_status = load_index(tp, wloc_mask, &ccb->ccb_error);
    else 
    {
	if ( dmse_end(tp->tpcb_srt, &local_dberr) )
        {
            uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
                    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    if (local_dberr.err_code > ccb->ccb_error.err_code)
	        ccb->ccb_error = local_dberr;
        }
	tp->tpcb_srt = 0;
    }
    if (wloc_mask != NULL)
	dm0m_deallocate((DM_OBJECT **)&wloc_mask);
    return(ccb->ccb_status);   
}

/*{
** Name: build_pindex_exit
**
** Description:
**	Whenever a create index thread exits, it calls this routine to 
**	decrement the count and wake up the parent.
** Inputs:
** Outputs:
**      Returns:
**	    Nothing because this is a void function.
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**	25-Jan-2002 (jenjo02)
**	    Must hold the cwait_mutex while the parent
**	    is CSresumed. It's only necessary to resume
**	    the parent when the last child exits, not
**	    when each child exits.
*/
VOID
build_pindex_exit(SCF_FTX *ftx)
{
    CHILD_CB *ccb = (CHILD_CB*)ftx->ftx_data;

    /* 
    ** Do the cleanup before exiting
    */
    CSp_semaphore(TRUE, &ccb->ccb_exch_hdr->ehdr_cwait_mutex);
    if (ccb->ccb_status && !ccb->ccb_exch_hdr->ehdr_error_flag)
	ccb->ccb_exch_hdr->ehdr_error_flag = CHILD_ERROR;
    /* Wake up parent if we're the last child */
    if ( --ccb->ccb_exch_hdr->ehdr_cwait_count == 0 )
	CSresume(ftx->ftx_thread_id);
    CSv_semaphore(&ccb->ccb_exch_hdr->ehdr_cwait_mutex);
#ifdef xDEBUG
    TRdisplay("Executing index exit routines\n");
#endif
    return;

}
/*{
** Name: check_child_status
**
** Description: Checks the status of children.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
*/
static STATUS
check_child_status(
CHILD_CB *ccb,
i4	 no_of_ind,
DB_ERROR *dberr)
{
    i4	ix;
    STATUS	status = E_DB_OK;

    CLRDBERR(dberr);

    for (ix = 0; ix < no_of_ind; ix++, ccb++)
    {
	if ( ccb->ccb_status )
	{
	    if (ccb->ccb_status == E_DB_WARN)
	    {
#ifdef xDEBUG
		TRdisplay("child %d exited with warning because he saw the error\n", ccb->ccb_childno);
#endif
		continue;
	    }
	    if (ccb->ccb_error.err_code > dberr->err_code)
		*dberr = ccb->ccb_error;
	    status = E_DB_ERROR;
	}
    }
    return(status);
}
/*{
** Name: wait_for_index_finish
**
** Description: This routine waits for all the children to finish.
**
** Inputs:
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**	24-Jan-2002 (jenjo02)
**	    Use EXCL (TRUE) p_sem instead of SHARED (FALSE).
**	    Add CScancelled() to clear any extraneous resume.
*/
static VOID
wait_for_index_finish(EXCH_BUF_HDR *exch_hdr)
{
  /*
  ** We may not get the mutex until all the child
  ** threads have ended and "cwait_count" is zero.
  ** If that happens (and it does on Linux), 
  ** we won't CSsuspend but will have been
  ** CSresume-d by the last child out.
  */
  CSp_semaphore(TRUE, &exch_hdr->ehdr_cwait_mutex);
  while (exch_hdr->ehdr_cwait_count)
  {
	CSv_semaphore(&exch_hdr->ehdr_cwait_mutex);
	CSsuspend(0,0,0);
	CSp_semaphore(TRUE, &exch_hdr->ehdr_cwait_mutex);
  }
  CSv_semaphore(&exch_hdr->ehdr_cwait_mutex);

  /* Cancel resume in case we didn't suspend */
  CScancelled((PTR)0);

  return;
}
/*{
** Name: get_record
**
** Description:
**	This routine reads nth record from the exchange buffer. This routine
**	does not make any attempt to check whether such a record exists.
**	It is the responsibility of the caller to check this.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
*/
static STATUS
get_record(
EXCH_BUF_HDR *exch_buf_hdr,
EXCH_BUF     *cur_buf,
DM2U_TPCB    *tp,
i4      nth,
i4	     reclen,
DM_TID	     *tid
)
{
	char 	*p;
	i4	i;

	p = cur_buf->exch_buffer + (nth * (reclen + sizeof(DM_TID)));
	MEcopy(p, sizeof(DM_TID), tid);
	p +=  sizeof(DM_TID);
	MEcopy(p, reclen, tp->tpcb_crecord);

#ifdef xDEBUG
	/* Now got it - so read it */
	TRdisplay("Child Record in buffer %d pos %d TID :",
		cur_buf->exch_bufno, nth);
	TRdisplay("%d\tRecord :", tid->tid_i4);
	for (i = 0; i < reclen; i++)
	{
	    TRdisplay("%1c", tp->tpcb_crecord[i]);
	}
	TRdisplay("\nAddresses 0x%x 0x%x\n", p-sizeof(DM_TID), p);
#endif
	return(E_DB_OK);
}

/*{
** Name: put_record
**
** Description:
**	This routine gets a record from the base table and puts it in the
**	current exchange buffer. If the current exchange buffer is full,
**	it gets the next exchage buffer by calling switch_buffer routine.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**      05-may-2000 (stial01, gupsh01)
**          If E_DM0055_NONEXT, and current buffer is full, the parent 
**          switches to the next buffer. Set exch_usr_cnt and exch_visit
**          in the next buffer.
*/
static STATUS
put_record(
EXCH_BUF_HDR *exch_buf_hdr,
EXCH_BUF **cur_buf_ptr,
DM_TID *tid,
char *record,
i4 reclen,
i4 noofchild,
i4	*end_of_file,
i4	local_status,
DB_ERROR	*dberr)
{
    char 	*start;
    i4	i;
    EXCH_BUF 	*cur_buf = *cur_buf_ptr;
    STATUS	status;    

    if (local_status == E_DB_OK)
    {
	if ((cur_buf->exch_noofrec + 1) > exch_buf_hdr->ehdr_rec_capacity)
	{
	    status = switch_buffer(exch_buf_hdr, cur_buf_ptr, noofchild);
	    if (status != E_DB_OK)
		return(E_DB_ERROR);
	    cur_buf = *cur_buf_ptr;
	}

	start = cur_buf->exch_buffer + 
		  (cur_buf->exch_noofrec * (reclen + sizeof(DM_TID)));
        MEcopy((char*)tid, sizeof(DM_TID), start);
        MEcopy(record, reclen, start+sizeof(DM_TID));

#ifdef xDEBUG
	TRdisplay("Parent Record in buffer %d pos %d TID: ",
		cur_buf->exch_bufno, 
		cur_buf->exch_noofrec);
	TRdisplay("%d\t Record:", tid->tid_i4);
	for (i = 0; i < reclen; i++)
	{
	    TRdisplay("%1c", record[i]);
	}
	TRdisplay("\n Addresses 0x%x 0x%x\n", start, start+sizeof(DM_TID));
#endif
	

        (cur_buf->exch_noofrec)++;
	return(E_DB_OK);
    }
    else {
	if (local_status == E_DB_ERROR && dberr->err_code == E_DM0055_NONEXT)
	{
	    /* if the current buffer is full, let children read it */
	    if ((cur_buf->exch_noofrec + 1) > exch_buf_hdr->ehdr_rec_capacity)
	    {
	  	status = switch_buffer(exch_buf_hdr, cur_buf_ptr, noofchild);
		if (status != E_DB_OK)
		    return(E_DB_ERROR);
	  	cur_buf = *cur_buf_ptr;
	    }

	    /*
	    ** Always set exch_usr_cnt and exch_visit when parent gets EOF
	    ** so that the child can detect if it switches to a buffer
	    ** before the parent.
	    */
	    cur_buf->exch_usr_cnt = noofchild;
	    MEfill(sizeof(cur_buf->exch_visit), '\0', &cur_buf->exch_visit);

	    CSv_semaphore(&cur_buf->exch_buf_mutex);
	    /* 
	    ** End of table - Tell children to start load phase
	    */
	    CLRDBERR(dberr);
	    *end_of_file = 1;
	    return(E_DB_OK);
        }
        else {
	    /* error occurred while reading - so bail out */

	    /* Tell children that parent got error */
	    exch_buf_hdr->ehdr_error_flag = PARENT_ERROR;
	    CSv_semaphore(&cur_buf->exch_buf_mutex);

	    return(E_DB_ERROR);
        }
    }
}

/*{
** Name: switch_buffer
**
** Description:
**	This routine checks if the caller has an error. If it doesnot
**	it gets the next available buffer when available.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**	25-Jan-2002 (jenjo02)
**	    Reorganized code to ensure that "parent_status" is
**	    set and reset while holding exch_buf_mutex.
*/
static STATUS
switch_buffer(EXCH_BUF_HDR *exch_buf_hdr, EXCH_BUF **cur_buf_ptr, 
		i4 noofchild)
{
    EXCH_BUF *next_buf;
    EXCH_BUF *cur_buf = *cur_buf_ptr;

    /* done with this buffer */
    cur_buf->exch_usr_cnt = noofchild;
    MEfill(sizeof(cur_buf->exch_visit), '\0', &cur_buf->exch_visit);

    /* Is it last buffer */
    if (cur_buf->exch_bufno == exch_buf_hdr->ehdr_noofbufs)
    {
	/* set the next buffer to 1 */
	next_buf = exch_buf_hdr->ehdr_exch_buffers;
    } 
    else 
    {
	next_buf = cur_buf;
	next_buf++;
    }
    CSv_semaphore(&cur_buf->exch_buf_mutex);

    CSp_semaphore(TRUE, &next_buf->exch_buf_mutex);

    while ( exch_buf_hdr->ehdr_error_flag ||
	    next_buf->exch_usr_cnt > 0 )
    {
	/* 
	** check the error code here for children & if they wake 
	** you up for error
	*/
	if (exch_buf_hdr->ehdr_error_flag)
	{
#ifdef xDEBUG
	    TRdisplay("Parent saw the error and going to exit routines for children to finish\n");
#endif
	    /* we have now seen child raised error ... hence exit */
    	    CSv_semaphore(&next_buf->exch_buf_mutex);
	    return(E_DB_ERROR);
	}
	
	/* Tell children we're waiting on this buffer */
	exch_buf_hdr->ehdr_parent_status = next_buf->exch_bufno;
	CSv_semaphore(&next_buf->exch_buf_mutex);
	CSsuspend(0,0,0);
	/* CSresume-er will clear ehdr_parent_status */
	CSp_semaphore(TRUE, &next_buf->exch_buf_mutex);

        /* Cancel any extra resumes from those pesky kids */
        CScancelled((PTR)0);
    }
    
    next_buf->exch_noofrec = 0;
    MEfill(sizeof(next_buf->exch_visit), '\0', &next_buf->exch_visit);
    *cur_buf_ptr = next_buf; 

    return(E_DB_OK);
}

/*{
** Name: setup_buffers
**
** Description:
**	This routine sets up the exchange buffer and properly initializes it.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
*/
static STATUS
setup_buffers(
EXCH_BUF_HDR *exch_hdr, 
char *exch_buf, 
i4 no_of_ind,
i4 no_of_buffers,
i4 no_of_records, 
i4 size_of_buffer)
{
    EXCH_BUF	*cur_buffer;
    char 	*p;
    i4 	ix, err_code;
    DB_STATUS 	status;

    exch_hdr->ehdr_exch_buffers = (EXCH_BUF *)exch_buf;
    exch_hdr->ehdr_noofbufs = no_of_buffers;
    exch_hdr->ehdr_rec_capacity = no_of_records;
    exch_hdr->ehdr_size = size_of_buffer;

    if (status = CSw_semaphore(&exch_hdr->ehdr_cwait_mutex,
                                CS_SEM_SINGLE,"cwait_mutex"))
    {
	uleFormat(NULL, E_DM9303_DM0P_MUTEX, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		   (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	return (E_DB_FATAL);
    }
    exch_hdr->ehdr_error_flag = 0;

    /* calculate the buffer address */
    p = exch_buf + (exch_hdr->ehdr_noofbufs * sizeof(EXCH_BUF));

    /* Init all EXCH_BUFs */
    MEfill((exch_hdr->ehdr_noofbufs * sizeof(EXCH_BUF)), '\0', (PTR)exch_buf);

    for (ix = 0; ix < exch_hdr->ehdr_noofbufs; ix++)
    {
	cur_buffer = (EXCH_BUF *) exch_buf;
	if (status = CSw_semaphore(&cur_buffer->exch_buf_mutex, CS_SEM_SINGLE,
				"per exch buf mutex "))
	{
	    uleFormat(NULL, E_DM9303_DM0P_MUTEX, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		   (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	    return (E_DB_FATAL);
	}
	cur_buffer->exch_bufno = ix + 1;
	cur_buffer->exch_noofrec = 0;
	cur_buffer->exch_usr_cnt = 0;
	cur_buffer->exch_buffer = p;
	MEfill(sizeof(cur_buffer->exch_visit), '\0', &cur_buffer->exch_visit);
	if (status = CSw_semaphore(&cur_buffer->exch_cnt_mutex, CS_SEM_SINGLE,
				"count mutex "))
	{
	    uleFormat(NULL, E_DM9303_DM0P_MUTEX, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		   (char *)NULL, (i4)0, (i4 *)NULL, &err_code, 0);
	    return (E_DB_FATAL);
	}
#ifdef xDEBUG
	TRdisplay("Exchange buff %d starts at 0x%x ends at 0x%x\n",
		ix + 1, p, p + size_of_buffer);
#endif

	p += size_of_buffer;
	exch_buf += sizeof(EXCH_BUF);
    }
 
    return(status);
}

/*{
** Name: cleanup_buffers
**
** Description:
**	At the end of the process, destroy all semaphores
**	and release the memory chunk.
**
** Inputs:
** Outputs:
**      Returns:
**          void
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	24-Jan-2002 (jenjo02)
**	    Created.
**	06-Feb-2002 (toumi01)
**	    Change terminating condition for loop to be simple buf counter.
*/
static VOID
cleanup_buffers(
char	     **buf_p,
EXCH_BUF_HDR *exch_hdr)
{
    EXCH_BUF	*exch_buf;
    i4		ix = exch_hdr->ehdr_noofbufs;

    CSr_semaphore(&exch_hdr->ehdr_cwait_mutex);

    for (exch_buf = exch_hdr->ehdr_exch_buffers;
	 ix--;
	 exch_buf++)
    {
	CSr_semaphore(&exch_buf->exch_buf_mutex);
	CSr_semaphore(&exch_buf->exch_cnt_mutex);
    }

    dm0m_deallocate((DM_OBJECT**)buf_p);

    return;
}

/*{
** Name: count_indexes
**
** Description: This routine returns the number of control blocks that
**	have been chained together. Effectively the number of control
**	blocks equal to number of indexes to be created.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
*/
static i4
count_indexes(
DM2U_MXCB	*mxcbs)
{
    DM2U_MXCB	*mxcb;
    i4  	i;

    for (i = 1,  mxcb = mxcbs;
         mxcb->mx_next;
         i++, mxcb = (DM2U_MXCB *)mxcb->mx_next)
       ;
    return(i);
}

/*{
** Name: load_sorter
**
** Description:
**	This routine relaizes the cols, appends the tid and then put it
** 	in sorter.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**      7-jan-98 (stial01)
**          Removed buffer parameter from dm1h_newhash.
**	21-Jan-2004 (jenjo02)
**	    Add TID's partition number, format TID8 when loading
**	    a Global Secondary Index.
**      21-may-2009 (huazh01)
**          dm1h_hash(), dm1h_newhash(), dm1h_keyhash() now includes
**          DMP_RCB and err_code in the parameter list. (b122075)
*/
static STATUS
load_sorter(
DM2U_TPCB  *tp, 
DM_TID     *tid,
DB_ERROR   *dberr)
{
    DM2U_MXCB		*m = tp->tpcb_mxcb;
    DM2U_M_CONTEXT      *mct = &tp->tpcb_mct;
    ADF_CB              *adf_cb = m->mx_rcb->rcb_adf_cb;
    DB_STATUS           status;
    i4		i, bucket;
    DB_DATA_VALUE 	obj;
    DB_DATA_VALUE 	range;
    DB_DATA_VALUE 	nbr;
    DB_DATA_VALUE 	hilbert;
    char	        *tup_ptr;
    DM_TID8		tid8;
    i4			error;


    /* Where we'll construct the index record */
    tup_ptr = tp->tpcb_irecord;

    /* The input row sits in tp->tpcb_crecord */

    if ( mct->mct_tidsize == sizeof(DM_TID8) )
    {
	/* Format TID8 with partition number, TID */
	tid8.tid_i4.tpf = 0;
	tid8.tid.tid_partno = mct->mct_partno;
	tid8.tid_i4.tid = tid->tid_i4;
    }

    if (m->mx_structure == TCB_RTREE)
    {
	/*  Copy the TIDP */
	tup_ptr += nbr.db_length + m->mx_hilbertsize;

	if ( mct->mct_tidsize == sizeof(DM_TID8) )
	    MEcopy((char*)&tid8, sizeof(DM_TID8), tup_ptr);
	else
	    MEcopy((char*)tid, sizeof(DM_TID), tup_ptr);

	/* Create the NBR */
	obj.db_data = &tp->tpcb_crecord[m->mx_b_key_atts[0]->offset];
	status = (*m->mx_acc_klv->dmpp_nbr)(adf_cb, &obj, &range, &nbr);
	if (status != E_DB_OK)
	    return(E_DB_ERROR);

	/* Create the HILBERT */
	tup_ptr = tp->tpcb_irecord;
	status = (*m->mx_acc_klv->dmpp_hilbert)(adf_cb, &nbr, &hilbert);
	if (status != E_DB_OK)
	    return(E_DB_ERROR);
    }
    else
    {
        for (i = 0; i < m->mx_ab_count; i++)
        {
	    MEcopy(&tp->tpcb_crecord[m->mx_b_key_atts[i]->offset],
	          m->mx_b_key_atts[i]->length, tup_ptr);
	    tup_ptr += m->mx_b_key_atts[i]->length;
        }

        /*
        ** Put the tidp at the end of the tuple.
        */
	if ( mct->mct_tidsize == sizeof(DM_TID8) )
	    MEcopy((char*)&tid8, sizeof(DM_TID8), tup_ptr);
	else
	    MEcopy((char*)tid, sizeof(DM_TID), tup_ptr);
    }

    /*
    ** If HASH index, put hash bucket at the end of the index
    ** row so it can be used in the sorter.
    */
    if (m->mx_structure == TCB_HASH)
    {
	if (dm1h_newhash(m->mx_rcb, m->mx_i_key_atts, m->mx_ai_count,
	       tp->tpcb_irecord, mct->mct_buckets, &bucket, dberr)
             != E_DB_OK)
            return (E_DB_ERROR); 

	I4ASSIGN_MACRO(bucket, tp->tpcb_irecord[m->mx_width]);
    }

    status = dmse_put_record(tp->tpcb_srt, tp->tpcb_irecord, dberr);
    return(status);
}

/*{
** Name: load_index
**
** Description: This routine actually creates the index. It gets record from
**	the sorter and builds the index.
**
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**      13-feb-03 (chash01) x-integrate change#461908
**          Add code to set and check duplicate for TCB_RTREE.  Compiler
**          complains about uninitialized variable "dup".
**	17-Mar-2004 (jenjo02)
**	    Keep new tuple counts in tpcb_newtup_cnt rather
**	    than mx_newtup_cnt; that's where dm2u_update_catalogs
**	    expects to find them.
**	30-apr-2004 (thaju02)
**	    Online modify. Change parameters to dm2u_put_dupchktbl.
**	30-apr-2004 (thaju02)
**	    Removed dupchk_rcb. 
**      18-dec-2004 (thaju02)
**          Replaced m->mx_dupchk_rcb with tp->tpcb_dupchk_rcb.
**	11-Nov-2005 (jenjo02)
**	    Check for external interrupts via XCB when available.
*/
STATUS
load_index(
DM2U_TPCB  *tp, 
DM_OBJECT  *wloc_mask,
DB_ERROR    *dberr)
{
    DM2U_MXCB		*m = tp->tpcb_mxcb;
    DM2U_M_CONTEXT      *mct = &tp->tpcb_mct;
    DB_STATUS		status, local_status;
    i4		local_err_code;
    DM_TID		tid;
    i4		dup, bucket;
    bool		online = (m->mx_flags & MX_ONLINE_MODIFY ? TRUE : FALSE);
    i4			error;
    DB_ERROR		local_dberr;

    /*
    **	Now start the load phase.  Open the output file.  Prepare to
    **  read the sorted record stream.  Modify to HEAP without keys
    **  will read the base table directly.
    */

    for (;;)
    {
	switch (m->mx_structure)
	{
	case TCB_HEAP:
	    /*  Prepare to load a HEAP table. */
	    status = dm1sbbegin(mct, dberr);
	    if (status != E_DB_OK)
		break;

	    for (;;)
	    {
		/*  Get record from sorter or table. */
		if (tp->tpcb_srt)
		    status = dmse_get_record(tp->tpcb_srt,
			    &tp->tpcb_irecord, dberr);
		else
		{
		    status = dm2r_get(m->mx_rcb, &tid, DM2R_GETNEXT,
			    tp->tpcb_irecord, dberr);
		    /* check to see if user interrupt occurred. */
		    if ( status == E_DB_OK && *(m->mx_rcb->rcb_uiptr) )
		    {
			/* If XCB, check via SCB */
			if ( m->mx_rcb->rcb_xcb_ptr )
			{
			    dmxCheckForInterrupt(m->mx_rcb->rcb_xcb_ptr, &error);
			    if ( error )
				SETDBERR(dberr, 0, error);
			}
			else if (*(m->mx_rcb->rcb_uiptr) & RCB_USER_INTR)
			    SETDBERR(dberr, 0, E_DM0065_USER_INTR);
			/* check to see if force abort occurred. */
			else if (*(m->mx_rcb->rcb_uiptr) & RCB_FORCE_ABORT)
			    SETDBERR(dberr, 0, E_DM010C_TRAN_ABORTED);

			if ( dberr->err_code )
			    status = E_DB_ERROR;
		    }
		}
		if (status != E_DB_OK)
		    break;

		/*	Add to the heap table. */
		status = dm1sbput(mct, tp->tpcb_irecord, m->mx_width, 
				  (i4)0, dberr);
		if (status != E_DB_OK)
		    break;
		tp->tpcb_newtup_cnt++;
	    }
	    if (status == E_DB_ERROR && dberr->err_code != E_DM0055_NONEXT)
	        break;

	    /*  Finish loading a HEAP table. */
	    status = dm1sbend(mct, dberr);
	    break;

	case TCB_ISAM:
	    /*  Prepare to load a ISAM table. */
	    status = dm1ibbegin(mct, dberr);
	    if (status != E_DB_OK)
		break;

	    for (;;)
	    {
		/*	Get record from the sorter. */
		status = dmse_get_record(tp->tpcb_srt,
		    &tp->tpcb_irecord, dberr);
		if (status == E_DB_ERROR)
		    break;
		dup = FALSE;
		if (status == E_DB_INFO)
		{
		    dup = TRUE;
		    if (mct->mct_unique)
		    {
			if (online)
			{
			    /* For online modify, if sorter encounters
			    ** duplicate keys, save key value to verify
			    ** that duplicates do not exist after sort &
			    ** sidefile processing.
			    */
			    status = dm2u_put_dupchktbl(tp, tp->tpcb_irecord,
							dberr);
			    if (status)
				break;
			}
			else
			{
			    SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
			    status = E_DB_ERROR;
			    break;
			}
		    }
		}

		/*	Add to the ISAM table. */
		status = dm1ibput(mct, tp->tpcb_irecord, m->mx_width,
		    dup, dberr);
		if (status != E_DB_OK)
		    break;
		tp->tpcb_newtup_cnt++;
	    }
	    if (status == E_DB_ERROR && dberr->err_code != E_DM0055_NONEXT)
		break;

	    /*  Finished loading ISAM table. */
	    status = dm1ibend(mct, dberr);
	    break;

	case TCB_HASH:
	    status = dm1hbbegin(mct, dberr);
	    if (status != E_DB_OK)
		break;

	    for (;;)
	    {
		/*	Get record from the sorter. */
		status = dmse_get_record(tp->tpcb_srt,
                                &tp->tpcb_irecord, dberr);
		if (status == E_DB_ERROR)
		    break;
		if (status == E_DB_INFO)
		{
		    if (mct->mct_unique)
		    {
			if (online)
			{
			    /* For online modify, if sorter encounters
			    ** duplicate keys, save key value to verify
			    ** duplicates do not exist after sort &
			    ** sidefile processing.
			    */
			    status = dm2u_put_dupchktbl(tp, tp->tpcb_irecord,
							dberr);
			    if (status)
				break;
			}
			else
			{
			    SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
			    status = E_DB_ERROR;
			    break;
			}
		    }
		}

		/*	Add to the HASH table. */
		I4ASSIGN_MACRO(tp->tpcb_irecord[m->mx_width], bucket);
		status = dm1hbput(mct, bucket,
		    tp->tpcb_irecord, m->mx_width, (i4)0, dberr);
		if (status != E_DB_OK)
		    break;
		tp->tpcb_newtup_cnt++;
	    }
	    if (status == E_DB_ERROR && dberr->err_code != E_DM0055_NONEXT)
		break;

	    /*  Finished loading HASH table. */
	    status = dm1hbend(mct, dberr);
	    break;

	case TCB_BTREE:
	    status = dm1bbbegin(mct, dberr);
	    if (status != E_DB_OK)
		break;

	    for (;;)
	    {
		/*	Get record from the sorter. */
		status = dmse_get_record(tp->tpcb_srt,
                             &tp->tpcb_irecord, dberr);
		if (status == E_DB_ERROR)
		    break;
		dup = FALSE;
		if (status == E_DB_INFO)
		{
		    dup = TRUE;
		    if (mct->mct_unique)
		    {
			if (online)
			{
			    /* For online modify, if sorter encounters
			    ** duplicate keys, save key value to verify
			    ** that duplicates do not exist after sort &
			    ** sidefile processing.
			    */
			    status = dm2u_put_dupchktbl(tp, tp->tpcb_irecord,
							dberr);
			    if (status)
				break;
			}
			else
			{
			    SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
			    status = E_DB_ERROR;
			    break;
			}
		    }
		}

		/*	Add to the BTREE table. */
		status = dm1bbput(mct, tp->tpcb_irecord,
                             m->mx_width, dup, dberr);
		if (status != E_DB_OK)
		    break;
		tp->tpcb_newtup_cnt++;
	    }
	    if (status == E_DB_ERROR && dberr->err_code != E_DM0055_NONEXT)
		break;

	    /*  Finished loading BTREE table. */
	    status = dm1bbend(mct, dberr);
	    break;

	case TCB_RTREE:
	    status = dm1mbbegin(mct, dberr);
	    if (status != E_DB_OK)
		break;

	    for (;;)
	    {
		/*  Get record from the sorter. */
		status = dmse_get_record(tp->tpcb_srt,
                                  &tp->tpcb_irecord, dberr);
		if (status == E_DB_ERROR)
		  break;

		dup = FALSE;
		if (status == E_DB_INFO)
		{
		    dup = TRUE;
		    if (mct->mct_unique)
		    {
			if (online)
			{
			    /* For online modify, if sorter encounters
			    ** duplicate keys, save key value to verify
			    ** that duplicates do not exist after sort &
			    ** sidefile processing.
			    */
			    status = dm2u_put_dupchktbl(tp, tp->tpcb_irecord,
							dberr);
			    if (status)
				break;
			}
			else
			{
			    SETDBERR(dberr, 0, E_DM0045_DUPLICATE_KEY);
			    status = E_DB_ERROR;
			    break;
			}
		    }
		}
		/*  Add to the RTREE table. */
		status = dm1mbput(mct, tp->tpcb_irecord,
                             m->mx_width, dup, dberr);
		if (status != E_DB_OK)
		    break;
		tp->tpcb_newtup_cnt++;
	    }

	    if (status == E_DB_ERROR && dberr->err_code != E_DM0055_NONEXT)
		break;

	    /*  Finished loading RTREE table. */
	    status = dm1mbend(mct, dberr);

	    break;
	}

	break;
    }

    /*
    **	Now perform the cleanup and error recovery phase.
    */

    for (;;)
    {
	if (tp->tpcb_dupchk_rcb)
	{
	    DB_STATUS   local_status = E_DB_OK;
	    i4          local_err = 0;

	    local_status = dm2t_close(tp->tpcb_dupchk_rcb, DM2T_PURGE, 
			&local_dberr);
	    if (local_status)
	    {
		/* FIX ME - report something? */
	    }
	    tp->tpcb_dupchk_rcb = (DMP_RCB *)NULL;
	}

	if (status != E_DB_OK)
	    break;

	if (tp->tpcb_srt)
	{
	    status = dmse_end(tp->tpcb_srt, dberr);
	    tp->tpcb_srt = 0;
	    if (status != E_DB_OK)
		break;
	}

	if (wloc_mask != NULL)
	    dm0m_deallocate((DM_OBJECT **)&wloc_mask);

	return (E_DB_OK);
    }

    /*	Error recovery and cleanup. */

    if (tp->tpcb_srt)
    {
	local_status = dmse_end(tp->tpcb_srt, &local_dberr);
	if (local_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    if (local_dberr.err_code > dberr->err_code)
	        *dberr = local_dberr;
	}
	tp->tpcb_srt = 0;
    }

    if (wloc_mask != NULL)
	dm0m_deallocate((DM_OBJECT **)&wloc_mask);

    if (dberr->err_code > E_DM_INTERNAL)
    {
	uleFormat(dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
	    (i4)0, (i4 *)NULL, &error, 0);
	SETDBERR(dberr, 0, E_DM9100_LOAD_TABLE_ERROR);
    }
    return (E_DB_ERROR);
}

/*{
** Name: init_child_thread
**
** Description:
**	This routine initializes each child thread for building secondary 
**	indexes. It sets up the control block and then sets up the sort.
** Inputs:
** Outputs:
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      10-apr-98 (nanpr01)
**          Created.
**	04-Apr-2003 (jenjo02)
**	    Added table_id to dmse_begin() prototype.
**	12-Aug-2004 (schka24)
**	    mct usage cleanups.
**	16-dec-04 (inkdo01)
**	    Add various db_collID's.
**	18-Apr-2004 (jenjo02)
**	    Add "cmp_count" to dmse_begin prototype.
**	04-Aug-2006 (kiria01) Bug 112407
**	    Make sure that on error from dmse_begin we don't
**	    allow code to skip execution of dmse_end. This should
**	    then allow sort workfiles to be closed if an error
**	    occurs such as out of disk space.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
**	25-Jun-2007 (kschendel) b118561
**	    Point work-loc mask at allocated memory, not caller's pointer.
**	24-Feb-2008 (kschendel) SIR 122739
**	    Extract all the mct initializing stuff to dm2uuti.
*/
static STATUS
init_child_thread(
DM2U_TPCB  *tp, 
DM_OBJECT  **wloc_mask,
DB_ERROR   *dberr)
{
    DM2U_MXCB		*m = tp->tpcb_mxcb;
    DM2U_M_CONTEXT      *mct = &tp->tpcb_mct;
    DB_STATUS           status, local_status;
    i4             local_err_code;
    i4             *wrk_loc_msk;
    i4             wrk_loc_size, i, bucket;
    i4			error;
    DB_ERROR		local_dberr;
 
    /* Initialize the MCT based on the TPCB and MXCB instructions */
    status = dm2uInitTpcbMct(tp);
    if (status != E_DB_OK)
	return (status);

    *wloc_mask = NULL;

    if (m->mx_flags & MX_ONLINE_INDEX_BUILD)
	return(E_DB_OK);

    /*  If keys are specified then sort the records so
    **  that they load faster. 
    */
    if (mct->mct_keys)
    {
	DMP_TCB	    *tcb = m->mx_rcb->rcb_tcb_ptr;
	i4	    sort_flag;
	i4	    sort_width;
	i4	    sort_kcount = mct->mct_keys;

	/*
	**  Indexes and unique keys don't have duplicate records eliminated
	**  because none exist in the case of an index, and unique keyed
	**  primary table get errors for non-unique keys.
	*/
	sort_flag = DMSE_ELIMINATE_DUPS;
	if (m->mx_duplicates ||
	    (tcb->tcb_rel.relstat & TCB_INDEX) ||
	    mct->mct_unique)
	{
    		sort_flag = 0;
	}

	sort_width = m->mx_width;
	switch (m->mx_structure)
	{
	       case TCB_HASH:
		   sort_width += sizeof(i4);
		   sort_kcount++;
		   if (m->mx_unique)
		       sort_flag |= DMSE_CHECK_KEYS;
		   break;

	       case TCB_ISAM:
	       case TCB_BTREE:

		   if (! mct->mct_index || m->mx_unique)
		       sort_flag |= DMSE_CHECK_KEYS;
		   break;
	}

	/*  Prepare the sorter to accept the records. */

	/*  First we determine the set of work locations which will be
	**  used to sort; if this is a dbms session then the location
	**	masks in the scb will determine which work locations the user
	**	will use.  If we are doing recovery, however, we must choose
	**	what set of work locations will be used; the current decision
	**	is to use all default work locations listed in the dcb.
	*/
	if (m->mx_xcb  &&  m->mx_xcb->xcb_scb_ptr)
	{
	    wrk_loc_msk = m->mx_xcb->xcb_scb_ptr->scb_loc_masks->locm_w_use;
	    wrk_loc_size = m->mx_xcb->xcb_scb_ptr->scb_loc_masks->locm_bits;
	}
	else
	{
	    /* allocate and fill a temporary work location bit mask; the
	    ** sorter will expect this object to live until the sort is
	    ** terminated
	    */
	    DMP_EXT		*ext;
	    i4		size;

	    ext = m->mx_dcb->dcb_ext;
	    size = sizeof(i4) *
			(ext->ext_count+BITS_IN(i4)-1)/BITS_IN(i4);

	    status = dm0m_allocate((i4)sizeof(DMP_MISC) + size,
	        DM0M_ZERO, (i4)MISC_CB, (i4)MISC_ASCII_ID,
	        (char *)0, wloc_mask, dberr);

	    if (status != E_DB_OK)
	        return(status);

	    wrk_loc_size = size * BITSPERBYTE;
	    wrk_loc_msk = (i4 *)((char *)(*wloc_mask) + sizeof(DMP_MISC));
	    (*(DMP_MISC**)wloc_mask)->misc_data = (char*)wrk_loc_msk;

	    /* initialize location usage bit masks in scb */
	    for (i=0; i < ext->ext_count; i++)
	    {
	        if (ext->ext_entry[i].flags & DCB_WORK)
	    	    BTset(i, (char *)wrk_loc_msk);
	    }
	}
	status = dmse_begin(sort_flag, 
			&m->mx_table_id,
			m->mx_cmp_list,
        	        m->mx_c_count, sort_kcount, m->mx_c_count,
			m->mx_dcb->dcb_ext->ext_entry, wrk_loc_msk, 
			wrk_loc_size, &m->mx_dcb->dcb_wrkspill_idx, 
			sort_width, 
			(i4)tcb->tcb_rel.reltups,
			m->mx_rcb->rcb_uiptr, m->mx_lk_id, m->mx_log_id,
			&tp->tpcb_srt, m->mx_rcb->rcb_collation, 
			m->mx_rcb->rcb_ucollation, dberr);

	/*
	**  Different handling for the following cases:
	**	Modify to anthing but hash
	**	Modify to hash
	**	Index or Index hash
	*/
	if (status == E_DB_OK)
	{
	    DB_DATA_VALUE obj;
	    DB_DATA_VALUE range;
	    DB_DATA_VALUE nbr;
	    DB_DATA_VALUE hilbert;

            if (m->mx_structure == TCB_RTREE)
	    {
	        /*
	        ** Fill in the constant DATA_VALUE fields
	        **
	        ** Note that the order is:
	        ** NBR | HILBERT | TIDP 
	        */
	        /* obj.db_data is variable */
	        obj.db_length = m->mx_b_key_atts[0]->length;
	        obj.db_datatype = abs(m->mx_b_key_atts[0]->type);
	        obj.db_prec = 0;
	        obj.db_collID = -1;
	        range.db_data = (char *)m->mx_range;
	        if (m->mx_acc_klv->range_type == 'F')
	          range.db_length = m->mx_dimension * sizeof(f8) * 2;
	        else
	          range.db_length = m->mx_dimension * sizeof(i4) * 2;
	        range.db_datatype = m->mx_acc_klv->box_dt_id;
	        range.db_prec = 0;
	        range.db_collID = -1;
	        nbr.db_data = (char *)tp->tpcb_irecord;
	        nbr.db_length = m->mx_hilbertsize * 2;
	        nbr.db_datatype = m->mx_acc_klv->nbr_dt_id;
	        nbr.db_prec = 0;
	        nbr.db_collID = -1;
	        hilbert.db_data = (char *)tp->tpcb_irecord + nbr.db_length;
	        hilbert.db_length = m->mx_hilbertsize;
	        hilbert.db_datatype = DB_BYTE_TYPE;
	        hilbert.db_prec = 0;
	        hilbert.db_collID = -1;
	    }
	}
    }
    if (status != E_DB_OK)
    {
	    local_status = dmse_end(tp->tpcb_srt, &local_dberr);
	    if (local_status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err_code, 0);
	    }
	    tp->tpcb_srt = 0;
	    if (*wloc_mask != NULL)
		dm0m_deallocate((DM_OBJECT **)wloc_mask);
	    return (status);
    }
    return(status);
}
