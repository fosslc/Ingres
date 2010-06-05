/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <cm.h>
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
**  Name: QEL3ST.C - Routines updating STANDARD catalogs for CREATE LINK
**
**  Description:
**      These routines perform STANDARD catalog updates for CREATE LINK.
**
**	    qel_31_std_cats	    - perform updates on STANDARD catalogs
**	    qel_32_tables	    - promote info to IIDD_TABLES
**	    qel_33_columns	    - promote info to IIDD_COLUMNS and
**				      IIDD_DDB_LDB COLUMNS if mapping columns
**	    qel_34_altcols	    - promote info to IIDD_ALT_COLUMNS
**	    qel_35_registrations    - insert query text to 
**				      IIDD_DDB_REGISTRATIONS
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog redefinition
**	22-apr-89 (carl)
**	    fixed qel_32_tables to set STATS field of IITABLES to 'N' in
**	    the case where the LDB has a different architecture
**	30-apr-89 (carl)
**	    changed qel_33_columns to suppress consistency check for
**	    column count in the case of REFRESH
**	02-may-89 (carl)
**	    support iidd_registrations text for CREATE TABLE
**	03-jul-90 (carl)
**	    fixed qel_32_tables to initialize both table_permits and 
**	    table_integrities to 'N' which is the proper STAR-level value
**	17-jul-90 (carl)
**	    fixed qel_32_tables to set l16_10_indexes to 'N' for gateways
**	04-aug-90 (carl)
**	    1) switched to use new unique global variable name IIQE_42_ing_60
**	    2) changed qel_32_tables to assign 'S' to system_use in 
**	       iidd_tables if link name starts with "ii" and "+Y" is in effect
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

GLOBALREF   char	IIQE_42_ing_60[];


/*{
** Name: qel_31_std_cats - Perform updates on non-index STANDARD catalogs.
**
** Description:
**	This function does all updates on the non-index STANDARD catalogs 
**  using supplied information.
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
**	02-may-89 (carl)
**	    support iidd_registrations text for CREATE TABLE
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_31_std_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;


    if (v_lnk_p->qec_10_haves & QEC_08_NO_IITABLES)
    {
	/* must be index from non-ingres gateway */

	if (
		(! (v_lnk_p->qec_10_haves & QEC_03_INGRES))
		||
		(ddl_p->qed_d6_tab_info_p->dd_t3_tab_type != DD_4OBJ_INDEX)
	    )
	{
	    status = qed_u1_gen_interr(& i_qer_p->error);
	    return(status);
	}
    }
    else if (ddl_p->qed_d6_tab_info_p->dd_t3_tab_type != DD_5OBJ_REG_PROC)
    {
	
	/* 1.  promote into IIDD_TABLES unless it is a registered procedure */

	status = qel_32_tables(i_qer_p, v_lnk_p);
	if (status)
	    return(status);

	/* 2.  promote into IIDD_COLUMNS */

	status = qel_33_columns(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    if ( (ddl_p->qed_d6_tab_info_p->dd_t3_tab_type != DD_3OBJ_VIEW) &&
	 (ddl_p->qed_d6_tab_info_p->dd_t3_tab_type != DD_5OBJ_REG_PROC)
       )
    {
	/* 4.  promote into IIDD_ALT_COLUMNS */

	status = qel_34_altcols(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    if (ddl_p->qed_d9_reg_info_p != NULL)	/* query text to insert 
						** into iidd_registrations */
	status = qel_35_registrations(i_qer_p, v_lnk_p);

    return(status);
}    


/*{
** Name: qel_32_tables - Insert to IIDD_TABLES
**
** Description:
**	This routine inserts an entry in IIDD_TABLES using information
**  retrieved from the LDB's IITABLES.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
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
**	05-jan-89 (carl)
**	    set ALL_TO_ALL to Y for all distributed objects with the 
**	    exception of indexes 
**	22-apr-89 (carl)
**	    fixed to set STATS field of IITABLES to 'N' in the case 
**	    where the LDB has a different architecture
**	03-jul-90 (carl)
**	    fixed qel_32_tables to initialize both table_permits and 
**	    table_integrities to 'N' which is the proper STAR-level value
**	17-jul-90 (carl)
**	    set l16_10_indexes to 'N' for gateways
**	04-aug-90 (carl)
**	    assign 'S' to iidd_tables.system_use  if link name starts
**	    with "ii" and "+Y" is in effect
**	07-dec-93 (barbara)
**	    Make test for "ii" case insensitive.
*/


DB_STATUS
qel_32_tables(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *tabinfo_p = ddl_p->qed_d6_tab_info_p;
    QEC_L16_TABLES  *tables_p = v_lnk_p->qec_9_tables_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p;
						/* working structure */
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    u_i4	    l_obj;


    if (v_lnk_p->qec_10_haves & QEC_08_NO_IITABLES)
	return(E_DB_OK);

    /* 1.  set up to retrieve from IITABLES */

    qed_u0_trimtail( tabinfo_p->dd_t1_tab_name, (u_i4) DB_TAB_MAXNAME,
		    tables_p->l16_1_tab_name);
    qed_u0_trimtail( tabinfo_p->dd_t2_tab_owner, (u_i4) DB_OWN_MAXNAME,
		    tables_p->l16_2_tab_owner);

    sel_p->qeq_c1_can_id = SEL_117_II_TABLES;
    sel_p->qeq_c3_ptr_u.l16_tables_p = tables_p;
    sel_p->qeq_c4_ldb_p = ldb_p;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;

    /* 2.  send SELECT query and fetch first tuple */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);
        
    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    if (v_lnk_p->qec_27_select_cnt != 1)
    {
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    if (v_lnk_p->qec_10_haves & QEC_10_USE_PHY_SRC)
    {
	/* 3.  read remaining information from the LDB's IIPHYSICAL_TABLES */

	sel_p->qeq_c1_can_id = SEL_113_II_PHYSICAL_TABLES;

	/* 3.1  send SELECT query and fetch first tuple */

	status = qel_s4_prepare(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
        
	if (! sel_p->qeq_c5_eod_b)
	{
	    status = qel_s3_flush(i_qer_p, v_lnk_p);
	    if (status)
		return(status);
	}

	if (v_lnk_p->qec_27_select_cnt != 1)
	{
	    status = qed_u1_gen_interr(& i_qer_p->error);
	    return(status);
	}
    }

    if (v_lnk_p->qec_10_haves & QEC_11_LDB_DIFF_ARCH)
    {
	/* binary statistics data cannot be propagated for use due to 
	** different LDB architecture */

	tables_p->l16_9_stats[0] = 'N';		/* declare no statistics */
    }
    else if (!
	(
	(tables_p->l16_9_stats[0] == 'N')
	||
	(tables_p->l16_9_stats[0] == 'n')
	))
	v_lnk_p->qec_10_haves |= QEC_05_STATS;

    if (!
	(
	(tables_p->l16_10_indexes[0] == 'N')
	||
	(tables_p->l16_10_indexes[0] == 'n')
	))
	v_lnk_p->qec_10_haves |= QEC_02_INDEXES;

    /* 3.  set up tuple for insertion */

    l_obj = (u_i4)qed_u0_trimtail( ddl_p->qed_d1_obj_name, (u_i4)DB_OBJ_MAXNAME,
		tables_p->l16_1_tab_name);

    qed_u0_trimtail( ddl_p->qed_d2_obj_owner, (u_i4) DB_OWN_MAXNAME,
		tables_p->l16_2_tab_owner);

    STcopy(v_lnk_p->qec_24_cur_time, 
	    tables_p->l16_3_cre_date);
    STcopy(v_lnk_p->qec_24_cur_time, 
	    tables_p->l16_4_alt_date);

    if (tabinfo_p->dd_t3_tab_type == DD_2OBJ_TABLE)
	tables_p->l16_5_tab_type[0] = 'T';	/* local table */
    else if (tabinfo_p->dd_t3_tab_type == DD_3OBJ_VIEW)
	tables_p->l16_5_tab_type[0] = 'V';	/* local view */
    else if (tabinfo_p->dd_t3_tab_type == DD_4OBJ_INDEX)
	tables_p->l16_5_tab_type[0] = 'I';	/* local index */
    else 
    {
	status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB,
		    & i_qer_p->error);
	return(status);
    }
    tables_p->l16_5_tab_type[1] = EOS;		/* null terminate */

    if (ddl_p->qed_d8_obj_type == DD_1OBJ_LINK)
   	tables_p->l16_6_sub_type[0] = 'L';	/* DDB link */
    else if (ddl_p->qed_d8_obj_type == DD_2OBJ_TABLE
	     ||
	     ddl_p->qed_d8_obj_type == DD_3OBJ_VIEW)
	tables_p->l16_6_sub_type[0] = 'N';	/* DDB native */
    else
    {
	status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB,
		    & i_qer_p->error);
	return(status);
    }
    tables_p->l16_6_sub_type[1] = EOS;		/* null terminate */

    STcopy(IIQE_42_ing_60, tables_p->l16_7_version);

    if (dds_p->qes_d9_ctl_info & QES_05CTL_SYSCAT_USER)
    {
	char *ch2 = ddl_p->qed_d1_obj_name + CMbytecnt(ddl_p->qed_d1_obj_name);

	if( ( l_obj >= 2 ) &&
	    (CMcmpnocase(ddl_p->qed_d1_obj_name, "i") == 0 ) &&
	    (CMcmpnocase(ch2, "i") == 0 ))
	{
	    tables_p->l16_8_sys_use[0] = 'S';	/* system object */

	}
	else
	    tables_p->l16_8_sys_use[0] = 'U';	/* user object */
    }
    else
	tables_p->l16_8_sys_use[0] = 'U';	/* user object */
    tables_p->l16_8_sys_use[1] = EOS;		/* null terminate */

    if (! (v_lnk_p->qec_10_haves & QEC_03_INGRES) )
    {
	/* index information NOT propagated for gateways */
	tables_p->l16_10_indexes[0] = 'N';	/* N */
	tables_p->l16_10_indexes[1] = EOS;	/* null terminate */
    }
    tables_p->l16_23_integrities[0] = 'N';	/* always N */
    tables_p->l16_23_integrities[1] = EOS;	/* null terminate */

    tables_p->l16_24_permits[0] = 'N';		/* always N */
    tables_p->l16_24_permits[1] = EOS;	/* null terminate */

    if (tabinfo_p->dd_t3_tab_type == DD_4OBJ_INDEX)
	tables_p->l16_25_all_to_all[0] = 'N';	/* always N if an index */
    else
	tables_p->l16_25_all_to_all[0] = 'Y';	/* always Y otherwise */
    tables_p->l16_25_all_to_all[1] = EOS;	/* null terminate */

    /* 4.  insert into IIDD_TABLES */

    ins_p->qeq_c1_can_id = INS_631_DD_TABLES;	
    ins_p->qeq_c3_ptr_u.l16_tables_p = tables_p;	
    ins_p->qeq_c4_ldb_p = cdb_p;
    status = qel_i1_insert(i_qer_p, v_lnk_p);

    return(status);
}    


/*{
** Name: qel_33_columns - Insert to IIDD_COLUMNS 
**
** Description:
**	This function inserts entries into IIDD_COLUMNS for the link.
**  Insertion into IIDD_DDB_LDB_COLUMNS in the event of column mapping
**  is done by qel_06_ldb_columns in QEL0DD.C.
**
** Inputs:
**      i_qer_p				    QEF_RCB
**	v_lnk_p				    ptr to QEC_LINK
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
**	30-apr-89 (carl)
**	    suppress consistency check for column count in the case of REFRESH
**	    since the LDB table's original schema can be a proper subset of the 
**	    new schema
**	01-mar-93 (barbara)
**	    Delimited id support.  Always assume there are mapped column
**	    names.  Even if user didn't specify a mapping, PSF will provide
**	    a mapping in case of incompatible case translation semantics
**	    between DDB and LDB.
**	18-may-93 (davebf)
**	    Get UDT supporting columns into IIDD_COLUMNS if special datatype
**	28-may-93 (barbara)
**	    Delimited id support.  Changed the way things were done on 1-mar-93.
**	    PSF will only provide us with a list of DDL columns if user
**	    explicity mapped columns on a REGISTER statement, or if columns
**	    are specified on a CREATE.   If PSF doesn't supply column names,
**	    QEF must map column names into appropriate case for DDB.
*/


DB_STATUS
qel_33_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *ldbtab_p = ddl_p->qed_d6_tab_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    QEC_L3_COLUMNS  dd_columns,
		    *ddcolumns_p = & dd_columns;	/* tuple struct */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p;
    DD_COLUMN_DESC  **col_descs_pp = ddl_p->qed_d4_ddb_cols_pp,
		    *name_p;
    i4		    tupcnt;

    if (v_lnk_p->qec_10_haves & QEC_08_NO_IITABLES)
	return(E_DB_OK);

    /* 1.  set up for looping through each retrieved local column */

    qed_u0_trimtail( ldbtab_p->dd_t1_tab_name, (u_i4) DB_TAB_MAXNAME,
		    ddcolumns_p->l3_1_tab_name);
    qed_u0_trimtail( ldbtab_p->dd_t2_tab_owner, (u_i4) DB_OWN_MAXNAME,
		    ddcolumns_p->l3_2_tab_owner);

    sel_p->qeq_c1_can_id = SEL_102_II_COLUMNS;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l3_columns_p = ddcolumns_p;
    sel_p->qeq_c4_ldb_p = ldb_p;

    ddcolumns_p->l3_15_internal_ingtype = 0;    /* init in case not ingres */

    /* 2.  send SELECT query and fetch first tuple */

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

    if (col_descs_pp)
    {
	col_descs_pp++;			/* must start from slot 1 */
	name_p = *col_descs_pp;
    }

    while (! sel_p->qeq_c5_eod_b && ! status)
    {
	/* 4.  do column mapping if necessary  */

	if (col_descs_pp)
	{

	    /* 4.  set up for insertion */

	    /* substitue the link column name for inserting into IIDD_COLUMNS */

	    qed_u0_trimtail( name_p->dd_c1_col_name, (u_i4) DB_ATT_MAXNAME,
			ddcolumns_p->l3_3_col_name);
	}
	else
	{
	    qel_a2_case_col(i_qer_p, ddcolumns_p->l3_3_col_name,
			    *i_qer_p->qef_cb->qef_dbxlate,
			    &ldbtab_p->dd_t9_ldb_p->dd_i2_ldb_plus.
			    dd_p3_ldb_caps);
	}

	/* 4+. handle special datatypes (if any) */

	if(ddcolumns_p->l3_15_internal_ingtype >= 16384 ||
	   ddcolumns_p->l3_15_internal_ingtype <= -16384)    /* user datatype */
	{
	    /* copy internal type, length, ingtype into external for 6.2 cats */
	    MEcopy((PTR)ddcolumns_p->l3_13_internal_datatype,
		   DB_TYPE_MAXLEN +1,
		   (PTR)ddcolumns_p->l3_4_data_type);
	    ddcolumns_p->l3_5_length = ddcolumns_p->l3_14_internal_length;
	    ddcolumns_p->l3_12_ing_datatype =
					ddcolumns_p->l3_15_internal_ingtype;
	}
	ddcolumns_p->l3_15_internal_ingtype = 0;    /* init for next */


	/* 5.  replace with link name, link owner, and link creation date */

	qed_u0_trimtail( ddl_p->qed_d1_obj_name, (u_i4) DB_OBJ_MAXNAME,
			ddcolumns_p->l3_1_tab_name);
	qed_u0_trimtail( ddl_p->qed_d2_obj_owner, (u_i4) DB_OWN_MAXNAME,
			ddcolumns_p->l3_2_tab_owner);

	/* 6.  insert into IIDD_COLUMNS */

	ins_p->qeq_c1_can_id = INS_602_DD_COLUMNS;
	ins_p->qeq_c3_ptr_u.l3_columns_p = ddcolumns_p;
	ins_p->qeq_c4_ldb_p = cdb_p;
	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	{
	    /* must flush sending LDB */

	    STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
	    ignore = qel_s3_flush(i_qer_p, v_lnk_p);
	    STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
	    return(status);
	}

	tupcnt++;

	if (col_descs_pp)
	{
	    col_descs_pp++;			/* increment to pick up the
						** next DD_COLUMN_DESC */
	    name_p = *col_descs_pp;
	}

	/* 3.  retrieve next tuple */

	sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);

    } /* end while */

    /*	the following check cannot be applied in the following 2 cases:
    **
    **	1)  an INGRES index because the count from IIINDEX_COLUMNS may be 
    **	    one less than the count from II_COLUMNS which includes "tidp" 
    **	    as an column,
    **	2)  on a REFRESH on an LDB table whose old schema can be a proper
    **	    subset of the new schema */

    if (ldbtab_p->dd_t3_tab_type == DD_2OBJ_TABLE
	&&
	dds_p->qes_d7_ses_state != QES_4ST_REFRESH)
    {
	if (tupcnt != ddl_p->qed_d3_col_count)
	{
	    status = qed_u1_gen_interr(& i_qer_p->error);
	    return(status);
	}
    }

    return(status);
}    


/*{
** Name: qel_34_altcols - Insert to IIDD_ALT_COLUMNS.
**
** Description:
**	This routine retrieves from the LDB and inserts the data into above 
**  table under the link name and owner.  
**
** Inputs:
**      i_qer_p			    QEF_RCB
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
**	08-jun-88 (carl)
**          written
**	01-mar-93 (barbara)
**	    Delimited id support.  Always assume there are mapped column
**	    names.  Even if user didn't specify a mapping, PSF will provide
**	    a mapping in case of incompatible case translation semantics
**	    between DDB and LDB.
**	19-jul-93 (barbara)
**	    Delimited id support.  Changed the way things were done on 1-mar-93.
**	    PSF will only provide us with a list of DDL columns if user
**	    explicity mapped columns on a REGISTER statement, or if columns
**	    are specified on a CREATE.   If PSF doesn't supply column names,
**	    QEF must map column names into appropriate case for DDB.
*/


DB_STATUS
qel_34_altcols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *ldbtab_p = ddl_p->qed_d6_tab_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_L2_ALT_COLUMNS
		    altcols,
		    *altcols_p = & altcols;	/* tuple struct */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p;
    DD_COLUMN_DESC  **col_descs_pp = ddl_p->qed_d4_ddb_cols_pp,
		    *from_p;
    i4		    tupcnt;


    /* 1.  set up for looping through each retrieved IIALT_COLUMNS tuple */

    qed_u0_trimtail( ldbtab_p->dd_t1_tab_name, (u_i4) DB_TAB_MAXNAME,
		    altcols_p->l2_1_tab_name);
    qed_u0_trimtail( ldbtab_p->dd_t2_tab_owner, (u_i4) DB_OWN_MAXNAME,
		    altcols_p->l2_2_tab_owner);

    sel_p->qeq_c1_can_id = SEL_101_II_ALT_COLUMNS;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l2_alt_columns_p = altcols_p;
    sel_p->qeq_c4_ldb_p = ldb_p;

    /* 2.  send SELECT query and fetch first tuple */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);
        
    if (sel_p->qeq_c5_eod_b)
	return(E_DB_OK);			/* no date */

    tupcnt = 0;

    if (col_descs_pp)
    {
	col_descs_pp++;			/* must start from slot 1 */
	from_p = *col_descs_pp;
    }

    while (! sel_p->qeq_c5_eod_b && ! status)
    {
	/* 3.  replace with link name and link owner */

	qed_u0_trimtail( ddl_p->qed_d1_obj_name, (u_i4) DB_OBJ_MAXNAME,
			altcols_p->l2_1_tab_name);
	qed_u0_trimtail( ddl_p->qed_d2_obj_owner, (u_i4) DB_OWN_MAXNAME,
			altcols_p->l2_2_tab_owner);
	
	/* 4.  do column mapping if necessary */

	if (col_descs_pp)
	{
	    /* substitute link col name from inserting into IIDD_ALT_COLUMNS */

	    qed_u0_trimtail( from_p->dd_c1_col_name, (u_i4) DB_ATT_MAXNAME,
			altcols_p->l2_4_col_name);
	}
	else
	{
	    qel_a2_case_col(i_qer_p, altcols_p->l2_4_col_name,
			    *i_qer_p->qef_cb->qef_dbxlate,
			    &ldbtab_p->dd_t9_ldb_p->dd_i2_ldb_plus.
			    dd_p3_ldb_caps);
	}
			

	/* 5.  insert into IIDD_ALT_COLUMNS */

	ins_p->qeq_c1_can_id = INS_601_DD_ALT_COLUMNS;
	ins_p->qeq_c3_ptr_u.l2_alt_columns_p = altcols_p;
	ins_p->qeq_c4_ldb_p = cdb_p;

	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	{
	    /* must flush sending LDB */

	    STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
	    ignore = qel_s3_flush(i_qer_p, v_lnk_p);
	    STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
	    return(status);
	}

	tupcnt++;
	
	if (col_descs_pp)
	{
	    col_descs_pp++;			/* increment to pick up the
						** next DD_COLUMN_DESC */
	    from_p = *col_descs_pp;
	}

	/* 6.  read next tuple from LDB */

	sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    return(status);
}    


/*{
** Name: qel_35_registrations - Insert to IIDD_DDB_REGISTRATIONS
**
** Description:
**	This function inserts entries into above catalog for a link.
**
** Inputs:
**      i_qer_p			    QEF_RCB
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
**	07-nov-88 (carl)
**          written
**	02-may-89 (carl)
**	    support iidd_registrations text for CREATE TABLE
**	19-nov-92 (teresa)
**	    Add support for registered procedures
*/

DB_STATUS
qel_35_registrations(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    QED_QUERY_INFO  *qry_p;
    DD_2LDB_TAB_INFO
		    *tab_p = ddl_p->qed_d6_tab_info_p;
    QEC_L14_REGISTRATIONS
		    regs,
		    *regs_p = & regs;
    QEC_L18_PROCEDURES
		    procs,
		    *proc_p = & procs;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    DD_PACKET	    *pkt_p;
    i4		    seq;


    /* insert query text only for CREATE LINK | TABLE */

    if (ddl_p->qed_d9_reg_info_p == NULL)
	return(E_DB_OK);			/* no query text to insert */

    switch (ddl_p->qed_d8_obj_type)
    {
    case DD_1OBJ_LINK:
    case DD_2OBJ_TABLE:
    case DD_5OBJ_REG_PROC:
	break;
    default:
	return(E_DB_OK);			/* no query text to insert */
    }

    qry_p = ddl_p->qed_d9_reg_info_p;

    /* 1.  set up constant values for all entries */

    qed_u0_trimtail( ddl_p->qed_d1_obj_name, (u_i2) DB_OBJ_MAXNAME,
	regs_p->l14_1_obj_name);

    qed_u0_trimtail( ddl_p->qed_d2_obj_owner, (u_i2) DB_OWN_MAXNAME,
	regs_p->l14_2_obj_owner);

    if (qry_p->qed_q2_lang == DB_SQL)
	regs_p->l14_3_dml[0] = 'S';
    else if (qry_p->qed_q2_lang == DB_QUEL)
	regs_p->l14_3_dml[0] = 'Q';
    else
    {
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }
    regs_p->l14_3_dml[1] = EOS;

    if (tab_p->dd_t3_tab_type == DD_2OBJ_TABLE)
    {
	regs_p->l14_4_obj_type[0] = 'T';	/* local table */
    }
    else if (tab_p->dd_t3_tab_type == DD_3OBJ_VIEW)
    {
	regs_p->l14_4_obj_type[0] = 'V';	/* local view */
    }
    else if (tab_p->dd_t3_tab_type == DD_4OBJ_INDEX)
    {
	regs_p->l14_4_obj_type[0] = 'I';	/* local index */
    }
    else if (tab_p->dd_t3_tab_type == DD_5OBJ_REG_PROC)
    {
	regs_p->l14_4_obj_type[0] = 'P';	/* local index */
    }
    else
    {
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }
    regs_p->l14_4_obj_type[1] = EOS;
    regs_p->l14_5_obj_subtype[0] = 'L';		/* must set to link */
    regs_p->l14_5_obj_subtype[1] = EOS;

    pkt_p = qry_p->qed_q4_pkt_p;

    for (seq = 0; pkt_p != NULL; seq++)
    {
	/* 2.  construct text segment for current entry */

	regs_p->l14_6_sequence = seq + 1;
	regs_p->l14_7_pkt_p = pkt_p;

	if (pkt_p->dd_p1_len > QEK_256_VIEW_SIZE)
	{
	    status = qed_u1_gen_interr(& i_qer_p->error);
	    return(status);
	}

	/* 3.  insert into IIDD_DDB_REGISTRATIONS */

	ins_p->qeq_c1_can_id = INS_629_DD_REGISTRATIONS;
	ins_p->qeq_c3_ptr_u.l14_registrations_p = regs_p;
	ins_p->qeq_c4_ldb_p = cdb_p;
	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	    return(status);

	pkt_p = pkt_p->dd_p3_nxt_p;		/* advance to next segment,
						** if any */
    } 

    /* if a registered procedure, update iiprocedures */
    if (tab_p->dd_t3_tab_type == DD_5OBJ_REG_PROC)
    {
	qed_u0_trimtail( ddl_p->qed_d1_obj_name, (u_i2) DB_OBJ_MAXNAME,
	    proc_p->l18_1_proc_name);

	qed_u0_trimtail( ddl_p->qed_d2_obj_owner, (u_i2) DB_OWN_MAXNAME,
	    proc_p->l18_2_proc_owner);

	STcopy(v_lnk_p->qec_24_cur_time, proc_p->l18_3_cre_date);
	proc_p->l18_4_proc_subtype[0] = 'L';		/* must set to link */
	proc_p->l18_4_proc_subtype[1] = EOS;

	pkt_p = qry_p->qed_q4_pkt_p;

	for (seq = 0; pkt_p != NULL; seq++)
	{
	    /* 3.  construct text segment for current entry */

	    proc_p->l18_5_sequence = seq + 1;
	    proc_p->l18_6_pkt_p = pkt_p;

	    if (pkt_p->dd_p1_len > QEK_256_VIEW_SIZE)
	    {
		status = qed_u1_gen_interr(& i_qer_p->error);
		return(status);
	    }

	    /* 4.  insert into IIDD_DDB_REGISTRATIONS */

	    ins_p->qeq_c1_can_id = INS_633_DD_PROCEDURES;
	    ins_p->qeq_c3_ptr_u.l18_procedures_p = proc_p;
	    ins_p->qeq_c4_ldb_p = cdb_p;
	    status = qel_i1_insert(i_qer_p, v_lnk_p);
	    if (status)
		return(status);

	    pkt_p = pkt_p->dd_p3_nxt_p;		/* advance to next segment,
						    ** if any */
	} 
    }

    return(E_DB_OK);
}    

