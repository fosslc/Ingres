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
**  Name: QEDEXE.C - Routines for executing LDB queries
**
**  Description:
**      These routines perform LDB query execution related operations:
**
**	    qed_e1_exec_imm - perform a DIRECT EXECUTE IMMEDIATE query
**	    qed_e4_exec_qry - process an LDB query
**
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	05-aug-90 (carl)
**	    1)  changed qed_e4_exec_qry to 1) process trace point qe55 to log 
**		query information and 2) begin and end a DX for query if 
**		session NOT in a DX 
**	    2)  changed all occurrences of rqf_call to qed_u3_rqf_call;
**	    3)	removed obsolete routine qed_e2_exchange
**	05-oct-90 (carl)
**	    modified qed_e4_exec_qry to also process trace point qe56
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	29-jan-1993 (fpang)
**	    Compare qef_auto against QEF_ON or QEF_OFF instead of 0 or !0.
**      14-sep-93 (smc)
**	    Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/



/*{
** Name: QED_E1_EXEC_IMM - perform a DIRECT EXECUTE IMMEDIATE query
**
**  External QEF call:	    status = qef_call(QED_EXEC_IMM, &qef_rcb);
**
** Description:
**      Input information is presented in the form of QED_LDB_QRYINFO
**	in QSF's storage.  Through the request control block, QEF accesses 
**	the input information either by qef_qso_handle if provided, or by 
**	qef_qp if qef_qso_handle is NULL.  QEF will destroy the inforation 
**	from QSF's strorage when execution is finished.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qef_cb		    ptr to QEF_CB, QEF's session cb, NULL if
**				    it is to be obtained from SCU_INFORMATION
**	    .qef_qp		    name of query plan in QSF storage
**	    .qef_qso_handle	    handle above query plan, NULL if none
**		<QED_LDB_QRYINFO>   structure at above handle
**		    .qed_i1_qry_info	    QED_QUERY_INFO  
**   			.qed_q1_timeout	    timeout quantum, 0 if none 
**			.qed_q2_lang	    DB_QUEL or DB_SQL, DB_NOLANG if 
**					    unknown 
**			.qed_q3_qmode	    DD_READ, or DD_UPDATE, DD_NOMODE
**					    if unknown 
**			.qed_q4_pkt_p	    query information
**		    .qed_i2_ldb_desc	    DD_LDB_DESC
**			.dd_l1_ingres_b	    FALSE
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    ldb name
**			.dd_l4_dbms_name    dbms name
**			.dd_l5_ldb_id	    0 for unknown
**				    
** Outputs:
**      none
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      02-sep-88 (carl)
**          adapted from qeu_dbu
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qed_e1_exec_imm(
QEF_RCB		*v_qer_p )
{
    DB_STATUS		status;
    DB_STATUS		exe_status = E_DB_OK;
    DB_ERROR		exe_error;
    DB_STATUS		qsf_status = E_DB_OK;
    DB_ERROR		qsf_error;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_CONN_SES    	*d8_ses_p = & dds_p->qes_d8_union.u1_con_ses;
    QED_CONN_INFO	*con_p = & d8_ses_p->qes_c2_con_info;
    QSF_RCB		qsf_rcb;
    bool		qsf_locked = FALSE;	/* object claimed from QSF */
    QED_LDB_QRYINFO	*ldbqry_p;
    QEF_RCB		qer_cb;
    DD_1LDB_INFO	ldb_info;


    /* 1.  get query control block from QSF */

    qsf_rcb.qsf_lk_state = QSO_EXLOCK;
    
    status = qed_u4_qsf_acquire(v_qer_p, & qsf_rcb);
    if (status)
    {
	STRUCT_ASSIGN_MACRO(qsf_rcb.qsf_error, v_qer_p->error);
	return(status);
    }

    qsf_locked = TRUE;	    

    /* we have successfully claimed the object */

    ldbqry_p = (QED_LDB_QRYINFO *) qsf_rcb.qsf_root;

    /* 2.  set up to call qed_e4_exec_qry to dispatch query to LDB */

    STRUCT_ASSIGN_MACRO(*v_qer_p, qer_cb);  /* copy contents */
    STRUCT_ASSIGN_MACRO(ldbqry_p->qed_i1_qry_info, ddr_p->qer_d4_qry_info);
    STRUCT_ASSIGN_MACRO(ldbqry_p->qed_i2_ldb_desc, ldb_info.dd_i1_ldb_desc);
    ddr_p->qer_d2_ldb_info_p = & ldb_info;

    status = qed_e4_exec_qry(& qer_cb);
    if (status)
    {
	/* save status and error */

	exe_status = status;
	STRUCT_ASSIGN_MACRO(qer_cb.error, exe_error);
    }

    /* 3.  delete the QSF object */

    if (qsf_locked)
    {
	status = qed_u5_qsf_destroy(& qer_cb, & qsf_rcb);
	if (status)
	{
	    qsf_status = status;
	    STRUCT_ASSIGN_MACRO(qsf_rcb.qsf_error, qsf_error);
	}
    }

    if (exe_status || qsf_status)
    {
	if (exe_status)
	{
	    STRUCT_ASSIGN_MACRO(exe_error, v_qer_p->error);
	    return(exe_status);
	}
	else if (qsf_status)
	{
	    STRUCT_ASSIGN_MACRO(qsf_error, v_qer_p->error);
	    return(qsf_status);
	}
	else
	{
	    return(status);
	}
    }    

    /* 4.  save LDB desc for further QED_EXCHNAGE actions */

    dds_p->qes_d7_ses_state = QES_3ST_EXCHANGE;

    STRUCT_ASSIGN_MACRO(
	ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc,
	con_p->qed_c1_ldb);

    return(status);
}


/*{
** Name: qed_e4_exec_qry - send a non-select query for LDB execution
**
** Description:
**  This routine sends a read-only statement to an LDB for execution.
**
** Inputs:
**	v_qer_p
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qef_cb		    ptr to QEF_CB, QEF's session cb, NULL if
**				    it is to be obtained from SCU_INFORMATION
**	    .qef_r3_ddb_req	    QEF_DDB_REQ
**		.qer_d1_ddb_p	    NULL
**		.qer_d2_ldb_info_p  DD_1LDB_INFO of LDB
**		.qer_d4_qry_info
**		    .qed_q1_timeout	    timeout quantum, 0 if none 
**		    .qed_q2_lang	    DB_QUEL 
**		    .qed_q3_qmode	    DD_1_MODE_READ or DD_2MODE_UPDATE
**		    .qed_q4_pkt_p	    subquery information
** Outputs:
**	    v_qer_p
**              .error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**					E_QE1998_BAD_QMODE
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
**	05-aug-90 (carl)
**	    1)  changed to process trace point qe55 to log query information
**	    2)  added code to begin and end a DX for query if session NOT in 
**		a DX 
**	05-oct-90 (carl)
**	    process trace point qe56; fixed to call qed_u17_tpf_call
*/


DB_STATUS
qed_e4_exec_qry(
QEF_RCB         *v_qer_p )
{
    DB_STATUS		status = E_DB_OK,
			dx_status = E_DB_OK;
    QEF_CB		*qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES		*dds_p = & qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QED_QUERY_INFO	*qry_p = & v_qer_p->qef_r3_ddb_req.qer_d4_qry_info;
    DD_LDB_DESC		*ldb_p = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    i4			tpf_code;
    bool                log_qry_55 = FALSE,
			log_err_56 = FALSE,
			log_err_59 = FALSE,
			start_dx_b = FALSE;
    i4             i4_1, 
			i4_2;
    TPR_CB		tpr,
			*tpr_p = & tpr;
    RQR_CB		rqr,
			*rqr_p = & rqr;


    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
	    & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
	qed_w7_prt_qryinfo(v_qer_p);
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_DDL_56,
	    & i4_1, & i4_2))
    {
        log_err_56 = TRUE;
	qed_w7_prt_qryinfo(v_qer_p);
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
	/* start an SST */

        status = qet_begin(qef_cb);
        if (status)
            return(status);

        start_dx_b = TRUE;
    }

    /* 1.  inform TPF */

    MEfill(sizeof(*tpr_p), '\0', (PTR) tpr_p);

    tpr_p->tpr_session = dds_p->qes_d2_tps_p;
    tpr_p->tpr_site = ldb_p;

    if (qry_p->qed_q3_qmode == DD_1MODE_READ)
	tpf_code = TPF_READ_DML;
    else if (qry_p->qed_q3_qmode == DD_2MODE_UPDATE)
	tpf_code = TPF_WRITE_DML;
    else
    {
        if (log_err_59 && (! log_qry_55) && (! log_err_56))
	    qed_w7_prt_qryinfo(v_qer_p);
	status = qed_u2_set_interr(E_QE1998_BAD_QMODE, & v_qer_p->error);
	goto END_DX;
    }		
	
    status = qed_u17_tpf_call(tpf_code, tpr_p, v_qer_p);
    if (status)
    {
        if (log_err_59 && (! log_qry_55) && (! log_err_56))
	    qed_w7_prt_qryinfo(v_qer_p);
	goto END_DX;
    }

    /* 2.  set up for calling RQF */

    MEfill(sizeof(*rqr_p), '\0', (PTR) rqr_p);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = qry_p->qed_q1_timeout;
    rqr_p->rqr_1_site = ldb_p;
    rqr_p->rqr_q_language = qry_p->qed_q2_lang;
    STRUCT_ASSIGN_MACRO(*qry_p->qed_q4_pkt_p, rqr_p->rqr_msg);

    /* 3.  deliver query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p); 
    if (status)
    {
        if (log_err_59 && (! log_qry_55) && (! log_err_56))
	    qed_w7_prt_qryinfo(v_qer_p);
    }

END_DX:

    if (start_dx_b && qef_cb->qef_auto == QEF_ON)
    {
	dx_status = qet_commit(qef_cb);        /* commit SST started here */
	if (status)
	    status = dx_status;
    }
    return(status);
}

