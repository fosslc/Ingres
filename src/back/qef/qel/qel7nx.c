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
**  Name: QEL7NX.C - Routines deleting from INDEX catalogs for DESTROY LINK
**
**  Description:
**      These routines perform INDEX catalog deletion for DESTROY LINK.
**
**	    qel_71_ndx_cats	- perform deletion on internal INDEX catalogs
**	    qel_72_xcolumns	- delete from IIDD_INDEX_COLUMNS
**	    qel_73_indexes	- delete from IIDD_INDEXES
**	    qel_74_ndx_lnks	- delete all indexes of link
**	    qel_75_ndx_ids	- retrieve all index ids of link
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog redefinition
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
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*{
** Name: qel_71_ndx_cats - Perform deletion on internal INDEX catalogs.
**
** Description:
**	This function does all deletion on the internal INDEX catalogs using 
**  supplied information.
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
**		.err_code		    E_QE0000_OK
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
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
[@history_line@]...
*/


DB_STATUS
qel_71_ndx_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEC_INDEX_ID    *cur_xid_p;
    i4		    i;				/* loop control variable */


    /* 1.  for each index, promote into IIDD_INGRES_TABLES,
    **	   IIDD_PHYSICAL_TABLES, and IIDD_INDEX_COLUMNS */

    if (v_lnk_p->qec_15_ndx_cnt == 0)
	return(E_DB_OK);			/* local table has no indexes */

    cur_xid_p = v_lnk_p->qec_16_ndx_ids_p;
    for (i = 0; i < v_lnk_p->qec_15_ndx_cnt; i++, cur_xid_p++)
    {
	status = qel_72_xcolumns(i_qer_p, v_lnk_p, cur_xid_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}
    }

    /* 2.  delete from IIDD_INDEXES */

    status = qel_73_indexes(i_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return(status);
    }

    status = qel_74_ndx_lnks(i_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return(status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qel_72_xcolumns - Delete a local index's infomation from 
**			   IIDD_INDEX_COLUMNS 
**
** Description:
**	This function deletes entries from IIDD_INDEX_COLUMNS.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	i_id_p			    ptr to QEC_INDEX_ID
**
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
[@history_line@]...
*/


DB_STATUS
qel_72_xcolumns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
QEC_INDEX_ID	*i_id_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L7_INDEX_COLUMNS
		    xcol,
		    *xcol_p = & xcol;	/* tuple struct */
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /* 1.  set up for for deleting from IIDD_DDB_LDB_X_COLUMNs */

    qed_u0_trimtail( i_id_p->qec_i1_name, (u_i4) DB_TAB_MAXNAME,
		    xcol_p->l7_1_ind_name);
    qed_u0_trimtail( i_id_p->qec_i2_owner, (u_i4) DB_OWN_MAXNAME,
		    xcol_p->l7_2_ind_owner);

    del_p->qeq_c1_can_id = DEL_522_DD_INDEX_COLUMNS;
    del_p->qeq_c3_ptr_u.l7_index_columns_p = xcol_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send SELECT query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_73_indexes - Delete index information from IIDD_INDEXES 
**
** Description:
**	This routine deletes all index entries from above catalog.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
** Outputs:
**	None
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
[@history_line@]...
*/


DB_STATUS
qel_73_indexes(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L6_INDEXES  indexes,
		    *indexes_p = & indexes;	/* tuple struct */
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /* 1.  set up */

    qed_u0_trimtail( objects_p->d6_1_obj_name, (u_i4) DB_OBJ_MAXNAME,
		indexes_p->l6_4_base_name);

    qed_u0_trimtail( objects_p->d6_2_obj_owner, (u_i4) DB_OWN_MAXNAME,
		indexes_p->l6_5_base_owner);

    del_p->qeq_c1_can_id = DEL_521_DD_INDEXES;
    del_p->qeq_c3_ptr_u.l6_indexes_p = indexes_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_74_ndx_lnks - Delete all indexes of link 
**
** Description:
**	This function retrieves the information of each index and uses
**  object id to do the deletion.
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
**	24-oct-88 (carl)
**          written
[@history_line@]...
*/


DB_STATUS
qel_74_ndx_lnks(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEC_INDEX_ID    *xid_p = v_lnk_p->qec_16_ndx_ids_p;
    DB_TAB_ID	    ndxid[QEK_0MAX_NDX_COUNT],
		    *ndx_id_p = ndxid;
    QEF_RCB	    qer,
		    *qer_p = & qer;
    QED_DDL_INFO    *ddl_p = & qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    i4		    i;				/* loop control variable */


    if (v_lnk_p->qec_15_ndx_cnt == 0)
	return(E_DB_OK);			/* link has no indexes */

    STRUCT_ASSIGN_MACRO(*i_qer_p, qer);

    /* 1.  retrieve all index ids */

    status = qel_75_ndx_ids(i_qer_p, v_lnk_p, xid_p, ndx_id_p);
    if (status)
	return(status);

    /* 2.  drop each index as link */

    for (i = 0; i < v_lnk_p->qec_15_ndx_cnt; i++, ndx_id_p++)
    {
	
	/* 3.  provide id for destroying index */

	STRUCT_ASSIGN_MACRO(*ndx_id_p, ddl_p->qed_d7_obj_id);

	status = qel_d0_des_lnk(qer_p);		/* recursive call */
	if (status)
	    return(status);

    }

    return(status);
}    


/*{
** Name: qel_75_ndx_ids - Retrieve ids of all indexes
**
** Description:
**      This routine retrieves from IIDD_DDB_OBJECTS link ids of all indexes.
**
** Inputs:
**	    i_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
**	    v_lnk_p			QEC_LINK
**	    i_xid_p			ptr to array of QEC_INDEX_IDs
** Outputs:
**	    o_ndx_id_p			ptr to array of DB_TAB_ID
**					link ids of indexes
**	    i_qer_p
**		.error
**		    .err_code		E_QE0002_INTERNAL_ERROR
**					(to be specified)
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-oct-88 (carl)
**          written
[@history_template@]...
*/


DB_STATUS
qel_75_ndx_ids(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p,
QEC_INDEX_ID	    *i_xid_p,
DB_TAB_ID	    *o_ndx_id_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS  obj,
		    *obj_p = & obj;
    QEC_LINK	    link,
		    *link_p = & link;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    QEC_INDEX_ID    *xid_p = i_xid_p;
    DB_TAB_ID	    *ndx_id_p = o_ndx_id_p;
    i4		    i;				/* loop control variable */


    if (v_lnk_p->qec_15_ndx_cnt == 0)
	return(E_DB_OK);

    STRUCT_ASSIGN_MACRO(*v_lnk_p, link);
    link_p->qec_6_select_p = sel_p;

    /* 1.  set up for retrieve IIDD_DDB_OBJECTS */

    sel_p->qeq_c1_can_id = SEL_015_DD_DDB_OBJECTS_BY_NAME;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.d6_objects_p = obj_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    for (i = 0; i < v_lnk_p->qec_15_ndx_cnt; i++, xid_p++, ndx_id_p++)
    {
	/* 2.  provide object name and owner as keying information */

	STcopy(xid_p->qec_i1_name, obj_p->d6_1_obj_name);
	STcopy(xid_p->qec_i2_owner, obj_p->d6_2_obj_owner);

	/* 3.  retrieve id of index */

	status = qel_s4_prepare(i_qer_p, v_lnk_p);
	if (status)
	    return(status);

	STRUCT_ASSIGN_MACRO(obj_p->d6_3_obj_id, *ndx_id_p);

	/* there should only be 1 tuple */

	if (! sel_p->qeq_c5_eod_b)
	{	
	    status = qel_s3_flush(i_qer_p, v_lnk_p);
	    if (status)
		return(status);
	}
    }

    return(E_DB_OK);
}

