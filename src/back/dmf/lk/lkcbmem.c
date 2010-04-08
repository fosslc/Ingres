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
#include    <tr.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>

/**
**
**  Name: LKCBMEM.C - Implements the control block allocation & free operations
**
**  Description:
**	This module contains the code which implements the allocation and free
**	operations for the various control blocks which LK manages.
**	
**	In 6.4, lock blocks and resource blocks share the small block table, but
**	in 6.5 resource blocks are separated out into an RBK table. Note that
**	"large blocks", "small blocks" etc are really misnomers, since currently
**	the lock block is the larged block, and it's in the small block table.
**	Oh, well.
**
**	Routines:
**	    LK_allocate_cb
**	    LK_deallocate_cb
**	    expand_list
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Maintain statistics on control block allocations and frees.
**	    Track RSB's in the rbk_table.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	23-aug-1993 (andys)
**	    In LK_deallocate_cb, complain if we don't know what the type is. 
**	    This may help to find memory leaks.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Change %x to %p for pointer values in LK_deallocate_cb().
**	21-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Added semaphore initialization logic for RSB, and
**		  LLB semaphores.
**		- Serialize allocation and deallocation using lkd_sm_mutex.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**	06-Dec-1995 (jenjo02)
**	    Moved CSa_semaphore() inside of LK_imutex().
**	11-Dec-1995 (jenjo02)
**	    Replaced lkd_sm_mutex with one for each queue,
**	    lkd_rbk_mutex, lkd_sbk_mutex, lkd_lbk_mutex.
**	    Move accounting of lkd_llb_inuse, lkd_rsb_inuse, 
**	    lkd_lkb_inuse to this module instead of being scattered
**	    all over the place! Initialize lkb_type, rsb_type, 
**	    llb_type in LK_allocate_cb() instead of by caller.
**	    In LK_allocate_cb(), return with allocated cb's
**	    mutex locked. In LK_deallocate_cb(), assume that
**	    cb's mutex is locked (it ought to be if one is
**	    following the correct protocols) and unlock it
**	    before freeing the cb.
**	19-Dec-1995 (jenjo02)
**	    Instead of always getting 4080 bytes for a new block
**	    regardless of type (where'd that number come from?),
**	    get an even multiple of cb types, each allocation
**	    increasing the total by 2 .
**	03-Jan-1996 (jenjo02)
**	    When deallocating an RSB, don't clear rsb_id; this is
**	    a permanent id assigned when the RSB was initially
**	    allocated. Clearing it sometimes causes LKshow() fits
**	    when called from dm0p with a LK_S_SRESOURCE request to
**	    count servers attached to the BM, particularly after
**	    a less-than-graceful shutdown.
**	12-Jan-1996 (jenjo02)
**	    Assignment of unique LLB id now takes place in here
**	    when LLB is allocated under the protection of the
**	    lkb_mutex.
**	20-May-1996 (jenjo02)
**	    When deallocating an RSB, don't nullify grant and wait queues;
**	    leave them as empty lists. Similarly, when deallocating an 
**	    LLB, leave its lists empty, not nulled.
**	30-may-1996 (pchang)
**	    Reserved 10% of the SBK table resource for use by the recovery
**	    process. (B76879)
**	30-may-1996 (pchang)
**	    The change by jenjo02 on 19-Dec-1995 actually increases the total
**	    allocation of a list by threefold (i.e. lkd_?bk_count * (1 + 2))
**	    each time the list is expanded. The effect of this rapid escalation
**	    of the requested extent size on machine with modest locking
**	    configuration is undesirable in that it often results in the
**	    locking system reporting that it has run out of lock resources
**	    when in actual fact there is still sufficient memory left in the
**	    LGK extent to meet the immediate need of a server, if only the
**	    latter had requested a smaller extent size (B76880).
**	    Changed to increasing the allocation on each request by 1.5 times
**	    and reverted to allocating 8160 bytes extents for the recovery
**	    process.
**	20-Aug-1996 (jenjo02)
**	    In expand_list, expand by cb type rather than list type. Iteratively
**	    try for 2* current list size, halve that, try again, until we
**	    suceed or really run out of memory.
**	21-aug-1996 (rosga02)
**	    Initialized rsb->rsb_dlock to 0 after RSB allocation in
**	    expand_list(), to prevent infinite loop in lk_do_unlock().
**	    It happens in clustered VMS installation when allocated memory
**	    has garbage in this field. 
**	15-Oct-1996 (jenjo02)
**	    Moved incrementing of lkd_llb_inuse, lkd_rsb_inuse, and 
**	    lkd_lkb_inuse under the protection of the appropriate mutex.
**	    Without the mutex, odd-looking numbers show up in lockstat.
**	16-Oct-1996 (jenjo02)
**	    Prefixed semaphore names with "LK" to better identify them
**	    to the outside world.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    New structures introduced by keving for callbacks,
**	    CBT and CBACK, are allocated from RBK pool, initialized,
**	    and returned with their mutexes locked - deallocated
**	    in a similar fashion. 
**	15-Nov-1996 (jenjo02)
**	    Removed references to rsb_dlock, rsb_dlock_mutex.
**	21-Apr-1997 (jenjo02)
**	    Removed cback_mutex. There is really no need for it.
**	26-Aug-1998 (jenjo02)
**	    Added missing 3rd parm to ule_format of E_DMA024.
**	07-Jan-1999 (somsa01)
**	    In expand_list(), when we were supposed to halve the amount of
**	    memory requested, we never did. Thus, we were stuck in the while
**	    loop forever.  (Bug #94759)
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	02-Aug-1999 (jenjo02)
**	    Implement preallocated stash of LKBs/RSBs.
**	30-Nov-1999 (jenjo02)
**	    Relocated code to initialize LLB here from lkclist.c,
**	    added init of llb_dis_tran_id.
**	15-Dec-1999 (jenjo02)
**	    Removed llb_dis_tran_id, no longer needed.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	07-Nov-2001 (jenjo02)
**	    BUG 106306, mutex deadlock during scavenge(). When allocatin
**	    a new LKB, pass RSB in (unused) llb_tick so that mutex 
**	    deadlocks can be avoided.
**	    Also changed scavenge() to steal only as many LKB/RSBs 
**	    as are needed rather than all available.
**	28-Feb-2002 (jenjo02)
**	    Fix to 07-Nov fix. Don't fiddle with rsb_wait_count, instead
**	    notify caller that RSB was unmutexed.
**	29-Aug-2002 (devjo01)
**	    Minor changes to CBT & CBACK allocation and deallocation.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**      22-Apr-2005 (mutma03)
**        While deallocationg lkb, clear the lkb_type before rsb.
**        Function LK_show accesses rsb after checking lkb_type and
**        rsb is null sometimes causing exception.
**	01-Oct-2007 (jonj)
**	    Deleted alloc/dealloc/init of CBACK structure, now embedded in
**	    LKB.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

/*
** Internal list management routine:
*/
static i4 expand_list(i4 type, LLB *l);
static i4 scavenge( i4 type, i4 count, LLB *l, i4 *mutex_was_released );

/*
** Name: LK_allocate_cb	    -- Allocate an LK control block.
**
** Description:
**	This function allocates an LK control block of the specified type.
**
**	Control blocks are maintained on "small block" and "large block" lists
**	which are expanded as necessary up to pre-set limits. The first free
**	control block is removed from the appropriate list and returned.
**
**	A third list is the "rbk_table", which tracks resource blocks (RSBs).
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
**	24-may-1993 (bryanp)
**	    Track RSB's separately from LKBs.
**	    Maintain statistics on control block allocations and frees.
**	11-Dec-1995 (jenjo02)
**	    Replaced lkd_sm_mutex with one for each queue,
**	    lkd_rbk_mutex, lkd_sbk_mutex, lkd_lbk_mutex.
**	    Move accounting of lkd_llb_inuse, lkd_rsb_inuse, 
**	    lkd_lkb_inuse to this module instead of being scattered
**	    all over the place! Initialize lkb_type, rsb_type, 
**	    llb_type in LK_allocate_cb() instead of by caller.
**	12-Jan-1996 (jenjo02)
**	    Assignment of unique LLB id now takes place in here
**	    when LLB is allocated under the protection of the
**	    lkb_mutex.
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to enable the reservation of SBK table
**	    resource for use by the recovery process. (B76879)
**	15-Oct-1996 (jenjo02)
**	    Moved incrementing of lkd_llb_inuse, lkd_rsb_inuse, and 
**	    lkd_lkb_inuse under the protection of the appropriate mutex.
**	    Without the mutex, odd-looking numbers show up in lockstat.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    New structures introduced by keving for callbacks,
**	    CBT and CBACK, are allocated from RBK pool, initialized,
**	    and returned with their mutexes locked - deallocated
**	    in a similar fashion. Lists are now expanded by cb type
**	    rather than by list type.
**	21-Apr-1997 (jenjo02)
**	    Removed cback_mutex. There is really no need for it.
**	02-Aug-1999 (jenjo02)
**	    Implement preallocated stash of LKBs/RSBs.
**	21-Aug-2001 (jenjo02)
**	    Fixed LKB/RSB leaks in stash, improved stashing algorithm,
**	    added stash scavenging.
**	09-Apr-2002 (jenjo02)
**	    Changed rsb_was_released notification by scavenge(). 
**	16-May-2003 (jenjo02)
**	    Increment rsb_inuse, lkb_inuse incrementally rather
**	    than by rsbs_to_stash, lkbs_to_stash.
**      10-Dec-2004 (jammo01) b110851 INGSRV2503 issue#12925740
**          While incrementing an RSB, the lkb_highwater value
**          is not updated, which should actually be done,
**          keeping in mind the embedded LKB's inside RSB's.
**          Modified the code to update the lkb_highwater mark,
**          if required,while incrementing an RSB.The lkb_inuse
**          and lkb_highwater updates have been provided with
**          'lkd->lkd_rbk_mutex' cover in allocate, deallocate and
**          scavenge blocks, to avoid inconsistency.
**	23-Sep-2005 (jenjo02)
**	    Fixed above change which provoked mutex deadlocks:
**	    thread 1 takes (rbk_mutex, sbk_mutex), thread 2
**	    takes (sbk_mutex, rbk_mutex).
**	04-Mar-2008 (jonj)
**	    If cluster build, init llb_dist_stamp array.
*/
PTR
LK_allocate_cb(
i4  type,
LLB *l)
{
    PTR	    return_cb = (PTR)NULL;
    RSB	    *rsb;
    LLB	    *llb;
    LKB	    *lkb;
    CBT	    *cbt;
    LKD	    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    SIZE_TYPE cb_offset;
    STATUS  status;
    i4	    mutex_was_released = FALSE;
    i4      lkb_inuse_temp;

    lkd->lkd_stat.allocate_cb++;

    switch (type)
    {
	case RSB_TYPE:

	    LK_WHERE("LK_allocate_cb (RSB)")
	    /*	Allocate an RSB for this resource */

	    /* If the stash is empty, get some more RSBs from the free list */
	    if ( (cb_offset = l->llb_rsb_stash) == 0 )
	    {
		i4	rsbs_to_stash;
		SIZE_TYPE	stash = 0;

		/*
		** If first RSB allocation on this list, get the
		** average number of RSBs allocated by all lists,
		** otherwise allocate half again of the number of 
		** locks currently held by this list.
		*/
		if ( (rsbs_to_stash = l->llb_lkb_count / 2) == 0 )
		{
		    if ( lkd->lkd_stat.lists_that_locked )
			rsbs_to_stash = lkd->lkd_stat.rsb_alloc 
				      / lkd->lkd_stat.lists_that_locked;
		}
		
		if ( rsbs_to_stash < 2 )
		    rsbs_to_stash = 2;
		
		LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);

		do
		{
		    if ( (cb_offset = lkd->lkd_rbk_free) == 0 &&
			 (cb_offset = scavenge(RSB_TYPE, rsbs_to_stash, l, &mutex_was_released)) == 0 &&
			 (cb_offset = expand_list(RSB_TYPE, l)) == 0 )
		    {
			break;
		    }

		    rsb = (RSB *)LGK_PTR_FROM_OFFSET(cb_offset);
		    lkd->lkd_rbk_free = rsb->rsb_q_next;
		    rsb->rsb_q_next = stash;
		    stash = cb_offset;
		    lkd->lkd_rsb_inuse++;
		} while ( --rsbs_to_stash );

		if ( lkd->lkd_rsb_inuse > lkd->lkd_stat.rsb_highwater )
		    lkd->lkd_stat.rsb_highwater = lkd->lkd_rsb_inuse;

		/* Count "inuse" embedded LKBs towards hwm */
		if ( lkd->lkd_lkb_inuse + lkd->lkd_rsb_inuse
			> lkd->lkd_stat.lkb_highwater )
		{
		    lkd->lkd_stat.lkb_highwater =
			lkd->lkd_lkb_inuse + lkd->lkd_rsb_inuse;
	        }
		(VOID)LK_unmutex(&lkd->lkd_rbk_mutex);

		/* Point the LLB at the RSB stash */
		l->llb_rsb_stash = stash;

		/* Notify caller if RSH mutex was released/reacquired */
		if ( mutex_was_released )
		    l->llb_tick = 0;
	    }

	    /* Pull next RSB from the stash */
	    if ( (cb_offset = l->llb_rsb_stash) )
	    {
		rsb = (RSB *)LGK_PTR_FROM_OFFSET(cb_offset);

		l->llb_rsb_stash = rsb->rsb_q_next;
		/* Identify the LLB that did this allocation */
		rsb->rsb_alloc_llb = LGK_OFFSET_FROM_PTR(l);
		/* Note RSB highwater allocation by LLB */
		if ( l->llb_lkb_count + 1 > l->llb_rsb_alloc )
		    l->llb_rsb_alloc = l->llb_lkb_count + 1;
		LK_mutex(SEM_EXCL, &rsb->rsb_mutex);
		return_cb = (PTR)rsb;
	    }
	    break;

	case LKB_TYPE:
	    LK_WHERE("LK_allocate_cb (LKB)")
	    /*	Allocate an LKB for this lock block */

	    /* If the stash is empty, get some more LKBs from the free list */
	    if ( (cb_offset = l->llb_lkb_stash) == 0 )
	    {
		i4	lkbs_to_stash, stash = 0;

		/*
		** If first LKB allocation on this list, get the
		** average number of LKBs allocated by all lists,
		** otherwise allocate half again of the number of 
		** locks held by this list.
		*/
		if ( (lkbs_to_stash = l->llb_lkb_count / 2) == 0 )
		{
		    if ( lkd->lkd_stat.lists_that_locked )
			lkbs_to_stash = lkd->lkd_stat.lkb_alloc 
				      / lkd->lkd_stat.lists_that_locked;
		}
		
		if ( lkbs_to_stash < 2 )
		    lkbs_to_stash = 2;

		LK_mutex(SEM_EXCL, &lkd->lkd_sbk_mutex);

		lkb_inuse_temp = 0;
		do
		{
		    if ( (cb_offset = lkd->lkd_sbk_free) == 0 &&
			 (cb_offset = scavenge(LKB_TYPE, lkbs_to_stash, l, &mutex_was_released)) == 0 &&
			 (cb_offset = expand_list(LKB_TYPE, l)) == 0 )
		    {
			break;
		    }

		    lkb = (LKB *)LGK_PTR_FROM_OFFSET(cb_offset);
		    lkd->lkd_sbk_free = lkb->lkb_q_next;
		    lkb->lkb_q_next = stash;
		    stash = cb_offset;
		    lkb_inuse_temp++;
		} while ( --lkbs_to_stash );

		LK_unmutex(&lkd->lkd_sbk_mutex);

		LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);

		lkd->lkd_lkb_inuse += lkb_inuse_temp;
		/* Count "inuse" embedded LKBs towards hwm */
		if ( lkd->lkd_lkb_inuse + lkd->lkd_rsb_inuse 
			> lkd->lkd_stat.lkb_highwater )
		{
		    lkd->lkd_stat.lkb_highwater = 
			lkd->lkd_lkb_inuse + lkd->lkd_rsb_inuse;
		}

		(VOID)LK_unmutex(&lkd->lkd_rbk_mutex);

		/* Point the LLB at the LKB stash */
		l->llb_lkb_stash = stash;

		/* Notify caller if RSB mutex was released/reacquired */
		if ( mutex_was_released )
		    l->llb_tick = 0;
	    }

	    /* Pull next LKB from the stash */
	    if ( (cb_offset = l->llb_lkb_stash) )
	    {
		lkb = (LKB *)LGK_PTR_FROM_OFFSET(cb_offset);
		l->llb_lkb_stash = lkb->lkb_q_next;
		/* Identify the LLB that did the allocation */
		lkb->lkb_alloc_llb = LGK_OFFSET_FROM_PTR(l);
		/* Note LKB highwater allocation by this LLB */
		if ( l->llb_lkb_count + 1 > l->llb_lkb_alloc )
		    l->llb_lkb_alloc = l->llb_lkb_count + 1;
		return_cb = (PTR)lkb;
	    }
	    break;

	case LLB_TYPE:
	    LK_WHERE("LK_allocate_cb (LLB)")
	    /*	Allocate a new LLB. */

	    if ((status = LK_mutex(SEM_EXCL, &lkd->lkd_lbk_mutex)) == 0)
	    {
		if ((cb_offset = lkd->lkd_lbk_free) == 0 &&
		    (cb_offset = expand_list(LLB_TYPE, (LLB *)0)) == 0)
		{
		    (VOID)LK_unmutex(&lkd->lkd_lbk_mutex);
		    TRdisplay("LK_allocate_cb: expand_list failed (LLB)\n");
		    break;
		}

		llb = (LLB *)LGK_PTR_FROM_OFFSET(cb_offset);

		if ( CXcluster_enabled() )
		{
		    CXunique_id( (LK_UNIQUE*)llb->llb_name );
		}
		else
		{
		    /* Compose unique LLB identifier while holding lbk_mutex. */
		    llb->llb_name[0] = 0;
		    llb->llb_name[1] = ++lkd->lkd_next_llbname;
		}

		lkd->lkd_lbk_free = llb->llb_q_next;

		if (++lkd->lkd_llb_inuse > lkd->lkd_stat.llb_highwater)
		    lkd->lkd_stat.llb_highwater = lkd->lkd_llb_inuse;
		(VOID)LK_unmutex(&lkd->lkd_lbk_mutex);

		/* Lock the LLB, initialize as much as possible */
		(VOID) LK_mutex(SEM_EXCL, &llb->llb_mutex);

		/* Assign the CrossProcess IDentifier of the allocating thread */
		CSget_cpid(&llb->llb_cpid);

		llb->llb_llbq.llbq_next = llb->llb_llbq.llbq_prev = 
		    LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);
		llb->llb_lkb_wait.llbq_next = llb->llb_lkb_wait.llbq_prev = 
		    LGK_OFFSET_FROM_PTR(&llb->llb_lkb_wait.llbq_next);
		llb->llb_related_llb = 0;
		llb->llb_related_count = 0;
		llb->llb_lkb_count = 0;
		llb->llb_llkb_count = 0;
		llb->llb_search_count = 0;
		llb->llb_stamp = 0;
		llb->llb_tick = 0;
		llb->llb_gsearch_count = 0;
		llb->llb_status = 0;
		llb->llb_max_lkb = 0;

# if defined(conf_CLUSTER_BUILD)
		/* Init cluster deadlock stamp array */
		MEfill(sizeof(llb->llb_dist_stamp), 0, &llb->llb_dist_stamp);
# endif /* CLUSTER_BUILD */

		llb->llb_type = LLB_TYPE;
		return_cb = (PTR)llb;
	    }
	    break;

	case CBT_TYPE:
	    LK_WHERE("LK_allocate_cb (CBT)")
	    /*
	    ** Allocate a CBT from the rbk pool 
	    **
	    ** CBTs have their own free list
	    */

	    if ((status = LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex)) == 0)
	    {
		if ((cb_offset = lkd->lkd_cbt_free) == 0 &&
		    (cb_offset = expand_list(CBT_TYPE, (LLB *)0)) == 0)
		{
		    (VOID)LK_unmutex(&lkd->lkd_rbk_mutex);
		    TRdisplay("LK_allocate_cb: expand_list failed (CBT)\n");
		    break;
		}

		cbt = (CBT *)LGK_PTR_FROM_OFFSET(cb_offset);

		lkd->lkd_cbt_free = cbt->cbt_q_next;

		(VOID)LK_unmutex(&lkd->lkd_rbk_mutex);

		cbt->cbt_type = CBT_TYPE;
		return_cb = (PTR)cbt;
	    }
	    break;


	default:
#ifdef CB_DEBUG
	    TRdisplay("LK_allocate_cb: allocated cb %p has unknown type %d\n",
		      return_cb, type);
#endif
	    break;
    }

    LGK_VERIFY_ADDR( return_cb, 32 );
    return (return_cb);
}

/*
** Name: LK_deallocate_cb	    -- Deallocate an LK control block.
**
** Description:
**	This function returns an LK control block to its free list.
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
**	24-may-1993 (bryanp)
**	    Track RSB's separately from LKBs.
**	    Maintain statistics on control block allocations and frees.
**	23-aug-1993 (andys)
**	    Complain if we don't know what the type is. This may help to
**	    find memory leaks.
**	12-oct-1993 (tad)
**	    Bug #56449
**	    Change %x to %p for pointer values.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**	19-jul-1995 (canor01)
**	    Mutex granularization:
**		- acquire LKD mutex before calling LK_deallocate_cb()
**	06-Dec-1995 (jenjo02)
**	    Moved CSa_semaphore() inside of LK_imutex().
**	11-Dec-1995 (jenjo02)
**	    Replaced lkd_sm_mutex with one for each queue,
**	    lkd_rbk_mutex, lkd_sbk_mutex, lkd_lbk_mutex.
**	    Don't acquire LKD mutex before calling LK_deallocate_cb();
**	    let LK_deallocate_cb() protect these lists using these
**	    new semaphores instead.
**	    Move accounting of lkd_llb_inuse, lkd_rsb_inuse, 
**	    lkd_lkb_inuse to this module instead of being scattered
**	    all over the place!
**	    Unlock cb's mutex prior to freeing it.
**	03-Jan-1996 (jenjo02)
**	    When deallocating an RSB, don't clear rsb_id; this is
**	    a permanent id assigned when the RSB was initially
**	    allocated. Clearing it sometimes causes LKshow() fits
**	    when called from dm0p with a LK_S_SRESOURCE request to
**	    count servers attached to the BM, particularly after
**	    a less-than-graceful shutdown.
**      20-May-1996 (jenjo02)
**          When deallocating an RSB, don't nullify grant and wait queues;
**          leave them as empty lists. Similarly, when deallocating an 
**          LLB, leave its lists empty, not nulled.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    New structures introduced by keving for callbacks,
**	    CBT and CBACK, are allocated from RBK pool, initialized,
**	    and returned with their mutexes locked - deallocated
**	    in a similar fashion. Lists are now expanded by cb type
**	    rather than by list type.
**	21-Apr-1997 (jenjo02)
**	    Removed cback_mutex. There is really no need for it.
**	02-Aug-1999 (jenjo02)
**	    Implement preallocated stash of LKBs/RSBs.
**	06-Oct-1999 (jenjo02)
**	    When deallocating LBB, increment its instance.
**	21-Aug-2001 (jenjo02)
**	    Fixed LKB/RSB leaks in stash, improved stashing algorithm,
**	    added stash scavenging.
**      10-Dec-2004 (jammo01) b110851 INGSRV2503 issue#12925740
**          Added 'lkd->lkd_rbk_mutex' cover for updates to 
**          lkb_inuse value.
**	23-Sep-2005 (jenjo02)
**	    Fixed above change which provoked mutex deadlocks:
**	    thread 1 takes (rbk_mutex, sbk_mutex), thread 2
**	    takes (sbk_mutex, rbk_mutex).
**	01-Oct-2007 (jonj)
**	    Clear cback_fcn, cback_q_next when releasing LKB.
**	15-Oct-2007 (jonj)
**	    Ok to clear cback_fcn, but not cback_q_next - let
**	    lkcback functions manage that.
*/
VOID
LK_deallocate_cb( i4 type, PTR cb, LLB *l )
{
    LKD	    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LKB	    *lkb;
    RSB	    *rsb;
    LLB	    *llb;
    CBT	    *cbt;
    i4	    i;
    STATUS  status;
    SIZE_TYPE	    cb_offset;


    lkd->lkd_stat.deallocate_cb++;

    LGK_VERIFY_ADDR( cb, 32 );

    switch (type)
    {
	case LKB_TYPE:
	    LK_WHERE("LK_deallocate_cb (LKB)")

	    /*	Deallocate the LKB. */

	    lkb = (LKB *)cb;

	    /* Clobber just enough to make the LKB unrecognizable */
	    lkb->lkb_type = 0;
	    lkb->lkb_lkbq.lkbq_rsb = 0;
	    lkb->lkb_lkbq.lkbq_llb = 0;
	    lkb->lkb_cback.cback_fcn = (LKcallback)NULL;

	    /* Return LKB to LLB's stash */
	    if ( (lkb->lkb_attribute &= LKB_IS_RSBS) == 0 )
	    {
		if ( l && lkb->lkb_alloc_llb == LGK_OFFSET_FROM_PTR(l)
		       && lkd->lkd_sbk_free )
		{
		    lkb->lkb_q_next = l->llb_lkb_stash;
		    l->llb_lkb_stash = LGK_OFFSET_FROM_PTR(lkb);
		}
		else
		{
		    /*
		    ** LLB not know, was not the allocator, 
		    ** or free list is empty, 
		    ** return to free list.
		    */
		    LK_mutex(SEM_EXCL, &lkd->lkd_sbk_mutex);
		    lkb->lkb_q_next = lkd->lkd_sbk_free;
		    lkd->lkd_sbk_free = LGK_OFFSET_FROM_PTR(lkb);
		    LK_unmutex(&lkd->lkd_sbk_mutex);
		    LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);
		    lkd->lkd_lkb_inuse--;
		    LK_unmutex(&lkd->lkd_rbk_mutex);
		}
	    }

	    break;

	case RSB_TYPE:
	    LK_WHERE("LK_deallocate_cb (RSB)")

	    /*	Deallocate the RSB. */

	    rsb = (RSB *)cb;

	    /* Clobber just enough to make the RSB unrecognizable */
	    rsb->rsb_name.lk_type = 0;
	    rsb->rsb_type = 0;

	    /* Return RSB to LLB's stash */
	    if ( l && rsb->rsb_alloc_llb == LGK_OFFSET_FROM_PTR(l)
		   && lkd->lkd_rbk_free )
	    {
		rsb->rsb_q_next = l->llb_rsb_stash;
		l->llb_rsb_stash = LGK_OFFSET_FROM_PTR(rsb);
	    }
	    else
	    {
		/*
		** LLB not know, was not the allocator, 
		** or free list is empty, 
		** return to free list.
		*/
		LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);
		rsb->rsb_q_next = lkd->lkd_rbk_free;
		lkd->lkd_rbk_free = LGK_OFFSET_FROM_PTR(rsb);
		lkd->lkd_rsb_inuse--;
		(VOID)LK_unmutex(&lkd->lkd_rbk_mutex);
	    }

	    (VOID)LK_unmutex(&rsb->rsb_mutex);

	    break;

	case LLB_TYPE:
	    LK_WHERE("LK_deallocate_cb (LLB)")

	    /*  Deallocate the LLB. */

	    llb = (LLB *)cb;

	    llb->llb_q_prev = 0;
	    llb->llb_type = 0;
	    llb->llb_id.id_instance++;

	    /* Return spare stashed RSBs/LKBs to free list */
	    if ( cb_offset = llb->llb_rsb_stash )
	    {
		/* Find last stashed RSB */
		for ( i = 0; cb_offset; cb_offset = rsb->rsb_q_next, i++)
		    rsb = (RSB *)LGK_PTR_FROM_OFFSET(cb_offset);

		/* Link them all back into the free list */
		LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);
		rsb->rsb_q_next = lkd->lkd_rbk_free;
		lkd->lkd_rbk_free = llb->llb_rsb_stash;
		lkd->lkd_rsb_inuse -= i;
		llb->llb_rsb_stash = 0;
		LK_unmutex(&lkd->lkd_rbk_mutex);
	    }

	    if ( cb_offset = llb->llb_lkb_stash )
	    {
		/* Find last stashed LKB */
		for ( i = 0; cb_offset; cb_offset = lkb->lkb_q_next, i++)
		    lkb = (LKB *)LGK_PTR_FROM_OFFSET(cb_offset);

		/* Link them all back into the free list */
		LK_mutex(SEM_EXCL, &lkd->lkd_sbk_mutex);
		lkb->lkb_q_next = lkd->lkd_sbk_free;
		lkd->lkd_sbk_free = llb->llb_lkb_stash;
		llb->llb_lkb_stash = 0;
		LK_unmutex(&lkd->lkd_sbk_mutex);
		LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);
		lkd->lkd_lkb_inuse -= i;
		LK_unmutex(&lkd->lkd_rbk_mutex);
	    }

	    (VOID)LK_unmutex(&llb->llb_mutex);

	    /* Return LLB to free list */
	    LK_mutex(SEM_EXCL, &lkd->lkd_lbk_mutex);
	    llb->llb_q_next = lkd->lkd_lbk_free;
	    lkd->lkd_lbk_free = LGK_OFFSET_FROM_PTR(llb);
	    lkd->lkd_llb_inuse--;
	    if (llb->llb_rsb_alloc || llb->llb_lkb_alloc)
	    {
		lkd->lkd_stat.lists_that_locked++;
		lkd->lkd_stat.rsb_alloc += llb->llb_rsb_alloc;
		lkd->lkd_stat.lkb_alloc += llb->llb_lkb_alloc;
		llb->llb_rsb_alloc = llb->llb_lkb_alloc = 0;
	    }
	    LK_unmutex(&lkd->lkd_lbk_mutex);

	    break;

	case CBT_TYPE:
	    LK_WHERE("LK_deallocate_cb (CBT)")

	    /*	Deallocate the CBT. */

	    cbt = (CBT *)cb;

	    cbt->cbt_q_prev = 0;
	    cbt->cbt_cback_list = 0;
	    cbt->cbt_type = 0;

	    if ((status = LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex)) == 0)
	    {
		cbt->cbt_q_next = lkd->lkd_cbt_free;
		lkd->lkd_cbt_free = LGK_OFFSET_FROM_PTR(cbt);
		(VOID)LK_unmutex(&lkd->lkd_rbk_mutex);
	    }

	    break;

	default:
#ifdef CB_DEBUG
	    TRdisplay("LK_deallocate_cb: freed cb %p has unknown type %d\n",
			    cb, type);
#endif
	    break;
    }

    return;
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
**      type                            The cb type to expand.
**	l				The caller's LLB
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
**	24-may-1993 (bryanp)
**	    Track RSB's separately from LKBs.
**	21-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Added semaphore initialization logic for RSB and
**		  LLB semaphores.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**	06-Dec-1995 (jenjo02)
**	    Moved CSa_semaphore() inside of LK_imutex().
**	19-Dec-1995 (jenjo02)
**	    Instead of always getting 4080 bytes for a new block
**	    regardless of type (where'd that number come from?),
**	    get an even multiple of cb types, each allocation
**	    increasing the total by 1.5 .
**	30-may-1996 (pchang)
**	    Added LLB as a parameter to enable the reservation of 10% of the
**	    SBK table resource for use by the recovery process. (B76879)
**	30-may-1996 (pchang)
**	    The above change by jenjo02 actually increases the total allocation
**	    of a list by threefold (i.e. lkd_?bk_count * (1 + 2)) each time
**	    the list is expanded.  The effect of this rapid escalation of the
**	    requested extent size on machine with modest locking configuration
**	    is undesirable in that it often results in the locking system
**	    reporting that it has run out of lock resources when in actual fact
**	    there is still sufficient memory left in the LGK extent to meet the
**	    immediate need of a server, if only the latter had requested a
**	    smaller extent size (B76880).
**	    Changed to increasing the allocation on each request by 1.5 times
**	    and reverted to allocating 8160 bytes extents for the recovery
**	    process.
**	20-Aug-1996 (jenjo02)
**	    In expand_list, expand by cb type rather than list type. Iteratively
**	    try for 2* current list size, halve that, try again, until we
**	    suceed or really run out of memory.
**	21-aug-1996 (rosga02)
**	    Initialized rsb->rsb_dlock to 0 after RSB allocation to prevent 
**	    infinite loop in lk_do_unlock(). It happens in clustered VMS
**	    installation when allocated memory has garbage in this field. 
**	01-aug-1996 (nanpr01 for jenjo02)
**	    New structures introduced by keving for callbacks,
**	    CBT and CBACK, are allocated from RBK pool, initialized,
**	    and returned with their mutexes locked - deallocated
**	    in a similar fashion. 
**	15-Nov-1996 (jenjo02)
**	    Removed references to rsb_dlock, rsb_dlock_mutex.
**	21-Apr-1997 (jenjo02)
**	    Removed cback_mutex. There is really no need for it.
**	26-Aug-1998 (jenjo02)
**	    Added missing 3rd parm to ule_format of E_DMA024.
**	07-Jan-1999 (somsa01)
**	    When we were supposed to halve the amount of memory requested,
**	    we never did. Thus, we were stuck in the while loop forever.
**	    (Bug #94759)
**	02-Aug-1999 (jenjo02)
**	    Initialize LKB embedded in RSB.
**	    Just get memory for 32 structures instead of 8160 / sizeof(structure).
**	21-Aug-2001 (jenjo02)
**	    Ensure that number of RSBs allocated does not exceed max
**	    number of LKBs.
**	09-sep-2002 (devjo01)
**	    Insure that lkb_cback_off is zero'ed when expanding list.
**	16-May-2003 (jenjo02)
**	    Check and adjust count -before- asking for memory to 
**	    avoid waste and 0-count allocations.
**	07-Apr-2005 (jenjo02)
**	    Set count of new objects after objects have
**	    all been initialized.
**	13-Jan-2006 (jenjo02)
**	    If RSB allocation fails, don't forget to release
**	    the SBK mutex.
**	29-Sep-2009 (thaju02) (B122672)
**	    Specify object type in E_CL102D.
*/
static i4
expand_list(
i4  type,
LLB *l)
{
    LKD                 *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB			*llb;
    LKB			*lkb;
    RSB			*rsb;
    CBT                 *cbt;
    PTR			items;
    i4		item_size;
    i4		sbk_size;
    i4		count, i, j;
    SIZE_TYPE		*sbk_table;
    SIZE_TYPE		*lbk_table;
    SIZE_TYPE		*rbk_table;
    i4		err_code;
    char		sem_name[CS_SEM_NAME_LEN];

    LK_WHERE("expand_list")

    switch (type)
    {
        case LKB_TYPE:
	     if (l && (l->llb_status & (LLB_MASTER | LLB_RECOVER)))
	     {
	    	 sbk_size = lkd->lkd_sbk_size;

	         if (lkd->lkd_sbk_count >= lkd->lkd_sbk_size)
	         {
		    uleFormat(NULL, E_CL102F_LK_EXPAND_LIST_FAILED, (CL_ERR_DESC *)NULL,
			  ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code,
			  2, 0, lkd->lkd_sbk_size, 0, lkd->lkd_sbk_count);
		   return (0);
	         }
	         item_size = sizeof(LKB);
		 count = lkd->lkd_sbk_count
		       ? lkd->lkd_sbk_count
		       : 32;
	     }
	     else
	     {
	         /*
	         ** Reserve 10% of the SBK table for use by processes
	         ** aborting transaction or doing recovery
	         */
	         sbk_size = lkd->lkd_sbk_size * 0.9;

	         if (lkd->lkd_sbk_count >= sbk_size)
	         {
		    uleFormat(NULL, E_CL102F_LK_EXPAND_LIST_FAILED, (CL_ERR_DESC *)NULL,
			  ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code,
			  3, 0, lkd->lkd_sbk_size, 0, lkd->lkd_sbk_count,
			  0, lkd->lkd_sbk_size - lkd->lkd_sbk_count);
		    return (0);
	         }
	         item_size = sizeof(LKB);
		 count = lkd->lkd_sbk_count
		       ? lkd->lkd_sbk_count
		       : 32;
	      }
	      if (count > sbk_size - lkd->lkd_sbk_count)
		  count = sbk_size - lkd->lkd_sbk_count;
	      break;
        case CBT_TYPE:
        case RSB_TYPE:
	     if (lkd->lkd_rbk_count == lkd->lkd_rbk_size)
	     {
	         uleFormat(NULL, E_DMA024_LK_EXPAND_LIST_FAILED, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code,
			3, 0, lkd->lkd_rbk_size, 0, lkd->lkd_rbk_count,
		        0, lkd->lkd_rbk_size - lkd->lkd_rbk_count);
	         return (0);
	     }
	     item_size = sizeof(RSB);
	     count = lkd->lkd_rbk_count
		   ? lkd->lkd_rbk_count
		   : 32;
	     if ( type == RSB_TYPE )
	     {
		 /* Must also mutex SBK as we adjust sbk_count */
		 LK_mutex(SEM_EXCL, &lkd->lkd_sbk_mutex);
		 /* Number of RSBs cannot be allowed to exceed SBK */
	         if ( count > lkd->lkd_sbk_size - lkd->lkd_sbk_count )
		     count = lkd->lkd_sbk_size - lkd->lkd_sbk_count;
	     }
	     if (count > lkd->lkd_rbk_size - lkd->lkd_rbk_count)
	         count = lkd->lkd_rbk_size - lkd->lkd_rbk_count;
	     break;

        case LLB_TYPE:
	     if (lkd->lkd_lbk_count == lkd->lkd_lbk_size)
	     {
	         uleFormat(NULL, E_CL1030_LK_EXPAND_LIST_FAILED, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code,
			2, 0, lkd->lkd_lbk_size, 0, lkd->lkd_lbk_count);
	         return (0);
	     }
	     item_size = sizeof(LLB);
	     count = lkd->lkd_lbk_count
		   ? lkd->lkd_lbk_count
		   : 32;
	     if (count > lkd->lkd_lbk_size - lkd->lkd_lbk_count)
	         count = lkd->lkd_lbk_size - lkd->lkd_lbk_count;
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
	    /* If RSB, release SBK mutex before error */
	    if ( type == RSB_TYPE )
		LK_unmutex(&lkd->lkd_sbk_mutex);

	    uleFormat(NULL, E_CL102D_LK_EXPAND_LIST_FAILED, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code,
			5, 0, type, 3, (type == LKB_TYPE ? "LKB" :
                        type == RSB_TYPE ? "RSB" : type == LLB_TYPE ?
                        "LLB" : "CBT"),
			0, lkd->lkd_lbk_count, 0, lkd->lkd_sbk_count,
			0, lkd->lkd_rbk_count);
	    return (0);
	}
    } while (items == NULL);
 
    if ( items )
	items = (PTR)((char *)items + sizeof(LGK_EXT));
 
    switch (type)
    {
        case LKB_TYPE:

	    if ( items )
	    {
		lkb = (LKB *)items;
		sbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_sbk_table);
		lkd->lkd_sbk_allocated++;
		if (lkd->lkd_sbk_allocated > lkd->lkd_stat.sbk_highwater)
		  lkd->lkd_stat.sbk_highwater = lkd->lkd_sbk_allocated;

		i = lkd->lkd_sbk_count;
		while ( count-- )
		{
		    lkb->lkb_type = 0;
		    lkb->lkb_attribute = 0;
		    lkb->lkb_id = ++i;
		    sbk_table[i] = LGK_OFFSET_FROM_PTR(lkb);
		    lkb->lkb_q_next = lkd->lkd_sbk_free;
		    lkd->lkd_sbk_free = LGK_OFFSET_FROM_PTR(lkb);
		    lkb->lkb_lkbq.lkbq_next = 0;
		    lkb->lkb_lkbq.lkbq_prev = 0;
		    lkb->lkb_cback.cback_fcn = (LKcallback)NULL;
		    lkb->lkb_cback.cback_q_next = 0;
		    lkb->lkb_cback.cback_posts = 0;
		    lkb++;
		}
		lkd->lkd_sbk_count = i;
	    }

	    return (lkd->lkd_sbk_free);

        case RSB_TYPE:
        case CBT_TYPE:

	    if ( items )
	    {
		rsb = (RSB *)items;
		rbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_rbk_table);
		sbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_sbk_table);

		lkd->lkd_rbk_allocated++;
		if (lkd->lkd_rbk_allocated > lkd->lkd_stat.rbk_highwater)
		    lkd->lkd_stat.rbk_highwater = lkd->lkd_rbk_allocated;

		i = lkd->lkd_rbk_count;
		j = lkd->lkd_sbk_count;
		while ( count-- )
		{
		    rbk_table[++i] = LGK_OFFSET_FROM_PTR(rsb);
		    switch (type)
		    {
			case RSB_TYPE :
			    rsb->rsb_q_next = lkd->lkd_rbk_free;
			    lkd->lkd_rbk_free = LGK_OFFSET_FROM_PTR(rsb);
			    rsb->rsb_type = 0;
			    rsb->rsb_id = i;
			    LK_imutex(&rsb->rsb_mutex,
			      STprintf(sem_name, "LK RSB mutex %8d", i));

			    /* Initialize the LKB embedded in the RSB */
			    lkb = &rsb->rsb_lkb;
			    lkb->lkb_type = 0;
			    lkb->lkb_attribute = LKB_IS_RSBS;
			    lkb->lkb_id = ++j;
			    sbk_table[j] = LGK_OFFSET_FROM_PTR(lkb);
			    lkb->lkb_q_next = 0;
			    lkb->lkb_lkbq.lkbq_next = 0;
			    lkb->lkb_lkbq.lkbq_prev = 0;
			    lkb->lkb_cback.cback_fcn = (LKcallback)NULL;
			    lkb->lkb_cback.cback_q_next = 0;
			    lkb->lkb_cback.cback_posts = 0;
			    break;

			case CBT_TYPE :
			    cbt = (CBT *)rsb;
			    cbt->cbt_q_next = lkd->lkd_cbt_free;
			    lkd->lkd_cbt_free = LGK_OFFSET_FROM_PTR(cbt);
			    cbt->cbt_type = 0;
			    break;
		    }
		    rsb++;
		} /* while ( count-- ) */
		lkd->lkd_rbk_count = i;
		lkd->lkd_sbk_count = j;
	    } /* if ( items ) */

            if (type == RSB_TYPE)
	    {
		LK_unmutex(&lkd->lkd_sbk_mutex);
                return (lkd->lkd_rbk_free);
	    }
	    return (lkd->lkd_cbt_free);

        case LLB_TYPE:

	    if ( items )
	    {
		llb = (LLB *)items;
		lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
		lkd->lkd_lbk_allocated++;
		if (lkd->lkd_lbk_allocated > lkd->lkd_stat.lbk_highwater)
		    lkd->lkd_stat.lbk_highwater = lkd->lkd_lbk_allocated;
		i = lkd->lkd_lbk_count;
		while ( count-- )
		{
		    llb->llb_type = 0;
		    llb->llb_id.id_instance = 0;
		    llb->llb_id.id_id = ++i;
		    llb->llb_rsb_stash = llb->llb_lkb_stash = 0;
		    llb->llb_rsb_alloc = llb->llb_lkb_alloc = 0;
		    lbk_table[i] = LGK_OFFSET_FROM_PTR(llb);
		    llb->llb_q_next = lkd->lkd_lbk_free;
		    lkd->lkd_lbk_free =  LGK_OFFSET_FROM_PTR(llb);
		    if (LK_imutex(&llb->llb_mutex,
			STprintf(sem_name, "LK LLB mutex %8d", i)))
			return(0);
		    llb++;
		}
		lkd->lkd_lbk_count = i;
	    }

	    return (lkd->lkd_lbk_free);
    }
}

/*{
** Name: scavenge	- Scavenge stashed RSBs/LKBs
**
** Description:
**
**	Called when the RSB/LKB free list is empty before calling
**	expand_list().
**
**	Steals stashed RSBs/LKBs from LLBs and returns them to
**	the free queue(s).
**
** Inputs:
**      type                            The cb type needed.
**	count				how many
**	l				caller's LLB
**
** Outputs:
**	mutex_was_released		TRUE if mutex was
**					released, reacquired.
**	Returns:
**	    NULL			No RSB/LKBs could be freed up.
**	    !NULL			Offset to object.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	21-Aug-2001 (jenjo02)
**	    Created.
**	18-Sep-2001 (jenjo02)
**	    Skip LLB's with LLB_ALLOC status. They're in the process
**	    of (de)allocating a lock/resource and want their stashes
**	    left alone. They also hold a variety of mutexes which
**	    we don't want to deadlock with.
**	07-Nov-2001 (jenjo02)
**	    BUG 106306, mutex deadlock during scavenge(). When allocatin
**	    a new LKB, pass RSB in (unused) llb_tick so that mutex 
**	    deadlocks can be avoided.
**	    Free up only as many RSB/LKBs as needed instead of all
**	    available (which sort of defeats the idea of a stash).
**	28-Feb-2002 (jenjo02)
**	    Fix to 07-Nov fix. Don't fiddle with rsb_wait_count, instead
**	    notify caller that RSB was unmutexed. Artificially changing
**	    rsb_wait_count can lead to no locks being granted.
**	09-Apr-2002 (jenjo02)
**	    Changed rsb_was_released notification by scavenge(), which
**	    may be called multiple times. If llb_tick is reset on the
**	    first call, we end up holding the rsb_mutex but fail to
**	    release it again on subsequent probes into scavenge().
**	12-Nov-2002 (jenjo02)
**	    Similar deadlock problem with RSH/RSB mutexes. Generalized
**	    "llb_tick" notifier to include RSH/RSB mutexes also.
**	16-May-2003 (jenjo02)
**	    Plugged free RSB/LKB leak.
**      10-Dec-2004 (jammo01) b110851 INGSRV2503 issue#12925740
**          Added 'lkd->lkd_rbk_mutex' cover for updates to 
**          lkb_inuse value.
**	23-Sep-2005 (jenjo02)
**	    Fixed above change which provoked mutex deadlocks:
**	    thread 1 takes (rbk_mutex, sbk_mutex), thread 2
**	    takes (sbk_mutex, rbk_mutex).
**	08-Jan-2008 (jonj)
**	    CS_SEMAPHORE is now LK_SEMAPHORE to LK.
*/
static i4
scavenge(
i4	type,
i4	count,
LLB	*l,
i4	*mutex_was_released)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB		*llb;
    LKB		*lkb, *last_lkb = (LKB*)NULL;
    RSB		*rsb, *last_rsb = (RSB*)NULL;
    SIZE_TYPE	*lbk_table = (SIZE_TYPE*)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
    i4		i;
    SIZE_TYPE	offset; 
    i4		free_list = 0;
    i4		freed = 0;
    LK_SEMAPHORE *sem = NULL;

    /*
    ** Caller must hold rbk_mutex (type == RSB) 
    ** or sbk_mutex (type == LKB)
    */

    /* Release RBK/SBK mutex to avoid deadlocks with LLB mutexes */
    if ( type == RSB_TYPE )
	LK_unmutex(&lkd->lkd_rbk_mutex);
    else if ( type == LKB_TYPE )
	LK_unmutex(&lkd->lkd_sbk_mutex);

    /*
    ** Find LLBs with stashed RSBs/LKBs.
    **
    ** Remove as many objects as we need from LLB's stash and 
    ** add them to the local LIFO free_list, remembering the
    ** first object placed on the list (which will be last when
    ** we're done).
    */
    for ( i = 1; 
	  i <= lkd->lkd_lbk_count && freed < count;
	  i++ )
    {
	llb = (LLB*)LGK_PTR_FROM_OFFSET(lbk_table[i]);

	/*
	** Skip LLBs in LLB_ALLOC state and those that
	** don't have what we're looking for.
	*/
	if ( llb->llb_type == LLB_TYPE &&
	    (llb->llb_status & LLB_ALLOC) == 0 &&
	     ( (type == RSB_TYPE && llb->llb_rsb_stash) ||
	       (type == LKB_TYPE && llb->llb_lkb_stash) )
	   ) 
	{
	    /*
	    ** To preclude RSB<->LLB mutex deadlocks when allocating
	    ** LKBs we must first release the RSB mutex before 
	    ** acquiring an LLB mutex.
	    ** Before leaving this function we'll reacquire the RSB 
	    ** mutex and notify the caller by setting llb_tick to zero.
	    **
	    ** The LKB allocation code in LK_request_lock() stuck
	    ** the corresponding RSB into "llb_tick".
	    */
	    if ( !sem && l->llb_tick )
	    {
		sem = (LK_SEMAPHORE*)LGK_PTR_FROM_OFFSET(l->llb_tick);
		LK_unmutex(sem);
	    }

	    LK_mutex(SEM_EXCL, &llb->llb_mutex);

	    /* Recheck LLB after mutex wait */
	    if ( llb->llb_type == LLB_TYPE &&
		(llb->llb_status & LLB_ALLOC) == 0 )
	    {
		if ( type == RSB_TYPE )
		{
		    while ( (offset = llb->llb_rsb_stash) && freed < count )
		    {
			rsb = (RSB*)LGK_PTR_FROM_OFFSET(offset);
			llb->llb_rsb_stash = rsb->rsb_q_next;
			rsb->rsb_q_next = free_list;
			free_list = offset;
			if ( freed++ == 0 )
			    last_rsb = rsb;
		    }
		}
		else if ( type == LKB_TYPE )
		{
		    while ( (offset = llb->llb_lkb_stash) && freed < count )
		    {
			lkb = (LKB*)LGK_PTR_FROM_OFFSET(offset);
			llb->llb_lkb_stash = lkb->lkb_q_next;
			lkb->lkb_q_next = free_list;
			free_list = offset;
			if ( freed++ == 0 )
			    last_lkb = lkb;
		    }
		}
	    }

	    LK_unmutex(&llb->llb_mutex);
	}
    }

    /* Reacquire mutex */
    if ( sem )
    {
	LK_mutex(SEM_EXCL, sem);
	/* Notify caller that object may be suspect */
	*mutex_was_released = TRUE;
    }

    /*
    ** Relock the RBK/SBK, add scavenged RSB/LKBs to
    ** the appropriate free list, return first freed object.
    */
    if ( type == RSB_TYPE )
    {
	LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);

	if ( freed )
	{
	    last_rsb->rsb_q_next = lkd->lkd_rbk_free;
	    lkd->lkd_rbk_free = free_list;
	    lkd->lkd_rsb_inuse -= freed;
	}
	return( lkd->lkd_rbk_free );
    }
    if ( type == LKB_TYPE )
    {
	if ( freed )
	{
	    LK_mutex(SEM_EXCL, &lkd->lkd_rbk_mutex);
	    lkd->lkd_lkb_inuse -= freed;
	    LK_unmutex(&lkd->lkd_rbk_mutex);
	    LK_mutex(SEM_EXCL, &lkd->lkd_sbk_mutex);
	    last_lkb->lkb_q_next = lkd->lkd_sbk_free;
	    lkd->lkd_sbk_free = free_list;
	}
	else
	    LK_mutex(SEM_EXCL, &lkd->lkd_sbk_mutex);

	return( lkd->lkd_sbk_free );

    }
    return(0);
}
