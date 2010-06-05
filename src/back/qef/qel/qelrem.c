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
**  Name: QELREM.C - Utility routines for deleting from catalogs.
**
**  Description:
**	    qel_r1_remove   - send a DELETE statement to the CDB
**
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog redefinition
**	04-apr-89 (carl)
**	    modified to get rid of trailing blanks from DELETE queries
**	20-sep-90 (carl)
**	    added trace point processing to qel_r1_remove
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
**      09-jan-2009 (stial01)
**          Fix buffers that are dependent on DB_MAXNAME
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/



/*{
** Name: qel_r1_remove - Send a DELETE statement to the CDB 
**
** Description:
**	This routine sends a DELETE statement on the designated catalog
**	to the CDB.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_21_delete_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c1_can_type	    TRUE if accessing DDB catalog,
**				    FALSE if LDB catalog
**		.qeq_c2_can_id	    catalog id
**		.qeq_c4_ldb_p	    ptr to CDB
** Outputs:
**	None
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
**	04-apr-89 (carl)
**	    modified to get rid of trailing blanks from DELETE queries
**	20-sep-90 (carl)
**	    added trace point processing 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_r1_remove(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;
    DD_LDB_DESC	    *ldb_p = del_p->qeq_c4_ldb_p;
    i4		    del_id = del_p->qeq_c1_can_id;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    bool            log_ddl_56 = FALSE,
		    log_err_59 = FALSE;
    i4	    i4_1,
		    i4_2;
    char	    *cat_name_p = NULL,		/* must initialize */
		    qrytxt[QEK_900_LEN + (2*DB_MAXNAME)];


    if ((ldb_p != cdb_p)
	||
	(! (DEL_501_DD_ALT_COLUMNS <= del_id && del_id <= DEL_532_DD_VIEWS)))
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

    /* 1.  set up DELETE query */

    switch (del_id)
    {
    case DEL_501_DD_ALT_COLUMNS:
	{
	    QEC_L2_ALT_COLUMNS *altcols_p = 
		del_p->qeq_c3_ptr_u.l2_alt_columns_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_01_ALT_COLUMNS];

	    /*	delete from IIDD_ALT_COLUMNS where table_name = c32 and
	    **  table_owner = c32; */

	    altcols_p->l2_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(altcols_p->l2_1_tab_name);

	    altcols_p->l2_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(altcols_p->l2_2_tab_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		altcols_p->l2_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		altcols_p->l2_2_tab_owner);
	}
	break;

    case DEL_502_DD_COLUMNS:
	{
	    QEC_L3_COLUMNS  *cols_p = 
		del_p->qeq_c3_ptr_u.l3_columns_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_02_COLUMNS];

	    /*	delete from IIDD_COLUMNS where table_name = c32 and
	    **  table_owner = c32; */

	    cols_p->l3_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(cols_p->l3_1_tab_name);

	    cols_p->l3_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(cols_p->l3_2_tab_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		cols_p->l3_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		cols_p->l3_2_tab_owner);
	}
	break;

    case DEL_505_DD_DDB_DBDEPENDS:
	{
	    QEC_D1_DBDEPENDS	*dep_p = 
		del_p->qeq_c3_ptr_u.d1_dbdepends_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_05_DDB_DBDEPENDS];

	    /*	delete from IIDD_DDB_DBDEPENDS where deid1 = i4 and
	    **  deid2 = i4; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = %d %s %s = %d;",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_44_DEID1],
		dep_p->d1_4_deid1,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_45_DEID2],
		dep_p->d1_5_deid2);
	}
	break;

    case DEL_506_DD_DDB_LDBIDS:
	{
	    QEC_D2_LDBIDS   *ldbids_p = del_p->qeq_c3_ptr_u.d2_ldbids_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_06_DDB_LDBIDS];

	    /*	delete from IIDD_DDB_LDBIDS where ldb_id = i4; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = %d;",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_20_LDB_ID],
		ldbids_p->d2_5_ldb_id);
	}
	break;

    case DEL_507_DD_DDB_LDB_COLUMNS:
	{
	    QEC_D3_LDB_COLUMNS 	*ldbcols_p = 
		del_p->qeq_c3_ptr_u.d3_ldb_columns_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_07_DDB_LDB_COLUMNS];

	    /*	delete from IIDD_DDB_LDB_COLUMNS where object_base = i4
	    **	and object_index = i4; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = %d %s %s = %d;",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_38_OBJECT_BASE],
		ldbcols_p->d3_1_obj_id.db_tab_base,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_39_OBJECT_INDEX],
		ldbcols_p->d3_1_obj_id.db_tab_index);
	}
	break;

    case DEL_508_DD_DDB_LDB_DBCAPS:
	{
	    QEC_D4_LDB_DBCAPS	*dbcaps_p = 
		del_p->qeq_c3_ptr_u.d4_ldb_dbcaps_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_08_DDB_LDB_DBCAPS];

	    /*	delete from IIDD_DDB_LDB_DBCAPS where ldb_id = i4; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = %d;",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_20_LDB_ID],
		dbcaps_p->d4_1_ldb_id);
	}
	break;

    case DEL_514_DD_DDB_LONG_LDBNAMES:
	{
	    QEC_D5_LONG_LDBNAMES	*long_p =
		del_p->qeq_c3_ptr_u.d5_long_ldbnames_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_14_DDB_LONG_LDBNAMES];

	    /*	delete from IIDD_DDB_LONGNAMES where long_ldb_id = i4; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = %d;",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_29_LONG_LDB_ID],
		long_p->d5_2_ldb_id);
	}
	break;

    case DEL_515_DD_DDB_OBJECTS:
	{
	    QEC_D6_OBJECTS	*objects_p = 
		del_p->qeq_c3_ptr_u.d6_objects_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_15_DDB_OBJECTS];

	    /*	delete from IIDD_DDB_OBJECTS where 
	    **  object_name = c32 and object_owner = c32; */

	    objects_p->d6_1_obj_name[DB_OBJ_MAXNAME] = EOS;
	    STtrmwhite(objects_p->d6_1_obj_name);

	    objects_p->d6_2_obj_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(objects_p->d6_2_obj_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_36_OBJECT_NAME],
		objects_p->d6_1_obj_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_37_OBJECT_OWNER],
		objects_p->d6_2_obj_owner);
	}
	break;

    case DEL_518_DD_DDB_TABLEINFO:
	{
	    QEC_D9_TABLEINFO	*tabinfo_p = 
		del_p->qeq_c3_ptr_u.d9_tableinfo_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_18_DDB_TABLEINFO];

	    /*	delete from IIDD_DDB_TABLEINFO where object_base = i4
	    **	and object_index = i4; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = %d %s %s = %d;",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_38_OBJECT_BASE],
		tabinfo_p->d9_1_obj_id.db_tab_base,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_39_OBJECT_INDEX],
		tabinfo_p->d9_1_obj_id.db_tab_index);
	}
	break;

    case DEL_519_DD_DDB_TREE:
	{
	    QEC_D10_TREE	*tree_p = 
		del_p->qeq_c3_ptr_u.d10_tree_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_19_DDB_TREE];

	    /*	delete from IIDD_DDB_TREE where treetabbase = i4
	    **	and treetabidx = i4; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = %d %s %s = %d;",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_25_TREETABBASE],
		tree_p->d10_1_treetabbase,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_26_TREETABIDX],
		tree_p->d10_2_treetabidx);
	}
	break;

    case DEL_520_DD_HISTOGRAMS:
	{
	    QEC_L5_HISTOGRAMS	*histos_p = 
		del_p->qeq_c3_ptr_u.l5_histograms_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_20_HISTOGRAMS];

	    /*	delete from IIDD_HISTOGRAMS where 
	    **  table_name = c32 and table_owner = c32; */

	    histos_p->l5_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(histos_p->l5_1_tab_name);

	    histos_p->l5_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(histos_p->l5_2_tab_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		histos_p->l5_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		histos_p->l5_2_tab_owner);
	}
	break;

    case DEL_521_DD_INDEXES:
	{
	    QEC_L6_INDEXES	*indexes_p = 
		del_p->qeq_c3_ptr_u.l6_indexes_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_21_INDEXES];

	    /*	delete from IIDD_INDEXES where base_name = c32 and
	    **  base_owner = c32; */

	    indexes_p->l6_4_base_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(indexes_p->l6_4_base_name);

	    indexes_p->l6_5_base_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(indexes_p->l6_5_base_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_12_BASE_NAME],
		indexes_p->l6_4_base_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_13_BASE_OWNER],
		indexes_p->l6_5_base_owner);
	}
	break;

    case DEL_522_DD_INDEX_COLUMNS:
	{
	    QEC_L7_INDEX_COLUMNS	*ndxcols_p = 
		del_p->qeq_c3_ptr_u.l7_index_columns_p;

	    if (cat_name_p == NULL)
		cat_name_p = IIQE_c8_ddb_cat[TDD_22_INDEX_COLUMNS];

	    /*	delete from IIDD_INDEX_COLUMNS where index_name = c32 and
	    **  index_owner = c32; */

	    ndxcols_p->l7_1_ind_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(ndxcols_p->l7_1_ind_name);

	    ndxcols_p->l7_2_ind_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(ndxcols_p->l7_2_ind_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_40_INDEX_NAME],
		ndxcols_p->l7_1_ind_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_41_INDEX_OWNER],
		ndxcols_p->l7_2_ind_owner);
	}
	break;
    case DEL_528_DD_PROCEDURES:
	{
	    QEC_L18_PROCEDURES	*regproc_p = 	    
		del_p->qeq_c3_ptr_u.l18_procedures_p;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_28_PROCEDURES];

	    /*	delete from IIDD_PROCEDURES where 
	    **  procedure_name = c32 and procedure_owner = c32; */

	    regproc_p->l18_1_proc_name[DB_DBP_MAXNAME] = EOS;
	    STtrmwhite(regproc_p->l18_1_proc_name);

	    regproc_p->l18_2_proc_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(regproc_p->l18_2_proc_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_46_PROCEDURE_NAME],
		regproc_p->l18_1_proc_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_47_PROCEDURE_OWNER],
		regproc_p->l18_2_proc_owner);
	}
	break;

    case DEL_529_DD_REGISTRATIONS:
	{
	    QEC_L14_REGISTRATIONS	*regs_p = 
		del_p->qeq_c3_ptr_u.l14_registrations_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_29_REGISTRATIONS];

	    /*	delete from IIDD_REGISTRATIONS where 
	    **  object_name = c32 and object_owner = c32; */

	    regs_p->l14_1_obj_name[DB_OBJ_MAXNAME] = EOS;
	    STtrmwhite(regs_p->l14_1_obj_name);

	    regs_p->l14_2_obj_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(regs_p->l14_2_obj_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_36_OBJECT_NAME],
		regs_p->l14_1_obj_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_37_OBJECT_OWNER],
		regs_p->l14_2_obj_owner);
	}
	break;

    case DEL_530_DD_STATS:
	{
	    QEC_L15_STATS	*stats_p = 
		del_p->qeq_c3_ptr_u.l15_stats_p;


	    cat_name_p = IIQE_c8_ddb_cat[TDD_30_STATS];

	    /*	delete from IIDD_STATS where table_name = c32 and
	    **  table_owner = c32; */

	    stats_p->l15_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(stats_p->l15_1_tab_name);

	    stats_p->l15_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(stats_p->l15_2_tab_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		stats_p->l15_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		stats_p->l15_2_tab_owner);
	}
	break;

    case DEL_531_DD_TABLES:
	{
	    QEC_L16_TABLES	*tables_p = 
		del_p->qeq_c3_ptr_u.l16_tables_p;

	
	    cat_name_p = IIQE_c8_ddb_cat[TDD_31_TABLES];

	    /*	delete from IIDD_TABLES where table_name = c32 and
	    **  table_owner = c32; */

	    tables_p->l16_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(tables_p->l16_1_tab_name);

	    tables_p->l16_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(tables_p->l16_2_tab_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		tables_p->l16_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		tables_p->l16_2_tab_owner);
	}
	break;

    case DEL_532_DD_VIEWS:
	{
	    QEC_L17_VIEWS	*views_p = 
		del_p->qeq_c3_ptr_u.l17_views_p;

	
	    cat_name_p = IIQE_c8_ddb_cat[TDD_32_VIEWS];

	    /*	delete from IIDD_VIEWS where table_name = c32 and
	    **  table_owner = c32; */

	    views_p->l17_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(views_p->l17_1_tab_name);

	    views_p->l17_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(views_p->l17_2_tab_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_01_DELETE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		views_p->l17_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		views_p->l17_2_tab_owner);
	}
	break;

    default:
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;	/* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = del_p->qeq_c4_ldb_p;	/* LDB */
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_ddl_56)
	qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying CDB info */
    
    /* 3.  execute DELETE query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (! log_ddl_56))
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying CDB info */
    }

    return(status);
}

