/*
** Copyright (c) 1992, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <di.h>
#include    <pc.h>
#include    <me.h>
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
**  Name: DMVEBTDEL.C - The recovery of a btree delete key operation.
**
**  Description:
**	This file contains the routine for the recovery of a 
**	delete key operation to a btree index/leaf page.
**
**          dmve_btdel - The recovery of a delete key operation.
**
**  History:
**	14-dec-1992 (rogerk)
**	    Written for 6.5 recovery.
**	18-jan-1992 (rogerk)
**	    Add check in redo routine for case when null page pointer is
**	    passed because redo was found to be not needed.
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
**	    Added error messages.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	26-jul-1993 (rogerk)
**	    Added tracing of page information during recovery.
**      11-aug-93 (ed)
**          added missing includes
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      06-mar-1996 (stial01)
**          Pass btd_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Do not get page lock if row locking.
**          Check/adjust bid if row locking.
**          When calling dm1cxdel(), pass reclaim_space param
**          Allocate space only if space reclaimed
**     12-dec-96 (dilma04)
**          Corrected mistake in dmve_btdel() which caused inconsistency 
**          between row_locking value and DM0L_ROW_LOCK flag after lock 
**          escalation.
**      04-feb-97 (stial01)
**          dmv_rebtree_del() Tuple headers are on LEAF and overflow (CHAIN) pgs
**          Tuple headers are not on INDEX pages.
**      27-feb-97 (stial01)
**          dmve_btdel() If row locking entries can shift requiring btundo_check
**          Fix page without locking and then mutex it if we have IX page lock.
**          Error if DO/REDO and only IX page lock.
**          Clean ALL deleted leaf entries if X page lock.
**          dmv_rebtree_del() Space is reclaimed when redoing DELETE,
**          unless last entry on leaf page with overflow chain.
**          dmv_unbtree_del() allocate parameter TRUE If dm1cxallocate needed 
**          Log key in CLR, needed for row locking
**      07-apr-97 (stial01)
**          dmv_rebtree_del() NonUnique primary btree (V2) dups span leaf pages,
**          not overflow chain. Remove code skipping reclaim of overflow key
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      29-may-1997 (stial01)
**          dmve_btdel: REDO recovery: bid check only if redo necessary
**      18-jun-1997 (stial01)
**          dmve_btdel() Request IX page lock if row locking.
**      30-jun-97 (dilma04)
**          Bug 83301. Set DM0P_INTENT_LK fix action if row locking.
**      30-jun-97 (stial01)
**          Use dm0p_lock_buf_macro to lock buffer to reduce mutex duration
**          Don't unfix tabio for bid check, changed args to dmve_bid_check()
**      29-jul-97 (stial01)
**          Validate/adjust bid if page size != DM_COMPAT_PGSIZE
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_ufx_tabio_cb() calls.
**      06-jan-98 (stial01)
**          If page size 2k, unfix pages,tbio before bid check (B87385) 
**      15-jan-98 (stial01)
**          dm1bxclean() Fixed dm1cxclean parameters, error handling
**	23-Jun-1998 (jenjo02)
**	    Added lock_value parm to dm0p_lock_page() and dm0p_unlock_page().
**	    Utilize lock_id value in both of those functions.
**      07-jul-98 (stial01)
**          New update_mode if row locking, changed paramater to dmve_bid_check
**	14-aug-1998 (nanpr01)
**	    Error code is getting reset to E_DB_OK on error and not making
**	    the database inconsistent.
**      08-dec-1999 (stial01)
**          Undo change for bug#85156, dmpe routines now get INTENT table locks.
**      21-dec-1999 (stial01)
**          Defer leaf clean until insert, generalize for etabs (PHYS PG locks)
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_btdel, eliminating need for dmve_name_unpack().
**	13-jun-2000 (somsa01)
**	    Added LEVEL1_OPTIM for HP-UX 11.0. The compiler was generating
**	    incorrect code with '-O' such that, in dmve_btdel(),
**	    "log_rec->btp_page_size != DM_COMPAT_PGSIZE" was failing, even
**	    though log_rec->btp_page_size was equal to DM_COMPAT_PGSIZE.
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
**      19-Jun-2002 (horda03) Bug 108074
**          Prevent FORCE_ABORT transaction filling TX log
**          file by not flushing SCONCUR pages unless we
**          really need to (i.e. the table isn't locked
**          exclusively).
**	08-nov-2002 (somsa01)
**	    Removed hpb_us5 from LEVEL1_OPTIM.
**      28-mar-2003 (stial01)
**          Child on DMPP_INDEX page may be > 512, so it must be logged
**          separately in DM0L_BTPUT, DM0L_BTDEL records (b109936)
**	16-Jan-2004 (jenjo02)
**	    Added support for Global/partitioned indexes.
**      01-jun-2004 (stial01)
**          Unlock page using lockid only if lockid still valid (b112400)
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
**          dmve_bid_check will unfix page/tbio if needed, don't do it here.
**      14-oct-2009 (stial01)
**          Call dmve_fix_page, dmve_unfix_page
**	29-Oct-2009 (kschendel) SIR 122739
**	    Kill ixklen, add btflags to log record.
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

static DB_STATUS 	dmv_rebtree_del(
				DMVE_CB             *dmve,
				DMP_TABLE_IO	    *tabio,
				DMP_PINFO	    *pinfo,
				DM_TID		    *bid);

static DB_STATUS	dmv_unbtree_del(
				DMVE_CB             *dmve,
				DMP_TABLE_IO	    *tabio,
				DMP_PINFO	    *pinfo,
				DM_TID		    *bid,
				bool                 allocate);


/*{
** Name: dmve_btdel - The recovery of a delete key operation.
**
** Description:
**	This function is used to do, redo and undo a delete key
**	operation to a btree index/leaf page. This function adds or
**	deletes the key from the index depending on the recovery mode.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the btree delete operation
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
**	14-dec-1992 (rogerk)
**	    Written for 6.5 recovery.
**	15-mar-1993 (rogerk)
**	    Reduced Logging - Phase IV:  Added lsn argument to btundo_check.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**      11-sep-1997 (thaju02) bug#85156 - on recovery lock extension
**          table with DM2T_S.  Attempting to lock table with DM2T_IX
**          causes deadlock, dbms resource deadlock error during recovery,
**          pass abort, rcp resource deadlock error during recovery,
**          rcp marks database inconsistent.
**	06-may-1996 (thaju02 & nanpr01)
**	    New Page Format Support: 
**		Change page header references to use macros.
**		Pass page size to dm1c_getaccessors().
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Do not get page lock if row locking.
**          Check/adjust bid if row locking.
**     12-dec-96 (dilma04)
**          Corrected mistake which caused inconsistency between row_locking
**          value and DM0L_ROW_LOCK flag after lock escalation.
**      27-feb-97 (stial01)
**          Whenever row locking, entries can shift requiring btundo_check
**          Fix page without locking and then mutex it if we have IX page lock.
**          Clean ALL deleted leaf entries if X page lock.
**      14-may-97 (dilma04)
**          Cursor Stability Project:
**          Add lock_id argument to dm0p_lock_page and dm0p_unlock_page.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      18-jun-1997 (stial01)
**          dmve_btdel() Request IX page lock if row locking.
**      30-jun-97 (dilma04)
**          Bug 83301. Set DM0P_INTENT_LK fix action if row locking.
**      30-jun-97 (stial01)
**          Use dm0p_lock_buf_macro to lock buffer to reduce mutex duration
**          Don't unfix tabio for bid check, changed args to dmve_bid_check()
**      29-jul-97 (stial01)
**          Validate/adjust bid if page size != DM_COMPAT_PGSIZE
**      06-jan-98 (stial01)
**          If page size 2k, unfix pages,tbio before bid check (B87385) 
**      19-Jun-2002 (horda03) Bug 108074
**          If the table is locked exclusively, then indicate that SCONCUR
**          pages don't need to be flushed immediately.
**      23-feb-2004 (thaju02) Bug 111470 INGSRV2635
**          For rollforwarddb -b option, do not compare the LSN's on the
**          page to that of the log record.
**	13-Apr-2006 (jenjo02)
**	    CLUSTERED Btree leaf pages don't have attributes - check
**	    that before calling dmve_bid_check.
**	13-Jun-2006 (jenjo02)
**	    Deletes of leaf entries may be up to
**	    DM1B_MAXLEAFLEN, not DM1B_KEYLENGTH, bytes.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
*/
DB_STATUS
dmve_btdel(
DMVE_CB		*dmve)
{
    DM0L_BTDEL		*log_rec = (DM0L_BTDEL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->btd_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*page = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    DM_TID		leaf_bid;
    char		*insert_key;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4		page_type = log_rec->btd_pg_type;
    bool		bid_check_done = FALSE;
    bool                bid_check = FALSE;
    bool                index_update;
    DB_TRAN_ID          zero_tranid;
    i4             opflag;
    bool                allocate = TRUE;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*pinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    if (dmve->dmve_flags & DMVE_MVCC)
	log_rec->btd_bid.tid_tid.tid_page = 
	DM1B_VPT_GET_PAGE_PAGE_MACRO(log_rec->btd_pg_type, dmve->dmve_pinfo->page);

    /*
    ** Store BID of insert into a local variable.  The insert BID may
    ** be modified in undo recovery by the dmve_btunto_check routine.
    */
    leaf_bid = log_rec->btd_bid;

    /*
    ** Call appropriate recovery action depending on the recovery type
    ** and record flags.  CLR actions are always executed as an UNDO
    ** operation.
    */
    recovery_action = dmve->dmve_action;
    if (log_rec->btd_header.flags & DM0L_CLR)
	recovery_action = DMVE_UNDO;

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->btd_header.type != DM0LBTDEL)
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
	    (dmve_location_check(dmve, (i4)log_rec->btd_cnf_loc_id) == FALSE))
	{
	    uleFormat(NULL, E_DM9668_TABLE_NOT_RECOVERED, (CL_ERR_DESC *)NULL, 
		ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, 
		&loc_error, 2, 0, log_rec->btd_tbl_id.db_tab_base,
		0, log_rec->btd_tbl_id.db_tab_index);
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
	status = dmve_fix_tabio(dmve, &log_rec->btd_tbl_id, &tbio);
	if (DB_FAILURE_MACRO(status))
	    break;
	if ((status == E_DB_WARN) && (dmve->dmve_error.err_code == W_DM9660_DMVE_TABLE_OFFLINE))
	{
	    CLRDBERR(&dmve->dmve_error);
	    return (E_DB_OK);
	}

	/* This might be partial tcb, but it will always basic table info */
	t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

	/*
	** Lock/Fix the page we need to recover in cache for write.
	*/
	status = dmve_fix_page(dmve, tbio, leaf_bid.tid_tid.tid_page,&pinfo);
	if (status != E_DB_OK)
	    break;
	page = pinfo->page;

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	    dmve_trace_page_info(log_rec->btd_pg_type, log_rec->btd_page_size,
		page, dmve->dmve_plv, "Page"); 

	index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page)
				& DMPP_INDEX) != 0);

	if (dmve->dmve_action == DMVE_UNDO)
	{
	    /*
	    ** UNDO Recovery SPLIT check:
	    **
	    ** If doing UNDO recovery, we need to verify that this is the 
	    ** correct page from which to undo the put operation.
	    ** If the row in question has been moved to a different page by
	    ** a subsequent SPLIT operation, then we have to find that new page
	    ** in order to perform the undo.
	    **
	    ** The bid_check will search for the correct BID to which to do
	    ** the recovery action and return with that value in 'leafbid'.
	    */
	   if ((LSN_GT(
		DM1B_VPT_ADDR_BT_SPLIT_LSN_MACRO(page_type, page), log_lsn)) &&
	    (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_LEAF))
	    {
		bid_check = TRUE;
	    }

	    /*
	    ** UNDO: if page type != TCB_PG_V1, validate/adjust bid
	    */
	    if ( (page_type != TCB_PG_V1)
		&& (index_update == FALSE) )
	    {
		bid_check = TRUE;
	    }
	}
	else
	{
	    /*
	    ** REDO recovery, the logged page is correct, but
	    ** due to page cleans the logged bid may be incorrect.
	    ** Only do the bid check if we need to REDO this update
	    */
	    if ( (page_type != TCB_PG_V1) 
		&& (index_update == FALSE)
		&&  (LSN_LT(
		    DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), 
		    log_lsn)))
	    {
		bid_check = TRUE;
	    }
	}

	if (bid_check && !bid_check_done)
	{
	    insert_key = &log_rec->btd_vbuf[0]; 
	    if (recovery_action == DMVE_UNDO)
	    {
		/*
		** Undo leaf delete when row locking or
		** VPS physical page lock (for etabs)
		** Space should not have been reclaimed after delete
		** Undo delete needs to FIND the deleted key
		*/
		if (dmve->dmve_lk_type == LK_ROW || dmve->dmve_lk_type == LK_CROW ||
		    (page_type != TCB_PG_V1 &&
		    dmve->dmve_lk_type == LK_PAGE && 
		    (log_rec->btd_header.flags & DM0L_PHYS_LOCK)))
		{
		    opflag = DMVE_FINDKEY;
		    allocate = FALSE;
		}
		else
		{
		    opflag = DMVE_FINDPOS;
		}
	    }
	    else
		opflag = DMVE_FINDKEY;

	    status = dmve_bid_check(dmve, opflag,
		&log_rec->btd_tbl_id, &log_rec->btd_bid, 
		&log_rec->btd_tid, insert_key, log_rec->btd_key_size, 
		&tbio, &leaf_bid, &pinfo); 

	    if (status == E_DB_WARN && (dmve->dmve_flags & DMVE_MVCC))
		return (E_DB_OK);

	    if (status != E_DB_OK)
		break;

            bid_check_done = TRUE;

	    /* Reset local tcb, page pointers */
	    /* This might be partial tcb, but it will always basic table info */
	    t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));
            page = pinfo->page;

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
		DM1B_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, page), log_lsn))
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
	if (status != E_DB_OK || !page)
	    break;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_rebtree_del(dmve, tbio, pinfo, &leaf_bid);
	    break;

	case DMVE_UNDO:
	    status = dmv_unbtree_del(dmve, tbio, pinfo, &leaf_bid, allocate);
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
    if (pinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, pinfo);
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9651_DMVE_BTREE_DEL);

    return(status);
}


/*{
** Name: dmv_rebtree_del - Redo the Delete of a btree key 
**
** Description:
**      This function adds a key to a btree index for the recovery of a
**	delete record operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      page				Pointer to the page to which to insert
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
**	    Written for 6.5 recovery.
**	18-jan-1992 (rogerk)
**	    Add check in redo routine for case when null page pointer is
**	    passed because redo was found to be not needed.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          When calling dm1cxdel(), pass reclaim_space param
**      04-feb-97 (stial01)
**          Tuple headers are on LEAF and overflow (CHAIN) pages
**          Tuple headers are not on INDEX pages.
**      27-feb-97 (stial01)
**          dmv_rebtree_del() Space is reclaimed when redoing DELETE,
**          unless last entry on leaf page with overflow chain.
**          Init flag param for dm1cxdel()
**      07-apr-97 (stial01)
**          dmv_rebtree_del() NonUnique primary btree (V2) dups span leaf pages,
**          not overflow chain. Remove code skipping reclaim of overflow key
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	13-Jun-2006 (jenjo02)
**	    Clustered TIDs need not match, as long as the keys match.
*/
static DB_STATUS
dmv_rebtree_del(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo,
DM_TID		    *bid)
{
    DM0L_BTDEL		*log_rec = (DM0L_BTDEL *)dmve->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    DM_LINE_IDX		childkey;
    DM_LINE_IDX		childtid;
    DM_TID		deltid;
    i4			delpartno;
    i4			page_type = log_rec->btd_pg_type;
    char		*key;
    char		*key_ptr;
    i4		key_len;
    bool		index_update;
    bool		Clustered;
    i4		dmcx_flag;
    i4		ix_compressed;
    DMP_DCB             *dcb = dmve->dmve_dcb_ptr;
    i4			*err_code = &dmve->dmve_error.err_code;
    LG_LRI		lri;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If there is nothing to recover, just return.
    */
    if (page == NULL)
	return (E_DB_OK);

    key = &log_rec->btd_vbuf[0];

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	DMPP_INDEX) != 0);
    Clustered = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	DMPP_CLUSTERED) != 0);
    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->btd_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /*
    ** Deletes to non-leaf index pages actually effect more than one entry
    ** on the page.  The logged bid describes the entry from which the
    ** TID pointer is deleted.  The key entry is deleted from the previous
    ** position (if there is one).
    */
    if (index_update)
    {
	childtid = log_rec->btd_bid_child;
	childkey = log_rec->btd_bid_child;
    }
    else
    {
	childtid = bid->tid_tid.tid_line;
	childkey = bid->tid_tid.tid_line;
    }

    if (index_update && (childkey != 0))
	childkey--;

    /*
    ** Consistency Checks:
    **
    ** Verify that there is an entry at the indicated BID and that it
    ** matches the logged key, tid, partition entry.
    ** 
    */

    dm1cxrecptr(page_type, log_rec->btd_page_size, page, 
	childkey, &key_ptr);
    dm1cxtget(page_type, log_rec->btd_page_size, page, 
	childtid, &deltid, &delpartno);

    /*
    ** We can only validate the key size on compressed tables; otherwise
    ** we must assume that the logged value was the correct table key length.
    */
    key_len = log_rec->btd_key_size;
    if (ix_compressed != DM1CX_UNCOMPRESSED)
    {
	dm1cx_klen(page_type, log_rec->btd_page_size, page,
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
    if ((log_rec->btd_key_size != key_len) ||
	(MEcmp((PTR)key, (PTR)key_ptr, key_len) != 0) ||
	(!Clustered &&
	  (log_rec->btd_tid.tid_i4 != deltid.tid_i4 ||
	   log_rec->btd_partno != delpartno)) )
    {
	uleFormat(NULL, E_DM966A_DMVE_KEY_MISMATCH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 8, 
	    sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
	    sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
	    sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
	    0, bid->tid_tid.tid_page, 0, bid->tid_tid.tid_line,
	    5, (index_update ? "INDEX" : "LEAF "),
	    0, log_rec->btd_bid.tid_tid.tid_page,
	    0, log_rec->btd_bid.tid_tid.tid_line);
	uleFormat(NULL, E_DM966B_DMVE_KEY_MISMATCH, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 7, 
	    0, key_len, 0, log_rec->btd_key_size,
	    0, deltid.tid_tid.tid_page, 0, deltid.tid_tid.tid_line,
	    0, log_rec->btd_tid.tid_tid.tid_page,
	    0, log_rec->btd_tid.tid_tid.tid_line,
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
	    dm1cxtget(page_type, log_rec->btd_page_size, page, 
		childkey, &deltid, &delpartno);
	    status = dm1cxtput(page_type, log_rec->btd_page_size, 
		page, childtid, &deltid, delpartno);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page, 
			page_type, log_rec->btd_page_size, childtid);
		break;
	    }
	}

	/*
	** REDO recovery can usually reclaim space
	** (except REDO physical page lock)
	*/
	if (!index_update && page_type != TCB_PG_V1 &&
	    ((dcb->dcb_status & DCB_S_EXCLUSIVE) == 0) &&
	    (log_rec->btd_header.flags & DM0L_PHYS_LOCK))
	    dmcx_flag = 0;
	else
	    dmcx_flag = DM1CX_RECLAIM;

	status = dm1cxdel(page_type, log_rec->btd_page_size, page,
			DM1C_DIRECT, ix_compressed,
			&dmve->dmve_tran_id,
			LOG_ID_ID(dmve->dmve_log_id),
			(i4)0, dmcx_flag, childkey);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E2_BAD_INDEX_DEL, (DMP_RCB*)NULL, page, 
		       page_type, log_rec->btd_page_size, childkey);
	    break;
	}

	break;
    }

    /*
    ** Write the LSN, etc, of the Put log record to the updated page
    */
    DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
    DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, page, &lri);
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
** Name: dmv_unbtree_del - UNDO of a delete key operation.
**
** Description:
**      This function removes a key from a btree index for the recovery of a
**	delete record operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**      page				Pointer to page on which row was insert
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
**	    Written for 6.5 recovery.
**	18-jan-1992 (rogerk)
**	    Add check in undo routine for case when null page pointer is
**	    passed because undo was found to be not needed.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to 
**	    use macros.
**      22-nov-96 (stial01,dilma04)
**          Row Locking Project:
**          Allocate space only if space reclaimed
**      27-feb-97 (stial01)
**          dmv_unbtree_del() allocate parameter TRUE If dm1cxallocate needed
**          Log key in CLR, needed for row locking
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_unbtree_del(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *pinfo,
DM_TID		    *bid,
bool                 allocate)
{
    DM0L_BTDEL		*log_rec = (DM0L_BTDEL *)dmve->dmve_log_rec;
    DB_STATUS		status = E_DB_OK;
    DM_LINE_IDX		childkey;
    DM_LINE_IDX		childtid;
    LG_LSN		lsn;
    DM_TID		temptid;
    i4			temppartno;
    i4			page_type = log_rec->btd_pg_type;
    i4			ix_compressed;
    char		*key;
    i4		flags;
    i4		loc_id;
    i4		loc_config_id;
    bool		index_update;
    i4             update_mode;
    i4			local_err;
    i4			*err_code = &dmve->dmve_error.err_code;
    LG_LRI		lri;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If there is nothing to recover, just return.
    */
    if (page == NULL)
	return (E_DB_OK);

    key = &log_rec->btd_vbuf[0];

    index_update = ((DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	DMPP_INDEX) != 0);
    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->btd_cmp_type != TCB_C_NONE)
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
    if (index_update)
    {
	childtid = log_rec->btd_bid_child;
	childkey = log_rec->btd_bid_child;
    }
    else
    {
	childtid = bid->tid_tid.tid_line;
	childkey = bid->tid_tid.tid_line;
    }

    /* Index pages do not contain partition numbers */
    temppartno = 0;

    if (index_update && (childkey != 0))
    {
	childkey--;
        dm1cxtget(page_type, log_rec->btd_page_size, page, 
		childkey, &temptid, &temppartno); 
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
    ** As of release OpenIngres 2.0, we need the key value in CLRs as well,
    ** because of row locking.
    */
    if ((log_rec->btd_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    flags = (log_rec->btd_header.flags | DM0L_CLR);

	    /* Extract previous page change info */
	    DM1B_VPT_GET_PAGE_LRI_MACRO(log_rec->btd_pg_type, page, &lri);

	    status = dm0l_btdel(dmve->dmve_log_id, flags, 
		&log_rec->btd_tbl_id, 
		tabio->tbio_relid, 0,
		tabio->tbio_relowner, 0,
		log_rec->btd_pg_type, log_rec->btd_page_size,
		log_rec->btd_cmp_type, 
		log_rec->btd_loc_cnt, loc_config_id,
		bid, childkey, &log_rec->btd_tid, log_rec->btd_key_size, key, 
		&log_rec->btd_header.lsn, &lri, log_rec->btd_partno, 
		log_rec->btd_btflags, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		dmveUnMutex(dmve, pinfo);
		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;

		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, &local_err, 0); 

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
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
    }

    /* 
    ** Write the LSN, etc, of the delete record onto the page, unless nologging
    */
    if (dmve->dmve_logging)
	DM1B_VPT_SET_PAGE_LRI_MACRO(page_type, page, &lri);

    update_mode = DM1C_DIRECT;
    if ((dmve->dmve_lk_type == LK_ROW || dmve->dmve_lk_type == LK_CROW) ||
	(!index_update && log_rec->btd_pg_type != TCB_PG_V1 &&
	dmve->dmve_lk_type == LK_PAGE && (log_rec->btd_header.flags & DM0L_PHYS_LOCK)))
	update_mode |= DM1C_ROWLK;

    /*
    ** Undo the delete operation.
    */
    for (;;)
    {
	/*
	** Allocate space if necessary
	*/
	if ( allocate == TRUE )
	{
	    status = dm1cxallocate(page_type, log_rec->btd_page_size,
			    page, update_mode, ix_compressed,
			    &dmve->dmve_tran_id, (i4)0, childkey,
			    log_rec->btd_key_size + 
				DM1B_VPT_GET_BT_TIDSZ_MACRO(
					page_type, page));
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93E0_BAD_INDEX_ALLOC, (DMP_RCB *)NULL, page,
			page_type, log_rec->btd_page_size, childkey);
		break;
	    }
	}

	/*
	** Reinsert the key,tid,partition values.
	*/
    /* If leaf overflow look for entry with matching tid */
    /* skip key comparison all the keys on overflow are the same */
    if (DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, page) & DMPP_CHAIN)
    {
	i4 i;
	DM_TID	tmptid;
	i4	tmppart;
	LG_LSN lsn;
	for (i = 0; i < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page); i++)
	{
	    dm1cxtget(page_type, log_rec->btd_page_size, page, i, &tmptid, &tmppart); 
	lsn = DMPP_VPT_GET_PAGE_LSN_MACRO(page_type, page);
	    if (log_rec->btd_tid.tid_i4 == tmptid.tid_i4 &&
		dmpp_vpt_test_free_macro(page_type,
		DM1B_VPT_BT_SEQUENCE_MACRO(page_type, page), 
		(i4)i + DM1B_OFFSEQ) == FALSE)
	    TRdisplay("dmvebtdl: dup entry on overflow %d for tid %d,%d CRPAGE %d page lsn %x\n",
		DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
		log_rec->btd_tid.tid_tid.tid_page,
		log_rec->btd_tid.tid_tid.tid_line,
	        DMPP_VPT_IS_CR_PAGE(page_type, page), lsn.lsn_low);
/*
does this trigger the dm1bxreserve failure from dm1bxovfl_alloc 
	    return (E_DB_ERROR);
*/
	}
    }
	status = dm1cxput(page_type, log_rec->btd_page_size, page,
			ix_compressed, update_mode, 
			&dmve->dmve_tran_id,
			LOG_ID_ID(dmve->dmve_log_id),
			(i4)0, childkey, 
			key, log_rec->btd_key_size, &log_rec->btd_tid,
			log_rec->btd_partno);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E4_BAD_INDEX_PUT, (DMP_RCB *)NULL, page,
			page_type, log_rec->btd_page_size, childkey);
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
	    status = dm1cxtput(page_type, log_rec->btd_page_size, 
				page, childtid, &log_rec->btd_tid,
				log_rec->btd_partno);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page,
			page_type, log_rec->btd_page_size, childtid);
		break;
	    }
	    status = dm1cxtput(page_type, log_rec->btd_page_size, 
			page, childkey, &temptid, temppartno);
	    if (status != E_DB_OK)
	    {
		dm1cxlog_error(E_DM93EB_BAD_INDEX_TPUT, (DMP_RCB *)NULL, page,
			page_type, log_rec->btd_page_size, childkey);
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
