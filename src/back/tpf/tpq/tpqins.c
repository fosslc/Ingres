
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
**  Name: TPQINS.C - Utility routines for inserting into CDB catalogs
**
**  Description:
**	    tpq_i1_insert   - insert a tuple into a CDB catalog
**	    tpq_i2_query    - generate query for inserting CDB catalog tuple 
**
**
**  History:    $Log-for RCS$
**	22-oct-89 (carl)
**          adapted
**	07-jul-90 (carl)
**	    changed tpq_i2_query to include new fields d2_8_lxid1, d2_9_lxid2,
**	    d2_10_lxname and d2_11_lxflags for TPC_D2_DXLDBS    
**      09-oct-90 (carl)
**          process trace points
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
** Name: tpq_i1_insert - Insert a tuple into a CDB catalog.
**
** Description:
**	This routine inserts a tuple as directed into the appropriate
**	CDB catalog by calling RQF.  
**
** Inputs:
**      v_tpr_p			    QEF_RCB
**	v_qcb_p			    ptr to TPQ_QRY_CB
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
**	22-oct-89 (carl)
**          adapted
**      09-oct-90 (carl)
**          process trace points
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
tpq_i1_insert(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p)
{
    DB_STATUS	    status;
    TPD_SS_CB	    *ss_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    DD_LDB_DESC	    *cdb_p = v_qcb_p->qcb_4_ldb_p;
    i4		    can_id = v_qcb_p->qcb_1_can_id,
		    rqr_opcode;
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

    /* 1.  set up insert query */

    switch (can_id)
    {
/*
	rqr_opcode = RQR_DATA_VALUES;
	status = tpq_i3_vdata(v_tpr_p, v_qcb_p);
	break;
*/
    default:
	rqr_opcode = RQR_NORMAL;
	status = tpq_i2_query(v_tpr_p, v_qcb_p, qrytxt);
    }
    if (status)
	return(status);

    if (trace_qry_102)
	tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, v_qcb_p->qcb_4_ldb_p, TPD_0TO_FE);

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = v_tpr_p->tpr_rqf;	/* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = cdb_p;
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;
    rqr_p->rqr_dv_cnt = v_qcb_p->qcb_8_dv_cnt;
    rqr_p->rqr_dv_p = v_qcb_p->qcb_9_dv;
    rqr_p->rqr_inv_parms = (PTR) NULL;

    /* 3.  execute CDB query */

    status = tpd_u4_rqf_call(rqr_opcode, rqr_p, v_tpr_p);
    if (status)
    {
	if (! trace_qry_102 && trace_err_104)
	    tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, v_qcb_p->qcb_4_ldb_p, 
		TPD_0TO_FE);
    }
    return(status);
}



/*{
** Name: tpq_i2_query - Generate insert query.
**
** Description:
**	This routine generates a query for insertion from the given information.
**
** Inputs:
**	v_tpr_p			    TPR_CB
**	v_qcb_p			    ptr to TPQ_QRY_CB
**		.qcb_1_can_id	    CDB catalog id
**		.qcb_3_ptr_u	    ptr to tuple data
**
** Outputs:
**	o_qry_p			    query text
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
**	22-oct-89 (carl)
**          adapted
**	07-jul-90 (carl)
**	    changed to include new fields d2_8_lxid1, d2_9_lxid2,
**	    d2_10_lxname and d2_11_lxflags for TPC_D2_DXLDBS    
*/


DB_STATUS
tpq_i2_query(
	TPR_CB		*v_tpr_p,
	TPQ_QRY_CB	*v_qcb_p,
	char            *o_qry_p)
{
    DB_STATUS	status;
    i4		can_id = v_qcb_p->qcb_1_can_id;
    char	seg1[100],
		seg2[100],
		seg3[100];    	    
	    
	    

    switch (can_id)
    {
    case INS_10_DXLOG:
	{
	    TPC_D1_DXLOG    *dxlog_p = v_qcb_p->qcb_3_ptr_u.u1_dxlog_p;


	    /*	insert into iidd_ddb_dxlog values (i4, i4, i4, i4, i4, 
		date, date, i4, i4, 32c, 256c, 32c, i4); */
	
	    dxlog_p->d1_3_dx_name[DB_MAXNAME] = EOS;
	    STtrmwhite(dxlog_p->d1_3_dx_name);

	    dxlog_p->d1_6_dx_create[DD_25_DATE_SIZE] = EOS;
	    STtrmwhite(dxlog_p->d1_6_dx_create);

	    dxlog_p->d1_7_dx_modify[DD_25_DATE_SIZE] = EOS;
	    STtrmwhite(dxlog_p->d1_7_dx_modify);

	    dxlog_p->d1_10_dx_ddb_node[DB_MAXNAME] = EOS;
	    STtrmwhite(dxlog_p->d1_10_dx_ddb_node);

	    dxlog_p->d1_11_dx_ddb_name[DD_256_MAXDBNAME] = EOS;
	    STtrmwhite(dxlog_p->d1_11_dx_ddb_name);

	    dxlog_p->d1_12_dx_ddb_dbms[DB_MAXNAME] = EOS;
	    STtrmwhite(dxlog_p->d1_12_dx_ddb_dbms);

	    STprintf(
		seg1, 
		"(%d, %d, '%s', %d, %d, ",
		dxlog_p->d1_1_dx_id1,	
		dxlog_p->d1_2_dx_id2,	
		dxlog_p->d1_3_dx_name,	
		dxlog_p->d1_4_dx_flags,	
		dxlog_p->d1_5_dx_state);

	    STprintf(
		seg2, 
		"'%s', '%s', %d, %d, ",
		dxlog_p->d1_6_dx_create,	
		dxlog_p->d1_7_dx_modify,
		dxlog_p->d1_8_dx_starid1,	
		dxlog_p->d1_9_dx_starid2);	

	    STprintf(
		seg3, 
		"'%s', '%s', '%s', %d)",
		dxlog_p->d1_10_dx_ddb_node,	
		dxlog_p->d1_11_dx_ddb_name,	
		dxlog_p->d1_12_dx_ddb_dbms,	
		dxlog_p->d1_13_dx_ddb_id);	

	    STprintf(
		o_qry_p, 
		"%s %s %s %s;", 
		"insert into iidd_ddb_dxlog values ",
		seg1, seg2, seg3);
	}
	break;

    case INS_20_DXLDBS:
	{

	    TPC_D2_DXLDBS   *dxldbs_p = v_qcb_p->qcb_3_ptr_u.u2_dxldbs_p;
	    i4	    lxflags = 0;

	    /*	
	    ** insert into iidd_ddb_dxldbs values 
	    ** (i4, i4, c32, c32, c256, i4, i4, i4, i4); 
	    */
	
	    dxldbs_p->d2_3_ldb_node[DB_MAXNAME] = EOS;
	    STtrmwhite(dxldbs_p->d2_3_ldb_node);

	    dxldbs_p->d2_4_ldb_name[DD_256_MAXDBNAME] = EOS;
	    STtrmwhite(dxldbs_p->d2_4_ldb_name);

	    dxldbs_p->d2_5_ldb_dbms[DB_MAXNAME] = EOS;
	    STtrmwhite(dxldbs_p->d2_5_ldb_dbms);

	    dxldbs_p->d2_10_ldb_lxname[DB_MAXNAME] = EOS;
	    STtrmwhite(dxldbs_p->d2_10_ldb_lxname);

	    if (dxldbs_p->d2_11_ldb_lxflags & TPC_LX_02FLAG_2PC)
		lxflags = TPC_LX_02FLAG_2PC;

	    STprintf(
		o_qry_p, 
		"%s (%d, %d, '%s', '%s', '%s', %d, %d, %d, %d, '%s', %d);",
		"insert into iidd_ddb_dxldbs values ",
		dxldbs_p->d2_1_ldb_dxid1,	
		dxldbs_p->d2_2_ldb_dxid2,	
		dxldbs_p->d2_3_ldb_node,	
		dxldbs_p->d2_4_ldb_name,
		dxldbs_p->d2_5_ldb_dbms,	
		dxldbs_p->d2_6_ldb_id,
		dxldbs_p->d2_7_ldb_lxstate,
		dxldbs_p->d2_8_ldb_lxid1,
		dxldbs_p->d2_9_ldb_lxid2,
		dxldbs_p->d2_10_ldb_lxname,
		lxflags);
	}
	break;

    default:
	status = tpd_u2_err_internal(v_tpr_p);
	return(status);
	break;
    }


    return(E_DB_OK);
}
