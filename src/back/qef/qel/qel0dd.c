
/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <cs.h>
#include    <me.h>
#include    <st.h>
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
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEL0DD.C - Routines updating STAR-specific catalogs for CREATE 
**		     or REGISTER WITH REFRESH
**
**  Description:
**      These routines perform STAR catalog updates for CREATE LINK.
**
**	    qel_01_ddb_cats	    - perform updates on DDB catalogs
**	    qel_02_objects	    - update IIDD_DDB_OBJECTS
**	    qel_03_object_base	    - update IIDD_DDB_OBJECT_BASE
**	    qel_04_tableinfo	    - update IIDD_DDB_TABLEINFO
**	    qel_05_ldbids	    - update IIDD_DDB_LDBIDS and
**					     IIDD_DDB_LONG_LDBNAMES
**	    qel_06_ldb_columns	    - update IIDD_DDB_LDB_COLUMNS
**	    qel_07_ldb_dbcaps	    - update IIDD_DDB_LDB_DBCAPS 
**
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog redefinition
**	22-jan-89 (carl)
**	    modified qel_07_ldb_dbcaps to include OWNER_NAME, etc.
**	02-apr-89 (carl)
**          modified to process REGISTER WITH REFRESH
**	02-aug-89 (carl)
**	    modified qel_01_ddb_cats to suppress incrementing object_base 
**	    in iidd_ddb_object_base since it has already been incremented
**	10-aug-90 (carl)
**	    removed all references to IIQE_c28_dbcapabilities
**	12-dec-91 (fpang)
**	    In qel_02_objects(), if object begins with 'ii' and is owned by 
**	    "$ingres", set iiddb_objects.system_object to 'Y'.
**	    Fixes B41115.
**	18-dec-91 (lan)
**	    Fixed compiler warnings
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	08-mar-1993 (fpang)
**	    In qel_01_ddb_cats(), don't re-insert into iidd_ddb_ldb_columns 
**	    if REGISTER w/refresh.
**	    Fixes B49737, Register w/refresh w/mapped columns failed with
**	    duplicate insert errors from the ldb.
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
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
**/

GLOBALREF	char	IIQE_c0_usr_ingres[];

/*{
** Name: qel_01_ddb_cats - Perform updates on DDB catalogs.
**
** Description:
**	This function does all updates on the DDB catalogs using supplied
**  information.
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
**	02-apr-89 (carl)
**          modified to process REGISTER WITH REFRESH
**	02-aug-89 (carl)
**	    modified qel_01_ddb_cats to suppress incrementing object_base 
**	    in iidd_ddb_object_base since it has already been incremented
**	08-mar-1993 (fpang)
**	    Don't re-insert into iidd_ddb_ldb_columns if REGISTER w/refresh.
**	    Fixes B49737, Register w/refresh w/mapped columns failed with
**	    duplicate insert errors from the ldb.
**	27-may-93 (barbara)
**	    Call qel_06_ldb_columns() to insert into iidd_ddb_ldb_columns
**	    even if user didn't specify a mapping of column names. 
**	    Incompatible case translation semantics between the LDB and
**	    the DDB require a mapping.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_01_ddb_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    QEC_D9_TABLEINFO    
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;


    /* 2.  insert entry into IIDD_DDB_OBJECTS */

    status = qel_02_objects(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    v_lnk_p->qec_3_ldb_id = v_lnk_p->qec_7_ldbids_p->d2_5_ldb_id;

    if (v_lnk_p->qec_3_ldb_id <= 0)
    {
	/* 3.0  insert for a new LDB; first assign an ldb id */ 
    
	v_lnk_p->qec_3_ldb_id = tabinfo_p->d9_1_obj_id.db_tab_base;
				
	tabinfo_p->d9_10_ldb_id = tabinfo_p->d9_1_obj_id.db_tab_base;

	/*  3.1  insert into IIDD_DDB_LDBIDS for new LDB */

	status = qel_05_ldbids(i_qer_p, v_lnk_p);
	if (status)
	    return(status);

	/* 3.2  promote relevant LDB capabilities into IIDD_DDB_LDB_DBCAPS */

	status = qel_07_ldb_dbcaps(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 4.  insert entry into IIDD_DDB_LDB_TABLEINFO */

    status = qel_04_tableinfo(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    /* 
    **	5. insert into IIDD_DDB_LDB_COLUMNS if columns are mapped 
    **	   or if incompatible case translation semantics between DDB
    **	   and LDB require a mapping
    */

    if (ddl_p->qed_d6_tab_info_p->dd_t6_mapped_b)
    {
	v_lnk_p->qec_4_col_cnt = ddl_p->qed_d6_tab_info_p->dd_t7_col_cnt;    
						/* # columns in link */
    }

    /* Don't re-insert if this is register w/refresh */
    if (dds_p->qes_d7_ses_state != QES_4ST_REFRESH)
    {
	status = qel_06_ldb_columns(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    return(status);
}    


/*{
** Name: qel_02_objects - update IIDD_DDB_OBJECTS
**
** Description:
**	This routine inserts a tuple into IIDD_DDB_OBJECTS for a link or
**  table.
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
**	08-jun-88 (carl)
**          written
**	12-dec-91 (fpang)
**	    If object begins with 'ii' and is owned by "$ingres", set
**	    iiddb_objects.system_object to 'Y'.
**	    Fixes B41115.
**	29-nov-93 (barbara)
**	    Must do case insensitive comparison for system catalog
**	    WRT "ii" prefix and owner "$ingres" to support upper case
**	    names.  Fixes bug 56567.
*/


DB_STATUS
qel_02_objects(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    u_i4	    l_obj, l_own, l_ing;
    char	    *ch2;


    /* 1.  set up insert information */

    l_obj = (u_i4)qed_u0_trimtail( ddl_p->qed_d1_obj_name, 
		(u_i2) DB_OBJ_MAXNAME, objects_p->d6_1_obj_name);

    l_own = (u_i4)qed_u0_trimtail( ddl_p->qed_d2_obj_owner, 
		(u_i2) DB_OWN_MAXNAME, objects_p->d6_2_obj_owner);

    l_ing = STlength( IIQE_c0_usr_ingres );

    STRUCT_ASSIGN_MACRO(tabinfo_p->d9_1_obj_id, objects_p->d6_3_obj_id);

    objects_p->d6_4_qry_id.db_qry_high_time = 0;
    objects_p->d6_4_qry_id.db_qry_low_time = 0;

    STcopy(
	v_lnk_p->qec_24_cur_time, 
	objects_p->d6_5_obj_cre);

    if (ddl_p->qed_d8_obj_type == DD_1OBJ_LINK)
	objects_p->d6_6_obj_type[0] = 'L';	/* link */
    else if (ddl_p->qed_d8_obj_type == DD_2OBJ_TABLE)
	objects_p->d6_6_obj_type[0] = 'T';	/* table */
    else
    {
	status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB,
		    & i_qer_p->error);
	return(status);
    }
    objects_p->d6_6_obj_type[1] = EOS;

    STcopy(
	objects_p->d6_5_obj_cre,
	objects_p->d6_7_obj_alt);

    /*
    ** Assume that if object name begins with ii (or II), and if owner is
    ** $ingres, (or $INGRES) then it is a system object.
    */

    ch2 = objects_p->d6_1_obj_name + CMbytecnt(objects_p->d6_1_obj_name);
    if( ( l_obj >= 2 ) &&
	(CMcmpnocase(objects_p->d6_1_obj_name, "i") == 0 ) &&
	(CMcmpnocase(ch2, "i") == 0 ) &&
	( l_own == l_ing ) &&
	(STbcompare(objects_p->d6_2_obj_owner, 7, IIQE_c0_usr_ingres, 7, TRUE)
		== 0 ))
    {
        objects_p->d6_8_sys_obj[0] = 'Y';	/* system object */
    }
    else
    {
        objects_p->d6_8_sys_obj[0] = 'N';	/* not a system object */
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
** Name: qel_03_object_base - update IIDD_DDB_OBJECT_BASE
**
** Description:
**	This routine replaces the current object base value in 
**  IIDD_DDB_OBJECT_BASE.
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
**	08-jun-88 (carl)
**          written
**	02-apr-89 (carl)
**          modified to process REGISTER WITH REFRESH
*/


DB_STATUS
qel_03_object_base(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QED_DDL_INFO    *ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_2LDB_TAB_INFO	
		    *tab_p = ddl_p->qed_d6_tab_info_p;
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    QEC_D7_OBJECT_BASE
		    obj_base,
		    *base_p = & obj_base;
    QEQ_1CAN_QRY    *upd_p = v_lnk_p->qec_23_update_p;


    /* 1.  set up update information */

    if (tab_p->dd_t3_tab_type == DD_4OBJ_INDEX)
	base_p->d7_1_obj_base = tabinfo_p->d9_1_obj_id.db_tab_index;
						/* creating link for index */
    else
    {
	if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH)
						/* REGISTER WITH REFRESH ? */
	    return(E_DB_OK);			/* retain original id */

	base_p->d7_1_obj_base = tabinfo_p->d9_1_obj_id.db_tab_base;
    }
    upd_p->qeq_c1_can_id = UPD_716_DD_DDB_OBJECT_BASE;
    upd_p->qeq_c3_ptr_u.d7_object_base_p = base_p;
    upd_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send UPDATE query */

    status = qel_u1_update(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_04_tableinfo - Insert tuple in IIDD_DDB_TABLEINFO
**
** Description:
**	This routine inserts a tuple into IIDD_DDB_TABLEINFO for a link or
**  table.
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
**	08-jun-88 (carl)
**          written
**	20-nov-92 (teresa)
**	    add support for procedures.
**	28-may-93 (barbara)
**	    The "columns_mapped" column in iidd_ddb_tableinfo is also
**	    set to 'Y' if case translation semantics require a mapping
**	    of columns names, i.e. if the LDB is case sensitive and the
**	    DDB is not OR if the LDB s case sensitive and the reg_case
**	    translation semantics of the LDB and DDB are not compatible.
**	09-jul-93 (barbara)
**	    Exclude registered procedures from the case translation checks.
**	    Procedure parameters are not treated like columns for the purposes
**	    of case translation because they are not mapped into Star names.
**	20-oct-93 (barbara)
**	    Adjusted the logic for deciding whether case translation semantics
**	    requires column mapping.  Full explanation is now contained in
**	    the header of qel_06_ldb_columns().
*/


DB_STATUS
qel_04_tableinfo(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *tabinfo_p = ddl_p->qed_d6_tab_info_p;
    QEC_D9_TABLEINFO
		    *tup_p = v_lnk_p->qec_2_tableinfo_p;
    QEC_L16_TABLES  *tables_p = v_lnk_p->qec_9_tables_p;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    i4		    ldb_reg, ldb_delim;


    /* 1.  set up insert information */

    /* tup_p->d9_1_obj_id already set up */

    switch (tabinfo_p->dd_t3_tab_type)
    {
    case DD_2OBJ_TABLE:
	tup_p->d9_2_lcl_type[0] = 'T';
	break;
    case DD_3OBJ_VIEW:
	tup_p->d9_2_lcl_type[0] = 'V';
	break;
    case DD_4OBJ_INDEX:
	tup_p->d9_2_lcl_type[0] = 'I';
	break;
    case DD_5OBJ_REG_PROC:
	tup_p->d9_2_lcl_type[0] = 'P';
	break;
    default:
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }
    tup_p->d9_2_lcl_type[1] = EOS;

    MEcopy(
	tables_p->l16_1_tab_name, 
	sizeof(tables_p->l16_1_tab_name), 
	tup_p->d9_3_tab_name);
    MEcopy(
	tables_p->l16_2_tab_owner, 
	sizeof(tables_p->l16_2_tab_owner), 
	tup_p->d9_4_tab_owner);
    MEcopy(
	tables_p->l16_3_cre_date, 
	sizeof(tables_p->l16_3_cre_date), 
	tup_p->d9_5_cre_date);
    MEcopy(
	tables_p->l16_4_alt_date, 
	sizeof(tables_p->l16_4_alt_date), 
	tup_p->d9_6_alt_date);

    tup_p->d9_7_rel_st1 = tables_p->l16_35_rel_st1;
    tup_p->d9_8_rel_st2 = tables_p->l16_36_rel_st2;

    ldb_reg = tabinfo_p->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.
		dd_c6_name_case;
    ldb_delim = tabinfo_p->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.
		dd_c10_delim_name_case;

    /*
    ** If this is not a registered procedure
    ** AND (psf has indicated that column mapping was used
    **     OR Star and the LDB have conflicting case semantics --
    **	   see qel_06_ldb_columns() header for explanation)
    **
    ** then mark this registration as having mapped column names.
    */
    if (   tabinfo_p->dd_t3_tab_type != DD_5OBJ_REG_PROC
	&&
           (   tabinfo_p->dd_t6_mapped_b
	    || (   ~(*i_qer_p->qef_cb->qef_dbxlate) & CUI_ID_DLM_M
	        && ldb_delim == DD_1CASE_MIXED
	       )
	    ||
	       (   ddl_p->qed_d4_ddb_cols_pp != (DD_COLUMN_DESC **)0
	    	&& *i_qer_p->qef_cb->qef_dbxlate & CUI_ID_DLM_M
	       )
	   )
       )
    {
	tup_p->d9_9_col_mapped[0] = 'Y';
    }
    else
    {
	tup_p->d9_9_col_mapped[0] = 'N';
    }
    tup_p->d9_9_col_mapped[1] = EOS;
    tup_p->d9_10_ldb_id = v_lnk_p->qec_3_ldb_id;

    /*  2.  set up for insert operation */ 

    ins_p->qeq_c1_can_id = INS_618_DD_DDB_TABLEINFO;;
    ins_p->qeq_c3_ptr_u.d9_tableinfo_p = tup_p;
    ins_p->qeq_c4_ldb_p = cdb_p;

    /* 3.  send INSERT query */

    status = qel_i1_insert(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_05_ldbids - update IIDD_DDB_LDBIDS
**
** Description:
**	This function inserts an entry for the LDB in IIDD_DDB_LDBIDS if
**  it is a new member introduced by the link.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**		.qec_c1_ddl_info_p	input information
**		.qed_c2_objbase		new object base (also new LDB id)
**		.qed_c3_ldbid		0 ==> generate new LDB id 
**		.qed_c4_longname_b	TRUE ==> database name exceeds 32 chars
**		.qec_c5_ldb_alias	LDB alias if name exceeds 32 chars
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
*/


DB_STATUS
qel_05_ldbids(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
/*
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
*/
    QEC_D2_LDBIDS   *ldbids_p = v_lnk_p->qec_7_ldbids_p;
    QEC_D5_LONG_LDBNAMES
		    *names_p = v_lnk_p->qec_8_longnames_p;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;


    /* 1.  insert into IIDD_DDB_LONG_LDBNAMES if long LDB name */

    if (v_lnk_p->qec_10_haves & QEC_04_LONG_LDBNAME)
    {
	/* 1.1  set up data */

	qed_u0_trimtail( ldb_p->dd_l3_ldb_name, (u_i4) DD_256_MAXDBNAME,
		names_p->d5_1_ldb_name);

	names_p->d5_2_ldb_id = v_lnk_p->qec_3_ldb_id;	

	qed_u0_trimtail( v_lnk_p->qec_5_ldb_alias, (u_i4) DB_DB_MAXNAME,
		 names_p->d5_3_ldb_alias);

	/* 1.3  insert the tuple */

	ins_p->qeq_c1_can_id = INS_614_DD_DDB_LONG_LDBNAMES;
	ins_p->qeq_c3_ptr_u.d5_long_ldbnames_p = names_p;
	ins_p->qeq_c4_ldb_p = cdb_p;

	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 2.  insert into IIDD_DDB_LDBIDS */

    qed_u0_trimtail( ldb_p->dd_l2_node_name, (u_i4) DB_NODE_MAXNAME,
		ldbids_p->d2_1_ldb_node);

    qed_u0_trimtail( ldb_p->dd_l4_dbms_name, (u_i4) DB_TYPE_MAXLEN,
		ldbids_p->d2_2_ldb_dbms);

    if (v_lnk_p->qec_10_haves & QEC_04_LONG_LDBNAME)
    {
	qed_u0_trimtail( names_p->d5_3_ldb_alias, (u_i4) DB_DB_MAXNAME,
	    ldbids_p->d2_3_ldb_database);

	ldbids_p->d2_4_ldb_longname[0] = 'Y';
	ldbids_p->d2_4_ldb_longname[1] = EOS;
    }
    else
    {
	qed_u0_trimtail( ldb_p->dd_l3_ldb_name, (u_i4) DB_DB_MAXNAME,
	    ldbids_p->d2_3_ldb_database);

	ldbids_p->d2_4_ldb_longname[0] = 'N';
	ldbids_p->d2_4_ldb_longname[1] = EOS;
    }    

    ldbids_p->d2_5_ldb_id = v_lnk_p->qec_3_ldb_id;	

    /* 3.  insert into IIDD_DDB_LDBIDS */

    ins_p->qeq_c1_can_id = INS_606_DD_DDB_LDBIDS;
    ins_p->qeq_c3_ptr_u.d2_ldbids_p = ldbids_p;
    ins_p->qeq_c4_ldb_p = cdb_p;

    status = qel_i1_insert(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_06_ldb_columns - update IIDD_DDB_LDB_COLUMNS
**
** Description:
**	This function inserts the entries for the local columns.
**
** Registered procedures: no mapping.
**
** Registered tables, views, and tables created through Star: mapping
** required if user specified (only possible in the case of registrations) or
** according to following table
**
** Delim Id Case  Delim Id Case   Case xlation for
**     DDB            LDB            entry into     iidd_ddb_ldb_columns
**                                  iidd_columns
**                                (applies to regis-
**                                tered tables only)
** _____________  _____________   ________________  ____________________
**    Upper        Upper/Lower        Upper            No entry
**    Upper        Mixed              Upper          Mixed case entry
**    Lower        Upper/Lower        Lower            No entry
**    Lower        Mixed              Lower          Mixed case entry
**    Mixed        Upper/Lower      * Upper/Lower      No entry **
**    Mixed        Mixed           No case xlation     No entry ***
** ---------------------------------------------------------------------
**
** *   The column name will be case translated following the case
**     translation semantics of regular identifiers.
** **  For tables created through Star, there will be an entry.  This
**     is required because a table created through Star may be later
**     registered with refresh.   Unless there are mapped column names
**     PSF will discard the old (possibly mixed-case) names in iidd_tables.
** *** For tables created through Star, there will be an entry.  This is
**     required because inconsistencies arise:
**	    - where the table was created without using delimited
**            identifiers on column names
**          - and where the same column name gets automatically  delimited
**            by Star on DML.
**
** Inputs:
**      i_qer_p				QEF_RCB
**	v_lnk_p				ptr to QEC_LINK
**		.qec_1_ddl_info_p	input information
**		.qed_c2_obj_base	object base 
**		.qed_c3_obj_index	object index
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
**	27-may-93 (barbara)
**	    Star delimited id support.  If LDB supports mixed case
**	    delimited ids and DDB does not, provide a mapping of
**	    column names.
**	19-jul-93 (barbara)
**	    No mapping for registered procedures.
**	19-oct-93 (barbara)
**	    After testing, revisited logic of where LDB column names
**	    require mapping.  Explanation of rules is now in header.
*/


DB_STATUS
qel_06_ldb_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *tabinfo_p = ddl_p->qed_d6_tab_info_p;
    QEC_D3_LDB_COLUMNS	
		    ldb_cols,			/* tuple struct */
		    *ldbcols_p = & ldb_cols;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
    		    *ins_p = v_lnk_p->qec_22_insert_p;
    DD_COLUMN_DESC  **cols_pp = tabinfo_p->dd_t8_cols_pp;
    QEC_L3_COLUMNS  dd_columns,
		    *ddcolumns_p = & dd_columns;
    i4		    tupcnt;
    i4		    ldb_delim = tabinfo_p->dd_t9_ldb_p->dd_i2_ldb_plus.
				dd_p3_ldb_caps.dd_c10_delim_name_case;

    /* See comments in header */

    if (   tabinfo_p->dd_t3_tab_type == DD_5OBJ_REG_PROC
	||
           (   !tabinfo_p->dd_t6_mapped_b
	    && (   *i_qer_p->qef_cb->qef_dbxlate & CUI_ID_DLM_M
	        || ldb_delim != DD_1CASE_MIXED
	       )
	    &&
	       (   ddl_p->qed_d4_ddb_cols_pp == (DD_COLUMN_DESC **)0
	    	|| (*i_qer_p->qef_cb->qef_dbxlate & CUI_ID_DLM_M) == 0
	       )
	   )
       )
    {
	return(E_DB_OK);
    }

    if (   tabinfo_p->dd_t6_mapped_b
	&& ddl_p->qed_d3_col_count != tabinfo_p->dd_t7_col_cnt
       )
    {
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    if (!tabinfo_p->dd_t6_mapped_b)
    {
	/* Set up for looping through local columns from iicolumns */
	qed_u0_trimtail( tabinfo_p->dd_t1_tab_name, (u_i4) DB_TAB_MAXNAME,
			ddcolumns_p->l3_1_tab_name);
	qed_u0_trimtail( tabinfo_p->dd_t2_tab_owner, (u_i4) DB_OWN_MAXNAME,
			ddcolumns_p->l3_2_tab_owner);
				
	sel_p->qeq_c1_can_id = SEL_121_II_COLNAMES;
	sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
	sel_p->qeq_c3_ptr_u.l3_columns_p = ddcolumns_p;
	sel_p->qeq_c4_ldb_p = ldb_p;

	/* send SELECT query and fetch first tuple */

	status = qel_s4_prepare(i_qer_p, v_lnk_p);
	if (status)
	    return(status);

	if (sel_p->qeq_c5_eod_b)
	{
	    /* no data! */
	    status = qed_u1_gen_interr(&i_qer_p->error);
	    return(status);
	}
    }

    /* set data for loop */

    STRUCT_ASSIGN_MACRO(
	v_lnk_p->qec_2_tableinfo_p->d9_1_obj_id, 
	ldbcols_p->d3_1_obj_id);

    ins_p->qeq_c1_can_id = INS_607_DD_DDB_LDB_COLUMNS;
    ins_p->qeq_c3_ptr_u.d3_ldb_columns_p = ldbcols_p;
    ins_p->qeq_c4_ldb_p = cdb_p;

    cols_pp++;		/* always start with 1st element */
    tupcnt = 0;

    while (1)
    {
	char	*ldb_colname;

    	if (tabinfo_p->dd_t6_mapped_b)
	{
	    ldb_colname = (*cols_pp)->dd_c1_col_name;
	}
	else
	{
	    ldb_colname = ddcolumns_p->l3_3_col_name;
	}

	qed_u0_trimtail( ldb_colname, (u_i4) DB_ATT_MAXNAME,
		    ldbcols_p->d3_2_lcl_name);

	ldbcols_p->d3_3_seq_in_row = tupcnt + 1;

	/* Insert the tuple */
	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	{
	    /* must flush sending LDB */

	    STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
    	    ignore = qel_s3_flush(i_qer_p, v_lnk_p);
	    STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
	    return(status);
	}

	tupcnt += 1;

    	if (tabinfo_p->dd_t6_mapped_b)
	{
	    if (tupcnt >= ddl_p->qed_d3_col_count)
		break;
	    cols_pp++;
	}
	else
	{
	    status = qel_s2_fetch(i_qer_p, v_lnk_p);
	    if (status)
		return (status);
	    if (sel_p->qeq_c5_eod_b && ! status)
		break;
	}

    }

    return(E_DB_OK);
}    


/*{
** Name: qel_07_ldb_dbcaps - Update IIDD_DDB_LDB_DBCAPS 
**
** Description:
**	For a newly arrived LDB, this function reads the known
**  capabilities from the LDB for insertion into IIDD_DDB_LDB_DBCAPS 
**  and subsequent reflection in IIDD_DBCAPABILITIES if necessary: 
**
**	COMMON/SQL_LEVEL, INGRE/QUEL_LEVEL, INGRES/SQL_LEVEL, SAVEPOINTS 
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
**	08-jun-88 (carl)
**          written
**	22-jan-89 (carl)
**	    modified to include OWNER_NAME, etc.
**	21-jun-91 (teresa)
**	    added support for QEK_14CAP_TID_LEVEL to fix bug 34684.
**	01-mar-93 (barbara)
**	    Support for delimited identifiers.  Get OPEN/SQL_LEVEL and
**	    DB_DELIMITED_CASE capabilities from local LDB.
**	02-sep-93 (barbara)
**	    Get DB_REAL_USER_CASE capability from LDB for support of delimited
**	    identifiers in Star.
*/


DB_STATUS
qel_07_ldb_dbcaps(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    save_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p,
		    *upd_p = v_lnk_p->qec_23_update_p;
    QEC_MIN_CAP_LVLS
		    *mins_p = v_lnk_p->qec_25_pre_mins_p;
    i4		    cap_code,
		    val_code;
    bool	    insert_b;
    QEC_D4_LDB_DBCAPS	
		    ddcaps,		/* tuple struct */
		    *ddcaps_p = & ddcaps;
    QEC_L4_DBCAPABILITIES
		    iicaps,		/* tuple struct */
		    *iicaps_p = & iicaps;


    /* set up for loop */

    ddcaps_p->d4_1_ldb_id = v_lnk_p->qec_3_ldb_id;

    sel_p->qeq_c1_can_id = SEL_104_II_DBCAPABILITIES;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l4_dbcapabilities_p = iicaps_p;
    sel_p->qeq_c4_ldb_p = ldb_p;
	    
    ins_p->qeq_c1_can_id = INS_608_DD_DDB_LDB_DBCAPS;
    ins_p->qeq_c2_rqf_bind_p = NULL;
    ins_p->qeq_c3_ptr_u.d4_ldb_dbcaps_p = ddcaps_p;
    ins_p->qeq_c4_ldb_p = cdb_p;

    upd_p->qeq_c1_can_id = 0;		    /* to  be filled in when used */
    upd_p->qeq_c2_rqf_bind_p = NULL;
    upd_p->qeq_c3_ptr_u.l4_dbcapabilities_p = iicaps_p;
					    /* input is output from SELECT */
    upd_p->qeq_c4_ldb_p = cdb_p;

    MEfill(sizeof(iicaps), '\0', (PTR) & iicaps);

    /* 1.  start by sending a SELECT query to get data stream ready */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (sel_p->qeq_c5_eod_b)
    {
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    while ( (! status) && (! sel_p->qeq_c5_eod_b) )
    {
	insert_b = FALSE;

	/* copy data from iicaps to ddcaps */
	MEcopy( iicaps_p->l4_1_cap_cap, DB_CAP_MAXLEN,
	    ddcaps_p->d4_2_cap_cap);
	    
	MEcopy( iicaps_p->l4_2_cap_val, DB_CAPVAL_MAXLEN,
	    ddcaps_p->d4_3_cap_val);

	ddcaps_p->d4_2_cap_cap[DB_CAP_MAXLEN] = EOS;	/* null terminate */
	cap_code = qek_c1_str_to_code(iicaps_p->l4_1_cap_cap);

	/* 2.  convert cap_value to cap_level if necessary */

	switch (cap_code)
	{
	case QEK_1CAP_COMMONSQL_LEVEL:
	case QEK_6CAP_INGRESQUEL_LEVEL:
	case QEK_7CAP_INGRESSQL_LEVEL:
	case QEK_15CAP_OPENSQL_LEVEL:

	    ddcaps_p->d4_3_cap_val[DB_CAPVAL_MAXLEN] = EOS;	
						/* null terminate */
	    ddcaps_p->d4_4_cap_lvl = 
		(i4) qek_c2_val_to_code(ddcaps_p->d4_3_cap_val);
	    insert_b = TRUE;	    
	    break;

	case QEK_5CAP_INGRES:
	case QEK_8CAP_SAVEPOINTS:
	case QEK_9CAP_SLAVE2PC:
	case QEK_10CAP_TIDS:

	    switch (ddcaps_p->d4_3_cap_val[0])
	    {
	    case 'Y':
	    case 'y':

		ddcaps_p->d4_4_cap_lvl = QEK_1FOR_Y;
		break;

	    default:

		ddcaps_p->d4_4_cap_lvl = QEK_0FOR_N;
		break;

	    }
		
	    insert_b = TRUE;	    
	    break;

	case QEK_2CAP_DBMS_TYPE:

	    val_code = qek_c1_str_to_code(ddcaps_p->d4_3_cap_val);
	    switch (val_code)
	    {
	    case QEK_5CAP_INGRES:
		ddcaps_p->d4_4_cap_lvl = QEK_0DBMS_INGRES_Y;
		break;

	    default:
		ddcaps_p->d4_4_cap_lvl = QEK_1DBMS_INGRES_N;
		break;

	    } /* end of switch 2 */

	    insert_b = TRUE;	    
	    break;

	case QEK_3CAP_DB_NAME_CASE:
	case QEK_16CAP_DELIM_NAME_CASE:
	case QEK_17CAP_REAL_USER_CASE:

	    val_code = qek_c1_str_to_code(ddcaps_p->d4_3_cap_val);
	    switch (val_code)
	    {
	    case QEK_5VAL_LOWER:		/* LOWER */
		ddcaps_p->d4_4_cap_lvl = DD_0CASE_LOWER;
		break;

	    case QEK_6VAL_MIXED:		/* MIXED */
		ddcaps_p->d4_4_cap_lvl = DD_1CASE_MIXED;
		break;

	    case QEK_7VAL_UPPER:		/* UPPER */
		ddcaps_p->d4_4_cap_lvl = DD_2CASE_UPPER;
		break;

	    default:
		status = qed_u1_gen_interr(& i_qer_p->error);
		return(status);

	    } /* end of switch 2 */
		
	    insert_b = TRUE;	    
	    break;

	case QEK_4CAP_DISTRIBUTED:
	case QEK_11CAP_UNIQUE_KEY_REQ:

	    switch (ddcaps_p->d4_3_cap_val[0])
	    {
	    case 'Y':
	    case 'y':

		ddcaps_p->d4_4_cap_lvl = QEK_0IS_LO;
		break;

	    default:

		ddcaps_p->d4_4_cap_lvl = QEK_1IS_HI;
		break;

	    }
		
	    insert_b = TRUE;	    
	    break;

	case QEK_12CAP_OWNER_NAME:

	    val_code = qek_c1_str_to_code(ddcaps_p->d4_3_cap_val);
	    switch (val_code)
	    {
	    case QEK_8VAL_NO:			/* NO */
		ddcaps_p->d4_4_cap_lvl = 0 /* DD_0OWN_NO */;
		break;

	    case QEK_9VAL_YES:			/* YES */
		ddcaps_p->d4_4_cap_lvl = 1 /* DD_1OWN_YES */;
		break;

	    case QEK_10VAL_QUOTED:		/* QUOTED */
		ddcaps_p->d4_4_cap_lvl = 2 /* DD_2OWN_QUOTED */;
		break;

	    default:
		status = qed_u1_gen_interr(& i_qer_p->error);
		return(status);

	    } /* end of switch 2 */
		
	    insert_b = TRUE;	    
	    break;

	case QEK_13CAP_PHYSICAL_SOURCE:

	    switch (ddcaps_p->d4_3_cap_val[0])
	    {
	    case 'P':
	    case 'p':

		ddcaps_p->d4_4_cap_lvl = QEK_0AS_P;
		break;

	    default:

		ddcaps_p->d4_4_cap_lvl = QEK_1AS_T;
		break;

	    }
		
	    insert_b = TRUE;	    
	    break;

	case QEK_14CAP_TID_LEVEL:
	    
	    /* currently, star simply wants to know whether or not tids are
	    ** supported.  If there is a non "00000" value for cap_value, then
	    ** assume tids are supported.  In later releases we may wish to
	    ** refine this and distinguish between the level of support --
	    ** this is a fix for bug 34684 */
	    ddcaps_p->d4_4_cap_lvl = qek_c1_str_to_code(ddcaps_p->d4_3_cap_val);

	    insert_b = TRUE;
	    break;

	default:				/* ignore this capability */

	    insert_b = FALSE;	    
	    break;

	} /* end of switch 1 */

	/* 3.  insert into IIDD_DDB_LDB_DBCAPS if necessary */

	if (insert_b)
	{
	    /* 3.1  insert into IIDD_DDB_LDB_DBCAPS */
    
	    v_lnk_p->qec_10_haves |= QEC_09_LDBCAPS;
	    status = qel_i1_insert(i_qer_p, v_lnk_p);
	    if (status)
	    {
		if (! sel_p->qeq_c5_eod_b)
		{
		    /* 3.2  must flush data stream before returning */

		    STRUCT_ASSIGN_MACRO(i_qer_p->error, save_error);
		    ignore = qel_s3_flush(i_qer_p, v_lnk_p);
		    STRUCT_ASSIGN_MACRO(save_error , i_qer_p->error);
		}
		return(status);		    
	    }
	    
	    /* 3.2   update IIDD_DBCAPABIITIES if current LDB capability
	    **	     comes in at a lower level */

	    switch (cap_code)
	    {
	    case QEK_1CAP_COMMONSQL_LEVEL:

		if (ddcaps_p->d4_4_cap_lvl < mins_p->qec_m1_com_sql)
		{
		    /* 3.3  update COMMON/SQL_LEVEL */

		    upd_p->qeq_c1_can_id = UPD_800_COM_SQL_LVL;
		    status = qel_u1_update(i_qer_p, v_lnk_p);
		}
		break;

	    case QEK_6CAP_INGRESQUEL_LEVEL:

		if (ddcaps_p->d4_4_cap_lvl < mins_p->qec_m2_ing_quel)
		{
		    /* 3.4  update INGRES/QUEL_LEVEL */

		    upd_p->qeq_c1_can_id = UPD_801_ING_QUEL_LVL;
		    status = qel_u1_update(i_qer_p, v_lnk_p);
		}
		break;
		
	    case QEK_7CAP_INGRESSQL_LEVEL:

		if (ddcaps_p->d4_4_cap_lvl < mins_p->qec_m3_ing_sql)
		{
		    /* 3.5  update INGRES/SQL_LEVEL */

		    upd_p->qeq_c1_can_id = UPD_802_ING_SQL_LVL;
		    status = qel_u1_update(i_qer_p, v_lnk_p);
		}
		break;

	    case QEK_8CAP_SAVEPOINTS:

		if (ddcaps_p->d4_4_cap_lvl < mins_p->qec_m4_savepts)
		{
		    /* 3.6  update SAVEPOINTS */

		    upd_p->qeq_c1_can_id = UPD_803_SAVEPOINTS;
		    status = qel_u1_update(i_qer_p, v_lnk_p);
		}
		break;

	    case QEK_11CAP_UNIQUE_KEY_REQ:

		if (ddcaps_p->d4_4_cap_lvl < mins_p->qec_m5_uniqkey)
		{
		    /* 3.7  update UNIQUE_KEY_REQ */

		    upd_p->qeq_c1_can_id = UPD_804_UNIQUE_KEY_REQ;
		    status = qel_u1_update(i_qer_p, v_lnk_p);
		}
		break;

	    case QEK_15CAP_OPENSQL_LEVEL:

		if (ddcaps_p->d4_4_cap_lvl < mins_p->qec_m6_opn_sql)
		{
		    /* 3.6 update OPEN/SQL_LEVEL */

		    upd_p->qeq_c1_can_id = UPD_805_OPN_SQL_LVL;
		    status = qel_u1_update(i_qer_p, v_lnk_p);
		}
		break;

	    default:
		break;

	    } /* end of switch */

	    if (status)				/* test result if any */
	    {
		if (! sel_p->qeq_c5_eod_b)
		{
		    /* 3.2  must flush data stream before returning */

		    STRUCT_ASSIGN_MACRO(i_qer_p->error, save_error);
		    ignore = qel_s3_flush(i_qer_p, v_lnk_p);
		    STRUCT_ASSIGN_MACRO(save_error , i_qer_p->error);
		}
		return(status);		    
	    }
	}

	/* 4.  read next tuple until EOD */

	MEfill(sizeof(iicaps), '\0', (PTR) & iicaps);

        sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);


    } /* end while */

    return(status);
}

