/*
**Copyright (c) 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <pc.h>
#include    <sr.h>
#include    <di.h>
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
#include    <dm0p.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm1p.h>
#include    <dm1s.h>
#include    <dm1c.h>
#include    <dm1h.h>
#include    <dm1i.h>
#include    <dm2umct.h>
#include    <dmpbuild.h>

/**
**
**  Name: DMVELOAD.C - The recovery of a load table operation.
**
**  Description:
**          dmve_load - The recovery of a load table operation.
**
**
**  History:
**	29-sep-87 (rogerk)
**	    Created new for Jupiter. 
**	6-Oct-88 (Edhsu)
**	    Fix error mapping bug
**	22-jan-90 (rogerk)
**	    Make sure the page buffer is aligned on ALIGN_RESTRICT boundary
**	    for byte-align machines.
**      14-jun-90 (jennifer)
**          Change routines to use dm1sbbegin and dm1sbend routines
**          to build empty files.  These routines were used to insure
**          all the new file header, bit map information was set correctly.
**          For bulkload into a heap with data, a new recovery algorithm
**          was used to insure file header and bit map were retained.
**	30-Dec-1991 (rmuth)
**	    When we abort a load into an empty Btree table we just rebuild
**	    an empty btree table ontop of the loaded table. We do this by
**	    calling the build routines dm1bbbegin and dm1bbend, the dm1bbbegin
**	    routine expects certain fields in the mct control block to be set,
**	    these fields are subsequently used during the dm1bbput operations.
**	    As we do not use dm1bbput we were not setting these fields hence
**	    dm1bbbegin was issuing an error. As we already have the values
**	    in the TCB we initialise the values in the mct.
**	8-Jan-1992 (rmuth)
**	    Abort of a LOAD operation was causing relpages and reltups
**	    to be set to incorrect values.
**	13-feb-1992 (bryanp)
**	    Set mct_allocation and mct_extend.
**	28-may-1992 (bryanp)
**	    Set mct_inmemory and mct_rebuild.
**	07-july-1992 (jnash)
**	    Add DMF function prototyping 
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Set mct_compressed from relcomptype.
**	    Set up new mct_data_atts instead of mct_atts_ptr (removed).
**	29-August-1992 (rmuth)
**	   Add parameter to dm1p_lastused.
**	3-oct-1992 (bryanp)
**	    Re-arrange includes to share transaction ID type definition.
**	05-oct-1992 (rogerk)
**	    Reduced Logging Project.  In preparation for support of Partial
**	    TCB's for the new recovery schemes, changed references to
**	    some tcb fields to use the new tcb_table_io structure fields.
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Changed arguments to dm0p_mutex and
**	    dm0p_unmutex call(s)s.
**	30-October-1992 (rmuth)
**	   Set the mct_guarantee_on_disk flag.
**	8-oct-1992 (bryanp)
**	    Initialize mct_buildrcb.
**	14-dec-1992 (jnash)
**	    Reduced logging project.  Back off the previous change 
**	    (in 6.5, never used in 6.4) where we always created an
**	    empty table, revert back to the old 6.4 file swapping 
**	    behavior.  This is because of recovery problems when someone 
**	    first deletes all rows in a table (including rows on overflow 
**	    pages), and then performs a load, and then aborts the load.  
**	    In this case installing an empty table is not the thing to do.
**	    Upcoming "with emptytable" option will once again introduce
**	    this feature.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	08-feb-1993 (rmuth)
**	    Use DM2T_A_OPEN_NOACCESS when opening the table so that
**	    we do not check the FHDR/FMAP as these may be ill at the
**	    moment.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	18-oct-1993 (rogerk)
**	    Changed heap load undo recovery to rewrite the allocated pages to
**	    be free pages rather than leaving them as formatted data pages.
**	    This allows verifydb and patch table to run without danger
**	    of thinking the freed pages should be restored and the tuples
**	    resurrected.  Also ifdef'd code which does recovery for structured
**	    tables when the EMPTY_TABLE copy option is specified.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size to format accessor.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**      15-aug-97 (stephenb)
**          add DML_SCB parameter to dm2t_open() call.
**	01-Jul-1998 (jenjo02)
**	    Added (DMPP **)page parameter to dm1p_lastused() prototype.
**	11-Aug-1998 (jenjo02)
**	    Clear rcb_tup_adds, rcb_page_adds, instead of TCB fields.
**      12-Apr-1999 (stial01)
**          Distinguish leaf/index info for BTREE/RTREE indexes
**      12-nov-1999 (stial01)
**          Changes to how btree key info is stored in tcb
**	23-Jun-2000 (jenjo02)
**	    Added fill_option to dmpp_format() prototype, set by caller
**	    to DM1C_ZERO_FILL if dmpp_format should MEfill the page,
**	    DM1C_ZERO_FILLED if the page has already been MEfilled.
**	    The page overhead space is always zero-filled.
**      06-jul-2000 (stial01)
**          Changes to btree leaf/index klen in tcb (b102026) 
**      23-oct-2000 (stial01)
**          Variable Page Size - Variable Page Type support (SIR 102955)
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      09-mar-2001 (stial01)
**          Changes for Variable Page Type SIR 102955
**      31-jan-05 (stial01)
**          Added flags arg to dm0p_mutex call(s).
**	06-Mar-2007 (jonj)
**	    Replace dm0p_cachefix_page() with dmve_cachefix_page()
**	    for Cluster REDO recovery.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	25-Nov-2008 (jonj)
**	    SIR 120874: dm0l_? functions converted to DB_ERROR *
**	28-Nov-2008 (jonj)
**	    SIR 120874: dm2t_? functions converted to DB_ERROR *
**	04-Dec-2008 (jonj)
**	    SIR 120874: dm1p_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace dm0p_mutex/unmutex with dmveMutex/Unmutex
**	    macros.
**	    Replace DMPP_PAGE* with DMP_PINFO* as needed.
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*{
** Name: dmve_load - The recovery of a load table opration.
**
** Description:
**
**	This function performs UNDO recovery of a load operation.
**	Load operations are NON-REDO and are not journalled.
**
**	User tables are loaded in two different ways.  For HEAP 
**	tables, rows are added to the already existing table.  For HASH,
**	BTREE and ISAM, a new file is created and loaded, and then 
**	renamed to the base file.  The method used is given by 
**	log_rec->dul_recreate.
**
**      If dul_recreate is set, recovery will have already destroyed the
**      new file and moved the old one back in place.  All we have to do is
**      close-purge the TCB.
**
**      If dul_recreate is not set, then we have to delete all the rows that
**      were added to the table.  We also have to deal with the possibility
**      that the load was killed off in the middle of a file extend, which
**      may leave some overflow pointers invalid.  To avoid these problems,
**      we bypass normal access methods and just read through the file page
**      by page, formatting each as an empty data page.
**
**      For HEAP files one can also bulkload into a table with data.
**	To support recovery for this type of bulkload, a new variable
**      was added to the log record and indicates that last valid page
**      of the heap table before the load began.  In all other cases this
**      variable is set to zero.  To recover this operation, all pages
**      allocated after the page indicated by the lastpage variable, are
**      freed and the overflow pointer on the lastpage is set to zero.
**     
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The load table operation log record.
**	    .dmve_action	    Should only be DMVE_UNDO.
**	    .dmve_lg_addr	    The log address of the log record.
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
**	29-sep-87 (rogerk)
**	    Created new for Jupiter. 
**	22-jan-90 (rogerk)
**	    Make sure the page buffer is aligned on ALIGN_RESTRICT boundary
**	    for byte-align machines.
**      14-jun-90 (jennifer)
**          Change routines to use dm1sbbegin and dm1sbend routines
**          to build empty files.  These routines were used to insure
**          all the new file header, bit map information was set correctly.
**          For bulkload into a heap with data, a new recovery algorithm
**          was used to insure file header and bit map were retained.
**	30-Dec-1991 (rmuth)
**	    When we abort a load into an empty Btree table we just rebuild
**	    an empty btree table ontop of the loaded table. We do this by
**	    calling the build routines dm1bbbegin and dm1bbend, the dm1bbbegin
**	    routine expects certain fields in the mct control block to be set,
**	    these fields are subsequently used during the dm1bbput operations.
**	    As we do not use dm1bbput we were not setting these fields hence
**	    dm1bbbegin was issuing an error. As we already have the values
**	    in the TCB we initialise the values in the mct.
**	8-Jan-1992 (rmuth)
**	    Abort of load was setting reltups and relpages to incorrect values,
**	    because it was not taking into account the fact that the update of
**	    relpages and reltups is logged by a SREP record which is also
**	    rolled back causing relpages and reltups to be set to their 
**	    correct preload values. The fix is to set tcb_page_adds and
**	    tcb_tup_adds to zero so that relpages and reltups are left alone
**	    by the rollback of the LOAD log record.
**	13-feb-1992 (bryanp)
**	    Set mct_allocation and mct_extend. The build routines won't actually
**	    need these, since we're rebuilding over an existing file, but it's
**	    good to set all our parameters before calling the build routines,
**	    just as a matter of practice.
**	28-may-1992 (bryanp)
**	    Set mct_inmemory and mct_rebuild to 0.
**	09-jun-1992 (kwatts)
**	    6.5 MPF Changes. Set mct_compressed from relcomptype.
**	    Set up new mct_data_atts instead of mct_atts_ptr (removed).
**	29-August-1992 (rmuth)
**	   Add parameter to dm1p_lastused.
**	30-October-1992 (rmuth)
**	   Set the mct_guarantee_on_disk flag.
**	8-oct-1992 (bryanp)
**	    Initialize mct_buildrcb.
**	14-dec-1992 (jnash)
**	    Reduced logging project.  Back off the previous change 
**	    (in 6.5, never used in 6.4) where we always created an
**	    empty table, revert back to the old 6.4 file swapping 
**	    behavior.  This is because of recovery problems when someone 
**	    first deletes all rows in a table (including rows on overflow 
**	    pages), and then performs a load, and then aborts the load.  
**	    In this case installing an empty table is not the thing to do.
**	    Upcoming "with emptytable" option will once again introduce
**	    this feature.
**	08-feb-1993 (rmuth)
**	    Use DM2T_A_OPEN_NOACCESS when opening the table so that
**	    we do not check the FHDR/FMAP as these may be ill at the
**	    moment.
**	18-oct-1993 (rogerk)
**	    Changed heap load recovery to rewrite the allocated pages to
**	    be free pages rather than leaving them as formatted data pages.
**	    This allows verifydb and patch table to run without danger
**	    of thinking the freed pages should be restored and the tuples
**	    resurrected.  Also ifdef'd code which does recovery for structured
**	    tables when the EMPTY_TABLE copy option is specified.
**	18-oct-1993 (rogerk)
**	    Added dmve_unreserve_space call to release logfile space
**	    reserved for logforces in the purge tcb call.
**	15-apr-1994 (chiku)
**	    Bug56702: return logfull indication.
**	06-mar-1996 (stial01 for bryanp)
**	    Pass page_size to format accessor.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to use
**	    macros.
**      21-may-1997 (stial01)
**          Added flags arg to dm0p_unmutex call(s).
**	01-Dec-2004 (jenjo02)
**	    Added DM0P_TABLE_LOCKED_X flag for bug 108074 completeness.
**      21-Sep-2006 (kschendel)
**          Apparently this never worked for partitions!  Fix.
**          Also, it's probably a bad idea to open the load table NOACCESS.
**          And finally, don't use TABLE_LOCKED_X here because we might
**          be processing an fhdr/fmap in the loop that reformats DATA
**          pages as free.  Causes Unfix RELEASE warnings in the BM.
**          We never update fmap/fhdr so don't worry about LOCKED_X.
**	16-Nov-2009 (kschendel) SIR 122890
**	    Don't close-purge the TCB if the load was non-recreate load.
**	    We may not have a control lock in that situation, and there's
**	    no need to close-purge anyway unless it was a recreate load.
**	13-Apr-2010 (kschendel) SIR 123485
**	    Open no-coupon to avoid unnecessary LOB overhead.
*/
DB_STATUS
dmve_load(
DMVE_CB		*dmve_cb)
{
    DMVE_CB		*dmve = dmve_cb;
    DM0L_LOAD		*log_rec = (DM0L_LOAD *)dmve_cb->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->dul_header.lsn; 
    DMP_RCB             *rcb;
    DMP_RCB             *master_rcb;
    DMP_TCB		*t;
    DMP_TABLE_IO	*tbio = NULL;
    DMPP_PAGE           *page;			
    DMPP_ACC_PLV	*loc_plv;
    DB_TAB_ID           master_id;
    DB_TAB_TIMESTAMP	timestamp;
    LG_LSN		lsn;
    DM_PAGENO		pageno;
    DM_PAGENO		last_page;
    i4		close_flags;
    i4		dm0l_flags;
    i4		error;
    i4		local_error;
    DB_STATUS		status;
    DB_ATTS             **keyatts = (DB_ATTS **)0;
    bool                is_partition = FALSE;
    DB_ERROR		local_dberr;
    DMP_PINFO		*pinfo = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    rcb = master_rcb = NULL;
    for (;;)
    {
	if (log_rec->dul_header.length != sizeof(DM0L_LOAD) || 
	    log_rec->dul_header.type != DM0LLOAD ||
	    dmve->dmve_action != DMVE_UNDO)
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
	    status = E_DB_ERROR;
	    break;
	}
	/* Finish with close-purge if this was a recreate load (file
	** renaming going on).  Don't purge if a non-recreate load undo;
	** not only is it unnecessary, but we may not have the necessary
	** X control lock in the non-recreate case.
	*/
	close_flags = DM2T_PURGE;
	if (! log_rec->dul_recreate)
	    close_flags = DM2T_NOPURGE;

	/*
	** Open up the table so we can possibly recover the load action
	** (if a non-recreate load was done) and so that we can close-purge
	** the TCB below.  Unfortunately for the non-recreate undo, we
        ** need a real RCB, not just a TBIO.  So if the loaded table was
        ** a partition, open the master as well.
        */
        STRUCT_ASSIGN_MACRO(log_rec->dul_tbl_id, master_id);
        if (master_id.db_tab_index < 0)
        {
            is_partition = TRUE;
            master_id.db_tab_index = 0;
        }
        status = dm2t_open(dmve->dmve_dcb_ptr, &master_id, DM2T_X,
            DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, (i4)0, (i4)20, 0,
	    dmve->dmve_log_id, dmve->dmve_lk_id, 0, 0, dmve->dmve_db_lockmode,
            &dmve->dmve_tran_id, &timestamp,
            &master_rcb, (DML_SCB *)0, &dmve->dmve_error);
	if (status != E_DB_OK)
	    break;
        if (is_partition)
        {
            status = dm2t_open(dmve->dmve_dcb_ptr, &log_rec->dul_tbl_id,
                DM2T_X, DM2T_UDIRECT, DM2T_A_WRITE_NOCPN, (i4)0, (i4)20, 0,
                dmve->dmve_log_id, dmve->dmve_lk_id, 0, 0,
                dmve->dmve_db_lockmode,
                &dmve->dmve_tran_id, &timestamp, &rcb, (DML_SCB *)0, &dmve->dmve_error);
            if (status != E_DB_OK)
		break;
        }
        else
        {
            rcb = master_rcb;
        }

        /* Turn off logging. */

	rcb->rcb_logging = 0;
	t = rcb->rcb_tcb_ptr;
	tbio = &t->tcb_table_io;
	dmve->dmve_tbio = tbio;
	loc_plv = t->tcb_acc_plv;
	MEfill(sizeof(dmve->dmve_pages), '\0', &dmve->dmve_pages);

	if (t->tcb_rel.relspec == TCB_BTREE || 
	    t->tcb_rel.relspec == TCB_RTREE)
	    keyatts = t->tcb_leafkeys;
	else
	    keyatts = t->tcb_key_atts; /* isam, hash */

	/*
	** If dul_recreate is set, then a new file was created for the load
	** and then renamed on top of the old file.  Recovery will have already
	** destroyed the new file and moved the old one back in place.
	** All we have to do now is close-purge the TCB at the bottom.
	**
	** If dul_recreate is not set then the COPY was to an existing heap
	** table and we added the new rows into the current file.  The undo
	** recovery action is then to delete the added rows by freeing the
	** new pages added in the load.
	** Non-recreate mode recovery is only permitted for heap.  There is
	** no good way to undo loads into a non-heap table that would be
	** guaranteed to restore the table to its exact physical state.
	** Thus additional DML undoes would not necessarily work.
	** Therefore Ingres never chooses non-recreate mode for anything
	** but heap tables.
	**
	*/
	if ( ! log_rec->dul_recreate)
	{
	    if (log_rec->dul_lastpage >= 0)
	    {
		/*
		** Copy into an existing heap (possibly non-empty).
		** Free the pages allocated during the copy.
		*/

		status = dm1p_lastused(rcb, 0, &last_page, (DMP_PINFO*)NULL,
					&dmve->dmve_error);	
		if (status != E_DB_OK)
		    break;
		status = dm1p_free(rcb, log_rec->dul_lastpage + 1, 
		    (i4) last_page, &dmve->dmve_error);
		if (status != E_DB_OK)
		    break;

		/*
		** Find the page which was at the end of the HEAP before
		** the load was started and change its ovfl page pointer
		** to unlink the new pages.
		*/
		status = dmve_cachefix_page(dmve, log_lsn,
					    tbio, log_rec->dul_lastpage,
					    DM0P_READAHEAD, 
					    loc_plv,
					    &pinfo);
		if (status != E_DB_OK)
		    break;
		page = pinfo->page;

		dmveMutex(dmve, pinfo);
		DMPP_VPT_SET_PAGE_OVFL_MACRO(t->tcb_rel.relpgtype, page, 0);
		DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page,
		    DMPP_MODIFY);
		dmveUnMutex(dmve, pinfo);

		status = dm0p_uncache_fix(tbio, DM0P_UNFIX, dmve->dmve_lk_id, 
		    dmve->dmve_log_id, &dmve->dmve_tran_id, pinfo, 
		    &dmve->dmve_error);
		if (status != E_DB_OK)
		    break;

		/*
		** Cycle through the allocated pages and re-format them to
		** be free pages.  Be careful only to update actual allocated
		** Data pages; don't muck with any pages that were assigned
		** as FMAP's for the extended table.
		**
		** Specify readahead flag and hope to get group IO actions.
		*/
		for (pageno = log_rec->dul_lastpage + 1; 
		     pageno <= last_page;
		     pageno++)
		{
		    status = dmve_cachefix_page(dmve, log_lsn,
						tbio, pageno,
						DM0P_READAHEAD, 
						loc_plv,
						&pinfo);
		    if (status != E_DB_OK)
			break;
		    page = pinfo->page;

		    if (DMPP_VPT_GET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page) & 
			DMPP_DATA)
		    {
			dmveMutex(dmve, pinfo);
			(*loc_plv->dmpp_format)(t->tcb_rel.relpgtype, 
				t->tcb_rel.relpgsize, page, pageno, DMPP_FREE,
				DM1C_ZERO_FILL);
			DMPP_VPT_SET_PAGE_STAT_MACRO(t->tcb_rel.relpgtype, page, 
			    DMPP_MODIFY);
			dmveUnMutex(dmve, pinfo);
		    }

		    status = dm0p_uncache_fix(tbio, DM0P_UNFIX, 
			dmve->dmve_lk_id, dmve->dmve_log_id, 
			&dmve->dmve_tran_id, pinfo, &dmve->dmve_error);
		    if (status != E_DB_OK)
			break;
		}
	    }
	    else
	    {
		/*
		** Copy into non-heap table that did not use recreate mode.
		** This mode is not currently supported by the recovery
		** system.
		*/
		TRdisplay("DMVE_LOAD: Unexpected load undo mode - non-\n");
		TRdisplay("    recreate mode load on a structured table.\n");
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		status = E_DB_ERROR;
		break;
	    }	

	    rcb->rcb_page_adds = 0;
	    rcb->rcb_tup_adds  = 0;
	}

	/*
	** Close and maybe purge the table.
	*/
	status = dm2t_close(rcb, close_flags, &dmve->dmve_error);
	rcb = (DMP_RCB *) 0;
	if (status != E_DB_OK)
	    break;
        if (is_partition)
        {
            status = dm2t_close(master_rcb, close_flags, &dmve->dmve_error);
            master_rcb = NULL;
            if (status != E_DB_OK)
		break;
        }

	/*
	** Write the CLR if necessary.
	*/
	if ((dmve->dmve_logging) &&
	    ((log_rec->dul_header.flags & DM0L_CLR) == 0))
	{
	    dm0l_flags = (log_rec->dul_header.flags | DM0L_CLR);

	    status = dm0l_load(dmve->dmve_log_id, dm0l_flags, 
				&log_rec->dul_tbl_id, &log_rec->dul_name, 
				&log_rec->dul_owner, &log_rec->dul_location,
				log_rec->dul_structure, log_rec->dul_recreate, 
				log_rec->dul_lastpage, log_lsn, &lsn, 
				&dmve->dmve_error);
	    if (status != E_DB_OK) 
	    {
		/*
		 * Bug56702: return logfull indication.
		 */
		dmve->dmve_logfull = dmve->dmve_error.err_code;
		break;
	    }

	    /*
	    ** Release log file space allocated when the LOAD log record
	    ** was written.  (LOAD allows for a FORCE in case of purging.)
	    */
	    dmve_unreserve_space(dmve, 1);
	}

	return (E_DB_OK);
    }

    /*
    ** Error cleanup.
    */
    if (pinfo && pinfo->page)
    {
	status = dm0p_uncache_fix(&rcb->rcb_tcb_ptr->tcb_table_io, DM0P_UNFIX, 
	    dmve->dmve_lk_id, dmve->dmve_log_id, &dmve->dmve_tran_id, 
	    pinfo, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }

    if (rcb)
    {
	status = dm2t_close(rcb, close_flags, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
	}
    }
    if (master_rcb && is_partition)
    {
       status = dm2t_close(master_rcb, close_flags, &local_dberr);
       if (status != E_DB_OK)
       {
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
               (char *)NULL, (i4)0, (i4 *)NULL, &local_error, 0);
       }
    }
        
    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL, 
	(i4)0, (i4 *)NULL, &error, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM961C_DMVE_LOAD);

    return(E_DB_ERROR);
}
