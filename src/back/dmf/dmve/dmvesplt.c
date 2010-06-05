/*
** Copyright (c) 1992, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <me.h>
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
#include    <dm0m.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVESPLT.C - The recovery of a Btree Split operation.
**
**  Description:
**	This file contains the routines for the recovery of a Btree split
**	operation.
**
**          dmve_btsplit - The recovery of a split operation.
**          dmve_rebtsplit - The Redo Recovery of a split operation.
**          dmve_unbtsplit - The Undo Recovery of a split operation.
**
**  History:
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**      13-jan-1993 (mikem)
**          Lint directed changes.  Fixed "=" vs. "==" problem in 
**	    dmv_rebtsplit().  Previous to this change errors in 2 calls to 
**	    dm1cxrshift() were ignored.
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
**      21-jun-1993 (mikem)
**          su4_us5 port.  Added include of me.h to pick up new macro
**          definitions for MEcopy, MEfill, ...
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	26-jul-1993 (rogerk)
**	    Fix splits of leafs that have overflow chains.  Check if the
**		split moved the overflow chain to the new page and if so redo
**		that operation.
**	    Fix MEcopy prototype warnings that occur when compiling with xDEBUG.
**	    Update page's split_lsn value when redo a split.
**	    Added tracing of page information during recovery.
**	    Added split direction field to the split log record.
**	23-aug-1993 (rogerk)
**	    Fix setting of physical lock flags.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxformat.
**      06-mar-1996 (stial01)
**          Limit kperpage to (DM_TIDEOF + 1) - DM1B_OFFSEQ
**          Pass spl_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**	12-dec-1996 (shero03)
**	    The split page is now in the variable part of the log record.
**	09-jan-1997 (nanpr01)
**	    Revert the order as done in previous integration to prevent 
**	    BUS error.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      29-may-1997 (stial01)
**          dmv_rebtsplit: REDO current: Since page cleans are not logged,
**          don't assume logged page is same as current. Copy logged page
**          to current before shifting entries from spl_split_pos.
**      12-jun-97 (stial01)
**          Pass new attribute, key parameters to dm1cxformat
**      22-aug-1997 (stial01)
**          Restore log_rec->spl_page_size bytes from log record
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_ufx_tabio_cb() calls.
**      13-jan-1997 (stial01)
**          Set DMPP_CLEAN on sibpage if it was set on split page (B88076)
**	23-Jun-1998 (jenjo02)
**	    Added lock_value parm to dm0p_lock_page() and dm0p_unlock_page().
**	    Utilize lock_id value in both of those functions.
**	    Consolidate redundant page unfix/unlock code.
**	14-aug-1998 (nanpr01)
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
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
**      08-Oct-2002 (hanch04)
**          Removed dm0m_lcopy,dm0m_lfill.  Use MEcopy, MEfill instead.
**      05-aug-2003 (stial01)
**          dmv_rebtsplit() Fixed DB_ATTS initialization (b110437)
**	19-Jan-2004 (jenjo02)
**	    Added partition number to dm1cxget, dm1cxtget,
**	    dm1cxput, dm1cxtput. TID size is now stored on and
**	    extracted from each page.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      26-sep-05 (stial01)
**          dmv_rebtsplit() Fixed initialization of collID (b115266)
**	13-Apr-2006 (jenjo02)
**	    Handle CLUSTERED pages, range entries without non-key
**	    attributes.
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
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**      14-oct-2009 (stial01)
**          Call dmve_fix_page, dmve_unfix_page
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
** Forward Declarations.
*/

static DB_STATUS dmv_rebtsplit(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *currentpinfo,
			DMP_PINFO	    *siblingpinfo,
			DMP_PINFO	    *datapinfo);

static DB_STATUS dmv_unbtsplit(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *currentpinfo,
			DMP_PINFO	    *siblingpinfo,
			DMP_PINFO	    *datapinfo);


/*{
** Name: dmve_btsplit - The recovery of a Btree Split operation.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the alloc operation.
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
**	14-dec-1992 (jnash & rogerk)
**	    Reduced Logging Project: Written with new 6.5 recovery protocols.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	23-aug-1993 (rogerk)
**	    Fix handling of physical lock flags.  We were always setting
**	    the physical lock variables for the sibling and data pages
**	    even when we were not using physical locks.  This caused us
**	    to attempt to release the physical locks at the end of the
**	    split operation even though we did not hold them (oops).  Fixed
**	    to only set physical flags when physical locks are used.  This
**	    but normally occurred during split redo's in non-fast_commit
**	    online recovery.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass page size to dm1c_getaccessors().
**      22-nov-96 (dilma04)
**          Row Locking Project:
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**	10-jan-1997 (nanpr01)
**	    Changed the spl_vbuf space holder as DMPP_PAGE instead of char to
**	    avoid alignment problem.
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
dmve_btsplit(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_BTSPLIT	*log_rec = (DM0L_BTSPLIT *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->spl_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*current = NULL;
    DMPP_PAGE		*sibling = NULL;
    DMPP_PAGE		*data = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    i4			recovery_action;
    i4			error;
    i4			loc_error;
    i4			page_type = log_rec->spl_pg_type;
    bool		current_page_recover;
    bool		sibling_page_recover;
    bool		data_page_recover;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*currentpinfo = NULL;
    DMP_PINFO		*siblingpinfo = NULL;
    DMP_PINFO		*datapinfo = NULL;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Consistency Check:  check for illegal log records.
	*/
	if ((log_rec->spl_header.type != DM0LBTSPLIT) ||
	    (log_rec->spl_header.length != 
                (sizeof(DM0L_BTSPLIT) - sizeof(DMPP_PAGE) +
                        log_rec->spl_desc_klen + log_rec->spl_page_size)))
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
	status = dmve_fix_tabio(dmve, &log_rec->spl_tbl_id, &tbio);
	if (DB_FAILURE_MACRO(status))
	    break;
	if (status == E_DB_WARN && 
		dmve->dmve_error.err_code == W_DM9660_DMVE_TABLE_OFFLINE)
	{
	    CLRDBERR(&dmve->dmve_error);
	    return (E_DB_OK);
	}

	/* This might be partial tcb, but it will always have basic info */
	t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

	/*
	** Check recovery requirements for this operation.  If partial
	** recovery is in use, then we may not need to recover all
	** the pages touched by the original update.
	**
	** Also, not all split operations require a data page to be allocated,
	** only those to leaf's in a non secondary index.  If the logged data
	** page is page # 0, then there is no data page to recover.
	*/
	current_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->spl_cur_loc_id);
	sibling_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->spl_sib_loc_id);
	data_page_recover = 
		((log_rec->spl_dat_pageno) && 
		 (dmve_location_check(dmve, (i4)log_rec->spl_dat_loc_id)));

	/*
	** Lock/Fix the pages we need to recover in cache for write.
	*/
	if (current_page_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->spl_cur_pageno,
			    &currentpinfo);
	    if (status != E_DB_OK)
		break;
	    current = currentpinfo->page;
	}

	if (sibling_page_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->spl_sib_pageno,
				&siblingpinfo);
	    if (status != E_DB_OK)
		break;
	    sibling = siblingpinfo->page;
	}

	if (data_page_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->spl_dat_pageno,
				&datapinfo);
	    if (status != E_DB_OK)
		break;
	    data = datapinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->spl_pg_type, log_rec->spl_page_size,
		current, dmve->dmve_plv, "CurPage"); 
	    dmve_trace_page_info(log_rec->spl_pg_type, log_rec->spl_page_size,
		sibling, t->tcb_acc_plv, "SibPage"); 
	    dmve_trace_page_info(log_rec->spl_pg_type, log_rec->spl_page_size,
		data, dmve->dmve_plv, "DATA");
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
            if (current && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, current),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, current),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, current),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, current),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, current),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		current = NULL;
	    }
            if (sibling && LSN_GTE(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, sibling),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, sibling),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, sibling),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, sibling),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, sibling),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		sibling = NULL;
	    }
            if (data && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, data), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, data),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, data),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, data),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, data),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		data = NULL;
	    }
	    break;

	case DMVE_UNDO:
	    if (current && (LSN_LT(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, current),
		log_lsn)))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, current),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, current),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, current),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, current), 
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
            if (sibling && LSN_LT(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, current),
		log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, current),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, current),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, current),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, current), 
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
            if (data && LSN_LT(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, data), 
		log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, data),
		    0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, data),
		    0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, data),
		    0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, data),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (status != E_DB_OK || (!current && !sibling && !data))
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->spl_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_rebtsplit(dmve, tbio, 
	    			(current) ? currentpinfo : NULL, 
				(sibling) ? siblingpinfo : NULL, 
				(data) ? datapinfo : NULL);
	    break;

	case DMVE_UNDO:
	    status = dmv_unbtsplit(dmve, tbio,
	    			(current) ? currentpinfo : NULL, 
				(sibling) ? siblingpinfo : NULL, 
				(data) ? datapinfo : NULL);
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
    ** Unfix/Unlock the updated pages.  No need to force them to disk - they
    ** will be tossed out through normal cache protocols if Fast
    ** Commit or at the end of the abort if non Fast Commit.
    */
    if (currentpinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, currentpinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (siblingpinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, siblingpinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (datapinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, datapinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    /* 
    ** Release our handle to the table control block.
    */
    if (tbio)
    {
	tmp_status = dmve_unfix_tabio(dmve, &tbio, 0);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (status != E_DB_OK)
	SETDBERR(&dmve->dmve_error, 0, E_DM9633_DMVE_SPLIT);

    return(status);
}

/*{
** Name: dmv_rebtsplit - Redo the Btree Split operation.
**
** Description:
**      This function redoes the effects of a Split Index Page Operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
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
**	    Written for 6.5 recovery project.
**      13-jan-1993 (mikem)
**          Lint directed changes.  Fixed "=" vs. "==" problem in 
**	    dmv_rebtsplit().  Previous to this change errors in 2 calls to 
**	    dm1cxrshift() were ignored.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	  - Fix splits of leafs that have overflow chains.  Check if the
**	    split moved the overflow chain to the new page and if so redo
**	    that operation.
**	  - Update page's split_lsn value so that if we must UNDO operations
**	    on the page after REDOing this one we know when to verify that
**	    the key exists on the same leaf page.
**	  - Added split direction field to the split log record so we can
**	    tell the difference between a split where the splitpos was zero
**	    because duplicates had to be moved to the new sibling and a split
**	    where the splitpos was zero because the leaf was empty (but had
**	    a non-empty overflow chain).  In this latter case the overflow
**	    chain does not move to the new sibling.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxformat.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros. Also pass the page_size parameter to DM1B_OVER macro.
**	    Change btree key per page calculation to account for tuple header.
**	12-dec-1996 (shero03)
**	    The split page is now in the variable part of the log record.
**	09-jan-1997 (nanpr01)
**	    Revert the order as done in previous integration to prevent 
**	    BUS error.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      29-may-1997 (stial01)
**          dmv_rebtsplit: REDO current: Since page cleans are not logged,
**          don't assume logged page is same as current. Copy logged page
**          to current before shifting entries from spl_split_pos.
**      12-jun-97 (stial01)
**          dmv_rebtsplit() Pass new attribute, key parameters to dm1cxformat
**      13-jan-1997 (stial01)
**          Set DMPP_CLEAN on sibpage if it was set on split page (B88076)
**	02-feb-1999 (nanpr01)
**	    Pass the pointer to a pointer in the deallocation routine.
**	    Also include the header for prototype checking and warning.
**      09-feb-1999 (stial01)   
**          dmv_rebtsplit() Added kperpage to log record for btsplit
**	13-dec-04 (inkdo01)
**	    Support union in DM1B_ATTR.
*/
static DB_STATUS
dmv_rebtsplit(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *currentpinfo,
DMP_PINFO	    *siblingpinfo,
DMP_PINFO	    *datapinfo)
{
    DM0L_BTSPLIT	*log_rec = (DM0L_BTSPLIT *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->spl_header.lsn; 
    DMPP_PAGE		*split_page;
    DMPP_PAGE		*local_page;
    DM_TID		tid;
    DM_TID		infinity;
    i4		ptype;
    i4		keylen;
    i4		page_type = log_rec->spl_pg_type;
    char		*desc_key;
    char		*range_key;
    DB_STATUS		status = E_DB_OK;
#define DMVE_MAX_ATTS 32
    DB_ATTS             db_atts[DMVE_MAX_ATTS];
    DB_ATTS             *db_atts_ptrs[DMVE_MAX_ATTS];
    DB_ATTS             *db_keys_ptrs[DMVE_MAX_ATTS];
    char		gen_att_names[32*DMVE_MAX_ATTS];
    DB_ATTS             **atts; 
    DB_ATTS             **keys;
    DB_ATTS             *atr;           /* Ptr to current attribute */
    DM1B_ATTR           attdesc;
    i4             page_attcnt;
    i4             attno;
    i4             keycnt;
    DM_OBJECT           *misc_buffer = NULL;
    i2			TidSize;
    i2			abs_type;
    i4			*err_code = &dmve->dmve_error.err_code;
    DMP_TCB		*t;
    LG_LRI		lri;
    DMPP_PAGE		*current = NULL;
    DMPP_PAGE		*sibling = NULL;
    DMPP_PAGE		*data = NULL;
    i4			attnmsz;
    char		*attnames;
    char		*nextattname;

    CLRDBERR(&dmve->dmve_error);

    if ( currentpinfo )
        current = currentpinfo->page;
    if ( siblingpinfo )
        sibling = siblingpinfo->page;
    if ( datapinfo )
        data = datapinfo->page;

    /* This might be partial tcb, but it will always have basic info */
    t = (DMP_TCB *)((char *)tabio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if ((current == NULL) && (sibling == NULL) && (data == NULL))
	return (E_DB_OK);

    split_page = (DMPP_PAGE *)&log_rec->spl_vbuf;
    desc_key = (char *)&log_rec->spl_vbuf + log_rec->spl_page_size;

    /* Calculate maximum keys that will fit on a page */
    if (page_type == TCB_PG_V1)
	page_attcnt = 0;
    else
	page_attcnt = DM1B_VPT_GET_BT_ATTCNT(page_type, split_page);

    if (page_attcnt > 0)
    {
	atts = &db_atts_ptrs[0];
	keys = &db_keys_ptrs[0];
	attnames = gen_att_names;
	nextattname = attnames;
	for (attno = 0; attno < DMVE_MAX_ATTS; attno++)
	{
	    db_atts_ptrs[attno] = &db_atts[attno];
	}

	attnmsz = (page_attcnt) * 32; /* generated att names */
	if (page_attcnt > DMVE_MAX_ATTS)
	{
	    status = dm0m_allocate((i4)( sizeof(DMP_MISC) +
		(page_attcnt * sizeof(DB_ATTS)) +
		(page_attcnt * sizeof(DB_ATTS *)) +
		(page_attcnt * sizeof(DB_ATTS *)) +
		attnmsz),
		(i4)0, (i4)MISC_CB, (i4)MISC_ASCII_ID,
		(char *)NULL, (DM_OBJECT **)&misc_buffer, &dmve->dmve_error);
	    if (status != OK)
		return (E_DB_ERROR);
	    ((DMP_MISC *)misc_buffer)->misc_data = (char *)misc_buffer +
		    sizeof(DMP_MISC);
	    atts = (DB_ATTS **)((char *)misc_buffer + sizeof(DMP_MISC));
	    keys = (DB_ATTS **)atts + page_attcnt;
	    atr = (DB_ATTS *)(keys + page_attcnt);
	    attnames = (char *)(atr + page_attcnt);
	    nextattname = attnames;
	    for (attno = 0; attno < page_attcnt; attno++)
	    {
		atts[attno] = atr++;
	    }
	}

	for (attno = 0, keycnt = 0; attno < page_attcnt; attno++)
	{
	    atr = atts[attno];

	    /*
	    ** The only fields we need to initialize in DB_ATTS for
	    ** dm1cxformat are type, length, precision and name,
	    ** but zero out entire DB_ATTS anyway
	    */
	    MEfill(sizeof(*atr), (u_char)0, atr);

	    /*
	    ** Init attribute name so dm1cxformat can compare atts with keys
	    */
	    atr->attnmstr = nextattname;
	    STprintf(atr->attnmstr, "%d", attno);
	    atr->attnmlen = STlength(atr->attnmstr);
	    nextattname += (atr->attnmlen + 1);

	    DM1B_VPT_GET_ATTR(page_type, log_rec->spl_page_size, 
			split_page, (attno+1), &attdesc);
	    atr->type = attdesc.type;
	    atr->length = attdesc.len;
	    atr->precision = 0;
	    atr->collID = -1;
	    abs_type = abs(atr->type);
	    if (abs_type == DB_CHA_TYPE || abs_type == DB_CHR_TYPE 
		|| abs_type == DB_VCH_TYPE || abs_type == DB_TXT_TYPE 
		|| abs_type == DB_NCHR_TYPE || abs_type == DB_NVCHR_TYPE)
		atr->collID = attdesc.u.collID;
	    else atr->precision = attdesc.u.prec;
	    if (attdesc.key)
		keys[keycnt++] = atts[attno];
	}
    }

    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, split_page) & DMPP_LEAF)
	ptype = DM1B_PLEAF;
    else if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, split_page) & DMPP_SPRIG)
	ptype = DM1B_PSPRIG;
    else
	ptype = DM1B_PINDEX;

    /* Extract the TID size from the page */
    TidSize = DM1B_VPT_GET_BT_TIDSZ_MACRO(page_type, split_page);

    if ((local_page = (DMPP_PAGE *)dm0m_pgalloc(tabio->tbio_page_size)) == 0)
    {
	if (misc_buffer)
	    dm0m_deallocate((DM_OBJECT **)&misc_buffer);
	return (E_DB_ERROR);
    }

    /*
    ** Mutex the pages while updating them.
    */
    if (current)
	dmveMutex(dmve, currentpinfo);
    if (sibling)
	dmveMutex(dmve, siblingpinfo);
    if (data)
	dmveMutex(dmve, datapinfo);

    /*
    ** Redo the Split operation.
    */
    for (;;)
    {

	/*
	** The "current" page is the one which is being split.
	**
	** Move all entries past the split position off of the page.
	** If the page is a LEAF then we must also update the high key range.
	*/
	if (current)
	{
	    /*
	    ** Format our local page as an empty index page so that we can
	    ** simulate the shift action.  The contents of the local page
	    ** will be discarded since we are only recovering the source
	    ** page of the split.
	    */
	    status = dm1cxformat(page_type, log_rec->spl_page_size,
		local_page, dmve->dmve_plv, log_rec->spl_cmp_type,
		log_rec->spl_kperpage, log_rec->spl_klen, 
		atts, page_attcnt, keys, keycnt,
		log_rec->spl_sib_pageno, ptype, TCB_BTREE, TidSize,
		DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, split_page) & DMPP_CLUSTERED
		);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E5_BAD_INDEX_FORMAT, (DMP_RCB*)NULL, 
		   local_page, page_type, log_rec->spl_page_size, 0);
		SETDBERR(&dmve->dmve_error, 0, E_DM9635_REDO_SPLIT);
		break;
	    }

	    /*
	    ** Since page cleans are not logged, don't assume that the
	    ** logged page is the same as the current.
	    ** Copy the logged page to the current before shifting
	    ** entries from spl_split_pos.
	    */
	    MEcopy((PTR) split_page, tabio->tbio_page_size, (PTR) current);

	    status = dm1cxrshift(page_type, log_rec->spl_page_size,
			current, local_page, log_rec->spl_cmp_type, 
			log_rec->spl_split_pos, log_rec->spl_klen + TidSize);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9635_REDO_SPLIT);
		break;
	    }

	    /*
	    ** If the page being split is a LEAF then update its RANGE keys.
	    ** Since splits are always to the right, only the high range key
	    ** must be updated.
	    */
	    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, current) & DMPP_LEAF)
	    {
		infinity.tid_i4 = 0;
		status = dm1cxreplace(page_type, log_rec->spl_page_size,
		    current, DM1C_DIRECT, log_rec->spl_cmp_type,
		    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, DM1B_RRANGE,
		    desc_key, log_rec->spl_desc_klen, 
		    &infinity, (i4)0);
		if (status != E_DB_OK)
		{
		    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
			current, page_type, log_rec->spl_page_size, 
			DM1B_RRANGE);
		    SETDBERR(&dmve->dmve_error, 0, E_DM9635_REDO_SPLIT);
		    break;
		}
	    }

	    /*
	    ** Other leaf page processing:
	    **
	    **      Update the page's next_page pointer to point to the newly
	    **      created sibling page.
	    **
	    **      If the split moved the leaf page's overflow chain then we
	    **      have to redo this action.  This is currently indicated by
	    **      having a zero split position.  It is assumed that the only
	    **      way to split at postion zero is when an attempt is made to
	    **      add a new row to a leaf that is full of duplicate 
	    **	    rows.  When the split direction is to the LEFT then
	    **      the new row is less than the duplicate key.  All the
	    **      duplicate keys are moved to a new leaf leaving the orignal
	    **      leaf empty and ready for the new key.  In these duplicate
	    **      splits, the overflow chain (if any) must move with the
	    **      split.
	    */
	    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, current) & DMPP_LEAF)
	    {
		DM1B_VPT_SET_BT_NEXTPAGE_MACRO(page_type, current,
		    log_rec->spl_sib_pageno);

		if ((log_rec->spl_split_pos == 0) &&
		    (log_rec->spl_split_dir == DM1B_LEFT))
		{
		    DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, current,0);
		}
	    }

	    /* Construct LG_LRI from log record */
	    DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
	    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, current, DMPP_MODIFY);
	    DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, current, &lri);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(page_type, current, *log_lsn);
	}

	/*
	** Format the new sibling page as a btree index and copy over the
	** entries from the logged split page.  If the page is a LEAF then
	** set its range key values.
	*/
	if (sibling)
	{
	    /*
	    ** Format sibling page.
	    */
	    status = dm1cxformat(page_type, log_rec->spl_page_size,
		sibling, dmve->dmve_plv, log_rec->spl_cmp_type,
		log_rec->spl_kperpage, 
		log_rec->spl_klen, 
		atts, page_attcnt, keys, keycnt,
		log_rec->spl_sib_pageno, ptype, TCB_BTREE, TidSize,
		DM1B_VPT_GET_PAGE_STAT_MACRO(page_type,
			split_page) & DMPP_CLUSTERED);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E5_BAD_INDEX_FORMAT, (DMP_RCB*)NULL,
		    sibling, page_type, log_rec->spl_page_size, 0);
		SETDBERR(&dmve->dmve_error, 0, E_DM9635_REDO_SPLIT);
		break;
	    }

	    /*
	    ** Copy the entries after the split position from the logged split
	    ** page to this new sibling page.
	    **
	    ** Make a copy of the logged split page and call the general btree
	    ** split routine which will update both sides of the split (the
	    ** new and old pages).  We will discard the old page (left side
	    ** of the split) since we are only recovering the new page here.
	    */
	    MEcopy((PTR) split_page, tabio->tbio_page_size,
		   (PTR) local_page);
	    status = dm1cxrshift(page_type, log_rec->spl_page_size,
			local_page, sibling, log_rec->spl_cmp_type, 
			log_rec->spl_split_pos, log_rec->spl_klen + TidSize);
	    if (status != E_DB_OK)
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9635_REDO_SPLIT);
		break;
	    }

	    /*
	    ** Update the page's range keys if it is a LEAF.
	    ** Since splits are aways performed to the right, the page's
	    ** low range key should be the descriminator key and the high
	    ** range the same as the old split page's high range.
	    */
	    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, sibling) & DMPP_LEAF)
	    {
		infinity.tid_i4 = 0;
		status = dm1cxreplace(page_type, log_rec->spl_page_size,
		    sibling, DM1C_DIRECT, log_rec->spl_cmp_type,
		    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
		    DM1B_LRANGE,
		    desc_key, log_rec->spl_desc_klen, 
		    &infinity, (i4)0);
		if (status != E_DB_OK)
		{
		    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
			sibling, page_type, log_rec->spl_page_size,
			DM1B_LRANGE);
		    SETDBERR(&dmve->dmve_error, 0, E_DM9635_REDO_SPLIT);
		    break;
		}

		dm1cxrecptr(page_type, log_rec->spl_page_size, 
				split_page, DM1B_RRANGE, &range_key);
		/* Sanity check old split formats */
		if ( (keylen = log_rec->spl_range_klen) == 0 )
		    keylen = log_rec->spl_klen;
		if (log_rec->spl_cmp_type == DM1CX_COMPRESSED)
		{
		    dm1cx_klen(page_type, log_rec->spl_page_size,
				split_page, DM1B_RRANGE, &keylen);
		}
		dm1cxtget(page_type, log_rec->spl_page_size,
			    split_page, DM1B_RRANGE, 
			    &infinity, (i4*)NULL); 

		status = dm1cxreplace(page_type, log_rec->spl_page_size,
		    sibling, DM1C_DIRECT, log_rec->spl_cmp_type,
		    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, DM1B_RRANGE,
		    range_key, keylen, &infinity, (i4)0);
		if (status != E_DB_OK)
		{
		    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
			sibling, page_type, log_rec->spl_page_size,
			DM1B_RRANGE);
		    SETDBERR(&dmve->dmve_error, 0, E_DM9635_REDO_SPLIT);
		    break;
		}
	    }

	    /*
	    ** Other leaf page processing:
	    **
	    **      Update the page's nexpage pointer with the value to which
	    **      the old split page used to point.
	    **
	    **      If the split moved the leaf page's overflow chain then we
	    **      have to redo this action.  This is currently indicated by
	    **      having a zero split position.  It is assumed that the only
	    **      way to split at postion zero is when an attempt is made to
	    **      add a new row to a leaf that is full of duplicate 
	    **	    rows.  When the split direction is to the LEFT then
	    **      the new row is less than the duplicate key.  All the
	    **      duplicate keys are moved to a new leaf leaving the orignal
	    **      leaf empty and ready for the new key.  In these duplicate
	    **      splits, the overflow chain (if any) must move with the
	    **      split.
	    */
	    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, sibling) & DMPP_LEAF)
	    {
	    	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(page_type, sibling, 
		    DM1B_VPT_GET_BT_NEXTPAGE_MACRO(page_type, split_page));

		if ((log_rec->spl_split_pos == 0) &&
		    (log_rec->spl_split_dir == DM1B_LEFT))
		{
		    DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, sibling,
			DM1B_VPT_GET_PAGE_OVFL_MACRO(page_type, split_page));
		}
	    }


	    /*
	    ** If a data page was allocated, then set the page's data pointer.
	    */
	    if (log_rec->spl_dat_pageno)
		DM1B_VPT_SET_BT_DATA_MACRO(page_type, sibling, 
		    log_rec->spl_dat_pageno);

	    /* We may have moved deleted tuples to sibling, update page_stat */
	    if (((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, split_page)
		    & DMPP_CLEAN) != 0) )
		DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, sibling, DMPP_CLEAN);

	    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, sibling, DMPP_MODIFY);
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, sibling, *log_lsn);
	    DM1B_VPT_SET_BT_SPLIT_LSN_MACRO(page_type, sibling, *log_lsn);
	}

	/*
	** Mark the data page as an associated page.
	*/
	if (data)
	{
	    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, data, 
		DMPP_DATA | DMPP_ASSOC | DMPP_PRIM | DMPP_MODIFY);
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, data, *log_lsn);
	}

	break;
    }

    if (current)
	dmveUnMutex(dmve, currentpinfo);
    if (sibling)
	dmveUnMutex(dmve, siblingpinfo);
    if (data)
	dmveUnMutex(dmve, datapinfo);
    
    dm0m_pgdealloc((char **)&local_page);

    if (misc_buffer)
	dm0m_deallocate((DM_OBJECT **)&misc_buffer);

    return(status);
}

/*{
** Name: dmv_unbtsplit - UNDO of a Btree Split operation.
**
** Description:
**      This function undoes the effects of a Split Index Page Operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fhdr				Table's FHDR page.
**	fmap				Table's FMAP page.
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
**	14-dec-1992 (jnash & rogerk)
**	    Written for 6.5 recovery project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-jul-1993 (rogerk)
**	    Added split direction field to the split log record.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**	12-dec-1996 (shero03)
**	    The split page is now in the variable part of the log record.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      22-aug-1997 (stial01)
**          Restore log_rec->spl_page_size bytes from log record
**      09-feb-1999 (stial01)   
**          dmv_unbtsplit() Added kperpage to log record for btsplit
*/
static DB_STATUS
dmv_unbtsplit(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *currentpinfo,
DMP_PINFO	    *siblingpinfo,
DMP_PINFO	    *datapinfo)
{
    DM0L_BTSPLIT	*log_rec = (DM0L_BTSPLIT *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->spl_header.lsn; 
    LG_LSN		lsn; 
    LG_LRI		lri; 
    DB_TAB_NAME		table_name;
    DB_OWN_NAME		table_owner;
    DMPP_PAGE		*split_page;
    char		*desc_key;
    DB_STATUS		status;
    i4		dm0l_flags;
    i4		page_type = log_rec->spl_pg_type;
    i4			*err_code = &dmve->dmve_error.err_code;
    DMP_TCB		*t;
    DMPP_PAGE		*current = NULL;
    DMPP_PAGE		*sibling = NULL;
    DMPP_PAGE		*data = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( currentpinfo )
        current = currentpinfo->page;
    if ( siblingpinfo )
        sibling = siblingpinfo->page;
    if ( datapinfo )
        data = datapinfo->page;

    /* This might be partial tcb, but it will always have basic info */
    t = (DMP_TCB *)((char *)tabio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if ((current == NULL) && (sibling == NULL) && (data == NULL))
	return (E_DB_OK);

    split_page = (DMPP_PAGE *)&log_rec->spl_vbuf;
    desc_key = (char *)&log_rec->spl_vbuf + log_rec->spl_page_size;

    /*
    ** Mutex the pages while updating them.
    */
    if (current)
	dmveMutex(dmve, currentpinfo);
    if (sibling)
	dmveMutex(dmve, siblingpinfo);
    if (data)
	dmveMutex(dmve, datapinfo);

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
    if ((log_rec->spl_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    /*
	    ** Extract previous page change info for current
	    ** No undo DM0LSPLIT if done in mini transaction,
	    ** so LRI in DM0LSPLIT CLR records is probably never used 
	    **
	    ** Undo SPLIT without all pages fixed is probably only possible
	    ** during rollforwarddb
	    */
	    if (current)
	    {
		DM1B_VPT_GET_PAGE_LRI_MACRO(log_rec->spl_pg_type, current, &lri);
	    }
	    else
		DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);

	    dm0l_flags = (log_rec->spl_header.flags | DM0L_CLR);

	    status = dm0l_btsplit(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->spl_tbl_id, &log_rec->spl_tblname, 
		&log_rec->spl_tblowner,
		log_rec->spl_pg_type, log_rec->spl_page_size,
		log_rec->spl_kperpage, log_rec->spl_cmp_type, 
		log_rec->spl_loc_cnt, 
		log_rec->spl_cur_loc_id, log_rec->spl_sib_loc_id, 
		log_rec->spl_dat_loc_id, log_rec->spl_cur_pageno,
		log_rec->spl_sib_pageno, log_rec->spl_dat_pageno,
		log_rec->spl_split_pos, log_rec->spl_split_dir,
		log_rec->spl_klen, 
		(log_rec->spl_range_klen) 	/* Max range entry length */
			? log_rec->spl_range_klen
			: log_rec->spl_klen,
		log_rec->spl_desc_klen, 
		desc_key, split_page, &log_rec->spl_header.lsn, &lri, 
		&dmve->dmve_error);

	    lsn = lri.lri_lsn;  /* for sib, data */

	    if (status != E_DB_OK)
	    {
		if (current) 
		    dmveUnMutex(dmve, currentpinfo);
		if (sibling) 
		    dmveUnMutex(dmve, siblingpinfo);
		if (data) 
		    dmveUnMutex(dmve, datapinfo);

		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
		SETDBERR(&dmve->dmve_error, 0, E_DM9634_UNDO_SPLIT);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are processing recovery of a Split CLR (redo-ing the undo
	** of an allocate) then we don't log a CLR but instead save the LSN
	** of the log record we are processing with which to update the 
	** page lsn's.
	*/
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
        lsn = lri.lri_lsn;
    }

    /*
    ** Undo the split operation.
    ** The split page is restored VIA the logged page image of the page before
    ** the split operation.  The LSN of the page is moved forward following
    ** the restoration.
    **
    ** The sibling and data pages are not really recovered as part of this
    ** split undo as it is assumed that they were recently allocated for
    ** the split and that they will be returned to the table's free space.
    ** The pages will be recovered via their respective ALLOC operations
    ** (their LSN's are moved forward however).
    */

    if (current)
    {
	MEcopy((PTR)split_page, log_rec->spl_page_size, (PTR)current);
	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, current, DMPP_MODIFY);

	/* Set DMPP_CREAD in the CR buffer */
	if (dmve->dmve_flags & DMVE_MVCC)
	    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, current, DMPP_CREAD);

	if (dmve->dmve_logging)
	    DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, current, &lri);
    }

    /*
    */
    if (sibling)
    {
	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, sibling, DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, sibling, lsn);
    }

    /*
    */
    if (data)
    {
        DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, data, DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, data, lsn);
    }

    if (current)
	dmveUnMutex(dmve, currentpinfo);
    if (sibling)
	dmveUnMutex(dmve, siblingpinfo);
    if (data)
	dmveUnMutex(dmve, datapinfo);

    return(E_DB_OK);
}
