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
#include    <ade.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <tpf.h>
#include    <rqf.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>


/**
**  Name: QEQCSR.C - RQF routines for distributed cursor
**
**  Description:
**      These routines represent a cursor interface to RQF:
**
**	    qeq_c1_close	- cursor close
**	    qeq_c2_delete	- cursor delete
**	    qeq_c3_fetch	- cursor fetch
**	    qeq_c4_open		- cursor open
**	    qeq_c5_replace	- cursor replace
**
**
**  History:    $Log-for RCS$
**	05-feb-89 (carl)
**          written
**	24-jul-90 (carl)
**	    optimized qeq_c3_fetch to skip calling TPF since it's done
**	    at cursor open time
**	30-aug-90 (carl)
**	    changed qeq_c5_replace to suppress transaction abort initiation 
**	    when error occurs
**	23-oct-90 (fpang)
**	    added v_qer_p parameter for calls to qed_u17_tpf_call().
**	22-nov-91 (fpang)
**	    In qeq_c1_close(), issue close cursor to ldb only if it's
**	    been defined.
**	    Fixes B41063.
**	01-oct-1992 (fpang)
**	    In qeq_c3_fetch() and qeq_c5_replace(), set qef_count 
**	    because SCF looks at it now.
**	24-oct-1992 (fpang)
**	    In qeq_c2_delete() and qeq_c5_replace(), don't have to 
**	    inform TPF of write anymore. These functions were formally
**	    called from qef_call(), but are now called from qeq_query().
**	    Qeq_query() already informs TPF of write intentions,
**	    the tpf calls here became redundant.
**	08-dec-1992 (fpang)
**	    In qeq_c2_delete() relay the ldb rowcount.
**      08-dec-92 (anitap)
**          Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-feb-04 (inkdo01)
**	    Changed dsh_ddb_cb from QEE_DDB_CB instance to ptr.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
**/


/*{
** Name: qeq_c1_close - Cursor close
**
** Description:
**	This routine maps the distributed cursor id to its local counterpart
**	which is then used to call RQR_CLOSE to close the local cursor.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_cb
**	i_dsh_p			    ptr to QEE_DSH for query plan
**				    
** Outputs:
**      none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	05-feb-89 (carl)
**          written
**	22-nov-91 (fpang)
**	    Issue close cursor to ldb only if it's been defined.
**	    Fixes B41063.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qeq_c1_close(
QEF_RCB	    *v_qer_p,
QEE_DSH	    *i_dsh_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEE_DDB_CB	    *qee_p = i_dsh_p->dsh_ddb_cb;
/*
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEF_QP_CB	    *qp_p = dsh_p->dsh_qp_ptr;
*/
    QEQ_D1_QRY	    *sub_p = & i_dsh_p->dsh_act_ptr->qhd_obj.qhd_d1_qry;
    bool	    log_qry_55 = FALSE,
                    log_err_59 = FALSE;
    i4         i4_1, i4_2;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
        & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
	qeq_p31_opn_csr(v_qer_p, i_dsh_p);
    }
    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
        & i4_1, & i4_2))
        log_err_59 = TRUE;

    /*
    ** Do actual close only if marked opened. We could be interrupted
    ** between when qef_open_count is incremented, and the actual open
    */

    if (!(qee_p->qee_d3_status & QEE_03Q_DEF))
	return (E_DB_OK);

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = sub_p->qeq_q1_lang;
    rqr_p->rqr_timeout = sub_p->qeq_q2_quantum;
    rqr_p->rqr_1_site = sub_p->qeq_q5_ldb_p;
    rqr_p->rqr_tmp = qee_p->qee_d1_tmp_p;

    STRUCT_ASSIGN_MACRO(qee_p->qee_d5_local_qid, rqr_p->rqr_qid);
    rqr_p->rqr_inv_parms = (PTR) NULL;

    status = qed_u3_rqf_call(RQR_CLOSE, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && !log_qry_55)
	    qeq_p31_opn_csr(v_qer_p, i_dsh_p);
	return(status);
    }	

    qee_p->qee_d3_status &= ~QEE_03Q_DEF;	/* cursor closed */

    return(E_DB_OK);
}


/*{
** Name: qeq_c2_delete - Cursor delete
**
** Description:
**	This routine maps the distributed cursor id to its local counterpart
**	which is then used to call RQF_DELETE to delete the tuple at the
**	local cursor.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to QEE_DSH for query plan
**
** Outputs:
**	none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-feb-89 (carl)
**	    written
**	24-oct-1992 (fpang)
**	    Don't have to inform TPF of write anymore, since we are
**	    now called from qeq_query, which has already done it.
**	08-dec-1992 (fpang)
**	    Return rowcount from local.
**	01-mar-93 (barbara)
**	    Transfer owner name as well as table name to RQF for DELETE
**	    CURSOR.
*/


DB_STATUS 
qeq_c2_delete(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DDB_CB	    *qee_p = i_dsh_p->dsh_ddb_cb;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;	
    bool	    log_qry_55 = FALSE,
                    log_err_59 = FALSE;
    i4         i4_1, i4_2;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
        & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
	qeq_p34_del_csr(v_qer_p, i_dsh_p);
    }
    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
        & i4_1, & i4_2))
        log_err_59 = TRUE;


    /* 2.  set up to call RQR_DELETE */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */
    rqr_p->rqr_tabl_name = ddq_p->qeq_d7_deltable;
    rqr_p->rqr_own_name = ddq_p->qeq_d9_delown;
						/* pass owner/table name for
						** CURSOR DELETE */
    rqr_p->rqr_timeout = QEK_0_TIME_QUANTUM;
    rqr_p->rqr_q_language = qss_p->qes_q2_lang;
    rqr_p->rqr_1_site = qss_p->qes_q3_ldb_p;
    STRUCT_ASSIGN_MACRO(qee_p->qee_d5_local_qid, rqr_p->rqr_qid);

    qss_p->qes_q1_qry_status |= QES_03Q_PHASE2;
						/* enter phase 2 for update */
    status = qed_u3_rqf_call(RQR_DELETE, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && ! log_qry_55)
	    qeq_p34_del_csr(v_qer_p, i_dsh_p);
    }
    else
    {
    	qss_p->qes_q1_qry_status &= ~QES_03Q_PHASE2;
						/* enter phase 2 for update */
	v_qer_p->qef_rowcount = rqr_p->rqr_tupcount;
	v_qer_p->qef_count = rqr_p->rqr_tupcount;
    }
    return(status);
}


/*{
** Name: qeq_c3_fetch - Cursor fetch
**
** Description:
**	This routine maps the distribured cursor id to its local counterpart
**	which is then used to call RQR_FETCH to retrieve and place a tuple in
**	qef_output.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_rowcount	    number of tuples wanted
**	    .qef_output		    address of output buffer with
**				    sufficient space to hold 
**				    specified number of tuples
**	i_dsh_p			    ptr to QEE_DSH for query plan
**
** Outputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_rowcount	    number of tuples returned
**	    .qef_output		    tuples place in buffer
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**				    E_QE0015_NO_MORE_ROWS
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-feb-89 (carl)
**	    written
**	24-jul-90 (carl)
**	    optimized qeq_c3_fetch to skip calling TPF since it's done
**	    at cursor open time
**	01-oct-1992 (fpang)
**	    Set qef_count because SCF looks at it now.
*/


DB_STATUS
qeq_c3_fetch(
QEF_RCB	    *v_qer_p,
QEE_DSH	    *i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DDB_CB	    *qee_p = i_dsh_p->dsh_ddb_cb;
/*
    QEF_QP_CB	    *qph_p = 
		    ((QEE+DSH *)(v_qer_p->qef_cb->qef_dsh))->dsh_qp_ptr;
*/
    bool	    log_qry_55 = FALSE,
                    log_err_59 = FALSE;
    i4         i4_1, i4_2;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
        & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
	qeq_p33_fet_csr(v_qer_p, i_dsh_p);
    }
    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
        & i4_1, & i4_2))
        log_err_59 = TRUE;

    if (! (qee_p->qee_d3_status & QEE_03Q_DEF))
    {
	/* cursor either not opened or closed */

	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    if (qee_p->qee_d3_status & QEE_04Q_EOD)
    {
	/* no more data */

	v_qer_p->qef_rowcount = 0;

	/* must return E_QE0015_NO_MORE_ROWS to signal exhaustion of data */

	v_qer_p->error.err_code = E_QE0015_NO_MORE_ROWS;
	return(E_DB_WARN);
    }

    /* 2.  set up to call RQF to load data */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = qss_p->qes_q2_lang;
    rqr_p->rqr_timeout = QEK_0_TIME_QUANTUM;
    rqr_p->rqr_1_site = & qee_p->qee_d11_ldbdesc;
    STRUCT_ASSIGN_MACRO(qee_p->qee_d5_local_qid, rqr_p->rqr_qid);
    rqr_p->rqr_inv_parms = (PTR) v_qer_p->qef_param;	

    /* 3.  relay SCF parameters */

    rqr_p->rqr_tupcount = v_qer_p->qef_rowcount;
    rqr_p->rqr_tupdata = (PTR) v_qer_p->qef_output;
    rqr_p->rqr_tupdesc_p = (PTR) NULL;

    status = qed_u3_rqf_call(RQR_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && !log_qry_55)
	    qeq_p33_fet_csr(v_qer_p, i_dsh_p);

	/* it is assumed that RQF has repaired itself before returning */

	dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* reset */
	STRUCT_ASSIGN_MACRO(rqr_p->rqr_error, v_qer_p->error);
	return(status);
    }

    if (rqr_p->rqr_end_of_data)
	qee_p->qee_d3_status |= QEE_04Q_EOD;	/* must set */

    /* return data gotten so far */

    v_qer_p->qef_rowcount = rqr_p->rqr_tupcount;
    v_qer_p->qef_count = rqr_p->rqr_count;

    if ((v_qer_p->qef_rowcount == 0) || (v_qer_p->qef_count == 0))
    {
	/* no more data */

	qee_p->qee_d3_status |= QEE_04Q_EOD;	/* must set */

	/* must return E_QE0015_NO_MORE_ROWS to signal no more data for 
	** subquery */

	v_qer_p->error.err_code = E_QE0015_NO_MORE_ROWS;
	return(E_DB_WARN);
    }
    else if ((v_qer_p->qef_rowcount == 0)
	     && (v_qer_p->qef_count != 0))
    {
	/* Here we emulate an incomplete condition */

	v_qer_p->error.err_code = E_AD0002_INCOMPLETE;
	return(E_DB_INFO);
    }

    return(E_DB_OK);
}


/*{
** Name: qeq_c4_open - Cursor open
**
** Description:
**	This routine calls RQF_DEFINE to open a local cursor, thus establishing
**	a distributed to local cursor mapping.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	v_dsh_p			    ptr to QEE_DSH
**
** Outputs:
**	None
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-feb-89 (carl)
**	    written
*/


DB_STATUS
qeq_c4_open(
QEF_RCB		*v_qer_p,
QEE_DSH		*v_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QEF_AHD	    *act_p = (QEF_AHD *) NULL,
		    *def_act_p = (QEF_AHD *) NULL;
    i4		    def_act_cnt = 0,
		    get_act_cnt = 0,
		    lnk_act_cnt = 0;
    QEQ_D1_QRY	    *sub_p = (QEQ_D1_QRY *) NULL;
    bool	    read_b = FALSE,
		    last_b = FALSE,
		    log_qry_55 = FALSE,
                    log_err_59 = FALSE;
    i4         i4_1, i4_2;


    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
        & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
        qeq_p31_opn_csr(v_qer_p, v_dsh_p);;
    }
    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
        & i4_1, & i4_2))
        log_err_59 = TRUE;

    /* verify that the following conditions are true of the query plan:
    **
    **  1)  there is exactly one last QEA_D7_OPN action,
    **  2)  there are no QEA_D2_GET, QEA_D4_LNK actions,
    **  3)  there are no read actions  */

    act_p = v_dsh_p->dsh_qp_ptr->qp_ahd; 

    while (act_p != (QEF_AHD *) NULL)
    {
	if (act_p->ahd_atype == QEA_D7_OPN)
	{
	    def_act_cnt++;
	    if (act_p->ahd_next == (QEF_AHD *) NULL)
		last_b = TRUE;
	}
	else if (act_p->ahd_atype == QEA_D2_GET)
	    get_act_cnt++;
	else if (act_p->ahd_atype == QEA_D4_LNK)
	    lnk_act_cnt++;
	else if (act_p->ahd_atype == QEA_D1_QRY)
	{
	    sub_p = & act_p->qhd_obj.qhd_d1_qry;
	    if (sub_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
		read_b = TRUE;
	}
	act_p = act_p->ahd_next;
    }
	
    if (! last_b || def_act_cnt != 1 || get_act_cnt != 0 || 
	lnk_act_cnt != 0 || read_b)
    {
	if (log_err_59 && !log_qry_55)
	    qeq_p31_opn_csr(v_qer_p, v_dsh_p);;

	status = qed_u2_set_interr(E_QE1999_BAD_QP, & v_qer_p->error);
	return(status);
    }

    qee_d1_qid(v_dsh_p);			/* generate query id for
						** sending to LDB */
    /* process each action in the QP */

    for (act_p = v_dsh_p->dsh_qp_ptr->qp_ahd; 
	 act_p != (QEF_AHD *)NULL; 
	 act_p = act_p->ahd_next)
    {
	/* check for deferred cursor, only one at a time is allowed */
	if (act_p->ahd_flags & QEA_DEF)
	{
	    v_dsh_p->dsh_qp_status |= DSH_DEFERRED_CURSOR;
	    if (qef_cb->qef_defr_curs)
	    {
		qed_u10_trap();
		v_qer_p->error.err_code = E_QE0026_DEFERRED_EXISTS;
		return(E_DB_ERROR);
	    }
	}

	switch (act_p->ahd_atype)
	{
	case QEA_D1_QRY:
	case QEA_D8_CRE:
	case QEA_D9_UPD:
	    status = qeq_d1_qry_act(v_qer_p, & act_p);
	    if (status)
		qed_u10_trap();
	    break;
	case QEA_D3_XFR:
	    status = qeq_d5_xfr_act(v_qer_p, act_p);
	    if (status)
		qed_u10_trap();
	    break;
	case QEA_D6_AGG:
	    status = qeq_d10_agg_act(v_qer_p, act_p);
	    if (status)
		qed_u10_trap();
	    break;
	case QEA_D7_OPN:
	    def_act_p = act_p;		/* keep track */

	    status = qeq_d8_def_act(v_qer_p, act_p);
	    if (status)
		qed_u10_trap();
	    break;
	default:
	    status = qed_u1_gen_interr(& v_qer_p->error);
	    qed_u10_trap();
	    break;
	}
	if (! status)
	    v_dsh_p->dsh_act_ptr = act_p;	/* promote to current dsh */
    }

    /* cleanup done by caller */

    v_dsh_p->dsh_act_ptr = def_act_p;    /* pt to define action */
    return (status);
}


/*{
** Name: qeq_c5_replace - Cursor replace
**
** Description:
**	This routine maps the distributed cursor id to its local counterpart
**	which is then used to call RQF_REPLACE to replace the tuple at the
**	local cursor.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to QEE_DSH for query plan
**
** Outputs:
**	None
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	05-feb-89 (carl)
**	    written
**	17-apr-90 (georgeg)
**	    get data values from ddq_p->qeq_d6_fixed_data instead of 
**	    uee_p->qee_d7_dv_p, reasoning: that's where opc puts them!!
**	30-aug-90 (carl)
**	    suppress transaction abort initiation when error occurs
**	01-oct-1992 (fpang)
**	    Set qef_count because SCF looks at it now.
**	24-oct-1992 (fpang)
**	    Don't have to inform TPF of write anymore, since we are
**	    now called from qeq_query, which has already done it.
*/


DB_STATUS
qeq_c5_replace(
QEF_RCB	    *v_qer_p,
QEE_DSH	    *i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEE_DSH	    *ree_p = i_dsh_p->dsh_aqp_dsh;  
						/* must use update dsh for 
						** cursor */
    QEE_DDB_CB	    *uee_p = ree_p->dsh_ddb_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEQ_DDQ_CB	    *ddq_p = & i_dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEF_AHD	    *act_p = i_dsh_p->dsh_qp_ptr->qp_ahd;
    QEQ_D1_QRY	    *sub_p = & act_p->qhd_obj.qhd_d1_qry;
    bool	    log_qry_55 = FALSE,
                    log_err_59 = FALSE;
    i4         i4_1, i4_2;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
        & i4_1, & i4_2))
    {
	log_qry_55 = TRUE;
	qeq_p35_upd_csr(v_qer_p, i_dsh_p);
    }

    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
        & i4_1, & i4_2))
        log_err_59 = TRUE;


    /* 2.  then set up to call RQF to replace */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = qss_p->qes_q2_lang; 
    rqr_p->rqr_timeout = sub_p->qeq_q2_quantum;
    STRUCT_ASSIGN_MACRO(*sub_p->qeq_q4_seg_p->qeq_s2_pkt_p, rqr_p->rqr_msg);
    rqr_p->rqr_1_site = sub_p->qeq_q5_ldb_p;
    if (sub_p->qeq_q8_dv_cnt == 0)
    {
	rqr_p->rqr_dv_cnt = v_qer_p->qef_pcount;
	rqr_p->rqr_dv_p = (DB_DATA_VALUE *) v_qer_p->qef_param;
						/* pass array base */
    }
    else
    {

	rqr_p->rqr_dv_cnt = ddq_p->qeq_d4_total_cnt;
	rqr_p->rqr_dv_p = ddq_p->qeq_d6_fixed_data;
/*	rqr_p->rqr_dv_p = uee_p->qee_d7_dv_p + 1;*/
						/* 1-based, make 0-based */
    }
    rqr_p->rqr_qid_first = sub_p->qeq_q12_qid_first;
    STRUCT_ASSIGN_MACRO(uee_p->qee_d5_local_qid, rqr_p->rqr_qid);
    rqr_p->rqr_tmp = uee_p->qee_d1_tmp_p;

    qss_p->qes_q1_qry_status |= QES_03Q_PHASE2;
						/* enter phase 2 for update */
    status = qed_u3_rqf_call(RQR_UPDATE, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && !log_qry_55)
	    qeq_p35_upd_csr(v_qer_p, i_dsh_p);
	return(status);
    }	
    else
	qss_p->qes_q1_qry_status &= ~QES_03Q_PHASE2;

    v_qer_p->qef_rowcount = rqr_p->rqr_tupcount;
    v_qer_p->qef_count = rqr_p->rqr_tupcount;
						/* return update row count */
    return(E_DB_OK);
}

