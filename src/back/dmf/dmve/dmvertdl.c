/*
** Copyright (c) 1996, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <me.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
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
#include    <dm1m.h>
#include    <dm1cx.h>
#include    <dm1r.h>
#include    <dm1h.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dmd.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVEBTDEL.C - The recovery of a rtree delete key operation.
**
**  Description:
**	This file contains the routine for the recovery of a 
**	delete key operation to a rtree index/leaf page.
**
**          dmve_rtdel - The recovery of a delete key operation.
**
**  History:
**	19-sep-1996 (shero03)
**	    Created from dmvebtdl.c
**	22-nov-1996 (stial01,dilam04)
**	    Row Locking Project:
**	    Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**	    When calling dm1cxdel(), pass reclaim_space parm
**	25-nov-1996 (shero03)
**	    Find the correct bid in the case where the deleted record's position has
**	    moved particularly after a page-split.
**      27-feb-97 (stial01)
**          Init flag param for dm1cxdel()
**	15-apr-1997 (shero03)
**	    Remove dimension from getklv, log record
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
**	    dm0l_rtdel, eliminating need for dmve_name_unpack().
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-feb-2001 (stial01)
**          Pass pg type to dmve_trace_page_info (Variable Page Type SIR 102955)
**      01-may-2001 (stial01)
**          Fixed parameters to ule_format (B104618)
**      20-jun-2003 (stial01)
**          Moved external declaration of dmve_rtadjust_mbrs() to dmve.h
**      14-jul-2003
**          Changed declaration of dmve_rtadjust_mbrs to use DMPP_PAGE
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
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
**	03-Dec-2008 (jonj)
**	    SIR 120874: dm1m_? functions converted to DB_ERROR *
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

static DB_STATUS 	dmv_rertree_del(
  	DMVE_CB             *dmve,
	ADF_CB		    *adf_scb,
  	DMP_TABLE_IO	    *tabio,
  	DMP_PINFO	    *pinfo,
	DM_TID		    *bid,
	DM0L_RTDEL	    *log_rec,
	DMPP_ACC_PLV 	    *plv,
	DMPP_ACC_KLV	    *klv);

static DB_STATUS	dmv_unrtree_del(
	DMVE_CB             *dmve,
	ADF_CB		    *adf_scb,
	DMP_TABLE_IO	    *tabio,
  	DMP_PINFO	    *pinfo,
	DM_TID		    *bid,
	DM0L_RTDEL	    *log_rec,
	DMPP_ACC_PLV 	    *plv,
	DMPP_ACC_KLV	    *klv);


/*{
** Name: dmve_rtdel - The recovery of a delete key operation.
**
** Description:
**	This function is used to do, redo and undo a delete key
**	operation to a rtree index/leaf page. This function adds or
**	deletes the key from the index depending on the recovery mode.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the rtree delete operation
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
**	    Created from dmvebtdl.c
**	22-nov-1996 (stial01,dilam04)
**	    Row Locking Project:
**	    Add lock_type argument to dm0p_lock_page and dm0p_unlock_page
**	    When calling dm1cxdel(), pass reclaim_space parm
**	25-nov-1996 (shero03)
**	    Find the correct bid in the case where the deleted record's position
**	    has moved particularly after a page-split.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      23-feb-2004 (thaju02) Bug 111470 INGSRV2635
**          For rollforwarddb -b option, do not compare the LSN's on the
**          page to that of the log record.
**	01-Dec-2004 (jenjo02)
**	    Added DM0P_TABLE_LOCKED_X fix_action, missed by changes
**	    for bug 108074. Pass fix_action to dmve_rtadjust_mbrs.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
**	17-Apr-2008 (kibro01) b120276
**	    Initialise ADF_CB structure
*/
DB_STATUS
dmve_rtdel(
DMVE_CB		*dmve)
{
    DM0L_RTDEL		*log_rec = (DM0L_RTDEL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->rtd_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*page = NULL;
    i4		pos; 
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    ADF_CB		adf_scb;
    DMPP_ACC_PLV	*loc_plv;
    DMPP_ACC_KLV	loc_klv;
    LK_LKID		lockid;
    LK_LKID		page_lockid;
    DM_TID		leaf_bid;
    DM_TID		tid;
    DM_PAGENO		next_page;
    char		*keyp;
    i4		lock_action;
    i4		grant_mode;
    i4		recovery_action;
    i4		fix_action = 0;
    i4		page_type = log_rec->rtd_pg_type;
    DM_TID	*stack;
    i4		stack_level;
    i4		error;
    i4		loc_error;
    bool		physical_page_lock = FALSE;
    bool		in_range;
    bool		entry_found = TRUE;
    bool		requalify_needed = FALSE;
    bool		is_leaf;
    char		key[DB_MAXRTREE_KEY];		
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
    leaf_bid = log_rec->rtd_bid;

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->rtd_header.type != DM0LRTDEL)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    break;
	}

	/*
	** Check for partial recovery during UNDO operations.
	** We cannot bypass recovery of index updates even if the page
	** is on a location which is not being recovered.  This is because
	** the logged page number may not be the page that actually needs
	** the UNDO action!!!  If the page which was originally updated
	** has been since SPLIT, then the logged key may have moved to a
	** new index/leaf page.  It is that new page to which this undo
	** action must be applied.  And that new page may be on a location
	** being recovered even if the original page is not.
	**
	** If recovery is being bypassed on the entire table then no recovery
	** needs to be done.
	*/
	if ((dmve->dmve_action == DMVE_UNDO) &&
	    (dmve_location_check(dmve, (i4)log_rec->rtd_cnf_loc_id) == FALSE))
	{
	  uleFormat(NULL, E_DM9668_TABLE_NOT_RECOVERED, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&loc_error, 2, 0, log_rec->rtd_tbl_id.db_tab_base,
		0, log_rec->rtd_tbl_id.db_tab_index);
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
	status = dmve_fix_tabio(dmve, &log_rec->rtd_tbl_id, &tbio);
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
		log_rec->rtd_obj_dt_id,
		&loc_klv);

        keyp = &log_rec->rtd_vbuf[log_rec->rtd_stack_size];
	MEcopy(keyp, log_rec->rtd_key_size, key);
	keyp = &key[0];	/* point to the local key */

	/*
	** Get required Table/Page locks before we can write the page.
	**
	** Some Ingres pages are locked only temporarily while they are
	** updated and then released immediately after the update.  The
	** instances of such page types that are recovered through this 
	** routine are system catalog pages.
	**
	** Except for these system catalog pages, we expect that any page
	** which requires recovery here has already been locked by the
	** original transaction and that the following lock requests will
	** not block.
	**
	** Note that if the database is locked exclusively, or if an X table
	** lock is granted then no page lock is requried.
	*/
	for (;;)
	{
	    if ((dcb->dcb_status & DCB_S_EXCLUSIVE) == 0)
	    {
	        /*
	        ** Request IX lock in preparation of requesting an X page lock
	        ** below.  If the transaction already holds an exclusive table
	        ** lock, then an X lock will be granted.  In this case we can
	        ** bypass the page lock request.
	        */
	        status = dm2t_lock_table(dcb, &log_rec->rtd_tbl_id, DM2T_IX, 
			dmve->dmve_lk_id, (i4)0, &grant_mode, &lockid, 
			&dmve->dmve_error);
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
		   if (log_rec->rtd_header.flags & DM0L_PHYS_LOCK)
		      lock_action = LK_PHYSICAL;
	
		   status = dm0p_lock_page(dmve->dmve_lk_id, dcb,
		   	&log_rec->rtd_tbl_id, leaf_bid.tid_tid.tid_page,
		    	LK_PAGE, LK_X, lock_action, (i4)0, tbio->tbio_relid,
			tbio->tbio_relowner, &dmve->dmve_tran_id,
			&page_lockid, (i4 *)NULL, (LK_VALUE *)NULL,
			&dmve->dmve_error);
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
		dmve_trace_page_info(log_rec->rtd_pg_type,
		log_rec->rtd_page_size, page, loc_plv, "Page");

	    /*
	    ** UNDO Recovery SPLIT check:
	    **
	    ** If doing UNDO recovery, we need to verify that this is the
	    ** correct page from which to undo the delete operation.  If the row
	    ** in question has been moved to a different page by a subsequent
	    ** SPLIT operation, then we have to find that new page in order to
	    ** perform the undo.
	    **
	    ** The undo_check will search for the correct BID to which to
	    ** perform the recovery action and return with that value in 
	    ** 'leafbid'.
	    */
	    if ( (requalify_needed) ||
	    	((dmve->dmve_action == DMVE_UNDO) &&
                 (LSN_GT(DM1B_VPT_ADDR_BT_SPLIT_LSN_MACRO(page_type,
		  page), log_lsn)) ) )
		
            {
	        /*
	        ** Go find the key again - it could have been shifted right 
	        */
        	entry_found = FALSE;

		/*
		** Ensure we keep looking for the correct page once we start
		*/
	        if (requalify_needed == FALSE)
		{
		    requalify_needed = TRUE;
		    is_leaf = DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page)
			   & DMPP_LEAF;
		    if (is_leaf)
 	                status = dm1mxhilbert(&adf_scb, &loc_klv, keyp,
                                 keyp + 2 * log_rec->rtd_hilbertsize);    	
		}
		status = dm1m_check_range(page_type, log_rec->rtd_page_size,
			page, &adf_scb, &loc_klv,
			key, &in_range, &dmve->dmve_error);
		if (status != E_DB_OK)
		   break;
		if (in_range)
		{
		    status = dm1mxsearch((DMP_RCB *)NULL, page, DM1C_FIND,
		    		DM1C_EXACT, key, &adf_scb, &loc_klv, 
				page_type, log_rec->rtd_page_size, 
				&tid, &pos, &dmve->dmve_error); 

		    leaf_bid.tid_tid.tid_line = pos;
		    if (status != E_DB_OK && dmve->dmve_error.err_code != E_DM0056_NOPART)
		        break;
		    entry_found = TRUE;
		    status = E_DB_OK;
		    CLRDBERR(&dmve->dmve_error);
		    break;
		}
		else
		{	  
		    next_page = 
			DM1B_VPT_GET_BT_NEXTPAGE_MACRO(page_type, page);
		    if (next_page)
		    {
		        /* Free the current page and move to the next */
		        status = dm0p_uncache_fix(tbio, DM0P_UNFIX,
				     dmve->dmve_lk_id, dmve->dmve_log_id,
				     &dmve->dmve_tran_id, pinfo, 
				     &dmve->dmve_error);
		        if (status != E_DB_OK)
			    break;

			if (physical_page_lock)
			{
			    status = dm0p_unlock_page(dmve->dmve_log_id, dcb,
			   	  &log_rec->rtd_tbl_id,
				  leaf_bid.tid_tid.tid_page, LK_PAGE,
				  tbio->tbio_relid, &dmve->dmve_tran_id,
				  &page_lockid, (LK_VALUE *)NULL, 
				  &dmve->dmve_error);
			    if (status != E_DB_OK)
				break;
			    physical_page_lock = FALSE;
			}
			 
		    leaf_bid.tid_tid.tid_page = next_page;
		    leaf_bid.tid_tid.tid_line = 0;
		    continue;
		    }
		}

	        /* page wasn't found - we're hosed */
	        uleFormat(NULL, E_DM9671_DMVE_BTUNDO_FAILURE, (CL_ERR_DESC *)NULL,
		    	ULE_LOG, NULL, (char *)NULL, (i4)0, (i4)0,
			&error, 0);
		SETDBERR(&dmve->dmve_error, 0, E_DM9669_DMVE_BTUNDO_CHECK);
	        break;
	    }  /* Requalification Needed ? */

	    if (status != E_DB_OK)
	        break; 		
	    if (entry_found)
	    	break;		/* The correct page is now locked */
	}

	if (status != E_DB_OK)
	    break; 		/* Go through error recovery */
		      
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
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type,page),
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
	    if (log_rec->rtd_header.flags & DM0L_CLR)
	        recovery_action = DMVE_UNDO;

	    switch (recovery_action)
	    {
	      case DMVE_DO:
	      case DMVE_REDO:
	        status = dmv_rertree_del(dmve, &adf_scb, tbio, pinfo,
			 &leaf_bid, log_rec, loc_plv, &loc_klv);
	        break;

	      case DMVE_UNDO:
	        status = dmv_unrtree_del(dmve, &adf_scb, tbio, pinfo,
			 &leaf_bid, log_rec, loc_plv, &loc_klv);
	      break;
	    }

	    if (status != E_DB_OK)
	      break;
	  
	/*
	** Adjust the tree from the leaf to the root with the new MBR and
	** LHV. As long as parent's MBR or LHV changes keep going up until
	** you reach the root.  Beware that a root split may have happened.
	*/
	    stack = (DM_TID*) (((char *)log_rec) + sizeof(*log_rec));
	    stack_level = log_rec->rtd_stack_size / sizeof(DM_TID);

	    status = dmve_rtadjust_mbrs(dmve, &adf_scb, tbio,
	  			log_rec->rtd_tbl_id,
	  			stack, stack_level,
				page_type,
				log_rec->rtd_page_size,
				log_rec->rtd_hilbertsize,
				log_rec->rtd_key_size, 
				log_rec->rtd_cmp_type,
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
	    &log_rec->rtd_tbl_id, leaf_bid.tid_tid.tid_page, LK_PAGE,
	    tbio->tbio_relid, &dmve->dmve_tran_id,
	    &page_lockid, (LK_VALUE *)NULL, &local_dberr);
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9651_DMVE_BTREE_DEL);

    return(status);
}


/*{
** Name: dmv_rertree_del - Redo the Put of a rtree key 
**
** Description:
**      This function adds a key to a rtree index for the recovery of a
**	delete record operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      page				Pointer to the page to which to insert
**	log_record			Pointer to the log record
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
**	19-sep-1996 (shero03)
**	    Created from dmvebtdl.c
**	22-nov-1996 (stial01,dilma04)
**	    Row Locking Project:
**	    When calling dm1cxdel(), pass reclaim_space param
**      27-feb-97 (stial01)
**          Init flag param for dm1cxdel()
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	19-Jan-2004 (jenjo02)
**	    Added partition number to dm1cxget, dm1cxtget.
*/
static DB_STATUS
dmv_rertree_del(
DMVE_CB             *dmve,
ADF_CB		    *adf_scb,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo,
DM_TID		    *bid,
DM0L_RTDEL	    *log_rec,
DMPP_ACC_PLV 	    *plv,
DMPP_ACC_KLV	    *klv)
{
    LG_LSN		*log_lsn = &log_rec->rtd_header.lsn; 
    DB_STATUS		status = E_DB_OK;
    i4		childkey;
    i4		childtid;
    DM_TID	deltid;
    char	*key;
    char	*key_ptr;
    i4		key_len;
    i4		page_type = log_rec->rtd_pg_type;
    i4		ix_compressed;
    bool	index_update;
    bool	is_leaf;
    i4		*err_code = &dmve->dmve_error.err_code;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If there is nothing to recover, just return.
    */
    if (page == NULL)
	  return (E_DB_OK);

    key = &log_rec->rtd_vbuf[log_rec->rtd_stack_size];

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	DMPP_INDEX) != 0);

    is_leaf = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_LEAF) != 0);
    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->rtd_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /*
    ** Deletes to non-leaf index pages actually effect more than one entry
    ** on the page.  The logged bid describes the entry from which the
    ** TID pointer is deleted.  The key entry is deleted from the previous
    ** position (if there is one).
    */
    childtid = bid->tid_tid.tid_line;
    childkey = bid->tid_tid.tid_line;

    if (index_update && (childkey != 0))
	childkey--;

    /*
    ** Consistency Checks:
    **
    ** Verify that there is an entry at the indicated BID and that it
    ** matches the logged key, tid entry.
    ** 
    */

    dm1cxrecptr(page_type, log_rec->rtd_page_size, page, childkey, &key_ptr);
    dm1cxtget(page_type, log_rec->rtd_page_size, page, childtid, 
		&deltid, (i4*)NULL);

    /*
    ** We can only validate the key size on compressed tables; otherwise
    ** we must assume that the logged value was the correct table key length.
    */
    key_len = log_rec->rtd_key_size;
    if (ix_compressed != DM1CX_UNCOMPRESSED)
    {
	dm1cx_klen(page_type, log_rec->rtd_page_size, page, 
		childkey, &key_len);
    }

    /*
    ** Compare the key,tid pair we are about to delete with the one we logged
    ** to make sure they are identical.  If the keys don't match but the tids
    ** do, then we make an assumption here that the mismatch is most likely due
    ** to this check being wrong (we have garbage at the end of the tuple 
    ** buffer or we allowed some sort of non-logged update to the row) and 
    ** we continue with the operation after logging the unexpected condition.
    */
    if ((log_rec->rtd_key_size != key_len) ||
	(MEcmp((PTR)key, (PTR)key_ptr, key_len) != 0) ||
	(log_rec->rtd_tid.tid_i4 != deltid.tid_i4))
    {
	uleFormat(NULL, E_DM966A_DMVE_KEY_MISMATCH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 8, 
	    sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
	    sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
	    sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
	    0, bid->tid_tid.tid_page, 0, bid->tid_tid.tid_line,
	    5, (index_update ? "INDEX" : "LEAF "),
	    0, log_rec->rtd_bid.tid_tid.tid_page,
	    0, log_rec->rtd_bid.tid_tid.tid_line);
	uleFormat(NULL, E_DM966B_DMVE_KEY_MISMATCH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 7, 
	    0, key_len, 0, log_rec->rtd_key_size,
	    0, deltid.tid_tid.tid_page, 0, deltid.tid_tid.tid_line,
	    0, log_rec->rtd_tid.tid_tid.tid_page,
	    0, log_rec->rtd_tid.tid_tid.tid_line,
	    0, dmve->dmve_action);
	dmd_log(1, (PTR) log_rec, 4096);
	uleFormat(NULL, E_DM9653_REDO_BTREE_DEL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
    }

    /*
    ** Mutex the page while updating it.
    */
    dmveMutex(dmve, pinfo);

    /*
    ** Redo the delete operation.
    */
    for (;;)
    {
	/*
	** If redoing a delete to a non leaf page, save the tid value from the
	** entry we are about to delete from (the key's position) and write it
	** over the entry at the next position (effectively deleting the TID).
	*/
	if (index_update && (childkey != childtid))
	{
	    dm1cxtget(page_type, log_rec->rtd_page_size, 
		page, childkey, &deltid, (i4*)NULL);
	    status = dm1cxtput(page_type, log_rec->rtd_page_size,
		page, childtid, &deltid, (i4)0);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page, 
			page_type, log_rec->rtd_page_size, childtid);
		break;
	    }
	}

	status = dm1cxdel(page_type, log_rec->rtd_page_size, page,
		    DM1C_DIRECT, ix_compressed,
		    &dmve->dmve_tran_id, (u_i2)0, (i4)0, (i4)DM1CX_RECLAIM, childkey);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, (DMP_RCB*)NULL, page, 
		       page_type, log_rec->rtd_page_size, childkey);
	    break;
	}

	break;
    }

    /*
    ** Write the LSN of the Put log record to the updated page.
    */
    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, page, *log_lsn);

    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);
    dmveUnMutex(dmve, pinfo);
    
    if (status != E_DB_OK)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9653_REDO_BTREE_DEL);
	return(E_DB_ERROR);
    }

    return(E_DB_OK);
}

/*{
** Name: dmv_unrtree_del - UNDO of a delete key operation.
**
** Description:
**      This function removes a key from a rtree index for the recovery of a
**	delete record operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      page				Pointer to page on which row was insert
**	log_record			Pointer to the log record
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
**	19-sep-1996 (shero03)
**		Created from dmvebtdl.c)
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	19-Jan-2004 (jenjo02)
**	    Added partition number parm to dm1cxput, dm1cxtput.
*/
static DB_STATUS
dmv_unrtree_del(
DMVE_CB             *dmve,
ADF_CB		    *adf_scb,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo,
DM_TID		    *bid,
DM0L_RTDEL	    *log_rec,
DMPP_ACC_PLV 	    *plv,
DMPP_ACC_KLV	    *klv)
{
    LG_LSN		*log_lsn = &log_rec->rtd_header.lsn; 
    DB_STATUS		status = E_DB_OK;
    i4		childkey;
    i4		childtid;
    LG_LSN		lsn;
    DM_TID		temptid;
    DM_TID		*stack;
    char		*key;
    i4		flags;
    i4		loc_id;
    i4		loc_config_id;
    i4		page_type = log_rec->rtd_pg_type;
    i4		ix_compressed;
    bool		index_update;
    i4			*err_code = &dmve->dmve_error.err_code;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If there is nothing to recover, just return.
    */
    if (page == NULL)
	return (E_DB_OK);

    stack = (DM_TID *) (((char *)log_rec) + sizeof(*log_rec));
    key = &log_rec->rtd_vbuf[log_rec->rtd_stack_size];

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	DMPP_INDEX) != 0);
    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->rtd_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;


    /*                                             
    ** Get information on the location to which the update is being made.
    */
    loc_id = DM2F_LOCID_MACRO(tabio->tbio_loc_count, 
	      (i4) DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page));
    loc_config_id = tabio->tbio_location_array[loc_id].loc_config_id;

    /*
    ** Deletes to non-leaf index pages actually effect more than one entry
    ** on the page.  The logged bid describes the entry from which the
    ** TID pointer is deleted.  The key entry is deleted from the previous
    ** position (if there is one).
    */
    childtid = bid->tid_tid.tid_line;
    childkey = bid->tid_tid.tid_line;

    if (index_update && (childkey != 0))
    {
	childkey--;
	dm1cxtget(page_type, log_rec->rtd_page_size, page,
		childkey, &temptid, (i4*)NULL); 
    }


    /*
    ** It would be nice to verify that the child position logged (or calculated
    ** by recovery) is the correct spot in the table, but since we have
    ** no attribute or key information to go on, we cannot do key comparisons.
    ** We must trust that the values are correct.
    ** 
    ** We assume here that there is sufficient space on the page.  If not,
    ** then the dm1cx calls below will catch the error.
    ** Since we will have backed out any inserts to this page that may have
    ** occurred after the delete, we should be assured that the the row will
    ** still fit.  If it doesn't, then a fatal recovery error will occur.
    */

    /* 
    ** Mutex the page.  This must be done prior to the log write.
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
    **
    ** The CLR for a RTPUT need not contain the key, just the bid.  For now, 
    ** however, we include the entire record for debugging.
    */
    if ((log_rec->rtd_header.flags & DM0L_CLR) == 0)
    {
	  if (dmve->dmve_logging)
      {
	    flags = (log_rec->rtd_header.flags | DM0L_CLR);

	    status = dm0l_rtdel(dmve->dmve_log_id, flags, &log_rec->rtd_tbl_id, 
		 tabio->tbio_relid, 0,
		 tabio->tbio_relowner, 0,
		 log_rec->rtd_pg_type, log_rec->rtd_page_size,
		 log_rec->rtd_cmp_type, 
		 log_rec->rtd_loc_cnt, loc_config_id,
		 log_rec->rtd_hilbertsize, log_rec->rtd_obj_dt_id,
		 bid, &log_rec->rtd_tid, log_rec->rtd_key_size, key, 
		 log_rec->rtd_stack_size, stack,  
		 log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
	      dmveUnMutex(dmve, pinfo);
	      /*
	       * Bug56702: return logfull indication.
	      */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
	      uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 

		SETDBERR(&dmve->dmve_error, 0, E_DM9652_UNDO_BTREE_DEL);
	      return(E_DB_ERROR);
	      }
	  }
    }
    else
    {
	  /*
	  ** If we are processing recovery of an Insert CLR (redo-ing the undo
	  ** of an insert) then we don't log a CLR but instead save the LSN
	  ** of the log record we are processing with which to update the
	  ** page lsn's.
	  */
	  lsn = *log_lsn;
    }

    /* 
    ** Unless nologging, write the LSN of the delete record onto the page.
    */
    if (dmve->dmve_logging)
	  DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, page, lsn);

    /*
    ** Undo the delete operation.
    */
    for (;;)
    {
	/*
	** Reallocate space on the page for the key,tid pair.
	*/
	status = dm1cxallocate(page_type, log_rec->rtd_page_size,
			page, DM1C_DIRECT, ix_compressed, 
			&dmve->dmve_tran_id, (i4)0, childkey,
			log_rec->rtd_key_size + DM1B_TID_S);
	if (status != E_DB_OK)
	{
	  dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)NULL, page,
		    page_type, log_rec->rtd_page_size, childkey);
	  break;
	}

	/*
	** Reinsert the key,tid values.
	*/
	status = dm1cxput(page_type, log_rec->rtd_page_size, 
			page, ix_compressed, DM1C_DIRECT, 
			&dmve->dmve_tran_id, (u_i2)0, (i4)0, childkey,
			key, log_rec->rtd_key_size, &log_rec->rtd_tid,
			(i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E4_BAD_INDEX_PUT, (DMP_RCB *)NULL, page,
		page_type, log_rec->rtd_page_size, childkey);
	    break;
	}

	/*
	** If the insert is to a non-leaf index page, then the logged tid
	** value must actually be insert to the position after the one
	** to which we just put the key.  Replace the old tidp from that
	** position and insert the new one to the next entry.
	*/
	if (index_update && (childkey != childtid))
	{
	    status = dm1cxtput(page_type, log_rec->rtd_page_size,
			page, childtid, &log_rec->rtd_tid, (i4)0);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page,
			page_type, log_rec->rtd_page_size, childtid);
		break;
	    }
	    status = dm1cxtput(page_type, log_rec->rtd_page_size,
		    page, childkey, &temptid, (i4)0);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page, 
			page_type, log_rec->rtd_page_size, childkey);
		break;
	    }
	}

	break;
    }

    DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);
    dmveUnMutex(dmve, pinfo);
    
    if (status != E_DB_OK)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9652_UNDO_BTREE_DEL);
	return(E_DB_ERROR);
    }

    return(E_DB_OK);
}
