/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <usererror.h>
#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <adf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <tm.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tr.h>
#include    <scf.h>
#include    <tpf.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpferr.h>
#include    <tpfqry.h>
#include    <tpfproto.h>

/**
**
**  Name: TPDUTIL.C - Utility functions for TPF
**
**  Description:
**      The routines defined in this file provide auxiliary functions.
**
**	tpd_u0_trimtail	    - trim trailing blanks off a string
**	tpd_u1_error	    - error rotuine, a debugging aid
**	tpd_u2_err_internal - generate internal error E_TP0020_INTERNAL
**	tpd_u3_trap	    - trap special error condition, a debugging aid
**	tpd_u4_rqf_call	    - interface to rqf_call
**	tpd_u5_gmt_now	    - get current time in GMT format
**	tpd_u6_new_cb	    - allocate a new CB from one of the ULM streams
**	tpd_u7_term_any_cb  - deallocate a CB from one of the ULM streams
**	tpd_u8_close_streams- close all substreams for session
**	tpd_u9_2pc_ldb_status   
**			    - extract the LX's LDB status after an RQF failure
**	tpd_u10_fatal	    - fatal-error rotuine, a debugging aid
**	tpd_u11_term_assoc  - terminate an association to an LDB
**      tpd_u12_trim_cdbstrs- trim trailing whites off CDB-related strings
**      tpd_u13_trim_ddbstrs- trim trailing whites off DDB-related strings
**	tpd_u14_log_rqferr  - log an RQF error
**
**  History:    $Log-for RCS$
**      26-oct-89 (carl)
**          adapted
**      07-jul-90 (carl)
**	    changed tpd_u6_new_cb to initialize LX id for a new LX CB
**      06-oct-90 (carl)
**	    process trace points
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values in tpd_u1_error(),
**	    tpd_u2_err_internal(), tpd_u6_new_cb(), tpd_u10_fatal().
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
**/

GLOBALREF   char	*IITP_00_tpf_p;

GLOBALREF   char	*IITP_07_tracing_p;

GLOBALREF   char	*IITP_09_3dots_p;

GLOBALREF   char        *IITP_50_rqf_call_tab[];
 
GLOBALREF   char        *IITP_51_tpf_call_tab[];
 
/* forward references */


/*{
** Name: tpd_u0_trimtail - Trim trailing spaces from string 1, place in
**			   string 2 and return new length as a u_i4
**
** Description:
**      This routine copies string 1 (not null-terminated) to string 2, 
**	trims the last and returns the new length.
**  
** Inputs:
**	    i_str_p			ptr to string to trim
**	    i_maxlen			maximum length of string to trim
** Outputs:
**	    o_str2_p			returned string
**	Returns:
**	    new length 
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      06-oct-90 (carl)
**          adapted
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/

i4
tpd_u0_trimtail(
	char		*i_str_p,
	i4		i_maxlen,
	char		*o_str_p)
{
    u_i4	len2;


    MEcopy((PTR) i_str_p, i_maxlen, (PTR) o_str_p);
    *(o_str_p + i_maxlen) = '\0';	/* null-terminate before calling
				** STtrmwhite */
    len2 = STtrmwhite(o_str_p);

    return(len2);
}


/*{
** Name: tpd_u1_error - an error condition routine
**
** Description:
**      This routine serves as a trap point for catching errors.
**
** Inputs:
**	i1_errcode		the error
**	v1_rcb_p		ptr to TPR_CB
**  
** Outputs:
**	v1_rcb_p->		ptr to TPR_CB
**	    tpr_error
**		.err_code	set to i1_errcode
**
**	Returns:
**	    E_DB_ERROR
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-oct-89 (carl)
**          adapted
**      06-oct-90 (carl)
**	    process error trace point
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	22-jul-1997 (nanpr01)
**	    If we are out of memory at the time of session initiate,
**	    we will get a segv in this routine trying to reference
**	    scb_p->tss_3_trace_vector.
*/

DB_STATUS
tpd_u1_error(
	DB_ERRTYPE  i1_errcode,
	TPR_CB	    *v_tpr_p)
{
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    i4     i4_1,
                i4_2;

    if (sscb_p == NULL)
    {
	v_tpr_p->tpr_error.err_code = i1_errcode;
	return(E_DB_ERROR);
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
	tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	TRdisplay("%s %p: ERROR %x detected!\n", 
	    IITP_00_tpf_p,
	    sscb_p,
	    i1_errcode - E_TP0000_OK);
    }

    v_tpr_p->tpr_error.err_code = i1_errcode;

    return(E_DB_ERROR);
}


/*{
** Name: tpd_u2_err_internal_fcn - Generate internal error E_QE0002_INTERNAL_ERROR.
**
** Description:
**      This routine generates internal error E_TP0020_INTERNAL.
**  
**
** Inputs:
**	
** Outputs:
**	o_rcb_p				    TPR_CB
**	    tpr_error
**		.err_code		    E_QE0002_INTERNAL_ERROR
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      26-oct-89 (carl)
**          adapted
**      06-oct-90 (carl)
**	    process error trace point
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	22-jul-1997 (nanpr01)
**	    If we are out of memory at the time of session initiate,
**	    we will get a segv in this routine trying to reference
**	    scb_p->tss_3_trace_vector.
**	04-Oct-2001 (jenjo02)
**	    Function now called via a tpd_u2_err_internal() macro
**	    passing __FILE__ and __LINE__ to identify the source
**	    of the error.
*/


DB_STATUS
tpd_u2_err_internal_fcn(
	TPR_CB    *v_tpr_p,
	char	  *file,
	i4	  line)
{
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    i4     i4_1,
                i4_2;


    if ( sscb_p &&
	   (ult_check_macro(&sscb_p->tss_3_trace_vector, 
			   TSS_TRACE_ERROR_104,
			    &i4_1, &i4_2)) )
    {
	tpd_p1_prt_gmtnow(v_tpr_p, TPD_1TO_TR);
	TRdisplay("%s %p: Internal error detected! FILE %s LINE %d\n", 
	    IITP_00_tpf_p,
	    sscb_p, file, line);
    }
    v_tpr_p->tpr_error.err_code = E_TP0020_INTERNAL;

    tp2_put(E_TP0020_INTERNAL, 0, 2, 
		0, file,
		0, (PTR)(SCALARP)line,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0);

    return(E_DB_ERROR);
}


/*{
** Name: tpd_u3_trap - Trap an error
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
**      26-oct-89 (carl)
**          adapted
*/

VOID
tpd_u3_trap(
	VOID)
{
    /* do nothing */
}


/*{
** Name: tpd_u4_rqf_call - interface to RQF_CALL
**
** Description:
**      This routine maps a call to RQF_CALL and extracts the returned LDB
**	status if an error is encountered.
**  
** Inputs:
**	    i_call_id			RQF call id
**	    v_rqr_p			RQF_CB
**	    v_tpr_p			TPR_CB
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
**      26-oct-89 (carl)
**          adapted
*/


DB_STATUS
tpd_u4_rqf_call(
	i4	i_call_id,
	RQR_CB	*v_rqr_p,
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	    status;
    TPD_SS_CB       *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    DD_LDB_DESC	    *ldb_p = v_rqr_p->rqr_1_site;


    v_rqr_p->rqr_session = sscb_p->tss_rqf; /* must pass RQF's session */
    status = rqf_call(i_call_id, v_rqr_p);
    if (status)
    {
	if (i_call_id == RQR_TERM_ASSOC)
	    return(E_DB_OK);	/* forgive error */

	tpd_u14_log_rqferr(i_call_id, v_rqr_p, v_tpr_p);

	/* check if LDB aborted transaction or if the association is lost */

	if (v_rqr_p->rqr_ldb_status & RQF_03_TERM_MASK
	    ||
	    v_rqr_p->rqr_error.err_code == E_RQ0045_CONNECTION_LOST
	    ||
	    v_rqr_p->rqr_error.err_code == E_RQ0046_RECV_TIMEOUT)
	{
	    tpd_u3_trap();
	}

	if (v_rqr_p->rqr_ldb_status & RQF_01_FAIL_MASK)
	{
	    tpd_u3_trap();
	}

	if (v_rqr_p->rqr_ldb_status & RQF_02_STMT_MASK)
	{
	    tpd_u3_trap();
	}
	if (v_rqr_p->rqr_error.err_code == E_RQ0039_INTERRUPTED)
	{
	    tpd_u3_trap();
	}
    }

    return(status);
}


/*{
** Name: tpd_u5_gmt_now - Get current time in GMT format.
**
** Description:
**	Call ADF to get the current time in GMT format.
**
** Inputs:
**	None
**
** Outputs:
**      o_gmt_now			    ptr to DD_DATE
**	v_tpr_p				    TPR_CB
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
**      26-oct-89 (carl)
**          adapted
**	17-Dec-2008 (kiria01) SIR120473
**	    Initialise the ADF_FN_BLK.adf_pat_flags.
**	23-Sep-2009 (kiria01) b122578
**	    Initialise the ADF_FN_BLK .adf_fi_desc and adf_dv_n members.
*/


DB_STATUS
tpd_u5_gmt_now(
	TPR_CB	    *v_tpr_p,
	DD_DATE	    o_gmt_now)
{
    DB_STATUS	    status;
    i4		    nfi;
    ADI_OP_ID	    op_date;
    ADI_FI_TAB	    fitab;
    SCF_CB	    scf_cb;
    SCF_SCI	    sci_list[1];
    DB_DATA_VALUE   dv_now;
    DB_DATA_VALUE   dv_date;
    DB_DATA_VALUE   dv_gmt;
    ADF_CB	    adfcb,
		    *adf_scb = & adfcb;
    ADI_FI_DESC	    *fi1 = NULL;
    ADI_FI_DESC	    *fi2 = NULL;
    char	    gmt_buffer[100];	/* Avoid dynamic allocation */


    /* set up adf_scb */

    scf_cb.scf_length = sizeof (SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_ADF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST*) sci_list;
    scf_cb.scf_len_union.scf_ilength = 1;
    sci_list[0].sci_length  = sizeof (adf_scb);
    sci_list[0].sci_code    = SCI_SCB;
    sci_list[0].sci_aresult = (PTR) &adf_scb;
    sci_list[0].sci_rlength = 0;
    sci_list[0].sci_rlength = 0;
    
    /* get the ADF session control block (ADF_CB) from SCF */

    status = scf_call(SCU_INFORMATION, &scf_cb);
    if (status)
    {
	status = tpd_u2_err_internal(v_tpr_p);
	return(status);
    }
    MEfill((u_i4) sizeof(DD_DATE), ' ', o_gmt_now);

    /* set up DB_DATA_VALUES */

    /* get op id, and set up the fi table */

    adi_opid(adf_scb, "date", &op_date);
    adi_fitab(adf_scb, op_date, &fitab);

    /* search the function instance in the list of function 
    ** instances pointed to by fitab.adi_tab_fi.
    */

    fi1 = fitab.adi_tab_fi;
    nfi = fitab.adi_lntab;

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


    adi_opid(adf_scb, "date_gmt", &op_date);
    adi_fitab(adf_scb, op_date, &fitab);

    fi2 = fitab.adi_tab_fi;
    nfi = fitab.adi_lntab;

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
    dv_now.db_datatype  = DB_CHA_TYPE;
    dv_now.db_length    = 3;
    dv_now.db_data	= (PTR)"now";

    adi_calclen(adf_scb, &fi1->adi_lenspec, &dv_now, NULL, &dv_date.db_length);

    if (dv_date.db_length > sizeof(gmt_buffer))
    {
        /* Error on gmt_buffer length */
	status = tpd_u2_err_internal(v_tpr_p);
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
	if (status = adf_func(adf_scb, &now2date))
	    return status;
    }
    if (status = adi_calclen(adf_scb, &fi2->adi_lenspec, &dv_date, NULL, &dv_gmt.db_length))
	return status;
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
** Name: tpd_u6_new_cb - allocate a new CB
** 
** Description: 
**	Allocate a new CB from one of the ULM streams.  Open the stream if
**	necessary
**
** Inputs:
**	i1_stream		- stream identifier
**	v_tpr_p			- TPR_CB
**  
** Outputs:
**	o1_cbptr_p		- ptr to ptr to new CB
**
**	Returns:
**		E_TP0000_OK
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None.
**
** History:
**      oct-31-89 (carl)
**          created
**      jul-07-90 (carl)
**	    initialize LX id for a  new LX CB
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
**	23-sep-97 (nanpr01)
**	    Fixed segv in ulm.
*/


DB_STATUS
tpd_u6_new_cb(
	i4		i1_stream,
	TPR_CB		*v_tpr_p,
	PTR		*o1_cbptr_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = NULL;
    TPD_SP_CB	*spcb_p = NULL,
		*last_spcb_p = NULL;
    TPD_LX_CB	*lxcb_p = NULL,
		*last_lxcb_p = NULL;
    TPD_CD_CB	*cdcb_p = NULL,
		*last_cdcb_p = NULL;
    TPD_OX_CB	*oxcb_p = NULL,
		*last_oxcb_p = NULL;
    ULM_RCB	ulm;
    SYSTIME	tm_now1,
		tm_now2;
    char	hi_ascii[DB_MAXNAME],
		lo_ascii[DB_MAXNAME],
		buff[DB_MAXNAME + DB_MAXNAME];
    i4		psize;


    *o1_cbptr_p = (PTR) NULL;

    /* set up for opening stream */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;

    /* Allocate a shared stream */
    ulm.ulm_flags = ULM_SHARED_STREAM;

    switch(i1_stream)
    {
    case TPD_0LM_LX_CB:
	lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
	psize = ulm.ulm_blocksize = sizeof(TPD_LX_CB);  
	break;
    case TPD_1LM_SP_CB:
	lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
        psize = ulm.ulm_blocksize = sizeof(TPD_SP_CB);  
	break;
    case TPD_2LM_CD_CB:
	lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
        psize = ulm.ulm_blocksize = sizeof(TPD_CD_CB);  
	break;
    case TPD_3LM_OX_CB:
	lmcb_p = & Tpf_svcb_p->tsv_9_odx_ulmcb;
        psize = ulm.ulm_blocksize = sizeof(TPD_OX_CB);  
	break;
    case TPD_4LM_SX_CB:
	lmcb_p = Tpf_svcb_p->tsv_10_slx_ulmcb_p;
        psize = ulm.ulm_blocksize = sizeof(TPD_LX_CB);  
	break;
    case TPD_5LM_SR_CB:
	lmcb_p = & Tpf_svcb_p->tsv_12_srch_ulmcb;
        psize = ulm.ulm_blocksize = sizeof(TPD_CD_CB);  
	break;
    default:
	return( tpd_u2_err_internal(v_tpr_p) );	
	break;
    }

    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
    /* see if CB stream has been allocated */

    if (lmcb_p->tlm_1_streamid == (PTR) NULL)
    {
	if (ulm_openstream(& ulm) != E_DB_OK)
	{
	    TRdisplay("%s %p: stream allocation failed\n",
		IITP_00_tpf_p,
		sscb_p);
	    v_tpr_p->tpr_error.err_code = E_TP0015_ULM_OPEN_FAILED;
	    return(E_DB_ERROR);
	}

	/* initialize this TPD_LM_CB for the new stream */

	lmcb_p->tlm_2_cnt = 0;
	lmcb_p->tlm_3_frst_ptr = 
	    lmcb_p->tlm_4_last_ptr = (PTR) NULL;
    }
	
    /* allocate a new CB from stream for the caller */
    ulm.ulm_psize = psize;

    if (ulm_palloc(& ulm) != E_DB_OK)
    {
	TRdisplay("%s %p: LX CB allocation failed\n",
	    IITP_00_tpf_p,
	    sscb_p);

	v_tpr_p->tpr_error.err_code = E_TP0016_ULM_ALLOC_FAILED;
	return(E_DB_ERROR);
    }

    *o1_cbptr_p = ulm.ulm_pptr;

    /* initialize ptrs of this new stream CB */

    switch (i1_stream)
    {
    case TPD_0LM_LX_CB:
    case TPD_4LM_SX_CB:
	/* initialize and set up for this LX CB */

	lxcb_p = (TPD_LX_CB *) ulm.ulm_pptr;

	MEfill(sizeof(*lxcb_p), '\0', (PTR) lxcb_p);

	lxcb_p->tlx_txn_type = DB_SQL;	/* assume */
	lxcb_p->tlx_start_time = 0;
	lxcb_p->tlx_state = 0;		/* none */
	lxcb_p->tlx_write = FALSE;	/* assume */
	lxcb_p->tlx_sp_cnt = 0;
	lxcb_p->tlx_20_flags = LX_00FLAG_NIL;
	lxcb_p->tlx_21_prev_p = NULL;
	lxcb_p->tlx_22_next_p = NULL;

	/* must set up LX id for this CB */

	TMnow(& tm_now1);
	STRUCT_ASSIGN_MACRO(tm_now1, tm_now2);
    
	while ( tm_now2.TM_secs == tm_now1.TM_secs
		&&
		tm_now2.TM_msecs == tm_now1.TM_msecs )
	    TMnow(& tm_now2);
	
	lxcb_p->tlx_23_lxid.dd_dx_id1 = tm_now2.TM_secs;
	lxcb_p->tlx_23_lxid.dd_dx_id2 = tm_now2.TM_msecs;

	/* construct a name from tm_now */

	CVla(tm_now2.TM_secs, hi_ascii);
	CVla(tm_now2.TM_msecs, lo_ascii);
	STpolycat((i4) 3,		/* 3 constituent pieces: "ll_", 2
					** ascii representations of the
					** timestamp */
	"ll_", lo_ascii, hi_ascii, buff);
	MEmove(STlength(buff), buff, ' ', 
	    DB_MAXNAME, lxcb_p->tlx_23_lxid.dd_dx_name);

	/* append the CB to the list hanging off the TPD_LM_CB */

	if (lmcb_p->tlm_3_frst_ptr == (PTR) NULL)
	{
	    /* this is the only (new) CB */

	    if (lmcb_p->tlm_2_cnt != 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 1;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;
					/* must pt to new LX CB */
	}
	else
	{
	    /* insert at end of list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    if (lmcb_p->tlm_2_cnt == 1)
	    {
		if (lmcb_p->tlm_3_frst_ptr != lmcb_p->tlm_4_last_ptr)
		    return( tpd_u2_err_internal(v_tpr_p) );	
	    }
	    lmcb_p->tlm_2_cnt += 1;		
	    last_lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_4_last_ptr;
	    lxcb_p->tlx_21_prev_p = last_lxcb_p;
					/* set prev ptr */
	    last_lxcb_p->tlx_22_next_p = lxcb_p; 
					/* set next ptr */
	    lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;	
					/* must pt to new LX CB */
	}
	break;

    case TPD_1LM_SP_CB:

	/* initialize and set up for this savepoint CB */

	spcb_p = (TPD_SP_CB *) ulm.ulm_pptr;

	MEfill(sizeof(*spcb_p), '\0', (PTR) spcb_p);

	spcb_p->tsp_3_prev_p = NULL;
	spcb_p->tsp_4_next_p = NULL;

	/* append the CB to the savepoint list hanging off the TPD_LM_CB */

	if (lmcb_p->tlm_3_frst_ptr == (PTR) NULL)
	{
	    /* the only CB */

	    if (lmcb_p->tlm_2_cnt != 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 1;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;
					/* must pt to new SP CB */
	}
	else
	{
	    /* insert at end of list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    if (lmcb_p->tlm_2_cnt == 1)
	    {
		if (lmcb_p->tlm_3_frst_ptr != lmcb_p->tlm_4_last_ptr)
		    return( tpd_u2_err_internal(v_tpr_p) );	
	    }
	    lmcb_p->tlm_2_cnt += 1;		
	    last_spcb_p = (TPD_SP_CB *) lmcb_p->tlm_4_last_ptr;
	    spcb_p->tsp_3_prev_p = last_spcb_p;
					/* set prev ptr */
	    last_spcb_p->tsp_4_next_p = spcb_p; 
					/* set next ptr */
	    lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;	
					/* must pt to new SP CB */
	}
	break;

    case TPD_2LM_CD_CB:
    case TPD_5LM_SR_CB:
	/* initialize and set up for this CDB CB */

	cdcb_p = (TPD_CD_CB *) ulm.ulm_pptr;

	MEfill(sizeof(*cdcb_p), '\0', (PTR) cdcb_p);
/*
	cdcb_p->tcd_1_starcdbs		** to be filled in **
*/
	cdcb_p->tcd_2_flags = TPD_0CD_FLAG_NIL;
	cdcb_p->tcd_3_prev_p = NULL;
	cdcb_p->tcd_4_next_p = NULL;

	/* append the CB to the list hanging off the TPD_LM_CB */

	if (lmcb_p->tlm_3_frst_ptr == (PTR) NULL)
	{
	    /* this is the only (new) CB */

	    if (lmcb_p->tlm_2_cnt != 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 1;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;
					/* must pt to new CDB B */
	}
	else
	{
	    /* insert at end of list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    if (lmcb_p->tlm_2_cnt == 1)
	    {
		if (lmcb_p->tlm_3_frst_ptr != lmcb_p->tlm_4_last_ptr)
		    return( tpd_u2_err_internal(v_tpr_p) );	
	    }
	    lmcb_p->tlm_2_cnt += 1;		
	    last_cdcb_p = (TPD_CD_CB *) lmcb_p->tlm_4_last_ptr;
	    cdcb_p->tcd_3_prev_p = last_cdcb_p;
					/* set prev ptr */
	    last_cdcb_p->tcd_4_next_p = cdcb_p; 
					/* set next ptr */
	    lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;	
					/* must pt to new CDB CB */
	}
	break;

    case TPD_3LM_OX_CB:
	/* initialize and set up for this OX CB */

	oxcb_p = (TPD_OX_CB *) ulm.ulm_pptr;

	MEfill(sizeof(*oxcb_p), '\0', (PTR) oxcb_p);
/*
	oxcb_p->tox_1_dxlog		** to be filled in **
*/
	oxcb_p->tox_2_flags = TPD_0OX_FLAG_NIL;
	oxcb_p->tox_3_prev_p = NULL;
	oxcb_p->tox_4_next_p = NULL;

	/* append the CB to the list hanging off the TPD_LM_CB */

	if (lmcb_p->tlm_3_frst_ptr == (PTR) NULL)
	{
	    /* the only CB */

	    if (lmcb_p->tlm_2_cnt != 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 1;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;
					/* must pt to new OX CB */
	}
	else
	{
	    /* insert at end of list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    if (lmcb_p->tlm_2_cnt == 1)
	    {
		if (lmcb_p->tlm_3_frst_ptr != lmcb_p->tlm_4_last_ptr)
		    return( tpd_u2_err_internal(v_tpr_p) );	
	    }
	    lmcb_p->tlm_2_cnt += 1;		
	    last_oxcb_p = (TPD_OX_CB *) lmcb_p->tlm_4_last_ptr;
	    oxcb_p->tox_3_prev_p = last_oxcb_p;
					/* set prev ptr */
	    last_oxcb_p->tox_4_next_p = oxcb_p; 
					/* set next ptr */
	    lmcb_p->tlm_4_last_ptr = (PTR) ulm.ulm_pptr;	
					/* must pt to new OX CB */
	}
	break;

    default:
	return( tpd_u2_err_internal(v_tpr_p) );	
	break;
    }

    return(E_DB_OK);
}


/*{ 
** Name: tpd_u7_term_any_cb - deallocate a CB
** 
** Description: 
**	Deallocate a CB from one of the ULM streams.  
**
** Inputs:
**	i1_stream		- stream identifier, DX_1LM_SP_CB, 
**				  DX_2LM_PRE_LXCB or DX_0LM_LX_CB
**	i2_cbptr_p		- ptr to CB ptr
**	v_tpr_p			- TPR_CB
**  
** Outputs:
**	    none
**
**	Returns:
**		E_TP0000_OK
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None.
**
** History:
**      oct-31-89 (carl)
**          created
*/


DB_STATUS
tpd_u7_term_any_cb(
	i4		i1_stream,
	PTR		*i2_cbptr_p,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = (TPD_LM_CB *) NULL;
    TPD_SP_CB	*spcb_p = (TPD_SP_CB *) NULL,
		*last_spcb_p = (TPD_SP_CB *) NULL,
		**spcb_pp = (TPD_SP_CB **) NULL;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL,
		*last_lxcb_p = (TPD_LX_CB *) NULL,
		**lxcb_pp = (TPD_LX_CB **) NULL;
    TPD_CD_CB	*cdcb_p = (TPD_CD_CB *) NULL,
		*last_cdcb_p = (TPD_CD_CB *) NULL,
		**cdcb_pp = (TPD_CD_CB **) NULL;
    TPD_OX_CB	*oxcb_p = (TPD_OX_CB *) NULL,
		*last_oxcb_p = (TPD_OX_CB *) NULL,
		**oxcb_pp = (TPD_OX_CB **) NULL;


    switch(i1_stream)
    {
    case TPD_0LM_LX_CB:
	lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
	break;
    case TPD_1LM_SP_CB:
	lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
	break;
    case TPD_2LM_CD_CB:
	lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
	break;
    case TPD_3LM_OX_CB:
	lmcb_p = & Tpf_svcb_p->tsv_9_odx_ulmcb;
	break;
    case TPD_4LM_SX_CB:
	lmcb_p = Tpf_svcb_p->tsv_10_slx_ulmcb_p;
	break;
    default:
	return( tpd_u2_err_internal(v_tpr_p) );	
    }

    switch (i1_stream)
    {
    case TPD_0LM_LX_CB:
    case TPD_4LM_SX_CB:

	lxcb_pp = (TPD_LX_CB **) i2_cbptr_p;
	lxcb_p = *lxcb_pp;

	if (lmcb_p->tlm_3_frst_ptr == (PTR) lxcb_p)
	{
	    /* the only CB */

	    if (lmcb_p->tlm_2_cnt != 1)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 0;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) NULL;
					/* remove only CB */
	}
	else
	{
	    /* remove from list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt -= 1;		

	    if (lmcb_p->tlm_3_frst_ptr == (PTR) lxcb_p)   
	    {
		/* remove from head of list */

		lxcb_p->tlx_22_next_p->tlx_21_prev_p = (TPD_LX_CB *) NULL;
					/* set no prev ptr in successor */
		lmcb_p->tlm_3_frst_ptr = (PTR) lxcb_p->tlx_22_next_p;
					/* new first LX CB */
	    }
	    else if (lmcb_p->tlm_4_last_ptr == (PTR) lxcb_p)   
	    {
		/* remove from tail of list */

		lxcb_p->tlx_21_prev_p->tlx_22_next_p = (TPD_LX_CB *) NULL;
					/* set no next ptr in predecessor */
		lmcb_p->tlm_4_last_ptr = (PTR) lxcb_p->tlx_21_prev_p;
					/* new last LX CB */
	    }
	    else
	    {
		/* remove from mid list */

		lxcb_p->tlx_21_prev_p->tlx_22_next_p = lxcb_p->tlx_22_next_p;
					/* set next ptr in predecessor */
		lxcb_p->tlx_22_next_p->tlx_21_prev_p = lxcb_p->tlx_21_prev_p;
					/* set prev ptr in successor */
	    }
	}
	break;
    case TPD_1LM_SP_CB:

	spcb_pp = (TPD_SP_CB **) i2_cbptr_p;
	spcb_p = *spcb_pp;

	if (lmcb_p->tlm_3_frst_ptr == (PTR) spcb_p)
	{
	    /* the only CB */

	    if (lmcb_p->tlm_2_cnt != 1)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 0;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) NULL;
	}
	else
	{
	    /* remove from list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt -= 1;		

	    if (lmcb_p->tlm_3_frst_ptr == (PTR) spcb_p)   
	    {
		/* remove from head of list */

		spcb_p->tsp_4_next_p->tsp_3_prev_p = (TPD_SP_CB *) NULL;
					/* set no prev ptr in successor */
		lmcb_p->tlm_3_frst_ptr = (PTR) spcb_p->tsp_4_next_p;
					/* new first SP CB */
	    }
	    else if (lmcb_p->tlm_4_last_ptr == (PTR) spcb_p)   
	    {
		/* remove from tail of list */

		spcb_p->tsp_3_prev_p->tsp_4_next_p = (TPD_SP_CB *) NULL;
					/* set no next ptr in predecessor */
		lmcb_p->tlm_4_last_ptr = (PTR) spcb_p->tsp_3_prev_p;
					/* new last SP CB */
	    }
	    else
	    {
		/* remove from mid list */

		spcb_p->tsp_3_prev_p->tsp_4_next_p = spcb_p->tsp_4_next_p;
					/* set next ptr in predecessor */
		spcb_p->tsp_4_next_p->tsp_3_prev_p = spcb_p->tsp_3_prev_p;
					/* set prev ptr in successor */
	    }
	}
	break;
    case TPD_2LM_CD_CB:

	cdcb_pp = (TPD_CD_CB **) i2_cbptr_p;
	cdcb_p = *cdcb_pp;

	if (lmcb_p->tlm_3_frst_ptr == (PTR) cdcb_p)
	{
	    /* the only CB */

	    if (lmcb_p->tlm_2_cnt != 1)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 0;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) NULL;
	}
	else
	{
	    /* remove from list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt -= 1;		

	    if (lmcb_p->tlm_3_frst_ptr == (PTR) cdcb_p)   
	    {
		/* remove from head of list */

		cdcb_p->tcd_4_next_p->tcd_3_prev_p = (TPD_CD_CB *) NULL;
					/* set no prev ptr in successor */
		lmcb_p->tlm_3_frst_ptr = (PTR) cdcb_p->tcd_4_next_p;
					/* new first CD CB */
	    }
	    else if (lmcb_p->tlm_4_last_ptr == (PTR) cdcb_p)   
	    {
		/* remove from tail of list */

		cdcb_p->tcd_3_prev_p->tcd_4_next_p = (TPD_CD_CB *) NULL;
					/* set no next ptr in predecessor */
		lmcb_p->tlm_4_last_ptr = (PTR) cdcb_p->tcd_3_prev_p;
					/* new last CD CB */
	    }
	    else
	    {
		/* remove from mid list */

		cdcb_p->tcd_3_prev_p->tcd_4_next_p = cdcb_p->tcd_4_next_p;
					/* set next ptr in predecessor */
		cdcb_p->tcd_4_next_p->tcd_3_prev_p = cdcb_p->tcd_3_prev_p;
					/* set prev ptr in successor */
	    }
	}
	break;
    case TPD_3LM_OX_CB:

	oxcb_pp = (TPD_OX_CB **) i2_cbptr_p;
	oxcb_p = *oxcb_pp;

	if (lmcb_p->tlm_3_frst_ptr == (PTR) oxcb_p)
	{
	    /* the only CB */

	    if (lmcb_p->tlm_2_cnt != 1)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt = 0;		
	    lmcb_p->tlm_3_frst_ptr = 
		lmcb_p->tlm_4_last_ptr = (PTR) NULL;
	}
	else
	{
	    /* remove from list */

	    if (lmcb_p->tlm_2_cnt <= 0)
		return( tpd_u2_err_internal(v_tpr_p) );	

	    lmcb_p->tlm_2_cnt -= 1;		

	    if (lmcb_p->tlm_3_frst_ptr == (PTR) oxcb_p)   
	    {
		/* remove from head of list */

		oxcb_p->tox_4_next_p->tox_3_prev_p = (TPD_OX_CB *) NULL;
					/* set no prev ptr in successor */
		lmcb_p->tlm_3_frst_ptr = (PTR) oxcb_p->tox_4_next_p;
					/* new first OX CB */
	    }
	    else if (lmcb_p->tlm_4_last_ptr == (PTR) oxcb_p)   
	    {
		/* remove from tail of list */

		oxcb_p->tox_3_prev_p->tox_4_next_p = (TPD_OX_CB *) NULL;
					/* set no next ptr in predecessor */
		lmcb_p->tlm_4_last_ptr = (PTR) oxcb_p->tox_3_prev_p;
					/* new last OX CB */
	    }
	    else
	    {
		/* remove from mid list */

		oxcb_p->tox_3_prev_p->tox_4_next_p = oxcb_p->tox_4_next_p;
					/* set next ptr in predecessor */
		oxcb_p->tox_4_next_p->tox_3_prev_p = oxcb_p->tox_3_prev_p;
					/* set prev ptr in successor */
	    }
	}
	break;
    default:
	return( tpd_u2_err_internal(v_tpr_p) );	
    }

    return(E_DB_OK);
}


/*{
** Name: tpd_u8_close_streams - close all substreams for session
**
** Description: 
**
**	Close the savepoint, registered LX CB, pre-registered LX CB streams
**	when the session commits, aborts or terminates.
**
** Inputs:
**	v_tpr_p->
**	    tpr_session			session control block
**
** Outputs:
**
**	none
**		
**
**	Returns:
**	    DB_STATUS
**
** Side Effects:
**	    None
**
** History:
**      nov-11-89 (carl)
**          created
**	30-jul-97 (nanpr01)
**	    After the memory is deallocated set the streamid to NULL.
*/


DB_STATUS
tpd_u8_close_streams(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = (TPD_LM_CB *) NULL;
    ULM_RCB	ulm;


    ulm.ulm_facility = DB_QEF_ID;

    /* close stream for session savepoints if any */

    lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
    if (lmcb_p->tlm_1_streamid != NULL)
    {
	ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;

	status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	/* ULM has nullified tlm_1_streamid */

	/* change to ignore any errors */
    }

    /* close stream for session registered LXs if any */

    lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    if (lmcb_p->tlm_1_streamid != NULL)
    {
	ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;

	status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	/* ULM has nullified tlm_1_streamid */

	/* change to ignore any errors */
    }
    return(E_DB_OK);
}


/*{
** Name: tpd_u9_2pc_ldb_status - extract the LX's LDB status from RQF's RCB 
**				after failure
**
** Description:
**      This routine extracts the returned LDB status after an RQF failure.
**  
** Inputs:
**	    i1_rqr_p			RQF_CB
**	    o1_lxcb_p			LX CB
**
** Outputs:
**	o1_lxcb_p->
**	    tlx_ldb_status
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
**      26-oct-89 (carl)
**          adapted
*/


DB_STATUS
tpd_u9_2pc_ldb_status(
	RQR_CB	    *i1_rqr_p,
	TPD_LX_CB   *o1_lxcb_p)
{


    switch (i1_rqr_p->rqr_3ldb_local_id)
    {
    case E_DB_OK:
	o1_lxcb_p->tlx_ldb_status = LX_00LDB_OK;
	break;
    case E_US0010_NO_SUCH_DB:   /* LDB does not exist */
	o1_lxcb_p->tlx_ldb_status = LX_08LDB_NO_SUCH_LDB;
	break;
    case E_US1266_4710_TRAN_ID_NOTUNIQUE:   /* DX id not unique */
	o1_lxcb_p->tlx_ldb_status = LX_02LDB_DX_ID_NOTUNIQUE;
	break;
    case E_US1267_4711_DIS_TRAN_UNKNOWN:    /* unknown DX */
	o1_lxcb_p->tlx_ldb_status = LX_03LDB_DX_ID_UNKNOWN;
	break;
    case E_US1268_4712_DIS_TRAN_OWNER:	    /* DX owned by another server */
	o1_lxcb_p->tlx_ldb_status = LX_04LDB_DX_ALREADY_ADOPTED;
	break;
    case E_US1269_4713_NOSECUREINCLUSTER:   /* SECURE not supported */
	o1_lxcb_p->tlx_ldb_status = LX_05LDB_NOSECURE;
	break;
    case E_US126A_4714_ILLEGAL_STMT:	    /* only ROLLBACK/COMMIT allowed 
					    ** following SECURE/RECONNECT */
	o1_lxcb_p->tlx_ldb_status = LX_06LDB_ILLEGAL_STMT;
	break;
    case E_US126B_4715_DIS_DB_UNKNOWN:	    /* unrecognized connecting DB */
	o1_lxcb_p->tlx_ldb_status = LX_07LDB_DB_UNKNOWN;
	break;
    default:
	o1_lxcb_p->tlx_ldb_status = LX_01LDB_UNEXPECTED_ERR;
	break;
    }

    return(E_DB_ERROR);
}


/*{
** Name: tpd_u10_fatal - a fatal-error condition routine
**
** Description:
**      This routine serves as a trap point for catching errors.
**
** Inputs:
**	i1_errcode		the error
**	v1_rcb_p		ptr to TPR_CB
**  
** Outputs:
**	v1_rcb_p->		ptr to TPR_CB
**	    tpr_error
**		.err_code	set to i1_errcode
**
**	Returns:
**	    E_DB_FATAL
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      19-nov-89 (carl)
**          adapted
**	07-oct-1993 (tad)
**	    Bug #56449
**	    Changed %x to %p for pointer values.
*/

DB_STATUS
tpd_u10_fatal(
	DB_ERRTYPE  i1_errcode,
	TPR_CB	    *v_tpr_p)
{
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;


    if (E_TP0000_OK < i1_errcode
	&&
	i1_errcode <= E_TP_ERROR_MAX)
    {
	TRdisplay("%s %p: Fatal TPF error %x occurred\n", 
	    IITP_00_tpf_p,
	    sscb_p,
	    i1_errcode);
    }
    else
	TRdisplay("%s %p: Fatal error %x occurred\n", 
	    IITP_00_tpf_p,
	    sscb_p,
	    i1_errcode);

    v_tpr_p->tpr_error.err_code = i1_errcode;
    return(E_DB_FATAL);
}


/*{
** Name: tpd_u11_term_assoc - terminate an association to an LDB
** 
**  Description:    
**	
**
** Inputs:
**	i1_ldb_p			ptr to the DD_LDB_DESC
**	v_tpr_p				ptr to the session CB
**	
** Outputs:
**	none
**
**	Returns:
**	    DB_STATUS
**	    
**	Exceptions:
**	    None
**
** Side Effects:
**	    
**
** History:
**      19-nov-89 (carl)
**          created
*/


DB_STATUS
tpd_u11_term_assoc(
	DD_LDB_DESC *i1_ldb_p,
	TPR_CB	    *v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    RQR_CB	rqr_cb;
    i4		rqr_request;


    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    /* set up static parameters to call RQF */

    rqr_cb.rqr_2pc_dx_id = (DD_DX_ID *) NULL;
    rqr_cb.rqr_q_language = DB_NOLANG;
    rqr_cb.rqr_session = sscb_p->tss_rqf;
    rqr_cb.rqr_msg.dd_p1_len = 0;
    rqr_cb.rqr_msg.dd_p2_pkt_p = NULL;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;
    rqr_cb.rqr_1_site = i1_ldb_p;
    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
    rqr_cb.rqr_timeout = 0;

    status = tpd_u4_rqf_call(RQR_TERM_ASSOC, & rqr_cb, v_tpr_p);
    return(E_DB_OK);	/* forgive any error */
}


/*{
** Name: tpd_u12_trim_cdbstrs - trim trailing whites off CDB-related strings
** 
**  Description:    
**	
**	(See above.)
**
** Inputs:
**	i1_cdb_p			ptr to DD_LDB_DESC
**	
** Outputs:
**	o1_cdb_node			ptr to char buffer[DB_MAXNAME + 1]
**	o2_cdb_name			ptr to char buffer[DB_MAXNAME + 1]
**	o3_cdb_dbms			ptr to char buffer[DB_MAXNAME + 1]
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
**      28-may-90 (carl)
**          created
*/


VOID
tpd_u12_trim_cdbstrs(
	DD_LDB_DESC *i1_cdb_p,
	char	    *o1_cdb_node,
	char	    *o2_cdb_name,
	char	    *o3_cdb_dbms)
{
    MEcopy(i1_cdb_p->dd_l2_node_name, DB_MAXNAME, o1_cdb_node);
    o1_cdb_node[DB_MAXNAME] = EOS;
    STtrmwhite(o1_cdb_node);

    MEcopy(i1_cdb_p->dd_l3_ldb_name, DB_MAXNAME, o2_cdb_name);
    o2_cdb_name[DB_MAXNAME] = EOS;
    STtrmwhite(o2_cdb_name);

    MEcopy(i1_cdb_p->dd_l4_dbms_name, DB_MAXNAME, o3_cdb_dbms);
    o3_cdb_dbms[DB_MAXNAME] = EOS;
    STtrmwhite(o3_cdb_dbms);
}


/*{
** Name: tpd_u13_trim_ddbstrs - trim trailing whites off DDB-related strings
** 
**  Description:    
**	
**	(See above.)
**
** Inputs:
**	i1_starcdb_p			ptr to TPC_I1_STAR_CDBS
**	
** Outputs:
**	o1_ddb_name			ptr to char buffer[DB_MAXNAME + 1]
**	o2_ddb_owner			ptr to char buffer[DB_MAXNAME + 1]
**	o3_cdb_name			ptr to char buffer[DB_MAXNAME + 1]
**	o4_cdb_owner			ptr to char buffer[DB_MAXNAME + 1]
**	o5_cdb_dbms			ptr to char buffer[DB_MAXNAME + 1]
**	o6_cdb_node			ptr to char buffer[DB_MAXNAME + 1]
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
**      28-may-90 (carl)
**          created
*/


VOID
tpd_u13_trim_ddbstrs(
	TPC_I1_STARCDBS	    
    *i1_starcdb_p,
	char	    *o1_ddb_name,
	char	    *o2_ddb_owner,
	char	    *o3_cdb_name,
	char	    *o4_cdb_node,
	char	    *o5_cdb_dbms)
{
    MEcopy(i1_starcdb_p->i1_1_ddb_name, DB_MAXNAME, o1_ddb_name);
    o1_ddb_name[DB_MAXNAME] = EOS;
    STtrmwhite(o1_ddb_name);

    MEcopy(i1_starcdb_p->i1_2_ddb_owner, DB_MAXNAME, o2_ddb_owner);
    o2_ddb_owner[DB_MAXNAME] = EOS;
    STtrmwhite(o2_ddb_owner);

    MEcopy(i1_starcdb_p->i1_3_cdb_name, DB_MAXNAME, o3_cdb_name);
    o3_cdb_name[DB_MAXNAME] = EOS;
    STtrmwhite(o3_cdb_name);

    MEcopy(i1_starcdb_p->i1_5_cdb_node, DB_MAXNAME, o4_cdb_node);
    o4_cdb_node[DB_MAXNAME] = EOS;
    STtrmwhite(o4_cdb_node);

    MEcopy(i1_starcdb_p->i1_6_cdb_dbms, DB_MAXNAME, o5_cdb_dbms);
    o5_cdb_dbms[DB_MAXNAME] = EOS;
    STtrmwhite(o5_cdb_dbms);
}


/*{
** Name: tpd_u14_log_rqferr - Map an error generated by RQF.
**
** Description:
**      This routine maps an LDB error generated by RQF.
**
** Inputs:
**	i1_rqfcall			RQR op code
**	i2_rqr_p->			RQR_CB
**	    rqr_error
**		.err_code
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
**      26-oct-89 (carl)
**          adapted
**      06-oct-90 (carl)
**	    process trace points
*/


DB_STATUS
tpd_u14_log_rqferr(
	i4		 i1_rqfcall,
	RQR_CB		*i2_rqr_p,
	TPR_CB		*v_tpr_p)
{
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    i4     i4_1,
                i4_2;


    if (Tpf_svcb_p->tsv_4_flags & TSV_01_IN_RECOVERY)
    {
	/* trace this RQF error to the log */

	tpd_p2_prt_rqferr(i1_rqfcall, i2_rqr_p, v_tpr_p, TPD_1TO_TR);
    }
    else if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
	/* trace this RQF error to the FE */

	tpd_p2_prt_rqferr(i1_rqfcall, i2_rqr_p, v_tpr_p, TPD_0TO_FE);
    }

    switch (i2_rqr_p->rqr_error.err_code)
    {
    case E_RQ0001_WRONG_COLUMN_COUNT:
	v_tpr_p->tpr_error.err_code = E_TP0501_WRONG_COLUMN_COUNT;
	break;
    case E_RQ0002_NO_TUPLE_DESCRIPTION:
	v_tpr_p->tpr_error.err_code = E_TP0502_NO_TUPLE_DESCRIPTION;
	break;
    case E_RQ0003_TOO_MANY_COLUMNS:
	v_tpr_p->tpr_error.err_code = E_TP0503_TOO_MANY_COLUMNS;
	break;
    case E_RQ0004_BIND_BUFFER_TOO_SMALL:
	v_tpr_p->tpr_error.err_code = E_TP0504_BIND_BUFFER_TOO_SMALL;
	break;
    case E_RQ0005_CONVERSION_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0505_CONVERSION_FAILED;
	break;
    case E_RQ0006_CANNOT_GET_ASSOCIATION:
	v_tpr_p->tpr_error.err_code = E_TP0506_CANNOT_GET_ASSOCIATION;
	break;
    case E_RQ0007_BAD_REQUEST_CODE:
	v_tpr_p->tpr_error.err_code = E_TP0507_BAD_REQUEST_CODE;
	break;
    case E_RQ0008_SCU_MALLOC_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0508_SCU_MALLOC_FAILED;
	break;
    case E_RQ0009_ULM_STARTUP_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0509_ULM_STARTUP_FAILED;
	break;
    case E_RQ0010_ULM_OPEN_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0510_ULM_OPEN_FAILED;
	break;
    case E_RQ0011_INVALID_READ:
	v_tpr_p->tpr_error.err_code = E_TP0511_INVALID_READ;
	break;
    case E_RQ0012_INVALID_WRITE:
	v_tpr_p->tpr_error.err_code = E_TP0512_INVALID_WRITE;
	break;
    case E_RQ0013_ULM_CLOSE_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0513_ULM_CLOSE_FAILED;
	break;
    case E_RQ0014_QUERY_ERROR:
	v_tpr_p->tpr_error.err_code = E_TP0514_QUERY_ERROR;
	break;
    case E_RQ0015_UNEXPECTED_MESSAGE:
	v_tpr_p->tpr_error.err_code = E_TP0515_UNEXPECTED_MESSAGE;
	break;
    case E_RQ0016_CONVERSION_ERROR:
	v_tpr_p->tpr_error.err_code = E_TP0516_CONVERSION_ERROR;
	break;
    case E_RQ0017_NO_ACK:
	v_tpr_p->tpr_error.err_code = E_TP0517_NO_ACK;
	break;
    case E_RQ0018_SHUTDOWN_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0518_SHUTDOWN_FAILED;
	break;
    case E_RQ0019_COMMIT_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0519_COMMIT_FAILED;
	break;
    case E_RQ0020_ABORT_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0520_ABORT_FAILED;
	break;
    case E_RQ0021_BEGIN_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0521_BEGIN_FAILED;
	break;
    case E_RQ0022_END_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0522_END_FAILED;
	break;
    case E_RQ0023_COPY_FROM_EXPECTED:
	v_tpr_p->tpr_error.err_code = E_TP0523_COPY_FROM_EXPECTED;
	break;
    case E_RQ0024_COPY_DEST_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0524_COPY_DEST_FAILED;
	break;
    case E_RQ0025_COPY_SOURCE_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0525_COPY_SOURCE_FAILED;
	break;
    case E_RQ0026_QID_EXPECTED:
	v_tpr_p->tpr_error.err_code = E_TP0526_QID_EXPECTED;
	break;
    case E_RQ0027_CURSOR_CLOSE_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0527_CURSOR_CLOSE_FAILED;
	break;
    case E_RQ0028_CURSOR_FETCH_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0528_CURSOR_FETCH_FAILED;
	break;
    case E_RQ0029_CURSOR_EXEC_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0529_CURSOR_EXEC_FAILED;
	break;
    case E_RQ0030_CURSOR_DELETE_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0530_CURSOR_DELETE_FAILED;
	break;
    case E_RQ0031_INVALID_CONTINUE:
	v_tpr_p->tpr_error.err_code = E_TP0531_INVALID_CONTINUE;
	break;
    case E_RQ0032_DIFFERENT_TUPLE_SIZE:
	v_tpr_p->tpr_error.err_code = E_TP0532_DIFFERENT_TUPLE_SIZE;
	break;
    case E_RQ0033_FETCH_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0533_FETCH_FAILED;
	break;
    case E_RQ0034_COPY_CREATE_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0534_COPY_CREATE_FAILED;
	break;
    case E_RQ0035_BAD_COL_DESC_FORMAT:
	v_tpr_p->tpr_error.err_code = E_TP0535_BAD_COL_DESC_FORMAT;
	break;
    case E_RQ0036_COL_DESC_EXPECTED:
	v_tpr_p->tpr_error.err_code = E_TP0536_COL_DESC_EXPECTED;
	break;
    case E_RQ0037_II_LDB_NOT_DEFINED:
	v_tpr_p->tpr_error.err_code = E_TP0537_II_LDB_NOT_DEFINED;
	break;
    case E_RQ0038_ULM_ALLOC_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0538_ULM_ALLOC_FAILED;
	break;
    case E_RQ0039_INTERRUPTED:
	v_tpr_p->tpr_error.err_code = E_TP0022_QUERY_ABORTED;
	break;
    case E_RQ0040_UNKNOWN_REPEAT_Q:
	v_tpr_p->tpr_error.err_code = E_TP0540_UNKNOWN_REPEAT_Q;
	break;
    case E_RQ0041_ERROR_MSG_FROM_LDB:
	v_tpr_p->tpr_error.err_code = E_TP0541_ERROR_MSG_FROM_LDB;
	break;
    case E_RQ0042_LDB_ERROR_MSG:
	v_tpr_p->tpr_error.err_code = E_TP0542_LDB_ERROR_MSG;
	break;
    case E_RQ0043_CURSOR_UPDATE_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0543_CURSOR_UPDATE_FAILED;
	break;
    case E_RQ0044_CURSOR_OPEN_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0544_CURSOR_OPEN_FAILED;
	break;
    case E_RQ0045_CONNECTION_LOST:
	v_tpr_p->tpr_error.err_code = E_TP0545_CONNECTION_LOST;
	break;
    case E_RQ0046_RECV_TIMEOUT:
	v_tpr_p->tpr_error.err_code = E_TP0546_RECV_TIMEOUT;
	break;
    case E_RQ0047_UNAUTHORIZED_USER:
	v_tpr_p->tpr_error.err_code = E_TP0547_UNAUTHORIZED_USER;
	break;
    case E_RQ0048_SECURE_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0548_SECURE_FAILED;
	break;
    case E_RQ0049_RESTART_FAILED:
	v_tpr_p->tpr_error.err_code = E_TP0549_RESTART_FAILED;
	break;
    default:
	v_tpr_p->tpr_error.err_code = i2_rqr_p->rqr_error.err_code;
	break;
    }
    return(E_DB_OK);
}

