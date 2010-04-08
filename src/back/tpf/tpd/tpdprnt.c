/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <cv.h>
#include    <pc.h>
#include    <di.h>
#include    <lo.h>
#include    <tm.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tr.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpfqry.h>
#include    <tpfproto.h>
#include    <tpferr.h>


/**
**
**  Name: TPDPRNT.C - Utility functions for displaying to the FE
**
**  Description:
**	tpd_p1_prt_gmtnow   - display time
**	tpd_p2_prt_rqferr   - display RQF error
**	tpd_p3_prt_dx_state - display the DX state 
**	tpd_p4_prt_tpfcall  - display TPF call
**	tpd_p5_prt_tpfqry   - display TPF query information
**	tpd_p6_prt_qrytxt   - display query text (segment)
**	tpd_p8_prt_ddbdesc  - display DDB descriptor
**	tpd_p9_prt_ldbdesc  - display LDB descriptor
**
**  History:    $Log-for RCS$
**      05-oct-90 (carl)
**          created
**      24-oct-90 (carl)
**	    modified to use string constants
**	29-apr-92 (fpang)
**	    Changed TRdisplay() calls to tp2_u13_log() if in recovery.
**          Fixes B43851 (DX Recovery thread is not logging recovery status).
**	15-oct-1992 (fpang)
**	    Added include files for SYBIL.
**      19-Nov-1992 (terjeb)
**          Included <tpfproto.h> <tpfqry.h> in order to declare
**          tpd_p6_prt_qrytxt and tpd_p9_prt_ldbdesc.
**	14-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA project.
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	12-aug-93 (swm)
**	    Specify %p print option when printing the sscb_p pointer and
**	    elminate truncating cast to nat.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in tpd_p1_prt_gmtnow(),
**	    tpd_p2_prt_rqferr(), tpd_p3_prt_dx_state(), 
**	    tpd_p4_prt_tpfcall(), tpd_p8_prt_ddbdesc(), and
**	    tpd_p9_prt_ldbdesc().
**	10-oct-93 (swm)
**	    Bug #56448
**	    Altered tpf_u0_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to tpf_u0_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    tpf_u0_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	24-feb-94 (swm)
**	    Bug #60636
**	    In assignment to lendone remove truncating cast of each pointer
**	    in the expression but leave (i4) cast of subtraction result as
**	    it is (in tpd_p6_prt_qrytxt() function).
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	24-Sep-1999 (hanch04)
**	    Replace tp2_u13_log with tp2_put.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF   struct _TPD_SV_CB   *Tpf_svcb_p;
					    /* TPF facility control block */
GLOBALREF   char    *IITP_00_tpf_p;	    /* "TPF" */
GLOBALREF   char    *IITP_07_tracing_p;	    /* "Tracing" */
GLOBALREF   char    *IITP_08_erred_calling_p;
					    /* "occurred while calling" */
GLOBALREF   char    *IITP_09_3dots_p;	    /* "..." */

GLOBALREF   char    *IITP_20_2pc_recovery_p;/* "2PC/RECOVERY" */
GLOBALREF   char    *IITP_28_unrecognized_p;/* "UNRECOGNIZED" */
GLOBALREF   char    *IITP_29_state_p;	    /* "state" */
GLOBALREF   char    *IITP_30_error_p;	    /* "ERROR" */
GLOBALREF   char    *IITP_33_unknown_rqfid_p;
					    /* "an UNRECOGNIZED RQF operation"
					    */
GLOBALREF   char    *IITP_34_unknown_tpfid_p;
					    /* "an UNRECOGNIZED TPF operation"
					    */
GLOBALREF   char    *IITP_35_qry_info_p;    /* "query information" */
GLOBALREF   char    *IITP_36_qry_text_p;    /* "query text" */
GLOBALREF   char    *IITP_37_target_ldb_p;  /* "the target LDB" */
GLOBALREF   char    *IITP_38_null_ptr_p;    /* "NULL pointer detected!" */
GLOBALREF   char    *IITP_39_ddb_name_p;    /* "DDB name" */
GLOBALREF   char    *IITP_40_ddb_owner_p;   /* "DDB owner" */
GLOBALREF   char    *IITP_41_cdb_name_p;    /* "CDB name" */
GLOBALREF   char    *IITP_42_cdb_node_p;    /* "CDB node" */
GLOBALREF   char    *IITP_43_cdb_dbms_p;    /* "CDB DBMS" */
GLOBALREF   char    *IITP_44_node_p;	    /* "NODE" */
GLOBALREF   char    *IITP_45_ldb_p;	    /* "LDB" */
GLOBALREF   char    *IITP_46_dbms_p;	    /* "DBMS" */
GLOBALREF   char    *IITP_47_cdb_assoc_p;   
			/* "using a special CDB association" */
GLOBALREF   char    *IITP_48_reg_assoc_p;   
			/* "using a regular association" */

GLOBALREF   char    *IITP_50_rqf_call_tab[];/* table of RQF op names */
GLOBALREF   char    *IITP_51_tpf_call_tab[];/* table of TPF op names */
GLOBALREF   char    *IITP_52_dx_state_tab[];/* table of state names */


/*{
** Name: tpd_p1_prt_gmtnow - display current time
**
** Description:
**      Display the current time.
**  (i_to_log TRUE if to the log, FALSE if to the FE)
**
** Inputs:
**      v_tpr_p             ptr to TPR_CB
**	i_to_log	    TRUE if to the log; FALSE if to the FE
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
**      05-oct-90 (carl)    
**          created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/


DB_STATUS
tpd_p1_prt_gmtnow(
    TPR_CB	    *v_tpr_p,
    bool	    i_to_log)
{
    DB_STATUS	    status = E_DB_OK;
    TPD_SS_CB	    *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	    *dxcb_p = & sscb_p->tss_dx_cb;
    SYSTIME	    now;
    char	    now_str[DD_25_DATE_SIZE + 1];
    char	    *cbuf = v_tpr_p->tpr_trfmt;
    i4		    cbufsize = v_tpr_p->tpr_trsize;


    TMnow(& now);
    TMstr(& now, now_str);


    if (i_to_log)
    {
        if (!(Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY))
	    TRdisplay(
		"%s %p: %s\n", 
		IITP_00_tpf_p,
		sscb_p,
		now_str);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s\n",
	    IITP_00_tpf_p,
	    (PTR) sscb_p,
	    now_str);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    return(E_DB_OK);
}


/*{
** Name: tpd_p2_prt_rqferr - display RQF error
**
** Description:
**      This routine displays an LDB error generated by RQF.
**
** Inputs:
**	i_rqf_id			RQR op code
**	i_rqr_p->			RQR_CB
**	    rqr_error
**		.err_code
**	i_to_log			TRUE if to the log; FALSE if to the FE
**
** Outputs:
**	v_tpr_p->			TPR_CB
**	    tpr_error
**		.err_code		E_QE0981_RQF_REPORTED_ERROR
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
**      13-oct-90 (carl)
**          adapted
**      24-oct-90 (carl)
**	    modified to use string constant "ERROR"
**	23-sep-1993 (fpang)
**	    Fixed erroneous %p.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/


VOID
tpd_p2_prt_rqferr(
	i4		 i_rqf_id,
	RQR_CB		*i_rqr_p,
	TPR_CB		*v_tpr_p,
	bool		 i_to_log)
{
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    i4     i4_1,
                i4_2;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    tpd_p1_prt_gmtnow(v_tpr_p, i_to_log);

    if (i_to_log)
    {
        if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
           tp2_put( E_TP0246_REC_RQF_ERROR, 0, 1,
                sizeof(i_rqr_p->rqr_error.err_code),
                (PTR)&i_rqr_p->rqr_error.err_code,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0 );
	else
            TRdisplay(
		"%s %p: %s %x %s ", 
		IITP_00_tpf_p,
		sscb_p,
		IITP_30_error_p,
		i_rqr_p->rqr_error.err_code,
		IITP_08_erred_calling_p);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s %x %s ", 
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_30_error_p,
	    i_rqr_p->rqr_error.err_code,
	    IITP_08_erred_calling_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    /* finish message line */

    if (0 <= i_rqf_id && i_rqf_id <= RQR_CLUSTER_INFO)
    {
	if (i_to_log)
	{
	    TRdisplay(
		"%s!\n", 
		IITP_50_rqf_call_tab[i_rqf_id]);
	}
	else
	{
	    STprintf(cbuf, 
		"%s!\n", 
		IITP_50_rqf_call_tab[i_rqf_id]);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    else
    {
	if (i_to_log)
	{
	    TRdisplay(
		"%s!\n",
		IITP_33_unknown_rqfid_p);
	}
	else
	{
	    STprintf(cbuf, 
		"%s!\n",
		IITP_33_unknown_rqfid_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
}


/*{ 
** Name: tpf_p3_prt_dx_state - display TPF state 
** 
** Description: 
**	Display TPF state.
**
** Inputs:
**	v_tpr_p			    - TPF session 
**
** Outputs:
**	None
**
**	Returns:
**		none
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      05-oct-90 (carl)    
**          created
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/


VOID
tpd_p3_prt_dx_state(
	TPR_CB	*v_tpr_p)
{
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = (TPD_DX_CB *) & sscb_p->tss_dx_cb;
    i4		state = sscb_p->tss_dx_cb.tdx_state;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    STprintf(cbuf, 
	    "%s %p: DX in ",
	    IITP_00_tpf_p,
	    sscb_p);
    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

    if (DX_0STATE_INIT <= state && state <= DX_5STATE_CLOSED)
    {
	STprintf(cbuf, 
	    "%s", IITP_52_dx_state_tab[state]);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s",
	    IITP_28_unrecognized_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    STprintf(cbuf, 
	" %s\n",
	IITP_29_state_p);
    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
}


/*{
** Name: tpd_p4_prt_tpfcall - display a TPF call.
**
** Description:
**      This routine displays a given TPF call.
**
** Inputs:
**	v_tpr_p			    TPR_CB ptr
**	i_call_id		    TPF operation code
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
**      05-oct-90 (carl)    
**          written
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/


VOID
tpd_p4_prt_tpfcall(
	TPR_CB	*v_tpr_p,
	i4	i_call_id)
{
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB   *dxcb_p = & sscb_p->tss_dx_cb;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (0 <= i_call_id && i_call_id <= TPF_MAX_OP)
    {
	STprintf(cbuf, 
		"%s %p: %s %s%s\n", 
		IITP_00_tpf_p,
		sscb_p,
		IITP_07_tracing_p,
		IITP_51_tpf_call_tab[i_call_id],
		IITP_09_3dots_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }
    else
    {
	STprintf(cbuf, 
		"%s %p: %s\n", 
		IITP_00_tpf_p,
		sscb_p,
		IITP_34_unknown_tpfid_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }
}


/*{
** Name: tpd_p5_prt_tpfqry - display internal query information
**
** Description:
**
** Inputs:
**	v_tpr_p				TPR_CB
**	i_txt_p				query text
**	i_ldb_p				LDB descriptor
**	i_to_log			TPD_0TO_FE to the FE,
**					TPD_1TO_TR to the log
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
**      05-oct-90 (carl)    
**          written
**      24-oct-90 (carl)
**	    modified to use string constants "query text", "the target LDB"
*/


DB_STATUS
tpd_p5_prt_tpfqry(
	TPR_CB		*v_tpr_p,
	char		*i_txt_p,
	DD_LDB_DESC	*i_ldb_p,
	bool		 i_to_log)
{
    DB_STATUS	    status = E_DB_OK;
    TPD_SS_CB	    *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	    *dxcb_p = & sscb_p->tss_dx_cb;
    i4		    txtlen = STlength(i_txt_p);
    char	    *cbuf = v_tpr_p->tpr_trfmt;
    i4		    cbufsize = v_tpr_p->tpr_trsize;


    if (i_to_log)
    {
        if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
           tp2_put( I_TP0247_REC_QUERY_INFO, 0, 0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0 );
    	else
	    TRdisplay(
		"%s %p: %s:\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_35_qry_info_p);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s:\n",
	    IITP_00_tpf_p,
	    (PTR) sscb_p,
	    IITP_35_qry_info_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    if (i_ldb_p != (DD_LDB_DESC *) NULL)
    {
	if (i_to_log)
	{
	    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
           tp2_put( I_TP0248_REC_TARGET_LDB, 0, 0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0 );
	    else
		TRdisplay(
		    "%s %p: %s%s:\n",
		    IITP_00_tpf_p,
		    (PTR) sscb_p,
		    IITP_09_3dots_p,
		    IITP_37_target_ldb_p);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: %s%s:\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_09_3dots_p,
		IITP_37_target_ldb_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	tpd_p9_prt_ldbdesc(v_tpr_p, i_ldb_p, i_to_log);
    }

    /* display query text */

    if (i_to_log)
    {
        if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
           tp2_put( I_TP0249_REC_QUERY_TEXT, 0, 0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0,
                0, (PTR)0 );
    	else
	    TRdisplay(
		"%s %p: %s%s:\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_09_3dots_p,
		IITP_36_qry_text_p);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s%s:\n",
	    IITP_00_tpf_p,
	    (PTR) sscb_p,
	    IITP_09_3dots_p,
	    IITP_36_qry_text_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
   }

    status = tpd_p6_prt_qrytxt(
		    v_tpr_p, 
		    i_txt_p,
		    txtlen, 
		    i_to_log);
    if (i_to_log)
    {
        if (!(Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY))
	    TRdisplay(
		"%s %p: \n",
		IITP_00_tpf_p,
		(PTR) sscb_p);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: \n",
	    IITP_00_tpf_p,
	    (PTR) sscb_p);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    return(status);
}


/*{
** Name: tpd_p6_prt_qrytxt - display a segment of query text
**			  
** Description:
**
** Inputs:
**      v_tpr_p			    ptr to TPR_CB
**      i1_txt_p		    ptr to beginning of query text
**      i2_txtlen		    length of query text
**	i3_to_log		    TPD_0TO_FE to the FE,
**				    TPD_1TO_TR to the log
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
**      05-oct-90 (carl)    
**          created
**	08-sep-93 (swm)
**	    Added comment about truncating pointer cast (that it is ASSUMED
**	    to be OK).
**	24-feb-94 (swm)
**	    Bug #60636
**	    In assignment to lendone remove truncating cast of each pointer
**	    in the expression but leave (i4) cast of subtraction result as
**	    it is (in tpd_p6_prt_qrytxt() function).
*/


DB_STATUS
tpd_p6_prt_qrytxt(
	TPR_CB	    *v_tpr_p,
	char	    *i1_txt_p,
	i4	    i2_txtlen,
	bool	    i3_to_log)
{
    DB_STATUS       status = E_DB_OK;
    TPD_SS_CB	    *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	    *dxcb_p = & sscb_p->tss_dx_cb;
    QEQ_TXT_SEG	    *seg_p = (QEQ_TXT_SEG *) NULL;
    i4		    lendone = 0,
		    lenleft = i2_txtlen;
    bool	    stop;
    char	    *p1 = i1_txt_p,	/* must initialize */
		    *p2 = i1_txt_p,	/* must initialize as p1 */

#define TPD_055_LEN	55
#define TPD_200_LEN	200

		    buff[TPD_200_LEN];	/* text buffer */
    char	    *cbuf = v_tpr_p->tpr_trfmt;
    i4		    cbufsize = v_tpr_p->tpr_trsize;


    if (i2_txtlen <= 0)
	return(E_DB_OK);    /* nothing to do */

    /* output a chunk at a time */

    while (lenleft > TPD_055_LEN)
    {
	/* output a 50-character chunk */

	p2 = p1 + TPD_055_LEN;
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
	/*
	** lint truncation warning if size of ptr > int, but code ASSUMED
	** to be valid since result is subsequently cast to a u_i2
	*/
	lendone = (i4) (p2 - p1);
	if (lendone < 1)
	    lendone = TPD_055_LEN;  /* output this entire chunk */

	MEcopy((PTR) p1, lendone, (PTR) & buff[0]);
	buff[lendone] = EOS;	    /* null-terminate */

	if (i3_to_log)
	    TRdisplay(
		"%s %p: %s   %s\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_09_3dots_p,
		& buff[0]);
	else
	{
	    STprintf(cbuf, 
		"%s %p: %s   %s\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_09_3dots_p,
		& buff[0]);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
	if (lendone == TPD_055_LEN)
	    p1 += TPD_055_LEN;	    /* cannot trust p2 */
	else
	    p1 = p2;		    /* advance to new text */
	lenleft -= lendone;
    }

    if (lenleft > 0)
    {
	MEcopy((PTR) p1, lenleft, (PTR) buff);

	buff[lenleft] = EOS;	/* null-terminate */
	
	if (i3_to_log)
	    TRdisplay(
		"%s %p: %s   %s\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_09_3dots_p,
		& buff[0]);
	else
	{
	    STprintf(cbuf, 
		"%s %p: %s   %s\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_09_3dots_p,
		& buff[0]);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }

    return(E_DB_OK);
}


/*{
** Name: tpd_p8_prt_ddbdesc - display DDB descriptor
** 
**  Description:    
**	
**	(See above.)
**
** Inputs:
**	v_tpr_p				ptr to TPR_CB
**	i_starcdb_p			ptr to TPC_I1_STAR_CDBS
**	i_to_log			TRUE if to the log, FALSE if to FE
**	
** Outputs:
**	none
**
**	Returns:
**	    nothing
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**	    
**
** History:
**      26-aug-90 (carl)
**          created
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/


VOID
tpd_p8_prt_ddbdesc(
	TPR_CB		    *v_tpr_p,
	TPC_I1_STARCDBS	    *i_starcdb_p,
	bool		    i_to_log)
{
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    char	namebuf[DB_MAXNAME + 1];
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;

 
    if (i_starcdb_p == (TPC_I1_STARCDBS *) NULL)
    {
	if (i_to_log)
	{
	    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
		tp2_put( E_TP024A_REC_NULL, 0, 0,
		    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0 );
	    else
		TRdisplay(
		    "%s %p: %s\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_38_null_ptr_p);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_38_null_ptr_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
        return;
    }
    tpd_u0_trimtail(
	i_starcdb_p->i1_1_ddb_name,
        (i4) DB_MAXNAME,
        namebuf);
    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP024B_DDB_NAME, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay( 
		"%s %p: %s   1) %s %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_39_ddb_name_p,
		namebuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   1) %s %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_39_ddb_name_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    tpd_u0_trimtail(
	i_starcdb_p->i1_2_ddb_owner,
        (i4) DB_MAXNAME,
        namebuf);
    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP024C_DDB_OWNER, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay( 
		"%s %p: %s   2) %s %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_40_ddb_owner_p,
		namebuf);

    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   2) %s %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_40_ddb_owner_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    tpd_u0_trimtail(
	i_starcdb_p->i1_3_cdb_name,
        (i4) DB_MAXNAME,
        namebuf);
    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP024D_CDB_NAME, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay( 
		"%s %p: %s   3) %s %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_41_cdb_name_p,
		namebuf);

    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   3) %s %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_41_cdb_name_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    tpd_u0_trimtail(
	i_starcdb_p->i1_5_cdb_node,
        (i4) DB_MAXNAME,
        namebuf);
    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP024E_CDB_NODE, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay( 
		"%s %p: %s   4) %s %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_42_cdb_node_p,
		namebuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   4) %s %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_42_cdb_node_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    tpd_u0_trimtail(
	i_starcdb_p->i1_6_cdb_dbms,
        (i4) DB_MAXNAME,
        namebuf);
    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP024F_CDB_DBMS, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay( 
		"%s %p: %s   5) %s %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_43_cdb_dbms_p,
		namebuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   5) %s %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_43_cdb_dbms_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }
}


/*{
** Name: tpd_p9_prt_ldbdesc - display LDB descriptor
** 
**  Description:    
**	
**	(See above.)
**
** Inputs:
**	v_tpr_p			ptr to TPR_CB
**	i_ldb_p			ptr to LDB descriptor
**	i_to_log		TRUE if to the log, FALSE if to FE
**	
** Outputs:
**	none
**
**	Returns:
**	    nothing
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      25-aug-90 (carl)
**          created
*/


DB_STATUS
tpd_p9_prt_ldbdesc(
	TPR_CB		*v_tpr_p,
	DD_LDB_DESC     *i_ldb_p,
	bool		 i_to_log)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    char	namebuf[DB_MAXNAME + 1];
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (i_ldb_p == (DD_LDB_DESC *) NULL)
    {
	if (i_to_log)
	{
	    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
		tp2_put( E_TP024A_REC_NULL, 0, 0,
		    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0,
                    0, (PTR)0 );
	    else
		TRdisplay(
		    "%s %p: %s\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_38_null_ptr_p);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_38_null_ptr_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}

	return(tpd_u2_err_internal(v_tpr_p));
    }
    tpd_u0_trimtail(
	i_ldb_p->dd_l2_node_name,
        (i4) DB_MAXNAME,
        namebuf);

    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP0250_NODE, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay(
		"%s %p: %s   1) %s %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_44_node_p,
		IITP_09_3dots_p,
		namebuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   1) %s %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_44_node_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    tpd_u0_trimtail(
	i_ldb_p->dd_l3_ldb_name,
        (i4) DB_MAXNAME,
        namebuf);
    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP0251_LDB, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay(
		"%s %p: %s   2) %s  %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_45_ldb_p,
		namebuf);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   2) %s  %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_45_ldb_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    tpd_u0_trimtail(
	i_ldb_p->dd_l4_dbms_name,
        (i4) DB_MAXNAME,
        namebuf);
    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP0252_DBMS, 0, 1,
		sizeof(namebuf), namebuf,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay(
		"%s %p: %s   3) %s %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_46_dbms_p,
		namebuf);
    }
    else
    {
	STprintf(cbuf,	
	    "%s %p: %s   3) %s %s\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_46_dbms_p,
	    namebuf);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    if (i_to_log)
    {
	if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
	    tp2_put( I_TP0253_LDB_ID, 0, 1,
		sizeof(i_ldb_p->dd_l5_ldb_id), (PTR)&i_ldb_p->dd_l5_ldb_id,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	else
	    TRdisplay(
		"%s %p: %s   4) %s id %d\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_45_ldb_p,
		i_ldb_p->dd_l5_ldb_id);
    }
    else
    {
	STprintf(cbuf, 
	    "%s %p: %s   4) %s id %d\n",
	    IITP_00_tpf_p,
	    sscb_p,
	    IITP_09_3dots_p,
	    IITP_45_ldb_p,
	    i_ldb_p->dd_l5_ldb_id);
	tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
    }

    if (i_ldb_p->dd_l1_ingres_b)
    {
	if (i_to_log)
	{
	    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
		tp2_put( I_TP0254_CDB_ASSOC, 0, 0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    else
		TRdisplay(
		    "%s %p: %s   5) %s\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_09_3dots_p,
		    IITP_47_cdb_assoc_p);
	}
    	else
	{
	    STprintf(cbuf, 
		"%s %p: %s   5) %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_47_cdb_assoc_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
/*
    else
    {
	if (i_to_log)
	{
	    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
		tp2_u13_log(
		    "%s: %s   5) %s\n",
		    IITP_20_2pc_recovery_p,
		    IITP_09_3dots_p,
		    IITP_48_reg_assoc_p);
	    else
		TRdisplay(
		    "%s %p: %s   5) %s\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_09_3dots_p,
		    IITP_48_reg_assoc_p);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: %s   5) %s\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p,
		IITP_48_reg_assoc_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    if (i_ldb_p->dd_l6_flags & DD_02FLAG_2PC)
    {
	if (i_to_log)
	{
	    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
		tp2_u13_log(
		    "%s: %s   6) a 2PC association to the CDB\n",
		    IITP_20_2pc_recovery_p,
		    IITP_09_3dots_p);
	    else
		TRdisplay(
		    "%s %p: %s   6) a 2PC association to the CDB\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_09_3dots_p);
	}
	else
	{
	    STprintf(cbuf, 
		"%s %p: %s   6) a 2PC association to the CDB\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
    else if (i_ldb_p->dd_l6_flags & DD_03FLAG_SLAVE2PC)
    {
	if (i_to_log)
	{
	    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
		tp2_u13_log(
		    "%s: %s   6) this LDB supports the 2PC protocol\n",
		    IITP_20_2pc_recovery_p,
		    IITP_09_3dots_p);
	    else
		TRdisplay(
		    "%s %p: %s   6) this LDB supports the 2PC protocol\n",
		    IITP_00_tpf_p,
		    sscb_p,
		    IITP_09_3dots_p);
	}
	else
	{
    	    STprintf(cbuf, 
		"%s %p: %s   6) this LDB supports the 2PC protocol\n",
		IITP_00_tpf_p,
		sscb_p,
		IITP_09_3dots_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);
	}
    }
*/
}

