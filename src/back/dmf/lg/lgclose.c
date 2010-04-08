/*
**Copyright (c) 2004 Ingres Corporation
**
*/

/*
NO_OPTIM = r64_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <er.h>
#include    <pc.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <ulf.h>
#include    <dm.h>
#include    <lg.h>
#include    <lgdef.h>
#include    <lgdstat.h>
#include    <lgkdef.h>

/**
**
**  Name: LGCLOSE.C - Implements the LGclose function of the logging system
**
**  Description:
**	This module contains the code which implements LGclose.
**	
**	    LGclose -- close down logging system operations for this process
**
**  History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Removed references to obsolete 
**	    lgd->lgd_n_servers counter
**	18-jan-1993 (bryanp)
**	    Check LG_logs_opened to see which log files to close.
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	17-mar-1993 (rogerk)
**	    Moved lgd status values to lgdstat.h so that they are visible
**	    by callers of LGshow(LG_S_LGSTS).
**	26-apr-1993 (bryanp)
**	    When the master closes the local log, if this is a cluster, clear
**	    the LSN generator.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    When the master closes a remote log, this does not take the
**		installation offline -- only closing the local log does so.
**	    De-allocate the LFB when closing a non-local log.
**      13-jun-1995 (chech02)
**          Added LGK_misc_sem semaphore to protect critical sections.(MCT)
**	05-Jan-1996 (jenjo02)
**	    Mutex granularity project. Semaphores must now be explicitly
**	    named in calls to LKMUTEX.C
**	18-Jan-1996 (jenjo02)
**          lbk_table replaced with lfbb_table (LFBs) and ldbb_table
**          (LDBs) to ensure that semaphores now embedded in those
**          structures are not corrupted. Similarly, sbk_table 
**          replaced with lpbb_table (LPBs), lxbb_table (LXBs), and
**          lpdb_table (LPDs).
**	03-jun-1996 (canor01)
**	    Removed LGK_misc_sem semaphore, since code section protected by LG mutex.
**	26-Jan-1998 (jenjo02)
**	    Partitioned Log File Project:
**	    Close all log file partitions.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      12-Feb-2003 (hweho01)
**          Turned off optimizer for AIX hybrid build by using 
**          the 64-bit configuration string (r64_us5), because the    
**          error happens only with the 64-bit compiler option.  
**      7-oct-2004 (thaju02)
**          Use SIZE_TYPE to allow memory pools > 2Gig.
**	6-Apr-2007 (kschendel) SIR 122890
**	    Use an intermediate for aliasing external i4 id's and LG_ID.
**	    Originally this was to fix gcc -O2's strict aliasing, but
**	    Ingres has so many other strict-aliasing violations that
**	    -fno-strict-aliasing is needed.  The id/LG_ID thing was so
**	    egregious, though, that I've kept this change.
**      27-Sep-2007 (horda03) Bug 119196
**          Prevent a mutex deadlock during ingstop.
**	09-Dec-2008 (jonj)
**	    SIR 120874: use new form uleFormat, CL_CLEAR_ERR.
*/
static STATUS	LG_close(
		    LG_ID               *lg_id,
		    CL_ERR_DESC		*sys_err);

/*{
** Name: LGclose	- CLose log file.
**
** Description:
**      This routine will close the log file for the current process.
**	If the caller specified LG_MASTER at LGopen time, then closing
**	the file will affect all other processes that also have the file open.
**	These other processes will be denied all further access to the logging
**	system.  This is the same as a shutdown call if the master is
**      requesting a close.  If a shutdown is in process, then all
**      LG calls should be rejected with LG_BADPARAM.
**
** Inputs:
**      lg_id                           The log indentifer for the file.
**
** Outputs:
**      sys_err                         Reason for error return status.
**	Returns:
**	    OK
**	    LG_BADPARAM		    Parameter(s) in error.
**	    LG_CANTCLOSE	    Error closing file.
**	    LG_NOTLOADED	    LG system not available.
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
**	18-jan-1993 (bryanp)
**	    Check LG_logs_opened to see which log files to close.
**	    Call CL_CLEAR_ERR to clear the sys_err.
**	16-feb-1993 (bryanp)
**	    When the master closes the local log, if this is a cluster, clear
**	    the LSN generator.
**	24-Jan-1996 (jenjo02)
**	    Removed acquisition and freeing of lgd_mutex.
*/
STATUS
LGclose(
LG_LGID             lg_id,
CL_ERR_DESC	    *sys_err)
{
    LGD             *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    LG_I4ID_TO_ID   xid;
    i4	    status;
    STATUS	    local_status;
    CL_ERR_DESC	    local_sys_err;
    i4	    err_code;

    CL_CLEAR_ERR(sys_err);

    xid.id_i4id = lg_id;
    status = LG_close(&xid.id_lgid, sys_err);

    if (status == OK)
    {
	status = LGlsn_term(sys_err);
    }

    return(status);
}

/*{
** Name: LG_close	- CLose log file.
**
** Description:
**      This routine closes a log file for a process.  If the master process
**	closes it's file, then all slave processes are orphaned.
**
** Inputs:
**      lg_id                           Log Identifier.
**
** Outputs:
**	Returns:
**	    LG_I_NORMAL
**	    LG_I_BADPARAM
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	Summer, 1992 (bryanp)
**	    Working on the new portable logging and locking system.
**	16-oct-1992 (jnash) for rogerk
**	    Reduced logging project.  Removed references to obsolete 
**	    lgd->lgd_n_servers counter
**	26-jul-1993 (bryanp)
**	    When the master closes a remote log, this does not take the
**		installation offline -- only closing the local log does so.
**	    De-allocate the LFB when closing a non-local log.
**	24-Jan-1996 (jenjo02)
**	    Dependence on single lgd_mutex removed.
**	24-Aug-1999 (jenjo02)
**	    Dropped lgd_status_mutex, lfb_cp_mutex, use lgd_mutex instead.
**      27-Sep-2007 (horda03) Bug 119196
**          Prevent a mutex deadlock between the lgd_mutex in this function
**          and the lpb_mutex in LG_check_cp_servers(). The LG_check_cp_server
**          may have selected the lpb being released by this server, and so
**          will be waiting for the lpb_mutex. The lpb_mutex is held by this
**          server, but it could have been stalled whilst the LG_check_cp_server
**          held the lgd_lpb_q_mutex. This mutex is released before the
**          LG_check_cp_server tries to get the lpb_mutex, but this is held by
**          a thread here. The deadlock occurs because this function needs the
**          lgd_mutex, but this is held by the thred in LG_check_cp_server.
*/
static STATUS
LG_close(
LG_ID               *lg_id,
CL_ERR_DESC	    *sys_err)
{
    register LGD    *lgd = (LGD *)LGK_base.lgk_lgd_ptr;
    register LPB    *lpb;
    register LFB    *lfb;
    LPB		    *next_lpb;
    LPB		    *prev_lpb;
    SIZE_TYPE	    *lpbb_table;
    i4	    err_code;
    STATUS	    local_status;
    STATUS	    status;
    CL_ERR_DESC	    local_sys_err;
    i4		    i;

    LG_WHERE("LG_close")
    
    /*	Check the lg_id. */

    if (lg_id->id_id == 0 || (i4)lg_id->id_id > lgd->lgd_lpbb_count)
    {
	uleFormat(NULL, E_DMA40D_LGCLOSE_BAD_ID, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 2,
		    0, lg_id->id_id, 0, lgd->lgd_lpbb_count);
	return (LG_BADPARAM);
    }

    lpbb_table = (SIZE_TYPE *)LGK_PTR_FROM_OFFSET(lgd->lgd_lpbb_table);
    lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpbb_table[lg_id->id_id]);

    if (lpb->lpb_type != LPB_TYPE ||
	lpb->lpb_id.id_instance != lg_id->id_instance)
    {
	uleFormat(NULL, E_DMA40E_LGCLOSE_BAD_PROC, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, NULL, 0, NULL, &err_code, 3,
		    0, lpb->lpb_type, 0, lpb->lpb_id.id_instance,
		    0, lg_id->id_instance);
	return (LG_BADPARAM);
    }

    lfb = (LFB *)LGK_PTR_FROM_OFFSET(lpb->lpb_lfb_offset);

    if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
	return(status);

    if ((lpb->lpb_status & LPB_MASTER) != 0 &&
	lgd->lgd_master_lpb == LGK_OFFSET_FROM_PTR(lpb))
    {
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
	    return(status);
	lgd->lgd_status &= ~LGD_ONLINE;
	lgd->lgd_master_lpb = 0;
	(VOID)LG_unmutex(&lgd->lgd_mutex);
    }

    status = OK;

    if (lfb->lfb_status & LFB_USE_DIIO)
    {
	if (lfb->lfb_status & LFB_PRIMARY_LOG_OPENED)
	{
	    for (i = 0, local_status = OK;
		 i < lfb->lfb_di_cbs[LG_PRIMARY_DI_CB].di_io_count &&
		     local_status == OK;
		 i++)
	    {
		local_status = DIclose(
			&lfb->lfb_di_cbs[LG_PRIMARY_DI_CB].di_io[i],
			&local_sys_err);
	    }
	    if ( status = local_status )
	    {
		(VOID) uleFormat(NULL, local_status, &local_sys_err, ULE_LOG, NULL,
				    NULL, 0, NULL,
				    &err_code, 0);
		STRUCT_ASSIGN_MACRO(local_sys_err, *sys_err);
	    }
	}

	if (lfb->lfb_status & LFB_DUAL_LOG_OPENED)
	{
	    for (i = 0, local_status = OK;
		 i < lfb->lfb_di_cbs[LG_DUAL_DI_CB].di_io_count &&
		     local_status == OK;
		 i++)
	    {
		local_status = DIclose(
			&lfb->lfb_di_cbs[LG_DUAL_DI_CB].di_io[i],
			&local_sys_err);
	    }
	    if (local_status != OK)
	    {
		(VOID) uleFormat(NULL, local_status, &local_sys_err, ULE_LOG, NULL,
				    NULL, 0, NULL,
				    &err_code, 0);
		if (status == OK)
		{
		    status = local_status;
		    STRUCT_ASSIGN_MACRO(local_sys_err, *sys_err);
		}
	    }
	}
    }
    else
    {
	if (LG_logs_opened & LG_PRIMARY_LOG_OPENED)
	{
	    for (i = 0, local_status = OK;
		 i < Lg_di_cbs[LG_PRIMARY_DI_CB].di_io_count &&
		     local_status == OK;
		 i++)
	    {
		local_status = DIclose(
			&Lg_di_cbs[LG_PRIMARY_DI_CB].di_io[i], 
			&local_sys_err);
	    }
	    if (status = local_status)
	    {
		(VOID) uleFormat(NULL, local_status, &local_sys_err, ULE_LOG, NULL,
				    NULL, 0, NULL,
				    &err_code, 0);
		STRUCT_ASSIGN_MACRO(local_sys_err, *sys_err);
	    }
	}

	/* should only do this if primary something was opened */
	if (LG_logs_opened & LG_DUAL_LOG_OPENED)
	{
	    for (i = 0, local_status = OK;
		 i < Lg_di_cbs[LG_DUAL_DI_CB].di_io_count &&
		     local_status == OK;
		 i++)
	    {
		local_status = DIclose(
			&Lg_di_cbs[LG_DUAL_DI_CB].di_io[i], 
			&local_sys_err);
	    }
	    if (local_status != OK)
	    {
		(VOID) uleFormat(NULL, local_status, &local_sys_err, ULE_LOG, NULL,
				    NULL, 0, NULL,
				    &err_code, 0);
		if (status == OK)
		{
		    status = local_status;
		    STRUCT_ASSIGN_MACRO(local_sys_err, *sys_err);
		}
	    }
	}
    }

    /*
    ** Check the process block to see if all database connections have been
    ** cleaned up (some connections may have their cleanup deferred if
    ** the RCP is currently "borrowing" these control blocks to perform recovery
    ** on the database). If all database connections are now gone, we can
    ** extinguish this process block. If the process was using a non-local LFB,
    ** then the LFB should now be deallocated as well.
    */

    if (lpb->lpb_lpd_count == 0)
    {
	/*	Remove from active LPB queue. */
	if (status = LG_mutex(SEM_EXCL, &lgd->lgd_lpb_q_mutex))
	    return(status);
	next_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb->lpb_next);
	prev_lpb = (LPB *)LGK_PTR_FROM_OFFSET(lpb->lpb_prev);
	
	next_lpb->lpb_prev = lpb->lpb_prev;
	prev_lpb->lpb_next = lpb->lpb_next;
	(VOID)LG_unmutex(&lgd->lgd_lpb_q_mutex);

	/*
	** If this lpb is a CPAGENT, decrement that count.
	*/
	if (lpb->lpb_status & LPB_CPAGENT &&
	    (lpb->lpb_status & LPB_VOID) == 0)
	{
            /* Although the lpb has been removed from the active queue,
            ** a session may have detected this LPB and is now waiting
            ** for the lpb_mutex. If this session holds the lgd_mutex
            ** then a deadlock will occur.
            ** The othe rsession may be interested in this LPB because it
            ** is a LPB_CPAGENT. Unset this flag before releasing the
            ** lpb mutex will ensure any other thread waiting for this
            ** LPB will recognose it has changed.
            */
            lpb->lpb_status &= ~LPB_CPAGENT;
            (VOID)LG_unmutex(&lpb->lpb_mutex);

	    if (status = LG_mutex(SEM_EXCL, &lgd->lgd_mutex))
		return(status);
	    lgd->lgd_n_cpagents--;
	    (VOID)LG_unmutex(&lgd->lgd_mutex);

            /* Re-acquire the lpb_mutex. We need it to release it */
            if (status = LG_mutex(SEM_EXCL, &lpb->lpb_mutex))
                return(status);
	}

	if (lfb->lfb_status & LFB_USE_DIIO)
	{
	    if (status = LG_mutex(SEM_EXCL, &lfb->lfb_mutex))
		return(status);
	    /*
	    ** Note that we de-allocate the lfb's buffers, before
	    ** we de-allocate the LFB itself.
	    */

	    LG_dealloc_lfb_buffers( lfb );
	    /*
	    ** Deallocating the LFB also frees the lfb_mutex
	    */
	    LG_deallocate_cb(LFB_TYPE, (PTR)lfb);
	} 


	/*
	** If this server was connected to a shared buffer manager, check
	** for any lpb's needing recovery that are waiting for all servers
	** connected to the buffer manager to exit.
	*/
	if (lpb->lpb_status & LPB_SHARED_BUFMGR)
	    LG_lpb_shrbuf_disconnect(lpb->lpb_bufmgr_id);

	/*
	** If this is the archiver, clear its LPB in the LGD.
	*/
	if (lpb->lpb_status & LPB_ARCHIVER)
	    lgd->lgd_archiver_lpb = 0;

	/*
	** Deallocate the LPB and free its lpb_mutex,
	** and decrement lgd_lpb_inuse.
	*/
	LG_deallocate_cb(LPB_TYPE, (PTR)lpb);

	/*
	** If shutdown was requested and all that is left is the recovery
	** process, then signal the shutdown.
	** Otherwise, check if this server exiting allows us to continue
	** with consistency point protocol.
	*/
	if (lgd->lgd_lpb_inuse == 1 && (lgd->lgd_status & LGD_START_SHUTDOWN))
	    LG_signal_event(LGD_IMM_SHUTDOWN, 0, FALSE);
	else
	    LG_last_cp_server();
    }
    else
    {
	/* 
	** Can't remove lpb, but mark lpb inactive so that the lpb will be
	** removed when the recovery process finishes aborting the transaction.
	*/

	lpb->lpb_status &= ~LPB_ACTIVE;
	(VOID)LG_unmutex(&lpb->lpb_mutex);
    }

    return (OK);
}
