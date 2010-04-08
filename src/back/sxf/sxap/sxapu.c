/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <pc.h>
# include    <cs.h>
# include    <lk.h>
# include    <cx.h>
# include    <tm.h>
# include    <di.h>
# include    <lo.h>
# include    <me.h>
# include    <lg.h>
# include    <st.h>
# include    <nm.h>
# include    <er.h>
# include    <pm.h>
# include    <cv.h>
# include    <st.h>
# include    <tr.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
# include    "sxapint.h"

/*
** Name: SXAPU.C - Utility routines for the SXAP auditing system
**
** Description:
**      This file contains utility the control routines used by the SXF
**      low level auditing system, SXAP.
**
**          sxap_ac_lock     - Take the ACCESS lock
**          sxap_q_lock      - Take the QUEUE lock
**          sxap_ac_unlock   - Release the ACCESS lock
**	    sxap_shm_dump    - Dump shared memory info to trace log.
**	    sxap_lkerr_reason- Print more detailed info on locking errors.
**          sxap_sem_lock    - Take sxap_semaphore if not already owned by
**                             thread.
**          sxap_sem_unlock  - Release sxap_semaphore if owned by thread.
**
** History:
**      2-apr-94 (robf)
**          Created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      14-sep-2005 (horda03) Bug 115198/INGSRV3422
**          On a clustered Ingres installation, LKrequest can return
**          LK_VAL_NOTVALID. This implies that teh lock request has been
**          granted, but integrity of the lockvalue associated with the
**          lock is in doubt. It is the responsility of the caller
**          to decide whether the lockvalue should be reset.
**      01-Oct-2008 (jonj)
**          Replace CSp/v_semaphore(sxap_semaphore) calls with more robust
**          sxap_sem_lock(), sxap_sem_unlock() functions.
*/

GLOBALREF SXAP_CB  *Sxap_cb ;

static bool debug_trace=FALSE;

/*
** Name: sxap_ac_lock
**
** Description: Take SXAP ACCESS lock, implementing retry for lock interrupts.
**
**	 The main reason for this is access to the access lock shouldn't be
**	 interruptable (if it fails then the audit system will generate an
**	 error which is serious) so we retry in case of interrupt here.
**
** Inputs:
**	scb    - scb
**	lkmode - requested mode
**
** Outputs:
**	lkid	- lock id
**	lkvalue - lock value
**	err_code- error code if applicable
**
** Returns
**	DBSTATUS
**
** History:
**	31-mar-94 (robf)
**         Created to modularize code
*/
DB_STATUS sxap_ac_lock(
    SXF_SCB		*scb,
    i4			lkmode,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
)
{
    i4  		lock_retry;
    STATUS 		cl_status=OK;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    CL_ERR_DESC		cl_err;
    i4		local_err;
# ifdef xDEBUG
    if(SXS_SESS_TRACE(SXS_TAUD_PLOCK) || debug_trace)
	TRdisplay("SXAP_AC_LOCK: %x: Taking ACCESS lock in mode %d\n", 
			scb, lkmode);
# endif

    lock_retry=0;
    for(;;)
    {
	    cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key, 
			lkmode, lk_val, lk_id, 0L, &cl_err);
	    if (cl_status==LK_INTERRUPT)
	    {
		lock_retry++;
		if(lock_retry<5)
			continue;
	    }
	    else if(cl_status==LK_INTR_GRANT)
	    {
		cl_status=OK;
	    }
	    if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	    {
                TRdisplay( "SXAPU::Bad LKrequest %d\n", __LINE__);
                _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	        sxap_lkerr_reason("SXAP_ACCESS", cl_status);
	        *err_code = E_SX1035_BAD_LOCK_REQUEST;
	        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    }
	    break;
   }
   if(cl_status!=OK)
	return E_DB_ERROR;
   else
   {
# ifdef xDEBUG
    	if(SXS_SESS_TRACE(SXS_TAUD_PLOCK) || debug_trace)
		TRdisplay("SXAP_AC_LOCK: %x: Got ACCESS lock in mode %d\n", 
			scb, lkmode);
# endif
	return E_DB_OK;
   }
}

/*
** Name: sxap_q_lock
**
** Description: Take SXAP QUEUE lock, implementing retry for lock interrupts.
**
**	 The main reason for this is access to the queue lock shouldn't be
**	 interruptable (if it fails then the audit system will generate an
**	 error which is serious) so we retry in case of interrupt here.
**
** Inputs:
**	scb    - scb
**	lkmode - requested mode
**
** Outputs:
**	lkid	- lock id
**	lkvalue - lock value
**	err_code- error code if applicable
**
** Returns
**	DBSTATUS
**
** History:
**	31-mar-94 (robf)
**         Created to modularize code
*/
DB_STATUS sxap_q_lock(
    SXF_SCB		*scb,
    i4			lkmode,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
)
{
    i4  		lock_retry;
    STATUS 		cl_status=OK;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_QUEUE, 0, 0, 0, 0};
    CL_ERR_DESC		cl_err;
    i4		local_err;
    i4	        nodeid, l_nodeid;

    /*
    ** If we are running on a cluster we need to initialize the
    ** query lock key to contain the node id, since this is per-node.
    */
# ifdef xDEBUG
    if(SXS_SESS_TRACE(SXS_TAUD_PLOCK) || debug_trace)
	TRdisplay("SXAP_Q_LOCK: %x: Taking QUEUE lock in mode %d\n", 
			scb, lkmode);
# endif
    if (CXcluster_enabled())
    {
	    _VOID_ LGshow(LG_A_NODEID, (PTR)&nodeid, 
			sizeof(nodeid), &l_nodeid, &cl_err);
	    lk_key.lk_key3 = (i4)nodeid;
    }				
    lock_retry=0;
    for(;;)
    {
	    cl_status = LKrequest(LK_PHYSICAL, lk_ll, &lk_key, 
			lkmode, lk_val, lk_id, 0L, &cl_err);
	    if (cl_status==LK_INTERRUPT)
	    {
		lock_retry++;
		if(lock_retry<5)
			continue;
	    }
	    else if(cl_status==LK_INTR_GRANT)
	    {
		cl_status=OK;
	    }
	    if ( (cl_status != OK) && (cl_status != LK_VAL_NOTVALID) )
	    {
                TRdisplay( "SXAPU::Bad LKrequest %d\n", __LINE__);
                _VOID_ ule_format(cl_status, &cl_err, ULE_LOG,
			NULL, NULL, 0L, NULL, &local_err, 0);
	        sxap_lkerr_reason("SXAP_QUEUE", cl_status);
	        *err_code = E_SX1035_BAD_LOCK_REQUEST;
	        _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, 
		NULL, NULL, 0L, NULL, &local_err, 0);
	    }
	    break;
   }
   if(cl_status!=OK)
	return E_DB_ERROR;
   else
   {
# ifdef xDEBUG
    	if(SXS_SESS_TRACE(SXS_TAUD_PLOCK) || debug_trace)
		TRdisplay("SXAP_Q_LOCK: %x: Got QUEUE lock in mode %d\n", 
			scb, lkmode);
# endif
	return E_DB_OK;
   }
}

/*
** Name: sxap_q_unlock
**
** Description: Releases SXAP QUEUE lock
**
** Inputs:
**	scb    - scb
**	lkid	- lock id
**	lkvalue - lock value
**
** Outputs:
**	err_code- error code if applicable
**
** Returns
**	DBSTATUS
**
** History:
**	31-mar-94 (robf)
**         Created to modularize code
*/
DB_STATUS sxap_q_unlock(
    SXF_SCB		*scb,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
)
{
    i4  		lock_retry;
    STATUS 		cl_status=OK;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_QUEUE, 0, 0, 0, 0};
    CL_ERR_DESC		cl_err;
    i4		local_err;

# ifdef xDEBUG
    if(SXS_SESS_TRACE(SXS_TAUD_PLOCK) || debug_trace)
	TRdisplay("SXAP_Q_UNLOCK: %x: Releasing QUEUE lock\n", scb);
# endif

    cl_status = LKrelease(0, lk_ll, lk_id, NULL, lk_val, &cl_err);
    if (cl_status != OK)
    {
	sxap_lkerr_reason("QUEUE,X ", cl_status);
	*err_code = E_SX1008_BAD_LOCK_RELEASE;
	_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,	
			NULL, NULL, 0L, NULL, &local_err, 0);
	return E_DB_ERROR;
    }
    else
	return E_DB_OK;
}

/*
** Name: sxap_ac_unlock
**
** Description: Releases SXAP ACCESS lock
**
** Inputs:
**	scb    - scb
**	lkid	- lock id
**	lkvalue - lock value
**
** Outputs:
**	err_code- error code if applicable
**
** Returns
**	DBSTATUS
**
** History:
**	31-mar-94 (robf)
**         Created to modularize code
*/
DB_STATUS sxap_ac_unlock(
    SXF_SCB		*scb,
    LK_LKID		*lk_id,
    LK_VALUE		*lk_val,
    i4		*err_code
)
{
    i4  		lock_retry;
    STATUS 		cl_status=OK;
    LK_LLID		lk_ll = scb->sxf_lock_list;
    LK_LOCK_KEY		lk_key = {LK_AUDIT, SXAP_LOCK, SXAP_ACCESS, 0, 0, 0, 0};
    CL_ERR_DESC		cl_err;
    i4		local_err;

# ifdef xDEBUG
    if(SXS_SESS_TRACE(SXS_TAUD_PLOCK) || debug_trace)
	TRdisplay("SXAP_AC_UNLOCK: %x: Releasing ACCESS lock\n", scb);
# endif

    cl_status = LKrelease(0, lk_ll, lk_id, NULL, lk_val, &cl_err);
    if (cl_status != OK)
    {
	sxap_lkerr_reason("ACCESS", cl_status);
	*err_code = E_SX1008_BAD_LOCK_RELEASE;
	_VOID_ ule_format(*err_code, &cl_err, ULE_LOG,	
			NULL, NULL, 0L, NULL, &local_err, 0);
	return E_DB_ERROR;
    }
    else
	return E_DB_OK;
}

/*
** Name: sxap_shm_dump - Dump sxap shared memory info
**
** Description:
**	Dump sxap shared memory info to trace log
**
** History:
**	3-apr-94 (robf)
*/
VOID
sxap_shm_dump()
{
	i4 i;	
	SXAP_SHM_SEG *shm;

	TRdisplay("----------- DUMP OF SXAP SHARED MEMORY -------------\n");
	if (Sxap_cb->sxap_shm==NULL)
	{
		TRdisplay("    Shared memory buffer is not initialized\n");
		return;
	}
	shm=Sxap_cb->sxap_shm;

	TRdisplay("    Shared memory version   : %d\n", 
			shm->shm_version);

	TRdisplay("    Number of shared record buffers: %d\n",
			shm->shm_num_qbuf);

 	TRdisplay("    Start : [%d,  %d]\n", 
			shm->shm_qstart.trip, shm->shm_qstart.buf);
 	TRdisplay("    Write : [%d,  %d]\n", 
			shm->shm_qwrite.trip, shm->shm_qwrite.buf);
 	TRdisplay("    End   : [%d,  %d]\n", 
			shm->shm_qend.trip, shm->shm_qend.buf);

	TRdisplay("Shared Buffers:\n");
	for(i=0;i <shm->shm_num_qbuf; i++)
	{
		TRdisplay("    Buffer %d Flags %v\n",
			i, 
			"EMPTY,USED,DETAIL_TXT,SECLABEL",
			shm->shm_qbuf[i].flags);
	}
}

/*
** Name: sxap_lkerr_reason.
**
** Description: Print a detailed reason why the lock request failed. 
**	        Although LK should print a diagnostic, this is used
** 		for cases where it doesn't.
**
** History:
**	15-dec-1992 (robf)
**	    Created.
**	13-jan-94 (robf)
**          Add LK_INTERRUPT, ule_format()
*/
VOID
sxap_lkerr_reason(char *lk_name, STATUS cl_status)
{
    char 	*mesg;
    i4	local_err;

    /*
    ** lock errors now  should match error messages so try to log if possible
    */
    if(cl_status)
    {
        _VOID_ ule_format(cl_status, 0, ULE_LOG,	
		NULL, NULL, 0L, NULL, &local_err, 0);
    }
    switch (cl_status)
    {
    case LK_NEW_LOCK:
		mesg="A new lock was granted (NEW_LOCK)"; 
		break;
    case LK_BADPARAM:
		mesg="A bad parameter was passed to LK (BADPARAM)";
		break;
    case LK_NOLOCKS:
		mesg="LK reports no more locks (NOLOCKS)";
		break;
    case LK_DEADLOCK:
		mesg="Deadlock was detected (DEADLOCK)";
		break;
    case LK_TIMEOUT:
		mesg="Lock request timed out (TIMEOUT)";
		break;
    case LK_BUSY:
		mesg="Lock requested is busy (BUSY)";
		break;
    case LK_INTR_GRANT:
		mesg="Lock was granted but was Interrupted (INTR_GRANT)";
		break;
#ifdef LK_INTERRUPT
    case LK_INTERRUPT:
		mesg="Lock request was interrupted (INTERRUPT)";
		break;
#endif
    default:
		mesg="Other locking system error.";
		TRdisplay("SXF: Other locking system error code %d (X%x)\n",
				cl_status,cl_status);
		break;
    }
    TRdisplay("SXF Lock Request for '%s' failed because: %s\n", lk_name, mesg);
}

/*
** Name: sxap_sem_lock.
**
** Description: Take sxap_semaphore on behalf of the thread if not  
**              already owned by the thread.
**
** History:
**      01-Oct-2008 (jonj)
**          Created.
*/
DB_STATUS
sxap_sem_lock(i4 *err_code)
{
    STATUS      status;
    CS_SID      my_sid;

    CSget_sid(&my_sid);

    /* If already owned, just incr pin count */
    if ( Sxap_cb->sxap_sem_owner == my_sid )
        Sxap_cb->sxap_sem_pins++;
    else
    {
        if ( status = CSp_semaphore(TRUE, &Sxap_cb->sxap_semaphore) )
        {
            TRdisplay("%@ sxap_sem_lock %d p_sem status %x\n",
                        __LINE__, status);
            *err_code = status;
            return(E_DB_ERROR);
        }
        else
        {
            /* Now we own it */
            Sxap_cb->sxap_sem_owner = my_sid;
            Sxap_cb->sxap_sem_pins++;
        }
    }
    return(E_DB_OK);
}

/*
** Name: sxap_sem_unlock.
**
** Description: Release sxap_semaphore if owned by this thread.
**
** History:
**      01-Oct-2008 (jonj)
**          Created.
*/
DB_STATUS
sxap_sem_unlock(i4 *err_code)
{
    STATUS      status;
    CS_SID      my_sid;

    CSget_sid(&my_sid);

    /* If we own it... */
    if ( Sxap_cb->sxap_sem_owner == my_sid )
    {
        /* ...and we no longer have it pinned.. */
        if ( --Sxap_cb->sxap_sem_pins == 0 )
        {
            /* ...then release it */
            Sxap_cb->sxap_sem_owner = (CS_SID)NULL;

            if ( status = CSv_semaphore(&Sxap_cb->sxap_semaphore) )
            {
                TRdisplay("%@ sxap_sem_unlock %d v_sem status %x\n",
                            __LINE__, status);
                *err_code = status;
                return(E_DB_ERROR);
            }
        }
    }
    return(E_DB_OK);
}
