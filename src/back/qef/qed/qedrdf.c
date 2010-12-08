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
**
**  Name: QEDRDF.C - STAR catalog aceess routines for RDF.
**
**  Description:
**      The routines defined in this file provide STAR catalog access services 
**  to RDF.
**
**	 qed_r1_query	  - execute a query on an LDB
**	 qed_r2_fetch	  - retrieve a tuple from a sending LDB
**	 qed_r3_flush	  - flush unread tuples from a sending LDB
**	 qed_r4_vdata	  - execute a query with data values on an LDB
**	 qed_r5_begin	  - inform TPF prior to execution of a series of read 
**			    or update queries on an LDB
**	 qed_r6_commit	  - send COMMIT message across the CDB association for
**			    RDF
**	 qed_r7_tp_rq	  - inform TPF before delivering query to LDB thru RQF
**	 qed_r8_diff_arch - Request LDB's architecture likeness from RQF
**
**
**  History:    $Log-for RCS$
**      25-jul-88 (carl)    
**          written
**      23-mar-89 (carl)    
**	    added qed_r6_commit
**      10-apr-89 (carl)    
**	    modified to omit calling TPF for read queries
**      20-may-89 (carl)
**	    changed qed_r7_tp_rq to directly call qed_u3_rqf_call to 
**	    include RDF's CDB traffic in DDL_CONCURRENCY management
**	    to avoid such traffic from deadlocking other sessions
**      10-jun-89 (carl)
**	    fixed qed_r8_diff_arch to map RQF to QEF error
**	    changed qed_r6_commit to call qed_u11_ddl_commit
**      22-aug-89 (carl)
**	    fixed qed_r7_tp_rq to inform RQF that attribute names are not 
**	    wanted for RDF's internal queries
**      08-oct-89 (carl)
**	    modified qed_r7_tp_rq to commit the CDB association for RDF to
**	    avoid deadlocks if DDL_CONCURRECY is ON
**	28-aug-90 (carl)
**	    changed all occurrences of rqf_call to qed_u3_rqf_call;
**	    changed all occurrences of tpf_call to qed_u17_tpf_call;
**	19-jul-91 (davebf)
**	    when not privileged assoc. and read-only and no QEF transaction,
**	    start QEF trans and inform TPF.  (b37829)
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	28-jan-1993 (fpang)
**	    In qed_r2_fetch(), and qed_r3_flush(), if metadata transaction was 
**	    started, and we are in autocommit, commit the internal transaction.
**	13-mar-1993 (fpang)
**	    In qed_r7_tp_rq(), call qed_u14_pre_commit() instead of 
**	    qed_u11_ddl_commit(). The former ignores autocommit state.
**	    Fixes B44827, mulitple select from views cause cdb deadlock.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**      07-Dec-94 (liibi01)
**          Cross integration from 6.4 (Bug 64573).
**          In qed_r1_query, we weren't initialising the TPR_CB - this
**          caused problems further down the line on some platforms.
**          Fixes 64573
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      19-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**          Internal RDF queries may cause QEF to qet_begin() an MST
**          whilst parsing a query. If the first statement in the user's 
**          TX is a set lockmode it will fail unless we commit the 
**          internal RDF query.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/



/*{
** Name: QED_R1_QUERY - send a query to an LDB for execution
**
** External QEF call:	status = qef_call(QED_1RDF_QUERY, & qef_rcb);
**
** Description:
**      This routine delivers a query to an LDB for execution.  If defaulting
**  to the CDB, then execution assumes the special $ingres status.
**
**  Note that this operation will label the RDF state as READ if the query mode
**  is READ.  RDF can only emerge from this state if a QED_3RDF_FLUSH is 
**  requested or a subsequent QED_2RDF_FETCH encounters end of data.
**
** Inputs:
**      v_qer_p				
**          .qef_cb		    internal QEF info
**		.qef_c2_ddb_ses	    
**		    .qes_d6_rdf_state  (QES_0ST_NONE)
**	    .qef_r2_ddb_req	    QEF_DDB_REQ 
**		.qer_d2_ldb_info_p  target LDB, NULL ==> accessing CDB as 
**				    $ingres
**		.qer_d4_qry_info
**		    .qed_q1_timeout timeout quantum, 0 if none
**		    .qed_q2_lang    DB_SQL or DB_QUEL
**		    .qed_q3_qmode   DD_READ or DD_UPDATE
**		    .qed_d4_packet  query text information
**		    .qed_q5_dv_cnt  0
**		    .qed_q6_dv_p    NULL
**
** Outputs:
**      v_qer_p
**          .qef_cb		    current QEF session cb
**		.qef_c2_ddb_ses	    current DDB session cb
**		    .qes_d6_rdf_state  QES_1ST_READ if 
**					qer_d4_qry_info.qed_q3_qmode is DD_READ
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE001F_INVALID_REQUEST
**				    (others to be specified)
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
**      25-jul-88 (carl)
**          written
**	19-jul-91 (davebf)
**	    when not privileged assoc. and read-only and no QEF transaction,
**	    start QEF trans and inform TPF.  (b37829)
**	27-jan-1993 (fpang)
**	    Don't start a  transaction if query is to special iidbdb association.
**	    Also, check for if autocommit is off with (qef_auto == QEF_OFF), 
**	    instead of (! autocommit).
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      07-Dec-94 (liibi01)
**          Cross integration from 6.4. (Bug 64573).
**          We weren't initialising the TPR_CB tpr - this
**          caused problems further down the line on some platforms.
**      19-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**          If we are starting a QEF_RDF_INTERNAL query and we
**          start an MST update the rdf_qefcb with QEF_RDF_MSTRAN.
*/


DB_STATUS
qed_r1_query(
QEF_RCB       *v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QEF_CB		*qcb_p = v_qer_p->qef_cb;
    DD_LDB_DESC		*which_p;
    TPR_CB		tpr,
			*tp_p = & tpr;
    bool		is_iidbdb = FALSE;


    /* 1.  verify RDF's state */

    if (
	(dds_p->qes_d6_rdf_state != QES_0ST_NONE)
	||
	(ddr_p->qer_d4_qry_info.qed_q5_dv_cnt > 0)
	)
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(status);
    }

    if (ddr_p->qer_d2_ldb_info_p == NULL)	/* defaulting to the CDB */
	which_p = & dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    else					/* use LDB id */
	which_p = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;

    if  ((!MEcmp(which_p->dd_l2_node_name, DB_ABSENT_NAME, DB_NODE_MAXNAME))
      && (!MEcmp(which_p->dd_l3_ldb_name,  DB_DBDB_NAME,   DB_DB_MAXNAME))
      && (!MEcmp(which_p->dd_l4_dbms_name, DB_ABSENT_NAME, DB_TYPE_MAXLEN)))
	is_iidbdb = TRUE;

    if(!which_p->dd_l1_ingres_b		    /* if not privileged assoc */
	&& is_iidbdb == FALSE		    /* not iidbdb */
	&& qcb_p->qef_stat == QEF_NOTRAN    /* no transaction */
	&& ddr_p->qer_d4_qry_info.qed_q3_qmode == DD_1MODE_READ)  /* r-only */
    {
	v_qer_p->qef_modifier = QEF_SSTRAN;
	status = qet_begin(qcb_p);	/* begin qef xn */
	if (status) return(status);

	if (qcb_p->qef_auto == QEF_OFF)
	{
	    if(v_qer_p->qef_stmt_type & QEF_RDF_INTERNAL)
            {
		/* We were QEF_NOTRAN and the QEF_RDF_INTERNAL query
		** means we are now QEF_RDF_MSTRAN. 
		*/
                v_qer_p->qef_stmt_type |= QEF_RDF_MSTRAN;
	    }
	    qcb_p->qef_stat = QEF_MSTRAN;	    /* escalate to MST */
        }

	/* inform tpf for read  */
        MEfill(sizeof(tpr), '\0', (PTR) & tpr);
	tp_p->tpr_session = dds_p->qes_d2_tps_p;
	tp_p->tpr_site = which_p;
	status = qed_u17_tpf_call(TPF_READ_DML, tp_p, v_qer_p);
	if(status) return(status);
    }

    status = qed_r7_tp_rq(v_qer_p);

    return(status);
}


/*{
** Name: QED_R2_FETCH - read the next tuple from a sending LDB 
**
** External QEF call:   status = qef_call(QED_2RDF_FETCH, & qef_rcb);
**
** Description:
**      This routine reads the next tuple from a sending LDB.
**  
**  Note that RDF must be in a READ state, i.e., in the receiving state as
**  a result of executing a READ query on the specified LDB.
**
** Inputs:
**      v_qer_p				
**          .qef_cb		    internal QEF info
**		.qef_c2_ddb_ses	    
**		    .qes_d6_rdf_state  (QES_1ST_READ)
**	    .qef_r2_ddb_req	    QEF_DDB_REQ 
**		.qer_d2_ldb_info_p  target LDB, NULL ==> accessing CDB as 
**				    $ingres
**		.qer_d4_qry_info
**		    .qed_q1_timeout timeout quantum, 0 if none
**		.qer_d5_out_info    tuple desc for receiving tuple
**		    .qed_o1_col_cnt number of columns in tuple <= 
**				    QER_50_COL_COUNT
**		    .qed_o2_bind_p  binding structure for tuple columns
** Outputs:
**      v_qer_p
**          .qef_cb		    current QEF session cb
**		.qef_c2_ddb_ses	    current DDB session cb
**		    .qes_d6_rdf_state  QES_0ST_NONE if already end of data
**	    .qef_r2_ddb_req	    QEF_DDB_REQ 
**		.qer_d5_out_info    output information
**		    .qed_o3_out_b   TRUE if there is output, FALSE if already
**				    end of data
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE001F_INVALID_REQUEST
**				    (others to be specified)
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
**	28-jan-1993 (fpang)
**	    If metadata transaction was started, and we are in autocommit, and
**	    there are no more rows, commit the internal transaction.
**	10-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Change the "," at the end of the second statement of step 3 to a
**	    ";" so that the dbx debugger will not get confused and skip over
**	    the line.
**	07-jan-02 (toumi01)
**	    Replace DD_300_MAXCOLUMN with DD_MAXCOLUMN (1024).
**      19-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**          When we hit eof we should commit the TX if we are
**          QEF_RDF_MSTRAN (A TX initiated by QEF_RDF_INTERNAL
**          operations executed by RDF) and we are not in
**          a distributed transaction.
*/


DB_STATUS
qed_r2_fetch(
QEF_RCB		*v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    RQR_CB		rqr,
			*rq_p = & rqr;


    /* 0. verify bind array dimension */

    if (ddr_p->qer_d5_out_info.qed_o1_col_cnt > DD_MAXCOLUMN)
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(E_DB_ERROR);
    }

    /* 1. verify RDF's state */

    if (dds_p->qes_d6_rdf_state != QES_1ST_READ)
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(E_DB_ERROR);
    }

    /* 2.  set up to call RQF to deliver next tuple */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rq_p->rqr_session = dds_p->qes_d3_rqs_p;   
    rq_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    if (ddr_p->qer_d2_ldb_info_p == NULL)   /* defaulting to the CDB */
	rq_p->rqr_1_site = & dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    else				    /* use LDB id */
	rq_p->rqr_1_site = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;

    /* 3.  set up bind structure for RQF */

    rq_p->rqr_col_count = ddr_p->qer_d5_out_info.qed_o1_col_cnt;
    rq_p->rqr_bind_addrs = (RQB_BIND *) ddr_p->qer_d5_out_info.qed_o2_bind_p;

    /* 4.  request 1 tuple */

    status = qed_u3_rqf_call(RQR_T_FETCH, rq_p, v_qer_p);
    if (status)
    {
	/* return RDF's state to normal */

	dds_p->qes_d6_rdf_state = QES_0ST_NONE; 
	return(status);
    }

    /* 5.  set result indicator */

    if (rq_p->rqr_end_of_data)
    {
	ddr_p->qer_d5_out_info.qed_o3_output_b = FALSE;
	dds_p->qes_d6_rdf_state = QES_0ST_NONE; 

	/* If autocommit and transaction was started, commit it.  */
	if (( v_qer_p->qef_cb->qef_auto == QEF_ON 
            && v_qer_p->qef_cb->qef_stat != QEF_NOTRAN 
	    && !rq_p->rqr_1_site->dd_l1_ingres_b )
          ||
            ((v_qer_p->qef_stmt_type & QEF_RDF_MSTRAN)
            &&
	    (dds_p->qes_d7_ses_state == QES_0ST_NONE)))
        {
	    v_qer_p->qef_stmt_type &= (~(QEF_RDF_MSTRAN));
	    status = qet_commit(v_qer_p->qef_cb);
	    if (status)
		return (status);
	}
    }
    else
	ddr_p->qer_d5_out_info.qed_o3_output_b = TRUE;

    return(E_DB_OK);
}


/*{
** Name: QED_R3_FLUSH - flush all tuples from sending LDB
**
** External QEF call:   status = qef_call(QED_3RDF_FLUSH, & qef_rcb);
**
** Description:
**      This routine flushes all tuples from the sending LDB.
**  
**  Note that RDF must be in a READ state, i.e., in the receiving state as
**  a result of executing a READ query on the specified LDB.
**
** Inputs:
**      v_qer_p				
**          .qef_cb		    internal QEF info
**		.qef_c2_ddb_ses	    
**		    .qes_d6_rdf_state  (QES_1ST_READ)
**	    .qef_r2_ddb_req	    QEF_DDB_REQ 
**		.qer_d2_ldb_info_p  target LDB, NULL ==> accessing CDB as 
**				    $ingres
** Outputs:
**      v_qer_p
**          .qef_cb		    current QEF session cb
**		.qef_c2_ddb_ses	    current DDB session cb
**		    .qes_d6_rdf_state  QES_0ST_NONE 
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE001F_INVALID_REQUEST
**				    (others to be specified)
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
**	28-jan-1993 (fpang)
**	    If metadata transaction was started, and we are in autocommit,
**	    commit the internal transaction.
**      19-Mar-2002 (hanal04) Bug 107330 INGSRV 1729.
**          We should commit the TX if we are QEF_RDF_MSTRAN (A TX 
**          initiated by QEF_RDF_INTERNAL operations executed by RDF) 
**          and we are not in a distributed transaction.
*/


DB_STATUS
qed_r3_flush(
QEF_RCB		*v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    RQR_CB		rqr,
			*rq_p = & rqr;
    DD_LDB_DESC		*which_p;


    /* 1. verify RDF's state */

    if (dds_p->qes_d6_rdf_state != QES_1ST_READ)
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(E_DB_ERROR);
    }

    /* 2.  set up to call RQF to flush */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rq_p->rqr_session = dds_p->qes_d3_rqs_p;   
    if (ddr_p->qer_d2_ldb_info_p == NULL)   /* defaulting to the CDB */
	which_p = & dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    else				    /* use LDB id */
	which_p = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;

    /* 3.  call RQF to flush */

    rq_p->rqr_1_site = which_p;

    status = qed_u3_rqf_call(RQR_T_FLUSH, rq_p, v_qer_p);
    if (status)
    {
	return(status);
    }
    dds_p->qes_d6_rdf_state = QES_0ST_NONE; /* must reinitialize state */

    /* If autocommit and transaction was started, commit it.  */
    if (( v_qer_p->qef_cb->qef_auto == QEF_ON 
        && v_qer_p->qef_cb->qef_stat != QEF_NOTRAN 
        && !which_p->dd_l1_ingres_b )
      ||
       ((v_qer_p->qef_stmt_type & QEF_RDF_MSTRAN)
        &&
	( dds_p->qes_d7_ses_state == QES_0ST_NONE)))
    {
	v_qer_p->qef_stmt_type &= (~(QEF_RDF_MSTRAN));
	status = qet_commit(v_qer_p->qef_cb);
    }

    return(status);
}


/*{
** Name: QED_R4_VDATA - inform TPF prior to execution of a series of read or 
**			update queries on an LDB
**
** External QEF call:	status = qef_call(QED_4RDF_VDATA, & qef_rcb);
**
** Description:
**      This routine delivers a query with data values to an LDB for execution.
**  If defaulting to the CDB, then execution assumes the special $ingres status.
**
**  Note that this operation will label the RDF state as READ if the query mode
**  is READ.  RDF can only emerge from this state if a QED_3RDF_FLUSH is 
**  requested or a subsequent QED_2RDF_FETCH encounters end of data.
**
** Inputs:
**      v_qer_p				
**          .qef_cb		    internal QEF info
**		.qef_c2_ddb_ses	    
**		    .qes_d6_rdf_state  (QES_0ST_NONE)
**	    .qef_r2_ddb_req	    QEF_DDB_REQ 
**		.qer_d2_ldb_info_p  target LDB, NULL ==> accessing CDB as 
**				    $ingres
**		.qer_d4_qry_info
**		    .qed_q1_timeout timeout quantum, 0 if none
**		    .qed_q2_lang    DB_SQL or DB_QUEL
**		    .qed_q3_qmode   DD_READ or DD_UPDATE
**		    .qed_d4_packet  query text information
**		    .qed_q5_dv_cnt  > 0
**		    .qed_q6_dv_p    != NULL
**
** Outputs:
**      v_qer_p
**          .qef_cb		    current QEF session cb
**		.qef_c2_ddb_ses	    current DDB session cb
**		    .qes_d6_rdf_state  QES_1ST_READ if 
**					qer_d4_qry_info.qed_q3_qmode is DD_READ
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE001F_INVALID_REQUEST
**				    (others to be specified)
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
**      21-oct-88 (carl)
**          written
*/


DB_STATUS
qed_r4_vdata(
QEF_RCB       *v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;


    /* 1.  verify RDF's state */

    if (
	(dds_p->qes_d6_rdf_state != QES_0ST_NONE)
	||
	(! (ddr_p->qer_d4_qry_info.qed_q5_dv_cnt > 0))
	)
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(status);
    }

    status = qed_r7_tp_rq(v_qer_p);
    return(status);
}


/*{
** Name: QED_R5_BEGIN - Inform TPF that a series of purely read/update
**			queries will be executed on an LDB.
**
** External QEF call:	status = qef_call(QED_5RDF_BEGIN, & qef_rcb);
**
** Description:
**      This routine informs TPF accordingly.
**
**	Note: This routine is never called by RDF    19-jul-91 (davebf)
**
** Inputs:
**      v_qer_p				
**          .qef_cb		    internal QEF info
**		.qef_c2_ddb_ses	    
**		    .qes_d6_rdf_state  (QES_0ST_NONE)
**	    .qef_r2_ddb_req	    QEF_DDB_REQ 
**		.qer_d2_ldb_info_p  target LDB, NULL ==> accessing CDB as 
**				    $ingres
**		.qer_d4_qry_info
**		    .qed_q3_qmode   DD_READ or DD_UPDATE
**
** Outputs:
**      v_qer_p
**          .qef_cb		    current QEF session cb
**		.qef_c2_ddb_ses	    current DDB session cb
**		    .qes_d6_rdf_state  QES_1ST_READ if qer_d4_qry_info
**					.qed_q3_qmode is DD_READ
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE001F_INVALID_REQUEST
**				    (others to be specified)
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
**      21-oct-88 (carl)
**          written
*/


DB_STATUS
qed_r5_begin(
QEF_RCB	    *v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC		*which_p;
    TPR_CB		tpr,
			*tp_p = & tpr;
    i4			tp_opcode;


    /* 1.  verify RDF's state */

    if (dds_p->qes_d6_rdf_state != QES_0ST_NONE)
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(status);
    }
    
    if (ddr_p->qer_d4_qry_info.qed_q3_qmode == DD_1MODE_READ)
	return(E_DB_OK);

    MEfill(sizeof(tpr), '\0', (PTR) & tpr);

    /* 2.  inform TPF of read/update intention */

    tp_p->tpr_session = dds_p->qes_d2_tps_p;
    if (ddr_p->qer_d2_ldb_info_p == NULL)	/* defaulting to the CDB */
	which_p = & dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    else					/* use LDB id */
	which_p = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;

    tp_p->tpr_site = which_p;

    if (ddr_p->qer_d4_qry_info.qed_q3_qmode == DD_1MODE_READ)
	tp_opcode = TPF_READ_DML;
    else if (ddr_p->qer_d4_qry_info.qed_q3_qmode == DD_2MODE_UPDATE)
	tp_opcode = TPF_WRITE_DML;
    else
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(status);
    }
    
    status = qed_u17_tpf_call(tp_opcode, tp_p, v_qer_p);

    return(status);
}


/*{
** Name: QED_R6_COMMIT - Send a COMMIT message across the CDB association
**
** External QEF call:	status = qef_call(QED_6RDF_COMMIT, & qef_rcb);
**
** Description:
**      This routine bypasses TPF.
**
** Inputs:
**      v_qer_p				
**          .qef_cb		    internal QEF info
**		.qef_c2_ddb_ses	    
**		    .qes_d6_rdf_state  (QES_0ST_NONE)
**
** Outputs:
**      v_qer_p
**          .qef_cb		    current QEF session cb
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE001F_INVALID_REQUEST
**				    (others to be specified)
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
**      23-mar-89 (carl)
**          written
**      10-jun-89 (carl)
*8	    changed to call qed_u11_ddl_commit
*/


DB_STATUS
qed_r6_commit(
QEF_RCB	    *v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
/*
    DD_LDB_DESC		*cdb_p = 
			 & dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
*/

    /* 1.  verify RDF's state */

    if (dds_p->qes_d6_rdf_state != QES_0ST_NONE)
    {
	status = qed_u2_set_interr(
		    E_QE001F_INVALID_REQUEST,
		    & v_qer_p->error);
	return(status);
    }
    
    status = qed_u11_ddl_commit(v_qer_p);
    return(status);
}


/*{
** Name: QED_R7_TP_RQ - inform TPF before sending a query to an LDB for execution
**
** Description:
**      This routine delivers a query to an LDB for execution.  If defaulting
**  to the CDB, then execution assumes the special $ingres status.
**
**  Note that this operation will label the RDF state as READ if the query mode
**  is READ.  RDF can only emerge from this state if a QED_3RDF_FLUSH is 
**  requested or a subsequent QED_2RDF_FETCH encounters end of data.
**
** Inputs:
**      v_qer_p				
**          .qef_cb		    internal QEF info
**		.qef_c2_ddb_ses	    
**		    .qef_rdf_state  (QES_0ST_NONE)
**	    .qef_r2_ddb_req	    QEF_DDB_REQ 
**		.qer_d2_ldb_info_p  target LDB, NULL ==> accessing CDB as 
**				    $ingres
**		.qer_d4_qry_info
**		    .qed_q1_timeout timeout quantum, 0 if none
**		    .qed_q2_lang    DB_SQL or DB_QUEL
**		    .qed_q3_qmode   DD_READ or DD_UPDATE
**		    .qed_q4_pkt_p   query information
**		    .qed_q5_dv_cnt  number of data values in query	
**		    .qed_q6_dv_p    ptr to first of DB_DATA_VALUE array
**				    containing as many elements as above
**				    count
** Outputs:
**      v_qer_p
**          .qef_cb		    current QEF session cb
**		.qef_c2_ddb_ses	    current DDB session cb
**		    .qef_rdf_state  QES_1ST_READ if qer_d4_qry_info
**					.qed_q3_qmode is DD_READ
**          .error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE001F_INVALID_REQUEST
**				    (others to be specified)
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
**      25-jul-88 (carl)
**          written
**      20-may-89 (carl)
**	    changed all direct rqf_call calls to qed_u3_rqf_call to
**	    include RDF's CDB traffic in DDL_CONCURRENCY management
**	    to avoid such traffic from deadlocking other sessions
**      22-aug-89 (carl)
**	    fixed to inform RQF that attribute names are not wanted 
**	    for internal queries
**      08-oct-89 (carl)
**	    modified qed_r7_tp_rq to commit the CDB association for RDF to
**	    avoid deadlocks if DDL_CONCURRECY is ON
**	13-mar-1993 (fpang)
**	    Call qed_u14_pre_commit() instead of qed_u11_ddl_commit(). The
**	    former ignores autocommit state.
**	    Fixes B44827, mulitple select from views cause cdb deadlock.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
*/


DB_STATUS
qed_r7_tp_rq(
QEF_RCB       *v_qer_p )
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC		*which_p;
    RQR_CB		rqr,
			*rq_p = & rqr;
    TPR_CB		tpr,
			*tp_p = & tpr;


    MEfill(sizeof(tpr), '\0', (PTR) & tpr);

    /* 1.  inform TPF of read/update intention if necessary */

    tp_p->tpr_session = dds_p->qes_d2_tps_p;
    if (ddr_p->qer_d2_ldb_info_p == NULL)   /* defaulting to the CDB */
    {
	which_p = & dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;

	/* commit the CDB association for RDF if DDL_CONCURRENCY is ON */

	status = qed_u14_pre_commit(v_qer_p);
	if (status)
	{
	    qed_u10_trap();
	    return(status);
	}
    }
    else				    /* use LDB id */
	which_p = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;

    tp_p->tpr_site = which_p;

    if (ddr_p->qer_d4_qry_info.qed_q3_qmode == DD_2MODE_UPDATE)
    {
	tp_p->tpr_lang_type = ddr_p->qer_d4_qry_info.qed_q2_lang;

	status = qed_u17_tpf_call(TPF_WRITE_DML, tp_p, v_qer_p);
	if (status)
	{
	    return(status);
	}
    }

    /* 2.  set up to call RQF to deliver query */

    MEfill( sizeof(rqr), '\0', (PTR) & rqr);

    rq_p->rqr_session = dds_p->qes_d3_rqs_p;   
    rq_p->rqr_1_site = which_p;
    rq_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rq_p->rqr_q_language = ddr_p->qer_d4_qry_info.qed_q2_lang;
    rq_p->rqr_dv_cnt = ddr_p->qer_d4_qry_info.qed_q5_dv_cnt;
    rq_p->rqr_dv_p = ddr_p->qer_d4_qry_info.qed_q6_dv_p;
    rq_p->rqr_inv_parms = (PTR) NULL;
    rq_p->rqr_no_attr_names = TRUE; /* no attribute names wanted */

    STRUCT_ASSIGN_MACRO(*ddr_p->qer_d4_qry_info.qed_q4_pkt_p, rq_p->rqr_msg);
    
    if (rq_p->rqr_dv_cnt > 0)
	status = qed_u3_rqf_call(RQR_DATA_VALUES, rq_p, v_qer_p);
    else    
	status = qed_u3_rqf_call(RQR_NORMAL, rq_p, v_qer_p);

    if (status)
    {
	return(status);
    }
    /* 3.  set correct RDF state */

    if (ddr_p->qer_d4_qry_info.qed_q3_qmode == DD_1MODE_READ)
	dds_p->qes_d6_rdf_state = QES_1ST_READ;
    else
	dds_p->qes_d6_rdf_state = QES_0ST_NONE;

    return(E_DB_OK);
}


/*{
** Name: QED_R8_DIFF_ARCH - Request LDB's architecture likeness from RQF.
**
**  External call:  status = qef_call(QED_7RDF_DIFF_ARCH, & qef_rcb);
**
** Description:
**      This routine returns a boolean value, TRUE if the LDB has a different
**  architecture from STAR's and FALSE otherwise.
**
**  Assumption:
**	Input structure DD_CAPS assumed to have been properly initialized.
**
** Inputs:
**	    v_qer_p			QEF_RCB
**		.qef_cb			QEF_CB, session cb
**		    .qef_c2_ddb_ses	QES_DDB_SES, DDB session
**			.qes_d2_tps_p	TPF session ptr
**			.qes_d3_rqs_p	RQF session ptr
**              .qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d2_ldb_info_p	DD_1LDB_INFO ptr
**			.dd_l1_ingres_b	    TRUE if $ingres access, FALSE 
**					    otherwise
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    LDB name
** Outputs:
**	    v_qer_p			QEF_RCB
**              .qef_r3_ddb_req		QEF_DDB_REQ
**		    .qer_d15_diff_arch_b
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
**      25-apr-89 (carl)
**          written
**      10-jun-89 (carl)
**	    map RQF to QEF error
*/


DB_STATUS
qed_r8_diff_arch(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    DD_LDB_DESC	    *ldb_p = 
			& ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    RQR_CB	    rqr,
		    *rq_p = & rqr;


    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rq_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rq_p->rqr_timeout = 0;
    rq_p->rqr_1_site = ldb_p;
    rq_p->rqr_q_language = DB_SQL;

    status = qed_u3_rqf_call(RQR_LDB_ARCH, rq_p, v_qer_p);
    if (status)
    {
	return(status);
    }
    else
	ddr_p->qer_d15_diff_arch_b = rq_p->rqr_diff_arch;

    return(status);
}

