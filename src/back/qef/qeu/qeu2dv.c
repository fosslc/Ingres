/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <qefkon.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefptr.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEU2DV.C - Routine processing DROP/DESTROY VIEW
**
**  Description:
**      This file contains routines for processing DROP VIEW statements.
**
**	    qeu_20_des_view	    - process DESTROY/DROP VIEW statement
**	    qeu_21_des_depends	    - destroy all dependent views
**	    qeu_22_sel_dbdepends    - retrieve from IIDD_DDB_DBDDEPENDS
**	    qeu_23_des_a_view	    - destroy a view
**	    qeu_24_view_cats	    - delete from view catalogs
**	    qeu_25_norm_cats	    - delete from normal catalogs
**	    qeu_26_tree		    - delete from IIDD_DDB_TREE
**	    qeu_27_views	    - delete from IIDD_VIEWS
**	    qeu_28_dbdepends	    - delete from IIDD_DDB_DBDDEPENDS
**
**  History:    $Log-for RCS$
**	10-nov-88 (carl)
**          written
**	12-aug-90 (carl)
**	    removed references to IIQE_c21_qef_rcb and IIQE_c41_link 
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> for CREATE SCHEMA.
**      14-sep-93 (smc)
**	    Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/


/*{
** Name: qeu_20_des_view - destroy a view definition
**
** Description:
**	This routine destroys a view and all the dependent views.
**  Note only one view can be destroyed at a time.
**
**  View destruction is cascading in the sense that all dependent views are
**  first destroyed before the current one.
**
** Inputs:
**      i_qer_p			QEF_RCB
**	    .qef_r3_ddb_req
**		.qer_d7_ddl_info
**		    .qed_d7_obj_id
**				view's id
** Outputs:
**      i_qer_p			QEF_RCB
**	    .error.err_code	one of the following
**				E_QE0002_INTERNAL_ERROR
**				(to be specified)
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-nov-88 (carl)
**	    written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qeu_20_des_view(
QEF_RCB         *i_qer_p)
{
    DB_STATUS		status;
    QED_DDL_INFO	*ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
/*
    QES_DDB_SES		*dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
*/
    QEC_L16_TABLES	tables;		    /* used for IITABLES-style info */
    QEC_D6_OBJECTS	objects;	    /* used for object info */
    QEC_LINK		link,		    /* used as working variable */
			*lnk_p = & link;
    QEQ_1CAN_QRY	del,
			sel,
			upd;		    /* used for ordering canned query */
    RQB_BIND		rq_bind[QEC_CAT_COL_COUNT_MAX + 1];


    /* 1.  set up control structure */

    MEfill(sizeof(link), '\0', (PTR) & link);
    lnk_p->qec_1_ddl_info_p = ddl_p;
    MEfill(DB_MAXNAME, ' ', lnk_p->qec_5_ldb_alias);
    lnk_p->qec_6_select_p = & sel;
    lnk_p->qec_9_tables_p = & tables;
    lnk_p->qec_13_objects_p = & objects;
    lnk_p->qec_20_rqf_bind_p = rq_bind;
    lnk_p->qec_21_delete_p = & del;
    lnk_p->qec_23_update_p = & upd;
/*
    status = qed_u8_gmt_now(i_qer_p, lnk_p->qec_24_cur_time);
    if (status)
	return(status);
    lnk_p->qec_24_cur_time[DD_25_DATE_SIZE] = EOS;
*/    
    /* 2.  retrieve view's id, name and owner */

    lnk_p->qec_1_ddl_info_p = ddl_p;
    status = qel_d3_objects(i_qer_p, lnk_p);
    if (status)
    {
	i_qer_p->qef_cb->qef_abort = TRUE;
	return(status);				/* cannot continue */
    }

    if (lnk_p->qec_27_select_cnt <= 0)
    {
	/* the view no longer exists possibly a casualty of cascading 
	** delete, hence do forgiving return */

	return(E_DB_OK);
    }

    /* 3.  destroy all dependent views if any */

    status = qeu_21_des_depends(i_qer_p, lnk_p);
    if (status)
    {
	/* assume ok to forgive */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    /* 4.  destroy given view */	

    status = qeu_23_des_a_view(i_qer_p, & ddl_p->qed_d7_obj_id);
    if (status)
    {
	/* assume ok to forgive */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qeu_21_des_depends - destroy all dependent views
**
** Description:
**	This routine destroys all the views dependent on given object: link,
**  table or view.
**
** Inputs:
**      i_qer_p			QEF_RCB
**	    .qef_r3_ddb_req
**		.qer_d7_ddl_info
**		    .qed_d7_obj_id
**				object's id
**	v_lnk_p			QEC_LINK
**
** Outputs:
**      i_qer_p			QEF_RCB
**	    .error.err_code	one of the following
**				E_QE0002_INTERNAL_ERROR
**				(to be specified)
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	03-nov-88 (carl)
**	    written
*/


DB_STATUS
qeu_21_des_depends(
QEF_RCB         *i_qer_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS		status;
/*
    QES_DDB_SES		*dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO	*ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
*/
    QEF_RCB		qer,			/* QEF_RCB for destroying 
						** dependent views */
			*qer_p = & qer;
    DB_TAB_ID		vids[QEK_1MAX_DEP_COUNT + 1],
			*vids_p;
    i4			i, dep_cnt;
    bool		done;


    MEfill(sizeof(qer), '\0', (PTR) & qer);
    qer_p->qef_cb = i_qer_p->qef_cb;

    /* 1.  loop until all dependent views are destroyed */

    done = FALSE;
    while (! done)
    {
	/* 1.1  retrieve a batch of at most 30 dependent view ids from 
	** IIDD_DDB_DBDEPENDS and flush the association for the next 
	** batch retrieval */

	status = qeu_22_sel_dbdepends(i_qer_p, v_lnk_p, & dep_cnt, vids);
	if (status)
	{
	    /* already in update phase, abort */

	    i_qer_p->qef_cb->qef_abort = TRUE;
	    return(status);
	}

	if (dep_cnt > QEK_1MAX_DEP_COUNT)
	{
	    status = qed_u1_gen_interr(& i_qer_p->error);
	    return(status);
	}
	else if (dep_cnt < QEK_1MAX_DEP_COUNT)
	    done = TRUE;

	/* 1.2  destroy each dependent view */

	for (i = 0, vids_p = vids; i < dep_cnt; i++, vids_p++)
	{
	    STRUCT_ASSIGN_MACRO(*vids_p,
		qer_p->qef_r3_ddb_req.qer_d7_ddl_info.qed_d7_obj_id);

	    status = qeu_20_des_view(qer_p);
	    if (status)
	    {
		/* assume ok to forgive */

		status = qed_u9_log_forgiven(i_qer_p);
		if (status)
		return (status);
	    }
	}
    }

    return(E_DB_OK);
}    


/*{
** Name: qeu_22_sel_dbddpends - Retrieve from IIDD_DDB_DBDEPENDS
**
** Description:
**      This routine retrieves information of dependent views from above 
**  catalog, at most 30 at a time.
**
** Inputs:
**	    i_qer_p			QEF_RCB
**	    v_lnk_p			QEC_LINK
**
** Outputs:
**	    o_cnt_p			count of dependent views
**	    o_vids_p			their ids
**		
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
**      10-nov-88 (carl)
**          written
[@history_template@]...
*/


DB_STATUS
qeu_22_sel_dbdepends(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p,
i4		    *o_cnt_p,
DB_TAB_ID	    *o_vids_p)
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
    QEQ_1CAN_QRY	*sel_p = v_lnk_p->qec_6_select_p;
    QEC_D6_OBJECTS	*objects_p = v_lnk_p->qec_13_objects_p;
    QEC_D1_DBDEPENDS	dbdepends,
			*dep_p = & dbdepends;
    DB_TAB_ID		*vid_p = o_vids_p;
    i4			vid_cnt = 0;


    *o_cnt_p = 0;				/* assume */

    /* 1.  set up select criteria: independent-view id */

    dep_p->d1_1_inid1 = objects_p->d6_3_obj_id.db_tab_base;
    dep_p->d1_2_inid2 = objects_p->d6_3_obj_id.db_tab_index;

    sel_p->qeq_c1_can_id = SEL_009_DD_DDB_DBDEPENDS_BY_IN;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.d1_dbdepends_p = dep_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    /* 2.  send SELECT query and fetch first tuple */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);
        
    if (sel_p->qeq_c5_eod_b)
	return(E_DB_OK);

    while (! sel_p->qeq_c5_eod_b && ! status)
    {
	if (dep_p->d1_6_dtype == DB_VIEW)
	{
	    /* 3.  save id of current dependent view */

	    vid_p->db_tab_base = dep_p->d1_4_deid1;
	    vid_p->db_tab_index = dep_p->d1_5_deid2;

	    vid_cnt++;
	    vid_p++;				/* advance to next slot */
	}

	/* 4.  retrieve next tuple from catalog */

	sel_p->qeq_c4_ldb_p = cdb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    v_lnk_p->qec_27_select_cnt = vid_cnt;	/* save independent-view 
						** count */
    *o_cnt_p = vid_cnt;
    return(E_DB_OK);
}


/*{
** Name: qeu_23_des_a_view - destroy a view definition
**
** Description:
**	This routine destroys a single view that is assumed to have no
**  dependent views.
**
** Assumption:
**	TPF has been informed of imminent CDB updates.
**
** Inputs:
**      i_qer_p			QEF_RCB
**      i_vid_p			DB_TAB_ID
**
** Outputs:
**      i_qer_p
**	    .error.err_code	one of the following
**				E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-nov-88 (carl)
**	    written
*/


DB_STATUS
qeu_23_des_a_view(
QEF_RCB		    *i_qer_p,
DB_TAB_ID	    *i_vid_p)
{
    DB_STATUS		status;
    QED_DDL_INFO	*ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
/*
    QES_DDB_SES		*dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
*/
    QEC_L16_TABLES	tables;		    /* used for IITABLES-style info */
    QEC_D6_OBJECTS	objects;	    /* used for object info */
    QEC_LINK		link,		    /* used as global control block */
			*lnk_p = & link;
    QEQ_1CAN_QRY	del,
			sel,
			upd;		    /* used for ordering canned query */
    RQB_BIND		rq_bind[QEC_CAT_COL_COUNT_MAX + 1];


    /* 1.  initialize control structure */

    sel.qeq_c2_rqf_bind_p = rq_bind;		/* must set up */
    del.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;
    upd.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;
    
    MEfill(sizeof(link), '\0', (PTR) & link);
    lnk_p->qec_1_ddl_info_p = ddl_p;
    MEfill(DB_MAXNAME, ' ', lnk_p->qec_5_ldb_alias);
    lnk_p->qec_6_select_p = & sel;
    lnk_p->qec_9_tables_p = & tables;
    lnk_p->qec_13_objects_p = & objects;
    lnk_p->qec_20_rqf_bind_p = rq_bind;
    lnk_p->qec_21_delete_p = & del;
    lnk_p->qec_23_update_p = & upd;

    status = qed_u8_gmt_now(i_qer_p, lnk_p->qec_24_cur_time);
    if (status)
	return(status);
    lnk_p->qec_24_cur_time[DD_25_DATE_SIZE] = EOS;
    
    /* 2.  retrieve infomation from IIDD_DDB_OBJECTS */

    STRUCT_ASSIGN_MACRO(*i_vid_p, objects.d6_3_obj_id);

    status = qel_d3_objects(i_qer_p, lnk_p);
    if (status)
	return(status);				/* ok to return at this point */

    if (lnk_p->qec_27_select_cnt <= 0)
    {
	/* the view no longer exists possibly a casualty of cascading 
	** delete, hence do forgiving return */

	return(E_DB_OK);
    }

    /* 3.  delete from view catalogs */	

    status = qeu_24_view_cats(i_qer_p, lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    /* 4.  delete from non-view catalogs */	

    status = qeu_25_norm_cats(i_qer_p, lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }


    return(E_DB_OK);
}    


/*{
** Name: qeu_24_view_cats - Delete from view catalogs.
**
** Description:
**	This routine deletes from view catalogs for a view.
**
** Assumption:
**	TPF has been informed of the imminent CDB updates.
**
** Inputs:
**      i_qer_p			QEF_RCB
**      v_lnk_p			QEC_LINK
**
** Outputs:
**      i_qer_p
**	    .error.err_code	E_QE0000_OK
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-nov-88 (carl)
**	    written
*/


DB_STATUS
qeu_24_view_cats(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p)
{
    DB_STATUS		status;
/*
    QES_DDB_SES		*dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO	*ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
*/


    /* 1.  delete from IIDD_DDB_TREE */	

    status = qeu_26_tree(i_qer_p, v_lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    /* 2.  delete from IIDD_VIEWS */	

    status = qeu_27_views(i_qer_p, v_lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    /* 3.  delete from IIDD_DDB_DBDEPENDS */

    status = qeu_28_dbdepends(i_qer_p, v_lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qeu_25_norm_cats - Delete from normal non-view catalogs.
**
** Description:
**	This routine deletes from non-view catalogs for a view.
**
** Assumption:
**	TPF has been informed of the imminent CDB updates.
**
** Inputs:
**      i_qer_p			QEF_RCB
**      v_lnk_p			QEC_LINK
**
** Outputs:
**      i_qer_p
**	    .error.err_code	E_QE0000_OK
**	Returns:
**	    E_DB_OK
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	10-nov-88 (carl)
**	    written
*/


DB_STATUS
qeu_25_norm_cats(
QEF_RCB		    *i_qer_p,
QEC_LINK    	    *v_lnk_p)
{
    DB_STATUS		status;
/*
    QES_DDB_SES		*dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO	*ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
*/

    /* 1.  delete from IIDD_COLUMNS */	

    status = qel_83_columns(i_qer_p, v_lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    /* 2.  delete from IIDD_IITABLES */	

    status = qel_82_tables(i_qer_p, v_lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    /* 3.  delete from IIDD_DDB_OBJECTS */	

    status = qel_55_objects(i_qer_p, v_lnk_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return (status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qeu_26_tree - delete from IIDD_DDB_TREE
**
** Description:
**	This function deletes from above catalog for c view.
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
**		.err_code		E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	10-nov-88 (carl)
**          written
*/


DB_STATUS
qeu_26_tree(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEC_D10_TREE    tree,
		    *tree_p = & tree;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /*  set up for delete operation */ 

    tree_p->d10_1_treetabbase = objects_p->d6_3_obj_id.db_tab_base;
    tree_p->d10_2_treetabidx = objects_p->d6_3_obj_id.db_tab_index;

    /*  send DELETE query */ 

    del_p->qeq_c1_can_id = DEL_519_DD_DDB_TREE;
    del_p->qeq_c3_ptr_u.d10_tree_p = tree_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qeu_27_views - delete from IIDD_VIEWS 
**
** Description:
**	This function deletes from above catalog for a view.
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
**		.err_code		E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	10-nov-88 (carl)
**          written
*/


DB_STATUS
qeu_27_views(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEC_L17_VIEWS   views,	
		    *views_p = & views;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /*  set up for delete operation */ 

    STcopy(objects_p->d6_1_obj_name, views_p->l17_1_tab_name);
    STcopy(objects_p->d6_2_obj_owner, views_p->l17_2_tab_owner);

    /*  send DELETE query */ 

    del_p->qeq_c1_can_id = DEL_532_DD_VIEWS;
    del_p->qeq_c3_ptr_u.l17_views_p = views_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qeu_28_dbdepends - delete from IIDD_DDB_DBDEPENDS
**
** Description:
**	This function deletes the given dependent view id from above catalog.
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
**		.err_code		E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	10-nov-88 (carl)
**          written
*/


DB_STATUS
qeu_28_dbdepends(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEC_D1_DBDEPENDS
		    dbdepends,	
		    *dep_p = & dbdepends;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /*  set up for delete operation */ 

    dep_p->d1_4_deid1 = objects_p->d6_3_obj_id.db_tab_base;
    dep_p->d1_5_deid2 = objects_p->d6_3_obj_id.db_tab_index;

    /*  send DELETE query */ 

    del_p->qeq_c1_can_id = DEL_505_DD_DDB_DBDEPENDS;
    del_p->qeq_c3_ptr_u.d1_dbdepends_p = dep_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    
