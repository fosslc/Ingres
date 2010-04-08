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
# include    <ulf.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <ulm.h>

/*
** Name: SXCEND.C - SXF session termination routines
**
** Description:
**	This file contains all of the routines used to terminate a SXF
**	session.
**
**	    sxc_end_session - End a SXF session.
**	    sxc_destroy_scb - Destroy a SXF session control block.
**
** History:
**	11-aug-1992 (markg)
**	    Initial creation.
**	26-oct-1992 (markg)
**	    Updated error handling.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	10-may-1993 (markg)
**	    Updated some comments.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: SXC_END_SESSION - end a SXF session
**
** Description:
**	This routine is used to end a SXF session, it is called at
**	session end time via the sxf_call(SXC_END_SESSION) request. 
**	This request must be made by a session that has previously been
**	registered with SXF via the sxc_bgn_session routine. All resourses
**	allocated by this session will be released.
**
**	Overview of algorithm:-
**
**	Locate the SXF_SCB for this session. 
**	Call sxac_end_session to remove the session from the auditing system.
**	Unlink SXF_SCB from SCB list in the server control block.
**	Destroy the SXF_SCB.
**
** Inputs:
**	rcb
**	    .sxf_scb		Pointer to the session control block.
**
** Outputs:
**	rcb
**	    .sxf_error		error code returned to the caller
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
**	26-oct-1992 (markg)
**	    Updated error handling.
*/
DB_STATUS
sxc_end_session(
    SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    i4		err_code = E_SX0000_OK;
    i4		local_err;
    SXF_SCB		*scb;
    bool		scb_found = FALSE;

    for (;;)
    {
	/* Locate SCB and remove the session from the audit system */
	if (Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER)
	{
	    if (Sxf_svcb->sxf_scb_list == NULL)
	    {
		err_code = E_SX000D_INVALID_SESSION;
		break;
	    }
	    else
	    {
	        scb = Sxf_svcb->sxf_scb_list;
	    }
	}
	else
	{
	    CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	    for (scb = Sxf_svcb->sxf_scb_list;
	        scb != NULL;
	        scb = scb->sxf_next)
	    {
	        if (scb == (SXF_SCB *) rcb->sxf_scb)
	        {
		    scb_found = TRUE;
		    break;
	        }
	    }
	    CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);
	    if (scb_found == FALSE)
	    {
	        err_code = E_SX000D_INVALID_SESSION;
	        break;
	    }
	}

	status = sxac_end_session(scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}

	/* Remove SCB from list of active sessions and destroy it */
        CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	if (scb->sxf_prev != NULL)
	    scb->sxf_prev->sxf_next = scb->sxf_next;
	else
	    Sxf_svcb->sxf_scb_list = scb->sxf_next;
	if (scb->sxf_next != NULL)
	    scb->sxf_next->sxf_prev = scb->sxf_prev;
	Sxf_svcb->sxf_active_ses--;
        CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	status = sxc_destroy_scb(scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}

	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX000E_BAD_SESSION_END;

	rcb->sxf_error.err_code = err_code;
	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}

/*
** Name: SXC_DESTROY_SCB - destroy a SXF session control block
**
** Description:
**	This routine frees all the resourses associated with a session control
**	block (SXF_SCB).
**
**	Overview of algorithm:-
**
**	Release the SCB's lock list.
**	Close the SCB's memory stream.
**
** Inputs:
**	scb		pointer to session control block to be destroyed
**
** Outputs:
**	err_code	
**
** Returns:
**	DB_STATUS
** History:
**	25-sept-92 (markg)
**	    Initial creation.
**	26-oct-1992 (markg)
**	    Updated error handling.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	10-may-1993 (robf)
**	    Added handling for database cb in session.
*/
DB_STATUS
sxc_destroy_scb(
    SXF_SCB	*scb,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    CL_ERR_DESC		cl_err;
    ULM_RCB		ulm_rcb;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	if (LKrelease(LK_ALL, scb->sxf_lock_list, 
		NULL, NULL, NULL, &cl_err) != OK)
	{
	    *err_code = E_SX1008_BAD_LOCK_RELEASE;
	    _VOID_ ule_format(*err_code, &cl_err,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}
	/*
	** Release database cb 
	*/
	sxc_freedbcb(scb, scb->sxf_db_info);

	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_streamid_p = &scb->sxf_scb_mem;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	status = ulm_closestream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    *err_code = E_SX106B_MEMORY_ERROR;
	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}
	/* ULM has nullified sxf_scb_mem */

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1007_BAD_SCB_DESTROY;

	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}
