/*
** Copyright (c) 1986, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <me.h>
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
#include    <dmve.h>
#include    <dm0p.h>
#include    <dm0m.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmd.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVEREP.C - The recovery of a replace record operation.
**
**  Description:
**	This file contains the routine for the recovery of a replace record 
**	operation.
**
**          dmve_rep - The recovery of a replace record operation.
**
**  History:
**      30-jun-86 (ac)    
**          Created new for Jupiter.
**	15-Jun-88 (rogerk)
**	    Added REDO processing.
**	27-jul-88 (greg for sandyh)
**          DMVE_DO action: set putflags argument of dmv_put() to
**		DMVE_DUPLICATES when appropriate--fixes bug 2995
**	24-Apr-89 (anton)
**	    Added local collation support
**	15-aug-89 (rogerk)
**	    Added support for Non-SQL Gateway.  Put in safety check for log
**	    records for gateway tables that are accidentally logged.  No log
**	    records should be written for operations on gateway tables.
**	28-Jul-90 (mikem)
**	    bug # 32051 - Bad redo recovery of cbtree non-key replaces.
**	    In the case of btree indexes redo recovery did not take into account
**	    changes to the index page caused by non-key replaces (ie. when 
**	    a compressed tuple is replaced by a larger tuple).  To fix this
**	    changed the "update index" requirement in dmve_rep from 
**	    (if key_changed) to (if (key_changed or tid changed)).
**	14-jun-1991 (Derek)
**	    Made changes related to changes in access method specific interface
**	    changes.
**	25-sep-1991 (mikem) integrated following change: 16-Oct-90 (walt)
**	    Fix bug # 33443 - bad undo of a replace on a btree secondary
**	    index where the replaced column was not part of the key.
**	25-sep-1991 (mikem) integrated following change: 19-nov-90 (rogerk)
**	    Changed to use the rcb's adf control block rather than allocating
**	    local adf cb's off of the stack.  This was done as part of the DBMS
**	    timezone changes.
**	25-sep-1991 (mikem) integrated following change: 14-jan-1991 (bryanp)
**	    Added support for Btree index compression: new arguments to
**	    dm1bxinsert() and dm1bxdelete().
**	8-dec-1991 (bryanp)
**	    B41204: one more fix for undoing a replace on a secondary index,
**	    and also removed an optimization which was causing wrong results.
**	7-july-1992 (jnash)
**	    Add DMF function prototypes.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (jnash & rogerk)
**	    Reduced Logging Project: Completely rewritten with new 6.5 
**	    recovery protocols.
**	17-dec-1992 (rogerk)
**	    Fixed problem that left pages fixed in redo recovery.
**	11-jan-1993 (jnash)
**	    Do not include new row in CLR log record, do not do tuple
**	    exists check if new row not logged.
**	18-jan-1993 (rogerk)
**	    Fixed problem in REDO of replace CLR's.  Make sure that we
**	    call the correct routine (dmv_unrep) when redo'ing a replace CLR.
**	26-jan-1993 (jnash)
**	    Fix bug introduced by 11-jan change -- preserve record size and
**	    compare sizes.
**	15-mar-1993 (rogerk & jnash)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**	    Fix xDEBUG "no space for tuple at tid" code.
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
**      11-aug-93 (ed)
**          added missing includes
**	20-sep-1993 (rogerk)
**	    Final support added for non-journaled tables.  Deletions of core
**	    catalog rows are always journaled while insertions of core catalog
**	    rows are never journaled.  Full explanation of non journaled table 
**	    handling given in DMRFP.
**	18-oct-1993 (jnash)
**	  - Add support for replace log record compresssion.  New record 
**	    images are compressed by default, old record images 
**	    compressed by tracepoint.  Compression is accomplished by
**	    logging only the bytes that change.  Old row compression
**	    is not supported for core catalogs, or for replace 
**	    operations on which old and new rows do not reside
**	    on the same page (to allow for partial recovery).
**	  - Dump page contents on certain errors.
**	09-dec-1993 (jnash)
**	    Fix problem where if error detected prior to dynamic memory 
**	    allocation, dm0m_deallocate() AVs.
**	11-dec-1993 (rogerk)
**	    Changed module to reflect new replace log record compression
**	    algorithms.  Use new rep_odata_len and rep_ndata_len.  Also, use
**	    rep_orec_size and rep_nrec_size to allocate tuple buffers rather
**	    than MAXTUP.
**	    Added dmd.h, dm0m.h includes and casted parameters to dmd_log
**	    calls to fix compiler warnings.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      23-may-1994 (mikem)
**          Bug #63556
**          Changed interface to dm0l_rep() so that a replace of a record only
**          need call dm0l_repl_comp_info() this routine once.  Previous to 
**          this change the routine was called once for LGreserve, and once for
**          dm0l_replace().
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size as the page_size argument to dmpp_reallocate.
**	    Pass tbio_page_size as the page_size argument to dmpp_reclen.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to macros.
**      22-jul-1996 (ramra01 for bryanp)
**          Alter Table support.
**          Pass row_version argument to dm0l_rep.
**          Pass log_rec->rep_nrow_version to dmpp_put when redoing replace.
**      19-Nov-1996 (hanch04)
**          set row_version for dmv_reput.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**          Do not get page lock if row locking.
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**      10-mar-97 (stial01)
**          dmve_rep() If IX page lock, mutex (OLD) page before looking at it
**          dmve_rep() When row locking OLD and NEW page must be same
**          dmv_rep: Clean ALL deleted tuples if X page lock. (old and new pgs)
**          dmv_rerep() Space is reclaimed when redoing DELETE
**          dmv_unrep() Space is not reclaimed if IX page lock held
**            (We should never be row locking for non in place REPLACE)
**      07-apr-97 (stial01)
**          dmv_unrep() Don't get mutex if we already have it
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      29-may-1997 (stial01)
**          Use dm0p_lock_buf_macro to lock buffer to reduce duration dmve  
**          holds buffer mutex.
**      18-jun-1997 (stial01)
**          dmve_rep() Request IX page lock if row locking.
**      30-jun-97 (dilma04)
**          Bug 83301. Set DM0P_INTENT_LK fix action if row locking.
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_ufx_tabio_cb() calls.
**      28-may-1998 (stial01)
**          Support VPS system catalogs, changed parameters for dmve_clean.
**	23-Jun-1998 (jenjo02)
**	    Added lock_value parm to dm0p_lock_page() and dm0p_unlock_page().
**	    Utilize lock_id value in both of those functions.
**	    Consolidate redundant page unfix/unlock code.
**      07-jul-1998 (stial01)
**          New update_mode if row locking, changed paramater to dmve_clean
**	14-aug-1998 (nanpr01)
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
**	01-dec-1998 (nanpr01)
**	    Get rid of DMPP_TUPLE_INFO structure. Use DMPP_TUPLE_HDR instead.
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
**      21-dec-1999 (stial01)
**          Page cleans only before put.
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_rep, eliminating need for dmve_name_unpack().
**      05-jun-2000 (stial01)
**          Set DM1C_ROWLK in update flags when physical page locking (b101708)
**      25-may-2000 (stial01)
**          Changed criteria for setting reclaim_space
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Arg changes for page accessors (Variable Page Type SIR 102955)
**          Pass pg type to dmve_trace_page_info
**      01-may-2001 (stial01)
**          Fixed parameters to ule_format (B104618)
**	05-Mar-2002 (jenjo02)
**	    Add IISEQUENCE table to the list of un-logged rows
**	    to only prefix-check.
**      10-feb-2004 (stial01)
**          Support for 256K rows, rows spanning pages.
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**      21-feb-2005 (stial01)
**          Cross integrate 470401 (inifa01) for INGSRV2569 b111886
**          New flag DM1C_DMVE_COMP passed to dmpp_delete data is compressed
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
**      14-oct-2009 (stial01)
**          Call dmve_fix_page, dmve_unfix_page
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs, move consistency check to dmveutil
**      29-Apr-2010 (stial01)
**          Use new routintes to compare rows in iirelation, iisequence
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
*/

static DB_STATUS	dmv_rerep(
				DMVE_CB             *dmve,
				DMP_TABLE_IO	    *tabio,
				DMP_PINFO	    *old_pinfo,
				DMP_PINFO	    *new_pinfo,
				char	    	    *old_row_buf,
				char	    	    *new_row_buf);

static DB_STATUS	dmv_unrep(
				DMVE_CB             *dmve,
				DMP_TABLE_IO	    *tabio,
				DMP_PINFO	    *old_pinfo,
				DMP_PINFO	    *new_pinfo,
				char	    	    *old_row_buf,
				char	    	    *new_row_buf);


/*{
** Name: dmve_rep - The recovery of a replace record operation.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the replace operation.
**	    .dmve_action	    Should be DMVE_DO, DMVE_REDO, or DMVE_UNDO(
**				    for btree leaf page only)
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
**	17-dec-1992 (rogerk)
**	    Fixed problem that left pages fixed in redo recovery.  Check
**	    proper old_page_ptr and new_page_ptr values to decide whether
**	    to unfix pages, not the temporary old_page, new_page values.
**	18-jan-1993 (rogerk)
**	    Fixed problem in REDO of replace CLR's.  Make sure that we
**	    call the correct routine (dmv_unrep) when redo'ing a replace CLR.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	18-oct-1993 (jnash)
**	  - Add support for replace log record compresssion.
**	  - Dump page contents on certain errors.
**	08-dec-1993 (jnash)
**	    Fix problem where if error detected prior to dynamic memory 
**	    allocation, dm0m_deallocate() AVs.
**	11-dec-1993 (rogerk)
**	    Changed routine to reflect new replace log record compression
**	    algorithms.  Use new rep_odata_len and rep_ndata_len.  Also, use
**	    rep_orec_size and rep_nrec_size to allocate tuple buffers rather
**	    than MAXTUP.
**	06-mar-1996 (stial01 for bryanp)
**	    Remove DB_MAXTUP dependencies in favor of DM_MAXTUP, the internal
**	    DMF maximum tuple size.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support: 
**		Change page header references to macros.
**		Pass page size to dm1c_getaccessors().
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Do not get page lock if row locking.
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      18-jun-1997 (stial01)
**          dmve_rep() Request IX page lock if row locking.
**      30-jun-97 (dilma04)
**          Bug 83301. Set DM0P_INTENT_LK fix action if row locking.
**      11-sep-1997 (thaju02) bug#85156 - on recovery lock extension
**          table with DM2T_S.  Attempting to lock table with DM2T_IX
**          causes deadlock, dbms resource deadlock error during recovery,
**          pass abort, rcp resource deadlock error during recovery,
**          rcp marks database inconsistent.
**       8-Oct-2003 (wanfr01)
**          Bug 111057, INGSRV 2542
**          The fact that REDO is allowed to skip records for non-journaled
**          tables means the UNDO has a risk of E_DM9665.  Ignore E_DM9665
**          when doing UNDO for non-journaled tables.
**      23-feb-2004 (thaju02) Bug 111470 INGSRV2635
**          For rollforwarddb -b option, do not compare the LSN's on the
**          page to that of the log record.
**	01-Dec-2004 (jenjo02)
**	    Added setting of DM0P_TABLE_LOCKED_X to fix action,
**	    inexplicitly missed in fix for bug 108074.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
dmve_rep(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_REP		*log_rec = (DM0L_REP *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->rep_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*old_page = NULL;
    DMPP_PAGE		*new_page = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4 		mem_needed;
    i4		page_type = log_rec->rep_pg_type;
    DM_OBJECT 		*mem_ptr = 0;
    bool		opage_recover;
    bool		npage_recover;
    char		*newrow_buf;
    char		*oldrow_buf = NULL;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*old_pinfo = NULL;
    DMP_PINFO		*new_pinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->rep_header.type != DM0LREP)
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
	status = dmve_fix_tabio(dmve, &log_rec->rep_tbl_id, &tbio);
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
	opage_recover = dmve_location_check(dmve, 
					(i4)log_rec->rep_ocnf_loc_id);
	npage_recover = dmve_location_check(dmve, 
					(i4)log_rec->rep_ncnf_loc_id);

	/*
	** If the old and new pages are the same page, then we only need
	** to lock/fix/recovery one of them.
	** 
	** When row locking we can only do in place replaces
	** Otherwise the replace would have been executed as a delete,put
	*/
	if (log_rec->rep_otid.tid_tid.tid_page == 
					log_rec->rep_ntid.tid_tid.tid_page)
	{
	    npage_recover = FALSE;
	}
	else if (log_rec->rep_header.flags & (DM0L_ROW_LOCK | DM0L_CROW_LOCK))
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM93F5_ROW_LOCK_PROTOCOL);
	    status = E_DB_ERROR;
	    break;
	}

	/*
	** Lock/Fix the pages we need to recover in cache for write.
	*/
	if (opage_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->rep_otid.tid_tid.tid_page,
					&old_pinfo);
	    if (status != E_DB_OK)
		break;
	    old_page = old_pinfo->page;
	}

	if (npage_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->rep_ntid.tid_tid.tid_page,
					&new_pinfo);
	    if (status != E_DB_OK)
		break;
	    new_page = new_pinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->rep_pg_type, log_rec->rep_page_size,
		old_page, dmve->dmve_plv, "OldDATA");
	    dmve_trace_page_info(log_rec->rep_pg_type, log_rec->rep_page_size,
		new_page, dmve->dmve_plv, "NewDATA");
	}

	/*
	** Compare the LSN on the page with that of the log record
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
            if (old_page && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, old_page),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, old_page),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, old_page),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, old_page),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, old_page),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		old_page = NULL;
	    }

            if (new_page && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, new_page),
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, new_page),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, new_page),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, new_page),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, new_page),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		new_page = NULL;
	    }
	    break;

	case DMVE_UNDO:
            if (old_page && LSN_LT(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, old_page),
		log_lsn))
	    {
	       /* Journal records for non-journaled tables are not necessarily redone.*/  
	       if (!(log_rec->rep_header.flags & DM0L_NONJNL_TAB))
               {
		   uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, old_page),
		    0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, old_page),
		    0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, old_page),
		    0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, old_page),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		   SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		   status = E_DB_ERROR;
		}
	    }

            if (new_page && LSN_LT(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, new_page),
		log_lsn))
	    {
	       /* Journal records for non-journaled tables are not necessarily redone.*/  
	       if (!(log_rec->rep_header.flags & DM0L_NONJNL_TAB))
               {
		   uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, 0, NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, new_page),
		    0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, new_page),
		    0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, new_page),
		    0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, new_page),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		   SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		   status = E_DB_ERROR;
	       }	
	    }
	    break;
	}
	if (status != E_DB_OK || (!old_page && !new_page))
	    break;

	/*
	** Allocate a buffer for the compressed new row.  Also allocate
   	** buffer for the compressed old row, if necessary.
	*/
	mem_needed = log_rec->rep_nrec_size + sizeof(DMP_MISC);
	if (log_rec->rep_header.flags & DM0L_COMP_REPL_OROW)
	     mem_needed += log_rec->rep_orec_size;

	status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB,
	    (i4)MISC_ASCII_ID, (char *)NULL, &mem_ptr, &dmve->dmve_error);
	if (status != OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
		0, mem_needed);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9636_UNDO_REP);
	    break;
	}

	newrow_buf = (char *)mem_ptr + sizeof(DMP_MISC);
	((DMP_MISC*)mem_ptr)->misc_data = (char*)newrow_buf;
	if (log_rec->rep_header.flags & DM0L_COMP_REPL_OROW)
	{
	     oldrow_buf = 
		(char *)mem_ptr + sizeof(DMP_MISC) + log_rec->rep_nrec_size;
	}

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->rep_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	/*
	** Call appropriate recovery action depending on the recovery type.
	*/
	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_rerep(dmve, tbio, 
	    		(old_page) ? old_pinfo : NULL, 
	    		(new_page) ? new_pinfo : NULL,
			oldrow_buf, newrow_buf);
	    break;

	case DMVE_UNDO:
	    status = dmv_unrep(dmve, tbio,
	    		(old_page) ? old_pinfo : NULL, 
	    		(new_page) ? new_pinfo : NULL,
			oldrow_buf, newrow_buf);
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
    if (old_pinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, old_pinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (new_pinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, new_pinfo);
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

    /*
    ** Deallocate space for the uncompressed log.
    */
    if (mem_ptr != 0) 
	dm0m_deallocate(&mem_ptr);

    if (status != E_DB_OK)
	SETDBERR(&dmve->dmve_error, 0, E_DM960F_DMVE_REP);

    return(status);
}

/*{
** Name: dmv_rerep - Redo the replace of a record 
**
** Description:
**	This routine executes redo recovery of a replace operation.  It
**	re-replaces the old row with the new copy.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      old_page			Pointer to the page for the old row
**      new_page			Pointer to the page for the new row
**	log_record			Pointer to the log record
**	old_row_buf			Buffer for uncompressed old row storage
**					 NULL if old row not compressed
**	new_row_buf			Buffer for uncompressed new row storage
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
**	20-sep-1993 (rogerk)
**	    Handling for non-journaled tables.  During rollforward, we
**	    must be aware that system catalog rows for non-journaled tables
**	    may not exist.  Repeating history is not enforced for these rows.
**	    If a replace operation is requested on a core catalog row that
**	    represents a non-journaled table, ignore row-not-exist errors.
**	    If the row is found to exist, then record in the rollforward action
**	    list that we successfully replaced the row.  Without this action
**	    a subsequent undo of the replace will be bypassed.
**	    Replaces of iirelation rows describing non-journaled tables only
**	    update a subset of the columns in iirelation.
**	18-oct-1993 (jnash)
**	  - Add support for replace log record compresssion.
**	  - Dump page contents on certain errors.
**	11-dec-1993 (rogerk)
**	    Changed routine to reflect new replace log record compression
**	    algorithms.  New parameters added to dm0l_row_unpack routine.
**	    Use new rep_[o/n]data_len field in replace log record.  Moved
**	    unpacking of new and old rows together.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size as the page_size argument to dmpp_reallocate.
**	    Pass tbio_page_size as the page_size argument to dmpp_reclen.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to macros.
**      20-may-1996 (ramra01)
**          Added DMPP_TUPLE_INFO to get/put accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      22-jul-1996 (ramra01 for bryanp)
**          Pass row_version argument to dmpp_get.
**          Pass log_rec->rep_nrow_version to dmpp_put when redoing replace.
**      19-Nov-1996 (hanch04)
**          set row_version for dmv_reput.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-mar-97 (stial01)
**          dmv_rerep() Space is reclaimed when redoing DELETE
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_rerep(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *old_pinfo,
DMP_PINFO	    *new_pinfo,
char	    	    *old_row_buf,
char	    	    *new_row_buf)
{
    DM0L_REP		*log_rec = (DM0L_REP *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->rep_header.lsn; 
    DMPP_PAGE	    	*newrow_page;
    DB_STATUS		status = E_DB_OK;
    i4		record_size;
    i4		page_type = log_rec->rep_pg_type;
    char		*record;
    char		*old_row;
    char		*new_row;
    char		*old_row_log_info;
    char		*new_row_log_info;
    bool		index_bit_changed;
    bool		index_count_changed;
    bool		comp_orow;
    i4		old_index_count = 0;
    RFP_REC_ACTION	*action;
    DMP_RELATION	temprow;
    DB_TAB_ID		reltid;
    i4			temprelstat;
    i4             reclaim_space;
    i4                  update_mode; 
    i4			*err_code = &dmve->dmve_error.err_code;
    DMPP_ACC_PLV	*plv = dmve->dmve_plv;
    DMPP_PAGE		*old_page = NULL;
    DMPP_PAGE		*new_page = NULL;
    bool		rowsdif;
    DMP_TCB		*t;

    CLRDBERR(&dmve->dmve_error);

    if ( old_pinfo )
        old_page = old_pinfo->page;
    if ( new_pinfo )
        new_page = new_pinfo->page;

    /*
    ** If recovery was found to be unneeded to both the old and new pages
    ** then we can just return.
    */
    if ((old_page == NULL) && (new_page == NULL))
	return (E_DB_OK);

    /*
    ** If large pages, the redo-rep (delete old tuple) should not reclaim space
    ** If row locking or physical page locking, don't shift the data on the page
    ** If uncompressed data we never need to shift the data on the page
    */ 
    if (tabio->tbio_page_type != TCB_PG_V1 ||
	log_rec->rep_comptype == TCB_C_NONE ||
	(log_rec->rep_header.flags & DM0L_PHYS_LOCK))
	reclaim_space = FALSE;
    else
	reclaim_space = TRUE;

    /*
    ** Determine if the full row before image was logged.  If not then we have
    ** to extract the row on the page to build the old and new row versions.
    */
    comp_orow = FALSE;
    if (log_rec->rep_header.flags & DM0L_COMP_REPL_OROW)
	comp_orow = TRUE;

    /*
    ** Get pointer to correct page for the new tuple (the one which must be
    ** deleted and replaced with its olf version) for the following consistency
    ** check.  If both the old and new rows belong to the same page, then only
    ** 'old_page' will be passed in, otherwise the new row is put on 'new_page'.
    */
    newrow_page = new_page;
    if (log_rec->rep_ntid.tid_tid.tid_page == 
		log_rec->rep_otid.tid_tid.tid_page)
    {
	newrow_page = old_page;
	new_page = 0;
    }

    /*
    ** Get pointers to new and old log record data segments.
    */
    old_row_log_info = ((char *)log_rec) + sizeof(*log_rec);
    new_row_log_info = old_row_log_info + log_rec->rep_odata_len;

    /*
    ** Consistency Check:
    ** 
    ** Make sure that the old tuple exists before attempting to delete it.
    **
    ** We ignore not-exist situations in rollforwarddb when the tuple is a
    ** system catalog row describing a non-journaled table.  Repeating history
    ** is not fully implemented for system catalogs; rows describing non-
    ** journaled tables are not re-inserted into the core catalogs.
    */
    if (old_page)
    {
	status = (*plv->dmpp_get)(page_type, log_rec->rep_page_size,
	    old_page, &log_rec->rep_otid, &record_size, &record, NULL, NULL, NULL, NULL);
	if (status != E_DB_OK)
	{
	    if ((dmve->dmve_action == DMVE_DO) &&
		(log_rec->rep_header.flags & DM0L_NONJNL_TAB))
	    {
		return (E_DB_OK);
	    }

	    uleFormat(NULL, E_DM966D_DMVE_ROW_MISSING, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6, 
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
		sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
		0, log_rec->rep_otid.tid_tid.tid_page,
		0, log_rec->rep_otid.tid_tid.tid_line,
		0, status);
	    dmd_log(1, (PTR) log_rec, 4096);
	    dmve_trace_page_header(page_type, log_rec->rep_page_size,
			old_page, plv, "DATA");
	    dmve_trace_page_contents(page_type,
	    		log_rec->rep_page_size, old_page, "DATA");
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}

	(*plv->dmpp_reclen)(page_type, log_rec->rep_page_size,
	    old_page, (i4)log_rec->rep_otid.tid_tid.tid_line, &record_size);
    }

    /*
    ** Get pointer to full row before image.  Normally the old row data from
    ** the replace log record can be used.  If the old row is compressed then
    ** it must be reconstructed using the record just read from the page.
    ** (Note that comp_orow can only be true if the old and new rows are on
    ** the same page : enforced by dm0l_rep).
    */
    old_row = old_row_log_info;
    if (comp_orow)
    {
	dm0l_row_unpack(record, record_size, 
	    old_row_log_info, log_rec->rep_odata_len,
	    old_row_buf, log_rec->rep_orec_size, log_rec->rep_diff_offset);
	old_row = old_row_buf;
    }

    /*
    ** Construct the new row using the old row (possibly constructed just
    ** above) and the changed bytes.
    */
    dm0l_row_unpack(old_row, log_rec->rep_orec_size, 
	new_row_log_info, log_rec->rep_ndata_len,
	new_row_buf, log_rec->rep_nrec_size, log_rec->rep_diff_offset);
    new_row = new_row_buf;

    /*
    ** Consistency Check:
    ** 
    ** Compare the row we are about to delete with the one we logged to
    ** make sure they are identical.  If they are not identical then we make
    ** an assumption here that the mismatch is more likely due to this check
    ** being wrong (we have garbage at the end of the tuple buffer or we
    ** allowed some sort of non-logged update to the row) and we continue with
    ** the operation after logging the unexpected condition.
    **
    ** If the table effected is IIRELATION we only compare a prefix of the
    ** row since iirelation is periodically updated without logging the
    ** changes. IISEQUENCE tuples are also updated without logging, so
    ** treat them the same way.
    **
    ** This check must be made after building the old row just above.
    */
    if (old_page)
    {
	/*
	** Compare the logged old row with the one on the page.
	*/
	if (log_rec->rep_tbl_id.db_tab_base == DM_B_RELATION_TAB_ID &&
		log_rec->rep_tbl_id.db_tab_index == DM_I_RELATION_TAB_ID)
	{
	    rowsdif = dmve_iirel_cmp(dmve, old_row, log_rec->rep_orec_size,
			record, record_size);
	}
	else if (log_rec->rep_tbl_id.db_tab_base == DM_B_SEQ_TAB_ID)
	{
	    rowsdif = dmve_iiseq_cmp(dmve, log_rec->rep_comptype,
			old_row, log_rec->rep_orec_size, record, record_size);
	}
	else if (log_rec->rep_orec_size != record_size ||
		MEcmp((PTR)old_row, (PTR)record, record_size) != 0)
	    rowsdif = TRUE;
	else 
	    rowsdif = FALSE;

	if (rowsdif)
	{
	    uleFormat(NULL, E_DM966C_DMVE_TUPLE_MISMATCH, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 8, 
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
		sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
		0, log_rec->rep_otid.tid_tid.tid_page,
		0, log_rec->rep_otid.tid_tid.tid_line,
		0, record_size, 0, log_rec->rep_orec_size,
		0, dmve->dmve_action);
	    dmd_log(1, (PTR) log_rec, 4096);
	    dmve_trace_page_header(page_type, log_rec->rep_page_size,
			old_page, plv, "DATA");
	    dmve_trace_page_contents(page_type, 
			log_rec->rep_page_size, old_page, "DATA");
	    uleFormat(NULL, E_DM9637_REDO_REP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
	}
    }

    /*
    ** Consistency Check:  Make sure that there is sufficient space for
    ** the new record.
    */
    if (newrow_page && page_type != TCB_PG_V1)
    {
	status = (*plv->dmpp_reallocate)(page_type, 
	    log_rec->rep_page_size, newrow_page,
	    log_rec->rep_ntid.tid_tid.tid_line, log_rec->rep_nrec_size);
	if (status != E_DB_OK)
	{
	    status = dmve_clean(dmve, new_pinfo, tabio); 
	    status = (*plv->dmpp_reallocate)(page_type, 
		log_rec->rep_page_size, newrow_page,
		log_rec->rep_ntid.tid_tid.tid_line, log_rec->rep_nrec_size);
	}
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM9674_DMVE_PAGE_NOROOM, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6, 
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
		sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
		0, log_rec->rep_ntid.tid_tid.tid_page,
		0, log_rec->rep_ntid.tid_tid.tid_line, 
		0, log_rec->rep_nrec_size);
	    dmd_log(1, (PTR) log_rec, 4096);
	    dmve_trace_page_header(page_type, log_rec->rep_page_size,
			newrow_page, plv, "DATA");
	    dmve_trace_page_contents(page_type, 
			log_rec->rep_page_size, newrow_page, "DATA");
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}
    }

    /*
    ** Non-journal table handling:
    **
    ** If this is rollforward of a replace on iirelation for a table which
    ** is not journaled, then we do special hacky handling.  Repeating history
    ** is not fully implemented on core catalogs as updates associated with
    ** non-journaled tables are not normally redone in journal recovery.
    **
    ** But a subset of core catalog updates on these non-journaled tables
    ** do get rolled forward:
    **
    **     - updates associated with DROP operations
    **     - alter operations on a table to describe schema changes
    **
    ** This leads to much confusion around the iirelation tuple, must have
    ** some updates rolled forward but not all of them.
    **
    ** The specific hacky work we do to implement rollforward of non-journal
    ** table DDL involves:
    **
    **     - We only rollforward changes to certain columns of the iirelation
    **       tuple - those associated with the above journaled updates.  Other
    **       colunms are left alone to ensure that the table meta-data stays
    **       in the same state it was in following restoration from the ckp.
    **
    **     - When secondary indexes are dropped, the base table index count
    **       and TCB_IDXD characteristic may change.  But we only want to
    **       update this field if rollforward successfully dropped the index
    **       (as opposed to skipping the index drop because the index did
    **       not exist in the restored database).  When an iirelation replace
    **       is encountered which changes one of these values we look through
    **       the rollforward context for a delete action on the secondary index
    **       row.
    */
    if ((dmve->dmve_action == DMVE_DO) &&
	(log_rec->rep_header.flags & DM0L_NONJNL_TAB) &&
	(log_rec->rep_tbl_id.db_tab_base == DM_B_RELATION_TAB_ID) &&
	(log_rec->rep_tbl_id.db_tab_index == DM_I_RELATION_TAB_ID))
    {
	DMP_RELATION	oreltup;
	DMP_RELATION	nreltup;
	DMP_RELATION	curtup;
	i4		old_size;
	i4		new_size;
	i4		cur_size;
	char		creltup[sizeof(DMP_RELATION)+50]; /* compress expand*/
	DMP_RELATION	*old_rel;
	DMP_RELATION	*new_rel;
	i4		old_relstat, new_relstat;
	i2		old_idxcount, new_idxcount;

	/* The tcb for iirelation will always have DMP_ROWACCESS  */
	t = (DMP_TCB *)((char *)tabio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

	status = (*t->tcb_data_rac.dmpp_uncompress)(&t->tcb_data_rac, 
		old_row, (char *)&oreltup, sizeof(DMP_RELATION), &old_size,
		NULL, 0, (ADF_CB *)NULL);
	old_rel = &oreltup;
	if (status)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}

	status = (*t->tcb_data_rac.dmpp_uncompress)(&t->tcb_data_rac, 
		new_row, (char *)&nreltup, sizeof(DMP_RELATION), &new_size,
		NULL, 0, (ADF_CB *)NULL);
	new_rel = &nreltup;
	if (status)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}

	status = (*t->tcb_data_rac.dmpp_uncompress)(&t->tcb_data_rac, 
		record, (char *)&curtup, sizeof(DMP_RELATION), &cur_size,
		NULL, 0, (ADF_CB *)NULL);
	if (status)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}

	/*
	** Extract values from (possibly non-aligned) log rows.
	*/
	I4ASSIGN_MACRO(old_rel->relstat, old_relstat);
	I2ASSIGN_MACRO(old_rel->relidxcount, old_idxcount);
	I4ASSIGN_MACRO(new_rel->relstat, new_relstat);
	I2ASSIGN_MACRO(new_rel->relidxcount, new_idxcount);
	MEcopy((PTR) &old_rel->reltid, sizeof(DB_TAB_ID), (PTR) &reltid);

	/* Check if index count changed. */
	index_count_changed = (old_idxcount > new_idxcount);

	/* Check if indexed characteristic changed. */
	index_bit_changed = ((old_relstat & TCB_IDXD) &&
			     ((new_relstat & TCB_IDXD) == 0));

	/*
	** If the change appears to be part of a drop index operation, check
	** if an index was recently dropped.
	*/
	if (index_count_changed || index_bit_changed)
	{
	    /*
	    ** Look for a recent delete of an iirelation row relating to
	    ** an index of this base table.  If a delete was done, then we
	    ** want to propagate the decrement of the index count with this
	    ** replace.  After finding the index drop action, remove the
	    ** index drop field to prevent a single index drop from causing
	    ** more than one index count decrement.
	    */
	    action = NULL;
	    for (;;)
	    {
		rfp_lookup_action(dmve, &dmve->dmve_tran_id, (LG_LSN *)NULL, 
			    RFP_REC_NEXT, &action);
		if (action == NULL)
		    break;

		if ((action->rfp_act_action == RFP_DELETE) &&
		    (action->rfp_act_tabid.db_tab_base == DM_B_RELATION_TAB_ID) &&
		    (action->rfp_act_tabid.db_tab_index == DM_I_RELATION_TAB_ID) &&
		    (action->rfp_act_usertabid.db_tab_base == 
							reltid.db_tab_base) &&
		    (action->rfp_act_index_drop))
		{
		    break;
		}
	    }

	    if (action)
	    {
		action->rfp_act_index_drop = 0;
	    }
	    else
	    {
		index_count_changed = 0;
		index_bit_changed = 0;
	    }
	}

	/*
	** Form new row to replace from the old row - masking in the changes
	** that we want to roll forward.
	*/
	MEcopy((PTR) &curtup, sizeof(temprow), (PTR) &temprow);
	MEcopy((PTR) &new_rel->relstamp12, sizeof(temprow.relstamp12),
	    (PTR) &temprow.relstamp12);
	I4ASSIGN_MACRO(new_rel->relqid1, temprow.relqid1);
	I4ASSIGN_MACRO(new_rel->relqid2, temprow.relqid2);
	I4ASSIGN_MACRO(new_rel->relstat, temprelstat);
	temprow.relstat &= ~RFP_RELSTAT_VALUES;
	temprow.relstat |= (temprelstat & RFP_RELSTAT_VALUES);

	if (index_bit_changed)
	{
	    old_index_count = temprow.relidxcount;
	    temprow.relidxcount = 0;
	    temprow.relstat &= ~TCB_IDXD;
	}
	else if (index_count_changed)
	{
	    temprow.relidxcount--;
	    if (temprow.relidxcount <= 0)
	    {
		temprow.relidxcount = 0;
		temprow.relstat &= ~TCB_IDXD;
	    }
	}

	status = (*t->tcb_data_rac.dmpp_compress)(&t->tcb_data_rac, 
		(char *)&temprow, sizeof(DMP_RELATION), creltup, &new_size);
	new_row = creltup;  
	if (status)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}
    }

    /*
    ** Mutex the pages while updating them.
    */
    if (old_page)
	dmveMutex(dmve, old_pinfo);
    if (new_page)
	dmveMutex(dmve, new_pinfo);

    /*
    ** Redo recovery must follow row locking protocols if phys locking
    ** Note we can't be row locking during redo recovery
    */
    if (log_rec->rep_header.flags & DM0L_PHYS_LOCK)
	update_mode = (DM1C_DIRECT | DM1C_ROWLK);
    else
	update_mode = DM1C_DIRECT;

    if (log_rec->rep_comptype != TCB_C_NONE)
	update_mode |= DM1C_DMVE_COMP;

    /*
    ** Redo the replace operation.
    **
    ** If the replace was done inplace (the old and new rows have the same
    ** size and point to the same TID) then just plop the new row over the
    ** top of the old one.  Otherwise, delete the old row and insert the
    ** new one.
    */
    if ((log_rec->rep_ntid.tid_i4 != log_rec->rep_otid.tid_i4) ||
	(log_rec->rep_nrec_size != log_rec->rep_orec_size))
    {
	if (old_page)
	{
	    (*plv->dmpp_delete)(page_type, log_rec->rep_page_size,
		old_page, update_mode, reclaim_space, &dmve->dmve_tran_id,
		LOG_ID_ID(dmve->dmve_log_id),
                &log_rec->rep_otid, log_rec->rep_orec_size);

	    /* Write the LSN of the Rep log record to the updated page. */
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, old_page, *log_lsn);
	    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, old_page, DMPP_MODIFY);
	}
    }

    if (newrow_page)
    {
	(*plv->dmpp_put)(page_type, log_rec->rep_page_size,
	    newrow_page, update_mode, &dmve->dmve_tran_id,
	    LOG_ID_ID(dmve->dmve_log_id),
	    &log_rec->rep_ntid,
	    log_rec->rep_nrec_size, new_row, log_rec->rep_nrow_version,
	    (DMPP_SEG_HDR *)NULL);

	/* Write the LSN of the Rep log record to the updated page. */
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, newrow_page, *log_lsn);
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, newrow_page, DMPP_MODIFY);
    }


    if (old_page)
	dmveUnMutex(dmve, old_pinfo);
    if (new_page)
	dmveUnMutex(dmve, new_pinfo);
    
    /*
    ** Non-journal table handling:  When we replace a system catalog row
    ** that describes a non-journaled table during journal recovery table
    ** we pend an action item recording this event.  This information will
    ** be referred to if this replace is undone to determine whether or not
    ** to restore the updated row.  We only want to put the row back during
    ** journal recovery if the row was actually replace.  If this routine
    ** found that the row to delete already did not exist, then we don't want
    ** undo recovery to process the row.
    */
    if ((dmve->dmve_action == DMVE_DO) &&
	(log_rec->rep_header.flags & DM0L_NONJNL_TAB))
    {
	status = rfp_add_action(dmve, &dmve->dmve_tran_id, log_lsn, 
			RFP_REPLACE, &action);
	if (status != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}

	action->rfp_act_idxbit_changed = index_bit_changed;
	action->rfp_act_idxcnt_changed = index_count_changed;
	action->rfp_act_oldidx_count = old_index_count;
	action->rfp_act_dbid = log_rec->rep_header.database_id;
	STRUCT_ASSIGN_MACRO(reltid, action->rfp_act_usertabid);
	STRUCT_ASSIGN_MACRO(log_rec->rep_tbl_id, action->rfp_act_tabid);
	STRUCT_ASSIGN_MACRO(log_rec->rep_otid, action->rfp_act_tid);
    }

    return(E_DB_OK);
}

/*{
** Name: dmv_unrep - UNDO of a rep operation.
**
** Description:
**	This routine executes undo recovery of a replace operation.  It
**	restores the old copy of the row and removes the new copy.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      old_page			Pointer to the page for the old row
**      new_page			Pointer to the page for the new row
**	log_record			Pointer to the log record
**	old_row_buf			Buffer for uncompressed old row storage
**					 NULL if old row not compressed
**	new_row				Buffer for uncompressed new row storage
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
**	    Written for Reduced logging project.
**	11-jan-1993 (jnash)
**	    Do not include new row in CLR log record, do not do tuple
**	    exists check if new row not logged.
**	26-jan-1993 (jnash)
**	    Fix bug introduced by 11-jan change -- preserve record and
**	    compare sizes.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	18-oct-1993 (jnash)
**	    Add support for replace log record compresssion.
**	11-dec-1993 (rogerk)
**	    Changed routine to reflect new replace log record compression
**	    algorithms.  New parameters added to dm0l_row_unpack routine.
**	    Use new rep_[o/n]data_len field in replace log record.  Moved
**	    unpacking of new and old rows together.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      23-may-1994 (mikem)
**          Bug #63556
**          Changed interface to dm0l_rep() so that a replace of a record only
**          need call dm0l_repl_comp_info() this routine once.  Previous to 
**          this change the routine was called once for LGreserve, and once for
**          dm0l_replace().
**	06-mar-1996 (stial01 for bryanp)
**	    Pass tbio_page_size as the page_size argument to dmpp_reclen.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to macros.
**      20-may-1996 (ramra01)
**          Added DMPP_TUPLE_INFO to get/put accessor routine
**      03-june-1996 (stial01)
**          Use DMPP_INIT_TUPLE_INFO_MACRO to init DMPP_TUPLE_INFO
**      22-jul-1996 (ramra01 for bryanp)
**          Alter Table support.
**          Pass row_version argument to dm0l_rep.
**          Pass log_rec->rep_nrow_version to dmpp_put when redoing replace.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Delete accessor: added reclaim_space 
**          Put accessor: changed DMPP_TUPLE_INFO param to table_version
**      10-mar-97 (stial01)
**          dmv_unrep() Space is not reclaimed if IX page lock held
**          (We should never be row locking for non in place REPLACE)
**      07-apr-97 (stial01)
**          dmv_unrep() Don't get mutex if we already have it
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	17-Jul-2008 (thaju02) (B120660)
**	    If a replace operation is requested on a iirelation tuple that
**	    represents a non-journaled table, ignore E_DM966D row missing 
**	    error. 
**	18-Feb-2010 (jonj)
**	    SIR 121619 MVCC: Restore previous row tran_id, lg_id 
**	    from log record, not transaction doing the undo.
*/
static DB_STATUS
dmv_unrep(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *old_pinfo,
DMP_PINFO	    *new_pinfo,
char	    	    *old_row_buf,
char	    	    *new_row_buf)
{
    DM0L_REP		*log_rec = (DM0L_REP *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->rep_header.lsn; 
    LG_LSN		*replace_lsn;
    LG_LSN		lsn;
    DMPP_PAGE	    	*newrow_page;
    DB_STATUS		status = E_DB_OK;
    i4		record_size;
    i4		flags;
    i4		page_type = log_rec->rep_pg_type;
    char		*record;
    char	    	*old_row;
    char	    	*old_row_log_info;
    char	    	*new_row;
    char	    	*new_row_log_info;
    bool		index_bit_changed;
    bool		index_count_changed;
    bool		comp_orow;
    i4		old_index_count;
    RFP_REC_ACTION	*action;
    DMP_RELATION	temprow;
    i4		temprelstat;
    i4		comp_odata_len;
    i4		comp_ndata_len;
    char	*comp_odata;
    char	*comp_ndata;
    i4		diff_offset;
    i4		reclaim_space;
    i4		update_mode;
    i4		*err_code = &dmve->dmve_error.err_code;
    DMPP_ACC_PLV	*plv = dmve->dmve_plv;
    LG_LRI		lri;
    DMPP_PAGE		*old_page = NULL;
    DMPP_PAGE		*new_page = NULL;
    DB_TRAN_ID		row_tran_id;
    bool		rowsdif;
    DMP_TCB		*t;

    CLRDBERR(&dmve->dmve_error);

    if ( old_pinfo )
        old_page = old_pinfo->page;
    if ( new_pinfo )
        new_page = new_pinfo->page;
 
    if ((old_page == NULL) && (new_page == NULL))
    {
	return (E_DB_OK);
    }

    /*
    ** If large pages, the undo-rep (delete new tuple) should not reclaim space
    ** If row locking or physical page locking, don't shift the data on the page
    ** If uncompressed data we never need to shift the data on the page
    */ 
    if (tabio->tbio_page_type != TCB_PG_V1 ||
	log_rec->rep_comptype == TCB_C_NONE ||
	(log_rec->rep_header.flags & DM0L_PHYS_LOCK))
	reclaim_space = FALSE;
    else
	reclaim_space = TRUE;

    /* Undo recovery must follow row/phys locking protocols */
    if ((dmve->dmve_lk_type == LK_ROW || dmve->dmve_lk_type == LK_CROW)
		|| (log_rec->rep_header.flags & DM0L_PHYS_LOCK))
	update_mode = (DM1C_DIRECT | DM1C_ROWLK);
    else
	update_mode = DM1C_DIRECT;

    if (log_rec->rep_comptype != TCB_C_NONE)
	update_mode |= DM1C_DMVE_COMP;

    /*
    ** Determine if the full row before image was logged.  If not then we have
    ** to extract the row on the page to build the old and new row versions.
    */
    comp_orow = FALSE;
    if (log_rec->rep_header.flags & DM0L_COMP_REPL_OROW)
	comp_orow = TRUE;

    /*
    ** Non-journal table handling:  When we replace a system catalog row
    ** that describes a non-journaled table during journal recovery table
    ** we pend an action item recording the event.  If later this routine
    ** is called to undo the replace, we need to lookup the action to verify
    ** that the original row was actually updated by journal recovery.  If
    ** If the row was not actually updated because it already did not exist
    ** at the start of journal recovery, then we don't want this undo
    ** operation to restore the row.
    */
    if ((dmve->dmve_action == DMVE_DO) &&
	(log_rec->rep_header.flags & DM0L_NONJNL_TAB))
    {
	replace_lsn = ((log_rec->rep_header.flags & DM0L_CLR) ? 
			    &log_rec->rep_header.compensated_lsn : log_lsn);
		
	rfp_lookup_action(dmve, &dmve->dmve_tran_id, replace_lsn, 
			    RFP_REC_LSN, &action);
	if (action == NULL)
	{
	    return (E_DB_OK);
	}

	/*
	** Save secondary index count information for construction of
	** of the replace row below.
	*/
	index_bit_changed = action->rfp_act_idxbit_changed;
	index_count_changed = action->rfp_act_idxcnt_changed;
	old_index_count = action->rfp_act_oldidx_count;

	/* Delete the action item since it has fulfilled its purpose. */
	rfp_remove_action(dmve, action);
    }

    /*
    ** Calculate pointers to log record row recovery information.
    */
    old_row_log_info = ((char *)log_rec) + sizeof(*log_rec);
    new_row_log_info = old_row_log_info + log_rec->rep_odata_len;

    /*
    ** Get pointer to correct page for the new tuple (the one which must be
    ** deleted and replaced with its olf version) for the following consistency
    ** check.  If both the old and new rows belong to the same page, then only
    ** 'old_page' will be passed in, otherwise the new row is put on 'new_page'.
    **
    ** Note that we won't compress old rows if the old and new rows are on
    ** different pages.
    */
    newrow_page = new_page;
    if (log_rec->rep_ntid.tid_tid.tid_page == 
		log_rec->rep_otid.tid_tid.tid_page)
    {
	newrow_page = old_page;
	new_page = NULL;
    }

    /*
    ** Consistency Check:
    ** Make sure that the new tuple exists before attempting to delete it.
    */
    if (newrow_page)
    {
	status = (*plv->dmpp_get)(page_type, log_rec->rep_page_size,
	    newrow_page, &log_rec->rep_ntid, &record_size,
	    &record, NULL, NULL, NULL, NULL);
	if (status != E_DB_OK)
	{
	    if ((log_rec->rep_tbl_id.db_tab_base == DM_B_RELATION_TAB_ID) &&
		(log_rec->rep_tbl_id.db_tab_index == DM_I_RELATION_TAB_ID) &&
		(log_rec->rep_header.flags & DM0L_NONJNL_TAB)) 
            {
                return (E_DB_OK);
            }

	    uleFormat(NULL, E_DM966D_DMVE_ROW_MISSING, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6, 
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
		sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
		0, log_rec->rep_ntid.tid_tid.tid_page,
		0, log_rec->rep_ntid.tid_tid.tid_line,
		0, status);
	    dmd_log(1, (PTR) log_rec, 4096);
	    dmve_trace_page_header(log_rec->rep_pg_type, log_rec->rep_page_size,
			newrow_page, plv, "DATA");
	    dmve_trace_page_contents(log_rec->rep_pg_type, 
			log_rec->rep_page_size, newrow_page, "DATA");
	    SETDBERR(&dmve->dmve_error, 0, E_DM9636_UNDO_REP);
	    return(E_DB_ERROR);
	}

	(*plv->dmpp_reclen)(page_type, log_rec->rep_page_size,
	    newrow_page, (i4)log_rec->rep_ntid.tid_tid.tid_line, &record_size);
    }

    /*
    ** Get pointer to full row before image.  Normally the old row data from
    ** the replace log record can be used.  If the old row is compressed then
    ** it must be reconstructed using the record just read from the page.
    ** (Note that comp_orow can only be true if the old and new rows are on
    ** the same page : enforced by dm0l_rep).
    */
    old_row = old_row_log_info;
    if (comp_orow)
    {
	dm0l_row_unpack(record, record_size, 
	    old_row_log_info, log_rec->rep_odata_len,
	    old_row_buf, log_rec->rep_orec_size, log_rec->rep_diff_offset);
	old_row = old_row_buf;
    }

    /*
    ** Construct the new row using the old row (possibly constructed just
    ** above) and the changed bytes.
    */
    dm0l_row_unpack(old_row, log_rec->rep_orec_size, 
	new_row_log_info, log_rec->rep_ndata_len,
	new_row_buf, log_rec->rep_nrec_size, log_rec->rep_diff_offset);
    new_row = new_row_buf;

    /*
    ** Consistency Check:
    ** 
    ** Compare the row we are about to delete with the one we logged to
    ** make sure they are identical.  If they are not identical then we make
    ** an assumption here that the mismatch is more likely due to this check
    ** being wrong (we have garbage at the end of the tuple buffer or we
    ** allowed some sort of non-logged update to the row) and we continue with
    ** the operation after logging the unexpected condition.
    **
    ** If the table effected is IIRELATION we only compare a prefix of the
    ** row since iirelation is periodically updated without logging the
    ** changes.
    **
    ** This check must be made after building the new row just above.
    ** This check is bypassed on CLR recovery since the new row is not logged.
    */
    if (newrow_page && log_rec->rep_nrec_size)
    {
	if (log_rec->rep_tbl_id.db_tab_base == DM_B_RELATION_TAB_ID &&
		log_rec->rep_tbl_id.db_tab_index == DM_I_RELATION_TAB_ID)
	{
	    rowsdif = dmve_iirel_cmp(dmve, new_row, log_rec->rep_nrec_size,
					record, record_size);
	}
	else if (log_rec->rep_tbl_id.db_tab_base == DM_B_SEQ_TAB_ID)
	{
	    rowsdif =dmve_iiseq_cmp(dmve, log_rec->rep_comptype,
		    new_row, log_rec->rep_nrec_size, record, record_size);
	}
	else if (log_rec->rep_nrec_size != record_size ||
		MEcmp((PTR)new_row, (PTR)record, record_size) != 0)
	    rowsdif = TRUE;
	else
	    rowsdif = FALSE;

	if (rowsdif)
	{
	    uleFormat(NULL, E_DM966C_DMVE_TUPLE_MISMATCH, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		err_code, 8, 
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
		sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
		0, log_rec->rep_ntid.tid_tid.tid_page,
		0, log_rec->rep_ntid.tid_tid.tid_line,
		0, record_size, 0, log_rec->rep_nrec_size,
		0, dmve->dmve_action);
	    dmd_log(1, (PTR) log_rec, 4096);
	    dmve_trace_page_header(log_rec->rep_pg_type, log_rec->rep_page_size,
			newrow_page, plv, "DATA");
	    dmve_trace_page_contents(log_rec->rep_pg_type, 
			log_rec->rep_page_size, newrow_page, "DATA");
	    uleFormat(NULL, E_DM9636_UNDO_REP, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
	    if (dmve->dmve_flags & DMVE_MVCC)
		dmd_pr_mvcc_info(dmve->dmve_rcb);
	}
    }

    /*
    ** Consistency Check:  Make sure that there is sufficient space to
    ** replace the old record.  
    **
    ** XXXX Note that this is kinda tricky if the new row is on the same
    ** page because you have to take its space into account.
    */
    if (old_page && page_type != TCB_PG_V1) 
    {
	status = (*plv->dmpp_reallocate)(page_type, 
	    log_rec->rep_page_size, old_page,
	    log_rec->rep_otid.tid_tid.tid_line, log_rec->rep_orec_size);
	if (status != E_DB_OK && 
		dmve->dmve_lk_type != LK_ROW && dmve->dmve_lk_type != LK_CROW)
	{
	    status = dmve_clean(dmve, old_pinfo, tabio); 
	    status = (*plv->dmpp_reallocate)(page_type, 
		log_rec->rep_page_size, old_page,
		log_rec->rep_otid.tid_tid.tid_line, log_rec->rep_orec_size);
	}
	if (status != E_DB_OK)
	{
	    uleFormat(NULL, E_DM9674_DMVE_PAGE_NOROOM, (CL_ERR_DESC *)NULL, ULE_LOG,
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6, 
		sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
		sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
		sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
		0, log_rec->rep_otid.tid_tid.tid_page,
		0, log_rec->rep_otid.tid_tid.tid_line, 
		0, log_rec->rep_orec_size);
	    dmd_log(1, (PTR) log_rec, 4096);
	    dmve_trace_page_header(log_rec->rep_pg_type, log_rec->rep_page_size,
			old_page, plv, "DATA");
	    dmve_trace_page_contents(log_rec->rep_pg_type, 
			log_rec->rep_page_size, old_page, "DATA");
	    SETDBERR(&dmve->dmve_error, 0, E_DM9636_UNDO_REP);
	    if (dmve->dmve_flags & DMVE_MVCC)
		dmd_pr_mvcc_info(dmve->dmve_rcb);
	    return(E_DB_ERROR);
	}
    }

    /*
    ** Non-journal table handling:
    **
    ** If this is rollforward of a replace on iirelation for a table which
    ** is not journaled, then we do special hacky handling.  Repeating history
    ** is not fully implemented on core catalogs as updates associated with
    ** non-journaled tables are not normally redone in journal recovery.
    **
    ** In rollforward we only replace certain columns of iirelation rows.
    */
    if ((dmve->dmve_action == DMVE_DO) &&
	(log_rec->rep_header.flags & DM0L_NONJNL_TAB) &&
	(log_rec->rep_tbl_id.db_tab_base == DM_B_RELATION_TAB_ID) &&
	(log_rec->rep_tbl_id.db_tab_index == DM_I_RELATION_TAB_ID))
    {
	char		creltup[sizeof(DMP_RELATION)+50]; /* compress expand*/
	DMP_RELATION	*old_rel;
	DMP_RELATION	oreltup;
	DMP_RELATION	curtup;
	i4		old_size;
	i4		cur_size;

	/* The tcb for iirelation will always have DMP_ROWACCESS  */
	t = (DMP_TCB *)((char *)tabio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

	status = (*t->tcb_data_rac.dmpp_uncompress)(&t->tcb_data_rac, 
		old_row, (char *)&oreltup, sizeof(DMP_RELATION), &old_size,
		NULL, 0, (ADF_CB *)NULL);
	if (status)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9636_UNDO_REP);
	    return(E_DB_ERROR);
	}
	old_rel = &oreltup;

	status = (*t->tcb_data_rac.dmpp_uncompress)(&t->tcb_data_rac, 
		record, (char *)&curtup, sizeof(DMP_RELATION), &cur_size,
		NULL, 0, (ADF_CB *)NULL);
	if (status)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9637_REDO_REP);
	    return(E_DB_ERROR);
	}

	/*
	** Form new row to replace from the old row - masking in the changes
	** that we want to roll forward.
	*/
	MEcopy((PTR) &curtup, sizeof(temprow), (PTR) &temprow);
	MEcopy((PTR) &old_rel->relstamp12, sizeof(temprow.relstamp12),
	    (PTR) &temprow.relstamp12);
	I4ASSIGN_MACRO(old_rel->relqid1, temprow.relqid1);
	I4ASSIGN_MACRO(old_rel->relqid2, temprow.relqid2);
	I4ASSIGN_MACRO(old_rel->relstat, temprelstat);
	temprow.relstat &= ~RFP_RELSTAT_VALUES;
	temprow.relstat |= (temprelstat & RFP_RELSTAT_VALUES);

	if (index_bit_changed)
	{
	    temprow.relstat |= TCB_IDXD;
	    temprow.relidxcount = old_index_count;
	}
	else if (index_count_changed)
	{
	    temprow.relidxcount++;
	    if ((temprow.relstat & TCB_IDXD) == 0)
		temprow.relstat |= TCB_IDXD;
	}

	status = (*t->tcb_data_rac.dmpp_compress)(&t->tcb_data_rac, 
		(char *)&temprow, sizeof(DMP_RELATION), creltup, &old_size);
	old_row = creltup;  
	if (status)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9636_UNDO_REP);
	    return(E_DB_ERROR);
	}
    }


    /*
    ** Mutex the pages while updating them.
    */
    if (old_page)
	dmveMutex(dmve, old_pinfo);

    if (new_page)
	dmveMutex(dmve, new_pinfo);

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
    **
    ** The CLR for a REPLACE need not contain the entire new row, just the tid.
    */
    if ((log_rec->rep_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    flags = (log_rec->rep_header.flags | DM0L_CLR);

	    /* Extract previous page change info */
	    DMPP_VPT_GET_PAGE_LRI_MACRO(log_rec->rep_pg_type, old_page, &lri);

            /*
            ** Calculate compressed row image lengths for the space reservation
            ** call.  The comp_odata_len and comp_ndata_len values returned
            ** here should match the compression done in dm0l_rep.
            */
            dm0l_repl_comp_info((PTR) old_row, log_rec->rep_orec_size,
                (PTR) new_row, 0, &log_rec->rep_tbl_id,
                &log_rec->rep_otid, &log_rec->rep_ntid, (i4) 0,
		(i4) 0, &comp_odata, &comp_odata_len,
                &comp_ndata, &comp_ndata_len, &diff_offset, &comp_orow);

	    status = dm0l_rep(dmve->dmve_log_id, flags, &log_rec->rep_tbl_id, 
		tabio->tbio_relid, 0,
		tabio->tbio_relowner, 0,
		&log_rec->rep_otid, 
		&log_rec->rep_ntid,
		log_rec->rep_pg_type, log_rec->rep_page_size,
		log_rec->rep_comptype,
		log_rec->rep_ocnf_loc_id, 
		log_rec->rep_ncnf_loc_id, log_rec->rep_loc_cnt,
		log_rec->rep_orec_size, 0, old_row, new_row, 
		log_lsn, 
	        comp_odata, comp_odata_len, comp_ndata, comp_ndata_len,
	        diff_offset, comp_orow,
		log_rec->rep_orow_version, log_rec->rep_nrow_version,
		log_rec->rep_otran_id,
		log_rec->rep_olg_id,
		&lri, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		if (old_page) 
		    dmveUnMutex(dmve, old_pinfo);
		if (new_page) 
		    dmveUnMutex(dmve, new_pinfo);

                /*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
								  
		SETDBERR(&dmve->dmve_error, 0, E_DM9636_UNDO_REP);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are processing recovery of a Replace CLR (redo-ing the
	** undo of a replace (wow)) then we don't log a CLR but instead
	** save the LSN of the log record we are processing with which to
	** update the page lsn's.
	*/
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
    }

    row_tran_id.db_high_tran = 0;
    row_tran_id.db_low_tran = log_rec->rep_otran_id;

    /*
    ** Redo the replace operation.
    ** Write the LSN of the replace record onto the page(s).
    **
    ** If the replace was done inplace (the old and new rows have the same
    ** size and point to the same TID) then just plop the old row back over the
    ** top of the new one.  Otherwise, delete the new row and reinsert the
    ** old one.
    */
    if ((log_rec->rep_ntid.tid_i4 != log_rec->rep_otid.tid_i4) ||
	(log_rec->rep_nrec_size != log_rec->rep_orec_size))
    {
	if (newrow_page)
	{
	    (*plv->dmpp_delete)(page_type, log_rec->rep_page_size,
		newrow_page, update_mode, reclaim_space,
		&row_tran_id,
		log_rec->rep_olg_id,
                &log_rec->rep_ntid, record_size);

	    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, newrow_page, DMPP_MODIFY);

	    if (dmve->dmve_logging)
		DMPP_VPT_SET_PAGE_LRI_MACRO(page_type, newrow_page, &lri);
	}
    }

    if (old_page)
    {
	(*plv->dmpp_put)(page_type, log_rec->rep_page_size, 
	    old_page, update_mode,
	    &row_tran_id,
	    log_rec->rep_olg_id,
	    &log_rec->rep_otid,
	    log_rec->rep_orec_size, old_row, log_rec->rep_orow_version,
	    (DMPP_SEG_HDR *)NULL);

	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, old_page, DMPP_MODIFY);

	if (dmve->dmve_logging)
	    DMPP_VPT_SET_PAGE_LRI_MACRO(page_type, old_page, &lri);
    }


    if (old_page)
	dmveUnMutex(dmve, old_pinfo);
    if (new_page)
	dmveUnMutex(dmve, new_pinfo);
    
    return(E_DB_OK);
}
