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
#include    <scf.h>
#include    <qefmain.h>
#include    <qefscb.h>
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
**  Name: QEDUTL.C - Utility functions for STAR QEF
**
**  Description:
**      The routines defined in this file provide auxiliary functions to
**  STAR QEF.
**
**	qed_u0_trimtail	    - trim trailing spaces from string 1, place in
**			      string 2 and return new length as a u_i4
**	qed_u1_gen_interr   - generate internal error E_QE0002_INTERNAL_ERROR
**	qed_u2_set_interr   - set internal error
**	qed_u3_rqf_call	    - interface to rqf_call
**	qed_u4_qsf_acquire  - acquire object in QSF storage
**	qed_u5_qsf_destroy  - destroy object in QSF storage
**	qed_u6_qsf_root	    - get root of object in QSF storage
**	qed_u7_msg_commit   - end internal LDB access by sending commit message 
**	qed_u8_gmt_now	    - get current time in GMT format
**	qed_u9_log_forgiven - log a forgiven error
**	qed_u10_trap	    - trap an error condition
**	qed_u11_ddl_commit  - commit the privileged CDB association if necessary
**	qed_u12_map_rqferr  - set RQF error
**	qed_u13_map_tpferr  - set TPF error
**	qed_u14_pre_commit  - commit the privileged CDB association to release
**			      RDF's catalog locks if necessary
**	qed_u15_trace_qefcall 
**			    - trace QEF call
**	qed_u16_rqf_cleanup - clean up all the association for session
**
**	qed_u17_tpf_call    - interface to tpf_call
**
**  History:    $Log-for RCS$
**      20-may-88 (carl)    
**          written
**      13-mar-89 (carl)    
**	    renamed qed_u7_acc_commit to qed_u7_msg_commit
**      01-apr-89 (carl)    
**	    modified qed_u9_log_forgiven to return an error to abort the
**	    transaction if RQF has reported a user interrupt
**      02-may-89 (carl)    
**	    changed qed_u9_log_forgiven to not forgive 
**	    E_QE0704_MULTI_SITE_WRITE
**      12-may-88 (carl)
**	    changed qed_u12_map_rqferr to map E_RQ0039_INTERRUPTED to 
**	    E_QE0022_QUERY_ABORTED, an existing message
**
**	    changed qed_u3_rqf_call to set QES_02CTL_COMMIT_CDB_TRAFFIC 
**	    ON if the present query is directed thru the privileged CDB 
**	    association
**	30-may-89 (carl)
**	    changed qed_u7_msg_commit to call qed_u3_rqf_call
**	    changed qed_u11_ddl_commit to check if AUTOCOMMIT is not ON
**	    added qed_u14_pre_commit to commit RDF's catalog locks if necessary
**	06-jun-89 (carl)
**	    added mapping of E_RQ0047_UNAUTHORIZED_USER to 
**	    E_QE0547_UNAUTHORIZED_USER 
**	27-jun-89 (carl)
**	    fixed lint problems in qed_u3_rqf_call, removed unused variables
**	    from qed_u3_rqf_call, qed_u7_msg_commit, qed_u12_map_rqferr and 
**	    qed_u13_map_tpferr
**	18-jul-89 (carl)
**	    added qed_u15_cdb_commit
**	22-jul-89 (carl)
**	    fixed qed_u13_map_tpferr to include the full complement of 
**	    TPF errors
**	29-jul-89 (carl)
**	    fixed qed_u3_rqf_call to reinitialize the session's state if 
**	    it has been interrupted
**	19-aug-89 (carl)
**	    added code to qed_u3_rqf_call to pass the tuple descriptor 
**	    CB ptr provided by SCF to RQF 
**	22-mar-90 (carl)
**	    created qed_u16_rqf_cleanup to clean up a current session
**	21-jul-90 (carl)
**	    added following new routines:
**		qed_u15_trace_qefcall(),
**		qed_u16_log_rqferr(), 
**		qed_u17_tpf_call()
**	07-oct-90 (carl)
**	    fixed qed_u3_rqf_call and qed_u17_tpf_call to test for trace point
**	    59 in case of error
**	21-oct-90 (carl)
**	    changed qed_u17_tpf_call to pass QEF's session id to TPF for
**	    tracing to the FE
**	18-apr-1991 (fpang)
**	    Qed_u3_rqf_call and qed_u17_tpf_call were not passing back
**	    rqf/tpf errors. Modified to properly set error code.
**	    Fixes B36913.
**	08-sep-1992 (fpang)
**	    Turned on distributed code by removing ifdef STAR.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	05-feb-1993 (fpang)
**	    In qed_u3_rqf_call(), added support to detect commits/aborts 
**	    when executing local database procedures or when executing direct 
**	    execute immediate.
**	09-feb-1993 (fpang)
**	    Fixed adf() problems found during prototyping.
**	15-apr-93 (rickh)
**	    Throttled some compiler diagnostics by removing the redundant
**	    "&" from some array type function arguments.
**      10-sep-93 (smc)
**          Corrected cast of qec_tprintf parameter to PTR. Moved <cs.h>
**	    for CS_SID.
**	08-oct-93 (swm)
**	    Bug #56448
**	    Altered qec_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to qec_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    qec_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	 7-jan-94 (swm)
**	    Bug #58635
**	    Added PTR cast for qsf_owner which has changed type to PTR.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	15-nov-94 (rudtr01)
**		initialize TPF's trace buffers before calling TPF in
**		qed_u17_tpf_call()
**	03-oct-96 (pchang)
** 	    In qed_u3_rqf_call(), after returning from a successful remote OPEN
**	    CURSOR call (RQR_OPEN), turn OFF the privileged CDB access commit 
**	    flag (QES_02CTL_COMMIT_CDB_TRAFFIC) if it had been turned on by 
**	    previous remote CREATE TABLE calls (RQR_QUERY) used to implement
**	    UNION SELECT in cursor definition.  This prevents premature closure
**	    of the cursor by an ensuing RQR_COMMIT message (B78777).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	10-Jan-2001 (jenjo02)
**	    Supply session's SID to QSF in qsf_sid.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	2-Dec-2010 (kschendel) SIR 124685
**	    Prototype fixes.
**/


/*  Forward references */

GLOBALREF   char    *IIQE_51_rqf_tab[];	    /* table of RQF op names */
GLOBALREF   char    *IIQE_52_tpf_tab[];	    /* table of TPF op names */
GLOBALREF   char    *IIQE_53_lo_qef_tab[];  /* table of common QEF op names */
GLOBALREF   char    *IIQE_54_hi_qef_tab[];  /* table of DDB QEF op names */
GLOBALREF   char    IIQE_61_qefsess[];	    /* string "QEF session" */
GLOBALREF   char    IIQE_62_3dots[];	    /* string "..." */
GLOBALREF   char    IIQE_63_qperror[];	    /* string "QP ERROR detected!" */
GLOBALREF   char    IIQE_64_callerr[];	    /* "ERROR occurred while calling" */
GLOBALREF   char    IIQE_65_tracing[];	    /* "tracing" */


/*{
** Name: qed_u0_trimtail - Trim trailing spaces from string 1, place in
**			   string 2 and return new length as a u_i4
**
** Description:
**      This routine copies string 1 (not null-terminated) into string 2, 
**  trims the last and returns the new length as a u_i4.
**  
** Inputs:
**	    i_str1_p			ptr to string 1
**	    i_len1			length of string 1
** Outputs:
**	    o_str2_p			string 2
**	Returns:
**	    new length (u_i4)
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-jul-88 (carl)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/

u_i4
qed_u0_trimtail(
char		*i_str1_p,
u_i4		i_len1,
char		*o_str2_p )
{
    u_i4	len2;
    i4	ignore;
    DB_ERROR	err_stub;


    MEcopy(i_str1_p, i_len1, o_str2_p);
    *(o_str2_p + i_len1) = '\0';	/* null-terminate before calling
					** STtrmwhite */
    len2 = STtrmwhite(o_str2_p);

    if (len2 > DB_MAXNAME)
	qef_error(E_QE0002_INTERNAL_ERROR, 0L, E_DB_ERROR, &ignore, &err_stub,
	    0);
	
    return(len2);
}


/*{
** Name: qed_u1_gen_interr - Generate internal error E_QE0002_INTERNAL_ERROR.
**
** Description:
**      This routine generates internal error E_QE0002_INTERNAL_ERROR.
**  errors.
**
** Inputs:
**	
** Outputs:
**	o_dberr_p		    DB_ERROR
**	    .err_code		    E_QE0002_INTERNAL_ERROR
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-jul-88 (carl)
**          written
*/

DB_STATUS
qed_u1_gen_interr(
DB_ERROR    *o_dberr_p )
{
/*
    i4	ignore;
    DB_ERROR	err_stub;
*/

    o_dberr_p->err_code = E_QE0002_INTERNAL_ERROR;
    qed_u10_trap();
/*
    qef_error(E_QE0002_INTERNAL_ERROR, 0L, E_DB_ERROR, & ignore, & err_stub,
		0);
*/
    return(E_DB_ERROR);
}


/*{
** Name: qed_u2_set_interr - Set an internal error.
**
** Description:
**      This routine sets an internal error.
**  errors.
**
** Inputs:
**	i_setcode		    error code for setting, 0L if none
** Outputs:
**	o_dberr_p		    DB_ERROR
**	    .err_code		    i_setcode
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-jul-88 (carl)
**          written
*/

DB_STATUS
qed_u2_set_interr(
DB_ERRTYPE  i_setcode,
DB_ERROR    *o_dberr_p )
{
/*
    i4	ignore;
    DB_ERROR	err_stub;
*/

    o_dberr_p->err_code = i_setcode;
    qed_u10_trap();
/*
    qef_error(i_setcode, 0L, E_DB_ERROR, & ignore, & err_stub, 0);
*/
    return(E_DB_ERROR);
}


/*{
** Name: qed_u3_rqf_call - interface to RQF_CALL
**
** Description:
**      This routine maps a call to RQF_CALL and extracts the returned LDB
**	status if an error is encountered.
**  
** Inputs:
**	    i_call_id			RQF call id
**	    v_rqr_p			RQF_CB
**	    v_qer_p			QEF_RCB
**
** Outputs:
**	    none
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      16-apr-89 (carl)
**          written
**      12-may-88 (carl)
**	    changed to set QES_02CTL_COMMIT_CDB_TRAFFIC ON if the present 
**	    query is directed thru the privileged CDB association
**	27-jun-89 (carl)
**	    fixed lint problems and removed unused variables
**	29-jul-89 (carl)
**	    reinitialize the session's state if it has been interrupted
**	19-aug-89 (carl)
**	    added code to pass the tuple descriptor CB ptr provided by
**	    SCF to RQF 
**	07-oct-90 (carl)
**	    fixed to test trace point before displaying diagnostic information
**	05-feb-1993 (fpang)
**	    Added support to detect commits/aborts when executing local 
**	    database procedures or when executing direct execute immediate.
**	12-oct-1993 (fpang)
**	    Fixed to recognize that when qes_d10_timeout is negative, the user
**	    has specified no timeout, session timeout, or system timeout.
**	    Fixes B53486, "set lockmode session where timeout = 0" doesn't work.
**	03-oct-96 (pchang)
** 	    After returning from a successful remote OPEN CURSOR call 
**	    (RQR_OPEN), turn OFF the privileged CDB association commit flag 
**	    (QES_02CTL_COMMIT_CDB_TRAFFIC) if it had been turned on by previous
**	    remote CREATE TABLE calls (RQR_QUERY) used to implement UNION SELECT**	    in cursor definition.  This prevents premature closure of the cursor
**	    by an ensuing RQR_COMMIT message (B78777).
*/


DB_STATUS
qed_u3_rqf_call(
i4	i_call_id,
RQR_CB	*v_rqr_p,
QEF_RCB	*v_qer_p )
{
    DB_STATUS	    status;
    QEF_DDB_REQ	    *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *ldb_p = v_rqr_p->rqr_1_site;
    bool	    log_err_59 = FALSE;
    i4	    i4_1, i4_2;
 

    if (v_rqr_p->rqr_timeout == 0
	&&
	dds_p->qes_d10_timeout != 0)
	v_rqr_p->rqr_timeout = dds_p->qes_d10_timeout;
						/* use session timeout */

    /* always pass the tuple descriptor ptr from SCF to RQF assusming
    ** that's the case which is ok */

    v_rqr_p->rqr_tupdesc_p = ddr_p->qer_d10_tupdesc_p;
    v_rqr_p->rqr_tds_size = -1;		/* default unused field */

    status = rqf_call(i_call_id, v_rqr_p);
    if (status)
    {
	if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, 
	    QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
	{
	    /* display the error and call id */

	    log_err_59 = TRUE;
	    qed_w2_prt_rqferr(v_qer_p, i_call_id, v_rqr_p);
	}
	/* check if LDB aborted transaction or if the association is lost */

	if (v_rqr_p->rqr_ldb_status & RQF_03_TERM_MASK
	    ||
	    v_rqr_p->rqr_ldb_status & RQF_04_XACT_MASK
	    ||
	    v_rqr_p->rqr_error.err_code == E_RQ0045_CONNECTION_LOST
	    ||
	    v_rqr_p->rqr_error.err_code == E_RQ0046_RECV_TIMEOUT)
	{
	    qed_u10_trap();
	    ddr_p->qer_d13_ctl_info |= QEF_03DD_LDB_XACT_ENDED;
	    v_qer_p->qef_cb->qef_abort = TRUE;	/* msut abort STAR transaction 
						*/
	}

	if (v_rqr_p->rqr_ldb_status & RQF_01_FAIL_MASK)
	{
	    ddr_p->qer_d13_ctl_info |= QEF_01DD_LDB_GENERIC_ERROR;
	}

	if (v_rqr_p->rqr_ldb_status & RQF_02_STMT_MASK)
	{
	    ddr_p->qer_d13_ctl_info |= QEF_02DD_LDB_BAD_SQL_QUERY;
	}
	if (v_rqr_p->rqr_error.err_code == E_RQ0039_INTERRUPTED)
	    dds_p->qes_d7_ses_state = QES_0ST_NONE;
						/* reset state */
	v_qer_p->error.err_code = v_rqr_p->rqr_error.err_code;
	v_qer_p->error.err_data = v_rqr_p->rqr_error.err_data;

    }
    else
    {
	/* here if status == E_DB_OK */

	switch (i_call_id)
	{
	case RQR_DATA_VALUES:			/* internal query with data 
						** values */
	case RQR_NORMAL:			/* internal query */
	case RQR_QUERY:				/* user query */

	    if (ldb_p)
	    {
		if (ldb_p->dd_l5_ldb_id == 1	/* the CDB's unique id */
		    &&
		    ldb_p->dd_l1_ingres_b)	/* privileged */
		    dds_p->qes_d9_ctl_info |= QES_02CTL_COMMIT_CDB_TRAFFIC;
	    }
	    break;

	case RQR_OPEN:				/* open cursor */

	    if (dds_p->qes_d9_ctl_info & QES_02CTL_COMMIT_CDB_TRAFFIC)
		dds_p->qes_d9_ctl_info &= ~QES_02CTL_COMMIT_CDB_TRAFFIC;
	    break;

	default:
	    break;
	}
    }

    return(status);
}


/*{
** Name: qed_u4_qsf_acquire - acquire object in QSF storage
**
** Description:
**      This routine locks the current object in QSF's storage.
**
** Inputs:
**	i_qer_p			    ptr to QEF_RCB
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qef_qp		    name of query plan in QSF storage
**	    .qef_qso_handle	    handle of above query plan, NULL if none
**      v_qsr_p			    ptr to QSF_RCB
**	    .qsf_lk_state	    QSO_SHLOCK or QSO_EXLOCK
** Outputs:
**      v_qsr_p			    ptr to QSF_RCB
**	    .qsf_obj_id
**		.qso_handle	    object's handle
**	i_qer_p
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK			    successful completion
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      02-sep-88 (carl)
**          written
*/


DB_STATUS
qed_u4_qsf_acquire(
QEF_RCB		*i_qer_p,
QSF_RCB		*v_qsr_p )
{
    DB_STATUS		status;


    if (! (v_qsr_p->qsf_lk_state == QSO_SHLOCK 
	    ||
	   v_qsr_p->qsf_lk_state == QSO_EXLOCK))
    {
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }	

    /* 1.  get query control block from QSF */

    v_qsr_p->qsf_next = 0;
    v_qsr_p->qsf_prev = 0;
    v_qsr_p->qsf_length = sizeof(QSF_RCB);
    v_qsr_p->qsf_type = QSFRB_CB;
    v_qsr_p->qsf_owner = (PTR)DB_QEF_ID;
    v_qsr_p->qsf_ascii_id = QSFRB_ASCII_ID;
    v_qsr_p->qsf_sid = i_qer_p->qef_cb->qef_ses_id;

    if (i_qer_p->qef_qso_handle)
    {
	/* access QSF by handle */

	v_qsr_p->qsf_obj_id.qso_handle = i_qer_p->qef_qso_handle;
	status = qsf_call(QSO_LOCK, v_qsr_p);
	if (status)
	{
	    STRUCT_ASSIGN_MACRO(v_qsr_p->qsf_error, i_qer_p->error);
	    return(status);
	}
    }
    else
    {
	/* get object by name */

	v_qsr_p->qsf_obj_id.qso_type = QSO_QP_OBJ;
	MEcopy((PTR)& i_qer_p->qef_qp, sizeof (DB_CURSOR_ID), 
		(PTR) v_qsr_p->qsf_obj_id.qso_name);
	v_qsr_p->qsf_obj_id.qso_lname = sizeof (DB_CURSOR_ID);
	status = qsf_call(QSO_GETHANDLE, v_qsr_p);
	if (status)
	{
	    STRUCT_ASSIGN_MACRO(v_qsr_p->qsf_error, i_qer_p->error);
	    return(status);
	}

	/* set handle in i_qer_p */

	i_qer_p->qef_qso_handle = v_qsr_p->qsf_obj_id.qso_handle;
    }

    return(E_DB_OK);
}


/*{
** Name: qed_u5_qsf_destroy - destroy object in QSF storage
**
** Description:
**      This routine destroys the current object in QSF's storage.
**
** Inputs:
**	i_qer_p			    ptr to QEF_RCB
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qef_qp		    name of query plan in QSF storage
**	    .qef_qso_handle	    handle of above query plan, NULL if none
**      i_qsr_p			    ptr to QSF_RCB
**	    .qsf_lk_state	    QSO_SHLOCK or QSO_EXLOCK
**	    .qsf_obj_id
**		.qso_handle	    object's handle
** Outputs:
**	none
**
**	i_qer_p
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK			    successful completion
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      02-sep-88 (carl)
**          written
*/


DB_STATUS
qed_u5_qsf_destroy(
QEF_RCB		*i_qer_p,
QSF_RCB		*i_qsr_p )
{
    DB_STATUS		status;


/*
    i_qsr_p->qsf_next = 0;
    i_qsr_p->qsf_prev = 0;
    i_qsr_p->qsf_length = sizeof(QSF_RCB);
    i_qsr_p->qsf_type = QSFRB_CB;
    i_qsr_p->qsf_owner = (PTR)DB_QEF_ID;
    i_qsr_p->qsf_ascii_id = QSFRB_ASCII_ID;
    i_qsr_p->qsf_sid = i_qer_p->qef_cb->qef_ses_id;
*/
    status = qsf_call(QSO_DESTROY, i_qsr_p);
    if (status)
	STRUCT_ASSIGN_MACRO(i_qsr_p->qsf_error, i_qer_p->error);

    return(status);
}


/*{
** Name: qed_u6_qsf_root - Get root of object in QSF storage
**
** Description:
**      This routine requests QSF to produce the root of an object.
**
** Inputs:
**	i_qer_p			    ptr to QEF_RCB
**	    .qef_qso_handle	    handle 
**      v_qsr_p			    ptr to QSF_RCB
**	    .qsf_lk_state	    QSO_SHLOCK or QSO_EXLOCK
** Outputs:
**      v_qsr_p			    ptr to QSF_RCB
**	    .qsf_root		    object's root
**	i_qer_p
**          .error		    One of the following
**		.err_code
**                                  E_QE0000_OK
**                                  (To be specified)
**
**      Returns:
**          E_DB_OK			    successful completion
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      02-sep-88 (carl)
**          written
*/


DB_STATUS
qed_u6_qsf_root(
QEF_RCB		*i_qer_p,
QSF_RCB		*v_qsr_p )
{
    DB_STATUS		status;


    if (! (v_qsr_p->qsf_lk_state == QSO_SHLOCK 
	    ||
	   v_qsr_p->qsf_lk_state == QSO_EXLOCK))
    {
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }	

    /* 1.  get query control block from QSF */

    v_qsr_p->qsf_next = 0;
    v_qsr_p->qsf_prev = 0;
    v_qsr_p->qsf_length = sizeof(QSF_RCB);
    v_qsr_p->qsf_type = QSFRB_CB;
    v_qsr_p->qsf_owner = (PTR)DB_QEF_ID;
    v_qsr_p->qsf_ascii_id = QSFRB_ASCII_ID;
    v_qsr_p->qsf_sid = i_qer_p->qef_cb->qef_ses_id;

    /* access QSF by handle */

    v_qsr_p->qsf_obj_id.qso_handle = i_qer_p->qef_qso_handle;
    status = qsf_call(QSO_SETROOT, v_qsr_p);
    if (status)
	STRUCT_ASSIGN_MACRO(v_qsr_p->qsf_error, i_qer_p->error);
    return(status);
}


/*{
** Name: qed_u7_msg_commit - Terminate internal LDB access by sending a commit
**			     message.
**
** Description:
**
**	This routine sends a commit message to a specified LDB to terminate 
**  internal access, typically used to end iidbdb access to avoid holding 
**  resources unnecessarily.
**
** Inputs:
**	    v_qer_p			QEF_RCB
**              .qef_cb			QEF_CB
**	    i_ldb_p			ptr to DD_LDB_DESC for LDB
** Outputs:
**	none
**          Returns:
**		E_DB_OK                 
**		E_DB_ERROR              caller error
**		E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	23-sep-88 (carl)
**          written
**	30-may-89 (carl)
**	    changed to call qed_u3_rqf_call to take advantage of error
**	    analysis
**	27-jun-89 (carl)
**	    removed unused variable cdb_p
*/


DB_STATUS
qed_u7_msg_commit(
QEF_RCB         *v_qer_p,
DD_LDB_DESC	*i_ldb_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    RQR_CB	    rqr_cb,
		    *rqr_p = & rqr_cb;


    v_qer_p->error.err_code = E_QE0000_OK;

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = i_ldb_p;
    rqr_p->rqr_q_language = DB_SQL;

    rqr_p->rqr_msg.dd_p1_len = 0;
    rqr_p->rqr_msg.dd_p2_pkt_p = (char *) NULL;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    status = qed_u3_rqf_call(RQR_COMMIT, rqr_p, v_qer_p);

    return(status);
}


/*{
** Name: qed_u8_gmt_now - Get current time in GMT format.
**
** Description:
**	Call ADF to get the current time in GMT format.
**
** Inputs:
**	None
**
** Outputs:
**      o_gmt_now			    ptr to DD_DATE
**	i_qer_p				    QEF_RCB
**	    .error
**		.err_code		    (to be specified)
**	Returns:
**	    DB_STATUS
**
**	Exceptions:
**	    none
**
** Side Effects:
**	none.
**
** History:
**	17-oct-88 (alexc)
**	    initial creation.
**      2-Jun-1989 (alexc)
**          remove call to scf to get adf control block because QEF
**             is already passing in an adf control block pointer.
**             Also use buffer for dv_date.db_data pointer.
**	09-Feb-1993 (fpang)
**	    Fixed adf() problems found during prototyping.
**	16-dec-04 (inkdo01)
**	    Added a collID.
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
*/


DB_STATUS
qed_u8_gmt_now(
QEF_RCB	    *i_qer_p,
DD_DATE	    o_gmt_now )
{
    DB_STATUS	    status;
    i4		    nfi;
    ADI_OP_ID	    op_date;
    ADI_FI_TAB	    fitab;
    DB_DATA_VALUE   dv_now;
    DB_DATA_VALUE   dv_date;
    DB_DATA_VALUE   dv_gmt;
    ADF_CB	    *adf_scb = i_qer_p->qef_cb->qef_adf_cb;
    ADI_FI_DESC	    *fi1 = NULL;
    ADI_FI_DESC	    *fi2 = NULL;
    char	    gmt_buffer[100];	/* Avoid dynamic allocation */
    ADI_OP_NAME	    op_name;

    MEfill((u_i4) sizeof(DD_DATE), ' ', o_gmt_now);

    /* set up DB_DATA_VALUES */

    /* get op id, and set up the fi table */

    do 
    {	/* something to break out of... */
	status = E_DB_OK;
	MEmove( sizeof("date"), "date", '\0', 
		sizeof(op_name.adi_opname), op_name.adi_opname);
	status = adi_opid(adf_scb, &op_name, &op_date);

	if (DB_FAILURE_MACRO(status))
	    break;

	status = adi_fitab(adf_scb, op_date, &fitab);

	if (DB_FAILURE_MACRO(status))
	    break;

	/* search the function instance in the list of function 
	** instances pointed to by fitab.adi_tab_fi.
	*/

	fi1 = fitab.adi_tab_fi;
	nfi = fitab.adi_lntab;
    }
    while (0);

    if (DB_FAILURE_MACRO(status))
    {
	/* Error on gmt_buffer length */
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    while (nfi != 0)
    {
        if (fi1->adi_dt[0] == DB_CHA_TYPE)
        {
	    break;
	}
	else
	{
	    fi1++;
	    nfi--;
	}
    }


    do
    {	/* Something to break out of... */
	status = E_DB_OK;
	MEmove( sizeof("date_gmt"), "date_gmt", '\0', 
		sizeof(op_name.adi_opname), op_name.adi_opname);
	status = adi_opid(adf_scb, &op_name, &op_date);

	if (DB_FAILURE_MACRO(status))
	    break;

	status = adi_fitab(adf_scb, op_date, &fitab);

	if (DB_FAILURE_MACRO(status))
	    break;

	fi2 = fitab.adi_tab_fi;
        nfi = fitab.adi_lntab;
    }
    while (0);

    if (DB_FAILURE_MACRO(status))
    {
	/* Error on gmt_buffer length */
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    while (nfi != 0)
    {
        if (fi2->adi_dt[0] == DB_DTE_TYPE)
	{
	    break;
	}
	else
	{
	    fi2++;
	    nfi--;
	}
    }

    dv_now.db_prec	= dv_date.db_prec = dv_gmt.db_prec = 0;
    dv_now.db_collID	= dv_date.db_collID = dv_gmt.db_collID = -1;
    dv_now.db_datatype  = DB_CHA_TYPE;
    dv_now.db_length    = 3;
    dv_now.db_data	= (PTR)"now";

    status = adi_calclen(adf_scb, &fi1->adi_lenspec, &dv_now, NULL, &dv_date.db_length);

    if (DB_FAILURE_MACRO(status))
    {
        /* Error on gmt_buffer length */
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }
    dv_date.db_data = (PTR) &gmt_buffer[0];

    dv_date.db_datatype = fi1->adi_dtresult;

    {
	ADF_FN_BLK now2date;
	now2date.adf_fi_id = fi1->adi_finstid;
	now2date.adf_fi_desc = NULL;
	now2date.adf_dv_n = 1;
	STRUCT_ASSIGN_MACRO(dv_now, now2date.adf_1_dv);
	STRUCT_ASSIGN_MACRO(dv_date, now2date.adf_r_dv);
	now2date.adf_pat_flags = AD_PAT_NO_ESCAPE;

	status = adf_func(adf_scb, &now2date);
    }

    if (!DB_FAILURE_MACRO(status))
	status = adi_calclen(adf_scb, &fi2->adi_lenspec, &dv_date, NULL, &dv_gmt.db_length);

    if (DB_FAILURE_MACRO(status))
    {
        /* Error on gmt_buffer length */
	status = qed_u1_gen_interr(& i_qer_p->error);
	return(status);
    }

    dv_gmt.db_data = o_gmt_now;
    dv_gmt.db_datatype = fi2->adi_dtresult;

    {
	ADF_FN_BLK date2gmt;

	date2gmt.adf_fi_id = fi2->adi_finstid;
	date2gmt.adf_fi_desc = NULL;
	date2gmt.adf_dv_n = 1;
	STRUCT_ASSIGN_MACRO(dv_date, date2gmt.adf_1_dv);
	STRUCT_ASSIGN_MACRO(dv_gmt, date2gmt.adf_r_dv);
	date2gmt.adf_pat_flags = AD_PAT_NO_ESCAPE;

	return adf_func(adf_scb, &date2gmt);
    }
}


/*{
** Name: qed_u9_log_forgiven - Log a forgiven error.
**
** Description:
**      This routine logs a forgiven error, e.g., in the cases of DROP 
**  LINK/TABLE/VIEW.
**
** Inputs:
**	i_err_p				DB_ERROR
**	    .err_code
** Outputs:
**	None
**
**	Returns:
**	    nothing
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      24-jul-88 (carl)
**          written
**      01-apr-89 (carl)    
**	    modified to return an error to abort the transaction if RQF has 
**	    reported a user interrupt
**      02-may-89 (carl)    
**	    changed qed_u9_log_forgiven to not forgive 
**	    E_QE0704_MULTI_SITE_WRITE
*/

DB_STATUS
qed_u9_log_forgiven(
QEF_RCB	    *i_qer_p )
{
    DB_STATUS	status = E_DB_OK;   /* preset */
    i4	ignore;
    DB_ERROR    error;


    switch (i_qer_p->error.err_code)
    {
    case E_RQ0039_INTERRUPTED:

	i_qer_p->qef_cb->qef_abort = TRUE;
	i_qer_p->error.err_code = E_QE0022_QUERY_ABORTED;
	status = E_DB_ERROR;
	break;

    case E_TP0004_MULTI_SITE_WRITE:
    case E_QE0704_MULTI_SITE_WRITE:

	status = E_DB_ERROR;
	break;

    default:
	qef_error(i_qer_p->error.err_code, 0L, E_DB_ERROR, & ignore, & error,
		    0);
					/* call qef_error to log error */
    } /* end case */

    return(status);
}



/*{
** Name: qed_u10_trap - Trap an error
**
** Description:
**      This routine serves as a trap point for catching errors.
**
** Inputs:
**	None
**
** Outputs:
**	None
**
**	Returns:
**	    nothing
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-nov-88 (carl)
**          written
*/

VOID
qed_u10_trap()
{
    /* do nothing */
}


/*{
** Name: qed_u11_ddl_commit - commit the CDB association if necessary
**
** Description:
**
**	This routine commits the special CDB association if the following
**  conditions are satisfied:
**  
**	    1.  DDL_CONCURRENCY is ON,
**	    2.  COMMIT_CDB_TRAFFIC is ON indicating there has been queries
**	        to the CDB,
**	    3.  AUTOCOMMIT is OFF (if ON, TPF can be relied to do the commit)
**
** Inputs:
**	    qer_p			QEF_RCB
**              .qef_cb			QEF_CB
**	    
** Outputs:
**	none
**          Returns:
**		E_DB_OK                 
**		E_DB_ERROR              caller error
**	
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	15-apr-89 (carl)
**          written
**	12-may-89 (carl)
**	    changed to check if QES_02CTL_COMMIT_CDB_TRAFFIC
**	30-may-89 (carl)
**	    changed to check if AUTOCOMMIT is not ON
*/


DB_STATUS
qed_u11_ddl_commit(
QEF_RCB         *qer_p )
{
    DB_STATUS	    status = E_DB_OK;		/* must set */
    QES_DDB_SES	    *dds_p = & qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;


    if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON 
	&&
	dds_p->qes_d9_ctl_info & QES_02CTL_COMMIT_CDB_TRAFFIC
	&&
	qer_p->qef_cb->qef_auto != QEF_ON)
    {
	dds_p->qes_d9_ctl_info &= ~QES_02CTL_COMMIT_CDB_TRAFFIC;
						/* must reset */
	status = qed_u7_msg_commit(qer_p, cdb_p);
    }
    return(status);
}


/*{
** Name: qed_u12_map_rqferr - Set an error generated by RQF.
**
** Description:
**      This routine sets an LDB error generated by RQF.
**
** Inputs:
**	i_rqr_p				RQR_CB
**	    .rqr_error
**		.err_code
**
** Outputs:
**	o_err_p				QEF_RCB
**	    .err_code			E_QE0981_RQF_REPORTED_ERROR
**	Returns:
**	    nothing
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-oct-88 (carl)
**          written
**      12-may-88 (carl)
**	    map E_RQ0039_INTERRUPTED to E_QE0022_QUERY_ABORTED, an existing
**	    message
**	06-jun-89 (carl)
**	    map E_RQ0047_UNAUTHORIZED_USER to E_QE0547_UNAUTHORIZED_USER 
**	27-jun-89 (carl)
**	    removed unused variables err_stub and ignore
*/


VOID
qed_u12_map_rqferr(
RQR_CB		*i_rqr_p,
DB_ERROR	*o_err_p )
{


    switch (i_rqr_p->rqr_error.err_code)
    {
    case E_RQ0001_WRONG_COLUMN_COUNT:
	o_err_p->err_code = E_QE0501_WRONG_COLUMN_COUNT;
	break;
    case E_RQ0002_NO_TUPLE_DESCRIPTION:
	o_err_p->err_code = E_QE0502_NO_TUPLE_DESCRIPTION;
	break;
    case E_RQ0003_TOO_MANY_COLUMNS:
	o_err_p->err_code = E_QE0503_TOO_MANY_COLUMNS;
	break;
    case E_RQ0004_BIND_BUFFER_TOO_SMALL:
	o_err_p->err_code = E_QE0504_BIND_BUFFER_TOO_SMALL;
	break;
    case E_RQ0005_CONVERSION_FAILED:
	o_err_p->err_code = E_QE0505_CONVERSION_FAILED;
	break;
    case E_RQ0006_CANNOT_GET_ASSOCIATION:
	o_err_p->err_code = E_QE0506_CANNOT_GET_ASSOCIATION;
	break;
    case E_RQ0007_BAD_REQUEST_CODE:
	o_err_p->err_code = E_QE0507_BAD_REQUEST_CODE;
	break;
    case E_RQ0008_SCU_MALLOC_FAILED:
	o_err_p->err_code = E_QE0508_SCU_MALLOC_FAILED;
	break;
    case E_RQ0009_ULM_STARTUP_FAILED:
	o_err_p->err_code = E_QE0509_ULM_STARTUP_FAILED;
	break;
    case E_RQ0010_ULM_OPEN_FAILED:
	o_err_p->err_code = E_QE0510_ULM_OPEN_FAILED;
	break;
    case E_RQ0011_INVALID_READ:
	o_err_p->err_code = E_QE0511_INVALID_READ;
	break;
    case E_RQ0012_INVALID_WRITE:
	o_err_p->err_code = E_QE0512_INVALID_WRITE;
	break;
    case E_RQ0013_ULM_CLOSE_FAILED:
	o_err_p->err_code = E_QE0513_ULM_CLOSE_FAILED;
	break;
    case E_RQ0014_QUERY_ERROR:
	o_err_p->err_code = E_QE0514_QUERY_ERROR;
	break;
    case E_RQ0015_UNEXPECTED_MESSAGE:
	o_err_p->err_code = E_QE0515_UNEXPECTED_MESSAGE;
	break;
    case E_RQ0016_CONVERSION_ERROR:
	o_err_p->err_code = E_QE0516_CONVERSION_ERROR;
	break;
    case E_RQ0017_NO_ACK:
	o_err_p->err_code = E_QE0517_NO_ACK;
	break;
    case E_RQ0018_SHUTDOWN_FAILED:
	o_err_p->err_code = E_QE0518_SHUTDOWN_FAILED;
	break;
    case E_RQ0019_COMMIT_FAILED:
	o_err_p->err_code = E_QE0519_COMMIT_FAILED;
	break;
    case E_RQ0020_ABORT_FAILED:
	o_err_p->err_code = E_QE0520_ABORT_FAILED;
	break;
    case E_RQ0021_BEGIN_FAILED:
	o_err_p->err_code = E_QE0521_BEGIN_FAILED;
	break;
    case E_RQ0022_END_FAILED:
	o_err_p->err_code = E_QE0522_END_FAILED;
	break;
    case E_RQ0023_COPY_FROM_EXPECTED:
	o_err_p->err_code = E_QE0523_COPY_FROM_EXPECTED;
	break;
    case E_RQ0024_COPY_DEST_FAILED:
	o_err_p->err_code = E_QE0524_COPY_DEST_FAILED;
	break;
    case E_RQ0025_COPY_SOURCE_FAILED:
	o_err_p->err_code = E_QE0525_COPY_SOURCE_FAILED;
	break;
    case E_RQ0026_QID_EXPECTED:
	o_err_p->err_code = E_QE0526_QID_EXPECTED;
	break;
    case E_RQ0027_CURSOR_CLOSE_FAILED:
	o_err_p->err_code = E_QE0527_CURSOR_CLOSE_FAILED;
	break;
    case E_RQ0028_CURSOR_FETCH_FAILED:
	o_err_p->err_code = E_QE0528_CURSOR_FETCH_FAILED;
	break;
    case E_RQ0029_CURSOR_EXEC_FAILED:
	o_err_p->err_code = E_QE0529_CURSOR_EXEC_FAILED;
	break;
    case E_RQ0030_CURSOR_DELETE_FAILED:
	o_err_p->err_code = E_QE0530_CURSOR_DELETE_FAILED;
	break;
    case E_RQ0031_INVALID_CONTINUE:
	o_err_p->err_code = E_QE0531_INVALID_CONTINUE;
	break;
    case E_RQ0032_DIFFERENT_TUPLE_SIZE:
	o_err_p->err_code = E_QE0532_DIFFERENT_TUPLE_SIZE;
	break;
    case E_RQ0033_FETCH_FAILED:
	o_err_p->err_code = E_QE0533_FETCH_FAILED;
	break;
    case E_RQ0034_COPY_CREATE_FAILED:
	o_err_p->err_code = E_QE0534_COPY_CREATE_FAILED;
	break;
    case E_RQ0035_BAD_COL_DESC_FORMAT:
	o_err_p->err_code = E_QE0535_BAD_COL_DESC_FORMAT;
	break;
    case E_RQ0036_COL_DESC_EXPECTED:
	o_err_p->err_code = E_QE0536_COL_DESC_EXPECTED;
	break;
    case E_RQ0037_II_LDB_NOT_DEFINED:
	o_err_p->err_code = E_QE0537_II_LDB_NOT_DEFINED;
	break;
    case E_RQ0038_ULM_ALLOC_FAILED:
	o_err_p->err_code = E_QE0538_ULM_ALLOC_FAILED;
	break;
    case E_RQ0039_INTERRUPTED:
	o_err_p->err_code = E_QE0022_QUERY_ABORTED;
	break;
    case E_RQ0040_UNKNOWN_REPEAT_Q:
	o_err_p->err_code = E_QE0540_UNKNOWN_REPEAT_Q;
	break;
    case E_RQ0041_ERROR_MSG_FROM_LDB:
	o_err_p->err_code = E_QE0541_ERROR_MSG_FROM_LDB;
	break;
    case E_RQ0042_LDB_ERROR_MSG:
	o_err_p->err_code = E_QE0542_LDB_ERROR_MSG;
	break;
    case E_RQ0043_CURSOR_UPDATE_FAILED:
	o_err_p->err_code = E_QE0543_CURSOR_UPDATE_FAILED;
	break;
    case E_RQ0044_CURSOR_OPEN_FAILED:
	o_err_p->err_code = E_QE0544_CURSOR_OPEN_FAILED;
	break;
    case E_RQ0045_CONNECTION_LOST:
	o_err_p->err_code = E_QE0545_CONNECTION_LOST;
	break;
    case E_RQ0046_RECV_TIMEOUT:
	o_err_p->err_code = E_QE0546_RECV_TIMEOUT;
	break;
    case E_RQ0047_UNAUTHORIZED_USER:
	o_err_p->err_code = E_QE0547_UNAUTHORIZED_USER;
	break;
    case E_RQ0048_SECURE_FAILED:
	o_err_p->err_code = E_QE0548_SECURE_FAILED;
	break;
    case E_RQ0049_RESTART_FAILED:
	o_err_p->err_code = E_QE0549_RESTART_FAILED;
	break;
    default:
	/* RQF's responsibility to bring QEF's error mapping up to date */
	break;
    }
}


/*{
** Name: qed_u13_map_tpferr - Set an error generated by TPF.
**
** Description:
**      This routine sets an LDB error generated by TPF.
**
** Inputs:
**	i_tpr_p				TPR_CB
**	    .tpr_error
**		.err_code
** Outputs:
**	o_err_p				DB_ERROR
**	    .err_code			E_QE0982_TPF_REPORTED_ERROR
**	Returns:
**	    nothing
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      21-oct-88 (carl)
**          written
**	27-jun-89 (carl)
**	    removed unused variables ignore and err_stub
**	22-jul-89 (carl)
**	    fixed to include the full complement of TPF errors
*/


VOID
qed_u13_map_tpferr(
TPR_CB		*i_tpr_p,
DB_ERROR	*o_err_p )
{
/*
    i4	ignore;
    DB_ERROR	err_stub;
*/

    switch (i_tpr_p->tpr_error.err_code)
    {
    case E_TP0001_INVALID_REQUEST:
	o_err_p->err_code = E_QE0701_INVALID_REQUEST; 
	break;
    case E_TP0002_SCF_MALLOC_FAILED:	
	o_err_p->err_code = E_QE0702_SCF_MALLOC_FAILED;
	break;
    case E_TP0003_SCF_MFREE_FAILED:	
	o_err_p->err_code = E_QE0703_SCF_MFREE_FAILED;
	break;
    case E_TP0004_MULTI_SITE_WRITE:	
	o_err_p->err_code = E_QE0704_MULTI_SITE_WRITE;
	break;
    case E_TP0005_UNKNOWN_STATE:	
	o_err_p->err_code = E_QE0705_UNKNOWN_STATE;
	break;
    case E_TP0006_INVALID_EVENT:	
	o_err_p->err_code = E_QE0706_INVALID_EVENT;
	break;
    case E_TP0007_INVALID_TRANSITION:
	o_err_p->err_code = E_QE0707_INVALID_TRANSITION;
	break;
    case E_TP0008_TXN_BEGIN_FAILED:	
	o_err_p->err_code = E_QE0708_TXN_BEGIN_FAILED;
	break;
    case E_TP0009_TXN_FAILED:	
	o_err_p->err_code = E_QE0709_TXN_FAILED;
	break;
    case E_TP0010_SAVEPOINT_FAILED:
	o_err_p->err_code = E_QE0710_SAVEPOINT_FAILED;
	break;
    case E_TP0011_SP_ABORT_FAILED:	
	o_err_p->err_code = E_QE0711_SP_ABORT_FAILED;
	break;
    case E_TP0012_SP_NOT_EXIST:	
	o_err_p->err_code = E_QE0712_SP_NOT_EXIST;
	break;
    case E_TP0013_AUTO_ON_NO_SP:	
	o_err_p->err_code = E_QE0713_AUTO_ON_NO_SP;
	break;
    case E_TP0014_ULM_STARTUP_FAILED:	
	o_err_p->err_code = E_QE0714_ULM_STARTUP_FAILED;
	break;
    case E_TP0015_ULM_OPEN_FAILED:	
	o_err_p->err_code = E_QE0715_ULM_OPEN_FAILED;
	break;
    case E_TP0016_ULM_ALLOC_FAILED:	
	o_err_p->err_code = E_QE0716_ULM_ALLOC_FAILED;
	break;
    case E_TP0017_ULM_CLOSE_FAILED:	
	o_err_p->err_code = E_QE0717_ULM_CLOSE_FAILED;
	break;
    case E_TP0020_INTERNAL:	
	o_err_p->err_code = E_QE0720_TPF_INTERNAL;
	break;
    case E_TP0021_LOGDX_FAILED:	
	o_err_p->err_code = E_QE0721_LOGDX_FAILED;
	break;
    case E_TP0022_SECURE_FAILED:	
	o_err_p->err_code = E_QE0722_SECURE_FAILED;
	break;
    case E_TP0023_COMMIT_FAILED:	
	o_err_p->err_code = E_QE0723_COMMIT_FAILED;
	break;
    case E_TP0024_DDB_IN_RECOVERY:	
	o_err_p->err_code = E_QE0724_DDB_IN_RECOVERY;
	break;
    default:
	/* TPF's responsibility to map into RQF's error range */
	break;
    }
}


/*{
** Name: qed_u14_pre_commit - pre-commit the CDB association if necessary
**
** Description:
**
**	This routine commits the special CDB association if the following
**  conditions are satisfied:
**  
**	    1.  DDL_CONCURRENCY is ON,
**	    2.  COMMIT_CDB_TRAFFIC is ON indicating there has been queries
**	        to the CDB,
**
** Inputs:
**	    qer_p			QEF_RCB
**              .qef_cb			QEF_CB
**	    
** Outputs:
**	none
**          Returns:
**		E_DB_OK                 
**		E_DB_ERROR              caller error
**	
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	30-may-89 (carl)
**          adapted from qed_u11_ddl_commit
*/


DB_STATUS
qed_u14_pre_commit(
QEF_RCB         *qer_p )
{
    DB_STATUS	    status = E_DB_OK;		/* must set */
    QES_DDB_SES	    *dds_p = & qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;


    if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON 
	&&
	dds_p->qes_d9_ctl_info & QES_02CTL_COMMIT_CDB_TRAFFIC)
    {
	dds_p->qes_d9_ctl_info &= ~QES_02CTL_COMMIT_CDB_TRAFFIC;
						/* must reset */
	status = qed_u7_msg_commit(qer_p, cdb_p);
    }
    return(status);
}


/*{
** Name: qed_u15_trace_qefcall - Trace a QEF call.
**
** Description:
**      This routine traces a given QEF call by writing its identity to the log.
**
** Inputs:
**	i_qefcb_p			QEF_CB
**	i_call_id			QEF operation code
**
** Outputs:
**	none
**
**	Returns:
**	    nothing
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      01-sep-90 (carl)
**          written
*/


VOID
qed_u15_trace_qefcall(
QEF_CB		*i_qefcb_p,
i4		i_call_id )
{
    QEF_RCB	qef_rcb;


    MEfill(sizeof(qef_rcb), '\0', (PTR) & qef_rcb);
    qef_rcb.qef_cb = i_qefcb_p;		/* only field needed by qec_tprintf */

    /* display "QEF xxxxxx: tracing XXX..." */

    STprintf(i_qefcb_p->qef_trfmt,
	"%s %p: %s ",
	IIQE_61_qefsess, 
	(PTR) i_qefcb_p,
	IIQE_65_tracing);
    qec_tprintf(& qef_rcb, i_qefcb_p->qef_trsize, i_qefcb_p->qef_trfmt);
    qed_w4_prt_qefcall(i_qefcb_p, i_call_id);
    STprintf(i_qefcb_p->qef_trfmt, "%s\n", IIQE_62_3dots);
    qec_tprintf(& qef_rcb, i_qefcb_p->qef_trsize, i_qefcb_p->qef_trfmt);
}


/*{
** Name: qed_u16_rqf_cleanup - clean a current session
**
** Description:
**	This routine requests RQF to clean up all the associations for 
**  the current session.
**
** Inputs:
**	    qer_p			QEF_RCB
**              .qef_cb			QEF_CB
** Outputs:
**	none
**          Returns:
**		DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	22-mar-90 (carl)
**	    created 
*/


DB_STATUS
qed_u16_rqf_cleanup(
QEF_RCB         *qer_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & qer_p->qef_cb->qef_c2_ddb_ses;
    RQR_CB	    rqr_cb,
		    *rqr_p = & rqr_cb;


    qer_p->error.err_code = E_QE0000_OK;

    rqr_p->rqr_session = dds_p->qes_d3_rqs_p;  /* RQF session cb ptr */
    rqr_p->rqr_timeout = 0;
    rqr_p->rqr_1_site = (DD_LDB_DESC *) NULL;
    rqr_p->rqr_q_language = DB_SQL;

    rqr_p->rqr_msg.dd_p1_len = 0;
    rqr_p->rqr_msg.dd_p2_pkt_p = (char *) NULL;
    rqr_p->rqr_msg.dd_p3_nxt_p = (DD_PACKET *) NULL;

    status = qed_u3_rqf_call(RQR_CLEANUP, rqr_p, qer_p);

    return(status);
}


/*{
** Name: qed_u17_tpf_call - interface to TPF_CALL
**
** Description:
**      This routine maps a call to TPF_CALL and extracts the returned 
**	status if an error is encountered.
**  
** Inputs:
**	    i_call_id			TPF call id
**	    v_tpr_p			TPF_CB
**	    v_qer_p			QEF_RCB
**
** Outputs:
**	    none
**
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-aug-90 (carl)
**          written
**	07-oct-90 (carl)
**	    fixed to test trace point before displaying diagnostic information
**	21-oct-90 (carl)
**	    changed qed_u17_tpf_call to pass QEF's session id to TPF for
**	    tracing to the FE
**      15-nov-94 (rudtr01)
**          initialize TPF's trace buffers before calling TPF in
**          qed_u17_tpf_call()
**	10-jul-97 (nanpr01)
**	    Took out unused extra local variables.
*/


DB_STATUS
qed_u17_tpf_call(
i4	i_call_id,
TPR_CB	*v_tpr_p,
QEF_RCB	*v_qer_p )
{
    DB_STATUS	    status;
    bool	    log_err_59 = FALSE;
    i4	    i4_1, i4_2;


    v_tpr_p->tpr_19_qef_sess = v_qer_p->qef_cb->qef_ses_id;
					    /* must pass to enable TPF
					    ** to call SCC_TRACE */
    v_tpr_p->tpr_trfmt = v_qer_p->qef_cb->qef_trfmt;
    v_tpr_p->tpr_trsize = v_qer_p->qef_cb->qef_trsize;

    status = tpf_call(i_call_id, v_tpr_p);
    if (status)
    {
	if (ult_check_macro(& v_qer_p->qef_cb->qef_trace, 
	    QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
	{
	    /* display the error and call id */

	    log_err_59 = TRUE;
	    qed_w3_prt_tpferr(v_qer_p, i_call_id, v_tpr_p);
	}
	v_qer_p->error.err_code = v_tpr_p->tpr_error.err_code;
	v_qer_p->error.err_data = v_tpr_p->tpr_error.err_data;
    }
    return(status);
}

