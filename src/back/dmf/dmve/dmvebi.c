/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
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
#include    <dmve.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2f.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm0p.h>
#include    <dm0l.h>
#include    <dmftrace.h>


/**
**
**  Name: DMVEBI.C - The Undo recovery of a Page Image log record.
**
**  Description:
**	This file contains the routine for the recovery of a page image record.
**
**          dmve_bipage - The recovery of a page image log record.
**
**
**  History:
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**	 6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Put in safety check for log
**	    records for gateway tables that are accidentally logged.  No log
**	    records should be written for operations on gateway tables.
**	 5-dec-89 (rogerk)
**	    Added fix for ENDFILE during online backup recovery.
**	14-jun-1991 (Derek)
**	    Changed to BI replacement logic not to keep the overflow pointer
**	    from the current page on disk.  The new allocation algorithms allow
**	    for an allocation to be aborted, thus the pointer doesn't have to
**	    be restored.  Also, use buffer manager for all operations don't
**	    use direct DI calls.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	10-sep-1992 (rogerk)
**	    Reduced logging project: changed references to tcb fields to
**	    use new tcb_table_io structure fields.
**	14-dec-1992 (rogerk)
**	    Reduced logging project (phase 2): rewrote routine.
**	28-jan-1993 (jnash)
**	    Online backup fixes.
**	     - Don't bother recovering page if page being recovered 
**	       is beyond the currently allocated eof.
**	     - Don't check for LSN_MISMATCH if dump record being processed --
**	       we don't put LSN's on BI pages.
**	     - Fix page for SCRATCH to handle case where recovered page 
**	       lies between the lpageno (not known to this routine) and 
**	       the physical end of file.
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
**	15-apr-1993 (rogerk)
**	    Don't use SCRATCH option when reading page during normal rollback
**	    processing, only during DUMP processing.  This lets us do the
**	    LSN checks during rollback.
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
**	    Added tracing of page information during recovery.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Support variable page sizes through the bi_page_size field.
**      06-mar-1996 (stial01)
**          Pass bi_page_size to dmve_trace_page_info
**      01-may-1996 (stial01)
**          Adjustments for removal of bi_page from DM0L_BI
**	06-may-1996 (thaju02)
**	    New Page Format Project: change page header references to use
**	    macros.
**      22-nov-1996 (dilma04)
**          Row Locking Project:
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**	10-jan-1997 (nanpr01)
**	    Put back the bi_page back to DM0L_BI.
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
**	    Removed extra code which was locking bi_pageno twice.
**	14-aug-1998 (nanpr01)
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
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
**      14-oct-2009 (stial01)
**          Call dmve_fix_page, dmve_unfix_page
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**/

static DB_STATUS dmv_unbipage(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *pinfo);


/*{
** Name: dmve_bipage - The recovery of a page.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The before image log record.
**	    .dmve_action	    Should only be DMVE_UNDO.
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
**	30-jun-86 (ac)
**          Created for Jupiter.
**	18-nov-88 (rogerk)
**	    Put in check for garbage before image before write to disk.
**	    If bi page number is not correct then don't write the before image.
**	 5-dec-89 (rogerk)
**	    Check if file has successfully been extended to include the
**	    logged page.  In online checkpoint recovery, we may have the case
**	    where this page is not included in the file.  Also, if we allow 
**	    file-extends without actually allocating the space until pages
**	    are flushed then this may occur.  If the page does not belong
**	    to the file, then there is no need to recover it.
**	14-jun-1991 (Derek)
**	    Changed to BI replacement logic not to keep the overflow pointer
**	    from the current page on disk.  The new allocation algorithms allow
**	    for an allocation to be aborted, thus the pointer doesn't have to
**	    be restored.  Also, use buffer manager for all operations don't
**	    use direct DI calls.
**	14-dec-1992 (rogerk)
**	    Reduced logging project (phase 2): rewrote routine.
**	28-jan-1993 (jnash)
**	    Online backup fixes.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	15-apr-1993 (rogerk)
**	    Don't use SCRATCH option when reading page during normal rollback
**	    processing, only during DUMP processing.  This lets us do the
**	    LSN checks during rollback.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	06-mar-1996 (stial01 for bryanp)
**	    Support variable page sizes through the bi_page_size field.
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to macros.
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
dmve_bipage(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_BI		*log_rec = (DM0L_BI *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->bi_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*page = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    i4			recovery_action;
    i4			error;
    i4			loc_error;
    i4			page_type = log_rec->bi_pg_type;
    bool		page_recover;
    bool		dump_processing;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*pinfo = NULL;

    CLRDBERR(&dmve->dmve_error);

    dump_processing = ((log_rec->bi_header.flags & DM0L_DUMP) != 0);

    for (;;)
    {
	/*
	** Consistency Check:  check for illegal log records.
	*/              
	if ((log_rec->bi_header.type != DM0LBI) ||
	    (log_rec->bi_header.length < sizeof(DM0L_BI)))
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
	status = dmve_fix_tabio(dmve, &log_rec->bi_tbl_id, &tbio);
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
	*/
	page_recover = dmve_location_check(dmve, (i4)log_rec->bi_loc_id);

	/*
	** Check if page number is past the current EOF.  If it is, then
	** the BI need not be applied.  We assume here that the database is
	** is being restored to a state when this page did not exist in the
	** table.  We trust that any page that points to this page will also 
	** be restored to a previous version that did not reference it.
	**
	** This case may arrive in recovery from an ONLINE backup operation.
	** The log records to apply during backup may be for records on pages
	** that were added during the backup but were not actually copied
	** during the backup in progress.
	*/
	if (log_rec->bi_pageno > tbio->tbio_lalloc)
	{
	    page_recover = FALSE;

	    /*
	    ** FIX ME XXXXXXXXXXXXX 
	    **
	    ** Page looks like it may be beyond EOF, but we need to DIsense
	    ** the file to make sure that our lalloc value is up-to-date.
	    ** It does not seem possible for us to have an out-of-date
	    ** lalloc that describes the table size as less than it really
	    ** is because that would require this process to have a TCB
	    ** open that reflects the table's state BEFORE the log record
	    ** was written!  It should not be possible in the server (since
	    ** the same TCB would have been used to log the BI) nor is it
	    ** possible in the RCP (where the TCB is not open until after
	    ** recovery begins).  But for completeness we should make sure.
	    */
	}

	if ( !page_recover )
	    break;

	/*
	** Lock/Fix the pages we need to recover in cache for write.
	** During dump processing, specify the SCRATCH option as we do
	** not know for sure that the page on disk will be readable (it may
	** not have been formatted yet on disk).  We can bypass reading in
	** the page since we are just going to overlay it with a page image
	** anyway.
	*/

	/*
	** Note dmve_fix_page will set DM0P_SCRATCH fix_option 
	** if (log_rec->type == DM0LBI && (log_rec->flags & DM0L_DUMP))
	*/

	status = dmve_fix_page(dmve, tbio, log_rec->bi_pageno, &pinfo);
	if (status != E_DB_OK)
	    break;

	page = pinfo->page;

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	    dmve_trace_page_info(log_rec->bi_pg_type, log_rec->bi_page_size,
		page, dmve->dmve_plv, "PAGE");

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
	** Skip LSN check if this is a DUMP record, we don't insert LSNs
	** when logging Before Images as part of online backup protocols.
	*/
	switch (dmve->dmve_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    if (page && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, page),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, page),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, page),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		page = NULL;
	    }
	    break;

	case DMVE_UNDO:
	    /*
	    ** Skip LSN check if this is a DUMP record, we don't insert LSNs
	    ** on dump pages.
	    */
	    if (( ! dump_processing) &&
		(page && (LSN_LT(
		    DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), 
		    log_lsn))))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
		    0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, page),
		    0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, page),
		    0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, page),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (status != E_DB_OK || !page )
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->bi_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_UNDO:
	    status = dmv_unbipage(dmve, tbio, pinfo);
	    break;

	default:
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
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
    if (pinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, pinfo);
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9603_DMVE_BIPAGE);

    return(status);
}

/*{
** Name: dmv_unbipage - UNDO of a Page Image log record.
**
** Description:
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
**	14-dec-1992 (rogerk)
**	    Written for Reduced logging project (phase II).
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	23-may-1994 (andys)
**	    Add cast(s) to quiet qac/lint
**	06-mar-1996 (stial01 for bryanp)
**	    Support variable page sizes through the bi_page_size field.
**	    Pass correct page_size to the dm0l_bi() call.
**	    Use dm0m_lcopy to support 64K page sizes.
**      01-may-1996 (stial01)
**          Adjustments for removal of bi_page from DM0L_BI
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to macros.
**	10-jan-1997 (nanpr01)
**	    Put back the bi_page back to DM0L_BI.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_unbipage(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo)
{
    DM0L_BI		*log_rec = (DM0L_BI *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->bi_header.lsn; 
    LG_LSN		lsn; 
    DB_TAB_NAME		table_name;
    DB_OWN_NAME		table_owner;
    DB_STATUS		status;
    i4		dm0l_flags;
    i4		mask;
    i4		page_type = log_rec->bi_pg_type;
    i4		local_err;
    i4		*err_code = &dmve->dmve_error.err_code;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If recovery was found to be unneeded then we can just return.
    */
    if (page == NULL)
	return (E_DB_OK);

    /*
    ** Mutex the pages while updating them.
    */
    dmveMutex(dmve, pinfo);

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
    if (((log_rec->bi_header.flags & (DM0L_CLR | DM0L_DUMP)) == 0) &&
	(dmve->dmve_logging))
    {
	dm0l_flags = (log_rec->bi_header.flags | DM0L_CLR);

	status = dm0l_bi(dmve->dmve_log_id, dm0l_flags, 
	    &log_rec->bi_tbl_id, &log_rec->bi_tblname, &log_rec->bi_tblowner, 
	    log_rec->bi_pg_type, log_rec->bi_loc_cnt, log_rec->bi_loc_id,
	    log_rec->bi_operation, log_rec->bi_pageno, log_rec->bi_page_size,
	    &log_rec->bi_page, log_lsn, &lsn, &dmve->dmve_error);
	if (status != E_DB_OK)
	{
	    /*
	     * Bug56702: return logfull indication.
	     */
	    dmve->dmve_logfull = dmve->dmve_error.err_code;

	    dmveUnMutex(dmve, pinfo);
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0); 

	    SETDBERR(&dmve->dmve_error, 0, E_DM965B_UNDO_BIPAGE);
	    return(E_DB_ERROR);
	}
    }
    else
    {
	/*
	** If we are processing recovery of a BI CLR (redo-ing the undo
	** of a before image) then we don't log a CLR but instead save the LSN
	** of the log record we are processing with which to update the 
	** page lsn's.
	*/
	lsn = *log_lsn;
    }

    /*
    ** Restore the contents of the page from the before image.
    */
    MEcopy((char *)&log_rec->bi_page,
		log_rec->bi_page_size, (char *)page);
    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);

    /*
    ** Move page lsn forward unless we are doing DUMP processing.
    */
    if (((log_rec->bi_header.flags & DM0L_DUMP) == 0) && (dmve->dmve_logging))
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, page, lsn);

    /*
    ** Since this process may have changed the type of page, we must inform
    ** the buffer manager of its new page type.
    */
    mask = (DMPP_FHDR|DMPP_FMAP|DMPP_DATA|DMPP_LEAF|DMPP_INDEX|DMPP_CHAIN);

    dm0p_pagetype(tabio, page, dmve->dmve_log_id, 
	(DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, page) & mask));

    dmveUnMutex(dmve, pinfo);

    return(E_DB_OK);
}
