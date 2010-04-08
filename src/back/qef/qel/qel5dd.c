/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <dudbms.h>
#include    <dmf.h> 
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ex.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefgbl.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEL5DD.C - Routines deleting from STAR-specific catalogs for 
**		     DESTROY LINK
**
**  Description:
**      These routines perform STAR catalog deletion for DESTROY LINK.
**
**	    qel_51_ddb_cats	    - delete from DDB catalogs
**	    qel_52_ldbids	    - delete from IIDD_DDB_LDBIDS 
**	    qel_53_caps_cats	    - delete from IIDD_DDB_LDB_DBCAPS 
**				      and adjust IIDD_DBCAPABILITIES if
**				      necessary
**	    qel_54_tableinfo	    - delete from IIDD_DDB_TABLEINFO
**	    qel_55_objects	    - delete from IIDD_DDB_OBJECTS
**	    qel_56_ldb_columns	    - delete from IIDD_DDB_LDB_COLUMNS
**	    qel_57_longnames	    - delete from IIDD_DDB_LONG_LDBNAMES
**	    qel_58_dbcapabilities   - adjust IIDD_DBCAPABILITIES
**	    qel_59_adjust_1_cap	    - adjust IIDD_DBCAPABILITIES
**
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	30-apr-89 (carl)
**          modified qel_51_ddb_cats and qel_56_ldb_columns to save 
**	    statistics for REGISTER WITH REFRESH
**	15-aug-91 (barbara)
**	    Fixed bug 38919.
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/


/*{
** Name: qel_51_ddb_cats - Delete from DDB catalogs.
**
** Description:
**	This function deletes a link's information from the DDB catalogs 
**  using supplied information:
**
**	1.  IIDD_DDB_LDBIDS, IIDD_DDB_LDB_DBCAPS if LDB leaves the DDB, and
**	    IIDD_CAPABILITIES if affected
**	2.  IIDD_DDB_LONG_LDBNAMES if the LDB has an entry due to its long
**	    long name
**	3.  IIDD_DDB_LDB_COLUMNS if the link's columns are mapped
**	4.  IIDD_DDB_TABLEINFO
**	5.  IIDD_DDB_OBJECTS
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**
** Outputs:
**	none
**	
**	i_qer_p				
**	    .error
**		.err_code		    E_QE0000_OK always
**					    
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	30-apr-89 (carl)
**          modified to save statistics for REGISTER WITH REFRESH if the
**	    LDB table does not come with statistics
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_51_ddb_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;


    if (v_lnk_p->qec_11_ldb_obj_cnt == 1)
    {
	/* last object of LDB is being deleted, forcing LDB's withdrawal
	** from the DDB; hence delete LDB and its capabilities from DDB, 
	** reflect any capability changes in IIDD_DBCAPABILITIES */

	status = qel_52_ldbids(i_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}

	status = qel_53_caps_cats(i_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}

	if (v_lnk_p->qec_10_haves & QEC_04_LONG_LDBNAME)
	{
	    status = qel_57_longnames(i_qer_p, v_lnk_p);
	    if (status)
	    {
		status = qed_u9_log_forgiven(i_qer_p);
		if (status)
		    return(status);
	    }
	}
    }

    if (dds_p->qes_d7_ses_state != QES_4ST_REFRESH)
				    /* always save column mapping info for 
				    ** REFRESH */
    {
	/* here for non-REFRESH */

	status = qel_56_ldb_columns(i_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}
    }

    status = qel_54_tableinfo(i_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return(status);
    }

    status = qel_55_objects(i_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return(status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qel_52_ldbids - delete from IIDD_DDB_LDBIDS
**
** Description:
**	This function deletes the LDB's entry from IIDD_DDB_LDBIDS if
**  the link being deleted is the last one in the DDB.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
** Outputs:
**	none
**
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
*/


DB_STATUS
qel_52_ldbids(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D2_LDBIDS   *ldbids_p = v_lnk_p->qec_7_ldbids_p;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /*  set up for delete operation */ 

    ldbids_p->d2_5_ldb_id = v_lnk_p->qec_2_tableinfo_p->d9_10_ldb_id;

    /*  send DELETE query */ 

    del_p->qeq_c1_can_id = DEL_506_DD_DDB_LDBIDS;
    del_p->qeq_c3_ptr_u.d2_ldbids_p = ldbids_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_53_caps_cats - Delete LDB's entries from IIDD_DDB_LDB_DBCAPS 
**			    and update IIDD_DBCAPABILITIES to reflect any
**			    capability changes.
**
** Description:
**	For a withdrawing LDB, this deletes its entries from catalog and 
**  maintain IIDD_DBCAPABILITIES to reflect the lowest denominators.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**
** Outputs:
**	none
**
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
*/


DB_STATUS
qel_53_caps_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D4_LDB_DBCAPS
		    ldb_dbcaps,
		    *ldbcaps_p = & ldb_dbcaps;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;



    /* 1.  compute the BEFORE-deletion lowest LDB capabilities */

    status = qel_a1_min_cap_lvls(TRUE, i_qer_p, v_lnk_p);
    if (status)
	return(status);

    /* 2.  delete from IIDD_DDB_LDB_DBCAPS */ 

    ldbcaps_p->d4_1_ldb_id = v_lnk_p->qec_2_tableinfo_p->d9_10_ldb_id;

    del_p->qeq_c1_can_id = DEL_508_DD_DDB_LDB_DBCAPS;
    del_p->qeq_c3_ptr_u.d4_ldb_dbcaps_p = ldbcaps_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    if (status) 
	return(status);

    /* 3.  compute the AFTER-deletion lowest LDB capabilities */

    status = qel_a1_min_cap_lvls(FALSE, i_qer_p, v_lnk_p);
    if (status)
	return(status);

    /* 4.  update IIDD_DBCAPABILITIES */

    status = qel_58_dbcapabilities(i_qer_p, v_lnk_p);
    return(status);
}


/*{
** Name: qel_54_tableinfo - Delete from IIDD_DDB_TABLEINFO
**
** Description:
**	This routine deletes the link's entry from IIDD_DDB_TABLEINFO.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**
** Outputs:
**	none
**
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
*/


DB_STATUS
qel_54_tableinfo(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D9_TABLEINFO	
		    *tableinfo_p = v_lnk_p->qec_2_tableinfo_p;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /*  set up for delete operation */ 

    del_p->qeq_c1_can_id = DEL_518_DD_DDB_TABLEINFO;;
    del_p->qeq_c3_ptr_u.d9_tableinfo_p = tableinfo_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_55_objects - Delete link entry from IIDD_DDB_OBJECTS
**
** Description:
**	This routine deletes a link's entry from IIDD_DDB_OBJECTS.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**
** Outputs:
**	none
**
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
*/


DB_STATUS
qel_55_objects(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /* 1.  set up delete information */

    del_p->qeq_c1_can_id = DEL_515_DD_DDB_OBJECTS;
    del_p->qeq_c3_ptr_u.d6_objects_p = objects_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_56_ldb_columns - Delete mapped columns from IIDD_DDB_LDB_COLUMNS
**
** Description:
**	This function deletes the link's mapped entries from above catalog.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**
** Outputs:
**	none
**
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	30-apr-89 (carl)
**          modified to save statistics for REGISTER WITH REFRESH if the
**	    LDB table does not come with statistics
**	15-aug-91 (barbara)
**	    Fixed bug 38919.  Logic was backwards in check for mapped
**	    column names.
*/


DB_STATUS
qel_56_ldb_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D3_LDB_COLUMNS
		    ldb_columns,
		    *ldbcols_p = & ldb_columns;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH)
				    /* always save column mapping info for 
				    ** REFRESH */
	return(E_DB_OK);

    if (v_lnk_p->qec_2_tableinfo_p->d9_9_col_mapped[0] != 'Y'
	&&
	v_lnk_p->qec_2_tableinfo_p->d9_9_col_mapped[0] != 'y')
	return(E_DB_OK);			/* no mapped columns to delete 
						*/
    /* set up for delete */

    STRUCT_ASSIGN_MACRO(
	v_lnk_p->qec_2_tableinfo_p->d9_1_obj_id, 
	ldbcols_p->d3_1_obj_id);

    del_p->qeq_c1_can_id = DEL_507_DD_DDB_LDB_COLUMNS;
    del_p->qeq_c3_ptr_u.d3_ldb_columns_p = ldbcols_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_57_longnames - delete from IIDD_DDB_LONG_LDBNAMES
**
** Description:
**	This routine deletes the LDB's entry from IIDD_DDB_LDBIDS if
**  the link being deleted is the last one in the DDB.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
** Outputs:
**	none
**
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
*/


DB_STATUS
qel_57_longnames(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    QEC_D5_LONG_LDBNAMES
		    *names_p = v_lnk_p->qec_8_longnames_p;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    if (! (v_lnk_p->qec_10_haves & QEC_04_LONG_LDBNAME) )
	return(E_DB_OK);

    names_p->d5_2_ldb_id = tabinfo_p->d9_10_ldb_id;
    del_p->qeq_c1_can_id = DEL_514_DD_DDB_LONG_LDBNAMES;
    del_p->qeq_c3_ptr_u.d5_long_ldbnames_p = names_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_58_dbcapabilities - Adjust IIDD_DBCAPABILITIES to reflect
**				 the lowest denominators in IIDD_DDB_LDB_DBCAPS
**
** Description:
**	For each withdrawing LDB, this routine derives from IIDD_DDB_LDB_DBCAPS 
**  the lowest denominators, compare them  against the precomputed values to
**  determine if they have changed and if so reflect them in 
**  IIDD_DBCAPABILITIES.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**	    .qec_25_pre_mins_p		lowest denominators before removal
**					of capabilities of LDB
**	    .qec_26_aft_mins_p		lowest denominators after removal
**					of capabilities of LDB
** Outputs:
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
*/


DB_STATUS
qel_58_dbcapabilities(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEC_MIN_CAP_LVLS  
		    *pre_mins_p = v_lnk_p->qec_25_pre_mins_p,
		    *aft_mins_p = v_lnk_p->qec_26_aft_mins_p;


    if (v_lnk_p->qec_11_ldb_obj_cnt != 1)
	return(E_DB_OK);			/* LDB not withdrawing from
						** DDB, do nothing */

    /* go thru all five known capabilities */

    if (aft_mins_p->qec_m1_com_sql > pre_mins_p->qec_m1_com_sql)
    {
	status = qel_59_adjust_1_cap(
		    UPD_800_COM_SQL_LVL, 
		    aft_mins_p->qec_m1_com_sql,
		    i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    if (aft_mins_p->qec_m2_ing_quel > pre_mins_p->qec_m2_ing_quel)
    {
	status = qel_59_adjust_1_cap(
		    UPD_801_ING_QUEL_LVL, 
		    aft_mins_p->qec_m2_ing_quel,
		    i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    if (aft_mins_p->qec_m3_ing_sql < pre_mins_p->qec_m3_ing_sql)
    {
	status = qel_59_adjust_1_cap(
		    UPD_802_ING_SQL_LVL, 
		    aft_mins_p->qec_m3_ing_sql,
		    i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    if (aft_mins_p->qec_m4_savepts > pre_mins_p->qec_m4_savepts)
    {
	status = qel_59_adjust_1_cap(
		    UPD_803_SAVEPOINTS, 
		    aft_mins_p->qec_m4_savepts,
		    i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    if (aft_mins_p->qec_m6_opn_sql > pre_mins_p->qec_m6_opn_sql)
    {
	status = qel_59_adjust_1_cap(
		    UPD_805_OPN_SQL_LVL, 
		    aft_mins_p->qec_m6_opn_sql,
		    i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qel_59_adjust_1_cap - Adjsut give capability entry in
**			       IIDD_DBCAPABILITIES
**
** Description:
**	This routine reads specified lowest LDB entry from IIDD_DDB_LDB_DBCAPS
**  and updates IIDD_DBCAPABILITIES with its contents.
**
** Inputs:
**	i_upd_id			UPD_800_MIN_COM_SQL .. 
**					UPD_803_MIN_SAVEPTS
**	i_cap_lvl			capability level of interest
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
** Outputs:
**	none
**
**	i_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	02-sep-93 (barbara)
**	    OPEN/SQL_LEVEL is another capability that adjusts
**	    iidd_dbcapabilities.
*/


DB_STATUS
qel_59_adjust_1_cap(
i4		i_upd_id,
i4		i_cap_lvl,
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *upd_p = v_lnk_p->qec_23_update_p;

    QEC_D4_LDB_DBCAPS
		    ldb_dbcaps,
		    *ldbcaps_p = & ldb_dbcaps;
    QEC_L4_DBCAPABILITIES
		    ddb_dbcaps,
		    *ddbcaps_p = & ddb_dbcaps;

    

    /* 1.  determine what capability to select */

    switch (i_upd_id)
    {
    case UPD_800_COM_SQL_LVL:
	sel_p->qeq_c1_can_id = SEL_205_COM_SQL_LVL;
	break;
    case UPD_801_ING_QUEL_LVL:
	sel_p->qeq_c1_can_id = SEL_206_ING_QUEL_LVL;
	break;
    case UPD_802_ING_SQL_LVL:
	sel_p->qeq_c1_can_id = SEL_207_ING_SQL_LVL;
	break;
    case UPD_803_SAVEPOINTS:
	sel_p->qeq_c1_can_id = SEL_208_SAVEPOINTS;
	break;
    case UPD_805_OPN_SQL_LVL:
	sel_p->qeq_c1_can_id = SEL_211_OPN_SQL_LVL;
	break;
    default:
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    /* 2.  retrieve tuple data from IIDD_DDB_LDB_DBCAPS */

    ldbcaps_p->d4_4_cap_lvl = i_cap_lvl;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.d4_ldb_dbcaps_p = ldbcaps_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	/* must flush */

	status = qel_s3_flush(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 3.  transfer tuple data into structure for updating IIDD_DBCAPABIITIES */

    STcopy(ldbcaps_p->d4_2_cap_cap, ddbcaps_p->l4_1_cap_cap);
    STcopy(ldbcaps_p->d4_3_cap_val, ddbcaps_p->l4_2_cap_val);

    upd_p->qeq_c1_can_id = i_upd_id;
    upd_p->qeq_c3_ptr_u.l4_dbcapabilities_p = ddbcaps_p;
    upd_p->qeq_c4_ldb_p = cdb_p;
    status = qel_u1_update(i_qer_p, v_lnk_p);
    return(status);
}    

