/*
**Copyright (c) 2010 Ingres Corporation
*/


/**
**
**  Name: DMFSRS.C - DMF function used for the ADF FEXI function
**  			looking up a spatial_ref_sys row using SRID
**
**  Description:
**
**	    dmf_get_srs - Entry point for ADF FEXI GETSRS
**
**  History:
**  	16-Mar-2010 (troal01)
**  	    Created.
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

#include    <spatial.h>

/*
 * Name: dmf_get_srs loads row with SRID specified into db_srs
 * 
 * Description:
 * 	Will try to load a row from spatial_ref_sys using the SRID
 * 	specified. Returns various errors handled by the caller. This
 * 	function is loaded into the ADF FEXI array.
 * 
 * History:
 * 	16-Mar-2010 (troal01)
 * 	    Created.
*/

STATUS
dmf_get_srs(
    DB_SPATIAL_REF_SYS *db_srs,
    i4 *errcode)
{
	DMT_SHW_CB srs_show;
	DMT_TBL_ENTRY srs_table;
	STATUS status;
    DML_SCB	*scb;
    CS_SID sid;
    DMT_CB dmt_cb;
    DMR_CB dmr_cb;
    DMR_ATTR_ENTRY key, *kptr = &key;
    SRS_ROW row;

    CSget_sid(&sid);
	scb = GET_DML_SCB(sid);

	/*
	** make sure we are have a transaction
	*/
	if (scb->scb_x_ref_count != 1)
	{
		*errcode = E_AD5603_NO_TRANSACTION;
	    return E_DB_ERROR;
	}

	/*
	 * First we need to grab spatial_ref_sys information
	 */
	MEfill(sizeof(DB_OWN_NAME), ' ', &srs_show.dmt_owner);
	MEfill(sizeof(DB_TAB_NAME), ' ', &srs_show.dmt_name);
	STncpy(srs_show.dmt_name.db_tab_name, "spatial_ref_sys", STlen("spatial_ref_sys"));
	STncpy(srs_show.dmt_owner.db_own_name, "$ingres", STlen("$ingres"));
	srs_show.type = DMT_SH_CB;
	srs_show.length = sizeof(DMT_SHW_CB);
	srs_show.dmt_session_id = sid;
	srs_show.dmt_db_id = (PTR) scb->scb_x_next->xcb_odcb_ptr;
	srs_show.dmt_flags_mask = DMT_M_NAME | DMT_M_TABLE;
	srs_show.dmt_char_array.data_address = (PTR) NULL;
	srs_show.dmt_char_array.data_in_size = 0;
	srs_show.dmt_char_array.data_out_size = 0;
	srs_show.dmt_table.data_address = (PTR) &srs_table;
	srs_show.dmt_table.data_in_size = sizeof(DMT_TBL_ENTRY);
	status = dmt_show(&srs_show);
	if(status != E_DB_OK)
	{
		//something went wrong
		*errcode = E_AD5604_SRS_NONEXISTENT;
		return status;
	}
	/*
	 * Next open the spatial_ref_sys table
	 */
	MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
	dmt_cb.type = DMT_TABLE_CB;
	dmt_cb.length = sizeof(DMT_CB);
	dmt_cb.dmt_db_id = (PTR) scb->scb_x_next->xcb_odcb_ptr;
	STRUCT_ASSIGN_MACRO(srs_table.tbl_id, dmt_cb.dmt_id);
	dmt_cb.dmt_flags_mask = 0;
	dmt_cb.dmt_lock_mode = DMT_IS;
	dmt_cb.dmt_update_mode = DMT_U_DIRECT;
	dmt_cb.dmt_mustlock = FALSE;
	dmt_cb.dmt_access_mode = DMT_A_READ;
	dmt_cb.dmt_char_array.data_in_size = 0;
	dmt_cb.dmt_sequence = 0;
	dmt_cb.dmt_tran_id = (PTR) scb->scb_x_next;
	status = dmt_open(&dmt_cb);
	if (status != E_DB_OK)
	{
		*errcode = E_AD5601_GEOSPATIAL_INTERNAL;
		return status;
	}
	/*
	 * Let's set up for the actual gets
	 */
	dmr_cb.type = DMR_RECORD_CB;
	dmr_cb.length = sizeof(DMR_CB);
	dmr_cb.dmr_access_id = dmt_cb.dmt_record_access_id;
	dmr_cb.dmr_tid = 0;
	dmr_cb.dmr_q_fcn = NULL;
	dmr_cb.dmr_position_type = DMR_QUAL;
	kptr->attr_number = SRS_SRID_COL;
	kptr->attr_operator = DMR_OP_EQ;
	kptr->attr_value = (char *) &db_srs->srs_srid;
	dmr_cb.dmr_attr_desc.ptr_address = (PTR) &kptr;
	dmr_cb.dmr_attr_desc.ptr_in_count = 1;
	dmr_cb.dmr_attr_desc.ptr_size = sizeof (DMR_ATTR_ENTRY);
	dmr_cb.dmr_flags_mask = 0;
	status = dmr_position(&dmr_cb);
	if (status != E_DB_OK)
	{
		if (dmr_cb.error.err_code == E_DM0055_NONEXT)
			*errcode = E_AD5605_INVALID_SRID;
		else
			*errcode = E_AD5601_GEOSPATIAL_INTERNAL;
		dmt_close(&dmt_cb);
		return status;
	}

	dmr_cb.dmr_data.data_in_size = SRS_ROW_SIZE;
	dmr_cb.dmr_data.data_address = row;
	dmr_cb.dmr_flags_mask = DMR_NEXT;
	status = dmr_get(&dmr_cb);
	if (status != E_DB_OK)
	{
		if (dmr_cb.error.err_code == E_DM0055_NONEXT)
		{
			*errcode = E_AD5605_INVALID_SRID;
		}
		else
		{
			*errcode = E_AD5601_GEOSPATIAL_INTERNAL;
		}
		dmt_close(&dmt_cb);
		return status;
	}

	/*
	 * If we get here we retrieved a row yay!
	 */
	MEcopy((row + SRS_AUTH_NAME_OFFSET), sizeof(db_srs->srs_auth_name), db_srs->srs_auth_name);
	db_srs->srs_auth_name[STlen(db_srs->srs_auth_name)] = '\0';
	MEcopy((row + SRS_AUTH_ID_OFFSET), sizeof(i4), &db_srs->srs_auth_id);
	MEcopy((row + SRS_SRTEXT_OFFSET), sizeof(db_srs->srs_srtext), db_srs->srs_srtext);
	db_srs->srs_srtext[STlen(db_srs->srs_srtext)] = '\0';
	MEcopy((row + SRS_PROJ4TEXT_OFFSET), sizeof(db_srs->srs_proj4text), db_srs->srs_proj4text);
	db_srs->srs_proj4text[STlen(db_srs->srs_proj4text)] = '\0';
	/*
	 * Must close the table when we're done
	 */
	status = dmt_close(&dmt_cb);
	if(status != E_DB_OK)
	{
		/* close failed?? */
		*errcode = E_AD5601_GEOSPATIAL_INTERNAL;
		return status;
	}
	return E_DB_OK;
}

