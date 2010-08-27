/*
** Copyright (c) 1990, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <ulf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dm1b.h>
#include    <dm1cx.h>
#include    <dm1c.h>
#include    <dm1p.h>
#include    <dmve.h>
#include    <dm0p.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmd.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVEUFMAP.C - The recovery of a Add Fmap operation.
**
**  Description:
**	This file contains the routines for the recovery of an Fmap Update
**	operation associated with a Add FMAP operation (see dmvefmap.c)
**      The new FMAP, logged in dm0l_fmap, had to be added to an existing
**      fmap, not the then current last fmap whose changes are logged
**      in dm0l_extend().
**
**          dmve_ufmap - The recovery of an Add Fmap operation.
**          dmve_reufmap - The Redo Recovery of an Add Fmap operation.
**          dmve_unufmap - The Undo Recovery of an Add Fmap operation.
**
**  History:
**	25-Feb-2009 (hanal04) Bug 121652
**          Created.
**	23-Oct-2009 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.  Dereference page-type once.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*
** Forward Declarations.
*/
static DB_STATUS dmv_reufmap(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fmappinfo,
			DM0L_FMAP	    *log_rec);

static DB_STATUS dmv_unufmap(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fmappinfo,
			DM0L_FMAP	    *log_rec,
			DMPP_ACC_PLV	    *loc_plv);


/*{
** Name: dmve_ufmap - The recovery of an Fmap Update operation.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the fmap operation.
**	    .dmve_action	    Should be DMVE_DO, DMVE_REDO, or DMVE_UNDO
**	    .dmve_dcb_ptr	    Pointer to DCB.
**	    .dmve_tran_id	    The physical transaction id.
**	    .dmve_lk_id		    The transaction lock list id.
**	    .dmve_log_id	    The logging system database id.
**	    .dmve_db_lockmode	    The lockmode of the database. Should be 
**				    DM2T_X or DM2T_S.
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
**	25-Feb-2009 (hanal04) Bug 121652
**          Created.
*/
DB_STATUS
dmve_ufmap(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_FMAP		*log_rec = (DM0L_FMAP *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->fmap_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DM1P_FMAP		*fmap = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DMPP_ACC_PLV	*loc_plv;
    LK_LKID		lockid;
    LK_LKID             fhdr_lockid;
    i4			lock_action;
    i4			grant_mode;
    i4			recovery_action;
    i4			error;
    i4			loc_error;
    i4  		fmap_page_recover;
    i4                  physical_fhdr_page_lock = FALSE;
    i4			fix_option = 0;
    i4			page_type = log_rec->fmap_pg_type;
    DB_ERROR		local_dberr;
    DMP_PINFO		*fmappinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    MEfill(sizeof(LK_LKID), 0, &lockid);
    MEfill(sizeof(LK_LKID), 0, &fhdr_lockid);

    for (;;)
    {
	/*
	** Consistency Check:  check for illegal log records.
	*/
	if ((log_rec->fmap_header.type != DM0LUFMAP) ||
	    (log_rec->fmap_header.length != sizeof(DM0L_UFMAP)))
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	/*
	** Get handle to a tableio control block with which to read
	** and write pages needed during recovery.
	**
	** Warning return indicates that no tableio control block was
	** built because no recovery is needed on any of the locations 
	** described in this log record.
	*/
	status = dmve_fix_tabio(dmve, &log_rec->fmap_tblid, &tbio);
	if (DB_FAILURE_MACRO(status))
	    break;
	if (status == E_DB_WARN && 
		dmve->dmve_error.err_code == W_DM9660_DMVE_TABLE_OFFLINE)
	{
	    CLRDBERR(&dmve->dmve_error);
	    return (E_DB_OK);
	}

	/*
	** Get page accessors for page recovery actions.
	*/
	dm1c_get_plv(page_type, &loc_plv);

	/*
	** Check recovery requirements for this operation.  If partial
	** recovery is in use, then we may not need to recover all
	** the pages touched by the original update.
	*/
	fmap_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->fmap_fmap_cnf_loc_id);

	/*
	** Get required Table/Page locks before we can start the updates.
	**
	** FHDR pages are locked using temporary physical locks.
	** Unlike many other recovery operations, it is possible that
	** this page lock request may block temporarily since the page
	** lock is not necessarily already held by the current transaction.
	** FMAP pages are not locked and are protected by the FHDR page lock.
	** (which means that we must lock the FHDR even if the FHDR is not
	** being recovered).
	**
	** Note that if the database is locked exclusively, or if an X table
	** lock is granted then no page lock is requried.
	*/
	if ((dcb->dcb_status & DCB_S_EXCLUSIVE) == 0)
	{
	    /*
	    ** Request IX lock in preparation of requesting an X page lock
	    ** below.  If the transaction already holds an exclusive table
	    ** lock, then an X lock will be granted.  In this case we can
	    ** bypass the page lock request.
	    */
	    status = dm2t_lock_table(dcb, &log_rec->fmap_tblid, DM2T_IX,
		dmve->dmve_lk_id, (i4)0, &grant_mode, &lockid, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;

	    if (grant_mode != DM2T_X)
	    {               
		/*
		** Page lock required.
		*/
		lock_action = LK_PHYSICAL;
	
		/*
		** Lock the FHDR page.  Use a physical lock.
		** We must request this lock even if we are only
		** processing the FMAP page because the FMAP is
		** implicitly protected by the FHDR lock.
	 	*/
		if (fmap_page_recover)
		{
		    status = dm0p_lock_page(dmve->dmve_lk_id, dcb, 
			&log_rec->fmap_tblid, log_rec->fmap_fhdr_pageno,
			LK_PAGE, LK_X, LK_PHYSICAL, (i4)0, tbio->tbio_relid,
			tbio->tbio_relowner, &dmve->dmve_tran_id,
			&fhdr_lockid, (i4 *)0, (LK_VALUE *)0, &dmve->dmve_error);
		    if (status != E_DB_OK)
			break;

		    physical_fhdr_page_lock = TRUE;
		}
	    }
            else
               fix_option |= DM0P_TABLE_LOCKED_X;
	}

	/*
	** Fix the pages we need to recover in cache for write.
	*/
	if (fmap_page_recover)
	{
	    status = dmve_cachefix_page(dmve, log_lsn,
					tbio, log_rec->fmap_fmap_pageno,
					fix_option, loc_plv,
					&fmappinfo);
	    if (status != E_DB_OK)
		break;
	    fmap = (DM1P_FMAP*)fmappinfo->page;

	    /* Have to set page type if fixing in scratch mode */
	    if (dmve->dmve_action != DMVE_UNDO)
	    {
		dm0p_pagetype(tbio, (DMPP_PAGE *)fmap, 
					    dmve->dmve_log_id, DMPP_FMAP);
	    }
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->fmap_pg_type, log_rec->fmap_page_size,
		(DMPP_PAGE *) fmap, loc_plv, "FMAP"); 
	}

	/*
	** Compare the LSN's on the pages with that of the log record
	** to determine what recovery will be needed.
	**
	**   - During Forward processing, if the page's LSN is greater than
	**     the log record then no recovery is needed.
	**
	**   - During Backward processing, it is an error for a page's LSN
	**     to be less than the log record LSN.
	**
	**   - Currently, during rollforward processing it is unexpected
	**     to find that a recovery operation need not be applied because
	**     of the page's LSN.  This is because rollforward must always
	**     begin from a checkpoint that is previous to any journal record
	**     begin applied.  In the future this requirement may change and
	**     Rollforward will use the same expectations as Redo.
	*/
	switch (dmve->dmve_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    if (fmap && LSN_GTE(
		DM1P_VPT_ADDR_FMAP_PG_LOGADDR_MACRO(page_type, fmap),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(page_type, fmap),
			0, DM1P_VPT_GET_FMAP_PAGE_STAT_MACRO(page_type, fmap),
			0, DM1P_VPT_GET_FMAP_LOGADDR_HI_MACRO(page_type, fmap),
			0, DM1P_VPT_GET_FMAP_LOGADDR_LOW_MACRO(page_type, fmap),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		fmap = NULL;
	    }
	    break;

	case DMVE_UNDO:
	    if (fmap && LSN_LT(
		DM1P_VPT_ADDR_FMAP_PG_LOGADDR_MACRO(page_type, fmap),
		log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(page_type, fmap),
		    0, DM1P_VPT_GET_FMAP_PAGE_STAT_MACRO(page_type, fmap),
		    0, DM1P_VPT_GET_FMAP_LOGADDR_HI_MACRO(page_type, fmap),
		    0, DM1P_VPT_GET_FMAP_LOGADDR_LOW_MACRO(page_type, fmap),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (status != E_DB_OK || !fmap)
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->fmap_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_reufmap(dmve, tbio, fmappinfo, log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unufmap(dmve, tbio, fmappinfo, log_rec, loc_plv);
	    break;
	}

	break;
    }

    if (status != E_DB_OK)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    /*
    ** Unfix and unlock the pages in the opposite order (FMAP, FHDR)
    ** in which they were acquired (FHDR, FMAP).
    ** No need to force them to disk - they
    ** will be tossed out through normal cache protocols if Fast
    ** Commit or at the end of the abort if non Fast Commit.
    */

    if (fmappinfo)
    {
	tmp_status = dm0p_uncache_fix(tbio, DM0P_UNFIX, dmve->dmve_lk_id, 
		dmve->dmve_log_id, &dmve->dmve_tran_id, 
		fmappinfo, &local_dberr);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}
    }

    if (physical_fhdr_page_lock)
    {
	tmp_status = dm0p_unlock_page(dmve->dmve_lk_id, dcb, 
	    &log_rec->fmap_tblid, log_rec->fmap_fhdr_pageno, LK_PAGE,
	    tbio->tbio_relid, &dmve->dmve_tran_id, &fhdr_lockid,
	    (LK_VALUE *)NULL, &local_dberr);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}
    }

    if (tbio)
    {
	tmp_status = dmve_unfix_tabio(dmve, &tbio, 0);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (status != E_DB_OK)
	SETDBERR(&dmve->dmve_error, 0, E_DM9641_DMVE_FMAP);

    return(status);
}

/*{
** Name: dmv_refmap - Redo the add fmap operation.
**
** Description:
**      This function redoes the effects of an Add Fmap Operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fmap				Table's FMAP page.
**	log_rec				Fmap log record.
**	plv				Pointer to page level accessor 
**
** Outputs:
**	error				Pointer to Error return area
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	25-Feb-2009 (hanal04) Bug 121652
**          Created.
*/
static DB_STATUS
dmv_reufmap(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fmappinfo,
DM0L_FMAP	    *log_rec)
{
    LG_LSN		*log_lsn = &log_rec->fmap_header.lsn; 
    i4			*err_code = &dmve->dmve_error.err_code;
    i4	page_type = log_rec->fmap_pg_type;
    i4  fseg = DM1P_FSEG_MACRO(page_type, log_rec->fmap_page_size);
    i4  first_bit   = fseg;
    DM1P_FMAP		*fmap = (DM1P_FMAP*)fmappinfo->page;

    CLRDBERR(&dmve->dmve_error);
    
    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if (fmap == NULL)
	return (E_DB_OK);

    if(log_rec->fmap_last_used / fseg == log_rec->fmap_map_index)
       first_bit = (log_rec->fmap_last_used % fseg) + 1;

    /*
    ** Mark the appropriate ranges of pages used.
    ** Write the FMAP log record's LSN to the updated page.
    */
    dmveMutex(dmve, fmappinfo);

    /*
    ** If some of the new FMAP page numbers fall within the range
    ** of this fmap then update the fmap to show that they are used
    */
    DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(page_type, fmap, first_bit);
    dm1p_fmused(fmap, log_rec->fmap_first_used, log_rec->fmap_last_used,
        page_type, log_rec->fmap_page_size);

    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);
    DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, *log_lsn);
    dmveUnMutex(dmve, fmappinfo);
    
    return(E_DB_OK);
}

/*{
** Name: dmv_unufmap - UNDO of an Fmap Update operation.
**
** Description:
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fmap				Table's FMAP page.
**	log_rec				Fmap log record.
**	plv				Pointer to page level accessor 
**
** Outputs:
**	error				Pointer to Error return area
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	23-Feb-2009 (hanal04) Bug 121652
**          Created.
*/
static DB_STATUS
dmv_unufmap(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fmappinfo,
DM0L_FMAP	    *log_rec,
DMPP_ACC_PLV	    *loc_plv)
{
    LG_LSN		*log_lsn = &log_rec->fmap_header.lsn; 
    LG_LSN		lsn;
    DB_STATUS		status;
    i4		dm0l_flags;
    i4		*err_code = &dmve->dmve_error.err_code;
    i4	page_type = log_rec->fmap_pg_type;
    i4  fseg = DM1P_FSEG_MACRO(page_type, log_rec->fmap_page_size);
    i4  first_bit   = 0;
    DM1P_FMAP		*fmap = (DM1P_FMAP*)fmappinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if (fmap == NULL)
	return (E_DB_OK);

    if(log_rec->fmap_first_used / fseg == log_rec->fmap_map_index)
       first_bit = (log_rec->fmap_first_used % fseg) + 1;

    if (DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(page_type, fmap) != 
        log_rec->fmap_map_index)
    {
        uleFormat(NULL, E_DM9677_DMVE_FMAP_FMAP_STATE, (CL_ERR_DESC *)NULL, 
            ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6, 
	    sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
	    sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
	    sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
	    0, DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(page_type, fmap),
	    0, DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(page_type, fmap),
	    0, log_rec->fmap_map_index);
        dmd_log(1, (PTR) log_rec, 4096);
        uleFormat(NULL, E_DM9642_UNDO_FMAP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    }

    /*
    ** Check direction of recovery operation:
    **
    **     If this is a normal Undo, then we log the CLR for the operation
    **     and write the LSN of this CLR onto the newly updated page (unless
    **     dmve_logging is turned off - in which case the rollback is not
    **     logged and the page lsn is unchanged).
    **
    **     If the record being processed is itself a CLR, then we are REDOing
    **     an update made during rollback processing.  Updates are not relogged
    **     in redo processing and the LSN is moved forward to the LSN value of
    **     of the original update.
    */
    if ((log_rec->fmap_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    dm0l_flags = DM0L_CLR;
	    if (log_rec->fmap_header.flags & DM0L_JOURNAL)
		dm0l_flags |= DM0L_JOURNAL;

	    status = dm0l_ufmap(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->fmap_tblid,
		tabio->tbio_relid, tabio->tbio_relowner,
		log_rec->fmap_pg_type, log_rec->fmap_page_size,
		log_rec->fmap_loc_cnt, 
		log_rec->fmap_fhdr_pageno, log_rec->fmap_fmap_pageno, 
		log_rec->fmap_map_index,
		log_rec->fmap_hw_mark,
		log_rec->fmap_fhdr_cnf_loc_id, log_rec->fmap_fmap_cnf_loc_id,
		log_rec->fmap_first_used, log_rec->fmap_last_used,
		log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
		SETDBERR(&dmve->dmve_error, 0, E_DM9642_UNDO_FMAP);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are processing recovery of an FMAP CLR (redo-ing the undo
	** of an extend) then we don't log a CLR but instead save the LSN
	** of the log record we are processing with which to update the 
	** page lsn's.
	*/
	lsn = *log_lsn;
    }

    /*
    ** Undo the Update FMAP operation.
    */

    /*
    ** FHDR updates will be performed in the associated DM0L_FMAP and/or
    ** DM0L_EXTEND processing.
    **
    ** Mark the appropriate ranges of pages as free.
    */
    dmveMutex(dmve, fmappinfo);

    DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(page_type, fmap, first_bit);
    dm1p_fmfree(fmap, log_rec->fmap_first_used, log_rec->fmap_last_used,
        page_type, log_rec->fmap_page_size);

    DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);
    if (dmve->dmve_logging)
        DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, lsn);

    dmveUnMutex(dmve, fmappinfo);

    /*
    ** Release log file space allocated for logfile forces that may be
    ** required by the buffer manager when unfixing the pages just recovered.
    */
    if (((log_rec->fmap_header.flags & DM0L_CLR) == 0) &&
	((log_rec->fmap_header.flags & DM0L_FASTCOMMIT) == 0) &&
        (dmve->dmve_logging))
    {
        dmve_unreserve_space(dmve, 1);
    }

    return(E_DB_OK);
}
