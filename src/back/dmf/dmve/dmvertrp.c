/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <tr.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dm2r.h>
#include    <dm2f.h>
#include    <dm1c.h>
#include    <dm1b.h>
#include    <dm1cx.h>
#include    <dm1r.h>
#include    <dm1h.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dmftrace.h>


/**
**
**  Name: DMVERTRP.C - The recovery of an rtree replace key operation.
**
**  Description:
**	This file contains the routine for the recovery of a 
**	replace key operation to a rtree index/leaf page.
**
**          dmve_rtrep - The recovery of an rtree replace key operation.
**
**  History:
**	19-sep-1996 (shero03)
**	    Created from dmvebtpt.c
**	22-nov-96 (stial01,dilma04)
**	    Row Locking Project
**	    Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**	15-apr-1997 (shero03)
**	    Remove dimension from getklv
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
** 	25-mar-1999 (wonst02)
** 	    Fix RTREE bug B95350 in Rel. 2.5.
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_rtrep, eliminating need for dmve_name_unpack().
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Pass pg type to dmve_trace_page_info (Variable Page Type SIR 102955
**      19-Jun-2002 (horda03) Bug 108074
**          Prevent FORCE_ABORT transaction filling TX log
**          file by not flushing SCONCUR pages unless we
**          really need to (i.e. the table isn't locked
**          exclusively).
**      20-jun-2003 (stial01)
**          Moved external declaration of dmve_rtadjust_mbrs() to dmve.h
**          Fix args to dmve_rtadjust_mbrs(), need to pass page type.
**      14-jul-2003
**          Changed declaration of dmve_rtadjust_mbrs to use DMPP_PAGE
**	19-Jan-2004 (jenjo02)
**	    Added partition number to dm1cxget, dm1cxtget,
**	    dm1cxput, dm1cxtput.
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
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**      01-apr-2010 (stial01)
**          Changes for Long IDs, move consistency check to dmveutil
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*
** Forward Declarations
*/

static DB_STATUS	dmv_rertree_rep(
				DMVE_CB             *dmve,
				DMP_TABLE_IO	    *tabio,
				DMP_PINFO	    *pinfo,
				DM_TID		    *bid,
				DM0L_RTREP	    *log_rec,
				DMPP_ACC_PLV 	    *plv,
				i4		    recovery_action);


/*{
** Name: dmve_rtrep - The recovery of a rtree replace key operation.
**
** Description:
**	This function is used to do, redo and undo a replace key
**	operation to a rtree index/leaf page. This function replaces
**	the old or new value of the key in the index depending on the recovery mode.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the system catalogs put 
**				    operation.
**	    .dmve_action	    Should be DMVE_DO, DMVE_REDO, or DMVE_UNDO.
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
**	    E_DB_FATAL			Operation was partially completed,
**					the transaction must be aborted.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	19-sep-1996 (shero03)
**	    Created from dmvebtpt.c
**	22-nov-96 (stial01,dilma04)
**	    Row Locking Project
**	    Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      19-Jun-2002 (horda03) Bug 108074
**          If the table is locked exclusively, then indicate that SCONCUR
**          pages don't need to be flushed immediately.
**      23-feb-2004 (thaju02) Bug 111470 INGSRV2635
**          For rollforwarddb -b option, do not compare the LSN's on the
**          page to that of the log record.
**	01-Dec-2004 (jenjo02)
**	    Pass fix_action to dmve_rtadjust_mbrs for bug 108074.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
*/
DB_STATUS
dmve_rtrep(
DMVE_CB		*dmve)
{
    DM0L_RTREP		*log_rec = (DM0L_RTREP *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->rtr_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    ADF_CB		adf_scb;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*page = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DMPP_ACC_PLV	*loc_plv;
    DMPP_ACC_KLV	loc_klv;
    LK_LKID		lockid;
    LK_LKID		page_lockid;
    DM_TID		leaf_bid;
    DM_TID		*stack;
    i4			stack_level;
    i4		lock_action;
    i4		grant_mode;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4		page_type = log_rec->rtr_pg_type;
    bool		physical_page_lock = FALSE;
    bool		undo_check_done = FALSE;
    i4                  fix_action = 0;
    DB_ERROR		local_dberr;
    DMP_PINFO		*pinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    MEfill(sizeof(LK_LKID), 0, &lockid);
    MEfill(sizeof(LK_LKID), 0, &page_lockid);
    MEfill(sizeof(ADF_CB),0,(PTR)&adf_scb);

    /*
    ** Store BID of insert into a local variable.  The insert BID may
    ** be modified in undo recovery by the dmve_btunto_check routine.
    */
    leaf_bid = log_rec->rtr_bid;

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->rtr_header.type != DM0LRTREP)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	/*
	** If recovery is being bypassed on the entire table then no recovery
	** needs to be done.
	*/
	if ((dmve->dmve_action == DMVE_UNDO) &&
	    (dmve_location_check(dmve, (i4)log_rec->rtr_cnf_loc_id) == FALSE))
	{
            uleFormat(NULL, E_DM9668_TABLE_NOT_RECOVERED, (CL_ERR_DESC *)NULL,
                ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
                &loc_error, 2, 0, log_rec->rtr_tbl_id.db_tab_base,
                0, log_rec->rtr_tbl_id.db_tab_index);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9667_NOPARTIAL_RECOVERY);
	    break;
	}

	/*
	** Get handle to a tableio control block with which to read
	** and write pages needed during recovery.
	**
	** Warning return indicates that no tableio control block was
	** built because no recovery is needed on any of the locations 
	** described in this log record.  Note that check above prevents
	** this case from occurring during UNDO recovery.
	*/
	status = dmve_fix_tabio(dmve, &log_rec->rtr_tbl_id, &tbio);
	if (status == E_DB_WARN && 
		dmve->dmve_error.err_code == W_DM9660_DMVE_TABLE_OFFLINE)
	{
	    CLRDBERR(&dmve->dmve_error);
	    return (E_DB_OK);
	}

	if (status != E_DB_OK)
	    break;


	/*
	** Get page accessors for page recovery actions.
	*/
	dm1c_get_plv(page_type, &loc_plv);

	adf_scb.adf_errcb.ad_ebuflen = 0;
	adf_scb.adf_errcb.ad_errmsgp = 0;
	adf_scb.adf_maxstring = DB_MAXSTRING;
	dm1c_getklv(&adf_scb,
		log_rec->rtr_obj_dt_id,
		&loc_klv);

	/*
	** Get required Table/Page locks before we can write the page.
	**
	** Some Ingres pages are locked only temporarily while they are
	** updated and then released immediately after the update.  The
	** instances of such page types that are recovered through this 
	** routine are system catalog pages.
	**
	** Except for these system catalog pages, we expect that any page
	 * which requires recovery here has already been locked by the
	** original transaction and that the following lock requests will
	** not block.
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
	    status = dm2t_lock_table(dcb, &log_rec->rtr_tbl_id, DM2T_IX, 
		dmve->dmve_lk_id, (i4)0, &grant_mode, &lockid, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;

	    if (grant_mode != DM2T_X)
	    {         
		/*
		** Page lock required.  If this is a system catalog page
		** or a non-leaf index page then we need to request a 
		** physical lock (and release it later).
		*/
		lock_action = LK_LOGICAL;
		if (log_rec->rtr_header.flags & DM0L_PHYS_LOCK)
		    lock_action = LK_PHYSICAL;
	
		status = dm0p_lock_page(dmve->dmve_lk_id, dcb, 
		    &log_rec->rtr_tbl_id, leaf_bid.tid_tid.tid_page, 
		    LK_PAGE, LK_X, lock_action, (i4)0, tbio->tbio_relid, 
		    tbio->tbio_relowner, &dmve->dmve_tran_id, &page_lockid,
		    (i4 *)NULL, (LK_VALUE *)NULL, &dmve->dmve_error);
		if (status != E_DB_OK)
		    break;

		if (lock_action == LK_PHYSICAL)
		    physical_page_lock = TRUE;
	    }
            else
               fix_action |= DM0P_TABLE_LOCKED_X;
	}

	/*
	** Fix the page we need to recover in cache for write.
	*/
	status = dmve_cachefix_page(dmve, log_lsn,
				    tbio, leaf_bid.tid_tid.tid_page,
				    fix_action, loc_plv,
				    &pinfo);
	if (status != E_DB_OK)
	    break;
	page = pinfo->page;

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	    dmve_trace_page_info(log_rec->rtr_pg_type, log_rec->rtr_page_size,
		page, loc_plv, "Page");

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
          if (LSN_GTE(
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, page),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, page),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		page = NULL;
	    }
	    break;

	case DMVE_UNDO:
          if (LSN_LT(
		         DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), 
		         log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, page),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, page),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (status != E_DB_OK)
	    break;


	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	if (page)
	{
	    recovery_action = dmve->dmve_action;
	    if (log_rec->rtr_header.flags & DM0L_CLR)
	        recovery_action = DMVE_UNDO;

            status = dmv_rertree_rep(dmve, tbio, pinfo, &leaf_bid, log_rec, 
					loc_plv, recovery_action);

	    if (status != E_DB_OK)
	        break;
	/*
	** Adjust the tree from the leaf to the root witih the new MBR and
	** LHV.  As long as the parent's MBR or LHV changes, keep going up 
	** the tree until you reach the root.  Beware that a root split may
	** have occured.
	*/
	    stack = (DM_TID*) (((char *)log_rec) + sizeof(*log_rec));
	    stack_level = log_rec->rtr_stack_size / sizeof(DM_TID);
	    status = dmve_rtadjust_mbrs(dmve, &adf_scb, tbio,
			log_rec->rtr_tbl_id,
			stack, stack_level,
			page_type,
			log_rec->rtr_page_size,
			log_rec->rtr_hilbertsize,
			log_rec->rtr_nkey_size,
			log_rec->rtr_cmp_type,
			pinfo, loc_plv, &loc_klv,
			fix_action);
	    if (status != E_DB_OK)
	        break;
	}  /* good page */

	break;
    }

    if ((status != E_DB_OK) && (dmve->dmve_error.err_code))
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
    }

    /*
    ** Unfix the updated page.  No need to force it to disk - it
    ** will be tossed out through normal cache protocols if Fast
    ** Commit or at the end of the abort if non Fast Commit.
    */
    if (pinfo && pinfo->page)
    {
	tmp_status = dm0p_uncache_fix(tbio, DM0P_UNFIX, dmve->dmve_lk_id, 
		dmve->dmve_log_id, &dmve->dmve_tran_id, pinfo, &local_dberr);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}
    }

    /*
    ** If a short term physical lock was acquired on the page
    ** then release it.
    */
    if (physical_page_lock)
    {
	tmp_status = dm0p_unlock_page(dmve->dmve_lk_id, dcb, 
	    &log_rec->rtr_tbl_id, leaf_bid.tid_tid.tid_page, LK_PAGE,
	    tbio->tbio_relid, &dmve->dmve_tran_id, &page_lockid, 
	    (LK_VALUE *)NULL, &local_dberr);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}
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
	SETDBERR(&dmve->dmve_error, 0, E_DM964E_DMVE_BTREE_PUT);

    return(status);
}


/*{
** Name: dmv_rertree_rep - Redo the replace of a rtree key 
**
** Description:
**      This function replaces a new key in a rtree index for the recovery of a
**	replace record operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      page				Pointer to the page to which to insert
**		log_record		 	Pointer to the log record
**		plv	 				Pointer to page level accessor 
**		recovery_action		Recovery type
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
**	19-sep-1996 (shero03)
**		Created from dmvebtpt.c
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_rertree_rep(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo,
DM_TID		    *bid,
DM0L_RTREP	    *log_rec,
DMPP_ACC_PLV 	    *plv,
i4		    recovery_action)
{
    LG_LSN		*log_lsn = &log_rec->rtr_header.lsn; 
    DB_STATUS		status = E_DB_OK;
    i4		child = bid->tid_tid.tid_line;
    i4		page_type = log_rec->rtr_pg_type;
    i4		ix_compressed;
    char	*old_key;
    char	*new_key;
    char	*key;
    i4		*err_code = &dmve->dmve_error.err_code;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If there is nothing to recover, just return.
    */
    if (page == NULL)
	  return (E_DB_OK);

    old_key = &log_rec->rtr_vbuf[log_rec->rtr_stack_size];
    new_key = old_key + log_rec->rtr_stack_size + log_rec->rtr_okey_size;

    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->rtr_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /* ****** FIXME (kschendel)
    ** Maybe I'm missing something obvious, but I don't see it writing the
    ** rtrep CLR when it's an UNDO...
    ** Do we somehow not need it?  or is it a bug?
    */

    /*
    ** Mutex the page while updating it.
    */
    dmveMutex(dmve, pinfo);

    /*
    ** If redoing the replace operation
	**   use the new mbr value
	** If undoing the replace operation
	**   use the old mbr value
    */
	switch (dmve->dmve_action)
	{
	  case DMVE_DO:
	  case DMVE_REDO:
		key = new_key;
	    break;
	  case DMVE_UNDO :
		key = old_key;
		break;
	}
	/*
	** Replace the mbr with the new value.
	*/
	status = dm1cxput(page_type, log_rec->rtr_page_size, page,
			ix_compressed, DM1C_DIRECT,
	    		&dmve->dmve_tran_id, (u_i2)0, (i4)0, child,
			key, log_rec->rtr_nkey_size, &log_rec->rtr_tid,
			(i4)0);
	if (status != E_DB_OK)
	{
	  dm1cxlog_error(E_DM93E4_BAD_INDEX_PUT, (DMP_RCB *)NULL, page, 
			   page_type, log_rec->rtr_page_size, child);
	}

    /*
    ** Write the LSN of the Replace log record to the updated page.
    */
    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, page, *log_lsn);

    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);
    dmveUnMutex(dmve, pinfo);
    
    if (status != E_DB_OK)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9650_REDO_BTREE_PUT);
	  return(E_DB_ERROR);
    }

    return(E_DB_OK);
}
