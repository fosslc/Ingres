/*
** Copyright (c) 1988, 2009 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <me.h>
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
#include    <cv.h>
#include    <st.h>
#include    <tm.h>
#include    <cm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QELINS.C - Utility routines for inserting into a CDB catalog
**
**  Description:
**	    qel_i1_insert   - insert a tuple into a CDB catalog
**	    qel_i2_query    - generate query for inserting CDB catalog tuple 
**	    qel_i3_vdata    - generate query with data values
**
**
**  History:
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog redefinition
**	29-jan-89 (carl)
**	    modified for gateways using upper-case naming convention
**	03-apr-89 (carl)
**	    modified to get rid of trailing blanks from insert queries
**	20-sep-90 (carl)
**	    added trace point processing to qel_i1_insert
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	16-dec-04 (inkdo01)
**	    Add a variety of collID's.
**      05-feb-2009 (joea)
**          Rename CM_ischarsetUTF8 to CMischarset_utf8.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*{
** Name: QEL_I1_INSERT - Insert a tuple into a CDB catalog.
**
** Description:
**	This routine inserts a tuple as directed into the appropriate
**  CDB catalog by calling RQF.  It assumes that a TPF call has been
**  made to register the proper access with the CDB.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_22_insert_p	    QEC_LINK
**		.qec_c1_can_id	    CDB catalog id
**		.qec_c3_ptr_u.	    ptr to tuple data
**		.qeq_c4_ldb_p	    ptr to CDB
**
** Outputs:
**	none
**	
**	v_qer_p				
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
**	20-sep-90 (carl)
**	    added trace point processing
**	20-nov-92 (teresa)
**	    support registered procedures
*/


DB_STATUS
qel_i1_insert(
QEF_RCB		*v_qer_p, 
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    DD_LDB_DESC	    *ldb_p = ins_p->qeq_c4_ldb_p;
    i4		    ins_id = ins_p->qeq_c1_can_id,
		    rqr_opcode;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    bool            log_ddl_56 = FALSE,
		    log_err_59 = FALSE;
    i4	    i4_1,
		    i4_2;
    DB_DATA_VALUE   dv[QEC_CAT_COL_COUNT_MAX];
    char	    qrytxt[QEK_900_LEN];


    if ((ldb_p != cdb_p)
	||
	(!(INS_601_DD_ALT_COLUMNS <= ins_id && ins_id <= INS_633_DD_PROCEDURES))
       )
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_DDL_56,
	    & i4_1, & i4_2))
    {
        log_ddl_56 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    ins_p->qeq_c7_qry_p = qrytxt;
    ins_p->qeq_c8_dv_cnt = 0;
    ins_p->qeq_c9_dv_p = dv;

    /* 1.  set up insert query */

    switch (ins_id)
    {
    case INS_619_DD_DDB_TREE:
    case INS_620_DD_HISTOGRAMS:
    case INS_629_DD_REGISTRATIONS:
    case INS_630_DD_STATS:
    case INS_632_DD_VIEWS:
    case INS_633_DD_PROCEDURES:
	rqr_opcode = RQR_DATA_VALUES;
	status = qel_i3_vdata(v_qer_p, v_lnk_p);
	break;
    default:
	rqr_opcode = RQR_NORMAL;
	status = qel_i2_query(v_qer_p, v_lnk_p);
    }
    if (status)
	return(status);

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;	/* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = v_lnk_p->qec_22_insert_p->qeq_c4_ldb_p;
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;
    rqr_p->rqr_dv_cnt = ins_p->qeq_c8_dv_cnt;
    rqr_p->rqr_dv_p = dv;
    rqr_p->rqr_inv_parms = (PTR) NULL;

    if (log_ddl_56)
	qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying CDB info */
    /* 3.  execute query */

    status = qed_u3_rqf_call(rqr_opcode, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (! log_ddl_56))
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying CDB info */
    }    

    return(status);
}


/*{
** Name: QEL_I2_QUERY - Generate insert query.
**
** Description:
**	This routine generates a query for insertion from the given information.
**
** Inputs:
**	v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_22_insert_p	    QEC_LINK
**		.qec_c1_can_id	    CDB catalog id
**		.qec_c3_ptr_u	    ptr to tuple data
**		.qeq_c4_ldb_p	    ptr to CDB
**
** Outputs:
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_22_insert_p	    QEC_LINK
**		.qec_c7_qry_p	    query text
**	v_qer_p				
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
**	29-jan-89 (carl)
**	    modified for gateways using upper-case naming convention
**	03-apr-89 (carl)
**	    modified to get rid of trailing blanks from insert queries
**	28-sep-89 (carl)
**	    rollback 7.0 iicolumns changes
**	04-jun-93 (barbara)
**	    Removed logic that lowered column names.  That is now done
**	    in qel_33_columns.
**	06-mar-96 (nanpr01)
**	    Standard catalogue interface change for variable page size.
**      27-jul-96 (ramra01)
**          Standard catalogue changes for Alter table project.
**          Added relversion and reltotwid to the iitables.
**	26-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri (table cache priority) to iitables.
*/

DB_STATUS
qel_i2_query(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    i4		    ins_id = ins_p->qeq_c1_can_id;
    char	    *cat_name_p = NULL;		/* must initialize */


    switch (ins_id)
    {
    case INS_601_DD_ALT_COLUMNS:
	{
	    QEC_L2_ALT_COLUMNS	*altcols_p = 
		ins_p->qeq_c3_ptr_u.l2_alt_columns_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_01_ALT_COLUMNS];

	    /*	insert into iidd_alt_columns [(table_name, table_owner,
	    **	key_id, column_name, key_sequence)] values 
	    **	(c32, c32, i4, c32, i2); */
	
	    altcols_p->l2_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(altcols_p->l2_1_tab_name);

	    altcols_p->l2_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(altcols_p->l2_2_tab_owner);

	    altcols_p->l2_4_col_name[DB_ATT_MAXNAME] = EOS;
	    STtrmwhite(altcols_p->l2_4_col_name);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
	    	"%s %s %s %s ('%s', '%s', %d, '%s', %d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		altcols_p->l2_1_tab_name,	
		altcols_p->l2_2_tab_owner,	
		altcols_p->l2_3_key_id,
		altcols_p->l2_4_col_name,	
		altcols_p->l2_5_seq_in_key);
	}
	break;

    case INS_602_DD_COLUMNS:
	{
	    QEC_L3_COLUMNS  *columns_p = 
		ins_p->qeq_c3_ptr_u.l3_columns_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_02_COLUMNS];


	    /*  insert into iidd_columns [(table_name, table_owner, 
	    **	column_name, column_datatype, column_length, column_scale, 
	    **  column_nulls, column_defaults, column_sequence, key_sequence,
	    **  sort_direction, column_ingdatatype, column_internal_datatype,
	    **  column_internal_length, column_internal_ingtype,
	    **  column_system_maintained)] values 
	    **	(c32, c32, c32, c32, i4, i4, c8, c8, i4, i4, c8, i4,
	    **   c32, i4, i4, c8); */
	
	    /* trim trailing white spaces */

	    columns_p->l3_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(columns_p->l3_1_tab_name);

	    columns_p->l3_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(columns_p->l3_2_tab_owner);

	    columns_p->l3_3_col_name[DB_ATT_MAXNAME] = EOS;
	    STtrmwhite(columns_p->l3_3_col_name);

	    columns_p->l3_4_data_type[DB_TYPE_MAXLEN] = EOS;
	    STtrmwhite(columns_p->l3_4_data_type);

	    columns_p->l3_7_nulls[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(columns_p->l3_7_nulls);

	    columns_p->l3_8_defaults[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(columns_p->l3_8_defaults);

	    columns_p->l3_11_sort_dir[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(columns_p->l3_11_sort_dir);

	    columns_p->l3_13_internal_datatype[DB_TYPE_MAXLEN] = EOS;
	    STtrmwhite(columns_p->l3_13_internal_datatype);

	    columns_p->l3_16_system_maintained[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(columns_p->l3_16_system_maintained);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
"%s %s %s %s ('%s','%s','%s','%s',%d,%d,'%s','%s',%d,%d,'%s',%d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		columns_p->l3_1_tab_name,	
		columns_p->l3_2_tab_owner,	
		columns_p->l3_3_col_name,
		columns_p->l3_4_data_type,
		columns_p->l3_5_length,
		columns_p->l3_6_scale,
		columns_p->l3_7_nulls,
		columns_p->l3_8_defaults,
		columns_p->l3_9_seq_in_row,
		columns_p->l3_10_seq_in_key,
		columns_p->l3_11_sort_dir,
		columns_p->l3_12_ing_datatype,
		columns_p->l3_13_internal_datatype,
		columns_p->l3_14_internal_length,
		columns_p->l3_15_internal_ingtype,
		columns_p->l3_16_system_maintained);
	}
	break;

    case INS_605_DD_DDB_DBDEPENDS:
	{
	    QEC_D1_DBDEPENDS   *dbdepends_p = 
		ins_p->qeq_c3_ptr_u.d1_dbdepends_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_05_DDB_DBDEPENDS];

	    /* insert into iidd_ddb_dbdepends values
	    ** (i4, i4, i4, i4, i4, i4, i4); */
	
	    STprintf(
		ins_p->qeq_c7_qry_p, 
	    	"%s %s %s %s (%d, %d, %d, %d, %d, %d, %d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		dbdepends_p->d1_1_inid1,	
		dbdepends_p->d1_2_inid2,	
		dbdepends_p->d1_3_itype,	
		dbdepends_p->d1_4_deid1,	
		dbdepends_p->d1_5_deid2,	
		dbdepends_p->d1_6_dtype,	
		dbdepends_p->d1_7_qid);	
	}
	break;

    case INS_606_DD_DDB_LDBIDS:
	{
	    QEC_D2_LDBIDS   *ldbids_p = 
		ins_p->qeq_c3_ptr_u.d2_ldbids_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_06_DDB_LDBIDS];

	    /* insert into iidd_ddb_ldbids [(ldb_node, ldb_dbms, ldb_database, 
	    ** ldb_longname, ldb_id, ldb_user, ldb_dba, ldb_dbaname)] values 
	    ** (c32, c32, c32, c8, i4, c8, c8, c32); */
	
	    ldbids_p->d2_1_ldb_node[DB_NODE_MAXNAME] = EOS;
	    STtrmwhite(ldbids_p->d2_1_ldb_node);

	    ldbids_p->d2_2_ldb_dbms[DB_TYPE_MAXLEN] = EOS;
	    STtrmwhite(ldbids_p->d2_2_ldb_dbms);

	    ldbids_p->d2_3_ldb_database[DD_256_MAXDBNAME] = EOS;
	    STtrmwhite(ldbids_p->d2_3_ldb_database);

	    ldbids_p->d2_4_ldb_longname[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(ldbids_p->d2_4_ldb_longname);

	    ldbids_p->d2_6_ldb_user[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(ldbids_p->d2_6_ldb_user);

	    ldbids_p->d2_7_ldb_dba[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(ldbids_p->d2_7_ldb_dba);

	    ldbids_p->d2_8_ldb_dbaname[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(ldbids_p->d2_8_ldb_dbaname);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
	    	"%s %s %s %s ('%s', '%s', '%s', '%s', %d, '%s', '%s', '%s');",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		ldbids_p->d2_1_ldb_node,	
		ldbids_p->d2_2_ldb_dbms,	
		ldbids_p->d2_3_ldb_database,	
		ldbids_p->d2_4_ldb_longname,	
		ldbids_p->d2_5_ldb_id,
		ldbids_p->d2_6_ldb_user,	
		ldbids_p->d2_7_ldb_dba,
		ldbids_p->d2_8_ldb_dbaname);	
	}
	break;

    case INS_607_DD_DDB_LDB_COLUMNS:
	{
	    QEC_D3_LDB_COLUMNS  *ldbcols_p = 
		ins_p->qeq_c3_ptr_u.d3_ldb_columns_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_07_DDB_LDB_COLUMNS];

	    /*
	    ** insert into iidd_ddb_ldb columns 
	    ** values (i4, i4, c32, i4);
	    */	

	    ldbcols_p->d3_2_lcl_name[DB_ATT_MAXNAME] = EOS;
	    STtrmwhite(ldbcols_p->d3_2_lcl_name);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
		"%s %s %s %s (%d, %d, '%s', %d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		ldbcols_p->d3_1_obj_id.db_tab_base,
		ldbcols_p->d3_1_obj_id.db_tab_index,
		ldbcols_p->d3_2_lcl_name,
		ldbcols_p->d3_3_seq_in_row);	
	}
	break;

    case INS_608_DD_DDB_LDB_DBCAPS:
	{
	    QEC_D4_LDB_DBCAPS	*ldb_dbcaps_p= 
		ins_p->qeq_c3_ptr_u.d4_ldb_dbcaps_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_08_DDB_LDB_DBCAPS];

	    /*	insert into iidd_ldb_dbcaps [(ldb_id, cap_capability, 
	    **	cap_value, cap_level)] values (i4, c32, c32, i4); */
	
	    ldb_dbcaps_p->d4_2_cap_cap[DB_CAP_MAXLEN] = EOS;
	    STtrmwhite(ldb_dbcaps_p->d4_2_cap_cap);

	    ldb_dbcaps_p->d4_3_cap_val[DB_CAPVAL_MAXLEN] = EOS;
	    STtrmwhite(ldb_dbcaps_p->d4_3_cap_val);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
	    	"%s %s %s %s (%d, '%s', '%s', %d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		ldb_dbcaps_p->d4_1_ldb_id,	
		ldb_dbcaps_p->d4_2_cap_cap,	
		ldb_dbcaps_p->d4_3_cap_val,	
		ldb_dbcaps_p->d4_4_cap_lvl);
	}
	break;

    case INS_614_DD_DDB_LONG_LDBNAMES:
	{
	    QEC_D5_LONG_LDBNAMES    *long_p = 
		ins_p->qeq_c3_ptr_u.d5_long_ldbnames_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_14_DDB_LONG_LDBNAMES];

	    /*
	    ** insert into iidd_ddb_long_ldbnames (long_ldbname, long_ldbid,
	    ** long_ldbalias) values (c256, i4, c32); 
	    */

	    long_p->d5_1_ldb_name[DD_256_MAXDBNAME] = EOS;
	    STtrmwhite(long_p->d5_1_ldb_name);

	    long_p->d5_3_ldb_alias[DB_DB_MAXNAME] = EOS;
	    STtrmwhite(long_p->d5_3_ldb_alias);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
		"%s %s %s %s ('%s', %d, '%s');",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		long_p->d5_1_ldb_name,	
		long_p->d5_2_ldb_id,	
		long_p->d5_3_ldb_alias);	
	}
	break;				

    case INS_615_DD_DDB_OBJECTS: 
	{
	    QEC_D6_OBJECTS  *objs_p = 
		ins_p->qeq_c3_ptr_u.d6_objects_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_15_DDB_OBJECTS];

	    /*
	    ** insert into iidd_objects values 
	    ** (c32, c32, i4, i4, i4, i4, c25, c8, c25, c8, c8, c25);
	    */	

	    STprintf(
		ins_p->qeq_c7_qry_p, 
"%s %s %s %s ('%s', '%s', %d, %d, %d, %d, '%s', '%s', '%s', '%s', '%s', '%s');",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		objs_p->d6_1_obj_name,
		objs_p->d6_2_obj_owner,
		objs_p->d6_3_obj_id.db_tab_base,
		objs_p->d6_3_obj_id.db_tab_index,
		objs_p->d6_4_qry_id.db_qry_high_time,
		objs_p->d6_4_qry_id.db_qry_low_time,
		objs_p->d6_5_obj_cre,
		objs_p->d6_6_obj_type,
		objs_p->d6_7_obj_alt,
		objs_p->d6_8_sys_obj,
		objs_p->d6_9_to_expire,
		objs_p->d6_10_exp_date);
	}
	break;

    case INS_618_DD_DDB_TABLEINFO:
	{
	    QEC_D9_TABLEINFO	*tabinfo_p = 
		ins_p->qeq_c3_ptr_u.d9_tableinfo_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_18_DDB_TABLEINFO];

	    /*
	    **	insert into iidd_ddb_tableinfo (object_base, object_index,
	    **	local_type, table_name, table_owner, table_date, table_alter,
	    **	columns_mapped, ldb_id) values 
	    **	(i4, i4, c8, c32, c32, c25, c25, c8, i4);
	    */	

	    tabinfo_p->d9_2_lcl_type[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tabinfo_p->d9_2_lcl_type);

	    tabinfo_p->d9_3_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(tabinfo_p->d9_3_tab_name);

	    tabinfo_p->d9_4_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(tabinfo_p->d9_4_tab_owner);

	    tabinfo_p->d9_9_col_mapped[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tabinfo_p->d9_9_col_mapped);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
		"%s %s %s %s (%d,%d,'%s','%s','%s','%s','%s',%d,%d,'%s',%d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		tabinfo_p->d9_1_obj_id.db_tab_base,
		tabinfo_p->d9_1_obj_id.db_tab_index,
		tabinfo_p->d9_2_lcl_type,
		tabinfo_p->d9_3_tab_name,
		tabinfo_p->d9_4_tab_owner,
		tabinfo_p->d9_5_cre_date,
		tabinfo_p->d9_6_alt_date,
		tabinfo_p->d9_7_rel_st1,
		tabinfo_p->d9_8_rel_st2,	
		tabinfo_p->d9_9_col_mapped,
		tabinfo_p->d9_10_ldb_id);	
	}
	break;

    case INS_621_DD_INDEXES:
	{
	    QEC_L6_INDEXES  *indexes_p = 
		ins_p->qeq_c3_ptr_u.l6_indexes_p;

	    if (cat_name_p == NULL)
		cat_name_p = IIQE_c8_ddb_cat[TDD_21_INDEXES];

	    /*  insert into iidd_ddb_ldb_indexes | iidd_indexes values 
	    **	(c32, c32, c25, c32, c32, c16, c8, c8); */
	
		indexes_p->l6_1_ind_name[DB_TAB_MAXNAME] = EOS;
		STtrmwhite(indexes_p->l6_1_ind_name);	

		indexes_p->l6_2_ind_owner[DB_OWN_MAXNAME] = EOS;	
		STtrmwhite(indexes_p->l6_2_ind_owner);	

		indexes_p->l6_4_base_name[DB_TAB_MAXNAME] = EOS;
		STtrmwhite(indexes_p->l6_4_base_name);

		indexes_p->l6_5_base_owner[DB_OWN_MAXNAME] = EOS;
		STtrmwhite(indexes_p->l6_5_base_owner);

		indexes_p->l6_6_storage[QEK_16_STOR_SIZE] = EOS;
		STtrmwhite(indexes_p->l6_6_storage);

		indexes_p->l6_7_compressed[QEK_8_CHAR_SIZE] = EOS;
		STtrmwhite(indexes_p->l6_7_compressed);

		indexes_p->l6_8_uniquerule[QEK_8_CHAR_SIZE] = EOS;
		STtrmwhite(indexes_p->l6_8_uniquerule);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
"%s %s %s %s ('%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		indexes_p->l6_1_ind_name,	
		indexes_p->l6_2_ind_owner,	
		indexes_p->l6_3_cre_date,
		indexes_p->l6_4_base_name,
		indexes_p->l6_5_base_owner,
		indexes_p->l6_6_storage,
		indexes_p->l6_7_compressed,
		indexes_p->l6_8_uniquerule,
		indexes_p->l6_9_pagesize);
	}
	break;

    case INS_622_DD_INDEX_COLUMNS:
	{
	    QEC_L7_INDEX_COLUMNS  *ndx_cols_p = 
		ins_p->qeq_c3_ptr_u.l7_index_columns_p;


	    if (cat_name_p == NULL)
		cat_name_p = IIQE_c8_ddb_cat[TDD_22_INDEX_COLUMNS];

	    /*  insert into iidd_ddb_ldb_xcolumns | iidd_index_columns
	    **	[(index_name, index_owner, column_name, key_sequence,
	    **	sort_direction)] values (c32, c32, c32, i4, c8); */
	
	    ndx_cols_p->l7_1_ind_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(ndx_cols_p->l7_1_ind_name);

	    ndx_cols_p->l7_2_ind_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(ndx_cols_p->l7_2_ind_owner);	

	    ndx_cols_p->l7_3_col_name[DB_ATT_MAXNAME] = EOS;
	    STtrmwhite(ndx_cols_p->l7_3_col_name);  

	    ndx_cols_p->l7_5_sort_dir[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(ndx_cols_p->l7_5_sort_dir);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
	    	"%s %s %s %s ('%s', '%s', '%s', %d, '%s');",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		ndx_cols_p->l7_1_ind_name,	
		ndx_cols_p->l7_2_ind_owner,	
		ndx_cols_p->l7_3_col_name,
		ndx_cols_p->l7_4_key_seq,
		ndx_cols_p->l7_5_sort_dir);
	}
	break;

    case INS_631_DD_TABLES:
	{
	    QEC_L16_TABLES	*tables_p = 
		ins_p->qeq_c3_ptr_u.l16_tables_p;

	    char		*buf_p;
	    u_i4		len;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_31_TABLES];

	    /*
	    **	insert into iidd_tables 
	    **  (table_name, table_owner, create_date, alter_date, table_type, 
	    **  table_subtype, table_version, system_use, table_stats, 
	    **  table_indexes, is_readonly, num_rows, storage_structure, 
	    **  is_compressed, duplicate_rows, unique_rule, number_pages, 
	    **  overflow_pages, row_width, expire_date, modify_date, 
	    **  location_name, table_integrities, table_permits, all_to_all, 
	    **  ret_to_all, is_journalled, view_base, multi_locations, 
	    **  table_ifillpct, table_dfillpct, table_lfillpct, table_minpages, 
	    **  table_maxpages, table_relstamp1, table_relstamp2, table_reltid, 
	    **	table_reltidx, table_pagesize) values
	    **	(c32, c32, c25, c25, c8,    <1..5>
	    **  c8, c8, c8, c8,		    <6..9>
	    **  c8, c8, i4, c16,	    <10..13>
	    **  c8, c8, c8, i4,		    <14..17>
	    **  i4, i4, i4, c25,	    <18..21>
	    **  c24, c8, c8, c8,	    <22..25>
	    **  c8, c8, c8, c8,		    <26..29>
	    **  i2, i2, i2, i4, 	    <30..33>
	    **	i4, i4, i4, i4, 	    <34..37>
	    **	i4, i4);		    <38..39>
	    */

	    buf_p = ins_p->qeq_c7_qry_p;

	    /* generate first of 4 query text portions */

	    tables_p->l16_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(tables_p->l16_1_tab_name);

	    tables_p->l16_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(tables_p->l16_2_tab_owner);

	    tables_p->l16_5_tab_type[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_5_tab_type);

	    tables_p->l16_6_sub_type[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_6_sub_type);

	    tables_p->l16_7_version[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_7_version);

	    tables_p->l16_8_sys_use[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_8_sys_use);

	    tables_p->l16_9_stats[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_9_stats);

	    STprintf(
		buf_p,
"%s %s %s %s ('%s','%s','%s','%s','%s','%s','%s','%s','%s',",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		tables_p->l16_1_tab_name,
		tables_p->l16_2_tab_owner,
		tables_p->l16_3_cre_date,
		tables_p->l16_4_alt_date,
		tables_p->l16_5_tab_type,
		tables_p->l16_6_sub_type,
		tables_p->l16_7_version,
		tables_p->l16_8_sys_use,
		tables_p->l16_9_stats);

	    /* generate second of 4 query text portions */
	    	
	    len = STlength(ins_p->qeq_c7_qry_p);
	    buf_p = ins_p->qeq_c7_qry_p + len;

	    tables_p->l16_10_indexes[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_10_indexes);

	    tables_p->l16_11_readonly[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_11_readonly);

	    tables_p->l16_13_storage[QEK_16_STOR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_13_storage);

	    tables_p->l16_14_compressed[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_14_compressed);

	    tables_p->l16_15_dup_rows[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_15_dup_rows);

	    tables_p->l16_16_uniquerule[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_16_uniquerule);

	    STprintf(
		buf_p,
		"'%s','%s',%d,'%s','%s','%s','%s',%d,%d,%d,",
		tables_p->l16_10_indexes,
		tables_p->l16_11_readonly,
		tables_p->l16_12_num_rows,
		tables_p->l16_13_storage,
		tables_p->l16_14_compressed,
		tables_p->l16_15_dup_rows,
		tables_p->l16_16_uniquerule,
		tables_p->l16_17_num_pages,
		tables_p->l16_18_overflow,
		tables_p->l16_19_row_width);

	    /* generate third of 4 query text portions */
	    	
	    len = STlength(ins_p->qeq_c7_qry_p);
	    buf_p = ins_p->qeq_c7_qry_p + len;

	    tables_p->l16_22_loc_name[QEK_24_LOCN_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_22_loc_name);

	    tables_p->l16_23_integrities[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_23_integrities);

	    tables_p->l16_24_permits[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_24_permits);

	    tables_p->l16_25_all_to_all[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_25_all_to_all);

	    /* grant implicit read permission */
	    tables_p->l16_26_ret_to_all[0] = 'Y';
	    tables_p->l16_26_ret_to_all[1] = EOS;

	    tables_p->l16_27_journalled[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_27_journalled);

	    tables_p->l16_28_view_base[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_28_view_base);

	    tables_p->l16_29_multi_loc[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(tables_p->l16_29_multi_loc);


	    STprintf(
		buf_p,
		"%d,'%s','%s','%s','%s','%s','%s','%s','%s','%s',",
		tables_p->l16_20_tab_expire,
		tables_p->l16_21_mod_date,
		tables_p->l16_22_loc_name,
		tables_p->l16_23_integrities,
		tables_p->l16_24_permits,
		tables_p->l16_25_all_to_all,
		tables_p->l16_26_ret_to_all,
		tables_p->l16_27_journalled,
		tables_p->l16_28_view_base,
		tables_p->l16_29_multi_loc);

	    /* generate last of 4 query text portions */
	    	
	    len = STlength(ins_p->qeq_c7_qry_p);
	    buf_p = ins_p->qeq_c7_qry_p + len;

	    STprintf(
		buf_p,
		"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d);",
		tables_p->l16_30_ifillpct,
		tables_p->l16_31_dfillpct,
		tables_p->l16_32_lfillpct,
		tables_p->l16_33_minpages,
		tables_p->l16_34_maxpages,
		tables_p->l16_35_rel_st1,
		tables_p->l16_36_rel_st2,
		tables_p->l16_37_reltid,
		tables_p->l16_38_reltidx,
		tables_p->l16_39_pagesize,
		tables_p->l16_40_relversion,
		tables_p->l16_41_reltotwid,
		tables_p->l16_42_reltcpri);
	}
	break;				

    default:
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
	break;
    }


    return(E_DB_OK);
}

/*{
** Name: QEL_I3_VDATA - Generate insert query with data values.
**
** Description:
**	This routine generates a query for insertion from the given information.
**
** Inputs:
**	v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_22_insert_p	    QEC_LINK
**		.qec_c1_can_id	    CDB catalog id
**		.qec_c3_ptr_u	    ptr to tuple data
**
** Outputs:
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_22_insert_p	    QEC_LINK
**		.qeq_c7_qry_p	    ptr to query buffer of sufficient size
**		.qeq_c8_dv_cnt	    number of data values in query
**		.qeq_c9_dv_p	    DB_DATA_VALUEs set up
**	v_qer_p				
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
**	29-jan-89 (carl)
**	    modified for gateways using upper-case naming convention
**	03-apr-89 (carl)
**	    modified to get rid of trailing blanks from insert queries
**	19-nov-92 (teresa)
**	    added INS_633_DD_PROCEDURES
**	04-jun-93 (barbara)
**	    Removed logic that lowered column names.  That is now done
**	    in qel_33_columns.
**	23-jul-93 (rganski)
**	    Added new iidd_stats columns to insert statement for that table.
**	22-nov-93 (rganski)
**	    Fix for b56798: initialize new iistats columns to defaults when LDB
**	    is pre-6.5.
**	23-may-2008 (gupsh01)
**	    On UTF8 installations, star catalogs contain varchar columns for
**	    query tree and histograms. To avoid corruption of these values 
**	    insert query tree and histograms as varbyte and byte values.
*/


DB_STATUS
qel_i3_vdata(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEQ_1CAN_QRY    *ins_p = v_lnk_p->qec_22_insert_p;
    i4		    ins_id = ins_p->qeq_c1_can_id;
    DB_DATA_VALUE   *dv_p = ins_p->qeq_c9_dv_p;
    char	    *cat_name_p = NULL;		/* must initialize */
    bool	    forUTF8 = FALSE;

    if (CMischarset_utf8())
      forUTF8 = TRUE;

    switch (ins_id)
    {
    case INS_619_DD_DDB_TREE:
	{
	    QEC_D10_TREE    *tree_p = 
		ins_p->qeq_c3_ptr_u.d10_tree_p;


	    /* 1.  construct query text */

	    cat_name_p = IIQE_c8_ddb_cat[TDD_19_DDB_TREE];

	    /*
	    **	insert into iidd_ddb_tree values 
	    **	(i4, i4, i4, i4, i2, i2, i2, ~V );
	    */	

	    STprintf(
		ins_p->qeq_c7_qry_p, 
		"%s %s %s %s (%d, %d, %d, %d, %d, %d, %d, ~V );",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		tree_p->d10_1_treetabbase,
		tree_p->d10_2_treetabidx,
		tree_p->d10_3_treeid1,
		tree_p->d10_4_treeid2,
	    	tree_p->d10_5_treeseq,
	    	tree_p->d10_6_treemode,
	    	tree_p->d10_7_treevers);

	    /* 2.  construct data value */

	    ins_p->qeq_c8_dv_cnt = 1;

	    dv_p->db_data = (PTR) & tree_p->d10_8_treetree[0];
	    dv_p->db_length = tree_p->d10_9_treesize;

	    if (forUTF8)
	      dv_p->db_datatype = DB_VBYTE_TYPE;
	    else
	      dv_p->db_datatype = DB_VCH_TYPE;

	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;
	}
	break;				

    case INS_620_DD_HISTOGRAMS:
	{
	    QEC_L5_HISTOGRAMS	*histos_p = 
		ins_p->qeq_c3_ptr_u.l5_histograms_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_20_HISTOGRAMS];

	    /*  insert into iidd_histograms 
	    **	[(table_name, table_owner, column_name, text_sequence,
	    **	text_segment)] values (c32, c32, c32, i4, c228); */
	
	    histos_p->l5_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(histos_p->l5_1_tab_name);

	    histos_p->l5_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(histos_p->l5_2_tab_owner);

	    histos_p->l5_3_col_name[DB_ATT_MAXNAME] = EOS;
	    STtrmwhite(histos_p->l5_3_col_name);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
	    	"%s %s %s %s ('%s', '%s', '%s', %d, ~V );",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		histos_p->l5_1_tab_name,	
		histos_p->l5_2_tab_owner,	
		histos_p->l5_3_col_name,
		histos_p->l5_4_txt_seq);

	    /* 2.  construct data value for RQF */

	    ins_p->qeq_c8_dv_cnt = 1;

	    dv_p->db_data = (PTR) & histos_p->l5_5_txt_seg[0];
	    dv_p->db_length = QEK_228_HIST_SIZE;
	    if (forUTF8)
	      dv_p->db_datatype = DB_BYTE_TYPE;
	    else
	      dv_p->db_datatype = DB_CHA_TYPE;
	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;
	}
	break;

    case INS_629_DD_REGISTRATIONS:
	{
	    QEC_L14_REGISTRATIONS	*regs_p = 
		ins_p->qeq_c3_ptr_u.l14_registrations_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_29_REGISTRATIONS];

	    /*
	    **	insert into iidd_registrations values
	    **	(c32, c32, c8, c8, c8, i4, ~V );
	    */	

	    regs_p->l14_1_obj_name[DB_OBJ_MAXNAME] = EOS;
	    STtrmwhite(regs_p->l14_1_obj_name);

	    regs_p->l14_2_obj_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(regs_p->l14_2_obj_owner);

	    regs_p->l14_3_dml[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(regs_p->l14_3_dml);

	    regs_p->l14_4_obj_type[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(regs_p->l14_4_obj_type);

	    regs_p->l14_5_obj_subtype[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(regs_p->l14_5_obj_subtype);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
		"%s %s %s %s ('%s', '%s', '%s', '%s', '%s', %d, ~V );",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		regs_p->l14_1_obj_name,
		regs_p->l14_2_obj_owner,
		regs_p->l14_3_dml,
		regs_p->l14_4_obj_type,
		regs_p->l14_5_obj_subtype,
		regs_p->l14_6_sequence);

	    /* 2.  construct data value */

	    ins_p->qeq_c8_dv_cnt = 1;

	    dv_p->db_data = (PTR) regs_p->l14_7_pkt_p->dd_p2_pkt_p;
	    dv_p->db_length = regs_p->l14_7_pkt_p->dd_p1_len;;
	    dv_p->db_datatype = DB_VCH_TYPE;
	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;
	}
	break;				

    case INS_633_DD_PROCEDURES:
	{

	    /* note: this case is closely tied to INS_629_DD_REGISTRATIONS */

	    QEC_L18_PROCEDURES	*proc_p = 
		ins_p->qeq_c3_ptr_u.l18_procedures_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_28_PROCEDURES];

	    /*
	    **	insert into iidd_procedurs values
	    **	(c32, c32, c25, c8, i4, ~V );
	    */	

	    proc_p->l18_1_proc_name[DB_DBP_MAXNAME] = EOS;
	    STtrmwhite(proc_p->l18_1_proc_name);

	    proc_p->l18_2_proc_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(proc_p->l18_2_proc_owner);

	    proc_p->l18_4_proc_subtype[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(proc_p->l18_4_proc_subtype);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
		"%s %s %s %s ('%s', '%s', '%s', '%s', %d, ~V );",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		proc_p->l18_1_proc_name,
		proc_p->l18_2_proc_owner,
		proc_p->l18_3_cre_date,
		proc_p->l18_4_proc_subtype,
		proc_p->l18_5_sequence);

	    /* 2.  construct data value */

	    ins_p->qeq_c8_dv_cnt = 1;

	    dv_p->db_data = (PTR) proc_p->l18_6_pkt_p->dd_p2_pkt_p;
	    dv_p->db_length = proc_p->l18_6_pkt_p->dd_p1_len;;
	    dv_p->db_datatype = DB_VCH_TYPE;
	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;
	}
	break;				

    case INS_630_DD_STATS:
	{
	    QEC_L15_STATS	*stats_p = 
		ins_p->qeq_c3_ptr_u.l15_stats_p;
	    i4			opensql_level =
		v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c9_opensql_level;

	    /* 1.  construct query text */

	    cat_name_p = IIQE_c8_ddb_cat[TDD_30_STATS];

	    /*
	    **	insert into iidd_stats values 
	    **	(c32, c32, c32, c25, ~V , ~V , c8, ~V , i4, i4, c8, c8, i4);
	    */	

	    stats_p->l15_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(stats_p->l15_1_tab_name);

	    stats_p->l15_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(stats_p->l15_2_tab_owner);

	    stats_p->l15_3_col_name[DB_ATT_MAXNAME] = EOS;
	    STtrmwhite(stats_p->l15_3_col_name);

	    stats_p->l15_7_has_uniq[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(stats_p->l15_7_has_uniq);

	    stats_p->l15_11_is_complete[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(stats_p->l15_11_is_complete);

	    stats_p->l15_12_stat_vers[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(stats_p->l15_12_stat_vers);

	    if (opensql_level < QEC_NEW_STATS_LEVEL)
	    {
		/* LDB does not contain new iistats columns - initialize to
		** defaults */
		stats_p->l15_10_col_domain = QEC_DEFAULT_DOMAIN;
		STcopy(QEC_DEFAULT_COMPLETE, stats_p->l15_11_is_complete);
		STcopy(DB_NO_STAT_VERSION, stats_p->l15_12_stat_vers);
		stats_p->l15_13_hist_len = QEC_DEFAULT_HIST_LEN;
	    }
		
	    STprintf(
		ins_p->qeq_c7_qry_p, 
"%s %s %s %s ('%s', '%s', '%s', '%s', ~V , ~V , '%s', ~V , %d, %d, '%s', '%s', %d);",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		stats_p->l15_1_tab_name,
		stats_p->l15_2_tab_owner,
		stats_p->l15_3_col_name,
		stats_p->l15_4_cre_date,
	    	stats_p->l15_7_has_uniq,
		stats_p->l15_9_num_cells,
		stats_p->l15_10_col_domain,
		stats_p->l15_11_is_complete,
		stats_p->l15_12_stat_vers,
		stats_p->l15_13_hist_len);

	    /* 2.  construct data values */

	    ins_p->qeq_c8_dv_cnt = 3;

	    dv_p->db_data = (PTR) & stats_p->l15_5_num_uniq;
	    dv_p->db_length = sizeof(stats_p->l15_5_num_uniq);
	    dv_p->db_datatype = DB_FLT_TYPE;
	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;

	    dv_p++;				/* pt to next DB_DATA_VALUE */

	    dv_p->db_data = (PTR) & stats_p->l15_6_rept_factor;
	    dv_p->db_length = sizeof(stats_p->l15_6_rept_factor);
	    dv_p->db_datatype = DB_FLT_TYPE;
	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;

	    dv_p++;				/* pt to next DB_DATA_VALUE */

	    dv_p->db_data = (PTR) & stats_p->l15_8_pct_nulls;
	    dv_p->db_length = sizeof(stats_p->l15_8_pct_nulls);
	    dv_p->db_datatype = DB_FLT_TYPE;
	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;
	}
	break;				

    case INS_632_DD_VIEWS:
	{
	    QEC_L17_VIEWS	*views_p = 
		ins_p->qeq_c3_ptr_u.l17_views_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_32_VIEWS];

	    /*
	    **	insert into iidd_views values
	    **	(c32, c32, c8, c8, i4, ~V );
	    */	

	    views_p->l17_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(views_p->l17_1_tab_name);

	    views_p->l17_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(views_p->l17_2_tab_owner);

	    views_p->l17_3_dml[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(views_p->l17_3_dml);

	    views_p->l17_4_chk_option[QEK_8_CHAR_SIZE] = EOS;
	    STtrmwhite(views_p->l17_4_chk_option);

	    STprintf(
		ins_p->qeq_c7_qry_p, 
		"%s %s %s %s ('%s', '%s', '%s', '%s', %d, ~V );",
		IIQE_c7_sql_tab[SQL_04_INSERT],
		IIQE_c7_sql_tab[SQL_03_INTO],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_10_VALUES],
		views_p->l17_1_tab_name,
		views_p->l17_2_tab_owner,
		views_p->l17_3_dml,
		views_p->l17_4_chk_option,
		views_p->l17_5_sequence);

	    /* 2.  construct data values for RQF */

	    ins_p->qeq_c8_dv_cnt = 1;

	    dv_p->db_data = (PTR) & views_p->l17_6_txt_seg[0];
	    dv_p->db_length = views_p->l17_7_txt_size;
	    dv_p->db_datatype = DB_VCH_TYPE;
	    dv_p->db_prec = 0;
	    dv_p->db_collID = -1;
	}
	break;				

    default:
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
	break;
    }


    return(E_DB_OK);
}

