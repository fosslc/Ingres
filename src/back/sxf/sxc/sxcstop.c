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
# include    <pc.h>
# include    <lk.h>
# include    <tm.h>
# include    <tr.h>
# include    <ulf.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <ulm.h>

/*
** Name: SXCSTOP.C - SXF facility termination routines
**
** Description:
**	The file contains the routines used to terminate the SXF 
**	facility.
**
**	    sxc_shutdown - Shutdown SXF.
**
** History:
**	11-aug-1992 (markg)
**	    Initial creation.
**	26-oct-1992 (markg)
**	    Updated error handling.
**      7-dec-1992 (markg)
**          Fix AV caused by passing incorrect number of parameters
**          to ule_format.
**	10-mar-1993 (markg)
**	    Fixed problem where errors returned from ulm were not getting
**	    handled correctly.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-aug-1997 (canor01)
**	    Remove semaphores before freeing memory.
**	02-sep-1997 (hanch04)
**	    Check for audit semaphore before removing it.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: SXC_SHUTDOWN - shutdown the SXF facility
**
** Description:
** 	This routine is used to shutdown the SXF facility, it is called at
**	server shutdown time via the sxf_call(SXC_SHUTDOWN) request. This 
**	routine will return an error if there are still active sessions 
**	registered unless the rcb->sxf_force flag is set. In this case the 
**	facility will be shutdown regardless. 
**
**	Overview of algorithm:-
**
**	If there are still active sessions and sxf_force not set, return error.
**	Mark the facility as being offline.
**	Call sxac_shutdown to shutdown the auditing system.
**	Destroy all allocated SCBs and free their associated resourses.
**	Destroy the facility lock list.
**	Destroy the facility memory pool.
**
** Inputs:
**	rcb
**	    .sxf_force		Forced shutdown will be initiated if this
**				contains a non-zero value.
**
** Outputs:
**	rcb
**	    .sxf_error		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-1992 (markg)
**	    Initial creation.
**      7-dec-1992 (markg)
**          Fix AV caused by passing incorrect number of parameters
**          to ule_format.
**	17-dec-1992 (robf)
**	    Added stats printing on shutdown. Must do this *after* closing
**	    the last audit file, but *before* freeing the control blocks which
**	    hold the stats.
**	10-mar-1993 (markg)
**	    Fixed problem where errors returned from ulm were not getting
**	    handled correctly.
**	21-july-93 (robf)
**	    Add call to sxlc_shutdown
**	24-july-97 (nanpr01)
**	    Add LK_MULTITHREAD flag in LKrelease.
**	08-aug-1997 (canor01)
**	    Remove semaphores before freeing memory.
**	28-aug-97 (nanpr01)
**	    back out the change #431377.
*/
DB_STATUS
sxc_shutdown(
    SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    i4		err_code = E_SX0000_OK;
    i4		local_err;
    SXF_SCB		*scb;
    ULM_RCB		ulm_rcb;
    CL_ERR_DESC		cl_err;

    for (;;)
    {
	/*
	** If the sxf_force flag has not been set and there are still
	** active sessions in the facility - return an error.
	*/
	if (rcb->sxf_force == 0 &&
	   Sxf_svcb->sxf_scb_list != NULL)
	{
	    err_code = E_SX000B_SESSIONS_ACTIVE;
	    break;
	}

	/*
	** Mark the facility as being off line, then shutdown the
	** auditing system and free all allocated resourses.
	*/
	Sxf_svcb->sxf_svcb_status &= ~(SXF_ACTIVE);

	status = sxac_shutdown(&local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}

        /*
        ** Print stats on shutdown. Do this prior to freeing any control
	** blocks which  may hold the stats, but after closing the
	** last audit log.
        */
        sxc_printstats();

	/* Free all session control blocks */
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	for (scb = Sxf_svcb->sxf_scb_list; 
	    scb != NULL;
	    scb = scb->sxf_next)
	{
	    status = sxc_destroy_scb(scb, &local_err);
	    if (status != E_DB_OK)
	    {
		err_code = local_err;
	        _VOID_ ule_format(err_code, NULL, 
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    }
	    Sxf_svcb->sxf_active_ses--;
	}
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	/*
	** Release the locklist
	*/
	if (LKrelease(LK_ALL, Sxf_svcb->sxf_lock_list, 
		NULL, NULL, NULL, &cl_err) != OK)
	{
	    err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(err_code, &cl_err, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}

	/* release the semaphores before freeing the memory */
        CSr_semaphore(&Sxf_svcb->sxf_svcb_sem);
        CSr_semaphore(&Sxf_svcb->sxf_db_sem);
	if ( Sxf_svcb->sxf_aud_state )
	    CSr_semaphore(&Sxf_svcb->sxf_aud_state->sxf_aud_sem);

	/* Free the facility memory pool */
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_poolid = Sxf_svcb->sxf_pool_id;
	status = ulm_shutdown(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    err_code = E_SX106B_MEMORY_ERROR;
	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(err_code, NULL, 
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}

	Sxf_svcb = NULL;
	break;
    }

    /* Handle any errors and return to the caller */
    if (err_code != E_SX0000_OK)
    {
	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX000C_BAD_SHUTDOWN;

	rcb->sxf_error.err_code = err_code;
	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }
    return (status);
}
/*
** Name: sxc_printstats. 
**
** Description:
**	Prints various SXF statistics on server shutdown (and potentially
**	other places as well)
**
** History:
**	14-dec-1992 (robf)
**		Initial creation.
**	30-mar-94 (robf)
**              Add shared memory stats.
*/
VOID
sxc_printstats(void)
{
    SXF_STATS *s;
    i4   write_part;

    TRdisplay("%31*- SXF Statistics %31*-\n");
    if (!Sxf_svcb || !Sxf_svcb->sxf_stats)
    {
	TRdisplay("\tStatistics not available (no control block).\n");
	return;
    }

    s=Sxf_svcb->sxf_stats;
    TRdisplay("General Statistics:\n");
    TRdisplay("-------------------\n");
    TRdisplay("MEMORY:\n");
    TRdisplay("     Maximum size of memory pool: %8d\n", s->mem_at_startup);
    TRdisplay("     Highwater memory used (est): %8d\n", 
			s->mem_at_startup-s->mem_low_avail);
    TRdisplay("     Available memory in pool   : %8d\n", Sxf_svcb->sxf_pool_sz);
    TRdisplay("\n");
    TRdisplay("Audit Operations:\n");
    TRdisplay("-----------------\n");
    TRdisplay("GENERAL:\n");
    TRdisplay("     Audit Log switches  : %8d\n",s->logswitch_count);
    TRdisplay("     Audit Thread Wakeups: %8d\n",s->audit_wakeup);
    TRdisplay("     Audit Writer Thread Wakeups: %8d\n",s->audit_writer_wakeup);
    TRdisplay("\n");
    TRdisplay("FILE:\n");
    TRdisplay("     CREATE : %8d\n", s->create_count);
    TRdisplay("     OPEN   : for Read: %8d, for Write: %8d, Total: %8d\n", 
			s->open_read,s->open_write,
			s->open_read+s->open_write);
    TRdisplay("     CLOSE  : %8d\n", s->close_count);
    TRdisplay("     EXTEND : %8d\n", s->extend_count);
    TRdisplay("PAGE:\n");
    TRdisplay("     READS  : %8d direct.\n",s->read_direct);
    TRdisplay("     WRITES : %8d direct, %8d due to page full.\n",
		s->write_direct,s->write_full);
    TRdisplay("     FLUSHES: %8d total,  %8d active, %8d piggybacked.\n",
		s->flush_count, s->flush_count-s->flush_empty,s->flush_predone);
    TRdisplay("              %8d flushes were empty so ignored.\n",
		s->flush_empty);
    TRdisplay("RECORD:\n");
    TRdisplay("     READS  : %8d buffered.\n",s->read_buff);
    TRdisplay("     WRITES : %8d buffered, %8d causing page full.\n",
		s->write_buff,s->write_full);
    TRdisplay("SHARED QUEUE:\n");
    TRdisplay("      WRITES: %8d total, %8d found queue full.\n",
					s->write_queue,
					s->flush_qfull);
    TRdisplay("     FLUSHES: %8d total, %8d active, %8d empty so ignored.\n",
				s->flush_queue,
				s->flush_queue-s->flush_qempty,
				s->flush_qempty);
    TRdisplay("   FLUSH ALL: %8d\n", s->flush_qall);
    TRdisplay("\n");
}
