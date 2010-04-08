/*
** Copyright (c) 1992, 2005 Ingres Corporation
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
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <lk.h>
#include    <cx.h>
#include    <lkdef.h>
#include    <lgkdef.h>

/**
**
**  Name: LKALTER.C - Implements the LKalter function of the locking system
**
**  Description:
**	This module contains the code which implements LKalter.
**	
**	    LKalter -- Adjust the characteristics of the locking system.
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	24-may-1993 (bryanp)
**	    Separated RSB's into their own separate rbk_table.
**	    New arguments to LK_wakeup for mutex optimizations.
**	    Also added lkreq argument to LK_wakeup for shared locklists.
**	    Enforce minimum limits for some parameters (llb's, blk's.)
**	21-jun-1993 (bryanp)
**	    Added LK_A_CSP_ENTER_DEBUGGER for internal development use.
**	30-jun-1993 (shailaja)
**	    Fixed compiler warnings on ANSI C semantics change.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Fixed stalled lock handling & moved code into subroutines.
**	    Improved error message logging.
**	    Added some casts to pacify compilers.
**	23-aug-1993 (bryanp)
**	    Removed old, unused ifdef's for resource allocation monitoring.
**	    Pass proper lkreq parameter to complete_stalled_request.
**	23-aug-1993 (andys)
**	    Add new parameter to LK_do_new_lock_request.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <tr.h>.
**      20-nov-1994 (andyw)
**          Added NO_OPTIM due to Solaris 2.4/Compiler 3.00 errors
**	17-nov-1994 (medji01
**	    Mutex Granularity Project
**		- Removed LK_mutex() calls from LKalter().
**		- Changed LK_mutex() calls to pass mutex field address.
**		- In LK_alter acquire and release llb semaphore in the
**		  LK_A_NOINTERRUPT, LK_A_INTERRUPT, and
**		  LK_A_CLR_DLK_SRCH cases.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	5-june-95 (hanch04)
**	    Removed NO_OPTIM, installed sun patch
**	22-aug-1995 (dougb)
**	    As a related update to the fix for bug #70696, update the
**	    parameters to LK_do_new_lock_request() in
**	    complete_stalled_request().
**	06-Dec-1995 (jenjo02)
**	    Moved CSa_semaphore() into LK_imutex().
**	14-Dec-1995 (jenjo02)
**	    Additional mutexes, cleaned up mutex positioning.
**	21-aug-1996 (boama01)
**	    Chgd complete_stalled_request() to access and lock RSB before
**	    LK_do_new_lock_request() calls, and unlock afterwards.
**	13-dec-96 (cohmi01)
**	    Add new parameter to LK_do_new_lock_request() call. (Bug 79763).
**	16-Oct-1996 (jenjo02)
**	    Prefixed semaphore names with "LK" to make them more identifiable
**	    to iimonitor, sampler users.
**	23-Apr-1997 (jenjo02)
**	    Dropped unused "release_mutex" parm from LK_do_new_lock_request()
**	    prototype, changed "free_res" parm from int to pointer to int.
**	16-Nov-1998 (jenjo02)
**	    Cross-process thread identity changed to CS_CPID structure
**	    from PID and SID.
**	13-Dec-1999 (jenjo02)
**	    Added LG_A_SHOW_DEADLOCKS debug flag.
**	24-Aug-2000 (jenjo02)
**	    Added LK_A_REASSOC function to prepare aborting WILLING_COMMIT
**	    SHARED lock lists for recovery.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	    - Recycle LK_A_STALL to stall all locks, since LKnode has
**	    gone away, and we cannot access lkd_lock_stall from dmfcsp.
**	    - Kill LK_A_CSP_ENTER_DEBUGGER.
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	09-may-2005 (devjo01)
**	    Add support for LK_A_DIST_DEADLOCK.
**	14-Jun-2005 (thaju02)
**	    More SIZE_TYPE changes in set_locklist_limit, set_rsb_limit,
**	    and set_block_limit.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/

static STATUS LK_alter(i4 flag, i4 value, CL_ERR_DESC *sys_err);
static STATUS	set_locklist_limit(LKD *lkd, u_i4 value);
static STATUS	set_block_limit(LKD *lkd, u_i4 value);
static STATUS	set_rsb_limit(LKD *lkd, u_i4 value);
static STATUS	set_lock_hash_size(LKD *lkd, u_i4 value);
static STATUS	set_resource_hash_size(LKD *lkd, u_i4 value);
static STATUS	resume_stalled_requests(i4 stall_state);

GLOBALREF	i4	csp_debug;


/*{
** Name: LKalter	- Alter the lock database.
**
** Description:
**	This routine handles the initialization and allocation of
**	tables in the lock database.  This routine can be used to dynamically
**	change the size of the internal tables.  These changes can only take 
**	place when the LK routines have no active lists or active locks.
**
** Inputs:
**	flag				The table size to change.
**	value				The new value for the table size.
**
** Outputs:
**	sys_err			Reason code for failure.
**	Returns:
**	    OK				Successful completion.
**	    LK_BUSY			List or locks are active.
**	    LK_NOLOCKS			Not enough memory.
**	    LK_BADPARAM			Bad parameter.
**
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
**		- Removed LK_mutex() and LK_unmutex() calls against LKD.
*/
STATUS
LKalter(
i4                  flag,
i4             value,
CL_ERR_DESC         *sys_err)
{
    STATUS	status;
    STATUS	local_status;
    CL_ERR_DESC	local_sys_err;
    i4	err_code;

    status = LK_alter(flag, value, sys_err);

    return(status);
}

/*{
** Name: LK_alter	- Alter the lock database.
**
** Description:
**	This routine handles the initialization and allocation of
**	tables in the lock database.  This routine can be used to dynamically
**	chanage the size of the internal tables.  These changes can only take 
**	place when the LK routines have no active lists or active locks.
**
** Inputs:
**	flag				The table size to change.
**	value				The new value for the table size.
**
** Outputs:
**	sys_err			Reason code for failure.
**	Returns:
**	    OK			Successful completion.
**	    LK_BUSY			List or locks are active.
**	    LK_NOLOCKS			Not enough memory.
**	    LK_BADPARAM			Bad parameter.
**
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
**	    Separated RSB's into their own separate rbk_table.
**	    New arguments to LK_wakeup for mutex optimizations.
**	    Also added lkreq argument to LK_wakeup for shared locklists.
**	    Enforce minimum limits for some parameters (llb's, blk's.)
**	    Split some code off into local subroutines to keep the routine
**		size reasonable (I hate thousand-line subroutines!).
**	21-jun-1993 (bryanp)
**	    Added LK_A_CSP_ENTER_DEBUGGER for internal development use.
**	26-jul-1993 (bryanp)
**	    Fixed stalled lock handling & moved code into subroutines.
**	    Added some casts to pacify compilers.
**	23-aug-1993 (bryanp)
**	    Pass proper lkreq parameter to complete_stalled_request.
**	17-nov-1994 (medji01)
**	    Mutex Granularity Project
**		- Acquire and release llb semaphore inLK_A_NOINTERRUPT,
**		  LK_A_INTERRUPT, and LK_A_CLR_DLK_SRCH cases.
**	28-mar-1995 (medji01)
**	    64K+ Lock Limit Project
**		- Removed references to id_instance in LK_ID.
**		- Changed error messages to reference id_id instead
**		  of id_instance.
**	14-Dec-1995 (jenjo02)
**	    Additional mutexes, cleaned up mutex positioning.
**	06-Oct-1999 (jenjo02)
**	    Replaced LK_ID references to LLBs with LLB_ID.
**	24-Aug-2000 (jenjo02)
**	    Added LK_A_REASSOC function to prepare aborting WILLING_COMMIT
**	    SHARED lock lists for recovery.
**	12-apr-2001 (devjo01)
**	    s103715 (Portable cluster support)
**	    - Recycle LK_A_STALL to stall all locks, since LKnode has
**	    gone away, and we cannot access lkd_lock_stall from dmfcsp.
**	    - Kill LK_A_CSP_ENTER_DEBUGGER.
**	16-Jun-2009 (hanal04) Bug 122117
**          Added LK_A_FORCEABORT and LK_A_NOFORCEABORT.
*/
static STATUS
LK_alter(
i4		    flag,
i4		    value,
CL_ERR_DESC	    *sys_err)
{
    LKD			*lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LLB			*llb;
    RSB			*rsb;
    SIZE_TYPE		*lbk_table;
    i4			i;
    i4			err_code;
    STATUS		status;
    STATUS		return_status;
    LLB_ID		llb_id;
    LLBQ		*llbq;
    LKB			*wait_lkb;
    RSB			*wait_rsb;

    CL_CLEAR_ERR(sys_err);

    return_status = OK;

    switch (flag)
    {
    case LK_I_LLB:
	return_status = set_locklist_limit(lkd, (u_i4)value);
	break;

    case LK_I_BLK:
	return_status = set_block_limit(lkd, (u_i4)value);
	break;

    case LK_I_RSB:
	return_status = set_rsb_limit(lkd, (u_i4)value);
	break;

    case LK_I_LKH:
	return_status = set_lock_hash_size(lkd, (u_i4)value);
	break;

    case LK_I_RSH:
	return_status = set_resource_hash_size(lkd, (u_i4)value);
	break;

    case LK_I_MAX_LKB:
	if (value <= 0)
	{
	    uleFormat(NULL, E_DMA02A_LKALTER_NEGATIVE_ARG, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, 0, 0, 0, &err_code, 2,
		    0, value, 0, "LK_I_MAX_LKB");
	    return (LK_BADPARAM);
	}
	lkd->lkd_max_lkb = value;
	break;

    case LK_A_NOINTERRUPT:
    case LK_A_INTERRUPT:
	{

	    /*
	    ** Force the indicated lock list to be interruptible, or to be
	    ** non-interruptible, depending on the operation code. DMF wishes
	    ** to control the interruptibility of the lock list at certain
	    ** critical times (during abort processing, a lock list is forced
	    ** to be non-interruptible; certain lock requests on otherwise
	    ** un-interruptable lock lists, such as DMT_SHOW on the sessions
	    ** lock list, are made interruptable to avoid "hung threads").
	    */

	    STRUCT_ASSIGN_MACRO((*(LLB_ID *) &value), llb_id);
	    if (llb_id.id_id == 0 ||
		(i4) llb_id.id_id > lkd->lkd_lbk_count)
	    {
		uleFormat(NULL, E_CL1025_LK_NOINTERRUPT_ALTER, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 2,
			    0, llb_id.id_id, 0, lkd->lkd_lbk_count);
		return (LK_BADPARAM);
	    }

	    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id.id_id]);

	    if (llb->llb_type != LLB_TYPE ||
		llb->llb_id.id_instance != llb_id.id_instance)
	    {
		uleFormat(NULL, E_CL1026_LK_NOINTERRUPT_ALTER, (CL_ERR_DESC *)NULL,
			ULE_LOG,
			NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 3,
			0, *(u_i4*)&llb_id, 0, llb->llb_type,
			0, *(u_i4*)&llb->llb_id);
		return (LK_BADPARAM);
	    }

	    if (status = LK_mutex(SEM_EXCL, &llb->llb_mutex))
	    {
		uleFormat(NULL, status, sys_err, ULE_LOG, NULL, 0, 0, 0,
			   &err_code, 0);
		return(E_DMA029_LKALTER_SYNC_ERROR);
	    }

	    if (flag == LK_A_NOINTERRUPT)
	    	llb->llb_status |= LLB_NOINTERRUPT;
	    else
	    	llb->llb_status &= ~(LLB_NOINTERRUPT);

	   (VOID) LK_unmutex(&llb->llb_mutex);

	    return (OK);
	}

    case LK_A_FORCEABORT:
    case LK_A_NOFORCEABORT:
	{

	    /*
	    ** Force the indicated lock list to be marked for force abort.
	    ** This is used to prevent lock waits in force abort sessions
            ** that have entered DMF before the SCB_FORCE_ABORT was set.
	    */

	    STRUCT_ASSIGN_MACRO((*(LLB_ID *) &value), llb_id);
	    if (llb_id.id_id == 0 ||
		(i4) llb_id.id_id > lkd->lkd_lbk_count)
	    {
		uleFormat(NULL, E_CL1071_LK_FORCEABORT_ALTER, (CL_ERR_DESC *)0,
			    ULE_LOG, NULL, (char *)0, 0L, (i4 *)0,
			    &err_code, 2,
			    0, llb_id.id_id, 0, lkd->lkd_lbk_count);
		return (LK_BADPARAM);
	    }

	    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id.id_id]);

	    if (llb->llb_type != LLB_TYPE ||
		llb->llb_id.id_instance != llb_id.id_instance)
	    {
		uleFormat(NULL, E_CL1072_LK_FORCEABORT_ALTER, (CL_ERR_DESC *)0,
			ULE_LOG,
			NULL, (char *)0, 0L, (i4 *)0, &err_code, 3,
			0, *(u_i4*)&llb_id, 0, llb->llb_type,
			0, *(u_i4*)&llb->llb_id);
		return (LK_BADPARAM);
	    }

	    if (status = LK_mutex(SEM_EXCL, &llb->llb_mutex))
	    {
		uleFormat(NULL, status, sys_err, ULE_LOG, NULL, 0, 0, 0,
			   &err_code, 0);
		return(E_DMA029_LKALTER_SYNC_ERROR);
	    }

	    if (flag == LK_A_FORCEABORT)
            {
                llb->llb_status |= LLB_FORCEABORT;
            }
	    else
            {
	    	llb->llb_status &= ~(LLB_FORCEABORT);
            }

	   (VOID) LK_unmutex(&llb->llb_mutex);

	    return (OK);
	}

    case LK_A_REASSOC:
	{
	    /*
	    ** Prepare SHARED lock list for adoption by recovery.
	    **
	    ** This is needed only for SHARED lock lists
	    ** belonging to transactions in a WILLING COMMIT
	    ** state.
	    */

	    STRUCT_ASSIGN_MACRO((*(LLB_ID *) &value), llb_id);
	    if (llb_id.id_id == 0 ||
		(i4) llb_id.id_id > lkd->lkd_lbk_count)
	    {
		uleFormat(NULL, E_CL1027_LK_BAD_ALTER_REQUEST, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 1, 0, flag);
		return (LK_BADPARAM);
	    }

	    lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[llb_id.id_id]);

	    if (llb->llb_type != LLB_TYPE ||
		llb->llb_id.id_instance != llb_id.id_instance)
	    {
		uleFormat(NULL, E_CL1027_LK_BAD_ALTER_REQUEST, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 1, 0, flag);
		return (LK_BADPARAM);
	    }

	    LK_mutex(SEM_EXCL, &llb->llb_mutex);

	    /* Verify that this is a handle to a SHARED list */
	    if ( llb->llb_status & LLB_PARENT_SHARED )
	    {
		LLB	*sllb, *rllb, *next_llb, *prev_llb;
		SIZE_TYPE	related_llb = llb->llb_related_llb;

		sllb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_shared_llb);

		/* Deallocate the handle llb. */
	        LK_mutex(SEM_EXCL, &lkd->lkd_llb_q_mutex);
		next_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_q_next);
		prev_llb = (LLB *)LGK_PTR_FROM_OFFSET(llb->llb_q_prev);
		next_llb->llb_q_prev = llb->llb_q_prev;
		prev_llb->llb_q_next = llb->llb_q_next;
		(VOID) LK_unmutex(&lkd->lkd_llb_q_mutex);

		/* The deallocate also frees llb_mutex. */
		LK_deallocate_cb(llb->llb_type, (PTR)llb, llb);

		/* Mutex the SHARED list */
		LK_mutex(SEM_EXCL, &sllb->llb_mutex);

		/* Add handle's related lbb, if any, to SHARED lbb's list */
		if ( related_llb )
		{
		    rllb = (LLB *)LGK_PTR_FROM_OFFSET(related_llb);

		    rllb->llb_related_llb = sllb->llb_related_llb;
		    sllb->llb_related_llb = related_llb;
		}

		/*
		** If no more handles, un-SHARE this LLB.
		**
		** The SHARED list now looks like an ordinary
		** lock list, with the exception that it may
		** have a list of, instead of one, related LLBs.
		** LKrelease() is prepared for this possibility;
		** When the (now un-SHARED) lock list is released,
		** all related LLBs will also be released.
		*/
		if ( --sllb->llb_connect_count <= 0 )
		    sllb->llb_status &= ~LLB_SHARED;

		LK_unmutex(&sllb->llb_mutex);

	    }
	    else
	        (VOID) LK_unmutex(&llb->llb_mutex);

	    return (OK);
	}

	    

    /*
    ** Cluster locking system alter-codes follow:
    **	    LK_A_CSPID
    **	    LK_A_CSID
    **	    LK_A_DLK_HANDLER
    **	    LK_A_AST_CLR
    **	    LK_A_CLR_DLK_SRCH
    **	    LK_A_STALL
    */

    case LK_A_CSPID:
	/*
	** Calling process has CSP responsibilities.
	*/
	lkd->lkd_csp_id = LGK_my_pid;
	break;

    case LK_A_DLK_HANDLER:

    	/* THIS IS NEVER CALLED */

	if (value == 0)
	{
	    uleFormat(NULL, E_DMA02C_LKALTER_MISSING_ARG, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, 0, 0, 0, &err_code, 0);
	    return (LK_BADPARAM);
	}

	lkd->lkd_dlk_handler = value;
	break;

    case LK_A_AST_CLR:

    	/* THIS IS NEVER CALLED */

	lkd->lkd_status &= ~LKD_AST_ENABLE;
	break;

    case LK_A_CLR_DLK_SRCH:
	
	/* 
	** Turn off all the global deadlock search requests that already 
	** sent out so that they can be resent.
	*/
	lbk_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lkd->lkd_lbk_table);

	for (i = 1; i <= lkd->lkd_lbk_count; i++)
	{
	    llb = (LLB *)LGK_PTR_FROM_OFFSET(lbk_table[i]);

	    if (llb->llb_type == LLB_TYPE &&
		(llb->llb_status & LLB_ENQWAITING) &&
		(llb->llb_status & LLB_GDLK_SRCH))
	    {
		if (status = LK_mutex(SEM_EXCL, &llb->llb_mutex))
		    return(E_DMA029_LKALTER_SYNC_ERROR);
		/*
		** Check LLB again after mutex wait.
		*/
		if (llb->llb_type == LLB_TYPE &&
		    (llb->llb_status & LLB_ENQWAITING) &&
		    (llb->llb_status & LLB_GDLK_SRCH))
		{
		    llb->llb_status &= ~LLB_GDLK_SRCH;
		    lkd->lkd_stat.gdlck_search++;
		}

		(VOID) LK_unmutex(&llb->llb_mutex);
	    }
	}
	break;

    case LK_A_STALL:
	lkd->lkd_lock_stall = LK_STALL_ALL_BUT_CSP;
	break;

    case LK_A_STALL_LOGICAL_LOCKS:
	/*
	** This routine is called just before the UNDO phase of node failure
	** recovery. Before calling this routine, the CSP recovery process
	** has re-acquired the physical locks held by the open transactions.
	** We can now awaken an physical lock requests which were stalled.
	*/
	return_status = resume_stalled_requests(LK_STALL_LOGICAL_LOCKS);
	break;

    case LK_A_STALL_NO_LOCKS:
	/*
	** This routine is called at the completion of node failure recovery.
	** The lock stall period is now over, so any LKB which is on the stall
	** queue (i.e., any which completed during the stall period) can now
	** be awakened and the lock completion sent to the caller.
	*/
	return_status = resume_stalled_requests(0);
	break;

    case LK_A_CSP_ENTER_DEBUGGER:
	csp_debug = 1;
	break;

    case LK_A_SHOW_DEADLOCKS:

	/* Turn on or off the switch */
	if ( value )
	    lkd->lkd_status |= LKD_SHOW_DEADLOCKS;
	else
	    lkd->lkd_status &= ~LKD_SHOW_DEADLOCKS;
	break;

    case LK_A_ENABLE_DIST_DEADLOCK:

	/* Turn on or off the switch */
	if ( value )
	    lkd->lkd_status |= LKD_DISTRIBUTE_DEADLOCK;
	else
	    lkd->lkd_status &= ~LKD_DISTRIBUTE_DEADLOCK;
	break;

    default:
	{
	    uleFormat(NULL, E_CL1027_LK_BAD_ALTER_REQUEST, (CL_ERR_DESC *)NULL,
			    ULE_LOG, NULL, (char *)NULL, 0L, (i4 *)NULL,
			    &err_code, 1, 0, flag);
	    return (LK_BADPARAM);
	}
    }

    return (return_status);
}

/*
** Name: set_locklist_limit	- implement LKalter(LK_I_LLB)
**
** Description:
**	This routine is called to implement setting the locklist limit.
**	Breaking the code up this way simply reduces the overall size of
**	LK_alter().
**
** Inputs:
**	lkd			- pointer to the LKD
**	value			- the requested locklist limit value.
**
** Outputs:
**	None, but the large block table will be set up in the LKD
**
** Returns:
**	STATUS
**
** History:
**	24-may-1993 (bryanp)
**	    Broke this routine out of LK_alter to reduce routine sizes.
**	    Added validation for minimum locklist limit (determined empirically)
*/
static STATUS
set_locklist_limit(LKD *lkd, u_i4 value)
{	
    LGK_EXT		*ext;
    i4		err_code;

    if (value > LKD_MAXNUM_LLB)
    {
	uleFormat(NULL, E_DMA000_LK_LLB_TOO_MANY, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MAXNUM_LLB);
	return (LK_BADPARAM);
    }
    if (value < LKD_MINNUM_LLB)
    {
	uleFormat(NULL, E_DMA018_LK_LLB_TOO_FEW, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MINNUM_LLB);
	return (LK_BADPARAM);
    }
    if (lkd->lkd_llb_inuse)
    {
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_llb_inuse);
	return (LK_BUSY);
    }
    if (lkd->lkd_lbk_table)
    {
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_lbk_table);
	return (LK_BUSY);
    }

    ext = (LGK_EXT *) lgkm_allocate_ext(sizeof(SIZE_TYPE) * value);
    if (ext == NULL)
    {
	uleFormat(NULL, E_CL1021_LK_LLB_ALLOC_FAILED, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, value);
	return (LK_NOLOCKS);
    }

    lkd->lkd_lbk_table = LGK_OFFSET_FROM_PTR(ext);
    lkd->lkd_lbk_count = 0;
    lkd->lkd_lbk_size = value -1;
    lkd->lkd_lbk_free = 0;

    return (OK);
}

/*
** Name: set_RSB_limit	- implement LKalter(LK_I_RSB)
**
** Description:
**	This routine is called to implement setting the resource limit.
**	Breaking the code up this way simply reduces the overall size of
**	LK_alter().
**
** Inputs:
**	lkd			- pointer to the LKD
**	value			- the requested rsb limit value.
**
** Outputs:
**	None, but the resource block table will be set up in the LKD
**
** Returns:
**	STATUS
**
** History:
**	24-may-1993 (bryanp)
**	    Invented this routine for managing RSB's separately.
**      01-aug-1996 (nanpr01 for jenjo02)
**          Initialize the callback fields.
**	01-Oct-2007 (jonj)
**	    Deleted lkd_cback_free. CBACK now embedded in LKB.
*/
static STATUS
set_rsb_limit(LKD *lkd, u_i4 value)
{	
    LGK_EXT		*ext;
    i4		err_code;

    if (value > LKD_MAXNUM_RSB)
    {
	uleFormat(NULL, E_DMA022_LK_RSB_TOO_MANY, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MAXNUM_RSB);
	return (LK_BADPARAM);
    }
    if (value < LKD_MINNUM_RSB)
    {
	uleFormat(NULL, E_DMA023_LK_RSB_TOO_FEW, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MINNUM_RSB);
	return (LK_BADPARAM);
    }
    if (lkd->lkd_rsb_inuse || lkd->lkd_lkb_inuse)
    {
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_lkb_inuse);
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_rsb_inuse);
	return (LK_BUSY);
    }
    if (lkd->lkd_rbk_table)
    {
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_rbk_table);
	return (LK_BUSY);
    }

    ext = (LGK_EXT *) lgkm_allocate_ext(sizeof(SIZE_TYPE) * value);
    if (ext == NULL)
    {
	uleFormat(NULL, E_CL1022_LK_LOCK_ALLOC_FAILED, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, value);
	return (LK_NOLOCKS);
    }

    lkd->lkd_rbk_table = LGK_OFFSET_FROM_PTR(ext);
    lkd->lkd_rbk_count = 0;
    lkd->lkd_rbk_size = value -1;
    lkd->lkd_rbk_free = 0;
    lkd->lkd_cbt_free = 0;

    return (OK);
}

/*
** Name: set_block_limit	- implement LKalter(LK_I_BLK)
**
** Description:
**	This routine is called to implement setting the block limit.
**	Breaking the code up this way simply reduces the overall size of
**	LK_alter().
**
** Inputs:
**	lkd			- pointer to the LKD
**	value			- the requested block limit value.
**
** Outputs:
**	None, but the small block table will be set up in the LKD
**
** Returns:
**	STATUS
**
** History:
**	24-may-1993 (bryanp)
**	    Broke this routine out of LK_alter to reduce routine sizes.
**	    Added validation for minimum locklist limit (determined empirically)
*/
static STATUS
set_block_limit(LKD *lkd, u_i4 value)
{	
    LGK_EXT		*ext;
    i4		err_code;

    if (value > LKD_MAXNUM_BLK)
    {
	uleFormat(NULL, E_DMA001_LK_BLK_TOO_MANY, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MAXNUM_BLK);
	return (LK_BADPARAM);
    }
    if (value < LKD_MINNUM_BLK)
    {
	uleFormat(NULL, E_DMA019_LK_BLK_TOO_FEW, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MINNUM_BLK);
	return (LK_BADPARAM);
    }
    if (lkd->lkd_rsb_inuse || lkd->lkd_lkb_inuse)
    {
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_lkb_inuse);
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_rsb_inuse);
	return (LK_BUSY);
    }
    if (lkd->lkd_sbk_table)
    {
	uleFormat(NULL, E_CL1020_LK_BUSY_ALTER, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, lkd->lkd_sbk_table);
	return (LK_BUSY);
    }

    ext = (LGK_EXT *) lgkm_allocate_ext(sizeof(SIZE_TYPE) * value);
    if (ext == NULL)
    {
	uleFormat(NULL, E_CL1022_LK_LOCK_ALLOC_FAILED, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, value);
	return (LK_NOLOCKS);
    }

    lkd->lkd_sbk_table = LGK_OFFSET_FROM_PTR(ext);
    lkd->lkd_sbk_count = 0;
    lkd->lkd_sbk_size = value -1;
    lkd->lkd_sbk_free = 0;

    return (OK);
}

/*
** Name: set_lock_hash_size		- implement LKalter(LK_I_LKH)
**
** Description:
**	This routine is called to allocate and initialize the lock hash.
**	Breaking the code up this way simply reduces the overall size of
**	LK_alter().
**
** Inputs:
**	lkd			- pointer to the LKD
**	value			- the requested block limit value.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	29-nov-1994 (medji01)
**	    Added this comment block.
*/
static STATUS
set_lock_hash_size(LKD *lkd, u_i4 value)
{
    LGK_EXT		*ext;
    i4		err_code;
    LKH 		*lkh;
    LKH    		*lkh_table;
    i4		i;
    char		sem_name[CS_SEM_NAME_LEN];

    if (value > LKD_MAXNUM_LKH)
    {
	uleFormat(NULL, E_DMA002_LK_LKH_TOO_MANY, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MAXNUM_LKH);
	return (LK_BADPARAM);
    }
    if (value < LKD_MINNUM_LKH)
    {
	uleFormat(NULL, E_DMA020_LK_LKH_TOO_FEW, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MINNUM_LKH);
	return (LK_BADPARAM);
    }
    if (lkd->lkd_lkh_table)
	return (LK_BUSY);

    lkd->lkd_lkh_table = 0;
    lkh = (LKH *) lgkm_allocate_ext(sizeof(LKH) * value);

    if (lkh == NULL)
    {
	uleFormat(NULL, E_CL1023_LK_LKH_ALLOC_FAILED, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, value);
	return (LK_NOLOCKS);
    }

    lkh_table = (LKH *)((char *)lkh + sizeof(LGK_EXT));
    lkd->lkd_lkh_table = LGK_OFFSET_FROM_PTR(lkh_table);

    for (i = 0; i < value; i++)
    {
	lkh = (LKH *) &lkh_table[i];
	lkh->lkh_lkbq_next = 0;
	lkh->lkh_lkbq_prev = 0;
	lkh->lkh_type = LKH_TYPE;
	if (LK_imutex(&lkh->lkh_mutex,
		STprintf(sem_name, "LK LKH mutex %8d", i)))
	    return(LK_NOLOCKS);
    }
    lkd->lkd_lkh_size = value;

    return (OK);
}

/*
** Name: set_resource_hash_size		- implement LKalter(LK_I_RSH)
**
** Description:
**	This routine is called to allocate and initialize the resource  
**	hash. Breaking the code up this way simply reduces the overall
**	size of LK_alter().
**
** Inputs:
**	lkd			- pointer to the LKD
**	value			- the requested block limit value.
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	29-nov-1994 (medji01)
**	    Added this comment block.
*/
static STATUS
set_resource_hash_size(LKD *lkd, u_i4 value)
{
    LGK_EXT		*ext;
    i4		err_code;
    RSH			*rsh;
    RSH    		*rsh_table;
    i4		i;
    char		sem_name[CS_SEM_NAME_LEN];

    if (value > LKD_MAXNUM_RSH)
    {
	uleFormat(NULL, E_DMA003_LK_RSH_TOO_MANY, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MAXNUM_RSH);
	return (LK_BADPARAM);
    }
    if (value < LKD_MINNUM_RSH)
    {
	uleFormat(NULL, E_DMA021_LK_RSH_TOO_FEW, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
		    0, value, 0, LKD_MINNUM_RSH);
	return (LK_BADPARAM);
    }
    if (lkd->lkd_rsh_table)
	return (LK_BUSY);

    lkd->lkd_rsh_table = 0;
    rsh = (RSH *) lgkm_allocate_ext(sizeof(RSH) * value);
    if (rsh == NULL)
    {
	uleFormat(NULL, E_CL1024_LK_RSH_ALLOC_FAILED, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, 0L, (i4 *)NULL, &err_code, 1,
		    0, value);
	return (LK_NOLOCKS);
    }

    rsh_table = (RSH *) ((char *)rsh + sizeof(LGK_EXT));
    lkd->lkd_rsh_table = LGK_OFFSET_FROM_PTR(rsh_table);

    for (i = 0; i < value; i++)
    {
	rsh = (RSH *) &rsh_table[i];
	rsh->rsh_rsb_next = 0;
	rsh->rsh_rsb_prev = 0;
	rsh->rsh_type = RSH_TYPE;
	if (LK_imutex(&rsh->rsh_mutex,
		STprintf(sem_name, "LK RSH mutex %8d", i)))
	    return(LK_NOLOCKS);
    }
    lkd->lkd_rsh_size = value;

    return (OK);
}

/*
** Name: resume_stalled_requests
**
** Description:
**
** Inputs:
**	stall_state		- New value for lkd_stall
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	16-apr-2001 (devjo01)
**	    created to replace 'complete_stalled_request'.
**	    Added this comment block.
**	10-apr-2002 (devjo01)
**	    consolidated code, removed bad (and unneeded) LLB
**	    calculations.
**	19-Mar-2007 (jonj)
**	    Note that LKBs are stalled, not LLBs, hence there's no
**	    need for the llb_mutex.
**	    Massage TRdisplay to supply better debug info.
**	19-Oct-2007 (jonj)
**	    Guard against null LLB, RSB, reset lkreq result_flags
**	    to indicate no longer on stall queue.
**	26-Oct-2007 (jonj)
**	    LKB_STALLED replaces LKREQ stall flags, show
**	    more info in display.
**	09-Jan-2009 (jonj)
**	    Deleted LKREQ, embed what's needed in LKB.
*/
static STATUS
resume_stalled_requests(i4 stall_state)
{
    LKD		    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    LKB		    *lkb;
    SIZE_TYPE	    end_offset, llbq_offset;
    LLBQ	    *llbq;
    LLBQ	    *next_llbq;
    LLBQ	    *prev_llbq;
    LLB		    *llb;
    RSB		    *rsb;
    STATUS	     status;
    i4		     filter;
    char	     keydispbuf[256];

    if ((status = LK_mutex(SEM_EXCL, &lkd->lkd_stall_q_mutex)))
	return (E_DMA029_LKALTER_SYNC_ERROR);

    lkd->lkd_lock_stall = stall_state;
    if ( 0 != lkd->lkd_lock_stall )
    {
	LK_dump_stall_queue( "Before resume physical" );
	filter = LKB_PHYSICAL;
    }
    else
    {
	LK_dump_stall_queue( "Before resume all" );
	filter = 0;
	lkd->lkd_node_fail = 0;
    }

    end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_stall_next);

    for (llbq_offset = lkd->lkd_stall_next;
	 llbq_offset !=  end_offset;
	)
    {
	llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);
	llbq_offset = llbq->llbq_next;

	lkb = (LKB *)((char *)llbq -
			CL_OFFSETOF(LKB,lkb_stall_wait));

	if (filter && ((lkb->lkb_attribute & filter) == 0))
	    continue;

	/*
	** Remove LKB from stall queue
	*/
	next_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq->llbq_next);
	prev_llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq->llbq_prev);
	next_llbq->llbq_prev = llbq->llbq_prev;
	prev_llbq->llbq_next = llbq->llbq_next;

	llb = (LLB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);
	rsb = (RSB*)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_rsb);

	TRdisplay("%@ Resume stalled LKB %x on LLB %x state %w gr %w rq %w\n"
		"\tKEY %s for PID %d (0x%x) SID %x, att %v\n",
	    lkb->lkb_id,
	    (llb) ? *(u_i4*)&llb->llb_id : 0,
	    LKB_STATE, lkb->lkb_state,
	    LOCK_MODE, lkb->lkb_grant_mode,
	    LOCK_MODE, lkb->lkb_request_mode,
	    (rsb) ? LKkey_to_string(&rsb->rsb_name, keydispbuf) : "no RSB",
	    lkb->lkb_cpid.pid,
	    lkb->lkb_cpid.pid,
	    lkb->lkb_cpid.sid,
	    LKB_ATTRIBUTE, lkb->lkb_attribute);

	/* Note LKB is unstalled now */
	lkb->lkb_attribute &= ~LKB_STALLED;

	LKcp_resume( &lkb->lkb_cpid );
    }

    LK_dump_stall_queue( "After resumes" );

    (VOID) LK_unmutex(&lkd->lkd_stall_q_mutex);

    return status;
}


/*
** Name: LK_dump_stall_queue
**
** Description:
**
** Inputs:
**	*message		- a string to be passed to TRdisplay()
**
** Outputs:
**	None.
**
** Returns:
**	STATUS
**
** History:
**	29-nov-1994 (medji01)
**	    Added this comment block.
**	14-Dec-1995 (jenjo02)
**	    Ensure that fuction caller has locked stall queue mutex.
**	19-Mar-2007 (jonj)
**	    Provide more useful info.
**	19-Oct-2007 (jonj)
**	    Guard against null LLB.
*/
void
LK_dump_stall_queue( char *message )
{
    LKD	    *lkd = (LKD *)LGK_base.lgk_lkd_ptr;
    SIZE_TYPE end_offset;
    SIZE_TYPE llbq_offset;
    LLBQ    *llbq;
    LKB	    *lkb;
    LLB	    *llb;

    TRdisplay("%@ Display stall queue (%s):\n", message);

    end_offset = LGK_OFFSET_FROM_PTR(&lkd->lkd_stall_next);

    for (llbq_offset = lkd->lkd_stall_next;
	 llbq_offset !=  end_offset;
	llbq_offset = llbq->llbq_next
	)
    {
	llbq = (LLBQ *)LGK_PTR_FROM_OFFSET(llbq_offset);

	lkb = (LKB *)((char *)llbq -
			    CL_OFFSETOF(LKB,lkb_stall_wait));
	llb = (LLB *)LGK_PTR_FROM_OFFSET(lkb->lkb_lkbq.lkbq_llb);

	TRdisplay("At %x, LKB %x LLB %x att %v -- n/p=(%x/%x)\n",
		llbq_offset,
		lkb->lkb_id,
		(llb) ? *(i4 *)&llb->llb_id : 0,
		LKB_ATTRIBUTE, lkb->lkb_attribute,
		llbq->llbq_next, llbq->llbq_prev);
    }

    return;
}
