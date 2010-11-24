/*
** Copyright (c) 1992, 2005 Ingres Corporation
**
NO_OPTIM=dr6_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <pc.h>
#include    <tr.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <st.h>
#include    <ulf.h>
#include    <lg.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>
#include    <dmf.h>
#include    <dmccb.h>
#include    <dm.h>

/**
** Name: LKCBACK.C  - The LK Blocking Callback Thread 
**
** Description:
**    
**      lk_callback()                 -  The blocking callback processor
**      lk_cback_init()               -  Init callback data structures
**	lk_cback_post()		      -  Queue up one callback
**      lk_cback_put()                -  Allocate, fill, & queue one callback
**					 for a non-distributed lock.
**      lk_cback_fire()               -  Search granted queue firing clashing 
**                                       non-distributed LKB's callbacks
**
** History:
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Created. Required by DMCM project.
**      01-aug-1996 (nanpr01 for jenjo02)
**          Defined new mutexi for cbt queue, cbt, and cback. LK_allocate_cb()
**          and LK_deallocate_cb() changed to recognize CBT_TYPE and CBACK_TYPE
**          instead of pretending they're RSBs.
**          Removed SCF references, which was only being used to update
**          the IIMONITOR comment line. SCF isn't available when IPM is
**          linked.
**          Added code to save lock key at time that a callback is fired.
**          Between the time of the firing and the invocation of the callback
**          code by the callback thread, the LKB may have been released and
**          in use by another resource. Using only the lock_id as a handle
**          will cause the callback code to operate on a resource/lock that
**          is was not registered for; adding the lock key lets the callback
**          function check that the lock it is being called with is in fact
**          the one it was fired for.
**	09-nov-96 (mcgem01)
**	    Global data moved to lkdata.c
**	09-Apr-1997 (jenjo02)
**	  - Rewrote lk_callback() to incorporate lk_cback_register(),
**	    lk_cback_get().
**	  - Added new stats for callback threads.
**	  - Removed lkd_cbt_next/prev, lkd_cbt_q_mutex, cback_mutex,
**	    none of which are needed.
**	  - Moved some function prototypes to lkdef.h
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	02-Aug-1999 (jenjo02)
**	    Added LLB parm to LK_deallocate_cb() prototype.
**	06-Oct-1999 (jenjo02)
**	    LK_ID changed to u_i4 from structure containing only a u_i4.
**	    Affects lkb_id, rsb_id.
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2000 (jenjo02)
**	    Replaced CScp_resume() with LKcp_resume macro.
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	09-sep-2002 (devjo01)
**	    Add LK_TRACE_CALLBACK_ENABLED stuff, new functions 
**	    lk_cback_alloc, lk_cback_dealloc, lk_cback_post.
**	    Change management of CBACK queue to a singly linked
**	    list updated by compare & swap, to avoid the need
**	    for mutex protection.
**	17-nov-2003 (devjo01)
**	    Replace CScas_long calls with CScas4.
**	25-jan-2004 (devjo01)
**	    For non-CAS platforms resurrect cbt_mutex, et.al.
**	13-feb-2004 (devjo01)
**	    Remove need to atomically update cbt_triggered.
**	07-Sep-2004 (mutma03)
**	    Added code to call CSsuspend_for_AST on linux clusters.
**      7-oct-2004 (thaju02)
**          Change offset to SIZE_TYPE.
**	20-jul-2005 (devjo01)
**	    'cx_key' is now a structure.
**	17-oct-2005 (devjo01) b114660.
**	    Add use of a pair of local retry queues to lk_callback.
**	12-Feb-2007 (jonj)
**	    Reinstate atomic update of cbt_triggered, add more
**	    LK_CALLBACK_TRACE messages for debugging.
**	22-Feb-2007 (jonj)
**	    Change first LK_CALLBACK_TRACE in lk_cback_post to not emit a
**	    TRdisplay.
**	01-Oct-2007 (jonj)
**	    Delete CBACK structure. LKB has everything we need to store
**	    callback info in situ, described by LKB-embedded _CBACK.
**	    Deprecated lk_cback_alloc(), lk_cback_dealloc() functions.
**	15-Oct-2007 (jonj)
**	    Expand trace buffer to 2048 entries.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
**      21-oct-2010 (joea)
**          Add a diagnostic trace in lk_cback_post if cback_posts != 1.
**          In lk_callback, fix the counting of callback posts.
*/

/*
**  Definition of static variables and forward static functions.
*/

/* Store the id of this process' callback thread descriptor */ 
GLOBALREF SIZE_TYPE LK_mycbt; 

GLOBALREF char LK_compatible[];

FUNC_EXTERN	STATUS		lk_cback_init(void);
FUNC_EXTERN	DB_STATUS	lk_callback(DMC_CB *dmc_cb);

# if defined(LK_TRACE_CALLBACK_ENABLED)

/****************************************************************\
**								**
**	CONDITIONALLY COMPILED LK CALLBACK TRACE DIAGS.		**
**								**
\****************************************************************/

# define LK_CBTRACE_ENTRY_MAX	2048

typedef struct _LK_CBTRACE_ENTRY	LK_CBTRACE_ENTRY;

struct _LK_CBTRACE_ENTRY
{
    i4		trace_location;
    LK_LOCK_KEY	trace_key;     
};

i4			Lk_Callback_Trace_Count = 0;

LK_CBTRACE_ENTRY	Lk_Callback_Traces[LK_CBTRACE_ENTRY_MAX] ZERO_FILL;


/*
** Function registered with iimonitor to perform trace dump.
**
** Beware, on an active system, start of trace may wind up dumping
** one or more entries added to circular trace buffer after initial
** 'ti' was calculated.  Trace will issue a warning if this may
** have happened.
*/
static i4
lk_callback_debug( i4 (*output_fcn)(PTR, i4, char *), char *command )
{
    LK_CBTRACE_ENTRY	*ptr;
    u_i4		 ti, et;
    char		 buf[132], keybuf[128];
    CBT 		*cbt = (CBT *)LGK_PTR_FROM_OFFSET(LK_mycbt);

    /* Right now, only one function: dump the circular trace array */
    if ( ( et = Lk_Callback_Trace_Count ) < LK_CBTRACE_ENTRY_MAX )
    {
	ti = 0; 
    }
    else
    {
	ti = et - LK_CBTRACE_ENTRY_MAX;
    }
    while ( ti < et )
    {
	ptr = Lk_Callback_Traces + ( ti % LK_CBTRACE_ENTRY_MAX );

	TRformat( output_fcn, (PTR)1, buf, sizeof(buf), "%d: %d %s", 
		  ti, ptr->trace_location,
		  LKkey_to_string(&ptr->trace_key, keybuf) );
	ti++;
    }
    if ( et != Lk_Callback_Trace_Count )
    {
	TRformat( output_fcn, (PTR)1, buf, sizeof(buf),
		  "WARNING: end of trace moved to %d while trace was printed",
		  Lk_Callback_Trace_Count );
    }
    /* Show stats accumulated thus far */
    TRformat( output_fcn, (PTR)1, buf, sizeof(buf), "%x CBT triggered %d list %d wakeup %d cbacks %d stale %d\n",
		    cbt->cbt_cpid.sid,
		    cbt->cbt_triggered, cbt->cbt_cback_list,
		    cbt->cbt_stat.wakeups, cbt->cbt_stat.cbacks,
		    cbt->cbt_stat.stale );
    return OK;
}

/*
** Guts of an enabled LK_CALLBACK_TRACE macro
**
** location	0 = Callback processing thread
**		1 = AST invokation.
*/
void
lk_callback_do_trace( i4 location, LK_LOCK_KEY *key )
{
    LK_CBTRACE_ENTRY	*ptr;
    u_i4		 ti;

    ti = CSadjust_counter( &Lk_Callback_Trace_Count, 1 );
    ptr = Lk_Callback_Traces + ( (ti - 1) % LK_CBTRACE_ENTRY_MAX );
    ptr->trace_location = location;
    ptr->trace_key = *key;
    if (location >= 90)
	TRdisplay("LK_CALLBACK_TRACE %d:%4.4{ %x%}\n",location,key);
}

# endif /* defined(LK_TRACE_CALLBACK_ENABLED) */


/*{
**
** Name: lk_callback	    -- Special thread which processes callbacks 
**                             for this process.
**
** Description:
**	This routine implements the LK blocking callback thread. One runs in 
**      each server. When woken it retrieves and executes the callbacks 
**      queued by LK for this process. When they have all been processed it
**      re-suspends.
**
**	If all goes well, this routine will not return under normal
**	circumstances until server shutdown time.
**
** Inputs:
**     dmc_cb
** 	.type		    Must be set to DMC_CONTROL_CB.
** 	.length		    Must be at least sizeof(DMC_CB).
**
** Outputs:
**     dmc_cb
** 	.error.err_code	    Error code, if any
**
** Returns:
**     E_DB_OK
**     E_DB_FATAL
**
** History:
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Created. Required for DMCM project.
**      01-aug-1996 (nanpr01 for jenjo02)
**	    Added code to save lock key at time that a callback is fired.
**	    Between the time of the firing and the invocation of the callback
**	    code by the callback thread, the LKB may have been released and
**	    in use by another resource. Using only the lock_id as a handle
**	    will cause the callback code to operate on a resource/lock that
**	    is was not registered for; adding the lock key lets the callback
**	    function check that the lock it is being called with is in fact
**	    the one it was fired for. Augmented the callback function call
**	    to pass the address of the key.
**	09-Apr-1997 (jenjo02)
**	    Rewrote lk_callback() to incorporate lk_cback_register(),
**	    lk_cback_get().
**	    Added new stats for callback threads.
**	29-aug-2002 (devjo01)
**	    Removed use of cbt_mutex, and now manage CBACK list
**	    as a singly linked list updated using atomic C&S
**	    operations.   This allows posting a callback from a
**	    signal handler/AST.
**	13-feb-2004 (devjo01)
**	    Tweak use of cbt_triggered to avoid need to use
**	    atomic operations when updating it, yet still
**	    preclude any chance that lk_callback will suspend
**	    with a pending callback in the queue.
**	17-oct-2005 (devjo01) b114660.
**	    Add use of a pair of local retry queues.  Under high contention,
**	    it was possible to have a scheduling deadlock where we'd
**	    block on a callback.  To avoid this, rather than having our
**	    callback block, in the cases where we see a page marked busy,
**	    we defer processing that particular callback until the page
**	    is clear.  During any one cycle, one rety queue is filled,
**	    while the other is emptied.   If any retries are required,
**	    thread will suspend for a maximum of one second.
**	12-Feb-2007 (jonj)
**	    Reinstate atomic update of cbt_triggered, add more
**	    LK_CALLBACK_TRACE messages for debugging.
**	01-Oct-2007 (jonj)
**	    CBACK embedded in LKB, no longer separately allocated
**	    structure.
**	23-Oct-2007 (jonj)
**	    New CBACK field, cback_posts, counts the number of callbacks
**	    that have fired since the CBACK was put on the CBT list.
**	    Reprocess the callback until that number is zero.
**	04-Apr-2008 (jonj)
**	    When doing "retry queue", increment cbt_triggered so 
**	    that we don't get resumed by lk_cback_post() while
**	    we are actually running.
**	02-Sep-2008 (jonj)
**	    If cback_cbt_id is no longer LK_mycbt, ignore this
**	    callback.
*/

DB_STATUS
lk_callback(DMC_CB	    *dmc_cb)
{
    LKD             *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    CBT             *cbt;
    CBACK      	    *cback;
    RSB		    *rsb;
    LKB		    *lkb;
    i4		    err_code;
    DB_STATUS	    status = E_DB_OK;
    STATUS          cl_status = OK;
    SIZE_TYPE       cback_offset, prev_offset, next_offset;
    SIZE_TYPE	    retry_queues[2];
    i4		    oddoreven = 0;
    i4		    suspmask = CS_INTERRUPT_MASK;
    i4		    timeout = 0;
    i4		    posts;
    LK_LKID	    CbackLockid;
    LK_LOCK_KEY	    DebugKey;
    bool            cback_is_valid;

    /* Find the callback thread descriptor from offset */
    cbt = (CBT *)LGK_PTR_FROM_OFFSET(LK_mycbt);

    if (!cbt || cbt->cbt_type != CBT_TYPE)
    {
	uleFormat(NULL, E_CL106A_LK_CBACK_REG_BADCBT, (CL_ERR_DESC *)NULL,
                   ULE_LOG, NULL, 0, 0, 0, &err_code, 1,
                   0, cbt->cbt_type);
        return(E_CL1062_LK_CBACK_GET_FAILED);
    }

    CSget_cpid(&cbt->cbt_cpid);

    TRdisplay("%@ Starting %s LK Blocking Callback Thread %x,%x CBT %x\n",
		(CXcluster_enabled()) ? "Clustered" : "",
		cbt->cbt_cpid.pid, cbt->cbt_cpid.sid, LK_mycbt);

    retry_queues[0] = retry_queues[1] = 0;

    /* For debugging suspend/resume */
    DebugKey.lk_type = LK_SS_EVENT;
    DebugKey.lk_key1 = 0;
    DebugKey.lk_key2 = 0;
    DebugKey.lk_key3 = 0;
    DebugKey.lk_key4 = 0;
    DebugKey.lk_key5 = 0;
    DebugKey.lk_key6 = 0;
    LK_CALLBACK_TRACE( 80, &DebugKey );


    /*
    ** Callback Thread main loop:
    **
    **   Invoke callbacks as long as there are CBACKS queued to the CBT,
    **   then go to sleep until interrupted (shutdown), or 
    **   reawakened.
    */
    while ( DB_SUCCESS_MACRO(status) && cl_status == OK )
    {
	/* Watch for new posts while we process this batch */
	posts = cbt->cbt_triggered;

	/*
	** First process retries from last pass.
	*/
	if ( 0 != (cback_offset = retry_queues[oddoreven ^ 1] ) )
	{
	    retry_queues[oddoreven ^ 1] = 0;
	    /*
	    ** Count retry as a "post" to keep lk_cback_post
	    ** from resuming us while we are running.
	    */
	    CSadjust_counter(&cbt->cbt_triggered, 1);
	}
	else
	{
	    /*
	    ** Unhook current LKB list.  LKB list is maintained
	    ** as a singly linked list, with the latest callback
	    ** at the front of the list. 
	    */
	    LK_HOOK_CBT(cbt, cback_offset, 0);
	}

	if ( !cback_offset )
	{
	    /* If no new posts, then time to sleep */
	    if ( (CSadjust_counter( &cbt->cbt_triggered, -posts )) == 0 )
	    {
		/* Cback queue is still empty, trace and go to sleep. */
		DebugKey.lk_key1 = cbt->cbt_triggered;
		DebugKey.lk_key2 = posts;
		DebugKey.lk_key3 = cbt->cbt_stat.wakeups;
		LK_CALLBACK_TRACE( 88, &DebugKey );

		cl_status = CSsuspend(suspmask, timeout, NULL);
		/* At shutdown time, E_CS0008_INTERRUPTED will be returned. */
		if (cl_status == OK || cl_status == E_CS0009_TIMEOUT)
		{
		    lkd->lkd_stat.cback_wakeups++;
		    cbt->cbt_stat.wakeups++;

		    /* Look at other retry queue this pass. */
		    oddoreven ^= 1;

		    /* Reset suspendmask, timeout */
		    suspmask = CS_INTERRUPT_MASK;
		    timeout = 0;
		    cl_status = OK;

		    /* Trace the wakeup */
		    DebugKey.lk_key1 = cbt->cbt_triggered;
		    DebugKey.lk_key2 = cbt->cbt_triggered;
		    DebugKey.lk_key3 = cbt->cbt_stat.wakeups;
		    LK_CALLBACK_TRACE( 89, &DebugKey );
		}
	    }

	    continue;
	}

	/*
	** Reverse list.  We can now operate on this list without
	** protection, since it is unhooked from the input queue.
	**
	** While we process this list fragment, new requests will
	** be atomically hooked onto the CBT for retrieval when
	** we loop on back to the top of the outer while loop.
	*/
	prev_offset = 0;
	for ( ; /* something to break out off */ ; )
	{
	    cback = (CBACK *) LGK_PTR_FROM_OFFSET(cback_offset);
	    next_offset = cback->cback_q_next;
	    cback->cback_q_next = prev_offset;
	    if ( 0 == next_offset )
	    {
		break;
	    }
	    prev_offset = cback_offset;
	    cback_offset = next_offset;
	}
	next_offset = cback_offset;

	/*
	** Now process list in correct order.
	*/
	while ( DB_SUCCESS_MACRO(status) && next_offset )
	{
            cback_is_valid = TRUE;
	    cback_offset = next_offset;
	    cback = (CBACK *)LGK_PTR_FROM_OFFSET(cback_offset);
	    
	    /* Extract forward offset */
	    next_offset = cback->cback_q_next;

	    lkb = (LKB*)((char*)cback - CL_OFFSETOF(LKB,lkb_cback));
	    rsb = (RSB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

	    if ( rsb && rsb->rsb_id == cback->cback_rsb_id &&
		 cback->cback_cbt_id == LK_mycbt &&
		 cback->cback_fcn != (LKcallback)NULL )
	    {

		LK_CALLBACK_TRACE( 2, &rsb->rsb_name );

		/* Construct lockid for callback's use */
		CbackLockid.lk_unique = lkb->lkb_id;
		CbackLockid.lk_common = cback->cback_rsb_id;

		/* Invoke callback */ 
		lkd->lkd_stat.cback_cbacks++;
		cbt->cbt_stat.cbacks++;
		status = (*cback->cback_fcn)
			    (cback->cback_blocked_mode,
			     cback->cback_arg,
			     &CbackLockid,
			     &rsb->rsb_name);
			
		LK_CALLBACK_TRACE( 3 + status, &rsb->rsb_name );
		if (status == E_DB_INFO)
		{
		    /* Put callback back on end of a local retry queue */
		    cback->cback_q_next = retry_queues[oddoreven];
		    retry_queues[oddoreven] = cback_offset;

		    /* Next suspend will sleep a max of 1 second. */
		    suspmask = CS_INTERRUPT_MASK | CS_TIMEOUT_MASK;
		    timeout = 1;
		    /*
		    ** Note that when we get around to retrying this cback,
		    ** it (and its LKB) may have been reused and now 
		    ** belongs to another server's cback list, which is
		    ** why we'll recheck cback_ct_id, above, next time around.
		    */
		}
	    }
	    else
	    {
                cback_is_valid = FALSE;
		/* Resource/cback is stale */
		if ( cback->cback_cbt_id != LK_mycbt )
		{
		    DebugKey.lk_key1 = cback->cback_cbt_id;
		    DebugKey.lk_key2 = LK_mycbt;
		    DebugKey.lk_key3 = 0;
		    LK_CALLBACK_TRACE( 86, &DebugKey );
		}
		else if ( rsb )
		{
		    LK_CALLBACK_TRACE( 86, &rsb->rsb_name );
		}
		else
		{
		    DebugKey.lk_key1 = cback->cback_rsb_id;
		    DebugKey.lk_key2 = 0;
		    DebugKey.lk_key3 = 0;
		    LK_CALLBACK_TRACE( 86, &DebugKey );
		}
		status = E_DB_WARN;
	    }

	    if ( status == E_DB_WARN )
	    {
		/*
		** Callback function returns a warning if there
		** was nothing to do. (Blocking lock released).
		*/
		lkd->lkd_stat.cback_stale++;
		cbt->cbt_stat.stale++;
	    }

	    /* Count one post handled, unless retrying */
	    if ( status != E_DB_INFO )
	    {
		/* If more cbacks queued on this lock, stay on this cback */
                if (cback_is_valid &&
		    CSadjust_counter(&cback->cback_posts, -1) > 0)
		{
		    if ( rsb )
		    {
			LK_CALLBACK_TRACE( 10 + cback->cback_posts, &rsb->rsb_name );
		    }
		    next_offset = cback_offset;
		    continue;
		}
	    }

	    /* Sanity check self-linked cback, should not happen... */
	    if ( next_offset == cback_offset )
	    {
		char	key[132];

		/*
		** Suspend forever so lkcbt can be run to see how we got
		** into this mess...
		*/
		TRdisplay("%@ lk_callback :%d, status %d CBT %x CBACK %x is linked to itself\n"
			  "\t next %x offset %x cbt_id %x LK_mycbt %x LKB %x posts %d\n"
			  "\t %s \t SLEEPING, do lkcbt !\n",
			  __LINE__, 
			  status,
			  cbt,
			  cback,
			  next_offset,
			  cback_offset,
			  cback->cback_cbt_id,
			  LK_mycbt,
			  lkb->lkb_id,
			  cback->cback_posts,
			  (rsb) ? LKkey_to_string(&rsb->rsb_name, key) : "no RSB");

		 CSsuspend(0, 0, 0);
	    }

	} /* while ( DB_SUCCESS_MACRO(status) && next_offset ) */

    } /* while ( DB_SUCCESS_MACRO(status) && cl_status == OK ) */
    
    if (cl_status != OK && cl_status != E_CS0008_INTERRUPTED)
    {
	uleFormat(NULL, cl_status, (CL_ERR_DESC *)NULL,
		   ULE_LOG, NULL, 0, 0, 0, &err_code, 0);
	status = E_DB_ERROR;
    }

    if ( DB_SUCCESS_MACRO(status) )
	status = E_DB_OK;
    else
        dmc_cb->error.err_code = E_CL1061_LK_CBACK_THREAD_DEAD; 

    TRdisplay("Process %d Callback Thread %x terminating, wakeups: %d, calls: %d, ignored: %d\n",
		cbt->cbt_cpid.pid, cbt->cbt_cpid.sid, cbt->cbt_stat.wakeups, 
		cbt->cbt_stat.cbacks, cbt->cbt_stat.stale);

    return (status);
}

/*
** Name: lk_cback_init - Initialise LK callback data structures
**
** Description:
**      Called from dmcstart to initialise callback data structures.
**	This routine causes a descriptor for this callback thread to be 
**      added to a queue off the lkd. 
**
** Inputs:
**      None.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Created.
**      01-aug-1996 (nanpr01 for jenjo02)
**          Defined new mutexi for cbt queue, cbt, and cback. LK_allocate_cb()
**          and LK_deallocate_cb() changed to recognize CBT_TYPE and CBACK_TYPE
**          instead of pretending they're RSBs.
**	09-Apr-1997 (jenjo02)
**	    Added new stats for callback threads.
**	    Removed lkd_cbt_next/prev, lkd_cbt_q_mutex.
**	29-aug-2002 (devjo01)
**	    Init 'cbt_cback_list' to zero, and remove depreciated
**	    'cbt_cback_next', 'cbt_cback_prev', and 'cbt_mutex'.
*/
STATUS
lk_cback_init()
{
    LKD                 *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    CBT                 *cbt;
    CBT                 *next_cbt;

    STATUS status;
    i4 err_code;
    
    /* 
    ** Allocate a Callback Thread Descriptor. Currently an RSB_TYPE, because
    ** that's the smallest control block (72 bytes) that a CBT will fit in
    */ 
    if ((cbt = (CBT *)LK_allocate_cb(CBT_TYPE, (LLB *)NULL)) == 
				(CBT *)NULL)
    {
        uleFormat(NULL, E_DMA00E_LK_NO_RSBS, (CL_ERR_DESC *)NULL, ULE_LOG,
                   NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 0);
        
        return (E_CL1069_LK_CBACK_INIT_FAILED);
    }

    /* Initialise this process' CBT descriptor */

    LK_mycbt = LGK_OFFSET_FROM_PTR(cbt);

    cbt->cbt_type = CBT_TYPE;
    cbt->cbt_triggered = 1;
    cbt->cbt_stat.wakeups = 0;
    cbt->cbt_stat.cbacks = 0;
    cbt->cbt_stat.stale = 0;
    cbt->cbt_cback_list = 0;
# if !defined(conf_CAS_ENABLED)
    (void)CSw_semaphore(&cbt->cbt_mutex, CS_SEM_MULTI, "CBT_mutex");
# endif

# if defined(LK_TRACE_CALLBACK_ENABLED)
    IICSmon_register( "lkcbt", lk_callback_debug );
# endif

    return(OK);
}

/*
** Name: lk_cback_post - Queue a callback to CBT.
**
** Description:
**	LK routines which request or release locks may find that thay
**      need to trigger a callback stored in an LKB which is blocking some
**      lock request. This routine queues a completely filled out CBACK
**	stucture containing the callback details, resets the LKBs callback
**	details and triggers the appropriate Callback Thread if needed.
**
** Inputs:
**      LKB lkb                     - lock descriptor for lock to be fired.
** Outputs:
**	None.
**
** Side Effects:
**      A CBACK structure is added to the appropriate CBT's list. If the
**      thread hasn't yet been triggered we set the triggered flag and 
**      resume that thread. 
**
** Returns:
**	STATUS
**
** History:
**	29-aug-2002 (devjo01)
**	    Created from guts of lk_cback_put.
**	13-feb-2004 (devjo01)
**	    No longer need to atomically update cbt_triggered.
**	12-Feb-2007 (jonj)
**	    Need to atomically update cbt_triggered.
**	    Add debug entries noting CSresume or not resume.
**	    When called from an AST, cbt_id has to be
**	    this server's CBT; if not, force it.
**	    (I haven't figured out why it's not sometimes - yet).
**	    Always use LKcp_resume, AST or not.
**	19-Jun-2007 (jonj)
**	    AST-called cbt_id will not necessarily be that
**	    of this server; forcing it such leads to NO TCB
**	    errors in dm0p_dmcm_callback. Trace the disparity
**	    but leave cbt_id as-is - it has to be that of the
**	    lock/buffer holder's server.
**	01-Oct-2007 (jonj)
**	    Prototype changed to pass *LKB containing CBACK, not *CBACK
**	23-Oct-2007 (jonj)
**	    Put CBACK on CBT list only if first post, otherwise it's
**	    already there.
*/
STATUS 
lk_cback_post(LKB *lkb, bool fromAST)
{
    CBT          *cbt;
    RSB		 *rsb = (RSB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);
    CBACK	 *cback = &lkb->lkb_cback;
    i4      	 err_code;
    SIZE_TYPE	 cback_offset;

    if ( fromAST && cback->cback_cbt_id != LK_mycbt )
    {
	/* Trace disparity */
	LK_CALLBACK_TRACE( 87, &rsb->rsb_name );
    }

    /* Check that LKB has a valid cbt descriptor */
    cbt = (CBT *) LGK_PTR_FROM_OFFSET(cback->cback_cbt_id);

    if ( !cbt || cbt->cbt_type != CBT_TYPE)
    {
	uleFormat(NULL, E_CL1065_LK_CBACK_PUT_BADCBT, (CL_ERR_DESC *)NULL,
                   ULE_LOG, NULL, 0, 0, 0, &err_code, 1, 0, cbt->cbt_type);
        return(E_CL1066_LK_CBACK_PUT_FAILED);
    }

 
    /* Put CBACK on list if first post on this resource */
    if ( (CSadjust_counter(&cback->cback_posts, 1)) == 1 )
    {
	/* Atomically compare and swap into head of singly linked list. */
	cback_offset = LGK_OFFSET_FROM_PTR(cback);
	LK_HOOK_CBT(cbt, cback->cback_q_next, cback_offset );
    }
    else
    {
        LK_LOCK_KEY key = rsb->rsb_name;
        key.lk_key1 = cback->cback_posts;
        LK_CALLBACK_TRACE(85, &key);
    }

    /* Now resume callback thread if necessary */
    if ( (CSadjust_counter( &cbt->cbt_triggered, 1 )) == 1 )
    {
	/* Is sleeping, wake it up */
	LK_CALLBACK_TRACE( cbt->cbt_triggered, &rsb->rsb_name );
        LKcp_resume( &cbt->cbt_cpid );
    }
    else
	/* Already running, trace #posts */
	LK_CALLBACK_TRACE( -cbt->cbt_triggered, &rsb->rsb_name );
    
    return(OK);
}

/*
** Name: lk_cback_put - Fire a callback.
**
** Description:
**	LK routines which request or release locks may find that thay
**      need to trigger a callback stored in an LKB which is blocking some
**      lock request. This routine queues the callback details, resets the
**      LKBs callback details and triggers the appropriate Callback Thread if
**      necessary.
**
**      The RSB mutex must be held when this routine is called.
**
** Inputs:
**      i4  blocked_mode            - mode of lock being blocked
**      LKB lkb                     - lock descriptor for lock to be fired.
** Outputs:
**	None.
**
** Side Effects:
**      A CBACK structure is added to the appropriate CBT's list. If the
**      thread hasn't yet been triggered we set the triggered flag and 
**      resume that thread. 
**
** Returns:
**	STATUS
**
** History:
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Created.
**      01-aug-1996 (nanpr01 for jenjo02)
**          Defined new mutexi for cbt queue, cbt, and cback. LK_allocate_cb()
**          and LK_deallocate_cb() changed to recognize CBT_TYPE and CBACK_TYPE
**          instead of pretending they're RSBs.
**          Added code to save lock key at time that a callback is fired.
**          Between the time of the firing and the invocation of the callback
**          code by the callback thread, the LKB may have been released and
**          in use by another resource. Using only the lock_id as a handle
**          will cause the callback code to operate on a resource/lock that
**          is was not registered for; adding the lock key lets the callback
**          function check that the lock it is being called with is in fact
**          the one it was fired for.
**	21-Apr-1997  (jenjo02)
**	    Removed cback_mutex.
**	29-aug-2002 (devjo01)
**	    Moved guts of this routine to lk_cback_alloc && lk_cback_post.
**	01-Oct-2007 (jonj)
**	    CBACK now embedded in LKB.
*/
STATUS 
lk_cback_put(i4 blocked_mode, LKB *lkb)
{
    RSB		 *rsb;

    rsb = (RSB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

    /*
    ** Init the CBACK structure for the callback info.
    */ 

    /* NB: cback_fcn, cback_arg have been pre-set */
    lkb->lkb_cback.cback_cbt_id = LK_mycbt;
    lkb->lkb_cback.cback_rsb_id = rsb->rsb_id;
    lkb->lkb_cback.cback_blocked_mode = blocked_mode;
    
    return(lk_cback_post( lkb, FALSE ));
}

/*
** Name: lk_cback_fire - Fire callbacks in the RSB's granted queue.
**
** Description:
**	Loop through the RSB's granted queue. Any LKBs with a callback
**      and whose granted lock mode conflicts with the lock being blocked
**      are fired.
**
**      The RSB's mutex must be held when this routine is called.
**
**	This routine is not invoked for distributed locks under
**	Clustered Ingres.
**
** Inputs:
**      RSB rsb                     - resource descriptor 
**      i4  blocked_mode            - mode of lock being blocked
**
** Outputs:
**	None.
**
** Side Effects:
**
** Returns:
**	STATUS
**
** History:
**	01-aug-1996 (nanpr01 for ICL keving)
**	    Created.
**      01-aug-1996 (nanpr01 for jenjo02)
**          Defined new mutexi for cbt queue, cbt, and cback. LK_allocate_cb()
**          and LK_deallocate_cb() changed to recognize CBT_TYPE and CBACK_TYPE
**          instead of pretending they're RSBs.
**	01-Oct-2007 (jonj)
**	    CBACK now embedded in LKB.
*/
STATUS 
lk_cback_fire(RSB *rsb, i4  blocked_lock_mode)
{
    SIZE_TYPE     end_offset;
    SIZE_TYPE     lkb_offset;
    LKB           *next_lkb;
    i4     	  err_code;
    STATUS        status = OK;
   
    /* sanity checks */

    if (rsb->rsb_type != RSB_TYPE)
    {
	uleFormat(NULL, E_CL1067_LK_CBACK_FIRE_BADRSB, (CL_ERR_DESC *)NULL,
                   ULE_LOG, NULL, 0, 0, 0, &err_code, 1, 0, rsb->rsb_type);
        return(E_CL1068_LK_CBACK_FIRE_FAILED);
    }
    end_offset = LGK_OFFSET_FROM_PTR(&rsb->rsb_grant_q_next);
    for (lkb_offset = rsb->rsb_grant_q_next;
         lkb_offset != end_offset;
         lkb_offset = next_lkb->lkb_q_next)
    {
        next_lkb = (LKB *)LGK_PTR_FROM_OFFSET(lkb_offset);

        if (next_lkb->lkb_state == LKB_CONVERT)
        {
            /* End of granted locks */
            break;
        }
        if (next_lkb->lkb_cback.cback_fcn == (LKcallback) NULL ||
	    (next_lkb->lkb_attribute & LKB_DISTRIBUTED_LOCK) ||
            ((LK_compatible[blocked_lock_mode] >> 
                                next_lkb->lkb_request_mode) & 1))
        {
            /*
	    ** No callback or a distributed lock, or compatible.
	    ** Distributed locks will rely on the DLM to post
	    ** callback asynchronously.
	    */
            continue;
        }
        status = lk_cback_put(blocked_lock_mode, next_lkb);
        if (status)
        {
            uleFormat(NULL, status, (CL_ERR_DESC *)NULL,
                       ULE_LOG, NULL, 0, 0, 0, &err_code, 0);
            status =  E_CL1068_LK_CBACK_FIRE_FAILED;
            break;
        }
        /* Reset LKBs callback */
        next_lkb->lkb_cback.cback_fcn = (LKcallback) NULL;
        rsb->rsb_cback_count--;
    }
    return(status);
}
