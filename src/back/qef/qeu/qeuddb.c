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
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefptr.h>
#include    <tm.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEUDDB.C - Routine processing CREATE/DROP TABLE/VIEW
**
**  Description:
**      This file contains routines for processing CREATE/DROP TABLE/VIEW,
**  SET [NO]STATISTICS and BEGIN/END/ABORT COPY statements.
**
**	    qeu_d1_cre_tab  - process CREATE TABLE statement (QEUS.C)
**	    qeu_d2_des_tab  - process DESTROY/DROP TABLE statement (QEUS.C)
**	    qeu_d3_cre_view - process CREATE VIEW statement (QEFCALL.C)
**	    qeu_d4_des_view - process DESTROY/DROP VIEW statement (QEFCALL.C)
**	    qeu_d5_alt_tab  - process SET [NO]STATISTICS statement (QEUS.C)
**	    qeu_d6_cre_view - auxiliary rotuine for qeu_d3_cre_view
**          qeu_d7_b_copy   - begin copy
**          qeu_d8_e_copy   - end copy
**          qeu_d9_a_copy   - abort copy
**
**  History:    $Log-for RCS$
**	31-oct-88 (carl)
**          written
**	18-dec-88 (carl)
**	    added qeu_d5_alt_tab
**	29-jan-89 (carl)
**	    modified qeu_d3_cre_view to call qeu_d6_cre_view
**	13-mar-89 (carl)
**	    added code to commit CDB association to avoid deadlocks
**	22-mar-90 (carl)
**	    created qeu_d9_a_copy
**	28-apr-90 (carl)
**	    modified qeu_d1_cre_tab to use the logged-in user name at the
**	    LDB instead of at the DDB level for locating the local table
**	22-may-90 (carl)
**	    modified qeu_d1_cre_tab to not abort the DX if the 
**	    local CREATE TABLE query fails
**	10-aug-90 (carl)
**	    removed all references to IIQE_c21_qef_rcb and IIQE_c41_lin
**	07-aug-91 (fpang)
**	    In qeu_d4_des_view() and qeu_d6_cre_view() initialized
**	    qer_p->error.err_code to 0 and 
**	    qer_p->qef_r3_ddb_req.qer_d13_ctl_info to QEF_00DD_NIL_INFO.
**	    Fixes B38882.
**	08-dec-92 (anitap)
**	    Added #include <psfparse.h> for CREATE SCHEMA.
**	29-jan-1993 (fpang)
**	    Where appropriate, compare qef_auto to QEF_ON or QEF_OFF instead 
**	    of 0 or !0. 
**	    Also in qeu_d4_des_view(), qeu_d5_alt_tab(), and qeu_d6_cre_view() 
**	    register to TPF as an updater if ddl concurrency is off.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	07-Aug-1997 (jenjo02)
**	    Changed *dsh to **dsh in qee_cleanup(), qee_destroy() prototypes.
**	    Pointer will be set to null if DSH is deallocated.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
**/


/*{
** Name: qeu_d1_cre_tab - Create a table.
**
** External call: qef_call(QEU_DBU, & qeu_cb, caller);
**
** Description:
**	This function is called by QEU_DBU (in QEUS.C) to create a DDB table.
**
**  Notes:
**	1) All updates on the CDB are done under $ingres for Titan.
**	2) Until 2PC is implemented, CDB updates are registered with TPF as 
**	   READ-ONLY operations to avoid jeopardizing the single-site update 
**	   requirement imposed on the user.
**  
** Inputs:
**   External:
**      qeu_cb			    QEU_CB
**	    .qeu_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qeu_db_id		    DDB id
**	    .qeu_d_op		    DMU_CREATE_TABLE
**	    .qeu_d_cb		    non-NULL for Titan
**	    .qeu_qp		    name of QEF structure in QSF storage
**	    .qeu_qso		    ptr to structure below
**		<QED_DDL_INFO>	    
**		    .qed_d1_obj_name	    object's link name 
**		    .qed_d2_obj_owner	    object's owner name
**		    .qed_d5_qry_info_p	    ptr to CREATE TABLE subquery for LDB
**			.qed_q1_timeout	    timeout quantum, 0 if none
**			.qed_q2_lang	    DB_SQL or DB_QUEL
**			.qed_q3_qmode	    DD_UPDATE
**			.qed_q4_pkt_p	    CREATE TABLE statement for LDB
**		    .qed_d6_tab_info_p	    ptr to LDB table information
**			.dd_t1_tab_name	    same as qed_d3_obj_ldbname above
**			.dd_t2_tab_owner    owner of LDB table, blanks if
**					    unknown
**			.dd_t3_tab_type	    DD_2OBJ_TABLE
**			.dd_t9_ldb_p	    ptr to LDB information
**			    .dd_i1_ldb_desc	    LDB descriptor
**				.dd_l1_ingres_b	    FALSE for non-$ingres
**				.dd_l2_node_name    node name
**				.dd_l3_ldb_name	    LDB name
**				.dd_l4_dbms_name    INGRES if default
**				.dd_l5_ldb_id	    0 for unknown
**		    .qed_d8_obj_type	    DD_2OBJ_TABLE for creating a DDB
**					    table
**		    .qed_d9_reg_info_p	    ptr to iiregistrations query text
**					    for insertion
**  Internal:
**	v_qer_p				    QEF_RCB
**
** Outputs:
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
**	31-oct-88 (carl)
**          written
**	28-apr-90 (carl)
**	    modified to use the logged-in user name at the LDB instead of 
**	    at the DDB level for locating the local table
**	22-may-90 (carl)
**	    modified to not abort the DX if the local CREATE TABLE query
**	    fails
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qeu_d1_cre_tab(
QEF_RCB		*v_qer_p)
{
    DB_STATUS		status;
    QEF_DDB_REQ	    	*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QED_QUERY_INFO	*ddq_p = & ddr_p->qer_d4_qry_info;
    QED_DDL_INFO	*ddl_p = & ddr_p->qer_d7_ddl_info;
    DD_LDB_DESC		*ldb_p = & ddl_p->qed_d6_tab_info_p->dd_t9_ldb_p->
				    dd_i1_ldb_desc;
    DD_1LDB_INFO	ldb_info,
			*ldbinfo_p = & ldb_info;
    QEC_LINK		lnk,
			*lnk_p = & lnk;
    QEQ_1CAN_QRY	sel,
			*sel_p = & sel;
    QEC_L1_DBCONSTANTS	dbconsts;	    /* tuple structure */
    RQB_BIND		rq_bind[QEC_01I_CC_DBCONSTANTS_2 + 1];


    /* 1.  execute CREATE TABLE statement */

    STRUCT_ASSIGN_MACRO(*ddl_p->qed_d5_qry_info_p, *ddq_p);
    STRUCT_ASSIGN_MACRO(*ldb_p, ldbinfo_p->dd_i1_ldb_desc);
    ddr_p->qer_d2_ldb_info_p = ldbinfo_p;
    status = qed_e4_exec_qry(v_qer_p);    
    if (status)
    {
/*
	v_qer_p->qef_cb->qef_abort = TRUE;
*/
	return(status);
    }

    /* 2.  retrieve the user's name from the LDB for locating the created table
    **/

    status = qel_s0_setup(SEL_105_II_DBCONSTANTS, rq_bind, ldb_p, v_qer_p,
		lnk_p, sel_p);
    if (status)
    {
	v_qer_p->qef_cb->qef_abort = TRUE;
	return(status);
    }
/*
    sel_p->qeq_c1_can_id = SEL_105_II_DBCONSTANTS;
    sel_p->qeq_c2_rqf_bind_p = lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c4_ldb_p = ldb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    lnk_p->qec_6_select_p = sel_p;  ** must set **
*/
    sel_p->qeq_c3_ptr_u.l1_dbconstants_p = & dbconsts; /* must set */

    status = qel_s4_prepare(v_qer_p, lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {	
	status = qel_s3_flush(v_qer_p, lnk_p);
	if (status)
	    return(status);
    }
/*
    ** this call wipes out the next field dd_t3_tab_type!!! **

    qed_u0_trimtail( dbconsts.l1_2_dba_name, DB_OWN_MAXNAME,
	    ddl_p->qed_d6_tab_info_p->dd_t2_tab_owner);
*/
    /* 3.  create the link */

    status = qel_c0_cre_lnk(v_qer_p);
    if (status)
	v_qer_p->qef_cb->qef_abort = TRUE;

    return(status);
}    


/*{
** Name: qeu_d2_des_tab - Destroy a table.
**
** External call: qef_call(QEU_DBU, & qeu_cb, caller);
**
** Description:
**	This function is called by QEU_DBU (in QEUS.C) to destroy a DDB table.
**
**  Notes:
**	1) All updates on the CDB are done under $ingres for Titan.
**	2) Until 2PC is implemented, CDB updates are registered with TPF as 
**	   READ-ONLY operations to avoid jeopardizing the single-site update 
**	   requirement imposed on the user.
**  
** Inputs:
**   External:
**      qeu_p			    ptr to QEU_CB
**	    .qeu_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qeu_db_id		    DDB id
**	    .qeu_d_op		    DMU_DESTROY_TABLE
**	    .qeu_d_cb		    non-NULL for Titan
**	    .qeu_qp		    name of structure in QSF storage
**	    .qeu_qso		    ptr to structure below
**		<QED_DDL_INFO>	    
**		    .qed_d1_obj_name	    object's link name 
**		    .qed_d2_obj_owner	    object's owner name
**		    .qed_d5_qry_info_p	    ptr to DROP TABLE subquery for LDB
**			.qed_q1_timeout	    timeout quantum, 0 if none
**			.qed_q2_lang	    DB_SQL or DB_QUEL
**			.qed_q3_qmode	    DD_UPDATE
**			.qed_q4_pkt_p	    DESTROY TABLE statement for LDB
**		    .qed_d6_tab_info_p	    ptr to LDB table information
**			.dd_t1_tab_name	    same as qed_d3_obj_ldbname above
**			.dd_t9_ldb_p	    ptr to LDB information
**			    .dd_i1_ldb_desc	    LDB descriptor
**				.dd_l1_ingres_b	    FALSE for non-$ingres
**				.dd_l2_node_name    node name
**				.dd_l3_ldb_name	    LDB name
**				.dd_l4_dbms_name    INGRES if default
**				.dd_l5_ldb_id	    0 for unknown
**
**   Internal:
**	v_qer_p			    ptr to QEF_RCB
**
** Outputs:
**   External:
**	qeu_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**   Internal:
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
**	31-oct-88 (carl)
**          written
*/


DB_STATUS
qeu_d2_des_tab(
QEF_RCB		*v_qer_p)
{
    DB_STATUS		status;
/*
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
*/
    QEF_DDB_REQ	    	*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QED_QUERY_INFO	*ddq_p = & ddr_p->qer_d4_qry_info;
    QED_DDL_INFO	*ddl_p = & ddr_p->qer_d7_ddl_info;
    DD_1LDB_INFO	ldb_info,
			*ldb_p = & ldb_info;


    /* 1.  execute DESTROY TABLE statement */

    STRUCT_ASSIGN_MACRO(*ddl_p->qed_d5_qry_info_p, *ddq_p);
    STRUCT_ASSIGN_MACRO(ddl_p->qed_d6_tab_info_p->dd_t9_ldb_p->dd_i1_ldb_desc,
	ldb_p->dd_i1_ldb_desc);
    ddr_p->qer_d2_ldb_info_p = ldb_p;

    /* 2.  execute DESTROY TABLE statement */

    status = qed_e4_exec_qry(v_qer_p);    
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(v_qer_p);
	if (status)
	    return(status);
    }

    /* 3.  destroy the link and hence all dependent views */

    status = qel_d0_des_lnk(v_qer_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(v_qer_p);
	if (status)
	    return(status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qeu_d3_cre_view - create a view definition
**
** External QEF call:   status = qef_call(QEU_CVIEW, &qeuq_cb);
**
** Description:
**	This routine is called directly by qef_call to create a DDB view.  
**
**      Given the base table id (or a list of ids), the DMU_CB containing 
**	column information and the tree and query text information, the
**	algorithm does the following:
**
**	1.  Entries are made in IIDD_DDB_OBJECTS, IIDD_TABLES, IIDD_COLUMNS,
**	    IIDD_INGRES_TABLES.
**	2.  Entries are made in the IIDD_DBDEPENDS catalog connecting 
**	    link/table ids with the view id.  
**	3.  The tree tuples are updated with their table_id and tree_id 
**	    before being appended to the IIDD_TREE catalog.
**
**  External:
**
** Inputs:
**      qef_cb                  qef session control block
**      i_quq_p
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_ano		number of columns of view
**	    .qeuq_tsz		number of base relations, 0 is ok
**	    .qeuq_tbl_id	array of table ids of base links/tables
**	    .qeuq_obj_typ_p 	array of types to match with above array
**	    .qeuq_cq		number of query text tuples
**	    .qeuq_qry_tup	query text tuples
**	    .qeuq_ct		number of tree catalog tuples
**	    .qeuq_tre_tup	tree catalog tuples
**	    .qeuq_dmf_cb	dmu_cb for creating the view	    
**	    .qeuq_db_id		DDB id
**	    .qeuq_ddb_cb
**		.qeu_1_lang	DB_QUEL or DB_SQL, language of CREATE VIEW
**				statement
**		.qeu_2_view_chk_b
**				TRUE if check option is specified in the
**				CREATE VIEW statement, FALSE otherwise
**		.qeu_3_row_width
**				view row size
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-jan-89 (carl)
**          written
*/


DB_STATUS
qeu_d3_cre_view(
QEF_CB          *qef_cb,
QEUQ_CB		*i_quq_p)
{
    DB_STATUS	status;
    i4	ignore;
    DB_ERROR	err_stub;
    
    
    status = qeu_d6_cre_view(qef_cb, i_quq_p);
    if (status)
	qef_error(i_quq_p->error.err_code, 0L, status, & ignore,
		  & err_stub, 0);

    return(status);
}    


/*{
** Name: qeu_d4_des_view - destroy a view definition
**
**  External QEF call:   status = qef_call(QEU_DVIEW, &qeuq_cb);
**
** Description:
**	This routine is called directly by qef_call to destroy a view.  
**	A table or a view and all views that depend on the table or view are
**	destroyed.  Note only one table or view can be destroyed at a time.
**
**	View destruction is cascading in the sense that all dependent views are
**	first destroyed before the current one.
**
**  External:
**
** Inputs:
**      qef_cb                  qef session control block
**      i_quq_p			QEUQ_CB
**	    .qeuq_eflag	        designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_rtbl		id of view or table
**          .qeuq_qid           non-zero query id of view 
**
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_QE0023_INVALID_QUERY
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
**	    adapted from QEU_DVIEW
**	13-mar-89 (carl)
**	    added code to commit CDB association to avoid deadlocks
**	07-aug-91 (fpang)
**	    Initialize qer_p->error.err_code to 0 and 
**	    qer_p->qef_r3_ddb_req.qer_d13_ctl_info to QEF_00DD_NIL_INFO.
**	    Fixes B38882.
**	29-jan-1993 (fpang)
**	    Compare qef_auto QEF_ON or QEF_OFF instead of 0 or !0.
**	    Also, register to TPF as an updater if ddl concurrency is off.
*/


DB_STATUS
qeu_d4_des_view(
QEF_CB          *qef_cb,
QEUQ_CB		*i_quq_p)
{
    DB_STATUS		status,
			sav_status = E_DB_OK;
    DB_ERROR		sav_error;
    QEF_RCB		qer,
			*qer_p = & qer;
    QED_DDL_INFO	*ddl_p = & qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    QES_DDB_SES		*dds_p = & qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
    bool		xact_b = FALSE;		/* assume no begin transaction 
						*/
    i4			xact_mode;
    
    /* 1.  check input information */

    if (i_quq_p->qeuq_type != QEUQCB_CB 
	||
        i_quq_p->qeuq_length != sizeof(i_quq_p))	
    {
	i_quq_p->error.err_code = E_QE0017_BAD_CB;
	return(E_DB_ERROR);
    }

    /* 2.  set up information */

    MEfill(sizeof(qer), '\0', (PTR) & qer);
    qer_p->qef_cb = qef_cb;
    qer_p->error.err_code = 0;
    qer_p->qef_r3_ddb_req.qer_d13_ctl_info = QEF_00DD_NIL_INFO;
    qef_cb->qef_rcb = qer_p;

    MEfill(sizeof(DD_OBJ_NAME), ' ', ddl_p->qed_d1_obj_name);
    MEfill(sizeof(DD_OWN_NAME), ' ', ddl_p->qed_d2_obj_owner);
    ddl_p->qed_d3_col_count = 0;
    ddl_p->qed_d4_ddb_cols_pp = NULL;
    ddl_p->qed_d5_qry_info_p = NULL;
    ddl_p->qed_d6_tab_info_p = NULL;
    STRUCT_ASSIGN_MACRO(*i_quq_p->qeuq_rtbl, ddl_p->qed_d7_obj_id);
    ddl_p->qed_d9_reg_info_p = NULL;

    /* transaction semantics
    **	 
    **	  1.   pre-processing: 
    **	    
    **		1.1  if no outstanding transaction, begin a transaction 
    **		     and set xact_b to TRUE;
    **		1.2  if not in auto-commit mode, escalate transaction to MST;
    **
    **	  2.	post-processing:
    **	    
    **		2.1  if processing terminates normally, commit transaction
    **		     only if xact_b is TRUE and not in auto-commit mode;
    **		2.2  if processing terminates abnormally, abort transaction
    **		     only if xact_b is TRUE;
    */

    /* 2.  begin a transaction if necessary */

    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	status = qet_begin(qef_cb);
	if (status)
	    return(status);

	xact_b = TRUE;
    }

    if (qef_cb->qef_auto == QEF_OFF)
	qef_cb->qef_stat = QEF_MSTRAN;		/* escalate to MST */

    /* 3.  inform TPF of update intention on CDB */

    /* 3.1 2PC is required if DDL concurrency is off. */
    if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON)
	xact_mode = QEK_4TPF_1PC;
    else
	xact_mode = QEK_3TPF_UPDATE;

    status = qet_t5_register(qer_p, cdb_p, DB_SQL, xact_mode);
    if (status)
    {
	if (! xact_b)
	{
	    STRUCT_ASSIGN_MACRO(qer_p->error, i_quq_p->error);
	    return(status);

	    /* ok to return */
	}
    }

    if (status == E_DB_OK)
    {
	/* 4.  destroy the view */

	status = qeu_20_des_view(qer_p);
	if (status)
	{
	    /* forgive error */

	    status = qed_u9_log_forgiven(qer_p);
	}
    }

    /* 5.  end transaction if necessary */

    if (status)
    {
	/* error condition */

	sav_status = status;
	STRUCT_ASSIGN_MACRO(qer_p->error, sav_error);
	
	if (xact_b)				/* abort SST */
	    status = qet_abort(qef_cb);
    }
    else
    {
	/* ok condition */

	if (xact_b && qef_cb->qef_auto == QEF_ON)		
	    status = qet_commit(qef_cb);	/* commit SST */
	else
	{
	    /* send message to commit CDB association to avoid deadlocks */

	    status = qed_u11_ddl_commit(qef_cb->qef_rcb);
	}
    }
    
    if (sav_status)
    {
	/* return saved error */

	STRUCT_ASSIGN_MACRO(sav_error, i_quq_p->error);
	return(sav_status);
    }
    else if (status)
    {
	/* return qef_cb error */

	STRUCT_ASSIGN_MACRO(qef_cb->qef_rcb->error, i_quq_p->error);
	return(status);
    }
    return(E_DB_OK);
}


/*{
** Name: qeu_d5_alt_tab - Alter characteristic of a table.
**
** External call: qef_call(QEU_DBU, & qeu_cb, caller);
**
** Description:
**	This function is called by QEU_DBU (in QEUS.C) to alter the
**  characteristics of a DDB table.
**
**  Notes:
**	1) All updates on the CDB are done under $ingres for Titan.
**	2) Until 2PC is implemented, CDB updates are registered with TPF as 
**	   READ-ONLY operations to avoid jeopardizing the single-site update 
**	   requirement imposed on the user.
**  
** Inputs:
**  EXTERNAL:
**      i_qeu_p			    ptr to QEU_CB
**	    .qeu_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qeu_db_id		    DDB id
**	    .qeu_d_cb		    non-NULL for Titan
**	    .qeu_qp		    name of QEF structure in QSF storage
**	    .qeu_qso		    ptr to DMT_CB
**		.dmt_id			    table name
**		.dmt_char_array.data_address
**		    .char_id		    DMT_C_ZOPTSTATS
**		    .char_value		    DMT_C_ON | DMT_C_OFF
**
**  INTERNAL:
**	v_qer_p			    ptr to QEF_RCB
**	i_dmt_p			    ptr to DMT_CB
**
** Outputs:
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
**	18-dec-88 (carl)
**          written
**	29-jan-1993 (fpang)
**	    Register to TPF as an updater if ddl concurrency is off.
*/


DB_STATUS
qeu_d5_alt_tab(
QEF_RCB		*v_qer_p,
DMT_CB		*i_dmt_p)
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D9_TABLEINFO	
		    tabinfo;			/* dummy part of link */
    QEC_D6_OBJECTS  objects,
		    *objs_p = & objects;
    QEC_L16_TABLES  tables,
		    *tabs_p = & tables;
    QEQ_1CAN_QRY    upd,
		    *upd_p = & upd;
    QEC_LINK	    link,
		    *lnk_p = & link; 
    DMT_CHAR_ENTRY  *chr_p;
    bool	    err_b = FALSE;		/* assume no error */
    i4		    xact_mode;


    MEfill(sizeof(link), '\0', (PTR) & link);
    lnk_p->qec_2_tableinfo_p = & tabinfo;
    lnk_p->qec_13_objects_p = objs_p;

    /* 1.  vefify input parameters */

    chr_p = (DMT_CHAR_ENTRY *) i_dmt_p->dmt_char_array.data_address;

    if (chr_p->char_id != DMT_C_ZOPTSTATS)
	err_b = TRUE;
    else 
    {
	if (chr_p->char_value == DMT_C_ON)
	{
	    tabs_p->l16_9_stats[0] = 'Y';
	    tabs_p->l16_9_stats[1] = EOS;
	}
	else if (chr_p->char_value == DMT_C_OFF)
	{
	    tabs_p->l16_9_stats[0] = 'N';
	    tabs_p->l16_9_stats[1] = EOS;
	}
	else
	    err_b = TRUE;
    }
    if (err_b)
    {
	status = qed_u2_set_interr(E_QE0018_BAD_PARAM_IN_CB, & v_qer_p->error);
	return(status);
    }

    /* 2.  inform TPF of update */

    /* 2.1 2PC is required if DDL concurrency is off. */
    if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON)
	xact_mode = QEK_4TPF_1PC;
    else
	xact_mode = QEK_3TPF_UPDATE;

    status = qet_t5_register(v_qer_p, cdb_p, DB_SQL, xact_mode);
    if (status)
	return(status);

    /* 3.  set up to update TABLE_STATS and MODIFY_DATE of IIDD_TABLES */

    qed_u0_trimtail( (char *) & i_dmt_p->dmt_table, DB_TAB_MAXNAME,
	tabs_p->l16_1_tab_name);

    qed_u0_trimtail( (char *) & i_dmt_p->dmt_owner, DB_OWN_MAXNAME,
	tabs_p->l16_2_tab_owner);

    status = qed_u8_gmt_now(v_qer_p, tabs_p->l16_21_mod_date);
    if (status)
	return(status);

    tabs_p->l16_21_mod_date[DD_25_DATE_SIZE] = EOS;

    upd_p->qeq_c1_can_id = UPD_731_DD_TABLES;
    upd_p->qeq_c2_rqf_bind_p = NULL;
    upd_p->qeq_c3_ptr_u.l16_tables_p = tabs_p;
    upd_p->qeq_c4_ldb_p = cdb_p;

    lnk_p->qec_23_update_p = upd_p;
    status = qel_u1_update(v_qer_p, lnk_p);    
    if (status)
    {
	return(status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qeu_d6_cre_view - auxiliary rotuine for qeu_d3_cre_view
**
** Description:
**	This routine is called directly by qef_call to create a DDB view.  
**
**      Given the base table id (or a list of ids), the DMU_CB containing 
**	column information and the tree and query text information, the
**	algorithm does the following:
**
**	1.  Entries are made in IIDD_DDB_OBJECTS, IIDD_TABLES, IIDD_COLUMNS,
**	    IIDD_INGRES_TABLES.
**	2.  Entries are made in the IIDD_DBDEPENDS catalog connecting 
**	    link/table ids with the view id.  
**	3.  The tree tuples are updated with their table_id and tree_id 
**	    before being appended to the IIDD_TREE catalog.
**
**  External:
**
** Inputs:
**      qef_cb                  qef session control block
**      i_quq_p
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_ano		number of columns of view
**	    .qeuq_tsz		number of base relations, 0 is ok
**	    .qeuq_tbl_id	array of table ids of base links/tables
**	    .qeuq_obj_typ_p 	array of types to match with above array
**	    .qeuq_cq		number of query text tuples
**	    .qeuq_qry_tup	query text tuples
**	    .qeuq_ct		number of tree catalog tuples
**	    .qeuq_tre_tup	tree catalog tuples
**	    .qeuq_dmf_cb	dmu_cb for creating the view	    
**	    .qeuq_db_id		DDB id
**	    .qeuq_ddb_cb
**		.qeu_1_lang	DB_QUEL or DB_SQL, language of CREATE VIEW
**				statement
**		.qeu_2_view_chk_b
**				TRUE if check option is specified in the
**				CREATE VIEW statement, FALSE otherwise
**		.qeu_3_row_width
**				view row size
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
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
**          adapted from QEU_CVIEW
**	13-mar-89 (carl)
**	    added code to commit CDB association to avoid deadlocks
**	07-aug-91 (fpang)
**	    Initialize qer_p->error.err_code to 0 and 
**	    qer_p->qef_r3_ddb_req.qer_d13_ctl_info to QEF_00DD_NIL_INFO.
**	    Fixes B38882.
**	10-feb-93 (andre)
**	    if a view ia based on 0 tables, qeuq_tbl_id WILL BE 0
**	29-jan-1993 (fpang)
**	    Compare qef_auto QEF_ON or QEF_OFF instead of 0 or !0.
**	    Also, register to TPF as an updater if ddl concurrency is off.
*/

DB_STATUS
qeu_d6_cre_view(
QEF_CB          *qef_cb,
QEUQ_CB		*i_quq_p)
{
    DB_STATUS		status,
			sav_status = E_DB_OK;
    DB_ERROR		sav_error;
    QEF_RCB		qer,
			*qer_p = & qer;
    QES_DDB_SES		*dds_p = & qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO	*ddl_p = & qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L16_TABLES	tables;		    /* used for IITABLES-style info */
    QEC_D6_OBJECTS	objects;	    /* used for object info */
    QEC_LINK		link,		    /* used as global control block */
			*lnk_p = & link;
    QEQ_1CAN_QRY	ins,
			sel,
			upd;		    /* used for ordering canned query */
    RQB_BIND		rq_bind[QEC_CAT_COL_COUNT_MAX + 1];
    QEP_PTR_UNION	ptr_u;
    DMU_CB		*dmu_p;
    SYSTIME		now;
    bool		xact_b = FALSE;		/* assume no begin transaction 
						*/
    i4			xact_mode;

    
    ptr_u.qep_ptr_u.qep_1_ptr = i_quq_p->qeuq_dmf_cb;
    dmu_p = ptr_u.qep_ptr_u.qep_3_dmu_cb_p;

    /* 1.  check input information */

    if (i_quq_p->qeuq_type != QEUQCB_CB 
	||
        i_quq_p->qeuq_length != sizeof(*i_quq_p))
    {
	i_quq_p->error.err_code = E_QE0017_BAD_CB;
	return(E_DB_ERROR);
    }

    if ((i_quq_p->qeuq_cq == 0 || i_quq_p->qeuq_qry_tup == 0)
        || 
	(i_quq_p->qeuq_ct == 0 || i_quq_p->qeuq_tre_tup == 0)
        || 
	(i_quq_p->qeuq_db_id == 0)
        || 
	(i_quq_p->qeuq_d_id == 0)
        || 
	(i_quq_p->qeuq_dmf_cb == 0))
    {
	i_quq_p->error.err_code = E_QE0018_BAD_PARAM_IN_CB;
	return(E_DB_ERROR);
    }

    /* 2.  set up control information */

    MEfill(sizeof(qer), '\0', (PTR) & qer);
    qer_p->qef_cb = qef_cb;
    qer_p->error.err_code = 0;
    qer_p->qef_r3_ddb_req.qer_d13_ctl_info = QEF_00DD_NIL_INFO;
    qef_cb->qef_rcb = qer_p;

    MEcopy(dmu_p->dmu_table_name.db_tab_name, 
	sizeof(ddl_p->qed_d1_obj_name),
	ddl_p->qed_d1_obj_name);
    MEcopy(dmu_p->dmu_owner.db_own_name, 
	sizeof(ddl_p->qed_d2_obj_owner),
	ddl_p->qed_d2_obj_owner);
    ddl_p->qed_d3_col_count = i_quq_p->qeuq_ano;
    ddl_p->qed_d4_ddb_cols_pp = NULL;
    ddl_p->qed_d5_qry_info_p = NULL;
    ddl_p->qed_d6_tab_info_p = NULL;
    ddl_p->qed_d7_obj_id.db_tab_base = 0;
    ddl_p->qed_d7_obj_id.db_tab_index = 0;
    ddl_p->qed_d9_reg_info_p = NULL;

    sel.qeq_c2_rqf_bind_p = rq_bind;		/* must set up */
    ins.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;
    upd.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;
    
    lnk_p->qec_1_ddl_info_p = ddl_p;
    lnk_p->qec_2_tableinfo_p = NULL;
    lnk_p->qec_3_ldb_id = 0;			
    lnk_p->qec_4_col_cnt = 0;
    MEfill(DB_DB_MAXNAME, ' ', lnk_p->qec_5_ldb_alias);
    lnk_p->qec_6_select_p = & sel;
    lnk_p->qec_7_ldbids_p = NULL;
    lnk_p->qec_8_longnames_p = NULL;
    lnk_p->qec_9_tables_p = & tables;
    lnk_p->qec_10_haves = QEC_07_CREATE;
    lnk_p->qec_11_ldb_obj_cnt = 0;
    lnk_p->qec_12_indexes_p = NULL;
    lnk_p->qec_13_objects_p = & objects;
    lnk_p->qec_15_ndx_cnt = 0;
    lnk_p->qec_16_ndx_ids_p = NULL;
    lnk_p->qec_19_ldb_p = NULL;
    lnk_p->qec_20_rqf_bind_p = rq_bind;
    lnk_p->qec_21_delete_p = NULL;
    lnk_p->qec_22_insert_p = & ins;
    lnk_p->qec_23_update_p = & upd;

    status = qed_u8_gmt_now(qer_p, lnk_p->qec_24_cur_time);
    if (status)
    {
	STRUCT_ASSIGN_MACRO(qer_p->error, i_quq_p->error);
	return(status);
    }
    lnk_p->qec_24_cur_time[DD_25_DATE_SIZE] = EOS;

    lnk_p->qec_25_pre_mins_p = NULL;
    lnk_p->qec_26_aft_mins_p = NULL;
    
    TMnow(& now);
    lnk_p->qec_17_ts1 = now.TM_secs;
    lnk_p->qec_18_ts2 = now.TM_msecs;

    /* transaction semantics
    **	 
    **	  1.	pre-processing: 
    **	    
    **		1.1  if no outstanding transaction, begin a transaction 
    **		     and set xact_b to TRUE;
    **		1.2  if not in auto-commit mode, escalate transaction to MST;
    **
    **	  2.	post-processing:
    **	    
    **		2.1  if processing terminates normally, commit transaction
    **		     only if xact_b is TRUE and not in auto-commit mode;
    **		2.2  if processing terminates abnormally, abort transaction
    **		     only if xact_b is TRUE;
    */

    /* 2.  begin a transaction if necessary */

    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	status = qet_begin(qef_cb);
	if (status)
	    return(status);

	xact_b = TRUE;
    }

    if (qef_cb->qef_auto == QEF_OFF)
	qef_cb->qef_stat = QEF_MSTRAN;		/* escalate to MST */

    /* 3.  inform TPF of read intention on CDB */

    status = qet_t5_register(qer_p, cdb_p, DB_SQL, QEK_2TPF_READ);
    if (status)
    {
	if (! xact_b)
	{
	    STRUCT_ASSIGN_MACRO(qer_p->error, i_quq_p->error);
	    return(status);
	
	    /* ok to return */
	}

	/* fall thru to terminate SST */
    }

    if (status == E_DB_OK)
    {
	/*  4.  set up new object base, query id */

	status = qeu_10_get_obj_base(qer_p, lnk_p);
	if (status)
	{
	    if (! xact_b)
	    {
		STRUCT_ASSIGN_MACRO(qer_p->error, i_quq_p->error);
		return(status);

		/* ok to return */
	    }
	
	    /* fall thru to terminate SST */
	}
	else
	{
	    objects.d6_4_qry_id.db_qry_high_time = now.TM_secs;
	    objects.d6_4_qry_id.db_qry_low_time = now.TM_msecs;
	}
    }    
 
    if (status == E_DB_OK)
    {
	/*  5.  inform TPF of update intention on CDB */

	/*  5.1 2PC is required if DDL concurrency is off. */
	if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON)
	    xact_mode = QEK_4TPF_1PC;
	else
	    xact_mode = QEK_3TPF_UPDATE;

	status = qet_t5_register(qer_p, cdb_p, DB_SQL, xact_mode);
	if (status)
	{
	    if (! xact_b)
	    {
		STRUCT_ASSIGN_MACRO(qer_p->error, i_quq_p->error);
		return(status);

		/* ok to return */
	    }
	    
	    /* fall thru to terminate SST */
	}
    }

    if (status == E_DB_OK)
    {
	/*  6.  update IIDD_DDB_OBJECT_BASE */	

	status = qeu_11_object_base(qer_p, lnk_p);
	if (status)
	    qef_cb->qef_abort = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*  7.  update IIDD_DDB_OBJECTS */	

	status = qeu_12_objects(qer_p, lnk_p);
	if (status)
	    qef_cb->qef_abort = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*  8.  update IIDD_IITABLES */	

	status = qeu_13_tables(qer_p, i_quq_p, lnk_p);
	if (status)
	    qef_cb->qef_abort = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*  9.  update IIDD_COLUMNS */	

	status = qeu_14_columns(qer_p, i_quq_p, lnk_p);
	if (status)
	    qef_cb->qef_abort = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*  10.  update IIDD_VIEWS */	

	status = qeu_15_views(qer_p, i_quq_p, lnk_p);
	if (status)
	    qef_cb->qef_abort = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*  11.  update IIDD_DDB_TREE */	

	status = qeu_16_tree(qer_p, i_quq_p, lnk_p);
	if (status)
	    qef_cb->qef_abort = TRUE;
    }

    if (status == E_DB_OK)
    {
	/*  12.  update IIDD_DDB_DDB_DBDEPENDS */	

	status = qeu_17_dbdepends(qer_p, i_quq_p, lnk_p);
	if (status)
	    qef_cb->qef_abort = TRUE;
    }

    /* 13.  end transaction if necessary */

    if (status)
    {
	/* error condition */

	sav_status = status;
	STRUCT_ASSIGN_MACRO(qer_p->error, sav_error);
	if (xact_b  || qef_cb->qef_abort)	/* SST or abort */
	    status = qet_abort(qef_cb);
    }
    else
    {
	/* ok condition */

	if (xact_b && qef_cb->qef_auto == QEF_ON)		
	    status = qet_commit(qef_cb);	/* commit SST */
	else
	{
	    /* send message to commit CDB association to avoid deadlocks */

	    status = qed_u11_ddl_commit(qef_cb->qef_rcb);
	}
    }
    
    if (sav_status)
    {
	/* returned saved error */
    
	STRUCT_ASSIGN_MACRO(sav_error, i_quq_p->error);
	return(sav_status);
    }
    else if (status)
    {
	/* return qef_cb error */

	STRUCT_ASSIGN_MACRO(qef_cb->qef_rcb->error, i_quq_p->error);
	return(status);
    }
    return(E_DB_OK);
}    


/*
**  Name: qeu_d7_b_copy	- begin the copy operation
**  
**  External QEF call:   status = qef_call(QED_B_COPY, &qef_rcb);
**  
**  Description:
**      Initialize for COPY command by establishing a DIRECT connection
**      for SCF and RQF for direct processing.
**  
**  Inputs:
**       qef_rcb                      
**          .qef_cb		     session control block
**	    .qef_r3_ddb_req		    QEF_DDB_REQ
**		.qer_d4_qry_info
**		    .qed_q2_lang	    DB_QUEL or DB_SQL
**		.qer_d6_conn_info
**		    .qed_c1_ldb		    DD_LDB_DESC, LDB desc
**			.dd_l1_ingres_b	    TRUE if $ingres access required
**					    FALSE otherwise
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    database name
**			.dd_l5_ldb_id	    0 for unknown
**               
**  Outputs:
**       qef_rcb
**           .error.err_code         One of the following
**                                   E_QE0000_OK
**                                   E_QE0002_INTERNAL_ERROR
**       Returns:
**           E_DB_OK
**           E_DB_ERROR                      caller error
**           E_DB_FATAL                      internal error
**       Exceptions:
**           none
**  
**  Side Effects:
**           none
**  
**  History:
**      27-mar-89 (carl)    
**	    written
*/


DB_STATUS
qeu_d7_b_copy(
QEF_RCB       *qef_rcb)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;



    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	/* start transaction */

	qef_rcb->qef_modifier = QEF_SSTRAN;
	qef_rcb->qef_flag = 0;
	status = qet_begin(qef_cb);
	if (status)
	    return(status);
    }

    status = qed_c1_conn(qef_rcb);		/* TPF informed of LDB by 
						** call */
    return(status);				/* always return for DDB */
}


/*
**  Name: qeu_d8_e_copy	- end the copy operation
**  
**  External QEF call:   status = qef_call(QED_E_COPY, &qef_rcb);
**  
**  Description:
**      A copy command is ended.  The transaction is ended if appropriate.  
**  
**  Inputs:
**       qef_rcb
**           .qef_cb		     session control block
**               
**  Outputs:
**       qef_rcb
**           .error.err_code         One of the following
**                                   E_QE0000_OK
**                                   E_QE0002_INTERNAL_ERROR
**                                   E_QE0004_NO_TRANSACTION
**       Returns:
**           E_DB_OK
**           E_DB_ERROR                      caller error
**           E_DB_FATAL                      internal error
**       Exceptions:
**           none
**  
**  Side Effects:
**           none
**  
**  History:
**      27-mar-89 (carl)    
**	     written
*/


DB_STATUS
qeu_d8_e_copy(
QEF_RCB     *qef_rcb)
{
    DB_STATUS		status = E_DB_OK;
    QEF_CB		*qef_cb = qef_rcb->qef_cb;
    QES_DDB_SES		*dds_p = & qef_rcb->qef_cb->qef_c2_ddb_ses;



    dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* must reset CONNECT state */

    if ((qef_cb->qef_stat == QEF_MSTRAN) && (qef_cb->qef_open_count == 0))
	return(status);			/* leave transaction alone */

    if ((qef_cb->qef_stat != QEF_NOTRAN) && (qef_cb->qef_open_count == 0))
    {
	status = qet_commit(qef_cb);
	/* clean up any cursor that might be opened */
	status = qee_cleanup(qef_rcb, (QEE_DSH **)NULL);
    }

    return(status);				/* always return for DDB */
}


/*
**  Name: qeu_d9_a_copy	- abort the copy operation
**  
**  External QEF call:   status = qef_call(QED_A_COPY, &qef_rcb);
**  
**  Description:
**      A copy command is aborted.  The transaction is aborted if appropriate.  
**  
**  Inputs:
**       qef_rcb
**           .qef_cb		     session control block
**               
**  Outputs:
**       qef_rcb
**           .error.err_code         
**       Returns:
**           E_DB_OK
**           E_DB_ERROR              
**       Exceptions:
**           none
**  
**  Side Effects:
**           none
**  
**  History:
**      22-mar-90 (carl)    
**	     written
*/


DB_STATUS
qeu_d9_a_copy(
QEF_RCB     *qef_rcb)
{
    DB_STATUS	    status = E_DB_OK;
    QEF_CB	    *qef_cb = qef_rcb->qef_cb;


    status = qed_u16_rqf_cleanup(qef_rcb);

    /* ignore error */

    status = qet_abort(qef_cb);
    return(status);
}

