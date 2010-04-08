/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <clconfig.h>
#include    <systypes.h>
#include    <rusage.h>
#include    <cs.h>
#include    <pc.h>

#include    <csinternal.h>

/**
**
**  Name: CSEF.C - Control System Event Flag routines.
**
**  Description:
**	This module contains the CS event flag routines.
**
**	The routines that drive the event mechanisms are:
**
**	    CS_ef_set	: set event wait flag for current session
**	    CS_ef_wait	: suspend current session until event is signalled
**	    CS_ef_wake	: signal event - wake up sessions waiting for event
**	    CS_ef_cancel: cancel current session's event wait flag
**
**	After calling CS_ef_set, the client must call either CS_ef_wait or
**	CS_ef_cancel.  Failing to do this will cause the system to get its
**	suspends and resumes out of sync.
**
**	The client of the event routines must be careful of the case where
**	the event desired occurs BEFORE the call to CS_ef_set.  If this
**	case cannot be handled by the client, then the calls to the event
**	routines should be protected by some sort of semaphore, as shown below.
**
**	The CS event routines are not intended to be high traffic - ultra fast
**	event drivers, they are used primarily to wait for out-of-resource
**	conditions to be resolved - these are not expected to happen very often.**
**	Event flag values should be reserved in CS.H.
**	Event waits are NOT interruptable.
**
**	The correct method for using the CS event routines is shown here (Note
**	that for brevity I have left out error checking - this doesn't condone
**	doing such in code).
**
**	    **
**	    ** Set semaphore before checking for resource, this prevents the
**	    ** race condition where the resource is freed immediately after
**	    ** we check for it, but before we set the wait flag.
**	    **
**	    CSp_semaphore(&Resource_semaphore);
**
**	    while ( ! resource_avialable)
**	    {
**		CS_ef_set(EVENT_FLAG);
**
**		** Release semaphore while waiting for event completion. **
**		CSv_semaphore(&Resource_semaphore);
**		CS_ef_wait();
**
**		CSp_semaphore(&Resource_semaphore);
**	    }
**
**	When releasing the resource:
**
**	    CSp_semaphore(&Resource_semaphore);
**	    free_resource();
**	    CS_ef_wake();	** Wake up anybody waiting for resource **
**	    CSv_semaphore(&Resource_semaphore);
**
[@func_list@]...
**
**  History:
**      24-jul-1989 (rogerk)
**          Created.
**	09-nov-1992 (mikem)
**	    su4_us5 port.  Using same compiler as su4_u42 so added NO_OPTIM
**	    entry.
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
**	9-aug-93 (robf)
**          add su4_cmw to NO_OPTIM
**      01-sep-1993 (smc)
**              Changed casts of sid to CS_SID.
**      24-jul-1996 (canor01)
**          integrate changes from cs.
**	06-sep-1996 (canor01)
**	    use scb->cs_self for sid, instead of scb with a cast.
**      18-feb-1997 (hanch04)
**          As part of merging of Ingres-threaded and OS-threaded servers,
**          use csmt version of this file.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**  Forward and/or External function references.
*/


/*
** Definition of all global variables used by this file.
*/

GLOBALREF CS_SYSTEM           Cs_srv_block;
GLOBALREF CS_SEMAPHORE        Cs_known_list_sem;


/*
** Name: CS_ef_wait - wait for specified event to occur.
**
** Description:
**	This routine suspends the current thread until a specified event
**	occurs.  The event must have been set using the CS_ef_set call before
**	the thread can call CS_ef_wait.  The event must be signaled by some
**	other thread calling CS_ef_wake with the same event flag.
**
**	The routines that drive the event mechanisms are:
**
**	    CS_ef_set	: set event wait flag for current session
**	    CS_ef_wait	: suspend current session until event is signalled
**	    CS_ef_wake	: signal event - wake up sessions waiting for event
**	    CS_ef_cancel: cancel current session's event wait flag
**
**	After calling CS_ef_set, the client must call either CS_ef_wait or
**	CS_ef_cancel.  Failing to do this will cause the system to get its
**	suspends and resumes out of sync.
**
**	The client of the event routines must be careful of the case where
**	the event desired occurs BEFORE the call to CS_ef_set.  If this
**	case cannot be handled by the client, then the calls to the event
**	routines should be protected by some sort of semaphore, as shown below.
**
**	The CS event routines are not intended to be high traffic - ultra fast
**	event drivers, they are used primarily to wait for out-of-resource
**	conditions to be resolved - these are not expected to happen very often.**
**	Event flag values should be reserved in CS.H.
**	Event waits are NOT interruptable.
**
**	The protocols for using the CS event wait calls are described above
**	in the module header.
**
** Inputs:
**      none
**
** Outputs:
**      none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	Thread is suspended until event is signalled by another session.
**
** History:
**      24-jul-89 (rogerk)
**          Created.
*/
VOID
CS_ef_wait()
{
    CS_SCB	*scb;

    CSget_scb(&scb);
    CSsuspend(0, 0, 0);
}

/*
** Name: CS_ef_set - set event wait flag for current session.
**
** Description:
**	This routine prepares a session for an event wait call.  It
**	marks the current session as waiting for the specified event.
**	If the event is signaled, then that session will be CSresume'd
**	regardless of whether or not the session has yet called CS_ef_wait.
**
**	After calling CS_ef_set, the caller MUST call either CS_ef_wait or
**	CS_ef_cancel.  Failure to do so will screw up the thread's suspend
**	resume protocols.
**
**	The protocols for using the CS event wait calls are described above
**	in the module header.
**
** Inputs:
**	event_flag	- event flag for which to wait.
**
** Outputs:
**      none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      24-jul-89 (rogerk)
**          Created.
*/
VOID
CS_ef_set(event_flag)
i4	    event_flag;
{
    CS_SCB	*scb;

    /*
    ** Set event flag in scb event wait mask, after this is done, any
    ** CS_ef_wake calls with this event flag will cause a CSresume on this
    ** session.
    */
    CSget_scb(&scb);
    scb->cs_ef_mask = event_flag;
}

/*
** Name: CS_ef_wake - signal event, wake up sessions waiting for it.
**
** Description:
**	This routine wakes all sessions (via CSresume call) that are waiting
**	for the specified event.
**
**	It scans the SCB queue looking for any sessions which have the
**	specified event wait flag value.
**
**	Note that the session may not have actually called CS_ef_wait yet
**	when we resume it - that session will just return immediately when
**	calling CS_ef_wait.
**
**	The protocols for using the CS event wait calls are described above
**	in the module header.
**
** Inputs:
**	event_flag	- event being signaled.
**
** Outputs:
**      none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      24-jul-89 (rogerk)
**          Created.
*/
VOID
CS_ef_wake(event_flag)
i4	    event_flag;
{
    CS_SCB	*scb;

    /*
    ** Scan the SCB queue looking for sessions with the cs_ef_mask set to
    ** this event flag.  When one is found, then call CSresume on it.
    ** Set inkernel so the scb list will not be mucked with while we are
    ** cycling through it.
    */
    CSp_semaphore( FALSE, &Cs_known_list_sem );

    for (scb = Cs_srv_block.cs_known_list->cs_next;
         scb != Cs_srv_block.cs_known_list;
         scb = scb->cs_next)
    {
	if (scb->cs_ef_mask & event_flag)
	{
	    scb->cs_ef_mask = 0;
	    CSresume(scb->cs_self);
	}
    }
    if (Cs_srv_block.cs_async)
	CS_move_async(Cs_srv_block.cs_current);

    CSv_semaphore( &Cs_known_list_sem );
}

/*
** Name: CS_ef_cancel - cancel event wait.
**
** Description:
**	This routine cancels an event wait for the current session.  It
**	turns off the event wait flag and cancels any CSresume that might
**	have been done if the event has already been signaled.
**
**	The protocols for using the CS event wait calls are described above
**	in the module header.
**
** Inputs:
**	none
**
** Outputs:
**      none
**
**	Returns:
**	    none
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none
**
** History:
**      24-jul-89 (rogerk)
**          Created.
*/
VOID
CS_ef_cancel()
{
    CS_SCB	*scb;

    CSget_scb(&scb);
    scb->cs_ef_mask = 0;
    CScancelled(0);
}
