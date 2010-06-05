
/*
**Copyright (c) 2004 Ingres Corporation
*/
 
# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <cs.h>
# include    <cx.h>
# include    <sl.h>
# include    <cv.h>
# include    <er.h>
# include    <pm.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <cs.h>
# include    <lk.h>
# include    <tm.h>
# include    <lo.h>
# include    <me.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
# include    <dmf.h>
# include    <dmccb.h>
# include    <dmxcb.h>
# include    <dmtcb.h>
# include    <dmrcb.h>
# include    <dudbms.h>
# include    <scf.h>


/*
** Name: SXAS.C - SXF audit state management routines
**
** Description:
**      This file contains the routines that are used to manage the 
**	audit state in the SXF auditing system. The audit state is used
**	to determine what different audit event types should cause
**	audit records to get written to the audit file. This is done
**	by looking up different event types in the SXF audit state cache.
**	All the routines used to manage the cache, and the checking of
**	different audit states are contained in this module. The are:-
**
**      sxas_check   -  check to see if a particular event should be audited
**	sxas_show    -  return information about the current audit configuration
**	sxas_alter   -  alter values in the audit state cache
**      sxas_startup -  initialize the audit state cache subsystem
**	sxas_cache_init -  initialize the audit state cache from the iidbdb
**
**	Static routines in this file are:-
**
**	sxas_change_file  - change the audit file to a new value
**	sxas_stop_audit   - suspend auditing
**	sxas_restart_audit- suspend auditing
**	sxas_suspend_audit- suspend auditing
**	sxas_resume_audit - resume auditing
**	sxas_open_sectab  - open the iisecuritystate catalog
**	sxas_close 	  - close a catalog
**	sxas_read_rec     - read a record from a table
**	sxas_replace_rec  - replace a record in a table
**	sxas_begin_xact   - begin a transaction
**	sxas_commit       - commit the current transaction
**	sxas_abort        - abort the current transaction
**	sxas_open_labtab  - open the iilabelaudit catalog
**
** History:
**      11-aug-1992 (markg)
**          Initial creation.
**	11-nov-1992 (markg)
**	    Added new rountines to support access to the iidbdb to access
**	    the current security audit state.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	10-may-1993 (markg)
**	    Updated some comments.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	7-jul-93 (robf)
**	    Add security level auditing for B1 DBMS
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	29-jul-93 (robf)
**	    Add audit state suspension check, sxas_suspend_audit() and
**	    sxas_resume_audit()
**	2-aug-93 (robf)
**	    Add sxas_change_file()
**	4-oct-93 (stephenb)
**	    change event value parameter in calls to LKevent() to
**	    SXA_EVENT_WAKEUP, before we were using a pointer which can cause
**	    problems on a 64 bit port.
**	26-jan-95 (stephenb)
**	    When executing DISABLE SECURITY AUDIT ALL we should only open
**	    iilabelmap if we are in B1 mode.
**      22-feb-95 (newca01)
**          Removed fix for bug 66592, change #415773, because it caused 
**	    problems for utilities.
**	01-May-1995 (jenjo02)
**	    Combined CSi|n_semaphore functions calls into single
**	    CSw_semaphore call.
**	13-jun-96 (nick)
**	    LINT directed changes.
**	27-nov-1996 (canor01)
**	    Changed sid from i4  to CS_SID and removed some unused sid
**	    variables.
**	17-feb-97 (mcgem01)
**          File overlooked in the Set Lockmode Cleanup:
**          Change readlock values according to new definitions.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	20-Jul-1998 (jenjo02)
**	    LKevent() event type and value enlarged and changed from 
**	    u_i4's to *LK_EVENT structure.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Nov-2000 (wansh01)
**	    changed if (err_code = E_DM0054_NONEXISTENT_TABLE ) in line 2166 
**          and 2616 from '=' to '=='  to do an error compare not set.   
**	30-may-2001 (devjo01)
**	    Replace LGcluster() with CXcluster_enabled. s103715.
**      14-sep-2005 (horda03) Bug 115198/INGSRV3422
**          On a clustered Ingres installation, LKrequest can return
**          LK_VAL_NOTVALID. This implies that teh lock request has been
**          granted, but integrity of the lockvalue associated with the
**          lock is in doubt. It is the responsility of the caller
**          to decide whether the lockvalue should be reset.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameter to LKrequest avoid random implicit
**	    lock conversions.
**      07-Apr-2007 (hweho01)
**          In sxas_commit(),  initialize dmx_option in the transaction    
**          control block, avoid getting into the wrong routine in 
**          dmx_commit(). Correct the typo error in status assignment.
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/
/*
** Forward function references.
*/
static DB_STATUS sxas_open_sectab(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_close(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_open_labtab(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_read_rec(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_replace_rec(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_begin_xact(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_commit(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_abort(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code);

static DB_STATUS sxas_insert_rec (
	SXAS_DMF	*sxldmf,
	i4 *err_code
);

static DB_STATUS sxas_delete_rec (
	SXAS_DMF	*sxldmf,
	i4 *err_code
);
static DB_STATUS sxas_enable_disable_audit(
    SXF_RCB	*rcb)
;

static DB_STATUS sxas_resume_audit(bool, i4 *, bool, char*);

static DB_STATUS sxas_restart_audit(i4 *, bool, char*);

static DB_STATUS sxas_change_file (SXF_RCB *rcb);

static DB_STATUS sxas_get_state_value(  
	LK_LLID	 *lk_ll,
	LK_VALUE *lk_newval,
	i4   *err_code
);
static DB_STATUS sxas_save_state_value(  
	LK_LLID	 *lk_ll,
	LK_VALUE *lk_newval,
	i4   *err_code
);

/*
** Name: SXAS_CHECK - check if an event type is enabled for auditing
**
** Description:
**	This routine consults the audit state cache to determine if a
**	particular event type has been enabled for auditing. To ensure
**	consistency of the cache a locking protocol is used. If the
**	cache is found to be invalid it is reinitialized from the 
**	underlying system catalog iisecuritystate.
**
**	Overview of functions in this routine:-
**	
**	Lock the audit state cache in shared mode
**	If (cache invalid) 
**		Release cache lock and take semaphore
**		Signal the audit thread to refresh the cache.
**		Wait for the refresh to complete.
**		Lock the audit state cache in S mode and release semaphore
**	Check cache entry for specified event type
**	Release cache lock.
**
** Inputs:
**	scb			Session control block of the caller.
**	event_type		Audit event type to check.
**
** Outputs:
**	result			Contains TRUE if auditing is enabled
**				for this event type, otherwise FALSE.
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-92 (markg)
**		Initial creation.
**	7-jul-93 (robf)
**	  - Check for valid event type on input, otherwise could get 
**	    AV on invalid  array reference
**	  - Add  per-user check for  query text auditing.
**	  - Add security label check for B1 DBMS
**	  - Check for SUSPEND/STOP lock values
**	1-oct-93 (robf)
**	   For single user servers audit state is fixed at start, (note,
**	   no audit threads to do refreshes)
**	4-oct-93 (stephenb)
**	    change event value parameter in calls to LKevent() to
**	    SXA_EVENT_WAKEUP, before we were using a pointer which can cause
**	    problems on a 64 bit port.
**	20-oct-93 (robf)
**	   Query text audit events should not disjoint from AUDIT_ALL
**	   events.
**         More tracing/cleanup for restart processing.
**	8-nov-93 (robf)
**         Add a check to make sure the audit thread id points to something
**	   before trying to CSresume() it - if not set likely to cause an AV.
**	29-feb-94 (stephenb)
**	    Make sure single user servers don't refresh the audit state cache
**	    it's done at startup time.
*/ 
DB_STATUS
sxas_check( 
    SXF_SCB	*scb,
    SXF_EVENT	event_type,
    bool	*result,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
    LK_LLID		lk_ll = scb->sxf_lock_list;
    SXF_ASCB		*ascb = Sxf_svcb->sxf_aud_state;
    bool		locked = FALSE;
    i4		sxf_status = Sxf_svcb->sxf_svcb_status;
    LK_EVENT		lk_event;

    *err_code = E_SX0000_OK;
    *result = FALSE;

	
	
    for(;;)
    {
	/*
	** If initializing the cache then stop
	*/
        if(scb->sxf_flags_mask& SXF_SESS_CACHE_INIT)
		break;

	/*
	** Check for valid event type (otherwise could get AV on
	** array dereference below)
	*/
	if(event_type<0 || event_type > SXF_E_MAX)
	{
		*err_code=E_SX0004_BAD_PARAMETER;
	        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	}

	/*
	** Get the audit state lock in shared mode. If the lockvalue
	** is not the same as the one stored in the audit state control 
	** block, our cached copy of the audit state may be out of date -
	** in this case we should reinitialize the audit cache from the 
	** underlying catalogs in the iidbdb.
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
	    LK_S, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = TRUE;
	
	/*
	** Check if stopping auditing, we only do this the first
	** time around
	*/
	if(lk_val.lk_low==SXAS_STOP_LKVAL &&
	   lk_val.lk_low != ascb->sxf_lockval.lk_low)
	{
	    if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY("Audit STOP detected\n");
	    /*
	    ** Mark audit as disabled for this server
	    */
	    Sxf_svcb->sxf_svcb_status |= SXF_AUDIT_DISABLED;
	    /*
	    ** Call the physical layer to disable for this server
	    */
	    status = (*Sxap_vector->sxap_alter)(
			scb,
			NULL,
			SXAP_C_STOP_AUDIT,
			err_code);
	    if(status!=E_DB_OK)
		break;
	    /*
	    ** Update the internal lock state to stopped
	    */
	    ascb->sxf_lockval.lk_low=SXAS_STOP_LKVAL;
	    break;
	}
	/*
	** Check for audit restart, this is detected when the internal
	** state is disabled by lock value, and the real lock value is
	** a regular active value.
	*/
	if(ascb->sxf_lockval.lk_low==SXAS_STOP_LKVAL &&
	   lk_val.lk_low>=SXAS_MIN_LKVAL)
	{
	        if(SXS_SESS_TRACE(SXS_TAUD_STATE))
			SXF_DISPLAY("Audit RESTART detected\n");
		/*
		** Call physical layer to restart audit
		*/
	        status = (*Sxap_vector->sxap_alter)(
			scb,
			NULL,
			SXAP_C_RESTART_AUDIT,
			err_code);
	  	/*
		** Check if failed because no files or some  other error
		*/
		if(status==E_DB_WARN &&
		   *err_code==E_SX104C_SXAP_NEED_AUDITLOGS)
		{
			/*
			** No auditing before, and still no auditing
			** so continue. 
			*/
			status=E_DB_OK;
			*err_code=E_SX0000_OK;
			break;
		}
		else if(status!=E_DB_OK)
		{
	        	if(SXS_SESS_TRACE(SXS_TAUD_STATE))
				SXF_DISPLAY("Physical layer failed to restart\n");
	    		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
			break;
		}
		/*
		** Remove disabled flag from the server
		*/
	        Sxf_svcb->sxf_svcb_status &= ~SXF_AUDIT_DISABLED;
		/*
		** We continue on here to refresh the audit state 
		** cache since it may have changed while auditing was
		** stopped
		*/
	}

	/* 
	** make sure we don't do this for single user servers it could
	** be bad news (there's no audit thread).
	*/
	if ((lk_val.lk_high != ascb->sxf_lockval.lk_high ||
	    lk_val.lk_low  != ascb->sxf_lockval.lk_low) &&
	    !(Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER))
	{
	    if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY("Audit state change detected\n");
	    /*
	    ** If we get here the cache is invalid. Before
	    ** we signal the audit thread to reinitialize it
	    ** we must release the lock that we hold. To avoid 
	    ** the possibility of multiple sessions attempting 
	    ** signal the audit thread simultainiously, access is
	    ** serialized using a semaphore.
	    */
	    cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	    if (cl_status != OK)
	    {
		*err_code = E_SX1008_BAD_LOCK_RELEASE;
		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    locked = FALSE;
	    /*
	    ** Check for audit thread, serious error if not available here
	    ** (and likely to get an AV)
	    */
	    if(!ascb->sxf_aud_thread)
	    {
		*err_code = E_SX103F_AUDIT_THREAD_ERROR;
		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    CSp_semaphore(TRUE, &ascb->sxf_aud_sem);
	    if (lk_val.lk_high != ascb->sxf_lockval.lk_high ||
		lk_val.lk_low  != ascb->sxf_lockval.lk_low)
	    {
		lk_event.type_high = SXA_EVENT;
		lk_event.type_low = 0;
		lk_event.value = SXA_EVENT_WAKEUP;

		cl_status = LKevent(LK_E_SET, lk_ll, &lk_event, &cl_err);

		if (cl_status != OK)
		{
		    *err_code = E_SX1045_BAD_EVENT_SET;
		    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
		    CSv_semaphore(&ascb->sxf_aud_sem);
		    break;
		}
		sxf_set_activity("Waiting for Security Audit state to be refreshed");

		CSresume(ascb->sxf_aud_thread);

		cl_status = LKevent(LK_E_WAIT, lk_ll, &lk_event, &cl_err);

		sxf_clr_activity();
		if (cl_status != OK)
		{
		    *err_code = E_SX1046_BAD_EVENT_WAIT;
		    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
		    CSv_semaphore(&ascb->sxf_aud_sem);
		    break;
		}
	    }
	    CSv_semaphore(&ascb->sxf_aud_sem);

	    cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_S, &lk_val, &lk_id, 0, &cl_err);
	    if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	    {
                TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
                _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		*err_code = E_SX1035_BAD_LOCK_REQUEST;
		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    locked = TRUE;
	}
	/*
	** Check if audit state changed due to audit suspension.
	** We do NOT do this for threads with  MAINTAIN_AUDIT privilege 
	** since they continue processing even when auditing is suspended.
	**
	** If the suspend lock value is set, then we wait for someone
	** to resume things. Since resuming involves updating the
	** suspend lock value, we set the event prior to releasing the
	** suspend lock, and then wait for the event.
	**
	*/
	if(lk_val.lk_low==SXAS_SUSPEND_LKVAL &&
	    !(scb->sxf_ustat & DU_UALTER_AUDIT))
	{
	    	if(SXS_SESS_TRACE(SXS_TAUD_STATE))
			SXF_DISPLAY("Audit SUSPEND detected\n");
		/*
		** Update the internal lock values
		*/
        	ascb->sxf_lockval.lk_low=lk_val.lk_low;
		ascb->sxf_lockval.lk_high=lk_val.lk_high;

	        if((Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER) &&
		    CXcluster_enabled())
		{
		    /*
		    ** Special processing for single user servers
		    ** on a cluster. Since there is no audit thread on
		    ** single user servers, and we can't do
		    ** anything until audit is resume, we poll instead.
		    */
		    /* Release the lock first so resumer can update it */
		    cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, 
				&cl_err);
		    if (cl_status != OK)
		    {
			*err_code = E_SX1008_BAD_LOCK_RELEASE;
			_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
			status=E_DB_ERROR;
			break;
		    }
		    locked = FALSE;
		    /* Now loop polling */
		    for(;;)
		    {
			/* 
			** Sleep a while (10 seconds)
			*/
			PCsleep(10000);
			/*
			** Poll for resume
			*/
			status=sxas_poll_resume(err_code);
			if(status==E_DB_OK)
			{
				/*
				** Auditing resumed, so carry on
				*/
				break;
			}
			else if(status!=E_DB_WARN)
			{
				/*
				** Error polling, break
				*/
				_VOID_ ule_format(*err_code, NULL, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
				break;
			}
			/* Default, continue looping */
		    }
		    /*
		    ** Stop if something went wrong
		    */
		    if(status!=E_DB_OK)
			break;
		}
		else
		{
		    /*
		    ** Normal (multi-user server or non-cluster) processing
		    **
		    ** Set the lock event
		    */
		    lk_event.type_high = SXA_EVENT;
		    lk_event.type_low = 0;
		    lk_event.value = SXA_EVENT_RESUME; 

		    cl_status = LKevent(LK_E_SET|LK_E_CROSS_PROCESS, 
				lk_ll, &lk_event, &cl_err);

		    if (cl_status != OK)
		    {
			*err_code =  E_SX1045_BAD_EVENT_SET;
			_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
			break;
		    }
# if 0 /* Not needed, see sxac_audit_thread() */
//		    /*
//		    ** On a cluster we have to resume the audit thread so
//		    ** it can look for the RESUME event, this allows just a
//		    ** single thread to poll rather than many different 
//		    ** sessions. (Only applies for multi-user servers)
//		    */
//		    if(CXcluster_enabled())
//		    {
//			CSresume(ascb->sxf_aud_thread);
//		    }
# endif
		    /*
		    ** Now Release the lock so the resumer can update it
		    */
		    cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
		    if (cl_status != OK)
		    {
			*err_code = E_SX1008_BAD_LOCK_RELEASE;
			_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
			break;
		    }
		    locked = FALSE;
		    /*
		    ** We are suspended, so wait for the resume event
		    */
		    sxf_set_activity("Waiting for Security Audit to RESUME");
		    status=LKevent(LK_E_WAIT|LK_E_CROSS_PROCESS, 
			lk_ll, &lk_event, &cl_err);
		    sxf_clr_activity();

		    if (cl_status != OK)
		    {
			*err_code = E_SX1046_BAD_EVENT_WAIT;
			_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
		        CSv_semaphore(&ascb->sxf_aud_sem);
			break;
		    }
		}
		/*
		** Take the lock again to continue processing
		*/
		cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
		    LK_S, &lk_val, &lk_id, 0, &cl_err);
		if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
		{
                    TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
                    _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		    *err_code = E_SX1035_BAD_LOCK_REQUEST;
		    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
				NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
		/*
		** Update the server lock value so we know things are
		** suspended
		*/
        	ascb->sxf_lockval.lk_low=lk_val.lk_low;
		ascb->sxf_lockval.lk_high=lk_val.lk_high;
		locked = TRUE;
	}

	/* If the user has the AUDIT_ALL bit, set result = TRUE and return
	** Exception is query text auditing which is orthogonal to all_events
	*/
	if ((scb->sxf_ustat & DU_UAUDIT) && event_type!=SXF_E_QRYTEXT)
	{
	    *result = TRUE;
	    break;
	}

	/*
	** Check for QUERY_TEXT auditing, need user status to include it
	*/
	if(event_type==SXF_E_QRYTEXT)
	{
	   if(scb->sxf_ustat & DU_UAUDIT_QRYTEXT)
		*result = ascb->sxf_event[event_type];
	   else
		*result=FALSE;
	}
	else
		*result = ascb->sxf_event[event_type];

	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = FALSE;
	break;
    }

    if(locked)
    {
	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	}
    }
    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX1040_SXAS_CHECK_STATE;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_CACHE_INIT - initialize (or reinitialize) the audit state cache
**
** Description:
**	This routine is called to load values from the iisecuritystate
**	system catalog into the audit state cache. It is assumed that
**	callers of this function are already connected to the iidbdb.
**	This routine is run by the audit thread when onother DBMS
**	session finds that the cache is invalid. 
**
**	Overview of algorithm:
**
**	Lock the audit state cache in X mode.
**	Locate the DMF database id from DMF.
**	Begin a transaction.
**	Open the iisecuritystate catalog.
**	Read each tuple from the catalog and update the cached audit state
**	Close the iisecuritystate catalog
*	Commit the transaction.
**	Update the cached audit state lock value.
**	Relese the audit state cache lock.
**	
** Inputs:
**	scb			session control block of the caller.
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	26-nov-92 (markg)
**	    Initial creation.
**	6-jul-93 (robf)
**	    Initialize audit label cache
*/
DB_STATUS
sxas_cache_init( 
    SXF_SCB	*scb,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    DU_SECSTATE		stuple;
    SXF_ASCB		*ascb = Sxf_svcb->sxf_aud_state;
    SXAS_DMF		sxas_dmf;
    i4			i = 0;
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
    bool		table_open = FALSE;
    ULM_RCB		ulm_rcb;
    bool		inxact = FALSE;
    bool		locked = FALSE;
    i4		sxf_status = Sxf_svcb->sxf_svcb_status;

    *err_code = E_SX0000_OK;

    for (;;)
    {
        scb->sxf_flags_mask|= SXF_SESS_CACHE_INIT;
	/*
	** Before updating the audit state cache from the database
	** the audit thread must hold the audit state lock in exclusive
	** mode. 
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_X, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = TRUE;

	/* Find the DMF database id from SCF */
	scf_cb.scf_length = sizeof (scf_cb);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_SXF_ID;
	scf_cb.scf_session = (SCF_SESSION) ascb->sxf_aud_thread;
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
	scf_cb.scf_len_union.scf_ilength = 1;
	sci_list[0].sci_length = sizeof (PTR);
	sci_list[0].sci_code = SCI_DBID;
	sci_list[0].sci_aresult = (char *) &sxas_dmf.dm_db_id;
	sci_list[0].sci_rlength = 0;
	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    *err_code = scf_cb.scf_error.err_code;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1005_SESSION_INFO;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}	

	sxas_dmf.dm_mode = SXF_READ;
	sxas_dmf.dm_status = 0;
	sxas_dmf.dm_tuple = (char *) &stuple;
	sxas_dmf.dm_tup_size = sizeof (stuple);

	status = sxas_begin_xact(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	inxact = TRUE;

	status = sxas_open_sectab(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    if (*err_code > E_SXF_INTERNAL)
		_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	table_open = TRUE;

	while ((status = sxas_read_rec(&sxas_dmf, &local_err)) == E_DB_OK)
	{
	    if (stuple.du_type == DU_SEVENT)
	    {
		if (stuple.du_id > SXF_E_MAX || stuple.du_id <= 0)
		{
		    local_err = E_SX1041_INVALID_EVENT_TYPE;
		    break;
		}
		else
		{
		    ascb->sxf_event[stuple.du_id] = stuple.du_state;
		}

		i++;
	    }
	}
	if (local_err == E_SX0007_NO_ROWS)
	{
	    if (i > SXF_E_MAX)
	    {
		*err_code = E_SX1042_BAD_ROW_COUNT;
		_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    else
		*err_code = E_SX0000_OK;
	}
	else
	{	
	    *err_code = local_err;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	status = sxas_close(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	table_open = FALSE;

	/*
	** Commit the transaction
	*/
	status = sxas_commit(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	inxact = FALSE;

	/*
	** The cache has now been successfully updated with the
	** latest values from the iidbdb. We have to update the
	** cached lock value, then release the audit state lock.
	** If auditing is disabled then we leave the value unchanged.
	*/
	if (!(Sxf_svcb->sxf_svcb_status & SXF_AUDIT_DISABLED))
	{
	    ascb->sxf_lockval.lk_high = lk_val.lk_high;
	    ascb->sxf_lockval.lk_low  = lk_val.lk_low;
	}

	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = FALSE;

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (table_open)
	    _VOID_ sxas_close(&sxas_dmf, &local_err);

	if (inxact)
	    _VOID_ sxas_abort(&sxas_dmf, &local_err);

	if (locked)
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	    
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1043_SXAS_CACHE_INIT;

	status = E_DB_ERROR;
    }
    /*
    ** Turn off cache init status
    */
    scb->sxf_flags_mask&= ~SXF_SESS_CACHE_INIT;

    return (status);
}

/*
** Name: SXAS_SHOW - return information anout the current audit state
**
** Description:
**	This routine is used to get information about the current auditing
**	state of the system. If the user specifies a valid event type the
**	value of event state will be set to show if the event is enabled
**	or disabled for auditing. If the value of filename is not NULL
**	an additional call to the low level audit routines will be made to 
**	get the audit file details.
**
**	Overview of algorithm:-
**	
**	If (event_type valid)
**	    Consult the audit state cache to determine if event type is enabled
**	Call the SXAP routine to get audit file information
**
** Inputs:
**	rcb.sxf_auditevent	Audit event type to check.
**	rsc.sxf_filename	Buffer to put audit file details, if NULL
**				audit file status will not be returned.
**
** Outputs:
**	rcb.sxf_auditstate 	Contains SXF_S_ENABLE if auditing is enabled
**				for this event type, otherwise SXF_S_DISABLE.
**			        If the event type is 0, then:
**				0 		== audit active
**				SXF_S_SUSPEND  	== audit suspended
**				SXF_S_STOP     	== audit stopped
**	rcb.sxf_filename	Name of the audit file currently being used
**				for writing audit records.
**	rcb.sxf_file_size	The maximum allowable size of an audit file.
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-92 (markg)
**	    Initial creation with partial functionality.
**	11-nov-92 (markg)
**	    Completed.
**	20-oct-93 (robf)
**          Return audit state.
*/ 
DB_STATUS
sxas_show(
    SXF_RCB	*rcb)
{
    DB_STATUS	status = E_DB_OK;
    i4	err_code = E_SX0000_OK;
    i4	local_err;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_ASCB	*ascb = Sxf_svcb->sxf_aud_state;

    rcb->sxf_error.err_code = E_SX0000_OK;

    for (;;)
    {
	if ((sxf_status & (SXF_C2_SECURE)) == 0)
	{
	    rcb->sxf_file_size = 0;
	    rcb->sxf_auditstate = 0;
	    if (rcb->sxf_filename)
		*rcb->sxf_filename = '\0';
	    break;
	}

	if (rcb->sxf_auditevent > 0 &&
	    rcb->sxf_auditevent < SXF_E_MAX)
	{
	    rcb->sxf_auditstate = (ascb->sxf_event[rcb->sxf_auditevent]) ? 
			SXF_S_ENABLE : SXF_S_DISABLE;
	}
	else if ((Sxf_svcb->sxf_svcb_status & SXF_AUDIT_DISABLED))
	{
	    /*
	    ** Disabled 
	    */
	    rcb->sxf_auditstate = SXF_S_STOP_AUDIT;
	}
	else if(ascb->sxf_lockval.lk_low==SXAS_SUSPEND_LKVAL)
	{
	    /*
	    ** Suspended
	    */
	    rcb->sxf_auditstate = SXF_S_SUSPEND_AUDIT;
	}

	status = (*Sxap_vector->sxap_show)(
			rcb->sxf_filename,
			&rcb->sxf_file_size,
			&local_err);
	if (status != E_DB_OK)
	{
            err_code = local_err;
            if (err_code > E_SXF_INTERNAL)
                _VOID_ ule_format(err_code, NULL,
                        ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
            break;
        }

	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
        if (err_code > E_SXF_INTERNAL)
            err_code = E_SX1013_SXA_SHOW_STATE;
 
        rcb->sxf_error.err_code = err_code;
        status = E_DB_ERROR;
    }
 
    return (status);
}

/*
** Name: SXAS_ALTER - alter the audit state information
**
** Description:
**	Check the new audit state, then call the appropriate routine
**	to make the change.
**
** Inputs:
**	rcb.sxf_auditstate	How the audit state  is changing.
**
** Outputs:
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	29-jul-93 (robf)
**	    Initial creation, switcher for other audit state altering
**	    routines. This stops  the old sxas_alter getting too big/complex
**	    as we add more functionality
*/
DB_STATUS
sxas_alter(
    SXF_RCB	*rcb)
{
    i4		state = rcb->sxf_auditstate;
    i4	local_err;
    DB_STATUS   status;
    i4     sxf_status = Sxf_svcb->sxf_svcb_status;

    if(state&(SXF_S_ENABLE|SXF_S_DISABLE))
    {
	status=sxas_enable_disable_audit(rcb);
    }
    else if(state&SXF_S_SUSPEND_AUDIT)
    {
	status=sxas_suspend_audit(TRUE, &rcb->sxf_error.err_code);
    }
    else if(state&SXF_S_RESUME_AUDIT)
    {
	status=sxas_resume_audit(TRUE, &rcb->sxf_error.err_code,
				(state&SXF_S_CHANGE_FILE)?TRUE:FALSE,
				 rcb->sxf_filename);
    }
    else if(state&SXF_S_STOP_AUDIT)
    {
	status=sxas_stop_audit( TRUE, &rcb->sxf_error.err_code);
    }
    else if(state&SXF_S_RESTART_AUDIT)
    {
	status=sxas_restart_audit(&rcb->sxf_error.err_code, 	
				(state&SXF_S_CHANGE_FILE)?TRUE:FALSE,
				 rcb->sxf_filename);
    }
    else if(state&SXF_S_CHANGE_FILE)
    {
	status=sxas_change_file(rcb);
    }
    else
    {
        rcb->sxf_error.err_code = E_SX0004_BAD_PARAMETER;
    }


    /*
    ** Log any internal errors and pass back external error
    */
    if (rcb->sxf_error.err_code > E_SXF_INTERNAL)
    {
        _VOID_ ule_format(rcb->sxf_error.err_code, NULL, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
    	rcb->sxf_error.err_code = E_SX0017_SXAS_ALTER;
    }
    return status;
}

/*
** Name: SXAS_ENABLE_DISABLE_AUDIT - alter the audit state information
**
** Description:
** 	This routine is used to change the current audit state information.
**	It will update both the underlying system catalog iisecuritystate
**	and also the cached copy of this data. The value of the audit state
**	cache lock will be incremented at this time. This will ensure that
**	all other servers in the instalation will be forced to reread the
**	audit state data from the iisecuritystate catalog.
**
**	Overview of functions in this routine:-
**
**	Validate parameters.
**	Locate the sessions SCB.
**	Lock the audit state cache in X mode.
**	Get the current database id from SCF.
**	Begin a transaction.
**	Open the iisecuritystate catalog.
**	Scan all tuples in the catalog and update as necessary.
**	Close the iisecuritystate catalog.
**	Increment the lock value.
**	Convert the audit state lock to IX mode
**	Commit the transacrion.
**	Release the audit state lock.
**
** Inputs:
**	rcb.sxf_auditevent	Audit event type to set.
**	rcb.sxf_auditstate	Contains SXF_S_ENABLED if auditing to be
**				enabled for this event type, otherwise 
**				SXF_S_DISABLE.
**
** Outputs:
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-92 (markg)
**	    Initial creation
**	12-jul-93  (robf)
**	    Add handling for security level auditing.
**	29-jul-93 (robf)
**	    Created from sxas_alter since that routine  does more now
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	1-oct-93 (robf)
**          Allow LEVEL with no label in single user case, this is a silent
**	    no-op.
**	26-jan-95 (stephenb)
**	    When executing DISABLE SECURITY_AUDIT ALL we should only open
**	    iilabelmap if we are running in B1 mode.
*/
static DB_STATUS
sxas_enable_disable_audit(
    SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    i4		err_code = E_SX0000_OK;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    SXAS_DMF		sxas_dmf;
    DU_SECSTATE		stuple;
    SXF_SCB		*scb;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LLID		lk_ll;
    CS_SID		sid;
    SCF_SCI		sci_list[1];
    SCF_CB		scf_cb;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
    SXF_EVENT		event = rcb->sxf_auditevent;
    i4			state = rcb->sxf_auditstate;
    bool		table_open = FALSE;
    bool		inxact = FALSE;
    bool		locked = FALSE;
    i4		sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_ASCB		*ascb = (SXF_ASCB *) Sxf_svcb->sxf_aud_state;

    rcb->sxf_error.err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	** Validate that the correct parameters have been passed in
	** in the RCB.
	*/
	if (event <= 0 || event > SXF_E_MAX ||
	   (state != SXF_S_ENABLE && state != SXF_S_DISABLE))
	{
	    err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}
	
	/*
	** If the facility is running in single user mode, all we
	** need to do is store the event value in the ascb, then return.
	*/
	if (Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER)
	{
	    if (sxf_status & (SXF_C2_SECURE) && ascb!=NULL)
	        ascb->sxf_event[event] = state;
	    break;
	}
	    
	/* Find the scb for the caller */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	lk_ll = scb->sxf_lock_list;

	/*
	** Get exclusive access to the audit state access lock.
	** Then process the change.
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_X, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = TRUE;

	/*
	** If auditing is disabled/stopped we don't let audit 
	** state change, since we can't signal the change while
	** auditing is disabled/suspended.
	*/
	if ((Sxf_svcb->sxf_svcb_status & SXF_AUDIT_DISABLED)  ||
	     lk_val.lk_low < SXAS_MIN_LKVAL)
	{
		err_code=E_SX002B_AUDIT_NOT_ACTIVE;
		status=E_DB_ERROR;
		break;
	}
	/* get the open database id from SCF */
	CSget_sid(&sid);
	scf_cb.scf_length = sizeof (scf_cb);
	scf_cb.scf_type = SCF_CB_TYPE;
	scf_cb.scf_facility = DB_SXF_ID;
	scf_cb.scf_session = (SCF_SESSION) sid;
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
	scf_cb.scf_len_union.scf_ilength = 1;
	sci_list[0].sci_length = sizeof (PTR);
	sci_list[0].sci_code = SCI_DBID;
	sci_list[0].sci_aresult = (char *) &sxas_dmf.dm_db_id;
	sci_list[0].sci_rlength = 0;
	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    err_code = scf_cb.scf_error.err_code;
	    _VOID_ ule_format(err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    err_code = E_SX1005_SESSION_INFO;
	    _VOID_ ule_format(err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	}	

	sxas_dmf.dm_mode = SXF_WRITE;
	sxas_dmf.dm_status = 0;
 
	status = sxas_begin_xact(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	inxact = TRUE;

	status = sxas_open_sectab(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	table_open = TRUE;

	sxas_dmf.dm_tuple = (char *) &stuple;
	sxas_dmf.dm_tup_size = sizeof (stuple);

		/* 
		** Get every tuple from the catalog and update
		** all those that match the request.
		*/
	while ((status = sxas_read_rec(&sxas_dmf, &local_err)) == E_DB_OK)
	{
	    if (event == SXF_E_ALL || stuple.du_id == event)
	    {
		stuple.du_state = 
		    (state == SXF_S_ENABLE) ? DU_STRUE : DU_SFALSE;
		status = sxas_replace_rec(&sxas_dmf, &local_err);
		if (status != E_DB_OK)
		{
		    break;
		}
	    }
	}
	if (status != E_DB_OK)
	{
	    if (local_err == E_SX0007_NO_ROWS)
	    {
		err_code = E_SX0000_OK;
		status = E_DB_OK;
	    }
	    else
	    {	
		err_code = local_err;
		_VOID_ ule_format(err_code, NULL, ULE_LOG, 
		    NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	}


	status = sxas_close(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	table_open = FALSE;
	/*
	** At this point we know that we want to go ahead and commit
	** the changed tuples to the catalogs. We must increment the 
	** value of the lock, however we don't want to let other
	** sessions get in until we have committed the xact. Because of
	** this we convert the lock down to IX now (this updates the
	** lock value), and release the lock once the xact has been 
	** committed.
	*/

	/*
	** We increment the lock state value if auditing is currently active
	** so other sessions pick up the change. 
	*/
	if (lk_val.lk_low++ >= MAXI4)
	{
	    lk_val.lk_low = SXAS_MIN_LKVAL;
	    if (lk_val.lk_high++ >= MAXI4)
		lk_val.lk_high = 0;
	}

	cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lk_ll, 
			&lk_key, LK_IX, &lk_val, &lk_id, 0, &cl_err); 
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    err_code = E_SX1019_BAD_LOCK_CONVERT;
	    _VOID_ ule_format(err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	status = sxas_commit(&sxas_dmf, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	inxact = FALSE;

	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
	    err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked = FALSE;
	
	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	if (table_open)
	    _VOID_ sxas_close(&sxas_dmf, &local_err);

	if (inxact)
	    _VOID_ sxas_abort(&sxas_dmf, &local_err);

	if (locked)
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	    

	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX0017_SXAS_ALTER;

	rcb->sxf_error.err_code = err_code;

	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_STARTUP - start up the audit state subsystem
**
** Description:
**	This routine is called at facility startup time to initialize
**	the audit state subsystem (SXAS). It allocates and builds the
**	audit state control block (SXF_ASCB). It also requests the audit
**	state access lock in NULL mode. This ensures that the lock values
**	held with the audit state lock will remain for the life of the server.
**
** Inputs:
**	None.
**
** Outputs:
**	err_code		error code returned to the caller
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	 2-june-1993 (robf)
**	    Name the audit semaphore
**	 7-july-93 (robf)
**	    Allocate the label audit cache if system has security labels
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to sxf_owner which has changed type
**	    to PTR.
**	1-mar-94 (robf)
**          When initializing the state on B1 servers enable SECURITY, USER
**	    and DATABASE events by default. This ensures that even if the
**	    audit state is unavilable from the iidbdb important events will
**	    be audited.
**	18-jul-94 (robf)
**          Enhanced 1-mar-94 change to be for both B1 and C2 servers.
*/
DB_STATUS
sxas_startup(
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    STATUS	cl_status;
    CL_ERR_DESC	cl_err;
    i4	local_err;
    ULM_RCB	ulm_rcb;
    SXF_ASCB	*ascb;
    LK_LKID	lk_id;
    LK_VALUE	lk_val;
    LK_LLID	lk_ll = Sxf_svcb->sxf_lock_list;
    LK_LOCK_KEY	lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 0, 0, 0, 0};
    LK_LOCK_KEY	lk_key_save = {LK_AUDIT, SXAS_LOCK, SXAS_SAVE_LOCK, 0, 0, 0, 0};
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    i4		i;
    char	*pmvalue;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	** Allocate the audit state control block from the 
	** facilities memory stream, and initialize it
	** with empty values.
	*/
        ulm_rcb.ulm_facility = DB_SXF_ID;
        ulm_rcb.ulm_streamid_p = &Sxf_svcb->sxf_svcb_mem;
        ulm_rcb.ulm_psize = sizeof (SXF_ASCB);
        ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
        status = ulm_palloc(&ulm_rcb);
        if (status != E_DB_OK)
        {
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
            break;
        }
        Sxf_svcb->sxf_aud_state = (SXF_ASCB *) ulm_rcb.ulm_pptr;
	ascb = Sxf_svcb->sxf_aud_state;

	ascb->sxf_next = NULL;
	ascb->sxf_prev = NULL;
	ascb->sxf_length = sizeof (SXF_ASCB);
	ascb->sxf_cb_type = SXFAS_CB;
	ascb->sxf_owner = (PTR)DB_SXF_ID;
	ascb->sxf_ascii_id = SXFASCB_ASCII_ID;
	ascb->sxf_lockval.lk_high = MAXI4;
	ascb->sxf_lockval.lk_low = MAXI4;
	ascb->sxf_aud_thread = 0;

        for (i = 0; i <= (SXF_E_MAX + 1); i++)
	    ascb->sxf_event[i] = 0;

	/* If B1 or C2 default to SECURITY, USER & DATABASE auditing on */
	if (sxf_status & (SXF_C2_SECURE))
	{
	    ascb->sxf_event[SXF_E_SECURITY] = 1;
	    ascb->sxf_event[SXF_E_USER] = 1;
	    ascb->sxf_event[SXF_E_DATABASE] = 1;
	}

	if (CSw_semaphore(&ascb->sxf_aud_sem, CS_SEM_SINGLE,
				"SXF aud sem" ) != OK)
	{
	    *err_code = E_SX1003_SEM_INIT;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/*
	** Hold the audit state cache lock in null mode
	** for the life of the server. If this is not done
	** the lock value will get set back to zero each
	** time the lock is granted. 
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
	    LK_X, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Check if lock value is empty, if so we initialize to normal
	** operation
	*/
	if(lk_val.lk_low==0 && lk_val.lk_high==0)
	{
		lk_val.lk_low=SXAS_MIN_LKVAL;
		lk_val.lk_high=0;
	}
	/*
	** Convert the lock back to NULL setting the lock value
	*/
	cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lk_ll, 
			&lk_key, LK_N, &lk_val, &lk_id, 0, &cl_err); 
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1019_BAD_LOCK_CONVERT;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Hold the audit state save lock in null mode
	** for the life of the server. If this is not done
	** the lock value will get set back to zero each
	** time the lock is granted. 
	*/
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key_save,
	    LK_X, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Check if lock value is empty, if so we initialize to normal
	** operation
	*/
	if(lk_val.lk_low==0 && lk_val.lk_high==0)
	{
		lk_val.lk_low=SXAS_MIN_LKVAL;
		lk_val.lk_high=0;
	}
	/*
	** Convert the lock back to NULL setting the lock value
	*/
	cl_status = LKrequest(LK_PHYSICAL | LK_CONVERT, lk_ll, 
			&lk_key_save, LK_N, &lk_val, &lk_id, 0, &cl_err); 
	if ((cl_status != OK) && (cl_status != LK_VAL_NOTVALID))
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1019_BAD_LOCK_CONVERT;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX1044_SXAS_STARTUP;
	status = E_DB_ERROR;
    }

    return (status);
}
/*
** Name: SXAS_BEGIN_XACT - begin a transaction with DMF
**
** Description:
**	This static routine is used to begin a DMF transaction on behalf of the
**	current SXF session.
**
** Inputs:
**	sxas_dmf		the DMF context block
**
** Outputs:
**	err_code		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
*/
static DB_STATUS
sxas_begin_xact(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    DMX_CB		dmx;
    CS_SID		sid;

    *err_code = E_SX0000_OK;

    for(;;)
    {
	CSget_sid(&sid);

	dmx.type = DMX_TRANSACTION_CB;
	dmx.length = sizeof (dmx);
	dmx.dmx_flags_mask = 0;
	if (sxas_dmf->dm_mode == SXF_READ)
	    dmx.dmx_flags_mask |= DMX_READ_ONLY;
	dmx.dmx_session_id = (PTR) sid;
     	dmx.dmx_option = 0;
     	dmx.dmx_db_id = sxas_dmf->dm_db_id;
     	status = (*Sxf_svcb->sxf_dmf_cptr)(DMX_BEGIN, &dmx);
	if (status != E_DB_OK)
	{
	    *err_code = dmx.error.err_code;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	sxas_dmf->dm_tran_id = dmx.dmx_tran_id;

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX1038_BEGIN_XACT_ERROR;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_OPEN_SECTAB - open the security state catalog
**
** Description:
**	This static routine opens the iisecuritystate catalog in the iidbdb.
**	The caller must already be connected to the iidbdb, and must already
**	be within a transaction.
**
** Inputs:
**	sxas_dmf		the DMF context block
**
** Outputs:
**	err_code		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	09-oct_93 (swm)
**	    Bug #56437
**	    Put sid into new dmc_session_id rather than dmc_id.
**	30-may-2001 (devjo01)
**	    Correct bad test for E_DM0054_NONEXISTENT_TABLE. ( = s/b == )
*/
static DB_STATUS
sxas_open_sectab(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    DMT_CB		dmt;
    DMC_CB		dmc;
    DMC_CHAR_ENTRY	dmc_char;
    CS_SID		sid;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	** If the table is being opened for read, alter the session
	** to use readlock = nolock. This will prevent possible locking
	** problems when the audit thread is attempting to refresh the 
	** audit state cache.
	*/
	if (sxas_dmf->dm_mode == SXF_READ)
	{
	    CSget_sid(&sid);

	    dmc.type = DMC_CONTROL_CB;
	    dmc.length = sizeof (dmc);
	    dmc.dmc_op_type = DMC_SESSION_OP;
	    dmc.dmc_flags_mask = DMC_SETLOCKMODE;
	    dmc.dmc_session_id = (PTR) sid;
	    dmc.dmc_db_id = sxas_dmf->dm_db_id;
	    dmc.dmc_sl_scope = DMC_SL_SESSION;
	    dmc.dmc_db_access_mode = 0;
	    dmc.dmc_char_array.data_address = (char *) &dmc_char;
	    dmc.dmc_char_array.data_in_size = sizeof (dmc_char);
	    dmc_char.char_id = DMC_C_LREADLOCK;
	    dmc_char.char_value = DMC_C_READ_UNCOMMITTED;
	    status = (*Sxf_svcb->sxf_dmf_cptr)(DMC_ALTER, &dmc);
	    if (status != E_DB_OK)
	    {
		*err_code = dmc.error.err_code;
		_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	}

	dmt.type = DMT_TABLE_CB;
	dmt.length = sizeof (dmt);
	dmt.dmt_db_id = sxas_dmf->dm_db_id;
	dmt.dmt_tran_id = sxas_dmf->dm_tran_id;
	dmt.dmt_sequence = 1; 
	dmt.dmt_flags_mask = 0;
	dmt.dmt_lock_mode = 
	    (sxas_dmf->dm_mode == SXF_READ) ? DMT_IS : DMT_IX;
	dmt.dmt_update_mode = DMT_U_DIRECT;
	dmt.dmt_access_mode = 
	    (sxas_dmf->dm_mode == SXF_READ) ? DMT_A_READ : DMT_A_WRITE;
	dmt.dmt_id.db_tab_base = DM_B_SECSTATE_TAB_ID;
	dmt.dmt_id.db_tab_index = DM_I_SECSTATE_TAB_ID;
	dmt.dmt_char_array.data_address = NULL;
	dmt.dmt_char_array.data_in_size = 0;

	status = (*Sxf_svcb->sxf_dmf_cptr)(DMT_OPEN, &dmt);
	if (status != E_DB_OK)
	{
	    if (dmt.error.err_code == E_DM0054_NONEXISTENT_TABLE)
	    {
		*err_code = E_SX0018_TABLE_NOT_FOUND;
	    }
	    else
	    {
		*err_code = dmt.error.err_code;
		_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    }
	    break;
	}

	sxas_dmf->dm_access_id = dmt.dmt_record_access_id;

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1039_BAD_SECCAT_OPEN;

	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_READ_REC - read the next record from the database table
**
** Description:
**	This static routine reads the next tuple from the open database table
**	given in the DMF contect block (SXAS_DMF). The table will be 
**	positioned for reading if this has not already been done.
**
** Inputs:
**	sxas_dmf		the DMF context block
**
** Outputs:
**	err_code		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
*/
static DB_STATUS
sxas_read_rec(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    DMR_CB		dmr;

    *err_code = E_SX0000_OK;

    for(;;)
    {
	dmr.type = DMR_RECORD_CB;
	dmr.length = sizeof (dmr);
	dmr.dmr_access_id = sxas_dmf->dm_access_id;
	dmr.dmr_flags_mask = DMR_NEXT;
	dmr.dmr_data.data_address = sxas_dmf->dm_tuple;
	dmr.dmr_data.data_in_size = sxas_dmf->dm_tup_size;

	if ((sxas_dmf->dm_status & SX_POSITIONED) == 0)
	{
	    dmr.dmr_position_type = DMR_ALL;
	    dmr.dmr_q_fcn = 0;
	    status = (*Sxf_svcb->sxf_dmf_cptr)(DMR_POSITION, &dmr);
	    if (status != E_DB_OK)
	    {
		*err_code = dmr.error.err_code;
		_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	    sxas_dmf->dm_status |= SX_POSITIONED;
	}

	status = (*Sxf_svcb->sxf_dmf_cptr)(DMR_GET, &dmr);
	if (status != E_DB_OK)
	{
	    if (dmr.error.err_code == E_DM0055_NONEXT)
	    {
	     	*err_code = E_SX0007_NO_ROWS;
		return (E_DB_WARN);
	    }
	    else
	    {
		*err_code = dmr.error.err_code;
		_VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    }
	    break;
	}

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX103A_BAD_SECCAT_READ;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_replace_rec - replace a database record
**
** Description:
**	This static routine replaces the tuple at the current position
** 	in the table with the one supplied by the caller.
**
** Inputs:
**	sxas_dmf		the DMF context block
**
** Outputs:
**	err_code		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
**	01-oct-1998 (nanpr01)
**	    Initialize the new element for update performance enhancement.
*/
static DB_STATUS
sxas_replace_rec(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    DMR_CB		dmr;

    *err_code = E_SX0000_OK;

    for(;;)
    {
	dmr.type = DMR_RECORD_CB;
	dmr.length = sizeof (dmr);
	dmr.dmr_access_id = sxas_dmf->dm_access_id;
	dmr.dmr_flags_mask = DMR_CURRENT_POS;
	dmr.dmr_data.data_address = sxas_dmf->dm_tuple;
	dmr.dmr_data.data_in_size = sxas_dmf->dm_tup_size;
	dmr.dmr_attset = (char *) 0;

	status = (*Sxf_svcb->sxf_dmf_cptr)(DMR_REPLACE, &dmr);
	if (status != E_DB_OK)
	{
	    *err_code = dmr.error.err_code;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX103B_BAD_SECCAT_REPLACE;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_COMMIT - commit a transaction
**
** Description:
**	 This static routine commits the sessions current DMF transaction.
**
** Inputs:
**	sxas_dmf		the DMF context block
**
** Outputs:
**	err_code		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
*/
static DB_STATUS
sxas_commit(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    i4	local_err;
    DMX_CB	dmx;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	dmx.type = DMX_TRANSACTION_CB;
	dmx.length = sizeof (dmx);
	dmx.dmx_tran_id = sxas_dmf->dm_tran_id;
	dmx.dmx_option = 0x00;

	status = (*Sxf_svcb->sxf_dmf_cptr)(DMX_COMMIT, &dmx);
	if (status != E_DB_OK)
	{
	    *err_code = dmx.error.err_code;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	break;
    }

    /* handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX103C_XACT_COMMIT_ERROR;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_ABORT - abort a transaction
**
** Description:
**	 This static routine aborts the sessions current DMF transaction.
**
** Inputs:
**	sxas_dmf		the DMF context block
**
** Outputs:
**	err_code		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
*/
static DB_STATUS
sxas_abort(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    i4	local_err;
    DMX_CB	dmx;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	dmx.type = DMX_TRANSACTION_CB;
	dmx.length = sizeof (dmx);
	dmx.dmx_tran_id = sxas_dmf->dm_tran_id;
	dmx.dmx_option = 0;
	MEfill(sizeof(dmx.dmx_savepoint_name.db_sp_name), 0, 
		(PTR)dmx.dmx_savepoint_name.db_sp_name);

	status = (*Sxf_svcb->sxf_dmf_cptr)(DMX_ABORT, &dmx);
	if (status != E_DB_OK)
	{
	    *err_code = dmx.error.err_code;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	break;
    }

    /* handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX103D_XACT_ABORT_ERROR;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: SXAS_CLOSE_SECTAB - close the security state catalog
**
** Description:
**	This static routine closes the iisecurity state catalog in the iidbdb.
**	The catalog must have previously been opened by a call to 
**	sxas_open_sectab.
**
** Inputs:
**	sxas_dmf		the DMF context block
**
** Outputs:
**	err_code		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-nov-1992 (markg)
**	    Initial creation.
*/
static DB_STATUS
sxas_close(
    SXAS_DMF	*sxas_dmf,
    i4	*err_code)
{
    DB_STATUS	status = E_DB_OK;
    i4	local_err;
    DMT_CB	dmt;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	dmt.type = DMT_TABLE_CB;
	dmt.length = sizeof (dmt);
	dmt.dmt_flags_mask = 0;
	dmt.dmt_record_access_id = sxas_dmf->dm_access_id;

	status = (*Sxf_svcb->sxf_dmf_cptr)(DMT_CLOSE, &dmt);
	if (status != E_DB_OK)
	{
	    *err_code = dmt.error.err_code;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	break;
    }

    /* handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX103E_BAD_SECCAT_CLOSE;
	status = E_DB_ERROR;
    }

    return (status);
}

/*
** Name: sxas_delete_rrec - delete the current row from a table in DMF
**
** Description:
**	Deletes the current row from a table in DMF
**
** Inputs:
**	sxldmf		the DMF context block
**
**
** Outputs:
**	err_code	Error code on failure.
**	
** Returns:
**	DB_STATUS
**
** History:
**	12-jul-93 (robf)
**	    Initial creation.
*/
static DB_STATUS sxas_delete_rec (
	SXAS_DMF	*sxldmf,
	i4 *err_code
)
{
	DB_STATUS 	status=E_DB_OK;
	i4		local_err;
	DMR_CB		dmr;

	*err_code=E_SX0000_OK;
	for(;;)
	{
		/* Build DMF CB */
		dmr.type = DMR_RECORD_CB;
		dmr.length=sizeof(dmr);
		dmr.dmr_access_id = sxldmf->dm_access_id;
		dmr.dmr_flags_mask = DMR_CURRENT_POS;

		/* Call DMF */
		status = (*Sxf_svcb->sxf_dmf_cptr) (DMR_DELETE, &dmr);
		if( status!=E_DB_OK)
		{
			*err_code = dmr.error.err_code;
			_VOID_ ule_format (*err_code, NULL, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err,0);
			break;
		}
		break;
	}
	/* Handle any errors */
	if (*err_code != E_SX0000_OK)
	{
		if (*err_code > E_SXF_INTERNAL)
		/* FIXME */
			*err_code=E_SX108F_BAD_SECID_DELETE;
	
		status= E_DB_ERROR;
	}
	return status;
}
/*
** Name: sxas_insert_rrec - Inserts a new row into a DMF table
**
** Description:
**	Inserts a new row into a DMF table
**
** Inputs:
**	sxldmf		the DMF context block
**
**
** Outputs:
**	err_code	Error code on failure.
**	
** Returns:
**	DB_STATUS
**
** History:
**	12-jul-93 (robf)
**	    Initial creation.
*/
static DB_STATUS sxas_insert_rec (
	SXAS_DMF	*sxldmf,
	i4 *err_code
)
{
	DB_STATUS 	status=E_DB_OK;
	i4		local_err;
	DMR_CB		dmr;

	*err_code=E_SX0000_OK;
	for(;;)
	{
		/* Build DMF CB */
		dmr.type = DMR_RECORD_CB;
		dmr.length=sizeof(dmr);
		dmr.dmr_data.data_in_size = sxldmf->dm_tup_size;
		dmr.dmr_data.data_address = sxldmf->dm_tuple;
		dmr.dmr_access_id = sxldmf->dm_access_id;

		/* Call DMF */
		status = (*Sxf_svcb->sxf_dmf_cptr) (DMR_PUT, &dmr);
		if( status!=E_DB_OK)
		{
			*err_code = dmr.error.err_code;
			_VOID_ ule_format (*err_code, NULL, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err,0);
			*err_code=E_SX108E_BAD_SECID_INSERT;
			break;
		}
		break;
	}
	/* Handle any errors */
	if (*err_code != E_SX0000_OK)
	{
		if (*err_code > E_SXF_INTERNAL)
			*err_code=E_SX108E_BAD_SECID_INSERT;
	
		status= E_DB_ERROR;
	}
	return status;
}

/*
** Name: sxas_suspend_audit
**
** Description:
**	Suspend security auditing
**
**	This routine implements the external request SXF_S_SUSPEND_AUDIT,
**	suspending auditing. 
**
**	Auditing is suspended by setting the audit state lock to 
**	SXAS_SUSPEND_LOCKVAL. This is checked by other sessions which want
**	to write an audit  record, and will cause them to "suspend" (wait
**	for a RESUME lock event)
**
** Inputs:
**	user_request	TRUE if user explicitly requested SUSPEND, 
**		        FALSE if system decided to suspend automatically.
**
** Change history:
**	29-jul-93 (robf)
**	   Created
*/
DB_STATUS
sxas_suspend_audit(
	bool user_request,
	i4	*err_code
)
{
    SXF_SCB		*scb;
    DB_STATUS		status;
    i4		local_code;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LLID		lk_ll;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
					0, 0, 0, 0};
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    bool		locked=FALSE;

    for(;;)
    {
	*err_code=E_SX0000_OK;
	/* Find the scb for the caller */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    break;
	}
	/*
	** Use the session's lock list
	*/
   	lk_ll = scb->sxf_lock_list;

	/*
	** First get the audit state lock in exclusive mode so things
	** don't move underneath us
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_X, &lk_val, &lk_id, 
			0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    break;
	}
	locked = TRUE;
	/*
	** Check if  already suspended
	*/
	if(lk_val.lk_low==SXAS_SUSPEND_LKVAL)
	{
		/*
		** Already suspended, error if user request
		*/
		if(user_request)
		{
			*err_code=E_SX0021_ALREADY_SUSPENDED;
			status=E_DB_ERROR;
		}
		break;
	}
	/*
	** Check auditing is enabled, we can't suspend log when
	** no auditing
	*/
	if ((Sxf_svcb->sxf_svcb_status & SXF_AUDIT_DISABLED)  ||
	     lk_val.lk_low==SXAS_STOP_LKVAL)
	{
		if(user_request)
		{
			*err_code=E_SX002B_AUDIT_NOT_ACTIVE;
			status=E_DB_ERROR;
		}
		break;
	}
	/* Save the current lock value for resume */
	status=sxas_save_state_value(&lk_ll, &lk_val, err_code);

	/* Update the lock value to indicate suspended */
	lk_val.lk_low=SXAS_SUSPEND_LKVAL;
	lk_val.lk_high=0;

	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, 
			&lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    break;
	}
	locked=FALSE;
	break;
    }
    /*
    ** Clean up
    */
    if(locked)
    {
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, 
			&cl_err);
    }
    if(*err_code!=E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX0023_SUSPEND_ERROR;
	}

	if(status<E_DB_ERROR)
		status= E_DB_ERROR;
    }
    else
    {
	/*
	** Log message that auditing is suspended
	*/
	if(user_request)
	{
		_VOID_ ule_format(I_SX10AA_AUDIT_SUSPEND, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 2,
			(i4)sizeof(scb->sxf_ruser), &scb->sxf_ruser);
	}
	else
	{
		_VOID_ ule_format(I_SX10AA_AUDIT_SUSPEND, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 2,
			 12, "audit system");
	}
    }
    return status;

}

/*
** Name: sxas_resume_audit
**
** Description:
**	Resume security auditing
**
**	This routine implements the external request SXF_S_RESUME_AUDIT,
**	suspending auditing
**
**	Security auditing is resumed by setting the state lock back to 
**	the normal minimum value, and  raising the RESUME lock event, which
**	causes any waiting threads to wake up and  start processing.
**
** Inputs:
**	user_request	- TRUE if on user request
**	new_log		- TRUE if new audit log requested
**	filename	- Name of new log
**
** Outputs:
**	err_code	- error code
**
** Change history:
**	29-jul-93 (robf)
**	   Created
**	9-feb-94 (robf)
**         Removed extra ule_format() calls for more consistent
**	   error handling. 
*/
static DB_STATUS
sxas_resume_audit(
    bool user_request,
    i4	*err_code,
    bool	new_log,
    char	*filename
)
{
    SXF_SCB		*scb;
    DB_STATUS		status;
    i4		local_code;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LLID		lk_ll;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    bool		locked=FALSE;
    LK_EVENT		lk_event;

    for(;;)
    {
	*err_code=E_SX0000_OK;
	/* Find the scb for the caller */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    break;
	}
	/*
	** Use the session's lock list
	*/
   	lk_ll = scb->sxf_lock_list;

	/*
	** First get the audit state lock in exclusive mode so things
	** don't move underneath us
	*/

	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_X, &lk_val, &lk_id, 
			0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    break;
	}
	locked = TRUE;
	/*
	** Check if already suspended
	*/
	if(lk_val.lk_low!=SXAS_SUSPEND_LKVAL)
	{
		/*
		** Already suspended, release lock and return
		*/
		*err_code=E_SX0022_NOT_SUSPENDED;
		status=E_DB_ERROR;
		break;
	}

	/*
	** Resume any threads waiting on the suspend event
	*/
	lk_event.type_high = SXA_EVENT;
	lk_event.type_low = 0;
	lk_event.value = SXA_EVENT_RESUME;

	cl_status = LKevent(LK_E_CLR|LK_E_CROSS_PROCESS, lk_ll, &lk_event, &cl_err);
	
	if (cl_status != OK)
	{
	    *err_code =  E_SX1047_BAD_EVENT_CLEAR;
	    break;
	}
        /*
	** If switching to a new audit log we do so now, rather  than
	** waiting for the next sxas_check call.
	*/
	if (new_log)
	{
		/*
		** Call physical layer to change the file
		*/
	        status = (*Sxap_vector->sxap_alter)(
			scb,
			filename,
			SXAP_C_CHANGE_FILE,
			err_code);
		if(status!=E_DB_OK)
		{
	        	if(SXS_SESS_TRACE(SXS_TAUD_STATE))
				SXF_DISPLAY("Physical layer failed to change files\n");
			break;
		}
	}
	/* Get the saved state lock value */
	status=sxas_get_state_value(&lk_ll, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;

	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, 
			&lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    break;
	}
	locked=FALSE;
	break;
    }
    /*
    ** Clean up
    */
    if(locked)
    {
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, 
			&cl_err);
    }
    if(*err_code!=E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX0024_RESUME_ERROR;
	}

	if(status<E_DB_ERROR)
		status= E_DB_ERROR;
    }
    else
    {
	/*
	** Log message that auditing is resumed
	*/
        _VOID_ ule_format(I_SX10AC_AUDIT_RESUME, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 2,
			(i4)sizeof(scb->sxf_ruser), &scb->sxf_ruser);
    }
    return status;
}

/*
** Name: sxas_change_file
**
** Description
**	This function implements the SXF_S_CHANGE_FILE request to
** 	switch to a new audit file. It checks the request, then calls the
**	physical layer to perform the requisite change
**
** Inputs:
**	rcn.sxf_filename	New file name
**
** Outputs:
**	rcb.sxf_error		Error status
**
** Returns:
**	DB_STATUS
**
** Change history:
**	2-aug-93 (robf)
**	   Created
*/
static DB_STATUS 
sxas_change_file (SXF_RCB *rcb)
{
   DB_STATUS 		status=E_DB_OK;
   i4   		err_code=E_SX0000_OK;
   i4   		local_err=E_SX0000_OK;
   SXF_SCB	  	*scb;
   STATUS		cl_status;
   CL_ERR_DESC		cl_err;
   LK_LKID		lk_id;
   LK_VALUE		lk_val;
   LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
   LK_LLID		lk_ll;
   bool			locked=FALSE;

   for(;;)
   {
	/* Find the scb for the caller */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
    	    err_code=local_err;
	    break;
	}
   	lk_ll = scb->sxf_lock_list;
	/*
	** Check auditing is enabled, we can't switch to a new log when
	** no auditing
	*/
	if (Sxf_svcb->sxf_svcb_status & SXF_AUDIT_DISABLED)
	{
		err_code=E_SX0028_SET_LOG_NEEDS_AUDIT;
		status=E_DB_ERROR;
		break;
	}
	/*
	** Get the state lock to check whether auditing is active, and also
	** to make sure things don't change under us.
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
		LK_S, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
                TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
                _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
		err_code = E_SX1035_BAD_LOCK_REQUEST;
		break;
	}
	locked = TRUE;
	/*
	** Check if auditing is active
	*/
	if(lk_val.lk_low<SXAS_MIN_LKVAL)
	{
		err_code=E_SX0028_SET_LOG_NEEDS_AUDIT;
		status=E_DB_ERROR;
		break;
	}
	/*
	** Now call physical layer to change the file
	*/
	status = (*Sxap_vector->sxap_alter)(
			scb,
			rcb->sxf_filename,
			SXAP_C_CHANGE_FILE,
			&err_code);
	if (status != E_DB_OK)
	{
	    _VOID_ ule_format(err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    err_code=E_SX0025_SET_LOG_ERR;
	    break;
        }
	/*
	** Release the state lock
	*/
	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
		err_code = E_SX1008_BAD_LOCK_RELEASE;
		break;
	}
	locked = FALSE;
	break;
    }
    if(locked)
    {
	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, &cl_err);
	if (cl_status != OK)
	{
		err_code = E_SX1008_BAD_LOCK_RELEASE;
	}
    }
    rcb->sxf_error.err_code=err_code;
    return status;
}

/*
** Name: sxas_stop_audit
**
** Description:
**	Stops security auditing
**
**	This routine implements the external request SXF_S_STOP_AUDIT,
**	stopping auditing. 
**
**	Auditing is stopped by setting the state lock value to SXAS_STOP_LKVAL
**	This is detected at the next audit state check for each server, 
**	which then stops auditing.
**
**	Note that if auditing is suspended this functon is rejected
**
** Inputs:
**	user_request	- TRUE if on user request
**			  FALSE if system request
**
** Outputs:
**	err_code	- error code
**
** Change history:
**	5-aug-93 (robf)
**	   Created
*/
DB_STATUS
sxas_stop_audit(
    bool user_request,
    i4	*err_code
)
{
    SXF_SCB		*scb;
    DB_STATUS		status;
    i4		local_code;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LLID		lk_ll;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    bool		locked=FALSE;

    for(;;)
    {
        if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY("SXAS: Audit STOP requested\n");
	*err_code=E_SX0000_OK;
	/* Find the scb for the caller */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    break;
	}
	/*
	** Use the session's lock list
	*/
   	lk_ll = scb->sxf_lock_list;

	/*
	** First get the audit state lock in exclusive mode so things
	** don't move underneath us
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_X, &lk_val, &lk_id, 
			0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    break;
	}
	locked = TRUE;
        if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY2("SXAS: Current state lock value is %d,%d\n",
					lk_val.lk_low, lk_val.lk_high);
	/*
	** Check if suspended, can't stop if suspened (any waiting
	** threads would hang around waiting for the resume)
	*/
	if(lk_val.lk_low==SXAS_SUSPEND_LKVAL)
	{
		if(user_request)
		{
			*err_code=E_SX0021_ALREADY_SUSPENDED;
			status=E_DB_ERROR;
		}
		break;
	}
	/*
	** Check auditing is enabled, we can't stop auditing when
	** no auditing
	*/
	if ((Sxf_svcb->sxf_svcb_status & SXF_AUDIT_DISABLED)  ||
	     lk_val.lk_low==SXAS_STOP_LKVAL)
	{
		/*
		** Silently do nothing if by the system
		*/
		if(user_request)
		{
			*err_code=E_SX002B_AUDIT_NOT_ACTIVE;
			status=E_DB_ERROR;
		}
		break;
	}

	/* Save the current lock value for restart */
	status=sxas_save_state_value(&lk_ll, &lk_val, err_code);
	if(status!=E_DB_OK)
		break;

	/* Update the lock value to indicate stopped */
	lk_val.lk_low=SXAS_STOP_LKVAL;
	lk_val.lk_high=0;

	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, 
			&lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    break;
	}
	locked=FALSE;
	break;
    }
    /*
    ** Clean up
    */
    if(locked)
    {
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, 
			&cl_err);
    }
    if(*err_code!=E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX0029_STOP_ERROR;
	}

	if(status<E_DB_ERROR)
		status= E_DB_ERROR;
    }
    else
    {
	/*
	** Log message that auditing is stopped
	*/
	if(user_request)
	{
		_VOID_ ule_format(I_SX10AB_AUDIT_USER_STOP, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 2,
			(i4)sizeof(scb->sxf_ruser), &scb->sxf_ruser);
	}
	else
	{
		_VOID_ ule_format(I_SX10AB_AUDIT_USER_STOP, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 2,
			 12, "audit system");
	}
    }
    return status;
}

/*
** Name: sxas_restart_audit
**
** Description:
**	Restarts security auditing
**
**	This routine implements the external request SXF_S_RESTART_AUDIT,
**	restarting auditing. 
**
** Inputs:
**	new_log		- TRUE if new audit log requested
**	filename	- Name of new log
**
** Change history:
**	5-aug-93 (robf)
**	   Created
**	26-oct-93 (robf)
**         Add tracing
**	23-dec-93 (robf)
**         Check for disabled flag on restart for case  when
**	   auditing is active except for this server.
*/
static DB_STATUS
sxas_restart_audit(
	i4	*err_code,
	bool	new_log,
	char	*filename
)
{
    SXF_SCB		*scb;
    DB_STATUS		status;
    i4		local_code;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LLID		lk_ll;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    bool		locked=FALSE;

    for(;;)
    {
        if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY("SXAS: Audit RESTART requested\n");
	*err_code=E_SX0000_OK;
	/* Find the scb for the caller */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    break;
	}
	/*
	** Use the session's lock list
	*/
   	lk_ll = scb->sxf_lock_list;

	/*
	** First get the audit state lock in exclusive mode so things
	** don't move underneath us
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_X, &lk_val, &lk_id, 
			0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID))
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    break;
	}
	locked = TRUE;
	/*
	** Check if suspended, if so leave alone
	*/
	if(lk_val.lk_low==SXAS_SUSPEND_LKVAL)
	{
		*err_code=E_SX0021_ALREADY_SUSPENDED;
		break;
	}
	/*
	** Check if not stopped, if so leave alone
	*/
	if(lk_val.lk_low>=SXAS_MIN_LKVAL)
	{
		/*
		** Auditing overall is  active, but check if still
		** disabled for this server. 
		*/
		if (Sxf_svcb->sxf_svcb_status & SXF_AUDIT_DISABLED)
		{
                    if(SXS_SESS_TRACE(SXS_TAUD_STATE))
			SXF_DISPLAY("SXAS: State lock indicates active but server is  disabled, so continue\n");
		}
		else
		{
			*err_code=E_SX002C_AUDIT_IS_ACTIVE;
			break;
		}
	}
        /*
	** If switching to a new audit log we do so now, rather  than
	** waiting for the next sxas_check call.
	*/
	if (new_log)
	{
		/*
		** Call physical layer to restart audit
		*/
	        status = (*Sxap_vector->sxap_alter)(
			scb,
			filename,
			SXAP_C_RESTART_AUDIT,
			err_code);
		if(status!=E_DB_OK)
		{
	        	if(SXS_SESS_TRACE(SXS_TAUD_STATE))
				SXF_DISPLAY("Physical layer failed to restart\n");
	    		_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
				NULL, NULL, 0L, NULL, &local_err, 0);
			break;
		}
		/*
		** Remove disabled flag from the server
		*/
	        Sxf_svcb->sxf_svcb_status &= ~SXF_AUDIT_DISABLED;
	}
        if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY2("SXAS: Current state lock value is %d,%d\n",
					lk_val.lk_low, lk_val.lk_high);
	/*
	** Reset lock value if not set already
	*/
	if(lk_val.lk_low==SXAS_STOP_LKVAL)
	{
		/* Get the saved state lock value */
		status=sxas_get_state_value(&lk_ll, &lk_val, err_code);
		if(status!=E_DB_OK)
			break;
        	if(SXS_SESS_TRACE(SXS_TAUD_STATE))
			SXF_DISPLAY2("SXAS: Saved(restoring) state lock value is %d,%d\n",
					lk_val.lk_low, lk_val.lk_high);
	}
	cl_status = LKrelease(0, lk_ll, &lk_id, NULL, 
			&lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    break;
	}
	locked=FALSE;
	break;
    }
    /*
    ** Clean up
    */
    if(locked)
    {
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, 
			&cl_err);
    }
    if(*err_code!=E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX002A_RESTART_ERROR;
	}

	if(status<E_DB_ERROR)
		status= E_DB_ERROR;
    }
    else
    {
	/*
	** Log message that auditing is restarted
	*/
	_VOID_ ule_format(I_SX10AD_AUDIT_RESTART, &cl_err, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 2,
		(i4)sizeof(scb->sxf_ruser), &scb->sxf_ruser);
    }
    return status;
}

/*
** Name: sxas_save_state_value
**
** Description:
**	This saves the current audit state lock value someplace where
**	it can be later retrieved from any server in the installation.
**
**	The current implementation uses another lock (the SAVE lock) which
**	is initialized at server startup and held in N mode until shutdown
**
** Inputs:
**	lk_ll		Lock list to use
**	
**	lk_newvalue 	Lock value to save
**
** Outputs:
**	err_code	Error code
**
** Returns:
**	DB_STATUS
**
** History:
**	9-aug-93 (robf)
**	   Created
*/

static DB_STATUS
sxas_save_state_value(  
	LK_LLID	 *lk_ll,
	LK_VALUE *lk_newval,
	i4   *err_code
)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    LK_LKID		lk_id;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_SAVE_LOCK, 
						0, 0, 0, 0};
    LK_VALUE		lk_val;
    bool		locked = FALSE;

    for(;;)
    {
	if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY("SAVE_STATE_VALUE: Saving audit state value\n");
	/*
	** Take the SAVE lock in X mode
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, *lk_ll, &lk_key,
	    LK_X, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked=TRUE;
	/*
	** Update the lock value to the new value
	*/
	STRUCT_ASSIGN_MACRO(*lk_newval, lk_val);
	/*
	** Release the lock, so updating the lock value
	*/
	cl_status = LKrelease(0, *lk_ll, &lk_id, NULL, 
			&lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked=FALSE;
	break;
    }
    if(locked)
    {
	    _VOID_ LKrelease(0, *lk_ll, &lk_id, NULL, &lk_val, 
			&cl_err);
    }
    return status;
}

/*
** Name: sxas_get_state_value
**
** Description:
**	This gets a saved audit state lock value and returns it to the
**	caller.
**
**	The current implementation uses another lock (the SAVE lock) which
**	is initialized at server startup and held in N mode until shutdown
**
** Inputs:
**	lk_ll		Lock list to use
**	
** Outputs:
**	err_code	Error code
**
**	lk_newvalue 	Lock value to save
**
** Returns:
**	DB_STATUS
**
** History:
**	9-aug-93 (robf)
**	   Created
**     14-sep-2005 (horda03) Bug 115198/INGSRV 3422
**         We want to pin the State lock value while we copy the value to
**         the return variable. This was being done using an LK_X request.
**         I don't see the need for this request to be LK_X, if we make it
**         a LK_S request, no other session can change the value, and we
**         won't get the value until any other user who is changing the value
**         releases the LK_X lock its taken.
**         This also reduces the likelihood of the server crashing while holding
**         the LK_X lock; on a clustered Ingres installation, if the lock
**         is help LK_X, then other node lock requests for the lock will return
**         LK_VAL_NOTVALID.
*/

static DB_STATUS
sxas_get_state_value(  
	LK_LLID	 *lk_ll,
	LK_VALUE *lk_newval,
	i4   *err_code
)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    LK_LKID		lk_id;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_SAVE_LOCK, 
						0, 0, 0, 0};
    LK_VALUE		lk_val;
    bool		locked = FALSE;

    for(;;)
    {
	/*
	** Take the SAVE lock in S mode
	*/
	if(SXS_SESS_TRACE(SXS_TAUD_STATE))
		SXF_DISPLAY("GET_STATE_VALUE: Fetching audit state value\n");
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, *lk_ll, &lk_key,
	    LK_S, &lk_val, &lk_id, 0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked=TRUE;
	/*
	** Get the lock value into the output
	*/
	STRUCT_ASSIGN_MACRO( lk_val, *lk_newval);
	/*
	** Release the lock
	*/
	cl_status = LKrelease(0, *lk_ll, &lk_id, NULL, 
			&lk_val, &cl_err);
	if (cl_status != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	locked=FALSE;
	break;
    }
    if(locked)
    {
	    _VOID_ LKrelease(0, *lk_ll, &lk_id, NULL, &lk_val, 
			&cl_err);
    }
    return status;
}

/*
** Name: sxas_poll_resume
**
** Description:
**	Poll to resume security auditing
**
**	This routine is only called on a VaxCluster, and checks to see whether
**	auditing has been resumed yet. The problem this routine is addressing
**	is that LKevents are not broadcast across nodes of a cluster, so
**	some other method is required to communicate the RESUME signal to
**	all nodes. The method chosen is to periodically poll on each node,
**	via the audit thread, to see whether the state lock has changed from
**	SUSPEND, if so we signal RESUME auditing on the local node, so any
**	waiting threads are woken up. The delay between polls is determined
**	by the audit thread (in sxac.c) this routine is called to implement
**	the actual poll/wakeup.
**
** Inputs:
**	none
**
** Returns:
**	E_DB_OK   - No action, audit is active
**
**	E_DB_WARN - Auditing has been suspened, need to loop
**
**	E_DB_ERROR- An error occurred
**
** Outputs:
**	err_code	- error code
**
** Change history:
**	10-aug-93 (robf)
**	   Created
*/
DB_STATUS
sxas_poll_resume(
    i4	*err_code
)
{
    SXF_SCB		*scb;
    DB_STATUS		status=E_DB_OK;
    i4		local_code;
    LK_LKID		lk_id;
    LK_VALUE		lk_val;
    LK_LLID		lk_ll;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAS_LOCK, SXAS_STATE_LOCK, 
						0, 0, 0, 0};
    STATUS		cl_status;
    CL_ERR_DESC		cl_err;
    i4		local_err;
    bool		locked=FALSE;
    SXF_ASCB		*ascb = Sxf_svcb->sxf_aud_state;
    LK_EVENT		lk_event;

    for(;;)
    {
	*err_code=E_SX0000_OK;
	/* Find the scb for the caller */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    *err_code = local_err;
	    break;
	}
	/*
	** Use the session's lock list
	*/
   	lk_ll = scb->sxf_lock_list;

	/*
	** First get the audit state lock in exclusive mode so things
	** don't move underneath us
	*/
	MEfill(sizeof(LK_LKID), 0, &lk_id);
	cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key,
			LK_X, &lk_val, &lk_id, 
			0, &cl_err);
	if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	{
            TRdisplay( "SXAS::Bad LKrequest %d\n", __LINE__);
            _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1035_BAD_LOCK_REQUEST;
	    break;
	}
	locked = TRUE;
	/*
	** Check if not suspended, if  so we
	** are done and can resume auditing. Note that this may already
	** have been done by some other thread, so we check the current 
	** lock value.
	*/
	if(lk_val.lk_low==SXAS_SUSPEND_LKVAL)
	{
		/*
		** Suspended, so done, the lock is released during
		** cleanup below.
		*/
		status=E_DB_WARN;
		break;
	}
	/*
	** Check if the DBMS is still set to suspended
	*/
        if(ascb->sxf_lockval.lk_low!=SXAS_SUSPEND_LKVAL)
        {
		/*
		** DBMS is no longer marked as suspended, so presumably
		** some other thread already woke us up, or we were never
		** suspended. Either way we are done.
		** The lock is released during cleanup below
		*/
		status=E_DB_OK;
		break;
	}
	/*
	** At this point we are resuming auditing, so wake up any waiting
	** threads on this node.
	*/
	lk_event.type_high = SXA_EVENT;
	lk_event.type_low = 0;
	lk_event.value = SXA_EVENT_RESUME;

	cl_status = LKevent(LK_E_CLR|LK_E_CROSS_PROCESS, lk_ll, &lk_event, &cl_err);

	if (cl_status != OK)
	{
	    *err_code =  E_SX1047_BAD_EVENT_CLEAR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** Update the internal lock value
	*/
        ascb->sxf_lockval.lk_low=lk_val.lk_low;
	ascb->sxf_lockval.lk_high=lk_val.lk_high;
	/*
	** Return OK say we are not suspended.
	*/
	status=E_DB_OK;
	break;
    }
    /*
    ** Clean up
    */
    if(locked)
    {
	    _VOID_ LKrelease(0, lk_ll, &lk_id, NULL, &lk_val, 
			&cl_err);
    }
    if(*err_code!=E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code=E_SX0024_RESUME_ERROR;
	}

	if(status<E_DB_ERROR)
		status= E_DB_ERROR;
    }
    return status;
}
