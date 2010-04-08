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
#include    <scf.h>

/**
**  Name: QEDCON.C - Routines performing CONNECT-related operations.
**
**  Description:
**      These routines perform CONNECT-specific functions. 
**
**          qed_c1_conn - process DIRECT CONNECT 
**	    qed_c2_cont	- exchange information between SCF and RQF in
**			  CONNECT state
**	    qed_c3_disc - process DIRECT DISCONNECT
**
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	23-may-89 (carl)
**	    fixed qed_c3_disc to commit DEIs if AUTOCOMMIT is ON
**	10-jun-89 (carl)
**	    fixed to initiailze rqr_timeout for RQF
**	13-mar-90 (carl)
**	    1.  fixed QED_C1_CONN for DEI to 1) begin a transaction if 
**		necessary and 2) call TPF to validate the LDB designated
**		by the DEI
**	    2.  fixed QED_C3_DISC to initialize rqr_dc_txn so that RQF will 
**		commit a DEI
**	23-jun-90 (carl)
**	    fixed QED_C1_CONN for DEI to retrieve the LDB's capabilities
**	10-aug-90 (carl)
**	    removed all references to IIQE_*;
**	    changed all occurrences of rqf_call to qed_u3_rqf_call;
**	    changed all occurrences of tpf_call to qed_u17_tpf_call;
**	31-jan-91 (fpang)
**	    For 'direct execute immediate', begin the transaction before
**	    informing TPF of immenent WRITE. Not doing so causes TPF
**	    to be confused about the transaction state. This fixes
**	    B35593, where optimizedb was not working on distributed 
**	    databases.
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	05-feb-1993 (fpang)
**	    Added support to detect commit/aborts when executing local database
**	    procedure and when executing direct execute immediate. If the ldb
**	    returns status that the local dbp or dei tried to commit/abort, 
**	    and if the current DX is in 2PC, abort the transaction.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	12-oct-1994	(ramra01)
**		Error initialize in sav_err in code
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/


/*{
** Name: QED_C1_CONN - Perform a DIRECT CONNECT command.
**
** External QEF call:   status = qef_call(QED_C1_CONN, &qef_rcb);
**
** Description:
**  This operation does the following:
**	1)  Verify that there is no current tranaction, and no current CONNECT
**	    session.
**	2)  Establish a CONNECT session by requesting one from RQF.
**	3)  Record the information in the session's control block.
**
** Inputs:
**      v_qer_p				    QEF_RCB
**	    qef_r3_ddb_req		    QEF_DDB_REQ
**		.qer_d4_qry_info	    QED_QUERY_INFO
**		    .qed_q1_timeout	    timeout quantum, 0 if none
**		    .qed_q2_lang	    DB_QUEL or DB_SQL
**		.qer_d6_conn_info
**		    .qed_c1_ldb		    DD_LDB_DESC, LDB desc
**			.dd_l1_ingres_b	    TRUE if $ingres access required
**					    FALSE otherwise
**			.dd_l2_node_name    node name
**			.dd_l3_ldb_name	    database name
**			.dd_l5_ldb_id	    0 for unknown
**
** Outputs:
**      v_qer_p
**          error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
**                                  E_QE0961_CONNECT_ON_XACTION
**                                  E_QE0960_CONNECT_ON_CONNECT
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
**	08-jun-88 (carl)
**          written
**	10-jun-89 (carl)
**	    fixed to initiailze rqr_timeout for RQF
**	13-mar-90 (carl)
**	    fixed for DEI to 1) begin a transaction if 	necessary and 
**	    2) call TPF to validate the LDB designated by the DEI
**	23-jun-90 (carl)
**	    fixed for the case of a DEI to retrieve the LDB's capabilities
**	31-jan-91 (fpang)
**	    For 'direct execute immediate', begin the transaction before
**	    informing TPF of immenent WRITE. Not doing so causes TPF
**	    to be confused about the transaction state. This fixes
**	    B35593, where optimizedb was not working on distributed 
**	    databases.
**	05-feb-1993 (fpang)
**	    Moved informing TPF of write to qed_c2_cont().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qed_c1_conn(
QEF_RCB       *v_qer_p ) 
{
    DB_STATUS		status;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QED_QUERY_INFO	*qry_p = & v_qer_p->qef_r3_ddb_req.qer_d4_qry_info;
    QES_CONN_SES	*conses_p = & dds_p->qes_d8_union.u1_con_ses;
    QED_CONN_INFO	*coninfo_p = & v_qer_p->qef_r3_ddb_req.qer_d6_conn_info;
    DD_LDB_DESC		*ldb_p = & coninfo_p->qed_c1_ldb;
    RQR_CB		rqr,
			*rqr_p = & rqr;
    TPR_CB		tpr,
			*tpr_p = & tpr;
    bool		dei_ok = FALSE;


    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    if (coninfo_p->qed_c5_tran_ok_b)
    {
	/* ok for CONNECT session in transaction */

	rqr_p->rqr_dc_txn = TRUE;		/* transaction */

    }
    else
    {
	/* this session not allowed in a transaction */

	if (v_qer_p->qef_cb->qef_stat != QEF_NOTRAN)
	{
	    status = qed_u2_set_interr(
		    E_QE0961_CONNECT_ON_XACTION,
		    & v_qer_p->error);
	    return(status);
	}

	rqr_p->rqr_dc_txn = FALSE;		/* no transaction */
    }

    /* 2.  if nested CONNECT session, error.  */

    if (dds_p->qes_d7_ses_state != QES_0ST_NONE)
    {
	status = qed_u2_set_interr(
		    E_QE0960_CONNECT_ON_CONNECT,
		    & v_qer_p->error);
	return(status);
    }

    /* 3.  set up to request CONNECT session from RQF  */

    v_qer_p->error.err_code = E_QE0000_OK;

    coninfo_p->qed_c1_ldb.dd_l5_ldb_id = DD_0_IS_UNASSIGNED;
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session */
    rqr_p->rqr_q_language = ddr_p->qer_d4_qry_info.qed_q2_lang;
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = ldb_p;
    rqr_p->rqr_dc_blkp = ddr_p->qer_d6_conn_info.qed_c4_exchange_p;
    rqr_p->rqr_dc_txn = ddr_p->qer_d6_conn_info.qed_c5_tran_ok_b;

    status = qed_u3_rqf_call(RQR_CONNECT, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (coninfo_p->qed_c5_tran_ok_b)
    {
	/* this is a DIRECT EXECUTE IMMEDIATE */

	if (! (ldb_p->dd_l6_flags & DD_03FLAG_SLAVE2PC) )
	{
	    /* here if unknown SLAVE 2PC capability */

	    status = qed_q5_ldb_slave2pc(ldb_p, v_qer_p);   
					/* get the LDB's SLAVE 2PC 
					** capability */
	    if (status)
	    {
		return(status);
	    }
	}
	/* make sure that this LDB with the assumed imminent WRITE statement 
	** fit in with the LDB mix of the transaction */

	status = qet_t11_ok_w_ldb(& coninfo_p->qed_c1_ldb, v_qer_p, & dei_ok);
	if (status)
	    return(status);
	if (! dei_ok)
	{
	    status = qed_u2_set_interr(
                            E_QE0990_2PC_BAD_LDB_MIX,
                            & v_qer_p->error);
	    return(status);
	}


	/* begin a transaction if necessary */

	if (v_qer_p->qef_cb->qef_stat == QEF_NOTRAN)
	{
	    v_qer_p->qef_modifier = QEF_SSTRAN;
	    v_qer_p->qef_flag = 0;
        
	    /* qet_begin will log any error message */

	    status = qet_begin(v_qer_p->qef_cb);
	    if (status) 
	    {
		(VOID) qed_u3_rqf_call(RQR_DISCONNECT, rqr_p, v_qer_p);
				    /* disconnect before return */
		return(status);
	    }
	}
    }

    /*  set up CONNECT control block in session cb */

    dds_p->qes_d7_ses_state = QES_2ST_CONNECT;
    conses_p->qes_c1_rqs_p = rqr_p->rqr_session;
    STRUCT_ASSIGN_MACRO(
	v_qer_p->qef_r3_ddb_req.qer_d6_conn_info, 
	conses_p->qes_c2_con_info);

    return(E_DB_OK);
}


/*{
** Name: QED_C2_CONT - Exchange information between SCF and RQF.
**
** External QEF call:   status = qef_call(QED_C2_CONT, &qef_rcb);
**
** Description:
**      This routine acts as an intermediary between SCF and RQF for exchanging
**  information within a CONNECT state.
**
** Inputs:
**	v_qer_p				    QEF_RCB
**	    qef_r3_ddb_req		    QEF_DDB_REQ
**		.qer_d4_qry_info	    QED_QUERY_INFO
**		    .qed_q1_timeout	    timeout quantum, 0 if none
**		    .qed_q2_lang	    language used
**		.qer_d6_conn_info	    QED_CONN_INFO
**		    .qed_c4_exchange_p	    ptr to strcuture for exchanging
**					    information between SCF and RQF
** Outputs:
**      v_qer_p
**          error.err_code         One of the following:
**                                  E_QE0000_OK
**                                  E_QE0002_INTERNAL_ERROR
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
**	05-feb-1993 (fpang)
**	    Added support to detect commit/aborts when executing local database
**	    procedure and when executing direct execute immediate. If the ldb
**	    returns status that the local dbp or dei tried to commit/abort, 
**	    and if the current DX is in 2PC, abort the transaction.
*/


DB_STATUS
qed_c2_cont(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_CONN_SES    *conses_p = & dds_p->qes_d8_union.u1_con_ses;
    QED_CONN_INFO   *coninfo_p = & v_qer_p->qef_r3_ddb_req.qer_d6_conn_info;
    DD_LDB_DESC	    *ldb_p = & conses_p->qes_c2_con_info.qed_c1_ldb; 
					/* LDB id */
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    TPR_CB	    tpr,
		    *tpr_p = & tpr;
    bool	    dei_ok = FALSE;
    QED_QUERY_INFO  *qry_p = & v_qer_p->qef_r3_ddb_req.qer_d4_qry_info;
    SCC_SDC_CB	    *dc = 
      (SCC_SDC_CB *)ddr_p->qer_d6_conn_info.qed_c4_exchange_p;


    /* 1. verify CONNECT context */

    if (dds_p->qes_d7_ses_state != QES_2ST_CONNECT)
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    if ((coninfo_p->qed_c5_tran_ok_b == TRUE) &&    /* DEI, or EXEC PROC */
	(dc->sdc_mode == SDC_WRITE))		    /* and writing to LDB */
    {
	/* inform TPF of this assumed WRITE */
	MEfill(sizeof(tpr), '\0', (PTR) & tpr);
	tpr_p->tpr_session = dds_p->qes_d2_tps_p;
	tpr_p->tpr_lang_type = qry_p->qed_q2_lang;
	coninfo_p->qed_c1_ldb.dd_l5_ldb_id = DD_0_IS_UNASSIGNED;
	tpr_p->tpr_site = & coninfo_p->qed_c1_ldb;

	status = qed_u17_tpf_call(TPF_WRITE_DML, tpr_p, v_qer_p);
	if (status)
	{
	    (VOID) qed_u3_rqf_call(RQR_DISCONNECT, rqr_p, v_qer_p);
				    /* disconnect before return */
	    return(status);
	}
    }

    /* 2.  set up to call RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = conses_p->qes_c1_rqs_p;   
					/* CONNECT session id */
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_1_site = ldb_p; 			/* LDB id */
    rqr_p->rqr_q_language = ddr_p->qer_d4_qry_info.qed_q2_lang;
    rqr_p->rqr_dc_blkp = ddr_p->qer_d6_conn_info.qed_c4_exchange_p;
    rqr_p->rqr_dc_txn = ddr_p->qer_d6_conn_info.qed_c5_tran_ok_b;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (  (coninfo_p->qed_c5_tran_ok_b == TRUE)     /* DEI or EXEC PROC */
	&&(dc->sdc_mode == SDC_WRITE)		    /* and writing to LDB */
        &&(tpr_p->tpr_22_flags & TPR_01FLAGS_2PC))  /* and in 2PC */
	rqr_p->rqr_2pc_txn = TRUE;
    else
	rqr_p->rqr_2pc_txn = FALSE;

    status = qed_u3_rqf_call(RQR_CONTINUE, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (  (coninfo_p->qed_c5_tran_ok_b == TRUE)	     /* DEI or EXEC PROC */
        &&(dc->sdc_mode == SDC_READ)		     /* and reading from LDB */
        &&(rqr_p->rqr_ldb_status & RQF_04_XACT_MASK))/* and LDB tried to */
    {						     /*     commit/abort */
	qet_abort(v_qer_p->qef_cb);		     /* Abort DX */
	v_qer_p->error.err_code = E_QE0991_LDB_ALTERED_XACT;
	return (E_DB_ERROR);
    }

    return(E_DB_OK);
}


/*{
** Name: QED_C3_DISC - Perform a DIRECT DISCONNECT directive.
**
** External QEF call:   status = qef_call(QED_C3_DISC, &qef_rcb);
**
** Description:
**	This operation returns the session to its normal DDB state.  It does
**  not commit or abort the CONNECT session.
**
** Inputs:
**	v_qer_p
**	    qef_r3_ddb_req 
**		.qer_d4_qry_info
**		    .qed_q2_lang    DB_NOLANG ==> no commit/abort for session
**				    DB_SQL ==> commit CONNECT session
**				    DB_QUEL ==> abort CONNECT session
** Outputs:
**	v_qer_p
**	    error.err_code         One of the following:
**                                      E_QE0000_OK
**                                      E_QE0002_INTERNAL_ERROR
**                                      E_QE096E_INVALID_DISCONNECT
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
**      08-jun-88 (carl)
**          written
**	23-may-89 (carl)
**	    fixed to commit DEIs if AUTOCOMMIT is ON
**	10-jun-89 (carl)
**	    fixed to initiailze rqr_timeout for RQF
**	13-mar-90 (carl)
**	    initialize rqr_dc_txn so that RQF will commit a DEI
**	12-oct-94 (ramra01)
**	Initialize sav_err to avoid junk error to be returned
*/


DB_STATUS
qed_c3_disc(
QEF_RCB       *v_qer_p )
{
    DB_STATUS		sav_status = E_DB_OK;
    DB_STATUS		status;
    DB_ERROR		sav_error;    
    QEF_DDB_REQ		*ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_CONN_SES	*conses_p = & dds_p->qes_d8_union.u1_con_ses;
    QED_CONN_INFO	*coninfo_p = & conses_p->qes_c2_con_info;
    RQR_CB		rqr,
			*rqr_p = & rqr;


    /*  1.  verify that session is in DIRECT CONNECT mode  */

    if (dds_p->qes_d7_ses_state != QES_2ST_CONNECT)
    {
	status = qed_u2_set_interr(
		    E_QE096E_INVALID_DISCONNECT,
		    & v_qer_p->error);
	return(E_DB_ERROR);
    }

    dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* first reset state */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_timeout = ddr_p->qer_d4_qry_info.qed_q1_timeout;
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    if (coninfo_p->qed_c5_tran_ok_b 		/* DEI ? */
	&&
	v_qer_p->qef_cb->qef_auto == QEF_ON)	/* AUTOCOMMIT ON ? */
    {
    	status = qed_u7_msg_commit(v_qer_p, & coninfo_p->qed_c1_ldb);   
	if (status)
	{
	    sav_status = status;
		/* save the returned error from msg_commit temporarily */
		/* This will be reassigned to v_qer_p - ramra01 65255  */
		STRUCT_ASSIGN_MACRO(v_qer_p->error, sav_error );
	}
    }
    else
	sav_status = E_DB_OK;

    /* 4.  terminate CONNECT session with RQF */

    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = & coninfo_p->qed_c1_ldb;   
					/* LDB id */
    rqr_p->rqr_dc_txn = ddr_p->qer_d6_conn_info.qed_c5_tran_ok_b;
					/* must set so that RQF will commit
					** any DEI which is considered part
					** of the transaction */
    status = qed_u3_rqf_call(RQR_DISCONNECT, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    if (sav_status)
    {
	STRUCT_ASSIGN_MACRO(sav_error, v_qer_p->error);
	return(sav_status);
    }

    return(status);
}

