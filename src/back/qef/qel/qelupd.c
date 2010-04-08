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
#include    <qefgbl.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QELUPD.C - Utility routines for updating CDB catalogs
**
**  Description:
**	    qel_u1_update	- update a CDB catalog tuple 
**	    qel_u2_query	- construct query for appropriate catalog
**	    qel_u3_lock_objbase - lock iidd_ddb_object_base to serialize
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	13-sep-89 (carl)
**	    added qel_u3_lock_objbase as a means of synchronization for DDLs
**	    to avoid deadlocks
**	23-sep-90 (carl)
**	    added trace point processing to qel_u1_update
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/


/*{
** Name: QEL_U1_UPDATE - Update a CDB catalog tuple
**
** Description:
**	This routine update a CDB catalog tuple as directed by calling RQF.
**  It assumes that a TPF call has been made to register the proper access 
**  with the CDB.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_23_update_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c1_can_id	    CDB catalog id
**		.qeq_c3_tup_u.	    ptr to tuple data
**		.qeq_c4_ldb_p	    ptr to CDB
**
** Outputs:
**	none
**	
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
**	08-jun-88 (carl)
**          written
**	23-sep-90 (carl)
**	    added trace point processing 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_u1_update(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QEF_CB	    *qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEQ_1CAN_QRY    *upd_p = v_lnk_p->qec_23_update_p;
    DD_LDB_DESC	    *ldb_p = upd_p->qeq_c4_ldb_p;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    bool            log_ddl_56 = FALSE,
		    log_err_59 = FALSE;
    i4	    i4_1,
		    i4_2;
    char	    qrytxt[QEK_900_LEN];


    if (ldb_p != cdb_p)
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_DDL_56,
	    & i4_1, & i4_2))
    {
        log_ddl_56 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    /* 1.  set up update query */

    status = qel_u2_query(v_qer_p, v_lnk_p, qrytxt);
    if (status)
	return(status);

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;	/* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = upd_p->qeq_c4_ldb_p;	/* CDB */
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    if (log_ddl_56)
	qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying CDB info */
    
    /* 3.  execute query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	if (log_err_59 && (! log_ddl_56))
	    qed_w8_prt_qefqry(v_qer_p, qrytxt, (DD_LDB_DESC *) NULL);
					/* suppress displaying CDB info */
	return(status);
    }
    return(E_DB_OK);
}


/*{
** Name: QEL_U2_QUERY - Construct query for appropriate CDB catalog.
**
** Description:
**	This routine generates an appropriate query from the given information.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**	    .qec_23_update_p	    ptr to QEQ_1CAN_QRY
**		.qeq_c1_can_id	    CDB catalog id
**		.qeq_c3_tup_u.	    ptr to tuple data
**		.qeq_c4_ldb_p	    ptr to CDB
**
** Outputs:
**	query length
**	o_qry_p			    query in output buffer of sufficient size
**	
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
**	08-jun-88 (carl)
**          written
**	01-mar-93 (barbara)
**	    Added support for OPEN/SQL_LEVEL capability.
*/


DB_STATUS
qel_u2_query(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p,
char		*o_qry_p )
{
    DB_STATUS	    status;
    QEQ_1CAN_QRY    *upd_p = v_lnk_p->qec_23_update_p;
    i4		    can_id = upd_p->qeq_c1_can_id;


    switch (can_id)
    {
    case UPD_716_DD_DDB_OBJECT_BASE:
	{
	    /*	update iidd_ddb_object_base set object_base = object_base + 1;
	    */

	    STprintf(
		o_qry_p,
		"%s %s %s %s = %s + 1;", 
		IIQE_c7_sql_tab[SQL_09_UPDATE],
		IIQE_c8_ddb_cat[TDD_16_DDB_OBJECT_BASE],
		IIQE_c7_sql_tab[SQL_08_SET],
		IIQE_10_col_tab[COL_38_OBJECT_BASE],
		IIQE_10_col_tab[COL_38_OBJECT_BASE]);
	}
	break;

    case UPD_731_DD_TABLES:
	{
	    QEC_L16_TABLES  *tables_p = upd_p->qeq_c3_ptr_u.l16_tables_p;

	    /*	update iidd_tables set table_stats = '%s', modify_date = '%s'
	    *	where table_name = '%s' and table_onwer = '%s';*/

	    STprintf(
		o_qry_p,
		"%s %s %s %s = '%s', %s = '%s' %s %s = '%s' %s %s = '%s';", 
		IIQE_c7_sql_tab[SQL_09_UPDATE],
		IIQE_c8_ddb_cat[TDD_31_TABLES],
		IIQE_c7_sql_tab[SQL_08_SET],
		IIQE_10_col_tab[COL_27_TABLE_STATS],
		tables_p->l16_9_stats,
		IIQE_10_col_tab[COL_32_MODIFY_DATE],
		tables_p->l16_21_mod_date,
		IIQE_c7_sql_tab[SQL_11_WHERE],
		IIQE_10_col_tab[COL_03_TABLE_NAME],
		tables_p->l16_1_tab_name,
		IIQE_c7_sql_tab[SQL_00_AND],
		IIQE_10_col_tab[COL_04_TABLE_OWNER],
		tables_p->l16_2_tab_owner);
	}
	break;

    case UPD_800_COM_SQL_LVL:
    case UPD_801_ING_QUEL_LVL:
    case UPD_802_ING_SQL_LVL:
    case UPD_803_SAVEPOINTS:
    case UPD_804_UNIQUE_KEY_REQ:
    case UPD_805_OPN_SQL_LVL:
	{
	    QEC_L4_DBCAPABILITIES   *ddbcaps_p = 
		upd_p->qeq_c3_ptr_u.l4_dbcapabilities_p;


	    /*	update iidd_dbcapabilities set cap_value = c32
	    **  where cap_capability = c32;  */

	    STprintf(
		o_qry_p, 
		"%s %s %s %s = '%s' %s %s = '%s';",
		IIQE_c7_sql_tab[SQL_09_UPDATE],
		IIQE_c8_ddb_cat[TDD_03_DBCAPABILITIES],
		IIQE_c7_sql_tab[SQL_08_SET],
		IIQE_10_col_tab[COL_34_CAP_VALUE],
		ddbcaps_p->l4_2_cap_val,
		IIQE_c7_sql_tab[SQL_11_WHERE],
	        IIQE_10_col_tab[COL_31_CAP_CAPABILITY],
		ddbcaps_p->l4_1_cap_cap);
	}   
	break;

    default:
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);		
    }
    return(E_DB_OK);
}


/*{
** Name: QEL_U3_LOCK_OBJBASE - Lock iidd_ddb_objbase
**
** Description:
**	This routine locks iidd_ddb_objbase by replacing the only tuple
**  with itself without altering the value of the singleton column.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**
** Outputs:
**	none
**	
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
**	13-sep-89 (carl)
**          written
**	12-sep-91 (teresa)
**	    fixed bug 39664 where qel_u3_lock_objbase was not translating
**	    the RQF error message to a QEF error message before taking an
**	    error exit.  [This is only noticable in events where the LDB
**	    session goes away (or fails) exactly when the iidd_ddb_object_base
**	    catalog is being updated, which is not very frequently!]
**	13-sep-91 (teresa)
**	    put in a work-around for bug 39827, which will translate the
**	    return status from E_DB_ERR to E_DB_SEVERE if RQF returnes either 
**	    E_RQ0045_CONNECTION_LOST or E_RQ0046_RECV_TIMEOUT, which 
**	    routine qed_u3_rqf_call() translates to either of
**	    E_QE0545_CONNECTION_LOST or E_QE0546_RECV_TIMEOUT.
**	    Routine qel_u3_lock_objbase() operates only on the CDB via the
**	    priviledged association and is called my many different faccets of
**	    QEF to attempt to avoid deadlocks accessing star catalogs. A return
**	    status of E_DB_SEVERE encourages SCF to shut down the session, so
**	    STAR does not keep attempting to talk to the CDB until the error
**	    count gets so high that it shuts down the star server.
*/
DB_STATUS
qel_u3_lock_objbase(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    char	    qrytxt[QEK_900_LEN];



    status = qed_u11_ddl_commit(v_qer_p);
    if (status)
    {
	qed_u10_trap();
	return(status);
    }
    /* 1.  generate replace query */

    /*	update iidd_ddb_object_base set object_base = object_base;  */

    STprintf(
		qrytxt,
		"%s %s %s %s = %s;", 
		IIQE_c7_sql_tab[SQL_09_UPDATE],
		IIQE_c8_ddb_cat[TDD_16_DDB_OBJECT_BASE],
		IIQE_c7_sql_tab[SQL_08_SET],
		IIQE_10_col_tab[COL_38_OBJECT_BASE],
		IIQE_10_col_tab[COL_38_OBJECT_BASE]);

    /* 2.  set up for calling RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;	/* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = cdb_p;			/* CDB */
    rqr_p->rqr_q_language = DB_SQL;
    rqr_p->rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_p->rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    /* 3.  execute CDB query */

    status = qed_u3_rqf_call(RQR_NORMAL, rqr_p, v_qer_p);
    if (status)
    {
	qed_u10_trap();

	if ( (rqr_p->rqr_error.err_code == E_RQ0045_CONNECTION_LOST) ||
	     (rqr_p->rqr_error.err_code == E_RQ0046_RECV_TIMEOUT) )
	{
	    /* this is a work-around for bug 39827 -- set return status to
	    ** E_DB_SERVERE because the priviledged association was lost.
	    ** this should force SCF to shut down the session
	    */
	    status = E_DB_SEVERE;
	}

	qed_u12_map_rqferr( rqr_p, &v_qer_p->error ); /* fix bug 39664, map
						      ** RQF error to QEF err */
	return(status);
    }
    return(status);
}

