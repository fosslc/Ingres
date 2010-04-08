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
#include    <dm2f.h>
#include    <dmd.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVEEXT.C - The recovery of a file extend operation.
**
**  Description:
**	This file contains the routines for the recovery of a file extend
**	operation.
**
**          dmve_extend - The recovery of a file extend operation.
**          dmve_reextend - The Redo Recovery of a file extend operation.
**          dmve_unextend - The Undo Recovery of a file extend operation.
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
**	08-dec-1992 (jnash)
**	    Fix bug where file not extended during ROLLFORWARD. 
**	18-jan-1993 (rogerk)
**	    Add check in undo for case when a null page pointer is passed in 
**	    because no recovery is needed on it.
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
**	11-jan-1993 (rmuth)
**	    b57811: dmve_reextend() uses dm2f_sense_file pageno as the
**	    total number of allocated pages need to add one.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	15-sep-1995 (nick)
**	    Correct error in deciding if we need to update an FMAP bitmap
**	    when performing a reextend.
**	26-mar-96 (nick)
**	    Any locations which are not already open by the time we get
**	    here we can ignore.
**      06-mar-1996 (stial01)
**          Pass ext_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02)
**	    New page format suppport: change page header references to use
**	    macros.
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages.
**	03-sep-1996 (pchang)
**	    Fixed bug 77416.  We should take into account any unused pages that
**	    exist in a table prior to an extension.  Any new FMAPs should start
**	    to occupy from an unused page (if any) instead of a newly allocated
**	    page.  The changes in dmv_reextend() and dmv_unextend() correspond
**	    with those in extend() in dm1p.c.
**      22-nov-96 (dilma04)
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
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      14-nov-2000 (stial01)
**          Fixed incorrect args to DM1P_VPT.. macro
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Pass pg type to dmve_trace_page_info (Variable Page Type SIR 102955)
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
**	    SIR 120874: dm2f_?, dm2t_? functions converted to DB_ERROR *
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
static DB_STATUS dmv_reextend(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fhdrpinfo,
			DMP_PINFO	    *fmappinfo,
			DM0L_EXTEND	    *log_rec);

static DB_STATUS dmv_unextend(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *fhdrpinfo,
			DMP_PINFO	    *fmappinfo,
			DM0L_EXTEND	    *log_rec);


/*{
** Name: dmve_extend - The recovery of a File Extend operation.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the extend operation.
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
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass page size to dm1c_getaccessors().
**      22-nov-96 (dilma04)
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
dmve_extend(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_EXTEND		*log_rec = (DM0L_EXTEND *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->ext_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DM1P_FHDR		*fhdr = NULL;
    DM1P_FMAP		*fmap = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DMPP_ACC_PLV	*loc_plv;
    LK_LKID		lockid;
    LK_LKID		fhdr_lockid;
    i4		lock_action;
    i4		grant_mode;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4		page_type = log_rec->ext_pg_type;
    i4  	fhdr_page_recover;
    i4  	fmap_page_recover;
    i4  	physical_fhdr_page_lock = FALSE;
    i4		fix_action = 0;
    DB_ERROR	local_dberr;
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
	if ((log_rec->ext_header.type != DM0LEXTEND) ||
	    (log_rec->ext_header.length != sizeof(DM0L_EXTEND)))
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
	status = dmve_fix_tabio(dmve, &log_rec->ext_tblid, &tbio);
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
					(i4)log_rec->ext_fhdr_cnf_loc_id);
	fmap_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->ext_fmap_cnf_loc_id);

	/*
	** If no changes were made to the FMAP page, then we can avoid
	** recovery operations on it.
	*/
	if ( ! log_rec->ext_fmap_update)
	    fmap_page_recover = FALSE;


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
	    status = dm2t_lock_table(dcb, &log_rec->ext_tblid, DM2T_IX,
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
			&log_rec->ext_tblid, log_rec->ext_fhdr_pageno,
			LK_PAGE, LK_X, LK_PHYSICAL, (i4)0,
			tbio->tbio_relid, tbio->tbio_relowner,
			&dmve->dmve_tran_id, &fhdr_lockid, (i4 *)0,
			(LK_VALUE *)0, &dmve->dmve_error);
		    if (status != E_DB_OK)
			break;

		    physical_fhdr_page_lock = TRUE;
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
					tbio, log_rec->ext_fhdr_pageno,
					fix_action, loc_plv, &fhdrpinfo);
	    if (status != E_DB_OK)
		break;
	    fhdr = (DM1P_FHDR*)fhdrpinfo->page;
	}

	if (fmap_page_recover)
	{
	    status = dmve_cachefix_page(dmve, log_lsn,
					tbio, log_rec->ext_fmap_pageno,
					fix_action, loc_plv, &fmappinfo);
	    if (status != E_DB_OK)
		break;
	    fmap = (DM1P_FMAP*)fmappinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->ext_pg_type, log_rec->ext_page_size,
		(DMPP_PAGE *) fhdr, loc_plv, "FHDR");
	    dmve_trace_page_info(log_rec->ext_pg_type, log_rec->ext_page_size,
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
	if (log_rec->ext_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_reextend(dmve, tbio, 
	    			(fhdr) ? fhdrpinfo : NULL, 
	    			(fmap) ? fmappinfo : NULL, 
				log_rec);
	    break;

	case DMVE_UNDO:
	    status = dmv_unextend(dmve, tbio,
	    			(fhdr) ? fhdrpinfo : NULL, 
	    			(fmap) ? fmappinfo : NULL, 
				log_rec);
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
	    &log_rec->ext_tblid, log_rec->ext_fhdr_pageno, LK_PAGE,
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9628_DMVE_EXTEND);

    return(status);
}

/*{
** Name: dmv_reextend - Redo the extend operation.
**
** Description:
**      This function redoes the effects of an Extend Operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fhdr				Table's FHDR page.
**	fmap				Table's FMAP page.
**	log_rec				Extend log record.
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
**	08-dec-1992 (jnash)
**	    Fix bug where file not extended during ROLLFORWARD. 
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	11-jan-1993 (rmuth)
**	    b57811: dmve_reextend() uses dm2f_sense_file pageno as the
**	    total number of allocated pages need to add one.
**	15-sep-1995 (nick)
**	    Correct calculation of whether we are updating the FMAP bitmaps
**	    based in information in the log record - since we are working on 
**	    an extend then the FMAP referenced is actually the last one in
**	    the file.  We need to update this if the pages marked used or
**	    free are in this FMAP and hence should be using '<=' 
**	    rather than '>=' in the calculation ...
**	    '(page_number / DM1P_FSEG) >= FMAP sequence' when deciding if we
**	    call dm1p_fmused() or dm1p_fmfree().
**	26-mar-96 (nick)
**	    Skip unopened locations when determining the require allocation
**	    actions.
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass added page size parameter to dm1p_fmfree().
**      21-may-1996 (thaju02)
**          Change hint macros to account for 64 bit tid support and
**          multiple fhdr pages.
**	03-sep-1996 (pchang)
**	    Fixed bug 77416.  We should take into account any unused pages that
**	    exist in a table prior to an extension.  Any new FMAPs should start
**	    to occupy from an unused page (if any) instead of a newly allocated
**	    page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      26-apr-2007 (smeke01) b118203
**          The fhdr can legitimately be NULL (if the fhdr page is not in need 
**          of recovery). However there was one reference to fhdr that was not
**          protected by a check for NULL.  
**      23-Feb-2009 (hanal04) Bug 121652
**          DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO expects a bit not a page number.
**          Make sure that any pages used for new FMAPs are not on an
**          earlier FMAP.
*/
static DB_STATUS
dmv_reextend(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fhdrpinfo,
DMP_PINFO	    *fmappinfo,
DM0L_EXTEND	    *log_rec)
{
    LG_LSN	*log_lsn = &log_rec->ext_header.lsn; 
    i4		i;
    i4		last_page;
    i4		new_last_page;
    i4		need;
    i4		incr;
    i4		page_type = log_rec->ext_pg_type;
    DMP_LOCATION	*loc;
    DB_STATUS		status;
    i4			*err_code = &dmve->dmve_error.err_code;
    i4  fseg = DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size);
    DM1P_FHDR	*fhdr = NULL;
    DM1P_FMAP	*fmap = NULL;

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
    ** Verify that the current EOF described in the FHDR matches the
    ** old EOF of the extend record and that the FHDR's current fmap
    ** matches the logged fmap index.
    */
    if (fhdr) 
    {
	if ((DM1P_VPT_GET_FHDR_PAGES_MACRO(page_type, fhdr) != 
		log_rec->ext_old_pages) ||
	    (DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr) != 
		(log_rec->ext_map_index + 1)))
	{
	    uleFormat(NULL, E_DM9675_DMVE_EXT_FHDR_STATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 8,
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), &log_rec->ext_tblname,
		sizeof(DB_OWN_NAME), &log_rec->ext_tblowner,
		0, DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(page_type, fhdr),
		0, DM1P_VPT_GET_FHDR_PAGES_MACRO(page_type, fhdr),
		0, log_rec->ext_old_pages,
		0, DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr),
		0, (log_rec->ext_map_index + 1));
	    dmd_log(1, (PTR) log_rec, 4096);
	    uleFormat(NULL, E_DM9646_REDO_EXTEND, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	}
    }

    /*
    ** Rollforward needs to allocate space in each table location.
    */
    if (dmve->dmve_action == DMVE_DO)
    {
	for (i = 0; i < log_rec->ext_loc_cnt; i++)
	{
	    /*
	    ** If this location is not being recovered, just continue.
	    */
	    if ( dmve_location_check(dmve, log_rec->ext_cnf_loc_array[i]) ==
		 FALSE )
		continue; 

	    loc = &tabio->tbio_location_array[i];

	    /*
	    ** given that we've previously built into the tabio all
	    ** locations required for this operation AND opened them, 
	    ** ignore non-open locations here as attempting to use them
	    ** will cause us to get our sums wrong 
	    */
	    if (!DM2F_LOC_ISOPEN_MACRO(loc))
		continue;

	    /*
	    ** Find amount of space necessary at this location.  It
	    ** should turn out that no space is needed on 
	    ** locations unaffected by this extend operation.
	    */
	    need = dm2f_file_size(log_rec->ext_new_pages, 
		log_rec->ext_loc_cnt, i);

	    /*
	    ** Find amount of space already existing at this location.
	    */
	    status = dm2f_sense_file(loc, 1, &log_rec->ext_tblname, 
		&dmve->dmve_dcb_ptr->dcb_name, &last_page, &dmve->dmve_error);
	    if (status != E_DB_OK)
		return(E_DB_ERROR);

	    /*
	    ** Allocate required additional space.
	    */
	    last_page++;
	    incr = need - last_page;
	    if (incr > 0)
	    {
		status = dm2f_galloc_file(loc, 1, &log_rec->ext_tblname, 
		    &dmve->dmve_dcb_ptr->dcb_name, incr, &last_page, 
		    &dmve->dmve_error);
		if (status != E_DB_OK)
		    return(E_DB_ERROR);
	    }
	}
    }


    /*
    ** Redo the extend operation.
    */

    /*
    ** Update the FHDR's page count.
    ** Update the FHDR Highwater and Free hints if they were originally updated.
    ** Write the Extend log record's LSN to the updated page.
    */
    if (fhdr)
    {
	dmveMutex(dmve, fhdrpinfo);
	DM1P_VPT_SET_FHDR_PAGES_MACRO(page_type, fhdr, log_rec->ext_new_pages);
	DM1P_VPT_INCR_FHDR_EXTEND_CNT_MACRO(page_type, fhdr);

	if (log_rec->ext_fhdr_hwmap)
	    DM1P_VPT_SET_FHDR_HWMAP_MACRO(page_type, fhdr, 
		((log_rec->ext_last_used % fseg) + 1));

	if (log_rec->ext_fhdr_hint)
	    VPS_SET_FREE_HINT_MACRO(page_type, 
		(DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, fhdr, 
		log_rec->ext_map_index)));

	DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(page_type,fhdr,DMPP_MODIFY);
	DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(page_type, fhdr, *log_lsn);
	dmveUnMutex(dmve, fhdrpinfo);
    }

    /*
    ** Update the FMAP to reflect the newly extended pages.
    */
    if (fmap)
    {
	DM_PAGENO  first_used_page = log_rec->ext_first_used;
	DM_PAGENO  last_used_page  = log_rec->ext_last_used;
	DM_PAGENO  first_free_page = log_rec->ext_first_free;
	DM_PAGENO  last_free_page  = log_rec->ext_last_free;

	dmveMutex(dmve, fmappinfo);

	/*
	** Update the FMAP page to reflect any of the new pages which
	** were marked used by the extend operation (those which were used
	** to create new FMAP pages).  The used pages are between the range
	** of ext_first_used and ext_last_used.
	**
	** Check to make sure there were used pages and that the used pages
	** fall within the range of the current FMAP page.
	*/
	if ( (first_used_page <= last_used_page) &&
	    ((first_used_page / fseg) <= log_rec->ext_map_index) &&
            ((last_used_page / fseg) >= log_rec->ext_map_index) )
	{
	    if (DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(page_type, fmap) == 
		(DM1P_VPT_GET_FMAP_HW_MARK_MACRO(page_type, fmap) + 1))
	    {
                /*
                ** There aren't any free pages other than any allocated but
                ** unused pages on this fmap.  We have to do this test here
                ** before the highwater mark gets updated by dm1p_fmused().
                **
                ** If the addition of new FMAP page(s) would use up all the
                ** free pages on this fmap then update the first free bit and
                ** clear the free hint;  Otherwise, just update the first
                ** free bit. (B77416)
                */
		DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(page_type, fmap,
		    ((last_used_page + 1) % fseg));

		if (DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(page_type, fmap)
		    >= DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size))
		{
		    DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(page_type, fmap,
			DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size));
		    /* 
		    ** (b118203) 
		    ** Update the fhdr fhdr_map, but only if the fhdr is not NULL. 
		    */ 
		    if (fhdr)
		    {
			VPS_CLEAR_FREE_HINT_MACRO(page_type,
			    (DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, fhdr,
			    log_rec->ext_map_index)));
		    }
		}
	    }
	    dm1p_fmused(fmap, first_used_page, last_used_page,
		page_type, log_rec->ext_page_size);
	}

	/*
	** Update the FMAP page to reflect any of the new pages which
	** were marked free by the extend operation.
	**
	** Check to make sure there were free pages and that the free pages
	** fall within the range of the current FMAP page.
	*/
	if ((first_free_page <= last_free_page) &&
	    ((first_free_page / DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size)) 
	    <= log_rec->ext_map_index))
	{
	    dm1p_fmfree(fmap, first_free_page, last_free_page, 
		page_type, log_rec->ext_page_size);
	}

	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);
	DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, *log_lsn);
	dmveUnMutex(dmve, fmappinfo);
    }
    
    return(E_DB_OK);
}

/*{
** Name: dmv_unextend - UNDO of a File Extend operation.
**
** Description:
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fhdr				Table's FHDR page.
**	fmap				Table's FMAP page.
**	log_rec				Extend log record.
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
**          multiple fhdr pages.
**	03-sep-1996 (pchang)
**	    Fixed bug 77416.  We should take into account any unused pages that
**	    exist in a table prior to an extension.  Any new FMAPs should start
**	    to occupy from an unused page (if any) instead of a newly allocated
**	    page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	30-Apr-2004 (jenjo02)
**	    Removed dmve_unreserve_space(); DM0LEXTEND
**	    doesn't reserve space or use LG_RS_FORCE.
**	9-Jan-2008 (kibro01) b119663
**	    Reinstate DM0LALLOC unreserve space if the space was allocated,
**	    determined by the DM0L_FASTCOMMIT flag on the log header.
*/
static DB_STATUS
dmv_unextend(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *fhdrpinfo,
DMP_PINFO	    *fmappinfo,
DM0L_EXTEND	    *log_rec)
{
    LG_LSN		*log_lsn = &log_rec->ext_header.lsn; 
    LG_LSN		lsn;
    DB_STATUS		status;
    i4		dm0l_flags;
    i4		first_page;
    i4		last_page;
    i4		first_bit;
    i4		last_bit;
    i4		bit;
    i4		page_type = log_rec->ext_pg_type;
    i4		*err_code = &dmve->dmve_error.err_code;
    DM1P_FHDR	*fhdr = NULL;
    DM1P_FMAP	*fmap = NULL;

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
    ** Verify that the current EOF described in the FHDR matches the
    ** new EOF of the extend record and that the FHDR's current fmap
    ** matches the logged fmap index.
    */
    if (fhdr)
    {
	if ((DM1P_VPT_GET_FHDR_PAGES_MACRO(page_type, fhdr) != 
		log_rec->ext_new_pages) ||
	    (DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr) != 
		(log_rec->ext_map_index + 1)))
	{
	    uleFormat(NULL, E_DM9675_DMVE_EXT_FHDR_STATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 8,
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), &log_rec->ext_tblname,
		sizeof(DB_OWN_NAME), &log_rec->ext_tblowner,
		0, DM1P_VPT_GET_FHDR_PAGE_PAGE_MACRO(page_type, fhdr),
		0, DM1P_VPT_GET_FHDR_PAGES_MACRO(page_type, fhdr),
		0, log_rec->ext_new_pages,
		0, DM1P_VPT_GET_FHDR_COUNT_MACRO(page_type, fhdr),
		0, (log_rec->ext_map_index + 1));
	    dmd_log(1, (PTR) log_rec, 4096);
	    uleFormat(NULL, E_DM9645_UNDO_EXTEND, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
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
    if ((log_rec->ext_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    dm0l_flags = (log_rec->ext_header.flags | DM0L_CLR);

	    status = dm0l_extend(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->ext_tblid, &log_rec->ext_tblname, 
		&log_rec->ext_tblowner, 
		log_rec->ext_pg_type, log_rec->ext_page_size,
		log_rec->ext_loc_cnt, 
		(i4 *)&log_rec->ext_cnf_loc_array[0], 
		log_rec->ext_fhdr_pageno, log_rec->ext_fmap_pageno, 
		log_rec->ext_map_index,
		log_rec->ext_fhdr_cnf_loc_id, log_rec->ext_fmap_cnf_loc_id,
		log_rec->ext_first_used, log_rec->ext_last_used,
		log_rec->ext_first_free, log_rec->ext_last_free, 
		log_rec->ext_old_pages, log_rec->ext_new_pages,
		(bool) log_rec->ext_fhdr_hwmap, (bool) log_rec->ext_fhdr_hint,
		(bool) log_rec->ext_fmap_update, log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
                /*
		 * Bug56702: returned logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 

		SETDBERR(&dmve->dmve_error, 0, E_DM9645_UNDO_EXTEND);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are processing recovery of an Extend CLR (redo-ing the undo
	** of an extend) then we don't log a CLR but instead save the LSN
	** of the log record we are processing with which to update the 
	** page lsn's.
	*/
	lsn = *log_lsn;
    }

    /*
    ** Undo the extend operation.
    */

    /*
    ** Update the FHDR page to back out the udpates made by the extend.
    */
    if (fhdr)
    {
	dmveMutex(dmve, fhdrpinfo);
	DM1P_VPT_SET_FHDR_PAGES_MACRO(page_type, fhdr, log_rec->ext_old_pages);
	DM1P_VPT_DECR_FHDR_EXTEND_CNT_MACRO(page_type, fhdr);
	if (log_rec->ext_fhdr_hint)
	    VPS_CLEAR_FREE_HINT_MACRO(page_type, 
		(DM1P_VPT_GET_FHDR_MAP_MACRO(page_type, fhdr, 
		log_rec->ext_map_index)));
	if (log_rec->ext_fhdr_hwmap)
	    DM1P_VPT_SET_FHDR_HWMAP_MACRO(page_type, fhdr, 
		log_rec->ext_map_index);

	DM1P_VPT_SET_FHDR_PAGE_STAT_MACRO(page_type,fhdr,DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1P_VPT_SET_FHDR_PG_LOGADDR_MACRO(page_type, fhdr, lsn);
	dmveUnMutex(dmve, fhdrpinfo);
    }

    /*
    ** Update the FMAP pages to back out the changes made by allocating
    ** a new range of pages.
    */
    if (fmap)
    {
	DM_PAGENO  first_used_page = log_rec->ext_first_used;
	DM_PAGENO  last_used_page  = log_rec->ext_last_used;

	dmveMutex(dmve, fmappinfo);

	/*
	** If any new FMAP added falls within the range of this fmap
	** then mark the pages used as free since we must have released
	** the new FMAPs by now.  Also, move back the highwater marker 
	** and the first free bit (only if it's been moved).  (B77416)
	*/
	if ( (first_used_page <= last_used_page) &&
	     ((first_used_page / DM1P_FSEG_MACRO(page_type,log_rec->ext_page_size)) == 
	     log_rec->ext_map_index) )
	{
	    first_bit = first_used_page % 
		DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size);
	    last_bit = last_used_page % 
		DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size);
	    for (bit = first_bit; bit <= last_bit; bit++)
		BITMAP_SET_MACRO(DM1P_VPT_FMAP_BITMAP_MACRO(page_type,
                        fmap), bit);

	    if ( DM1P_VPT_GET_FMAP_FIRSTBIT_MACRO(page_type, fmap) > first_bit )
		DM1P_VPT_SET_FMAP_FIRSTBIT_MACRO(page_type, fmap, first_bit);
	    if ( DM1P_VPT_GET_FMAP_HW_MARK_MACRO(page_type, fmap) >= first_bit )
		DM1P_VPT_SET_FMAP_HW_MARK_MACRO(page_type, fmap, ( first_bit - 1 ));
	}
	    
	/*
	** If any of the new pages fall within the range of this fmap
	** then zero the free bits for those pages since that is the
	** initial state of the fmap bits for unallocated pages.
	** Also, move back the lastbit marker.
	*/
	first_page = log_rec->ext_old_pages;

	if ((first_page / DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size)) 
	    == log_rec->ext_map_index)
	{
	    last_page = log_rec->ext_new_pages - 1;
	    if ((last_page / 
		DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size)) 
		> log_rec->ext_map_index)
	    {
		last_page = ((log_rec->ext_map_index + 1) * 
		DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size)) - 1;
	    }

	    first_bit = first_page % 
		DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size);
	    last_bit = last_page % 
		DM1P_FSEG_MACRO(page_type, log_rec->ext_page_size);

	    for (bit = first_bit; bit <= last_bit; bit++)
		BITMAP_CLR_MACRO(DM1P_VPT_FMAP_BITMAP_MACRO(page_type, fmap), bit);

	    if ((i4)DM1P_VPT_GET_FMAP_LASTBIT_MACRO(page_type, fmap) >= first_bit)
		DM1P_VPT_SET_FMAP_LASTBIT_MACRO(page_type, fmap, (first_bit - 1));
	}
	DM1P_VPT_SET_FMAP_PAGE_STAT_MACRO(page_type,fmap,DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1P_VPT_SET_FMAP_PG_LOGADDR_MACRO(page_type, fmap, lsn);

	dmveUnMutex(dmve, fmappinfo);
    }

    /*
    ** Release log file space allocated for logfile forces that may be
    ** required by the buffer manager when unfixing the pages just recovered.
    */
    if (((log_rec->ext_header.flags & DM0L_CLR) == 0) &&
	((log_rec->ext_header.flags & DM0L_FASTCOMMIT) == 0) &&
        (dmve->dmve_logging))
    {
        dmve_unreserve_space(dmve, 1);
    }
    return(E_DB_OK);
}
