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
**  Name: QELSEL.C - Utility routines for retrieving from catalogs.
**
**  Description:
**	    qel_s0_setup    - setup link CB for catalog retrieval.
**	    qel_s1_select   - send a SELECT statement to an LDB
**	    qel_s2_fetch    - read a tuple from data stream
**	    qel_s3_flush    - flush the data stream
**	    qel_s4_prepare  - qel_s1_select + qel_s2_fetch
**
**
**  History:
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog definition changes
**	15-jan-89 (carl)
**	    modified QEL_S1_SELECT to add case SEL_105_II_DBCONSTANTS
**	06-apr-89 (carl)
**	    modified qel_s1_select to remove trailing blanks from query data
**	28-sep-89 (carl)
**	    rollback 7.0 iicolumns changes from qel_s1_select
**	28-apr-90 (carl)
**	    added qel_s0_setup 
**	20-sep-90 (carl)
**	    added trace point processing to qel_s1_select
**	01-may-91 (fpang)
**	    In qel_s1_select(), when selecting from iicolumns or iidd_columns,
**	    (SEL_002_DD_COLUMNS and SEL_102 cases)
**	    added 'order by column_sequence' to ensure that
**	    column info comes back in ascending order.  
**	    Fixes B37132.
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	08-dec-93 (peters)
**	    Fix for b57555:  In qel_s1_select(), remove extra ',' at end of
**	    string from query fragment in SEL_002_II_COLUMNS/gateway case.
**      27-jul-96 (ramra01)
**          Standard catalogue changes for Alter table project.
**          Added relversion and reltotwid to the iitables.
**	26-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri (table cache priority) to iitables.
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

GLOBALREF   char        *IIQE_27_cap_names[];


/*{
** Name: qel_s0_setup - Setup link CB for catalog retrieval.
**
** Description:
**	This function initializes a link CB for doing catalog retrieval.
**
** Inputs:
**	i1_canned_id		    the canned query id
**	i2_rq_bind_p		    pointer to the RQB_BIND array space
**	i3_ldb_p		    pointer to the LDB for retrieval
**      v_qer_p			    pointer to QEF_RCB
**	    .qef_cb
**		.qef_c2_ddb_ses
**	    .qef_r3_ddb_req	    QEF_DDB_REQ
**		.qer_d7_ddl_info    QED_DDL_INFO
**		    .qed_d1_obj_name	    object's name at DDB level or 
**					    link name
**		    .qed_d2_obj_owner	    object owner name
**		    .qed_d3_col_count	    number of columns for object
**		    .qed_d4_ddb_cols_pp	    ptr to array of ptrs to DDB columns
**		    .qed_d5_qry_info_p	    NULL
**		    .qed_d6_tab_info_p	    ptr to LDB table information
**			.dd_t1_tab_name	    same as qed_d3_obj_ldbname above
**			.dd_t2_tab_owner    (to be retrieved)
**			.dd_t3_tab_type	    type of LDB table:
**					    DD_4OBJ_INDEX for index
**			.dd_t6_mapped_b	    TRUE if column mapping is specified
**			.dd_t7_col_cnt	    same as qed_d5_col_count above
**			.dd_t8_cols_pp	    ptr to array of ptrs to columns
**			.dd_t9_ldb_p	    ptr to LDB information
**			    .dd_i1_ldb_desc	    LDB descriptor
**				.dd_l1_ingres_b	    FALSE for non-$ingres
**				.dd_l2_node_name    node name
**				.dd_l3_ldb_name	    LDB name
**				.dd_l4_dbms_name    INGRES if default
**				.dd_l5_ldb_id	    0 for unknown
**		    .qed_d7_obj_id	    for passing object id, e.g., to 
**					    identify a link 
**		    .qed_d8_obj_type	    DD_1OBJ_LINK for REGISTER/CREATE,
**		    .qed_d9_reg_info_p	    ptr to iiregistrations query text 
**					    NULL if none
**			.qed_i1_qry_info
** Outputs:
**	o1_lnk_p->			    initialized link CB
**	o2_sel_p->			    initialized canned query CB
**	v_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR	
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	28-apr-90 (carl)
**          written
*/


DB_STATUS
qel_s0_setup(
i4		i1_canned_id,
RQB_BIND	*i2_rq_bind_p,
DD_LDB_DESC	*i3_ldb_p,
QEF_RCB		*v_qer_p,
QEC_LINK	*o1_lnk_p,
QEQ_1CAN_QRY	*o2_sel_p )
{
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO	*ddl_p = & v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
/*
    DD_LDB_DESC		*ldb_p = & ddl_p->qed_d6_tab_info_p->
			    dd_t9_ldb_p->dd_i1_ldb_desc,
			*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
*/
    
    MEfill( sizeof(QEC_LINK), '\0', (PTR) o1_lnk_p);
    MEfill( sizeof(QEQ_1CAN_QRY), '\0', (PTR) o2_sel_p);

    o2_sel_p->qeq_c1_can_id = i1_canned_id;
    o2_sel_p->qeq_c2_rqf_bind_p = i2_rq_bind_p;
    o2_sel_p->qeq_c4_ldb_p = i3_ldb_p;

    o1_lnk_p->qec_6_select_p = o2_sel_p;
    o1_lnk_p->qec_19_ldb_p = i3_ldb_p;
    o1_lnk_p->qec_20_rqf_bind_p = i2_rq_bind_p;
    o1_lnk_p->qec_27_select_cnt = 0;
    return(E_DB_OK);
}    


/*{
** Name: QEL_S1_SELECT - send a SELECT statement to an LDB
**
** Description:
**	This routine sends a SELECT statement on the designated catalog
**  to an LDB, holds the data stream for repeated tuple fetch.  
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c1_can_id	    catalog id
**		.qeq_c4_ldb_p	    ptr to CDB/LDB
** Outputs:
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c5_eod_b	    TRUE if no data, FALSE otherwise
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
**	15-jan-89 (carl)
**	    added case SEL_105_II_DBCONSTANTS
**	06-apr-89 (carl)
**	    modified to remove trailing blanks from query data
**	28-sep-89 (carl)
**	    rollback 7.0 iicolumns changes
**	20-sep-90 (carl)
**	    added trace point processing
**	01-may-91 (fpang)
**	    When selecting from iicolumns or iidd_columns,
**	    (SEL_002_DD_COLUMNS and SEL_102_II_COLUMNS cases)
**	    added 'order by column_sequence' to ensure that
**	    column info comes back in ascending order.  
**	19-nov-92 (teresa)
**	    Add support to query iiprocedures.
**	23-jul-93 (rganski)
**	    Added new iistats columns to case SEL_116_II_STATS.
**	02-sep-93 (barbara)
**	    Added case SEL_210_MIN_OPN_SQL_LVL and SEL_211_OPN_SQL_LVL.
**	    For SEL_210_MIN_OPN_SQL_LVL, set up for result to go in
**	    qec_m6_opn_sql field in the QEC_MIN_CAP_LVLS structure.
**	22-nov-93 (rganski)
**	    Fix for b56798: don't select new iistats columns when LDB is
**	    pre-6.5.
**	08-dec-93 (peters)
**	    Fix for b57555:  remove extra ',' at end of string from 
**	    query fragment in SEL_002_II_COLUMNS/gateway case.
**	06-mar-96 (nanpr01)
**	    Standard catalogue changes for variable page size project.
**      27-jul-96 (ramra01)
**          Standard catalogue changes for Alter table project.
**	    Added relversion and reltotwid to the iitables.
**	26-Mar-1997 (jenjo02)
**	    Table priority project:
**	    Added reltcpri (table cache priority) to iitables.
**	5-May-2002 (bonro01)
**	    Fix overlay caused by using wrong stucture fields.
*/


DB_STATUS
qel_s1_select(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *l_tabinfo_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    DD_LDB_DESC	    *ldb_p = sel_p->qeq_c4_ldb_p;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    RQB_BIND	    *bind_p = sel_p->qeq_c2_rqf_bind_p;
    i4		    can_id = sel_p->qeq_c1_can_id;
    bool	    where_b,
		    log_ddl_56 = FALSE,
		    log_err_59 = FALSE;
    i4	    i4_1,
		    i4_2;
    char	    *cat_name_p = NULL,		/* must initialize to NULL */
		    *cap_cap_p = NULL,		/* must initialize to NULL */
		    *where_p = NULL,		/* force case setting */
		    obj_name[DB_OBJ_MAXNAME + 1],
		    obj_owner[DB_OWN_MAXNAME + 1],
		    tab_name[DB_TAB_MAXNAME + 1],
		    tab_owner[DB_OWN_MAXNAME + 1],
		    proc_name[DB_DBP_MAXNAME + 1],
		    proc_owner[DB_OWN_MAXNAME + 1],

		    /* BASE_NAME and BASE_OWNER WHERE clause */
		    base_where[QEK_200_LEN + DB_TAB_MAXNAME + DB_OWN_MAXNAME],

		    /* OBJECT_NAME and OBJECT_OWNER WHERE clause */
		    obj_where[QEK_200_LEN + DB_OBJ_MAXNAME + DB_OWN_MAXNAME],

		    /* TABLE_NAME and TABLE_OWNER WHERE clause */
		    tab_where[QEK_200_LEN + DB_TAB_MAXNAME + DB_OWN_MAXNAME],

		    /* PROCEDURE_NAME and PROCEDURE_OWNER WHERE clause */
		    proc_where[QEK_200_LEN + DB_DBP_MAXNAME + DB_OWN_MAXNAME],

		    qrytxt[QEK_900_LEN + (2*DB_MAXNAME)];


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

    switch (can_id)
    {
    case SEL_035_DD_DDB_OBJECTS_BY_ID:
    case SEL_105_II_DBCONSTANTS:
	
	/* skip set up */

	break;
    
    default:

	l_tabinfo_p = ddl_p->qed_d6_tab_info_p;

	/* assume object name and owner available */

	qed_u0_trimtail(ddl_p->qed_d1_obj_name, (u_i4)DB_OBJ_MAXNAME, obj_name);
	qed_u0_trimtail(ddl_p->qed_d2_obj_owner, (u_i4)DB_OWN_MAXNAME, obj_owner);

	STprintf(
	    obj_where,
	    "%s %s = '%s' %s %s = '%s'",
	    IIQE_c7_sql_tab[SQL_11_WHERE],
	    IIQE_10_col_tab[COL_36_OBJECT_NAME],
	    obj_name,
	    IIQE_c7_sql_tab[SQL_00_AND],
	    IIQE_10_col_tab[COL_37_OBJECT_OWNER],
	    obj_owner);

	STprintf(
	    base_where,
	    "%s %s = '%s' %s %s = '%s'",
	    IIQE_c7_sql_tab[SQL_11_WHERE],
	    IIQE_10_col_tab[COL_12_BASE_NAME],
	    obj_name,
	    IIQE_c7_sql_tab[SQL_00_AND],
	    IIQE_10_col_tab[COL_13_BASE_OWNER],
	    obj_owner);

	if (can_id == SEL_019_DD_PROCEDURES)
	{
	    qed_u0_trimtail(v_lnk_p->qec_13_objects_p->d6_1_obj_name, 
			    (u_i4) DB_OBJ_MAXNAME, proc_name);
	    qed_u0_trimtail(v_lnk_p->qec_13_objects_p->d6_2_obj_owner,
			    (u_i4) DB_OWN_MAXNAME, proc_owner);
	    STprintf(
		proc_where,
		"%s %s = '%s' %s %s = '%s'",
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_46_PROCEDURE_NAME],    /* procedure_name */
		proc_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_47_PROCEDURE_OWNER],   /* procedure_owner */
		proc_owner);
	}

	if ((v_lnk_p->qec_10_haves & QEC_07_CREATE && l_tabinfo_p != NULL)
		|| 
		dds_p->qes_d7_ses_state == QES_4ST_REFRESH)
	{	    
	    /*
	    ** only CREATE LINK needs to access the LDB, hence create general 
	    ** WHERE clause for TABLE_NAME and TABLE_OWNER 
	    */

	    qed_u0_trimtail( l_tabinfo_p->dd_t1_tab_name, (u_i4) DB_TAB_MAXNAME,
		tab_name);

	    qed_u0_trimtail( l_tabinfo_p->dd_t2_tab_owner, (u_i4)DB_OWN_MAXNAME,
		tab_owner);

	    STprintf(
		tab_where,
		"%s %s = '%s' %s %s = '%s'",
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		tab_owner);

	    if (can_id == SEL_118_II_PROCEDURES)
	    {
		STprintf(
		    proc_where,
		    "%s %s = '%s' %s %s = '%s' and text_sequence = 1",
		    IIQE_c7_sql_tab[SQL_11_WHERE],
		    IIQE_10_col_tab[COL_46_PROCEDURE_NAME],    /* procedure_name */
		    tab_name,
		    IIQE_c7_sql_tab[SQL_00_AND],
		    IIQE_10_col_tab[COL_47_PROCEDURE_OWNER],   /* procedure_owner */
		    tab_owner);
	    }
	}
	break;
    }

    /* 1.  set up SELECT query */

    switch (can_id)
    {
    case SEL_001_DD_ALT_COLUMNS:

	    cat_name_p = IIQE_c8_ddb_cat[TDD_01_ALT_COLUMNS];

	    /* fall thru */

    case SEL_101_II_ALT_COLUMNS:
	{
	    QEC_L2_ALT_COLUMNS *altcols_p = 
		sel_p->qeq_c3_ptr_u.l2_alt_columns_p;

	    if (cat_name_p == NULL)
		cat_name_p = IIQE_c9_ldb_cat[TII_01_ALT_COLUMNS];

	    sel_p->qeq_c6_col_cnt = QEC_02I_CC_ALT_COLUMNS_5;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) altcols_p->l2_1_tab_name;
	    altcols_p->l2_1_tab_name[DB_TAB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) altcols_p->l2_2_tab_owner;
	    altcols_p->l2_2_tab_owner[DB_OWN_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & altcols_p->l2_3_key_id;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_ATT_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) altcols_p->l2_4_col_name;
	    altcols_p->l2_4_col_name[DB_ATT_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i2);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & altcols_p->l2_5_seq_in_key;

	    /*	select table_name, table_owner, key_id, column_name, 
	    **  key_sequence from iialt_columns | iidd_alt_columns
	    **	where table_name | object_name = 'c32' and table_owner |
	    **	object_owner = 'c32'; */

	    altcols_p->l2_1_tab_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(altcols_p->l2_1_tab_name);

	    altcols_p->l2_2_tab_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(altcols_p->l2_2_tab_owner);

	    STprintf(
		tab_where,
		"%s %s = '%s' %s %s = '%s'",
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		altcols_p->l2_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		altcols_p->l2_2_tab_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		"table_name, table_owner, key_id, column_name, key_sequence",
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		tab_where);
	}
	break;


    case SEL_002_DD_COLUMNS:

	    cat_name_p = IIQE_c8_ddb_cat[TDD_02_COLUMNS];
	    where_p = obj_where;

	    /* fall thru */
	
    case SEL_102_II_COLUMNS:
	{
	    QEC_L3_COLUMNS  *cols_p = 
		sel_p->qeq_c3_ptr_u.l3_columns_p;

	    if (cat_name_p == NULL)
	    {
		cat_name_p = IIQE_c9_ldb_cat[TII_04_COLUMNS];
		where_p = tab_where;
	    }

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_1_tab_name;
	    cols_p->l3_1_tab_name[DB_TAB_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_2_tab_owner;
	    cols_p->l3_2_tab_owner[DB_OWN_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */

	    bind_p->rqb_length = DB_ATT_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_3_col_name;
	    cols_p->l3_3_col_name[DB_ATT_MAXNAME] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = DB_TYPE_MAXLEN;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_4_data_type;
	    cols_p->l3_4_data_type[DB_TYPE_MAXLEN] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & cols_p->l3_5_length;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & cols_p->l3_6_scale;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_7_nulls;
	    cols_p->l3_7_nulls[QEK_8_CHAR_SIZE] = EOS;

	    bind_p++;				/* point to next element */
    
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_8_defaults;
	    cols_p->l3_8_defaults[QEK_8_CHAR_SIZE] = EOS;

	    bind_p++;				/* point to next element */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & cols_p->l3_9_seq_in_row;

	    bind_p++;				/* point to next element */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & cols_p->l3_10_seq_in_key;

	    bind_p++;	

	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_11_sort_dir;
	    cols_p->l3_11_sort_dir[QEK_8_CHAR_SIZE] = EOS;
						/* must null terminate */
	    bind_p++;				/* point to next element */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & cols_p->l3_12_ing_datatype;

	    if(v_lnk_p->qec_10_haves & QEC_03_INGRES)
	    {

		bind_p++;			    /* point to next element */

		bind_p->rqb_length = DB_TYPE_MAXLEN;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) cols_p->l3_13_internal_datatype;
		cols_p->l3_13_internal_datatype[DB_TYPE_MAXLEN] = EOS;
						/* must null terminate */
		bind_p++;	    		/* point to next element */

		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & cols_p->l3_14_internal_length;

		bind_p++;   			/* point to next element */

		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & cols_p->l3_15_internal_ingtype;

		bind_p++;	

		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) cols_p->l3_16_system_maintained;
		cols_p->l3_16_system_maintained[QEK_8_CHAR_SIZE] = EOS;
						/* must null terminate */

		sel_p->qeq_c6_col_cnt = 16;

		STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s %s %s %s %s %s;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner, column_name,",
		    "column_datatype, column_length, column_scale,",
		    "column_nulls, column_defaults, column_sequence,",
		    "key_sequence, sort_direction, column_ingdatatype,",
		    "column_internal_datatype, column_internal_length,",
		    "column_internal_ingtype, column_system_maintained",
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    cat_name_p,
		    where_p,
		    "order by column_sequence");
	    }
	    else 
	    {
		/* gateway */
		sel_p->qeq_c6_col_cnt = 12;

		STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s %s %s %s;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner, column_name,",
		    "column_datatype, column_length, column_scale,",
		    "column_nulls, column_defaults, column_sequence,",
		    "key_sequence, sort_direction, column_ingdatatype",
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    cat_name_p,
		    where_p,
		    "order by column_sequence");
	    }
	}
	break;

    case SEL_005_DD_DDB_LDBIDS_BY_NAME:
	{
	    QEC_D2_LDBIDS   *ldbids_p = sel_p->qeq_c3_ptr_u.d2_ldbids_p;

	    sel_p->qeq_c6_col_cnt = QEK_2_COL_COUNT;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_4_ldb_longname;
	    ldbids_p->d2_4_ldb_longname[QEK_8_CHAR_SIZE] = EOS;
    
	    bind_p++; 

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ldbids_p->d2_5_ldb_id;
    
	    /* select ldb_longname, ldb_id from iidd_ddb_ldb_ids where 
	    ** ldb_node = 'c32' and ldb_dbms = 'c32' and ldb_database 
	    ** = 'c32'; */

		ldbids_p->d2_1_ldb_node[DB_NODE_MAXNAME] = EOS;
		STtrmwhite(ldbids_p->d2_1_ldb_node);

		ldbids_p->d2_2_ldb_dbms[DB_TYPE_MAXLEN] = EOS;
		STtrmwhite(ldbids_p->d2_2_ldb_dbms);

		ldbids_p->d2_3_ldb_database[DD_256_MAXDBNAME] = EOS;
		STtrmwhite(ldbids_p->d2_3_ldb_database);

	    STprintf(
		qrytxt,
		"%s %s, %s %s %s %s %s = '%s' %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_10_col_tab[COL_23_LDB_LONGNAME],
		IIQE_10_col_tab[COL_20_LDB_ID],
		IIQE_c7_sql_tab[SQL_02_FROM],
	    	IIQE_c8_ddb_cat[TDD_06_DDB_LDBIDS],
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_21_LDB_NODE],
		ldbids_p->d2_1_ldb_node,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_22_LDB_DBMS],
		ldbids_p->d2_2_ldb_dbms,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_24_LDB_DATABASE],
		ldbids_p->d2_3_ldb_database);
	}
	break;

    case SEL_006_DD_DDB_LDBIDS_BY_ID:
	{
	    QEC_D2_LDBIDS   *ldbids_p = sel_p->qeq_c3_ptr_u.d2_ldbids_p;

	    sel_p->qeq_c6_col_cnt = QEC_02D_CC_LDBIDS_8;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_NODE_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_1_ldb_node;
	    ldbids_p->d2_1_ldb_node[DB_NODE_MAXNAME] = EOS;

	    bind_p++; 

	    bind_p->rqb_length = DB_TYPE_MAXLEN;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_2_ldb_dbms;
	    ldbids_p->d2_2_ldb_dbms[DB_TYPE_MAXLEN] = EOS;

	    bind_p++; 

	    /* should this have been DD_256_MAXDBNAME ?? */
	    bind_p->rqb_length = DB_DB_MAXNAME;	/* include null space */
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_3_ldb_database;
	    ldbids_p->d2_3_ldb_database[DB_DB_MAXNAME] = EOS;
    
	    bind_p++; 

	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_4_ldb_longname;
	    ldbids_p->d2_4_ldb_longname[QEK_8_CHAR_SIZE] = EOS;

	    bind_p++; 

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ldbids_p->d2_5_ldb_id;

	    bind_p++; 

	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_6_ldb_user;
	    ldbids_p->d2_6_ldb_user[QEK_8_CHAR_SIZE] = EOS;

	    bind_p++; 

	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_7_ldb_dba;
	    ldbids_p->d2_7_ldb_dba[QEK_8_CHAR_SIZE] = EOS;

	    bind_p++; 

	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbids_p->d2_8_ldb_dbaname;
	    ldbids_p->d2_8_ldb_dbaname[DB_OWN_MAXNAME] = EOS;

	    /*  select * from iidd_ddb_ldb_ids where ldb_id = i4; */

	    STprintf(
		qrytxt,
		"%s * %s %s %s %s = %d;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_c7_sql_tab[SQL_02_FROM],
	    	IIQE_c8_ddb_cat[TDD_06_DDB_LDBIDS],
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_20_LDB_ID],
		ldbids_p->d2_5_ldb_id);
	}
	break;

    case SEL_007_DD_DDB_LDB_COLUMNS:
	{
	    QEC_D3_LDB_COLUMNS 	*ldbcols_p = 
		sel_p->qeq_c3_ptr_u.d3_ldb_columns_p;

	    sel_p->qeq_c6_col_cnt = QEC_03D_CC_LDB_COLUMNS_4;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ldbcols_p->d3_1_obj_id.db_tab_base;

	    bind_p++;	/* 2nd item */
	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ldbcols_p->d3_1_obj_id.db_tab_index;
    
	    bind_p++;	/* 3rd item */
	
	    bind_p->rqb_length = DB_ATT_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbcols_p->d3_2_lcl_name;
	    ldbcols_p->d3_2_lcl_name[DB_ATT_MAXNAME] = EOS;
    
	    bind_p++;	/* 4th item */
	
	    bind_p->rqb_length = sizeof(i2);
					    /* above should be i4, pending on
					    ** resolution */
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ldbcols_p->d3_3_seq_in_row;
    
	    /* select * from iidd_ddb_ldb_columns where ldb_node = <node> and
	    ** ldb_dbms = <dbms> and ldb_database = <ldb>; */

		ldb_p->dd_l2_node_name[DB_NODE_MAXNAME] = EOS;
		STtrmwhite(ldb_p->dd_l2_node_name);

		ldb_p->dd_l4_dbms_name[DB_TYPE_MAXLEN] = EOS;
		STtrmwhite(ldb_p->dd_l4_dbms_name);

		ldb_p->dd_l3_ldb_name[DD_256_MAXDBNAME] = EOS;
		STtrmwhite(ldb_p->dd_l3_ldb_name);

	    STprintf(
		qrytxt,
		"%s * %s %s %s %s = '%s' %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_c7_sql_tab[SQL_02_FROM],
	    	IIQE_c8_ddb_cat[TDD_07_DDB_LDB_COLUMNS],
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_21_LDB_NODE],
		ldb_p->dd_l2_node_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_22_LDB_DBMS],
		ldb_p->dd_l4_dbms_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_24_LDB_DATABASE],
		ldb_p->dd_l3_ldb_name);
	}
	break;

    case SEL_009_DD_DDB_DBDEPENDS_BY_IN:

    case SEL_010_DD_DDB_DBDEPENDS_BY_DE:
	{
	    QEC_D1_DBDEPENDS	*dep_p = sel_p->qeq_c3_ptr_u.d1_dbdepends_p;


	    sel_p->qeq_c6_col_cnt = QEC_01D_CC_DBDEPENDS_7;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(dep_p->d1_1_inid1);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dep_p->d1_1_inid1;
    
	    bind_p++; 

	    bind_p->rqb_length = sizeof(dep_p->d1_2_inid2);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dep_p->d1_2_inid2;
    
	    bind_p++; 

	    bind_p->rqb_length = sizeof(dep_p->d1_3_itype);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dep_p->d1_3_itype;
    
	    bind_p++; 

	    bind_p->rqb_length = sizeof(dep_p->d1_4_deid1);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dep_p->d1_4_deid1;
    
	    bind_p++; 

	    bind_p->rqb_length = sizeof(dep_p->d1_5_deid2);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dep_p->d1_5_deid2;;
    
	    bind_p++; 

	    bind_p->rqb_length = sizeof(dep_p->d1_6_dtype);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dep_p->d1_6_dtype;
    
	    bind_p++; 

	    bind_p->rqb_length = sizeof(dep_p->d1_7_qid);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & dep_p->d1_7_qid;
    
    
	    if (can_id == SEL_009_DD_DDB_DBDEPENDS_BY_IN)
	    {
		/* select * from IIDD_DDB_DBDEPENDS where inid1 = i4 and
		** inid2 = i4; */

		STprintf(
		    qrytxt,
		    "%s * %s %s %s %s = %d %s %s = %d;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    IIQE_c8_ddb_cat[TDD_05_DDB_DBDEPENDS],
		    IIQE_c7_sql_tab[SQL_11_WHERE],
		    IIQE_10_col_tab[COL_42_INID1],
		    dep_p->d1_1_inid1,
		    IIQE_c7_sql_tab[SQL_00_AND],
		    IIQE_10_col_tab[COL_43_INID2],
		    dep_p->d1_2_inid2);
	    }
	    else
	    {
		/* select * from IIDD_DDB_DBDEPENDS where deid1 = i4 and
		** deid2 = i4; */

		STprintf(
		    qrytxt,
		    "%s * %s %s %s %s = %d %s %s = %d;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    IIQE_c8_ddb_cat[TDD_05_DDB_DBDEPENDS],
		    IIQE_c7_sql_tab[SQL_11_WHERE],
		    IIQE_10_col_tab[COL_44_DEID1],
		    dep_p->d1_4_deid1,
		    IIQE_c7_sql_tab[SQL_00_AND],
		    IIQE_10_col_tab[COL_45_DEID2],
		    dep_p->d1_5_deid2);
	    }
	}
	break;

    case SEL_014_DD_DDB_LONG_LDBNAMES:
	{
	    QEC_D5_LONG_LDBNAMES	*long_p =
		sel_p->qeq_c3_ptr_u.d5_long_ldbnames_p;

	    sel_p->qeq_c6_col_cnt = QEK_1_COL_COUNT;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_DB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) long_p->d5_3_ldb_alias;
	    long_p->d5_3_ldb_alias[DB_DB_MAXNAME] = EOS;

	    /* select long_ldbalias from iidd_ddb_long_ldbnames where 
	    ** long_ldbname = <long_ldbname>; */

	    long_p->d5_1_ldb_name[DD_256_MAXDBNAME] = EOS;
	    STtrmwhite(long_p->d5_1_ldb_name);

	    STprintf(
		qrytxt,
		"%s %s %s %s %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_10_col_tab[COL_30_LONG_LDB_ALIAS],
		IIQE_c7_sql_tab[SQL_02_FROM],
	    	IIQE_c8_ddb_cat[TDD_14_DDB_LONG_LDBNAMES],
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_28_LONG_LDB_NAME],
		long_p->d5_1_ldb_name);
	}
	break;

    case SEL_015_DD_DDB_OBJECTS_BY_NAME:
	{
	    QEC_D6_OBJECTS	*objects_p = 
		sel_p->qeq_c3_ptr_u.d6_objects_p;

	    objects_p->d6_1_obj_name[DB_OBJ_MAXNAME] = EOS;
	    STtrmwhite(objects_p->d6_1_obj_name);

	    objects_p->d6_2_obj_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(objects_p->d6_2_obj_owner);

	    STprintf(
		obj_where,
		"%s %s = '%s' %s %s = '%s'",
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_36_OBJECT_NAME],
		objects_p->d6_1_obj_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_37_OBJECT_OWNER],
		objects_p->d6_2_obj_owner);

	    where_p = obj_where;
	}

	/* fall thru */

    case SEL_035_DD_DDB_OBJECTS_BY_ID:
	{
	    QEC_D6_OBJECTS	*objects_p = 
		sel_p->qeq_c3_ptr_u.d6_objects_p;

	    if (where_p == NULL)
	    {
		/* construct WHERE clause using object id */

		STprintf(
		    base_where,
		    "%s %s = %d %s %s = %d",
		    IIQE_c7_sql_tab[SQL_11_WHERE],
		    IIQE_10_col_tab[COL_38_OBJECT_BASE],
		    objects_p->d6_3_obj_id.db_tab_base,
		    IIQE_c7_sql_tab[SQL_00_AND],
		    IIQE_10_col_tab[COL_39_OBJECT_INDEX],
		    objects_p->d6_3_obj_id.db_tab_index);

		where_p = base_where;
	    }
	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_OBJ_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_1_obj_name;
	    objects_p->d6_1_obj_name[DB_OBJ_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_2_obj_owner;
	    objects_p->d6_2_obj_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & objects_p->d6_3_obj_id.db_tab_base;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & objects_p->d6_3_obj_id.db_tab_index;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & objects_p->d6_4_qry_id.db_qry_high_time;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & objects_p->d6_4_qry_id.db_qry_low_time;

	    bind_p++;	
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_5_obj_cre;
	    objects_p->d6_5_obj_cre[DD_25_DATE_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_6_obj_type;
	    objects_p->d6_6_obj_type[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_7_obj_alt;
	    objects_p->d6_7_obj_alt[DD_25_DATE_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_8_sys_obj;
	    objects_p->d6_8_sys_obj[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_9_to_expire;
	    objects_p->d6_9_to_expire[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) objects_p->d6_10_exp_date;
	    objects_p->d6_10_exp_date[DD_25_DATE_SIZE] = EOS;
					    /* must null terminate */

	    sel_p->qeq_c6_col_cnt = QEC_06D_CC_OBJECTS_12;

	    /*  select * from IIDD_DDB_OBJECTS where ...  */

	    STprintf(
		qrytxt, 
		"%s * %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c8_ddb_cat[TDD_15_DDB_OBJECTS],
		where_p);
	}
	break;

    case SEL_016_DD_DDB_OBJECT_BASE:
	{
	    QEC_D7_OBJECT_BASE	*base_p = 
		sel_p->qeq_c3_ptr_u.d7_object_base_p;

	    sel_p->qeq_c6_col_cnt = QEC_07D_CC_OBJECT_BASE_1;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & base_p->d7_1_obj_base;
    
	    /*	select object_base from iidd_ddb_object_base; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_10_col_tab[COL_38_OBJECT_BASE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c8_ddb_cat[TDD_16_DDB_OBJECT_BASE]);
	}
	break;

    case SEL_018_DD_DDB_TABLEINFO:
	{
	    QEC_D9_TABLEINFO	*tableinfo_p = 
		sel_p->qeq_c3_ptr_u.d9_tableinfo_p;

	    sel_p->qeq_c6_col_cnt = QEC_09D_CC_TABLEINFO_11;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = 
		(PTR) & tableinfo_p->d9_1_obj_id.db_tab_base;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = 
		(PTR) & tableinfo_p->d9_1_obj_id.db_tab_index;

	    bind_p++;
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tableinfo_p->d9_2_lcl_type;
	    tableinfo_p->d9_2_lcl_type[QEK_8_CHAR_SIZE] = EOS;

	    bind_p++;
	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tableinfo_p->d9_3_tab_name;
	    tableinfo_p->d9_3_tab_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tableinfo_p->d9_4_tab_owner;
	    tableinfo_p->d9_4_tab_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tableinfo_p->d9_5_cre_date;
	    tableinfo_p->d9_5_cre_date[DD_25_DATE_SIZE] = EOS;

	    bind_p++;
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tableinfo_p->d9_6_alt_date;
	    tableinfo_p->d9_6_alt_date[DD_25_DATE_SIZE] = EOS;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & tableinfo_p->d9_7_rel_st1;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & tableinfo_p->d9_8_rel_st2;

	    bind_p++;
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tableinfo_p->d9_9_col_mapped;
	    tableinfo_p->d9_9_col_mapped[QEK_8_CHAR_SIZE] = EOS;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & tableinfo_p->d9_10_ldb_id;

	    /* construct WHERE clause using object id */

	    STprintf(
		    base_where,
		    "%s %s = %d %s %s = %d",
		    IIQE_c7_sql_tab[SQL_11_WHERE],
		    IIQE_10_col_tab[COL_38_OBJECT_BASE],
		    tableinfo_p->d9_1_obj_id.db_tab_base,
		    IIQE_c7_sql_tab[SQL_00_AND],
		    IIQE_10_col_tab[COL_39_OBJECT_INDEX],
		    tableinfo_p->d9_1_obj_id.db_tab_index);

	    /*  select * from iidd_ddb_tableinfo where object_base = i4
	    **	and object_index = i4; */

	    STprintf(
		qrytxt, 
		"%s * %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c8_ddb_cat[TDD_18_DDB_TABLEINFO],
		base_where);
	}
	break;

    case SEL_021_DD_INDEXES:

	cat_name_p = IIQE_c8_ddb_cat[TDD_21_INDEXES];

	/* fall thru */

    case SEL_107_II_INDEXES:
	{
	    QEC_L6_INDEXES	*indexes_p = 
		sel_p->qeq_c3_ptr_u.l6_indexes_p;
            i4                  ingsql_level;


	    /* 
	    ** for certain DDL, tab_info_p is not set. 
	    */
	    if (v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p)
	      ingsql_level = v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p->
		dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c4_ingsql_lvl;
	    else
	      ingsql_level = QEC_NEW_STDCAT_LEVEL;

	    if (cat_name_p == NULL)
		cat_name_p = IIQE_c9_ldb_cat[TII_07_INDEXES];

            if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
	      sel_p->qeq_c6_col_cnt = QEC_06I_CC_INDEXES_9;
	    else
	      sel_p->qeq_c6_col_cnt = QEC_06I_CC_INDEXES_8;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_1_ind_name;
	    indexes_p->l6_1_ind_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_2_ind_owner;
	    indexes_p->l6_2_ind_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;				/* point to next element */
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_3_cre_date;
	    indexes_p->l6_3_cre_date[DD_25_DATE_SIZE] = EOS;

	    bind_p++;				/* point to next element */
	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_4_base_name;
	    indexes_p->l6_1_ind_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_5_base_owner;
	    indexes_p->l6_2_ind_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_16_STOR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_6_storage;
	    indexes_p->l6_6_storage[QEK_16_STOR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_7_compressed;
	    indexes_p->l6_7_compressed[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) indexes_p->l6_8_uniquerule;
	    indexes_p->l6_8_uniquerule[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */

            if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
	    {
	      bind_p++;	
	      bind_p->rqb_length = sizeof(i4);
	      bind_p->rqb_dt_id = DB_INT_TYPE;
	      bind_p->rqb_addr = (PTR) & indexes_p->l6_9_pagesize;
	    }
	    else
	      indexes_p->l6_9_pagesize = DB_MIN_PAGESIZE;

	    /*  select from iiindexes | iidd_indexes
	    **	where base_name = 'c32' and base_owner = 'c32'; */


	    indexes_p->l6_4_base_name[DB_TAB_MAXNAME] = EOS;
	    STtrmwhite(indexes_p->l6_4_base_name);

	    indexes_p->l6_5_base_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(indexes_p->l6_5_base_owner);

	    STprintf(
		base_where,
		"%s %s = '%s' %s %s = '%s'",
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_12_BASE_NAME],
		indexes_p->l6_4_base_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_13_BASE_OWNER],
		indexes_p->l6_5_base_owner);

            if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
	      STprintf(
		qrytxt, 
		"%s %s %s %s %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		"index_name, index_owner, create_date, base_name,",
		"base_owner, storage_structure, is_compressed, unique_rule,",
		"index_pagesize",
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		base_where);
	    else
	      STprintf(
		qrytxt, 
		"%s %s %s %s %s %s ;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		"index_name, index_owner, create_date, base_name,",
		"base_owner, storage_structure, is_compressed, unique_rule",
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p,
		base_where);
	}
	break;

    case SEL_022_DD_INDEX_COLUMNS:

	cat_name_p = IIQE_c8_ddb_cat[TDD_22_INDEX_COLUMNS];

	/* fall thru */

    case SEL_108_II_INDEX_COLUMNS:
	{
	    QEC_L7_INDEX_COLUMNS	*ndxcols_p = 
		sel_p->qeq_c3_ptr_u.l7_index_columns_p;

	    if (cat_name_p == NULL)
		cat_name_p = IIQE_c9_ldb_cat[TII_08_INDEX_COLUMNS];

	    sel_p->qeq_c6_col_cnt = QEC_07I_CC_INDEX_COLUMNS_5;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ndxcols_p->l7_1_ind_name;
	    ndxcols_p->l7_1_ind_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ndxcols_p->l7_2_ind_owner;
	    ndxcols_p->l7_2_ind_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;
	    bind_p->rqb_length = DB_ATT_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ndxcols_p->l7_3_col_name;
	    ndxcols_p->l7_3_col_name[DB_ATT_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ndxcols_p->l7_4_key_seq;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ndxcols_p->l7_5_sort_dir;
	    ndxcols_p->l7_5_sort_dir[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */

	    /*  select from iiindex_columns | iidd_index_columns 
	    **	where index_name = 'c32' and index_owner = 'c32'; */

		ndxcols_p->l7_1_ind_name[DB_TAB_MAXNAME] = EOS;
		STtrmwhite(ndxcols_p->l7_1_ind_name);

		ndxcols_p->l7_2_ind_owner[DB_OWN_MAXNAME] = EOS;
		STtrmwhite(ndxcols_p->l7_2_ind_owner);

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		"index_name, index_owner, column_name,",
		"key_sequence, sort_direction",
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

    case SEL_104_II_DBCAPABILITIES:
	{
	    QEC_L4_DBCAPABILITIES    *caps_p = 
		sel_p->qeq_c3_ptr_u.l4_dbcapabilities_p;

	    sel_p->qeq_c6_col_cnt = QEC_04I_CC_DBCAPABILITIES_2;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_CAP_MAXLEN;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) caps_p->l4_1_cap_cap;
	    caps_p->l4_1_cap_cap[DB_CAP_MAXLEN] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_CAPVAL_MAXLEN;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) caps_p->l4_2_cap_val;
	    caps_p->l4_2_cap_val[DB_CAPVAL_MAXLEN] = EOS;
					    /* must null terminate */

	    /*  select cap_capability, cap_value from iidbcapabilities; */

	    STprintf(
		qrytxt, 
		"%s %s, %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_10_col_tab[COL_31_CAP_CAPABILITY],
		IIQE_10_col_tab[COL_34_CAP_VALUE],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c9_ldb_cat[TII_02_DBCAPABILITIES]);
	}
	break;

    case SEL_105_II_DBCONSTANTS:
	{
	    QEC_L1_DBCONSTANTS	*consts_p = 
		sel_p->qeq_c3_ptr_u.l1_dbconstants_p;

	    cat_name_p = IIQE_c9_ldb_cat[TII_03_DBCONSTANTS];

	    sel_p->qeq_c6_col_cnt = QEC_01I_CC_DBCONSTANTS_2;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) consts_p->l1_1_usr_name;
	    consts_p->l1_1_usr_name[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) consts_p->l1_2_dba_name;
	    consts_p->l1_2_dba_name[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */

	    /*  select from iidbconstants; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		"user_name, dba_name",
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p);
	}
	break;

    case SEL_106_II_HISTOGRAMS:
	{
	    QEC_L5_HISTOGRAMS	*histos_p = 
		sel_p->qeq_c3_ptr_u.l5_histograms_p;

	    sel_p->qeq_c6_col_cnt = QEC_05I_CC_HISTOGRAMS_5;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) histos_p->l5_1_tab_name;
	    histos_p->l5_1_tab_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) histos_p->l5_2_tab_owner;
	    histos_p->l5_2_tab_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_ATT_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) histos_p->l5_3_col_name;
	    histos_p->l5_3_col_name[DB_ATT_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & histos_p->l5_4_txt_seq;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_228_HIST_SIZE;	
					    /* include null space */
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) histos_p->l5_5_txt_seg;

	    /*  select from iihistograms
	    **	where table_name = 'c32' and table_owner = 'c32'; */

	    STprintf(
		qrytxt, 
		"%s %s %s %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		"table_name, table_owner, column_name,",
		"text_sequence, text_segment",
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c9_ldb_cat[TII_06_HISTOGRAMS],
		tab_where);
	}
	break;

    case SEL_116_II_STATS:
	{
	    QEC_L15_STATS	*stats_p = 
		sel_p->qeq_c3_ptr_u.l15_stats_p;
	    i4			opensql_level =
		ddl_p->qed_d6_tab_info_p->dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c9_opensql_level;

	    if (opensql_level >= QEC_NEW_STATS_LEVEL)
		/* LDB contains new iistats columns */
		sel_p->qeq_c6_col_cnt = QEC_15I_CC_STATS_13;
	    else
		/* LDB does not contain new iistats columns */
		sel_p->qeq_c6_col_cnt = QEC_15I_CC_STATS_9;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) stats_p->l15_1_tab_name;
	    stats_p->l15_1_tab_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) stats_p->l15_2_tab_owner;
	    stats_p->l15_2_tab_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DB_ATT_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) stats_p->l15_3_col_name;
	    stats_p->l15_3_col_name[DB_ATT_MAXNAME] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) stats_p->l15_4_cre_date;
	    stats_p->l15_4_cre_date[DD_25_DATE_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = sizeof(f4);
	    bind_p->rqb_dt_id = DB_FLT_TYPE;
	    bind_p->rqb_addr = (PTR) & stats_p->l15_5_num_uniq;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(f4);
	    bind_p->rqb_dt_id = DB_FLT_TYPE;
	    bind_p->rqb_addr = (PTR) & stats_p->l15_6_rept_factor;

	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) stats_p->l15_7_has_uniq;
	    stats_p->l15_7_has_uniq[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = sizeof(f4);
	    bind_p->rqb_dt_id = DB_FLT_TYPE;
	    bind_p->rqb_addr = (PTR) & stats_p->l15_8_pct_nulls;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & stats_p->l15_9_num_cells;

	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & stats_p->l15_10_col_domain;

	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) stats_p->l15_11_is_complete;
	    stats_p->l15_11_is_complete[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) stats_p->l15_12_stat_vers;
	    stats_p->l15_12_stat_vers[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
	    bind_p++;	
	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & stats_p->l15_13_hist_len;

	    /*  select from iistats
	    **	where table_name = 'c32' and table_owner = 'c32'; */

	    if (opensql_level >= QEC_NEW_STATS_LEVEL)
	    {
		/* LDB contains new iistats columns */
		STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s %s;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner, column_name, create_date,",
		    "num_unique, rept_factor, has_unique, pct_nulls, num_cells,",
		    "column_domain, is_complete, stat_version, hist_data_length",
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    IIQE_c9_ldb_cat[TII_16_STATS],
		    tab_where);
	    }
	    else
	    {
		/* LDB does not contain new iistats columns */
		STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner, column_name, create_date,",
		    "num_unique, rept_factor, has_unique, pct_nulls, num_cells",
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    IIQE_c9_ldb_cat[TII_16_STATS],
		    tab_where);
	    }
	}
	break;

    case SEL_113_II_PHYSICAL_TABLES:		/* a view of iitables */

	cat_name_p = IIQE_c9_ldb_cat[TII_13_PHYSICAL_TABLES];

    case SEL_117_II_TABLES:
	{
	    QEC_L16_TABLES  *tables_p = sel_p->qeq_c3_ptr_u.l16_tables_p;
	    i4		    colcnt = 0;
            i4              ingsql_level;

	    ingsql_level = v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p->
		dd_t9_ldb_p->dd_i2_ldb_plus.dd_p3_ldb_caps.dd_c4_ingsql_lvl;

	    if (cat_name_p == NULL)
		cat_name_p = IIQE_c9_ldb_cat[TII_17_TABLES];

	    if (can_id == SEL_117_II_TABLES)
              if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
		sel_p->qeq_c6_col_cnt = QEC_16I_CC_TABLES_42;
	      else
		sel_p->qeq_c6_col_cnt = QEC_16I_CC_TABLES_38;
	    else
              if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
		sel_p->qeq_c6_col_cnt = QEC_013I_CC_PHYSICAL_TABLES_14;
	      else
		sel_p->qeq_c6_col_cnt = QEC_013I_CC_PHYSICAL_TABLES_13;

	    if (can_id == SEL_117_II_TABLES)
	    {
		/* set up column types and sizes for fetching with RQF */

		colcnt++;
		bind_p->rqb_length = DB_TAB_MAXNAME;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_1_tab_name;
		tables_p->l16_1_tab_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = DB_OWN_MAXNAME;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_2_tab_owner;
		tables_p->l16_2_tab_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = DD_25_DATE_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_3_cre_date;
		tables_p->l16_3_cre_date[DD_25_DATE_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = DD_25_DATE_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_4_alt_date;
		tables_p->l16_4_alt_date[DD_25_DATE_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_5_tab_type;
		tables_p->l16_5_tab_type[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_6_sub_type;
		tables_p->l16_6_sub_type[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_7_version;
		tables_p->l16_7_version[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_8_sys_use;
		tables_p->l16_8_sys_use[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_9_stats;
		tables_p->l16_9_stats[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_10_indexes;
		tables_p->l16_10_indexes[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_11_readonly;
		tables_p->l16_11_readonly[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_12_num_rows;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_16_STOR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_13_storage;
		tables_p->l16_13_storage[QEK_16_STOR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_14_compressed;
		tables_p->l16_14_compressed[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_15_dup_rows;
		tables_p->l16_15_dup_rows[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_16_uniquerule;
		tables_p->l16_16_uniquerule[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_17_num_pages;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_18_overflow;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_19_row_width;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_20_tab_expire;

		colcnt++;
		bind_p++;				
		bind_p->rqb_length = DD_25_DATE_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_21_mod_date;
		tables_p->l16_21_mod_date[DD_25_DATE_SIZE] = EOS;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_24_LOCN_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_22_loc_name;
		tables_p->l16_22_loc_name[QEK_24_LOCN_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_23_integrities;
		tables_p->l16_23_integrities[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_24_permits;
		tables_p->l16_24_permits[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_25_all_to_all;
		tables_p->l16_25_all_to_all[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_26_ret_to_all;
		tables_p->l16_26_ret_to_all[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_27_journalled;
		tables_p->l16_27_journalled[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_28_view_base;
		tables_p->l16_28_view_base[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_29_multi_loc;
		tables_p->l16_29_multi_loc[QEK_8_CHAR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i2);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_30_ifillpct;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i2);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_31_dfillpct;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i2);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_32_lfillpct;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_33_minpages;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_34_maxpages;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_35_rel_st1;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_36_rel_st2;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_37_reltid;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_38_reltidx;

                if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
	        {
		  colcnt++;
		  bind_p++;	
		  bind_p->rqb_length = sizeof(i4);
		  bind_p->rqb_dt_id = DB_INT_TYPE;
		  bind_p->rqb_addr = (PTR) & tables_p->l16_39_pagesize;
		}
		else
		  tables_p->l16_39_pagesize = DB_MIN_PAGESIZE;

                if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
                {
                  colcnt++;
                  bind_p++;
                  bind_p->rqb_length = sizeof(i2);
                  bind_p->rqb_dt_id = DB_INT_TYPE;
                  bind_p->rqb_addr = (PTR) & tables_p->l16_40_relversion;
		}
		else
		  tables_p->l16_40_relversion = 0;

                if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
                {
                  colcnt++;
                  bind_p++;
                  bind_p->rqb_length = sizeof(i4);
                  bind_p->rqb_dt_id = DB_INT_TYPE;
                  bind_p->rqb_addr = (PTR) & tables_p->l16_41_reltotwid;
                }
                else
                  tables_p->l16_41_reltotwid = DB_MIN_PAGESIZE;  

                if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
                {
                  colcnt++;
                  bind_p++;
                  bind_p->rqb_length = sizeof(i2);
                  bind_p->rqb_dt_id = DB_INT_TYPE;
                  bind_p->rqb_addr = (PTR) & tables_p->l16_42_reltcpri;
                }
                else
                  tables_p->l16_42_reltcpri = 0;  
 

		/*  select from iitables
		**	where table_name = 'c32' and table_owner = 'c32'; */

                if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
		  STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s %s;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner, create_date, alter_date,",
		    "table_type, table_subtype, table_version, system_use,",
		    "table_stats, table_indexes, is_readonly, num_rows,",
		    "storage_structure, is_compressed, duplicate_rows,",
		    "unique_rule, number_pages, overflow_pages, row_width,",
		    "expire_date, modify_date, location_name,",      
		    "table_integrities,table_permits, all_to_all, ret_to_all,",
		    "is_journalled,view_base, multi_locations, table_ifillpct,",
		    "table_dfillpct, table_lfillpct, table_minpages,",
		    "table_maxpages, table_relstamp1, table_relstamp2,",
		    "table_reltid, table_reltidx, table_pagesize,",
		    "table_relversion, table_reltotwid, table_reltcpri",
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    cat_name_p,
		    tab_where);
	        else
		  STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s %s %s %s %s %s %s %s %s %s;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner, create_date, alter_date,",
		    "table_type, table_subtype, table_version, system_use,",
		    "table_stats, table_indexes, is_readonly, num_rows,",
		    "storage_structure, is_compressed, duplicate_rows,",
		    "unique_rule, number_pages, overflow_pages, row_width,",
		    "expire_date, modify_date, location_name,",      
		    "table_integrities,table_permits, all_to_all, ret_to_all,",
		    "is_journalled,view_base, multi_locations, table_ifillpct,",
		    "table_dfillpct, table_lfillpct, table_minpages,",
		    "table_maxpages, table_relstamp1, table_relstamp2,",
		    "table_reltid, table_reltidx",
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    cat_name_p,
		    tab_where);
	    }
	    else
	    {
		/* set up column types and sizes for fetching from 
		** iiphysical_tables */

		colcnt++;
		bind_p->rqb_length = DB_TAB_MAXNAME;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_1_tab_name;
		tables_p->l16_1_tab_name[DB_TAB_MAXNAME] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = DB_OWN_MAXNAME;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_2_tab_owner;
		tables_p->l16_2_tab_owner[DB_OWN_MAXNAME] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_9_stats;
		tables_p->l16_9_stats[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_10_indexes;
		tables_p->l16_10_indexes[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_11_readonly;
		tables_p->l16_11_readonly[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_12_num_rows;

		colcnt++;
		bind_p++;
		bind_p->rqb_length = QEK_16_STOR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_13_storage;
		tables_p->l16_13_storage[QEK_16_STOR_SIZE] = EOS;
					    /* must null terminate */
		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_14_compressed;
		tables_p->l16_14_compressed[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_15_dup_rows;
		tables_p->l16_15_dup_rows[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = QEK_8_CHAR_SIZE;
		bind_p->rqb_dt_id = DB_CHA_TYPE;
		bind_p->rqb_addr = (PTR) tables_p->l16_16_uniquerule;
		tables_p->l16_16_uniquerule[QEK_8_CHAR_SIZE] = EOS;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_17_num_pages;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_18_overflow;

		colcnt++;
		bind_p++;	
		bind_p->rqb_length = sizeof(i4);
		bind_p->rqb_dt_id = DB_INT_TYPE;
		bind_p->rqb_addr = (PTR) & tables_p->l16_19_row_width;

                if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
		{
		  colcnt++;
		  bind_p++;	
		  bind_p->rqb_length = sizeof(i4);
		  bind_p->rqb_dt_id = DB_INT_TYPE;
		  bind_p->rqb_addr = (PTR) & tables_p->l16_39_pagesize;
		}
		else
		  tables_p->l16_39_pagesize = DB_MIN_PAGESIZE;

		/*  select from iiphysical_tables
		**	where table_name = 'c32' and table_owner = 'c32'; */

                if (ingsql_level >= QEC_NEW_STDCAT_LEVEL)
		  STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s %s %s %s;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner,",
		    "table_stats, table_indexes, is_readonly, num_rows,",
		    "storage_structure, is_compressed, duplicate_rows,",
		    "unique_rule, number_pages, overflow_pages, row_width, ",
	 	    "table_pagesize",	
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    cat_name_p,
		    tab_where);
		else
		  STprintf(
		    qrytxt, 
		    "%s %s %s %s %s %s %s %s ;",
		    IIQE_c7_sql_tab[SQL_07_SELECT],
		    "table_name, table_owner,",
		    "table_stats, table_indexes, is_readonly, num_rows,",
		    "storage_structure, is_compressed, duplicate_rows,",
		    "unique_rule, number_pages, overflow_pages, row_width ",
		    IIQE_c7_sql_tab[SQL_02_FROM],
		    cat_name_p,
		    tab_where);
	    }
	    if (colcnt != sel_p->qeq_c6_col_cnt)
	    {
		status = qed_u1_gen_interr(& v_qer_p->error);
		return(status);
	    }
	}
	break;

	case SEL_118_II_PROCEDURES:
	{
	    QEC_L16_TABLES  *tables_p = sel_p->qeq_c3_ptr_u.l16_tables_p;
	    i4		    colcnt = 0;


	    cat_name_p = IIQE_c9_ldb_cat[TII_18_PROCEDURES];
	    sel_p->qeq_c6_col_cnt = QEC_18I_CC_PROC_4;

	    /* initialize the unused columns */
	    MEfill(sizeof(*tables_p), '\0', (PTR) tables_p);

	    /* set up column types and sizes for fetching with RQF */

	    colcnt++;
	    bind_p->rqb_length = DB_TAB_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tables_p->l16_1_tab_name;
	    tables_p->l16_1_tab_name[DB_TAB_MAXNAME] = EOS;
					/* must null terminate */
	    colcnt++;
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tables_p->l16_2_tab_owner;
	    tables_p->l16_2_tab_owner[DB_OWN_MAXNAME] = EOS;
					/* must null terminate */
	    colcnt++;
	    bind_p++;	
	    bind_p->rqb_length = DD_25_DATE_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tables_p->l16_3_cre_date;
	    tables_p->l16_3_cre_date[DD_25_DATE_SIZE] = EOS;
					/* must null terminate */

	    colcnt++;
	    bind_p++;	
	    bind_p->rqb_length = QEK_8_CHAR_SIZE;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) tables_p->l16_6_sub_type;
	    tables_p->l16_6_sub_type[QEK_8_CHAR_SIZE] = EOS;
					/* must null terminate */


	    /*  select (procedure_name, procedure_owner, create_date, 
	    **		proc_subtype) from iiprocedures
	    **	where procedure_name = 'c32' and procedure_owner = 'c32'
	    **	      and text_sequence = 1; 
	    **	(NOTE: this is read into the same structure that iitables
	    **	       info is read into)
	    */

	    STprintf(
	      qrytxt, 
	      "%s %s %s %s %s;",
	      IIQE_c7_sql_tab[SQL_07_SELECT],
	      "procedure_name, procedure_owner, create_date, proc_subtype",
	      IIQE_c7_sql_tab[SQL_02_FROM],
	      cat_name_p,
	      proc_where);

	    break;
	}
	case SEL_019_DD_PROCEDURES:	/* distributed iidd_procedures */
	{
	    QEC_L18_PROCEDURES   *regproc_p =
		v_lnk_p->qec_21_delete_p->qeq_c3_ptr_u.l18_procedures_p;
	    i4		    colcnt = 0;

	    cat_name_p = IIQE_c8_ddb_cat[TDD_28_PROCEDURES];
	    sel_p->qeq_c6_col_cnt = QEC_18I_CC_PROC_4;

	    /* initialize the unused columns */
	    MEfill(sizeof(*regproc_p), '\0', (PTR) regproc_p);

	    /* set up column types and sizes for fetching with RQF */

	    colcnt++;
	    bind_p->rqb_length = DB_DBP_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) regproc_p->l18_1_proc_name,
	    regproc_p->l18_1_proc_name[DB_DBP_MAXNAME] = EOS;
					/* must null terminate */
	    colcnt++;
	    bind_p++;	
	    bind_p->rqb_length = DB_OWN_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) regproc_p->l18_2_proc_owner,
	    regproc_p->l18_2_proc_owner[DB_OWN_MAXNAME] = EOS;
					/* must null terminate */

	    /*  select (procedure_name, procedure_owner) from iiprocedures
	    **	where procedure_name = 'c32' and procedure_owner = 'c32'
	    */

	    STprintf(
	      qrytxt, 
	      "%s %s, %s %s %s %s;",
	      IIQE_c7_sql_tab[SQL_07_SELECT],
	      IIQE_10_col_tab[COL_46_PROCEDURE_NAME],    /* procedure_name */
	      IIQE_10_col_tab[COL_47_PROCEDURE_OWNER],   /* procedure_owner */
	      IIQE_c7_sql_tab[SQL_02_FROM],
	      cat_name_p,
	      proc_where);

	    break;
	}
    case SEL_120_II_STATS_COUNT:
	{
	    sel_p->qeq_c6_col_cnt = QEK_1_COL_COUNT;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & v_lnk_p->qec_28_iistats_cnt;

	    /*  select COUNT(*) from iistats where table_name = c32 and
	    **  table_owner = c32; */

	    STprintf(
		qrytxt, 
		"%s count(*) %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c9_ldb_cat[TII_16_STATS],
		tab_where);
	}
	break;

    case SEL_121_II_COLNAMES:
	{
	    QEC_L3_COLUMNS  *cols_p = 
		sel_p->qeq_c3_ptr_u.l3_columns_p;

	    cat_name_p = IIQE_c9_ldb_cat[TII_04_COLUMNS];
	    where_p = tab_where;

	    sel_p->qeq_c6_col_cnt = QEK_2_COL_COUNT;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = DB_ATT_MAXNAME;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) cols_p->l3_3_col_name;
	    cols_p->l3_3_col_name[DB_ATT_MAXNAME] = EOS;

	    bind_p++;

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & cols_p->l3_9_seq_in_row;

	    /* select column_name, column_sequence
	    ** from iicolumns where table_name = 'c32' and
	    ** table_owner = 'c32'; */

	    STprintf(
		qrytxt,
		"%s %s %s %s %s %s %s;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		"column_name,", "column_sequence",
		IIQE_c7_sql_tab[SQL_02_FROM],
		cat_name_p, where_p, "order by column_sequence");
	}
	break;

    case SEL_200_MIN_COM_SQL:

	    cap_cap_p = IIQE_27_cap_names[QEK_0NAME_COMMONSQL_LEVEL];  

	    /* fall thru */

    case SEL_201_MIN_ING_QUEL:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_5NAME_INGRESQUEL_LEVEL];  

	    /* fall thru */

    case SEL_202_MIN_ING_SQL:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_6NAME_INGRESSQL_LEVEL];  

	    /* fall thru */

    case SEL_203_MIN_SAVEPTS:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_7NAME_SAVEPOINTS];  

	    /* fall thru */

    case SEL_204_MIN_UNIQUE_KEY_REQ:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_10NAME_UNIQUE_KEY_REQ];  

	    /* fall thru */

    case SEL_210_MIN_OPN_SQL_LVL:
	{
	    QEC_MIN_CAP_LVLS	*mins_p = 
		sel_p->qeq_c3_ptr_u.min_cap_lvls_p;


	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_12NAME_OPENSQL_LEVEL];

	    sel_p->qeq_c6_col_cnt = QEK_1_COL_COUNT;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;

	    if (can_id == SEL_200_MIN_COM_SQL)
		bind_p->rqb_addr = (PTR) & mins_p->qec_m1_com_sql;
	    else if (can_id == SEL_201_MIN_ING_QUEL)
		bind_p->rqb_addr = (PTR) & mins_p->qec_m2_ing_quel;
	    else if (can_id == SEL_202_MIN_ING_SQL)
		bind_p->rqb_addr = (PTR) & mins_p->qec_m3_ing_sql;
	    else if (can_id == SEL_203_MIN_SAVEPTS)
		bind_p->rqb_addr = (PTR) & mins_p->qec_m4_savepts;
	    else if (can_id == SEL_204_MIN_UNIQUE_KEY_REQ)
		bind_p->rqb_addr = (PTR) & mins_p->qec_m5_uniqkey;
	    else if (can_id == SEL_210_MIN_OPN_SQL_LVL)
		bind_p->rqb_addr = (PTR) & mins_p->qec_m6_opn_sql;
	    else
	    {
		status = qed_u1_gen_interr(& v_qer_p->error);
		return(status);
	    }

	    /*	select MIN(cap_lvel) from iidd_ddb_ldb_dbcaps where 
	    **  cap_capability = '%s'; */

	    STprintf(
		qrytxt, 
		"%s min(%s) %s %s %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_10_col_tab[COL_33_CAP_LEVEL],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c8_ddb_cat[TDD_08_DDB_LDB_DBCAPS],
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_31_CAP_CAPABILITY],
		cap_cap_p);
	}
	break;

    case SEL_205_COM_SQL_LVL:

	    cap_cap_p = IIQE_27_cap_names[QEK_0NAME_COMMONSQL_LEVEL];  

	    /* fall thru */

    case SEL_206_ING_QUEL_LVL:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_5NAME_INGRESQUEL_LEVEL];  

	    /* fall thru */

    case SEL_207_ING_SQL_LVL:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_6NAME_INGRESSQL_LEVEL];  

	    /* fall thru */

    case SEL_208_SAVEPOINTS:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_7NAME_SAVEPOINTS];  

	    /* fall thru */

    case SEL_209_UNIQUE_KEY_REQ:

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_10NAME_UNIQUE_KEY_REQ];  

	    /* fall thru */

    case SEL_211_OPN_SQL_LVL:
	{
	    QEC_D4_LDB_DBCAPS	*ldbcaps_p = 
		sel_p->qeq_c3_ptr_u.d4_ldb_dbcaps_p;

	    if (cap_cap_p == NULL)
		cap_cap_p = IIQE_27_cap_names[QEK_12NAME_OPENSQL_LEVEL];  

	    sel_p->qeq_c6_col_cnt = QEK_4_COL_COUNT;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ldbcaps_p->d4_1_ldb_id;

	    bind_p++;	

	    bind_p->rqb_length = DB_CAP_MAXLEN;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbcaps_p->d4_2_cap_cap;
	    ldbcaps_p->d4_2_cap_cap[DB_CAP_MAXLEN] = EOS;
					    /* must null terminate */
	    bind_p++;	

	    bind_p->rqb_length = DB_CAPVAL_MAXLEN;
	    bind_p->rqb_dt_id = DB_CHA_TYPE;
	    bind_p->rqb_addr = (PTR) ldbcaps_p->d4_3_cap_val;
	    ldbcaps_p->d4_3_cap_val[DB_CAPVAL_MAXLEN] = EOS;
					    /* must null terminate */
	    bind_p++;	

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & ldbcaps_p->d4_4_cap_lvl;

	    /*	select * from iidd_ddb_ldb_dbcaps where 
	    **  cap_capability = '%s' and cap_level = i4; */

	    STprintf(
		qrytxt, 
		"%s * %s %s %s %s = '%s' %s %s = %d;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c8_ddb_cat[TDD_08_DDB_LDB_DBCAPS],
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_31_CAP_CAPABILITY],
		cap_cap_p,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_33_CAP_LEVEL],
		ldbcaps_p->d4_4_cap_lvl);
	}
	break;

    case SEL_036_DD_LDB_OBJECT_COUNT:
	{
	    QEC_D9_TABLEINFO	*tableinfo_p = 
		sel_p->qeq_c3_ptr_u.d9_tableinfo_p;

	    sel_p->qeq_c6_col_cnt = QEK_1_COL_COUNT;

	    /* set up column types and sizes for fetching with RQF */

	    bind_p->rqb_length = sizeof(i4);
	    bind_p->rqb_dt_id = DB_INT_TYPE;
	    bind_p->rqb_addr = (PTR) & v_lnk_p->qec_11_ldb_obj_cnt;

	    /*  select COUNT(*) from iidd_ddb_tableinfo where ldb_id = i4; */

	    STprintf(
		qrytxt, 
		"%s count(*) %s %s %s %s = %d;",
		IIQE_c7_sql_tab[SQL_07_SELECT],
		IIQE_c7_sql_tab[SQL_02_FROM],
		IIQE_c8_ddb_cat[TDD_18_DDB_TABLEINFO],
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_20_LDB_ID],
		tableinfo_p->d9_10_ldb_id);
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
    rqr_p->rqr_1_site = ldb_p;			/* LDB */
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_ddl_56)
	qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying the LDB info */
    /* 3.  execute query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (! log_ddl_56))
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying the LDB info */
	return(status);
    }

    sel_p->qeq_c5_eod_b = rqr_p->rqr_end_of_data;

    return(status);
}

/*{
** Name: qel_s2_fetch - Read the next tuple from a sending LDBMS
**
** Description:
**      This routine reads the next tuple from a sending LDBMS.
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c2_can_id	    DDB/LDB catalog id
**		.qeq_c4_ldb_p	    ptr to LDB
** Outputs:
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c5_eod_b	    TRUE if no data, FALSE otherwise
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
*/


DB_STATUS
qel_s2_fetch(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    /* 1. verify state */

    if (sel_p->qeq_c5_eod_b)			/* no data to read */
	return(E_DB_OK);

    /* 2.  set up to call RQF to obtain next tuple */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = sel_p->qeq_c4_ldb_p;
    rqr_p->rqr_col_count = sel_p->qeq_c6_col_cnt;
    rqr_p->rqr_bind_addrs = sel_p->qeq_c2_rqf_bind_p;
						/* must use invariant array */
    /* 3.  request 1 tuple */

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	/* must flush before returning */

	STRUCT_ASSIGN_MACRO(v_qer_p->error, sav_error);
	ignore = qel_s3_flush(v_qer_p, v_lnk_p);
	STRUCT_ASSIGN_MACRO(sav_error, v_qer_p->error);
	return(status);
    }

    sel_p->qeq_c5_eod_b = rqr_p->rqr_end_of_data;
    if (! sel_p->qeq_c5_eod_b)
	v_lnk_p->qec_27_select_cnt++;

    return(E_DB_OK);
}


/*{
** Name: qel_s3_flush - flush all unread tuples from sending LDB
**
** Description:
**      This routine flushes all unread tuples from the sending LDB.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qec_c2_ldb_p	    ptr to LDB
**		.qec_c4_eod_b	    FALSE 
** Outputs:
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qec_c4_eod_b	    FALSE 
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
*/


DB_STATUS
qel_s3_flush(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    /* 1. verify state */

    if (sel_p->qeq_c5_eod_b)			/* nothing to flush */
	return(E_DB_OK);

    sel_p->qeq_c5_eod_b = TRUE;

    /* 2.  set up to call RQF to flush */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   
    rqr_p->rqr_1_site = sel_p->qeq_c4_ldb_p;

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }
    
    return(status);
}


/*{
** Name: QEL_S4_PREPARE - Equivalent to calling qel_s1_select and then 
**			  qel_s2_fetch.
**
** Description:
**      This routine sends a SELECT query and then if successful requests a
**  first tuple.
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c1_can_id	    DDB/LDB catalog id
**		.qeq_c2_rqf_bind_p  ptr to RQF binding array
**		.qeq_c4_ldb_p	    ptr to LDB
** Outputs:
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_6_select_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c5_eod_b	    TRUE if no data, FALSE otherwise
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
*/


DB_STATUS
qel_s4_prepare(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;


    v_lnk_p->qec_27_select_cnt = 0;

    /* 1.  send SELECT query */

    status = qel_s1_select(v_qer_p, v_lnk_p);
    if (status)
	return(status);


    if (! sel_p->qeq_c5_eod_b)			/* any data to read ? */
	status = qel_s2_fetch(v_qer_p, v_lnk_p);

    return(status);
}
