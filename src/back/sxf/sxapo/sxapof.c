
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
# include    <di.h>
# include    <lo.h>
# include    <st.h>
# include    <sa.h>
# include    <ulf.h>
# include    <ulm.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
# include    "sxapoint.h"

/*
** Name: SXAPFO.C - audit trail control routines for the SXAPO auditing system
**
** Description:
**      This file contains all of the control routines used by the SXF
**      low level auditing system, SXAPO.
**
**          sxapo_open     - open an audit trail for reading or writing
**          sxapo_close    - close an audit trail
**
** History:
**	6-jan-94 (stephenb)
**	    Initial Creation.
**	11-feb-94 (stephenb)
**	    Updated SA calls to current spec.
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(). It's gross overkill to semaphore-protect a
**	    simple statistic.
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
**      06-nov-00 (wansh01)
**          changed if(ulm_rcb.ulm_error.err_code = E_UL0005_NOMEM) in line153
**          from '=' to '==' to compare the error code not set.
*/

/*
** Name: sxapo_open - open an audit trail
**
** Description:
**	This routine is used to open an audit trail that can, in theory, be used
**	for either reading or writing audit records, although reading of 
**	operating system audit records is not currently supported. SXF supports 
**	the ability to have a maximum of one audit trail open for writing at 
**	a time. This routine
**	returns an access identifier (actually a pointer to a SXAPO_RSCB 
**	structure) that must be specified for all other operations requested 
**	on this trail. if no filename is specified the current audit trail
**	will be opened.
**
** 	Overview of algorithm:-
**
**	Validate the parameters passed to the routine.
**	Open a memory stream and allocate and build a SXAPO_RSCB.
**	If filename is NULL get the name of the current audit trail.
**	Call the SAopen routine to perform the physical open call.
**
**
** Inputs:
**	filename		Name of the audit trail to open, if this is NULL
**				the current audit trail will be opened.
**	mode			either SXF_READ of SXF_WRITE (must currently
**				be SXF_WRITE).
**	sxf_rscb		the SXF_RSCB for the audit trail
**
** Outputs:
**	sxf_rscb		Access identifier for audit file. 
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	6-jan-94 (stephenb)
**	    Initial Creation.
**	7-feb-94 (stephenb)
**	    log error E_SX002E_OSAUDIT_NO_READ since we don't do it anywhere
**	    else yet.
**	11-feb-94 (stephenb)
**	    Update SA call to current spec.
*/
DB_STATUS
sxapo_open(
    PTR			filename,
    i4			mode,
    SXF_RSCB		*sxf_rscb,
    i4		*filesize,
    PTR			sptr,
    i4		*err_code)
{
    ULM_RCB		ulm_rcb;
    DB_STATUS		status = E_DB_OK;
    i4		local_err;
    bool		stream_open = FALSE;
    SXAPO_RSCB		*sxapo_rscb;
    STATUS		cl_stat;
    CL_ERR_DESC		cl_err;

    *err_code = E_SX0000_OK;
    for (;;)
    {
	/* validate parameters, write is currently the only valid mode */
	if (sxf_rscb == NULL)
	{
	    *err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}
	else if (mode != SXF_WRITE)
	{
	    *err_code = E_SX002E_OSAUDIT_NO_READ;
	    /*
	    ** Log this here since we don't yet do it anywhere else.
	    */
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, NULL, NULL, 0L,
		NULL, &local_err, 0);
	    break;
	}
	/*
	** Open a memory stream and allocate some memory for the SXAPO_RSCB
	*/
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_poolid = Sxf_svcb->sxf_pool_id;
	ulm_rcb.ulm_blocksize = 0;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	ulm_rcb.ulm_streamid_p = &ulm_rcb.ulm_streamid;
	/* No other allocations on this stream - make it private */
	/* Open stream and allocate SXAPO_RSCB with one call */
	ulm_rcb.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm_rcb.ulm_psize = sizeof (SXAPO_RSCB);
	status = ulm_openstream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    if(ulm_rcb.ulm_error.err_code == E_UL0005_NOMEM)
		*err_code = E_SX1002_NO_MEMORY;
	    else
		*err_code = E_SX106B_MEMORY_ERROR;
	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	stream_open = TRUE;

	sxf_rscb->sxf_sxap = ulm_rcb.ulm_pptr;
	sxapo_rscb = (SXAPO_RSCB *) sxf_rscb->sxf_sxap;
	sxapo_rscb->rs_memory = ulm_rcb.ulm_streamid;
	/*
	** Filename should not be supplied for OS audit trail writes
	*/
	if (filename)
	{
	    *err_code = E_SX101E_INVALID_FILENAME;
	    break;
	}
	else
	{
	    /*
	    ** Auditing must be enabled to use "current" file
	    */
	    if (Sxapo_cb->sxapo_status & SXAPO_STOPAUDIT)
	    {
		_VOID_ ule_format(E_SX10A7_NO_CURRENT_FILE, NULL, ULE_LOG,
		    NULL, NULL, 0L, NULL, &local_err, 0);
		*err_code = E_SX0015_FILE_NOT_FOUND;
		break;
	    }
	}
	/*
	** Open the audit trail with SAopen
	*/
	cl_stat = SAopen(NULL, SA_WRITE, 
		&sxapo_rscb->sxapo_desc, &cl_err);
	if (cl_stat != OK)
	{
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, NULL, NULL,
		0L, NULL, &local_err, 0);
	    break;
	}
	break;
    }
    /*
    ** Handle any errors
    */
    if (*err_code != E_SX0000_OK)
    {
	if (stream_open)
	    _VOID_ ulm_closestream(&ulm_rcb);
	if (*err_code > E_SXF_INTERNAL)
	{
	    _VOID_ ule_format(E_SX10B6_OSAUDIT_OPEN_FAILED, NULL, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
	    *err_code = E_SX1023_SXAP_OPEN;
	}
	if (status == E_DB_OK)
	    status = E_DB_ERROR;
    }
    else
    {
	/*
	** Incriment stats counter
	*/
	Sxf_svcb->sxf_stats->open_write++;
    }

    return (status);
}

/*
** Name: sxapo_close - close an audit trail
**
** Description:
**	This routine closes an audit trail that has previously been opened 
**	by a call to sxap_open.
**
** 	Overview of algorithm:-
**
**	Call SAclose.
**	close the memory stream ascociated with this audit trail.
**
** Inputs:
**	sxf_rscb		The SXF_RSCB structure associated with this
**				audit trail
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	6-jan-94 (stephenb)
**	    Initial creation.
**	11-feb-94 (stephenb)
**	    Update SA call to current spec.
*/
sxapo_close(
    SXF_RSCB	*rscb,
    i4	*err_code)
{
    SXAPO_RSCB		*sxapo_rscb = (SXAPO_RSCB *)rscb->sxf_sxap;
    STATUS		cl_stat;
    CL_ERR_DESC		cl_err;
    DB_STATUS		status;
    ULM_RCB		ulm_rcb;
    i4		local_err;

    *err_code = E_SX0000_OK;
    for(;;)
    {
	if (!sxapo_rscb)
	{
	    *err_code = E_SX0004_BAD_PARAMETER;
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, NULL, NULL,
		0L, NULL, &local_err, 0);
	    break;
	}
	/*
	** incriment stats counter
	*/
	Sxf_svcb->sxf_stats->close_count++;

	cl_stat = SAclose(&sxapo_rscb->sxapo_desc, &cl_err);
	if (cl_stat != OK)
	{
	    *err_code = E_SX1018_FILE_ACCESS_ERROR;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, NULL, NULL,
		0L, NULL, &local_err, 0);
	}
	/*
	** Close memory stream
	*/
	ulm_rcb.ulm_facility = DB_SXF_ID;
	ulm_rcb.ulm_streamid_p = &sxapo_rscb->rs_memory;
	ulm_rcb.ulm_memleft = &Sxf_svcb->sxf_pool_sz;
	status = ulm_closestream(&ulm_rcb);
	if (status != E_DB_OK)
	{
	    *err_code = E_SX106B_MEMORY_ERROR;
	    _VOID_ ule_format(ulm_rcb.ulm_error.err_code, NULL, ULE_LOG,
		NULL, NULL, 0L, NULL, &local_err, 0);
	    _VOID_ ule_format(*err_code, NULL, ULE_LOG, NULL, NULL,
		0L, NULL, &local_err, 0);
	}
	/* ULM has nullified rs_memory */
	rscb->sxf_sxap = NULL;
	break;
    }
    if (*err_code != E_SX0000_OK)
    {
	*err_code = E_SX1026_SXAP_CLOSE;
	status = E_DB_ERROR;
    }
    return (status);
}
