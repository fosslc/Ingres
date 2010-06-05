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
#include    <qefqeu.h>
#include    <tpf.h>
#include    <rqf.h>
#include    <qefqry.h>
#include    <qefcat.h>

#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qeuqcb.h>
#include    <sxf.h>
#include    <qefprotos.h>


GLOBALREF   char    *IIQE_50_act_tab[];     /* table of STAR action names
                                            ** defined in QEKCON.ROC */
GLOBALREF   char     IIQE_61_qefsess[];     /* string "QEF session" defined in
                                            ** QEKCON.ROC */
GLOBALREF   char     IIQE_63_qperror[];     /* string "QP ERROR detected!"
                                            ** defined in QEKCON.ROC */
    

/**
**  Name: QEQDDB.C - Routines for DDB Query Execution Scheme
**
**  Description:
**      These routines implement QEF's part of the Query Execution Scheme:
**
**	    qeq_d0_val_act	- validate a DDB query action (called from 
**				  qeq_query in QEQ.C)
**	    qeq_d1_qry_act	- process a QEA_D1_QRY action (called from 
**				  qeq_query in QEQ.C)
**	    qeq_d2_get_act	- process a QEA_D2_GET action (called from 
**				  qef_call)
**	    qeq_d3_end_act	- process QEQ_ENDRET for Titan (called from 
**				  qeq_endret in QEQ.C)
**	    qeq_d4_sub_act	- process a subquery
**	    qeq_d5_xfr_act	- process internal data transfer 
**	    qeq_d6_tmp_act	- drop all temporary tables created to support
**				  internal data transfer
**	    qeq_d7_rpt_act	- process a single-site repeat subquery action
**	    qeq_d8_def_act	- process a DEFINE QUERY for repeat/cursor 
**				  subquery
**	    qeq_d9_inv_act	- invoke a single-site repeat subquery that has 
**				  been defined 
**	    qeq_d10_agg_act	- process a QEA_D6_AGG action
**	    qeq_d20_exe_qp	- process query plan
**	    qeq_d21_val_qp	- validate query plan
**	    qeq_d22_err_qp	- log query information to trace error
**
**
**  History:    $Log-for RCS$
**	11-oct-88 (carl)
**          written
**	30-mar-88 (carl)
**	    fixed lint complaints discovered by UNIX port
**	31-mar-89 (carl)
**	    modified qeq_d8_def_act to save cursor tuple descriptor for 
**	    subsequent fetches with RQF
**	09-apr-89 (carl)
**	    modified to use new field qeq_q3_ctl_info of QEQ_D1_QRY
**	09-may-89 (carl)
**	    modified qeq_d2_get_act and qeq_d8_def_act to communicate tuple 
**	    descriptor information to RQF
**	10-jun-89 (carl)
**	    modified qeq_d2_get_act to do qed_u11_ddl_commit instead of
**	    qed_u4_msg_commit to enforce the DDL_CONCURRENCY mode
**	27-jun-89 (carl)
**	    modified qeq_d7_rpt_act to call qee_d7_rptadd to register a
**	    REPEAT QUERY handle for undefining its state when the session
**	    terminates
**	24-nov-89 (carl)
**	    changed qeq_d4_sub_act to test qeq_q3_read_b instead of 
**	    qeq_q3_ctl_info which is never set by OPC to QEQ_002_USER_UPDATE
**	18-mar-90 (carl)
**	    fixed qeq_d5_xfr_act to mark the creation of a temporary table
**	    in accordance with rqr_crttmp_ok
**	02-jun-90 (carl)
**	    changed qeq_d4_sub_act, d6_tmp_act, d10_agg_act to use
**	    QEQ_002_USER_UPDATE instead of qeq_q3_read_b which is obsolete
**	24-jul-90 (georgeg)
**	    fixed qeq_d8_def_act() to call TPF for TPF_READ_DML prior to
**	    delivering DEFINE QUERY or OPEN CURSOR to the LDB
**	21-aug-90 (carl)
**	    1) extracted DDB logic, qeq_d20_exe_qp(), from qeq_query
**	       rewrote qeq_d21_val_qp(), added qeq_d22_err_qp()
**	    2) changed all occurrences of tpf_call to qed_u17_tpf_call
**      12-jun-91 (fpang)
**          In qeq_d7_rpt_act() check for E_RQ0040_UNKNOWN_REPEAT_Q instead 
**	    of E_QE0540 because rqf error code has not been remapped to 
**	    qef error code yet.
**	    Fixes B37915.
**	01-oct-1992 (fpang)
**	    In qeq_d92_get_act(), qeq_d4_sub_act(), and qeq_d9_inv_act(),
**	    set qef_count because SCF looks at it now.
**	23-oct-1992 (fpang)
**	    Save and restore qefrcb->qef_pcount in qeq_d6_tmp_act(). 
**      08-dec-92 (anitap)
**          Added #include <qsf.h> and <psfparse.h.
**	13-jan-1993 (fpang)
**	    Added LDB temp table support. In qeq_d5_xfr_act(), don't
**	    set QEE_01_CREATED if LDB supports temp tables.
**	21-jan-1992 (fpang)
**	    1. qeq_d4_sub_act(), don't mark QEA_D8_CRE temp tables created 
**	       if query fails.
**	    2. qeq_d5_xfr_act(), mark temp table created regardless of how 
**	       it was created.
**	05-feb-1993 (fpang)
**	    Added code to detect commits/aborts when executing local database
**	    procedures and executing direct execute immediate. If the LDB
**	    indicates that a commit/abort was attempted, and if the current
**	    DX is in 2PC, the DX must be aborted.
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	10-jul-1993 (shailaja)
**	    Cast parameters to correct assignment type mismatches.
**	    Fixed "statement not reached" compiler warning for code still under
**	    development.
**      13-sep-93 (smc)
**          Fixed pointer printing to use %p. Removed redundant cast of 
**          repeat.
**      14-Oct-1993 (fred)
**          Add support for large objects.  Distinguish between the
**          tuple count and the buffer count when doing a select or
**          whatever. 
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
**	    Changed dsh_ddb_cb from QEE_DDB_CB instance to ptr.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*
		Titan Query Execution and LDB Error Protocol
		--------------------------------------------

The Titan scheme is based on the existing QEF query execution paradigm.  It
includes two interfaces, one between SCF and QEF, and the other between QEF 
and RQF.  In additon, it includes an LDB error protocol which involves only
RQF and SCF.


1.  Query Execution Scheme
--------------------------

    SCF initiates by specifying QEQ_QUERY to QEF.  

    QEF executes each subquery of the query plan by specifying the 
    appropriate operation to RQF.  

    There are 4 possible outcomes and hence 4 reacting scenarios:

    1.1	The update query plan completes successfully.  

	QEF returns status E_DB_OK to SCF.

    1.2 The exeuction of a subquery causes an LDB to return data to STAR.  

	RQF returns an approprate message signalling pending data.  QEF 
	relays the message to SCF to start transmitting data to the 
	client.  SCF can specify to QEF either QEU_GET to receive data or 
	or QEQ_ENDRET to terminate data reception.

    1.3	A STAR-level error occurs.  

	Status E_DB_ERROR or E_DB_FATAL is returned by the detecting facility 
	upward to SCF.

    1.4	An LDB error happens.  

	RQF informs QEF and QEF will so inform SCF.  SCF and RQF observe 
	a new LDB error protocol to consummate a complete error unit (which 
	may consist of multiple messages) from the LDB.  QEF will not be 
	involved for error message transmission to the client.  Upon 
	completion of such initial consummation, RQF will itself observe 
	the GCF error protocol by consuming or discarding any remaining 
	messages from the sender before returning to QEF; QEF returns an 
	LDB error to SCF thus terminating execution.


2.  Query Execution Initiation
------------------------------

    SCF specifies QEQ_QUERY and one of the following to QEF:

	qef_rcb
	    .qef_qp 
	    .qef_qso_handle

    QEF executes each subquery by specifying the appropriate operation 
    (RQR_QUERY) to RQF.  


3.  Receiving Data from an LDB
------------------------------

    SCF specifies QEQ_QUERY and the following to QEF:

	qef_rcb
	    .qef_rowcount			number of tuples wanted (set
						by SCF) and then returned 
						(set by QEF)
	    .qef_output				address of output buffer with
						sufficient space to hold 
						specified number of tuples

    QEF relays the last two parameters to RQF for loading data (RQR_GET) 
    before returning.

    When the data is exhausted, QEF returns to SCF:

	status			    E_DB_WARN
	qef_rcb->error.err_code	    E_QE0015_NO_MORE_ROWS

    to signal end of data reception.


4.  Terminating Data Reception
------------------------------

    SCF specifies QEQ_ENDRET and the following to QEF:

	qef_rcb
	    .qef_eflag				QEF_INTERNAL | QEF_EXTERNAL
	    .qef_db_id				DDB id
	    .qef_qp or .qef_qso_handle

   QEF terminates data reception with RQF (RQR_ENDRET).


5.  LDB Error Protocol
----------------------

    RQF is responsible for delivering LDB error messages to SCF for
    its consummation.

    When the error unit is consummated, SCF returns control to RQF and
    then to QEF.
*/


/*{
** Name: qeq_d0_val_act - Validate a DDB query action (called from qeq_query
**			  in QEQ.C)
** Description:
**      This routine validates a given DDB query action.
**
** Inputs:
**	v_qer_p				ptr to QEF_RCB
**	    .qef_cb
**	i_act_p				ptr to query action
**				    
** Outputs:
**      none
**
**	v_qer_p				ptr to QEF_RCB
**          .error			One of the following
**		.err_code
**					E_QE0000_OK
**					E_QE1996_BAD_ACTION
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      31-oct-88 (carl)
**          written
**	30-oct-92 (barbara)
**	    Added case for delete cursor.
**	10-feb-93 (andre)
**	    recognize QEA_CREATE_VIEW as a valid action type
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qeq_d0_val_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
    bool	    error_b = FALSE;
/*
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEF_QP_CB	    *qp_p = ((QEE_DSH *)(v_qer_p->qef_cb->qef_dsh))->dsh_qp_ptr;
*/
    QEQ_D1_QRY	    *cur_p, *src_p, *tmp_p, *des_p;
    QEQ_D3_XFR	    *xfr_p;
    QEQ_TXT_SEG	    *seg_p;
    QEF_AHD	    *nxt_p;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;
    

    switch (i_act_p->ahd_atype)
    {
    case QEA_D1_QRY:
    case QEA_D5_DEF:

	cur_p = & i_act_p->qhd_obj.qhd_d1_qry;	
	seg_p = cur_p->qeq_q4_seg_p;

	if (cur_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
	{
	    /* successor must be QEA_D2_GET action */
			
	    nxt_p = i_act_p->ahd_next;

	    if (nxt_p == NULL)
		error_b = TRUE;
	    else if (nxt_p->ahd_atype != QEA_D2_GET)
		error_b = TRUE;
	    else if (cur_p->qeq_q5_ldb_p != 
			nxt_p->qhd_obj.qhd_d2_get.qeq_g1_ldb_p)
		error_b = TRUE;
	    else if (! seg_p->qeq_s1_txt_b)
		error_b = TRUE;
	    else if (! seg_p->qeq_s2_pkt_p->dd_p1_len > 0)
		error_b = TRUE;
	    else if (seg_p->qeq_s2_pkt_p->dd_p2_pkt_p == NULL)
		error_b = TRUE;

	    if (error_b)
	    {
		status = qed_u2_set_interr(
		    E_QE1996_BAD_ACTION, & v_qer_p->error);
		return(status);
	    }
	}
	break;

    case QEA_D3_XFR:

	/* QEQ_D3_XFR consists of 1) a SELECT/RETRIEVE query from the 
	** source LDB, 2) a template for creating a temporary table and 
	** 3) a COPY FROM query for the destination LDB */

	xfr_p = & i_act_p->qhd_obj.qhd_d3_xfr;

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

	    status = qed_u2_set_interr(
		E_QE1996_BAD_ACTION, & v_qer_p->error);
	    return(status);
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

	    status = qed_u2_set_interr(
		E_QE1996_BAD_ACTION, & v_qer_p->error);
	    return(status);
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

	    status = qed_u2_set_interr(
		E_QE1996_BAD_ACTION, & v_qer_p->error);
	    return(status);
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

	    status = qed_u2_set_interr(
		E_QE1996_BAD_ACTION, & v_qer_p->error);
	    return(status);
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

	    status = qed_u2_set_interr(
		E_QE1996_BAD_ACTION, & v_qer_p->error);
	    return(status);
	}

	break;

    case QEA_D2_GET:
    case QEA_D4_LNK:
    case QEA_D8_CRE:
    case QEA_D9_UPD:
    case QEA_RDEL:
    case QEA_D10_REGPROC:
    case QEA_CREATE_VIEW:
	break;

    case QEA_D6_AGG:
	cur_p = & i_act_p->qhd_obj.qhd_d1_qry;
	if (! (cur_p->qeq_q6_col_cnt > 0))
	{
	    /* error: COLUMN count CANNOT be 0! */

	    qed_w6_prt_qperror(v_qer_p);	/* banner line */

	    STprintf(cbuf, 
"%s %p: ...AGGREGATE RETRIEVAL CANNOT have ZERO column count!\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb);
            qec_tprintf(v_qer_p, cbufsize, cbuf);

	    status = qed_u2_set_interr(
		E_QE1996_BAD_ACTION, & v_qer_p->error);
	    return(status);
	}
	break;

    default:
	STprintf(cbuf, 
	    "%s %p: ...Bad action detected\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	status = qed_u2_set_interr(
	    E_QE1996_BAD_ACTION, & v_qer_p->error);
	return(status);
    }
    return(status);
}   


/*
** {
** Name: qeq_d1_qry_act - Execute a QEA_D1_QRY action (called from qeq_query
**			  in QEQ.C)
**
** Description:
**      This routine executes a QEA_D1_QRY action and any following QEA_D2_GET.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_cb
**		.qef_dsh	    ptr to QEE_DSH for query plan
**	v_act_pp		    ptr to ptr to QEF_AHD
**				    
** Outputs:
**      none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      31-oct-88 (carl)
**          written
**      15-Oct-1993 (fred)
**          Make tolerant of E_DB_INFO status from qeq_d2_get_act()
**          for use when processing large objects.
*/


DB_STATUS
qeq_d1_qry_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    **v_act_pp )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEF_AHD	    *act_p = *v_act_pp;
/*
    QEF_QP_CB	    *qp_p = dsh_p->dsh_qp_ptr;
    QEQ_D1_QRY	    *sub_p = & qp_p->qp_ahd->qhd_obj.qhd_d1_qry;
*/
    QED_DDL_INFO    sav_ddl;			/* for saving & restoring */


    dsh_p->dsh_act_ptr = *v_act_pp;		/* mark current dsh */

    switch (act_p->ahd_atype)
    {
    case QEA_D1_QRY:
    case QEA_D8_CRE:
    case QEA_D9_UPD:

	MEfill(sizeof(*qss_p), '\0', (PTR) qss_p);

	status = qeq_d4_sub_act(v_qer_p, act_p);

	if (status == E_DB_OK)
	{
	    if (qss_p->qes_q1_qry_status & QES_01Q_READ)
	    {
		act_p = act_p->ahd_next;

		/* verify that the next action is a GET */

		if (act_p->ahd_atype != QEA_D2_GET)
		{
		    status = qed_u2_set_interr(E_QE1996_BAD_ACTION, 
				& v_qer_p->error);
		    return(status);
		}
		dsh_p->dsh_act_ptr = act_p;
					/* advance to pt to QEA_D2_GET action */
		*v_act_pp = act_p;	/* must inform qeq_query about change */

		status = qeq_d2_get_act(v_qer_p, act_p);
		if ((status == E_DB_OK) || (status == E_DB_INFO))
			return(E_DB_OK);
	    }
	    /* got here if error */

	    dsh_p->dsh_act_ptr = NULL;
	    dds_p->qes_d7_ses_state = QES_0ST_NONE;
						/* must reset state */
	}
	break;
    default:
	status = qed_u1_gen_interr(& v_qer_p->error);
    }

    return(status);
}


/*{
** Name: qeq_d2_get_act - Process GET action for Titan 
**
** Description:
**	This routine acts as an intermediary, relays the necesary information
**  to RQF to load data in the specified area as requested by SCF.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_rowcount	    number of tuples wanted
**	    .qef_output		    address of output buffer with
**				    sufficient space to hold 
**				    specified number of tuples
**	i_act_p			    ptr to QEF_AHD
** Outputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_rowcount	    number of tuples returned
**	    .qef_output		    tuples place in buffer
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**				    E_QE0015_NO_MORE_ROWS
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-oct-88 (carl)
**	    written
**	09-may-89 (carl)
**	    modified to communicate tuple descriptor information to RQF
**	10-jun-89 (carl)
**	    modified to do qed_u11_ddl_commit instead of qed_u4_msg_commit 
**	    to enforce the DDL_CONCURRENCY mode
**	01-oct-1992 (fpang)
**	    Set qef_count because SCF looks at it now.
**      14-Oct-1993 (fred)
**          Add support for large objects.  Distinguish between the
**          tuple count and the buffer count when doing a select or
**          whatever. 
*/


DB_STATUS
qeq_d2_get_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			 & dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
/*
    QEF_QP_CB	    *qph_p = dsh_p->dsh_qp_ptr;
*/
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    bool	    eod = FALSE;	    /* assume there's data */


    qss_p->qes_q5_ldb_rowcnt = 0;	    /* initialize for safety */

    if (i_act_p->qhd_obj.qhd_d2_get.qeq_g1_ldb_p != qss_p->qes_q3_ldb_p)
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    if (qee_p->qee_d3_status & QEE_02Q_RPT)
    {
	if (qee_p->qee_d3_status & QEE_04Q_EOD)
	    eod = TRUE;

    }
    else if (qss_p->qes_q1_qry_status & QES_02Q_EOD)
	eod = TRUE;

    if (eod)
    {
	if (qee_p->qee_d3_status & QEE_05Q_CDB)
	{
	    /* must preserve above mask bit for persistent REPEAT queries */

	    if (! (qee_p->qee_d3_status & QEE_02Q_RPT))
		qee_p->qee_d3_status &= ~QEE_05Q_CDB;
					    /* reset only if non-repeat 
					    ** query */
	    /* commit rerouted catalog queries */
	    
	    status = qed_u11_ddl_commit(v_qer_p);
	    if (status)
		return(status);
	}

	/* no more data */

	if (qss_p->qes_q1_qry_status & QES_02Q_EOD)
	    dds_p->qes_d7_ses_state = QES_0ST_NONE;/* reset */

	v_qer_p->qef_rowcount = 0;

	/* must return E_QE0015_NO_MORE_ROWS to signal exhaustion of data */

	v_qer_p->error.err_code = E_QE0015_NO_MORE_ROWS;
	return(E_DB_WARN);
    }

    /* 2.  set up to call RQF to load data */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_timeout = QEK_0_TIME_QUANTUM;
    rqr_p->rqr_q_language = qss_p->qes_q2_lang;
    rqr_p->rqr_1_site = i_act_p->qhd_obj.qhd_d2_get.qeq_g1_ldb_p;
						/* GET action might belong to 
						** that of a repeat query, 
						** must use LDB ptr from
						** action */
    /* 3.  relay SCF parameters */

    rqr_p->rqr_tupcount = v_qer_p->qef_rowcount;
    rqr_p->rqr_count = v_qer_p->qef_count;
    rqr_p->rqr_tupdata = (PTR) v_qer_p->qef_output;
    rqr_p->rqr_tupdesc_p = (PTR) NULL;

    if (qee_p->qee_d3_status & QEE_07Q_ATT)
	rqr_p->rqr_no_attr_names = FALSE;	/* want attribute names */
    else
	rqr_p->rqr_no_attr_names = TRUE;	/* no attribute names */

    status = qed_u3_rqf_call(RQR_GET, rqr_p, v_qer_p);
    if (status)
    {
	/* it is assumed that RQF has repaired itself before returning */

	dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* reset */
	STRUCT_ASSIGN_MACRO(rqr_p->rqr_error, v_qer_p->error);
	return(status);
    }
    if (rqr_p->rqr_end_of_data)
    {
	qss_p->qes_q1_qry_status |= QES_02Q_EOD;

	qee_p->qee_d3_status |= QEE_04Q_EOD;

	if (qee_p->qee_d3_status & QEE_05Q_CDB)   
	{
	    if (! (qee_p->qee_d3_status & QEE_02Q_RPT))
		qee_p->qee_d3_status &= ~QEE_05Q_CDB;
					    /* reset only if non-repeat 
					    ** query */

	    /* commit rerouted catalog queries */
	    
	    status = qed_u11_ddl_commit(v_qer_p);
	    if (status)
		return(status);
	}

    }
    /* return data gotten so far */

    v_qer_p->qef_rowcount = rqr_p->rqr_tupcount;
    v_qer_p->qef_count = rqr_p->rqr_count;

    if ((v_qer_p->qef_rowcount == 0) && (v_qer_p->qef_count == 0))
    {
	/* no more data */

	dds_p->qes_d7_ses_state = QES_0ST_NONE;	/* reset */

	/* must return E_QE0015_NO_MORE_ROWS to signal no more data for 
	** subquery */

	v_qer_p->error.err_code = E_QE0015_NO_MORE_ROWS;
	return(E_DB_WARN);
    }
    else if ((v_qer_p->qef_rowcount == 0)
	     && (v_qer_p->qef_count != 0))
    {
	/* Here we emulate an incomplete condition */

	v_qer_p->error.err_code = E_AD0002_INCOMPLETE;
	return(E_DB_INFO);
    }

    return(E_DB_OK);
}


/*{
** Name: qeq_d3_end_act - Process QEQ_ENDRET for Titan (called from 
**			  qeq_endret in QEQ.C)
**
** Description:
**	This routine acts as an intermediary, relays the necessary information
**  to RQF to terminate data reception.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**
** Outputs:
**	none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-oct-88 (carl)
**	    written
*/


DB_STATUS 
qeq_d3_end_act(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;


    /* 1.  check if in READ state */

    if (dds_p->qes_d7_ses_state != QES_1ST_READ)
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    v_qer_p->qef_rowcount = 0;
    dds_p->qes_d7_ses_state = QES_0ST_NONE;

    if (! (qss_p->qes_q1_qry_status & QES_02Q_EOD))
    {
	/* 2.  set up to request RQF to discard any pending data */

	MEfill(sizeof(rqr), '\0', (PTR) & rqr);
	rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

	rqr_p->rqr_timeout = QEK_0_TIME_QUANTUM;
	rqr_p->rqr_q_language = qss_p->qes_q2_lang;
	rqr_p->rqr_1_site = qss_p->qes_q3_ldb_p;

	status = qed_u3_rqf_call(RQR_ENDRET, rqr_p, v_qer_p);
    }

    qss_p->qes_q1_qry_status = QES_00Q_NIL;

    return(status);
}


/*{
** Name: qeq_d4_sub_act - Process a subquery action.
**
** Description:
**	This routine executes a subquery.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_act_p			    ptr to QEF_AHD	
**
** Outputs:
**	None
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	11-oct-88 (carl)
**	    written
**	24-nov-89 (carl)
**	    switched test to qeq_q3_read_b instead of qeq_q3_ctl_info
**	02-jun-90 (carl)
**	    changed to test QEQ_002_USER_UPDATE instead of
**	    qeq_q3_read_b which is obsolete
**	01-oct-1992 (fpang)
**	    Set qef_count because SCF looks at it now.
**	21-jan-1993 (fpang)
**	    Don't mark QEA_D8_CRE temp tables created if query fails.
*/


DB_STATUS
qeq_d4_sub_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEQ_DDQ_CB	    *ddq_p = & dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEQ_D1_QRY	    *sub_p = & i_act_p->qhd_obj.qhd_d1_qry;
    TPR_CB	    tpr,
		    *tpr_p = & tpr;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    i4		    tpr_op,
    		    rqr_op;
    DD_PACKET	    *pkt_p;
    i4		    temp_id;
    i4	    *long_p;
    bool	    update_b;


    if (sub_p->qeq_q3_ctl_info & QEQ_002_USER_UPDATE)
	update_b = TRUE;		/* update subquery */
    else		
	update_b = FALSE;		/* read subquery */

    /* 1.  set up to call TPF first for its transaction manipulation */

    MEfill(sizeof(tpr), '\0', (PTR) & tpr);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session id */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session id */
    tpr_p->tpr_lang_type = sub_p->qeq_q1_lang;
    tpr_p->tpr_site = sub_p->qeq_q5_ldb_p;

    qss_p->qes_q2_lang = sub_p->qeq_q1_lang;

    if (update_b)
	tpr_op = TPF_WRITE_DML;
    else
    	tpr_op = TPF_READ_DML;

    status = qed_u17_tpf_call(tpr_op, tpr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 2.  then set up to call RQF to execute subquery */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = sub_p->qeq_q1_lang;
    rqr_p->rqr_timeout = sub_p->qeq_q2_quantum;
    STRUCT_ASSIGN_MACRO(*sub_p->qeq_q4_seg_p->qeq_s2_pkt_p, rqr_p->rqr_msg);
    rqr_p->rqr_1_site = sub_p->qeq_q5_ldb_p;
    rqr_p->rqr_tmp = dsh_p->dsh_ddb_cb->qee_d1_tmp_p;
    
    if (sub_p->qeq_q8_dv_cnt == 0)
    {
	rqr_p->rqr_dv_cnt = 0;
    }
    else
    {
	rqr_p->rqr_dv_cnt = ddq_p->qeq_d4_total_cnt;
	rqr_p->rqr_dv_p = qee_p->qee_d7_dv_p + 1;
						/* 1-based, make 0-based */
    }

    if (update_b)
	qss_p->qes_q1_qry_status |= QES_03Q_PHASE2;
						/* enter phase 2 for update */

    if (sub_p->qeq_q8_dv_cnt > 0)
    	rqr_op = RQR_DATA_VALUES;
    else
    	rqr_op = RQR_QUERY;

    if (i_act_p->ahd_atype == QEA_D8_CRE)
    {
	/* 3. assume and label temporary table created */
    
	for (pkt_p = sub_p->qeq_q4_seg_p->qeq_s2_pkt_p; 
	    pkt_p->dd_p2_pkt_p != NULL;
	    pkt_p = pkt_p->dd_p3_nxt_p);
	
	temp_id = pkt_p->dd_p4_slot;
	if (! (0 <= temp_id && temp_id <= ddq_p->qeq_d3_elt_cnt))
	{
	    status = qed_u1_gen_interr(& v_qer_p->error);
	    return(status);
	}

	long_p = qee_p->qee_d2_sts_p;
	long_p += temp_id;			/* pt to status word for temp
						** table */
	*long_p |= QEE_01M_CREATED;		/* mark table created */
    }
    if (qee_p->qee_d3_status & QEE_07Q_ATT)
	rqr_p->rqr_no_attr_names = FALSE;	/* want attribute names */
    else
	rqr_p->rqr_no_attr_names = TRUE;	/* no attribute names */

    status = qed_u3_rqf_call(rqr_op, rqr_p, v_qer_p);
    if (status)
    {
	if (i_act_p->ahd_atype == QEA_D8_CRE)
	{
	    /* Query failed, mark table as not created. */
	    *long_p &= ~QEE_01M_CREATED;
	}
	return(status);
    }	

    if (sub_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
    {
	dds_p->qes_d7_ses_state = QES_1ST_READ;	/* must set state to allow
						** SCF to read data */
	qss_p->qes_q1_qry_status |= QES_01Q_READ;
	qss_p->qes_q1_qry_status &= ~QES_02Q_EOD;    
	qss_p->qes_q3_ldb_p = sub_p->qeq_q5_ldb_p;
    }
    else
    {
	v_qer_p->qef_rowcount = rqr_p->rqr_tupcount;
	v_qer_p->qef_count = rqr_p->rqr_tupcount;
						/* return update row count */
    }

    if (sub_p->qeq_q3_ctl_info & QEQ_003_ROW_COUNT_FROM_LDB)
	qss_p->qes_q5_ldb_rowcnt = rqr_p->rqr_tupcount;

    return(E_DB_OK);
}


/*{
** Name: qeq_d5_xfr_act - Process a data transfer action.
**
** Description:
**	This routine informs TPF and requests RQF to carry out a transfer
**  action.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_act_p			    ptr to QEF_AHD	
**
** Outputs:
**	None
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	31-oct-88 (carl)
**	    written
**	18-mar-90 (carl)
**	    fixed to mark the creation of a temporary table in accordance 
**	    with rqr_crttmp_ok
**	13-jan-1993 (fpang)
**	    Added LDB temp table support. If LDB suports temp tables
**		1). Prefix table name with 'SESSION.' , and
**		2). Don't set QEE_01_CREATED (mark the table as created).
**	21-jan-1992 (fpang)
**	    Mark temp table created regardless of how it was created.
*/


DB_STATUS
qeq_d5_xfr_act(
QEF_RCB		*v_qer_p,
QEF_AHD		*i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEQ_DDQ_CB	    *ddq_p = & dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEQ_D3_XFR	    *xfr_p = & i_act_p->qhd_obj.qhd_d3_xfr;
    QEQ_D1_QRY	    *src_p = & xfr_p->qeq_x1_srce, 
		    *tmp_p = & xfr_p->qeq_x2_temp,  
		    *des_p = & xfr_p->qeq_x3_sink;  /* for debugging */
    TPR_CB	    tpr,
		    *tpr_p = & tpr;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    DD_PACKET	    *pkt_p;
    i4		    temp_id;
    i4	    *long_p = (i4 *) NULL;


    /* 1.1  inform TPF of SELECT/RETRIEVE */

    MEfill(sizeof(tpr), '\0', (PTR) & tpr);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session id */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session id */
    tpr_p->tpr_lang_type = src_p->qeq_q1_lang;
    tpr_p->tpr_site = src_p->qeq_q5_ldb_p;

    qss_p->qes_q2_lang = src_p->qeq_q1_lang;

    status = qed_u17_tpf_call(TPF_READ_DML, tpr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* 1.2  inform TPF of CREATE TABLE */

    tpr_p->tpr_lang_type = tmp_p->qeq_q1_lang;
    tpr_p->tpr_site = tmp_p->qeq_q5_ldb_p;

    qss_p->qes_q2_lang = tmp_p->qeq_q1_lang;

    status = qed_u17_tpf_call(TPF_READ_DML, tpr_p, v_qer_p);	
						/* to be changed to
						** TPF_WRITE_DML when
						** 2PC is implemented */
    if (status)
    {
	return(status);
    }

    /* 2.  call RQF to execute transfer action */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = src_p->qeq_q1_lang;
    rqr_p->rqr_timeout = 0;

    if (src_p->qeq_q8_dv_cnt == 0)
    {
	rqr_p->rqr_dv_cnt = v_qer_p->qef_pcount;
	rqr_p->rqr_dv_p = (DB_DATA_VALUE *) v_qer_p->qef_param;
    }
    else
    {
	rqr_p->rqr_dv_cnt = ddq_p->qeq_d4_total_cnt;
	rqr_p->rqr_dv_p = qee_p->qee_d7_dv_p + 1;
						/* 1-based */
    }

    rqr_p->rqr_xfr = (PTR) xfr_p;
    rqr_p->rqr_tmp = qee_p->qee_d1_tmp_p;
						
    /* 3. validate position of temporary-table id */
    
    for (pkt_p = tmp_p->qeq_q4_seg_p->qeq_s2_pkt_p; 
	 pkt_p->dd_p2_pkt_p != NULL;
	 pkt_p = pkt_p->dd_p3_nxt_p);
	
    temp_id = pkt_p->dd_p4_slot;
    if (! (0 <= temp_id && temp_id <= ddq_p->qeq_d3_elt_cnt))
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    /* 3.1 If ldb supports temp tables, modify table name by prefixing 
    ** 'session.'. It will fit because sizof(DD_TAB_NAME) is allocated, but 
    ** name is only 10 for open sql.
    */

    if (tmp_p->qeq_q5_ldb_p->dd_l6_flags & DD_04FLAG_TMPTBL)
    {
	char    tmpdd[sizeof(DD_TAB_NAME) * 2];
	char    *tmpnm = (char *)(qee_p->qee_d1_tmp_p + temp_id);

	tmpnm[sizeof(DD_TAB_NAME)-1] = EOS;
	STpolycat( 3, "session", ".", tmpnm, tmpdd );
	STmove( tmpdd, (char)' ', sizeof(DD_TAB_NAME), tmpnm );
    }

    /* 4. execute transfer action */

    qss_p->qes_q1_qry_status |= QES_03Q_PHASE2;
						/* enter phase 2 */
    status = qed_u3_rqf_call(RQR_XFR, rqr_p, v_qer_p);

    /* label temporary table created as indicated */

    if (rqr_p->rqr_crttmp_ok)
    {
	long_p = qee_p->qee_d2_sts_p;
	long_p += temp_id;			/* pt to status word for temp
						** table */
	*long_p |= QEE_01M_CREATED;		/* mark table created for
						** deletion */
    }
    return(status);
}


/*{
** Name: qeq_d6_tmp_act - destroy all temporary tables for a DDB query plan
**
** Description:
**	For a DDB query plan, traverse the termination action list to destroy
**  all temporary tables created with one or more LDBs to persist until now.
**
** Inputs:
**	v_qer_p		    QEF request block
**	i_dsh_p		    DSH for destroying temporary tables
**
** Outputs:
**	none
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	07-dec-88 (carl)
**	    moved here from QEDEXE.C
**	20-jan-90 (carl)
**	    deguise temporary table destruction as READ operation for 2PC
**	02-jun-90 (carl)
**	    changed to unset QEQ_002_USER_UPDATE instead of
**	    qeq_q3_read_b which is obsolete
**	23-oct-1992 (fpang)
**	    Save and restore qefrcb->qef_pcount because 'drop' does not
**	    have parameters.
**	15-mar-1993 (fpang)
**	    Pay attention to transaction state when dropping temp tables.
**	    If transaction is started, commit it if autocommit is on.
**	    Fixes B44827, multiple sessions running select queries
**	    gets deadlocks on system catalogs.
*/


DB_STATUS 
qeq_d6_tmp_act(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	     status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
/*
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
*/
    QEE_DDB_CB	    *qee_p = i_dsh_p->dsh_ddb_cb;
    QEQ_DDQ_CB	    *ddq_p = & i_dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEF_AHD	    *ahd_p = ddq_p->qeq_d1_end_p,
		    ahd_tmp;
    QEQ_D1_QRY	    *sub_p = (QEQ_D1_QRY *) NULL;
    DD_PACKET	    *pkt_p = (DD_PACKET *) NULL;
    i4	    *long_p = (i4 *) NULL;
    i4		     temp_id;
    i4		     sav_pcount;
    QEF_CB	    *qcb_p = v_qer_p->qef_cb;
    bool	     xact_b = FALSE;		/* Assume no bgn xact */

    if (ahd_p == NULL				/* nothing to do */
	||
	qcb_p->qef_abort)		/* transaction to be aborted */
	return(E_DB_OK);	

    /* Start a transaction if not already in a transaction */
    if (qcb_p->qef_stat == QEF_NOTRAN)
    {
	status = qet_begin(qcb_p);
	if (status)
	    return(status);
	xact_b = TRUE;
    }
    
    /* If not autocommit, escalate to MSTRAN */
    if (qcb_p->qef_auto == QEF_OFF)
	qcb_p->qef_stat = QEF_MSTRAN;

    /* drop all temporary tables */

    sav_pcount = v_qer_p->qef_pcount;
    v_qer_p->qef_pcount = 0;

    while (ahd_p != NULL && status == E_DB_OK)
    {

	/* determine if temporary has ever been created */

	sub_p = & ahd_p->qhd_obj.qhd_d1_qry;

	for (pkt_p = sub_p->qeq_q4_seg_p->qeq_s2_pkt_p; 
	     pkt_p->dd_p2_pkt_p != NULL;
	     pkt_p = pkt_p->dd_p3_nxt_p);
	
	temp_id = pkt_p->dd_p4_slot;
	if (! (0 <= temp_id && temp_id <= ddq_p->qeq_d3_elt_cnt))
	{
	    status = qed_u1_gen_interr(& v_qer_p->error);
	    break;
	}

	long_p = qee_p->qee_d2_sts_p;
	long_p += temp_id;			/* pt to status word for temp
						** table */
	if (*long_p & QEE_01M_CREATED)		/* created ? */
	{
	    STRUCT_ASSIGN_MACRO(*ahd_p, ahd_tmp);
						/* make a copy of header */
	    ahd_tmp.qhd_obj.qhd_d1_qry.qeq_q3_ctl_info &= ~QEQ_002_USER_UPDATE;
						/* disguise for safety */
	    status = qeq_d4_sub_act(v_qer_p, & ahd_tmp);    
	    if (status)
	    {
		break;
	    }
	}

	ahd_p = ahd_p->ahd_next;		/* advance to next DROP action 
						**/
    }

    /* If xact started and autocommit, commit */
    if (xact_b == TRUE && qcb_p->qef_auto == QEF_ON)
	status = qet_commit(qcb_p);

    v_qer_p->qef_pcount = sav_pcount;
    return(status);
}


/*{
** Name: qeq_d7_rpt_act - Process a repeat subquery action.
**
** Description:
**	This routine implements the following REPEAT QUERY protocols:  
**
**  1)	It defines the repeat subquery if it's not already defined with the LDB.
**	
**  2)  It invokes the repeat subquery.  If the subquery has been destroyed by 
**	the LDB, as indicated by an error reflecting a GCA_RPQ_INK_MASK, this
**	routine will attempt to redefine and reinvoke the subquery with the LDB.
**
**  If the subquery is a SELECT/RETRIEVE, the current action pointer will be 
**  advanced to point to the next QEA_D2_GET action.
**
**  Also, TPF will be informed only when the subquery is invoked.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_eflag		    QEF_INTERNAL or QEF_EXTERNAL
**	    .qef_cb		    session control block
**	    .qef_db_id		    DDB id
**	    .qef_qp		    name of query plan of type DB_CURSOR_ID
**	    .qef_pso_handle	    internal query plan id, used instead of
**				    above if non-zero
**	    .qef_param		    ptr to array of parameters of type 
**				    QEF_PARAM 
**	    .qef_rowcount	    number of output rows wanted 
**				    (SELECT/RETRIEVE)
**	    .qef_output		    ptr to output buffer of type QEF_DATA
**	v_act_pp		    prt to ptr to QEF_AHD	
**	    .ahd_type		    QEA_D5_DEF
**	    .qhd_obj
**		.qhd_d1_qry	    QEQ_D1_QRY
** Outputs:
**	v_act_pp		    advanced to point to the next QEA_D2_GET
**				    action if subquery is a SELECT/RETRIEVE
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-jan-89 (carl)
**	    written
**	27-jun-89 (carl)
**	    modified to call qee_d7_rptadd to register a REPEAT QUERY handle 
**	    in the session control block's list for undefining its state when 
**	    the session terminates
**	12-jun-91 (fpang)
**	    Check for E_RQ0040_UNKNOWN_REPEAT_Q instead of E_QE0540 because
**	    rqf error has not been remapped to qef error yet.
**	    Fixes B37915.
**	13-may-93 (davebf)
**	    Check if this can be a repeat query: single site, one action. If
**	    not, execute as normal query and destroy after one use.
**	13-may-93 (davebf)
**	    Insert pointer to invoke parms into DVs from query plan.
**      15-Oct-1993 (fred)
**          Make tolerant of E_DB_INFO status from qeq_d2_get_act()
**          for use when processing large objects.
*/


DB_STATUS
qeq_d7_rpt_act(
QEF_RCB		*v_qer_p,
QEF_AHD		**v_act_pp )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEF_AHD	    *act_p = *v_act_pp;
    QEQ_D1_QRY	    *sub_p = & act_p->qhd_obj.qhd_d1_qry;
    QEF_QP_CB	    *qp_p =  dsh_p->dsh_qp_ptr;
    QEF_AHD	    *tempact = qp_p->qp_ahd;
    bool	    done_b,
		    redef_b;
    DB_DATA_VALUE   **parmarray;
    DB_DATA_VALUE   *dv;
    i4		    count;

    /* check if this should be a repeat query */
    if(!(tempact->ahd_next == (QEF_AHD *)NULL ||
         (tempact->ahd_next->ahd_atype == QEA_D2_GET &&
	  tempact->ahd_next->ahd_next == (QEF_AHD *)NULL)))
    {
	/* either a complex query or multisite */
	/* force it to be destroyed after one use and be processed as
	** normal query */
	dsh_p->dsh_qp_status |= DSH_QP_OBSOLETE;   /* qee_destroy will delete */

	status = qeq_d1_qry_act(v_qer_p, v_act_pp);  /* do normal query */
	return(status);
    }

    /* insert pointer to invoke parms (if any) into DVs from query plan */

    if (sub_p->qeq_q8_dv_cnt > 0)   		/* QP has parms */
    {
	dv = qee_p->qee_d7_dv_p + 1;		/* first query plan DV */
	parmarray = 
	     ((DB_DATA_VALUE **)v_qer_p->qef_param->dt_data)+1; /* first parm */
	for(count = v_qer_p->qef_pcount; count; --count, ++dv, ++parmarray)
	{
	    if(dv->db_datatype != (*parmarray)->db_datatype)
	    {
		v_qer_p->error.err_code = E_QE0002_INTERNAL_ERROR;
		return(E_DB_ERROR);
	    }
	    dv->db_data = (*parmarray)->db_data;
	    dv->db_length = (*parmarray)->db_length;
	}
    }
    


/* following is a kludge until a way is designed to make the OPC generated
   text acceptable to the LDB as a define query. This kludge will still
   get the benefit of being an invoke-able repeat query from the FE */

    /* run local query as non-repeat */
    status = qeq_d1_qry_act(v_qer_p, v_act_pp);  /* do normal query */
    if(1) return(status);  
	/* if(1)-temporary compiler fix till following code is turned on */
				
        
/* end of kludge */

#ifdef REPT_FIXED

    /* subquery invocation can happen at most twice in the following loop
    ** if the already defined subquery has been destroyed by the LDB */

    done_b = FALSE;
    redef_b = FALSE;
    while ((! done_b) && status == E_DB_OK)
    {
		    
	/* 1.  If the subquery is not already defined, define it to the LDB */

	if (! (qee_p->qee_d3_status & QEE_03Q_DEF))
	{
	    status = qeq_d8_def_act(v_qer_p, act_p);
	    if (status)
		return(status);
	    
	    status = qee_d7_rptadd(v_qer_p);
	    if (status)
		return(status);

	    qee_p->qee_d3_status |= QEE_03Q_DEF;

	    if (redef_b)
		done_b = TRUE;			/* terminate loop */
	}

	/* 2.  invoke the presumably defined subquery with the LDB */

	qee_p->qee_d3_status &= ~QEE_04Q_EOD;
	status = qeq_d9_inv_act(v_qer_p, act_p);
	if (status)
	{
	    if (v_qer_p->error.err_code == E_RQ0040_UNKNOWN_REPEAT_Q)
	    {
		/* subquery not found in LDB, define it one more time */

		status = E_DB_OK;		/* reset status to continue */
		qee_p->qee_d3_status &= ~QEE_03Q_DEF;
		redef_b = TRUE;
	    }
	    else
		return(status);			/* return error */
	}
	else
	    done_b = TRUE;			/* terminate loop */
    } /* end while */

    if (status == E_DB_OK)
    {
	/* ok so far, do post read processing if necessary */

	if (sub_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
	{
	    dds_p->qes_d7_ses_state = QES_1ST_READ;
						/* set state to allow SCF to 
	    					** read data */
	    qss_p->qes_q1_qry_status = QES_01Q_READ;
/*
	    qss_p->qes_q3_ldb_p = sub_p->qeq_q5_ldb_p;
*/
	    /* advance to execute QEA_D2_GET action */

	    act_p = act_p->ahd_next;
	    dsh_p->dsh_act_ptr = act_p;
	    *v_act_pp = act_p;			/* must inform qeq_query
						** about advance */
	    status = qeq_d2_get_act(v_qer_p, act_p);
	}
    }
    
    if (status && (status != E_DB_INFO))
    {
	/* error, reset state */

	dsh_p->dsh_act_ptr = NULL;
	dds_p->qes_d7_ses_state = QES_0ST_NONE;
	qss_p->qes_q1_qry_status = QES_00Q_NIL;
    }

    return(status);
#endif
}


/*{
** Name: qeq_d8_def_act - define a single-site repeat/cursor subquery.
**
** Description:
**      This routine requests RQF to define a repeat/cursor query to an LDB,
**  saves the query id returned by the LDB for subsequent execution.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qed_param		    ptr to first of parameter array 
**	    .qef_cb
**		.qef_dsh	    ptr to QEE_DSH for repeat query 
**	i_act_p			    ptr to DEFINE QUERY action
**
** Outputs:
**	none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	02-jan-89 (carl)
**          written
**	09-may-89 (carl)
**	    modified to communicate tuple descriptor information to RQF
**	24-jul-90 (georgeg)
**	    fixed qeq_d8_def_act() to call TPF for TPF_READ_DML prior to
**	    delivering DEFINE QUERY or OPEN CURSOR to the LDB
**	13-may-93 (davebf)
**	    Pass rqr_dv_p and count to RQF instead of rqr_inv_parms.
*/


DB_STATUS
qeq_d8_def_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
/*
    QEF_QP_CB	    *qp_p = dsh_p->dsh_qp_ptr;
*/
    QEQ_D1_QRY	    *sub_p = & i_act_p->qhd_obj.qhd_d1_qry;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    i4		    rqr_opcode;
    TPR_CB	    tpr,
		    *tpr_p = & tpr;
    char	    repeat[] = "repeat ";


    /* 1.  set up to call TPF for read operation */

    MEfill(sizeof(tpr), '\0', (PTR) & tpr);
    tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session id */
    tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session id */
    tpr_p->tpr_lang_type = sub_p->qeq_q1_lang;
    tpr_p->tpr_site = sub_p->qeq_q5_ldb_p;

    status = qed_u17_tpf_call(TPF_READ_DML, tpr_p, v_qer_p);
    if (status)
    {
	return(status);
    }

    /* set up to call RQF for DEFINE QUERY or OPEN CURSOR */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = sub_p->qeq_q1_lang;
    rqr_p->rqr_timeout = sub_p->qeq_q2_quantum;
    rqr_p->rqr_1_site = sub_p->qeq_q5_ldb_p;
    rqr_p->rqr_tmp = dsh_p->dsh_ddb_cb->qee_d1_tmp_p;
    rqr_p->rqr_qid_first = sub_p->qeq_q12_qid_first;

    if(i_act_p->ahd_atype != QEA_D7_OPN)	/* if repeat query */
    {
	/* make RQF's first packet be a special one containing "Repeat" 
	** which points to the real list */
	rqr.rqr_msg.dd_p2_pkt_p = repeat;
	rqr.rqr_msg.dd_p3_nxt_p = sub_p->qeq_q4_seg_p->qeq_s2_pkt_p;
	rqr.rqr_msg.dd_p1_len = 6;
	rqr.rqr_msg.dd_p4_slot = DD_NIL_SLOT;
    }
    else
	STRUCT_ASSIGN_MACRO(*sub_p->qeq_q4_seg_p->qeq_s2_pkt_p, rqr_p->rqr_msg);


    rqr_p->rqr_dv_cnt = sub_p->qeq_q8_dv_cnt;	    /* may be zero */
    rqr_p->rqr_dv_p = qee_p->qee_d7_dv_p + 1;

    STRUCT_ASSIGN_MACRO(qee_p->qee_d4_given_qid, rqr_p->rqr_qid);

    if (i_act_p->ahd_atype == QEA_D7_OPN)
    {
	rqr_opcode = RQR_OPEN;

	qee_p->qee_d3_status |= QEE_01Q_CSR;

    }
    else 
    {
	rqr_opcode = RQR_DEFINE;

	qee_p->qee_d3_status |= QEE_02Q_RPT;
    }

    if (qee_p->qee_d3_status & QEE_07Q_ATT)
	rqr_p->rqr_no_attr_names = FALSE;	/* want attribute names */
    else
	rqr_p->rqr_no_attr_names = TRUE;	/* no attribute names */

    status = qed_u3_rqf_call(rqr_opcode, rqr_p, v_qer_p);
    if (status)
    {
	if (i_act_p->ahd_atype == QEA_D7_OPN)
	    v_qer_p->error.err_code = E_QE0544_CURSOR_OPEN_FAILED;

	return(status);
    }	
    /* define query or open cursor OK, save query id returned by LDB 
    ** and pertinent information */

    qee_p->qee_d3_status |= QEE_03Q_DEF;

    STRUCT_ASSIGN_MACRO(rqr_p->rqr_qid, qee_p->qee_d5_local_qid);

    STRUCT_ASSIGN_MACRO(* sub_p->qeq_q5_ldb_p, qee_p->qee_d11_ldbdesc);

    qss_p->qes_q2_lang = sub_p->qeq_q1_lang;
    qss_p->qes_q3_ldb_p = sub_p->qeq_q5_ldb_p;

    return(E_DB_OK);
}


/*{
** Name: qeq_d9_inv_act - Invoke a single-site repeat subquery 
**
** Description:
**	This routine invokes a previously defined repeat subquery via RQF.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_cb
**		.qef_dsh	    ptr to QEE_DSH for repeat query 
**	i_act_p			    ptr to DEFINE QUERY action
**				    
** Outputs:
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-jan-89 (carl)
**	    written
**	01-oct-1992 (fpang)
**	    Set qef_count because SCF looks at it now.
**	13-may-93 (davebf)
**	    Pass rqr_dv_p and count to RQF instead of rqr_inv_parms.
*/


DB_STATUS
qeq_d9_inv_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
/*
    QEF_QP_CB	    *qp_p = dsh_p->dsh_qp_ptr;
*/
    QEQ_D1_QRY	    *sub_p = & i_act_p->qhd_obj.qhd_d1_qry;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    i4		    tpr_mode;
    bool	    update_b = FALSE,
		    log_qry_55 = FALSE;
    i4	    i4_1, i4_2;


    if (sub_p->qeq_q3_ctl_info & QEQ_002_USER_UPDATE)
	update_b = TRUE;		/* update subquery */
    else		
	update_b = FALSE;		/* read subquery */

    if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, QEF_TRACE_DDB_LOG_QRY_55,
        & i4_1, & i4_2))
    {
        log_qry_55 = TRUE;
	qeq_p6_inv_act(v_qer_p, i_act_p);
    }
    /* 1.  inform TPF of invocation */

    if (update_b)
    	tpr_mode = QEK_3TPF_UPDATE;
    else
    	tpr_mode = QEK_2TPF_READ;

    status = qet_t5_register(v_qer_p, sub_p->qeq_q5_ldb_p, sub_p->qeq_q1_lang,
		tpr_mode);
    if (status)
    {
	return(status);
    }
    /* 2.  invoke subquery via RQF */

    MEfill(sizeof(rqr), '\0', (PTR) & rqr);
    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */

    rqr_p->rqr_q_language = sub_p->qeq_q1_lang;
    rqr_p->rqr_timeout = sub_p->qeq_q2_quantum;
    STRUCT_ASSIGN_MACRO(*sub_p->qeq_q4_seg_p->qeq_s2_pkt_p, rqr_p->rqr_msg);
    rqr_p->rqr_1_site = sub_p->qeq_q5_ldb_p;
    rqr_p->rqr_tmp = dsh_p->dsh_ddb_cb->qee_d1_tmp_p;

    STRUCT_ASSIGN_MACRO(qee_p->qee_d5_local_qid, rqr_p->rqr_qid);

    rqr_p->rqr_dv_cnt = sub_p->qeq_q8_dv_cnt;
    rqr_p->rqr_dv_p = qee_p->qee_d7_dv_p + 1;

    status = qed_u3_rqf_call(RQR_EXEC, rqr_p, v_qer_p);
    if (status)
    {
	return(status);
    }	

    if (sub_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
    {
	dds_p->qes_d7_ses_state = QES_1ST_READ;	/* must set state to allow
						** SCF to read data */
	qss_p->qes_q1_qry_status |= QES_01Q_READ;
	qss_p->qes_q1_qry_status &= ~QES_02Q_EOD;    
	qss_p->qes_q3_ldb_p = sub_p->qeq_q5_ldb_p;
    }
    else
    {
	v_qer_p->qef_rowcount = rqr_p->rqr_tupcount;
	v_qer_p->qef_count = rqr_p->rqr_count;
						/* return update row count */
    }

    return(E_DB_OK);
}


/*{
** Name: qeq_d10_agg_act - Process an QEA_D6_AGG action
**
** Description:
**      This routine executes a parameterized subquery by calling RQF
**  and stores the intermediate result returned in the data segment for
**  subsequent subqueries.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_param		    ptr to first of parameter array 
**	    .qef_cb
**		.qef_dsh	    ptr to QEE_DSH for query 
**	i_act_p			    ptr to QEA_D6_AGG action
**
** Outputs:
**	none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	14-feb-89 (carl)
**          written
**	02-jun-90 (carl)
**	    changed to test QEQ_002_USER_UPDATE instead of
**	    qeq_q3_read_b which is obsolete
*/


DB_STATUS
qeq_d10_agg_act(
QEF_RCB	    *v_qer_p,
QEF_AHD	    *i_act_p )
{
    DB_STATUS	    status = E_DB_OK;
/*
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEF_QP_CB	    *qp_p = dsh_p->dsh_qp_ptr;
    QEQ_DDQ_CB	    *ddq_p = & qp_p->qp_ddq_cb;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
*/
    QEQ_D1_QRY	    *sub_p = & i_act_p->qhd_obj.qhd_d1_qry;
    i4		    tpr_mode,
		    tupcnt = 0;
    bool	    eod_b,
		    tupread_b;


    if (sub_p->qeq_q3_ctl_info & QEQ_002_USER_UPDATE)
    {
	status = qed_u2_set_interr(E_QE1996_BAD_ACTION, & v_qer_p->error);
	return(status);
    }

    /* 1.  register operation with TPF */

    status = qet_t5_register(v_qer_p, sub_p->qeq_q5_ldb_p, sub_p->qeq_q1_lang,
		(i4) QEK_2TPF_READ);
    if (status)
	return(status);

    /* 2.  send the aggregate subquery */

    status = qeq_a1_doagg(v_qer_p, i_act_p);
    if (status)
	return(status);

    /* 3.  consume all tuples */

    eod_b = FALSE;
    while (! eod_b && ! status)	/* any data to read ? */
    {
	status = qeq_a2_fetch(v_qer_p, i_act_p, & tupread_b, & eod_b);
	if (status)
	    return(status);
	if (tupread_b)
	    ++tupcnt;
    }
    
    /* 4.  verify that there is one and only one result tuple */

    if (tupcnt != 1)
    	status = qed_u2_set_interr(E_QE1995_BAD_AGGREGATE_ACTION, 
		    & v_qer_p->error);
		    
    return(status);
}


/*{
** Name: qeq_d21_val_qp - Validate all actions of query plan
**
** Description:
**      This routine validates a query plan.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to current DSH
**
** Outputs:
**	none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	10-aug-90 (carl)
**          rewritten
*/


DB_STATUS
qeq_d21_val_qp(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_QP_CB	    *qp_p = i_dsh_p->dsh_qp_ptr;
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

    for (act_cnt = 1; act_p != (QEF_AHD *) NULL; ++act_cnt)
    {
	if (act_p->ahd_atype == QEA_D1_QRY)
	{
	    cur_p = & act_p->qhd_obj.qhd_d1_qry;
	    if (cur_p->qeq_q3_ctl_info & QEQ_003_ROW_COUNT_FROM_LDB)
		sel_ldb++;

	    seg_p = cur_p->qeq_q4_seg_p;
	    if (cur_p->qeq_q3_ctl_info & QEQ_001_READ_TUPLES)
	    {
		/* successor MUST be QEA_D2_GET action */
			
		nxt_p = act_p->ahd_next;

		if (nxt_p == NULL)
		{
		    qed_w6_prt_qperror(v_qer_p);   /* banner line */

		    STprintf(cbuf, 
			"%s %p: ...expected GET action MISSING\n",
			IIQE_61_qefsess,
			(PTR) v_qer_p->qef_cb);
            	    qec_tprintf(v_qer_p, cbufsize, cbuf);

		    status = qed_u2_set_interr(
				E_QE1996_BAD_ACTION, 
				& v_qer_p->error);
		    return(status);
		}
		else 
		{
		    if (nxt_p->ahd_atype != QEA_D2_GET)
		    {
			qed_w6_prt_qperror(v_qer_p);	/* banner line */

			STprintf(cbuf, 
			    "%s %p: ...expected GET action MISSING\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
            		qec_tprintf(v_qer_p, cbufsize, cbuf);

			status = qed_u2_set_interr(
				    E_QE1996_BAD_ACTION, 
				    & v_qer_p->error);
			return(status);
		    }
		    if (cur_p->qeq_q5_ldb_p 
			!= 
			nxt_p->qhd_obj.qhd_d2_get.qeq_g1_ldb_p)
		    {
			qed_w6_prt_qperror(v_qer_p);	/* banner line */

			STprintf(cbuf, 
"%s %p: ...RETRIEVAL and GET actions on inconsistent LDBs\n",
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

			status = qed_u2_set_interr(
				    E_QE1996_BAD_ACTION, 
				    & v_qer_p->error);
			return(status);
		    }

		    if (seg_p == (QEQ_TXT_SEG *) NULL)
		    {
			qed_w6_prt_qperror(v_qer_p);	/* banner line */

			STprintf(cbuf, 
			    "%s %p: ...QUERY TEXT missing\n",
			    IIQE_61_qefsess,
			    (PTR) v_qer_p->qef_cb);
            		qec_tprintf(v_qer_p, cbufsize, cbuf);

			status = qed_u2_set_interr(
				    E_QE1996_BAD_ACTION, 
				    & v_qer_p->error);
			return(status);
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

			    status = qed_u2_set_interr(
					E_QE1996_BAD_ACTION, 
					& v_qer_p->error);
			    return(status);
			}
			if (seg_p->qeq_s2_pkt_p == (DD_PACKET *) NULL)
			{
			    qed_w6_prt_qperror(v_qer_p);   /* banner line */

			    STprintf(cbuf, 
				"%s %p: ...QUERY TEXT pointer MISSING\n",
				IIQE_61_qefsess,
				(PTR) v_qer_p->qef_cb);
            		    qec_tprintf(v_qer_p, cbufsize, cbuf);

			    status = qed_u2_set_interr(
					E_QE1996_BAD_ACTION, 
					& v_qer_p->error);
			    return(status);
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

				status = qed_u2_set_interr(
					    E_QE1996_BAD_ACTION, 
					    & v_qer_p->error);
				return(status);
			    }
			    if (seg_p->qeq_s2_pkt_p->dd_p2_pkt_p == NULL)
			    {
				qed_w6_prt_qperror(v_qer_p);/* banner line */

				STprintf(cbuf, 
				    "%s %p: ...QUERY TEXT pointer MISSING\n",
				    IIQE_61_qefsess,
				    (PTR) v_qer_p->qef_cb);
            			qec_tprintf(v_qer_p, cbufsize, cbuf);

				status = qed_u2_set_interr(
					    E_QE1996_BAD_ACTION, 
					    & v_qer_p->error);
				return(status);
			    }
			}
		    }
		}
	    }

	    status = qeq_d0_val_act(v_qer_p, act_p);
	    if (status)
		return(status);
	}
	act_p = act_p->ahd_next;	/* advance */
    }

    if (act_cnt != qp_p->qp_ahd_cnt)
    {
	qed_w6_prt_qperror(v_qer_p);	/* banner line */

	STprintf(cbuf, 
	    "%s %p: ...AGGREGATE RETRIEVAL CANNOT have ZERO column count!\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	status = qed_u2_set_interr(
	    E_QE1996_BAD_ACTION, & v_qer_p->error);
	return(status);
    }

    if (sel_ldb > 1)
    {
	qed_w6_prt_qperror(v_qer_p);	/* banner line */

	STprintf(cbuf, 
	    "%s %p: ...SELECTING from MORE than 1 LDB!\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	status = qed_u2_set_interr(E_QE1996_BAD_ACTION, & v_qer_p->error);
	return(status);
    }
    return(E_DB_OK);
}


/*{
** Name: qeq_d22_err_qp - Log the query information to trace error
**
** Description:
**      This routine logs a query plan.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_dsh_p			    ptr to current DSH
**
** Outputs:
**	none
**
**      Returns:
**          E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	10-aug-90 (carl)
**          created
*/

DB_STATUS
qeq_d22_err_qp(
QEF_RCB		*v_qer_p,
QEE_DSH		*i_dsh_p )
{
    QES_DDB_SES     *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_QP_CB       *qp_p = i_dsh_p->dsh_qp_ptr;


    if (dds_p->qes_d9_ctl_info & QES_04CTL_OK_TO_OUTPUT_QP)
        dds_p->qes_d9_ctl_info &= ~QES_04CTL_OK_TO_OUTPUT_QP;
                                /* reset to flag QP written to the log to
                                ** avoid redundant writing of this same QP */
    else
        return(E_DB_OK);        /* QP already written to the log */

    qed_w0_prt_gmtnow(v_qer_p);
    qeq_p10_prt_qp(v_qer_p, i_dsh_p);
}

/*{
** Name: qeq_d11_regproc - Execute a QEA_D10_REGPROC action (called from 
**			   qeq_query in QEQ.C) - PHASE I
**
** Description:
**
**      This routine initiates the execution of a QEA_D10_REGPROC action.
**	The execution is two-phase, since the protocol requires a prefetch of
**	the tuple descriptor (for byref) and a GCA_RESPONSE from the LDBMS to
**	indicate the end of the protocol.
**
**	This routine executes phase I of a two-phase protocol.
**  
**	PHASE I:  call rqf_call with opcode = RQR_INVPROC
**
**	Inputs to RQF_CALL: 
**	-------------------
**	rqr_session         - Session control block,        *RQS_CB
**	rqr_timeout         - Timeout length,               i4
**	rqr_1_site          - Target Ldb,                   *DD_LDB_DESC
**	rqr_q_language      - Language, DB_SQL              DB_LANG
**	rqr_dv_cnt          - # of parameters,              i4
**	rqr_dBP_PARAM	    - Array of parameters,	    *QEF_USR_PARAM
**	rqr_own_name	    - Owner of procedure at local site
**	rqr_qid             - Ldb procedure identifier,     DB_CURSOR_ID
**        .db_cursor_id     - Ldb procedure id,		    array of 2 i4s
**        .db_cur_name      - Ldb procedure name,    char[DB_CURSOR_MAXNAME]
**	rqr_tupdesc_p       - Tuple descriptor description  *SCF_TD_CB
**                          NULL if not byref
**        ->scf_ldb_tp_p  - Where to put LDB TDESCR	    *GCA_TD_DATA
**        ->scf_star_td_p - TDESCR as calculated by SCF	    *GCA_TD_DATA
**
**	Outputs Frm RQF_CALL:
**	---------------------
**	rqr_qid             - Ldb procedure identifier,     DB_CURSOR_ID
**        .db_cursor_id   - Ldb procedure id,		    array of 2 i4s
**        .db_cur_name    - Ldb procedure name,	    char[DB_CURSOR_MAXNAME]
**	rqr_dbp_rstat       - Ldb return status,            i4
**	rqr_ldb_abort       - Ldb abort status,             bool
**                          TRUE if ldb aborted
**	rqr_ldb_status      - Ldb statement status          i4
**                          Mapped from GCA_RESPONSE.gca_rqstatus
**                          RQF_01_FAIL_MASK  - general failure
**                          RQF_02_STMT_MASK  - Ldb rejected statement
**                          RQF_03_TERM_MASK  - Ldb terminated transaction
**	rqr_error           - RQF error status              DB_ERROR
**	rqr_rx_timeout      - RQF timeout status            bool
**                          TRUE if GCA_RECEIVE timed out
**	rqr_1ldb_severity   - Ldb error severity of last
**                          GCA_ERROR message		    i4
**	rqr_2ldb_error_id   - Ldb generic error id of last
**                          GCA_ERROR message		    i4
**	rqr_3ldb_local_id   - Ldb local error id of last
**                          GCA_ERROR message		    i4
**
**	NOTE:	The rqr_qid.db_cursor_id must be saved in the QSF object.
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_cb
**		.qef_dsh	    ptr to QEE_DSH for query plan
**	v_act_pp		    ptr to ptr to QEF_AHD
**				    
** Outputs:
**      none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      14-dec-92 (teresa)
**          Initial creation for REGPROC project.
**	05-feb-1993 (fpang)
**	    Added code to detect commits/aborts when executing local database
**	    procedures and executing direct execute immediate. If the LDB
**	    indicates that a commit/abort was attempted, and if the current
**	    DX is in 2PC, the DX must be aborted.
**	14-apr-93 (teresa)
**	    Added rqr_own_name to RQF interface so we could specify the 
**	    LDB owner of the registered procedure.
**	11-apr-94 (swm)
**	    Bug #62665
**	    STRUCT_ASSIGN_MACRO actually expands to a direct pointer
**	    assignment of dd_p2_ldbproc_owner address to rqr_own_name
**	    and not a new copy of the struct. This is actually what
**	    the code intends but the STRUCT_ASSIGN_MACRO is misleading
**	    so replace it with the assignment.
*/

DB_STATUS qeq_d11_regproc(QEF_RCB	    *v_qer_p,
			  QEF_AHD	    *act_p)
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEQ_DDQ_CB	    *ddq_p = & dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEQ_D10_REGPROC *sub_p = & act_p->qhd_obj.qed_regproc;
    TPR_CB	    tpr,
		    *tpr_p = & tpr;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    i4		    tpr_op,
    		    rqr_op;
    DD_PACKET	    *pkt_p;
    i4		    temp_id;
    i4	    *long_p;


    /*
    ** preliminary: mark current dsh and run a sanity check that we've got
    ** the right type of action 
    */
    dsh_p->dsh_act_ptr = act_p;
    if (act_p->ahd_atype != QEA_D10_REGPROC)
    {
	/* report error */
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    for (;;)	/* control loop for error handling */
    {
	/* 
	** set up to call TPF first for its transaction manipulation --
	**
	** we cannot know whether or not the procedure does an update operation,
	** so assume all procedures are update.
	*/
	MEfill(sizeof(tpr), '\0', (PTR) & tpr);
	tpr_p->tpr_session = dds_p->qes_d2_tps_p;	/* TPF session id */
	tpr_p->tpr_rqf = dds_p->qes_d3_rqs_p;	/* RQF session id */
	tpr_p->tpr_lang_type = sub_p->qeq_r1_lang;
	tpr_p->tpr_site = sub_p->qeq_r3_regproc->dd_p5_ldbdesc;
	qss_p->qes_q2_lang = sub_p->qeq_r1_lang;
	tpr_op = TPF_WRITE_DML;
	status = qed_u17_tpf_call(tpr_op, tpr_p, v_qer_p);
	if (status)
	{
	    return(status);
	}

	/*
	** call RQF to execute subquery 
	*/
	MEfill(sizeof(rqr), '\0', (PTR) & rqr);
	rqr_op = RQR_INVPROC;
	rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */
	rqr_p->rqr_q_language = sub_p->qeq_r1_lang; /* Language (DB_SQL) */
	rqr_p->rqr_timeout = sub_p->qeq_r2_quantum; /* TImeout */
	rqr_p->rqr_1_site = sub_p->qeq_r3_regproc->dd_p5_ldbdesc;   /* LDB */
	rqr_p->rqr_tupdesc_p = v_qer_p->qef_r3_ddb_req.qer_d10_tupdesc_p;
	rqr_p->rqr_dv_cnt = v_qer_p->qef_pcount;    /* number of parameters */
	rqr_p->rqr_dbp_param = v_qer_p->qef_usr_param;
	/* Local DBP id */
	STRUCT_ASSIGN_MACRO(sub_p->qeq_r3_regproc->dd_p1_local_proc_id,
			    rqr_p->rqr_qid);
	/* Owner of Local Procedure */	
	rqr_p->rqr_own_name = &sub_p->qeq_r3_regproc->dd_p2_ldbproc_owner;

	/* If DX is in 2PC, tell RQF */
	if (tpr_p->tpr_22_flags & TPR_01FLAGS_2PC)
	    rqr_p->rqr_2pc_txn = TRUE;
	else
	    rqr_p->rqr_2pc_txn = FALSE;
	
	status = qed_u3_rqf_call(rqr_op, rqr_p, v_qer_p);
	if (status)
	{
	    break;
	}	

	/* set state for subsequent RQF calls to handle BYREF */
	qss_p->qes_q1_qry_status |= QES_03Q_PHASE2;

	{
	    /* must set state to allow SCF to read data */
	    dds_p->qes_d7_ses_state = QES_1ST_READ;	
	    qss_p->qes_q1_qry_status |= QES_01Q_READ;
	    qss_p->qes_q1_qry_status &= ~QES_02Q_EOD;    
	    qss_p->qes_q3_ldb_p = sub_p->qeq_r3_regproc->dd_p5_ldbdesc;
	}

	/*
	** Save the LDB's procedure ID in the query plan
	*/
	if (
	     (sub_p->qeq_r3_regproc->dd_p1_local_proc_id.db_cursor_id[0] !=
	      rqr_p->rqr_qid.db_cursor_id[0])
	   ||
	     (sub_p->qeq_r3_regproc->dd_p1_local_proc_id.db_cursor_id[1] !=
	      rqr_p->rqr_qid.db_cursor_id[1])
	   )	
		STRUCT_ASSIGN_MACRO(rqr_p->rqr_qid,
				    sub_p->qeq_r3_regproc->dd_p1_local_proc_id);
	/*
	** supply procedure return status to SCF
	*/
	v_qer_p->qef_dbp_status = rqr_p->rqr_dbp_rstat;

	break;
    }

    if (status != E_DB_OK)
    {
	dsh_p->dsh_act_ptr = NULL;
	dds_p->qes_d7_ses_state = QES_0ST_NONE;
    }

    return(status);
}

/*{
** Name: qeq_d12_regproc - Execute a QEA_D10_REGPROC action (called from 
**			   qeq_query in QEQ.C) - PHASE II.
**
** Description:
**
**      This routine completes the execution of a QEA_D10_REGPROC action.
**	The execution is two-phase, since the protocol requires a prefetch of
**	the tuple descriptor (for byref) and a GCA_RESPONSE from the LDBMS to
**	indicate the end of the protocol.
**
**	This routine executes phase II of a two-phase protocol.
**  
**	PHASE II:  call rqf_call with opcode = RQR_GET
**
**	Inputs to RQF_CALL: 
**	-------------------
**	rqr_session         - Session control block,        *RQS_CB
**	rqr_timeout         - Timeout length,               i4
**	rqr_1_site          - Target Ldb,                   *DD_LDB_DESC
**	rqr_q_language      - Language, DB_SQL              DB_LANG
**	rqr_tupcount        - Number of tuples requested    i4
**	rqr_tupdata         - QEF data description          *QEF_DATA
**			      NULL if not byref
**	    ->dt_size       - Size of this chunk            i4
**	    ->dt_data       - Ptr to data area              PTR
**	    ->dt_next       - Next chunk                    *QEF_DATA
**	rqr_tupdesc_p       - Tuple descriptor description  *SCF_TD_CB
**			      NULL if not byref
**	    ->scf_ldb_tp_p  - Where to put LDB TDESCR       *GCA_TD_DATA
**	    ->scf_star_td_p - TDESCR as calculated by SCF   *GCA_TD_DATA
**
**	Outputs From RQF_CALL:
**	----------------------
**	rqr_ldb_abort       - Ldb abort status,             bool
**                            TRUE if ldb aborted
**	rqr_ldb_status      - Ldb statement status          i4
**                            Mapped from GCA_RESPONSE.gca_rqstatus
**			      RQF_01_FAIL_MASK  - general failure
**                            RQF_02_STMT_MASK  - Ldb rejected statement
**                            RQF_03_TERM_MASK  - Ldb terminated transaction
**	rqr_error           - RQF error status              DB_ERROR
**	rqr_rx_timeout      - RQF timeout status            bool
**                            TRUE if GCA_RECEIVE timed out
**	rqr_1ldb_severity   - Ldb error severity of last
**                            GCA_ERROR message		    i4
**	rqr_2ldb_error_id   - Ldb generic error id of last
**                            GCA_ERROR message		    i4
**	rqr_3ldb_local_id   - Ldb local error id of last
**                            GCA_ERROR message		    i4
**	rqr_tupdata         - QEF data description          *QEF_DATA
**			      NULL if not byref
**	    ->dt_data       - GCA_TUPLES returned by LDB    PTR
**      rqr_tupdesc_p       - Tuple descriptor description  *SCF_TD_CB
**                            NULL if not byref
**	    ->scf_ldb_tp_p  - LDB tuple descriptor          *GCA_TD_DATA
**	rqr_tupcount        - Number of tuples returned     i4
**
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	    .qef_cb
**		.qef_dsh	    ptr to QEE_DSH for query plan
**	v_act_pp		    ptr to ptr to QEF_AHD
**				    
** Outputs:
**      none
**
**	v_qer_p			    ptr to QEF_RCB
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK
**          E_DB_ERROR              error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      15-dec-92 (teresa)
**          Initial creation for REGPROC project.
**	05-feb-1993 (fpang)
**	    Added code to detect commits/aborts when executing local database
**	    procedures and executing direct execute immediate. If the LDB
**	    indicates that a commit/abort was attempted, and if the current
**	    DX is in 2PC, the DX must be aborted.
*/

DB_STATUS qeq_d12_regproc(QEF_RCB	    *v_qer_p,
			  QEF_AHD	    *act_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QES_QRY_SES	    *qss_p = & dds_p->qes_d8_union.u2_qry_ses;
    QEE_DSH	    *dsh_p = (QEE_DSH *)(v_qer_p->qef_cb->qef_dsh);
    QEE_DDB_CB	    *qee_p = dsh_p->dsh_ddb_cb;
    QEQ_DDQ_CB	    *ddq_p = & dsh_p->dsh_qp_ptr->qp_ddq_cb;
    QEQ_D10_REGPROC *sub_p = & act_p->qhd_obj.qed_regproc;
    TPR_CB	    tpr,
		    *tpr_p = & tpr;
    RQR_CB	    rqr,
		    *rqr_p = & rqr;
    i4		    tpr_op,
    		    rqr_op;
    DD_PACKET	    *pkt_p;
    i4		    temp_id;
    i4	    *long_p;


    /*
    ** preliminary: mark current dsh and run a sanity check that we've got
    ** the right type of action 
    */
    dsh_p->dsh_act_ptr = act_p;
    if (act_p->ahd_atype != QEA_D10_REGPROC)
    {
	/* report error */
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    for (;;)	/* control loop for error handling */
    {

	/*
	** call RQF to complete execution of subquery 
	*/
	MEfill(sizeof(rqr), '\0', (PTR) & rqr);
	rqr_op = RQR_GET;
	rqr_p->rqr_session = dds_p->qes_d3_rqs_p;   /* RQF session id */
	rqr_p->rqr_q_language = sub_p->qeq_r1_lang; /* Language (DB_SQL) */
	rqr_p->rqr_timeout = sub_p->qeq_r2_quantum; /* TImeout */
	rqr_p->rqr_1_site = sub_p->qeq_r3_regproc->dd_p5_ldbdesc;   /* LDB */
	rqr_p->rqr_tupcount = 1;
	rqr_p->rqr_tupdata = (PTR)v_qer_p->qef_output;/* area to put data in */
	rqr_p->rqr_tupdesc_p = v_qer_p->qef_r3_ddb_req.qer_d10_tupdesc_p;
	rqr_p->rqr_dv_cnt = 0;			    /* number of parameters */
	rqr_p->rqr_dbp_param = NULL;

	status = qed_u3_rqf_call(rqr_op, rqr_p, v_qer_p);
	if (DB_SUCCESS_MACRO(status))
	{
	    if (rqr_p->rqr_end_of_data)
	    {
		if (rqr_p->rqr_ldb_status & RQF_04_XACT_MASK)
		{
		    /* If LDB tried to commit/abort set the error.
		    ** qeq_cleanup() will abort.
		    */
		    v_qer_p->error.err_code = E_QE0991_LDB_ALTERED_XACT;
		    status = E_DB_ERROR ;
		}
		else
		{
		    /* return a warning that there is no more data */
		    v_qer_p->error.err_code = E_QE0015_NO_MORE_ROWS;
		    status = E_DB_WARN;
		}
	    }
	}

	break;
    }

    return(status);
}
