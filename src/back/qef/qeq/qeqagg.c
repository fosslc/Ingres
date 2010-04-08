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
#include    <rqf.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QEQAGG.C - Support routines for aggregate processing
**
**  Description:
**	    qeq_a1_doagg    - send an aggregate subquery to an LDB
**	    qeq_a2_fetch    - read a tuple from data stream
**	    qeq_a3_flush    - flush the data stream
**
**
**  History:    $Log-for RCS$
**	20-feb-89 (carl)
**          adapted from QELSEL.C
**	30-mar-88 (carl)
**	    fixed lint complaints discovered by UNIX port
**	30-may-89 (carl)
**	    fixed qeq_a1_agg to set correct data reception address for RQF
**	28-aug-90 (carl)
**	    simplified post-processing for qed_u3_rqf_call
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	13-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	06-oct-93 (davebf)
**	    b55503.  clear agg return when null indicator set
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Copy precision details to the RQB_BIND structure in
**          qeq_a1_doagg().
**	24-feb-04 (inkdo01)
**	    Changed dsh_ddb_cb from QEE_DDB_CB instance to ptr.
**/



/*{
** Name: qeq_a1_doagg - send an aggregate subquery to an LDB
**
** Description:
**	This routine sets up and sends an aggregate subquery to an LDB
**  holds the association stream for a tuple fetch.  
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	i_act_p			    QEF_AHD
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
**	20-feb-89 (carl)
**          written
**	23-may-89 (carl)
**	    initialize rqr_p->rqr_dv_cnt to 0 if sub_p->qeq_q8_dv_cnt == 0
**	30-may-89 (carl)
**	    fixed to set correct data reception address for RQF
**	7-jun-89 (seputis)
**	    use proper array offset to send parameters to LDB
**	    
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      05-Dec-2001 (hanal04) Bug 105723, INGSTR 42.
**          Copy precision details to the RQB_BIND structure to
**          support DB_DEC_TYPE to DB_DEC_TYPE conversions in
**          rqf_t_fetch().
*/


DB_STATUS
qeq_a1_doagg(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEF_QP_CB	    *qp_p = dsh_p->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;
    QES_QRY_SES	    *qqs_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEQ_D1_QRY	    *sub_p = & i_act_p->qhd_obj.qhd_d1_qry;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    i4		    n, size;
    DB_DATA_VALUE   *dv_p;
    char	    **pp,
		    *attr_p;
    RQB_BIND	    *bind_p = (RQB_BIND *) qee_p->qee_d9_bind_p;


    /* set up RQB_BIND array for retrieving intermediate result */

    dv_p = qee_p->qee_d7_dv_p;
    dv_p += sub_p->qeq_q11_dv_offset; 
    pp = qee_p->qee_d8_attr_pp;
    pp += sub_p->qeq_q11_dv_offset; 		/* get attribute space ptr */
    attr_p = *pp;

    for (n = 0; n < sub_p->qeq_q6_col_cnt; ++n)
    {
	/* set up column types and sizes for fetching with RQF */

	bind_p->rqb_length = dv_p->db_length;
	bind_p->rqb_dt_id = dv_p->db_datatype;
	bind_p->rqb_prec = dv_p->db_prec;
	bind_p->rqb_addr = (PTR) dv_p->db_data;

	bind_p++;	
	dv_p++;
    }

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = sub_p->qeq_q1_lang;
    rqr_p->rqr_timeout = sub_p->qeq_q2_quantum;
    STRUCT_ASSIGN_MACRO(*sub_p->qeq_q4_seg_p->qeq_s2_pkt_p,
	rqr_p->rqr_msg);
    rqr_p->rqr_1_site = sub_p->qeq_q5_ldb_p;
    rqr_p->rqr_tmp = dsh_p->dsh_ddb_cb->qee_d1_tmp_p;

    if (sub_p->qeq_q8_dv_cnt == 0)
    {
	rqr_p->rqr_dv_cnt = 0;
	rqr_p->rqr_dv_p = (DB_DATA_VALUE *) NULL;
	rqr_p->rqr_inv_parms = (PTR) v_qer_p->qef_param;
    }
    else
    {
	rqr_p->rqr_dv_cnt = sub_p->qeq_q8_dv_cnt;
	rqr_p->rqr_dv_p = qee_p->qee_d7_dv_p + 1;
	rqr_p->rqr_inv_parms = (PTR) NULL;
    }
						
    rqr_p->rqr_qid_first = sub_p->qeq_q12_qid_first;
    status = qed_u3_rqf_call(RQR_DATA_VALUES, rqr_p, v_qer_p);

    return(status);
}


/*{
** Name: qeq_a2_fetch - read tuple from sending LDB
**
** Description:
**      Using the RQR_BIND mecahnism this routine reads a tuple from a 
**  sending LDBMS.
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	i_act_p			    current action structure
**	o_tupread_b		    TRUE if a tuple is read, FALSE
**				    otherwise
**	o_eod_b			    TRUE if end of data stream, FALSE
**				    otherwise
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
**	20-feb-89 (carl)
**          written
*/


DB_STATUS
qeq_a2_fetch(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p,
bool		*o_tupread_p,
bool		*o_eod_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEQ_D1_QRY	    *sub_p = & i_act_p->qhd_obj.qhd_d1_qry;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    RQB_BIND	    *bind;


    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */
    rqr_p->rqr_q_language = sub_p->qeq_q1_lang;
    rqr_p->rqr_timeout = sub_p->qeq_q2_quantum;
    rqr_p->rqr_1_site = sub_p->qeq_q5_ldb_p;
    rqr_p->rqr_col_count = sub_p->qeq_q6_col_cnt;
    rqr_p->rqr_bind_addrs = (RQB_BIND *) qee_p->qee_d9_bind_p;
						/* must use invariant array */
    status = qed_u3_rqf_call(RQR_T_FETCH, rqr_p, v_qer_p);
    if (status)
    {
	/* must flush before returning */

	STRUCT_ASSIGN_MACRO(v_qer_p->error, sav_error);
	ignore = qeq_a3_flush(v_qer_p, sub_p->qeq_q5_ldb_p);
	STRUCT_ASSIGN_MACRO(sav_error, v_qer_p->error);
	return(status);
    }

    bind = rqr_p->rqr_bind_addrs;

    /* fix for b55503  FIXME: more general fix needed */
    if(bind->rqb_r_dt_id < 0 && bind->rqb_addr[bind->rqb_length-1] > 0)
    {
	/* nullable and null byte on */
	MEfill(bind->rqb_length-1, '\0', (PTR) bind->rqb_addr);
    }

    *o_eod_p = rqr_p->rqr_end_of_data;
    *o_tupread_p = ! rqr_p->rqr_end_of_data;

    return(E_DB_OK);
}


/*{
** Name: qeq_a3_flush - flush a sending LDB association
**
** Description:
**      This routine flushes an LDB association.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	i_ldb_p			    LDB ptr
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
**	20-feb-89 (carl)
**          written
*/


DB_STATUS
qeq_a3_flush(
QEF_RCB		*v_qer_p,
DD_LDB_DESC	*i_ldb_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   
    rqr_p->rqr_1_site = i_ldb_p;

    status = qed_u3_rqf_call(RQR_T_FLUSH, rqr_p, v_qer_p);
    
    return(status);
}

