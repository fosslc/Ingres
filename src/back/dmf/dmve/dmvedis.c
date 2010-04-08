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
#include    <dmftrace.h>

/*
**
**  Name: DMVEDISASSOC.C - The recovery of a btree data page dissociate action.
**
**  Description:
**	This file contains the routines to recover a btree data page
**	disassociate action.
**
**          dmve_disassoc   - Recover disassociation of btree data page.
**          dmve_redisassoc - Perform REDO recovery.
**          dmve_undisassoc - Perform UNDO recovery.
**
**
**  History:
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
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
**	    Add missing break statement in page LSN checking case statement.
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
*      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**     06-mar-1996 (stial01)
**          Pass dis_page_size to dmve_trace_page_info
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      22-nov-1996 (dilma04)
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
**      14-oct-2009 (stial01)
**          Call dmve_fix_page, dmve_unfix_page
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**/

/*
**  Definition of static variables and forward static functions.
*/

static DB_STATUS dmv_undisassoc(
		DMVE_CB             *dmve,
		DMP_TABLE_IO        *tabio,
		DMP_PINFO           *pinfo);

static DB_STATUS dmv_redisassoc(
 		DMVE_CB             *dmve,
 		DMP_TABLE_IO        *tabio,
		DMP_PINFO           *pinfo);



/*{
** Name: dmve_disassoc - Recover disassociation of btree data page.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The disassoc log record.
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
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**		Correct the message parameter lengths for E_DM9665.
**	21-jun-1993 (rogerk)
**	    Add missing break statement in page LSN checking case statement.
**	    In rollforward we were mistakenly falling through to the undo
**	    case and failing the undo LSN comparison.
**	26-jul-1993 (bryanp)
**	    Replace all uses of DM_LKID with LK_LKID.
**	06-may-1996 (thaju02)
**	    New Page Format Support: 
**		Change page header references to use macros.
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
dmve_disassoc(
DMVE_CB		*dmve)
{
    DM0L_DISASSOC	*log_rec = (DM0L_DISASSOC *)dmve->dmve_log_rec;
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    LG_LSN		*log_lsn = &log_rec->dis_header.lsn; 
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE		*data;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    i4		error;
    i4		loc_error;
    i4		recovery_action;
    i4		page_type = log_rec->dis_pg_type;
    bool		page_recover;
    DB_ERROR		local_dberr;
    DMP_TCB		*t = NULL;
    DMP_PINFO		*pinfo = NULL;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	/*
	** Consistency Check:  check for illegal log records.
	*/
	if ((log_rec->dis_header.type != DM0LDISASSOC) ||
	    (log_rec->dis_header.length != sizeof(DM0L_DISASSOC)))
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
	status = dmve_fix_tabio(dmve, &log_rec->dis_tbl_id, &tbio);
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
	if ( !dmve_location_check(dmve, (i4)log_rec->dis_cnf_loc_id) )
	    break;

	/*
	** Lock/Fix the page we need to recover in cache for write.
	*/
	status = dmve_fix_page(dmve, tbio, log_rec->dis_pageno, &pinfo);
	if (status != E_DB_OK)
	    break;

	data = pinfo->page;

	/*
	** Dump debug trace info about pages if such tracing is configured.
	*/
	if (DMZ_ASY_MACRO(15))
	    dmve_trace_page_info(log_rec->dis_pg_type, log_rec->dis_page_size,
		data, dmve->dmve_plv, "DATA");

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
	if ( status != E_DB_OK || !data )
	    break;

	/*
	** Call appropriate recovery action depending on the recovery type
	** and record flags.  CLR actions are always executed as an UNDO
	** operation.
	*/
	recovery_action = dmve->dmve_action;
	if (log_rec->dis_header.flags & DM0L_CLR)
	    recovery_action = DMVE_UNDO;

	/*
	** Do the recovery.
	*/
        switch (recovery_action)
        {
        case DMVE_DO:
        case DMVE_REDO:
            status = dmv_redisassoc(dmve, tbio, pinfo);
            break;

        case DMVE_UNDO:
	    status = dmv_undisassoc(dmve, tbio, pinfo);
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
    if ( pinfo )
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
	SETDBERR(&dmve->dmve_error, 0, E_DM9647_DMVE_DISASSOC);

    return(status);
}


/*{
** Name: dmv_undisassoc - Undo disassociate record operation.
**
** Description:
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
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**	15-mar-1993 (jnash)
**	    Check dmve->dmve_logging to determine if logging required.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_undisassoc(
DMVE_CB             *dmve,
DMP_TABLE_IO        *tabio,
DMP_PINFO           *pinfo)
{
    DM0L_DISASSOC	*log_rec = (DM0L_DISASSOC *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->dis_header.lsn; 
    LG_LSN		lsn;
    i4		dm0l_flags;
    i4		page_type = log_rec->dis_pg_type;
    DB_STATUS		status;
    i4			*err_code = &dmve->dmve_error.err_code;
    DMPP_PAGE		*page = pinfo->page;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** Check for partial recovery
    */
    if (page == NULL)
	return(E_DB_OK);

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
    */
    if ((log_rec->dis_header.flags & DM0L_CLR) == 0)
    {
	if (dmve->dmve_logging)
	{
	    dm0l_flags = (log_rec->dis_header.flags | DM0L_CLR);

	    status = dm0l_disassoc(dmve->dmve_log_id, dm0l_flags,
		&log_rec->dis_tbl_id, &log_rec->dis_tblname, 
		&log_rec->dis_tblowner,
		log_rec->dis_pg_type, log_rec->dis_page_size,
		log_rec->dis_loc_cnt,
		log_rec->dis_cnf_loc_id, log_rec->dis_pageno, 
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
		SETDBERR(&dmve->dmve_error, 0, E_DM9648_UNDO_DISASSOC);
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
	lsn = *log_lsn;
    }

    /* 
    ** Write the LSN of the overflow record onto the page, unless nologging.
    */
    if (dmve->dmve_logging)
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, pinfo->page, lsn);

    /*
    ** Reassociate the page.
    */
    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, pinfo->page, DMPP_ASSOC);
    DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, pinfo->page, DMPP_MODIFY);

    dmveUnMutex(dmve, pinfo);

    return(E_DB_OK);
}

/*{
** Name: dmv_redisassoc - REDO disassociate record.
**
** Description:
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
**	14-dec-1992 (rogerk)
**	    Written for Reduced Logging Project.
**      26-apr-1993 (bryanp)
**          6.5 Cluster support:
**              Replace all uses of DM_LOG_ADDR with LG_LA or LG_LSN.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
*/
static DB_STATUS
dmv_redisassoc(
DMVE_CB             *dmve,
DMP_TABLE_IO        *tabio,
DMP_PINFO           *pinfo)
{
    DM0L_DISASSOC	*log_rec = (DM0L_DISASSOC *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->dis_header.lsn; 
    i4			page_type = log_rec->dis_pg_type;

    CLRDBERR(&dmve->dmve_error);

    /*
    ** Re-dissassociate the page.
    */
    if (pinfo->page) 
    {
	dmveMutex(dmve, pinfo);

	DMPP_VPT_CLR_PAGE_STAT_MACRO(page_type, pinfo->page, DMPP_ASSOC);

	DMPP_VPT_SET_PAGE_STAT_MACRO(page_type, pinfo->page, DMPP_MODIFY);
	DMPP_VPT_SET_PAGE_LOG_ADDR_MACRO(page_type, pinfo->page, *log_lsn);

	dmveUnMutex(dmve, pinfo);
    }

    return(E_DB_OK);
}
