/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <cs.h>
# include    <lk.h>
# include    <cx.h>
# include    <tm.h>
# include    <cm.h>
# include    <st.h>
# include    <er.h>
# include    <sa.h>
# include    <pm.h>
# include    <ulf.h>
# include    <dmf.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>

/*
** Name: SXAC.C - control routines for the SXF auditing system
**
** Description:
**	This file contains all of the control routines used by the SXF 
**	auditing system, SXA.
**
**	    sxac_startup      - initialize the auditing system
**	    sxac_bgn_session  - register a session with the auditing system
**	    sxac_shutdown     - shutdown the auditing system
**	    sxac_end_session  - remove a session from the auditing system
**	    sxac_audit_thread - perform the audit thread task
**	    sxac_audit_writer_thread - perform the audit writer thread task
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation as stubs.
**	15-oct-1992 (markg)
**	    Write routines to have full functionality.
**	26-nov-1992 (markg)
**	    Added new routines sxac_audit_thread() and sxac_audit_error().
**	20-feb-1993 (markg)
**	    In sxac_shutdown, only attempt to close the current audit file 
**	    if it is open.
**	10-may-93 (markg)
**	    Updated some comments. 
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	4-oct-93 (stephenb)
**	    Change event value parameter in call to LKevent to SXA_EVENT_WAKEUP,
**	    before we were using pointer which may cause problems on 64 bit
**	    ports.
**      11-jan-94 (stephenb)
**          Updated to include posibility of running with operating system
**          audit trails.
**	14-apr-94 (robf)
**          Add audit writer thread
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(). It's gross overkill to semaphore-protect a
**	    simple statistic.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-may-2001 (devjo01)
**	    Replace LGcluster() with CXcluster_enabled. s103715.
**	26-Aug-2009 (kschendel) 121804
**	    Need sa.h to satisfy gcc 4.3.
*/

/*
** Name: SXAC_STARTUP - startup the SXF auditing system
**
** Description:
**	This routine is used to initialize the SXF auditing system, it should
**	be called at facility startup. Only one call should be made to this
**	routine during the life of the facility. 
**
**	In future releases of SXF there may be many different low level 
**	audit file access routines, it will be up to the system administrator 
**	to decide which he/she wishes to use. This information will be 
**	stored in the configuration system, and retrieved using the PM 
**	interface. However, currently there is only one set of audit file 
**	access routines, so these will always be used.
**
** 	Overview of algorithm:-
**
**	Call sxas_startup to initialize the cached audit state.
**	Build a RSCB to be used for the current audit file.
**	Determine which audit file access routines to use.
**	Initialize the low level audit file access routines by calling the 
**	initialization routine specified in the table SXAP_INIT.
**	Put the RSCB onto the list of active RSCBs.
**
**
** Inputs:
**	none
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation as stub.
**	15-oct-1992 (markg)
**	    Written.
**	23-dec-93 (robf)
**          Mark auditing as stopped if physical layer tells us so.
**	29-dec-93 (robf)
**          Initialize stop_audit to FALSE. Also only reference ascb
**	    if its set (may not be set on non-C2 servers)
**      11-jan-94 (stephenb)
**          Added capability to run a version of sxap (sxapo), that writes to
**          operating system audit trails.
**	27-nov-1996 (canor01)
**	    If old default auditing mechanism is found, accept it, so 
**	    server can be started for upgrade.
**	   
*/
DB_STATUS
sxac_startup(
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    i4	local_err;
    SXF_RSCB	*rscb;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    bool	rscb_built = FALSE;
    bool	sxap_started = FALSE;
    bool	stop_audit=FALSE;
    char	*pmvalue;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	if ((sxf_status & (SXF_C2_SECURE | SXF_B1_SECURE)) == 0)
	{
	    break;
	}

	status = sxas_startup(&local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	status = sxau_build_rscb(&rscb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	rscb_built = TRUE;
	rscb->sxf_rscb_status |= SXRSCB_WRITE;

	/*
	** Currently there is only one set of audit file access 
	** methods to use - the default ones. In the future
	** there may be many different types. It is at this point
	** in the code that a decision about which access methods 
	** to use needs to be taken. For now we just assume that
	** we'll use the default ones.
	*/
	/*
	** (stephenb):
	** Above comment is no longer valid, we can now write to operating
	** system audit trails, here we check the PM file to decide which
	** access method the user wants and then initialize using the relavent
	** routine.
	*/

	/*
	** Call the low level audit file access methods 
	** initialization routine.
	*/
	if (PMget(SX_C2_AUDIT_METHOD,&pmvalue) == OK)
	{
	    if (!STbcompare(pmvalue, 0, SX_C2_OSAUDIT, 0, TRUE))
	    {
		/*
		** Operating system audit trail, check if it's enabled first.
		*/
		if (SAsupports_osaudit())
		{
		    status = (*Sxap_itab[SXAP_OSAUDIT])(rscb, &local_err);
		}
		else
		{
		    *err_code = E_SX10BA_OSAUDIT_NOT_ENABLED;
		    _VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	  	    break;
		}
	    }
	    else if (!STbcompare(pmvalue, 0, SX_C2_INGAUDIT, 0, TRUE) ||
	    	     !STbcompare(pmvalue, 0, SX_C2_OLDDEFAUDIT, 0, TRUE))
	    {
		/*
		** Ingres audit logs
		*/
		status = (*Sxap_itab[SXAP_DEFAULT])(rscb, &local_err);
	    }
	    else
	    {
		/*
		** Bad value
		*/
		*err_code = E_SX10B7_BAD_AUDIT_METHOD;
		_VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	  	break;
	    }
	}
	else
	{
	    /*
	    ** No value found, default to ingres audit logs
	    */
	    status = (*Sxap_itab[SXAP_DEFAULT])(rscb, &local_err);
	}

	if (status == E_DB_WARN &&
	   local_err==E_SX102D_SXAP_STOP_AUDIT)
	{
	    /*
	    ** Low level auditing didn't start up because no more
	    ** files so we should disable logical auditing also
	    */
	    stop_audit=TRUE;
	    status=E_DB_OK;
	}
	else if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(*err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	sxap_started = TRUE;

	rscb->sxf_rscb_status |= SXRSCB_OPENED;
	Sxf_svcb->sxf_write_rscb = rscb;

	/* Put the RSCB on the list of active RSCBs */
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	if (Sxf_svcb->sxf_rscb_list != NULL)
	    Sxf_svcb->sxf_rscb_list->sxf_prev = rscb;
	rscb->sxf_next = Sxf_svcb->sxf_rscb_list;
	Sxf_svcb->sxf_rscb_list = rscb;
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	break;
    }

    /* Clean up after any errors */
    if (*err_code != E_SX0000_OK)
    {	
	if (sxap_started)
	    _VOID_ (*Sxap_vector->sxap_term)(&local_err);

	if (rscb_built)
	    _VOID_ sxau_destroy_rscb(rscb, &local_err);

	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1009_SXA_STARTUP;

	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    /*
    ** If audit stop was requested by physical layer then 
    ** call sxas to do so
    */
    if(stop_audit==TRUE)
    {
        SXF_ASCB	*ascb = Sxf_svcb->sxf_aud_state;
	/* Mark server as stopped */
	Sxf_svcb->sxf_svcb_status |= SXF_AUDIT_DISABLED;
	if(ascb)
		ascb->sxf_lockval.lk_low=SXAS_STOP_LKVAL;
    }
    return (status);
}

/*
** Name: SXAC_SHUTDOWN - shutdown the SXF auditing system
**
** Description:
**	This routine is used to shutdown the SXF auditing system, it should
**	be called once at facility shutdown. Once the call has completed all
**	audit specific resources will have been freed.
**
** 	Overview of algorithm:-
**
**	For each active session call sxac_end_session
**	Close the current audit file and destroy the corresponding RSCB
**	Call the SXAP routine which shuts down the low level auditing system
**
** Inputs:
**	none
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation as stub.
**	15-oct-1992 (markg)
**	    Written.
**	20-feb-1993 (markg)
**	    Only attempt to close the current audit file if it is open.
*/
DB_STATUS
sxac_shutdown( 
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    i4	local_err;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_SCB	*scb;
    SXF_SCB	*scb_list = NULL; /* list of SCBs to process */

    *err_code = E_SX0000_OK;

    if ((sxf_status & (SXF_C2_SECURE | SXF_B1_SECURE)) == 0)
    {
	return (E_DB_OK);
    }

    /* Build a list of the active sessions */
    CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
    for (scb = Sxf_svcb->sxf_scb_list;
	 scb != NULL;
	 scb = scb->sxf_next)
    {
	scb->sxf_next = scb_list;
	scb_list = scb;
    }
    CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

    /* Call sxac_end_session for each SCB on the list */
    for (scb = scb_list;
	 scb != NULL;
	 scb = scb->sxf_next)
    {
	status = sxac_end_session(scb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(local_err, NULL,
		   ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}
    }

    /* 
    ** If the current audit file is still open call the low level
    ** file close routine.
    */
    if (Sxf_svcb->sxf_write_rscb->sxf_sxap != NULL)
    { 
	status = (*Sxap_vector->sxap_close)
			(Sxf_svcb->sxf_write_rscb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(local_err, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}
    }

    /* destroy the RSCB for the current audit file */
    status = sxau_destroy_rscb(Sxf_svcb->sxf_write_rscb, &local_err);
    if (status != E_DB_OK)
    {
	*err_code = local_err;
	_VOID_ ule_format(local_err, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
    }


    /* shutdown the low level auditing system */
    status = (*Sxap_vector->sxap_term)(&local_err);
    if (status != E_DB_OK)
    {
	*err_code = local_err;
	_VOID_ ule_format(local_err, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX100A_SXA_SHUTDOWN;

	if (status == E_DB_OK)
	    status = E_DB_ERROR; 
   }

    return (status);
}

/*
** Name: SXAC_BGN_SESSION - begin a session
**
** Description:
**	This routine should be called at session startup time to initialize
**	any audit specific data structures associated with the session. It
**	should be called only once for each session.
**
** 	Overview of algorithm:-
**
**	Call SXAP begin session routine which allocates session 
**	specific SXAP resourses.
**
** Inputs:
**	scb			SXF session control block.
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation as stub.
**	15-oct-1992 (markg)
**	    Written.
*/
DB_STATUS
sxac_bgn_session( 
    SXF_SCB	*scb,
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    i4	local_err;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;

    *err_code = E_SX0000_OK;

    if ((sxf_status & (SXF_C2_SECURE | SXF_B1_SECURE)) == 0)
    {
	return (status);
    }

    status = (*Sxap_vector->sxap_begin)(scb, &local_err);
    if (status != E_DB_OK)
    {
	_VOID_ ule_format(local_err, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);

	*err_code = E_SX100B_SXA_BGN_SESSION;
    }

    return (status);
}

/*
** Name: SXAC_END_SESSION - end a SXA session
**
** Description:
**	This routine is used to end a session in the SXF auditing system. It
**	will close an audit file opened by this session, and free any session
**	specific resourses. 
**
** 	Overview of algorithm:-
**
**	Locate open audit files belonging to this session.
**	For each open audit file belonging to this session
**	close the audit file and destroy te correspondig RSCB.
**
** Inputs:
**	scb			SXF_SCB of the session to end
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation as stub.
**	15-oct-1992 (markg)
**	    Written.
**	4-may-94 (robf)
**          Call the Physical layer end-session exit.
*/
DB_STATUS
sxac_end_session(   
    SXF_SCB	*scb, 
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    DB_STATUS	local_status = E_DB_OK;
    i4	local_err;
    SXF_RSCB	*rscb;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_RSCB	*rscb_destroy = NULL; /* list of RSCBs to be destroyed */

    *err_code = E_SX0000_OK;

    for (;;)
    {
	if ((sxf_status & (SXF_C2_SECURE | SXF_B1_SECURE)) == 0)
	{
	    break;
	}

	/*
	** Scan down the RSCB list for any open audit file
	** belonging to this session. If you find one, remove 
	** it from the active RSCB list and put it on the 
	** destroy list.
	*/
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	for (rscb = Sxf_svcb->sxf_rscb_list;
	     rscb != NULL;
	     rscb = rscb->sxf_next)
	{
	    if (rscb->sxf_scb_ptr == scb)
	    {
		/* remove from active list */
		if (rscb->sxf_prev != NULL)
		    rscb->sxf_prev->sxf_next = rscb->sxf_next;
		else
		    Sxf_svcb->sxf_rscb_list = rscb->sxf_next;
		if (scb->sxf_next != NULL)
		    rscb->sxf_next->sxf_prev = rscb->sxf_prev;

		/* put it on the destroy list */
		rscb->sxf_next = rscb_destroy;
		rscb_destroy = rscb;
	    }
	}
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	/*
	** For each RSCB on the destroy list, close the audit
	** file, then destroy the RSCB.
	*/
	for (rscb = rscb_destroy;
	     rscb != NULL;
	     rscb = rscb->sxf_next)
	{
	    status = (*Sxap_vector->sxap_close)(rscb, &local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
	        if (*err_code > E_SXF_INTERNAL)
		    _VOID_ ule_format(local_err, NULL, ULE_LOG, 
				NULL, NULL, 0L, NULL, &local_err, 0);
	    }

	    status = sxau_destroy_rscb(rscb, &local_err);
	    if (status != E_DB_OK)
	    {
		*err_code = local_err;
	        if (*err_code > E_SXF_INTERNAL)
		    _VOID_ ule_format(local_err, NULL, ULE_LOG, 
				NULL, NULL, 0L, NULL, &local_err, 0);
	    }
	}
	/*
	** Tell the physical layer the session is ended
	*/
        local_status = (*Sxap_vector->sxap_end)(scb, &local_err);
	if(local_status != E_DB_OK)
	{
		*err_code = local_err;
	        if (*err_code > E_SXF_INTERNAL)
		    _VOID_ ule_format(local_err, NULL, ULE_LOG, 
				NULL, NULL, 0L, NULL, &local_err, 0);

		if(local_status>status)
			status=local_status;
	}
	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX100C_SXA_END_SESSION;

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAC_AUDIT_THREAD - perform the audit thread task
**
** Description:
**	This routine is the entry point for the audit thread. The audit 
**	will only exist in secure servers (i.e. C2 or B1 servers).
**	The task performed by this thread is to maintian consistency of the
**	SXF audit state cache (a cached copy of tuples from the
**	iisecurity_state catalog in the iidbdb. The main loop of the audit
**	thread resides in SCF (see scs!scsdbfcn.c) and performs the 
**	following:-
**	
**	    do forever
**		attempt to open the iidbdb
**		call SXAC_AUDIT_THREAD
**		if the iidbdb is open
**		   close the iidbdb
**		suspend thread
**	    end do
**
**	Any user thread that detects that the audit state cache may be
**	inconsistent wakes up the audit thread and then waits for a lock
**	event to be signalled (see sxa!sxas.c). The audit thread opens the 
**	iidbdb, calls SXF to update the audit state cache, closes the iidbdb
**	and then goes back to sleep.
**
**	NOTE: It may be possible that SCF is unable to open the iidbdb
**	because it doesn't exist yet (i.e. createdb iidbdb). This 
**	condition must be handled in this routine.	
**
** 	Overview of algorithm:-
**
**	Get the SCB for the audit thread session
**	Call sxas_cache_init to initialize that audit state cache.
**	Signal waiting threads to resume.
**
** Inputs:
**	rcb			Request control block from caller
**
** Outputs:
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	26-nov-1992 (markg)
**	    Initial creation.
**	31-dec-1992 (robf)
**	    Error handling changes:
**	    - Add E_DM012A_CONFIG_NOT_FOUND as a "legal" error when trying
**	      to open the iidbdb (which is what DMF seems to return when
**	      iidbdb is missing sometimes)
**	    - Add E_SX1068_AUD_THREAD_IIDBDB_ERR to specifically link
**	      previous error messages with the audit thread trying to open
**	      the iidbdb and failing for some reason.
**	    - Distinguish problem getting the SCB for the audit thread
**	      rather than returning the generic 'audit thread error'
**	    - If sxas_cache_init returns TABLE_NOT_FOUND this is a "non-error"
**	      we were resetting the errcode but not status. Result was 
**	      the audit thread (and server) would terminate with an error
**            of "0" in the error log. Cure by also resetting status to E_DB_OK.
**	    - Add warning W_SX1069_AUD_THREAD_NO_IIDBDB when the IIDBDB
**            doesn't exist. This goes in the error log after all the
**            lower level DMF/SCF errors so we can tell where these messages
**            come from.
**	2-jun-93 (robf)
**	    Only print W_SX1069 once per occurrance, reset when successful
**	    open of iidbdb (this stops the error log getting clogged with
**	    multiple errors)
**	    Increment wakeup counter stats
**	    Add check for SUSPEND audit state on VaxClusters. In this case
**	    we have to poll to wake up any suspended threads in this
**	    server, since LKevents() don't currently work across nodes
**	    of a cluster.
**	4-oct-93 (stephenb)
**	    Change event value parameter in call to LKevent to SXA_EVENT_WAKEUP,
**	    before we were using pointer which may cause problems on 64 bit
**	    ports.
**	12-jul-94 (robf)
**          Allow for E_DM0113_DB_INCOMPATABLE, returned when upgrading
**          from an earlier release of INGRES.
**	30-Sep-2004 (schka24)
**	    Non-upgraded iidbdb's now get DM0086's.  Add to the list.
*/

DB_STATUS
sxac_audit_thread(
    SXF_RCB	*rcb)
{
    DB_STATUS	status = E_DB_OK;
    STATUS	cl_status;
    i4	err_code = E_SX0000_OK;
    CL_ERR_DESC	cl_err;
    i4	local_err;
    SXF_SCB	*scb;
    LK_LLID	lk_ll;
    SXF_ASCB	*ascb = Sxf_svcb->sxf_aud_state;
    LK_EVENT	lk_event;

    for(;;)
    {
        Sxf_svcb->sxf_stats->audit_wakeup++;

	/* Store the thread's session id in the ASCB */
	if (ascb->sxf_aud_thread == 0)
	    CSget_sid(&ascb->sxf_aud_thread);

	/* get the scb for the session */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    /* 
	    ** If we fail to find the scb for the Audit Thread this
	    ** is a server fatal condition. There is no point in 
	    ** attempting to wake up any waiting threads because
	    ** we are not able to locate the lock list to use.
	    */
	    _VOID_ ule_format(local_err, NULL, ULE_LOG,
			 NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(E_SX1067_AUD_THREAD_NO_SCB, NULL, 
		    ULE_LOG, NULL, NULL, 0l, NULL, &local_err, 0);
	    rcb->sxf_error.err_code = E_SX0005_INTERNAL_ERROR;
	    return(E_DB_FATAL);
	}
	lk_ll = scb->sxf_lock_list;

# if 0	/* Exclude problematic cluster code. */

	/*
	** If a cluster check for SUSPEND processing
	*/
	if(CXcluster_enabled()) 
	{
		/*
		** On a cluster, so poll for audit suspend. 
		*/
		for(;;)
		{
			i4 timeout=5;
			/*
			** Call SXAS to poll for resume
			*/
			sxf_set_activity("Polling for RESUME operation within cluster");
			status=sxas_poll_resume(&local_err);
			if(status==E_DB_OK)
				break;
			else if(status!=E_DB_WARN)
			{
				_VOID_ ule_format(local_err, NULL, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
				break;
			}
			sxf_set_activity("Waiting to check RESUME operation within cluster");
			cl_status=CSsuspend(CS_LOCK_MASK|CS_TIMEOUT_MASK, 
					timeout, NULL);

			if(cl_status!=OK && cl_status!=E_CS0009_TIMEOUT)
			{
				_VOID_ ule_format(cl_status, NULL, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
				break;
			}
		}
		sxf_clr_activity();
	}
# endif
	/*
	** If SCF was unsuccessful while attempting to
	** open the iidbdb for the audit thread it will have 
	** passed in the error code here. The failure to open 
	** open the iidbdb may not necessarily be an error (for 
	** example if the iidbdb does not yet exist). We need to
	** examine the error code here and determine the next course
	** of action. If the iidbdb doesn't exist, or if it's busy or
	** inconsistent, we should return without updating the cache, 
	** any other database open problem will cause an error to be 
	** returned to SCF.
	*/
	if (rcb->sxf_msg_id)
	{
	    if (rcb->sxf_msg_id == E_DM004C_LOCK_RESOURCE_BUSY ||
		rcb->sxf_msg_id == E_DM0100_DB_INCONSISTENT ||
		rcb->sxf_msg_id == E_DM012A_CONFIG_NOT_FOUND ||
		rcb->sxf_msg_id == E_DM0113_DB_INCOMPATABLE ||
		rcb->sxf_msg_id == E_DM0086_ERROR_OPENING_DB ||
		rcb->sxf_msg_id == E_DM0053_NONEXISTENT_DB)
	    {
		/*
		** Log a message that audit state not refreshed since iidbdb
		** not accessible
		*/
		if(!(Sxf_svcb->sxf_svcb_status&SXF_NO_IIDBDB))
		{
			_VOID_ ule_format(W_SX1069_AUD_THREAD_NO_IIDBDB, NULL, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
			Sxf_svcb->sxf_svcb_status|=SXF_NO_IIDBDB;
		}
		err_code = E_SX0000_OK;
	    }
	    else
	    {
		/*
		** Log the reason why we  couldn't access the iidbdb
		*/
		_VOID_ ule_format(rcb->sxf_msg_id, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		/*
		** Set the error code to the specific error saying SXF
		** was unable to open the iidbdb.
		*/
		err_code=E_SX1068_AUD_THREAD_IIDBDB_ERR;
		_VOID_ ule_format(err_code, NULL, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    }

	    break;
	}
	else
	{
		/*
		** Connected to iidbdb so reset error indicator
		*/
		Sxf_svcb->sxf_svcb_status &= ~SXF_NO_IIDBDB;
	}

	/* initialize the audit state cache */
	status = sxas_cache_init(scb, &local_err);
	if (status != E_DB_OK)
	{
	    if (local_err == E_SX0018_TABLE_NOT_FOUND)
	    {
		/* 
		** This error status is returned if the iisecuritystate
		** catalog does not exist. This may happen if the dbdb is
		** being upgraded from an earlier version that doesn't 
		** support security auditing. This is not an error.
		*/
		err_code = E_SX0000_OK;
		status = E_DB_OK;
	    }
	    else
	    {
		err_code = local_err;
		_VOID_ ule_format(err_code, NULL, ULE_LOG,
			 NULL, NULL, 0l, NULL, &local_err, 0);           
	    }
	    break;
	}

	break;
    }

    /* signal all waiting threads to wake up */
    lk_event.type_high = SXA_EVENT;
    lk_event.type_low = 0;
    lk_event.value = SXA_EVENT_WAKEUP;

    cl_status = LKevent(LK_E_CLR, lk_ll, &lk_event, &cl_err);

    if (cl_status != OK)
    {
	err_code = E_SX1047_BAD_EVENT_CLEAR;
	_VOID_ ule_format(err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
    }

    /* handle any errors */
    if (err_code != E_SX0000_OK)
    {
	_VOID_ ule_format(E_SX103F_AUDIT_THREAD_ERROR, NULL, 
		ULE_LOG, NULL, NULL, 0l, NULL, &local_err, 0);
	status = sxac_audit_error();
	if (status != E_DB_OK)
	    rcb->sxf_error.err_code = E_SX0005_INTERNAL_ERROR;
    }

    return (status);
}

/*
** Name: SXAC_AUDIT_ERROR 
**
** Description:
**	If an error occurs during security audit processing, this
**	routine is called to determine the action that should take place.
**	Because the handling of errors of this type could affect the
**	security of the system, the system adminstrator is allowed to
**	specify what action should take place. The two options available are
**	either a) shutdown the server, or b) disable auditing for the
**	server. This information is stored in the PM system. The relevant PM
**	resource is loaded into the SXF_SVCB at facility startup time.
**
**	When this routine is called, the SXF_SVCB is consulted to determine
**	what action should be taken, and either the DBMS is terminated, or
**	security auditing is disabled. If the DBMS is terminated, this routine
**	will not return to the caller. If auditing is disabled, a value of
**	E_DB_OK is returned to the caller.
**
** Inputs:
**	None.
**
** Outputs:
**	None.
**
** Returns:
**	E_DB_OK if auditing has been disabled.
**	May not return if the DBMS is to be terminated.
**
** Side Effects:
**	Either the server will be terminated, or security auditing will be
**	disabled.
**
** History:
**	26-nov-1992 (markg)
**	    Initial creation.
**	31-dec-1992 (robf)
**	    Updated messages to refer to general audit processing rather
**	    than just writing audit records since this can be called from
**          other places, like the audit thread.
**	    Changed E_SX1037 to ONERR_SERVER_SHUTDOWN to better distinguish
**	    from some other kind (normal?) shutdown.
**	6-aug-93 (robf)
**	    Notify audit state auditing should be stopped
*/
DB_STATUS
sxac_audit_error()
{
    i4	error;
    DB_STATUS   status=E_DB_OK;

    for(;;)
    {
	if (Sxf_svcb->sxf_act_on_err == SXF_ERR_STOPAUDIT)
	{	
	    _VOID_ ule_format(E_SX1049_AUDIT_DISABLED, NULL, ULE_LOG,
	    		NULL, NULL, 0L, NULL, &error, 0);

	    status=sxas_stop_audit(FALSE, &error);
	    if(status!=E_DB_OK)
	    {
		/*
		** Stop audit failed, this is regarded as a server fatal
		** condition.
		*/
	        _VOID_ ule_format(error, NULL, ULE_LOG,
	    		NULL, NULL, 0L, NULL, &error, 0);
	        _VOID_ ule_format(E_SX1053_SHUTDOWN_NOW, NULL, ULE_LOG,
	    		NULL, NULL, 0L, NULL, &error, 0);
	        _VOID_ CSterminate(CS_KILL, NULL);
	    }
	}
	else
	{
	    _VOID_ ule_format(E_SX1037_ONERR_SERVER_SHUTDOWN, NULL, ULE_LOG,
	    		NULL, NULL, 0L, NULL, &error, 0);
	    _VOID_ ule_format(E_SX1053_SHUTDOWN_NOW, NULL, ULE_LOG,
	    		NULL, NULL, 0L, NULL, &error, 0);
	    _VOID_ CSterminate(CS_KILL, NULL);
	}
	break;
    }
    return status;
}	    

/*
** Name: SXAC_AUDIT_WRITER_THREAD - perform the audit writer thread task
**
** Description:
**	This routine is the entry point for the audit writer thread. The audit 
**	writer thread will only exist in secure servers (i.e. C2 or B1 servers).
**	The task performed by this thread is to help write out records from
**	the security audit queue in SXAP when it gets full.
**	
** Inputs:
**	rcb			Request control block from caller
**
** Outputs:
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	14-apr-94  (robf)
**	    Initial creation.
**	3-jun-94 (robf)
**          Don't initialize the aud_writer_thread value here, it should
**	    be encapsulated inside SXAP.
*/

DB_STATUS
sxac_audit_writer_thread(
    SXF_RCB	*rcb)
{
    DB_STATUS	status = E_DB_OK;
    STATUS	cl_status;
    i4	err_code = E_SX0000_OK;
    CL_ERR_DESC	cl_err;
    i4	local_err;
    SXF_SCB	*scb;
    LK_LLID	lk_ll;
    SXF_ASCB	*ascb = Sxf_svcb->sxf_aud_state;

    for(;;)
    {
        Sxf_svcb->sxf_stats->audit_writer_wakeup++;


	/* get the scb for the session */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    /* 
	    ** If we fail to find the scb for the Audit Writer Thread this
	    ** is a server fatal condition. 
	    */
	    _VOID_ ule_format(local_err, NULL, ULE_LOG,
			 NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(E_SX10C5_AUDIT_WRITER_NO_SCB, NULL, 
		    ULE_LOG, NULL, NULL, 0l, NULL, &local_err, 0);
	    rcb->sxf_error.err_code = E_SX0005_INTERNAL_ERROR;
	    return(E_DB_FATAL);
	}
	/*
	** Now start flushing audit records
	*/
        status = (*Sxap_vector->sxap_alter)(
			scb,
			NULL,
			SXAP_C_AUDIT_WRITER,
			&err_code);
	break;
    }
    if(status==E_DB_WARN)
    {
	/*
	** Some unusual condition occurred.
	*/
	if(err_code==E_SX102D_SXAP_STOP_AUDIT)
	{
		/*
		** Auditing should be stopped, so set stopped bit.
		*/
		status=sxas_stop_audit(FALSE, &err_code);
		if(status!=E_DB_OK)
			status=E_DB_FATAL;
		else
			err_code=E_SX0000_OK;
	}
	else if(err_code==E_SX10AE_SXAP_AUDIT_SUSPEND)
	{
		/*
		** Auditing should be suspended, so suspend auditing
		*/
		status=sxas_suspend_audit(FALSE, &err_code);
		if(status!=E_DB_OK)
			status=E_DB_FATAL;
		else
			err_code=E_SX0000_OK;
	}
	else
	{
		status=E_DB_ERROR;
	}
    }
    /*
    ** Handle any errors
    */
    if (err_code != E_SX0000_OK)
    {
        /* handle any errors */
	_VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0l, NULL, &local_err, 0);
	_VOID_ ule_format(E_SX10C6_AUDIT_WRITER_ERROR, NULL, 
		ULE_LOG, NULL, NULL, 0l, NULL, &local_err, 0);
	status = sxac_audit_error();
	if (status != E_DB_OK)
	    rcb->sxf_error.err_code = E_SX0005_INTERNAL_ERROR;
    }
    return (status);
}
