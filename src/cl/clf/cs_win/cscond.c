/*
** Copyright (c) 1997, 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <sp.h>
#include    <cs.h>
#include    <pc.h>
#include    <csinternal.h>

#include    "cslocal.h"

/*
**  Name: CSCONDITION.C - Control System CONDITION Routines
**
**  Description:
**      This module contains those routines which provide support for
**      the CS_CONDITION concept.
**
**      CScnd_get_name	- get the name bound to a CS_CONDITION strucuture
**      CScnd_name		- attach a name to a CS_CONDITION strucuture
**      CScnd_broadcast	- wake all waiters on a condition
**      CScnd_signal	- condition signal
**      CScnd_wait		- condition wait
**      CScnd_free		- free condition object
**      CScnd_init		- initialize condition object
**
** History:
**	15-jul-95 (emmag)
**	    Use a NULL Discretionary Access Control List (DACL) for 
**	    security when creating events and semaphores, to give 
**	    implicit access to everyone.
**	24-jul-95 (emmag)
**	    Back to using csintern.h for now.  
**	12-sep-1995 (shero03)
**	    Added sync_obj and register with MO
**	30-oct-1995 (canor01)
**	    pSCB not needed
**      20-may-1997 (canor01)
**          Prevent a closed handle from hanging around.  Set it to
**          zero when closed.  If we are signaled at server shutdown,
**          it may not be valid.
**	30-sep-1997 (canor01)
**	    Protect access to scb flags with cs_access_sem.  Double-check
**	    CS_ABORTED_MASK on CScnd_wait().
**	13-oct-1997 (canor01)
**	    In CScnd_signal, do not create a mutex that already exists.
**	06-aug-1999 (mcgem01)
**	    Replace nat and longnat with i4.
**	06-jan-2000 (somsa01)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	18-Jan-2000 (fanra01)
**	    In CScnd_wait(), replace the assignment of scb from cs_current
**	    to using CSget_scb().
**	12-feb-2001 (somsa01)
**	    Removed prototype of CS_find_scb(), as it is not needed.
**	17-mar-2003 (somsa01)
**	    Remove usage of 'cnd_mutex_sem'.  All critical data items
**	    are actually protected by 'cnd_event_sem', and 'cnd_mutex_sem'
**	    is just harmful (leaks mem on Tru64) baggage.
**	22-jan-2004 (somsa01)
**	    Cleaned up CS_CNDWAIT condition handling.
**	28-jan-2004 (somsa01)
**	    Corrected typo in CScnd_wait().
*/

/******************************************************************************
** # defines
******************************************************************************/

/******************************************************************************
** typedefs
******************************************************************************/

/******************************************************************************
** forward references
******************************************************************************/

/******************************************************************************
** externs
******************************************************************************/

/******************************************************************************
** statics
******************************************************************************/

/*
** Name: CScnd_init - initialize condition object
**
** Description:
**  Prepare a CS_CONDITION structure for use.
**
** Inputs:
**  cond            CS_CONDITION structure to prepare
**
** Outputs:
**  cond            Prepared CS_CONDITION structure
**
**  Returns:
**      OK
**      not OK      Usage error
**
**  Exceptions:
**      none
**
** Side Effects:
**  none.
**
** History:
**	22-jan-2004 (somsa01)
**	    Updated to remove cnd_event_sem and MO stuff and cnd_waiters and
**	    cnd_tid, added cnd_mtx and cnd_waiter and cnd_next and cnd_prev.
*/
STATUS
CScnd_init(CS_CONDITION *cnd)
{
    cnd->cnd_mtx = NULL;
    cnd->cnd_waiter = NULL;
    cnd->cnd_next = cnd->cnd_prev = cnd;
    cnd->cnd_name = NULL;

    return(OK);
}

/*
** Name: CScnd_free - free condition object
**
** Description:
**  Release any internal state or objectes related to the condition
**
**  Note that the condition object should not be used again
**  unless re-initialized.
**
** Inputs:
**  cond            CS_CONDITION structure to free
**
** Outputs:
**  none
**
** Returns:
**  OK
**  !OK      Usage error possibly due to freeing a condition
**           on which some other session is waiting
**
** Exceptions:
**  none
**
** Side Effects:
**  none.
**
** History:
**	22-jan-2004 (somsa01)
**	    Removed cnd_event_sem and MO stuff.
*/
STATUS
CScnd_free(CS_CONDITION *cnd)
{
    if (!cnd || cnd->cnd_waiter != NULL || cnd != cnd->cnd_next)
	return(FAIL);

    return(OK);
}

/*
** Name: CScnd_wait - condition wait
**
** Description:
**  Wait for a condition to be signaled, releasing
**  a semaphore while waiting and getting the semaphore again
**  once awakened.  Users of CScnd_wait must be prepared for
**  CScnd_wait to return without the condition actually being
**  met.  Typical use will be the body of a while statement
**  which performs the actual resource check.
**
**  A CScnd_wait may return even when the condition is not met.
**  CScnd_wait may even return without being signaled to wake a waiter.
**  This requires the user to check that whatever the session
**  is waiting for has happened.
**
**  When waiting on a condition, the same mutex should be
**  specified on each wait.  (i.e. a mutex can be associated with
**  several conditions, but a condition can be associated with
**  only one mutex.)
**
**  Internally to CScnd_wait, a session may be in either of two
**  wait states:
**      1) waiting for the condition to be signalled or broadcast, and
**      2) waiting to re-acquire the semaphore.
**
**  These two wait states should be distinguished by CS, so that
**  users of CS (such as iimonitor) can identify in which state a
**  session is currently blocked.
**
** Inputs:
**  cond            CS_CONDITION to wait upon.
**  mutex           CS_SEMAPHORE to release while waiting
**
** Outputs:
**  mutex           CS_SEMAPHORE released and reacquired
**
**  Returns:
**      OK
**      not OK          Usage error
**
**      E_CS000F_REQUEST_ABORTED    Thread was removed during the wait
**
**  Exceptions:
**      none
**
** Side Effects:
**  The mutex must be released atomically with the wait upon the
**  condition and reacquired before return.
**
** History:
**	20-may-1997 (canor01)
**	    Prevent a closed handle from hanging around.  Set it to
**	    zero when closed.  If we are signaled at server shutdown,
**	    it may not be valid.
**	30-sep-1997 (canor01)
**	    Protect access to scb flags with cs_access_sem.  Double-check
**	    CS_ABORTED_MASK. 
**	06-jan-2000 (somsa01)
**	    Standardize cs_self = (CS_SID *)scb, session id's are always
**	    a pointer to the thread's SCB, never OS thread_id, which is
**	    contained in cs_thread_id.
**	18-Jan-2000 (fanra01)
**	    Replace the assignment of scb from cs_current to using
**	    CSget_scb().
**	22-jan-2004 (somsa01 for jenjo02)
**	    Rearchitected to eliminate persistent race conditions.
**	    External semaphore is acquired even if REQUEST_ABORTED.
**	28-jan-2004 (somsa01)
**	    We should be waiting for cs_event_sem to be signaled, not
**	    cs_access_sem. And, release and reaquire cs_access_sem when
**	    waiting.
*/
STATUS
CScnd_wait(CS_CONDITION *cnd, CS_SEMAPHORE *mtx)
{
    CS_SCB	*scb;
    STATUS	status = OK, mstatus;

    CSget_scb(&scb);

    if (!cnd || !(cnd->cnd_mtx = mtx))
	return(FAIL);

    WaitForSingleObject(scb->cs_access_sem, INFINITE);

    if (scb->cs_mask & CS_IRPENDING_MASK)
    {
	/* interrupt received while thread was computable */
	scb->cs_mask &= ~CS_IRPENDING_MASK;
	scb->cs_mask |= CS_ABORTED_MASK;
    }
    if (scb->cs_mask & CS_ABORTED_MASK)
    {
	scb->cs_mask &= ~CS_ABORTED_MASK;
        ReleaseMutex(scb->cs_access_sem);
	return(E_CS000F_REQUEST_ABORTED);
    }

    scb->cs_state = CS_CNDWAIT;
    scb->cs_cnd = &scb->cs_cndq;
    scb->cs_sync_obj = (PTR)cnd;

    /* Add this SCB (last) to list of waiters */
    scb->cs_cndq.cnd_name = cnd->cnd_name;
    scb->cs_cndq.cnd_mtx = mtx;
    scb->cs_cndq.cnd_waiter = scb;
    scb->cs_cndq.cnd_next = cnd;
    scb->cs_cndq.cnd_prev = cnd->cnd_prev;
    cnd->cnd_prev->cnd_next = &scb->cs_cndq;
    cnd->cnd_prev = &scb->cs_cndq;

    /* Release the semaphore, then wait */
    if (status = CSv_semaphore(mtx))
    {
	/* Something wrong with semaphore, abort */
	scb->cs_cnd = NULL;
	scb->cs_sync_obj = NULL;
	scb->cs_state = CS_COMPUTABLE;

	/*
	** If the semaphore was not held, then the
	** cnd queue may well be corrupted now, but
	** try to repair it anyway...
	*/
	scb->cs_cndq.cnd_next->cnd_prev = scb->cs_cndq.cnd_prev;
	scb->cs_cndq.cnd_prev->cnd_next = scb->cs_cndq.cnd_next;
	ReleaseMutex(scb->cs_access_sem);
    }
    else
    {
	/* Wait 'til brought out of our reverie */
	while ( scb->cs_state == CS_CNDWAIT &&
		!(scb->cs_mask & CS_ABORTED_MASK) )
	{
	    /*
	    ** What we will do now is similar to CS_cond_wait() on UNIX
	    ** (which is atomic, but is not available on Windows). First,
	    ** we'll release cs_access_sem, then wait until cs_event_sem is
	    ** signalled, and then reacquire cs_access_sem.
	    */
	    ReleaseMutex(scb->cs_access_sem);
	    WaitForSingleObject(scb->cs_event_sem, INFINITE);
	    WaitForSingleObject(scb->cs_access_sem, INFINITE);
	}

	if (scb->cs_mask & CS_ABORTED_MASK)
	    status = E_CS000F_REQUEST_ABORTED;

	scb->cs_mask &= ~CS_ABORTED_MASK;
	scb->cs_state = CS_COMPUTABLE;
	ReleaseMutex(scb->cs_access_sem);

	mstatus = CSp_semaphore(1, mtx);
    }

    return ((status) ? status : mstatus);
}

/*
** Name: CScnd_signal - condition signal
**
** Description:
**  Signal that one waiter on the condition should be set runable.
**
** Inputs:
**  cond            CS_CONDITION to wake up.
**  sid             CS_SID of specific session to wake up, or zero.
**
** Outputs:
**  none
**
**  Returns:
**      OK
**      not OK      Usage error
**
**  Exceptions:
**      none
**
** Side Effects:
**  The awakened session will reacquire a semaphore.
**  Typically, the signaler will hold that semaphore when
**  signaling so the awakened thread will not really run until the
**  semaphore is released.  See examples.
**
** History:
**	13-oct-1997 (canor01)
**	    Do not create a mutex that already exists.
**	22-jan-2004 (somsa01 for jenjo02)
**	    Added CS_SID to awaken a specific session.
*/
STATUS
CScnd_signal(CS_CONDITION *cnd, CS_SID sid)
{
    CS_SCB		*scb = NULL;
    CS_CONDITION	*cndp;

    /* Caller must hold cnd_mtx */

    /* Signal specific thread? */
    if (sid)
    {
	scb = CSfind_scb(sid);

	if (scb)
	{
	    WaitForSingleObject(scb->cs_access_sem, INFINITE);

	    /* Error if not waiting on this "cnd" */
	    if (scb->cs_sync_obj != (PTR)cnd || scb->cs_state != CS_CNDWAIT)
	    {
		ReleaseMutex(scb->cs_access_sem);
		return(FAIL);
	    }

	    cndp = scb->cs_cnd;
	}
	else
	    return(FAIL);
    }
    else if ((cndp = cnd->cnd_next) != cnd)
    {
	/* Signal first thread on queue */
	scb = cndp->cnd_waiter;
	WaitForSingleObject(scb->cs_access_sem, INFINITE);
    }

    if (scb && cndp)
    {
	/* Remove from condition queue */
	cndp->cnd_next->cnd_prev = cndp->cnd_prev;
	cndp->cnd_prev->cnd_next = cndp->cnd_next;

	scb->cs_state = CS_COMPUTABLE;
	scb->cs_cnd = NULL;
	scb->cs_sync_obj = NULL;

	/* Wake it up */
	SetEvent(scb->cs_event_sem);
	ReleaseMutex(scb->cs_access_sem);
    }

    return(OK);
}

/*
** Name: CScnd_broadcast - wake all waiters on a condition
**
** Description:
**  Set runable all waiters on the condition.
**
** Inputs:
**  cond            CS_CONDITION to wake up.
**
** Outputs:
**  none
**
**  Returns:
**      OK
**      not OK      Usage error
**
**  Exceptions:
**      none
**
** Side Effects:
**  The awakened sessions will reacquire a semaphore.
**  Typically, the signaler will hold that semaphore when
**  signaling so the awakened threads will not really run until the
**  semaphore is released.  And then they will run only as
**  the semaphore becomes available.
**
** History:
**	22-jan-2004 (somsa01 for jenjo02)
**	    Rearchitected to use condition queue to avoid all sorts of
**	    past race conditions.
*/
STATUS
CScnd_broadcast(CS_CONDITION *cnd)
{
    CS_CONDITION	*cndq = cnd;
    CS_SCB		*scb;

    /* Caller must hold cnd_mtx */

    if (!cnd || cnd->cnd_waiter != NULL)
	return FAIL;

    /* Drain the queue... */
    while ((cndq = cndq->cnd_next) != cnd)
    {
	scb = cndq->cnd_waiter;

	WaitForSingleObject(scb->cs_access_sem, INFINITE);

	scb->cs_state = CS_COMPUTABLE;
	scb->cs_cnd = NULL;
	scb->cs_sync_obj = NULL;

	/* Wake it up */
	SetEvent(scb->cs_event_sem);
	ReleaseMutex(scb->cs_access_sem);
    }

    /* Queue is empty now */
    cnd->cnd_next = cnd->cnd_prev = cnd;

    return (OK);
}

/******************************************************************************
** Name: CScnd_name - attach a name to a CS_CONDITION strucuture
**
** Description:
**  Attach a name to a CS_CONDITION for debugging and monitoring
**  uses.
**
** Inputs:
**  cond        CS_CONDITION to name
**  name        String containing name The space for the string must be left
**              until a subsequent CScnd_name or a CScnd_free.
**
** Outputs:
**  none
**
**  Returns:
**      OK
**      not OK      Usage error
**
**  Exceptions:
**      none
**
** Side Effects:
**  The CL may register the condition with a monitoring service
**  not part of CS.
**
******************************************************************************/
STATUS
CScnd_name(CS_CONDITION *cond,
           char         *name)
{
	cond->cnd_name = name;
	return OK;
}

/******************************************************************************
**
** Name: CScnd_get_name - get the name bound to a CS_CONDITION strucuture
**
** Description:
**  Return the name which may have been bound to the CS_CONDITION
**  structure.
**
** Inputs:
**  cond            CS_CONDITION to name
**
** Outputs:
**  none
**
** Returns:
**  pointer to string   pointer to the name (NULL if no bound name)
**
** Exceptions:
**  none
**
** Side Effects:
**  none
**
******************************************************************************/
char *
CScnd_get_name(CS_CONDITION *cond)
{
	return cond->cnd_name;
}
