/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <tr.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmpp.h>
#include    <dm1c.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmve.h>
#include    <dm1b.h>
#include    <dm0l.h>

/**
**
**  Name: DMVECRVF.C - The recovery of a create table verify operation.
**
**  Description:
**	This file contains the routine for the recovery of a create table
**	verify operation.  This log is written to ensure that create table 
**	recovery rebuilt the table as it was originally built.  The log is 
**	processed during ROLLFORWARD recovery only.
**
**	    dmve_crverify  - The main routine
**
**  History:    
**	12-nov-1992 (jnash)
**	    Created for the reduced logging project.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1992 (jnash)
**	    Temp fix: don't verify create index operations.  This is
**	    a problem with the ordering of log records and will be fixed
**	    properly at the next integration.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Verify that ducv_page_size matches tcb_rel.relpgsize.
**	    Corrected a typo in the TRdisplay for relprim.
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	14-aug-1998 (nanpr01)
**	    Fix up the code.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/



/*
** Name: dmve_crverify - The recovery of a create table verify opration.
**
** Description:
**	This function checks the structure of a previously recovered
**	table.  It is used to ensure, to as great a degree as possible,
**	that the table is recovered as it originally built.  The 
**	routine performs logical checks, and cannot run in a partial
**	recovery environment.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The create table operation log record.
**	    .dmve_action	    Should only be DMVE_DO.
**	    .dmve_lg_addr	    The log address of the log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**
** Outputs:
**	dmve_cb
**	    .dmve_error.err_code    The reason for error status.
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	17-nov-92 (jnash)
**          Written for the reduced logging project.
**	06-mar-1996 (stial01 for bryanp)
**	    Verify that ducv_page_size matches tcb_rel.relpgsize.
**	    Corrected a typo in the TRdisplay for relprim.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for indexes, not just != 0.
**	12-Feb-2004 (schka24)
**	    Don't verify partitions either.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Open no-coupon to avoid unnecessary LOB overhead.
*/
DB_STATUS
dmve_crverify(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_CRVERIFY	*log_rec = (DM0L_CRVERIFY *)dmve_cb->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		error;
    i4		loc_error;
    DMP_RCB		*rcb;
    bool		mismatch;
    DB_TAB_TIMESTAMP	timestamp;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);
	
    for (;;)
    {
	if (log_rec->ducv_header.length > sizeof(DM0L_CRVERIFY) || 
	    log_rec->ducv_header.type != DM0LCRVERIFY)
	{
	    error = E_DM9601_DMVE_BAD_PARAMETER;
	    break;
	}

	/* If partial recovery cannot perform verify */
	if ( dmve_partial_recov_check(dmve_cb) ) 
	    return(E_DB_OK);

	mismatch = FALSE;
	switch (dmve->dmve_action)
	{
	case DMVE_DO:
	{
	    /*
	    ** Temp fix: Do not verify the creation of indexes, the dm2t_open()
	    ** will fail because the ii_index entry does not yet exist.
	    ** Note the base table must be close-purged prior to this point.
	    ** Don't verify partitions either, for similar reasons: the
	    ** master isn't open, so this open will fail.  FIXME we could
	    ** arrange to open the master, but verify is not essential.
	    */
	    if (log_rec->ducv_tbl_id.db_tab_index != 0)
		break;

	    status = dm2t_open(dmve->dmve_dcb_ptr, &log_rec->ducv_tbl_id,
		DM2T_X, DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, (i4) 0, (i4) 20,
		0, dmve->dmve_log_id, dmve->dmve_lk_id, 0, 0,
		dmve->dmve_db_lockmode, &dmve->dmve_tran_id, &timestamp,
		&rcb, (DML_SCB *)0, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
#ifdef xDEBUG
		TRdisplay("dmve_crverify: failure in dm2t_open: status: %d, error: %d\n",
		    status, dmve->dmve_error.err_code);
#endif
		break;
	    }

	    if (rcb->rcb_tcb_ptr->tcb_rel.relpages != log_rec->ducv_relpages)
	    {
		TRdisplay(
		    "dmve_crverify: relpages mismatch, log: %d tbl: %d\n",
		    log_rec->ducv_relpages, rcb->rcb_tcb_ptr->tcb_rel.relpages);
		mismatch = TRUE;
	    }

	    if (rcb->rcb_tcb_ptr->tcb_rel.relprim != log_rec->ducv_relprim)
	    {
		TRdisplay(
		    "dmve_crverify: relprim mismatch, log: %d tbl: %d\n",
		    log_rec->ducv_relprim, rcb->rcb_tcb_ptr->tcb_rel.relprim);
		mismatch = TRUE;
	    }

	    if (rcb->rcb_tcb_ptr->tcb_rel.relmain != log_rec->ducv_relmain)
	    {
		TRdisplay(
		    "dmve_crverify: relmain mismatch, log: %d tbl: %d\n",
		    log_rec->ducv_relmain, rcb->rcb_tcb_ptr->tcb_rel.relmain);
		mismatch = TRUE;
	    }

	    if (rcb->rcb_tcb_ptr->tcb_rel.relfhdr != log_rec->ducv_fhdr_pageno)
	    {
		TRdisplay(
		    "dmve_crverify: relfhdr mismatch, log: %d tbl: %d\n",
		    log_rec->ducv_fhdr_pageno, 
		    rcb->rcb_tcb_ptr->tcb_rel.relfhdr);
		mismatch = TRUE;
	    }

	    if (rcb->rcb_tcb_ptr->tcb_rel.relallocation != 
		log_rec->ducv_allocation)
	    {
		TRdisplay(
		    "dmve_crverify: allocation mismatch, log: %d tbl: %d\n",
		    log_rec->ducv_allocation, 
		    rcb->rcb_tcb_ptr->tcb_rel.relallocation);
		mismatch = TRUE;
	    }
	
	    if (rcb->rcb_tcb_ptr->tcb_rel.relpgsize != log_rec->ducv_page_size)
	    {
		TRdisplay(
		    "dmve_crverify: Page size mismatch, log: %d tbl: %d\n",
		    log_rec->ducv_page_size,
		    rcb->rcb_tcb_ptr->tcb_rel.relpgsize);
		mismatch = TRUE;
	    }

	    status = dm2t_close(rcb, 0, &dmve->dmve_error);
	    if (status == E_DB_OK)
		break;
	}

	case DMVE_UNDO:
	case DMVE_REDO:
	    break;
	}

	if ( (status != E_DB_OK) || mismatch ) 
	    break;

	return(E_DB_OK);
    }

    /*
    ** Error handling
    */

    if (rcb)
    {
	status = dm2t_close(rcb, 0, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	}
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9606_DMVE_CREATE); /* XXXX NEW ERROR XX*/
 
    return(E_DB_ERROR);
}
