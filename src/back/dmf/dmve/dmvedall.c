/*
** Copyright (c) 1990, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <tr.h>
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
#include    <dm1p.h>
#include    <dm1cx.h>
#include    <dm1c.h>
#include    <dmve.h>
#include    <dm0p.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVEDALL.C - The recovery of a page deallocate operation.
**
**  Description:
**	This file contains the routines for the recovery of an deallocate
**	operation.
**
**          dmve_dealloc - The recovery of a page deallocate operation.
**          dmve_redealloc - The Redo Recovery of a page deallocate operation.
**          dmve_undealloc - The Undo Recovery of a page deallocate operation.
**
**  History:
**	02-apr-1990 (Derek)
**          Created.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk)
**	    Rewritten for Reduced Logging Project.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (rogerk)
**	    Added error messages.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
** 	26-jul-1993 (jnash)
**	    Fix bug where LSN check of free page erroneously updated 'fmap'. 
** 	26-jul-1993 (rogerk)
**	    Fix TRdisplays to correctly identify this module.
**	    Added tracing of page information during recovery.
**	    Include respective CL headers for all cl calls.
**	    Fix integration problem between jnash and bryanp changes above.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the undo recovery.
**	19-oct-1993 (wolf)
**	    Fix compile error from yesterday's integration
**	 3-jan-1994 (rogerk)
**	    Update page_tran_id when redo a deallocate action.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size as the page_size argument to dmpp_format.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages.
**      22-nov-1996 (dilma04)
**          Row Locking Project:
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_ufx_tabio_cb() calls.
**	23-Jun-1998 (jenjo02)
**	    Added lock_value parm to dm0p_lock_page() and dm0p_unlock_page().
**	    Utilize lock_id value in both of those functions.
**	    Consolidate redundant page unfix/unlock code.
**	14-aug-1998 (nanpr01)
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
**      04-Feb-1999 (hanal04)
**          dmv_undealloc will format a page. If not dmve_logging restore
**          the page's original lsn after formatting the page to prevent
**          lsn mismatch errors during rollforwarddb -e time. b94849.
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_dealloc, eliminating need for dmve_name_unpack().
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Pass pg type to dmve_trace_page_info (Variable Page Type SIR 102955)
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      19-Jun-2002 (horda03) Bug 108074
**          Prevent FORCE_ABORT transaction filling TX log
**          file by not flushing SCONCUR pages unless we
**          really need to (i.e. the table isn't locked
**          exclusively).
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	06-Mar-2007 (jonj)
**	    Replace dm0p_cachefix_page() with dmve_cachefix_page()
**	    for Cluster REDO recovery.
**      14-jan-2008 (stial01)
**          Call dmve_unfix_tabio instead of dm2t_ufx_tabio_cb
**	20-Feb-2008 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.  Dereference page-type once.
**	20-may-2008 (joea)
**	    Add relowner parameter to dm0p_lock_page.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs, move consistency check to dmveutil
**/

/*
** Forward Declarations.
*/

static DB_STATUS dmv_redealloc(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fhdrpinfo,
			DMP_PINFO	    *fmappinfo,
			DMP_PINFO	    *freepinfo,
			DM0L_DEALLOC	    *log_rec,
			DMPP_ACC_PLV	    *loc_plv);

static DB_STATUS dmv_undealloc(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fhdrpinfo,
			DMP_PINFO	    *fmappinfo,
			DMP_PINFO	    *freepinfo,
			DM0L_DEALLOC	    *log_rec,
			DMPP_ACC_PLV	    *loc_plv);


/*{
** Name: dmve_dealloc - The recovery of a Page Deallocate operation.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the deallocate operation.
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
**	26-oct-1992 (jnash & rogerk)
**	    Reduced Logging Project: Completely rewritten with new 6.5 
**	    recovery protocols.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
** 	26-jul-1993 (jnash)
**	    Fix bug where LSN check of free page erroneously updated 'fmap'. 
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass page size to dm1c_getaccessors().
**      22-nov-1996 (dilma04)
**          Row Locking Project:
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      11-sep-1997 (thaju02) bug#85156 - on recovery lock extension
**          table with DM2T_S.  Attempting to lock table with DM2T_IX
**          causes deadlock, dbms resource deadlock error during recovery,
**          pass abort, rcp resource deadlock error during recovery,
**          rcp marks database inconsistent.
**      19-Jun-2002 (horda03) Bug 108074
**          If the table is locked exclusively, then indicate that SCONCUR
**          pages don't need to be flushed immediately.
**      23-feb-2004 (thaju02) Bug 111470 INGSRV2635
**          For rollforwarddb -b option, do not compare the LSN's on the
**          page to that of the log record.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
*/
DB_STATUS
dmve_dealloc(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_DEALLOC	*log_rec = (DM0L_DEALLOC *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->dall_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DM1P_FHDR		*fhdr = NULL;
    DM1P_FMAP		*fmap = NULL;
    DMPP_PAGE		*free = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DMPP_ACC_PLV	*loc_plv;
    LK_LKID		lockid;
    LK_LKID		fhdr_lockid;
    LK_LKID		page_lockid;
    i4		lock_action;
    i4		grant_mode;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4  	fhdr_page_recover;
    i4  	fmap_page_recover;
    i4  	free_page_recover;
    i4  	physical_fhdr_page_lock = FALSE;
    i4  	physical_free_page_lock = FALSE;
    i4		fix_action = 0;
    i4		page_type = log_rec->dall_pg_type;
    DB_ERROR		local_dberr;
    DMP_PINFO		*fhdrpinfo = NULL;
    DMP_PINFO		*fmappinfo = NULL;
    DMP_PINFO		*freepinfo = NULL;

    CLRDBERR(&dmve->dmve_error);

    MEfill(sizeof(LK_LKID), 0, &lockid);
    MEfill(sizeof(LK_LKID), 0, &fhdr_lockid);
    MEfill(sizeof(LK_LKID), 0, &page_lockid);

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->dall_header.type != DM0LDEALLOC)
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
	status = dmve_fix_tabio(dmve, &log_rec->dall_tblid, &tbio);
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
	fhdr_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->dall_fhdr_cnf_loc_id);
	fmap_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->dall_fmap_cnf_loc_id);
	free_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->dall_free_cnf_loc_id);

	/*
	** Deallocate actions do not always update the FHDR page - only when
	** the operation changes the fhdr's free hint (a page is freed to
	** an fmap that previously had no free pages).
	** 
	** If the recovery actions need not change the state of these
	** values, then we can skip recovery of the FHDR.
	**
	**   - During REDO, we need only update the FHDR if it was updated
	**     during the original action.
	**   - During UNDO, we may need to turn off the free hint even if it
	**     was not turned on by this allocate action.  We can never
	**     bypass recovery of the fhdr here (even though it may turn
	**     out that we won't end up updating the FHDR anyway).
	*/
	if ((dmve->dmve_action != DMVE_UNDO) && ( ! log_rec->dall_fhdr_hint))
	    fhdr_page_recover = FALSE;


	/*
	** Get required Table/Page locks before we can start the updates.
	**
	** FHDR and system catalog pages are locked using temporary physical
	** locks.  Unlike many other recovery operations, it is possible that
	** these page lock requests may block temporarily since the page
	** locks are not necessarily already held by the current transaction.
	** FMAP pages are not locked and are protected by the FHDR page lock
	** (which means that we must lock the FHDR even if the FHDR is not
	** being recovered).
	**
	** Note that if the database is locked exclusively, or if an X table
	** lock is granted then no page locks are requried.
	*/
	if ((dcb->dcb_status & DCB_S_EXCLUSIVE) == 0)
	{
	    /*
	    ** Request IX lock in preparation of requesting an X page lock
	    ** below.  If the transaction already holds an exclusive table
	    ** lock, then an X lock will be granted.  In this case we can
	    ** bypass the page lock request.
	    */
	    status = dm2t_lock_table(dcb, &log_rec->dall_tblid, DM2T_IX,
		dmve->dmve_lk_id, (i4)0, &grant_mode, &lockid, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;

	    if (grant_mode != DM2T_X)
	    {
		/*
		** Page locks required.
		*/
		lock_action = LK_LOGICAL;
		if (log_rec->dall_header.flags & DM0L_PHYS_LOCK)
		    lock_action = LK_PHYSICAL;

		/*
		** Lock the FHDR page.  Use a physical lock.
		** We must request this lock even if we are only
		** processing the FMAP page because the FMAP is
		** implicitly protected by the FHDR lock.
	 	*/
		if (fhdr_page_recover || fmap_page_recover)
		{
		    status = dm0p_lock_page(dmve->dmve_lk_id, dcb, 
			&log_rec->dall_tblid, log_rec->dall_fhdr_pageno,
			LK_PAGE, LK_X, LK_PHYSICAL, (i4)0,
			tbio->tbio_relid, tbio->tbio_relowner,
			&dmve->dmve_tran_id, &fhdr_lockid, (i4 *)0,
			(LK_VALUE *)0, &dmve->dmve_error);
		    if (status != E_DB_OK)
			break;

		    physical_fhdr_page_lock = TRUE;
		}
	
		/*
		** Lock the free page.
		*/
		if (free_page_recover)
		{
		    status = dm0p_lock_page(dmve->dmve_lk_id, dcb, 
			&log_rec->dall_tblid, log_rec->dall_free_pageno,
			LK_PAGE, LK_X, lock_action, (i4)0,
                        tbio->tbio_relid, tbio->tbio_relowner,
			&dmve->dmve_tran_id, &page_lockid, (i4 *)0,
			(LK_VALUE *)0, &dmve->dmve_error);
		    if (status != E_DB_OK)
			break;

		    if (lock_action == LK_PHYSICAL)
			physical_free_page_lock = TRUE;
		}
	    }
            else
               fix_action |= DM0P_TABLE_LOCKED_X;
	}

	/*
	** Fix the pages we need to recover in cache for write.
	*/
	if (fhdr_page_recover)
	{
	    status = dmve_cachefix_page(dmve, log_lsn,
					tbio, log_rec->dall_fhdr_pageno,
					fix_action, loc_plv,
					&fhdrpinfo);
	    if (status != E_DB_OK)
		break;
	    fhdr = (DM1P_FHDR*)fhdrpinfo->page;
	}

	if (fmap_page_recover)
	{
	    status = dmve_cachefix_page(dmve, log_lsn,
					tbio, log_rec->dall_fmap_pageno,
					fix_action, loc_plv,
					&fmappinfo);
	    if (status != E_DB_OK)
		break;
	    fmap = (DM1P_FMAP*)fmappinfo->page;
	}

	if (free_page_recover)
	{
	    status = dmve_cachefix_page(dmve, log_lsn,
					tbio, log_rec->dall_free_pageno,
					fix_action, loc_plv,
					&freepinfo);
	    if (status != E_DB_OK)
		break;
	    free = freepinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->dall_pg_type, log_rec->dall_page_size,
		(DMPP_PAGE *) fhdr, loc_plv, "FHDR");
	    dmve_trace_page_info(log_rec->dall_pg_type, log_rec->dall_page_size,
		(DMPP_PAGE *) fmap, loc_plv, "FMAP");
	    dmve_trace_page_info(log_rec->dall_pg_type, log_rec->dall_page_size,
		free, loc_plv, "FREE");
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
	**
	** Note that during UNDO, we may update the fhdr's free hint for
	** the affected fmap page even though we did not turn off that free
	** hint as part of the logged operation.  This is an effect of not
	** holding locks on fhdr pages for transaction duration.  In cases
	** when the FHDR was not updated by the original allocate, we cannot
	** make any assumptions about the fhdr's LSN.
	*/
	switch (dmve->dmve_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
            if (fhdr && LSN_GTE(
		DM1P_VPT_ADDR_FHDR_PG_LOGADDR_MACRO(page_type, fhdr),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(page_type, fhdr),
			0, DM1P_VPT_GET_FHDR_PAGE_STAT_MACRO(page_type, fhdr),
			0, DM1P_VPT_GET_FHDR_LOGADDR_HI_MACRO(page_type, fhdr),
			0, DM1P_VPT_GET_FHDR_LOGADDR_LOW_MACRO(page_type, fhdr),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		fhdr = NULL;
	    }

            if (fmap && LSN_GTE(
		DM1P_VPT_ADDR_FMAP_PG_LOGADDR_MACRO(page_type, fmap),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(page_type, fmap),
			0, DM1P_VPT_GET_FMAP_PAGE_STAT_MACRO(page_type, fmap),
			0, DM1P_VPT_GET_FMAP_LOGADDR_HI_MACRO(page_type, fmap),
			0, DM1P_VPT_GET_FMAP_LOGADDR_LOW_MACRO(page_type, fmap),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		fmap = NULL;
	    }

            if (free && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, free),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, free),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, free),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, free),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, free),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		free = NULL;
	    }
	    break;

	case DMVE_UNDO:
	    if (fhdr && (log_rec->dall_fhdr_hint) &&
		(LSN_LT(DM1P_VPT_ADDR_FHDR_PG_LOGADDR_MACRO(page_type, fhdr),
			log_lsn)))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(page_type, fhdr),
		    0, DM1P_VPT_GET_FHDR_PAGE_STAT_MACRO(page_type, fhdr),
		    0, DM1P_VPT_GET_FHDR_LOGADDR_HI_MACRO(page_type, fhdr),
		    0, DM1P_VPT_GET_FHDR_LOGADDR_LOW_MACRO(page_type, fhdr),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
            if (fmap && LSN_LT(
		DM1P_VPT_ADDR_FMAP_PG_LOGADDR_MACRO(page_type, fmap),
		log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(page_type, fmap),
		    0, DM1P_VPT_GET_FMAP_PAGE_STAT_MACRO(page_type, fmap),
		    0, DM1P_VPT_GET_FMAP_LOGADDR_HI_MACRO(page_type, fmap),
		    0, DM1P_VPT_GET_FMAP_LOGADDR_LOW_MACRO(page_type, fmap),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
            if (free && LSN_LT(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, free),
		log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, free),
		    0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, free),
		    0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, free),
		    0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, free),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (status != E_DB_OK || (!fhdr && !fmap && !free))
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->dall_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_redealloc(dmve, tbio,
				    (fhdr) ? fhdrpinfo : NULL, 
				    (fmap) ? fmappinfo : NULL, 
				    (free) ? freepinfo : NULL, 
				    log_rec, loc_plv);
	    break;

	case DMVE_UNDO:
	    status = dmv_undealloc(dmve, tbio,
				    (fhdr) ? fhdrpinfo : NULL, 
				    (fmap) ? fmappinfo : NULL, 
				    (free) ? freepinfo : NULL, 
				    log_rec, loc_plv);
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
    ** Unwind the fixed pages an locks in the opposite order (page, FMAP, FHDR)
    ** in which they were acquired (FHDR, FMAP, page).
    */

    if (freepinfo && freepinfo->page)
    {
	tmp_status = dm0p_uncache_fix(tbio, DM0P_UNFIX, dmve->dmve_lk_id, 
		dmve->dmve_log_id, &dmve->dmve_tran_id, freepinfo, &local_dberr);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}
    }

    if (physical_free_page_lock)
    {
	tmp_status = dm0p_unlock_page(dmve->dmve_lk_id, dcb, 
	    &log_rec->dall_tblid, log_rec->dall_free_pageno, 
	    LK_PAGE, tbio->tbio_relid, &dmve->dmve_tran_id,
	    &page_lockid, (LK_VALUE *)NULL, &local_dberr);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}
    }

    if (fmappinfo && fmappinfo->page)
    {
	tmp_status = dm0p_uncache_fix(tbio, DM0P_UNFIX, dmve->dmve_lk_id, 
		dmve->dmve_log_id, &dmve->dmve_tran_id, fmappinfo, &local_dberr);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}
    }

    if (fhdrpinfo && fhdrpinfo->page)
    {
	tmp_status = dm0p_uncache_fix(tbio, DM0P_UNFIX, dmve->dmve_lk_id, 
		dmve->dmve_log_id, &dmve->dmve_tran_id, fhdrpinfo, &local_dberr);
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
	    &log_rec->dall_tblid, log_rec->dall_fhdr_pageno, 
	    LK_PAGE, tbio->tbio_relid, &dmve->dmve_tran_id,
	    &fhdr_lockid, (LK_VALUE *)NULL, &local_dberr);
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9626_DMVE_DEALLOC);

    return(status);
}

/*{
** Name: dmv_realloc - Redo the allocate operation.
**
** Description:
**      This function redoes the effects of an Page Allocate Operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fhdr				Table's FHDR page.
**	fmap				Table's FMAP page.
**	log_rec				Dealloc log record.
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
**	26-oct-1992 (jnash & rogerk)
**	    Rewritten for 6.5 recovery project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	 3-jan-1994 (rogerk)
**	    Update page_tran_id when redo a deallocate action since it is
**	    part of the updates done in a regular deallocate.  The tran_id
**	    is really only used when the transaction is open, and currently
**	    redo only occurs on alread-committed or about-to-be-aborted xacts,
**	    but we redo the update here to be consistent.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size as the page_size argument to dmpp_format.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_redealloc(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fhdrpinfo,
DMP_PINFO	    *fmappinfo,
DMP_PINFO	    *freepinfo,
DM0L_DEALLOC	    *log_rec,
DMPP_ACC_PLV	    *loc_plv)
{
    LG_LSN		*log_lsn = &log_rec->dall_header.lsn; 
    i4		free_bit;
    i4		page_type = log_rec->dall_pg_type;
    DM1P_FHDR		*fhdr = NULL;
    DM1P_FMAP		*fmap = NULL;
    DMPP_PAGE		*free = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( fhdrpinfo )
        fhdr = (DM1P_FHDR*)fhdrpinfo->page;
    if ( fmappinfo )
        fmap = (DM1P_FMAP*)fmappinfo->page;
    if ( freepinfo )
        free = (DMPP_PAGE*)freepinfo->page;

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if ((fhdr == NULL) && (fmap == NULL) && (free == NULL))
	return (E_DB_OK);

    free_bit = log_rec->dall_free_pageno % 
		DM1P_FSEG_MACRO(page_type, log_rec->dall_page_size);

    /*
    ** Consistency Checks:
    **
    ** Verify that the deallocated page falls within the range of the FMAP.
    ** Verify that the page is currently marked used in the FMAP page.
    */
    if ((log_rec->dall_free_pageno / DM1P_FSEG_MACRO(page_type, log_rec->dall_page_size)) 
	    != log_rec->dall_map_index)
	TRdisplay("dmv_redealloc: Consistency check, map index mismatch\n");
    if (fmap && (BITMAP_TST_MACRO(
	DM1P_VPT_FMAP_BITMAP_MACRO(page_type, fmap), free_bit)))
    {
	TRdisplay("dmv_redealloc: Consistency check, freebit mismatch\n");
    }


    /*
    ** Redo the deallocate operation.
    */

    /*
    ** Update the FHDR's Freemap hints.
    ** If the deallocate of this page caused us to update the FHDR's
    ** free hint (indicating free space on this fmap), then reapply that
    ** update.
    */
    if (fhdr)
    {
	dmveMutex(dmve, fhdrpinfo);
	if (log_rec->dall_fhdr_hint)
	    VPS_SET_FREE_HINT_MACRO(page_type, 
		(DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, fhdr, 
		log_rec->dall_map_index)));

	DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(page_type,fhdr,DMPP_MODIFY);
	DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(page_type, fhdr, *log_lsn);
	dmveUnMutex(dmve, fhdrpinfo);
    }

    /*
    ** Update the FMAP page to reflect the deallocate of the new page.
    ** Move the page's first-freebit pointer if the page just deallocated
    ** is previous to its current value.
    */
    if (fmap)
    {
	dmveMutex(dmve, fmappinfo);
	BITMAP_SET_MACRO(DM1P_VPT_FMAP_BITMAP_MACRO(page_type, fmap), free_bit);

	if (free_bit < 
	    (i4)DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(page_type, fmap))
	    DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(page_type,fmap,free_bit);

	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);
	DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, *log_lsn);
	dmveUnMutex(dmve, fmappinfo);
    }

    /*
    ** Format the allocated page as an empty data page.
    */
    if (free)
    {
	dmveMutex(dmve, freepinfo);
        (*loc_plv->dmpp_format)(page_type, log_rec->dall_page_size,
				free, log_rec->dall_free_pageno, DMPP_FREE,
				DM1C_ZERO_FILL);

        DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, free, DMPP_MODIFY);
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, free, *log_lsn);
	DMPP_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, free, dmve->dmve_tran_id);
	dmveUnMutex(dmve, freepinfo);
    }
    
    return(E_DB_OK);
}

/*{
** Name: dmv_undealloc - UNDO of a Page Deallocate operation (reallocate page).
**
** Description:
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fhdr				Table's FHDR page.
**	fmap				Table's FMAP page.
**	log_rec				Dealloc log record.
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
**	06-oct-1992 (jnash & rogerk)
**	    Written for 6.5 recovery project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the undo recovery.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size as the page_size argument to dmpp_format.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      04-Feb-1999 (hanal04)
**          If not dmve_logging restore the page's original lsn after
**          formatting the page to prevent lsn mismatch errors during
**          rollforwarddb -e time. b94849.
**	30-Apr-2004 (jenjo02)
**	    Removed dmve_unreserve_space(); DM0LDEALLOC
**	    doesn't reserve space or use LG_RS_FORCE.
**	9-Jan-2008 (kibro01) b119663
**	    Reinstate DM0LALLOC unreserve space if the space was allocated,
**	    determined by the DM0L_FASTCOMMIT flag on the log header.
*/
static DB_STATUS
dmv_undealloc(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fhdrpinfo,
DMP_PINFO	    *fmappinfo,
DMP_PINFO	    *freepinfo,
DM0L_DEALLOC	    *log_rec,
DMPP_ACC_PLV	    *loc_plv)
{
    LG_LSN		*log_lsn = &log_rec->dall_header.lsn; 
    LG_LSN		lsn;
    LG_LSN              save_lsn;
    DB_STATUS		status;
    i4		dm0l_flags;
    i4		free_bit;
    i4		first_bit_index;
    i4		last_bit_index;
    i4		i;
    i4		page_type = log_rec->dall_pg_type;
    bool		fhdr_freehint_update = FALSE;
    i4			*err_code = &dmve->dmve_error.err_code;
    DM1P_FHDR		*fhdr = NULL;
    DM1P_FMAP		*fmap = NULL;
    DMPP_PAGE		*free = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( fhdrpinfo )
        fhdr = (DM1P_FHDR*)fhdrpinfo->page;
    if ( fmappinfo )
        fmap = (DM1P_FMAP*)fmappinfo->page;
    if ( freepinfo )
        free = (DMPP_PAGE*)freepinfo->page;

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if ((fhdr == NULL) && (fmap == NULL) && (free == NULL))
	return (E_DB_OK);

    free_bit = log_rec->dall_free_pageno % 
		DM1P_FSEG_MACRO(page_type, log_rec->dall_page_size);

    /*
    ** Consistency Checks:
    **
    ** Verify that the allocated page falls within the range of the FMAP.
    ** Verify that the allocated page is currently marked used in the FMAP page.
    */
    if ((log_rec->dall_free_pageno / 
		DM1P_FSEG_MACRO(page_type, log_rec->dall_page_size))
		!= log_rec->dall_map_index)
	TRdisplay("dmv_undealloc: Consistency check, map index mismatch\n");
    if (fmap && (BITMAP_TST_MACRO(
	DM1P_VPT_FMAP_BITMAP_MACRO(page_type, fmap), free_bit) == 0))
    {
	TRdisplay("dmv_undealloc: Consistency check, freebit mismatch\n");
    }

    /*
    ** Pre-determine whether the fhdr's free hint needs to be updated as
    ** part of the undo operation so we can log the appropriate action.
    **
    ** Because of short term locking done on fhdr/fmap pages, we do not
    ** simply undo the free-hint action done by the deallocate operation.
    ** Allocate/deallocate operations done by other transactions after
    ** our allocate may have changed the state of fmap and fhdr pages.
    **
    ** The only way we can accurately know whether or not we should update
    ** the free hint is to search the fmap page to see whether our un-deallocate
    ** will use up the last free page in the fmap.  This is only possible
    ** if the fmap is being recovered along with the fhdr page.  In situations
    ** where partial recovery is being done, then we never update the free
    ** hint since it is better to mistakenly leave it on than to mistakenly
    ** turn it off.  If this occurs then the server will believe the fmap
    ** has free space and will uselessly search it for space periodically
    ** before going on to the next fmap.  This will continue until some 
    ** subsequent update is done to the fmap.
    **
    ** (During REDO processing of CLR records, we can simply take the same
    ** action as did the original undo.  Only during original Rollback
    ** operations does this check need to be made).
    */
    if (fhdr)
    {
	if (log_rec->dall_header.flags & DM0L_CLR)
	{
	    fhdr_freehint_update = log_rec->dall_fhdr_hint;
	}
	else if (fmap)
	{
	    first_bit_index = 
		(i4)DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(page_type, 
							    fmap)/DM1P_BTSZ;
            last_bit_index = 
		DM1P_MBIT_MACRO(page_type, log_rec->dall_page_size) - 1;
            if (DM1P_VPT_GET_FMAP_LASTBIT_MACRO(page_type, fmap) <
		DM1P_FSEG_MACRO(page_type, log_rec->dall_page_size))
	    {
                last_bit_index = 
		(i4)DM1P_VPT_GET_FMAP_LASTBIT_MACRO(page_type, fmap)
		    		/ DM1P_BTSZ;
	    }

	    fhdr_freehint_update = TRUE;
            for (i = first_bit_index; i <= last_bit_index; i++)
            {
                if ((DM1P_VPT_FMAP_BITMAP_MACRO(page_type, fmap)[i]))
                {
		    /*
		    ** Found a free page, if it is anything other than the
		    ** page we are currently un-deallocating then we should
		    ** not be updating the fhdr free hint.
		    */
		    if ((i != (free_bit / 
			DM1P_FSEG_MACRO(page_type, log_rec->dall_page_size)))
			||
			((i4)(DM1P_VPT_FMAP_BITMAP_MACRO(page_type,
						fmap)[i]) !=
			(free_bit % DM1P_FSEG_MACRO(page_type, log_rec->dall_page_size))))
		    {
			fhdr_freehint_update = FALSE;
			break;
		    }
                }
            }
	}
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
    if ((log_rec->dall_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    dm0l_flags = (log_rec->dall_header.flags | DM0L_CLR);

	    status = dm0l_dealloc(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->dall_tblid, 
		(DB_TAB_NAME*)&log_rec->dall_vbuf[0], log_rec->dall_tab_size, 
		(DB_OWN_NAME*)&log_rec->dall_vbuf[log_rec->dall_tab_size], log_rec->dall_own_size, 
		log_rec->dall_pg_type, log_rec->dall_page_size,
		log_rec->dall_loc_cnt, 
		log_rec->dall_fhdr_pageno, log_rec->dall_fmap_pageno, 
		log_rec->dall_free_pageno, log_rec->dall_map_index,
		log_rec->dall_fhdr_cnf_loc_id, log_rec->dall_fmap_cnf_loc_id,
		log_rec->dall_free_cnf_loc_id, (bool) fhdr_freehint_update,
		log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
                /*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 

		SETDBERR(&dmve->dmve_error, 0, E_DM9626_DMVE_DEALLOC);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are processing recovery of a Deallocate CLR (redo-ing the 
	** undo of a deallocate (wow)) then we don't log a CLR but instead 
	** save the LSN of the log record we are processing with which to 
	** update the page lsn's.
	*/
	lsn = *log_lsn;
    }

    /*
    ** Undo the dealloc operation.
    */

    /*
    ** Update the FHDR's Freemap hint.
    ** The undo of a deallocate involves removing a free page from its free map.
    ** If this page is the last free page described in the fmap, then the free
    ** hint in the fhdr needs to be udpated.
    ** 
    ** See description above for situations in which the free hint is updated.
    */
    if (fhdr)
    {
	if (fhdr_freehint_update)
	{
	    dmveMutex(dmve, fhdrpinfo);
	    VPS_CLEAR_FREE_HINT_MACRO(page_type, 
		(DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, fhdr, 
		log_rec->dall_map_index)));
	    DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(page_type, fhdr, DMPP_MODIFY);

	    if (dmve->dmve_logging)
		DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(page_type, fhdr, lsn);
	    dmveUnMutex(dmve, fhdrpinfo);
	}
    }

    /*
    ** Update the FMAP page to undo the deallocate of the new page.
    */
    if (fmap)
    {
	dmveMutex(dmve, fmappinfo);
	BITMAP_CLR_MACRO(DM1P_VPT_FMAP_BITMAP_MACRO(page_type, fmap),
	    free_bit);
	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);

	if (dmve->dmve_logging)
	    DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, lsn);
	dmveUnMutex(dmve, fmappinfo);
    }

    /*
    ** Reformat the page as an empty data page.  It is not linked back
    ** into its data structure by this operation (its main and ovfl pointers
    ** are left zero'd) - it is currently left as a formatted, but orphaned 
    ** data page.  It may be linked back into a data structure by a soon-to-
    ** be-undone join record, or it may just be left as a disassocated data
    ** page (ready to have whatever tuples used to occupy it put back by
    ** un-delete actions).
    */
    if (free)
    {
	dmveMutex(dmve, freepinfo);

        /* b94849 - Save the page lsn prior to format */
	DMPP_VPT_REC_PAGE_LOG_ADDR_MACRO(page_type, save_lsn, free);

        (*loc_plv->dmpp_format)(page_type, log_rec->dall_page_size,
				free, log_rec->dall_free_pageno, DMPP_DATA,
				DM1C_ZERO_FILL);
	DMPP_VPT_SET_PAGE_TRAN_ID_MACRO(page_type, free, dmve->dmve_tran_id);
        DMPP_VPT_SET_PAGE_SEQNO_MACRO(page_type, free, 0);
        DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, free, DMPP_MODIFY);

        /* b94849 - If not dmve_logging restore the page lsn as we are
        ** undoing open transactions during a rollforwarddb
        */
	if (dmve->dmve_logging)
        {
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, free, lsn);
        }
        else
        {
            DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, free, save_lsn);
        }
	dmveUnMutex(dmve, freepinfo);
    }

    /*
    ** Release log file space allocated for logfile forces that may be
    ** required by the buffer manager when unfixing the pages just recovered.
    */
    if (((log_rec->dall_header.flags & DM0L_CLR) == 0) &&
	((log_rec->dall_header.flags & DM0L_FASTCOMMIT) == 0) &&
        (dmve->dmve_logging))
    {
        dmve_unreserve_space(dmve, 1);
    }
    return(E_DB_OK);
}
