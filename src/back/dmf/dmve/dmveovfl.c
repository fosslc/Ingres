/*
** Copyright (c) 1991, 2008 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <lg.h>
#include    <lk.h>
#include    <adf.h>
#include    <ulf.h>
#include    <dmf.h>
#include    <dm.h>
#include    <dmp.h>
#include    <dmve.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm1c.h>
#include    <dm0p.h>
#include    <dm0l.h>
#include    <dmd.h>
#include    <dmftrace.h>

/*
**
**  Name: DMVEOVFL.C - The recovery of an overflow page allocation.
**
**  Description:
**	This file contains the dmve_ovfl routines, used to UNDO and REDO
**	linking a new data page onto an overflow chain.
**
**          dmve_ovfl   - Recover linking of data page to overflow chain.
**          dmve_reovfl - Perform REDO recovery.
**          dmve_unovfl - Perform UNDO recovery.
**
**
**  History:
**	 3-nov-91 (Rogerk)
**	    Created to fix File Extend Redo problems.
**	 5-nov-91 (rogerk)
**	    Fix typo bug : change page_ovfl to page_main in recovery of the
**	    new overflow page.
**	7-july-92 (jnash)
**	    Add DMF function prototyping.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed arguments to dm0p_mutex and
**	    dm0p_unmutex call(s)s.
**	20-oct-1992 (jnash)
**	    Reduced Logging Project: First cut at new recovery logic.
**	    We now perform both UNDO and REDO recovery, with UNDO
**	    putting the old root page in order and REDO redo'ing
**	    updates to both the root and overflow pages.
**	16-feb-1993 (rogerk)
**	    Removed xDEBUG code in undo that AV'd if the page pointer was NULL.
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
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      23-may-1994 (jnash)
**          REDO/ROLLDB of add overflow did not set overflow status bit.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      06-mar-1996 (stial01)
**          Pass ovf_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to macros.
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
**	04-May-2000 (jenjo02)
**	    Pass blank-suppressed lengths of table/owner names to 
**	    dm0l_put, eliminating need for dmve_name_unpack().
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
**      01-apr-2010 (stial01)
**          Changes for Long IDs, move consistency check to dmveutil
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*
**  Definition of static variables and forward static functions.
*/

static DB_STATUS dmv_unovfl(
		DMVE_CB             *dmve,
		DMP_TABLE_IO        *tabio,
		DMP_PINFO           *datapinfo);

static DB_STATUS dmv_reovfl(
 		DMVE_CB             *dmve,
 		DMP_TABLE_IO        *tabio,
		DMP_PINFO	    *datapinfo,
 		DMP_PINFO           *ovflpinfo);



/*{
** Name: dmve_ovfl - Recover linking of data page to overflow chain.
**
** Description:
**
**	This routine is used for recovery of the operation which links
**	a new data page into an overflow chain.  It is logged during
**	new page allocations to HASH, ISAM and HEAP tables.
**
**	This routine is called during UNDO and REDO recovery.  
**
**	During UNDO, the original page is restored to its prior
**	condition, and the new page is left dangling.  It is assumed that
**	the page will be marked free in the FMAP by dmve_alloc code.
**
**	During REDO, we need to link the new page back into the overflow 
**	chain by setting:
**
**	  - the original page to point to the newly allocated data page
**	  - the new data page to point to the next overflow page on the chain
**	  - the new data page's page_main to point to the original's page_main
**
**	Again we assume FMAP maintenance is handled by dmve_alloc logic.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The ovfl log record.
**	    .dmve_action	    Should be DMVE_REDO.
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
**	 3-nov-91 (Rogerk)
**	    Created to fix File Extend Redo problems.
**	 5-nov-91 (rogerk)
**	    Fix typo bug : change page_ovfl to page_main in recovery of the
**	    new overflow page.
**	20-oct-1992 (jnash)
**	    Reduced Logging Project: Complete rewrite for new recovery.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to macros.
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
**      23-feb-2004 (thaju02) Bug 111470 INGSRV2635
**          For rollforwarddb -b option, do not compare the LSN's on the
**          page to that of the log record.
**	01-Dec-2004 (jenjo02)
**	    Added DM0P_TABLE_LOCKED_X flag for bug 108074 completeness.
**	01-Dec-2006 (kiria01) b117225
**	    Initialise the lockid parameters that will be passed to LKrequest
**	    to avoid random implicit lock conversions.
*/
DB_STATUS
dmve_ovfl(
DMVE_CB		*dmve)
{
    DM0L_OVFL		*log_rec = (DM0L_OVFL *)dmve->dmve_log_rec;
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    LG_LSN		*log_lsn = &log_rec->ovf_header.lsn; 
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*data = NULL;       /* root */
    DMPP_PAGE		*ovfl = NULL;    /* overflow */
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    i4		error;
    i4		loc_error;
    i4		recovery_action;
    i4		page_type = log_rec->ovf_pg_type;
    bool		page_recover;
    bool		ovfl_recover;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*datapinfo = NULL;
    DMP_PINFO		*ovflpinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    for (;;)
    {
	/* Consistency Check:  check for illegal log records */
	if (log_rec->ovf_header.type != DM0LOVFL)
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
	status = dmve_fix_tabio(dmve, &log_rec->ovf_tbl_id, &tbio);
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
	page_recover = dmve_location_check(dmve,
	    (i4)log_rec->ovf_cnf_loc_id);
	ovfl_recover = dmve_location_check(dmve,
	    (i4)log_rec->ovf_ovfl_cnf_loc_id);

	if ( !page_recover && !ovfl_recover )
	    break;
	/*
	** Lock/Fix the page we need to recover in cache for write.
	*/
	if (page_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->ovf_page, &datapinfo);
	    if (status != E_DB_OK)
		break;
	    data = datapinfo->page;
	}

	if (ovfl_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->ovf_newpage, &ovflpinfo);
	    if (status != E_DB_OK)
		break;
	    ovfl = ovflpinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	{
	    dmve_trace_page_info(log_rec->ovf_pg_type, log_rec->ovf_page_size,
		data, dmve->dmve_plv, "DATA");
	    dmve_trace_page_info(log_rec->ovf_pg_type, log_rec->ovf_page_size,
		ovfl, dmve->dmve_plv, "OVFL");
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
	    if (data && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, data), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, data),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, data),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, data),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, data),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		data = NULL;
	    }
 
	    if (ovfl && LSN_GTE(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, ovfl), 
		log_lsn) && ((dmve->dmve_flags & DMVE_ROLLDB_BOPT) == 0))
	    {
		if (dmve->dmve_action == DMVE_DO)
		{
		    uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
			ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
			&loc_error, 8,
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, ovfl),
			0, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, ovfl),
			0, DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, ovfl),
			0, DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, ovfl),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		ovfl = NULL;
	    }
	    break;

	case DMVE_UNDO:
	    if (data && LSN_LT(
		DMPP_VPT_ADDR_PAGE_LOG_ADDR_MACRO(page_type, data), 
		log_lsn))
	    {
		uleFormat(NULL, E_DM9665_PAGE_OUT_OF_DATE, (CL_ERR_DESC *)NULL,
		    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL,
		    &loc_error, 8,
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
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
	if (status != E_DB_OK || (!data && !ovfl) )
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->ovf_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	/*
	** Do the recovery.
	*/
        switch (recovery_action)
        {
        case DMVE_DO:
        case DMVE_REDO:
            status = dmv_reovfl(dmve, tbio, 
	    		(data) ? datapinfo : NULL,
	    		(ovfl) ? ovflpinfo : NULL);
            break;

        case DMVE_UNDO:
	    if ( data )
		status = dmv_unovfl(dmve, tbio, datapinfo);
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
    ** Unfix/Unlock the updated page.  No need to force it to disk - it
    ** will be tossed out through normal cache protocols if Fast
    ** Commit or at the end of the abort if non Fast Commit.
    */
    if (datapinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, datapinfo);
	if (tmp_status > status)
	    status = tmp_status;
    }

    if (ovflpinfo)
    {
	tmp_status = dmve_unfix_page(dmve, tbio, ovflpinfo);
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
	SETDBERR(&dmve->dmve_error, 0, E_DM962A_DMVE_OVFL);

    return(status);
}


/*{
** Name: dmv_unovfl - Undo add overflow record operation.
**
** Description:
**      This function performs UNDO recovery on the adding of an
**	overflow record to a hash, heap or isam table.
**
**	We don't touch the overflow page in this routine, we just
**	update the root page overflow pointer, the FULLCHAIN
**	bit (if necessary) and the page lsn.
**
** Inputs:
**      dmve 				Pointer to dmve control block.
**      tabio                           Pointer to table io control block
**      page 				Pointer to the page 
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
**	16-feb-1993 (rogerk)
**	    Removed xDEBUG code that AV'd if the page pointer was NULL.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_unovfl(
DMVE_CB             *dmve,
DMP_TABLE_IO        *tabio,
DMP_PINFO           *datapinfo)
{
    DM0L_OVFL		*log_rec = (DM0L_OVFL *)dmve->dmve_log_rec;
    i4		dm0l_flags;
    i4		page_type = log_rec->ovf_pg_type;
    DB_STATUS		status;
    i4			*err_code = &dmve->dmve_error.err_code;
    LG_LRI		lri;
    DMPP_PAGE		*page = datapinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** Check for partial recovery
    */
    if (page == NULL)
	return(E_DB_OK);

    /*
    ** Consistency Check:
    ** Verify that the parent page's page_main field is the same as that
    ** of the overflow page.
    */
    if (DMPP_VPT_GET_PAGE_MAIN_MACRO(page_type, page) != 
	log_rec->ovf_main_ptr) 
    {
	uleFormat(NULL, E_DM9678_DMVE_OVFL_STATE, (CL_ERR_DESC *)NULL, ULE_LOG,
	    NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 6,
	    sizeof(DB_DB_NAME), tabio->tbio_dbname->db_db_name,
	    sizeof(DB_TAB_NAME), tabio->tbio_relid->db_tab_name,
	    sizeof(DB_OWN_NAME), tabio->tbio_relowner->db_own_name,
	    0, DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, page),
	    0, DMPP_VPT_GET_PAGE_MAIN_MACRO(page_type, page),
	    0, log_rec->ovf_main_ptr);
	dmd_log(1, (PTR) log_rec, 4096);
	uleFormat(NULL, E_DM963C_UNDO_OVFL, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
	    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    }

    /* 
    ** Mutex the page.  This must be done prior to the log write.
    */
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
    if ((log_rec->ovf_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    dm0l_flags = (log_rec->ovf_header.flags | DM0L_CLR);

	    /* Extract previous page change info */
	    DMPP_VPT_GET_PAGE_LRI_MACRO(log_rec->ovf_pg_type, page, &lri);

	    status = dm0l_ovfl(dmve->dmve_log_id, dm0l_flags, TRUE, 
		&log_rec->ovf_tbl_id, 
		tabio->tbio_relid, 0,
		tabio->tbio_relowner, 0,
		log_rec->ovf_pg_type, log_rec->ovf_page_size,
		log_rec->ovf_loc_cnt,
		log_rec->ovf_cnf_loc_id, log_rec->ovf_ovfl_cnf_loc_id,
		log_rec->ovf_newpage, log_rec->ovf_page, log_rec->ovf_ovfl_ptr,
		log_rec->ovf_main_ptr, &log_rec->ovf_header.lsn,
		&lri, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		dmveUnMutex(dmve, datapinfo);
		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
		SETDBERR(&dmve->dmve_error, 0, E_DM963C_UNDO_OVFL);
		return(E_DB_ERROR);        
	    }
	}
    }
    else
    {
	/*
	** If we are redoing the undo of an overflow add,
	** don't log a CLR but instead save the LSN of the log record we 
	** are processing with which to update the page lsn's.
	*/
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
    }

    /* 
    ** Write the LSN of the overflow record onto the page, unless nologging.
    */
    if (dmve->dmve_logging)
	DMPP_VPT_SET_PAGE_LRI_MACRO(page_type, page, &lri);

    /*
    ** Undo the change, including the undo of the the change to the 
    ** FULLCHAIN bit, if appropriate.
    */

    DMPP_VPT_SET_PAGE_OVFL_MACRO(page_type, page, log_rec->ovf_ovfl_ptr);
    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);

    if (log_rec->ovf_fullc)
	DMPP_VPT_CLR_PAGE_STAT_MACRO(page_type, page, DMPP_FULLCHAIN);

    dmveUnMutex(dmve, datapinfo);

    return(E_DB_OK);
}

/*{
** Name: dmv_reovfl - REDO adding an overflow record.
**
** Description:
**      This function performs REDO recovery of the adding of an
**	overflow record to a hash, heap or isam table.  The REDO
**	may involve updates to one or both of the related pages.
**
** Inputs:
**      dmve 				Pointer to dmve control block.
**      tabio                           Pointer to table io control block
**      page 				Pointer to the page
**      newpage			Pointer to the new page
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
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**      23-may-1994 (jnash)
**          REDO/ROLLDB of add overflow did not set overflow status bit.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_reovfl(
DMVE_CB             *dmve,
DMP_TABLE_IO        *tabio,
DMP_PINFO	    *datapinfo,
DMP_PINFO	    *ovflpinfo)
{
    LG_LRI		lri;
    DM0L_OVFL		*log_rec = (DM0L_OVFL *)dmve->dmve_log_rec;
    DB_STATUS		status;
    i4			page_type = log_rec->ovf_pg_type;
    DMPP_PAGE		*page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** There are two page updates that may need to be done, one
    ** may be done without the other, or neither may be necessary.
    */
    if ((datapinfo == NULL) && (ovflpinfo == NULL))
	return(E_DB_OK);

    if (datapinfo) 
    {
	dmveMutex(dmve, datapinfo);

	page = datapinfo->page;

	/*
	** Redo the change, update the page LRI.  We can unconditionally
	** set FULLCHAIN here. 
	*/
	DM0L_MAKE_LRI_FROM_LOG_RECORD(&lri, log_rec);
	DMPP_VPT_SET_PAGE_OVFL_MACRO(page_type, page, log_rec->ovf_newpage);
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_MODIFY);
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, page, DMPP_FULLCHAIN);
	DMPP_VPT_SET_PAGE_LRI_MACRO(page_type, page, &lri);

	dmveUnMutex(dmve, datapinfo);
    }

    if (ovflpinfo)
    {
	/*
	**
	*/
	dmveMutex(dmve, ovflpinfo);

	page = ovflpinfo->page;

	/*
	** Redo the change an update the page lsn only.
	*/
	DMPP_VPT_SET_PAGE_OVFL_MACRO(page_type, page, log_rec->ovf_ovfl_ptr);
	DMPP_VPT_SET_PAGE_MAIN_MACRO(page_type, page, log_rec->ovf_main_ptr);
	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, page, (DMPP_MODIFY | DMPP_OVFL));
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, page, log_rec->ovf_header.lsn);

	dmveUnMutex(dmve, ovflpinfo);
    }

    return(E_DB_OK);
}
