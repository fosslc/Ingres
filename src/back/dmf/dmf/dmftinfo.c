/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <tm.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>

#include    <ex.h>
#include    <cs.h>
#include    <st.h>
#include    <me.h>
#include    <pc.h>

#include    <dmf.h>
#include    <dm.h>
#include    <dmccb.h>
#include    <dmdcb.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmxcb.h>
#include    <dmucb.h>
#include    <dmmcb.h>
#include    <lk.h>
#include    <lg.h>
#include    <scf.h>
#include    <adf.h>

#include    <dmp.h>
#include    <dml.h>
#include    <dmu.h>
#include    <dmm.h>
#include    <dmpp.h>
#include    <dm1b.h>
#include    <dm0l.h>
#include    <dmd.h>


/**
**
**  Name: DMFTINFO.C - Entry point into DMF used by the ADF FEXI functions
**			 iitotal_allocated_pages, iitotal_overflow_pages.
**                       and iitableinfo
**
**  Description:
**
**	    dmf_tbl_info - Entry point for ADF FEXI functions 
**			   iitotal_allocated_pages and iioverflow_pages.
**
**
**  History:
**	30-November-1992 (rmuth)
**	    Created.
**	14-dec-1992 (jnash)
**	    Move dm1b.h before dm0l.h for prototype definitions.
**	18-dec-1992 (robf)
**	    Removed obsolete dm0a.h
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	25-oct-93 (vijay)
**	    Remove incorrect type cast.
**      28-aug-1997 (hanch04)
**          added tm.h
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	08-Jan-2001 (jenjo02)
**	    Get *DML_SCB with macro instead of SCU_INFORMATION.
**      16-apr-2001 (stial01)
**          Added support for iitableinfo function
**      17-apr-2001 (stial01)
**          Added iitableinfo 'num_rows', 'num_pages'
**/

/*
**  Definition of static variables and forward static functions.
*/
static DB_STATUS
dmf_iitableinfo(
	    char		*op_str,
	    DMT_TBL_ENTRY	*dmt_tbl_entry,
	    i4		*return_count);

DB_STATUS
dmf_tbl_info(
    i4  	*reltid,
    i4	  	*reltidx,
    i4		op_code,
    char	*tableinfo_request,
    i4		*return_count)
{

    DB_STATUS		status;
    DML_SCB		*scb;
    DMT_SHW_CB		dmt_show;
    DMT_TBL_ENTRY	dmt_tbl_entry;
    CS_SID 		sid;

    do
    {

	*return_count = 0;

	/*
	** Find out information about the current query, specifically
	** the database and transaction ids
	*/
	CSget_sid(&sid);
	scb = GET_DML_SCB(sid);

	/*
	** make sure we are have a transaction
	*/
	if (scb->scb_x_ref_count != 1)
	    break;

	dmt_show.type = DMT_SH_CB;
	dmt_show.length = sizeof(DMT_SHW_CB);
	dmt_show.dmt_char_array.data_in_size = 0;
	dmt_show.dmt_flags_mask = DMT_M_TABLE;
	dmt_show.dmt_table.data_address = (PTR) &dmt_tbl_entry;
	dmt_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	dmt_show.dmt_char_array.data_address = (PTR)NULL;
	dmt_show.dmt_char_array.data_in_size = 0;
	dmt_show.dmt_char_array.data_out_size  = 0;
	dmt_show.error.err_code = 0;

	dmt_show.dmt_session_id	= sid;
	dmt_show.dmt_db_id = (PTR)scb->scb_x_next->xcb_odcb_ptr;

	dmt_show.dmt_tab_id.db_tab_base  =  *reltid;
	dmt_show.dmt_tab_id.db_tab_index =  *reltidx;



	status = dmf_call(DMT_SHOW, &dmt_show);
	if ( status != OK )
	    break;

	switch (op_code)
	{
	case ADI_03ALLOCATED_FEXI:
	    *return_count = dmt_tbl_entry.tbl_alloc_pg_count;
	    break;

	case ADI_04OVERFLOW_FEXI:
	    *return_count = dmt_tbl_entry.tbl_opage_count;
	    break;

	case ADI_06TABLEINFO_FEXI:
	    if (!tableinfo_request)
	    {
		status = E_DB_ERROR;
		break;
	    }
	    status = dmf_iitableinfo(tableinfo_request, &dmt_tbl_entry, 
			return_count);
	    break;

	default:
	    status = FAIL;
	    break;
	}


    } while (FALSE);

    /*
    ** If this routine is called on a non-existent table then ignore the
    ** error and return zero as the return_count. This routine is called
    ** from the iitables view so the only time this should be true is for
    ** VIEWS, as we do not want to fail the query in this case do this
    ** kludge.
    */
    if (( status != OK ) && 
	( dmt_show.error.err_code == E_DM0054_NONEXISTENT_TABLE ))
    {
	status = E_DB_OK;
	dmt_show.error.err_code = 0;
    }

    return(status);

}

static DB_STATUS
dmf_iitableinfo(
char		*op_str,
DMT_TBL_ENTRY	*dmt_tbl_entry,
i4		*return_count)
{

    if (!STcompare(op_str, "tups_per_page"))
	*return_count = dmt_tbl_entry->tbl_tperpage;

    else if (!STcompare(op_str, "keys_per_page"))
	*return_count = dmt_tbl_entry->tbl_kperpage;

    else if (!STcompare(op_str, "keys_per_leaf"))
	*return_count = dmt_tbl_entry->tbl_kperleaf;

    else if (!STcompare(op_str, "index_levels_count"))
	*return_count = dmt_tbl_entry->tbl_ixlevels;

    else if (!STcompare(op_str, "data_page_count"))
	*return_count = dmt_tbl_entry->tbl_dpage_count;

    else if (!STcompare(op_str, "overflow_page_count"))
	*return_count = dmt_tbl_entry->tbl_opage_count;

    else if (!STcompare(op_str, "index_page_count"))
	*return_count = dmt_tbl_entry->tbl_ipage_count;

    else if (!STcompare(op_str, "leaf_page_count"))
	*return_count = dmt_tbl_entry->tbl_lpage_count;

    else if (!STcompare(op_str, "alloc_page_count"))
	*return_count = dmt_tbl_entry->tbl_alloc_pg_count;

    else if (!STcompare(op_str, "num_rows"))
	*return_count = dmt_tbl_entry->tbl_record_count;

    else if (!STcompare(op_str, "num_pages"))
	*return_count = dmt_tbl_entry->tbl_page_count;

    else
	*return_count = 0;
    return (E_DB_OK);
}
