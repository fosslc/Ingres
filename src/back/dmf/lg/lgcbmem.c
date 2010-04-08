/*
**Copyright (c) 2004 Ingres Corporation
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>
#include    <dm.h>

/**
**
**  Name: LGCBMEM.C - Implements the control block allocation & free operations
**
**  Description:
**	This module contains the code which implements the allocation and free
**	operations for the various control blocks which LG manages.
**	
**	    LG_allocate_cb
**	    LG_deallocate_cb
**	    expand_list
**	    LG_allocate_lbb
**	    LG_allocate_lxb
**	    LG_dealloc_lfb_buffers
**	    LG_assign_tran_id
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	21-oct-1992 (bryanp)
**	    Log error messages when resources are exhausted.
**	18-jan-1993 (jnash)
**	    Reduced logging project.  Check for write beyond begin of file. 
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:
**	    Removed LG_allocate_ldb_by_id routine.  Recovery no longer
**		requires a database to be added back to the logging system
**		with the same database id.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (andys)
**	    Cluster 6.5 Project I
**	    => Renamed stucture members of LG_LA to new names. This means
**	       replacing lga_high with la_sequence, and lga_low with la_offset.
**	    => Large blocks are now allocated in terms of LFBs because
**	       they are now larger than LDBs
**	24-may-1993 (bryanp)
**	    Initialize lbh_address in LG_allocate_lbb.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Added LG_dealloc_lfb_buffers for use with non-local (cluster) LFB's.
**	18-oct-1993 (rogerk)
**	    Changed allocate_lbb to always call LG_check_logfull.
**	    Changed allocate_lbb to panic rather than returning a null lbb
**	    when the EOF wraps around the EOF.
**	15-apr-1994 (chiku)
**	    Bug56702: changed definition of LG_allocate_lbb() to
**	    return E_DM9070_LOG_LOGFULL on logfull.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	    Centralized accounting of lgd_???_inuse fields in this code
**	    instead of being responsibility of caller.
**	29-May-1996 (jenjo02)
**	    Added lxb parm to LG_allocate_LBB(), used solely to pass
**	    on up to LG_check_logfull(), thence to LG_abort_oldest().
**	08-Oct-1996 (jenjo02)
**	    Prefixed semaphore names with "LG" to make them more identifiable.
**	19-Oct-1996 (jenjo02)
**	    In LG_allocate_lbb(), don't return LOGFULL if allocating a log
**	    buffer for a record span operation. We can't give up on the
**	    the completion of a partially written log record.
**      08-Dec-1997 (hanch04)
**        Calculate number of blocks from new block field in
**        LG_LA for support of logs > 2 gig
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    LXBs are now allocated by their own function, LG_allocate_lxb(),
**	    which at the same time assigns the transaction id.
**	    Added "LBB **lbbo" parm to LG_allocate_lbb into which a pointer
**	    to the allocated LBB is returned.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED XA transactions.
**	03-Mar-2000 (jenjo02)
**	    BUG 98473: Release current_lbb_mutex before calling
**	    LG_check_logfull() in LG_allocate_lbb() to prevent
**	    deadlocks with LXB mutexes in LG_abort_oldest().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Assign UNIQUE low tran (Variable Page Type SIR 102955)
**	08-apr-2002 (devjo01)
**	    Use 'lfb_first_lbb' to find beginning of contiguous allocation
**	    containing all the LBB and associated log buffers for an LFB.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      01-Sep-2005 (horda03) Bug 115142/INGSRV3410
**          Verify that the conditions for raising the E_DMA481 error still
**          exist once the start and end of the TX log file has been fixed.
**          That is, after a dirty check indicates that the E_DMA481 error
**          has occured, pin the TX log file and redo the check.
**	28-Aug-2006 (jonj)
**	    Add code to allocate/deallocate (new) LFD structures for 
**	    XA transactions.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      24-Mar-2009 (ashco01) Bug: 121843
**          Check Shared LXB has been fully initialized before attempting to utilize it
**          in a shared XA transaction.
*/


/*
** Internal list management routine:
*/
static i4 expand_list(i4 type);

/*
** Name: LG_allocate_cb	    -- Allocate an LG control block.
**
** Description:
**	This function allocates an LG control block of the specified type.
**
**	Control blocks are maintained on "small block" and "large block" lists
**	which are expanded as necessary up to pre-set limits. The first free
**	control block is removed from the appropriate list and returned.
**
**	Some aspects of the free list algorithms require that the various
**	control blocks have common initial header fields such as "type" and
**	"next" and "id".
**
**	The block is returned without its type field set. The caller should
**	set the type field correctly after allocating the control block.
**
** Inputs:
**	type		    -- type of control block desired.
**
** Outputs:
**	none
**
** Returns:
**	process-specific address of the desired control block, or NULL if
**	no control block can be allocated.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	21-oct-1992 (bryanp)
**	    Improve error message logging when out-of-resources occurs.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	    Centralized accounting of lgd_???_inuse fields in this code
**	    instead of being responsibility of caller.
** 	    Object is returned with its mutex latched, where appropriate.
**	24-Aug-1999 (jenjo02)
**	    LXBs are now allocated by their own function, LG_allocate_lxb(),
**	    which at the same time assigns the transaction id.
*/
PTR
LG_allocate_cb( i4  type )
{
    PTR	    		return_cb = (PTR)0;
    register LPB	*lpb;
    register LDB	*ldb;
    register LPD	*lpd;
    register LFB	*lfb;
    register LFD	*lfd;
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    SIZE_TYPE		cb_offset;
    i4 			err_code;
    STATUS  		status;

    switch (type)
    {
	case LPB_TYPE:
	    /*	Allocate a LPB for this process. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpbb_mutex) == 0)
	    {
		if ((cb_offset = lgd->lgd_lpbb_free) == 0 &&
		    (cb_offset = expand_list(LPB_TYPE)) == 0)
		{
		    uleFormat(NULL, E_DMA437_NOT_ENOUGH_SBKS, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lpbb_count, 0, lgd->lgd_lpbb_size);
		    uleFormat(NULL, E_DMA438_LPB_ALLOC_FAIL, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    return ((PTR)NULL);
		}

		lpb = (LPB *)LGK_PTR_FROM_OFFSET(cb_offset);
		lgd->lgd_lpbb_free = lpb->lpb_next;
		lgd->lgd_lpb_inuse++;
		(VOID)LG_unmutex(&lgd->lgd_lpbb_mutex);
		(VOID)LG_mutex(SEM_EXCL, &lpb->lpb_mutex);
		lpb->lpb_type = LPB_TYPE;
		return_cb = (PTR)lpb;
	    }
	    break;

	case LPD_TYPE:
	    /*	Allocate an LPD for this process's connection to this DB. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpdb_mutex) == 0)
	    {
		if ((cb_offset = lgd->lgd_lpdb_free) == 0 &&
		    (cb_offset = expand_list(LPD_TYPE)) == 0)
		{
		    uleFormat(NULL, E_DMA437_NOT_ENOUGH_SBKS, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lpdb_count, 0, lgd->lgd_lpdb_size);
		    uleFormat(NULL, E_DMA43A_LPD_ALLOC_FAIL, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    return ((PTR)NULL);
		}

		lpd = (LPD *)LGK_PTR_FROM_OFFSET(cb_offset);
		lgd->lgd_lpdb_free = lpd->lpd_next;
		lgd->lgd_lpd_inuse++;
		(VOID)LG_unmutex(&lgd->lgd_lpdb_mutex);
		lpd->lpd_type = LPD_TYPE;
		return_cb = (PTR)lpd;
	    }
	    break;

	case LFD_TYPE:
	    /*	Allocate an LFD to enqueue a post-commit file delete (XA only) */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lfdb_mutex) == 0)
	    {
		if ((cb_offset = lgd->lgd_lfdb_free) == 0 &&
		    (cb_offset = expand_list(LFD_TYPE)) == 0)
		{
		    uleFormat(NULL, E_DMA437_NOT_ENOUGH_SBKS, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lfdb_count, 0, lgd->lgd_lfdb_size);
		    uleFormat(NULL, E_DMA441_LFD_ALLOC_FAIL, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    return ((PTR)NULL);
		}

		lfd = (LFD *)LGK_PTR_FROM_OFFSET(cb_offset);
		lgd->lgd_lfdb_free = lfd->lfd_next;
		lgd->lgd_lfd_inuse++;
		(VOID)LG_unmutex(&lgd->lgd_lfdb_mutex);
		lfd->lfd_type = LFD_TYPE;
		return_cb = (PTR)lfd;
	    }
	    break;

	case LDB_TYPE:
	    /*	Allocate a new LDB. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_ldbb_mutex) == 0)
	    {
		if ((cb_offset = lgd->lgd_ldbb_free) == 0 &&
		    (cb_offset = expand_list(LDB_TYPE)) == 0)
		{
		    uleFormat(NULL, E_DMA43B_NOT_ENOUGH_LBKS, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_ldbb_count, 0, lgd->lgd_ldbb_size);
		    uleFormat(NULL, E_DMA43C_LDB_ALLOC_FAIL, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    return ((PTR)NULL);
		}

		ldb = (LDB *)LGK_PTR_FROM_OFFSET(cb_offset);
		lgd->lgd_ldbb_free = ldb->ldb_next;
		lgd->lgd_ldb_inuse++;
		(VOID)LG_unmutex(&lgd->lgd_ldbb_mutex);
		(VOID)LG_mutex(SEM_EXCL, &ldb->ldb_mutex);
		ldb->ldb_type = LDB_TYPE;
		return_cb = (PTR)ldb;
	    }
	    break;

	case LFB_TYPE:
	    /*	Allocate a new LFB. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lfbb_mutex) == 0)
	    {
		if ((cb_offset = lgd->lgd_lfbb_free) == 0 &&
		    (cb_offset = expand_list(LFB_TYPE)) == 0)
		{
		    uleFormat(NULL, E_DMA43B_NOT_ENOUGH_LBKS, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lfbb_count, 0, lgd->lgd_lfbb_size);
		    uleFormat(NULL, E_DMA43C_LDB_ALLOC_FAIL, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    return ((PTR)NULL);
		}

		lfb = (LFB *)LGK_PTR_FROM_OFFSET(cb_offset);
		lgd->lgd_lfbb_free = lfb->lfb_next;
		(VOID)LG_unmutex(&lgd->lgd_lfbb_mutex);
		(VOID)LG_mutex(SEM_EXCL, &lfb->lfb_mutex);
		lfb->lfb_type = LFB_TYPE;
		return_cb = (PTR)lfb;
	    }
	    break;

	default:
	    break;
    }
    LGK_VERIFY_ADDR(return_cb, 32);
    return (return_cb);
}

/*
** Name: LG_deallocate_cb	    -- Deallocate an LG control block.
**
** Description:
**	This function returns an LG control block to its free list.
**
**	Control blocks are maintained on "small block" and "large block" lists
**	which are expanded as necessary up to pre-set limits. The control block
**	is placed onto the head of the appropriate free list.
**
**	Some aspects of the free list algorithms require that the various
**	control blocks have common initial header fields such as "type" and
**	"next" and "id".
**
** Inputs:
**	type		    -- type of control block being freed.
**	cb		    -- process-specific address of the control block
**
** Outputs:
**	none
**
** Returns:
**	VOID
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	    Centralized accounting of lgd_???_inuse fields in this code
**	    instead of being responsibility of caller.
**	    Object's mutex is assumed to be held when the call to 
**	    deallocate the object is made, at which time it's released.
*/
VOID
LG_deallocate_cb( i4  type, PTR cb )
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB	*lpb;
    register LXB	*lxb;
    register LPD	*lpd;
    register LDB	*ldb;
    register LFB	*lfb;
    register LFD	*lfd;
    STATUS  status;

    LGK_VERIFY_ADDR(cb, 32);
    switch (type)
    {
	case LPB_TYPE:
	    lpb = (LPB *)cb;

	    lpb->lpb_type = 0;
	    lpb->lpb_status = 0;
	    lpb->lpb_id.id_instance++;

	    (VOID)LG_unmutex(&lpb->lpb_mutex);

	    /*	Deallocate the LPB. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpbb_mutex) == 0)
	    {
		lpb->lpb_next = lgd->lgd_lpbb_free;
		lgd->lgd_lpbb_free = LGK_OFFSET_FROM_PTR(lpb);
		lgd->lgd_lpb_inuse--;
		(VOID)LG_unmutex(&lgd->lgd_lpbb_mutex);
	    }

	    break;

	case LXB_TYPE:
	    lxb = (LXB *)cb;

	    lxb->lxb_type = 0;
	    lxb->lxb_id.id_instance++;

	    (VOID)LG_unmutex(&lxb->lxb_mutex);

	    /*	Deallocate the LXB. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex) == 0)
	    {
		/* If SHARED LXB, remove from shared list */
		if ( lxb->lxb_status & LXB_SHARED )
		{
		    LXBQ	*next_lxbq, *prev_lxbq;

		    next_lxbq = 
			(LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_slxb.lxbq_next);
		    prev_lxbq = 
			(LXBQ*)LGK_PTR_FROM_OFFSET(lxb->lxb_slxb.lxbq_prev);
		    next_lxbq->lxbq_prev = lxb->lxb_slxb.lxbq_prev;
		    prev_lxbq->lxbq_next = lxb->lxb_slxb.lxbq_next;
		}

		lxb->lxb_next = lgd->lgd_lxbb_free;
		lgd->lgd_lxbb_free = LGK_OFFSET_FROM_PTR(lxb);
		lgd->lgd_lxb_inuse--;
		(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);
	    }

	    break;

	case LPD_TYPE:
	    lpd = (LPD *)cb;

	    lpd->lpd_type = 0;
	    lpd->lpd_id.id_instance++;

	    /*	Deallocate the LPD. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpdb_mutex) == 0)
	    {
		lpd->lpd_next = lgd->lgd_lpdb_free;
		lgd->lgd_lpdb_free = LGK_OFFSET_FROM_PTR(lpd);
		lgd->lgd_lpd_inuse--;
		(VOID)LG_unmutex(&lgd->lgd_lpdb_mutex);
	    }

	    break;

	case LFD_TYPE:
	    lfd = (LFD *)cb;

	    lfd->lfd_type = 0;

	    /*	Deallocate the LFD. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lfdb_mutex) == 0)
	    {
		lfd->lfd_next = lgd->lgd_lfdb_free;
		lgd->lgd_lfdb_free = LGK_OFFSET_FROM_PTR(lfd);
		lgd->lgd_lfd_inuse--;
		(VOID)LG_unmutex(&lgd->lgd_lfdb_mutex);
	    }

	    break;

	case LDB_TYPE:
	    ldb = (LDB *)cb;

	    ldb->ldb_type = 0;
	    ldb->ldb_status = 0;
	    ldb->ldb_id.id_instance++;

	    (VOID)LG_unmutex(&ldb->ldb_mutex);

	    /*  Deallocate the LDB. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_ldbb_mutex) == 0)
	    {
		ldb->ldb_next = lgd->lgd_ldbb_free;
		lgd->lgd_ldbb_free = LGK_OFFSET_FROM_PTR(ldb);
		lgd->lgd_ldb_inuse--;
		(VOID)LG_unmutex(&lgd->lgd_ldbb_mutex);
	    }

	    break;

	case LFB_TYPE:
	    lfb = (LFB *)cb;

	    lfb->lfb_type = 0;
	    lfb->lfb_status = 0;
	    lfb->lfb_id.id_instance++;

	    (VOID)LG_unmutex(&lfb->lfb_mutex);

	    /*  Deallocate the LFB. */

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lfbb_mutex) == 0)
	    {
		lfb->lfb_next = lgd->lgd_lfbb_free;
		lgd->lgd_lfbb_free = LGK_OFFSET_FROM_PTR(lfb);
		(VOID)LG_unmutex(&lgd->lgd_lfbb_mutex);
	    }

	    break;

	default:
	    break;
    }

    return;
}

/*{
** Name: LG_allocate_lbb	- Allocate and initialize a free lbb.
**
** Description:
**	This routine returns an LBB suitable for writing into.
**	If the log file has reached "full", or some other error is
**	detected, an error status is returned so that the caller
**	can suspend.
**
**	If there is a "current" LBB, we simply return it.  If there is
**	no current LBB, we make one current from the free queue.
**	If no free LBB is available, we return OK but with a null LBB
**	address;  *and* we leave the lfb_fq_mutex held.  The caller should
**	not drop the mutex until it gets itself safely queued on the
**	free buffer wake-me-up list.
**
**	A special case exists when the caller sees that it needs multiple
**	buffers to hold a long or spanning record.  In that case, the caller
**	has already locked the current LBB, and has decremented lfb_fq_count
**	guaranteeing access to N more.  For that special case, all we
**	want to do is to grab successive buffers from the free list and
**	initialize them;  they are not made CURRENT.  For this case, the
**	caller passes a nonzero "reserved count", which we will count down.
**
**	For some reason (convenience, I suppose), this is where we check
**	to see if enough log has been written to ask for a CP.
**
**	Initially, the lbh_address field in the block is set to contain the
**	correct sequence number, but an offset field which is "empty". This
**	lbh_address field is updated by LG_write each time that it completes the
**	adding of a record to the page; thus the lbh_address field ends up
**	holding the address of the last complete log record on the page. If the
**	page's one and only log record does not end on this page, then the
**	lbh_address field's offset portion remains "empty". This is handled as
**	a special case by LGopen. LGread already knows how to reconstruct
**	spanned records, which it does using lbh_used, not lbh_address.
**
** Inputs:
**
** Outputs:
**	Returns:
**	    0				    Caller must suspend
**	    lbb				    The assigned LBB.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	18-jan-1993 (jnash)
**	    Reduced logging project.  Check for write beyond begin of file. 
**	16-feb-1993 (bryanp)
**	    Initialize lbb_[first,last]_lsn.
**	24-may-1993 (bryanp)
**	    Initialize lbh_address in LG_allocate_lbb.
**	18-oct-1993 (rogerk)
**	    Made call to LG_check_logfull unconditional as check-stall check
**	    in now made within.  Made CP space calculation not overflow on
**	    MAXI4 size logfiles.  Changed check for logfile wrap-around to
**	    panic rather than returning a zero lbb.  Checks in LG_check_logfull
**	    should always prevent logfile wrap-around from occurring.
**	15-apr-1994 (chiku)
**	    Bug56702: changed definition of LG_allocate_lbb() to
**	    return E_DM9070_LOG_LOGFULL on logfull.
**	01-Mar-1996 (jenjo02)
**	    Added lfb_fq_count, lfb_wq_count, new mutexes for each queue. 
**	    The next LBB is picked from the free queue, initialized,
**	    established as current, and returned.
**	    Caller must hold EXCL lock on lfb_current_lbb_mutex.
**	26-Apr-1996 (jenjo02)
**	    Added reserved_lbb_count argument. If non-zero, caller
**	    wants a "reserved" LBB from the free queue. If granted,
**	    caller's count is decremented.
**	08-May-1996 (jenjo02)
**	    Removed 2nd parm (LBB **lbbo). This function's function 
**	    is to select a new lfb_current_lbb, which it fills in 
**	    when successful, leaving 0 otherwise. Defining an output
**	    LBB parm in addition to lfb_current_lbb seems redundant.
**	29-May-1996 (jenjo02)
**	    Added lxb parm to LG_allocate_LBB(), used solely to pass
**	    on up to LG_check_logfull(), thence to LG_abort_oldest().
**	19-Oct-1996 (jenjo02)
**	    In LG_allocate_lbb(), don't return LOGFULL if allocating a log
**	    buffer for a record span operation. We can't give up on the
**	    the completion of a partially written log record.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	    Added "LBB **lbbo" parm to LG_allocate_lbb into which a pointer
**	    to the allocated LBB is returned.
**	03-Mar-2000 (jenjo02)
**	    BUG 98473: Release current_lbb_mutex before calling
**	    LG_check_logfull() in LG_allocate_lbb() to prevent
**	    deadlocks with LXB mutexes in LG_abort_oldest().
**	03-Mar-2000 (jenjo02)
**	    Don't check logfull if allocating a "reserved" LBB.
**      01-Sep-2005 (horda03) Bug 115142/INGSRV3410
**          Verify that the conditions for raising the E_DMA481 error still
**          exist once the start and end of the TX log file has been fixed.
**          That is, after a dirty check indicates that the E_DMA481 error
**          has occured, pin the TX log file and redo the check.
**	15-Mar-2006 (jenjo02)
**	    Init (new) lbb_commit_count.
**	    Init lbb_forced_lsn, lbb_forced_lga
**	21-Mar-2006 (jenjo02)
**	    Maintain (new) lfb_fq_leof.
**	20-Sep-2006 (kschendel)
**	    Exit holding lfb-fq-mutex if no available LBB's, so that
**	    the caller can safely queue for wakeup.
*/
STATUS
LG_allocate_lbb(
register LFB *lfb, 
i4 	*reserved_lbb_count, 
LXB 	*lxb,
LBB	**lbbo)
{
    register LGD	*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LBB	*lbb;
    register LBH	*lbh;
    LBB			*next_lbb;
    LBB			*prev_lbb;
    i4		used_blocks;
    i4		err_code;
    STATUS		status;

    /* Assume failure */
    *lbbo = (LBB *)NULL;

    /*
    ** Check for any logfull force-abort conditions.
    **
    **   - If we have reached the force-abort limit, then we may have to
    **     start aborting transactions to free up log space.
    **   - If the log has reached the full limit, then we have to stall
    **     until new space is freed up in the log file.
    **   - If we are executing a consistency point and we have reached the
    **     cpstall limit, then stall until the CP is finished.
    **
    ** This call sets up masks in the lgd_status_mask field which may cause
    ** future LGreserve calls to stall until the conditions are cleared.
    **
    ** If allocating a "reserved" buffer for a span operation, don't
    ** check logfull. We hold the 1st buffer of the span's lbb_mutex
    ** on which other threads may be waiting, and we wish to avoid
    ** a mutex deadlock with their lxb_mutex's if we are in a 
    ** force-abort situation.
    */
    if ( *reserved_lbb_count == 0 &&
         LG_check_logfull(lfb, lxb) == E_DM9070_LOG_LOGFULL )
    {
	/*
	** Dead logfull condition.
	*/
	return ( E_DM9070_LOG_LOGFULL);
    }

    /*
    ** If a CP interval has elapsed, then signal recovery to generate
    ** a new Consistency Point.
    **
    ** We can't signal a new CP if one is already in progress.
    */
    if (lgd->lgd_no_bcp == 0)
    {
	if (lfb->lfb_header.lgh_end.la_sequence > 
					lfb->lfb_header.lgh_cp.la_sequence)
	{
	    used_blocks= 
		lfb->lfb_header.lgh_count + 1 - 
		(lfb->lfb_header.lgh_cp.la_block - 
				  lfb->lfb_header.lgh_end.la_block);
	}
	else
	{
	    used_blocks= lfb->lfb_header.lgh_end.la_block - 
		     lfb->lfb_header.lgh_cp.la_block + 1;
	}

	if (used_blocks > lfb->lfb_header.lgh_l_cp)
	{
	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
		return(status);
	    if (lgd->lgd_no_bcp == 0)
	    {
		lgd->lgd_no_bcp = 1;
		LG_signal_event(LGD_CPNEEDED, LGD_ECPDONE, TRUE);
	    }
	    (VOID)LG_unmutex(&lgd->lgd_mutex);
	}
    }

    /*
    ** Check for writing beyond the begin of file.
    ** This should be totally impossible due to panic checks in check_logfull.
    **
    ** Take the Free Queue mutex before making the check, this will partially
    ** prevent the situation where the EOF can be change while we are checking it.
    ** If it appears that the write is beyond the EOF, then get the CURRENT_LBB
    ** (if there's no CURRENT_LBB then no other session can move the EOF as we now
    ** own the Free Queue mutex) so the EOF can be safely determined.
    */

    /* Lock the free queue */
    if (status = LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex))
	return(status);

    if ( (lfb->lfb_header.lgh_end.la_sequence > 
	  lfb->lfb_header.lgh_begin.la_sequence) &&
	 (lfb->lfb_header.lgh_end.la_block >=
	    lfb->lfb_header.lgh_begin.la_block) )
    {
        /* OK - we think we're writing beyond the begin of the file,
        **      but we did the check without pinning the beginning
        **      or end, so it is possible that beginning of the
        **      TX log moved around to the "start" of the file after
        **      the la_sequence number was checked, but before the
        **      la_offset.
        **
        **      So lets do it again but now holding the required
        **      mutex CURRENT_LBB (if one exists).
        */
        i4 mutex_held  = FALSE;

        for(;;)
        {
            /* Get the CURRENT_LBB, if there isn't one we're OK 
            ** because we hold the Free Queue mutex, so new CURRENT_LBB can
            ** be added.
            */
            lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb);

            if (lbb->lbb_state & LBB_CURRENT)
            {
                LG_unmutex(&lfb->lfb_fq_mutex);
                if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
                    return(status);
                if (lbb->lbb_state & LBB_CURRENT)
                {
                    mutex_held = TRUE;
                    break;
                }
                LG_unmutex(&lbb->lbb_mutex);
                if (status = LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex))
                    return(status);
            }
            else
                break;
        }

        if ( (lfb->lfb_header.lgh_end.la_sequence > 
	      lfb->lfb_header.lgh_begin.la_sequence) &&
	     (lfb->lfb_header.lgh_end.la_block >=
	        lfb->lfb_header.lgh_begin.la_block) )
        {
           LG_unmutex( mutex_held ? &lbb->lbb_mutex : &lfb->lfb_fq_mutex);

           uleFormat(NULL, E_DMA481_WRITE_PAST_BOF, (CL_ERR_DESC *)NULL,
	              ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
           PCexit(FAIL);
        }

        TRdisplay( "E_DMA481 error has been avoided\n");

        if (mutex_held)
        {
            /* Currently hold the CURRENT_LBB mutex, no the Free Queue
            ** mutex.
            */
            LG_unmutex( &lbb->lbb_mutex );

            if (status = LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex))
               return(status);
        }
    }

    /*
    ** While we waited for the mutex, a new current may
    ** have been allocated. If it's still CURRENT, return
    ** it, otherwise continue with the allocation.
    */
    while (*reserved_lbb_count == 0)
    {
	lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_current_lbb);

	if (lbb->lbb_state & LBB_CURRENT)
	{
	    LG_unmutex(&lfb->lfb_fq_mutex);
	    if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
		return(status);
	    if (lbb->lbb_state & LBB_CURRENT)
	    {
		*lbbo = lbb;
		return(OK);
	    }
	    LG_unmutex(&lbb->lbb_mutex);
	    if (status = LG_mutex(SEM_EXCL, &lfb->lfb_fq_mutex))
		return(status);
	}
	else
	    break;
    }

    /*	
    **  Pull a LBB from the free queue, decrementing
    **  the "reserved" count if this request is for
    **  a "reserved" LBB otherwise decrementing the free count.
    */
    if (lfb->lfb_fq_count || *reserved_lbb_count)
    {
	lbb = (LBB *)LGK_PTR_FROM_OFFSET(lfb->lfb_fq_next);

	next_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb->lbb_next);
	prev_lbb = (LBB *)LGK_PTR_FROM_OFFSET(lbb->lbb_prev);
	next_lbb->lbb_prev = lbb->lbb_prev;
	prev_lbb->lbb_next = lbb->lbb_next;

	/* If this LBB is logical EOF, reset leof to last on fq */
	if ( lfb->lfb_fq_leof == LGK_OFFSET_FROM_PTR(lbb) )
	    lfb->lfb_fq_leof = lfb->lfb_fq_prev;

	if (*reserved_lbb_count)
	    (*reserved_lbb_count)--;
	else
	    lfb->lfb_fq_count--;
    }
    else
    {
	/* No free buffers (*lbbo = 0) */
	/* Return, still holding lfb_fq_mutex */
	return (OK);
    }

    /*	Mutex the LBB */
    /* Although the normal order is lbb mutex before lfb fq mutex, we
    ** get away with this one!  LG never takes the fq mutex when holding
    ** an LBB that *was on the free list*.
    */
    if (status = LG_mutex(SEM_EXCL, &lbb->lbb_mutex))
	return(status);

    lbb->lbb_state = LBB_MODIFIED;
    
    /* If a normal LBB (not reserved), mark it as current */
    if (*reserved_lbb_count == 0)
    {
	lbb->lbb_state |= LBB_CURRENT;
	lfb->lfb_current_lbb = LGK_OFFSET_FROM_PTR(lbb);
    }

    /* Release the free queue */
    (VOID)LG_unmutex(&lfb->lfb_fq_mutex);

    lbb->lbb_next_byte = lbb->lbb_buffer + sizeof(LBH);
    lbb->lbb_bytes_used = sizeof(LBH);

    lbb->lbb_commit_count = 0;
    lbb->lbb_owning_lxb = 0;

    lbb->lbb_first_lsn.lsn_high = lbb->lbb_first_lsn.lsn_low =
	lbb->lbb_last_lsn.lsn_high = lbb->lbb_last_lsn.lsn_low = 
	lbb->lbb_forced_lsn.lsn_high = lbb->lbb_forced_lsn.lsn_low =0;

    lbb->lbb_forced_lga.la_sequence = lbb->lbb_forced_lga.la_block =
	lbb->lbb_forced_lga.la_offset = 0;

    /*	Calculate the new LGA for this page. */

    lbb->lbb_lga.la_sequence = lfb->lfb_header.lgh_end.la_sequence;
    lbb->lbb_lga.la_block = lfb->lfb_header.lgh_end.la_block + 1;
    lbb->lbb_lga.la_offset = 0;

    /*	Handle wrap around. */

    if (lbb->lbb_lga.la_block >= lfb->lfb_header.lgh_count)
    {
	lbb->lbb_lga.la_sequence++;
	lbb->lbb_lga.la_block  = 1;
    }

    lbh = (LBH *)LGK_PTR_FROM_OFFSET(lbb->lbb_buffer);
    lbh->lbh_address = lbb->lbb_lga;
    lbh->lbh_checksum = 0;

    /*	Set begin if never set. */

    if (lfb->lfb_header.lgh_begin.la_block == 0) 
    {
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	if (lfb->lfb_header.lgh_begin.la_block == 0) 
	    lfb->lfb_header.lgh_begin = lbb->lbb_lga;
	(VOID)LG_unmutex(&lgd->lgd_mutex);
    }

    /* return with lbb_mutex locked */
    *lbbo = lbb;
    return (OK);
}

/*
** Name: LG_allocate_lxb	    -- Allocate an LXB control block.
**
** Description:
**	This function allocates an LXB and assigns a transaction id.
**
** Inputs:
**	lfb		    -- pointer to transaction's LFB.
**	lpd		    -- pointer to process LPD
**	DisTranId	    -- optional pointer to DB_DIS_TRAN_ID.
**
** Outputs:
**	shared_lxb	    -- pointer to pointer to SHARED LXB,
**			       filled in if SHARED LXB is allocated
**			       with that LXB's mutex held.
**	status		    -- OK if no problems
**			       LG_EXCEED_LIMIT if no LXBs available
**			       LG_BADPARAM if XA SHARED txn
**			       is aborting.
**			       LG_DUPLICATE_TXN if LG_XA_START_XA 
**			       without JOIN and XID exists.
**			       LG_NO_TRANSACTION if LG_XA_START_XA 
**			       with JOIN and XID does not exist.
**
** Returns:
**	pointer to the allocated (handle) LXB, or NULL if no LXB can be allocated.
**
** History:
**	24-Aug-1999 (jenjo02)
**	    Written to isolate LXB allocation and transaction-id assignment.
**	15-Dec-1999 (jenjo02)
**	    Added support for SHARED/HANDLE LXBs.
**	18-Sep-2001 (jenjo02)
**	    Replace call to LGshow(LG_S_ETRANSACTION) with inline
**	    code to derive transaction id.
**	22-Oct-2002 (jenjo02)
**	    Added "status" to prototype, return LG_BADPARAM
**	    if SHARED txn is aborting.
**	28-Mar-2003 (jenjo02)
**	    Add lxb_wait_lwq to list of things to be initialized.
**	    B109938, STAR 12560435, INGSRV2170
**	17-Dec-2003 (jenjo02)
**	    Added "flags" to support LG_CONNECT calls, in which
**	    the shared LXB already exists.
**	20-Apr-2004 (jenjo02)
**	    Extracted tran_id computation to LG_assign_tran_id()
**	    so it can be called elsewhere.
**	20-Jun-2006 (jonj)
**	    Add support for explicit xa_start([JOIN]).
**	27-Jul-2006 (jonj)
**	    Oops, gotta match on XID, not just GTRID.
**	01-Aug-2006 (jonj)
**	    Cleanup inconsistencies with XA_START, XA_JOIN,
**	    GTRIDs and XIDs.  
**	11-Sep-2006 (jonj)
**	    Init (new) lxb_handle_wts, lxb_handle_ets.
**      24-Mar-2009 (ashco01) Bug: 121843
**          Check Shared LXB has been fully initialized before attempting to utilize it
**          in a shared XA transaction.
*/
LXB*
LG_allocate_lxb( 
i4		flags,
LFB 		*lfb, 
LPD  		*lpd,
DB_DIS_TRAN_ID *DisTranId, 
LXB 		**shared_lxb,
STATUS  	*status)
{
    LXB		*return_lxb;
    LGD		*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    SIZE_TYPE 	cb_offset;
    i4 		err_code;
    LXBQ	*lxbq;
    LXB		**lxbp, *slxb, *lxb;
    LPD		*slpd;


    /*	Allocate a LXB for this transaction. */

    if ( flags & LG_CONNECT )
    {
	/* Mutexed by caller */
	slxb = *shared_lxb;
    }
    else
    {
	*shared_lxb = (LXB*)NULL;
	slxb = (LXB*)NULL;
    }

    return_lxb = (LXB*)NULL;

    if (*status = LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex) == OK)
    {
	do
	{
	    lxbp = &return_lxb;

	    /*
	    ** Look for an existing matching XA transaction
	    ** in the same database:
	    */
	    if ( slxb == (LXB *)NULL &&
		 DisTranId && 
		 DisTranId->db_dis_tran_id_type == DB_XA_DIS_TRAN_ID_TYPE )
	    {
		lxbp = &slxb;
		lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_slxb.lxbq_next);

		/* Search the list of SHARED LXBs on GTRID */
		while ( lxbq != (LXBQ *)&lgd->lgd_slxb.lxbq_next )
		{
		    slxb = (LXB *)((char *)lxbq - CL_OFFSETOF(LXB,lxb_slxb));

                     if (!slxb->lxb_lpd)
                     {
                         /* Wait for mutex on slxb to ensure that the session initialising it has
                         ** actually completed the task even though the slxb is already on the queue.
                         */
                         if ( (*status = LG_mutex(SEM_EXCL, &slxb->lxb_mutex)) == OK)
                         {
                             if (!slxb->lxb_lpd)
                             {
                                 LG_unmutex( &slxb->lxb_mutex);
                                 *status = LG_BADPARAM;
                                 return((LXB*)NULL);
                             }
                             LG_unmutex( &slxb->lxb_mutex);
                         }
                         else
                         {
                             return((LXB*)NULL);
                         }
                     }

		    slpd = (LPD *)LGK_PTR_FROM_OFFSET(slxb->lxb_lpd);
		    if ( slpd->lpd_ldb == lpd->lpd_ldb &&
			 XA_XID_GTRID_EQL_MACRO(*DisTranId, slxb->lxb_dis_tran_id) )
		    {
			(VOID)LG_mutex(SEM_EXCL, &slxb->lxb_mutex);

			if ( slxb->lxb_status & LXB_ABORT )
			{
			    LG_I4ID_TO_ID xid;
			    /* Reject connect to aborting SHR txn */
			    xid.id_lgid = slxb->lxb_id;
			    uleFormat(NULL, E_DMA495_LG_BAD_SHR_TXN_STATE, (CL_ERR_DESC *)NULL,
				ULE_LOG, NULL, NULL, 0, NULL, &err_code, 4,
				0, xid.id_i4id,
				0, slxb->lxb_status,
				0, "LGbegin",
				0, 0);
			    (VOID)LG_unmutex(&slxb->lxb_mutex);
			    (VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);
			    *status = LG_BADPARAM;
			    return((LXB*)NULL);
			}

			/* 
			** If XA_START_XA, the Branch XID cannot exist;
			** if XA_START_JOIN, the Branch XID must exist.
			**
			** Each handle LXB contains a Branch XID.
			*/
			if ( flags & (LG_XA_START_XA | LG_XA_START_JOIN) )
			{
			    LXB		*handle_lxb;
			    LXBQ	*handle_lxbq;

			    *status = OK;
			    handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);

			    while ( handle_lxbq != (LXBQ*)&slxb->lxb_handles && *status == OK )
			    {
				handle_lxb = (LXB*)((char*)handle_lxbq - CL_OFFSETOF(LXB,lxb_handles));
				if ( DB_DIS_TRAN_ID_EQL_MACRO(*DisTranId, handle_lxb->lxb_dis_tran_id) )
				    *status = LG_DUPLICATE_TXN;
				handle_lxbq = (LXBQ*)LGK_PTR_FROM_OFFSET(handle_lxbq->lxbq_next);
			    }

			    if ( flags & LG_XA_START_JOIN )
			    {
				if ( *status == OK )
				    *status = LG_NO_TRANSACTION;
				else
				    *status = OK;
			    }
				
			    if ( *status )
			    {
				(VOID)LG_unmutex(&slxb->lxb_mutex);
				(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);
				return((LXB*)NULL);
			    }

			    /*
			    ** If TMJOINing to a disassociated context, adopt the context
			    ** instead of allocating yet another LXB.
			    ** Seeing LXB_RESUME tells the caller (LGbegin()) what we've
			    ** done.
			    */
			    if ( flags & LG_XA_START_JOIN && handle_lxb->lxb_status & LXB_ORPHAN )
			    {
				(VOID)LG_unmutex(&slxb->lxb_mutex);
				(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);
				(VOID)LG_mutex(SEM_EXCL, &handle_lxb->lxb_mutex);

				if ( handle_lxb->lxb_type == LXB_TYPE &&
				     handle_lxb->lxb_status & LXB_SHARED_HANDLE && 
				     handle_lxb->lxb_status & LXB_ORPHAN && 
				    DB_DIS_TRAN_ID_EQL_MACRO(*DisTranId, handle_lxb->lxb_dis_tran_id) )
				{
				    *status = LG_adopt_xact(handle_lxb, lpd);
				    if ( *status == OK )
				    {
					handle_lxb->lxb_status &= ~LXB_REASSOC_WAIT;
					handle_lxb->lxb_status |= LXB_RESUME;
					return(handle_lxb);
				    }
				}
				(VOID)LG_unmutex(&handle_lxb->lxb_mutex);
				(VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex);
				lxbq = (LXBQ *)LGK_PTR_FROM_OFFSET(lgd->lgd_slxb.lxbq_next);
				continue;
			    }
			}
			break;
		    }
		    lxbq = (LXBQ*)
			LGK_PTR_FROM_OFFSET(slxb->lxb_slxb.lxbq_next);
		    slxb = (LXB *)NULL;
		}
	    }
	    
	    /* If LXB needed, allocate one */
	    if ( (lxb = *lxbp) == (LXB *)NULL )
	    {
		if ((cb_offset = lgd->lgd_lxbb_free) == 0 &&
		    (cb_offset = expand_list(LXB_TYPE)) == 0)
		{
		    /* Give up shared lxb if we allocated it */
		    if ( slxb )
			(VOID)LG_unmutex(&slxb->lxb_mutex);
		    if ( *shared_lxb && (flags & LG_CONNECT) == 0 )
		    {
			slxb->lxb_next = lgd->lgd_lxbb_free;
			lgd->lgd_lxbb_free = LGK_OFFSET_FROM_PTR(slxb);
			lgd->lgd_lxb_inuse--;
			*shared_lxb = (LXB *)NULL;
		    }
		    (VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);

		    uleFormat(NULL, E_DMA437_NOT_ENOUGH_SBKS, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			0, lgd->lgd_lxbb_count, 0, lgd->lgd_lxbb_size);
		    uleFormat(NULL, E_DMA439_LXB_ALLOC_FAIL, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
		    *status = LG_EXCEED_LIMIT;
		    return((LXB*)NULL);
		}
		*lxbp = lxb = (LXB *)LGK_PTR_FROM_OFFSET(cb_offset);
		lgd->lgd_lxbb_free = lxb->lxb_next;
		lgd->lgd_lxb_inuse++;

		/* Lock LXB's mutex */
		(VOID)LG_mutex(SEM_EXCL, &lxb->lxb_mutex);

		lxb->lxb_lpd = 0;
		lxb->lxb_status = 0;
		lxb->lxb_dis_tran_id.db_dis_tran_id_type = 0;
		lxb->lxb_handle_count = 0;
		lxb->lxb_handle_preps = 0;
		lxb->lxb_handle_wts = 0;
		lxb->lxb_handle_ets = 0;
		lxb->lxb_wait_lwq = 0;
		lxb->lxb_lfd_next = LGK_OFFSET_FROM_PTR(&lxb->lxb_lfd_next);
		lxb->lxb_type = LXB_TYPE;

		/*
		** If allocating a SHARED LXB or just a normal one,
		** we need a new transaction identifier. Shared HANDLEs
		** employ the already assigned txn id of the SHARED txn.
		*/
		if ( slxb == (LXB *)NULL || lxb == slxb )
		{
		    LG_assign_tran_id(lfb, &lxb->lxb_lxh.lxh_tran_id, TRUE);

		    /* Just allocated SHARED LXB? */
		    if ( lxb == slxb )
		    {
			/* Link SHARED LXB to LGD queue */
			lxb->lxb_slxb.lxbq_next = lgd->lgd_slxb.lxbq_next;
			lxb->lxb_slxb.lxbq_prev = 
			    LGK_OFFSET_FROM_PTR(&lgd->lgd_slxb.lxbq_next);
			lxbq = (LXBQ *)
			    LGK_PTR_FROM_OFFSET(lgd->lgd_slxb.lxbq_next);
			lxbq->lxbq_prev = lgd->lgd_slxb.lxbq_next = 
			    LGK_OFFSET_FROM_PTR(&lxb->lxb_slxb);

			lxb->lxb_status = (LXB_SHARED | LXB_DISTRIBUTED);
			lxb->lxb_dis_tran_id = *DisTranId;

			/* Init the list of HANDLE LXBs */
			lxb->lxb_handle_count = 0;
			lxb->lxb_handles.lxbq_next =
				lxb->lxb_handles.lxbq_prev =
			    LGK_OFFSET_FROM_PTR(&lxb->lxb_handles.lxbq_next);

			/* Return pointer to just-allocated SHARED LXB */
			*shared_lxb = slxb;

#ifdef LG_SHARED_DEBUG
			{
			    CS_SID	sid;
			    CSget_sid(&sid);
			    TRdisplay("%@ SHARED lxb %x allocated to SID %x\n", *(u_i4*)&slxb->lxb_id, sid);
			}
#endif /* LG_SHARED_DEBUG */
		    }
		}
		/* Must be connecting a new HANDLE to a SHARED LXB */
		else
		{
		    /* Connect HANDLE LXB to SHARED LXB */
		    lxb->lxb_shared_lxb = 
			LGK_OFFSET_FROM_PTR(slxb);

		    if ( flags & LG_CONNECT )
		    {
			lxb->lxb_status = slxb->lxb_status & 
			    ~(LXB_ACTIVE | LXB_PROTECT | LXB_SHARED);
			lxb->lxb_status |= 
			     (LXB_SHARED_HANDLE | LXB_INACTIVE);
		    }
		    else
		    {
			lxb->lxb_status = (LXB_SHARED_HANDLE | LXB_DISTRIBUTED);
			lxb->lxb_dis_tran_id = *DisTranId;
		    }
		    
		    /* Copy SHARED transaction id to HANDLE */
		    lxb->lxb_lxh.lxh_tran_id =  slxb->lxb_lxh.lxh_tran_id;

		    /* Link HANDLE LXB to SHARED LXB list */
		    lxb->lxb_handles.lxbq_next = 
			slxb->lxb_handles.lxbq_next;
		    lxb->lxb_handles.lxbq_prev = 
			LGK_OFFSET_FROM_PTR(&slxb->lxb_handles.lxbq_next);
		    lxbq = (LXBQ *)
			LGK_PTR_FROM_OFFSET(slxb->lxb_handles.lxbq_next);
		    lxbq->lxbq_prev = slxb->lxb_handles.lxbq_next = 
			LGK_OFFSET_FROM_PTR(&lxb->lxb_handles);
		    slxb->lxb_handle_count++;

#ifdef LG_SHARED_DEBUG
		    {
			CS_SID	sid;
			CSget_sid(&sid);
			TRdisplay("%@ HANDLE lxb %x allocated to SID %x SHARED %x, count %d\n",
			    *(u_i4*)&lxb->lxb_id, sid, *(u_i4*)&slxb->lxb_id,
			    slxb->lxb_handle_count);
		    }
#endif /* LG_SHARED_DEBUG */

		    /* Release SHARED LXB's mutex if not allocated */
		    if ( *shared_lxb == (LXB*)NULL )
			(VOID)LG_unmutex(&slxb->lxb_mutex);
		}
	    }
	} while ( return_lxb == (LXB *)NULL );
	
	(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);
    }
    return (return_lxb);
}

/*{
** Name: expand_list	- Expand the size of a list.
**
** Description:
**      This routine is called when one of the allocation lists is empty.
**	This routine then attempts to allocate more storage for the list.  If
**	successful, a list item is returned.  If not successful a NULL is
**	returned.
**
** Inputs:
**      list                            The list to expand.
**	type				The type of cb we're allocating
**					for.
**
** Outputs:
**	Returns:
**	    NULL			List could not be extended.
**	    !NULL			Pointer to allocated item.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	21-oct-1992 (bryanp)
**	    Improve error message logging on system resource exhaustion.
**	17-mar-1993 (keving)
**	    =>Large blocks are now allocated in terms of LFBs because
**	    =>they are now larger than LDBs
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	    Blocks are now allocated by "type" instead of "large"
**	    or "small" blocks.
**	23-Apr-1996 (jenjo02)
**	    When expanding an existing list, double its size.
**	16-May-2003 (jenjo02)
**	    Check/adjust count -before- requesting memory to 
**	    avoid waste and zero count allocations.
**	07-Apr-2005 (jenjo02)
**	    Set count of new objects after objects have
**	    all been initialized.
*/
static i4
expand_list(i4 type)
{
    register LGD        *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    PTR			items;
    register LFB	*lfb;
    register LDB	*ldb;
    register LPB	*lpb;
    register LXB	*lxb;
    register LPD	*lpd;
    register LFD	*lfd;
    i4			item_size;
    i4			count, i;
    SIZE_TYPE		*lpbb_table;
    SIZE_TYPE		*lxbb_table;
    SIZE_TYPE		*lpdb_table;
    SIZE_TYPE		*lfbb_table;
    SIZE_TYPE		*ldbb_table;
    SIZE_TYPE		*lfdb_table;
    i4			err_code;
    char		sem_name[CS_SEM_NAME_LEN];

    switch (type)
    {
	case LPB_TYPE:
	    if (lgd->lgd_lpbb_count == lgd->lgd_lpbb_size)
	    {
		uleFormat(NULL, E_DMA43D_SBK_LIMIT_REACHED, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lpbb_count, 0, lgd->lgd_lpbb_size);
		return (0);
	    }
	    item_size = sizeof(LPB);
	    count = lgd->lgd_lpbb_count
		  ? lgd->lgd_lpbb_count
		  : 8160 / sizeof(LPB) ;
	    if (count > lgd->lgd_lpbb_size - lgd->lgd_lpbb_count)
		count = lgd->lgd_lpbb_size - lgd->lgd_lpbb_count;
	    break;

	case LXB_TYPE:
	    if (lgd->lgd_lxbb_count == lgd->lgd_lxbb_size)
	    {
		uleFormat(NULL, E_DMA43D_SBK_LIMIT_REACHED, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lxbb_count, 0, lgd->lgd_lxbb_size);
		return (0);
	    }
	    item_size = sizeof(LXB);
	    count = lgd->lgd_lxbb_count
		  ? lgd->lgd_lxbb_count
		  : 8160 / sizeof(LXB) ;
	    if (count > lgd->lgd_lxbb_size - lgd->lgd_lxbb_count)
		count = lgd->lgd_lxbb_size - lgd->lgd_lxbb_count;
	    break;

	case LPD_TYPE:
	    if (lgd->lgd_lpdb_count == lgd->lgd_lpdb_size)
	    {
		uleFormat(NULL, E_DMA43D_SBK_LIMIT_REACHED, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lpdb_count, 0, lgd->lgd_lpdb_size);
		return (0);
	    }
	    item_size = sizeof(LPD);
	    count = lgd->lgd_lpdb_count
		  ? lgd->lgd_lpdb_count
		  : 8160 / sizeof(LPD) ;
	    if (count > lgd->lgd_lpdb_size - lgd->lgd_lpdb_count)
		count = lgd->lgd_lpdb_size - lgd->lgd_lpdb_count;
	    break;

	case LFD_TYPE:
	    if (lgd->lgd_lfdb_count == lgd->lgd_lfdb_size)
	    {
		uleFormat(NULL, E_DMA43D_SBK_LIMIT_REACHED, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lfdb_count, 0, lgd->lgd_lfdb_size);
		return (0);
	    }
	    item_size = sizeof(LFD);
	    /*
	    ** Get memory for just a few LFDs. They're hopfully 
	    ** relatively infrequently needed by XA transactions
	    ** (DROP table, MODIFY) and we don't want to starve
	    ** the need for LXBs and/or LPDs which come from the
	    ** same "small" block of memory as LFDs.
	    */
	    count = lgd->lgd_lfdb_count
		  ? lgd->lgd_lfdb_count
		  : 8;
	    if (count > lgd->lgd_lfdb_size - lgd->lgd_lfdb_count)
		count = lgd->lgd_lfdb_size - lgd->lgd_lfdb_count;
	    break;

	case LFB_TYPE:
	    if (lgd->lgd_lfbb_count == lgd->lgd_lfbb_size)
	    {
		uleFormat(NULL, E_DMA43E_LBK_LIMIT_REACHED, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_lfbb_count, 0, lgd->lgd_lfbb_size);
		return (0);
	    }
	    item_size = sizeof(LFB);
	    count = lgd->lgd_lfbb_count
		  ? lgd->lgd_lfbb_count
		  : 8160 / sizeof(LFB) ;
	    if (count > lgd->lgd_lfbb_size - lgd->lgd_lfbb_count)
		count = lgd->lgd_lfbb_size - lgd->lgd_lfbb_count;
	    break;

	case LDB_TYPE:
	    if (lgd->lgd_ldbb_count == lgd->lgd_ldbb_size)
	    {
		uleFormat(NULL, E_DMA43E_LBK_LIMIT_REACHED, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
			    0, lgd->lgd_ldbb_count, 0, lgd->lgd_ldbb_size);
		return (0);
	    }
	    item_size = sizeof(LDB);
	    count = lgd->lgd_ldbb_count
		  ? lgd->lgd_ldbb_count
		  : 8160 / sizeof(LDB) ;
	    if (count > lgd->lgd_ldbb_size - lgd->lgd_ldbb_count)
		count = lgd->lgd_ldbb_size - lgd->lgd_ldbb_count;
	    break;
    }

    /*
    ** If we can't get the allocation we want, halve it
    ** until we get some minimum count. If that still
    ** fails, we're really out of memory, so return an error.
    */
    if ( count ) do
    {
	items = (PTR)lgkm_allocate_ext(count * item_size);

	if (items == NULL && ((count >>= 1) == 0))
	{
	    uleFormat(NULL, E_DMA43F_LG_SHMEM_NOMORE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, NULL, 0, NULL, &err_code, 0);
	    return (0);
	}
    } while (items == NULL);

    if ( items )
	items = (PTR)((char *)items + sizeof(LGK_EXT));

    switch (type)
    {
	case LPB_TYPE:
	    if ( items )
	    {
		lpb = (LPB *)items; 
		lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);

		i = lgd->lgd_lpbb_count;
		while ( count-- )
		{
		    lpb->lpb_type = 0;
		    lpb->lpb_id.id_id = ++i;
		    lpb->lpb_id.id_instance = 1;
		    lpbb_table[i] = LGK_OFFSET_FROM_PTR(lpb);
		    lpb->lpb_next = lgd->lgd_lpbb_free;
		    lgd->lgd_lpbb_free = LGK_OFFSET_FROM_PTR(lpb);
		    (VOID)LG_imutex(&lpb->lpb_mutex, 
			STprintf(sem_name, "LG LPB mutex %8d", i));
		    lpb++;
		}
		lgd->lgd_lpbb_count = i;
	    }

	    return (lgd->lgd_lpbb_free);

	case LXB_TYPE:
	    if ( items )
	    {
		lxb = (LXB *)items; 
		lxbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lxbb_table);

		i = lgd->lgd_lxbb_count;
		while ( count-- )
		{
		    lxb->lxb_type = 0;
		    lxb->lxb_id.id_id = ++i;
		    lxb->lxb_id.id_instance = 1;
		    lxbb_table[i] = LGK_OFFSET_FROM_PTR(lxb);
		    lxb->lxb_next = lgd->lgd_lxbb_free;
		    lgd->lgd_lxbb_free = LGK_OFFSET_FROM_PTR(lxb);
		    (VOID)LG_imutex(&lxb->lxb_mutex, 
			STprintf(sem_name, "LG LXB mutex %8d", i));
		    lxb++;
		}
		lgd->lgd_lxbb_count = i;
	    }

	    return (lgd->lgd_lxbb_free);

	case LPD_TYPE:
	    if ( items )
	    {
		lpd = (LPD *)items; 
		lpdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpdb_table);

		i = lgd->lgd_lpdb_count;
		while ( count-- )
		{
		    lpd->lpd_type = 0;
		    lpd->lpd_id.id_id = ++i;
		    lpd->lpd_id.id_instance = 1;
		    lpdb_table[i] = LGK_OFFSET_FROM_PTR(lpd);
		    lpd->lpd_next = lgd->lgd_lpdb_free;
		    lgd->lgd_lpdb_free = LGK_OFFSET_FROM_PTR(lpd);
		    lpd++;
		}
		lgd->lgd_lpdb_count = i;
	    }

	    return (lgd->lgd_lpdb_free);

	case LFD_TYPE:
	    if ( items )
	    {
		lfd = (LFD *)items; 
		lfdb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lfdb_table);

		i = lgd->lgd_lfdb_count;
		while ( count-- )
		{
		    lfd->lfd_type = 0;
		    lfdb_table[i] = LGK_OFFSET_FROM_PTR(lfd);
		    lfd->lfd_next = lgd->lgd_lfdb_free;
		    lgd->lgd_lfdb_free = LGK_OFFSET_FROM_PTR(lfd);
		    lfd++;
		}
		lgd->lgd_lfdb_count = i;
	    }

	    return (lgd->lgd_lfdb_free);

	case LFB_TYPE:
	    if ( items )
	    {
		lfb = (LFB *)items; 
		lfbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lfbb_table);

		i = lgd->lgd_lfbb_count;
		while ( count-- )
		{
		    lfb->lfb_type = 0;
		    lfb->lfb_id.id_id = ++i;
		    lfb->lfb_id.id_instance = 1;
		    lfbb_table[i] = LGK_OFFSET_FROM_PTR(lfb);
		    lfb->lfb_next = lgd->lgd_lfbb_free;
		    lgd->lgd_lfbb_free = LGK_OFFSET_FROM_PTR(lfb);
		    (VOID)LG_imutex(&lfb->lfb_mutex, 
			STprintf(sem_name, "LG LFB mutex %8d", i));
		    (VOID)LG_imutex(&lfb->lfb_fq_mutex, 
			STprintf(sem_name, "LG LFB fq mutex %8d", i));
		    (VOID)LG_imutex(&lfb->lfb_wq_mutex, 
			STprintf(sem_name, "LG LFB wq mutex %8d", i));
		    lfb++;
		}
		lgd->lgd_lfbb_count = i;
	    }

	    return (lgd->lgd_lfbb_free);

	case LDB_TYPE:
	    if ( items )
	    {
		ldb = (LDB *)items; 
		ldbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_ldbb_table);

		i = lgd->lgd_ldbb_count;
		while ( count-- )
		{
		    ldb->ldb_type = 0;
		    ldb->ldb_id.id_id = ++i;
		    ldb->ldb_id.id_instance = 1;
		    ldbb_table[i] = LGK_OFFSET_FROM_PTR(ldb);
		    ldb->ldb_next = lgd->lgd_ldbb_free;
		    lgd->lgd_ldbb_free = LGK_OFFSET_FROM_PTR(ldb);
		    (VOID)LG_imutex(&ldb->ldb_mutex, 
			STprintf(sem_name, "LG LDB mutex %8d", i));
		    ldb++;
		}
		lgd->lgd_ldbb_count = i;
	    }

	    return (lgd->lgd_ldbb_free);
    }
}

/*
** Name: LG_dealloc_lfb_buffers		- release the LFB's buffer memory.
**
** Description:
**	This routine frees the LLB and associated log buffer pages for an LFB.
**	This is used by LG_close and LG_rundown when a non-local
**	LFB is being tossed. (the local LFB is only tossed when the entire
**	installation shuts down, so we don't bother to explicitly free its
**	pages).
**
** Inputs:
**	lfb				- the Log File Block
**
** Outputs:
**	None
**
** Returns:
**	VOID
**
** History:
**	26-jul-1993 (bryanp)
**	    created.
**	18-Jan-1996 (jenjo02)
**	    lfb_mutex must be held on entry to this function.
**	08-apr-2002 (devjo01)
**	    Use 'lfb_first_lbb' to find beginning of contiguous allocation,
**	    and simply free entire chunk.
*/
VOID
LG_dealloc_lfb_buffers(LFB *lfb)
{
    if (lfb->lfb_first_lbb)
    {
	lgkm_deallocate_ext(
	    (LGK_EXT *)(((char *)LGK_PTR_FROM_OFFSET(lfb->lfb_first_lbb)) -
	    sizeof(LGK_EXT) ) );
    }
    return;
}

/*
** Name: LG_assign_tran_id	- Assign next avail transaction id.
**
** Description:
**	Computes the next available transaction id, 
**	returns it in tran_id.
**
** Inputs:
**	lfb			- the Log File Block
**	mutex_held		TRUE if caller holds lgd_lxbb_mutex.
**
** Outputs:
**	tran_id			Holds the assigned tran_id.
**
** Returns:
**	VOID
**
** History:
**	20-Apr-2004 (jenjo02)
**	    Extracted from LG_allocate_lxb.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace LXB parameter with
**	    DB_TRAN_ID pointer, filled with assigned
**	    tran_id.
**	    If known, keep dmf_svcb->svcb_last_tranid
**	    up to date.
*/
VOID
LG_assign_tran_id(
LFB		*lfb,
DB_TRAN_ID	*tran_id,
bool		mutex_held)
{
    LGD		*lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LXH		*lxh;
    LXBQ	*lxbq;
    LXBQ	*bucket_array = (LXBQ *)LGK_PTR_FROM_OFFSET(
					    lgd->lgd_lxh_buckets);
    SIZE_TYPE	lxh_offset, end_offset;

    if ( !mutex_held )
	(VOID)LG_mutex(SEM_EXCL, &lgd->lgd_lxbb_mutex);
	
    do
    {
	/*
	** Compute and assign next transaction identifier while
	** holding lxbb_mutex.
	**
	** Reserve tran_id (n, 0)
	** (so we can init tuple headers with invalid low tranid)
	*/
	if (++lfb->lfb_header.lgh_tran_id.db_low_tran == 0)
	{
	    lfb->lfb_header.lgh_tran_id.db_low_tran = 1;
	    lfb->lfb_header.lgh_tran_id.db_high_tran++;
	}

	/*
	** Reserve tranid (0,anything)
	** This is necessary so that unique ids generated by LK (0,n)
	** do not conflict with LG generated transaction ids
	**
	** In the event of RCP online recovery of a server crash,
	** RCP would not be able to assume ownership of a locklist with
	** a unique id of (0,n) which are reserved for LK generated unique ids.
	*/
	if (lfb->lfb_header.lgh_tran_id.db_high_tran == 0)
	    lfb->lfb_header.lgh_tran_id.db_high_tran = 1;
			    
	/*
	** Keep skipping incrementing low tran if (n, low) active
	*/
	*tran_id = lfb->lfb_header.lgh_tran_id;
	    
	lxbq = &bucket_array[tran_id->db_low_tran % lgd->lgd_lxbb_size];

	end_offset = LGK_OFFSET_FROM_PTR(&lxbq->lxbq_next);

	lxh_offset = lxbq->lxbq_prev;

	while (lxh_offset != end_offset)
	{
	    lxh = (LXH *)LGK_PTR_FROM_OFFSET(lxh_offset);
	    lxh_offset = lxh->lxh_lxbq.lxbq_prev;

	    if ( tran_id->db_low_tran == lxh->lxh_tran_id.db_low_tran )
		break;
	}
	/* If active, try next tran id */
    } while ( lxh_offset != end_offset );

    if ( dmf_svcb )
	dmf_svcb->svcb_last_tranid = tran_id->db_low_tran;

    if ( !mutex_held )
	(VOID)LG_unmutex(&lgd->lgd_lxbb_mutex);

    return;
}
