/*
** Copyright (c) 1992, 2005 Ingres Corporation
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <me.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>

#ifdef VMS
#include    <ssdef.h>
#include    <lib$routines.h>
#endif
/**
**
**  Name: LKINIT.C - Implements the LKinitialize function of the locking system
**
**  Description:
**	This module contains the code which implements LKinitialize.
**	
**	    LKinitialize -- Prepare this process for using the locking system
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	7-oct-1992 (bryanp)
**	    Error handling improvements. Call LKmo_attach_lk().
**	24-may-1993 (bryanp)
**	    Set LK_my_pid when initializing LK.
**	    Break rundown handling into LK_rundown and LK_do_rundown, with
**		the latter being called by the CSP in a cluster environment.
**	21-jun-1993 (bryanp)
**	    If the CSP isn't alive, don't try to ask it to run rundown.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Include <tr.h>
**	26-jul-1993 (jnash)
**	    Add zero flag param to LGK_initialize.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems, attach shared sems.
**	16-aug-1995 (duursma)
**	    For debugging purposes, set lkrequest_id to -3 in LK_rundown()
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass mutex address.
**              - Removed LK_mutex() and LK_unmutex() calls from LK_rundown
**                and LK_do_rundown().
**              - In LK_do_rundown() acquire LLB mutex for shared LLB's.
**		- In LK_do_rundown() acquire RSB mutex when processing RSB.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	19-jul-1995 (canor01)
**	    Mutex Granularity Project
**		- Acquire LKD mutex before calling LK_cancel_event_wait()
**		  or LK_deallocate_cb()
**	16-aug-1995 (duursma)
**	    For debugging purposes, set lkrequest_id to -3 in LK_rundown()
**	06-Dec-1995 (jenjo02)
**	    Move CSa_semaphore() inside LK_imutex().
**	    Define lkd_ew_q_mutex to block event wait queue instead of
**	    using lkd_mutex.
**	11-Dec-1995 (jenjo02)
**	    Replaced lkd_sm_mutex with lkd_rbk_mutex,
**	    lkd_sbk_mutex, lkd_lbk_mutex.
**	    No longer a need to acquire LKD mutex before calling
**	    LK_deallocate_cb() or LK_cancel_event_wait().
**	    Maintenance of lkd_llb|rsb|lkb_inuse moved to 
**	    LK_allocate|deallocate_cb().
**	14-May-1996 (jenjo02)
**	    Added lkd_dlock_mutex to single-thread deadlock().
**	21-Aug-1996 (boama01)
**	    Moved csp_debug GLOBALDEF here from DMFCSP.C so that LK can also
**	    use the switch;  initialized in LKinitialize() (and also in
**	    DMFCSP).
**	08-Oct-1996 (jenjo02)
**	    Added lkd_mutex to protect lkd_rqst_id.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Initialize lkd_cbt_q_mutex.
**	23-sep-1996 (canor01)
**	    Moved global data definitions to lkdata.c.
**	16-Nov-1996 (jenjo02)
**	    Initialize lkd_dw_next, prev, lkd_dw_q_mutex for Deadlock
**	    Detection thread.
**	06-Dec-1996 (jenjo02)
**	    Removed lkd_mutex.
**      27-feb-1997 (stial01)
**          LK_do_rundown() Release LK_SVDB lock list only if it is the last 
**          lock list remaining for a dying process.
**          Moved code to free a lock list to free_lock_list()
**	08-Apr-1997 (jenjo02)
**	    Removed lkd_cbt_next/prev, lkd_cbt_q_mutex. Each server has but one
**	    CBT, referenced thru a globaldef, so there's no need for a queue of
**	    them or a mutex with which to protect it.
**	07-May-1997 (jenjo02)
**	    In LK_do_rundown(), release RSB mutex before calling LK_do_cancel()
**	    lest a mutex deadlock ensue.
**	16-Nov-1998 (jenjo02)
**	  o Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	  o Installed macros to use ReaderWriter locks when CL-available
**	    for blocking the deadlock matrix.
**	08-Feb-1999 (jenjo02)
**	    LLB can contain more than one waiting LKB.
**	02-Aug-1999 (jenjo02)
**	    Added LLB parm to LK_deallocate_cb() prototype.
**	22-Oct-1999 (jenjo02)
**	    Deleted lkd_dlock_lock.
**	15-Dec-1999 (jenjo02)
**	    Cleaned up LK_do_rundown() in support of SHARED log
**	    transactions which are associated with PROTECTed
**	    SHARED lock lists which must be recoverable.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Replaced CScp_resume() with LKcp_resume macro.
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	    - remove init of LK_my_pid.
**	    - collapse LK_do_shutdown into LK_shutdown.
**	17-sep-2003 (abbjo03)
**	    Changes to use VMS headers provided by HP.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	26-may-2005 (devjo01)
**	    Set lkd_csid.
**	21-jun-2005 (devjo01)
**	    - Have LKinitialize tell CX some useful facts. OK if non-clustered.
**	    - Set LKD_NO_DLM_GLC if clustered, and no group lock container.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**	10-Dec-2008 (jonj)
**	    SIR 120874: Remove last vestiges of CL_SYS_ERR,
**	    old form uleFormat.
**	11-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** Following switch moved here from DMFCSP.C so that LK routines can benefit
** from CSP tracing setting as well. (boama01, 08/21/96)
*/
GLOBALREF  i4	csp_debug;

static VOID free_lock_list(
			LLB *llb,
			LLB **shared_llb,
			LLB **related_llb);


/*{
** Name: LKinitialize  - Initialize the LK lock database
**
** Description:
**      This routine is called once from the interface code to initialize
**      the LK database. It must be called prior to any other LK calls.
**
**	This routine calls LGK_initialize() to connect this process to the
**	LG/LK shared memory segment (this may have already occurred if
**	LGinitialize has already been called).
**
**	Depending on the return from LGK_initialize, we may need to initialize
**	the LKD structure.
**
** Inputs:
**	none.
**
** Outputs:
**	err_code				system error.
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Set LK_my_pid when initializing LK.
**	26-jul-1993 (jnash)
**	    Add zero flag param to LGK_initialize.
**      17-May-1994 (daveb)
**          attach LK semaphore, always.
**	06-Dec-1995 (jenjo02)
**	    Move CSa_semaphore() inside LK_imutex().
**	21-Aug-1996 (boama01)
**	    Initialize new csp_debug global to 0 (always).
**	21-jun-2005 (devjo01)
**	    Have LK tell CX some useful facts.  OK if non-clustered.
**	15-May-2007 (toumi01)
**	    For supportability add process info to shared memory.
**	09-Jan-2009 (jonj)
**	    LKREQ structure deleted, lkb_cpid replaces lkb_lkreq.cpid.
**	 6-Aug-2009 (wanfr01)
**	    Bug 122418 - Lockstat requires attaching to a valid
**	    shared memory segment
*/
STATUS
LKinitialize(CL_ERR_DESC *sys_err, char *lgk_info)
{
    STATUS	status;
    i4	err_code;
    LKD		*lkd;
    i4     flag;

    if (!STcompare(lgk_info,"lockstat"))
      flag = LOCK_LGK_MUST_ATTACH;
    else
        flag = 0;
    if (status = LGK_initialize(flag, sys_err, lgk_info))
    {
	if (status != E_DMA812_LGK_NO_SEGMENT)
	    uleFormat(NULL, status, (CL_ERR_DESC *)sys_err, ULE_LOG,
			NULL, (char *)0, 0L, (i4 *)0, &err_code, 0);
	return(E_DMA013_LK_BAD_INIT);
    }

    lkd = (LKD *)LGK_base.lgk_lkd_ptr;
 
    CXalter( CX_A_LK_FORMAT_FUNC, (PTR)LKkey_to_string );
    /* Wacky casts are to keep compilers happy.
    ** Basically, we're passing a structure offset thru a generic
    ** PTR-sized cxalter parameter.  The CX proxy-offset is an
    ** i4 so explicitly clip to that, even though not really needed.
    */
    CXalter( CX_A_PROXY_OFFSET, (PTR)(i4)((SCALARP)(&((LKB *)0)->lkb_cpid)) );

    if (status = LKmo_attach_lk())
    {
	uleFormat(NULL, status, (CL_ERR_DESC *)NULL, ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
	return(E_DMA014_LK_INIT_MO_ERROR);
    }

    /*
    ** Initialize CSP debug trace switch here for all servers that link LK
    ** routines in.  DMFCSP will subsequently set this switch appropriately
    ** for its own process (based on logical).
    */
    csp_debug = FALSE;

    return (OK);
}

/*
** Name: LK_meminit	    - one time only memory initialization of LK memory
**
** Description:
**	This routine is called when the LK shared memory is first created. It
**	fills in the various LKD fields.
**
** Inputs:
**	None
**
** Outputs:
**	sys_err		    - system specific error code.
**
** Returns:
**	STATUS
**
** History:
**	24-may-1993 (bryanp)
**	    Added function header comments.
**      17-May-1994 (daveb) 59127
**          Fixed semaphore leaks, named sems, attach shared sems.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Changed LK_xmutex() calls to pass mutex address.
**		- Added lkd_type initialization.
**	11-Dec-1995 (jenjo02)
**	    Replaced lkd_sm_mutex with lkd_rbk_mutex,
**	    lkd_sbk_mutex, lkd_lbk_mutex. Added a bunch more
**	    list/queue-specific mutexes to relieve the load
**	    on lkd_mutex.
**	14-May-1996 (jenjo02)
**	    Added lkd_dlock_mutex to single-thread deadlock().
**	08-Oct-1996 (jenjo02)
**	    Added lkd_mutex to protect lkd_rqst_id.
**      01-aug-1996 (nanpr01 for ICL keving)
**          Initialise queue of CBT descriptors.
**	01-aug-1996 (nanpr01 for jenjo02)
**	    Initialize lkd_cbt_q_mutex.
**	08-Apr-1997 (jenjo02)
**	    Removed lkd_cbt_next/prev, lkd_cbt_q_mutex. Each server has but one
**	    CBT, referenced thru a globaldef, so there's no need for a queue of
**	    them or a mutex with which to protect it.
**	08-jul-1997 (canor01)
**	    The lkd_llb_q_mutex was being initialized twice.
**	21-jun-2005 (devjo01)
**	    Set LKD_NO_DLM_GLC if clustered, and no group lock container.
**	04-Apr-2008 (jonj)
**	    Set LKD_NO_ULOCKS if VMS, it doesn't support update locks.
*/
STATUS
LK_meminit(CL_ERR_DESC *sys_err)
{
    LKD		*lkd;
    STATUS	status;

    lkd = (LKD *)LGK_base.lgk_lkd_ptr;

    MEfill(sizeof(LKD), (u_char) 0, (PTR) lkd);

    lkd->lkd_type = LKD_TYPE;

    if (status = LK_imutex(&lkd->lkd_rbk_mutex,"LK rbk mutex"))
	return (status);
    if (status = LK_imutex(&lkd->lkd_sbk_mutex,"LK sbk mutex"))
	return (status);
    if (status = LK_imutex(&lkd->lkd_lbk_mutex,"LK lbk mutex"))
	return (status);
    if (status = LK_imutex(&lkd->lkd_llb_q_mutex,"LK llb q mutex"))
	return (status);
    if (status = LK_imutex(&lkd->lkd_ew_q_mutex,"LK ew q mutex"))
	return (status);
    if (status = LK_imutex(&lkd->lkd_dw_q_mutex,"LK deadlock q mutex"))
	return (status);
    if (status = LK_imutex(&lkd->lkd_stall_q_mutex,"LK stall q mutex"))
	return (status);
    if (status)
	return(status);

    lkd->lkd_llb_next = lkd->lkd_llb_prev =
			    LGK_OFFSET_FROM_PTR(&lkd->lkd_llb_next);

    lkd->lkd_ew_next = lkd->lkd_ew_prev =
			    LGK_OFFSET_FROM_PTR(&lkd->lkd_ew_next);

    lkd->lkd_dw_next = lkd->lkd_dw_prev =
			    LGK_OFFSET_FROM_PTR(&lkd->lkd_dw_next);

    lkd->lkd_stall_next = lkd->lkd_stall_prev =
			    LGK_OFFSET_FROM_PTR(&lkd->lkd_stall_next);
                                                                    
    if (CXcluster_enabled())
    {
	lkd->lkd_csid = CXnode_number(NULL);
	lkd->lkd_status |= LKD_CLUSTER_SYSTEM;
	if ( !CXconfig_settings( CX_HAS_DLM_GLC ) )
	{
	    lkd->lkd_status |= LKD_NO_DLM_GLC;
	}
#ifdef VMS
	/* VMS DLM does not support Update locks */
	lkd->lkd_status |= LKD_NO_ULOCKS;
#endif 
    }


    return (OK);
}

/*{
** Name: LK_rundown	- Rundown LK for a process.
**
** Description:
**      This routine is called from the interface code when a process
**	using LK exits.
**
**	// If this is a cluster environment, then the actual rundown processing
**	// needs to be done by the CSP, rather than by the server/rcp, since
**	// the CSP was the process which actually took out the locks.
**
**	// We don't perform any cross-process synchronization, however; we queue
**	// a request to the CSP to perform rundown processing and then just
**	// return -- the CSP will get around to the rundown processing when 
**	// it can.
**
**	Above paragraph no longer true as of 103715.
**
** Inputs:
**      pid                             Process ID.
**
** Outputs:
**	Returns:
**	    None
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
**	    Break rundown handling into LK_rundown and LK_do_rundown, with
**		the latter being called by the CSP in a cluster environment.
**	21-jun-1993 (bryanp)
**	    If the CSP isn't alive, don't try to ask it to run rundown.
**	16-aug-1995 (duursma)
**	    For debugging purposes, set lkrequest_id to -3
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Removed LK_mutex() and LK_unmutex() calls against LKD mutex.
**	16-aug-1995 (duursma)
**	    For debugging purposes, set lkrequest_id to -3
**	13-jul-2001 (devjo01)
**	    Collapse LK_do_rundown into LK_rundown, since we no longer
**	    need to send a request to CSP under clusters.
*/
/*{
** Name: LK_rundown	- Rundown LK for a process.
**
** Description:
**	 As of s103715, this has been merged into its LK_shutdown wrapper.
**
** Inputs:
**      lkreq                           Pointer to LKREQ.
**
** Outputs:
**	Returns:
**	    None
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Moved LK_mutex() and LK_unmutex() calls against LKD mutex.
**              - Acquire LLB lock when deallocating a shared LLB.
**		- Acquire RSB mutex when processing RSB.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	11-Dec-1995 (jenjo02)
**	    Protect event wait queue with its own mutex (lkd_ew_q_mutex)
**	    rather than rely on blocking effects of lkd_mutex.
**	16-Nov-1996 (jenjo02)
**	    Ifdef'd code which check for an LLB already on the event wait
**	    queue. This code must have been originally placed to track
**	    a rare bug, one I've never encountered. Cleaned up code to 
**	    ensure that the event status bits are maintained correctly.
**      27-feb-1997 (stial01)
**          LK_do_rundown() Release LK_SVDB lock list only if it is the last 
**          lock list remaining for a dying process.
**          Moved code to free a lock list to free_lock_list()
**	07-May-1997 (jenjo02)
**	    In LK_do_rundown(), release RSB mutex before calling LK_do_cancel()
**	    lest a mutex deadlock ensue.
**	15-Dec-1999 (jenjo02)
**	    Cleaned up LK_do_rundown() in support of SHARED log
**	    transactions which are associated with PROTECTed
**	    SHARED lock lists which must be recoverable.
**	13-jul-2001 (devjo01)
**	    Collapse LK_do_rundown into LK_rundown, since we no longer
**	    need to send a request to CSP under clusters.
**	28-Feb-2002 (jenjo02)
**	    LLB_WAITING is contrived from the value of llb_waiters; test
**	    llb_waiters instead of the flag, which will never be on.
**	20-Jul-2006 (jonj)
**	    After getting llb_mutex, make sure we still have a legitimate
**	    LLB_TYPE.
**	13-Mar-2007 (jonj)
**	    Check llb_waiters!0 when cancelling lock waiters, JIC the DBMS
**	    died while the llb wait list was being updated.
**	19-Mar-2007 (jonj)
**	    Add IsRundown parm to LK_cancel_event_wait()
**	29-Oct-2007 (jonj)
**	    Add more sanity checks, be aware that mutexes may be 
**	    still owned by dead processes, skip doing stuff if so
**	    as the underlying structures must be considered inconsistent.
*/
VOID
LK_rundown(i4 pid)
{
    LKD		*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB		*llb;
    LLB		*shared_llb, *related_llb;
    LKB		*lkb;
    RSB		*rsb;
    LKB		*next_lkb;
    LKB		*wait_lkb;
    i4		i;
    SIZE_TYPE	lkb_offset, last_offset;
    SIZE_TYPE	end_offset;
    SIZE_TYPE	llb_offset;
    LLBQ	*llbq;
    LLB		*owning_llb;
    LLB		*prev_llb;
    LLB		*next_llb;
    STATUS	status;
    CL_ERR_DESC	sys_err;
    SIZE_TYPE	*lbk_table;
    i4		err_code;
    LLB         *xsvdb_llb = NULL;
    i4     	llb_count = 0;

    LK_WHERE("LK_rundown")

    /* Only release the NONPROTECT llb. */

    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);

    for (i = 1; i <= lkd->lkd_lbk_count; i++)
    {
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]);

	if ( llb && llb->llb_type == LLB_TYPE && llb->llb_cpid.pid == pid )
	{
	    /* If we find a XSVDB server lock list, remember it */
	    if ((llb->llb_status & LLB_XSVDB))
	    {
		xsvdb_llb = llb;
	    }

	    llb_count++;

	    /*
	    ** If shared lock list, then skip.
	    ** These are dealt with when all connections to them go away.
	    */
	    if (llb->llb_status & LLB_SHARED)
		continue;

	    /*
	    ** Loop in case we find a another lock list connected to
	    ** this one to deallocate.  If this is the last connection to
	    ** a shared list, then we will loop through this again to
	    ** release or orphan the shared llb. If this is the last reference
	    ** to a related lock list, then we will loop through again to
	    ** release or orphan the related llb.
	    */
	    shared_llb = NULL;
	    related_llb = NULL;

	    while ( llb )
	    {
		/* If mutex owner is dead, ignore this LLB */
		if ( (LK_mutex(SEM_EXCL, &llb->llb_mutex)) == E_CS0029_SEM_OWNER_DEAD )
		    break;

		if ( llb->llb_type == LLB_TYPE )
		{
		    /*	Cancel any event wait. */

		    (VOID) LK_cancel_event_wait( llb, lkd, TRUE, &sys_err);

		    /*	Cancel all lock waits. */
		    last_offset = 0;

		    llb_offset = LGK_OFFSET_FROM_PTR(llb);

		    while ( llb->llb_waiters > 0 &&
		           llb->llb_lkb_wait.llbq_next != 
			       LGK_OFFSET_FROM_PTR(&llb->llb_lkb_wait.llbq_next) )
		    {
			llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llb->llb_lkb_wait.llbq_next);
			lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llb_wait));
			lkb_offset = LGK_OFFSET_FROM_PTR(lkb);
			/* Watch for endless broken list */
			if ( lkb_offset == last_offset )
			    break;
			last_offset = lkb_offset;
			/* Sanity-check LKB */
			if ( lkb->lkb_lkbq.lkbq_llb == llb_offset &&
			     lkb->lkb_lkbq.lkbq_rsb )
			{
			    if ( (LK_do_cancel(&lkb_offset)) == E_CS0029_SEM_OWNER_DEAD )
				break;
			}
			else
			{
			    /* LKB trashed, ignore it */
			    llb->llb_waiters--;
			}
		    }

		    /*
		    ** Release all locks for non-protected lists.
		    ** Don't release the XSVDB lock list unless all other
		    ** lock lists are released.
		    */
		    if ((llb->llb_status & LLB_NONPROTECT)
			&& (llb->llb_related_count == 0)
			&& ((llb->llb_status & LLB_XSVDB) == 0) )
		    {
			free_lock_list(llb, &shared_llb, &related_llb);

			/*
			** Decrement llb_count if we counted this llb.
			*/
			if (llb <= (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]))
			    llb_count--;
		    }
		    else
		    {
			llb->llb_status |= LLB_ORPHANED;
			(VOID) LK_unmutex(&llb->llb_mutex);
		    }
		}
		else
		    (VOID) LK_unmutex(&llb->llb_mutex);

		/* Free shared list then related lock list, if either */
		if ( llb = shared_llb )
		    shared_llb = NULL;
		else if ( llb = related_llb )
		    related_llb = NULL;
	    }
	}
	else if (llb &&
		 llb->llb_type == LLB_TYPE &&
		 llb->llb_status & LLB_RECOVER &&
		 llb->llb_waiters
		)
	{
	    /*
	    ** Check if the resource that RCP is waiting for is held by 
	    ** the exiting process.
	    */
	    /* RCP can only be waiting on a single LKB at a time */
	    llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llb->llb_lkb_wait.llbq_next);
	    wait_lkb = (LKB *)((char *)llbq - CL_OFFSETOF(LKB,lkb_llb_wait));
	    rsb = (RSB *)LGK_PTR_FROM_OFFSET(wait_lkb->lkb_lkbq.lkbq_rsb);

	    if ( rsb && (LK_mutex(SEM_EXCL, &rsb->rsb_mutex)) == OK )
	    {
		end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
	     
		for (lkb_offset = rsb->rsb_grant_q_next;
			lkb_offset != end_offset;
			lkb_offset = next_lkb->lkb_q_next)
		{
		    next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);
		    owning_llb = (LLB *)
			    LGK_PTR_FROM_OFFSET(next_lkb->lkb_lkbq.lkbq_llb);

		    if (owning_llb->llb_cpid.pid == pid)
		    {
			LKcp_resume(&llb->llb_cpid);
			/*
			** Release the RSB. LK_do_cancel() needs to acquire the
			** mutex, so we can't be holding it.
			*/
			(VOID)LK_unmutex(&rsb->rsb_mutex);
			(VOID)LK_do_cancel(&lkb_offset);
			rsb = (RSB *)NULL;
			break;
		    }
		}
		if (rsb)
		    (VOID)LK_unmutex(&rsb->rsb_mutex);
	    }
	}
    }

    if ( xsvdb_llb && llb_count == 1 )
    {
	if ( (LK_mutex(SEM_EXCL, &xsvdb_llb->llb_mutex)) == OK )
	    free_lock_list(xsvdb_llb, &shared_llb, &related_llb);
    }

    return;
}

/*
**	01-jul-2008 (smeke01)
**	    Fixed compilation error when xEVENT_DEBUG specified.
**	09-Jan-2009 (jonj)
**	    Add boolean llbIsMutexed to LK_do_unlock() prototype.
*/
static VOID
free_lock_list(LLB *llb, LLB **shared_llb, LLB **related_llb)
{
    LKD         *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LKB		*lkb;
    RSB		*rsb;
    i4	i;
    SIZE_TYPE	end_offset;
    SIZE_TYPE	llb_offset;
    LLBQ	*last_llbq;
    LLB		*sllb, *rllb;
    LLB		*prev_llb;
    LLB		*next_llb;
    STATUS	status;
    CL_ERR_DESC	sys_err;

    llb_offset = LGK_OFFSET_FROM_PTR(llb);

    end_offset = LGK_OFFSET_FROM_PTR(&llb->llb_llbq.llbq_next);
    while (llb->llb_llbq.llbq_next != end_offset)
    {
	/*
	**  Move backwards from the LLBQ in the LKB to the
	**  beginning of the LKB.
	*/

	last_llbq = (LLBQ *)
		LGK_PTR_FROM_OFFSET(llb->llb_llbq.llbq_prev);
	lkb = (LKB *)((char *)last_llbq - 
		CL_OFFSETOF(LKB,lkb_llbq));
	/* Sanity check LKB */
	if ( lkb->lkb_lkbq.lkbq_llb == llb_offset &&
		 lkb->lkb_lkbq.lkbq_rsb )
	{
	    /*  Unlock this LKB. */
	    /*  "TRUE:" we hold llb_mutex */
	    if ( (LK_do_unlock(lkb, (LK_VALUE *) 0, LK_DO_UNLOCK_RUNDOWN, TRUE))
			== E_CS0029_SEM_OWNER_DEAD )
	    {
		break;
	    }
	}
    }

    /* Deallocate the LLB. */

    /*
    ** If this references a shared lock list, then decrement
    ** the count of connections to that lock list.  If the
    ** count goes to zero and it's a NONPROTECT lock list
    ** (not needed by recovery), then deallocate the shared list
    ** (we will reexecute the loop with shared_llb).
    ** If the shared list count goes to zero and its a
    ** protected list (needed by recovery), then just un-SHARE
    ** the list and let recovery have it.
    */

    if (llb->llb_status & LLB_PARENT_SHARED)
    {
	sllb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);
	if ( sllb 
	     && sllb->llb_type == LLB_TYPE 
	     && sllb->llb_status & LLB_SHARED
	     && (LK_mutex(SEM_EXCL, &sllb->llb_mutex)) == OK )
	{
	    if ( --sllb->llb_connect_count <= 0 )
	    {
		sllb->llb_status &= ~LLB_SHARED;
		*shared_llb = sllb;
	    }
	    (VOID) LK_unmutex(&sllb->llb_mutex);
	}
    }

    /*
    ** If this list references a related lock list, then 
    ** decrement the count of references to the related list.
    ** If the count goes to zero, return a pointer to 
    ** the related list; it'll be deallocated if it's
    ** NONPROTECT, ORPHANED otherwise.
    */
    if ( llb->llb_related_llb )
    {
	rllb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_related_llb);
	if ( rllb 
	     && rllb->llb_type == LLB_TYPE
	     && (LK_mutex(SEM_EXCL, &rllb->llb_mutex)) == OK )
	{
	    if ( --rllb->llb_related_count <= 0 )
		*related_llb = rllb;
	    (VOID)LK_unmutex(&rllb->llb_mutex);
	}
    }

    if ( (LK_mutex(SEM_EXCL, &lkd->lkd_llb_q_mutex)) == OK )
    {
	next_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_q_next);
	prev_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_q_prev);
	next_llb->llb_q_prev = llb->llb_q_prev;
	prev_llb->llb_q_next = llb->llb_q_next;

	(VOID) LK_unmutex(&lkd->lkd_llb_q_mutex);
    }

#ifdef xEVENT_DEBUG
    if (LK_llb_on_list(llb, lkd))
    {
	i4 err_code;
	uleFormat(NULL, E_DMA016_LKEVENT_LIST_ERROR,
		    (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL,
		    &err_code, 1,
		    0, llb->llb_id.id_id);
	TRdisplay(
	    "ERROR: Lock list %d still on event list!\n",
	    llb->llb_id.id_id);
#ifdef VMS
	lib$signal(SS$_DEBUG);
#endif
    }
#endif

    /*
    ** Free the llb. This also releases the llb_mutex.
    **
    ** Decrement llb_count if we counted this llb.
    */
    LK_deallocate_cb(llb->llb_type, (PTR)llb, llb);
}
