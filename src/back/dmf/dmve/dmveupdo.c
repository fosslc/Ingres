/*
** Copyright (c) 1992, 2008 Ingres Corporation
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
#include    <dm1p.h>
#include    <dm1cx.h>
#include    <dm1c.h>
#include    <dmve.h>
#include    <dm0p.h>
#include    <dmpp.h>
#include    <dm0l.h>
#include    <dmtcb.h>
#include    <dm2t.h>
#include    <dmftrace.h>

/**
**
**  Name: DMVEUPDO.C - The recovery of a Btree Update Overflow operation.
**
**  Description:
**	This file contains the routines for the recovery of the overflow
**	chain updates performed during a split of a leaf with overflow pages.
**
**          dmve_btupdovfl - The recovery of an Update Overflow operation.
**          dmve_rebtupdovfl - Redo Recovery of an Update Overflow operation.
**          dmve_unbtupdovfl - Undo Recovery of an Update Overflow operation.
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
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**      06-mar-1996 (stial01) 
**          Pass btu_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
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
**	19-Jan-2004 (jenjo02)
**	    Added partition number parm to dm1cxreplace. RANGE entries
**	    are oblivious to partition number, so the parm is always
**	    "(i4)0" in here.
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
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*
** Forward Declarations.
*/

static DB_STATUS dmv_rebtupdovfl(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *ovflpinfo);

static DB_STATUS dmv_unbtupdovfl(
			DMVE_CB             *dmve,
			DMP_TABLE_IO	    *tabio,
			DMP_PINFO	    *ovflpinfo);


/*{
** Name: dmve_btupdovfl - The recovery of a Btree Split operation.
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
**	06-may-1996 (thaju02 & nanpr01)
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
dmve_btupdovfl(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_BTUPDOVFL	*log_rec = (DM0L_BTUPDOVFL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->btu_header.lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*ovfl = NULL;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    i4		recovery_action;
    i4		error;
    i4		loc_error;
    i4		page_type = log_rec->btu_pg_type;
    bool		ovfl_page_recover;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*ovflpinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    for (;;)
    {
	/*
	** Consistency Check:  check for illegal log records.
	*/
	if ((log_rec->btu_header.type != DM0LBTUPDOVFL) ||
	    (log_rec->btu_header.length != 
                (sizeof(DM0L_BTUPDOVFL) -
                        (DM1B_KEYLENGTH - log_rec->btu_lrange_len) - 
                        (DM1B_KEYLENGTH - log_rec->btu_olrange_len) - 
                        (DM1B_KEYLENGTH - log_rec->btu_rrange_len) - 
                        (DM1B_KEYLENGTH - log_rec->btu_orrange_len))))
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
	status = dmve_fix_tabio(dmve, &log_rec->btu_tbl_id, &tbio);
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
	ovfl_page_recover = dmve_location_check(dmve, 
					(i4)log_rec->btu_cnf_loc_id);

	/*
	** Fix the pages we need to recover in cache for write.
	*/
	if (ovfl_page_recover)
	{
	    status = dmve_fix_page(dmve, tbio, log_rec->btu_pageno, &ovflpinfo);
	    if (status != E_DB_OK)
		break;
	    ovfl = ovflpinfo->page;
	}

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	    dmve_trace_page_info(log_rec->btu_pg_type, log_rec->btu_page_size,
		ovfl, dmve->dmve_plv, "OVFL"); 

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
			sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
			sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
			0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, ovfl),
			0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, ovfl),
			0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, ovfl),
			0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, ovfl),
			0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
		}
		ovfl = NULL;
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
		    sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name,
		    sizeof(DB_OWN_NAME), tbio->tbio_relowner->db_own_name,
		    0, DM1B_VPT_GET_PAGE_PAGE_MACRO(page_type, ovfl),
		    0, DM1B_VPT_GET_PAGE_STAT_MACRO(page_type, ovfl),
		    0, DM1B_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, ovfl),
		    0, DM1B_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, ovfl),
		    0, log_lsn->lsn_high, 0, log_lsn->lsn_low);
	        SETDBERR(&dmve->dmve_error, 0, E_DM9666_PAGE_LSN_MISMATCH);
		status = E_DB_ERROR;
	    }
	    break;
	}
	if (status != E_DB_OK || !ovfl)
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->btu_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	switch (recovery_action)
	{
	case DMVE_DO:
	case DMVE_REDO:
	    status = dmv_rebtupdovfl(dmve, tbio, ovflpinfo);
	    break;

	case DMVE_UNDO:
	    status = dmv_unbtupdovfl(dmve, tbio, ovflpinfo);
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9630_DMVE_UPDATE_OVFL);

    return(status);
}

/*{
** Name: dmv_rebtupdovfl - Redo the Btree Update Overflow operation.
**
** Description:
**      This function redoes the update to a btree overflow page affected
**	by a Split Index Page Operation.
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	log_rec				Alloc log record.
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
**	14-dec-1992 (jnash & rogerk)
**	    Written for 6.5 recovery project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_rebtupdovfl(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *ovflpinfo)
{
    DM0L_BTUPDOVFL	*log_rec = (DM0L_BTUPDOVFL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->btu_header.lsn; 
    char		*lrange_ptr;
    char		*rrange_ptr;
    DB_STATUS		status = E_DB_OK;
    i4			page_type = log_rec->btu_pg_type;
    i4			ix_compressed;
    i4			*err_code = &dmve->dmve_error.err_code;
    DMPP_PAGE		*ovfl = ovflpinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If recovery was found to be unneeded then we can just return.
    */
    if (ovfl == NULL)
	return (E_DB_OK);

    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->btu_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /*
    ** Get range key pointers from variable length section of the log record.
    */
    lrange_ptr = &log_rec->btu_vbuf[0];
    rrange_ptr = &log_rec->btu_vbuf[log_rec->btu_lrange_len];

    /*
    ** Mutex the overflow page while updating it.
    */
    dmveMutex(dmve, ovflpinfo);

    /*
    ** Redo the Split operation.
    ** Update the page's mainpage and nextpage pointers and its range keys.
    */
    for (;;)
    {
	DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, ovfl, log_rec->btu_mainpage);
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(page_type, ovfl, log_rec->btu_nextpage);

	status = dm1cxreplace(page_type, log_rec->btu_page_size,
	    ovfl, DM1C_DIRECT, ix_compressed,
	    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, DM1B_LRANGE,
	    lrange_ptr, log_rec->btu_lrange_len, 
	    &log_rec->btu_lrtid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
		ovfl, page_type, log_rec->btu_page_size, 
		DM1B_LRANGE);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9632_REDO_UPDATE_OVFL);
	    break;
	}

	status = dm1cxreplace(page_type, log_rec->btu_page_size,
	    ovfl, DM1C_DIRECT, ix_compressed,
	    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, DM1B_RRANGE,
	    rrange_ptr, log_rec->btu_rrange_len, 
	    &log_rec->btu_rrtid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
		ovfl, page_type, log_rec->btu_page_size,
		DM1B_RRANGE);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9632_REDO_UPDATE_OVFL);
	    break;
	}

	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, ovfl, DMPP_MODIFY);
	DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, ovfl, *log_lsn);
	break;
    }

    dmveUnMutex(dmve, ovflpinfo);
    
    return(status);
}

/*{
** Name: dmv_unbtupdovfl - Undo the Btree Update Overflow operation.
**
** Description:
**      This function undoes the update to a btree overflow page affected
**	by a Split Index Page Operation.
**
** Description:
**
** Inputs:
**      dmve				Pointer to dmve control block.
**      tabio				Pointer to table io control block
**	fhdr				Table's FHDR page.
**	fmap				Table's FMAP page.
**	log_rec				Alloc log record.
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
**	14-dec-1992 (jnash & rogerk)
**	    Written for 6.5 recovery project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**	    Also, fix bug where *log_lsn, not lsn, was inserted into 
**	    page_log_addr.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_unbtupdovfl(
DMVE_CB             *dmve,
DMP_TABLE_IO	    *tabio,
DMP_PINFO	    *ovflpinfo)
{
    DM0L_BTUPDOVFL	*log_rec = (DM0L_BTUPDOVFL *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->btu_header.lsn; 
    LG_LSN		lsn; 
    i4		dm0l_flags;
    i4		page_type = log_rec->btu_pg_type;
    i4		ix_compressed;
    char		*lrange_ptr;
    char		*rrange_ptr;
    char		*olrange_ptr;
    char		*orrange_ptr;
    DB_STATUS		status = E_DB_OK;
    i4			*err_code = &dmve->dmve_error.err_code;
    DMPP_PAGE		*ovfl = ovflpinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** If recovery was found to be unneeded then we can just return.
    */
    if (ovfl == NULL)
	return (E_DB_OK);

    ix_compressed = DM1CX_UNCOMPRESSED;
    if (log_rec->btu_cmp_type != TCB_C_NONE)
	ix_compressed = DM1CX_COMPRESSED;

    /*
    ** Get old range key pointers from variable length section of the 
    ** log record
    */
    lrange_ptr = &log_rec->btu_vbuf[0];
    rrange_ptr = &log_rec->btu_vbuf[log_rec->btu_lrange_len];
    olrange_ptr = &log_rec->btu_vbuf[log_rec->btu_lrange_len + 
				     log_rec->btu_rrange_len];
    orrange_ptr = &log_rec->btu_vbuf[log_rec->btu_lrange_len + 
				     log_rec->btu_rrange_len +
				     log_rec->btu_olrange_len];

    /*
    ** Mutex the overflow page while updating it.
    */
    dmveMutex(dmve, ovflpinfo);

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
    if ((log_rec->btu_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    dm0l_flags = (log_rec->btu_header.flags | DM0L_CLR);

	    status = dm0l_btupdovfl(dmve->dmve_log_id, dm0l_flags, 
		&log_rec->btu_tbl_id, tabio->tbio_relid, 
		tabio->tbio_relowner,
		log_rec->btu_pg_type, log_rec->btu_page_size,
		log_rec->btu_cmp_type, 
		log_rec->btu_loc_cnt, 
		log_rec->btu_cnf_loc_id, log_rec->btu_pageno,
		log_rec->btu_mainpage, log_rec->btu_omainpage,
		log_rec->btu_nextpage, log_rec->btu_onextpage,
		log_rec->btu_lrange_len, lrange_ptr, &log_rec->btu_lrtid,
		log_rec->btu_olrange_len, olrange_ptr, &log_rec->btu_olrtid,
		log_rec->btu_rrange_len, rrange_ptr, &log_rec->btu_rrtid,
		log_rec->btu_orrange_len, orrange_ptr, &log_rec->btu_orrtid,
		log_lsn, &lsn, &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		dmveUnMutex(dmve, ovflpinfo);

		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		    (char *)NULL, (i4)0, (i4 *)NULL, err_code, 0); 
		SETDBERR(&dmve->dmve_error, 0, E_DM9631_UNDO_UPDATE_OVFL);
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
	lsn = *log_lsn;
    }

    /*
    ** Undo the Split operation.
    ** Reset the page's mainpage and nextpage pointers and its range keys.
    */
    for (;;)
    {
	DM1B_VPT_SET_PAGE_MAIN_MACRO(page_type, ovfl, 
	    log_rec->btu_omainpage);
	DM1B_VPT_SET_BT_NEXTPAGE_MACRO(page_type, ovfl, 
	    log_rec->btu_onextpage);

	status = dm1cxreplace(page_type, log_rec->btu_page_size,
	    ovfl, DM1C_DIRECT, ix_compressed,
	    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, DM1B_LRANGE,
	    lrange_ptr, log_rec->btu_olrange_len, 
	    &log_rec->btu_olrtid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
		ovfl, page_type, log_rec->btu_page_size, 
		DM1B_LRANGE);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9631_UNDO_UPDATE_OVFL);
	    break;
	}

	status = dm1cxreplace(page_type, log_rec->btu_page_size,
	    ovfl, DM1C_DIRECT, ix_compressed,
	    (DB_TRAN_ID *)NULL, (u_i2)0, (i4)0, DM1B_RRANGE,
	    rrange_ptr, log_rec->btu_orrange_len, 
	    &log_rec->btu_orrtid, (i4)0);
	if (status != E_DB_OK)
	{
	    dm1cxlog_error(E_DM93E6_BAD_INDEX_REPLACE, (DMP_RCB *)NULL, 
		ovfl, page_type, log_rec->btu_page_size, 
		DM1B_RRANGE);
	    SETDBERR(&dmve->dmve_error, 0, E_DM9631_UNDO_UPDATE_OVFL);
	    break;
	}

	DM1B_VPT_SET_PAGE_STAT_MACRO(page_type, ovfl, DMPP_MODIFY);
	if (dmve->dmve_logging)
	    DM1B_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, ovfl, lsn);
	break;
    }

    dmveUnMutex(dmve, ovflpinfo);
    
    return(status);
}
