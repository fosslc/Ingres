/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <adf.h>
#include    <ade.h>
#include    <dudbms.h>
#include    <cui.h>
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
#include    <cv.h>
#include    <me.h>
#include    <st.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEL2NX.C - Routines updating INDEX catalogs for CREATE LINK
**
**  Description:
**      These routines perform INDEX catalog updates for CREATE LINK.
**
**	    qel_21_ndx_cats	- perform updates on internal INDEX catalogs
**	    qel_22_indexes	- promote info to IIDD_INDEXES
**	    qel_23_ndx_cols	- promote info to IIDD_INDEX_COLUMNS
**	    qel_24_ndx_lnks	- create link for each index
**	    qel_25_lcl_cols	- retrieve LDB column names of index
**	    qel_26_ndx_name	- generate link name for index
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	15-aug-1991 (barbara)
**	    Fixed bugs 38918 and 38921.
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-mar-95 (rudtr01)
**	    Fix bug 67496.  For catalog indices use iiddx when
**	    assembling the registered object name.
**	    see qel_26_ndx_name()
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**      09-jan-2009 (stial01)
**          Fix buffers that are dependent on DB_MAXNAME
**/

/*  Forward reference */


/*{
** Name: qel_21_ndx_cats - Promote information into IIDD_INDEXES and
**			   IIDD_INDEX_COLUMNS.
** Description:
**	This function does all updates on the internal INDEX catalogs using 
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
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_21_ndx_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS		status;
    DD_2LDB_TAB_INFO	*tabinfo_p = 
			    v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p;
    QEC_INDEX_ID	*cur_xid_p;
    i4			i;			/* loop control variable */


    if ((tabinfo_p->dd_t3_tab_type == DD_3OBJ_VIEW)
						/* view has no indices */
	||
	(tabinfo_p->dd_t3_tab_type == DD_4OBJ_INDEX)
						/* index has no indices */
	||
	! (v_lnk_p->qec_10_haves & QEC_03_INGRES))
						/* no gateway indices yet */
	return(E_DB_OK);

    /* 1.  promote into IIDD_INDEXES */

    status = qel_22_indexes(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    /* 2.  for each index, promote information from the LDB to the CDB catalog
    **	   IIDD_INDEX_COLUMNS */

    if (v_lnk_p->qec_15_ndx_cnt == 0)
	return(E_DB_OK);			/* local table has no indexes */

    if (v_lnk_p->qec_15_ndx_cnt > QEK_0MAX_NDX_COUNT)
    {
	/* out of range */

	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    cur_xid_p = v_lnk_p->qec_16_ndx_ids_p;
    for (i = 0; i < v_lnk_p->qec_15_ndx_cnt; i++, cur_xid_p++)
    {
	/* 3.  promote the II_INDEX_COLUMNS entries of each LDB index */

	status = qel_23_ndx_cols(i_qer_p, v_lnk_p, cur_xid_p);
	if (status)
	    return(status);
    }

    status = qel_24_ndx_lnks(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_22_indexes - Promote index information of local table into
**			  IIDD_INDEXES.
**
** Description:
**	This routine retrieves from the LDB and inserts the data into above 
**  table under the link name and owner.  
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	v_ids_p			    ptr to array of QEC_INDEX_ID
**
** Outputs:
**	v_lnk_p
**	    .qec_4_col_cnt	    number of indexes of table
**	v_ids_p			    index ids retrieved 
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
qel_22_indexes(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *dd_info_p = ddl_p->qed_d6_tab_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_L6_INDEXES  indexes,
		    *indexes_p = & indexes;	/* tuple struct */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p;
    QEC_INDEX_ID    *xid_p = v_lnk_p->qec_16_ndx_ids_p;


    /* 1.  set up for looping through each retrieved IIINDEXES tuple */

    v_lnk_p->qec_15_ndx_cnt = 0;

    qed_u0_trimtail(
		    dd_info_p->dd_t1_tab_name,
		    (u_i4) DB_MAXNAME,
		    indexes_p->l6_4_base_name);
    qed_u0_trimtail(
		    dd_info_p->dd_t2_tab_owner,
		    (u_i4) DB_MAXNAME,
		    indexes_p->l6_5_base_owner);

    sel_p->qeq_c1_can_id = SEL_107_II_INDEXES;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l6_indexes_p = indexes_p;
    sel_p->qeq_c4_ldb_p = ldb_p;

    /* 2.  send SELECT query */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (sel_p->qeq_c5_eod_b)
	return(E_DB_OK);

    v_lnk_p->qec_10_haves |= QEC_02_INDEXES;

    while (! sel_p->qeq_c5_eod_b && ! status)
    {
	/* 3.  save LDB index name and owner */

	MEcopy(
	    indexes_p->l6_1_ind_name,
	    sizeof(DD_NAME), 
	    xid_p->qec_i1_name);

	MEcopy(
	    indexes_p->l6_2_ind_owner,
	    sizeof(DD_NAME), 
	    xid_p->qec_i2_owner);

	/* 4.  use link name and link owner as base identification */

	qed_u0_trimtail(
			ddl_p->qed_d1_obj_name,
			(u_i4) DB_MAXNAME,
			indexes_p->l6_4_base_name);
	qed_u0_trimtail(
			ddl_p->qed_d2_obj_owner,
			(u_i4) DB_MAXNAME,
			indexes_p->l6_5_base_owner);

	/* 5.  generate link name for index */

	qel_26_ndx_name(i_qer_p, v_lnk_p, v_lnk_p->qec_15_ndx_cnt + 1,
		    ( DD_NAME * ) xid_p->qec_i3_given);

	MEcopy(xid_p->qec_i3_given, sizeof(DD_NAME), 
	    indexes_p->l6_1_ind_name);
	MEcopy(ddl_p->qed_d2_obj_owner, sizeof(DD_NAME), 
	    indexes_p->l6_2_ind_owner);

	/* 6.  insert into IIDD_INDEXES */

	ins_p->qeq_c1_can_id = INS_621_DD_INDEXES;
	ins_p->qeq_c3_ptr_u.l6_indexes_p = indexes_p;
	ins_p->qeq_c4_ldb_p = cdb_p;

	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	{
	    /* must flush data stream before returning */

	    STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
	    sel_p->qeq_c4_ldb_p = ldb_p;
	    ignore = qel_s3_flush(i_qer_p, v_lnk_p);
	    STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
	    return(status);
	}

	v_lnk_p->qec_15_ndx_cnt++;		/* increment index count */
	xid_p++;				/* pt to next slot */

	/* 6.  read next tuple from LDB */

	sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    return(status);
}    


/*{
** Name: qel_23_ndx_cols - Insert to IIDD_INDEX_COLUMNS for a local index.
**
** Description:
**	This function inserts entries into IIDD_COLUMNS for the link and
**  in the event of mapping the local column names into above catalog.
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
**	01-mar-93 (barbara)
**	    Delimited id support.  Always assume there are mapped column
**	    names.  Even if user didn't specify a mapping, PSF will provide
**	    a mapping in case of incompatible case translation semantics
**	    between DDB and LDB.
**	01-jun-93 (barbara)
**	    Removed the logic of the last change.  QEF, not PSF, performs
**	    case translation.  (There were some cases that PSF could not
**	    catch.)
*/


DB_STATUS
qel_23_ndx_cols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
QEC_INDEX_ID	*i_xid_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *tabinfo_p = ddl_p->qed_d6_tab_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_L7_INDEX_COLUMNS
		    xcol,
		    *xcol_p = & xcol;	/* tuple struct */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p;
    i4		    tupcnt;
    bool	    found_b;
    DD_COLUMN_DESC  *from_p;


    /* 1.  use local index name to retrieve each II_INDEX_COLUMN column */

    qed_u0_trimtail(
		    i_xid_p->qec_i1_name,
		    (u_i4) DB_MAXNAME,
		    xcol_p->l7_1_ind_name);
    qed_u0_trimtail(
		    i_xid_p->qec_i2_owner,
		    (u_i4) DB_MAXNAME,
		    xcol_p->l7_2_ind_owner);

    sel_p->qeq_c1_can_id = SEL_108_II_INDEX_COLUMNS;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l7_index_columns_p = xcol_p;
    sel_p->qeq_c4_ldb_p = ldb_p;

    /* 2.  send SELECT query */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (sel_p->qeq_c5_eod_b)
    {
	/* no data ! */

	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    tupcnt  = 0;

    while (! sel_p->qeq_c5_eod_b && ! status)
    {
	/* 4.  if mapping columns, use link column name */

	if (tabinfo_p->dd_t6_mapped_b)
	{
	    found_b = qel_a0_map_col(i_qer_p, v_lnk_p, xcol_p->l7_3_col_name,
							& from_p);
	    if (! found_b)			/* should never happen */
	    {
		status = qed_u1_gen_interr(& i_qer_p->error);
		STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
		ignore = qel_s3_flush(i_qer_p, v_lnk_p);
		STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
		return(status);
	    }
	    /* replace with local column name */

	    MEcopy(from_p->dd_c1_col_name, sizeof(DD_NAME), 
		    xcol_p->l7_3_col_name);
	}
	else
	{
	    /* May need to massage case of LDB column name */
	    qel_a2_case_col(i_qer_p, xcol_p->l7_3_col_name, 
			    *i_qer_p->qef_cb->qef_dbxlate,
			    &tabinfo_p->dd_t9_ldb_p->dd_i2_ldb_plus.
							dd_p3_ldb_caps);
	}

	/* 5.  insert into IIDD_INDEX_COLUMNS */

	if (! status)
	{
	    /* use generated index name and link owner for insertion */

	    qed_u0_trimtail(
		    i_xid_p->qec_i3_given,
		    (u_i4) DB_MAXNAME,
		    xcol_p->l7_1_ind_name);

	    qed_u0_trimtail(
		    ddl_p->qed_d2_obj_owner,
		    (u_i4) DB_MAXNAME,
		    xcol_p->l7_2_ind_owner);

	    ins_p->qeq_c1_can_id = INS_622_DD_INDEX_COLUMNS;
	    ins_p->qeq_c3_ptr_u.l7_index_columns_p = xcol_p;
	    ins_p->qeq_c4_ldb_p = cdb_p;

	    status = qel_i1_insert(i_qer_p, v_lnk_p);
	    if (status)
	    {
	        /* 6.  must flush sending LDB */

		STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
		ignore = qel_s3_flush(i_qer_p, v_lnk_p);
		STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
		return(status);
	    }
	}

	tupcnt++;

	/* 6.  retrieve next tuple */

	sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    return(status);
}    


/*{
** Name: qel_24_ndx_lnks - Create link for each index.
**
** Description:
**	This function retrieves the information of all indexes and create
**  a link on each.
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
**	08-jun-88 (carl)
**          written
**	15-aug-91 (barbara)
**	    Fixed bugs 38918, 38921.  Set up arrays of pointers to 
**	    DD_COLUMN_DESC structs starting from array element 1, not 0 
**	    (otherwise qel_33_columns doesn't pick up all columns).  Declare 
**	    an extra DD_COLUMN_DESC for tidp column.
**	27-oct-91 (barbara)
**	    When comparing column names to "tidp" allow for upper case
**	    version of that column. (bug 56236)
*/


DB_STATUS
qel_24_ndx_lnks(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS		status;
    QEC_INDEX_ID	*xid_p;
    i4			i,
			j;			/* loop control variables */
    bool		found_b;
    QEF_RCB		lnk_rcb,
			*lnkrcb_p = & lnk_rcb;
    DD_2LDB_TAB_INFO	tab_info,
			*tab_p = & tab_info;
    QED_DDL_INFO	*ddl_p = & lnkrcb_p->qef_r3_ddb_req.qer_d7_ddl_info,
		    	*i_ddl_p = v_lnk_p->qec_1_ddl_info_p;
    QED_QUERY_INFO	*reg_p;			/* for saving and restoring 
						** iiregistration query info */
    QEC_D9_TABLEINFO	*i_tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    DD_COLUMN_DESC	ddbcols[DB_MAXKEYS+1],
			*dcols_p[DB_MAXKEYS+2],
			**ddb_cols_pp = dcols_p,
			*from_p,
			*to_p;
    DD_COLUMN_DESC	ldbcols[DB_MAXKEYS+1],
			*l_cid_p,
			*lcols_p[DB_MAXKEYS+2],
			**ldb_cols_pp = lcols_p;


    if (v_lnk_p->qec_15_ndx_cnt == 0)
	return(E_DB_OK);			/* local table has no indexes */

    /* 1.  set up for creating link on each index */

    *ddb_cols_pp = NULL;
    *ldb_cols_pp = NULL;

    /* 
    ** Start at element 1 in array of DD_COLUMN_DESC ptrs.  Extra last
    ** element is for tidp.
    */
    for (i = 0; i < DB_MAXKEYS +1; i++)
    {
	*(ddb_cols_pp + i+1) = & ddbcols[i];
	*(ldb_cols_pp + i+1) = & ldbcols[i];
    }

    STRUCT_ASSIGN_MACRO(*i_qer_p, lnk_rcb);
    lnkrcb_p->qef_eflag = QEF_INTERNAL;
    STRUCT_ASSIGN_MACRO(i_tabinfo_p->d9_1_obj_id, ddl_p->qed_d7_obj_id);
						/* pass embedded base object id
						** to create link routine */

    MEcopy(i_ddl_p->qed_d2_obj_owner, sizeof(DD_NAME), 
	ddl_p->qed_d2_obj_owner);
    ddl_p->qed_d3_col_count = 0;		/* assume */
    ddl_p->qed_d4_ddb_cols_pp = ddb_cols_pp;
    reg_p = ddl_p->qed_d9_reg_info_p;		/* save */
    ddl_p->qed_d9_reg_info_p = NULL;
    ddl_p->qed_d6_tab_info_p = tab_p;

    tab_p->dd_t3_tab_type = DD_4OBJ_INDEX;
    MEcopy(i_ddl_p->qed_d6_tab_info_p->dd_t2_tab_owner, sizeof(DD_NAME), 
	tab_p->dd_t2_tab_owner);
    tab_p->dd_t3_tab_type = DD_4OBJ_INDEX;
    tab_p->dd_t6_mapped_b = i_ddl_p->qed_d6_tab_info_p->dd_t6_mapped_b;
    tab_p->dd_t7_col_cnt = 0;			/* assume */
    tab_p->dd_t8_cols_pp = ldb_cols_pp;
    tab_p->dd_t9_ldb_p = i_ddl_p->qed_d6_tab_info_p->dd_t9_ldb_p;

    /* 2.  create link on each index */

    xid_p = v_lnk_p->qec_16_ndx_ids_p;
    for (i = 0; i < v_lnk_p->qec_15_ndx_cnt; i++, xid_p++)
    {
	/* 2.1  obtain column information for index */

	if (tab_p->dd_t6_mapped_b)
	{

	    status = qel_25_lcl_cols(
			i_qer_p, v_lnk_p, xid_p, & ddl_p->qed_d3_col_count,
			ldb_cols_pp);
		if (status)
	    {
	    	ddl_p->qed_d9_reg_info_p = reg_p;	/* restore */
	    	return(status);
	    }

	    if (ddl_p->qed_d3_col_count > DB_MAXKEYS +1)
	    {
	        status = qed_u1_gen_interr(& i_qer_p->error);
	        return(status);
	    }

	    tab_p->dd_t7_col_cnt = ddl_p->qed_d3_col_count;

	    {
	    	/* 2.2  provide link's column names at the DDB level */

	    	for (j = 1; j <= ddl_p->qed_d3_col_count; j++)
	     	{
		    l_cid_p = *(ldb_cols_pp + j);

		    found_b = qel_a0_map_col(i_qer_p, v_lnk_p, 
			    l_cid_p->dd_c1_col_name, & from_p);
		    to_p = *(ddb_cols_pp + j);
		    if (found_b)
		    {
		    	MEcopy(from_p->dd_c1_col_name, 
			    sizeof(DD_NAME), 
			    to_p->dd_c1_col_name);
		    }
		    else
		    {
		    	if (   MEcmp("tidp", l_cid_p->dd_c1_col_name, 4)
									== 0
			    || MEcmp("TIDP", l_cid_p->dd_c1_col_name, 4)
									== 0
			   )
			{
			    if (*i_qer_p->qef_cb->qef_dbxlate & CUI_ID_REG_U)
			    	MEcopy("TIDP",
			            sizeof(DD_NAME), 
			            to_p->dd_c1_col_name);
			    else
			    	MEcopy("tidp",
			            sizeof(DD_NAME), 
			            to_p->dd_c1_col_name);
			}
		        else
		        {
			    ddl_p->qed_d9_reg_info_p = reg_p;
					    	/* restore */
			    status = qed_u1_gen_interr(& i_qer_p->error);
			    return(status);
		    	}
		    }
	        }
	    }
	}
	else
	{
	    ddl_p->qed_d4_ddb_cols_pp = (DD_COLUMN_DESC **)0;
	}

	MEcopy(xid_p->qec_i3_given, sizeof(DD_NAME), 
	    ddl_p->qed_d1_obj_name);

	MEcopy(xid_p->qec_i1_name, sizeof(DD_NAME), 
	    tab_p->dd_t1_tab_name);

	status = qel_c0_cre_lnk(lnkrcb_p);
	if (status)
	{
	    ddl_p->qed_d9_reg_info_p = reg_p;	/* restore */
	    return(status);
	}
    }

    ddl_p->qed_d9_reg_info_p = reg_p;	/* restore */
    return(E_DB_OK);
}    


/*{
** Name: qel_25_lcl_cols - Retrieve LDB columns of index.
**
** Description:
**	This function retrieves column names from IIDD_COLUMNS for an index.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	i_xid_p			    ptr to QEC_INDEX_ID
**
** Outputs:
**	o_cnt_p			    number of columns of index
**	o_cid_pp		    column names of index saved
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
**	13-aug-91 (barbara)
**	    Fix bugs 38918, 38921.  Add tidp column to list of columns from
**	    iiindexes.  Otherwise, tidp doesn't get promoted to iidd_columns
**	    and iidd_ddb_ldb_columns.
**	27-oct-91 (barbara)
**	    When comparing column names to "tidp" allow for upper case
**	    version of that column.
*/


DB_STATUS
qel_25_lcl_cols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
QEC_INDEX_ID	*i_xid_p,
i4		*o_cnt_p,
DD_COLUMN_DESC	**o_cid_pp )
{
    DB_STATUS	    status;
    DD_LDB_DESC	    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_L7_INDEX_COLUMNS
		    xcol,
		    *xcol_p = & xcol;		/* tuple struct */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    DD_COLUMN_DESC  *cid_p;
    i4		    i;				/* loop control variable */
    bool	    seen_tidp = FALSE;


    /* 1.  set up for looping through each retrieved II_INDEX_COLUMN column */

    qed_u0_trimtail(
		    i_xid_p->qec_i1_name,
		    (u_i4) DB_MAXNAME,
		    xcol_p->l7_1_ind_name);
    qed_u0_trimtail(
		    i_xid_p->qec_i2_owner,
		    (u_i4) DB_MAXNAME,
		    xcol_p->l7_2_ind_owner);

    sel_p->qeq_c1_can_id = SEL_108_II_INDEX_COLUMNS;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l7_index_columns_p = xcol_p;
    sel_p->qeq_c4_ldb_p = ldb_p;

    /* 2.  send SELECT query */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (sel_p->qeq_c5_eod_b)
    {
	/* no data ! */

	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    i = 0;
    while (! sel_p->qeq_c5_eod_b)
    {
	/* save local column name */

	i++;
	cid_p = *(o_cid_pp + i);

	MEmove(STlength(xcol_p->l7_3_col_name), 
		xcol_p->l7_3_col_name, ' ', 
		sizeof(cid_p->dd_c1_col_name), 
		cid_p->dd_c1_col_name);

	if (   MEcmp("tidp", cid_p->dd_c1_col_name, 4) == 0
	    || MEcmp("TIDP", cid_p->dd_c1_col_name, 4) == 0
	   )
	    seen_tidp = TRUE;

	/* 3.  retrieve next tuple */

	sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    *o_cnt_p = i;

    /* 
    ** Save tidp column if not picked up from iiindex_columns.  When we
    ** register indexes from gateways we'll have to check that this is 
    ** an INGRES index.
    */

    if (!seen_tidp)
    {
	char *tidp_str;

	if (*i_qer_p->qef_cb->qef_dbxlate & CUI_ID_REG_U)
	    tidp_str = "TIDP";
	else
	    tidp_str = "tidp";
	cid_p = *(o_cid_pp + i + 1);
	MEmove(4, tidp_str, ' ', sizeof(cid_p->dd_c1_col_name),
	cid_p->dd_c1_col_name);
	*o_cnt_p = *o_cnt_p + 1;
    }

    return(E_DB_OK);
}    


/*{
** Name: qel_26_ndx_name - Generate link name for index.
**
** Description:
**	This function generates a link name for an index.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	i_ndx_ord		    ordinal of index of LDB table
**
** Outputs:
**	o_given_p		    ptr to space for given name
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
**	27-oct-88 (carl)
**          written
**	21-mar-95 (rudtr01)
**	    Fix bug 67496.  For catalog indices use iiddx when
**	    assembling the registered object name.
*/


VOID
qel_26_ndx_name(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p,
i4		i_ndx_ord,
DD_NAME		*o_given_p )
{
    QEC_D9_TABLEINFO	*tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    char		*prefix_p, *cp1, *cp2;
    u_i4		name_l;
    QEC_DBNAMESTR	base_ascii,
			ord_ascii;
    char		ndx_name[QEK_100_LEN + DB_MAXNAME];

    /*
    ** Convert the two object ids (table & index) into 4-digit
    ** ascii numeric strings
    */
    CVla(tabinfo_p->d9_1_obj_id.db_tab_base, base_ascii);
    CVla(tabinfo_p->d9_1_obj_id.db_tab_base + i_ndx_ord, ord_ascii);	

    /* If it's a catalog table, use "iiddx" as a prefix for the 
    ** registered name, otherwise just use "ddx".  Then assemble
    ** the pieces to make something like "ddx_1001_1002".
    */
    name_l = qed_u0_trimtail( tabinfo_p->d9_3_tab_name,
				(u_i4) DB_MAXNAME,
				ndx_name );
    prefix_p = "ddx_";
    if ( name_l >= 2 )
    {
	cp1 = &ndx_name[ 0 ];
	cp2 = cp1 + CMbytecnt( ndx_name );
	if ( CMcmpnocase( cp1, "i" ) == 0 &&
	     CMcmpnocase( cp2, "i" ) == 0 )
	{
	    prefix_p = "iiddx_";
	}
    }
    STpolycat(4, prefix_p, base_ascii, "_", ord_ascii, ndx_name);
    MEmove(STlength(ndx_name), ndx_name, ' ', sizeof(DD_NAME),
	    (char *)o_given_p);
}    
