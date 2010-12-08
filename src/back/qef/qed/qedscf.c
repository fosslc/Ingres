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
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEDSCF.C - Routines performing SCF operations.
**
**  Description:
**      These routines perform general DDB functions. 
**
**          qed_s0_sesscdb  - pass CDB descriptor to TPF
**          qed_s1_iidbdb   - set up control block for accessing iidbdb
**          qed_s2_ddbinfo  - get access status of DDB and the CDB's information
**          qed_s3_ldbplus  - get the the LDB's charateristics & capabilities
**          qed_s4_usrstat  - get user's general DB access status 
**          qed_s5_ldbcaps  - get LDB's capabilities
**          qed_s6_modify   - modify the CDB's association
**          qed_s7_term_assoc 
**			    - terminate an open association with RQF
**          qed_s8_cluster_info
**			    - pass cluster node names to RQF
**	    qed_s9_system_owner
**			    - fetch iidbconstants.system_owner if it exists
**
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	15-jan-89 (carl)
**	    modified qed_s3_ldbplus in accordance with the new DD_0LDB_PLUS
**      22-jan-89 (carl)
**	    modified to qed_s5_ldbcaps to include OWNER_NAME, etc
**      04-mar-89 (carl)
**	    added qed_s6_modify 
**      13-mar-89 (carl)
**	    renamed qed_u7_acc_commit to qed_u7_msg_commit
**      10-apr-89 (carl)
**	    modified to omit calling TPF for startup READ queries
**      04-may-89 (carl)
**	    changed qed_s5_ldbcaps to commit the LDB to leave a clean
**	    local session for the user's transaction
**	17-jun-89 (carl)
**	    changed qed_s5_ldbcaps to check for no-transaction state before 
**	    calling qed_u7_msg_commit
**	16-nov-89 (carl)
**	    modified qed_s2_ddbinfo to provide the DDB's information to TPF
**	02-jun-90 (carl)
**	    modified qed_s3_ldbplus to initialize to 0 new field dd_l6_flags
**	    introduced for 2PC
**	06-jun-90 (carl)
**	    modified qed_s2_ddbinfo to determine if CDB is 2PC/1PC;
**	    added qed_s0_sesscdb
**	08-jun-90 (carl)
**	    modified qed_s3_ldbplus to skip initializing field dd_l6_flags
**	    introduced for 2PC
**	23-jun-90 (carl)
**	    changed qed_s5_ldbcaps to set in LDB descriptor's dd_l6_flags 
**	    DD_03FLAG_SLAVE2PC if the LDB does NOT disclaim the SLAVE 2PC 
**	    protocol
**	14-jul-90 (carl)
**	    added qed_s8_cluster_info to inform RQF of the cluster node names
**      04-aug-90 (carl)
**	    changed qed_s6_modify to maintain new flag,
**	    QES_05CTL_SYSCAT_USER , in DDB session control subblock to 
**	    indicate whether "+Y" is on
**	10-aug-90 (carl)
**	    changed qed_s3_ldbplus to replace structure assignment with MEfill 
**	    for DD_0LDB_PLUS initialization;
**	    changed all occurrences of rqf_call to qed_u3_rqf_call;
**	    changed all occurrences of tpf_call to qed_u17_tpf_call;
**	20-feb-91 (fpang)
**	    Added qed_s9_system_owner() to fetch iidbconstants.system_owner.
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	13-jan-1993 (fpang)
**	    Added ldb temp table support.
**	9-jul-93 (rickh)
**	    Make MEfill call conform to prototype.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	10-dec-93 (robf)
**          Reworked database access to use standard catalog (iidatabase_info)
**          not core catalog (iidatabase)
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
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning / prototype fixes.
**/


/*  Global variables referenced in this module */

GLOBALREF   char	     IIQE_c1_iidbdb[];	    /* string "iidbdb" */
GLOBALREF   char	     IIQE_c6_cap_ingres[];  /* string "INGRES" */
GLOBALREF   char	    *IIQE_c7_sql_tab[];	    /* table of SQL keywords */
GLOBALREF   char	    *IIQE_c9_ldb_cat[];	    /* table of LDB catalog 
						    ** names */
GLOBALREF   char	    *IIQE_10_col_tab[];    /* table of column names */


/*{
** Name: QED_S0_SESSCDB - pass CDB descriptor to TPF for new session
**
** External QEF call:   status = qef_call(QED_SESSCDB, &qef_rcb);
**
** Description:
**
**  (See above.)
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              qef_cb			QEF_CB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d1_ddb_p	DD_DDB_DESC
**			.dd_d1_ddb_name		    DDB name
**			.dd_d4_iidbdb_p		    DD_1LDB_INFO
**			    .dd_i1_ldb_desc	    DD_LDB_DESC
**				.dd_l1_ingres_b	    FALSE
**				.dd_l2_node_name    host name or blanks
**				.dd_l3_ldb_name	    "iidbdb"
**				.dd_l4_dbms_name    "INGRES"
**				.dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**				.dd_l6_ldb_id	    DD_UNASSIGNED_IS_0
**		    .qer_d5_timeout	timeout quantum 
** Outputs:
**	none
**          Returns:
**		E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	06-jun-90 (carl)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qed_s0_sesscdb(
QEF_RCB         *v_qer_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    TPR_CB	    tp_cb,
		    *tpr_p = & tp_cb;


    v_qer_p->error.err_code = E_QE0000_OK;

    /* inform TPF of the CDB information */

    MEfill(sizeof(tp_cb), '\0', ( PTR ) &tp_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;
    tpr_p->tpr_16_ddb_p = v_qer_p->qef_r3_ddb_req.qer_d1_ddb_p;;

    status = qed_u17_tpf_call(TPF_SET_DDBINFO, tpr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    return(E_DB_OK);
}


/*{
** Name: QED_S1_IIDBDB - set up control structure for accessing iidbdb.
**
** External QEF call:   status = qef_call(QED_IIDBDB, &qef_rcb);
**
** Description:
**
**	This function sets up an LDB control block for the purpose of accessing
**  iidbdb.  It queries dbservice value from IIDATABASE to get case translation
**  semantics of the iidbdb.
**  
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d2_ldb_info_p	DD_LDB_DESC ptr
** Outputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d1_ddb_p	DD_DDB_DESC ptr
**		    	.dd_d2_dba_desc	    dba name
**			.dd_d6_dbservice    dbservice (flags) column info
**		    .qer_d2_ldb_info_p	DD_LDB_DESC ptr
**			.dd_l1_ingres_b	    FALSE
**			.dd_l2_node_name    blanks for default
**			.dd_l3_ldb_name	    "iidbdb"
**			.dd_l4_dbms_name    INGRES
**			.dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**			.dd_l6_ldb_id	DD_UNASSIGNED_IS_0
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**          Returns:
**		E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	25-mar-93 (barbara)
**	    This function now queries IIDATABASE for dbservice in order
**	    to ascertain case translation semantics of the iidbdb.
*/


DB_STATUS
qed_s1_iidbdb(
QEF_RCB         *v_qer_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *iidbdb_p = 
		    &v_qer_p->qef_r3_ddb_req.qer_d2_ldb_info_p->dd_i1_ldb_desc;
    DD_DDB_DESC	    *ddb_p = v_qer_p->qef_r3_ddb_req.qer_d1_ddb_p;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;
    RQB_BIND	    rq_bind[QEK_2_COL_COUNT];	    /* for 2 columns */
    bool	    log_qry_55 = FALSE,
		    log_err_59 = FALSE;
    i4         i4_1, 
		    i4_2;
    char	    qrytxt[QEK_200_LEN];


    v_qer_p->error.err_code = E_QE0000_OK;

    /* 1.  set up LDB control block for iidbdb */

    iidbdb_p->dd_l1_ingres_b = FALSE;
    MEfill(DB_NODE_MAXNAME, ' ', iidbdb_p->dd_l2_node_name);
    MEmove(STlength(IIQE_c1_iidbdb), IIQE_c1_iidbdb, ' ', 
	sizeof(iidbdb_p->dd_l3_ldb_name), iidbdb_p->dd_l3_ldb_name);
    MEfill(DB_TYPE_MAXLEN, ' ', iidbdb_p->dd_l4_dbms_name);
    iidbdb_p->dd_l5_ldb_id = DD_0_IS_UNASSIGNED;

    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
    }
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    /* 2.  set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = iidbdb_p;
    rqr_p->rqr_q_language = DB_SQL;

    /* 3.  set up query: 
    **
    **	select database_owner, database_service from iidatabase_info where 
    **	database_name = 'iidbdb' or database_name = 'IIDBDB';
    */

    STprintf(qrytxt, "%s %s, %s %s %s %s %s = 'iidbdb' %s %s = 'IIDBDB';",
	    IIQE_c7_sql_tab[SQL_07_SELECT],
	    IIQE_10_col_tab[COL_49_DATABASE_OWNER],
	    IIQE_10_col_tab[COL_50_DATABASE_SERVICE],
	    IIQE_c7_sql_tab[SQL_02_FROM],
	    IIQE_c9_ldb_cat[TII_24_IIDATABASE_INFO],
	    IIQE_c7_sql_tab[SQL_11_WHERE],
	    IIQE_10_col_tab[COL_48_DATABASE_NAME],
	    IIQE_c7_sql_tab[SQL_06_OR],
	    IIQE_10_col_tab[COL_48_DATABASE_NAME]);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
    }

    /* 4.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
	}

	return(status);
    }

    /* 5.  set up to fetch result */

    rq_bind[0].rqb_addr = (PTR) ddb_p->dd_d2_dba_desc.dd_u1_usrname;
    rq_bind[0].rqb_length = DB_OWN_MAXNAME;
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[1].rqb_addr = (PTR) & ddb_p->dd_d6_dbservice;
    rq_bind[1].rqb_length = sizeof(ddb_p->dd_d6_dbservice);
    rq_bind[1].rqb_dt_id = DB_INT_TYPE;

    rqr_p->rqr_col_count = QEK_2_COL_COUNT;
    rqr_p->rqr_bind_addrs = rq_bind;

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
	}

	return(status);
    }

    if (rqr_p->rqr_end_of_data)
    {	/* no data! */
	status = qed_u2_set_interr(
		    E_QE0061_BAD_DB_NAME,
		    & v_qer_p->error);
	return(status);
    }

    /* must flush */
    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 6.  commit read operation to release resources */

    status = qed_u7_msg_commit(v_qer_p, iidbdb_p);

    return(status);
}


/*{
** Name: QED_S2_DDBINFO - get access status of DDB, the CDB information
**
** External QEF call:   status = qef_call(QED_DDBINFO, &qef_rcb);
**
** Description:
**
**  Given a DDB name, QEF will query IIDBDB in the current node to retrieve:
**
**	1)  the access status (private or public) of the DDB from 
**	    IIDATABASE and 
**	2)  the CDB's information from IISTAR_CDBS.
**
**  Note that if this operation is successful, QEF will record the pointer to
**  the DDB descriptor in its own session control block.
**
**  Note that STAR accesses IIDBDB using the default GCA mechanism without
**  specifying a user id.
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              qef_cb			QEF_CB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d1_ddb_p	DD_DDB_DESC
**			.dd_d1_ddb_name		    DDB name
**			.dd_d4_iidbdb_p		    DD_1LDB_INFO
**			    .dd_i1_ldb_desc	    DD_LDB_DESC
**				.dd_l1_ingres_b	    FALSE
**				.dd_l2_node_name    host name or blanks
**				.dd_l3_ldb_name	    "iidbdb"
**				.dd_l4_dbms_name    "INGRES"
**				.dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**				.dd_l6_ldb_id	    DD_UNASSIGNED_IS_0
**		    .qer_d5_timeout	timeout quantum 
** Outputs:
**	    v_qer_p				QEF_RCB
**		qef_cb
**		    .qef_c2_ddb_ses
**			.qes_d4_ddb_p		v_qer_p->qef_r3_ddb_req
**						    .qer_d1_ddb_p
**              qef_r3_ddb_req			QEF_DDB_REQ
**		    .qer_d1_ddb_p		DD_DDB_DESC ptr
**			.dd_d3_cdb_info		DD_1LDB_INFO, CDB info 
**						partially filled in
**			    .dd_i1_ldb_desc	DD_LDB_DESC, CDB identity
**				.dd_l1_ingres_b	TRUE
**				.dd_l2_node_name    node name
**				.dd_l3_ldb_name	    CDB name
**				.dd_l4_dbms_name    ingres
**				.dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**				.dd_l6_ldb_id	    DD_CDB_ID_IS_1 (1)
**				.dd_l7_uniq_id	    system-wide unique id
**			.dd_d2_dba_desc		DD_USR_DESC, DBA info 
**						partially filled in
**			    .dd_u1_usrname	DBA name
**			.dd_d4_access	DD_D_PRIVATE_ACC or DD_D_PUBLIC_ACC
**			.dd_d7_uniq_id	CDB's unique id in iidbdb
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0061_BAD_DB_NAME
**          Returns:
**		E_DB_OK                 
**		E_DB_ERROR              caller error
**		E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	16-nov-89 (carl)
**	    modified to provide the DDB's information to TPF when first 
**	    available
**	06-jun-90 (carl)
**	    query iitables to determine if CDB is 1PC or 2PC
**	24-mar-93 (barbara)
**	    Delimited id support for Star.  Removed query to CDB's iitables
**	    for iidd_ddb_dxlog.  This query was intended to ascertain
**	    whether or not the CDB supports 2PC.  TPF finds out whether or
**	    not a local db (LDB or CDB) supports 2PC at the time it enters
**	    into multi-site transaction.
**      22-Nov-2002 (hanal04) Bug 107159 INGSRV 1696
**          Call qed_u12_map_rqferr() after a failure in the call
**          to qed_u3_rqf_call() in order to prevent ULE_FORMAT errors
**          when qef_call() calls qef_error() with an RQF error which
**          requires parameters. E.g. E_RQ0006.
*/


DB_STATUS
qed_s2_ddbinfo(
QEF_RCB         *v_qer_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_DDB_DESC	    *ddb_p = v_qer_p->qef_r3_ddb_req.qer_d1_ddb_p;
    DD_LDB_DESC	    *iidbdb_p = & ddb_p->dd_d4_iidbdb_p->dd_i1_ldb_desc,
		    *cdb_p = & ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    TPR_CB	    tp_cb,
		    *tpr_p = & tp_cb;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;
    RQB_BIND	    rq_bind[QEK_4_COL_COUNT],	    /* for 4 columns */
		    *bind_p = rq_bind;
    bool	    log_qry_55 = FALSE,
		    log_err_59 = FALSE;
    i4         i4_1, 
		    i4_2;
    char	    ddbname[DB_DB_MAXNAME + 1];
    char	    qrytxt[QEK_200_LEN + DB_DB_MAXNAME];


    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    v_qer_p->error.err_code = E_QE0000_OK;

    /* 1.  set up common information for queries */

    qed_u0_trimtail( ddb_p->dd_d1_ddb_name, DB_DB_MAXNAME, ddbname);

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = iidbdb_p;
    rqr_p->rqr_q_language = DB_SQL;

    /* 3.  set up 1st query: 
    **
    **	select database_owner, access, database_service, database_id from 
    **  iidatabase_info where database_name = '<ddbname>';
    */

    STprintf(qrytxt, "%s %s, %s, %s, %s %s %s %s %s = '%s';",
	    IIQE_c7_sql_tab[SQL_07_SELECT],
	    IIQE_10_col_tab[COL_49_DATABASE_OWNER],
	    IIQE_10_col_tab[COL_10_ACCESS],
	    IIQE_10_col_tab[COL_50_DATABASE_SERVICE],
	    IIQE_10_col_tab[COL_51_DATABASE_ID],
	    IIQE_c7_sql_tab[SQL_02_FROM],
	    IIQE_c9_ldb_cat[TII_24_IIDATABASE_INFO],
	    IIQE_c7_sql_tab[SQL_11_WHERE],
	    IIQE_10_col_tab[COL_48_DATABASE_NAME],
	    ddbname);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
    }

    /* 4.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
	}

        qed_u12_map_rqferr( rqr_p, &v_qer_p->error );    
	return(status);
    }

    /* 5.  set up to fetch result */

    rq_bind[0].rqb_addr = (PTR) ddb_p->dd_d2_dba_desc.dd_u1_usrname;
    rq_bind[0].rqb_length = DB_OWN_MAXNAME;
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
    rq_bind[1].rqb_addr = (PTR) & ddb_p->dd_d5_access;
    rq_bind[1].rqb_length = sizeof(ddb_p->dd_d5_access);
    rq_bind[1].rqb_dt_id = DB_INT_TYPE;
    rq_bind[2].rqb_addr = (PTR) & ddb_p->dd_d6_dbservice;
    rq_bind[2].rqb_length = sizeof(ddb_p->dd_d6_dbservice);
    rq_bind[2].rqb_dt_id = DB_INT_TYPE;
    rq_bind[3].rqb_addr = (PTR) & ddb_p->dd_d7_uniq_id;
    rq_bind[3].rqb_length = sizeof(ddb_p->dd_d7_uniq_id);
    rq_bind[3].rqb_dt_id = DB_INT_TYPE;

    rqr_p->rqr_col_count = QEK_4_COL_COUNT;
    rqr_p->rqr_bind_addrs = rq_bind;

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (rqr_p->rqr_end_of_data)
    {	/* no data! */
	status = qed_u2_set_interr(
		    E_QE0061_BAD_DB_NAME,
		    & v_qer_p->error);
	return(status);
    }

    /* must flush */

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 6.  set up 2nd query: select cdb_node, cdb_name, cdb_dbms,
	   from iistar_cdbs where ddb_name = '<ddbname>'; */

    STprintf(qrytxt, "%s %s, %s, %s %s %s %s %s = '%s';",
	    IIQE_c7_sql_tab[SQL_07_SELECT],
	    IIQE_10_col_tab[COL_06_CDB_NODE],
	    IIQE_10_col_tab[COL_08_CDB_NAME],
	    IIQE_10_col_tab[COL_07_CDB_DBMS],
	    IIQE_c7_sql_tab[SQL_02_FROM],
	    IIQE_c9_ldb_cat[TII_23_STAR_CDBS],
	    IIQE_c7_sql_tab[SQL_11_WHERE],
	    IIQE_10_col_tab[COL_09_DDB_NAME],
	    ddbname);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
    }

    /* 7.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
	}
	return(status);
    }

    /* 8.  set up to fetch result */

    ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc.dd_l1_ingres_b = TRUE;  
					    /* CDB access requires
					    ** ingres status */
    bind_p = rq_bind;	/* MUST first initialize */
    bind_p->rqb_addr = 
	(PTR) ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc.dd_l2_node_name;
    bind_p->rqb_length = DB_NODE_MAXNAME;
    bind_p->rqb_dt_id = DB_CHA_TYPE;
    bind_p++;				/* pt to next bind element */

    MEfill(
	sizeof(ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc.dd_l3_ldb_name),
	' ',
	ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc.dd_l3_ldb_name);
    bind_p->rqb_addr = 
	(PTR) ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc.dd_l3_ldb_name;

    bind_p->rqb_length = DB_DB_MAXNAME;
    bind_p->rqb_dt_id = DB_CHA_TYPE;
    bind_p++;				/* pt to next bind element */

    bind_p->rqb_addr = 
	(PTR) ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc.dd_l4_dbms_name;
    bind_p->rqb_length = DB_TYPE_MAXLEN;
    bind_p->rqb_dt_id = DB_CHA_TYPE;

    rqr_p->rqr_col_count = QEK_3_COL_COUNT;
    rqr_p->rqr_bind_addrs = rq_bind;

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (rqr_p->rqr_end_of_data)
    {	/* no data! */
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    /* must flush */
    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }
    /* 9.  commit read operation to release resources */

    status = qed_u7_msg_commit(v_qer_p, iidbdb_p);
    if (status)
	return(status);
    
    /* 10.  complete CDB information */

    cdb_p->dd_l6_flags = DD_00FLAG_NIL;
    cdb_p->dd_l5_ldb_id = DD_1_IS_CDB_ID;
    
    /* 14.  must record DDB's descriptor ptr in QEF's session control block */

    v_qer_p->qef_cb->qef_c2_ddb_ses.qes_d4_ddb_p = 
	v_qer_p->qef_r3_ddb_req.qer_d1_ddb_p;

    /* 11.  inform TPF of the DDB's information */

    MEfill(sizeof(tp_cb), '\0', ( PTR ) &tp_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;
    tpr_p->tpr_16_ddb_p = v_qer_p->qef_r3_ddb_req.qer_d1_ddb_p;;

    status = qed_u17_tpf_call(TPF_SET_DDBINFO, tpr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    return(E_DB_OK);
}


/*{
** Name: QED_S3_LDBPLUS	- retrieve from the LDB its characteristics & 
**			  capabilities
**
** External QEF call:   status = qef_call(QED_LDBPLUS, &qef_rcb);
**
** Description:
**      This routine retrieves from the LDB its characteristics and capabilities
**  for encoding in the LDB control structure.
**
** Inputs:
**	    v_qer_p			QEF_RCB
**		qef_cb			QEF_CB, session cb
**		    .qef_c2_ddb_ses	QES_DDB_SES, DDB session
**			.qes_d2_tps_p	TPF session ptr
**			.qes_d3_rqs_p	RQF session ptr
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d4_qry_info
**			.qed_q1_timeout	timeout quantum 
**		    .qer_d2_ldb_info_p	DD_LDB_DESC ptr
**			.dd_l1_ingres_b	    TRUE if $ingres access, FALSE 
**					    otherwise
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    LDB name
** Outputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d2_ldb_info_p	DD_LDB_DESC ptr
**			.dd_i2_ldb_plus	DD_1LDB_PLUS
**			    .dd_p1_character	characteristics
**			    .dd_p2_dba_name	DBA name, valid if such notion
**						exists
**			    .dd_p3_ldb_caps	DD_CAPS, LDB's capabilities 
**
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0060_BAD_DB_NAME
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-may-88 (carl)
**          written
**	15-jan-89 (carl)
**	    modified in accordance with the new DD_0LDB_PLUS
**	08-jun-90 (carl)
**	    modified qed_s3_ldbplus to skip initializing field dd_l6_flags
**	    introduced for 2PC
**	18-jun-1990 (georgeg)
**	    modified the select * from iidbcapabilities to 
**	    select user_name, dba_name from iidbconstants
**	10-aug-90 (carl)
**	    replaced structure assignment with MEfill for DD_0LDB_PLUS
**	    initialization
**	20-feb-91 (fpang)
**	    If system_name exists in iidbconstants, fill in dd_p6_sys_name.
*/


DB_STATUS
qed_s3_ldbplus(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC	    *ldb_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    DD_0LDB_PLUS    *plus_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i2_ldb_plus;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;
    bool	    log_qry_55 = FALSE,
		    log_err_59 = FALSE;
    i4         i4_1, 
		    i4_2;
    char	    usrname[DB_OWN_MAXNAME],
		    qrytxt[QEK_200_LEN];
    RQB_BIND	    rq_bind[QEK_2_COL_COUNT];	/* for 2 columns */


    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    v_qer_p->error.err_code = E_QE0000_OK;

    /* 0.  initialize output structure */

    MEfill(sizeof(*plus_p), '\0', (PTR) plus_p);

    /* MUST NOT touch the LDB portion, especially dd_l6_flags which has
    ** been initialized to contian the 1PD/2PC information about the CDB */

    /* 1.  set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = ldb_p;
    rqr_p->rqr_q_language = DB_SQL;

    /* 2.1.  set up query: select user_name, dba_name from iidbconstants; */

    STprintf(qrytxt, "%s user_name, dba_name %s %s;",
	    IIQE_c7_sql_tab[SQL_07_SELECT],
	    IIQE_c7_sql_tab[SQL_02_FROM],
	    IIQE_c9_ldb_cat[TII_03_DBCONSTANTS]);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;
    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
    }

    /* 2.3.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
	}
	return(status);
    }

    /* 2.4.  set up to fetch result */

    rq_bind[0].rqb_addr = (PTR) usrname;
    rq_bind[0].rqb_length = DB_OWN_MAXNAME;
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;

    rq_bind[1].rqb_addr = (PTR) plus_p->dd_p2_dba_name;
    rq_bind[1].rqb_length = DB_OWN_MAXNAME;
    rq_bind[1].rqb_dt_id = DB_CHA_TYPE;

    rqr_p->rqr_col_count = QEK_2_COL_COUNT;
    rqr_p->rqr_bind_addrs = rq_bind;

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (rqr_p->rqr_end_of_data)			/* no data ? */
    {	
	status = qed_u2_set_interr(
			E_QE0061_BAD_DB_NAME,
			& v_qer_p->error);
	return(status);
    }

    /* must flush */

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 2.4 Fetch system_name if it exists in iidbconstants */

    status = qed_s9_system_owner(v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 3.  set up characteristics of LDB in DD_0LDB_PLUS */

    if (! (plus_p->dd_p2_dba_name[0] == '$'))
	plus_p->dd_p1_character |= DD_1CHR_DBA_NAME;

    if (! (usrname[0] == '$'))
	plus_p->dd_p1_character |= DD_2CHR_USR_NAME;

    /* 4.  set up remainder of DD_0LDB_PLUS */

    status = qed_s5_ldbcaps(v_qer_p);
    if (status)
	return(status);

    return(E_DB_OK);
}


/*{
** Name: QED_S4_USRSTAT - get user's DB access status 
**
** External QEF call:   status = qef_call(QED_USRSTAT, &qef_rcb);
**
** Description:
**
**	Given a user, QEF will query IIDBDB in the current node to retrieve
**  his/her DB access status from IIUSER.
**
**  Note that STAR accesses IIDBDB using the default GCA mechanism without
**  specifying a user id.
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              qef_cb			QEF_CB
**		    .qef_c2_ddb_ses	QES_DDB_SES, DDB session cb
**			.qes_d2_tps_p	TPF session ptr
**			.qes_d3_rqs_p	RQF session ptr
**              qef_r3_ddb_req			QEF_DDB_REQ
**		    .qer_d2_ldb_info_p
**			.dd_i1_ldb_desc		descriptor of IIDBDB
**			    .dd_l1_ingres_b	FALSE
**			    .dd_l2_node_name    host name
**			    .dd_l3_ldb_name	"iidbdb"
**			    .dd_l4_dbms_name    "INGRES"
**			    .dd_l5_dbms_code    DD_1DBMS_INGRES (1)
**			    .dd_l6_ldb_id	DD_UNASSIGNED_IS_0
**		    .qer_d4_qry_info
**			.qed_q1_timeout	timeout quantum 
**		    .qer_d8_usr_p		DD_USR_DESC, user descriptor
**			    .dd_u1_usrname	user name
** Outputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req			QEF_DDB_REQ
**		    .qer_d8_usr_p		DD_USR_DESC, user descriptor
**			.dd_u4_status	user's DB status
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0970_BAD_USER_NAME
**          Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
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
qed_s4_usrstat(
QEF_RCB         *v_qer_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC	    *iidbdb_p = 
		    & v_qer_p->qef_r3_ddb_req.qer_d2_ldb_info_p->dd_i1_ldb_desc;
    DD_USR_DESC	    *usr_p = v_qer_p->qef_r3_ddb_req.qer_d8_usr_p;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;
    bool	    log_qry_55 = FALSE,
		    log_err_59 = FALSE;
    i4         i4_1, 
		    i4_2;
    char	    qrytxt[QEK_200_LEN + DB_OWN_MAXNAME];
    char	    usrname[DB_OWN_MAXNAME + 1];
    RQB_BIND	    rq_bind;


    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    v_qer_p->error.err_code = E_QE0000_OK;

    /* 1.  set up for calling RQF */

    qed_u0_trimtail( usr_p->dd_u1_usrname, DB_OWN_MAXNAME, usrname);

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = iidbdb_p;
    rqr_p->rqr_q_language = DB_SQL;

    /* 2.  set up query: 
    **
    **	select internal_status from iiusers where user_name = '<usrname>';
    **	    1	2	3   4	    5	    6	    7
    */

    STprintf(qrytxt, "%s %s %s %s %s %s = '%s';",
	    IIQE_c7_sql_tab[SQL_07_SELECT],
	    IIQE_10_col_tab[COL_53_INTERNAL_STATUS],
	    IIQE_c7_sql_tab[SQL_02_FROM],
	    IIQE_c9_ldb_cat[TII_25_IIUSERS],
	    IIQE_c7_sql_tab[SQL_11_WHERE],
	    IIQE_10_col_tab[COL_52_USER_NAME],
	    usrname);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;
    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
    }

    /* 4.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, iidbdb_p);
	}
	return(status);
    }

    /* 5.  set up to fetch result */

    rq_bind.rqb_addr = (PTR) & usr_p->dd_u4_status;
    rq_bind.rqb_length = sizeof(i4);
    rq_bind.rqb_dt_id = DB_INT_TYPE;

    rqr_p->rqr_col_count = QEK_1_COL_COUNT;
    rqr_p->rqr_bind_addrs = & rq_bind;

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (rqr_p->rqr_end_of_data)
    {	/* no data! */
	status = qed_u2_set_interr(
		    E_QE0970_BAD_USER_NAME,
		    & v_qer_p->error);
	return(E_DB_ERROR);
    }

    /* must flush */

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 6.  commit read operation to release resources */

    status = qed_u7_msg_commit(v_qer_p, iidbdb_p);

    return(status);
}


/*{
** Name: QED_S5_LDBCAPS	- retrieve the LDB's capabilities
**
** Description:
**      This routine retrieves from the LDB the capabilities for encoding 
**  in the LDB control structure.
**
**  Assumption:
**	Input structure DD_CAPS assumed to have been properly initialized.
**
** Inputs:
**	    v_qer_p			QEF_RCB
**		qef_cb			QEF_CB, session cb
**		    .qef_c2_ddb_ses	QES_DDB_SES, DDB session
**			.qes_d2_tps_p	TPF session ptr
**			.qes_d3_rqs_p	RQF session ptr
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d4_qry_info
**			.qed_q1_timeout	timeout quantum 
**		    .qer_d2_ldb_info_p	DD_1LDB_INFO ptr
**			.dd_l1_ingres_b	    TRUE if $ingres access, FALSE 
**					    otherwise
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    LDB name
** Outputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d2_ldb_info_p	DD_LDB_DESC ptr
**			.dd_i2_ldb_plus	DD_1LDB_PLUS
**			    .dd_p3_ldb_caps	DD_CAPS, LDB's capabilities 
**
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-may-88 (carl)
**          written
**      22-jan-89 (carl)
**	    modified to include OWNER_NAME, etc
**      04-may-89 (carl)
**	    changed to commit the LDB to leave a clean local session for 
**	    the user's transaction to allow SET LOCKMODE ON table WHERE 
**	    ... to be accepted without causing the LDB to reject for such
**	    reason as multi-query statement.
**	17-jun-89 (carl)
**	    changed to check for no-transaction state before calling 
**	    qed_u7_msg_commit
**	23-jun-90 (carl)
**	    changed to set in LDB descriptor's dd_l6_flags DD_03FLAG_SLAVE2PC
**	    if the LDB does NOT disclaim the SLAVE 2PC protocol
**	14-jun-91 (teresa)
**	    determine whether or not tids are supported for bug 34684.  Also
**	    fix bug where INGRES capability set to Y regardless of the value
**	    in cap_value.
**	12-nov-92 (fpang)
**	    Added LDB temp table support. If INGRES/SQL_LEVEL of LDB is
**	    greater than or equal to 0605, set DD_04FLAG_TMPTBL so RQF
**	    will use temp table syntax.
**	01-mar-93 (barbara)
**	    Support for delimited ids.  Recognize two new capabilities from
**	    the LDB: OPEN/SQL_LEVEL and DB_DELIMITED_CASE.
**	02-sep-93 (barbara)
**	    Recognize DB_REAL_USER_CASE capability, also for Star support of
**	    delimited ids.
**	28-Jan-2009 (kibro01) b121521
**	    Recognise ingresdate support level.
*/


DB_STATUS
qed_s5_ldbcaps(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC	    *ldb_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    DD_0LDB_PLUS    *plus_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i2_ldb_plus;
    DD_CAPS	    *caps_p = & plus_p->dd_p3_ldb_caps;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;
    bool	    log_qry_55 = FALSE,
		    log_err_59 = FALSE;
    i4         i4_1, 
		    i4_2;
    RQB_BIND	    rq_bind[QEK_2_COL_COUNT];	/* for 2 columns */
    char	    cap_cap[DB_CAP_MAXLEN + 1];
    char	    cap_val[DB_CAPVAL_MAXLEN + 1];
    i4		    tup_cnt;
    i4		    str_code;
    i4		    val_code;
    char	    qrytxt[QEK_200_LEN];


    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    v_qer_p->error.err_code = E_QE0000_OK;

    caps_p->dd_c1_ldb_caps = 0;			/* initialize */
    caps_p->dd_c2_ldb_caps = 0;			/* initialize */
    caps_p->dd_c8_owner_name = DD_2OWN_QUOTED;	/* assume default */

    /* 
    ** assume LDB supports cohort 2PC protocol unless disclaimed explicitly
    ** in the iidbcapabilities catalog 
    */

    caps_p->dd_c1_ldb_caps |= DD_4CAP_SLAVE2PC;
    ldb_p->dd_l6_flags |= DD_03FLAG_SLAVE2PC;	
					    /* must set for TPF */

    /* 0.  never initialize output structure already initialized by caller */

    /* 1.  set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = ldb_p;
    rqr_p->rqr_q_language = DB_SQL;

    /* 2.  set up query:
    ** 
    **	select cap_capability, cap_value from iidbcapabilities;
    **	    1	    2		3	  4	    5
    */

    STprintf(qrytxt, "%s %s, %s %s %s;",
	IIQE_c7_sql_tab[SQL_07_SELECT],
	IIQE_10_col_tab[COL_31_CAP_CAPABILITY],
	IIQE_10_col_tab[COL_34_CAP_VALUE],
	IIQE_c7_sql_tab[SQL_02_FROM],
	IIQE_c9_ldb_cat[TII_02_DBCAPABILITIES]);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;
    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
    }

    /* 4.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
	}
	return(status);
    }


    /* 5.  initialize for loop */

    tup_cnt = 0;

    while (! rqr_p->rqr_end_of_data)
    {
	/* reiniatilize rqr_p for RQR_T_FETCH for safety */

	rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
	rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
	rqr_p->rqr_1_site = ldb_p;
	rqr_p->rqr_q_language = DB_SQL;

	rq_bind[0].rqb_addr = (PTR) cap_cap;
	rq_bind[0].rqb_length = DB_CAP_MAXLEN;
	rq_bind[0].rqb_dt_id = DB_CHA_TYPE;
	
	rq_bind[1].rqb_addr = (PTR) cap_val;
	rq_bind[1].rqb_length = DB_CAPVAL_MAXLEN;
	rq_bind[1].rqb_dt_id = DB_CHA_TYPE;

	/* last is QEK_2_COL_COUNT */

	rqr_p->rqr_col_count = QEK_2_COL_COUNT;
	rqr_p->rqr_bind_addrs = rq_bind;
	rqr_p->rqr_end_of_data = FALSE;	

	status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
	if (status)
	{
	    return(status);
	}

	if (!rqr_p->rqr_end_of_data)
	{   
	    /* process tuple: <capability>,<cap_value> */

	    str_code = qek_c1_str_to_code(cap_cap);
	    switch (str_code)
	    {
	    case QEK_1CAP_COMMONSQL_LEVEL:
		val_code = qek_c2_val_to_code(cap_val);
		caps_p->dd_c3_comsql_lvl = val_code;
		break;
	    case QEK_2CAP_DBMS_TYPE:
		MEcopy(cap_val, DB_TYPE_MAXLEN, caps_p->dd_c7_dbms_type);
		str_code = qek_c1_str_to_code(cap_val);
		break;
	    case QEK_3CAP_DB_NAME_CASE:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_7VAL_UPPER)
		    caps_p->dd_c6_name_case = DD_2CASE_UPPER;
		else if (str_code == QEK_6VAL_MIXED)
		    caps_p->dd_c6_name_case = DD_1CASE_MIXED;
		else if (str_code == QEK_5VAL_LOWER)
		    caps_p->dd_c6_name_case = DD_0CASE_LOWER;
		break;
	    case QEK_4CAP_DISTRIBUTED:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_1VAL_Y)
		    caps_p->dd_c1_ldb_caps |= DD_1CAP_DISTRIBUTED;
		break;
	    case QEK_5CAP_INGRES:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_1VAL_Y)
		    caps_p->dd_c1_ldb_caps |= DD_2CAP_INGRES;
		break;
	    case QEK_6CAP_INGRESQUEL_LEVEL:
		val_code = qek_c2_val_to_code(cap_val);
		caps_p->dd_c5_ingquel_lvl = val_code;
		break;
	    case QEK_7CAP_INGRESSQL_LEVEL:
		val_code = qek_c2_val_to_code(cap_val);
		caps_p->dd_c4_ingsql_lvl = val_code;
		break;
	    case QEK_8CAP_SAVEPOINTS:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_1VAL_Y)
		    caps_p->dd_c1_ldb_caps |= DD_3CAP_SAVEPOINTS;
		break;
	    case QEK_9CAP_SLAVE2PC:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code != QEK_1VAL_Y)
		{
		    /* assume LDB disclaiming cohort 2PC protocol */

		    ldb_p->dd_l6_flags &= ~DD_03FLAG_SLAVE2PC;
					    /* must reset for TPF */
		    caps_p->dd_c1_ldb_caps &= ~DD_4CAP_SLAVE2PC;
		}
		break;
	    case QEK_10CAP_TIDS:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_1VAL_Y)
		    caps_p->dd_c1_ldb_caps |= DD_5CAP_TIDS;
		break;
	    case QEK_11CAP_UNIQUE_KEY_REQ:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_1VAL_Y)
		    caps_p->dd_c1_ldb_caps |= DD_6CAP_UNIQUE_KEY_REQ;
		break;
	    case QEK_12CAP_OWNER_NAME:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_8VAL_NO)
		    caps_p->dd_c8_owner_name = DD_0OWN_NO;
		else if (str_code == QEK_9VAL_YES)
		    caps_p->dd_c8_owner_name = DD_1OWN_YES;
		else if (str_code == QEK_10VAL_QUOTED)
		    caps_p->dd_c8_owner_name = DD_2OWN_QUOTED;
		else
		{
		    status = qed_u1_gen_interr(& v_qer_p->error);
		    return(status);
		}
		break;
	    case QEK_13CAP_PHYSICAL_SOURCE:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_11VAL_P)
		    caps_p->dd_c1_ldb_caps |= DD_7CAP_USE_PHYSICAL_SOURCE;
		break;
	    case QEK_14CAP_TID_LEVEL:
		/* currently, star simply wants to know whether or not tids
		** are supported.  If there is a non "00000" value for 
		** cap_value, then assume tids are supported.  In later
		** releases we may wish to refine this and distinguish between
		** the level of support -- fix bug 34684 */
		val_code = qek_c2_val_to_code(cap_val);
		if (val_code)
		    caps_p->dd_c1_ldb_caps |= DD_5CAP_TIDS;
		break;
	    case QEK_15CAP_OPENSQL_LEVEL:
		val_code = qek_c2_val_to_code(cap_val);
		caps_p->dd_c9_opensql_level = val_code;
		break;
	    case QEK_16CAP_DELIM_NAME_CASE:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_7VAL_UPPER)
		    caps_p->dd_c10_delim_name_case = DD_2CASE_UPPER;
		else if (str_code == QEK_6VAL_MIXED)
		    caps_p->dd_c10_delim_name_case = DD_1CASE_MIXED;
		else if (str_code == QEK_5VAL_LOWER)
		    caps_p->dd_c10_delim_name_case = DD_0CASE_LOWER;
		caps_p->dd_c1_ldb_caps |= DD_8CAP_DELIMITED_IDS;
		break;
	    case QEK_17CAP_REAL_USER_CASE:
		str_code = qek_c1_str_to_code(cap_val);
		if (str_code == QEK_7VAL_UPPER)
		    caps_p->dd_c11_real_user_case = DD_2CASE_UPPER;
		else if (str_code == QEK_6VAL_MIXED)
		    caps_p->dd_c11_real_user_case = DD_1CASE_MIXED;
		else if (str_code == QEK_5VAL_LOWER)
		    caps_p->dd_c11_real_user_case = DD_0CASE_LOWER;
		break;
	    default:
		/* skip unrecognized capability */
		break;
	    } 
	    tup_cnt++;				/* debugging aid */
	}
    }

    /* 7.  must flush */

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if ((caps_p->dd_c1_ldb_caps & DD_2CAP_INGRES)
	&&					
	! (caps_p->dd_c1_ldb_caps & DD_1CAP_DISTRIBUTED))
    {
	/* local INGRES supports savepoints and tids */
	
	caps_p->dd_c1_ldb_caps |= (DD_3CAP_SAVEPOINTS | DD_5CAP_TIDS);
    }

    if (caps_p->dd_c4_ingsql_lvl == QEK_600CAP_LEVEL
	||
	caps_p->dd_c4_ingsql_lvl == QEK_601CAP_LEVEL)
	caps_p->dd_c2_ldb_caps |= DD_0CAP_PRE_602_ING_SQL;
    
    if (caps_p->dd_c4_ingsql_lvl >= QEK_605CAP_LEVEL)
	ldb_p->dd_l6_flags |= DD_04FLAG_TMPTBL;

    if (caps_p->dd_c4_ingsql_lvl >= QEK_910CAP_LEVEL)
	ldb_p->dd_l6_flags |= DD_05FLAG_INGRESDATE;
    
    status = qed_u3_rqf_call(RQR_LDB_ARCH, rqr_p, v_qer_p);
    if (status)
	return(status);

    if (rqr_p->rqr_diff_arch)
	caps_p->dd_c2_ldb_caps |= DD_201CAP_DIFF_ARCH;
    else
	caps_p->dd_c2_ldb_caps &= ~DD_201CAP_DIFF_ARCH;

    /* commit the LDB only if there is no outstanding transaction */

    if (v_qer_p->qef_cb->qef_stat == QEF_NOTRAN)
	status = qed_u7_msg_commit(v_qer_p, ldb_p);

    return(status);
}


/*{
** Name: qed_s6_modify	- inform RQF to modify the characteristics of the 
**			  CDB's association
**
** External QEF call:   status = qef_call(QED_1SCF_MODIFY, &qef_rcb);
**
** Description:
**      (See above comment.)
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d11_modify
** Outputs:
**	    v_qer_p			QEF_RCB
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0060_BAD_DB_NAME
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      04-mar-89 (carl)
**          written
**      04-aug-90 (carl)
**	    maintain new flag QES_05CTL_SYSCAT_USER in DDB session
**	    control subblock to indicate whether "+Y" is ON
*/


DB_STATUS
qed_s6_modify(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;


    v_qer_p->error.err_code = E_QE0000_OK;

    /* set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = (DD_LDB_DESC *) NULL;
    rqr_p->rqr_q_language = DB_NOLANG;
    rqr_p->rqr_mod_flags = & ddr_p->qer_d11_modify;

    status = qed_u3_rqf_call(RQR_MODIFY, rqr_p, v_qer_p);
    if (status == E_DB_OK)
    {
	if (ddr_p->qer_d11_modify.dd_m2_y_flag == DD_1MOD_ON)
	    dds_p->qes_d9_ctl_info |= QES_05CTL_SYSCAT_USER;
	else
	    dds_p->qes_d9_ctl_info &= ~QES_05CTL_SYSCAT_USER;
    }
    return(status);
}


/*{
** Name: QED_S7_TERM_ASSOC - inform RQF to terminate an open association
**
** External QEF call:   status = qef_call(QED_2SCF_TERM_ASSOC, &qef_rcb);
**
** Description:
**      (See above comment.)
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d2_ldb_info_p	DD_1LDB_INFO ptr
** Outputs:
**	    v_qer_p			QEF_RCB
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0060_BAD_DB_NAME
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-mar-89 (carl)
**          written
*/


DB_STATUS
qed_s7_term_assoc(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC	    *ldb_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;


    v_qer_p->error.err_code = E_QE0000_OK;

    /* set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = ldb_p;
    rqr_p->rqr_q_language = DB_NOLANG;
    rqr_p->rqr_mod_flags = (DD_MODIFY *) NULL;

    status = qed_u3_rqf_call(RQR_TERM_ASSOC, rqr_p, v_qer_p);


    return(status);
}


/*{
** Name: QED_S8_CLUSTER_INFO - inform RQF of the cluster node names
**
** External QEF call:   status = qef_call(QED_3SCF_CLUSTER_INFO, &qef_rcb);
**
** Description:
**      (See above comment.)
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d16_anything_p	ptr to the cluster node names
**		    .qer_d17_anything	number of cluster node names
** Outputs:
**	    v_qer_p			QEF_RCB
**              error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE0060_BAD_DB_NAME
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      14-jul-90 (carl)
**          written
*/


DB_STATUS
qed_s8_cluster_info(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;


    v_qer_p->error.err_code = E_QE0000_OK;

    /* set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = NULL;
    rqr_p->rqr_cluster_p = ddr_p->qer_d16_anything_p;
    rqr_p->rqr_nodes = ddr_p->qer_d17_anything;

    status = qed_u3_rqf_call(RQR_CLUSTER_INFO, rqr_p, v_qer_p);

    return(status);
}


/*{
** Name: QED_S9_SYSTEM_OWNER - fetch iidbconstants.system_owner
**
** Description :
**	Fetch iidbconstants.system_owner if it exists.
**
** Assumption :
**      It exists if 'system_owner' is in iicolumns and table_name is iidbconstants.
**
** Inputs :
**	v_qer_p			    QEF_RCB
**	    qef_cb		    QEF_CB, session cb
**		.qef_c2_ddb_ses	    QES_DDB_SES, DDB session
**		    .qes_d2_tps_p   TPF session ptr
**		    .qes_d3_rqs_p   RQF session ptr
**	    qef_r3_ddb_req	    QEF_DDB_REQ
**		.qer_d4_qry_info
**		    .qed_q1_timeout timeout quantum
**		.qer_d2_ldb_info_p  DD_1LDB_INFO ptr
**		    .dd_l1_ingres_b	TRUE if $ingres assess, FALSE
**					otherwise
**		    .dd_l2_node_name	node name	    
**		    .dd_l3_ldb_name	LDB name
** Outputs :
**	v_qer_p			    QEF_RCB
**	    qef_r3_ddb_req	    QEF_DDB_REQ
**		.qer_d2_ldb_info_p  DD_LDB_DESC ptr   
**		    .dd_i2_ldb_plus DD_0LDB_PLUS
**			.dd_p1_character    DD_3CHR_SYS_NAME set if 
**					    system_owner exists.
**			.dd_p6_sys_name	    system_owner
**	    error.err_code	    One of the followng:
**				     E_QE0000_OF
**				     E_QE0002_INTERNAL_ERROR
** Returns :
**	DB_STATUS
** Exceptions :
**	none
**
** History :
**	20-feb-91 (fpang)
**	    Created.
**
*/

DB_STATUS
qed_s9_system_owner(
QEF_RCB		  *v_qer_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC	    *ldb_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    DD_0LDB_PLUS    *plus_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i2_ldb_plus;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;
    bool	    log_qry_55 = FALSE,
		    log_err_59 = FALSE;
    i4         i4_1, 
		    i4_2;
    char	    sysowner[DB_OWN_MAXNAME],
		    qrytxt[QEK_200_LEN]; /* buf for select from catalog */
    RQB_BIND	    rq_bind[QEK_1_COL_COUNT];	/* for 1 columns */


    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    v_qer_p->error.err_code = E_QE0000_OK;

    /* 0.  initialize output structure */

    MEfill(sizeof(plus_p->dd_p6_sys_name), ' ', (PTR) plus_p->dd_p6_sys_name);

    /* 1.  set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = ldb_p;
    rqr_p->rqr_q_language = DB_SQL;

    /* 2.1.  set up query: select column_name from iicolumns
    **				where column_name = 'system_owner' and
    **				      table_name  = 'iidbconstants'
    */

    STprintf(qrytxt, "%s %s %s %s %s %s = 'system_owner' %s %s = '%s' ;",
	    IIQE_c7_sql_tab[SQL_07_SELECT],
	    IIQE_10_col_tab[COL_01_COLUMN_NAME],
	    IIQE_c7_sql_tab[SQL_02_FROM],
	    IIQE_c9_ldb_cat[TII_04_COLUMNS],
	    IIQE_c7_sql_tab[SQL_11_WHERE],
	    IIQE_10_col_tab[COL_01_COLUMN_NAME],
	    IIQE_c7_sql_tab[SQL_00_AND],
	    IIQE_10_col_tab[COL_03_TABLE_NAME],
	    IIQE_c9_ldb_cat[TII_03_DBCONSTANTS]);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
    }

    /* 2.3.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
	}
	return(status);
    }

    /* 2.4.  set up to fetch result */

    rq_bind[0].rqb_addr = (PTR) sysowner;
    rq_bind[0].rqb_length = DB_OWN_MAXNAME;
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;

    rqr_p->rqr_col_count = QEK_1_COL_COUNT;
    rqr_p->rqr_bind_addrs = rq_bind;

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* If end of data, then column does not exist,
    ** Just flush and return;
    */
    if (rqr_p->rqr_end_of_data)			/* no data ? */
    {	
	status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
        if (status)
        {
	    return(status);
        }
	return(E_DB_OK);
    }

    /* must flush */

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 3. System_owner exists, so get it */

    /* 3.1 Set up query : select system_owner from iidbconstants; */

    STprintf(qrytxt, "%s system_owner %s %s;",
	    IIQE_c7_sql_tab[SQL_07_SELECT],
	    IIQE_c7_sql_tab[SQL_02_FROM],
	    IIQE_c9_ldb_cat[TII_03_DBCONSTANTS]);

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
    }

    /* 3.2.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (!log_qry_55))
	{
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, ldb_p);
	}
	return(status);
    }

    /* 3.3.  set up to fetch result */

    rq_bind[0].rqb_addr = (PTR) plus_p->dd_p6_sys_name;
    rq_bind[0].rqb_length = DB_OWN_MAXNAME;
    rq_bind[0].rqb_dt_id = DB_CHA_TYPE;

    rqr_p->rqr_col_count = QEK_1_COL_COUNT;
    rqr_p->rqr_bind_addrs = rq_bind;

    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (rqr_p->rqr_end_of_data)			/* no data ? */
    {	
	status = qed_u2_set_interr(E_QE0061_BAD_DB_NAME, & v_qer_p->error);
	return(status);
    }

    /* must flush */

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 3.4 Mark as filled in. */

    plus_p->dd_p1_character |= DD_3CHR_SYS_NAME;

    return(E_DB_OK);
}
