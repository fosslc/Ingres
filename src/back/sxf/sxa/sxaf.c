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
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
 
/*
** Name: SXAC.C -  file control routines for the SXF auditing system
**
** Description:
**      This file contains the file control routines used by the SXF
**      auditing system, SXA.
**
**          sxaf_open	    - open an audit file for reading.
**          sxaf_close	    - close an audit file opened by sxaf_open
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation.
**	10-dec-1992 (markg)
**	    Fix error handling bug in sxaf_open.
**	10-feb-1993 (markg)
**	    Removed unnecessary include of ulm.h.
**	10-may-1993 (markg)
**	    Updated some comments.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
** Name: SXAF_OPEN - open an audit file
**
** Description:
**	This routine opens an audit file for reading and returns an audit 
**	file access identifier to the caller. This access identifier 
**	must be specified for all subsequent access requests for this 
**	audit file.
**
** 	Overview of algorithm:-
**
**	Locate the callers session control block.
**	Allocate and initialize a SXF_RSCB.
**	Call the SXAP routine to perform low level file open.
**	Link SXF_RCB onto list of known RCBs.
**
** Inputs:
**	rcb.filename		The name of the file to open, may be NULL
**				for the current audit file.
**	
** Outputs:
**	rcb.sxf_access_id	Access identifier, this must be specified by
**				the caller for all subsequent access to 
**				this file.
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation.
**	10-dec-1992 (markg)
**	    Fix error handling bug which caused zero error values to be
**	    returned to the caller when an error had occured.
*/
DB_STATUS
sxaf_open(SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    i4		err_code = E_SX0000_OK;
    i4		local_err;
    SXF_SCB		*scb;
    SXF_RSCB		*rscb;
    i4		size;
    i4		sxf_status = Sxf_svcb->sxf_svcb_status;
    bool		rscb_built = FALSE;

    rcb->sxf_error.err_code = E_SX0000_OK;
    rcb->sxf_access_id = NULL;

    for (;;)
    {
	if ((sxf_status & (SXF_C2_SECURE | SXF_B1_SECURE)) == 0)
	{
	    err_code = E_SX0010_SXA_NOT_ENABLED;
	    break;
	}

	/* Go and get the SCB for this user */
	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code >E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* Build a RSCB */
	status = sxau_build_rscb(&rscb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code >E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	rscb->sxf_scb_ptr = scb;
	rscb->sxf_rscb_status |= SXRSCB_READ;
	rscb_built = TRUE;

	/* Call SXAP to perform the actual file open */
	status = (*Sxap_vector->sxap_open)(
		rcb->sxf_filename, SXF_READ, rscb, &size, (PTR) NULL, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code >E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	rscb->sxf_rscb_status |= SXRSCB_OPENED;

	/* Put the structure onto the list of RSCBs */
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	if (Sxf_svcb->sxf_rscb_list != NULL)
	    Sxf_svcb->sxf_rscb_list->sxf_prev = rscb;
	rscb->sxf_next = Sxf_svcb->sxf_rscb_list;
	Sxf_svcb->sxf_rscb_list = rscb;
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	rcb->sxf_access_id = (PTR) rscb;
	rcb->sxf_file_size = size;
	 
	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	if (rscb_built)
	    _VOID_ sxau_destroy_rscb(rscb, &local_err);

	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX100D_SXA_OPEN;

	rcb->sxf_error.err_code = err_code;
	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}

/*
** Name: SXAF_CLOSE - close an audit file
**
** Description:
**	This routine closes an audit file that has previously been opened 
**	by a call to sxaf_open. Once the file has been closed successfuly
**	all resourses related to the file are freed.
**
** 	Overview of algorithm:
**
**	Validate the access identifier
**	Unlink SXF_RSCB from list of known RSCBs.
**	Call SXAP routine to perform low level file close.
**	Destroy the RSCB.
**
** Inputs:
**	rcb.sxf_access_id	The access identifier for the audit file to
**				close. This should have been returned by
**				a call to sxaf_open.
**
** Outputs:
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	11-aug-92 (markg)
**	    Initial creation.
*/	    
DB_STATUS
sxaf_close( 
    SXF_RCB	*rcb)
{
    DB_STATUS	status = E_DB_OK;
    i4	err_code = E_SX0000_OK;
    i4	local_err;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_RSCB	*sxf_rscb;
    SXF_RSCB	*rscb = (SXF_RSCB *) rcb->sxf_access_id;
    bool	rscb_found = FALSE;

    rcb->sxf_error.err_code = E_SX0000_OK;

    for (;;)
    {
	if ((sxf_status & (SXF_C2_SECURE | SXF_B1_SECURE)) == 0)
	{
	    err_code = E_SX0010_SXA_NOT_ENABLED;
	    break;
	}

	/* Check that the RSCB is on the active list */
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	for (sxf_rscb = Sxf_svcb->sxf_rscb_list;
	    sxf_rscb != NULL;
	    sxf_rscb = sxf_rscb->sxf_next)
	{
	    if (sxf_rscb == rscb)
	    {
		rscb_found = TRUE;
		break;
	    }
	}
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);
	if (rscb_found == FALSE)
	{
	    err_code = E_SX0011_INVALID_ACCESS_ID;
	    break;
	}

	/* 
	** Remove the RSCB from the list and call the 
	** low level SXAP file close routine, then 
	** destroy the RSCB.
	*/
	CSp_semaphore(TRUE, &Sxf_svcb->sxf_svcb_sem);
	if (rscb->sxf_prev != NULL)
	    rscb->sxf_prev->sxf_next = rscb->sxf_next;
	else
	    Sxf_svcb->sxf_rscb_list = rscb->sxf_next;
	if (rscb->sxf_next != NULL)
	    rscb->sxf_next->sxf_prev = rscb->sxf_prev;
	CSv_semaphore(&Sxf_svcb->sxf_svcb_sem);

	status = (*Sxap_vector->sxap_close)(rscb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(local_err, NULL, ULE_LOG, 
		   NULL, NULL, 0L, NULL, &local_err, 0);
	}

	status = sxau_destroy_rscb(rscb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(local_err, NULL, ULE_LOG, 
		   NULL, NULL, 0L, NULL, &local_err, 0);
	}

    	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX100E_SXA_CLOSE;

	rcb->sxf_error.err_code = err_code;

	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }

    return (status);
}
