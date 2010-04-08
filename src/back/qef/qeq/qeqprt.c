/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <cs.h>
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

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>


/**
**  Name: QEQPRT.C - DDB query plan print routines
**
**  Description:
**	qeq_p0_tmp_tbl	- display temporary table used by an action
**	qeq_p1_sub_act	- display a SUBQUERY ACT
**	qeq_p2_get_act	- display a GET action
**	qeq_p3_xfr_act	- display a TRANSFER action
**	qeq_p4_lnk_act	- display a CREATE LINK action
**	qeq_p5_def_act	- display a DEFINE/CURSOR subquery
**	qeq_p6_inv_act	- display an INVOKE action
**	qeq_p7_agg_act	- display an AGGREGATE action
**
**	qeq_p10_prt_qp	- display query plan
**	qeq_p11_qp_hdrs	- display header of query plan
**	qeq_p12_qp_ldbs	- display all LDBs of query plan
**	qeq_p13_qp_vals	- display user-supplied parameters of query plan
**	qeq_p14_qp_acts	- display actions of a query plan
**	qeq_p15_prm_vals- display user-supplied parameter values
**
**	qeq_p20_sub_qry	- display a subquery
**	qeq_p21_hdr_gen	- display generic information from header of 
**				  query plan
**	qeq_p22_hdr_ddb	- display DDB-specific information from header 
**				  of query plan
**	qeq_p31_opn_csr	- display all actions of an OPEN (CURSOR) plan
**	qeq_p32_cls_csr	- display all actions of a CLOSE (CURSOR) plan
**	qeq_p33_fet_csr	- display all actions of a FETCH (CURSOR) plan
**	qeq_p34_del_csr	- display all actions of a DELTE (CURSOR) plan
**	qeq_p35_upd_csr	- display all actions of an UPDATE (CURSOR) plan
**
**
**  History:    $Log-for RCS$
**	03-jul-90 (carl)
**          written
**	27-oct-90 (carl)
**	    fixed to correctly display query parameters
**	27-oct-92 (fpang)
**	    In qeq_p22_hdr_ddb(), Supplied missing parameters to 
**	    qeq_p13_qp_vals().
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**      13-sep-93 (smc)
**          Corrected cast of qec_tprintf to PTR & used %p.
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
**          
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-feb-04 (inkdo01)
**	    Changed dsh_ddb_cb from QEE_DDB_CB instance to ptr.
**      09-jan-2009 (stial01)
**          Fix buffers that are dependent on DB_MAXNAME
**/


GLOBALREF   char    *IIQE_50_act_tab[];	    /* table of STAR action names 
					    ** defined in QEKCON.ROC */
GLOBALREF   char     IIQE_61_qefsess[];	    /* string "QEF session" defined in
					    ** QEKCON.ROC */
GLOBALREF   char     IIQE_63_qperror[];	    /* string "QP ERROR detected!" 
					    ** defined in QEKCON.ROC */

/*	static functions	*/

static DB_STATUS
qeq_p2_get_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEF_AHD		*act_p );


/*{
** Name: qeq_p1_sub_act - display the information of a subquery action
**			  
** Description:
**
** Inputs:
**      v_qer_p			    ptr to QEF_RCB
**      i1_dsh_p		    ptr to the DSH
**      i2_sub_p		    ptr to the QEF_ACT
**
** Outputs:
**      nothing
**        
**	Returns:
**	    	E_DB_OK
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-aug-90 (carl)
**          created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qeq_p1_sub_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEF_AHD		*i2_sub_p )
{
    DB_STATUS       status = E_DB_OK;
    QEQ_D1_QRY	    *qry_p = & i2_sub_p->qhd_obj.qhd_d1_qry;
    DD_LDB_DESC	    *ldb_p = qry_p->qeq_q5_ldb_p;
    QEQ_TXT_SEG	    *seg_p = (QEQ_TXT_SEG *) NULL;
    DD_PACKET	    *pkt_p = (DD_PACKET *) NULL;
    DB_DATA_VALUE   *dv_p = (DB_DATA_VALUE *) NULL;
    DD_NAME	    *tmpname_p = (DD_NAME *) NULL;
    i4		    dvcnt = 0,
		    lenleft,
		    lendone;
    char	    tmpname[DB_MAXNAME + 1];


    /* output query text to the log */

    qeq_p20_sub_qry(v_qer_p, i1_dsh_p, qry_p);

    return(E_DB_OK);
}


/*{
** Name: qeq_p2_get_act - display the information of a GET action
**			  
** Description:
**
** Inputs:
**      v_qer_p			    ptr to QEF_RCB
**      i1_dsh_p		    ptr to the DSH
**      i2_get_p		    ptr to the QEF_ACT
**
** Outputs:
**      nothing
**        
**	Returns:
**	    	E_DB_OK
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-aug-90 (carl)
**          created
**	12-jan-92 (rickh)
**	    Function prototyping fallout:  made args conform to caller.
*/


static DB_STATUS
qeq_p2_get_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEF_AHD		*act_p )
{
    DB_STATUS       status = E_DB_OK;
    QEQ_D2_GET	    *i2_get_p = & act_p->qhd_obj.qhd_d2_get;
    DD_LDB_DESC	    *ldb_p = i2_get_p->qeq_g1_ldb_p;


    if (ldb_p != (DD_LDB_DESC *) NULL)
	qed_w1_prt_ldbdesc(v_qer_p, ldb_p);
 
    return(E_DB_OK);
}


/*{
** Name: qeq_p3_xfr_act - display the information of a transfer action
**			  
** Description:
**
** Inputs:
**      v_qer_p			    ptr to QEF_RCB
**      i1_dsh_p		    ptr to the DSH
**      i2_xfr_p		    ptr to the QEQ_D3_XFR
**
** Outputs:
**      nothing
**        
**	Returns:
**	    	E_DB_OK
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-aug-90 (carl)
**          created
*/


DB_STATUS
qeq_p3_xfr_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEQ_D3_XFR	*i2_xfr_p )
{
    DB_STATUS	status = E_DB_OK;
    QEE_DDB_CB  *qee_p = i1_dsh_p->dsh_ddb_cb;
    QEQ_DDQ_CB  *ddq_p = & i1_dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEQ_D1_QRY	*src_p = & i2_xfr_p->qeq_x1_srce,
		*cpy_p = & i2_xfr_p->qeq_x3_sink,
		*tmp_p = & i2_xfr_p->qeq_x2_temp;
    DD_PACKET   *pkt_p;
    i4          temp_slot;
    DD_NAME	*name_p = (DD_NAME *) NULL;
    char	name_str[DB_MAXNAME + 1],
		copy_qry[QEK_100_LEN + DB_MAXNAME];
    char	*cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		cbufsize = v_qer_p->qef_cb->qef_trsize;

    /*
    ** 1.  display source query information
    */

    STprintf(cbuf, 
	"%s %p: ...X1) the source information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qeq_p20_sub_qry(v_qer_p, i1_dsh_p, src_p);

    /*
    ** 2.  display temporary table name
    */

    STprintf(cbuf, 
	"%s %p: ...X2) the temporary-table information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    for (pkt_p = tmp_p->qeq_q4_seg_p->qeq_s2_pkt_p;
         pkt_p->dd_p2_pkt_p != NULL;
         pkt_p = pkt_p->dd_p3_nxt_p);

    temp_slot = pkt_p->dd_p4_slot;
 
    name_p = qee_p->qee_d1_tmp_p + temp_slot;

    qed_u0_trimtail((char *) name_p, (u_i4) sizeof(*name_p), name_str); 

    STprintf(cbuf, 
	"%s %p: ...LDB for creating table %s:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb,
	name_str);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
 
    qed_w1_prt_ldbdesc(v_qer_p, tmp_p->qeq_q5_ldb_p);
 
    /*
    ** 3.  display "copy from" query information
    */

    STprintf(cbuf, 
	"%s %p: ...X3) the destination information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(cbuf, 
	"%s %p: ...destination LDB:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qed_w1_prt_ldbdesc(v_qer_p, tmp_p->qeq_q5_ldb_p);

    STprintf(cbuf, 
	"%s %p: ...destination query:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(
	copy_qry,
	"copy table %s () from 'junk';",
	name_str);

    STprintf(cbuf, 
	"%s %p: ...   %s\n\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb,
	copy_qry);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(E_DB_OK);
}


/*{
** Name: qeq_p5_def_act - display the information of a DEFINE/CURSOR action
**			  
** Description:
**
** Inputs:
**      v_qer_p			    ptr to QEF_RCB
**      i1_dsh_p		    ptr to the DSH
**      i2_def_p		    ptr to the QEF_AHD
**
** Outputs:
**      nothing
**        
**	Returns:
**	    	E_DB_OK
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-aug-90 (carl)
**          created
**	27-oct-90 (carl)
**	    fixed to correctly display query parameters
*/


DB_STATUS
qeq_p5_def_act(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEF_AHD		*i2_def_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEE_DDB_CB	    *qee_p = i1_dsh_p->dsh_ddb_cb;
    QEQ_DDQ_CB	    *ddq_p = & i1_dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEQ_D1_QRY	    *sub_p = & i2_def_p->qhd_obj.qhd_d1_qry;
    QEF_PARAM	    *prm_p = (QEF_PARAM *) NULL;
    DB_DATA_VALUE   **dv_pp = (DB_DATA_VALUE **) NULL;
    i4		    temp_slot;


    qee_p5_qids(v_qer_p, i1_dsh_p);
 
    qeq_p20_sub_qry(v_qer_p, i1_dsh_p, sub_p);

    if (sub_p->qeq_q8_dv_cnt == 0)
    {
	/* DEFINE QUERY */

        if (v_qer_p->qef_pcount > 0)
	{
	    prm_p = (QEF_PARAM *) v_qer_p->qef_param;
	    dv_pp = (DB_DATA_VALUE **) prm_p->dt_data + 1;
					/* point to first parameter */
	    qeq_p15_prm_vals(v_qer_p, i1_dsh_p, v_qer_p->qef_pcount,
		dv_pp);
					/* user-defined parameters 
					** for DEFINE QUERY */
	}
    }
    else
    {
	/* OPEN CURSOR */

        if (sub_p->qeq_q8_dv_cnt > 0)
	    qeq_p13_qp_vals(v_qer_p, i1_dsh_p, sub_p->qeq_q8_dv_cnt,
		(DB_DATA_VALUE *) (qee_p->qee_d7_dv_p + 1));
                                                /* (1-based) parameters for 
                                                ** OPEN CURSOR */
    }

    return(E_DB_OK);
}


/*{
** Name: qeq_p6_inv_act - display information if an INVOKE action 
**
** Description:
**      This routine displays information of:
**	    1) the query id, and
**	    2) the target LDB
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_act_p			    ptr to the action header
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
**	27-oct-90 (carl)
**	    fixed to correctly display REPEAT query invocation parameters
*/


DB_STATUS
qeq_p6_inv_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEE_DSH         *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
				    /* current query */
    QEE_DDB_CB      *qee_p = dsh_p->dsh_ddb_cb;
    QEQ_D1_QRY      *sub_p = & i_act_p->qhd_obj.qhd_d1_qry;
    QEF_PARAM	    *prm_p = (QEF_PARAM *) NULL;
    DB_DATA_VALUE   **dv_pp = (DB_DATA_VALUE **) NULL;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (sub_p->qeq_q3_ctl_info & QEQ_002_USER_UPDATE)
    {
	STprintf(cbuf, 
	    "%s %p: ...invoking an UPDATE repeat query on:\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: ...invoking a READ repeat query on:\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    qed_w1_prt_ldbdesc(v_qer_p, sub_p->qeq_q5_ldb_p);

    qee_p5_qids(v_qer_p, dsh_p);

    if (v_qer_p->qef_pcount > 0)
    {
	prm_p = (QEF_PARAM *) v_qer_p->qef_param;
	dv_pp = (DB_DATA_VALUE **) prm_p->dt_data + 1;
					/* point to first parameter */
	qeq_p15_prm_vals(v_qer_p, dsh_p, v_qer_p->qef_pcount,
		dv_pp);
					/* user-defined parameters 
					** for DEFINE QUERY */
    }
    return(E_DB_OK);
}


/*{
** Name: qeq_p10_prt_qp - display query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
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
*/


DB_STATUS
qeq_p10_prt_qp(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (dds_p->qes_d9_ctl_info & QES_04CTL_OK_TO_OUTPUT_QP)
	dds_p->qes_d9_ctl_info &= ~QES_04CTL_OK_TO_OUTPUT_QP;
				/* reset to flag QP written to the log to 
				** avoid redundant writing of this same QP */
    else
	return(E_DB_OK);	/* QP already written to the log */

    STprintf(cbuf, 
	"%s %p: Query plan information...\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
/*
    ** display the DSH information **

    STprintf(cbuf, 
	"%s %p: 1) Data segment header information\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qee_p0_prt_dsh(v_qer_p, i_dsh_p);
*/
    /* display the QP header information */

    STprintf(cbuf, 
	"%s %p: 1) Query plan header information\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qeq_p11_qp_hdrs(v_qer_p, i_dsh_p);

    /* display the QP information */

    if (qp_p->qp_status & QEQP_RPT)
    {
	STprintf(cbuf, 
	    "%s %p: 2) Repeat query information\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: 2) Query plan information\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    status = qeq_p14_qp_acts(v_qer_p, i_dsh_p);
/*
    STprintf(cbuf, 
	"%s %p: Query information display ends.\n\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
*/
    return(status);
}


/*{
** Name: qeq_p11_qp_hdrs - display header information of query plan
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
*/


DB_STATUS
qeq_p11_qp_hdrs(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    /* Part 1: print generic information from header of QP */

    STprintf(cbuf, 
	"%s %p: ...1.1) General QP header information\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    status = qeq_p21_hdr_gen(v_qer_p, i_dsh_p);
    if (status)
	return(status);

    /* Part 2: print DDB-specific information from header of QP */

    STprintf(cbuf, 
	"%s %p: ...1.2) DDB-specific QP header information\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    status = qeq_p22_hdr_ddb(v_qer_p, i_dsh_p);

    return(status);
}


/*{
** Name: qeq_p12_qp_ldbs - display header info and actions of repeat query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
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
*/


DB_STATUS
qeq_p12_qp_ldbs(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;
    QEQ_LDB_DESC    *curldb_p = ddq_p->qeq_d2_ldb_p;
    i4		    ldbcnt;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (curldb_p == (QEQ_LDB_DESC *) NULL)
	return(E_DB_OK);

    ldbcnt = 0; 
    while (curldb_p != (QEQ_LDB_DESC *) NULL)
    {
	++ldbcnt;
	curldb_p = curldb_p->qeq_l2_nxt_ldb_p;	/* advance to next if any */

    }

    STprintf(cbuf, 
	"%s %p: ...number of LDBs used by this query: %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	ldbcnt);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    ldbcnt = 0; 
    while (curldb_p != (QEQ_LDB_DESC *) NULL)
    {
	++ldbcnt;
	STprintf(cbuf, 
	    "%s %p: ...LDB (%d):\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    ldbcnt);
	qec_tprintf(v_qer_p, cbufsize, cbuf);

	qed_w1_prt_ldbdesc(v_qer_p, & curldb_p->qeq_l1_ldb_desc);

	curldb_p = curldb_p->qeq_l2_nxt_ldb_p;	/* advance to next if any */

    }

    return(E_DB_OK);
}


/*{
** Name: qeq_p13_qp_vals - display header info and actions of repeat query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to the DSH
**	i_dvcnt			    number of DB_DATA_VALUEs
**	i_dvptr			    DB_DATA_VALUE array ptr
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
*/


DB_STATUS
qeq_p13_qp_vals(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p,
i4		 i_dvcnt,
DB_DATA_VALUE	*i_dv_p )
{
/*
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
*/
    i4		    i;
    DB_DATA_VALUE   *dv_p;    
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (i_dvcnt <= 0)
	return(E_DB_OK);

    STprintf(cbuf, 
	"%s %p: ...number of data values: %d\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    i_dvcnt);

    for (i = 0, dv_p = i_dv_p; i < i_dvcnt; ++i, dv_p = dv_p + 1)
    {
	STprintf(cbuf, 
	    "%s %p: ...   data value %d)\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    i + 1);
	qec_tprintf(v_qer_p, cbufsize, cbuf);

	qed_w9_prt_db_val(v_qer_p, dv_p);
    }
    return(E_DB_OK);
}


/*{
** Name: qeq_p14_qp_acts - display all actions of non-repeat query plan
**
** Description:
**      This routine traverses given query plan and displays all the actions.
**	It can be called directly.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i1_dsh_p		    ptr to the DSH
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
*/


DB_STATUS
qeq_p14_qp_acts(
QEF_RCB		*v_qer_p,
QEE_DSH		*i1_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i1_dsh_p->dsh_qp_ptr;
    QEF_AHD	    *act_p = qp_p->qp_ahd,
		    *nxt_p;
    QEQ_D1_QRY	    *cur_p,
		    *src_p,
		    *tmp_p,
		    *des_p;
    QEQ_D3_XFR      *xfr_p = (QEQ_D3_XFR *) NULL;
    QEQ_TXT_SEG     *seg_p;
    i4		    sel_ldb,
		    act_cnt,
		    i;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;

    sel_ldb= 0;
    act_p = qp_p->qp_ahd; 

    /* loop thru all actions */

    for (act_cnt = 0; act_p != (QEF_AHD *) NULL; ++act_cnt)
    {
	if (act_p->ahd_atype == QEA_D1_QRY)
	{
	    cur_p = & act_p->qhd_obj.qhd_d1_qry;
	    if (cur_p->qeq_q3_ctl_info & QEQ_003_ROW_COUNT_FROM_LDB)
		sel_ldb++;
	}

        switch (act_p->ahd_atype)
	{
	case QEA_D1_QRY:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) SUBQUERY action:\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    cur_p = & act_p->qhd_obj.qhd_d1_qry;	
	    seg_p = cur_p->qeq_q4_seg_p;

	    if (cur_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
	    {
		/* successor MUST be QEA_D2_GET action */
			
		nxt_p = act_p->ahd_next;

		if (nxt_p == NULL)
		{
		    qed_w6_prt_qperror(v_qer_p);

		    STprintf(cbuf, 
			"%s %p: ...expected GET action MISSING!\n",
			IIQE_61_qefsess,
			(PTR) v_qer_p->qef_cb);
		    qec_tprintf(v_qer_p, cbufsize, cbuf);
		}
		else 
		{
		    if (nxt_p->ahd_atype != QEA_D2_GET)
		    {
			qed_w6_prt_qperror(v_qer_p);	/* banner line */

			STprintf(cbuf, 
			    "%s %p: ...expected GET action MISSING!\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
			qec_tprintf(v_qer_p, cbufsize, cbuf);
		    }
		    if (cur_p->qeq_q5_ldb_p 
			!= 
			nxt_p->qhd_obj.qhd_d2_get.qeq_g1_ldb_p)
		    {
			qed_w6_prt_qperror(v_qer_p);	/* banner line */

			STprintf(cbuf, 
"%s %p: ...RETRIEVAL and GET actions on inconsistent LDBs!\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
			qec_tprintf(v_qer_p, cbufsize, cbuf);

			STprintf(cbuf, 
			    "%s %p: ...RETRIEVAL action on\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
			qec_tprintf(v_qer_p, cbufsize, cbuf);
			qed_w1_prt_ldbdesc(v_qer_p, cur_p->qeq_q5_ldb_p);

			STprintf(cbuf, 
			    "%s %p: ...GET action on\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
			qec_tprintf(v_qer_p, cbufsize, cbuf);
			qed_w1_prt_ldbdesc(v_qer_p, 
			    nxt_p->qhd_obj.qhd_d2_get.qeq_g1_ldb_p);
		    }
		    if (seg_p == (QEQ_TXT_SEG *) NULL)
		    {
			qed_w6_prt_qperror(v_qer_p);	/* banner line */

			STprintf(cbuf, 
			    "%s %p: ...QUERY TEXT missing\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
			qec_tprintf(v_qer_p, cbufsize, cbuf);
		    }
		    else
		    {
			if (! seg_p->qeq_s1_txt_b)
			{
			    qed_w6_prt_qperror(v_qer_p);   /* banner line */

			    STprintf(cbuf, 
				"%s %p: ...QUERY TEXT MISSING\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
			    qec_tprintf(v_qer_p, cbufsize, cbuf);
			}
			if (seg_p->qeq_s2_pkt_p == (DD_PACKET *) NULL)
			{
			    qed_w6_prt_qperror(v_qer_p);   /* banner line */

			    STprintf(cbuf, 
				"%s %p: ...QUERY TEXT pointer MISSING\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
			    qec_tprintf(v_qer_p, cbufsize, cbuf);
			}
			else
			{
			    if (! seg_p->qeq_s2_pkt_p->dd_p1_len > 0)
			    {
				qed_w6_prt_qperror(v_qer_p);/* banner line */

				STprintf(cbuf, 
				    "%s %p: ...QUERY TEXT has ZERO length\n",
				    IIQE_61_qefsess,
				    (PTR) v_qer_p->qef_cb);
				qec_tprintf(v_qer_p, cbufsize, cbuf);
			    }
			    if (seg_p->qeq_s2_pkt_p->dd_p2_pkt_p == NULL)
			    {
				qed_w6_prt_qperror(v_qer_p);/* banner line */

				STprintf(cbuf, 
				    "%s %p: ...QUERY TEXT pointer MISSING\n",
				    IIQE_61_qefsess,
				    (PTR) v_qer_p->qef_cb);
				    qec_tprintf(v_qer_p, cbufsize, cbuf);
			    }
			}
		    }
		}
	    }
	    qeq_p1_sub_act(v_qer_p, i1_dsh_p, act_p);
	    break;
	    
	case QEA_D2_GET:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) GET action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    qeq_p2_get_act(v_qer_p, i1_dsh_p, act_p);
	    break;

	case QEA_D3_XFR:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) TRANSFER action:\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    /* QEQ_D3_XFR consists of 1) a SELECT/RETRIEVE query from the 
	    ** source LDB, 2) a template for creating a temporary table and 
	    ** 3) a COPY FROM query for the destination LDB */

	    xfr_p = & act_p->qhd_obj.qhd_d3_xfr;

	    src_p = & xfr_p->qeq_x1_srce;
	    tmp_p = & xfr_p->qeq_x2_temp;
	    des_p = & xfr_p->qeq_x3_sink;

	    /* 1. check source LDB and subquery */

	    if (src_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
	    {
		/* error: tuples are MEANT for the destination site, 
		** NOT SCF! */

		qed_w6_prt_qperror(v_qer_p);/* banner line */

		STprintf(cbuf, 
		    "%s %p: ...SOURCE LDB returning result to SCF!\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
	        qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }

	    /* 2. check source LDB and subquery */

	    if (tmp_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
	    {
		/* error: CANNOT be a RETRIEVAL action ! */

		qed_w6_prt_qperror(v_qer_p);/* banner line */

		STprintf(cbuf, 
		    "%s %p: ...RETRIEVING from the destination LDB!\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
		qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }
	    if (! (tmp_p->qeq_q6_col_cnt > 0))
	    {
		/* error: TEMPORARY TABLE must have non-zero column count! */

		qed_w6_prt_qperror(v_qer_p);/* banner line */

		STprintf(cbuf, 
		    "%s %p: ...TEMPORARY TABLE has ZERO column count!\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
		qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }
	    if (tmp_p->qeq_q7_col_pp == NULL)
	    {
		/* error: TEMPORARY TABLE has NO column names! */

		qed_w6_prt_qperror(v_qer_p);/* banner line */

		STprintf(cbuf, 
		    "%s %p: ...TEMPORARY TABLE has NO column names!\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
		qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }

	    /* 3. check source LDB and subquery */

	    if (des_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
	    {
		/* error: CANNOT be a RETRIEVAL action ! */

		qed_w6_prt_qperror(v_qer_p);/* banner line */

		STprintf(cbuf, 
		    "%s %p: ...RETRIEVING from the destination LDB!\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
		qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }
	    if (des_p->qeq_q5_ldb_p != tmp_p->qeq_q5_ldb_p)
	    {
		/* error: LDB inconsistency! */
	
		qed_w6_prt_qperror(v_qer_p);	/* banner line */

		STprintf(cbuf, 
"%s %p: ...CREATE TABLE and COPY FROM on different LDBs!\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
		qec_tprintf(v_qer_p, cbufsize, cbuf);

		STprintf(cbuf, 
		    "%s %p: ...CREATE TABLE on\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
		qec_tprintf(v_qer_p, cbufsize, cbuf);
		qed_w1_prt_ldbdesc(v_qer_p, tmp_p->qeq_q5_ldb_p);

		STprintf(cbuf, 
		    "%s %p: ...COPY FROM on\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
		qec_tprintf(v_qer_p, cbufsize, cbuf);
		qed_w1_prt_ldbdesc(v_qer_p, des_p->qeq_q5_ldb_p);
	    }
	    STprintf(cbuf, 
		"%s %p: ...2.%d) TRANSFER action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    qeq_p3_xfr_act(v_qer_p, i1_dsh_p, xfr_p);
	    break;

	case QEA_D4_LNK:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) CREATE LINK action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);
	    break;

	case QEA_D5_DEF:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) DEFINE QUERY action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    qeq_p5_def_act(v_qer_p, i1_dsh_p, act_p);

	    break;

	case QEA_D6_AGG:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) AGGREGATE action:\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    cur_p = & act_p->qhd_obj.qhd_d1_qry;
	    if (! (cur_p->qeq_q6_col_cnt > 0))
	    {
		/* error: COLUMN count CANNOT be 0! */

		qed_w6_prt_qperror(v_qer_p);	/* banner line */

		STprintf(cbuf, 
"%s %p: ...AGGREGATE RETRIEVAL CANNOT have ZERO column count!\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb);
	        qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }
	    STprintf(cbuf, 
		"%s %p: ...2.%d) AGGREGATE action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    break;

	case QEA_D7_OPN:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) OPEN CURSOR action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    qeq_p5_def_act(v_qer_p, i1_dsh_p, act_p);

	    break;

	case QEA_D8_CRE:

	    STprintf(cbuf, 
		"%s %p: ...2.%d) CREATE TABLE action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    break;

	case QEA_D9_UPD:
	    STprintf(cbuf, 
		"%s %p: ...2.%d) UPDATE action\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		act_cnt + 1);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);

	    break;

	default:
	    qed_w6_prt_qperror(v_qer_p);	/* banner line */

	    STprintf(cbuf, 
		"%s %p: ...UNRECOGGNIZED ACTION detected!\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb);
	    qec_tprintf(v_qer_p, cbufsize, cbuf);
	    break;
	}
	act_p = act_p->ahd_next;	/* advance */
    }

    if (act_cnt != qp_p->qp_ahd_cnt)
    {
	qed_w6_prt_qperror(v_qer_p);	/* banner line */

	STprintf(cbuf, 
	    "%s %p: ...INCONSISTENT action count detected!\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    if (sel_ldb > 1)
    {
	qed_w6_prt_qperror(v_qer_p);	/* banner line */

	STprintf(cbuf, 
	    "%s %p: ...SELECTING from MORE than 1 LDB!\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    return(E_DB_OK);
}


/*{
** Name: qeq_p15_prm_vals - display parameter values
**
** Description:
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to the DSH
**	i_dvcnt			    number of DB_DATA_VALUEs
**	i_dv_pp			    ptr to array of DB_DATA_VALUE ptrs
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
**	27-oct-90 (carl)
**          written
*/


DB_STATUS
qeq_p15_prm_vals(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p,
i4		 i_dvcnt,
DB_DATA_VALUE  **i_dv_pp )
{
    i4		    i;
    DB_DATA_VALUE   **dv_pp = i_dv_pp,
		    *dv_p = (DB_DATA_VALUE *) NULL;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (i_dvcnt <= 0)
	return(E_DB_OK);

    STprintf(cbuf, 
	"%s %p: ...number of data values: %d\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    i_dvcnt);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    for (i = 0; i < i_dvcnt; ++i, dv_pp++)
    {
	dv_p = *dv_pp;
	STprintf(cbuf, 
	    "%s %p: ...   data value %d)\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    i + 1);
	qec_tprintf(v_qer_p, cbufsize, cbuf);

	qed_w9_prt_db_val(v_qer_p, dv_p);
    }
    return(E_DB_OK);
}


/*{
** Name: qeq_p20_sub_qry - display the information of a subquery 
**			  
** Description:
**
** Inputs:
**      v_qer_p			    ptr to QEF_RCB
**      i1_dsh_p		    ptr to the DSH
**      i1_sub_p		    ptr to the QEQ_D1_QRY
**
** Outputs:
**      nothing
**        
**	Returns:
**	    	E_DB_OK
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      02-aug-90 (carl)
**          created
**	27-oct-90 (carl)
**	    fixed to correctly display query parameters
*/


DB_STATUS
qeq_p20_sub_qry(
QEF_RCB         *v_qer_p,
QEE_DSH         *i1_dsh_p,
QEQ_D1_QRY	*i2_sub_p )
{
    DB_STATUS       status = E_DB_OK;
    QEE_DDB_CB	    *qee_p = i1_dsh_p->dsh_ddb_cb;
    QEQ_TXT_SEG	    *seg_p = (QEQ_TXT_SEG *) NULL;
    DD_PACKET	    *pkt_p = (DD_PACKET *) NULL;
    DD_LDB_DESC	    *ldb_p = i2_sub_p->qeq_q5_ldb_p;
    QEF_PARAM	    *prm_p = (QEF_PARAM *) NULL;
    DB_DATA_VALUE   **dv_pp = (DB_DATA_VALUE **) NULL;
    DD_NAME	    *tmpname_p = (DD_NAME *) NULL;
    i4		    dvcnt = 0;
    char	    tmpname[DB_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, 
	"%s %p: ...the target LDB:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qed_w1_prt_ldbdesc(v_qer_p, ldb_p);

    /* output query text to the log */

    STprintf(cbuf, 
	"%s %p: ...the query text:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    seg_p = i2_sub_p->qeq_q4_seg_p;
    while (seg_p != (QEQ_TXT_SEG *) NULL)
    {
	/* output query text segment to the log */

	pkt_p = seg_p->qeq_s2_pkt_p;
	while (pkt_p != (DD_PACKET *) NULL)
	{
	    if (pkt_p->dd_p4_slot < 0)
	    {
		/* query text in hand */

		status = qed_w10_prt_qrytxt(
			    v_qer_p, 
			    pkt_p->dd_p2_pkt_p, 
			    pkt_p->dd_p1_len);
		if (status)
		    return(status);
	    }
	    else	
	    {
		/* a temporary table */

		if (pkt_p->dd_p4_slot < 0)
		{
		    STprintf(cbuf, 
			"%s %p: ...%s\n",
			IIQE_61_qefsess,
			(PTR) v_qer_p->qef_cb,
			IIQE_63_qperror);
		    qec_tprintf(v_qer_p, cbufsize, cbuf);

		    return(qed_u1_gen_interr(& v_qer_p->error));
		}

		tmpname_p = i1_dsh_p->dsh_ddb_cb->qee_d1_tmp_p + 
				pkt_p->dd_p4_slot;
		MEcopy((PTR) tmpname_p, DB_MAXNAME, (PTR) tmpname);
		tmpname[DB_MAXNAME] = EOS;

		STprintf(cbuf, 
		    "%s %p: ...   %s\n",
		    IIQE_61_qefsess,
		    (PTR) v_qer_p->qef_cb,
		    tmpname);
		qec_tprintf(v_qer_p, cbufsize, cbuf);
	    }
	    pkt_p = pkt_p->dd_p3_nxt_p;	/* advance to next packet */
	}
	seg_p = seg_p->qeq_s3_nxt_p;	/* advance to next segment */
    }

    /* output binary data descriptions if any */
/*
    STprintf(cbuf, 
	"%s %p: ...number of data values: %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	i2_sub_p->qeq_q8_dv_cnt);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
*/
    if (i2_sub_p->qeq_q8_dv_cnt == 0)
    {
	/* DEFINE QUERY */

        if (v_qer_p->qef_pcount > 0)
	{
	    prm_p = (QEF_PARAM *) v_qer_p->qef_param;
	    dv_pp = (DB_DATA_VALUE **) prm_p->dt_data + 1;
					/* point to first parameter */
	    qeq_p15_prm_vals(v_qer_p, i1_dsh_p, v_qer_p->qef_pcount,
		dv_pp);
					/* user-defined parameters 
					** for DEFINE QUERY */
	}
    }
    else
    {
	/* OPEN CURSOR */

        if (i2_sub_p->qeq_q8_dv_cnt > 0)
	    qeq_p13_qp_vals(v_qer_p, i1_dsh_p, i2_sub_p->qeq_q8_dv_cnt,
		(DB_DATA_VALUE *) (qee_p->qee_d7_dv_p + 1));
                                                /* (1-based) parameters for 
                                                ** OPEN CURSOR */
    }

    return(E_DB_OK);
}


/*{
** Name: qeq_p21_hdr_gen - display generic header information of query plan
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
*/


DB_STATUS
qeq_p21_hdr_gen(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    char	    qpname[DB_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    /* null-terminate name part for printing */

    qed_u0_trimtail(qp_p->qp_id.db_cur_name, (u_i4) DB_MAXNAME,
	qpname);

    STprintf(cbuf, 
	"%s %p: ...query plan id: %d %d %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb, 
	qp_p->qp_id.db_cursor_id[0],
	qp_p->qp_id.db_cursor_id[1],
	qpname);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    if (qp_p->qp_status & QEQP_RPT)
    {
	STprintf(cbuf, 
	    "%s %p: ...a REPEAT query\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (qp_p->qp_status & QEQP_UPDATE)
    {
	STprintf(cbuf, 
	    "%s %p: ...UPDATE allowed for cursor\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (qp_p->qp_status & QEQP_DELETE)
    {
	STprintf(cbuf, 
	    "%s %p: ...DELETE allowed for cursor\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    if (qp_p->qp_status & QEQP_SINGLETON)
    {
	STprintf(cbuf, 
	    "%s %p: ...at most 1 row will be returned\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    switch (qp_p->qp_qmode)
    {
    case QEQP_01QM_RETRIEVE:
	STprintf(cbuf, 
	    "%s %p: ...query mode: %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    "RETRIEVE");
	qec_tprintf(v_qer_p, cbufsize, cbuf);
	STprintf(cbuf, 
	    "%s %p: ...result tuple size (bytes): %d\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb, 
	    qp_p->qp_res_row_sz);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
	break;
    case QEQP_02QM_DEFCURS:
	STprintf(cbuf, 
	    "%s %p: ...query mode: %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    "DEFINE CURSOR");
	qec_tprintf(v_qer_p, cbufsize, cbuf);
	break;
    case QEQP_03QM_RETINTO:
	STprintf(cbuf, 
	    "%s %p: ...query mode: %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    "RETRIEVE INTO");
	qec_tprintf(v_qer_p, cbufsize, cbuf);
	break;
    default:
	break;
    }
    STprintf(cbuf, 
	"%s %p: ...action count: %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	qp_p->qp_ahd_cnt);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(E_DB_OK);
}


/*{
** Name: qeq_p22_hdr_ddb - display DDB-specific header information of query plan
**
** Description:
**      This routine displays the information from the QEQ_DDQ_CB part of
**	the query plan header.
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
**	27-oct-90 (carl)
**	    fixed to correctly display query parameters
**	27-oct-92 (fpang)
**	    Supplied missing parameters to qeq_p13_qp_vals().
**
*/


DB_STATUS
qeq_p22_hdr_ddb(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;
/*
    QEE_DDB_CB	    *qee_p = i_dsh_p->dsh_ddb_cb;
*/
    char	    tblname[DB_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (ddq_p->qeq_d1_end_p != (QEF_AHD *) NULL)
    {
	/* display terminating queries which drop all temporary tables */
    }

    if (ddq_p->qeq_d2_ldb_p != (QEQ_LDB_DESC *) NULL)
    {
	status = qeq_p12_qp_ldbs(v_qer_p, i_dsh_p);
    }

    STprintf(cbuf, 
"%s %p: ...number of temporary table(s) and attribute name(s): %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	ddq_p->qeq_d3_elt_cnt);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
/*
    ** cannot trust these numbers! **

    STprintf(cbuf, 
	"%s %p: ...total number of parameters for query: %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	ddq_p->qeq_d4_total_cnt);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(cbuf, 
	"%s %p: ...number of user-supplied parameters: %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	ddq_p->qeq_d5_fixed_cnt);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
*/
    if (ddq_p->qeq_d5_fixed_cnt > 0)
    {
	status = qeq_p13_qp_vals(v_qer_p, i_dsh_p, 
				 ddq_p->qeq_d5_fixed_cnt,
				 (DB_DATA_VALUE *) ddq_p->qeq_d6_fixed_data );
    }

    if (ddq_p->qeq_d7_deltable != (DD_NAME *) NULL)
    {
	qed_u0_trimtail( ( char * ) ddq_p->qeq_d7_deltable, (u_i4) DB_MAXNAME,
	    tblname);

	STprintf(cbuf, 
	    "%s %p: ...LDB table name for cursor delete: %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    tblname);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    if (ddq_p->qeq_d8_mask & QEQ_D8_INGRES_B)
    {
	STprintf(cbuf, 
	    "%s %p: ...query rerouted thru the CDB association,\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
	STprintf(cbuf, 
	    "%s %p: ...   to be committed when done\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
	qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    return(E_DB_OK);
}


/*{
** Name: qeq_p31_opn_csr - display header info and actions of repeat query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
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
*/


DB_STATUS
qeq_p31_opn_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
/*
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
*/
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, 
	"%s %p: ...OPEN CURSOR query information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qee_p5_qids(v_qer_p, i_dsh_p);
    
    qeq_p10_prt_qp(v_qer_p, i_dsh_p);

    STprintf(cbuf, 
	"%s %p:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(E_DB_OK);
}


/*{
** Name: qeq_p32_cls_csr - display header info and actions of repeat query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
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
*/


DB_STATUS
qeq_p32_cls_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
/*
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    DB_CURSOR_ID    *qid_p = (DB_CURSOR_ID *) NULL;
*/
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, 
	"%s %p: ...CLOSE CURSOR query information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qee_p5_qids(v_qer_p, i_dsh_p);
    
    return(E_DB_OK);
}


/*{
** Name: qeq_p33_fet_csr - display header info and actions of repeat query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
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
*/


DB_STATUS
qeq_p33_fet_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
/*
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
*/
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;

    STprintf(cbuf, 
	"%s %p: FETCH CURSOR query information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qee_p5_qids(v_qer_p, i_dsh_p);
    
    return(E_DB_OK);
}


/*{
** Name: qeq_p34_del_csr - display header info and actions of repeat query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
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
*/


DB_STATUS
qeq_p34_del_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
/*
    DB_STATUS	    status = E_DB_OK;
*/
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
    QEQ_DDQ_CB      *ddq_p = & qp_p->qp_ddq_cb;
    char	    deltable[DB_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, 
	"%s %p: DELETE CURSOR query information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qee_p5_qids(v_qer_p, i_dsh_p);
    
    qed_u0_trimtail( ( char * ) ddq_p->qeq_d7_deltable,
        (u_i4) DB_MAXNAME,
        deltable);

    STprintf(cbuf, 
	"%s %p: ...target table %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	deltable);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(E_DB_OK);
}


/*{
** Name: qeq_p35_upd_csr - display header info and actions of repeat query plan
**
** Description:
**      This routine displays information of:
**	    1) the query plan header, and
**	    2) all the actions by traversing the given query plan.
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
**	27-oct-90 (carl)
**	    fixed to correctly display REPEAT QUERY invocation parameters
*/


DB_STATUS
qeq_p35_upd_csr(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
/*
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
*/
    QEE_DDB_CB      *qee_p = i_dsh_p->dsh_ddb_cb;
    QEE_DSH         *ree_p = i_dsh_p->dsh_aqp_dsh;
                                                /* must use update dsh for
                                                ** cursor */
    QEE_DDB_CB      *uee_p = ree_p->dsh_ddb_cb;
    DB_CURSOR_ID    *qid_p = & uee_p->qee_d5_local_qid;
    QES_DDB_SES     *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES     *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEQ_DDQ_CB      *ddq_p = & i_dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEF_AHD         *act_p = i_dsh_p->dsh_qp_ptr->qp_ahd;
    QEQ_D1_QRY      *sub_p = & act_p->qhd_obj.qhd_d1_qry;
    QEF_PARAM	    *prm_p = (QEF_PARAM *) NULL;
    DB_DATA_VALUE   **dv_pp = (DB_DATA_VALUE **) NULL;
    char            cur_name[DB_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;
 

    STprintf(cbuf, 
	"%s %p: UPDATE CURSOR query information:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
    
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

    qed_u0_trimtail(qid_p->db_cur_name, (u_i4) DB_MAXNAME, cur_name);

    STprintf(cbuf, 
	"%s %p: ...   3) DB_CUR_NAME     %s\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb,
        cur_name);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
 
    qeq_p20_sub_qry(v_qer_p, i_dsh_p, sub_p);

    if (sub_p->qeq_q8_dv_cnt == 0)
    {
        if (v_qer_p->qef_pcount > 0)
	{
	    prm_p = (QEF_PARAM *) v_qer_p->qef_param;
	    dv_pp = (DB_DATA_VALUE **) prm_p->dt_data + 1;
					/* point to first parameter */
	    qeq_p15_prm_vals(v_qer_p, i_dsh_p, v_qer_p->qef_pcount,
		dv_pp);
					/* user-defined parameters 
					** for DEFINE QUERY */
	}
    }
    else
    {
        if (sub_p->qeq_q8_dv_cnt > 0)
	    qeq_p13_qp_vals(v_qer_p, i_dsh_p, ddq_p->qeq_d4_total_cnt,
		(DB_DATA_VALUE *) ddq_p->qeq_d6_fixed_data);
                                                /* OPC parameters */
    }

    return(E_DB_OK);
}

