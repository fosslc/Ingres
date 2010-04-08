/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
#include    <me.h>
#include    <st.h>
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
**  Name: QEDWRT.C - Utility functions for displaying diagnostic information
**
**  Description:
**	qed_w0_prt_gmtnow   - display time
**	qed_w1_prt_ldbdesc  - display the contents of an LDB descriptor
**	qed_w2_prt_rqferr   - display RQF error
**	qed_w3_prt_tpferr   - display TPF error
**	qed_w4_prt_qefcall  - display QEF call
**	qed_w5_prt_qeferr   - display QEF error
**	qed_w6_prt_qperror  - display banner line for a query plan error 
**	qed_w7_prt_qryinfo  - display user's query information
**	qed_w8_prt_qefqry   - display QEF query information
**	qed_w9_prt_db_val   - display the information of a data type
**	qed_w10_prt_qrytxt  - display query text (segment)
**
**  History:
**      18-sep-90 (carl)    
**          created
**      07-oct-90 (carl)    
**	    fixed qed_w2_prt_rqferr to suppress displaying interrupt error
**      27-oct-90 (carl)
**	    improved qed_w9_prt_db_val's display contents
**	26-MAR-92 (fpang)
**	    In qed_w9_prt_db_val(), don't display DB_DTE_TYPE types. 
**	    It is a binary encoding.
**	    Fixes B42842.
**	08-dec-92 (anitap)
**	    Included <psfparse.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      10-sep-93 (smc)
**          Removed redundant non-portable casts of p1 & p2. Used %p
**          to print out pointers. Moved <cs.h> for CS_SID.
**       6-Oct-1993 (fred)
**          Fixed qed_w9_prt_db_val() so that it won't have to be
**          seriously changed for each and every new datatype.  Folks
**          got to start using ADF for what it's for.
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
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/

GLOBALREF   char     IIQE_33_integer[];
GLOBALREF   char     IIQE_34_float[];
GLOBALREF   char     IIQE_36_date[];
GLOBALREF   char     IIQE_37_money[];
GLOBALREF   char     IIQE_38_char[];
GLOBALREF   char     IIQE_39_varchar[];
GLOBALREF   char     IIQE_40_decimal[];
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
** Name: qed_w0_prt_gmtnow - display current time
**
** Description:
**      
**
** Inputs:
**      v_qer_p                ptr to QEF_RCB
**
** Outputs:
**      none
**
**      Returns:
**          E_DB_ERROR
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      26-aug-90 (carl)
**          created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/

DB_STATUS
qed_w0_prt_gmtnow(
QEF_RCB	    *v_qer_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    SYSTIME	    now;
    char	    now_str[DD_25_DATE_SIZE + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    TMnow(& now);
    TMstr(& now, now_str);

    STprintf(cbuf, "\n%s %p: %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
        now_str);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(E_DB_OK);
}


/*{
** Name: qed_w1_prt_ldbdesc - display an LDB descriptor
**
** Description:
**      (See above comment.)
**
** Inputs:
**	v_qer_p			    ptr to QEF_RCB
**	i_ldb_p			    ptr to LDB descriptor
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
**	21-jul-90 (carl)
**          written
*/


DB_STATUS
qed_w1_prt_ldbdesc(
QEF_RCB		*v_qer_p,
DD_LDB_DESC	*i_ldb_p )
{
    DB_STATUS	    status = E_DB_OK;
    QEF_CB	    *qecb_p = v_qer_p->qef_cb;
    char	    namebuf[DB_MAXNAME + 1];
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (i_ldb_p == (DD_LDB_DESC *) NULL)
    {
	STprintf(cbuf, "%s %p: Bad LDB pointer detected!\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	return(qed_u1_gen_interr(& v_qer_p->error));
    }
    qed_u0_trimtail(i_ldb_p->dd_l2_node_name,
	(u_i4) DB_MAXNAME,
	namebuf);

    STprintf(cbuf, "%s %p: ...   1) NODE   %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	namebuf);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qed_u0_trimtail(i_ldb_p->dd_l3_ldb_name,
	(u_i4) DB_MAXNAME,
	namebuf);

    STprintf(cbuf, "%s %p: ...   2) LDB    %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	namebuf);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qed_u0_trimtail(i_ldb_p->dd_l4_dbms_name,
	(u_i4) DB_MAXNAME,
	namebuf);

    STprintf(cbuf, "%s %p: ...   3) DBMS   %s\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	namebuf);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    STprintf(cbuf, "%s %p: ...   4) LDB id %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	i_ldb_p->dd_l5_ldb_id);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
/*
    if (i_ldb_p->dd_l6_flags & DD_01FLAG_1PC)
    {
	STprintf(cbuf, "%s %p: ...   this CDB has no 2PC catalogs\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
*/
    if (i_ldb_p->dd_l1_ingres_b)
    {
	STprintf(cbuf, 
	    "%s %p: ...   5) using a SPECIAL CDB association\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: ...   5) using a REGULAR LDB association\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    if (i_ldb_p->dd_l6_flags & DD_02FLAG_2PC)
    {
	STprintf(cbuf, 
	    "%s %p: ...   6) a 2PC association to the CDB\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    else if (i_ldb_p->dd_l6_flags & DD_03FLAG_SLAVE2PC)
    {
	STprintf(cbuf, 
	    "%s %p: ...   6) this LDB supports the 2PC protocol\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    return(E_DB_OK);
}


/*{
** Name: qed_w2_prt_rqferr - display an error generated by RQF
**
** Description:
**      This routine displays the information pertaining to an RQF error.
**
** Inputs:
**	i1_call_id			RQF operation code
**	i2_rqr_p->			ptr to RQR_CB
**	    rqr_error
**		.err_code
**
** Outputs:
**	v_qer_p->			ptr to QEF_RCB
**	    error
**		.err_code		mapped error code
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
**      21-jul-90 (carl)
**          written
**      07-oct-90 (carl)    
**	    fixed to suppress displaying interrupt error
*/


VOID
qed_w2_prt_rqferr(
QEF_RCB		*v_qer_p,
i4		i1_call_id,
RQR_CB		*i2_rqr_p )
{
    bool	prt_ldbdesc = TRUE;	    /* assume there is one */
    DD_LDB_DESC	*ldb_p = i2_rqr_p->rqr_1_site;
    char	*cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (i2_rqr_p->rqr_error.err_code != E_RQ0039_INTERRUPTED)
    {
	/* display RQF error */

	qed_w0_prt_gmtnow(v_qer_p);

	STprintf(cbuf, "%s %p: %s ",
	    IIQE_61_qefsess, (PTR) v_qer_p->qef_cb, IIQE_64_callerr);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	if (0 <= i1_call_id && i1_call_id <= RQR_CLUSTER_INFO)
	{
	    STprintf(cbuf, "%s\n", IIQE_51_rqf_tab[i1_call_id]);
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	else
	{
	    STprintf(cbuf, "an UNRECOGNIZED RQF operation!\n");
            qec_tprintf(v_qer_p, cbufsize, cbuf);
        }
    }
    switch (i2_rqr_p->rqr_error.err_code)
    {
    case E_RQ0001_WRONG_COLUMN_COUNT:
	v_qer_p->error.err_code = E_QE0501_WRONG_COLUMN_COUNT;
	break;
    case E_RQ0002_NO_TUPLE_DESCRIPTION:
	v_qer_p->error.err_code = E_QE0502_NO_TUPLE_DESCRIPTION;
	break;
    case E_RQ0003_TOO_MANY_COLUMNS:
	v_qer_p->error.err_code = E_QE0503_TOO_MANY_COLUMNS;
	break;
    case E_RQ0004_BIND_BUFFER_TOO_SMALL:
	v_qer_p->error.err_code = E_QE0504_BIND_BUFFER_TOO_SMALL;
	break;
    case E_RQ0005_CONVERSION_FAILED:
	v_qer_p->error.err_code = E_QE0505_CONVERSION_FAILED;
	break;
    case E_RQ0006_CANNOT_GET_ASSOCIATION:
	v_qer_p->error.err_code = E_QE0506_CANNOT_GET_ASSOCIATION;
	break;
    case E_RQ0007_BAD_REQUEST_CODE:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0507_BAD_REQUEST_CODE;
	break;
    case E_RQ0008_SCU_MALLOC_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0508_SCU_MALLOC_FAILED;
	break;
    case E_RQ0009_ULM_STARTUP_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0509_ULM_STARTUP_FAILED;
	break;
    case E_RQ0010_ULM_OPEN_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0510_ULM_OPEN_FAILED;
	break;
    case E_RQ0011_INVALID_READ:
	v_qer_p->error.err_code = E_QE0511_INVALID_READ;
	break;
    case E_RQ0012_INVALID_WRITE:
	v_qer_p->error.err_code = E_QE0512_INVALID_WRITE;
	break;
    case E_RQ0013_ULM_CLOSE_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0513_ULM_CLOSE_FAILED;
	break;
    case E_RQ0014_QUERY_ERROR:
	v_qer_p->error.err_code = E_QE0514_QUERY_ERROR;
	break;
    case E_RQ0015_UNEXPECTED_MESSAGE:
	v_qer_p->error.err_code = E_QE0515_UNEXPECTED_MESSAGE;
	break;
    case E_RQ0016_CONVERSION_ERROR:
	v_qer_p->error.err_code = E_QE0516_CONVERSION_ERROR;
	break;
    case E_RQ0017_NO_ACK:
	v_qer_p->error.err_code = E_QE0517_NO_ACK;
	break;
    case E_RQ0018_SHUTDOWN_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0518_SHUTDOWN_FAILED;
	break;
    case E_RQ0019_COMMIT_FAILED:
	v_qer_p->error.err_code = E_QE0519_COMMIT_FAILED;
	break;
    case E_RQ0020_ABORT_FAILED:
	v_qer_p->error.err_code = E_QE0520_ABORT_FAILED;
	break;
    case E_RQ0021_BEGIN_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0521_BEGIN_FAILED;
	break;
    case E_RQ0022_END_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0522_END_FAILED;
	break;
    case E_RQ0023_COPY_FROM_EXPECTED:
	v_qer_p->error.err_code = E_QE0523_COPY_FROM_EXPECTED;
	break;
    case E_RQ0024_COPY_DEST_FAILED:
	v_qer_p->error.err_code = E_QE0524_COPY_DEST_FAILED;
	break;
    case E_RQ0025_COPY_SOURCE_FAILED:
	v_qer_p->error.err_code = E_QE0525_COPY_SOURCE_FAILED;
	break;
    case E_RQ0026_QID_EXPECTED:
	v_qer_p->error.err_code = E_QE0526_QID_EXPECTED;
	break;
    case E_RQ0027_CURSOR_CLOSE_FAILED:
	v_qer_p->error.err_code = E_QE0527_CURSOR_CLOSE_FAILED;
	break;
    case E_RQ0028_CURSOR_FETCH_FAILED:
	v_qer_p->error.err_code = E_QE0528_CURSOR_FETCH_FAILED;
	break;
    case E_RQ0029_CURSOR_EXEC_FAILED:
	v_qer_p->error.err_code = E_QE0529_CURSOR_EXEC_FAILED;
	break;
    case E_RQ0030_CURSOR_DELETE_FAILED:
	v_qer_p->error.err_code = E_QE0530_CURSOR_DELETE_FAILED;
	break;
    case E_RQ0031_INVALID_CONTINUE:
	v_qer_p->error.err_code = E_QE0531_INVALID_CONTINUE;
	break;
    case E_RQ0032_DIFFERENT_TUPLE_SIZE:
	v_qer_p->error.err_code = E_QE0532_DIFFERENT_TUPLE_SIZE;
	break;
    case E_RQ0033_FETCH_FAILED:
	v_qer_p->error.err_code = E_QE0533_FETCH_FAILED;
	break;
    case E_RQ0034_COPY_CREATE_FAILED:
	v_qer_p->error.err_code = E_QE0534_COPY_CREATE_FAILED;
	break;
    case E_RQ0035_BAD_COL_DESC_FORMAT:
	v_qer_p->error.err_code = E_QE0535_BAD_COL_DESC_FORMAT;
	break;
    case E_RQ0036_COL_DESC_EXPECTED:
	v_qer_p->error.err_code = E_QE0536_COL_DESC_EXPECTED;
	break;
    case E_RQ0037_II_LDB_NOT_DEFINED:
	v_qer_p->error.err_code = E_QE0537_II_LDB_NOT_DEFINED;
	break;
    case E_RQ0038_ULM_ALLOC_FAILED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0538_ULM_ALLOC_FAILED;
	break;
    case E_RQ0039_INTERRUPTED:
	prt_ldbdesc = FALSE;
	v_qer_p->error.err_code = E_QE0022_QUERY_ABORTED;
	break;
    case E_RQ0040_UNKNOWN_REPEAT_Q:
	v_qer_p->error.err_code = E_QE0540_UNKNOWN_REPEAT_Q;
	break;
    case E_RQ0041_ERROR_MSG_FROM_LDB:
	v_qer_p->error.err_code = E_QE0541_ERROR_MSG_FROM_LDB;
	break;
    case E_RQ0042_LDB_ERROR_MSG:
	v_qer_p->error.err_code = E_QE0542_LDB_ERROR_MSG;
	break;
    case E_RQ0043_CURSOR_UPDATE_FAILED:
	v_qer_p->error.err_code = E_QE0543_CURSOR_UPDATE_FAILED;
	break;
    case E_RQ0044_CURSOR_OPEN_FAILED:
	v_qer_p->error.err_code = E_QE0544_CURSOR_OPEN_FAILED;
	break;
    case E_RQ0045_CONNECTION_LOST:
	v_qer_p->error.err_code = E_QE0545_CONNECTION_LOST;
	break;
    case E_RQ0046_RECV_TIMEOUT:
	v_qer_p->error.err_code = E_QE0546_RECV_TIMEOUT;
	break;
    case E_RQ0047_UNAUTHORIZED_USER:
	v_qer_p->error.err_code = E_QE0547_UNAUTHORIZED_USER;
	break;
    case E_RQ0048_SECURE_FAILED:
	v_qer_p->error.err_code = E_QE0548_SECURE_FAILED;
	break;
    case E_RQ0049_RESTART_FAILED:
	v_qer_p->error.err_code = E_QE0549_RESTART_FAILED;
	break;
    default:
	prt_ldbdesc = FALSE;
	break;
    }
    if (i2_rqr_p->rqr_error.err_code != E_RQ0039_INTERRUPTED)
    {
	/* finish displaying RQF error information */

	if (prt_ldbdesc && ldb_p != (DD_LDB_DESC *) NULL)
	    qed_w1_prt_ldbdesc(v_qer_p, ldb_p);
    }
}


/*{
** Name: qed_w3_prt_tpferr - display an error generated by TPF
**
** Description:
**      This routine displays the information pertaining to an TPF error.
**
** Inputs:
**	i1_call_id			RQF operation code
**	i2_tpr_p->			ptr to TPR_CB
**	    rqr_error
**		.err_code
**
** Outputs:
**	v_qer_p->			ptr to QEF_RCB
**	    error
**		.err_code		mapped error code
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
**      21-jul-90 (carl)
**          written
*/


VOID
qed_w3_prt_tpferr(
QEF_RCB		*v_qer_p,
i4		i1_call_id,
TPR_CB		*i2_tpr_p )
{
    DD_LDB_DESC	*ldb_p = i2_tpr_p->tpr_site;
    char	*cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		cbufsize = v_qer_p->qef_cb->qef_trsize;


    qed_w0_prt_gmtnow(v_qer_p);

    STprintf(cbuf, "%s %p: %s ",
	    IIQE_61_qefsess, (PTR) v_qer_p->qef_cb, IIQE_64_callerr);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    if (0 <= i1_call_id && i1_call_id <= TPF_TRACE)
    {
	STprintf(cbuf, "%s\n", IIQE_52_tpf_tab[i1_call_id]);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, "an UNRECOGNIZED TPF operation!\n");
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    switch (i2_tpr_p->tpr_error.err_code)
    {
    case E_TP0001_INVALID_REQUEST:
	v_qer_p->error.err_code = E_QE0701_INVALID_REQUEST; 
	break;
    case E_TP0002_SCF_MALLOC_FAILED:	
	v_qer_p->error.err_code = E_QE0702_SCF_MALLOC_FAILED;
	break;
    case E_TP0003_SCF_MFREE_FAILED:	
	v_qer_p->error.err_code = E_QE0703_SCF_MFREE_FAILED;
	break;
    case E_TP0004_MULTI_SITE_WRITE:	
	v_qer_p->error.err_code = E_QE0704_MULTI_SITE_WRITE;
	break;
    case E_TP0005_UNKNOWN_STATE:	
	v_qer_p->error.err_code = E_QE0705_UNKNOWN_STATE;
	break;
    case E_TP0006_INVALID_EVENT:	
	v_qer_p->error.err_code = E_QE0706_INVALID_EVENT;
	break;
    case E_TP0007_INVALID_TRANSITION:
	v_qer_p->error.err_code = E_QE0707_INVALID_TRANSITION;
	break;
    case E_TP0008_TXN_BEGIN_FAILED:	
	v_qer_p->error.err_code = E_QE0708_TXN_BEGIN_FAILED;
	break;
    case E_TP0009_TXN_FAILED:	
	v_qer_p->error.err_code = E_QE0709_TXN_FAILED;
	break;
    case E_TP0010_SAVEPOINT_FAILED:
	v_qer_p->error.err_code = E_QE0710_SAVEPOINT_FAILED;
	break;
    case E_TP0011_SP_ABORT_FAILED:	
	v_qer_p->error.err_code = E_QE0711_SP_ABORT_FAILED;
	break;
    case E_TP0012_SP_NOT_EXIST:	
	v_qer_p->error.err_code = E_QE0712_SP_NOT_EXIST;
	break;
    case E_TP0013_AUTO_ON_NO_SP:	
	v_qer_p->error.err_code = E_QE0713_AUTO_ON_NO_SP;
	break;
    case E_TP0014_ULM_STARTUP_FAILED:	
	v_qer_p->error.err_code = E_QE0714_ULM_STARTUP_FAILED;
	break;
    case E_TP0015_ULM_OPEN_FAILED:	
	v_qer_p->error.err_code = E_QE0715_ULM_OPEN_FAILED;
	break;
    case E_TP0016_ULM_ALLOC_FAILED:	
	v_qer_p->error.err_code = E_QE0716_ULM_ALLOC_FAILED;
	break;
    case E_TP0017_ULM_CLOSE_FAILED:	
	v_qer_p->error.err_code = E_QE0717_ULM_CLOSE_FAILED;
	break;
    case E_TP0020_INTERNAL:	
	v_qer_p->error.err_code = E_QE0720_TPF_INTERNAL;
	break;
    case E_TP0021_LOGDX_FAILED:	
	v_qer_p->error.err_code = E_QE0721_LOGDX_FAILED;
	break;
    case E_TP0022_SECURE_FAILED:	
	v_qer_p->error.err_code = E_QE0722_SECURE_FAILED;
	break;
    case E_TP0023_COMMIT_FAILED:	
	v_qer_p->error.err_code = E_QE0723_COMMIT_FAILED;
	break;
    case E_TP0024_DDB_IN_RECOVERY:	
	v_qer_p->error.err_code = E_QE0724_DDB_IN_RECOVERY;
	break;
    default:
	/* TPF's responsibility to map into RQF's error range */
	break;
    }
}


/*{
** Name: qed_w4_prt_qefcall - display a QEF call
**
** Description:
**      This routine displays a given QEF call.
**
** Inputs:
**	i_qefcb_p			QEF_CB ptr
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
qed_w4_prt_qefcall(
QEF_CB	*i_qefcb_p,
i4	i_call_id )
{
    QEF_RCB	qef_rcb;		/* for qec_tprintf */
    char	*cbuf = i_qefcb_p->qef_trfmt;
    i4		cbufsize = i_qefcb_p->qef_trsize;


    MEfill(sizeof(qef_rcb), '\0', (PTR) & qef_rcb);
    qef_rcb.qef_cb = i_qefcb_p;		/* only field needed by qec_tprintf */

    if (0 <= i_call_id && i_call_id <= QEF_LO_MAX_OP)
    {
	STprintf(cbuf, "%s", IIQE_53_lo_qef_tab[i_call_id]);
        qec_tprintf(& qef_rcb, cbufsize, cbuf);
    }
    else if (QEF_HI_MIN_OP <= i_call_id && i_call_id <= QEF_HI_MAX_OP)
    {
	STprintf(cbuf, 
	    "%s", IIQE_54_hi_qef_tab[i_call_id - QEF_HI_MIN_OP]);
        qec_tprintf(& qef_rcb, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, "an UNRECOGNIZED QEF operation");
        qec_tprintf(& qef_rcb, cbufsize, cbuf);
    }
}


/*{
** Name: qed_w5_prt_qeferr - display a failing QEF call
**
** Description:
**      This routine displays a given QEF call.
**
** Inputs:
**	v_qer_p				QEF_RCB
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
qed_w5_prt_qeferr(
QEF_RCB		*v_qer_p,
i4		i_call_id )
{
    char	*cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		cbufsize = v_qer_p->qef_cb->qef_trsize;


    qed_w0_prt_gmtnow(v_qer_p);

    /* display "QEF xxxxxx: ERROR occurred while calling XXX" */

    STprintf(cbuf, "%s %p: %s ",
	IIQE_61_qefsess, 
	(PTR) v_qer_p->qef_cb, 
	IIQE_64_callerr);
    qec_tprintf(v_qer_p, cbufsize, cbuf);
    qed_w4_prt_qefcall(v_qer_p->qef_cb, i_call_id);
    STprintf(cbuf, "\n");
    qec_tprintf(v_qer_p, cbufsize, cbuf);
}


/*{
** Name: qed_w6_prt_qperror - display banner line for a query plan error 
**
** Description:
**      This routine displays a banner line:
**
**	    "QEF session nnn: Query plan error detected!".
**
** Inputs:
**	i_qer_p->			ptr to QEF_RCB
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
**      21-jul-90 (carl)
**          written
*/


VOID
qed_w6_prt_qperror(
QEF_RCB		*i_qer_p )
{
    char	*cbuf = i_qer_p->qef_cb->qef_trfmt;
    i4		cbufsize = i_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, "%s %p: %s\n",
	IIQE_61_qefsess, (PTR) i_qer_p->qef_cb, IIQE_63_qperror);
    qec_tprintf(i_qer_p, cbufsize, cbuf);
}


/*{
** Name: qed_w7_prt_qryinfo - display query information
**
** Description:
**
** Inputs:
**	v_qer_p->			ptr to QEF_RCB
**          qef_r3_ddb_req		QEF_DDB_REQ
**              .qer_d1_ddb_p		NULL
**              .qer_d2_ldb_info_p	DD_1LDB_INFO of LDB
**              .qer_d4_qry_info
**                  .qed_q1_timeout         timeout quantum, 0 if none
**                  .qed_q2_lang            DB_QUEL
**                  .qed_q3_qmode           DD_DD_1_MODE_READ or DD_2MODE_UPDATE
**                  .qed_q4_pkt_p           subquery information
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
**      21-jul-90 (carl)
**          written
*/


DB_STATUS
qed_w7_prt_qryinfo(
QEF_RCB		*v_qer_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ     *ddr_p = & v_qer_p->qef_r3_ddb_req;
    QED_QUERY_INFO  *qry_p = & v_qer_p->qef_r3_ddb_req.qer_d4_qry_info;
    DD_PACKET	    *pkt_p = qry_p->qed_q4_pkt_p;
    DD_LDB_DESC	    *ldb_p = & ddr_p->qer_d2_ldb_info_p->dd_i1_ldb_desc;
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, "\n%s %p: ...the target LDB:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    qed_w1_prt_ldbdesc(v_qer_p, ldb_p);

    /* output query text */

    STprintf(cbuf, "%s %p: ...the query text:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    while (pkt_p != (DD_PACKET *) NULL)
    {
	status = qed_w10_prt_qrytxt(
		    v_qer_p, 
		    pkt_p->dd_p2_pkt_p, 
		    pkt_p->dd_p1_len);
	if (status)
	    return(status);
	pkt_p = pkt_p->dd_p3_nxt_p;	/* advance to next packet */
    }

    STprintf(cbuf, "\n");
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(status);
}


/*{
** Name: qed_w8_prt_qefqry - display internal query information
**
** Description:
**
** Inputs:
**	v_qer_p				QEF_RCB
**	i_txt_p				query text
**	i_ldb_p				LDB descriptor
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
**      18-sep-90 (carl)
**          written
*/


DB_STATUS
qed_w8_prt_qefqry(
QEF_RCB		*v_qer_p,
char		*i_txt_p,
DD_LDB_DESC	*i_ldb_p )
{
    DB_STATUS	    status = E_DB_OK;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QEF_DDB_REQ     *ddr_p = & v_qer_p->qef_r3_ddb_req;
    i4		    txtlen = STlength(i_txt_p);
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    STprintf(cbuf, "%s %p: Internal query information:\n",
        IIQE_61_qefsess,
        (PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    if (i_ldb_p != (DD_LDB_DESC *) NULL)
    {
	STprintf(cbuf, "%s %p: ...the target LDB:\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	qed_w1_prt_ldbdesc(v_qer_p, i_ldb_p);
    }

    /* output query text */

    STprintf(cbuf, "%s %p: ...the query text:\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    status = qed_w10_prt_qrytxt(
		    v_qer_p, 
		    i_txt_p,
		    txtlen);
    STprintf(cbuf, "\n");
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    return(status);
}


/*
** {
** Name: qed_w9_prt_db_val - display the information of a data type
**			  
** Description:
**
** Inputs:
**      v_qer_p			    ptr to QEF_RCB
**      i1_dv_p			    ptr to DB_DATA_VALUE
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
**      18-sep-90 (carl)
**          created
**      27-oct-90 (carl)
**	    improved display contents
**	26-MAR-92 (fpang)
**	    Don't display DB_DTE_TYPE types. It is a binary encoding.
**	    Fixes B42842.
**       6-Oct-1993 (fred)
**          Changed default so that it calls ADF to get the datatype
**          name rather than "No TYPE!".
*/


DB_STATUS
qed_w9_prt_db_val(
QEF_RCB         *v_qer_p,
DB_DATA_VALUE	*i1_dv_p )
{
    DB_STATUS	status = E_DB_OK;
    bool	no_type = FALSE,
		do_val = FALSE,		/* assume */
		*bool_p = (bool *) NULL;
    i4		*nat_p = (i4 *) NULL,
		data_len = 0,
		var_len = 0;
    char	*char_p = (char *) NULL,
		*typename_p = (char *) NULL,
                buff[6000];		/* should be big enough */
    ADI_DT_NAME  tyname;
    char	*cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (i1_dv_p == (DB_DATA_VALUE *) NULL)
	return(E_DB_OK);

    switch (i1_dv_p->db_datatype)
    {
    case DB_INT_TYPE:
	typename_p = IIQE_33_integer;
	do_val = TRUE;
	break;
    case DB_FLT_TYPE:
	typename_p = IIQE_34_float;
	break;
    case DB_CHR_TYPE:
	typename_p = "C";
	do_val = TRUE;
	break;
    case DB_DTE_TYPE:
	typename_p = IIQE_36_date;
	do_val = FALSE;
	break;
    case DB_MNY_TYPE:
	typename_p = IIQE_37_money;
	break;
    case DB_DEC_TYPE:
	typename_p = IIQE_40_decimal;
	break;
    case DB_LOGKEY_TYPE:
	typename_p = "LOGICAL KEY";
	break;
    case DB_TABKEY_TYPE:
	typename_p = "TABLE KEY";
	break;
    case DB_CHA_TYPE:
	typename_p = IIQE_38_char;
	do_val = TRUE;
	break;
    case DB_VCH_TYPE:
	typename_p = IIQE_39_varchar;
	do_val = TRUE;
	break;
    case DB_BOO_TYPE:
	typename_p = "BOOLEAN";
	do_val = TRUE;
	break;
    case DB_DYC_TYPE:
	typename_p = "DYNAMIC CHAR STRING";
	break;
    case DB_LTXT_TYPE:
	typename_p = "LONG TEXT";
	break;
    case DB_QUE_TYPE:
	typename_p = "QUEUE";
	break;
    case DB_DMY_TYPE:
	typename_p = "DUMMY";
	break;
    case DB_DBV_TYPE:
	typename_p = "DATA VALUE";
	break;
    default:
	status = adi_tyname(v_qer_p->qef_cb->qef_adf_cb,
			    i1_dv_p->db_datatype,
			    &tyname);
	if (DB_FAILURE_MACRO(status))
	{
	    no_type = TRUE;
	    typename_p = "NO type!";
	}
	else
	{
	    typename_p = tyname.adi_dtname;
	}
	break;
    }
    if (no_type)
    {
	STprintf(cbuf, "%s %p: ...      db_datatype: %d!\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    i1_dv_p->db_datatype);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, "%s %p: ...      db_datatype: %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    typename_p);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    if (i1_dv_p->db_datatype == DB_VCH_TYPE)
	data_len = i1_dv_p->db_length - 2;  /* adjust for 2-byte length field  
					    */
    else
	data_len = i1_dv_p->db_length;	    /* display full length */

    STprintf(cbuf, "%s %p: ...      db_length: %d\n",
	IIQE_61_qefsess,
	(PTR) v_qer_p->qef_cb,
	data_len);
    qec_tprintf(v_qer_p, cbufsize, cbuf);

    if (do_val)
    {
	STprintf(cbuf, "%s %p: ...      db_data: ",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    /* finish message/line */

    switch (i1_dv_p->db_datatype)
    {
    case DB_INT_TYPE:
	nat_p = (i4 *) i1_dv_p->db_data;
	STprintf(cbuf, "%d\n", nat_p);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
	break;
	break;
    case DB_CHR_TYPE:
    case DB_CHA_TYPE:
	char_p = (char *) i1_dv_p->db_data;
	qed_u0_trimtail(char_p, (u_i4) i1_dv_p->db_length, buff);
	if (var_len < 41)
	{
	    STprintf(cbuf, "%s\n", buff);	/* on same line */
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	else
	{
	    /* on multiple lines */

	    STprintf(cbuf, "\n");
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	    qed_w10_prt_qrytxt(v_qer_p, buff, i1_dv_p->db_length);
	}
	break;
    case DB_BOO_TYPE:
	bool_p = (bool *) i1_dv_p->db_data;
	if (*bool_p)
	{
	    STprintf(cbuf, "%s\n", "TRUE");
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	else
	{
	    STprintf(cbuf, "%s\n", "FALSE");
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	break;
    case DB_VCH_TYPE:
	char_p = (char *) i1_dv_p->db_data;
	char_p += 2;		/* skip 2-byte length field */
	var_len = i1_dv_p->db_length - 2;
				/* adjust for length of data */
	MEcopy(char_p, var_len, buff);
	buff[var_len] = EOS;
	if (var_len < 41)
	{
	    STprintf(cbuf, "%s\n", buff);	/* on same line */
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	}
	else
	{
	    /* on multiple lines */

	    STprintf(cbuf, "\n");
            qec_tprintf(v_qer_p, cbufsize, cbuf);
	    qed_w10_prt_qrytxt(v_qer_p, buff, var_len);	
	}
	break;
    case DB_DEC_TYPE:
    case DB_DBV_TYPE:
    case DB_DYC_TYPE:
    case DB_DMY_TYPE:
    case DB_FLT_TYPE:
    case DB_LTXT_TYPE:
    case DB_MNY_TYPE:
    case DB_QUE_TYPE:
    case DB_LOGKEY_TYPE:
    case DB_TABKEY_TYPE:
    case DB_DTE_TYPE:
    default:
	break;
    }

    return(E_DB_OK);
}


/*{
** Name: qed_w10_prt_qrytxt - display a segment of query text
**			  
** Description:
**
** Inputs:
**      v_qer_p			    ptr to QEF_RCB
**      i1_txt_p		    ptr to beginning of query text
**      i2_txtlen		    length of query text
**
** Outputs:
**      nothing
**        
**	Returns:
**	    E_DB_OK
**		
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      18-sep-90 (carl)
**          created
*/


DB_STATUS
qed_w10_prt_qrytxt(
QEF_RCB         *v_qer_p,
char		*i1_txt_p,
i4		i2_txtlen )
{
    DB_STATUS       status = E_DB_OK;
    QEQ_TXT_SEG	    *seg_p = (QEQ_TXT_SEG *) NULL;
    i4		    lendone = 0,
		    lenleft = i2_txtlen;
    bool	    stop;
    char	    *p1 = i1_txt_p,	/* must initialize */
		    *p2 = i1_txt_p,	/* must initialize as p1 */
		    buff[QEK_200_LEN];	/* text buffer */
    char	    *cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4		    cbufsize = v_qer_p->qef_cb->qef_trsize;


    if (i2_txtlen <= 0)
	return(E_DB_OK);    /* nothing to do */

    /* output a chunk at a time */

    while (lenleft > QEK_055_LEN)
    {
	/* output a 50-character chunk */

	p2 = p1 + QEK_055_LEN;
	stop = FALSE;
	while (p2 > p1 && ! stop)
	{
	    if (*p2 == ' '
		||
		*p2 == ',' 
		||
		*p2 == '='
		|| 
		*p2 == ';')
	    {
		stop = TRUE;	    /* natural break */
		++p2;		    /* include this character */
	    }
	    else if (*p2 == ')')    /* include this character */
	    {	
		stop = TRUE;	    /* natural break */
		++p2;
		if (*p2 == ';')	    /* include this character */
		    ++p2;
	    }
	    else if (*p2 == '(')    /* exclude this character */
	    {
		stop = TRUE;	    /* natural break */
	    }
	    else
		--p2;		    /* backup */
	}

	lendone = (i4) (p2 - p1);
	if (lendone < 1)
	    lendone = QEK_055_LEN;  /* output this entire chunk */

	MEcopy((PTR) p1, lendone, (PTR) & buff[0]);
	buff[lendone] = EOS;	    /* null-terminate */
	STprintf(cbuf, 
	    "%s %p: ...   %s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    & buff[0]);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
	if (lendone == QEK_055_LEN)
	    p1 += QEK_055_LEN;	    /* cannot trust p2 */
	else
	    p1 = p2;		    /* advance to new text */
	lenleft -= lendone;
    }

    if (lenleft > 0)
    {
	MEcopy((PTR) p1, lenleft, (PTR) buff);

	buff[lenleft] = EOS;	/* null-terminate */
	STprintf(cbuf, "%s %p: ...   %s\n",
		IIQE_61_qefsess,
		(PTR) v_qer_p->qef_cb,
		& buff[0]);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    return(E_DB_OK);
}

