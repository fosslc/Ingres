/*
** Copyright (c) 1992, 2008 Ingres Corporation
*/

/*
**  NO_OPTIM = nc4_us5
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
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
#include    <dm2f.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVEBTOVFL.C - The recovery of a Btree Allocate Overflow operation.
**
**  Description:
**	This file contains the routines for the recovery of the action of
**	adding a new leaf overflow page to a btree overflow chain.
**
**          dmve_btovfl - The recovery of a Btree Overflow operation.
**          dmve_rebtovfl - Redo Recovery of a Btree Overflow operation.
**          dmve_unbtovfl - Undo Recovery of a Btree Overflow operation.
**
**  History:
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**	15-mar-1993 (jnash & rogerk)
**	    Reduced Logging - Phase IV:
**	    Changed log record format: added database_id, tran_id and LSN
**		in the log record header.  The LSN of a log record is now
**		obtained from the record itself rather than from the dmve
**		control block.
**	    Check dmve->dmve_logging to determine if logging required.
**	    Also, fix bug where *log_lsn, not lsn, inserted onto page 
**		during REDO of UNDO.
**	15-apr-1993 (rogerk)
**	    Fixed coding bug which caused us to sometimes skip undo recovery.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-apr-1993 (rogerk)
**	    In undo code, don't reference through leaf pointer when the leaf is
**	    not present during recovery processing.
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
**	    Include respective CL headers for all cl calls.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxformat.
**      06-mar-1996 (stial01)
**          Limit kperpage to (DM_TIDEOF + 1) - DM1B_OFFSEQ
**          Pass bto_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      12-jun-97 (stial01)
**          Pass new attribute, key parameters to dm1cxformat
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_ufx_tabio_cb() calls.
**	23-Jun-1998 (jenjo02)
**	    Added lock_value parm to dm0p_lock_page() and dm0p_unlock_page().
**	    Utilize lock_id value in both of those functions.
**	14-aug-1998 (nanpr01)
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
**      07-Dec-1998 (stial01)
**          Added kperleaf to distinguish from kperpage (keys per index page)
**      09-feb-99 (stial01)
**          dmv_rebtovfl() Pass relcmptlvl = 0 to dm1cxkperpage
**      22-jun-1999 (musro02)
**          Added NO_OPTIM for NCR (nc4_us5) to fix error:
**                invalid register for instruction: %bp in testb
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
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
**	22-Jan-2004 (jenjo02)
**	    Add support for Global indexes, TID8s.
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
**/

/*
** Forward Declarations.
*/

static DB_STATUS dmv_rebtovfl(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *ovflpinfo,
			DMP_PINFO	    *leafpinfo);

static DB_STATUS dmv_unbtovfl(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *ovflpinfo,
			DMP_PINFO	    *leafpinfo);


/*{
** Name: dmve_btovfl - The recovery of a Btree Allocate Overflow operation.
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
**	17-dec-1992 (rogerk)
**	    Added logic to check for splits which have occurred since the
**	    overflow page was added.  If the parent is a leaf which has
**	    been split, it may no longer be the owner of the overflow chain,
**	    and the overflow page's new parent leaf has to be updated.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	06-may-1996 (thaju02 & nanpr01)
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
dmve_btovfl(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_BTOVFL		*log_rec = (DM0L_BTOVFL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->bto_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*ovfl = NULL;
    DMPP_PAGE		*leaf = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DM_PAGENO		leaf_page_number;
    i4		leaf_loc_id;
    i4		loc_id;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4		page_type = log_rec->bto_pg_type;
    char		*duplicate_key;
    bool		ovfl_page_recover;
    bool		leaf_page_recover;
    bool		undo_check_done = FALSE;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*ovflpinfo = NULL;
    DMP_PINFO		*leafpinfo = NULL;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** Store previous page number into a local variable.  This value may
    ** be modified in undo recovery if the leaf has been split.
    */
    leaf_page_number = log_rec->bto_leaf_pageno;

    for (;;)
    {
	/*
	** Consistency Check:  check for illegal log records.
	*/
	if ((log_rec->bto_header.type != DM0LBTOVFL) ||
	    (log_rec->bto_header.length != 
                (sizeof(DM0L_BTOVFL) -
                        (DM1B_KEYLENGTH - log_rec->bto_lrange_len) - 
                        (DM1B_KEYLENGTH - log_rec->bto_rrange_len))))
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
	status = dmve_fix_tabio(dmve, &log_rec->bto_tbl_id, &tbio);
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
	** Check recovery requirements for this operation.
	** Note that the previous page we are using may be different than
	** the one logged in the original operation.
	*/
	loc_id = DM2F_LOCID_MACRO(tbio->tbio_loc_count,
						(i4) leaf_page_number);
	leaf_loc_id = tbio->tbio_location_array[loc_id].loc_config_id;

	ovfl_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->bto_ovfl_loc_id);
	leaf_page_recover = dmve_location_check(dmve, leaf_loc_id);


	/*
	** Check for partial recovery during UNDO operations.
	** We cannot bypass recovery of the leaf page even if the page
	** is on a location which is not being recovered.  This is because
	** the logged page number may not be the page that actually needs
	** the UNDO action!!!  If the page which was originally updated
	** has been since SPLIT, then the ovfl chain may have moved to a
	** new leaf page.  It is that new page to which this undo
	** action must be applied.  And that new page may be on a location
	** being recovered even if the original page is not.
	**
	** If recovery is being bypassed on the entire table then no recovery
	** needs to be done.
	*/
	if ((dmve->dmve_action == DMVE_UNDO) &&
	    (! leaf_page_recover))
	{
            uleFormat(NULL, E_DM9668_TABLE_NOT_RECOVERED, (CL_ERR_DESC *)NULL,
                ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                &loc_error, 2, 0, log_rec->bto_tbl_id.db_tab_base,
                0, log_rec->bto_tbl_id.db_tab_index);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9667_NOPARTIAL_RECOVERY);
	    break;
	}

	/*
	** Lock/Fix the pages we need to recover in cache for write.
	*/
	if (ovfl_page_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->bto_ovfl_pageno,
					&ovflpinfo);
	    if (status != E_DB_OK)
		break;
	    ovfl = ovflpinfo->page;
	}

	if (leaf_page_recover)
	{
	    status = dmve_fix_page(dmve, tbio, leaf_page_number, &leafpinfo);
	    if (status != E_DB_OK)
		break;
	    leaf = leafpinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->bto_pg_type, log_rec->bto_page_size,
		ovfl, dmve->dmve_plv, "OVFL");
	    dmve_trace_page_info(log_rec->bto_pg_type, log_rec->bto_page_size,
		leaf, dmve->dmve_plv, "LEAF");
	}

	/*
	** UNDO Recovery SPLIT check:
	**
	** If doing UNDO recovery, we need to verify that we have the correct 
	** previous page from which to unlink the overflow page.  If the
	** parent leaf page of this overflow chain has been split since the
	** overflow page was freed then the overflow chain may now belong
	** to a different leaf chain.
	**
	** If the previous page is a LEAF and it has been split since the
	** free operation was logged then use the overflow page's page_main
	** value to get the correct leaf.
	*/
	if ((dmve->dmve_action == DMVE_UNDO) &&
	    (leaf_page_recover) &&
	    (LSN_GT(DM1B_VPT_ADDR_BT_SPLIT_LSN_MACRO(page_type, leaf), 
		log_lsn)) &&
	    ( ! undo_check_done))
	{
	    /*
	    ** At this point, if we are not recovering the overflow page
	    ** then we cannot continue - we need the overflow page to
	    ** determine which leaf to recover.
	    */
	    if (! ovfl_page_recover)
	    {
		uleFormat(NULL, E_DM9668_TABLE_NOT_RECOVERED, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 2, 0, log_rec->bto_tbl_id.db_tab_base,
		    0, log_rec->bto_tbl_id.db_tab_index);
		SETDBERR(&dmve->dmve_error, 0, E_DM9667_NOPARTIAL_RECOVERY);
		break;
	    }

	    /*
	    ** Jump back to the start of the routine and start this recovery
	    ** process all over again using the leaf page number in the
	    ** overflow page's page_main field.
	    **
	    ** We have to unfix our table and pages first for the retry.
	    */
	    leaf_page_number = DM1B_VPT_GET_PAGE_MAIN_MACRO(page_type, ovfl);
	    undo_check_done = TRUE;

	    /*
	    ** Release our current page and the tbio so we can call undo_check
	    ** to perform a logical search for the desired row.
	    */
	    status = dmve_unfix_page(dmve, tbio, leafpinfo);
	    leafpinfo = NULL;
	    if (status != E_DB_OK)
		break;

	    if (ovfl_page_recover)
	    {
		status = dmve_unfix_page(dmve, tbio, ovflpinfo);
		ovflpinfo = NULL;
		if (status != E_DB_OK)
		    break;
	    }

	    status = dmve_unfix_tabio(dmve, &tbio, 0);
	    if (status != E_DB_OK)
		break;
	    tbio = NULL;
	    
	    continue;
	}

	/*
	** Consistency Check:
	** If we are UNDOing an overflow operation and we have both the
	** overflow and leaf fixed, then the overflow better point back
	** to the leaf (and visa versa).
	** (It better in all cases, but we can only check it if we have
	** both the leaf and overflow fixed).
	*/
	if ((dmve->dmve_action == DMVE_UNDO) &&
	    (leaf_page_recover) &&
	    (ovfl_page_recover))
	{
	    if ((DM1B_VPT_GET_PAGE_OVFL_MACRO(page_type, leaf) != 
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, ovfl)) ||
		(DM1B_VPT_GET_PAGE_MAIN_MACRO(page_type, ovfl) != 
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, leaf)))
	    {
		TRdisplay("Consistency Check, UNDO of Btree Overflow action\n");
		TRdisplay("\tLeaf and Overflow don't match:\n");
		TRdisplay("\tLeaf page: %d, overflow pointer %d\n",
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, leaf), 
		    DM1B_VPT_GET_PAGE_OVFL_MACRO(page_type, leaf));
		TRdisplay("\tOverflow page: %d, mainpage pointer %d\n",
		    DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, ovfl), 
		    DM1B_VPT_GET_PAGE_MAIN_MACRO(page_type, ovfl));
	    }
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
	    if (ovfl && LSN_GTE(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, ovfl), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, ovfl),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, ovfl),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, ovfl),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, ovfl),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		ovfl = NULL;
	    }
	    if (leaf && LSN_GTE(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, leaf), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(*tbio->tbio_relid), tbio->tbio_relid,
			sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, leaf),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, leaf),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, leaf),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, leaf),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		leaf = NULL;
	    }
	    break;

	case DMVE_UNDO:
	    if (ovfl && (LSN_LT(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, ovfl),
		log_lsn)))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, ovfl),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, ovfl),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, ovfl),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, ovfl),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    if (leaf && (LSN_LT(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, leaf),
		log_lsn)))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(*tbio->tbio_relid), tbio->tbio_relid,
		    sizeof(*tbio->tbio_relowner), tbio->tbio_relowner,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, leaf),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, leaf),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, leaf),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, leaf),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (status != E_DB_OK || (!leaf && !ovfl) )
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->bto_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_rebtovfl(dmve, tbio, 
	    			(ovfl) ? ovflpinfo : NULL, 
				(leaf) ? leafpinfo : NULL);
	    break;

	case DMVE_UNDO:
	    status = dmv_unbtovfl(dmve, tbio,
	    			(ovfl) ? ovflpinfo : NULL, 
				(leaf) ? leafpinfo : NULL);
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
    ** Error handling: this implies that the abort action failed and the
    ** database will be marked inconsistent by the caller.
    */

    if (ovflpinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, ovflpinfo); 
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (leafpinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, leafpinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (tbio)
    {
	tmp_status = dmve_unfix_tabio(dmve, &tbio, 0);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (status != E_DB_OK)
	SETDBERR(&dmve->dmve_error, 0, E_DM9657_DMVE_BTREE_OVFL);

    return(status);
}

/*{
** Name: dmv_rebtovfl - Redo the Btree Allocate Overflow operation.
**
** Description:
**      This function redoes the updates to a btree required to allocate an
**	overflow leaf page.
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
**	14-dec-1992 (jnash & rogerk)
**	    Written for 6.5 recovery project.
**	15-apr-1993 (rogerk)
**	    Fixed coding bug which caused us to sometimes skip undo recovery.
**	    Only return immediately if BOTH ovfl and leaf are null, not if
**	    either is null.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size argument to dm1cxformat.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros. Also pass the page_size parameter in DM1B_OVER macro.
**	    Change btree key per page calculation to account for tuple header.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      12-jun-97 (stial01)
**          dmv_rebtovfl() Pass new attribute, key parameters to dm1cxformat
**      09-feb-99 (stial01)
**          dmv_rebtovfl() Pass relcmptlvl = 0 to dm1cxkperpage
**	19-Jan-2004 (jenjo02)
**	    Added tidsize for global indexes.
**	13-Apr-2006 (jenjo02)
**	    Add CLUSTERED indicator to dm1cxkperpage, dm1cxformat
**	    prototypes.
*/
static DB_STATUS
dmv_rebtovfl(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *ovflpinfo,
DMP_PINFO	    *leafpinfo)
{
    DM0L_BTOVFL		*log_rec = (DM0L_BTOVFL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->bto_header.lsn; 
    i4		kperpage;
    i4		page_type = log_rec->bto_pg_type;
    i4		ix_compressed;
    char		*lrange_ptr;
    char		*rrange_ptr;
    DB_STATUS		status = E_DB_OK;
    DMPP_ACC_PLV	*plv = dmve->dmve_plv;
    LG_LRI		lri;
    DMPP_PAGE		*ovfl = NULL;
    DMPP_PAGE		*leaf = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( ovflpinfo )
        ovfl = ovflpinfo->page;
    if ( leafpinfo )
        leaf = leafpinfo->page;

    /*
    ** If recovery was found to be unneeded then we can just return.
    */
    if ((ovfl == NULL) && (leaf == NULL))
	return (E_DB_OK);

    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->bto_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /*
    ** Get old range key pointers from variable length section of the 
    ** log record
    */
    lrange_ptr = &log_rec->bto_vbuf[0];
    rrange_ptr = &log_rec->bto_vbuf[log_rec->bto_lrange_len];

    /*
    ** Mutex the overflow pages while updating them.
    */
    if (ovfl)
	dmveMutex(dmve, ovflpinfo);
    if (leaf)
	dmveMutex(dmve, leafpinfo);

    /*
    ** Redo the Allocate operation.
    **
    ** Reformat the old page as a btree leaf overflow page and link it into
    ** the overflow chain by pointing the parent leaf page to it.
    */
    DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
    if (leaf)
    {
	DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, leaf, log_rec->bto_ovfl_pageno);

	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, leaf, DMPP_MODIFY);
	DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, leaf, &lri);
    }

    while (ovfl)
    {
	/* Sanity check tidsize */
	if ( log_rec->bto_tidsize != sizeof(DM_TID) && 
	     log_rec->bto_tidsize != sizeof(DM_TID8) )
	{
	     log_rec->bto_tidsize == sizeof(DM_TID);
	}
	/*
	** Calculate maximum keys that will fit on a leaf/overflow page
	*/
	kperpage = dm1cxkperpage(page_type, log_rec->bto_page_size, 
			0, TCB_BTREE, 0, log_rec->bto_klen, DM1B_PLEAF,
			log_rec->bto_tidsize, 0);

	/*
	** Format overflow leaf page.
	*/
	status = dm1cxformat(page_type, log_rec->bto_page_size, ovfl,
	    plv, ix_compressed,
	    kperpage, log_rec->bto_klen,
	    (DB_ATTS **)NULL, (i4)0, (DB_ATTS **)NULL, (i4)0,
	    log_rec->bto_ovfl_pageno, DM1B_POVFLO, TCB_BTREE,
	    log_rec->bto_tidsize, 0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E5_BAD_INDEX_FORMAT, (DMP_RCB *)NULL, ovfl, 
			   page_type, log_rec->bto_page_size, 0);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9659_REDO_BTREE_OVFL);
	    break;
	}

	/*
	** Fill in the range key values.
	*/
	status = dm1cxreplace(page_type, log_rec->bto_page_size,
	    ovfl, DM1C_DIRECT, ix_compressed,
	    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, 
	    DM1B_LRANGE,lrange_ptr,log_rec->bto_lrange_len,
	    &log_rec->bto_lrtid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
		ovfl, page_type, log_rec->bto_page_size,DM1B_LRANGE);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9659_REDO_BTREE_OVFL);
	    break;
	}

	status = dm1cxreplace(page_type, log_rec->bto_page_size, 
	    ovfl, DM1C_DIRECT, ix_compressed,
	    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0,
	    DM1B_RRANGE,rrange_ptr,log_rec->bto_rrange_len,
	    &log_rec->bto_rrtid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
		ovfl, page_type, log_rec->bto_page_size,DM1B_RRANGE);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9659_REDO_BTREE_OVFL);
	    break;
	}

	/*
	** Link the new overflow page into the overflow chain.
	*/
	DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, ovfl, log_rec->bto_mainpage);
	DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, ovfl, log_rec->bto_nextovfl);
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(page_type, ovfl, log_rec->bto_nextpage);

	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, ovfl, DMPP_MODIFY);
	DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, ovfl, *log_lsn);
	break;
    }

    if (ovfl)
	dmveUnMutex(dmve, ovflpinfo);
    if (leaf)
	dmveUnMutex(dmve, leafpinfo);
    
    return(status);
}

/*{
** Name: dmv_unbtovfl - Undo the Btree Allocate Overflow operation.
**
** Description:
**      This function undoes the updates to a btree done by the allocation
**	of a Btree Overflow Leaf Page.
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
**	14-dec-1992 (jnash & rogerk)
**	    Written for 6.5 recovery project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**	    Also, fix bug where *log_lsn, not lsn, insert onto page.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	26-apr-1993 (rogerk)
**	    Don't reference through leaf pointer when the leaf is not
**	    present during recovery processing (if for instance, we are
**	    in REDO and the leaf page did not need recovery).
**	21-jun-1993 (rogerk)
**	    Add error messages.
**	15-apr-1994 (chiku)
**	    Bug56702: returned logfull indication.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_unbtovfl(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *ovflpinfo,
DMP_PINFO	    *leafpinfo)
{
    DM0L_BTOVFL		*log_rec = (DM0L_BTOVFL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->bto_header.lsn; 
    LG_LSN		lsn; 
    i4		dm0l_flags;
    i4		loc_id;
    i4		leaf_loc_id;
    i4		leaf_page_number;
    i4		page_type = log_rec->bto_pg_type;
    char		*lrange_ptr;
    char		*rrange_ptr;
    DB_STATUS		status = E_DB_OK;
    i4			local_err;
    i4			*err_code = &dmve->dmve_error.err_code;
    LG_LRI		lri;
    DMPP_PAGE		*ovfl = NULL;
    DMPP_PAGE		*leaf = NULL;

    CLRDBERR(&dmve->dmve_error);

    if ( ovflpinfo )
        ovfl = ovflpinfo->page;
    if ( leafpinfo )
        leaf = leafpinfo->page;

    /*
    ** If recovery was found to be unneeded then we can just return.
    */
    if ((ovfl == NULL) && (leaf == NULL))
	return (E_DB_OK);

    /*
    ** Get old range key pointers from variable length section of the 
    ** log record
    */
    lrange_ptr = &log_rec->bto_vbuf[0];
    rrange_ptr = &log_rec->bto_vbuf[log_rec->bto_lrange_len];

    /*
    ** Get leaf page number for log call below.
    **
    ** On btree operations, splits may cause rows to move such that the
    ** undo action for some update operation is applied to a different leaf
    ** than the original update.
    **
    ** When no 'leaf' is present in recovery then we can be assured that we
    ** have not been affected by a split as this is checked by the caller.
    */
    leaf_page_number = log_rec->bto_leaf_pageno;
    leaf_loc_id = log_rec->bto_leaf_loc_id;
    if (leaf)
    {
	leaf_page_number = DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, leaf);
	loc_id = DM2F_LOCID_MACRO(tabio->tbio_loc_count, 
					(i4)leaf_page_number);
	leaf_loc_id = tabio->tbio_location_array[loc_id].loc_config_id;
    }

    /*
    ** Mutex the overflow pages while updating them.
    */
    if (ovfl)
	dmveMutex(dmve, ovflpinfo);
    if (leaf)
	dmveMutex(dmve, leafpinfo);

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
    if ((log_rec->bto_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    /*
	    ** Extract previous page change info for current
	    ** No undo DM0LBTOVFL if done in mini transaction,
	    ** so LRI in DM0LBTOVFL CLR records is probably never used 
	    **
	    ** Undo BTOVFL without all pages fixed is probably only possible
	    ** during rollforwarddb
	    */
	    if (leaf)
	    {
		DM1B_VPT_GET_PAGE_LRI_MACRO(log_rec->bto_pg_type, leaf, &lri);
	    }
	    else
		DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);


	    dm0l_flags = (log_rec->bto_header.flags | DM0L_CLR);

	    status = dm0l_btovfl(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->bto_tbl_id, &log_rec->bto_tblname, 
		&log_rec->bto_tblowner,
		log_rec->bto_pg_type, log_rec->bto_page_size,
		log_rec->bto_cmp_type, 
		log_rec->bto_loc_cnt, log_rec->bto_klen,
		leaf_loc_id, log_rec->bto_ovfl_loc_id,
		leaf_page_number, log_rec->bto_ovfl_pageno,
		log_rec->bto_mainpage, log_rec->bto_nextpage, 
		log_rec->bto_nextovfl,
		log_rec->bto_lrange_len, lrange_ptr, &log_rec->bto_lrtid,
		log_rec->bto_rrange_len, rrange_ptr, &log_rec->bto_rrtid,
		log_lsn, &lri, log_rec->bto_tidsize, &dmve->dmve_error);

	    lsn = lri.lri_lsn;  /* for ovfl */

	    if (status != E_DB_OK)
	    {
		if (ovfl) 
		    dmveUnMutex(dmve, ovflpinfo);
		if (leaf) 
		    dmveUnMutex(dmve, leafpinfo);
		/*
		 * Bug56702: returned logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;

		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0); 
		SETDBERR(&dmve->dmve_error, 0, E_DM9658_UNDO_BTREE_OVFL);
		return(E_DB_ERROR);
	    }
	}
    }
    else
    {
	/*
	** If we are processing recovery of an Update Ovfl CLR (redo-ing the 
	** undo of an allocate) then we don't log a CLR but instead save the 
	** LSN of the log record we are processing with which to update the 
	** page lsn's.
	*/
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
        lsn = lri.lri_lsn;
    }

    /*
    ** Undo the allocate operation.
    ** 
    ** Unlink the page from the overflow chain by changing the previous page's
    ** overflow pointer to point past the unallocated page.
    **
    ** The unlinked overflow page is not itself updated other than to move
    ** its LSN forward.  It should be formatted later as a Free page when
    ** the DEALLOC log record is processed.
    */
    if (leaf)
    {
	DM1B_VPT_SET_PAGE_OVFL_MACRO(page_type, leaf, log_rec->bto_nextovfl);

	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, leaf, DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, leaf, &lri);
    }

    if (ovfl)
    {
	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, ovfl, DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, ovfl, lsn);
    }

    if (ovfl)
	dmveUnMutex(dmve, ovflpinfo);
    if (leaf)
	dmveUnMutex(dmve, leafpinfo);
    
    return(E_DB_OK);
}
