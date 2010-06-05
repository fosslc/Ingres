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
#include    <qsf.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
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
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEU1CV.C - Contains supporting routines for CREATE VIEW
**
**  Description:
**      Supporting routines for CREATE VIEW.
**
**	    qeu_10_get_obj_base	- retrieve current object base value
**	    qeu_11_object_base	- update IIDD_DDB_OBJECT_BASE
**	    qeu_12_objects	- insert into IIDD_DDB_OBJECTS
**	    qeu_13_tables	- insert into IIDD_TABLES
**	    qeu_14_columns	- insert into IIDD_COLUMNS
**	    qeu_15_views	- insert into IIDD_VIEWS
**	    qeu_16_tree		- insert into IIDD_DDB_TREE
**	    qeu_17_dbdepends	- insert into IIDD_DDB_DBDEPENDS
**
**  History:    $Log-for RCS$
**	04-nov-88 (carl)
**          written
**	11-dec-88 (carl)
**	    modified for catalog redefinition
**	07-aug-90 (carl)
**	    1) switched to use unique global variable names
**	    2) changed qeu_13_tables to assign 'S' to iidd_tables.system_use 
**	       if view name starts with "ii" and "+Y" is in effect
**	12-dec-91 (fpang)
**	    In qeu_12_objects(), if object begins with 'ii' and is owned by 
**	    "$ingres', set iiddb_objects.system_object to 'Y'.
**	    Fixes B4115.
**      25-dec-91 (fpang)
**          Fixed compiler warnings.
**	22-jul-1993 (fpang)
**	    Back integrate 64 bug fix.
**		09-oct-1992 (fpang)
**			In qeu_17_dbdepends(), avoid duplicate insertion into
**			iidd_ddb_dbdepends.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**      15-apr-94 (heleny)
**          fix b59672, call adi_tyname
**	26-apr-94 (lauraw/heleny)
**	    correct typo in adi_dname.adi_dtname
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	07-Dec-2009 (troal01)
**	    Consolidated DMU_ATTR_ENTRY, DMT_ATTR_ENTRY, and DM2T_ATTR_ENTRY
**	    to DMF_ATTR_ENTRY. This change affects this file.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


GLOBALREF   char	IIQE_33_integer[];
GLOBALREF   char	IIQE_34_float[];
GLOBALREF   char	IIQE_35_text[];
GLOBALREF   char	IIQE_36_date[];
GLOBALREF   char	IIQE_37_money[];
GLOBALREF   char	IIQE_38_char[];
GLOBALREF   char	IIQE_39_varchar[];
GLOBALREF   char	IIQE_40_decimal[];
GLOBALREF   char	IIQE_42_ing_60[];
GLOBALREF   char	IIQE_43_heap[];
GLOBALREF   char	IIQE_c0_usr_ingres[];



/*{
** Name: qeu_10_get_obj_base - Retrieve current object base value
**
** Description:
**      This routine retrieves the current object base value from 
**  IIDD_DDB_OBJECT_BASE.
**
** Inputs:
**	    i_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
** Outputs:
**	    v_lnk_p			QEC_LINK
**		.qec_13_objects_p	QEC_D6_OBJECTS
**		    .d6_3_obj_id
**			.db_tab_base	current value + 1
**			.db_tab_index	0
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
**      04-nov-88 (carl)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
[@history_template@]...
*/


DB_STATUS
qeu_10_get_obj_base(
QEF_RCB		    *i_qer_p,
QEC_LINK	    *v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc;
/*
    QED_DDL_INFO    *ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
*/
    QEC_D7_OBJECT_BASE
		    obj_base;	    /* tuple structure */
    QEC_D6_OBJECTS
		    *objects_p = v_lnk_p->qec_13_objects_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;


    /* 1.  retrieve IIDD_DDB_OBJECT_BASE */

    sel_p->qeq_c1_can_id = SEL_016_DD_DDB_OBJECT_BASE;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.d7_object_base_p = & obj_base; 
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    /* 2.  retrieve current OBJECT_BASE value */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {	
	status = qel_s3_flush(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 3.  increment to get new object_base */

    objects_p->d6_3_obj_id.db_tab_base = obj_base.d7_1_obj_base + 1;
    objects_p->d6_3_obj_id.db_tab_index = 0;

    return(E_DB_OK);
}


/*{
** Name: qeu_11_object_base - update IIDD_DDB_OBJECT_BASE
**
** Description:
**	This routine replaces the current object base value in
**  IIDD_DDB_OBJECT_BASE for a view.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				QEC_LINK
**	    .qec_13_objects_p		QEC_D6_OBJECTS
**		.d6_3_obj_id
**		    .db_tab_base	current value + 1
**		    .db_tab_index	0
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
**	04-nov-88 (carl)
**          written
[@history_line@]...
*/


DB_STATUS
qeu_11_object_base(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
/*
    QED_DDL_INFO    *ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
*/
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEC_D7_OBJECT_BASE
		    obj_base,
		    *base_p = & obj_base;
    QEQ_1CAN_QRY    *upd_p = v_lnk_p->qec_23_update_p;


    /* 1.  set up update information */

    base_p->d7_1_obj_base = objects_p->d6_3_obj_id.db_tab_base;

    upd_p->qeq_c1_can_id = UPD_716_DD_DDB_OBJECT_BASE;
    upd_p->qeq_c3_ptr_u.d7_object_base_p = base_p;
    upd_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send UPDATE query */

    status = qel_u1_update(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qeu_12_objects - update IIDD_DDB_OBJECTS
**
** Description:
**	This routine inserts a tuple into IIDD_DDB_OBJECTS for a view.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**		.qec_1_ddl_info_p	input information
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
**	04-nov-88 (carl)
**          written
**	12-dec-91 (fpang)
**	    If object begins with 'ii' and is owned by "$ingres',
**	    set iiddb_objects.system_object to 'Y'.
**	    Fixes B4115.
[@history_line@]...
*/


DB_STATUS
qeu_12_objects(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    u_i4	    l_obj, l_own, l_ing;


    /* 1.  set up insert information */

    l_obj = (u_i4)qed_u0_trimtail( ddl_p->qed_d1_obj_name, DB_OBJ_MAXNAME, 
	    objects_p->d6_1_obj_name);

    l_own = (u_i4)qed_u0_trimtail( ddl_p->qed_d2_obj_owner, DB_OWN_MAXNAME,
	    objects_p->d6_2_obj_owner);

    l_ing = STlength( IIQE_c0_usr_ingres );

    STcopy(
	v_lnk_p->qec_24_cur_time, 
	objects_p->d6_5_obj_cre);

    objects_p->d6_6_obj_type[0] = 'V';		/* view */
    objects_p->d6_6_obj_type[1] = EOS;

    STcopy(
	objects_p->d6_5_obj_cre,
	objects_p->d6_7_obj_alt);

    /*
    ** Assume that if object name begins with ii, and if owner is $ingres,
    ** then it is a system object.
    */

    if( ( l_obj >= 2 ) &&
        (MEcmp( objects_p->d6_1_obj_name, "ii", 2 ) == 0 ) &&
        ( l_own == l_ing ) &&
        (MEcmp( objects_p->d6_2_obj_owner, IIQE_c0_usr_ingres, l_ing ) == 0 ))
    {
        objects_p->d6_8_sys_obj[0] = 'Y';       /* system object */
    }
    else
    {
        objects_p->d6_8_sys_obj[0] = 'N';       /* not a system object */
    }

    objects_p->d6_8_sys_obj[1] = EOS;

    objects_p->d6_9_to_expire[0] = 'N';		/* no expiration date */
    objects_p->d6_9_to_expire[1] = EOS;

    objects_p->d6_10_exp_date[0] = EOS;		/* no expiration date */

    ins_p->qeq_c1_can_id = INS_615_DD_DDB_OBJECTS;
    ins_p->qeq_c3_ptr_u.d6_objects_p = objects_p;
    ins_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send INSERT query */

    status = qel_i1_insert(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qeu_13_tables - Insert to IIDD_TABLES
**
** Description:
**	This routine inserts an entry in IIDD_TABLES for a view.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	i_quq_p			    QEUQ_CB
**	v_lnk_p			    ptr to QEC_LINK
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
**	04-nov-88 (carl)
**          written
**	07-aug-90 (carl)
**	    changed qeu_13_tables to assign 'S' to iidd_tables.system_use 
**	    if view name starts with "ii" and "+Y" is in effect
**	06-mar-96 (nanpr01)
**	    Initialized the l16_39_pagesize to 0 since for view it is unknown. 
*/


DB_STATUS
qeu_13_tables(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEC_L16_TABLES  *tables_p = v_lnk_p->qec_9_tables_p;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
						/* working structure */


    /* 1.  set up tuple for insertion */

    qed_u0_trimtail( ddl_p->qed_d1_obj_name, DB_OBJ_MAXNAME,
		tables_p->l16_1_tab_name);

    qed_u0_trimtail( ddl_p->qed_d2_obj_owner, DB_OWN_MAXNAME, 
		tables_p->l16_2_tab_owner);

    STcopy(v_lnk_p->qec_24_cur_time, tables_p->l16_3_cre_date);
    STcopy(v_lnk_p->qec_24_cur_time, tables_p->l16_4_alt_date);

    tables_p->l16_5_tab_type[0] = 'V';
    tables_p->l16_5_tab_type[1] = EOS;		/* null terminate */

    tables_p->l16_6_sub_type[0] = 'N';		/* native */
    tables_p->l16_6_sub_type[1] = EOS;		/* null terminate */

    STcopy(IIQE_42_ing_60, tables_p->l16_7_version);

    if (dds_p->qes_d9_ctl_info & QES_05CTL_SYSCAT_USER)
    {
	if (ddl_p->qed_d1_obj_name[0] == 'i'
	    &&
	    ddl_p->qed_d1_obj_name[1] == 'i')
	{
	    tables_p->l16_8_sys_use[0] = 'S';	/* system object */

	}
	else
	    tables_p->l16_8_sys_use[0] = 'U';	/* user object */
    }
    else
	tables_p->l16_8_sys_use[0] = 'U';	/* user object */
    tables_p->l16_8_sys_use[1] = EOS;		/* null terminate */

    tables_p->l16_9_stats[0] = 'N';		/* no statistics for view */
    tables_p->l16_9_stats[1] = EOS;		/* null terminate */

    tables_p->l16_10_indexes[0] = 'N';		/* no indexes for view */
    tables_p->l16_10_indexes[1] = EOS;		/* null terminate */

    tables_p->l16_11_readonly[0] = 'N';		/* may be updateable */
    tables_p->l16_11_readonly[1] = EOS;		/* null terminate */

    tables_p->l16_12_num_rows = 0;		/* unknown */

    STcopy(IIQE_43_heap, tables_p->l16_13_storage);
						/* view structure is HEAP */
    tables_p->l16_14_compressed[0] = 'N';	/* not compressed */
    tables_p->l16_14_compressed[1] = EOS;	/* null terminate */

    tables_p->l16_15_dup_rows[0] = 'U';		/* unknown */
    tables_p->l16_15_dup_rows[1] = EOS;		/* null terminate */

    tables_p->l16_16_uniquerule[0] = 'D';	/* duplicate */
    tables_p->l16_16_uniquerule[1] = EOS;	/* null terminate */

    tables_p->l16_17_num_pages = 1;		/* assume */
    tables_p->l16_18_overflow = 0;		/* assume */
    tables_p->l16_19_row_width = i_quq_p->qeuq_ddb_cb.qeu_3_row_width;
    tables_p->l16_20_tab_expire = 0;		/* no expiration date */
    tables_p->l16_21_mod_date[0] = EOS;		/* none */
    tables_p->l16_22_loc_name[0] = EOS;		/* none */

    tables_p->l16_23_integrities[0] = 'N';
    tables_p->l16_23_integrities[1] = EOS;

    tables_p->l16_24_permits[0] = 'N';
    tables_p->l16_24_permits[1] = EOS;

    tables_p->l16_25_all_to_all[0] = 'Y';
    tables_p->l16_25_all_to_all[1] = EOS;

    tables_p->l16_26_ret_to_all[0] = 'Y';
    tables_p->l16_26_ret_to_all[1] = EOS;

    tables_p->l16_27_journalled[0] = 'N';
    tables_p->l16_27_journalled[1] = EOS;

    tables_p->l16_28_view_base[0] = 'N';
    tables_p->l16_28_view_base[1] = EOS;

    tables_p->l16_29_multi_loc[0] = EOS;	/* none */
    tables_p->l16_30_ifillpct = 0;
    tables_p->l16_31_dfillpct = 100;
    tables_p->l16_32_lfillpct = 0;
    tables_p->l16_33_minpages = 1;
    tables_p->l16_34_maxpages = 1;
    tables_p->l16_35_rel_st1 = v_lnk_p->qec_17_ts1;
    tables_p->l16_36_rel_st2 = v_lnk_p->qec_18_ts2;
    tables_p->l16_37_reltid = objects_p->d6_3_obj_id.db_tab_base;
    tables_p->l16_38_reltidx = objects_p->d6_3_obj_id.db_tab_index; 
    tables_p->l16_39_pagesize = 0; 	/* unknown */


    /* 2.  insert into IIDD_TABLES */

    ins_p->qeq_c1_can_id = INS_631_DD_TABLES;	
    ins_p->qeq_c3_ptr_u.l16_tables_p = tables_p;	
    ins_p->qeq_c4_ldb_p = cdb_p;
    status = qel_i1_insert(i_qer_p, v_lnk_p);

    return(status);
}    


/*{
** Name: qeu_14_columns - Insert to IIDD_COLUMNS 
**
** Description:
**	This function inserts entries into IIDD_COLUMNS for a view.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	i_quq_p			    QEUQ_CB
**	v_lnk_p			    ptr to QEC_LINK
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
**	04-nov-88 (carl)
**          written
**      15-apr-94 (heleny)
**          call adi_tyname to get data type name
**	26-apr-94 (lauraw/heleny)
**	    correct typo in adi_dname.adi_dtname
[@history_line@]...
*/


DB_STATUS
qeu_14_columns(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L3_COLUMNS  dd_columns,
		    *columns_p = & dd_columns;	/* tuple struct */
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    QEP_PTR_UNION   ptr_u;
    DMU_CB	    *dmu_p;
    DMF_ATTR_ENTRY  **col_pp,
		    *curcol_p;
    i4		    colnum,
		    coltype,
		    postype;
    QEC_LONGNAT_TO_I4  
		    coerce;
    ADI_DT_NAME	    adi_dname;


    ptr_u.qep_ptr_u.qep_1_ptr = i_quq_p->qeuq_dmf_cb;
    dmu_p = ptr_u.qep_ptr_u.qep_3_dmu_cb_p;

    ptr_u.qep_ptr_u.qep_1_ptr = dmu_p->dmu_attr_array.ptr_address;
    col_pp = ptr_u.qep_ptr_u.qep_4_dmu_attr_pp;

    /* 1.  set up constant values for all columns */

    qed_u0_trimtail( ddl_p->qed_d1_obj_name, DB_OBJ_MAXNAME,
	columns_p->l3_1_tab_name);

    qed_u0_trimtail( ddl_p->qed_d2_obj_owner, DB_OWN_MAXNAME,
	columns_p->l3_2_tab_owner);

    columns_p->l3_10_seq_in_key = 0;

    for (colnum = 0; colnum < i_quq_p->qeuq_ano; colnum++, col_pp++)
    {
	curcol_p = *col_pp;

	/* 2.  fill in specific information for current column */

	qed_u0_trimtail( curcol_p->attr_name.db_att_name, DB_ATT_MAXNAME,
	    columns_p->l3_3_col_name);

	coerce.qec_i4_i4.qec_1_i4 = curcol_p->attr_type;
	coltype = coerce.qec_i4_i4.qec_2_i4;
	if (coltype < 0)
	{
	    columns_p->l3_7_nulls[0] = 'Y';
	    postype = -coltype;			    /* must make positive */
	}
	else
	{
	    columns_p->l3_7_nulls[0] = 'N';
	    postype = coltype;
	}
	columns_p->l3_7_nulls[1] = EOS;

        /* call adi_tyname to get data type name */
	status = adi_tyname(i_qer_p->qef_adf_cb, postype, &adi_dname);
	if (status == E_DB_OK) 
		STcopy(adi_dname.adi_dtname, columns_p->l3_4_data_type);
	else
	{
	    status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB,
			& i_qer_p->error);
	    return(status);
	}

	coerce.qec_i4_i4.qec_1_i4 = curcol_p->attr_size;
	columns_p->l3_5_length = coerce.qec_i4_i4.qec_2_i4;

	coerce.qec_i4_i4.qec_1_i4 = curcol_p->attr_precision;
	columns_p->l3_6_scale = coerce.qec_i4_i4.qec_2_i4;

	if (curcol_p->attr_flags_mask & DMU_F_NDEFAULT)
	    columns_p->l3_8_defaults[0] = 'N';
	else
	    columns_p->l3_8_defaults[0] = 'Y';
	columns_p->l3_8_defaults[1] = EOS;

	columns_p->l3_9_seq_in_row = colnum + 1;
	columns_p->l3_10_seq_in_key = 0;	/* none */

	columns_p->l3_11_sort_dir[0] = 'A';	/* assume ascending */
	columns_p->l3_11_sort_dir[1] = EOS;	/* null terminate */

	columns_p->l3_12_ing_datatype = coltype;

	/* 3.  insert into IIDD_COLUMNS */

	ins_p->qeq_c1_can_id = INS_602_DD_COLUMNS;
	ins_p->qeq_c3_ptr_u.l3_columns_p = columns_p;
	ins_p->qeq_c4_ldb_p = cdb_p;
	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    } 

    return(E_DB_OK);
}    


/*{
** Name: qeu_15_views - Insert to IIDD_VIEWS
**
** Description:
**	This function inserts entries into IIDD_VIEWS for a view.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	i_quq_p			    QEUQ_CB
**	v_lnk_p			    ptr to QEC_LINK
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
**	04-nov-88 (carl)
**          written
[@history_line@]...
*/


DB_STATUS
qeu_15_views(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEUQ_DDB_CB	    *ddu_p = & i_quq_p->qeuq_ddb_cb;
    QEF_DATA	    *data_p = i_quq_p->qeuq_qry_tup;
    QEC_L17_VIEWS   views,
		    *views_p = & views;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    i4		    seq;


    /* 1.  set up constant values for all entries */

    qed_u0_trimtail( ddl_p->qed_d1_obj_name, DB_OBJ_MAXNAME,
	views_p->l17_1_tab_name);

    qed_u0_trimtail( ddl_p->qed_d2_obj_owner, DB_OWN_MAXNAME,
	views_p->l17_2_tab_owner);

    if (ddu_p->qeu_1_lang == DB_SQL)
	views_p->l17_3_dml[0] = 'S';
    else if (ddu_p->qeu_1_lang == DB_QUEL)
	views_p->l17_3_dml[0] = 'Q';
    else
    {
	status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB,
			& i_qer_p->error);
	return(status);
    }
    views_p->l17_3_dml[1] = EOS;

    if (ddu_p->qeu_2_view_chk_b)
	views_p->l17_4_chk_option[0] = 'Y';
    else
	views_p->l17_4_chk_option[0] = 'N';
    views_p->l17_4_chk_option[1] = EOS;

    for (seq = 0; seq < i_quq_p->qeuq_cq; seq++)
    {
	/* 2.  fill in specific information for current entry */

	views_p->l17_5_sequence = seq + 1;

	if (data_p->dt_size > QEK_256_VIEW_SIZE)
	{
	    status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB,
			& i_qer_p->error);
	    return(status);
	}
	else
	{
	    MEcopy((char *) data_p->dt_data, data_p->dt_size,
		views_p->l17_6_txt_seg);
	    views_p->l17_6_txt_seg[data_p->dt_size] = EOS;
	    views_p->l17_7_txt_size = data_p->dt_size;
	}

	data_p = data_p->dt_next;		/* advance to next item */

	/* 3.  insert into IIDD_VIEWS */

	ins_p->qeq_c1_can_id = INS_632_DD_VIEWS;
	ins_p->qeq_c3_ptr_u.l17_views_p = views_p;
	ins_p->qeq_c4_ldb_p = cdb_p;
	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    } 

    return(E_DB_OK);
}    


/*{
** Name: qeu_16_tree - Insert to IIDD_DDB_TREE.
**
** Description:
**	This function inserts entries into above catalog for a view.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	i_quq_p			    QEUQ_CB
**	v_lnk_p			    ptr to QEC_LINK
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
**	04-nov-88 (carl)
**          written
[@history_line@]...
*/


DB_STATUS
qeu_16_tree(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
/*
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
*/
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS
		    *objects_p = v_lnk_p->qec_13_objects_p;
    QEF_DATA	    *data_p = i_quq_p->qeuq_tre_tup;
    QEC_D10_TREE    tree,
		    *tree_p = & tree;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    i4		    seq;


    /* 1.  set up constant values for all entries */

    tree_p->d10_1_treetabbase = objects_p->d6_3_obj_id.db_tab_base;
    tree_p->d10_2_treetabidx = objects_p->d6_3_obj_id.db_tab_index; 

    tree_p->d10_3_treeid1 = (i4) objects_p->d6_4_qry_id.db_qry_high_time;
    tree_p->d10_4_treeid2 = (i4) objects_p->d6_4_qry_id.db_qry_low_time; 

    tree_p->d10_6_treemode = DB_VIEW;
    tree_p->d10_7_treevers = DD_0TV_UNKNOWN;

#ifdef	DDB_1TV_VAX

    tree_p->d10_7_treevers = DD_1TV_VAX;
						/* VAX binary representation */
#endif

    for (seq = 0; seq < i_quq_p->qeuq_ct; seq++)
    {
	/* 2.  fill in specific information for current entry */

	tree_p->d10_5_treeseq = seq;		/* starts from 0 */

	if (data_p->dt_size > QEK_1024_TREE_SIZE + 2)
						/* include 2-byte length */
	{
	    status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB,
			& i_qer_p->error);
	    return(status);
	}
	else
	{
	    MEcopy((char *) data_p->dt_data, data_p->dt_size,
		tree_p->d10_8_treetree);
	    tree_p->d10_8_treetree[data_p->dt_size] = EOS;
	    tree_p->d10_9_treesize = data_p->dt_size;
	}

	data_p = data_p->dt_next;		/* advance to next item */

	/* 3.  insert into IIDD_DDB_TREE */

	ins_p->qeq_c1_can_id = INS_619_DD_DDB_TREE;
	ins_p->qeq_c3_ptr_u.d10_tree_p = tree_p;
	ins_p->qeq_c4_ldb_p = cdb_p;
	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    } 

    return(E_DB_OK);
}    


/*{
** Name: qeu_17_dbdepends - Insert to IIDD_DDB_DBDEPENDS.
**
** Description:
**	This function inserts entries into above catalog for a view.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	i_quq_p			    QEUQ_CB
**	v_lnk_p			    ptr to QEC_LINK
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
**	04-nov-88 (carl)
**          written
**	09-oct-1992 (fpang)
**	    Avoid duplicate insertion into iidd_ddb_dbdepends.
[@history_line@]...
*/


DB_STATUS
qeu_17_dbdepends(
QEF_RCB		*i_qer_p,
QEUQ_CB		*i_quq_p,
QEC_LINK	*v_lnk_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
/*
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
*/
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D6_OBJECTS
		    *objects_p = v_lnk_p->qec_13_objects_p;
    DB_TAB_ID	    *inid_p = i_quq_p->qeuq_tbl_id;
    DD_OBJ_TYPE	    *intyp_p = i_quq_p->qeuq_obj_typ_p;
    QEC_D1_DBDEPENDS
		    dbdepends,
		    *depends_p = & dbdepends;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    i4		    i;
    DB_TAB_ID	    *tnid_p;
    DD_OBJ_TYPE	    *tntyp_p;


    /* 1.  set up dependent values for all entries */

    depends_p->d1_4_deid1 = objects_p->d6_3_obj_id.db_tab_base;
    depends_p->d1_5_deid2 = objects_p->d6_3_obj_id.db_tab_index; 

    depends_p->d1_6_dtype = DB_VIEW;
    depends_p->d1_7_qid = 0;

    for (i = 0; i < i_quq_p->qeuq_tsz; i++, inid_p++, intyp_p++)
    {
	/* 2. Check for duplicate insertion. */

	for ( tnid_p = i_quq_p->qeuq_tbl_id, tntyp_p = i_quq_p->qeuq_obj_typ_p;
	      (tnid_p != inid_p)  &&
		  (!((tnid_p->db_tab_base  == inid_p->db_tab_base)  &&
		     (tnid_p->db_tab_index == inid_p->db_tab_index) &&
		     (*tntyp_p == *intyp_p))) ;
	      tnid_p++, tntyp_p++ ) ;

	if (tnid_p != inid_p)	/* This row already inserted. */
	    continue;

	/* 3.  fill in specific information for current entry */

	depends_p->d1_1_inid1 = inid_p->db_tab_base;
	depends_p->d1_2_inid2 = inid_p->db_tab_index;
	depends_p->d1_3_itype = *intyp_p;	/* type of base table */

	/* 3.  insert into IIDD_DDB_DBDEPENDS */

	ins_p->qeq_c1_can_id = INS_605_DD_DDB_DBDEPENDS;
	ins_p->qeq_c3_ptr_u.d1_dbdepends_p = depends_p;
	ins_p->qeq_c4_ldb_p = cdb_p;
	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    } 

    return(E_DB_OK);
}    
