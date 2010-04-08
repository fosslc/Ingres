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
**  Name: DMVEFMAP.C - The recovery of a Add Fmap operation.
**
**  Description:
**	This file contains the routines for the recovery of an Add Fmap
**	operation.
**
**          dmve_fmap - The recovery of an Add Fmap operation.
**          dmve_refmap - The Redo Recovery of an Add Fmap operation.
**          dmve_unfmap - The Undo Recovery of an Add Fmap operation.
**
**  History:
**	02-apr-1990 (Derek)
**          Created.
**	26-oct-1992 (rogerk)
**	    Written for Reduced Logging Project.
**	18-jan-1993 (rogerk)
**	    Add check in undo for case when no recovery is needed.
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
**	    Add error messages.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	26-jul-1993 (rogerk)
**	    Added tracing of page information during recovery.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the undo recovery.
**	19-oct-1993 (wolf)
**	    Fix compile error from yesterday's integration
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size as the page_size argument to dmpp_format.
**      06-mar-1996 (stial01)
**          Pass fmap_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages. Pass page size to MAP_FROM_NUMBER_MACRO.
**	03-sep-1996 (pchang)
**	    Fixed bug 77416.  We should take into account any unused pages that
**	    exist in a table prior to an extension.  Any new FMAPs should start
**	    to occupy from an unused page (if any) instead of a newly allocated
**	    page.  The changes in dmv_refmap() and dmv_unfmap() correspond with
**	    those in extend() in dm1p.c.
**      22-nov-96 (stial01,dilma04)
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
**	   Error code is getting reset to E_DB_OK on error and not making
**	   the database inconsistent.
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
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
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Reapply the original fmap highwater mark from the log record
**          in dmv_refmap().
**          Add the fmap_hw_mark parameter to the call to dm0l_fmap() in
**          dmv_unfmap.
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
**/

/*
** Forward Declarations.
*/
static DB_STATUS dmv_refmap(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fhdrpinfo,
			DMP_PINFO	    *fmappinfo,
			DM0L_FMAP	    *log_rec);

static DB_STATUS dmv_unfmap(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fhdrpinfo,
			DMP_PINFO	    *fmappinfo,
			DM0L_FMAP	    *log_rec,
			DMPP_ACC_PLV	    *loc_plv);


/*{
** Name: dmve_fmap - The recovery of an Add Fmap operation.
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
**	26-oct-1992 (jnash & rogerk)
**	    Reduced Logging Project: Created for new 6.5 recovery protocols.
**	14-dec-1992 (rogerk)
**	    Reduced Logging Fixes. Added dm0p_pagetype call.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass page size to dm1c_getaccessors().
**      22-nov-96 (stial01,dilma04)
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
dmve_fmap(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_FMAP		*log_rec = (DM0L_FMAP *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->fmap_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DM1P_FHDR		*fhdr = NULL;
    DM1P_FMAP		*fmap = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DMPP_ACC_PLV	*loc_plv;
    LK_LKID		lockid;
    LK_LKID		fhdr_lockid;
    i4			lock_action;
    i4			grant_mode;
    i4			recovery_action;
    i4			error;
    i4			loc_error;
    i4  		fhdr_page_recover;
    i4  		fmap_page_recover;
    i4  		physical_fhdr_page_lock = FALSE;
    i4			fix_option = 0;
    i4			page_type = log_rec->fmap_pg_type;
    DB_ERROR		local_dberr;
    DMP_PINFO		*fhdrpinfo = NULL;
    DMP_PINFO		*fmappinfo = NULL;

    CLRDBERR(&dmve->dmve_error);

    MEfill(sizeof(LK_LKID), 0, &lockid);
    MEfill(sizeof(LK_LKID), 0, &fhdr_lockid);

    for (;;)
    {
	/*
	** Consistency Check:  check for illegal log records.
	*/
	if ((log_rec->fmap_header.type != DM0LFMAP) ||
	    (log_rec->fmap_header.length != sizeof(DM0L_FMAP)))
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
	fhdr_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->fmap_fhdr_cnf_loc_id);
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
		if (fhdr_page_recover || fmap_page_recover)
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
	**
	** If we are doing Forward processing (Redo, Rollforward) then
	** we must fix the newly allocated fmap page for SCRATCH in the
	** the buffer manager since we cannot trust its state on disk.
	**
	** NOTE THAT THIS WILL CAUSE US TO ALWAYS HAVE TO REDO CHANGES
	** TO NEWLY ALLOCATED PAGES even if we did happen to write them
	** before recovery was needed.
	*/
	if (fhdr_page_recover)
	{
	    status = dmve_cachefix_page(dmve, log_lsn,
					tbio, log_rec->fmap_fhdr_pageno,
					fix_option, loc_plv, &fhdrpinfo);
	    if (status != E_DB_OK)
		break;
	    fhdr = (DM1P_FHDR*)fhdrpinfo->page;
	}

	if (fmap_page_recover)
	{
	    if (dmve->dmve_action != DMVE_UNDO)
		fix_option |= DM0P_SCRATCH;

	    status = dmve_cachefix_page(dmve, log_lsn,
					tbio, log_rec->fmap_fmap_pageno,
					fix_option, loc_plv, &fmappinfo);
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
		(DMPP_PAGE *) fhdr, loc_plv, "FHDR"); 
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
	    break;

	case DMVE_UNDO:
	    if (fhdr && LSN_LT(
		DM1P_VPT_ADDR_FHDR_PG_LOGADDR_MACRO(page_type, fhdr),
		log_lsn))
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
	    break;
	}
	if (status != E_DB_OK || (!fhdr && !fmap))
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
	    status = dmv_refmap(dmve, tbio, 
	    			(fhdr) ? fhdrpinfo : NULL, 
	    			(fmap) ? fmappinfo : NULL, 
				log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unfmap(dmve, tbio, 
	    			(fhdr) ? fhdrpinfo : NULL, 
	    			(fmap) ? fmappinfo : NULL, 
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
    ** Unfix and unlock the pages in the opposite order (FMAP, FHDR)
    ** in which they were acquired (FHDR, FMAP).
    ** No need to force them to disk - they
    ** will be tossed out through normal cache protocols if Fast
    ** Commit or at the end of the abort if non Fast Commit.
    */

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
**	fhdr				Table's FHDR page.
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
**	26-oct-1992 (jnash & rogerk)
**	    Rewritten for 6.5 recovery project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass added page size parameter to dm1p_fmfree, dm1p_fminit.
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages. Pass page size to MAP_FROM_NUMBER_MACRO.
**	03-sep-1996 (pchang)
**	    Fixed bug 77416.  We should take into account any unused pages that
**	    exist in a table prior to an extension.  Any new FMAPs should start
**	    to occupy from an unused page (if any) instead of a newly allocated
**	    page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Reapply the original fmap highwater mark from the log record.
**          This prevents the incorrect default of 16000 being used at
**          some point in the future.
**	10-Nov-2003 (thaju02)  INGSRV2593/B111276
**	    No need to update fmap_hw_mark here, do it in dmv_realloc()-
**	    thus mimicing what we do during normal forward processing. 
*/
static DB_STATUS
dmv_refmap(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fhdrpinfo,
DMP_PINFO	    *fmappinfo,
DM0L_FMAP	    *log_rec)
{
    LG_LSN		*log_lsn = &log_rec->fmap_header.lsn; 
    i4			*err_code = &dmve->dmve_error.err_code;
    i4			page_type = log_rec->fmap_pg_type;
    DM1P_FHDR		*fhdr = NULL;
    DM1P_FMAP		*fmap = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( fhdrpinfo )
        fhdr = (DM1P_FHDR*)fhdrpinfo->page;
    if ( fmappinfo )
        fmap = (DM1P_FMAP*)fmappinfo->page;

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if ((fhdr == NULL) && (fmap == NULL))
	return (E_DB_OK);

    /*
    ** Consistency Checks:
    ** Verify that the fhdr indicates the expected number of fmaps.
    */
    if (fhdr) 
    {
	if (DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr) != 
	    log_rec->fmap_map_index)
	{
	    uleFormat(NULL, E_DM9676_DMVE_FMAP_FHDR_STATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), &log_rec->fmap_tblname,
		sizeof(DB_OWN_NAME), &log_rec->fmap_tblowner,
		0, DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(page_type, fhdr),
		0, DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr),
		0, log_rec->fmap_map_index);
	    dmd_log(1, (PTR) log_rec, 4096);
	    uleFormat(NULL, E_DM9643_REDO_FMAP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
    }


    /*
    ** Redo the extend operation.
    */

    /*
    ** Update the FHDR to reflect the addition of a new FMAP.
    **   - Update the count of fmaps.
    **   - Record the new fmap's page number in the fmap array.
    **   - Set the free hint to record that there are free pages in the new fmap
    **   - If there are used pages in this fmap, update the highwater mark.
    **   - Write the FMAP log record's LSN to the updated page.
    */
    if (fhdr)
    {
	dmveMutex(dmve, fhdrpinfo);
	DM1P_VPT_INCR_FHDR_COUNT_MACRO(page_type, fhdr);
	VPS_MAP_FROM_NUMBER_MACRO(page_type, 
	    (DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, fhdr, 
	    log_rec->fmap_map_index)), log_rec->fmap_fmap_pageno);
	VPS_SET_FREE_HINT_MACRO(page_type,
	    (DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, fhdr,
	    log_rec->fmap_map_index)));

	if ((log_rec->fmap_last_used /
		DM1P_FSEG_MACRO(page_type, log_rec->fmap_page_size))
	    >= log_rec->fmap_map_index)
	    DM1P_VPT_SET_FHDR_HWMAP_MACRO(page_type, fhdr, (log_rec->fmap_map_index + 1));

	DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(page_type,fhdr,DMPP_MODIFY);
	DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(page_type, fhdr, *log_lsn);
	dmveUnMutex(dmve, fhdrpinfo);
    }

    /*
    ** Initialize the new FMAP page.
    ** Mark the appropriate ranges of pages used & free.
    ** Write the FMAP log record's LSN to the updated page.
    */
    if (fmap)
    {
	dmveMutex(dmve, fmappinfo);
	dm1p_fminit(fmap, page_type, log_rec->fmap_page_size);
	DM1P_VPT_SET_FMAP_SEQUENCE_MACRO(page_type, fmap, log_rec->fmap_map_index);

	/*
	** Update the FMAP page to reflect any of the new pages which
	** were marked free by the extend operation.
	*/
	dm1p_fmfree(fmap, log_rec->fmap_first_free, log_rec->fmap_last_free,
	    page_type, log_rec->fmap_page_size);

	/*
	** If some of the new FMAP page numbers fall within the range
	** of this fmap then update the fmap to show that they are used
	*/
	if ((log_rec->fmap_last_used /
		DM1P_FSEG_MACRO(page_type, log_rec->fmap_page_size))
	    >= log_rec->fmap_map_index)
	{
	    dm1p_fmused(fmap, log_rec->fmap_first_used, log_rec->fmap_last_used,
		page_type, log_rec->fmap_page_size);
	}

	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);
	DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, *log_lsn);
	dmveUnMutex(dmve, fmappinfo);
    }
    
    return(E_DB_OK);
}

/*{
** Name: dmv_unfmap - UNDO of an Add Fmap operation.
**
** Description:
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fhdr				Table's FHDR page.
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
**	06-oct-1992 (jnash & rogerk)
**	    Written for 6.5 recovery project.
**	18-jan-1993 (rogerk)
**	    Add check for case when no recovery is needed.
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
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages. Pass page size to MAP_FROM_NUMBER_MACRO.
**	03-sep-1996 (pchang)
**	    Fixed bug 77416.  We should take into account any unused pages that
**	    exist in a table prior to an extension.  Any new FMAPs should start
**	    to occupy from an unused page (if any) instead of a newly allocated
**	    page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      30-Oct-2000 (hanal04) Bug 103037 INGSRV 1291
**          Added fmap_hw_mark parameter to dm0l_fmap().
**	30-Apr-2004 (jenjo02)
**	    Removed dmve_unreserve_space(); DM0LFMAP
**	    doesn't reserve space or use LG_RS_FORCE.
**	9-Jan-2008 (kibro01) b119663
**	    Reinstate DM0LALLOC unreserve space if the space was allocated,
**	    determined by the DM0L_FASTCOMMIT flag on the log header.
*/
static DB_STATUS
dmv_unfmap(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fhdrpinfo,
DMP_PINFO	    *fmappinfo,
DM0L_FMAP	    *log_rec,
DMPP_ACC_PLV	    *loc_plv)
{
    LG_LSN		*log_lsn = &log_rec->fmap_header.lsn; 
    LG_LSN		lsn;
    DB_STATUS		status;
    i4		dm0l_flags;
    i4		first_page;
    i4		last_page;
    i4		first_bit;
    i4		last_bit;
    i4		bit;
    i4		page_type = log_rec->fmap_pg_type;
    i4		*err_code = &dmve->dmve_error.err_code;
    DM1P_FHDR		*fhdr = NULL;
    DM1P_FMAP		*fmap = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( fhdrpinfo )
        fhdr = (DM1P_FHDR*)fhdrpinfo->page;
    if ( fmappinfo )
        fmap = (DM1P_FMAP*)fmappinfo->page;

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if ((fhdr == NULL) && (fmap == NULL))
	return (E_DB_OK);

    /*
    ** Consistency Checks:
    ** 
    ** Verify that the fmap's sequence number matches the logged sequence
    ** number and that the fhdr indicates the expected number of fmaps.
    */
    if (fhdr)
    {
	if (DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr) != 
	    log_rec->fmap_map_index + 1)
	{
	    uleFormat(NULL, E_DM9676_DMVE_FMAP_FHDR_STATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), &log_rec->fmap_tblname,
		sizeof(DB_OWN_NAME), &log_rec->fmap_tblowner,
		0, DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(page_type, fhdr),
		0, DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr),
		0, (log_rec->fmap_map_index + 1));
	    dmd_log(1, (PTR) log_rec, 4096);
	    uleFormat(NULL, E_DM9642_UNDO_FMAP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
    }

    if (fmap)
    {
	if (DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(page_type, fmap) != 
	    log_rec->fmap_map_index)
	{
	    uleFormat(NULL, E_DM9677_DMVE_FMAP_FMAP_STATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), &log_rec->fmap_tblname,
		sizeof(DB_OWN_NAME), &log_rec->fmap_tblowner,
		0, DM1P_VPT_GET_FMAP_PAGE_PAGE_MACRO(page_type, fmap),
		0, DM1P_VPT_GET_FMAP_SEQUENCE_MACRO(page_type, fmap),
		0, log_rec->fmap_map_index);
	    dmd_log(1, (PTR) log_rec, 4096);
	    uleFormat(NULL, E_DM9642_UNDO_FMAP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
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
    if ((log_rec->fmap_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    dm0l_flags = DM0L_CLR;
	    if (log_rec->fmap_header.flags & DM0L_JOURNAL)
		dm0l_flags |= DM0L_JOURNAL;

	    status = dm0l_fmap(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->fmap_tblid,
		&log_rec->fmap_tblname, &log_rec->fmap_tblowner, 
		log_rec->fmap_pg_type, log_rec->fmap_page_size,
		log_rec->fmap_loc_cnt, 
		log_rec->fmap_fhdr_pageno, log_rec->fmap_fmap_pageno, 
		log_rec->fmap_map_index,
		log_rec->fmap_hw_mark,
		log_rec->fmap_fhdr_cnf_loc_id, log_rec->fmap_fmap_cnf_loc_id,
		log_rec->fmap_first_used, log_rec->fmap_last_used,
		log_rec->fmap_first_free, log_rec->fmap_last_free, 
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
    ** Undo the Add FMAP operation.
    */

    /*
    ** Update the FHDR page to back out the udpates made by the extend.
    ** Decrement the count of fmaps, clear out information in the array
    ** of fmap's written by the extend.
    */
    if (fhdr)
    {
	dmveMutex(dmve, fhdrpinfo);
	DM1P_VPT_DECR_FHDR_COUNT_MACRO(page_type, fhdr);
	VPS_CLEAR_FREE_HINT_MACRO(page_type, 
	    (DM1P_VPT_GET_FHDR_MAP_MACRO(page_type,
	    fhdr, log_rec->fmap_map_index)));
	VPS_MAP_FROM_NUMBER_MACRO(page_type, 
	    (DM1P_VPT_GET_FHDR_MAP_MACRO(page_type,
	    fhdr, log_rec->fmap_map_index)), 0);

	DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(page_type,fhdr,DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(page_type, fhdr, lsn);
	dmveUnMutex(dmve, fhdrpinfo);
    }

    /*
    ** Format the deallocated FMAP as a free page.
    */
    if (fmap)
    {
	dmveMutex(dmve, fmappinfo);
	(*loc_plv->dmpp_format)(page_type, log_rec->fmap_page_size,
				(DMPP_PAGE *) fmap,
				log_rec->fmap_fmap_pageno, DMPP_FREE,
				DM1C_ZERO_FILL);

	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, lsn);
	dmveUnMutex(dmve, fmappinfo);
    }

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
