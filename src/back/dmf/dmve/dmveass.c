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
#include    <dmve.h>
#include    <dm1c.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0p.h>
#include    <dm0l.h>
#include    <dmftrace.h>

/*
**
**  Name: DMVEASS.C - The recovery of a associated page.
**
**  Description:
**	This file contains the routine for the recovery of an associated
**	page change operation.
**
**          dmve_assoc   - The recovery of a page.
**          dmve_reassoc - REDO recovery of a page.
**          dmve_unassoc - UNDO recovery of a page.
**
**
**  History:
**	 2-apr-1990 (Derek)
**          Created.
**	 3-nov-1991 (rogerk)
**	    Changed REDO handling to update the ASSOC status on the old
**	    and new data pages.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed arguments to dm0p_mutex and
**	    dm0p_unmutex call(s)s.
**	22-oct-1992 (jnash)
**	    Reduced Logging Project: Completely rewritten with new 6.5
**	    recovery protocols.
**	18-jan-1993 (rogerk)
**	    Fixed UNDO recovery to handle case where no recovery is needed.
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
**	23-aug-1993 (rogerk)
**	    Set physical_page_lock flag correctly when recovering
**	    the data pages with physical locks.  We were setting the
**	    physical_new_data_lock flag instead of physical_old_data_lock.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      06-mar-1996 (stial01)
**          Pass ass_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02)
**	    New Page Format Project: change page header references to macros.
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
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_assoc, eliminating need for dmve_name_unpack().
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
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**      14-oct-2009 (stial01)
**          Call dmve_fix_page, dmve_unfix_page
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs, move consistency check to dmveutil
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

static DB_STATUS dmv_unassoc(
		DMVE_CB             *dmve,
		DMP_TABLE_IO        *tabio,
		DMP_PINFO 	    *leafpinfo,
		DMP_PINFO           *odatapinfo,
		DMP_PINFO           *ndatapinfo);

static DB_STATUS dmv_reassoc(
		DMVE_CB             *dmve,
		DMP_TABLE_IO        *tabio,
		DMP_PINFO 	    *leafpinfo,
		DMP_PINFO           *odatapinfo,
		DMP_PINFO           *ndatapinfo);


/*{
** Name: dmve_assoc - The recovery of a associated page change.
**
** Description:
**
**	This function replaces the old associated data page pointer for
**	UNDO and the new associated page pointer for REDO.
**
**	For REDO it also updates the new assocated data page to show that
**	it is assocated.
**
**	The ASSOC operation involves the following changes:
**
**	    - New data page is allocated (updating free map pages)
**	    - New data page is formatted as an associated data page
**	    - Old data page is set not non-associated
**	    - Leaf page is set to point to the new assocated data page
**
**	For undo processing, pages are recovered as follows:
**	    - New data page is restored via the ALLOC log record; this
**	      causes the page to be formatted as FREE as well as updating
**	      the free map pages.
**	    - Old data page is restored from a BI.
**	    - Leaf page is restored by this dmve_assoc routine.
**	    - (During Online Backup dump processing, the leaf is restored
**	      using a Before Image).
**
**	For redo processing, pages are recovered as follows:
**	    - New data page is formatted as DATA page via the ALLOC record;
**	      this also updates the free map pages to show it not free.
**	      The new data page status is set to ASSOC by this dmve_assoc call.
**	    - Old data page is set to not ASSOC by this dmve_assoc call.
**	    - Leaf page set to point to new data page by this dmve_assoc call.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The assoc log record.
**	    .dmve_action	    Should DMVE_UNDO or DMVE_REDO.
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
**	 2-apr-1990 (Derek)
**          Created.
**	 3-nov-1991 (rogerk)
**	    Changed REDO handling to update the ASSOC status on the old
**	    and new data pages.  Also added checks for page log address
**	    before updating pages.
**	22-oct-1992 (jnash)
**	    Reduced Logging Project: Completely rewritten for new 6.5
**	    recovery protocols.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	23-aug-1993 (rogerk)
**	    Set physical_page_lock flag correctly when recovering
**	    the data pages with physical locks.  We were setting the
**	    physical_new_data_lock flag instead of physical_old_data_lock.
**	06-may-1996 (thaju02)
**	    New Page Format Project: 
**		Change page header references to macros.
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
dmve_assoc(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO 	*tbio = NULL;
    DM0L_ASSOC		*log_rec = (DM0L_ASSOC *)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->ass_header.lsn; 
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    i4		error = E_DB_OK;
    DMPP_PAGE		*leaf;
    DMPP_PAGE		*odata;
    DMPP_PAGE		*ndata;
    DMP_TCB             *t = NULL;
    i4		loc_error;
    i4		page_type = log_rec->ass_pg_type;
    bool		leaf_recover;
    bool		old_data_recover;
    bool		new_data_recover;
    i4		recovery_action;
    DB_ERROR		local_dberr;
    DMP_PINFO		*leafpinfo = NULL;
    DMP_PINFO		*odatapinfo = NULL;
    DMP_PINFO		*ndatapinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->ass_header.type != DM0LASSOC)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
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
	status = dmve_fix_tabio(dmve, &log_rec->ass_tbl_id, &tbio);
	if (DB_FAILURE_MACRO(status))
	    break;
	if ((status == E_DB_WARN) && (dmve->dmve_error.err_code == W_DM9660_DMVE_TABLE_OFFLINE))
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
	leaf_recover = dmve_location_check(dmve, 
	    (i4)log_rec->ass_lcnf_loc_id);
	old_data_recover = dmve_location_check(dmve,
	    (i4)log_rec->ass_ocnf_loc_id);
	new_data_recover = dmve_location_check(dmve,
	    (i4)log_rec->ass_ncnf_loc_id);

	/*
	** Lock/Fix the pages we need to recover in cache for write.
	*/
	if (leaf_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->ass_leaf_page,
					&leafpinfo);
	    if (status != E_DB_OK)
		break;
	    leaf = leafpinfo->page;
	}

	if (old_data_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->ass_old_data,
					&odatapinfo);
	    if (status != E_DB_OK)
		break;
	    odata = odatapinfo->page;
	}

	if (new_data_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->ass_new_data,
					&ndatapinfo);
	    if (status != E_DB_OK)
		break;
	    ndata = ndatapinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->ass_pg_type, log_rec->ass_page_size,
		leaf, dmve->dmve_plv, "LEAF");
	    dmve_trace_page_info(log_rec->ass_pg_type, log_rec->ass_page_size,
		odata, dmve->dmve_plv, "OldDATA"); 
	    dmve_trace_page_info(log_rec->ass_pg_type, log_rec->ass_page_size,
		ndata, dmve->dmve_plv, "NewDATA"); 
	}

	/*
	** Compare the LSN's on the pages with that of the log record
	** to determine what recovery will be needed.
	**
	**     During Forward processing, if the page's LSN is greater than
	**     the log record then no recovery is needed.
	**
	**     During Backward processing, it is an error for a page's LSN
	**     to be less than the log record LSN.
	*/

	switch (dmve->dmve_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    if (leaf && LSN_GTE(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, leaf), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, leaf),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, leaf),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, leaf),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, leaf),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		leaf = NULL;
	    }
 
	    if (odata && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, odata), log_lsn))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, odata),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, odata),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, odata),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, odata),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		odata = NULL;
	    }

	    if (ndata && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, ndata), log_lsn))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, ndata),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, ndata),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, ndata),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, ndata),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		ndata = NULL;
	    }
	    break;

	case DMVE_UNDO:
	    if (leaf && LSN_LT(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, leaf), log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, leaf),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, leaf),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, leaf),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, leaf),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }

	    if (odata && LSN_LT(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, odata), log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, odata),
		    0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, odata),
		    0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, odata),
		    0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, odata),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }

	    if (ndata && LSN_LT(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, ndata), log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, ndata),
		    0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, ndata),
		    0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, ndata),
		    0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, ndata),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
 	}
	if (status != E_DB_OK || (!leaf && !odata && !ndata))
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.  Code within UNDO recognizes the CLR and does not
	** write another CLR log record.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->ass_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

        switch (recovery_action)
        {
        case DMVE_DO:
        case DMVE_REDO:
            status = dmv_reassoc(dmve, tbio, 
	    			(leaf) ? leafpinfo : NULL, 
				(odata) ? odatapinfo : NULL, 
				(ndata) ? ndatapinfo : NULL);
            break;

        case DMVE_UNDO:
	    status = dmv_unassoc(dmve, tbio,
	    			(leaf) ? leafpinfo : NULL, 
				(odata) ? odatapinfo : NULL, 
				(ndata) ? ndatapinfo : NULL);
            break;
        }

	break;
    }

    if (status != E_DB_OK)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
	    (i4)0, (i4 *)NULL, &error, 0);
    }

    /*
    ** Unfix/Unlock the updated page.  No need to force it to disk - it
    ** will be tossed out through normal cache protocols if Fast
    ** Commit or at the end of the abort if non Fast Commit.
    */
    if (leafpinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, leafpinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (ndatapinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, ndatapinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (odatapinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, odatapinfo);
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9627_DMVE_ASSOC);
   
    return(status);
}

/*{
** Name: dmv_reassoc - Redo adding an associated data page.
**
** Description:
**      This function performs redo and rollforward recovery of
**	adding an associated data page.
**
** Inputs:
**      dmve 				Pointer to dmve control block.
**      tabio                           Pointer to table io control block
**      leaf				Pointer to the leaf page 
**      old_data 			Pointer to the old assoc datapage 
**      new_data 			Pointer to the new assoc datapage 
**      log_record			Pointer to the log record
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
**	    New Page Format Project: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_reassoc(
DMVE_CB             *dmve,
DMP_TABLE_IO        *tabio,
DMP_PINFO 	    *leafpinfo,
DMP_PINFO           *odatapinfo,
DMP_PINFO           *ndatapinfo)
{
    LG_LRI		lri_leaf;
    LG_LRI		lri_odata;
    DM0L_ASSOC		*log_rec = (DM0L_ASSOC *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->ass_header.lsn; 
    DB_STATUS  		status;
    i4			page_type = log_rec->ass_pg_type;
    DMPP_PAGE		*leaf = NULL;
    DMPP_PAGE		*odata = NULL;
    DMPP_PAGE		*ndata = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( leafpinfo )
        leaf = leafpinfo->page;
    if ( odatapinfo )
        odata = odatapinfo->page;
    if ( ndatapinfo )
        ndata = ndatapinfo->page;

    /*
    ** Check if recovery needed.
    */
    if ((odata == NULL) && (ndata == NULL) && (leaf == NULL) )
	return (E_DB_OK);

    /*
    ** Mutex the pages while updating them.
    */
    if (leaf)
	dmveMutex(dmve, leafpinfo);
    if (odata)
	dmveMutex(dmve, odatapinfo);
    if (ndata)
	dmveMutex(dmve, ndatapinfo);


    /*
    ** Redo the assoc operation.
    */
    if (leaf)
    {
	/* Note DM0LASSOC has two CRHDRS, one for leaf, one for odata */
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri_leaf, log_rec);

	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, leaf, DMPP_MODIFY);
	DM1B_VPT_SET_BT_DATA_MACRO(page_type, leaf, log_rec->ass_new_data);
	DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, leaf, &lri_leaf);
	DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, leaf, *log_lsn);
    }
    if (ndata)
    {
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, ndata,
	    (DMPP_MODIFY | DMPP_PRIM | DMPP_ASSOC));
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, ndata, *log_lsn);
    }
    if (odata)
    {
	/* Note DM0LASSOC has two CRHDRS, one for leaf, one for odata */
	DM0L_MAKE_LRI_FROM_LOG_CRHDR2(&lri_odata, log_rec);

	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, odata, DMPP_MODIFY);
	DMPP_VPT_CLR_PAGE_STAT_MACRO(page_type, odata, DMPP_ASSOC);
	DMPP_VPT_SET_PAGE_LRI_MACRO(page_type, odata, &lri_odata);
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, odata, *log_lsn);
    }

    if (leaf)
	dmveUnMutex(dmve, leafpinfo);
    if (odata)
	dmveUnMutex(dmve, odatapinfo);
    if (ndata)
	dmveUnMutex(dmve, ndatapinfo);
    
    return(E_DB_OK);
}

/*{
** Name: dmv_unassoc - Undo adding an associated data page.
**
** Description:
**      This function performs UNDO recovery on the adding of an
**	associated data page to an BTREE table.
**
** Inputs:
**      dmve 				Pointer to dmve control block.
**      tabio                           Pointer to table io control block
**      leaf				Pointer to the leaf page 
**      old_data 			Pointer to the old assoc datapage 
**      new_data 			Pointer to the new assoc datapage 
**      log_record			Pointer to the log record
**
** Outputs:
**      error                           Pointer to Error return area
**      Returns:
**          E_DB_OK
**          E_DB_ERROR
**
**      Exceptions:
**          none
**
** History:
**      06-oct-1992 (jnash)
**          Written for 6.5 recovery project.
**	18-jan-1992 (rogerk)
**	    Handle case where no recovery is needed on any of the
**	    pages.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-may-1996 (thaju02)
**	    New Page Format Project: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_unassoc(
DMVE_CB             *dmve,
DMP_TABLE_IO        *tabio,
DMP_PINFO 	    *leafpinfo,
DMP_PINFO           *odatapinfo,
DMP_PINFO           *ndatapinfo)
{
    DM0L_ASSOC		*log_rec = (DM0L_ASSOC *)dmve->dmve_log_rec;
    LG_LSN		lsn;
    LG_LSN		*log_lsn = &log_rec->ass_header.lsn; 
    i4		dm0l_flags;
    i4		page_type = log_rec->ass_pg_type;
    DB_STATUS		status;
    i4			local_err;
    i4			*err_code = &dmve->dmve_error.err_code;
    LG_LRI		lri_leaf;
    LG_LRI		lri_odata;
    DMPP_PAGE		*leaf = NULL;
    DMPP_PAGE		*odata = NULL;
    DMPP_PAGE		*ndata = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( leafpinfo )
        leaf = leafpinfo->page;
    if ( odatapinfo )
        odata = odatapinfo->page;
    if ( ndatapinfo )
        ndata = ndatapinfo->page;

    /*
    ** Check if recovery needed.
    */
    if ((odata == NULL) && (ndata == NULL) && (leaf == NULL) )
	return (E_DB_OK);

    /* 
    ** Mutex the page.  This must be done prior to the log write.
    */
    if (leaf)
	dmveMutex(dmve, leafpinfo);
    if (odata)
	dmveMutex(dmve, odatapinfo);
    if (ndata)
	dmveMutex(dmve, ndatapinfo);

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
    if ((log_rec->ass_header.flags & DM0L_CLR) == 0)
    {
	dm0l_flags = (log_rec->ass_header.flags | DM0L_CLR);

	if (dmve->dmve_logging)
	{
	    /*
	    ** Extract previous page change info
	    ** No undo DM0LASSOC if done in mini transaction,
	    ** so LRI in DM0LASSOC CLR records is probably never used 
	    **
	    ** Undo DM0LASSOC without all pages fixed is probably only possible
	    ** during rollforwarddb
	    */
	    if (leaf)
	    {
		DM1B_VPT_GET_PAGE_LRI_MACRO(log_rec->ass_pg_type, leaf, &lri_leaf);
	    }
	    else
		DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri_leaf, log_rec);

	    if (odata)
	    {
		DMPP_VPT_GET_PAGE_LRI_MACRO(log_rec->ass_pg_type, odata, &lri_odata);
	    }
	    else
		DM0L_MAKE_LRI_FROM_LOG_CRHDR2(&lri_odata, log_rec);

	    status = dm0l_assoc(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->ass_tbl_id,
		tabio->tbio_relid, 0,
		tabio->tbio_relowner, 0,
		log_rec->ass_pg_type, 
		log_rec->ass_page_size,
		log_rec->ass_loc_cnt, log_rec->ass_lcnf_loc_id, 
		log_rec->ass_ocnf_loc_id, log_rec->ass_ncnf_loc_id, 
		log_rec->ass_leaf_page, log_rec->ass_old_data, 
		log_rec->ass_new_data, log_lsn,
		&lri_leaf, &lri_odata, &dmve->dmve_error);

	    STRUCT_ASSIGN_MACRO(lri_leaf.lri_lsn, lsn); /* for ndata */

	    if (status != E_DB_OK)
	    {
		if (leaf)
		    dmveUnMutex(dmve, leafpinfo);
		if (ndata)
		    dmveUnMutex(dmve, ndatapinfo);
		if (odata)
		    dmveUnMutex(dmve, odatapinfo);

                /*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;

		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0); 

		SETDBERR(&dmve->dmve_error, 0, E_DM965E_UNDO_ASSOC);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are redo-ing the undo of the associate operation,
	** then we don't log a CLR but instead save the LSN
	** of the log record we are processing with which to update the
	** affected page lsn's.
	*/
	lsn = *log_lsn; /* for ndata */
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri_leaf, log_rec);
	DM0L_MAKE_LRI_FROM_LOG_CRHDR2(&lri_odata, log_rec);
    }

    /*
    ** Undo the assoc operation, 
    */

    if (leaf)
    {
	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, leaf, DMPP_MODIFY);
    	DM1B_VPT_SET_BT_DATA_MACRO(page_type, leaf, log_rec->ass_old_data);
    }

    if (ndata)
    {
	DMPP_VPT_CLR_PAGE_STAT_MACRO(page_type, ndata, DMPP_ASSOC);
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, ndata, DMPP_MODIFY);
    }

    if (odata)
    {
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, odata, DMPP_ASSOC);
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, odata, DMPP_MODIFY);
    }

    /* 
    ** Write the LSN of the overflow record onto the page.
    */
    if (dmve->dmve_logging)
    {
	if (leaf)
	    DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, leaf, &lri_leaf);
        if (ndata) 
	    DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, ndata, lsn);
	if (odata) 
	{
	    DMPP_VPT_SET_PAGE_LRI_MACRO(page_type, odata, &lri_odata);
	}
    }

    if (leaf)
	dmveUnMutex(dmve, leafpinfo);
    if (odata)
	dmveUnMutex(dmve, odatapinfo);
    if (ndata)
	dmveUnMutex(dmve, ndatapinfo);

    return(E_DB_OK);
}
