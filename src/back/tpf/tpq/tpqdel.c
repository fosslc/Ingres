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
**  Name: TPQDEL.C - Utility routines for deleting from catalogs.
**
**  Description:
**	    tpq_d1_delete   - send a DELETE statement to a CDB
**
**
**  History:    $Log-for RCS$
**	22-oct-89 (carl)
**          adapted
**	03-oct-90 (fpang)
**	    Numerous casting changes for 64 sun4 unix port.
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
** Name: tpq_d1_delete - send a DELETE statement to the CDB 
**
** Description:
**	This routine sends a DELETE statement on the designated catalog
**	to the CDB.
**
** Inputs:
**      v_tpr_p			    TPR_CB
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_1_can_id
**		.qcb_4_ldb_p	    ptr to CDB
** Outputs:
**	None
**
**	v_tpr_p				
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
**          Catalog updated
**
** History:
**	22-oct-89 (carl)
**          adapted
**	09-oct-90 (carl)
**	    process trace points
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
tpq_d1_delete(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p)
{
    DB_STATUS	    status;
    TPD_SS_CB	    *ss_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    DD_LDB_DESC	    *cdb_p = v_qcb_p->qcb_4_ldb_p;
    i4		    del_id = v_qcb_p->qcb_1_can_id;
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
    /* 1.  set up DELETE query */

    switch (del_id)
    {
    case DEL_10_DXLOG_BY_DXID:
	{
	    TPC_D1_DXLOG    *dxlog_p = v_qcb_p->qcb_3_ptr_u.u1_dxlog_p;


	    /*	delete from iidd_ddb_dxlog where dx_id1 = i4 and
	    **  dx_id2 = i4; */

	    STprintf(
		qrytxt, 
		"delete from iidd_ddb_dxlog where dx_id1 = %d and dx_id2 = %d;",
		dxlog_p->d1_1_dx_id1,
		dxlog_p->d1_2_dx_id2);
	}
	break;

    case DEL_11_DXLOG_BY_STARID:
	{
	    TPC_D1_DXLOG    *dxlog_p = v_qcb_p->qcb_3_ptr_u.u1_dxlog_p;


	    /*	delete from iidd_ddb_dxlog where dx_starid1 = i4 and
	    **  dx_starid2 = i4; */

	    STprintf(
		qrytxt, 
	"delete from iidd_ddb_dxlog where dx_starid1 = %d and dx_starid2 = %d;",
		dxlog_p->d1_8_dx_starid1,
		dxlog_p->d1_9_dx_starid2);
	}
	break;

    case DEL_20_DXLDBS_BY_DXID:
	{
	    TPC_D2_DXLDBS    *dxldbs_p = v_qcb_p->qcb_3_ptr_u.u2_dxldbs_p;


	    /*	delete from iidd_ddb_dxldbs where ldb_dxid1 = i4 and
	    **  ldb_dxid2 = i4; */

	    STprintf(
		qrytxt, 
	"delete from iidd_ddb_dxldbs where ldb_dxid1 = %d and ldb_dxid2 = %d;",
		dxldbs_p->d2_1_ldb_dxid1,
		dxldbs_p->d2_2_ldb_dxid2);
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

    rqr_p->rqr_session = (PTR) v_tpr_p->tpr_rqf;	/* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = v_qcb_p->qcb_4_ldb_p;	/* LDB */
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    /* 3.  execute DELETE query */

    status = tpd_u4_rqf_call(RQR_NORMAL, rqr_p, v_tpr_p);
    if (status)
    {
	if (! trace_qry_102 && trace_err_104)
	    tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, v_qcb_p->qcb_4_ldb_p, 
		TPD_0TO_FE);
    }
    return(status);
}

