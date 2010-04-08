/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpfqry.h>
#include    <tpfproto.h>

/**
**  Name: TPQUPD.C - Utility routines for updating CDB catalogs
**
**  Description:
**	    tpq_u1_update	- update a CDB catalog tuple 
**
**  History:    $Log-for RCS$
**	22-oct-89 (carl)
**          adapted
**	09-oct-90 (carl)
**	    process trace points
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Oct-2001 (jenjo02)
**	    #include tpfproto.h for typing tpd_u2_err_internal macro.
**/


/*{
** Name: tpq_u1_update - Update a CDB catalog tuple
**
** Description:
**	This routine updates a CDB catalog as directed by calling RQF.
**
** Inputs:
**      v_tpr_p			    QEF_RCB
**	v_qcb_p			    ptr to QEC_LINK
**		.qcb_1_can_id	    CDB catalog id
**		.qcb_3_ptr_u.	    ptr to tuple data
**		.qcb_4_ldb_p	    ptr to CDB
**
** Outputs:
**	none
**	
**	v_tpr_p				
**	    .tpr_error
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
**	09-oct-90 (carl)
**	    process trace points
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
tpq_u1_update(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p)
{
    DB_STATUS	    status;
    TPD_SS_CB	    *ss_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    DD_LDB_DESC	    *cdb_p = v_qcb_p->qcb_4_ldb_p;
    i4		    can_id = v_qcb_p->qcb_1_can_id;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    bool            trace_qry_102 = FALSE,
                    trace_err_104 = FALSE;
    i4         i4_1,
                    i4_2;
    char	    qrytxt[1000];


    if (ult_check_macro(& ss_p->tss_3_trace_vector, TSS_TRACE_QUERIES_102,
            & i4_1, & i4_2))
    {
        trace_qry_102 = TRUE;
    }

    if (ult_check_macro(& ss_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    switch (can_id)
    {
    case UPD_10_DXLOG_STATE:
	{
	    TPC_D1_DXLOG    *dxlog_p = v_qcb_p->qcb_3_ptr_u.u1_dxlog_p;


	    /* update iidd_ddb_dxlog set dx_state = i4 where
	    ** dx_id1 = i4 and dx_id2 = i4 */

	    STprintf(
		qrytxt, 
"update iidd_ddb_dxlog set dx_state = %d where dx_id1 = %d and dx_id2 = %d;",
		dxlog_p->d1_5_dx_state,
		dxlog_p->d1_1_dx_id1,
		dxlog_p->d1_2_dx_id2);
	}   
	break;

    default:
	status = tpd_u2_err_internal(v_tpr_p);
	return(status);		
    }

    if (trace_qry_102)
	tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, v_qcb_p->qcb_4_ldb_p, TPD_0TO_FE);

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = v_tpr_p->tpr_rqf;	/* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = cdb_p;	/* CDB */
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    /* 3.  execute CDB query */

    status = tpd_u4_rqf_call(RQR_NORMAL, rqr_p, v_tpr_p);
    if (status)
    {
	if (! trace_qry_102 && trace_err_104)
	    tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, v_qcb_p->qcb_4_ldb_p, 
		TPD_0TO_FE);
    }
    return(status);
}

