/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
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
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <ex.h>
#include    <tm.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <dudbms.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <sxf.h>
#include    <qefdsh.h>
#include    <qefprotos.h>


/**
**  Name: QEEPRT.C - DDB DSH print routines
**
**  Description:
**	qee_p0_prt_dsh	- display the DSH information
**	qee_p1_dsh_gen	- display the generic DSH information
**	qee_p2_dsh_ddb	- display the DDB-specific DSH information
**      qee_p3_qry_id   - display STAR id of current query
**      qee_p4_ldb_qid  - display the local query id
**      qee_p5_qids	- display both the STAR and LDB ids for current query 
**
**  History:    $Log-for RCS$
**	03-jul-90 (carl)
**          written.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	21-dec-92 (anitap)
**	    Prototyped qee_p0_prt_dsh(), qee_p1_dsh_gen(), qee_p2_dsh_ddb(),
**	    qee_p3_qry_id(), qee_p4_ldb_qid(), qee_p5_qids().
**      10-sep-93 (smc)
**          Corrected cast of qec_tprintf to PTR and used %p. Added <cs.h>
**          for CS_SID.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Altered qec_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to qec_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    qec_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-feb-04 (inkdo01)
**	    Change dsh_ddb_cb to ptr rather than instance.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

GLOBALREF   char	IIQE_61_qefsess[];	/* string "QEF session" */


/*{
** Name: qee_p0_prt_dsh - display DSH information of query plan
**
** Description:
**      (Not used.)
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to the DSH
**
** Outputs:
**	none
**
**      Returns:
**          DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	03-jul-90 (carl)
**          written
**	21-dec-92 (anitap)
**	    Prototyped.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/

DB_STATUS
qee_p0_prt_dsh(
	QEF_RCB		*v_qer_p,
	QEE_DSH		*i_dsh_p)
{
    DB_STATUS	    status = E_DB_OK;
    QEF_CB	    *qecb_p = v_qer_p->qef_cb;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (TRUE)
	return(E_DB_OK);

    STprintf(cbuf, 
	"%s %p: ...1.1) Generic DSH information\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    status = qee_p1_dsh_gen(v_qer_p, i_dsh_p);
    if (status)
	return(status);

    if (qecb_p->qef_c1_distrib & DB_3_DDB_SESS) 
    {
	STprintf(cbuf, 
	    "%s %p: ...1.2) DDB-specific DSH information\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	status = qee_p2_dsh_ddb(v_qer_p, i_dsh_p);
    }
    return(status);
}


/*{
** Name: qee_p1_dsh_gen - display generic DSH information of query plan
**
** Description:
**      (See above comment.)
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to the DSH
**
** Outputs:
**	none
**
**      Returns:
**          DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	03-jul-90 (carl)
**          written
**	21-dec-92 (anitap)
**	    Prototyped.
*/


DB_STATUS
qee_p1_dsh_gen(
	QEF_RCB		*v_qer_p,
	QEE_DSH		*i_dsh_p)
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;    /* debugging aid */
    QEE_DDB_CB	    *dde_p = i_dsh_p->dsh_ddb_cb;
    char	    cur_name[DB_CURSOR_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    qed_u0_trimtail(i_dsh_p->dsh_qp_id.db_cur_name, DB_CURSOR_MAXNAME, cur_name);

    STprintf(cbuf, 
	"%s %p: ...    query id: %x %x %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	i_dsh_p->dsh_qp_id.db_cursor_id[0],
	i_dsh_p->dsh_qp_id.db_cursor_id[1],
	cur_name);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    if (i_dsh_p->dsh_qp_status & DSH_QP_LOCKED)
    {
 	STprintf(cbuf, 
	    "%s %p: ...     query plan LOCKED by this session\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (i_dsh_p->dsh_qp_status & DSH_QP_OBSOLETE)
    {
	STprintf(cbuf, 
	    "%s %p: ...     query plan is OBSOLETE; to be destroyed\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (i_dsh_p->dsh_qp_status & DSH_DEFERRED_CURSOR)
    {
	STprintf(cbuf, 
	    "%s %p: ...     query plan contains a DEFERRED cursor\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (i_dsh_p->dsh_positioned)
    {
	STprintf(cbuf, 
	    "%s %p: ...     cursor is positioned\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    return(E_DB_OK);
}


/*{
** Name: qee_p2_dsh_ddb - display DDB-specific DSH information of query plan
**
** Description:
**      (See above comment.)
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to the DSH
**
** Outputs:
**	none
**
**      Returns:
**          DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	03-jul-90 (carl)
**          written
**	21-dec-92 (anitap)
**	    Prototyped.
*/


DB_STATUS
qee_p2_dsh_ddb(
	QEF_RCB		*v_qer_p,
	QEE_DSH		*i_dsh_p)
{
    DB_STATUS	    status = E_DB_OK;
    QEF_CB	    *qecb_p = v_qer_p->qef_cb;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;    /* debugging aid */
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;
    QEE_DDB_CB	    *dde_p = i_dsh_p->dsh_ddb_cb;
    DD_TAB_NAME	    *tmptbl_p = (DD_TAB_NAME *) NULL;
    i4	    *sts_p = (i4 *) NULL;
    i4		    i;
    char	    namebuf[DB_TAB_MAXNAME + 1];
    char	    cur_name[DB_CURSOR_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (! (qecb_p->qef_c1_distrib & DB_3_DDB_SESS) )
	return(E_DB_OK);    /* NOT a DDB session */

    if (ddq_p->qeq_d3_elt_cnt == 0)
    {
	STprintf(cbuf, 
	    "%s %p: ...     0 temporary tables used by query\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    else
    {
	if (ddq_p->qeq_d3_elt_cnt == 1)
	{
	    STprintf(cbuf, 
		"%s %p: ...     1 temporary table ",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb);
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: ...     %d temporary tables ",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		ddq_p->qeq_d3_elt_cnt);
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	STprintf(cbuf, "used by query\n");
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	/* print all the temporary table names */

	for (i = 1, tmptbl_p = dde_p->qee_d1_tmp_p,
		    sts_p = dde_p->qee_d2_sts_p;
		i <= ddq_p->qeq_d3_elt_cnt; 
		i++, tmptbl_p++, sts_p++)
	    if (*sts_p & QEE_01M_CREATED)
	    {
		qed_u0_trimtail((char *) tmptbl_p, DB_TAB_MAXNAME, namebuf);
		STprintf(cbuf, 
		    "%s %p: ...     %d) %s\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb,
		    i,
		    namebuf);
        	qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }
    }

    if (dde_p->qee_d3_status & QEE_01Q_CSR)
    {
        STprintf(cbuf, 
	    "%s %p: ...     query pertains to a CURSOR\n",
	    IIQE_61_qefsess,
            (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d3_status & QEE_02Q_RPT)
    {
        STprintf(cbuf, 
	    "%s %p: ...     this is a REPEAT query\n",
	    IIQE_61_qefsess,
            (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d3_status & QEE_03Q_DEF)
    {
        STprintf(cbuf, 
	    "%s %p: ...     query has been DEFINED to the LDB\n",
	    IIQE_61_qefsess,
            (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d3_status & QEE_04Q_EOD)
    {
	STprintf(cbuf, 
	    "%s %p: ...     query has NO more retrieved data coming back\n",
	    IIQE_61_qefsess,
            (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d3_status & QEE_05Q_CDB)
    {
        STprintf(cbuf, 
"%s %p: ...     retrieval REROUTED thru the CDB, to be committed when done\n",
	    IIQE_61_qefsess,
            (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d3_status & QEE_06Q_TDS)
    {
        STprintf(cbuf, 
	    "%s %p: ...     RQF returning a tuple descriptor to SCF\n",
	    IIQE_61_qefsess,
            (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d3_status & QEE_07Q_ATT)
    {
        STprintf(cbuf, 
	    "%s %p: ...     tuple descriptor contains attribute names\n",
	    IIQE_61_qefsess,
            (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d4_given_qid.db_cursor_id[0] != 0)
    {
	/* null-terminate name part for printing */

	qed_u0_trimtail( dde_p->qee_d4_given_qid.db_cur_name, DB_CURSOR_MAXNAME,
	    cur_name);

	STprintf(cbuf, 
	    "%s %p: ...     DDB query id: %x %x %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    dde_p->qee_d4_given_qid.db_cursor_id[0],
	    dde_p->qee_d4_given_qid.db_cursor_id[1],
	    cur_name);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    if (dde_p->qee_d5_local_qid.db_cursor_id[0] != 0)
    {
	/* null-terminate name part for printing */

	qed_u0_trimtail( dde_p->qee_d5_local_qid.db_cur_name, DB_CURSOR_MAXNAME,
	    cur_name);

	STprintf(cbuf, 
	    "%s %p: ...     LDB (sub)query id: %x %x %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    dde_p->qee_d5_local_qid.db_cursor_id[0],
	    dde_p->qee_d5_local_qid.db_cursor_id[1],
	    cur_name);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    if (dde_p->qee_d6_dynamic_cnt > 0)
    {
	if (dde_p->qee_d6_dynamic_cnt == 1)
	{
	    STprintf(cbuf, 
		"%s %p: ...     1 DB_DATA_VALUE ",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb);
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: ...     %d DB_DATA_VALUEs ",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		dde_p->qee_d6_dynamic_cnt);
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	STprintf(cbuf, "for query\n");
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (dde_p->qee_d3_status & QEE_01Q_CSR
	||
	dde_p->qee_d3_status & QEE_02Q_RPT)
    {
	if (dde_p->qee_d3_status & QEE_01Q_CSR)
	{
	    STprintf(cbuf, 
		"%s %p: ...     LDB for this CURSOR query:\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb);
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: ...     LDB for this REPEAT query:\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb);
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}

	/* print the LDB's information */

	status = qed_w1_prt_ldbdesc(v_qer_p, & dde_p->qee_d11_ldbdesc);
    }

    return(E_DB_OK);
}


/*{
** Name: qee_p3_qry_id - display query id for DEFINE/CURSOR query
**
** Description:
**
** Inputs:
**	v_qer_p->			ptr to QEF_RCB
**          qef_cb
**      i_dsh_p                         data segment header
**          .dsh_ddb_cb
**              .qee_d4_given_qid       query id
**
** Outputs:
**	none
**	
**	Returns:
**	    DB_STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-aug-90 (carl)
**          written
**	21-dec-92 (anitap)
**	    Prototyped.
*/


DB_STATUS
qee_p3_qry_id(
	QEF_RCB		*v_qer_p,
	QEE_DSH         *i_dsh_p)
{
/*
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ     *ddr_p = & v_qer_p->qef_r3_ddb_req;
*/
    QEE_DDB_CB      *qee_p = i_dsh_p->dsh_ddb_cb;
    DB_CURSOR_ID    *qid_p = & qee_p->qee_d4_given_qid;
    char	    cur_name[DB_CURSOR_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, 
	"%s %p: ...the query id:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(cbuf, 
	"%s %p: ...   1) DB_CURSOR_ID[0] %x,\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	qid_p->db_cursor_id[0]);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(cbuf, 
	"%s %p: ...   2) DB_CURSOR_ID[1] %x,\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	qid_p->db_cursor_id[1]);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qed_u0_trimtail(qid_p->db_cur_name, DB_CURSOR_MAXNAME, cur_name);

    STprintf(cbuf, 
	"%s %p: ...   3) DB_CUR_NAME     %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	cur_name);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(E_DB_OK);
}


/*{
** Name: qee_p4_ldb_qid - display the local query id 
**
** Description:
**
** Inputs:
**	v_qer_p->			ptr to QEF_RCB
**          qef_cb
**      i_dsh_p                         data segment header
**          .dsh_ddb_cb
**              .qee_d4_given_qid       query id
**
** Outputs:
**	none
**	
**	Returns:
**	    DB_STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-aug-90 (carl)
**          written
**	21-dec-92 (anitap)
**	    Prototyped.
*/


DB_STATUS
qee_p4_ldb_qid(
	QEF_RCB		*v_qer_p,
	QEE_DSH         *i_dsh_p)
{
/*
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ     *ddr_p = & v_qer_p->qef_r3_ddb_req;
*/
    QEE_DDB_CB      *qee_p = i_dsh_p->dsh_ddb_cb;
    DB_CURSOR_ID    *ldb_qid_p = & qee_p->qee_d5_local_qid;
    char	    cur_name[DB_CURSOR_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, 
	"%s %p: ...the LDB query id:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(cbuf, 
	"%s %p: ...   1) DB_CURSOR_ID[0] %x,\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	ldb_qid_p->db_cursor_id[0]);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(cbuf, 
	"%s %p: ...   2) DB_CURSOR_ID[1] %x,\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	ldb_qid_p->db_cursor_id[1]);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qed_u0_trimtail(ldb_qid_p->db_cur_name, DB_CURSOR_MAXNAME, cur_name);

    STprintf(cbuf, 
	"%s %p: ...   3) DB_CUR_NAME     %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	cur_name);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(E_DB_OK);
}


/*{
** Name: qee_p5_qids - display both the STAR and LDB id for the current query
**
** Description:
**
** Inputs:
**	v_qer_p->			ptr to QEF_RCB
**          qef_cb
**      i_dsh_p                         data segment header
**          .dsh_ddb_cb
**              .qee_d4_given_qid       STAR query id
**		.qee_d5_local_qid;	local query id
**
** Outputs:
**	none
**	
**	Returns:
**	    DB_STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-aug-90 (carl)
**          written
**	21-dec-92 (anitap)
*/


DB_STATUS
qee_p5_qids(
	QEF_RCB		*v_qer_p,
	QEE_DSH         *i_dsh_p)
{
    qee_p3_qry_id(v_qer_p, i_dsh_p);
    qee_p4_ldb_qid(v_qer_p, i_dsh_p);

    return(E_DB_OK);
}

