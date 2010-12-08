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
#include    <scf.h>
#include    <qefmain.h>
#include    <qefscb.h>
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
**  Name: QEDQEF.C - Routines performing DDB operations.
**
**  Description:
**      These routines perform DDB functions. 
**
**          qed_q1_init_ddv	- initialize DDB server
**	    qed_q2_shut_ddv	- shutdown DDB server
**	    qed_q3_beg_ses	- begin a DDB QEF session
**	    qed_q4_end_ses	- end a DDB QEF session
**	    qed_q5_ldb_slave2pc	- retrieve the LDB's 2PC capability
**
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	17-jan-89 (carl)
**	    modified qed_q3_beg_ses to pass along actual and effective
**	    user descriptions
**	30-jan-89 (carl)
**	    modified to use new DB_DISTRIB definition
**	26-feb-89 (carl)
**	    modified qed_q3_beg_ses to pass session characteristics to RQF
**	21-may-89 (carl)
**	    modified qed_q3_beg_ses to inform TPF to set DDL_CONCURRENCY ON
**	27-jun-89 (carl)
**	    1)  modified qed_q3_beg_ses to initialize the session control 
**		block for DDB REPEAT QUERY management
**	    2)  modified qed_q3_end_ses to use the session control block's 
**		DDB REPEAT QUERY management information to reinitialize the 
**		REPEAT QUERY DSHs used by the session
**	06-jun-90 (carl)
**	    modified qeq_q3_beg_ses to pass the DDB descriptor ptr to TPF 
**	23-jun-90 (carl)
**	    added new routine qed_q5_ldb_slave2pc
**      04-aug-90 (carl)
**	    1)  changed qed_q3_beg_ses to maintain new flag 
**	    2)  QES_05CTL_SYSCAT_USER in DDB session control subblock to 
**		indicate whether "+Y" is ON;
**	    3)	changed occurrences of rqf_call to qed_u3_rqf_call where 
**		QEF_RCB is passed in;
**	    4)	changed all occurrences of tpf_call to qed_u17_tpf_call where 
**		QEF_RCB is passed in
**	20-sep-90 (carl)
**	    added trace point processing to qed_q5_ldb_slave2pc
**	08-feb-92 (fpang)
**	    Turned on distributed code by removing ifdef STAR.
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      14-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**	10-oct-93 (tam)
**	    Use 'CAP_SLAVE2PC' instead of 'SLAVE2PC' in querying
**	    iidbcapabilities. (b50446)
**	14-mar-94 (swm)
**	    Bug #59883
**	    Setup new buffer and size elements in RQR_CB and TPR_CB from
**	    corresponding values in QEF_CB in (qed_q3_beg_ses()). This is to
**	    share use of the QEF TRformat buffer rather than rely on
**	    automatic stack storage for buffers which could lead to stack
**	    overrun.
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

/*  Global readonly variables referenced by this file.  */

GLOBALREF QEF_S_CB	*Qef_s_cb;	    /* QEF server control block */


/*{
** Name: QED_Q1_INIT_DDV - Initialize DDB server.
**
**  External QEF call:   status = qef_call(QEC_INITIALIZE, &qef_srcb);
**
** Description:
**      This operation complements initializing QEF for DDB operations. 
**  The DDB portion of the server control block is initialized.  RQF and
**  TPF are called to start up.
**
**  Assumptions: 
**	1)  A memory pool is collected and the environment queue is created. 
**	2)  If this is a warm boot type operation, memory is freed prior to 
**	    execution of the steps. 
**	3)  The server control block has been allocated.
**
**  Called by QEC_INITIALIZE.
**  
** Inputs:
**      qef_srcb
**
** Outputs:
**      qef_srcb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  To be specified
**      Returns:
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
**      29-jul-88 (carl)
**          written
**      29-oct-89 (carl)
**	    changed to pass qef_srcb->qef_sess_max to TPF
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qed_q1_init_ddv(
QEF_SRCB       *qef_srcb )
{
    DB_STATUS   status = E_DB_OK;
    RQR_CB	rq_cb,
		*rq_p = & rq_cb;
    TPR_CB	tp_cb,
		*tp_p = & tp_cb;


    if (! (qef_srcb->qef_s1_distrib & DB_2_DISTRIB_SVR))
    {
	status = qed_u1_gen_interr(& qef_srcb->error);
        return(E_DB_FATAL);
    }

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    status = rqf_call(RQR_STARTUP, rq_p);
    if (status)
    {
        qed_u12_map_rqferr(rq_p, & qef_srcb->error);
        return(status);
    }
    MEfill(sizeof(tp_cb), '\0', (PTR) & tp_cb);
    tp_p->tpr_11_max_sess_cnt = qef_srcb->qef_sess_max;
						/* maximum number of sessions 
						*/
    status = tpf_call(TPF_STARTUP, tp_p);
    if (status)
	qed_u13_map_tpferr(tp_p, & qef_srcb->error);

    return(status);
}


/*{
** Name: QED_Q2_SHUT_DDV - Shutdown DDB server.
**
**  External QEF call:   status = qef_call(QEC_SHUTDOWN, &qef_srcb);
**
** Description:
**	This operation routinely returns E_DB_OK after informing TPF and
**  RQF to shutdown.
**
**  Assumption:
**	Called after the regular shutdown opertions are completed, i.e.
**  all DSH resources released and system resources (memory) returned 
**  to SCF.
**
**  Called by QEC_SHUTDOWN.
**
** Inputs:
**      qef_srcb
**
** Outputs:
**      qef_srcb
**	    .error.err_code
**                                      E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      29-jul-88 (carl)
**          written
*/


DB_STATUS
qed_q2_shut_ddv(
QEF_SRCB       *qef_srcb )
{
    DB_STATUS   status = E_DB_OK;
    RQR_CB	rq_cb,
		*rq_p = & rq_cb;
    TPR_CB	tp_cb,
		*tp_p = & tp_cb;


    if (! (qef_srcb->qef_s1_distrib & DB_2_DISTRIB_SVR))
    {
	status = qed_u2_set_interr(
		    E_QE0002_INTERNAL_ERROR,
		    & qef_srcb->error);
        return(status);
    }

    MEfill(sizeof(tp_cb), '\0', (PTR) & tp_cb);
    status = tpf_call(TPF_SHUTDOWN, tp_p);
    if (status)
    {
	/* log and ignore error */
	qed_u13_map_tpferr(tp_p, & qef_srcb->error);
    }

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    status = rqf_call(RQR_SHUTDOWN, rq_p);
    if (status)
        qed_u12_map_rqferr(rq_p, & qef_srcb->error);

    return(status);
}


/*{
** Name: QED_Q3_BEG_SES - Begin a QEF session.
**
**  External QEF call:   status = qef_call(QEC_BEGIN_SESSION, &qef_rcb);
**
** Description:
**      The DDB subcontrol block within the session control block is 
**  initialized.  RQF and TPF are informed to establish their own 
**  sessions for the client.  RQF's session id is stored in the subcontrol
**  block.  The DDB control information initialized in this routine must be 
**  used on all subsequent calls to QEF that require session information.
**
**  Called by QEC_BEGIN_SESSION.
**
** Inputs:
**	    v_qer_p
**		qef_cb			session control block
**
** Outputs:
**	    v_qer_p
**              qef_cb			session control block
**		    .qef_c2_ddb_ses	DDB session cb initialized
**			.qes_d1_ok_b	TRUE
**			.qes_d2_tps_p	set
**			.qes_d3_rqs_p	set
**			.qes_d6_rdf_state	QED_0ST_NONE
**			.qes_d7_ses_state	QED_0ST_NONE
**			.qes_d8_union
**			    .u2_qry_ses
**				.qes_q2_lang	DB_SQL or DB_QUEL
**              .error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      To be specified
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
**      29-jul-88 (carl)
**          written
**      29-dec-88 (carl)
**          added code to pass language type to RQF and TPF
**	17-jan-89 (carl)
**	    modified to pass along actual and effective user descriptions
**	26-feb-89 (carl)
**	    modified to pass session characteristics to RQF
**	21-may-89 (carl)
**	    modified to inform TPF to set DDL_CONCURRENCY ON
**	27-jun-89 (carl)
**	    modified to initialize the session control block for DDB REPEAT 
**	    QUERY management
**	06-jun-90 (carl)
**	    modified to pass the DDB descriptor ptr to TPF 
**      04-aug-90 (carl)
**	    changed to maintain new flag QES_05CTL_OWNER_IS_INGRES in DDB 
**	    session control subblock to indicate whether "+Y" is ON
**	14-mar-94 (swm)
**	    Bug #59883
**	    Setup new buffer and size elements in RQR_CB and TPR_CB from
**	    corresponding values in QEF_CB in (qed_q3_beg_ses()). This is to
**	    share use of the QEF TRformat buffer rather than rely on
**	    automatic stack storage for buffers which could lead to stack
**	    overrun.
*/


DB_STATUS
qed_q3_beg_ses(
QEF_RCB         *v_qer_p )
{
    DB_STATUS   status;
    RQR_CB	rq_cb,
		*rqr_p = & rq_cb;
    TPR_CB	tp_cb,
		*tpr_p = & tp_cb;
    QEF_DDB_REQ	*ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_DDB_DESC	*ddb_p = ddr_p->qer_d1_ddb_p;
    QES_DDB_SES	*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DB_LANG	lang = v_qer_p->qef_cb->
		    qef_c2_ddb_ses.qes_d8_union.u2_qry_ses.qes_q2_lang;


    /* 1.  initialize the DDB session control subblock */

    MEfill(sizeof(v_qer_p->qef_cb->qef_c2_ddb_ses), '\0',
	(PTR) & v_qer_p->qef_cb->qef_c2_ddb_ses);

    dds_p->qes_d11_handle_cnt = 0;
    dds_p->qes_d12_handle_p = (QES_RPT_HANDLE *) NULL;

    if (ddr_p->qer_d11_modify.dd_m2_y_flag == DD_1MOD_ON)
	dds_p->qes_d9_ctl_info |= QES_05CTL_SYSCAT_USER;
    else
	dds_p->qes_d9_ctl_info &= ~QES_05CTL_SYSCAT_USER;

    /* 2.  request RQF session */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_q_language = lang;
    rqr_p->rqr_user = ddr_p->qer_d8_usr_p;
    rqr_p->rqr_com_flags = ddr_p->qer_d9_com_p;
    rqr_p->rqr_mod_flags = & ddr_p->qer_d11_modify;

    rqr_p->rqr_trfmt = v_qer_p->qef_cb->qef_trfmt;
    rqr_p->rqr_trsize = v_qer_p->qef_cb->qef_trsize;

    status = qed_u3_rqf_call(RQR_S_BEGIN, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }


    dds_p->qes_d3_rqs_p = (PTR) rqr_p->rqr_session;
					/* store RQF session id */

    /* 3.  request TPF session */

    MEfill(sizeof(tp_cb), '\0', (PTR) & tp_cb);
    tpr_p->tpr_rqf = rqr_p->rqr_session;
    tpr_p->tpr_lang_type = lang;
    tpr_p->tpr_16_ddb_p = ddb_p;

    tpr_p->tpr_trfmt = v_qer_p->qef_cb->qef_trfmt;
    tpr_p->tpr_trsize = v_qer_p->qef_cb->qef_trsize;

    status = qed_u17_tpf_call(TPF_INITIALIZE, tpr_p, v_qer_p);
    if (status)
    {
	return(status);
    }
    dds_p->qes_d2_tps_p = (PTR) tpr_p->tpr_session;
					/* store TPF session id */

    /* 4.  OK to use DDB session control (sub)block */

    dds_p->qes_d1_ok_b = TRUE;
    dds_p->qes_d4_ddb_p = v_qer_p->qef_r3_ddb_req.qer_d1_ddb_p;
    dds_p->qes_d6_rdf_state = QES_0ST_NONE;
    dds_p->qes_d7_ses_state = QES_0ST_NONE;
    dds_p->qes_d9_ctl_info |= QES_01CTL_DDL_CONCURRENCY_ON;
						/* default */
    dds_p->qes_d10_timeout = 0;

    status = qed_u17_tpf_call(TPF_1T_DDL_CC, tpr_p, v_qer_p);	/* default */
    if (status)
    {
	return(status);
    }

    return(E_DB_OK);
}


/*{
** Name: QED_Q4_END_SES - End a QEF session.
**
**  External QEF call:   status = qef_call(QEC_END_SESSION, &qef_rcb);
**
** Description:
**	This operation terminates the session after informing TPF 
**  to abort any outstanding transaction and RQF to terminate its
**  notion of this sessin.
**
**  Assumption:
**
**	QEC_END_SESSION will call environment manager to remove any DSHs
**  for this session. 
**
**  Called by QEC_END_SESSION.
**
** Inputs:
**      v_qer_p
**	    qef_cb			session control block
**		.qef_c2_ddb_ses
**		    .qes_d1_ok_b		TRUE
**		    .qes_d2_tps_p		set
**		    .qes_d3_rqs_p		set
** Outputs:
**	v_qer_p
**	    qef_cb			session control block
**		.qef_c2_ddb_ses
**		    .qes_d1_ok_b		FALSE
**	    error.err_code		One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      To be specified.
**      Returns:
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
**      29-jul-88 (carl)
**          written
**	27-jun-89 (carl)
**	    modified to use the session control block's DDB REPEAT QUERY 
**	    management information to reinitialize the REPEAT QUERY DSHs 
**	    used by the session
*/


DB_STATUS
qed_q4_end_ses(
QEF_RCB       *v_qer_p )
{
    DB_STATUS   status;
    DB_STATUS   sav_status = E_DB_OK;
    DB_ERROR	sav_error;
    RQR_CB	rq_cb,
		*rqr_p = & rq_cb;
    TPR_CB	tp_cb,
		*tpr_p = & tp_cb;
    QES_DDB_SES	*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;


    /* 1.  validate DDB session control subblock */

    if (! dds_p->qes_d1_ok_b)
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    if (dds_p->qes_d11_handle_cnt > 0)
    {
	status = qee_d9_undefall(v_qer_p);  /* reinitialize REPEAT QUERY DSHs */
	if (status)
	{
	    /* log and continue */

	    sav_status = status;
	    STRUCT_ASSIGN_MACRO(v_qer_p->error, sav_error);
	}
    }
	
    /* 2.  invalidate DDB session control subblock */

    dds_p->qes_d1_ok_b = FALSE;

    /* 3.  terminate TPF session */

    MEfill(sizeof(tp_cb), '\0', (PTR) & tp_cb);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;
					/* TPF session id */
    status = qed_u17_tpf_call(TPF_TERMINATE, tpr_p, v_qer_p);
    if (status)
    {
	/* log and continue */

	sav_status = status;
	STRUCT_ASSIGN_MACRO(tpr_p->tpr_error, sav_error);
    }

    /* De-reference TRformat buffer from session control block */

    tpr_p->tpr_trfmt = (char *)0;
    tpr_p->tpr_trsize = 0;

    /* 4.  terminate RQF session */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;
					/* RQF session id */
    status = qed_u3_rqf_call(RQR_S_END, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }
    if (sav_status)
    {
	STRUCT_ASSIGN_MACRO(sav_error, v_qer_p->error);
	return(sav_status);
    }

    /* De-reference TRformat buffer from session control block */

    rqr_p->rqr_trfmt = (char *)0;
    rqr_p->rqr_trsize = 0;

    return(E_DB_OK);
}


/*{
** Name: QED_Q5_LDB_SLAVE2PC	- retrieve the LDB's 2PC capability
**
** Description:
**      This routine retrieves from the LDB its support/non-support of 
**	the SLAVE 2PC protocol.
**
** Inputs:
**	v1_ldb_p			ptr to DD_LDB_DESC
**	    .dd_l1_ingres_b		TRUE if $ingres access, FALSE 
**					otherwise
**	    .dd_l2_node_name		node name
**	    .dd_l3_ldb_name		LDB name
**	v2_qer_p				ptr to QEF_RCB
**	    .qef_cb			QEF_CB, session cb
**	    .qef_c2_ddb_ses		QES_DDB_SES, DDB session
**		.qes_d2_tps_p		TPF session ptr
**		.qes_d3_rqs_p		RQF session ptr
**          .qef_r3_ddb_req		QEF_DDB_REQ
** Outputs:
**	v1_ldb_p
**          .dd_l6_flags		DD_03FLAG_SLAVE2PC set if the LDB 
**					supports SLAVE 2PC, unsert otherwise
**
**              .error.err_code         One of the following:
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
**	23-jun-90 (carl)
**	    created
**	20-sep-90 (carl)
**	    added trace point processing
**	10-oct-93 (tam)
**	    Use 'CAP_SLAVE2PC' instead of 'SLAVE2PC' in querying
**	    iidbcapabilities. (b50446)
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
*/


DB_STATUS
qed_q5_ldb_slave2pc(
DD_LDB_DESC	*v1_ldb_p,
QEF_RCB		*v2_qer_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v2_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v2_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v2_qer_p->qef_r3_ddb_req;
    RQR_CB	    rq_cb,
		    *rqr_p = & rq_cb;
    RQB_BIND	    rq_bind[QEK_2_COL_COUNT];	/* for 2 columns */
    char	    cap_cap[DB_CAP_MAXLEN + 1];
    char	    cap_val[DB_CAPVAL_MAXLEN + 1];
    i4		    tup_cnt;
    i4		    str_code;
    bool	    log_qry_55 = FALSE,
                    log_err_59 = FALSE;
    i4         i4_1, i4_2;
    char	    qrytxt[QEK_200_LEN];



    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
        & i4_1, & i4_2))
        log_qry_55 = TRUE;

    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
        & i4_1, & i4_2))
        log_err_59 = TRUE;

    v2_qer_p->error.err_code = E_QE0000_OK;

    /* 
    ** assume LDB supports cohort 2PC protocol unless disclaimed explicitly
    ** in the iidbcapabilities catalog 
    */

    v1_ldb_p->dd_l6_flags |= DD_03FLAG_SLAVE2PC;	
					    /* must set for TPF */

    /* 0.  never initialize output structure already initialized by caller */

    /* 1.  set up for calling RQF */

    MEfill(sizeof(rq_cb), '\0', (PTR) & rq_cb);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = v1_ldb_p;
    rqr_p->rqr_q_language = DB_SQL;

    /* 2.  set up query:
    ** 
    **	select cap_capability, cap_value from iidbcapabilities
    **	    1	    2		3	  4	    5
    **  where cap_capability = 'CAP_SLAVE2PC';
    */

    STprintf(qrytxt, "%s %s, %s %s %s %s;",
	IIQE_c7_sql_tab[SQL_07_SELECT],
	IIQE_10_col_tab[COL_31_CAP_CAPABILITY],
	IIQE_10_col_tab[COL_34_CAP_VALUE],
	IIQE_c7_sql_tab[SQL_02_FROM],
	IIQE_c9_ldb_cat[TII_02_DBCAPABILITIES],
	"where cap_capability = 'CAP_SLAVE2PC'");

    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_qry_55)
    {
	qed_w8_prt_qefqry(v2_qer_p, qrytxt, v1_ldb_p);
    }
    /* 3.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v2_qer_p);
    if (status)
    {
	if (log_err_59 && (! log_qry_55))
	{
	    qed_w8_prt_qefqry(v2_qer_p, qrytxt, v1_ldb_p);
	}
	return(status);
    }

    tup_cnt = 0;

    if (! rqr_p->rqr_end_of_data)
    {
	/* reiniatilize rqr_p for RQR_T_FETCH for safety */

	rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
	rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
	rqr_p->rqr_1_site = v1_ldb_p;
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

	status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v2_qer_p);
	if (status)
	{
	    return(status);
	}

	if (!rqr_p->rqr_end_of_data)
	{   
	    /* process tuple: <capability>,<cap_value> */

	    str_code = qek_c1_str_to_code(cap_val);
	    if (str_code != QEK_1VAL_Y)
	    {
		/* assume LDB disclaiming cohort 2PC protocol */

		v1_ldb_p->dd_l6_flags &= ~DD_03FLAG_SLAVE2PC;
					    /* must reset for TPF */
	    }
	    tup_cnt++;			    /* debugging aid */
	}
    }

    /* 4.  must flush */

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v2_qer_p);
    if (status)
    {
	return(status);
    }

    /* 5.  commit the LDB only if there is no outstanding transaction */

    if (v2_qer_p->qef_cb->qef_stat == QEF_NOTRAN)
	status = qed_u7_msg_commit(v2_qer_p, v1_ldb_p);

    return(status);
}

