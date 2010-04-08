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
**  Name: QEDPSF.C - Routines for PSF routines
**
**  Description:
**      These routines provides support for PSF:
**
**	    qed_p1_range    - process an LDB RANGE statement for PSF
**	    qed_p2_set	    - relay a SET statement to RQF
**
**
**  History:    $Log-for RCS$
**	04-mar-89 (carl)
**	    split off from QEDEXE.C
**	28-aug-90 (carl)
**	    changed all occurrences of rqf_call to qed_u3_rqf_call
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/



/*{
** Name: QED_P1_RANGE - send on behalf of PSF a QUEL RANGE statement
**
**  External QEF call:	    status = qef_call(QED_1PSF_RANGE, & qef_rcb);
**
** Description:
**      Input information is presented in the form of QED_LDB_QRYINFO.
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
**	    .qef_r3_ddb_req	    QEF_DDB_REQ
**		.qer_d1_ddb_p	    NULL
**		.qer_d2_ldb_info_p  DD_1LDB_INFO of LDB
**		.qer_d4_qry_info	    QED_QUERY_INFO
**		    .qed_q1_timeout	    timeout quantum, 0 if none 
**		    .qed_q2_lang	    DB_QUEL 
**		    .qed_q3_qmode	    DD_READ
**		    .qed_q4_pkt_p	    subquery information
**				    
** Outputs:
**      none
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**				    E_QE001F_INVALID_REQUEST
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
**      31-oct-88 (carl)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qed_p1_range(
QEF_RCB		*v_qer_p )
{
    DB_STATUS		status;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QED_QUERY_INFO	*qry_p = & ddr_p->qer_d4_qry_info;


    if ( (qry_p->qed_q2_lang != DB_QUEL)
	 ||
	 (qry_p->qed_q3_qmode != DD_1MODE_READ)
       )
    {
	status = qed_u2_set_interr(E_QE001F_INVALID_REQUEST, & v_qer_p->error);
	return(status);
    }

    status = qed_e4_exec_qry(v_qer_p);
    return(status);
}


/*{
** Name: QED_P2_SET - relay a SET statement to RQF
**
**  External QEF call:	    status = qef_call(QED_2PSF_SET, &qef_rcb);
**
** Description:
**	This routine relays a SET statement to RQF.
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
**		.qer_d2_ldb_info_p  NULL
**		.qer_d4_qry_info
**		    .qed_q2_lang    DB_SQL or DB_QUEL
**		    .qed_q4_pkt_p   SET statement text
** Outputs:
**	    v_qer_p
**              .error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**          Returns:
**		E_DB_OK                 
**		E_DB_ERROR              caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	14-feb-89 (carl)
**          written
*/


DB_STATUS
qed_p2_set(
QEF_RCB         *v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QED_QUERY_INFO	*qry_p = & v_qer_p->qef_r3_ddb_req.qer_d4_qry_info;
    RQR_CB		rqr,
			*rqr_p = & rqr;


    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = qry_p->qed_q1_timeout;
    rqr_p->rqr_1_site = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    rqr_p->rqr_q_language = qry_p->qed_q2_lang;
    STRUCT_ASSIGN_MACRO(*qry_p->qed_q4_pkt_p, rqr_p->rqr_msg);

    status = qed_u3_rqf_call(RQR_SET_FUNC, rqr_p, v_qer_p);

    return(status);
}

