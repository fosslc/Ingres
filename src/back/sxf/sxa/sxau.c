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
# include    <tm.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <scf.h>
# include    <sxf.h>
# include    <sxfint.h>

/*
** Name: SXAU.C - SXF auditing system utility routines
**
** Description:
**      This file contains the utility routines that are used throughout
**	the SXF auditing system. Routines in this file are:-
**
**      sxau_get_scb      - locate the SCB for the current session
**      sxau_build_rscb   - build a record stream control block
**	sxau_destrol_rscb - destroy a record stream control block
**
** History:
**      20-oct-1992 (markg)
**         Initial creation.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: SXAU_GET_SCB - locate session control block
**
** Description:
**	This routine locates the session control block for the current
**	session and checks that its valid. If the facility is running
**	in multi-user mode we call SCF to get the address of the SCB,
**	if the facility is running in single user mode there will only
**	be one SCB on the SCB list - so return this one.
**
** History:
**	20-oct-1992 (markg)
**	    Initial creation.
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
*/
DB_STATUS
sxau_get_scb(
    SXF_SCB	**scb,
    i4	*err_code)
{
    DB_STATUS	status  = E_DB_OK;
    i4	local_err;
    SCF_CB	scf_cb;
    SCF_SCI	sci_list[1]; 
    CS_SID	sid;
    SXF_SCB	*sxf_scb;
    bool	scb_found = FALSE;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;


    *err_code = E_SX0000_OK;

    for (;;)
    {
	if (sxf_status & SXF_SNGL_USER)
	{
	    *scb = Sxf_svcb->sxf_scb_list;
	    if (*scb == NULL)
	    {
		*err_code = E_SX000D_INVALID_SESSION;
		break;
	    }
	}
	else
	{
	    /* Get the scb information from SCF */
	    scf_cb.scf_length = sizeof(SCF_CB);
	    scf_cb.scf_type = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_SXF_ID;
	    scf_cb.scf_session = DB_NOSESSION;
	    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
	    scf_cb.scf_len_union.scf_ilength = 1;
	    sci_list[0].sci_length = sizeof (PTR);
	    sci_list[0].sci_code = SCI_SCB;
	    sci_list[0].sci_aresult = (char *) scb;
	    sci_list[0].sci_rlength = 0;
	    status = scf_call(SCU_INFORMATION, &scf_cb);
	    if (status != E_DB_OK)
	    {
		*err_code = scf_cb.scf_error.err_code;
		_VOID_ ule_format(*err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		*err_code = E_SX1005_SESSION_INFO;
		_VOID_ ule_format(*err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }

	    /* Check that the SCB is on the list of known SCBs */
	    CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	    for (sxf_scb = Sxf_svcb->sxf_scb_list;
		 sxf_scb != NULL;
		 sxf_scb = sxf_scb->sxf_next)
	    {
		if (sxf_scb == *scb)
		{
		    scb_found = TRUE;
		    break;
		}
	    }
	    CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);
	    if (scb_found == FALSE)
	    {
	        *err_code = E_SX000D_INVALID_SESSION;
	        break;
	    }
	}

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1014_SXA_GET_SCB;

	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}

/*
** Name: sxau_build_rscb - build a record stream control block
**
** Description:
**	This routine is used to allocate and build a record stream
**	control block (SXF_RSCB). Each RSCB is allocated from its own
**	memory stream.
**
** History:
**	20-oct-1992 (markg)
**	    Initial creation.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for assignment to sxf_owner which has changed type
**	    to PTR.
*/
DB_STATUS
sxau_build_rscb(
    SXF_RSCB	**rscb,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    ULM_RCB		ulm_rcb;
    bool		stream_open = FALSE;

    *err_code = E_SX0000_OK;

    for (;;)
    {
	/*
	** Open a new memory stream from the SXF pool
	** and allocate a chunk big enough to hold a
	** RSCB.
	 */
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_poolid = Sxf_svcb->sxf_pool_id;
	ulm_rcb.ulm_blocksize = 0;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;

	/* No other allocations on this stream, so make it private */
	/* Open stream and allocate SXF_RSCB in one call */
	ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb.ulm_psize = sizeof (SXF_RSCB);

	status = ulm_openstream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if (ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;

	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	stream_open = TRUE;

	*rscb = (SXF_RSCB *) ulm_rcb.ulm_pptr;
	(*rscb)->sxf_next = NULL;
	(*rscb)->sxf_prev = NULL;
	(*rscb)->sxf_length = sizeof (SXF_RSCB);
	(*rscb)->sxf_cb_type = SXFRSCB_CB;
	(*rscb)->sxf_owner = (PTR)DB_SXF_ID;
	(*rscb)->sxf_ascii_id = SXRSCB_ASCII_ID;
	(*rscb)->sxf_rscb_status = 0;
	(*rscb)->sxf_scb_ptr = NULL;
	(*rscb)->sxf_rscb_mem = ulm_rcb.ulm_streamid;
	(*rscb)->sxf_sxap = NULL;

	break;
    }

    /* Handle any errors */
    if (*err_code != E_SX0000_OK)
    {
	if (stream_open)
	    _VOID_ ulm_closestream(&ulm_rcb);

	if (*err_code > E_SXF_INTERNAL)
	    *err_code = E_SX1015_SXA_RSCB_BUILD;

	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}

/*
** Name: sxau_destroy_rscb - destroy a record stream control block
**
** Description:
**	This routine is used to deallocate a record stream
**	control block (SXF_RSCB). Each RSCB is allocated from its own
**	memory stream, this routine simply destroys the memory stream.
**
** History:
**	20-oct-1992 (markg)
**	    Initial creation.
**      10-mar-1993 (markg)
**          Fixed problem where errors returned from ulm were not getting
**          handled correctly.
*/
DB_STATUS
sxau_destroy_rscb(
    SXF_RSCB	*rscb,
    i4	*err_code)
{
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    ULM_RCB		ulm_rcb;

    *err_code = E_SX0000_OK;

    ulm_rcb.ulm_facility = DB_SXF_ID;
    ulm_rcb.ulm_streamid_p = &rscb->sxf_rscb_mem;
    ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
    status = ulm_closestream(&ulm_rcb);
    if (status != E_DB_OK)
    {
	_VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL,
		ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	*err_code = E_SX106B_MEMORY_ERROR;
    }
    /* ULM has nullified sxf_rscb_mem */
    return (status);
}
