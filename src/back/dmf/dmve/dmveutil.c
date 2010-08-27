/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <di.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <tr.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
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
#include    <dm1r.h>
#include    <dm1h.h>
#include    <dmve.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dm0p.h>
#include    <dm0c.h>
#include    <dm0m.h>
#include    <dmfjsp.h>
#include    <dmftrace.h>
#include    <dm0pbmcb.h>

/**
**
**  Name: DMVEUTIL.C - Utility routines for DMVE recovery.
**
**  Description:
**	This file contains utility routines used during recovery processing.
**
**          dmve_fix_tabio - Get Table Io Control Block for recovery operation.
**	    dmve_location_check - Check if specified location needs recovery.
**	    dmve_partial_recov_check - Check if partial recovery performed.
**          dmve_get_iirel_row - Get a row from iirelation for a given table.
**          dmve_cachefix_page - Wrapper for dm0p_cachefix_page()
**
**  History:
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: written
**	    Written for 6.5 recovery.
**	14-dec-1992 (rogerk)
**	    Include dm1b.h before dm0l.h for prototype definitions.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	21-jun-1993 (rogerk)
**	    Add error messages.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-1993 (rogerk)
**	    Include respective CL headers for all cl calls.
**	23-aug-1993 (rogerk)
**	    Changed dmve_trace_page_info enabling tracepoint to DM1315.
**	21-sep-1993 (rogerk)
**	    Moved rfp_add_action, rfp_lookup_action, and rfp_remove_action
**	    routines here from dmfrfp.c so that executables which do not
**	    include dmfrfp will link properly.
**	18-oct-1993 (jnash)
**	    Split out code in dmve_trace_page_info() into new
**	    dmve_trace_page_lsn(), dmve_trace_page_header() and
**	    dmve_trace_page_contents() functions.  These are now called by 
**	    recovery routines in certain error cases.
**	18-oct-1993 (rmuth)
**	    CL prototype, add <dm0m.h>.
**	18-oct-1993 (rogerk)
**	    Add dmve_unreserve_space routine.
**	10-nov-93 (tad)
**	    Bug #56449
**	    Replaced %x with %p for pointer values where necessary in
**	    dmve_trace_page_contents().
**	20-jun-1994 (jnash)
**	    fsync project.  If database opened sync-at-close, then open 
**	    underlying tables in the same way.
**      20-may-95 (hanch04)
**          Added include dmtcb.h for ndmfapi
**	31-jan-1996 (nick)
**	    In build_recovery_info(), build a list of locations touched 
**	    by a DM0L_EXTEND - the locations containing the FHDR and FMAP
**	    are not sufficient.
**	07-may-96 (nick)
**	    Fix for #76348 ; UNDO recovery on multiple
**	    location btrees could fail.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm2t_init_tabio_cb and dm2t_add_tabio_file
**		for managing a table's page size during recovery.
**      06-mar-1996 (stial01)
**          Pass page_size to dmve_trace_page_info and dmve_trace_page_header 
**          dmve_fix_tabio(): Since RCP and JSP don't always alloc page arrays 
**          for each page size when the buffer manager is initialized, allocate 
**          the page array here if necessary to perform a recovery operation.
**	06-may-1996 (thaju02)
**	    New page format support: change page header references to macros.
**	    Pass page_size to dmve_trace_page_lsn().
**	29-may-96 (nick)
**	    Location assignment for DM0LASSOC was broken. #76748
**	10-oct-1996 (shero03)
**	    Added rtput, rtrep, and rtdel for RTREE project
**	25-Aug-1997 (jenjo02)
**	    Added log_id parm to dm2t_ufx_tabio_cb(), dm2t_add_tabio_file(),
**	    dm2t_init_tabio_cb() calls.
**	09-Mar-1999 (schte01)
**       Changed l_id assignment within for loop so there is no dependency on 
**       order of evaluation (in build_recovery_info routine).
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
**      18-feb-2004 (stial01)
**          Fixed incorrect casting of length arguments.
**      30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**          Added dmve_get_iirel_row().
**      07-apr-2005 (stial01)
**          Added dmve_get_tab() 
**	06-Mar-2007 (jonj)
**	    Added dmve_cachefix_page() for online Cluster REDO
**      24-Sep-2007 (horda03) Bug 119172
**       dmve_fix_tabio(). If the tcb can't be fixed because the partial TCB
**       built are tossed before it can be fixed, try creating/fixing the
**       TCB.
**      14-jan-2008 (stial01)
**          Added dmve_unfix_tabio() for dmve exception handling.
**      28-jan-2008 (stial01)
**          Changes for improved error handling during offline recovery
**	30-May-2008 (jonj)
**	    LSNs are 2 u_i4's; display them in hex rather than
**	    signed decimal.
**	14-Nov-2008 (jonj)
**	    SIR 120874 Change function prototypes to pass DB_ERROR *dberr
**	    instead of i4 *err_code, use new form uleFormat.
**	24-Nov-2008 (jonj)
**	    SIR 120874: dm0m_? functions converted to DB_ERROR *
**	26-Nov-2008 (jonj)
**	    SIR 120874: dm2t_?, dm2r_? functions converted to DB_ERROR *
**	05-Dec-2008 (jonj)
**	    SIR 120874: dm0p_? functions converted to DB_ERROR *
**      12-oct-2009 (stial01)
**          Use DMPP_PAGE for all page types.
**      14-oct-2009 (stial01)
**          Added  dmve_fix_page, dmve_unfix_page
**	23-Oct-2009 (kschendel) SIR 122739
**	    Use get-plv instead of getaccessors.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**      29-Apr-2010 (stial01)
**          Added new routines for iirelation/iisequence recovery        
**	25-May-2010 (kschendel)
**	    Add missing CM inclusion, required for bool declarations.
**	21-Jul-2010 (stial01) (SIR 121123 Long Ids)
**          Remove table name,owner from log records.
**/

/*
** Local structure definitions.
*/

/*
** Name: DMVE_LOC_ENTRY - Gives information on location being recovered.
**
** Description:
**      This structure is used below to build a list of all locations effected
**	by a particular recovery action.  The list can then be used to
**	verify and perhaps build access to each applicable location.
*/
typedef struct
{
    i4	loc_pg_num;
    i4	loc_id;
    i4	loc_cnf_id;
} DMVE_LOC_ENTRY;

/*
** Forward declarations.
*/
static DB_STATUS	build_recovery_info(
				DMVE_CB		*dmve,
				DMVE_LOC_ENTRY	*info_array,
				i4		*num_entries,
				i4		*page_type,
				i4		*page_size,
				i4		*location_count);

static VOID dmve_init_tabinfo(
				DMVE_CB		*dmve,
				DMP_TABLE_IO	*tbio);


/*{
** Name: dmve_fix_tabio - Get Table Io Control Block.
**
** Description:
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the system catalogs put 
**				    operation.
**	    .dmve_action	    Should be DMVE_DO, DMVE_REDO, or DMVE_UNDO.
**	    .dmve_lg_addr	    The log address of the log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**	    .dmve_tran_id	    The physical transaction id.
**	    .dmve_lk_id		    The transaction lock list id.
**	    .dmve_log_id	    The logging system database id.
**	    .dmve_db_lockmode	    The lockmode of the database. Should be 
**				    DM2T_X or DM2T_S.
**
** Outputs:
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
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	18-oct-1992 (jnash)
**	    Fix ule_format of E_DM9679_DMVE_TABIO_BUILD that clobbered err_code.
**	20-jun-1994 (jnash)
**	    fsync project.  If database opened sync-at-close, then open 
**	    underlying tables in the same way.
**	31-jan-1996 (nick)
**	    Increase size of array to hold required locations ; this needs to
**	    be able to contain at least the maximum number of locations in
**	    a table ( which is DM_LOC_MAX ) plus two ( as a DM0L_EXTEND can 
**	    legally contain all locations plus the FHDR and FMAP ).  Note
**	    that this array size is excessive for most other records and 
**	    we should probably allocate entries as required instead.
**	07-may-96 (nick)
**	    Catch those combinations of records and actions which require
**	    ALL the locations of a relation to be open in order for recovery
**	    to proceed.
**	06-mar-1996 (stial01 for bryanp)
**	    Add page_size argument to dm2t_init_tabio_cb and dm2t_add_tabio_file
**		for managing a table's page size during recovery.
**      06-mar-1996 (stial01)
**          Since RCP and JSP don't always allocate page arrays for each  
**          page size when the buffer manager is initialized, allocate 
**          the page array here if necessary to perform a recovery operation.
**      24-sep-2007 (horda03) Bug 119172
**          Call dm2t_init_tabio_cb() after check for excessive loops. If the
**          TCB could not be fixed using dm2t_fx_tabio_cb(), have one more
**          attempt to fix the TCB with dm2t_fix_tcb().
**      27-sep-2007 (horda03) Bug 119172
**          Change to dm2t_init_tabio_cb() interface from 2.0 original.
*/
DB_STATUS
dmve_fix_tabio(
DMVE_CB		*dmve,
DB_TAB_ID	*table_id,
DMP_TABLE_IO	**table_io)
{
    DMP_DCB			*dcb = dmve->dmve_dcb_ptr;
    DMP_TABLE_IO		*tbio = NULL;
    DB_STATUS			status = E_DB_OK;
    DB_OWN_NAME 		table_owner;
    DB_TAB_NAME			table_name;
    i4			page_type;
    i4			page_size;
    i4			i;
    i4			location_count;
    i4			num_entries;
    i4			fix_retry;
    i4			loc_error;
    i4			build_retries = 0;
    i4			flag;
    bool			recovery_info_obtained = FALSE;
    bool			need_full_tcb = FALSE;
    DMVE_LOC_ENTRY		loc_info_array[DM_LOC_MAX + 2];
    DMVE_LOC_ENTRY		*loc_entry;
    i4			*err_code = &dmve->dmve_error.err_code;
    DB_ERROR		local_dberr;
    DMP_RELATION	reltup;
    DMP_TCB		*t = NULL;

    CLRDBERR(&dmve->dmve_error);
    DMVE_CLEAR_TABINFO_MACRO(dmve);

    /*
    ** If MVCC, make_consistent has provided dmve_tbio, dmve_plv
    ** and dmve_lk_type
    */
    if ( dmve->dmve_flags & DMVE_MVCC )
    {
	*table_io = dmve->dmve_tbio;
	MEfill(sizeof(dmve->dmve_pages), '\0', &dmve->dmve_pages);
	/* currently dont need to call dmve_init_tabinfo for MVCC */
        return(E_DB_OK);
    }

    for (;;)
    {
	/*
	** Build a list of all locations required in order to recover the 
	** passed-in log record.  We use this list to build/verify a tableio 
	** control block that allows access to all the required locations.
	**
	** Note: warning return indicates that no locations were found
	** to be recovered.  This is possible during partial recovery
	** operations.
	*/
	status = build_recovery_info(dmve, loc_info_array, &num_entries,
		&page_type, &page_size, &location_count);
	if (DB_FAILURE_MACRO(status))
	    break;
	if (status == E_DB_WARN)
	{
	    *table_io = tbio;
	    SETDBERR(&dmve->dmve_error, 0, W_DM9660_DMVE_TABLE_OFFLINE);
	    dmve->dmve_tbio = tbio;
	    return (E_DB_WARN);
	}
	recovery_info_obtained = TRUE;

	dm1c_get_plv(page_type, &dmve->dmve_plv);

	/*
	** Init adf control block to use for operations on this table.
	** Set default values for adf variables - collation and timezone.
	*/
	MEfill(sizeof(ADF_CB), 0, &dmve->dmve_adf_cb);
	dmve->dmve_adf_cb.adf_errcb.ad_ebuflen = 0;
	dmve->dmve_adf_cb.adf_errcb.ad_errmsgp = 0;
	dmve->dmve_adf_cb.adf_maxstring = DB_MAXSTRING;
	dmve->dmve_adf_cb.adf_tzcb = dmf_svcb->svcb_tzcb;
	dmve->dmve_adf_cb.adf_collation = dmve->dmve_dcb_ptr->dcb_collation;
	dmve->dmve_adf_cb.adf_ucollation = dmve->dmve_dcb_ptr->dcb_ucollation;
	if (dmve->dmve_adf_cb.adf_ucollation)
	{
	    if (dmve->dmve_dcb_ptr->dcb_dbservice & DU_UNICODE_NFC)
		dmve->dmve_adf_cb.adf_uninorm_flag = AD_UNINORM_NFC;
	    else dmve->dmve_adf_cb.adf_uninorm_flag = AD_UNINORM_NFD;
	}
	else 
	    dmve->dmve_adf_cb.adf_uninorm_flag = 0;

	if (CMischarset_utf8())
	    dmve->dmve_adf_cb.adf_utf8_flag = AD_UTF8_ENABLED;
	else
	    dmve->dmve_adf_cb.adf_utf8_flag = 0;

	MEfill(sizeof(ADI_WARN), '\0', &dmve->dmve_adf_cb.adf_warncb);

	/* Reset fields that keep track of pages fixed by dmve */
	MEfill(sizeof(dmve->dmve_pages), '\0', &dmve->dmve_pages);
	dmve->dmve_lk_type = 0;
			
	/* Make sure we have allocated pages for this page size */
	if (!(dm0p_has_buffers(page_size)))
	{
	    status = dm0p_alloc_pgarray(page_size, &dmve->dmve_error);
	    if (status)
		return (E_DB_ERROR);
	}

	/*
	** Try looking up the tableio control block.  Hopefully most
	** calls will simply result in this finding a usable TABIO.
	** Expected return values from this call:
	**     E_DB_OK   - tcb found
	**     E_DB_WARN - tcb not found
	**     E_DB_ERROR with E_DM0054_NONEXISTENT_TABLE
	**
	** If the database is opened sync-at-close (e.g. rolldb) then open 
	** underlying tables/files in the same way.  SCONCUR tcb's are 
	** opened specially and are not futzed with.
	*/
	flag = DM2T_VERIFY;
	if ( (dcb->dcb_status & DCB_S_SYNC_CLOSE) &&
	     (table_id->db_tab_base > DM_SCONCUR_MAX) )
	    flag |= DM2T_CLSYNC;

	if (need_full_tcb)
	{
	    DMP_TCB	*tcb = NULL;

	    /* ensure all locations are open */
	    status = dm2t_fix_tcb(dcb->dcb_id, table_id, &dmve->dmve_tran_id,
			dmve->dmve_lk_id, dmve->dmve_log_id,
			(flag | DM2T_NOPARTIAL), dcb, &tcb, &dmve->dmve_error);
	    
	    if (status == E_DB_OK)
		tbio = &tcb->tcb_table_io;
	}
	else
	{
	    status = dm2t_fx_tabio_cb(dcb->dcb_id, table_id, 
			&dmve->dmve_tran_id, dmve->dmve_lk_id, 
			dmve->dmve_log_id, flag, &tbio, &dmve->dmve_error);
	}

	if ((DB_FAILURE_MACRO(status)) && 
	    (dmve->dmve_error.err_code != E_DM0054_NONEXISTENT_TABLE))
	{
	    break;
	}

	/*
	** If a Complete (non-partial) TCB was found, then we can always
	** just return it.
	*/
	if ((status == E_DB_OK) && ((tbio->tbio_status & TBIO_PARTIAL_CB) == 0))
	{
	    *table_io = tbio;
	    dmve->dmve_tbio = tbio;
	    dmve_init_tabinfo(dmve, tbio);
	    return (E_DB_OK);
	}
	status = E_DB_OK;                                               

	/*
	** If there was no tcb found, call dm2t_init_tabio to build a partial
	** table control block.  The CB built will have no open files and
	** will need to open the ones necessary to us with dm2t_add_tabio
	** calls below.
	**
	** After building a partial Table Descriptor into the tcb cache
	** then start back at the top.  We should find the newly-built 
	** table this time.
	*/
	if (tbio == NULL)
	{
	    if (!build_retries)
	    {
		/* Get the iirelation tuple, we need the table name, owner */
		status = dmve_get_iirel_row(dmve, table_id, &reltup);
		if (status)
		{
		    SETDBERR(&dmve->dmve_error, 0, E_DM9661_DMVE_FIX_TABIO_RETRY);
		    status = E_DB_ERROR;
		    break;
		}
	    }

	    /*
	    ** Make sure we're not looping in here forever.
	    ** (Note that it is possible for somebody to toss out our
	    ** newly built tcb to make room in the TCB cache between the
	    ** time that we initialized it (see below) and when we went back to
	    ** fix it.  It is unlikely that this will happen multiple
	    ** times consecutively, however.)
	    */
	    if (build_retries++ > 10)
	    {
                if (!need_full_tcb)
                {
                   /* It looks like the unlikely event described above has
                   ** occured, rather than abort the recovery, try to fix
                   ** the full TCB (this will fix it after it is created).
                   ** If this fails to deliver the TCB, report the error.
                   */
                   need_full_tcb = TRUE;

                   continue;
                }

		SETDBERR(&dmve->dmve_error, 0, E_DM9661_DMVE_FIX_TABIO_RETRY);
		status = E_DB_ERROR;
		break;
	    }

	    status = dm2t_init_tabio_cb(dcb, table_id, &table_name, 
		&table_owner, location_count, dmve->dmve_lk_id, dmve->dmve_log_id,
		page_type, page_size, &dmve->dmve_error);
	    if (status != E_DB_OK)
		break;

	    continue;
	}

	/*
	** Run through each entry in the location array and verify that
	** that location is accessable through our tableio control block.
	** If it isn't, then make it so.
	*/
	fix_retry = FALSE;
	for (i = 0; i < num_entries; i++)
	{
	    loc_entry = &loc_info_array[i];

	    /*
	    ** If this location is not included in the partial TCB then
	    ** unfix our handle and request that this new location
	    ** be added (the unfix is required because the add call needs
	    ** exclusive access to the TCB).
	    **
	    ** After adding the location, start the process of fixing the
	    ** TCB all over again  (We have to break out of the current
	    ** check loop in order to continue back to the original loop).
	    */
	    if (((tbio->tbio_status & TBIO_OPEN) == 0) ||
	    	(dm2f_check_access(tbio->tbio_location_array,
		    tbio->tbio_loc_count, loc_entry->loc_pg_num) == FALSE))
	    {
		status = dm2t_ufx_tabio_cb(tbio, DM2T_VERIFY, 
				dmve->dmve_lk_id, dmve->dmve_log_id, 
				&dmve->dmve_error);
		if (status != E_DB_OK)
		    break;
		tbio = NULL;

		/*
		** Catch location array entries which we
		** need to have open but for which the mapping
		** for table location number vs. extent directory 
		** index is missing.  In this instance, force
		** a non-partial TCB to be built.
		*/
		if (loc_entry->loc_cnf_id != -1)
		{
		    /* normal case */
		    status = dm2t_add_tabio_file(dcb, table_id, 
			&table_name, &table_owner, 
			loc_entry->loc_cnf_id, loc_entry->loc_id, 
			location_count, dmve->dmve_lk_id, dmve->dmve_log_id,
			page_type, page_size, &dmve->dmve_error);
		    if (status != E_DB_OK)
			break;
		}
		else
		{
		    /* need to go build a full TCB */
		    need_full_tcb = TRUE;
		}

		fix_retry = TRUE;
		break;
	    }
	}
	if (status != E_DB_OK)
	    break;

	/*
	** If we broke out of the above loop with a retry flag, then start
	** the fix/validation cycle over again.
	*/
	if (fix_retry)
	    continue;

	*table_io = tbio;
	dmve->dmve_tbio = tbio;
	dmve_init_tabinfo(dmve, tbio);

	return (E_DB_OK);
    }

    /*
    ** Clean up if we left the TCB fixed and return an error.
    */
    if (tbio)
    {
	status = dm2t_ufx_tabio_cb(tbio, DM2T_VERIFY, 
	    dmve->dmve_lk_id, dmve->dmve_log_id, &local_dberr);
	if (status != E_DB_OK)
	{
	    uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	}
    }

    if (recovery_info_obtained)
    {
	uleFormat(NULL, E_DM9679_DMVE_TABIO_BUILD, (CL_ERR_DESC *)NULL, 
	    ULE_LOG, NULL, (char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 3,
	    sizeof(DB_DB_NAME), dcb->dcb_name.db_db_name,
	    sizeof(DB_TAB_NAME), table_name.db_tab_name,
	    sizeof(DB_OWN_NAME), table_owner.db_own_name);
    }

    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
    SETDBERR(&dmve->dmve_error, 0, E_DM9662_DMVE_FIX_TABIO);

    *table_io = NULL;
    dmve->dmve_tbio = tbio;
    return(E_DB_ERROR);
}

/*{
** Name: dmve_unfix_tabio - Unfix Table Io Control Block.
**
** Description:
**
** Inputs:
**	dmve_cb
**      table_io
**      flag
**
** Outputs:
**      table_io			Set to NULL
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    None
**
** History:
**	14-jan-2008 (stial01)
**          Created.
*/
DB_STATUS
dmve_unfix_tabio(
DMVE_CB		*dmve,
DMP_TABLE_IO	**table_io,
i4		flag)
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DB_STATUS		status = E_DB_OK;
    DMP_TABLE_IO	*tbio;
    i4			error;
    i4			dm0p_flags;
    DB_ERROR		local_dberr;

    /* Don't disturb MVCC tbio, fixed by make_consistent() */
    if ( !(dmve->dmve_flags & DMVE_MVCC) )
    {
	tbio = *table_io;
	if (tbio)
	{
	    if (flag & DMVE_INVALID)
	    {
		TRdisplay("Remove invalid table from buffer cache.\n");
		dm0p_flags = (DM0P_INVALID_UNFIX | DM0P_INVALID_UNLOCK);
		dm0p_unfix_invalid_table(dcb, &tbio->tbio_reltid, dm0p_flags);
	    }

	    status = dm2t_ufx_tabio_cb(tbio, DM2T_VERIFY, 
		dmve->dmve_lk_id, dmve->dmve_log_id, &local_dberr);
	    if (status != E_DB_OK)
	    {
		uleFormat(&local_dberr, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &error, 0);
	    }
	}

	dmve->dmve_tbio = NULL;
    }
    *table_io = NULL;

    return (status);
}

/*
** Name: dmve_location_check - Check if specified location needs recovery.
**
** Description:
**	NOTE, CURRENTLY THIS ROUTINE ALWAYS RETURNS TRUE as partial recovery
**	has not yet been implemented.
**
**	This routine is used for partial recovery handling.  It takes
**	a location id (specified by index to that location in the database 
**	config file location list) and table id and returns whether that
**	location/table is included in this recovery operation.
**
**	If the table_id is not specified, then the return value is dependent
**	only upon whether the indicated location is being recovered.
**
**	If the current recovery operation is not operating in partial recovery
**	mode, then this call will always return TRUE.
**
** Inputs:
**	dmve			- DMVE control block
**	location_cnf_id		- Location Id.
**
** Outputs:
**	Returns:
**	    TRUE		- Location is being recovered
**	    FALSE		- Location is not being recovered
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Written.
*/
bool
dmve_location_check(
DMVE_CB		*dmve,
i4		location_cnf_id)
{
    return (TRUE);
}

/*
** Name: dmve_partial_recov_check - Check if partial recovery being done. 
**
** Description:
**	NOTE, CURRENTLY THIS ROUTINE ALWAYS RETURNS FALSE as partial recovery
**	has not yet been implemented.
**
**	This routine is used for partial recovery handling.  It 
**	checks if partial recovery is being performed, returning
**	true if it is.
**
** Inputs:
**	dmve			- DMVE control block
**
** Outputs:
**	Returns:
**	    TRUE		- Partial recovery is being recovered
**	    FALSE		- Partial recovery is not being recovered
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-oct-1992 (jnash)
**	    Reduced Logging Project: Written.
*/
bool
dmve_partial_recov_check(
DMVE_CB		*dmve)
{
    return (FALSE);
}

/*
** Name: build_recover_info - Build array of DMVE_LOC_INFO for checking
**                            access to all required locations.
**
** Description:
**	This routine also figures out the page size for the table being
**	recovered and sets it for the caller.
**
** Inputs:
**	dmve_cb
**	    .dmve_log_rec	    The log record of the system catalogs put 
**				    operation.
**	    .dmve_action	    Should be DMVE_DO, DMVE_REDO, or DMVE_UNDO.
**	    .dmve_lg_addr	    The log address of the log record.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**	    .dmve_tran_id	    The physical transaction id.
**	    .dmve_lk_id		    The transaction lock list id.
**	    .dmve_log_id	    The logging system database id.
**	    .dmve_db_lockmode	    The lockmode of the database. Should be 
**				    DM2T_X or DM2T_S.
**
** Outputs:
**	page_size		    table page size 
**	page_type		    table page type
**
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
**	26-oct-1992 (rogerk)
**	    Reduced Logging Project: Written.
**	21-oct-1992 (jnash)
**	    Add DM0LOVFL, DMVENOFULL, DMVEASSOC
**	31-jan-1996 (nick)
**	    Locations touched by an extend are needed for recovery and
**	    not just the locations which contain the FHDR and FMAP. #74058
**	07-may-96 (nick)
**	    Solve UNDO recovery problems on multiple location
**	    btrees ; add placeholders for locations we need open
**	    but haven't got all the requisite information ; our caller 
**	    will resolve these. #76348
**	06-mar-1996 (stial01 for bryanp)
**	    Return page size to caller.
**	29-may-96 (nick)
**	    DM0LASSOC handling broken - new page location was being
**	    populated with the leaf location id and hence broke recovery
**	    when the table was multilocation and the leaf and new pages
**	    resided on different locations.
**	10-oct-1996 (shero03)
**	    Added rtput, rtrep, and rtdel for RTREE project
**	09-Mar-1999 (schte01)
**       Changed l_id assignment within for loop so there is no dependency on 
**       order of evaluation.
**      23-Feb-2009 (hanal04) Bug 121652
**          Added DM0LUFMAP case to deal with FMAP updates needed for an
**          extend operation where a new FMAP did not reside in the last
**          FMAP or new FMAP itself.
*/
static DB_STATUS
build_recovery_info(
DMVE_CB		*dmve,
DMVE_LOC_ENTRY	*info_array,
i4		*num_entries,
i4		*page_type,
i4		*page_size,
i4		*location_count)
{
    DB_STATUS		status = E_DB_OK;
    i4		i = 0;

    *location_count = 0;
    *num_entries = 0;

    switch (((DM0L_HEADER *)dmve->dmve_log_rec)->type)
    {
	case DM0LPUT:
	{
	    DM0L_PUT	*put_rec = (DM0L_PUT *)dmve->dmve_log_rec;

	    if (put_rec->put_header.length != 
		    (sizeof(DM0L_PUT) + put_rec->put_rec_size))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)put_rec->put_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = put_rec->put_tid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = put_rec->put_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(put_rec->put_loc_cnt, 
				(i4)put_rec->put_tid.tid_tid.tid_page);
		i++;
	    }

	    *location_count = put_rec->put_loc_cnt;
	    *page_type =  put_rec->put_pg_type;
	    *page_size = put_rec->put_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LDEL:
	{
	    DM0L_DEL	*del_rec = (DM0L_DEL *)dmve->dmve_log_rec;

	    if (del_rec->del_header.length != 
		(sizeof(DM0L_DEL) + del_rec->del_rec_size))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)del_rec->del_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = del_rec->del_tid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = del_rec->del_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(del_rec->del_loc_cnt, 
				(i4)del_rec->del_tid.tid_tid.tid_page);
		i++;
	    }

	    *location_count = del_rec->del_loc_cnt;
	    *page_type = del_rec->del_pg_type;
	    *page_size = del_rec->del_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LREP:
	{
	    DM0L_REP	*rep_rec = (DM0L_REP *)dmve->dmve_log_rec;

	    if (rep_rec->rep_header.length != 
		(sizeof(DM0L_REP) + 
			rep_rec->rep_odata_len +
			rep_rec->rep_ndata_len))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)rep_rec->rep_ocnf_loc_id))
	    {
		info_array[i].loc_pg_num = rep_rec->rep_otid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = rep_rec->rep_ocnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(rep_rec->rep_loc_cnt, 
				(i4)rep_rec->rep_otid.tid_tid.tid_page);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)rep_rec->rep_ncnf_loc_id))
	    {
		info_array[i].loc_pg_num = rep_rec->rep_ntid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = rep_rec->rep_ncnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(rep_rec->rep_loc_cnt, 
				(i4)rep_rec->rep_ntid.tid_tid.tid_page);
		i++;
	    }

	    *location_count = rep_rec->rep_loc_cnt;
	    *page_type = rep_rec->rep_pg_type;
	    *page_size = rep_rec->rep_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LBI:
	{
	    DM0L_BI	*bi_rec = (DM0L_BI *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)bi_rec->bi_loc_id))
	    {
		info_array[i].loc_pg_num = bi_rec->bi_pageno;
		info_array[i].loc_cnf_id = bi_rec->bi_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(bi_rec->bi_loc_cnt, 
				(i4)bi_rec->bi_pageno);
		i++;
	    }

	    *location_count = bi_rec->bi_loc_cnt;
	    *page_type = bi_rec->bi_pg_type;
	    *page_size = bi_rec->bi_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LAI:
	{
	    DM0L_AI	*ai_rec = (DM0L_AI *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)ai_rec->ai_loc_id))
	    {
		info_array[i].loc_pg_num = ai_rec->ai_pageno;
		info_array[i].loc_cnf_id = ai_rec->ai_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(ai_rec->ai_loc_cnt, 
				(i4)ai_rec->ai_pageno);
		i++;
	    }

	    *location_count = ai_rec->ai_loc_cnt;
	    *page_type = ai_rec->ai_pg_type;
	    *page_size = ai_rec->ai_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LASSOC:
	{
	    DM0L_ASSOC	*assoc_rec = (DM0L_ASSOC *)dmve->dmve_log_rec;

	    if (assoc_rec->ass_header.length != sizeof(DM0L_ASSOC))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		status = E_DB_ERROR;
		break;
	    }

	    if (dmve_location_check(dmve, (i4)assoc_rec->ass_lcnf_loc_id))
	    {
		info_array[i].loc_pg_num = assoc_rec->ass_leaf_page;
		info_array[i].loc_cnf_id = assoc_rec->ass_lcnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(
				assoc_rec->ass_loc_cnt, 
				(i4)assoc_rec->ass_leaf_page);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)assoc_rec->ass_ocnf_loc_id))
	    {
		info_array[i].loc_pg_num = assoc_rec->ass_old_data;
		info_array[i].loc_cnf_id = assoc_rec->ass_ocnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(
				assoc_rec->ass_loc_cnt, 
				(i4)assoc_rec->ass_old_data);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)assoc_rec->ass_ncnf_loc_id))
	    {
		info_array[i].loc_pg_num = assoc_rec->ass_new_data;
		info_array[i].loc_cnf_id = assoc_rec->ass_ncnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(
				assoc_rec->ass_loc_cnt, 
				(i4)assoc_rec->ass_new_data);
		i++;
	    }

	    *location_count = assoc_rec->ass_loc_cnt;
	    *page_type = assoc_rec->ass_pg_type;
	    *page_size = assoc_rec->ass_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LALLOC:
	{
	    DM0L_ALLOC	*all_rec = (DM0L_ALLOC *)dmve->dmve_log_rec;

	    if (all_rec->all_header.length != sizeof(DM0L_ALLOC))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)all_rec->all_fhdr_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = all_rec->all_fhdr_pageno;
		info_array[i].loc_cnf_id = all_rec->all_fhdr_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(all_rec->all_loc_cnt, 
					(i4)all_rec->all_fhdr_pageno);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)all_rec->all_fmap_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = all_rec->all_fmap_pageno;
		info_array[i].loc_cnf_id = all_rec->all_fmap_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(all_rec->all_loc_cnt, 
					(i4)all_rec->all_fmap_pageno);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)all_rec->all_free_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = all_rec->all_free_pageno;
		info_array[i].loc_cnf_id = all_rec->all_free_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(all_rec->all_loc_cnt, 
					(i4)all_rec->all_free_pageno);
		i++;
	    }

	    *location_count = all_rec->all_loc_cnt;
	    *page_type = all_rec->all_pg_type;
	    *page_size = all_rec->all_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LDEALLOC:
	{
	    DM0L_DEALLOC    *dall_rec = (DM0L_DEALLOC *)dmve->dmve_log_rec;

	    if (dall_rec->dall_header.length != sizeof(DM0L_DEALLOC))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)dall_rec->dall_fhdr_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = dall_rec->dall_fhdr_pageno;
		info_array[i].loc_cnf_id = dall_rec->dall_fhdr_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(dall_rec->dall_loc_cnt, 
					(i4)dall_rec->dall_fhdr_pageno);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)dall_rec->dall_fmap_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = dall_rec->dall_fmap_pageno;
		info_array[i].loc_cnf_id = dall_rec->dall_fmap_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(dall_rec->dall_loc_cnt, 
					(i4)dall_rec->dall_fmap_pageno);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)dall_rec->dall_free_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = dall_rec->dall_free_pageno;
		info_array[i].loc_cnf_id = dall_rec->dall_free_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(dall_rec->dall_loc_cnt, 
					(i4)dall_rec->dall_free_pageno);
		i++;
	    }

	    *location_count = dall_rec->dall_loc_cnt;
	    *page_type = dall_rec->dall_pg_type;
	    *page_size = dall_rec->dall_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LEXTEND:
	{
	    DM0L_EXTEND	*ext_rec = (DM0L_EXTEND *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)ext_rec->ext_fhdr_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = ext_rec->ext_fhdr_pageno;
		info_array[i].loc_cnf_id = ext_rec->ext_fhdr_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(ext_rec->ext_loc_cnt, 
					(i4)ext_rec->ext_fhdr_pageno);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)ext_rec->ext_fmap_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = ext_rec->ext_fmap_pageno;
		info_array[i].loc_cnf_id = ext_rec->ext_fmap_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(ext_rec->ext_loc_cnt, 
					(i4)ext_rec->ext_fmap_pageno);
		i++;
	    }

	    /* 
	    ** if there is more than one location for this table then we
	    ** may not have the appropriate locations touched by this 
	    ** extend already open so find out what they are and note
	    ** that we need them
	    */
	    if (ext_rec->ext_loc_cnt > 1)
	    {
		i4	loc_old, loc_new;
		i4	num_extents, l_id;

		/* 
		** N.B. first new page number is ext_old_pages, 
		** last new page number is ext_new_pages - 1
		*/
	    	loc_old = DM2F_LOCID_MACRO(ext_rec->ext_loc_cnt, 
			    ext_rec->ext_old_pages);

		num_extents = (ext_rec->ext_new_pages - 
				ext_rec->ext_old_pages) / DI_EXTENT_SIZE;

		if (num_extents >= ext_rec->ext_loc_cnt)
		{
		    /* all locations included in extend */
		    loc_new = (loc_old + ext_rec->ext_loc_cnt - 1)
				% ext_rec->ext_loc_cnt;
		}
		else
		{
		    loc_new = DM2F_LOCID_MACRO(ext_rec->ext_loc_cnt, 
				(ext_rec->ext_new_pages - 1));
		}

		for (l_id = loc_old;;++l_id, l_id = l_id % ext_rec->ext_loc_cnt)
		{
	    	    if (dmve_location_check(dmve, 
			(i4)ext_rec->ext_cnf_loc_array[l_id]))
		    {
			/* 
			** this page number is only used to determine
			** later on if the location which we need open
			** already is so just make sure it stripes to it 
			** - the value has no other meaning for this
			** particular operation ( yuk )
			*/
		    	info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    	info_array[i].loc_cnf_id = 
				ext_rec->ext_cnf_loc_array[l_id];
		    	info_array[i].loc_id = l_id;
		    	i++;
		    }
		    if (l_id == loc_new)
			break;
		}
	    }

	    *location_count = ext_rec->ext_loc_cnt;
	    *page_type = ext_rec->ext_pg_type;
	    *page_size = ext_rec->ext_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LOVFL:
	{
	    DM0L_OVFL	*ovf_rec = (DM0L_OVFL *)dmve->dmve_log_rec;

	    if (ovf_rec->ovf_header.length != sizeof(DM0L_OVFL))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)ovf_rec->ovf_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = ovf_rec->ovf_page;
		info_array[i].loc_cnf_id = ovf_rec->ovf_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(ovf_rec->ovf_loc_cnt, 
				(i4)ovf_rec->ovf_page);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)ovf_rec->ovf_ovfl_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = ovf_rec->ovf_newpage;
		info_array[i].loc_cnf_id = ovf_rec->ovf_ovfl_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(ovf_rec->ovf_loc_cnt, 
				(i4)ovf_rec->ovf_newpage);
		i++;
	    }

	    *location_count = ovf_rec->ovf_loc_cnt;
	    *page_type = ovf_rec->ovf_pg_type;
	    *page_size = ovf_rec->ovf_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LNOFULL:
	{
	    DM0L_NOFULL	*nofull_rec = (DM0L_NOFULL *)dmve->dmve_log_rec;

	    if (nofull_rec->nofull_header.length != sizeof(DM0L_NOFULL))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)nofull_rec->nofull_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = nofull_rec->nofull_pageno;
		info_array[i].loc_cnf_id = nofull_rec->nofull_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(
				nofull_rec->nofull_loc_cnt, 
				(i4)nofull_rec->nofull_pageno);
		i++;
	    }

	    *location_count = nofull_rec->nofull_loc_cnt;
	    *page_type = nofull_rec->nofull_pg_type;
	    *page_size = nofull_rec->nofull_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LFMAP:
	{
	    DM0L_FMAP	*fmap_rec = (DM0L_FMAP *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)fmap_rec->fmap_fhdr_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = fmap_rec->fmap_fhdr_pageno;
		info_array[i].loc_cnf_id = fmap_rec->fmap_fhdr_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(fmap_rec->fmap_loc_cnt, 
					(i4)fmap_rec->fmap_fhdr_pageno);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)fmap_rec->fmap_fmap_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = fmap_rec->fmap_fmap_pageno;
		info_array[i].loc_cnf_id = fmap_rec->fmap_fmap_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(fmap_rec->fmap_loc_cnt, 
					(i4)fmap_rec->fmap_fmap_pageno);
		i++;
	    }

	    *location_count = fmap_rec->fmap_loc_cnt;
	    *page_type = fmap_rec->fmap_pg_type;
	    *page_size = fmap_rec->fmap_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LUFMAP:
	{
	    DM0L_UFMAP	*fmap_rec = (DM0L_UFMAP *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)fmap_rec->fmap_fhdr_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = fmap_rec->fmap_fhdr_pageno;
		info_array[i].loc_cnf_id = fmap_rec->fmap_fhdr_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(fmap_rec->fmap_loc_cnt, 
					(i4)fmap_rec->fmap_fhdr_pageno);
		i++;
	    }

	    if (dmve_location_check(dmve, (i4)fmap_rec->fmap_fmap_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = fmap_rec->fmap_fmap_pageno;
		info_array[i].loc_cnf_id = fmap_rec->fmap_fmap_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(fmap_rec->fmap_loc_cnt, 
					(i4)fmap_rec->fmap_fmap_pageno);
		i++;
	    }

	    *location_count = fmap_rec->fmap_loc_cnt;
	    *page_type = fmap_rec->fmap_pg_type;
	    *page_size = fmap_rec->fmap_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LBTPUT:
	{
	    DM0L_BTPUT	*put_rec = (DM0L_BTPUT *)dmve->dmve_log_rec;
	    i4	l_id; 
	    i4	put_loc_id = -1;

	    if (put_rec->btp_header.length != (sizeof(DM0L_BTPUT) - 
		(DM1B_MAXLEAFLEN - put_rec->btp_key_size)))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)put_rec->btp_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = put_rec->btp_bid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = put_rec->btp_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(put_rec->btp_loc_cnt, 
				(i4)put_rec->btp_bid.tid_tid.tid_page);
		put_loc_id = info_array[i].loc_id;
		i++;
	    }

	    if (dmve->dmve_action == DMVE_UNDO)
	    {
		/*
		** we may need additional locations which aren't
		** identified in the log record ( and we can't change
		** that right now ) so identify them and note that
		** we don't actually have the extent index at the
		** moment
		*/
	    	for (l_id = 0; l_id < put_rec->btp_loc_cnt; l_id++)
	    	{
		    if (l_id == put_loc_id)
			continue;

		    info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    info_array[i].loc_cnf_id = -1;
		    info_array[i].loc_id = l_id;
		    i++;
	    	}
	    }

	    *location_count = put_rec->btp_loc_cnt;
	    *page_type = put_rec->btp_pg_type;
	    *page_size = put_rec->btp_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LBTDEL:
	{
	    DM0L_BTDEL	*del_rec = (DM0L_BTDEL *)dmve->dmve_log_rec;
	    i4	l_id; 
	    i4	del_loc_id = -1;

	    if (del_rec->btd_header.length != (sizeof(DM0L_BTDEL) - 
		(DM1B_MAXLEAFLEN - del_rec->btd_key_size)))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)del_rec->btd_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = del_rec->btd_bid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = del_rec->btd_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(del_rec->btd_loc_cnt, 
				(i4)del_rec->btd_bid.tid_tid.tid_page);
		del_loc_id = info_array[i].loc_id;
		i++;
	    }

	    if (dmve->dmve_action == DMVE_UNDO)
	    {
		/*
		** we may need additional locations which aren't
		** identified in the log record ( and we can't change
		** that right now ) so identify them and note that
		** we don't actually have the extent index at the
		** moment
		*/
	    	for (l_id = 0; l_id < del_rec->btd_loc_cnt; l_id++)
	    	{
		    if (l_id == del_loc_id)
			continue;

		    info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    info_array[i].loc_cnf_id = -1;
		    info_array[i].loc_id = l_id;
		    i++;
	    	}
	    }

	    *location_count = del_rec->btd_loc_cnt;
	    *page_type = del_rec->btd_pg_type;
	    *page_size = del_rec->btd_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LBTSPLIT:
	{
	    DM0L_BTSPLIT  *spl_rec = (DM0L_BTSPLIT *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)spl_rec->spl_cur_loc_id))
	    {
		info_array[i].loc_pg_num = spl_rec->spl_cur_pageno;
		info_array[i].loc_cnf_id = spl_rec->spl_cur_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(spl_rec->spl_loc_cnt, 
				(i4)spl_rec->spl_cur_pageno);
		i++;
	    }
	    if (dmve_location_check(dmve, (i4)spl_rec->spl_sib_loc_id))
	    {
		info_array[i].loc_pg_num = spl_rec->spl_sib_pageno;
		info_array[i].loc_cnf_id = spl_rec->spl_sib_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(spl_rec->spl_loc_cnt, 
				(i4)spl_rec->spl_sib_pageno);
		i++;
	    }
	    if ((spl_rec->spl_dat_pageno) &&
		(dmve_location_check(dmve, (i4)spl_rec->spl_dat_loc_id)))
	    {
		info_array[i].loc_pg_num = spl_rec->spl_dat_pageno;
		info_array[i].loc_cnf_id = spl_rec->spl_dat_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(spl_rec->spl_loc_cnt, 
				(i4)spl_rec->spl_dat_pageno);
		i++;
	    }

	    *location_count = spl_rec->spl_loc_cnt;
	    *page_type = spl_rec->spl_pg_type;
	    *page_size = spl_rec->spl_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LBTOVFL:
	{
	    DM0L_BTOVFL  *bto_rec = (DM0L_BTOVFL *)dmve->dmve_log_rec;
	    i4	l_id; 
	    i4	leaf_loc_id = -1;
	    i4	ovfl_loc_id = -1;

	    if (dmve_location_check(dmve, (i4)bto_rec->bto_leaf_loc_id))
	    {
		info_array[i].loc_pg_num = bto_rec->bto_leaf_pageno;
		info_array[i].loc_cnf_id = bto_rec->bto_leaf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(bto_rec->bto_loc_cnt, 
				(i4)bto_rec->bto_leaf_pageno);
		leaf_loc_id = info_array[i].loc_id;
		i++;
	    }
	    if (dmve_location_check(dmve, (i4)bto_rec->bto_ovfl_loc_id))
	    {
		info_array[i].loc_pg_num = bto_rec->bto_ovfl_pageno;
		info_array[i].loc_cnf_id = bto_rec->bto_ovfl_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(bto_rec->bto_loc_cnt, 
				(i4)bto_rec->bto_ovfl_pageno);
		ovfl_loc_id = info_array[i].loc_id;
		i++;
	    }

	    if (dmve->dmve_action == DMVE_UNDO)
	    {
		/*
		** we may need additional locations which aren't
		** identified in the log record ( and we can't change
		** that right now ) so identify them and note that
		** we don't actually have the extent index at the
		** moment
		*/
	    	for (l_id = 0; l_id < bto_rec->bto_loc_cnt; l_id++)
	    	{
		    if ((l_id == leaf_loc_id) || (l_id == ovfl_loc_id))
			continue;

		    info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    info_array[i].loc_cnf_id = -1;
		    info_array[i].loc_id = l_id;
		    i++;
	    	}
	    }

	    *location_count = bto_rec->bto_loc_cnt;
	    *page_type = bto_rec->bto_pg_type;
	    *page_size = bto_rec->bto_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LBTFREE:
	{
	    DM0L_BTFREE  *btf_rec = (DM0L_BTFREE *)dmve->dmve_log_rec;
	    i4	l_id; 
	    i4	prev_loc_id = -1;
	    i4	ovfl_loc_id = -1;

	    if (dmve_location_check(dmve, (i4)btf_rec->btf_prev_loc_id))
	    {
		info_array[i].loc_pg_num = btf_rec->btf_prev_pageno;
		info_array[i].loc_cnf_id = btf_rec->btf_prev_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(btf_rec->btf_loc_cnt, 
				(i4)btf_rec->btf_prev_pageno);
		prev_loc_id = info_array[i].loc_id;
		i++;
	    }
	    if (dmve_location_check(dmve, (i4)btf_rec->btf_ovfl_loc_id))
	    {
		info_array[i].loc_pg_num = btf_rec->btf_ovfl_pageno;
		info_array[i].loc_cnf_id = btf_rec->btf_ovfl_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(btf_rec->btf_loc_cnt, 
				(i4)btf_rec->btf_ovfl_pageno);
		ovfl_loc_id = info_array[i].loc_id;
		i++;
	    }

	    if (dmve->dmve_action == DMVE_UNDO)
	    {
		/*
		** we may need additional locations which aren't
		** identified in the log record ( and we can't change
		** that right now ) so identify them and note that
		** we don't actually have the extent index at the
		** moment
		*/
	    	for (l_id = 0; l_id < btf_rec->btf_loc_cnt; l_id++)
	    	{
		    if ((l_id == prev_loc_id) || (l_id == ovfl_loc_id))
			continue;

		    info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    info_array[i].loc_cnf_id = -1;
		    info_array[i].loc_id = l_id;
		    i++;
	    	}
	    }

	    *location_count = btf_rec->btf_loc_cnt;
	    *page_type = btf_rec->btf_pg_type;
	    *page_size = btf_rec->btf_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LRTPUT:
	{
	    DM0L_RTPUT	*put_rec = (DM0L_RTPUT *)dmve->dmve_log_rec;
	    i4	l_id; 
	    i4	put_loc_id = -1;

	    if (put_rec->rtp_header.length !=  (i4)(sizeof(DM0L_RTPUT) - 
		(DB_MAXRTREE_KEY - put_rec->rtp_key_size) -
		(RCB_MAX_RTREE_LEVEL * sizeof(DM_TID) - 
		put_rec->rtp_stack_size)))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)put_rec->rtp_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = put_rec->rtp_bid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = put_rec->rtp_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(put_rec->rtp_loc_cnt, 
				(i4)put_rec->rtp_bid.tid_tid.tid_page);
		put_loc_id = info_array[i].loc_id;
		i++;
	    }

	    if (dmve->dmve_action == DMVE_UNDO)
	    {
		/*
		** we may need additional locations which aren't
		** identified in the log record ( and we can't change
		** that right now ) so identify them and note that
		** we don't actually have the extent index at the
		** moment
		*/
	    	for (l_id = 0; l_id < put_rec->rtp_loc_cnt; l_id++)
	    	{
		    if (l_id == put_loc_id)
			continue;

		    info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    info_array[i].loc_cnf_id = -1;
		    info_array[i].loc_id = l_id;
		    i++;
	    	}
	    }

	    *location_count = put_rec->rtp_loc_cnt;
	    *page_type = put_rec->rtp_pg_type;
	    *page_size = put_rec->rtp_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LRTDEL:
	{
	    DM0L_RTDEL	*del_rec = (DM0L_RTDEL *)dmve->dmve_log_rec;
	    i4	l_id; 
	    i4	del_loc_id = -1;

	    if (del_rec->rtd_header.length != (sizeof(DM0L_RTDEL) - 
		(RCB_MAX_RTREE_LEVEL * sizeof(DM_TID) -
		del_rec->rtd_stack_size) -
		(DB_MAXRTREE_KEY - del_rec->rtd_key_size)))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)del_rec->rtd_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = del_rec->rtd_bid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = del_rec->rtd_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(del_rec->rtd_loc_cnt, 
				(i4)del_rec->rtd_bid.tid_tid.tid_page);
		del_loc_id = info_array[i].loc_id;
		i++;
	    }

	    if (dmve->dmve_action == DMVE_UNDO)
	    {
		/*
		** we may need additional locations which aren't
		** identified in the log record ( and we can't change
		** that right now ) so identify them and note that
		** we don't actually have the extent index at the
		** moment
		*/
	    	for (l_id = 0; l_id < del_rec->rtd_loc_cnt; l_id++)
	    	{
		    if (l_id == del_loc_id)
			continue;

		    info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    info_array[i].loc_cnf_id = -1;
		    info_array[i].loc_id = l_id;
		    i++;
	    	}
	    }

	    *location_count = del_rec->rtd_loc_cnt;
	    *page_type = del_rec->rtd_pg_type;
	    *page_size = del_rec->rtd_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LRTREP:
	{
	    DM0L_RTREP	*rep_rec = (DM0L_RTREP *)dmve->dmve_log_rec;
	    i4	l_id; 
	    i4	rep_loc_id = -1;

	    if (rep_rec->rtr_header.length != (i4) (sizeof(DM0L_RTREP) - 
		(DB_MAXRTREE_KEY - rep_rec->rtr_okey_size) -
		(DB_MAXRTREE_KEY - rep_rec->rtr_nkey_size) -
		(RCB_MAX_RTREE_LEVEL * sizeof(DM_TID) - rep_rec->rtr_stack_size)))
	    {
		SETDBERR(&dmve->dmve_error, 0, E_DM9601_DMVE_BAD_PARAMETER);
		break;
	    }

	    if (dmve_location_check(dmve, (i4)rep_rec->rtr_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = rep_rec->rtr_bid.tid_tid.tid_page;
		info_array[i].loc_cnf_id = rep_rec->rtr_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(rep_rec->rtr_loc_cnt, 
				(i4)rep_rec->rtr_bid.tid_tid.tid_page);
		rep_loc_id = info_array[i].loc_id;
		i++;
	    }

	    if (dmve->dmve_action == DMVE_UNDO)
	    {
		/*
		** we may need additional locations which aren't
		** identified in the log record ( and we can't change
		** that right now ) so identify them and note that
		** we don't actually have the extent index at the
		** moment
		*/
	    	for (l_id = 0; l_id < rep_rec->rtr_loc_cnt; l_id++)
	    	{
		    if (l_id == rep_loc_id)
			continue;

		    info_array[i].loc_pg_num = l_id * DI_EXTENT_SIZE;
		    info_array[i].loc_cnf_id = -1;
		    info_array[i].loc_id = l_id;
		    i++;
	    	}
	    }

	    *location_count = rep_rec->rtr_loc_cnt;
	    *page_type = rep_rec->rtr_pg_type;
	    *page_size = rep_rec->rtr_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LBTUPDOVFL:
	{
	    DM0L_BTUPDOVFL  *btu_rec = (DM0L_BTUPDOVFL *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)btu_rec->btu_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = btu_rec->btu_pageno;
		info_array[i].loc_cnf_id = btu_rec->btu_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(btu_rec->btu_loc_cnt, 
				(i4)btu_rec->btu_pageno);
		i++;
	    }

	    *location_count = btu_rec->btu_loc_cnt;
	    *page_type = btu_rec->btu_pg_type;
	    *page_size = btu_rec->btu_page_size;
	    *num_entries = i;
	}
	break;

	case DM0LDISASSOC:
	{
	    DM0L_DISASSOC  *dis_rec = (DM0L_DISASSOC *)dmve->dmve_log_rec;

	    if (dmve_location_check(dmve, (i4)dis_rec->dis_cnf_loc_id))
	    {
		info_array[i].loc_pg_num = dis_rec->dis_pageno;
		info_array[i].loc_cnf_id = dis_rec->dis_cnf_loc_id;
		info_array[i].loc_id = DM2F_LOCID_MACRO(dis_rec->dis_loc_cnt, 
				(i4)dis_rec->dis_pageno);
		i++;
	    }

	    *location_count = dis_rec->dis_loc_cnt;
	    *page_type = dis_rec->dis_pg_type;
	    *page_size = dis_rec->dis_page_size;
	    *num_entries = i;
	}
	break;

      default:
	status = E_DB_ERROR;
	SETDBERR(&dmve->dmve_error, 0, E_DM9664_DMVE_INFO_UNKNOWN);
	break;
    }

    /*
    ** If no locations were found in this log record that need to be
    ** recovered, then return a warning to indicate that no recovery 
    ** processing is required.
    */
    if ((status == E_DB_OK) && (i == 0))
	status = E_DB_WARN;

    return (status);
}

/*
** Name: dmve_trace_page_info - Write trace information about page used in
**				recovery processing.
**
** Description:
**	Used for debug tracing, this routine formats page information about
**	pages read during recovery processing.
**
**	It is driven by the following DMF trace points:
**
**	DM1306	- Trace Page LSN's
**	DM1310	- Trace page header information
**	DM1311	- Give full hex dump of page
**
**	If a NULL page pointer is given, then a trace line will be printed
**	which indicates that recovery is bypassed on the page.
**
** Inputs:
**	page_type	- page type 
**	page_size	- page size
**	page		- Pointer to page to trace.
**	plv		- Page level accessors.
**	type		- Ascii string which briefly describes page type.
**
** Outputs:
**	Returns:
**	    none
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	26-jul-1993 (rogerk)
**	    Written
**	23-aug-1993 (rogerk)
**	    Changed enabling tracepoint to DM1315 from DM1306 so that 
**	    DBMS servers would not unintentionally trace pages when the 
**	    installation defines DM1306 to get RCP tracing.  The RCP
**	    automatically sets DM1315 in its own process if II_DMF_TRACE
**	    specifies DM1306.
**	18-oct-1993 (jnash)
**	    Move code previously in this routine into new 
**	    dmve_trace_page_lsn(), dmve_trace_header() and  
**	    dmve_trace_page_contents() functions to make them independently 
**	    callable.
**	06-may-1996 (thaju02)
**	    New page format support: added page_size to dmve_trace_page_lsn
**	    call.
*/
VOID
dmve_trace_page_info(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
DMPP_ACC_PLV 	*plv,
char		*type)
{
    if ( !page || ! DMZ_ASY_MACRO(15))
	return;

    dmve_trace_page_lsn(page_type, page_size, page, plv, type);

    if (DMZ_ASY_MACRO(10))
	dmve_trace_page_header(page_type, page_size, page, plv, type);

    if (DMZ_ASY_MACRO(11))
	dmve_trace_page_contents(page_type, page_size, page, type);

    return;
}

/*
** Name: rfp_add_action - Add action item to rollforward action list.
**
** Description:
**	This routine adds a rollforward event to the action list.  This
**	list describes work done by recovery processing in rollforward that
**	is needed to be remembered so that appropriate actions can be taken
**	later.
**
**	Actions that are currently added to this list:
**
**	    FRENAME operations that rename a file to a delete file name.
**		These operations are added so we can remember to remove the
**		physical file when the End Transaction record is found.
**
**	    DELETE operations on system catalog rows corresponding to non
**		journaled tables.  These operations are added so that if/when
**		undo of such an action is required (or a CLR of such an action
**		is encountered) we can tell whether or not to restore the
**		deleted row.  See non-journaled table handling notes in DMFRFP.
**
**	    REPLACE operations on iirelation rows corresponding to non
**		journaled tables.  These operations are added so that if/when
**		an undo of such an action is required we can tell what items
**		of the iirelation row need to be restored.  See non-journaled
**		table handling notes in DMFRFP.
**
**	An ADD action call (this routine) takes only the Transaction id, LSN,
**	and action type as parameters and returns a pointer to the allocated
**	action entry in which the caller can fill in more information.
**
** Inputs:
**	dmve		- recovery context
**	xid		- Transaction id
**	lsn		- LSN of record being recovered
**	type		- action type
**
** Outputs:
**	item		- action list item
**
** Returns:
**	E_DB_OK
**	E_DB_ERROR
**
** History:
**	20-sep-1993 (rogerk)
**	    Created in place of old pend_et_action routine.
**	25-Apr-2007 (kibro01) b115996
**	    Ensure the misc_data pointer in a MISC_CB is set correctly and
**	    that we correctly allocate sizeof(DMP_MISC), not sizeof(DM_OBJECT)
**	    since that's what we're actually using (the type is MISC_CB)
*/
DB_STATUS
rfp_add_action(
DMVE_CB		    *dmve,
DB_TRAN_ID	    *xid,
LG_LSN		    *lsn,
i4		    type,
RFP_REC_ACTION	    **item)
{
    RFP_REC_ACTION	*act_ptr;
    DM_OBJECT		*mem_ptr;
    i4		mem_needed;
    DB_STATUS		status;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    for (;;)
    {
	mem_needed = sizeof(RFP_REC_ACTION) + sizeof(DMP_MISC);
	status = dm0m_allocate(mem_needed, (i4)0, (i4)MISC_CB, 
	    (i4)MISC_ASCII_ID, (char *)NULL, &mem_ptr, &dmve->dmve_error);
	if (status != OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, err_code, 0);
	    uleFormat(NULL, E_DM9425_MEMORY_ALLOCATE, (CL_ERR_DESC *)NULL, ULE_LOG, 
		NULL, (char *)NULL, (i4)0, (i4 *)NULL, err_code, 1,
		0, mem_needed);
	    break;
	}
	act_ptr = (RFP_REC_ACTION *)((char *)mem_ptr + sizeof(DMP_MISC));
	((DMP_MISC*)mem_ptr)->misc_data = (char*)act_ptr;

	STRUCT_ASSIGN_MACRO(*xid, act_ptr->rfp_act_xid);
	STRUCT_ASSIGN_MACRO(*lsn, act_ptr->rfp_act_lsn);
	act_ptr->rfp_act_action = type;

	/*
	** Put event on the rollforward action list.
	*/
	act_ptr->rfp_act_next = dmve->dmve_rolldb_action;
	act_ptr->rfp_act_prev = 0;

	if (dmve->dmve_rolldb_action)
	    dmve->dmve_rolldb_action->rfp_act_prev = act_ptr;
	dmve->dmve_rolldb_action = act_ptr;

	*item = act_ptr;
	return (E_DB_OK);
    }

    SETDBERR(&dmve->dmve_error, 0, E_DM134B_RFP_ACTION_LIST);
    return (E_DB_ERROR);
}

/*
** Name: rfp_lookup_action - Lookup particular action on the action list.
**
** Description:
**	This routine searches through the rollforward action list looking
**	for events belonging to a particular transaction.
**
**	The routine will search for any event with the specified XID or
**	for a specific event searching by LSN.
**
**	If the RFP_REC_NEXT flag is specified then the search will begin
**	searching the list after the indicated item.  This allows the
**	caller to iteratively call the routine to find all instances of
**	actions by a given transaction.
**	
** Inputs:
**	dmve		- recovery context
**	xid		- Transaction id
**	flag		- Type of search:
**			    RFP_REC_LSN	    - search for specific LSN
**			    RFP_REC_NEXT    - start search at given item
**	lsn		- LSN of record requested (if mode is RFP_REC_LSN)
**	item		- Pointer to last item returned if mode is RFP_REC_NEXT
**
** Outputs:
**	item		- action list item - null if none found
**
** Returns:
**	none
**
** History:
**	20-sep-1993 (rogerk)
**	    Created.
*/
VOID
rfp_lookup_action(
DMVE_CB		    *dmve,
DB_TRAN_ID	    *xid,
LG_LSN		    *lsn,
i4		    flag,
RFP_REC_ACTION	    **item)
{
    RFP_REC_ACTION	*act_ptr;

    /*
    ** Starting place for search is either the head of the list or the
    ** last item returned.
    */
    act_ptr = dmve->dmve_rolldb_action;
    if ((flag & RFP_REC_NEXT) && (*item))
	act_ptr = (*item)->rfp_act_next;

    *item = NULL;
    while (act_ptr != NULL)
    {
	/*
	** Return action entry if it is from the correct xid and either
	** all entries are being returned or it matches the given LSN.
	*/
	if ((act_ptr->rfp_act_xid.db_high_tran == xid->db_high_tran) &&
	    (act_ptr->rfp_act_xid.db_low_tran  == xid->db_low_tran) &&
	    (((flag & RFP_REC_LSN) == 0) || 
		(LSN_EQ(&act_ptr->rfp_act_lsn, lsn))))
	{
	    break;
	}

	act_ptr = act_ptr->rfp_act_next;
    }

    *item = act_ptr;
    return;
}

/*
** Name: rfp_remove_action - Remove action from the action list.
**
** Description:
**	This routine removes an action list entry from the rollforward
**	action list.
**
**	Presumably the given action was found by rfp_lookup_action.
**
** Inputs:
**	dmve		- recovery context
**	item		- item to remove
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	20-sep-1993 (rogerk)
**	    Created.
*/
VOID
rfp_remove_action(
DMVE_CB		    *dmve,
RFP_REC_ACTION	    *item)
{
    RFP_REC_ACTION	*act_ptr;

    act_ptr = item;

    if (act_ptr->rfp_act_next)
	act_ptr->rfp_act_next->rfp_act_prev = act_ptr->rfp_act_prev;

    if (act_ptr->rfp_act_prev)
	act_ptr->rfp_act_prev->rfp_act_next = act_ptr->rfp_act_next;
    else
	dmve->dmve_rolldb_action = act_ptr->rfp_act_next;

    act_ptr = (RFP_REC_ACTION *)((char *)act_ptr - sizeof(DMP_MISC));
    dm0m_deallocate((DM_OBJECT **)&act_ptr);
}

/*
** Name: dmve_trace_page_lsn - Display page lsn information
**
** Description:
**	This routine displays page lsn information.
**
** Inputs:
**	page_type	- page type 
**	page_size	- page size
**	page		- Pointer to page 
**	plv		- page level vector
**	type		- type of page
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	18-oct-1993 (jnash)
**	    Split out from dmve_trace_page_info().
**	06-may-1996 (thaju02)
**	    New page format support.  Added page_size calling parameter.
**	    Change page header references to use macros.
*/
VOID
dmve_trace_page_lsn(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
DMPP_ACC_PLV 	*plv,
char		*type)
{
    if (page == NULL)
    {
	TRdisplay("\t%s Not Recovered\n", type);
	return;
    }

    TRdisplay("\t%s LSN: (%x,%x)\n", type,
	DMPP_VPT_GET_LOG_ADDR_HIGH_MACRO(page_type, page), 
	DMPP_VPT_GET_LOG_ADDR_LOW_MACRO(page_type, page));

    if (DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	(DMPP_LEAF | DMPP_INDEX | DMPP_CHAIN))
    {
	TRdisplay("\tSplit LSN: (%x,%x)\n", type,
	    DM1B_VPT_GET_SPLIT_LSN_HIGH_MACRO(page_type, page), 
	    DM1B_VPT_GET_SPLIT_LSN_LOW_MACRO(page_type, page));
    }

    return;
}

/*
** Name: dmve_trace_page_header - Display page lsn information
**
** Description:
**	This routine displays page header information.
**
** Inputs:
**	page_type	- page type 
**	page_size	- page size
**	page		- Pointer to page 
**	plv		- page level vector
**	type		- type of page
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	18-oct-1993 (jnash)
**	    Split out from dmve_trace_page_info().
**	06-may-1996 (thaju02 & nanpr01)
**	    New page format support: change page header references to macros.
**	19-Jan-2004 (jenjo02)
**	    Added partition number to display in the case of
**	    global index leaf pages.
**	15-Jan-2010 (jonj)
**	    Use new PAGE_STAT define to get page_stat bit names.
*/
VOID
dmve_trace_page_header(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
DMPP_ACC_PLV 	*plv,
char 		*type)
{
    DM_TID		tid;
    char		*key_ptr;
    i4		offset;
    i4		i;
    i4		partno;

    if (page == NULL)
    {
	TRdisplay("\t%s Not Recovered\n", type);
	return;
    }

    TRdisplay("\tPage Type: %v\n",
        PAGE_STAT, DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, page));
    TRdisplay("\tPage Number: %d, Main Page: %d, Ovfl Page: %d\n",
	DMPP_VPT_GET_PAGE_PAGE_MACRO(page_type, page), 
	DMPP_VPT_GET_PAGE_MAIN_MACRO(page_type, page), 
	DMPP_VPT_GET_PAGE_OVFL_MACRO(page_type, page));
    TRdisplay("\tPage TranId (%x%x), Sequence Number: %d\n", 
	DMPP_VPT_GET_TRAN_ID_HIGH_MACRO(page_type, page), 
	DMPP_VPT_GET_TRAN_ID_LOW_MACRO(page_type, page),
	DMPP_VPT_GET_PAGE_SEQNO_MACRO(page_type, page));

    if (DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, page) & (DMPP_DATA))
    {
	TRdisplay("\tLine Table size: %d\n", 
	    DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(page_type, page));
	TRdisplay("\tLine Table contents:\n");

	for (i = 0; i < 
	    (i4) DMPP_VPT_GET_PAGE_NEXT_LINE_MACRO(page_type, page); i++)
	{
	    offset = (*plv->dmpp_get_offset)(page_type, page_size, page, i);
	    TRdisplay("\t    [%d]: Offset %d\n", i, offset);
	}
    }
    else if (DMPP_VPT_GET_PAGE_STAT_MACRO(page_type, page) & 
	(DMPP_LEAF | DMPP_INDEX | DMPP_CHAIN))
    {
	TRdisplay("\tChildren: %d, NextPage: %d, DataPage: %d\n", 
	    DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page), 
	    DM1B_VPT_GET_BT_NEXTPAGE_MACRO(page_type, page), 
	    DM1B_VPT_GET_BT_DATA_MACRO(page_type, page));

	TRdisplay("\tPage contents:\n");
	for (i = 0; i < DM1B_VPT_GET_BT_KIDS_MACRO(page_type, page); i++)
	{
	    key_ptr = 
		(char *)dm1bvpt_keyof_macro(page_type, page, i);
	    dm1bvpt_kidget_macro(page_type, page, i, &tid, &partno);

	    TRdisplay("\t    [%d]: Key_offset %d, TID [%d,%d], Partno %d\n",
		i, (key_ptr - (char *)page), 
		tid.tid_tid.tid_page, tid.tid_tid.tid_line, partno);
	}
    }

    return;
}

/*
** Name: dmve_trace_page_contents - Display page contents 
**
** Description:
**	This routine displays page contents 
**
** Inputs:
**	page_type	- page type 
**	page_size	- page size
**	page		- Pointer to page 
**	type		- type of page
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	18-oct-1993 (jnash)
**	    Split out from dmve_trace_page_info().
**	10-nov-93 (tad)
**	    Bug #56449
**	    Replaced %x with %p for pointer values where necessary in
**	    dmve_trace_page_contents().
**	28-aug-97 (nanpr01)
**	    added page_size as parameter.
*/
VOID
dmve_trace_page_contents(
i4		page_type,
i4		page_size,
DMPP_PAGE	*page,
char 		*type)
{
    i4		i;

    if (page == NULL)
    {
	TRdisplay("\t%s Not Recovered\n", type);
	return;
    }

    TRdisplay("\t_________________ START OF PAGE DUMP _______________\n");
    for (i = 0; i < page_size; i += 16)
	TRdisplay("\t   0x%p:%< %4.4{%x%}%2< >%16.1{%c%}<\n",
		    ((char *)page) + i, 0);
    TRdisplay("\t__________________ END OF PAGE DUMP ________________\n");

    return;
}

/*
** Name: dmve_unreserve_space - Unreserve log blocks reserved for rollback.
**
** Description:
**	This routine is called during undo recovery by those operations
**	which pre-reserve logspace for Log File forces that their undo
**	protocols require.
**
**	During rollback, this routine calls LG to decrement the transaction's
**	reserved space counter to show that the log space for the force is
**	no longer needed.
**
**	If an error is encountered it is logged, but will not be returned.
**	The caller should continue its rollback in the hope that the space
**	unreservation problem will not affect the system.
**
** Inputs:
**	dmve		- recovery context
**	log_blocks	- count of log buffers reserved
**
** Outputs:
**	none
**
** Returns:
**	none
**
** History:
**	18-oct-1993 (rogerk)
**	    Created.
*/
VOID
dmve_unreserve_space(
DMVE_CB		*dmve,
i4		log_blocks)
{
    i4		    i;
    i4	    error;
    CL_ERR_DESC	    sys_err;
    STATUS	    cl_status;

    /*
    ** Call LG to unreserve each logblock reserved for this operation.
    */
    for (i = 0; i < log_blocks; i++)
    {
	cl_status = LGalter(LG_A_UNRESERVE_SPACE, 
		(PTR)&dmve->dmve_log_id, sizeof(dmve->dmve_log_id), &sys_err);
	if (cl_status != OK)
	{
	    uleFormat(NULL, cl_status, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4) 0, (i4 *)NULL, &error, 0); 
	    uleFormat(NULL, E_DM900B_BAD_LOG_ALTER, &sys_err, ULE_LOG, NULL,
		(char *)NULL, (i4) 0, (i4 *)NULL, &error, 1, 0, 
		LG_A_UNRESERVE_SPACE);
	    uleFormat(NULL, E_DM968B_DMVE_UNRESERVE, (CL_ERR_DESC *)NULL, ULE_LOG, NULL,
		(char *)NULL, (i4) 0, (i4 *)NULL, &error, 0);
	    break;
	}
    }

    return;
}
/*
** Name: dmve_get_iirel_row - Retrieve a row from iirelation for a given table.
**
** Description:
**
**	This routine is used to obtain the current DMP_RELATION values for
**      a given table in an iirelation row.
**
**	This routine can be used to obtain the current iirelation
**      values for tuples which describe non-journalled tables.
**      The caller can then mask the current values into the
**      log record values before applying the journal entry.
**      Refer to the DM0L_NONJNL_TAB and DM0L_TEMP_IIRELATION
**      flags.
**
**	If an error is encountered it is logged and returned to the caller.
**
** Inputs:
**	dmve            - recovery context
**	rel_id		- Table identifier.
**
** Outputs:
**	rel_row		- Retrieved row.
**
** Returns:
**	status		- E_DB_OK if row found.
**	      		- E_DB_WARN if row not found.
**	      		- Or underlying error status.
** History:
**	30-Mar-2004 (hanal04) Bug 107828 INGSRV2701
**	    Created.
**	11-Apr-2008 (kschendel)
**	    dm2r position call updated, fix here.
*/
DB_STATUS
dmve_get_iirel_row(
DMVE_CB         *dmve,
DB_TAB_ID	*rel_id,
DMP_RELATION    *rel_row)
{
    DB_STATUS		status, close_status;
    DB_TAB_ID		table_id;
    DMP_RCB		*rel_rcb = 0;
    i4			error;
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DB_TAB_TIMESTAMP	timestamp;
    DM2R_KEY_DESC	key_desc[2];
    DM_TID		tid;
    DB_ERROR		local_dberr;

    CLRDBERR(&dmve->dmve_error);

    table_id.db_tab_base = DM_B_RELATION_TAB_ID;
    table_id.db_tab_index = DM_I_RELATION_TAB_ID;

    for(;;)
    {
        status = dm2t_open(dcb, &table_id, DM2T_IS,
            DM2T_UDIRECT, DM2T_A_READ, (i4)0, (i4)20,
            0, 0, dmve->dmve_lk_id, (i4)0,(i4)0,
            0, &dmve->dmve_tran_id, &timestamp, &rel_rcb, (DML_SCB *)NULL, 
            &dmve->dmve_error);
        if (status != E_DB_OK)
            break;

        /*  Position table using base table id. */
        key_desc[0].attr_operator = DM2R_EQ;
        key_desc[0].attr_number = DM_1_RELATION_KEY;
        key_desc[0].attr_value = (char *) &rel_id->db_tab_base;
 
        status = dm2r_position(rel_rcb, DM2R_QUAL, key_desc, (i4)1,
                          (DM_TID *)NULL, &dmve->dmve_error);
        if (status != E_DB_OK)
            break;

        for (;;)
        {
 
            /*
            ** Get a qualifying relation record.  This will retrieve
            ** the record for a table.
            */
 
            status = dm2r_get(rel_rcb, &tid, DM2R_GETNEXT,
                              (char *)rel_row, &dmve->dmve_error);
			      
            if (status != E_DB_OK)
	    {
                if(dmve->dmve_error.err_code == E_DM0055_NONEXT)
                {
                    status = E_DB_WARN;
                }
                break;
            }
 
            if (rel_row->reltid.db_tab_base != rel_id->db_tab_base)
                continue;
            if (rel_row->reltid.db_tab_index != rel_id->db_tab_index)
                continue;

            /*
            ** Found the relation record.
            */
            break;

        } /* End query loop */

	break;
    } /* end for */

    if (DB_FAILURE_MACRO(status))
    {
        uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, (char *)NULL,
            (i4)0, (i4 *)NULL, &error, 0);
        uleFormat(NULL, E_DM9690_DMVE_IIREL_GET, (CL_ERR_DESC *)NULL, ULE_LOG,
            NULL, (char *)NULL, (i4)0, (i4 *)NULL, &error, 1,
            sizeof(DB_DB_NAME), &dcb->dcb_name);
    }

    if (rel_rcb)
    {
        close_status = dm2t_close(rel_rcb, DM2T_NOPURGE, &local_dberr);
        if (close_status != E_DB_OK)
        {
            uleFormat(&local_dberr, 0, NULL, ULE_LOG, NULL, (char *)NULL, 0L,
                (i4 *)NULL, &error, 0);
            if(status == E_DB_OK)
	    {
		status = close_status;
	    }
        }
    }

    return(status);
}

/*
** Name: dmve_get_tabinfo - Get table id,name,owner from DMVE_CB
**                          Called from rollforwarddb and rcp offline recovery
**                          ... after a dmve error.
** 
** Description:
**      This routine is called after a dmve error to get the 
**      table id, name and owner associated with the last dmve call.
**      It gets the table id from the log record. (dmve->dmve_log_rec)
**      The table name,owner are present in some log records, 
**      or can be found in the DMVE_CB ...
**      ... if the dmve error happened after dmve_fix_tbio was called.
**
**      Note tablename, owner is not strictly required for dmve error handling.
**      (error handling mostly works off of tabid which is always in the logrec)
**
** Inputs:
**      dmve		- the dmve cb
**
** Outputs:
**      None
**
** Returns:
**      tabid
**      tabname
**      tabowner
**
** History:
**	07-apr-2005 (stial01)
**	    Created.
**      23-Feb-2009 (hanal04) Bug 121652
**          Added DM0LUFMAP case to deal with FMAP updates needed for an
**          extend operation where a new FMAP did not reside in the last
**          FMAP or new FMAP itself.
*/
VOID
dmve_get_tabinfo(
DMVE_CB		    *dmve,
DB_TAB_ID	    *tabid,
DB_TAB_NAME	    *tabname,
DB_OWN_NAME	    *ownname)
{
    DMP_DCB	    *dcb = dmve->dmve_dcb_ptr;
    DM0L_HEADER     *h = (DM0L_HEADER *)dmve->dmve_log_rec;
    DB_TAB_ID	    *log_tabid;
    DB_TAB_NAME     *log_tabname;
    DB_OWN_NAME     *log_ownname;
    char	    tnamebuf[DB_TAB_MAXNAME + 1];
    DB_ERROR	    local_dberr;
    DB_TAB_NAME     relid;
    DB_OWN_NAME     relowner;

    if (tabid)
    {
	tabid->db_tab_base = 0;
	tabid->db_tab_index = 0;
    }
    if (tabname)
	MEfill(sizeof(DB_TAB_NAME), ' ', tabname->db_tab_name);
    if (ownname)
	MEfill(sizeof(DB_OWN_NAME), ' ', ownname->db_own_name);

    if (!tabid)
	return;

    log_tabid = NULL;
    log_tabname = NULL;
    log_ownname = NULL;

    switch (h->type)
    {
	case DM0LBI:
	    log_tabid = &((DM0L_BI *)h)->bi_tbl_id;
	    break;

	case DM0LAI:
	    log_tabid = &((DM0L_AI *)h)->ai_tbl_id;
	    break;

	case DM0LPUT:
	    log_tabid = &((DM0L_PUT *)h)->put_tbl_id;
	    break;

	case DM0LDEL:
	    log_tabid = &((DM0L_DEL *)h)->del_tbl_id;
	    break;

	case DM0LREP:
	    log_tabid = &((DM0L_REP *)h)->rep_tbl_id;
	    break;

	case DM0LFRENAME:
	    log_tabid = &((DM0L_FRENAME *)h)->fr_tbl_id;
	    break;

	case DM0LFCREATE:
	    log_tabid = &((DM0L_FCREATE *)h)->fc_tbl_id;
	    break;

	case DM0LCREATE:
	{
	    DM0L_CREATE	    *r = (DM0L_CREATE*)h;

	    log_tabid = &r->duc_tbl_id;
	    log_tabname = &r->duc_name;
	    log_ownname = &r->duc_owner;
	}
	break;

	case DM0LCRVERIFY:
	{
	    DM0L_CRVERIFY    *r = (DM0L_CRVERIFY*)h;

	    log_tabid = &r->ducv_tbl_id;
	    log_tabname = &r->ducv_name;
	    log_ownname = &r->ducv_owner;
	}
	break;

	case DM0LDESTROY:
	{
	    DM0L_DESTROY    *r = (DM0L_DESTROY*)h;

	    log_tabid = &r->dud_tbl_id;
	    log_tabname = &r->dud_name;
	    log_ownname = &r->dud_owner;
	}
	break;

    case DM0LRELOCATE:
	{
	    DM0L_RELOCATE   *r = (DM0L_RELOCATE*)h;

	    log_tabid = &r->dur_tbl_id;
	    log_tabname = &r->dur_name;
	    log_ownname = &r->dur_owner;
	}
	break;

    case DM0LMODIFY:
	{
	    DM0L_MODIFY	    *r = (DM0L_MODIFY*)h;

	    log_tabid = &r->dum_tbl_id;
	    log_tabname = &r->dum_name;
	    log_ownname = &r->dum_owner;
	}
	break;

    case DM0LLOAD:
	{
	    DM0L_LOAD	    *r = (DM0L_LOAD*)h;

	    log_tabid = &r->dul_tbl_id;
	    log_tabname = &r->dul_name;
	    log_ownname = &r->dul_owner;
	}
	break;

    case DM0LINDEX:
	{
	    DM0L_INDEX	    *r = (DM0L_INDEX*)h;

	    log_tabid = &r->dui_tbl_id;
	    log_tabname = &r->dui_name;
	    log_ownname = &r->dui_owner;
	}
	break;

	case DM0LSM1RENAME:
	    log_tabid = &((DM0L_SM1_RENAME *)h)->sm1_tbl_id;
	    break;

	case DM0LSM2CONFIG:
	    log_tabid = &((DM0L_SM2_CONFIG *)h)->sm2_tbl_id;
	    break;

	case DM0LSM0CLOSEPURGE:
	    log_tabid = &((DM0L_SM0_CLOSEPURGE *)h)->sm0_tbl_id; 
	    break;

	case DM0LALTER:
	{
	    DM0L_ALTER	    *r = (DM0L_ALTER*)h;

	    log_tabid = &r->dua_tbl_id;
	    log_tabname = &r->dua_name;
	    log_ownname = &r->dua_owner;
	}
	break;

	case DM0LDMU:
	{
	    DM0L_DMU	    *r = (DM0L_DMU*)h;

	    log_tabid = &r->dmu_tabid;
	    log_tabname = &r->dmu_tblname;
	    log_ownname = &r->dmu_tblowner;
	}
	break;

	case DM0LASSOC:
	    log_tabid = &((DM0L_ASSOC *)h)->ass_tbl_id;
	    break;

	case DM0LALLOC:
	    log_tabid = &((DM0L_ALLOC *)h)->all_tblid;
	    break;

	case DM0LDEALLOC:
	    log_tabid = &((DM0L_DEALLOC *)h)->dall_tblid;
	    break;

	case DM0LEXTEND:
	    log_tabid = &((DM0L_EXTEND *)h)->ext_tblid;
	    break;

	case DM0LOVFL:
	    log_tabid = &((DM0L_OVFL *)h)->ovf_tbl_id;
	    break;

	case DM0LNOFULL:
	    log_tabid = &((DM0L_NOFULL *)h)->nofull_tbl_id;
	    break;

	case DM0LFMAP:
	    log_tabid = &((DM0L_FMAP *)h)->fmap_tblid;
	    break;

	case DM0LUFMAP:
	    log_tabid = &((DM0L_UFMAP *)h)->fmap_tblid;
	    break;

	case DM0LBTPUT:
	    log_tabid = &((DM0L_BTPUT *)h)->btp_tbl_id;
	    break;

	case DM0LBTDEL:
	    log_tabid = &((DM0L_BTDEL *)h)->btd_tbl_id;
	    break;

	case DM0LBTSPLIT:
	    log_tabid = &((DM0L_BTSPLIT *)h)->spl_tbl_id;
	    break;

	case DM0LBTOVFL:
	    log_tabid = &((DM0L_BTOVFL *)h)->bto_tbl_id;
	    break;

	case DM0LBTFREE:
	    log_tabid = &((DM0L_BTFREE *)h)->btf_tbl_id;
	    break;

	case DM0LBTUPDOVFL:
	    log_tabid = &((DM0L_BTUPDOVFL *)h)->btu_tbl_id;
	    break;

	case DM0LDISASSOC:
	    log_tabid = &((DM0L_DISASSOC *)h)->dis_tbl_id;
	    break;

	case DM0LRTDEL:
	    log_tabid = &((DM0L_RTDEL *)h)->rtd_tbl_id;
	    break;

	case DM0LRTPUT:
	    log_tabid = &((DM0L_RTPUT *)h)->rtp_tbl_id;
	    break;

	case DM0LRTREP:
	    log_tabid = &((DM0L_RTREP *)h)->rtr_tbl_id;
	    break;

	case DM0LBSF:
	{
	    DM0L_BSF    *r = (DM0L_BSF *)h;

	    log_tabid = &r->bsf_tbl_id;
	    log_tabname = &r->bsf_name;
	    log_ownname = &r->bsf_owner;
	}
	break;

	case DM0LESF:
	{
	    DM0L_ESF    *r = (DM0L_ESF *)h;

	    log_tabid = &r->esf_tbl_id;
	    log_tabname = &r->esf_name;
	    log_ownname = &r->esf_owner;
	}
	break;

	case DM0LRNLLSN:
        {
            DM0L_RNL_LSN    *r = (DM0L_RNL_LSN *)h;

	    log_tabid = &r->rl_tbl_id;
	    log_tabname = &r->rl_name;
	    log_ownname = &r->rl_owner;

        }
        break;

    default:
	break;
    }	

    if (log_tabid)
	STRUCT_ASSIGN_MACRO(*log_tabid, *tabid);
    if (log_tabname)
    {
	STRUCT_ASSIGN_MACRO(*log_tabname, *tabname);
	if (log_ownname)
	    STRUCT_ASSIGN_MACRO(*log_ownname, *ownname);
	return;
    }

    if (tabname)
    {
	/*
	** Lookup table name/owner
	** Should find tcb if dmve_fix_tbio was successful
	*/
	if (dmve->dmve_tabinfo.tabid.db_tab_base == tabid->db_tab_base &&
	    dmve->dmve_tabinfo.tabid.db_tab_index == tabid->db_tab_index)
	{
	    STmove(dmve->dmve_tabinfo.tabstr, ' ',
		sizeof(DB_TAB_NAME), tabname->db_tab_name);
	    STmove(dmve->dmve_tabinfo.ownstr, ' ',
		sizeof(DB_OWN_NAME), ownname->db_own_name);
	}
	else if (dm2t_lookup_tabname(dcb, tabid,
		    &relid, &relowner, &local_dberr) == E_DB_OK)
	{
	    STRUCT_ASSIGN_MACRO(relid, *tabname);
	    STRUCT_ASSIGN_MACRO(relowner, *ownname);
	}
	else
	{
	    /* alternative error handling would be to reset tabid to 0,0 */
	    STprintf(tnamebuf, "reltid%d_reltidx%d", 
		tabid->db_tab_base, tabid->db_tab_index);
	    STmove(tnamebuf, ' ', sizeof(DB_TAB_NAME), tabname->db_tab_name);
	}
    }

    return;
}

/*
** Name: dmve_get_tabid - Extract table id from a log record
**
** Description:
**
**      This routine is called to extract the table id from a log record.
**
** Inputs:
**      record		- log record
**
** Outputs:
**      None
**
** Returns:
**      tabid
**
** History:
**	21-jul-2010 (stial01)
**	    Created.
*/
VOID
dmve_get_tabid(
PTR                 record,
DB_TAB_ID	    *tabid)
{
    DMVE_CB	dmve_cb;
    DMVE_CB	*dmve = &dmve_cb;

    /* init DMVE_CB for dmve_get_tabinfo */
    DMVE_CLEAR_TABINFO_MACRO(dmve);
    dmve->dmve_log_rec = record;

    dmve_get_tabinfo(dmve, tabid, (DB_TAB_NAME *)NULL, (DB_OWN_NAME *)NULL);
    return;
}


/*
** Name: dmve_cachefix_page - Wrapper for dm0p_cachefix_page() to accomodate
**			      Cluster REDO cache locking protocols.
**
** Description:
**
**	During online Cluster REDO recovery of committed transactions,
**	normal cache locking protocols can cause recovery to wait 
**	indefinitely on a SV_PAGE cache lock on	a "stale" page, 
**	i.e., one who's LSN is greater than or equal to
**	the LSN of the log record's LSN.
**
**	While in Cluster recovery, non-CSP lock requests are stalled until
**	the recovery process completes. During the stall, a still-active
**	DBMS session may hold a SV_PAGE lock on a page needed by REDO.
**
**	If that page has been updated beyond the needed LSN, no REDO
**	action is needed.
**
**	To compare the page LSN, the page must be fixed without
**	waiting for or taking a SV_PAGE cache lock.
**
**	Special code in dm0p_cachefix_page and fault_page does just that,
**	triggered by the presence of the "RedoLSN" parameter.
**
**	If RedoLSN, the page is faulted in (or found in cache) and its
**	LSN compared with RedoLSN; if the page LSN is GTE RedoLSN
**	then REDO is not needed and the page is fixed without a cache
**	lock. If the page LSN is LT RedoLSN, then dm0p_cachefix_page
**	will acquire a cache lock.
**
**	Either way, this all happens under the covers, and to dmve just
**	appears as a normal dm0p_cachefix_page.
**
** Inputs:
**	dmve            Recovery context:
**	    .dmve_action	    DMVE_DO, DMVE_REDO, or DMVE_UNDO.
**	    .dmve_dcb_ptr	    Pointer to DCB.
**	    .dmve_tran_id	    The physical transaction id.
**	    .dmve_lk_id		    The transaction lock list id.
**	    .dmve_log_id	    The logging system database id.
**	log_lsn		The LSN to be recovered
**	tbio		tbio
**	page_number	The page to fix
**	fix_action	dm0p_cachefix_page action(s)
**	plv		containing dmpp_format info
**
** Outputs:
**	page		Pointer to pointer to page
**			just fixed.
**	err_code	Why dm0p_cachefix_page failed,
**			if it did.
**
** Returns:
**	status		E_DB_OK if dm0p_cache_fix succeeded.
**
** History:
**	06-Mar-2007 (jonj)
**	    Created.
**	11-Oct-2007 (jonj)
**	    dmve_flag is DMVE_CSP if doing cluster recovery.
*/
DB_STATUS
dmve_cachefix_page(
DMVE_CB         *dmve,
LG_LSN		*log_lsn,
DMP_TABLE_IO	*tbio,
DM_PAGENO	page_number,
i4		fix_action,
DMPP_ACC_PLV	*plv,
DMP_PINFO	**pinfo)
{
    DMP_DCB	*dcb = dmve->dmve_dcb_ptr;
    LG_LSN	*RedoLSN = (LG_LSN*)NULL;
    DB_STATUS   status;	
    i4		i;
    i4			*err_code = &dmve->dmve_error.err_code;

    CLRDBERR(&dmve->dmve_error);

    /* Find an DMVE_PAGE entry to use */
    for (i = 0; i < DMVE_PGFIX_MAX; i++)
    {
	if (!dmve->dmve_pages[i].pinfo.page)
	    break;
    }

    if (i < DMVE_PGFIX_MAX)
	*pinfo = &dmve->dmve_pages[i].pinfo;
    else
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9691_DMVE_PAGE_FIX);
	return (E_DB_ERROR);
    }

    /* Cluster REDO recovery? */
    if ( dmve->dmve_flags & DMVE_CSP &&
         dmve->dmve_action == DMVE_REDO )
    {
	/* Tell dm0p it's cluster REDO */
	fix_action |= DM0P_CSP_REDO;

	/* If REDO during online recovery, pass log record LSN */
	if ( dcb->dcb_status & DCB_S_ONLINE_RCP )
	{
	    /* This is the LSN to check vs page LSN */
	    RedoLSN = log_lsn;
	    /* Suppress group reads */
	    fix_action &= ~DM0P_READAHEAD;
	}
    }

    return(dm0p_cachefix_page(tbio,
				page_number,
				DM0P_WRITE,
				fix_action,
				dmve->dmve_lk_id,
				dmve->dmve_log_id,
				&dmve->dmve_tran_id,
				(DMPP_PAGE*)NULL,
				plv->dmpp_format,
				*pinfo,
				RedoLSN,
				&dmve->dmve_error));
}


/*{
** Name: dmve_fix_page - lock/fix DATA or INDEX page for recovery
**
** Description: Lock/fix DATA or INDEX page for recovery
**              (This routine is not for FMAP/FHDR pages)
**
** Inputs:
**	dmve_cb
**
** Outputs:
**	    none
**
**
** A note about row locking during recovery
** Currently during REDO recovery we must have exclusive control
** of the page being recovered. This allows us to clean all
** deleted tuples from the page regardless of transaction id.
** Since page cleans are not logged, this simplifies REDO recovery.
** 
** rollforwarddb DMVE_DO:
**     We should have X table or database lock.
** 
** Offline recovery by RCP, DMVE_REDO:
**     We should have X database lock.
**
** Online recovery by RCP, DMVE_REDO:
**     Fast commit db -> full database REDO, X database lock
**
**     Non-fast commit db -> redo of transactions being recovered.
**         We disallow row locking when multiple servers are using
**         different data caches. (And online recovery will not get
**         exclusive access to database).
** 
** History:
**	14-oct-2009 (stial01)
**          Created.
**	15-Jan-2010 (jonj)
**	    SIR 121619 MVCC: Replace DMPP_PAGE* with DMP_PINFO* where needed.
*/
DB_STATUS
dmve_fix_page(
DMVE_CB		*dmve,
DMP_TABLE_IO	*tbio,
DM_PAGENO	pageno,
DMP_PINFO	**pinfo)
{
    DM0L_HEADER		*log_rec = (DM0L_HEADER *)dmve->dmve_log_rec;
    LG_LSN		*log_lsn = &log_rec->lsn; 
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    i4			grant_mode;
    i4			lock_action;
    i4			lock_mode;
    LK_LKID		tbl_lockid;
    DB_STATUS		status = E_DB_OK;
    DB_STATUS		tmp_status;
    LG_LSN		*RedoLSN = (LG_LSN*)NULL;
    i4			loc_error;
    i4			i;
    DMVE_FIX_HISTORY	*fixinfo;
    DMP_TCB		*t;

    /* This might be partial tcb, but it will always basic table info */
    t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

    MEfill(sizeof(LK_LKID), 0, &tbl_lockid);

    for ( ; ; )
    {
	/* Init return values */
	*pinfo = (DMP_PINFO*)NULL;

	if ((dmve->dmve_flags & DMVE_MVCC) && dmve->dmve_pinfo->page)
	{
	    if (DMPP_VPT_GET_PAGE_PAGE_MACRO(tbio->tbio_page_type, 
	    		dmve->dmve_pinfo->page) == pageno)
	    {
		*pinfo = dmve->dmve_pinfo;
	    }
	    else
	    {
		/* possible when log record updates multiple pages! */
		for (i = 0; i < DMVE_PGFIX_MAX; i++)
		{
		    if (!dmve->dmve_pages[i].pinfo.page)
			break;
		}
		if (i < DMVE_PGFIX_MAX)
		{
		    /* Return pointer to pinfo with null page pointer */
		    *pinfo = &dmve->dmve_pages[i].pinfo;
		    DMP_PINIT(*pinfo);
		}
		else
		{
		    SETDBERR(&dmve->dmve_error, 0, E_DM9691_DMVE_PAGE_FIX);
		    return(E_DB_ERROR);
		}
	    }
	
	    return (E_DB_OK);
	}

	/* Find an DMVE_PAGE entry to record dmve_page_fix */
	for (i = 0; i < DMVE_PGFIX_MAX; i++)
	{
	    if (!dmve->dmve_pages[i].pinfo.page)
		break;
	}

	if (i < DMVE_PGFIX_MAX)
	{
	    fixinfo = &dmve->dmve_pages[i];
	    DMP_PINIT(&fixinfo->pinfo);
	    fixinfo->page_number = pageno;
	    fixinfo->fix_action = 0;
	    fixinfo->page_lockid.lk_unique = 0;
	    fixinfo->page_lockid.lk_common = 0;

	    /* Special fix flag if dump processing for DM0LBI logrec */
	    if (log_rec->type == DM0LBI && log_rec->flags & DM0L_DUMP)
		fixinfo->fix_action |= DM0P_SCRATCH;
	}
	else
	{
	    SETDBERR(&dmve->dmve_error, 0, E_DM9691_DMVE_PAGE_FIX);
	    return (E_DB_ERROR);
	}


	/*
	** Get required Table/Page locks before we can start the update.
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
	** 
	** Note some log records update > 1 page, so this code is structured
	** such that we determine table lock grant mode once
	*/

	if (dcb->dcb_status & DCB_S_EXCLUSIVE)
	{
	    dmve->dmve_lk_type = LK_DATABASE;
	}
	else if (!dmve->dmve_lk_type)
	{
	    /*
	    ** Request IX table lock in preparation of requesting an X page lock
	    ** below.  If the transaction already holds an exclusive table
	    ** lock, then an exclusive lock will be granted.  In this case we can
	    ** bypass the page lock request.
	    */
	    status = dm2t_lock_table(dcb, &tbio->tbio_reltid, LK_IX,
		dmve->dmve_lk_id, (i4)0, &grant_mode, &tbl_lockid, 
		&dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
		break;
	    }

	    if (grant_mode == LK_X)
		dmve->dmve_lk_type = LK_TABLE;
	    else if ( log_rec->flags & DM0L_CROW_LOCK )
	        dmve->dmve_lk_type = LK_CROW;
	    else if ( log_rec->flags & DM0L_ROW_LOCK )
		dmve->dmve_lk_type = LK_ROW;
	    else
		dmve->dmve_lk_type = LK_PAGE;
	}

	if (dmve->dmve_lk_type == LK_TABLE)
	    fixinfo->fix_action |= DM0P_TABLE_LOCKED_X;
	else if (dmve->dmve_lk_type == LK_PAGE || dmve->dmve_lk_type == LK_ROW)
	{
	    /*
	    ** Page lock(s) required.  If this is a system catalog page
	    ** (indicated by the logged page type) then we need to
	    ** request physical locks (and release them later).
	    */
	    if (log_rec->flags & DM0L_PHYS_LOCK)
	    {
		lock_action = LK_PHYSICAL;
		lock_mode = LK_X;
	    }
	    else
	    {
		lock_action = LK_LOGICAL;
	
		if ((log_rec->flags & DM0L_ROW_LOCK) == 0)
		    lock_mode = LK_X;
		else
		{
		    lock_mode = LK_IX;
		    fixinfo->fix_action |= DM0P_INTENT_LK;
		    if (dmve->dmve_action == DMVE_REDO || 
				dmve->dmve_action == DMVE_DO)
		    {
			SETDBERR(&dmve->dmve_error, 0, E_DM93F5_ROW_LOCK_PROTOCOL);
			status = E_DB_ERROR;
			break;
		    }
		}
	    }

	    /* Lock the page */
	    status = dm0p_lock_page(dmve->dmve_lk_id, dcb, 
		    &tbio->tbio_reltid, fixinfo->page_number, LK_PAGE, lock_mode,
		    lock_action, (i4)0, tbio->tbio_relid,
		    tbio->tbio_relowner, &dmve->dmve_tran_id,
		    &fixinfo->page_lockid, (i4 *)0, (LK_VALUE *)0, 
		    &dmve->dmve_error);
	    if (status != E_DB_OK)
	    {
		uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG,
		    NULL, (char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
		break;
	    }
	}

	/* Cluster REDO recovery? */
	if ( dmve->dmve_flags & DMVE_CSP &&
	     dmve->dmve_action == DMVE_REDO )
	{
	    /* Tell dm0p it's cluster REDO */
	    fixinfo->fix_action |= DM0P_CSP_REDO;

	    /* If REDO during online recovery, pass log record LSN */
	    if ( dcb->dcb_status & DCB_S_ONLINE_RCP )
	    {
		/* This is the LSN to check vs page LSN */
		RedoLSN = log_lsn;
		/* Suppress group reads */
		fixinfo->fix_action &= ~DM0P_READAHEAD;
	    }
	}

	/* Fix the page we need to recover in cache for write */
	status = dm0p_cachefix_page(tbio, pageno, DM0P_WRITE, fixinfo->fix_action,
		    dmve->dmve_lk_id, dmve->dmve_log_id, &dmve->dmve_tran_id,
		    (DMPP_PAGE*)NULL, dmve->dmve_plv->dmpp_format,
		    &fixinfo->pinfo,
		    RedoLSN, &dmve->dmve_error);

	if (status == E_DB_OK)
	{
	    if ( (dmve->dmve_lk_type == LK_ROW || dmve->dmve_lk_type == LK_CROW)
	        && !DMPP_VPT_IS_CR_PAGE(tbio->tbio_page_type, fixinfo->pinfo.page) )
	    {
		/* Exclusively pin the Root page */
		dm0pPinBuf(tbio, DM0P_MPINX, dmve->dmve_lk_id, &fixinfo->pinfo);
	    }
	}
	else
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		    (char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);

	    if ( fixinfo->page_lockid.lk_unique )
	    {
		tmp_status = dm0p_unlock_page(dmve->dmve_lk_id, dcb, 
		    &tbio->tbio_reltid, fixinfo->page_number,
		    LK_PAGE, tbio->tbio_relid, &dmve->dmve_tran_id, &fixinfo->page_lockid, 
		    (LK_VALUE *)NULL, &dmve->dmve_error);
		if (tmp_status != E_DB_OK)
		{
		    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
			(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
		    if (tmp_status > status)
			status = tmp_status;
		}
	    }
	}

	break; /* ALWAYS */
    }

    if (status == E_DB_OK)
    {
	/* Save LK_LKID for LK_PHYSICAL page locks only */
	if (lock_action != LK_PHYSICAL)
	{
	    fixinfo->page_lockid.lk_unique = 0;
	    fixinfo->page_lockid.lk_common = 0;
	}
	*pinfo = &fixinfo->pinfo;
    }

    return (status);
}

/*{
** Name: dmve_unfix_page - Unfix/unlock DATA or INDEX page for recovery
**
** Description: Unfix/unlock DATA or INDEX page for recovery
**
** Inputs:
**	dmve_cb
**
** Outputs:
**	    none
**
** History:
**	14-oct-2009 (stial01)
**          Created.
*/
DB_STATUS
dmve_unfix_page(
DMVE_CB		*dmve,
DMP_TABLE_IO	*tbio,
DMP_PINFO	*pinfo)
{
    DMP_DCB		*dcb = dmve->dmve_dcb_ptr;
    DB_STATUS		tmp_status;
    DB_STATUS		status = E_DB_OK;
    i4			loc_error;
    i4			i;
    DMP_TCB		*t = NULL;
    DMVE_FIX_HISTORY	*fixinfo;

    /* Forgive NULL page pointer if MVCC-related */
    if ( !pinfo->page && dmve->dmve_flags & DMVE_MVCC )
	return (E_DB_OK);

    if ( DMPP_VPT_IS_CR_PAGE(tbio->tbio_page_type, pinfo->page) )
    {
	/* Make sure the make_consistent() page is left "unmodified" */
	DMPP_VPT_CLR_PAGE_STAT_MACRO(tbio->tbio_page_type, pinfo->page,
					DMPP_MODIFY);
	/* Do NOT clear pinfo->page - it's still fixed by dm0p_make_consistent! */
	return (E_DB_OK);
    }

    /* This might be partial tcb, but it will always basic table info */
    t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

    /* Find the DMVE_FIX_HISTORY entry for this page fixed by dmve */
    for (i = 0, fixinfo = &dmve->dmve_pages[0]; 
	    i < DMVE_PGFIX_MAX;
		i++, fixinfo++)
    {
	if (fixinfo->pinfo.page == pinfo->page &&
	    fixinfo->page_number == 
		DMPP_VPT_GET_PAGE_PAGE_MACRO(t->tcb_rel.relpgtype, pinfo->page))
	    break;
    }

    if (i >= DMVE_PGFIX_MAX)
    {
	SETDBERR(&dmve->dmve_error, 0, E_DM9691_DMVE_PAGE_FIX);
	return (E_DB_ERROR);
    }
    
    /* Unfix page */
    tmp_status = dm0p_uncache_fix(tbio, DM0P_UNFIX, dmve->dmve_lk_id, 
	    dmve->dmve_log_id, &dmve->dmve_tran_id, pinfo, 
	    &dmve->dmve_error);
    if (tmp_status != E_DB_OK)
    {
	uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
	    (char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	if (tmp_status > status)
	    status = tmp_status;
    }

    /* Check if we have a PHYSICAL page lock */
    if ( fixinfo->page_lockid.lk_unique )
    {
	/* Unlock page */
	tmp_status = dm0p_unlock_page(dmve->dmve_lk_id, dcb, 
	    &tbio->tbio_reltid, fixinfo->page_number,
	    LK_PAGE, tbio->tbio_relid, &dmve->dmve_tran_id, 
	    &fixinfo->page_lockid, (LK_VALUE *)NULL, &dmve->dmve_error);
	if (tmp_status != E_DB_OK)
	{
	    uleFormat(&dmve->dmve_error, 0, (CL_ERR_DESC *)NULL, ULE_LOG, NULL, 
		(char *)NULL, (i4)0, (i4 *)NULL, &loc_error, 0);
	    if (tmp_status > status)
		status = tmp_status;
	}

	fixinfo->page_lockid.lk_unique = 0;
	fixinfo->page_lockid.lk_common = 0;
    }

    return (status);
}


/*{
** Name: dmve_cleanup - cleanup after dmve exception.
**
** Description:
**
** Inputs:
**	dmve_cb
**
** Outputs:
**	    none
**
**
** History:
**	14-oct-2009 (stial01)
**          Created.
*/
VOID
dmve_cleanup(
DMVE_CB		*dmve)
{
    DMP_TABLE_IO	*tbio;
    DB_STATUS		status;

    /*
    ** dmve_tbio should only be set when tbio is fixed in dmve routines
    ** This is being set in case dmve routines get an exception and the 
    ** caller of dmve routines wants to try to cleanup and continue
    */
    tbio = dmve->dmve_tbio;
    if (tbio)
    {
	dmve->dmve_tbio = NULL;
	TRdisplay("dmve_cleanup unfixing tbio for %~t\n",
		sizeof(DB_TAB_NAME), tbio->tbio_relid->db_tab_name);
	status = dmve_unfix_tabio(dmve, &tbio, DMVE_INVALID);
    }

    return;
}


/*{
** Name: dmve_iirel_cmp - Compare iirelation records before undo/redo recovery
**
** Description: Compare logged iirelation row to the one we are about to update
**      Compare reltid only, since iirelation is periodically updated
**      withoug logging.
**
** Inputs:
**	dmve_cb
**      rec1			iirelation row (compressed)
**      rec2			iirelation row (compressed)
**
** Outputs:
**	    none
**
**
** History:
**	29-Apr-2010 (stial01)
**          Created.
**	5-May-2010 (kschendel)
**	    Copy to aligned ints for alignment-sensitive platforms (SPARC).
*/
bool
dmve_iirel_cmp(
DMVE_CB		*dmve,
char		*rec1,
i4		size1,
char		*rec2,
i4		size2)
{
    DMP_TABLE_IO	*tbio;
    DMP_TCB		*t;
    DMP_RELATION	*rel1;
    DMP_RELATION	*rel2;
    i4			rel1_base, rel2_base;
    i4			rel1_index, rel2_index;

    tbio = dmve->dmve_tbio;

    /* The TCB for iirelation will always have DMP_ROWACCESS init */
    t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

    /* This routine assumes reltid is first column in iirelation */
    if (CL_OFFSETOF( DMP_RELATION, reltid ) != 0)
	return (FALSE);

    /*
    ** Since iirelation uses data compression, compare_size=sizeof(DB_TAB_ID)
    ** (relid will be compressed, and it is redundant to compare that also)
    ** We could uncompress the row here, but since we only need to look
    ** at reltid,reltidx at the start of the row we won't bother
    */
    rel1 = (DMP_RELATION *)rec1;
    rel2 = (DMP_RELATION *)rec2;

    /* Records aren't necessarily aligned, copy to locals. */
    I4ASSIGN_MACRO(rel1->reltid.db_tab_base, rel1_base);
    I4ASSIGN_MACRO(rel2->reltid.db_tab_base, rel2_base);
    I4ASSIGN_MACRO(rel1->reltid.db_tab_index, rel1_index);
    I4ASSIGN_MACRO(rel2->reltid.db_tab_index, rel2_index);

#ifdef xDEBUG
    TRdisplay("CONSISTENCY CHECK iirel (%d,%d), (%d,%d)\n",
	rel1_base, rel1_index, rel2_base, rel2_index);
#endif

    if (rel1_base != rel2_base || rel1_index != rel2_index)
	return (TRUE);
    else
	return (FALSE);

}


/*{
** Name: dmve_iiseq_cmp - Compare iisequence records before undo/redo recovery
**
** Description: Compare logged iisequence row to the one we are about to update
**      Compare sequence name only, since iisequence is periodically updated
**      withoug logging.
**
** Inputs:
**	dmve_cb
**      comptype		compression type
**      rec1			iisequence row (might be compressed)
**      size1
**      rec2			iisequence row (might be compressed)
**      size2
**
** Outputs:
**	    none
**
**
** History:
**	29-Apr-2010 (stial01)
**          Created.
*/
bool
dmve_iiseq_cmp(
DMVE_CB		*dmve,
i2		comptype,
char		*rec1,
i4		size1,
char		*rec2,
i4		size2)
{
    DMP_TABLE_IO	*tbio = NULL;
    DMP_TCB		*t;
    DB_IISEQUENCE	*seq1;
    DB_IISEQUENCE	*seq2;
    i2			name_len1;
    i2			name_len2;
    i4			compare_size;

    tbio = dmve->dmve_tbio;
    /* This might be partial tcb (no attribute information) */
    t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

    /* This routine assumes dbs_name is first column in iisequence */
    if (CL_OFFSETOF( DB_IISEQUENCE, dbs_name ) != 0)
	return (FALSE);

    seq1 = (DB_IISEQUENCE *)rec1;
    seq2 = (DB_IISEQUENCE *)rec2;
    if (comptype == TCB_C_NONE)
    {
	if (size1 != size2
		|| MEcmp(&seq1->dbs_name, &seq1->dbs_name, DB_SEQ_MAXNAME)
		|| MEcmp(&seq1->dbs_owner, &seq1->dbs_owner, DB_OWN_MAXNAME))
	    return (TRUE);
	else
	    return (FALSE);
    }

    /*
    ** If iisequence uses data compression, extract the 2 byte length
    ** and compare the compressed sequence name
    ** For iisequence rows is valid to compare the compressed lengths
    ** (non-char fields are not compressed, and char fields should not change)
    */
    MEcopy(rec1, sizeof(name_len1), (char *)&name_len1);
    MEcopy(rec2, sizeof(name_len2), (char *)&name_len2);
    compare_size = name_len1 + sizeof(name_len1);

#ifdef xDEBUG
    TRdisplay("CONSISTENCY CHECK: iiseq size1 %d size2 %d name_len %d %~t %~t\n",
	size1, size2, name_len1, 
	name_len1, rec1 + DB_CNTSIZE, name_len2, rec2 + DB_CNTSIZE);
#endif

    /* Make sure this looks like a valid DB_IISEQUENCE record */
    if (name_len1 > DB_SEQ_MAXNAME || name_len2 > DB_SEQ_MAXNAME)
	return (TRUE);

    if (size1 != size2 || name_len1 != name_len2 ||
	MEcmp(rec1, rec2, compare_size))
	return (TRUE);
    else
	return (FALSE);

}

/*
** Name: dmve_init_tabinfo - Init table info inside DMVE_CB
**
** Description:
**
**      This routine is called to init table information inside DMVE_CB
**      It should be called after successful dmve_fix_tbio
**      Table information is then available in the dmve cb for error handling
**
** Inputs:
**      record		- log record
**
** Outputs:
**      None
**
** Returns:
**      tabid
**
** History:
**	21-jul-2010 (stial01)
**	    Created.
*/
static VOID
dmve_init_tabinfo(
DMVE_CB		*dmve,
DMP_TABLE_IO	*tbio)
{
    DMP_TCB *t;

    if (!tbio)
    {
	dmve->dmve_tabinfo.ownlen = 0;
	dmve->dmve_tabinfo.tablen = 0;
	dmve->dmve_tabinfo.tabid.db_tab_base = 0;
	dmve->dmve_tabinfo.tabid.db_tab_index = 0;
    }
    else
    {
	/* This might be partial tcb, but it will always have basic info */
	t = (DMP_TCB *)((char *)tbio - CL_OFFSETOF(DMP_TCB, tcb_table_io));

	MEcopy(t->tcb_rel.relid.db_tab_name, t->tcb_relid_len, 
		dmve->dmve_tabinfo.tabstr);
	dmve->dmve_tabinfo.tabstr[t->tcb_relid_len] = '\0';

	MEcopy(t->tcb_rel.relowner.db_own_name, t->tcb_relowner_len, 
		dmve->dmve_tabinfo.ownstr);
	dmve->dmve_tabinfo.ownstr[t->tcb_relowner_len] = '\0';

	dmve->dmve_tabinfo.ownlen = t->tcb_relowner_len;
	dmve->dmve_tabinfo.tablen = t->tcb_relid_len;
	dmve->dmve_tabinfo.tabid = t->tcb_rel.reltid;
    }
}
