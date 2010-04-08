/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lk.h>
#include    <scf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dm0s.h>

/**
**
**  Name: DM0S.C - SCF interface routines.
**
**  Description:
**      This module implements SCF interface routines.
**
**          dm0s_minit - Initialize mutex.
**          dm0s_mlock - Lock a mutex.
**          dm0s_mrelease - Release a mutex.
**          dm0s_munlock - Unlock a mutex.
**	    dm0s_elprepare - Prepare for event using LKrequest.
**	    dm0s_elrelease - Release event using LKrelease.
**	    dm0s_elwait - Wait for event using LKrequest.
**	    dm0s_erelease - Release event.
**	    dm0s_ewait - Wait for event.
**	    dm0s_eset - Set an event.
**
**
**  History:
**      01-oct-1986 (Derek)
**	    Created for Jupiter.
**	28-feb-1989 (rogerk)
**	    Initialize semaphores with CSi_semaphore call.
**	    Changed DB_STATUS types to STATUS where appropriate.
**	07-jul-92 (jrb)
**	    Prototype DMF.
**	17-jul-92 (daveb)
**	    Actually call CSr_semaphore in dm0s_mrelease, duh.
**	30-sep-92 (daveb)
**	    Add dm0s_name to name things.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	09-may-1995 (cohmi01)
**	    Added return value in dm0s_ewait for interrupt of read-ahead thread.
**	26-Oct-1996 (jenjo02)
**	    Added *name parameter to dm0s_minit() prototype to eliminate
**	    need for dm0s_name() function call.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**      05-Aug-2005 (hanje04)
**          Back out change 478041 as it should never have been X-integrated
**          into main in the first place.
**	08-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat.
[@history_template@]...
**/

/*{
** Name: DM0S_MINIT	- Initialize a mutex.
**
** Description:
**	Call CS to initialize semaphore.
**	All semaphores used by DM0S routines are single process semaphores.
**
**	Note that DM_MUTEX structure must be defined to mimic CS_SEMAPHORE
**	structure.
**
** Inputs:
**      mutex                           Pointer to mutex.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Signal dmd_check on errors.
**
** History:
**      01-oct-1986 (Derek)
**	    Created for Jupiter.
**	28-feb-1989 (rogerk)
**	    Initialize semaphores with CSi_semaphore call.
**	23-Oct-1992 (daveb)
**	    log sem errors.
**	25-Apr-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	26-Oct-1996 (jenjo02)
**	    Added *name parameter to dm0s_minit() prototype to eliminate
**	    need for dm0s_name() function call.
[@history_template@]...
*/
VOID
dm0s_minit(DM_MUTEX *mutex, char *name)
{
    STATUS	    status;
    i4             err_code;

    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	status = CSw_semaphore((CS_SEMAPHORE *)mutex, CS_SEM_SINGLE, name);
	if (status == OK)
	    return;
	uleFormat(NULL, status, NULL, ULE_LOG , NULL, 
	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0 );
	dmd_check(E_DMF000_MUTEX_INIT);
    }
}

/*{
** Name: DM0S_NAME	- name a mutex.
**
** Description:
**	Add a name to a semaphore, for debugging and monitoring purposes.
**
** Inputs:
**      mutex                           Pointer to mutex.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-sep-92 (daveb)
**	    Created 
*/
VOID
dm0s_name(DM_MUTEX *mutex, char *name)
{
    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
	CSn_semaphore((CS_SEMAPHORE *)mutex, name );
}

/*{
** Name: DM0S_MLOCK     - Lock a mutex.
**
** Description:
**      Call SCF to lock a mutex for exclusive access.
**
** Inputs:
**      mutex                           Pointer to mutex.
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          Signal dmd_check on errors.
**
** History:
**      01-oct-1986 (Derek)
**          Created for Jupiter.
**      23-Oct-1992 (daveb)
**          log sem errors.
**	16-Mar-2001 (thaju02)
**	    If session waiting on mutex is deleted via ipm, do not 
**	    perform dmd_check, but rather requeue request for mutex.
**	    Session will eventually be aborted. (B104254)
[@history_template@]...
*/
VOID
dm0s_mlock(DM_MUTEX *mutex)
{
    STATUS          status;
    i4             err_code;
	 
    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	while(1)
	{
	status = CSp_semaphore(1,mutex);
	if (status == OK)
	    return;
	    else if (status != E_CS000F_REQUEST_ABORTED)
	    {
	uleFormat(NULL, status, NULL, ULE_LOG , NULL,
	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0 );
	dmd_check(E_DMF001_MUTEX_LOCK);
		break;
	    }
	}
    }
}

/*{
** Name: DM0S_MRELEASE	- Release a mutex.
**
** Description:
**      Call SCF to release the mutex.
**
** Inputs:
**      mutex                           Pointer to mutex.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Signal dmd_check on errors.
**
** History:
**      01-oct-1986 (Derek)
**	    Created for Jupiter.
**	17-jul-92 (daveb)
**	    Actually call CSr_semaphore, duh.
**	23-Oct-1992 (daveb)
**	    log sem errors.
[@history_template@]...
*/
VOID
dm0s_mrelease(DM_MUTEX *mutex)
{
    STATUS	    status;
    i4             err_code;

    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	status = CSr_semaphore((CS_SEMAPHORE *)mutex);
	if (status == OK)
	    return;
	uleFormat(NULL, status, NULL, ULE_LOG , NULL, 
	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 0 );
	dmd_check(E_DMF002_MUTEX_FREE);
    }
}

/*{
** Name: DM0S_MUNLOCK   - Unlock a mutex.
**
** Description:
**      Call SCF to unlock a mutex.
**
** Inputs:
**      mutex                           Pointer to mutex.
**
** Outputs:
**      Returns:
**          VOID
**      Exceptions:
**          none
**
** Side Effects:
**          Signal dmd_check on errors.
**
** History:
**      01-oct-1986 (Derek)
**          Created for Jupiter.
**      23-Oct-1992 (daveb)
**          log sem errors.
[@history_template@]...
*/
VOID
dm0s_munlock(DM_MUTEX *mutex)
{
    STATUS          status;
    i4             err_code;
	     
    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
         status = CSv_semaphore(mutex);
	 if (status == OK)
	     return;
	 uleFormat(NULL, status, NULL, ULE_LOG , NULL,
	     (char *)NULL, 0L, (i4 *)NULL, &err_code, 0 );
	 dmd_check(E_DMF003_MUTEX_UNLOCK);
    }
}

/*{
** Name: dm0s_eset - Set an event.
**
** Description:
**      This routine set an event that other threads can clear it
**	with a dm0s_erelease call.
**
** Inputs:
**      lock_list                       Normally a session lock list.
**	type				Type of event : DM0S_E_MEM or DM0S_E_TBL
**      event                           A unique event id.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Dmd_checks on errors.
**
** History:
**      01-Jul-1987 (ac)
**          Created for Jupiter.
*/
VOID
dm0s_eset(
    i4             lock_list,
    i4		type,
    i4		event)
{
    STATUS		status;
    CL_ERR_DESC		sys_err;
    LK_EVENT		lk_event;

    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	/* Set the event. */
	lk_event.type_high = type;
	lk_event.type_low = 0;
	lk_event.value = event;

	status = LKevent(LK_E_SET, lock_list, &lk_event, &sys_err);
	if (status == OK)
	    return;
	dmd_check(E_DMF015_EVENT_SET);
    }
}

/*{
** Name: dm0s_elprepare	- Prepare an event.
**
** Description:
**      This routine prepares to declare a event that other threads can wait 
**	for with a dm0s_elwait call.  If the event could not be prepared an
**	error is returned to signal that a dm0s_elwait should be performed
**	wait for the event to clear.  This call is used when the event to
**	declare could already be in progress.  This allows the caller to
**	wait for the event before declaring or preparing again.
**
** Inputs:
**      lock_list                       Normally a session lock list.
**	type				Type of event : DM0S_E_MEM or DM0S_E_TBL
**      event                           A unique event id.
**
** Outputs:
**	Returns:
**	    E_DB_OK		    Event declared.
**	    E_DB_INFO		    Event nor prepared.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Dmd_checks on errors.
**
** History:
**      02-oct-1986 (Derek)
**          Created for Jupiter.
**	25-oct-93 (vijay)
**	    Add type cast to i4 for key.
[@history_template@]...
*/
DB_STATUS
dm0s_elprepare(
    i4             lock_list,
    i4		    type,
    i4		    event)
{
    LK_LOCK_KEY         key;
    STATUS		status;
    CL_ERR_DESC		sys_err;
    i4             err_code;
    LK_EVENT		lk_event;

    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	/*	Initialize lock key for the event. */

	key.lk_type = LK_SS_EVENT;
	key.lk_key1 = (i4) dmf_svcb->svcb_id;
	key.lk_key2 = type;
	key.lk_key3 = event;
	key.lk_key4 = key.lk_key5 = key.lk_key6 = 0;

	/*	Lock the resource. */

	status = LKrequest(LK_PHYSICAL | LK_NOWAIT | LK_NONINTR,
	    lock_list, &key, LK_X, 0, 0, 0, &sys_err);
	if (status == OK)
	    return (E_DB_OK);
	if (status == LK_BUSY)
	{
	    lk_event.type_high = type;
	    lk_event.type_low = 0;
	    lk_event.value = event;

	    status = LKevent(LK_E_SET, lock_list, &lk_event, &sys_err);
	    if (status == E_DB_OK)
		return (E_DB_INFO);
	}
	uleFormat(NULL, E_DM901C_BAD_LOCK_REQUEST, &sys_err, ULE_LOG , NULL, 
	    (char *)NULL, 0L, (i4 *)NULL, &err_code, 2,
	    0, LK_X, 0, lock_list);
	if (status == LK_NOLOCKS)
	    return(status);
	dmd_check(E_DMF016_EVENT_LWAIT);
    }
    return (E_DB_OK);
}

/*{
** Name: dm0s_elrelease	- Release event using LKrelease.
**
** Description:
**      This routine releases a previous declared event.  All dm0s_elwait caller
**	are rescheduled.
**
** Inputs:
**      lock_list                       Normally a session lock list.
**	type				Type of event : DM0S_E_MEM or DM0S_E_TBL
**      event                           A unique event id.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Dmd_checks on errors.
**
** History:
**      02-oct-1986 (Derek)
**          Created for Jupiter.
[@history_template@]...
*/
VOID
dm0s_elrelease(
    i4             lock_list,
    i4		    type,
    i4		    event)
{
    LK_LOCK_KEY         key;
    STATUS		status;
    CL_ERR_DESC		sys_err;
    i4             local_error;
    LK_EVENT		lk_event;

    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	/*	Initialize lock key for the event. */

	key.lk_type = LK_SS_EVENT;
	key.lk_key1 = (i4) dmf_svcb->svcb_id;
	key.lk_key2 = type;
	key.lk_key3 = event;
	key.lk_key4 = key.lk_key5 = key.lk_key6 = 0;

	/*	Unlock the resource. */

	if (status = LKrelease(0, lock_list, 0, &key, 0, &sys_err))
	{
	    uleFormat(NULL, E_DM901B_BAD_LOCK_RELEASE, &sys_err, ULE_LOG , NULL, 
		(char *)NULL, 0L, (i4 *)NULL, &local_error, 1,
		0, lock_list);
	}
	else
	{
	    lk_event.type_high = type;
	    lk_event.type_low = 0;
	    lk_event.value = event;

	    status = LKevent(LK_E_CLR, lock_list, &lk_event, &sys_err);
	    if (status == OK)
		return;
	}
	dmd_check(E_DMF004_EVENT_LRELEASE);
    }
}

/*{
** Name: dm0s_elwait	- Wait for event using LKrequest.
**
** Description:
**      This routine waits for an event to be signaled by dms0_elrelease.
**
** Inputs:
**      lock_list                       Normally a session lock list.
**	type				Type of event : DM0S_E_MEM or DM0S_E_TBL
**      event                           A unique event id.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Dmd_checks on errors.
**
** History:
**      02-oct-1986 (Derek)
**          Created for Jupiter.
[@history_template@]...
*/
VOID
dm0s_elwait(
    i4             lock_list,
    i4		type,
    i4		event)
{
    STATUS		status;
    CL_ERR_DESC		sys_err;
    LK_EVENT		lk_event;

    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	lk_event.type_high = type;
	lk_event.type_low = 0;
	lk_event.value = event;

	status = LKevent(LK_E_WAIT, lock_list, &lk_event, &sys_err);
	if (status == OK)
	    return;
	dmd_check(E_DMF016_EVENT_LWAIT);
    }
}

/*{
** Name: dm0s_erelease	- Release event.
**
** Description:
**      This routine clears a declared event.  All dm0s_ewait caller
**	are rescheduled.
**
** Inputs:
**      lock_list                       Normally a session lock list.
**	type				Type of event : DM0S_E_MEM or DM0S_E_TBL
**      event                           A unique event id.
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    Dmd_checks on errors.
**
** History:
**      02-oct-1986 (Derek)
**          Created for Jupiter.
*/
VOID
dm0s_erelease(
    i4             lock_list,
    i4		type,
    i4		event)
{
    STATUS		status;
    CL_ERR_DESC		sys_err;
    LK_EVENT		lk_event;

    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	/* Clear the event. */
	lk_event.type_high = type;
	lk_event.type_low = 0;
	lk_event.value = event;

	status = LKevent(LK_E_CLR, lock_list, &lk_event, &sys_err);
	if (status == OK)
	    return;

	dmd_check(E_DMF006_EVENT_RELEASE);
    }
}

/*{
** Name: dm0s_ewait	- Wait for event.
**
** Description:
**      This routine waits for an event to be signaled 
**	by dms0_erelease.
**
** Inputs:
**      lock_list                       Normally a session lock list.
**	type				Type of event : DM0S_E_MEM or DM0S_E_TBL
**      event                           A unique event id.
**
** Outputs:
**	Returns:
**	    DB_STATUS			E_DB_OK	- normal wake up
**					E_DB_WARN if readahead and we were
**						interrupted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Dmd_checks on errors.
**
** History:
**      02-oct-1986 (Derek)
**          Created for Jupiter.
**	09-may-1995 (cohmi01)
**	    Added return value for interrupt of read-ahead thread.    
*/
DB_STATUS
dm0s_ewait(
    i4             lock_list,
    i4		    type,
    i4		    event)
{
    STATUS		status;
    CL_ERR_DESC		sys_err;
    i4             event_flag = LK_E_WAIT;
    LK_EVENT		lk_event;


    if ((dmf_svcb->svcb_status & SVCB_SINGLEUSER) == 0)
    {
	/* if processing readahead thread, allow interupts to cancel */
	if (type == DM0S_READAHEAD_EVENT)
	    event_flag |= LK_E_INTERRUPT;

	/* Wait the event. */
	lk_event.type_high = type;
	lk_event.type_low = 0;
	lk_event.value = event;

	status = LKevent(event_flag, lock_list, &lk_event, &sys_err);
	if (status == OK)
	    return E_DB_OK;
	else
	if (status == LK_INTERRUPT && type == DM0S_READAHEAD_EVENT)
	    return E_DB_WARN;
	else
	    dmd_check(E_DMF007_EVENT_WAIT);

	/* dmd_check() should never return */
	return (E_DB_ERROR);

    }
    return E_DB_OK;
}
