/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <di.h>
#include    <lo.h>
#include    <tm.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfddb.h>
#include    <tpfqry.h>
#include    <tpfproto.h>


/**
**
** Name: TPDSVPT.C - TPF's savepoint routines
** 
** Description: 
**  This file includes all the routines for managing savepoints.
** 
**	tpd_s1_srch_sp		- search for a savepoint in the list
**	tpd_s2_clear_all_sps	- clear all savepoints in the list
**	tpd_s3_send_sps_to_ldb 	- send new savepoints to an LDB
**	tpd_s4_abort_to_svpt	- abort to a savepoint 
**
**  History:    $Log-for RCS$
**      nov-07-89 (carl)
**          split off from tpdmisc.c
**      oct-09-90 (carl)
**	    process trace points
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	feb-02-1993 (fpang)
**	    In tpd_s4_abort_to_svpt(), if 'rollback' is sent to LDB, register 
**	    it in the DX so that it can be committed/aborted.
**	    Fixes B49222.
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Include tpfqry.h, tpfproto.h to get prototypes.
**/

GLOBALREF   char		*IITP_10_savepoint_p;	/* "savepoint" */

GLOBALREF   char		*IITP_11_abort_to_p;	/* "abort to" */

GLOBALREF   char		*IITP_12_rollback_to_p;	/* "rollback to" */


/*{ 
** Name: tpd_s1_srch_sp	- search for a savepoint
** 
** Description: 
**	Search a savepoint list for the specified search point.
**
** Inputs:
**	v_sscb_p		- TPF session
**	sp_name			- target save point name
**
** Outputs:
**	o1_good_spcnt_p		- ptr to returned number of good savepoints
**
**	Returns:
**		TPD_SP_CB * or NULL if sp not found
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      Dec-27-88 (alexh)
**          created
**      jun-03-89 (carl)
**	    added output parameter o1_good_cnt_p
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


TPD_SP_CB *
tpd_s1_srch_sp(
	TPD_SS_CB	*v_sscb_p,
	DB_SP_NAME	*sp_name,
	i4		*o1_good_cnt_p)
{
    TPD_DX_CB	*dxcb_p = & v_sscb_p->tss_dx_cb;
    TPD_LM_CB	*splm_p = & dxcb_p->tdx_23_sp_ulmcb;
    TPD_SP_CB	*cur_sp = (TPD_SP_CB *) splm_p->tlm_3_frst_ptr;
    bool	found = FALSE;			/* assume */
    i4		good_cnt;


    *o1_good_cnt_p = 0;				/* assume */

    if (cur_sp == (TPD_SP_CB *) NULL)
	return((TPD_SP_CB *) NULL);

    for (found = FALSE, good_cnt = 0; !found && (cur_sp != NULL); ++good_cnt)
    { 
	if (MEcmp(sp_name, & cur_sp->tsp_1_name, sizeof(DB_SP_NAME)) == 0)
	{
	    /* match */

	    found = TRUE;
	}
	else
	    cur_sp = cur_sp->tsp_4_next_p;	/* advance to next sp */
    }

    *o1_good_cnt_p = good_cnt;			/* return good count */
    return(cur_sp);
}


/*{ 
** Name: tpd_s2_clear_all_sps - clear all savepoints
** 
** Description: 
**	Remove all savepoints info. Usually called during ending of
**	transactions or the last savepoint abort,
**
** Inputs:
**	v_tpr_p				- TPR_CB with session CB
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
**      Dec-27-88 (alexh)
**          created
**	jun-03-89 (carl)
**	    modified to handle new fields tdx_13_frst_sp_p and tlm_4_last_ptr  
*/


DB_STATUS
tpd_s2_clear_all_sps(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    TPD_LM_CB	*lxlm_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPD_LM_CB	*splm_p = & dxcb_p->tdx_23_sp_ulmcb;
    ULM_RCB	ulm;


    /* 1.  MUST reinitialize every LDB's savepoint count */

    lxcb_p = (TPD_LX_CB *) lxlm_p->tlm_3_frst_ptr;  /* first LX CB in list */
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	lxcb_p->tlx_sp_cnt = 0;
	lxcb_p = lxcb_p->tlx_22_next_p;	/* advance to next LX CB */
    }

    if (splm_p->tlm_1_streamid == (PTR) NULL)
	return(E_DB_OK);    /* session has no outstanding savepoints */

    /* 2.  MUST close the memory stream opend for savepoints */

    ulm.ulm_facility = DB_QEF_ID;   /* borrow QEF's id */
    ulm.ulm_streamid_p = &splm_p->tlm_1_streamid;	/* use before nulling */
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;

    splm_p->tlm_2_cnt = 0;
    splm_p->tlm_3_frst_ptr = 
        splm_p->tlm_4_last_ptr = (PTR) NULL;

    status = ulm_closestream(& ulm);
    if (status != E_DB_OK)
    {
	return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
    }
    /* ULM has nullified tlm_1_streamid */
    return(E_DB_OK);
}


/*{ 
** Name: tpd_s3_send_sps_to_ldb - Send new savepoints to an LDB.
** 
** Description: 
**	All new registered savepoints will be sent to the LDB.
**
** Inputs:
**	v_tpr_p				- TPR_CB with session ptr
**	v1_lxcb_p			- ptr to the site's entry
**
** Outputs:
**	None
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
**      jun-01-89 (carl)
**          created
**      oct-09-90 (carl)
**	    process trace points
*/


DB_STATUS
tpd_s3_send_sps_to_ldb(
	TPR_CB		*v_tpr_p,
	TPD_LX_CB	*v1_lxcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*splm_p = & dxcb_p->tdx_23_sp_ulmcb;
    TPD_SP_CB	*spcb_p = (TPD_SP_CB *) splm_p->tlm_3_frst_ptr;
    i4		i;
    RQR_CB	rqr_cb;
    bool        trace_svpt_103 = FALSE,
                trace_err_104 = FALSE;
    i4     i4_1,
                i4_2;
    char	svpt_name[sizeof(DB_SP_NAME) + 1],
		qrytxt[100];


    if (spcb_p == NULL			/* no savepoints ? */
	||
	v1_lxcb_p->tlx_sp_cnt >= splm_p->tlm_2_cnt)
					/* LDB has seen all savepoints ? */
	return(E_DB_OK);		/* yes, nothing to do */

    /* skip the privileged CDB if DDL concurrency is ON, an "autocommit"
    ** condition */

    if (sscb_p->tss_ddl_cc && v1_lxcb_p->tlx_site.dd_l1_ingres_b)	
	return(E_DB_OK);		    /* nothing to do */

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_SVPT_103,
            & i4_1, & i4_2))
    {
        trace_svpt_103 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    /* pre-set static parameters to call RQF */

    rqr_cb.rqr_timeout = 0;
    rqr_cb.rqr_q_language = v_tpr_p->tpr_lang_type;
    rqr_cb.rqr_session = sscb_p->tss_rqf;
    
    /* query length to be set below when constructed */

    rqr_cb.rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;
    rqr_cb.rqr_1_site = & v1_lxcb_p->tlx_site;
    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;

    /* position at the first savepoint not yet sent to the site */

    if (v1_lxcb_p->tlx_sp_cnt > 0)
	for (i = 0; spcb_p != NULL && i < v1_lxcb_p->tlx_sp_cnt; ++i)
	    spcb_p = spcb_p->tsp_4_next_p;  
				/* advance to the next savepoint */
    while (spcb_p != NULL)
    {
	/* set up dynamic parameters to request current savepoint */

	/* construct query for current savepoint on the fly */

	tpd_u0_trimtail((char *) & spcb_p->tsp_1_name, 
	    sizeof(DB_SP_NAME), svpt_name);
	if (v_tpr_p->tpr_lang_type == DB_QUEL)
	    STprintf(qrytxt, "%s %s;", IITP_10_savepoint_p, svpt_name);	
	else
	    STprintf(qrytxt, "%s %s;", IITP_10_savepoint_p, svpt_name);	

	rqr_cb.rqr_msg.dd_p1_len = (i4) STlength(qrytxt);

	if (trace_svpt_103)
	    tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, & v1_lxcb_p->tlx_site,
		TPD_0TO_FE);

	status = tpd_u4_rqf_call(RQR_QUERY, & rqr_cb, v_tpr_p);
	if (status)
	{
	    if (trace_err_104)
		tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, & v1_lxcb_p->tlx_site,
		    TPD_0TO_FE);
	    return(status);
	}
	spcb_p = spcb_p->tsp_4_next_p;	/* advance to next savepoint */
    }

    v1_lxcb_p->tlx_sp_cnt = splm_p->tlm_2_cnt;
					/* all savepoints are now known to site
					*/
    return(E_DB_OK);
}


/*{ 
** Name: tpd_s4_abort_to_svpt - abort to savepoint 
** 
** Description: 
**	Request abort to savepoint if already sent and hence known to LDB.
**
** Inputs:
**	v_tpr_p			- TPR_CB
**	i_new_svpt_cnt		- number of savepoints left
**	i_svpt_name_p		- abort savepoint name
**
** Outputs:
**	None
**
**	Returns:
**	    E_DB_OK	if all aborted sucessfully
**	    E_DB_ERROR	not all aborted successfully
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      jun-03-89 (carl)
**	    adapted 
**      oct-09-90 (carl)
**	    process trace points
**	feb-02-1993 (fpang)
**	    If 'rollback' is sent to LDB, register it in the DX so that it can
**	    be committed/aborted.
**	    Fixes B42999.
*/


DB_STATUS
tpd_s4_abort_to_svpt(
	TPR_CB		*v_tpr_p,
	i4		 i_new_svpt_cnt,
	DB_SP_NAME	*i_svpt_name_p)
{
    DB_STATUS	status = E_DB_OK;
    DB_ERROR	save_err;
    bool	ok = TRUE;	    /* indicate whether all aborts are OK */
    TPD_SS_CB   *sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;    /* transaction context */
    TPD_LM_CB	*splm_p = & dxcb_p->tdx_23_sp_ulmcb,
		*lxlm_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = NULL;
    TPD_SP_CB	*spcb_p = NULL;
    i4		i,
		j;
    bool        trace_svpt_103 = FALSE,
                trace_err_104 = FALSE;
    i4     i4_1,
                i4_2;
    RQR_CB	rqr_cb;		    /* RQF request control block */
    char	svpt_name[sizeof(DB_SP_NAME) + 1],
		qrytxt[100];


    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_SVPT_103,
            & i4_1, & i4_2))
    {
        trace_svpt_103 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    rqr_cb.rqr_q_language = v_tpr_p->tpr_lang_type;
    rqr_cb.rqr_session = sscb_p->tss_rqf;

    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;

    tpd_u0_trimtail((char*)i_svpt_name_p, sizeof(DB_SP_NAME), svpt_name);
    if (v_tpr_p->tpr_lang_type == DB_QUEL)
	STprintf(qrytxt, "%s %s;", IITP_11_abort_to_p, svpt_name);	
    else
	STprintf(qrytxt, "%s %s;", IITP_12_rollback_to_p, svpt_name);	
    rqr_cb.rqr_msg.dd_p1_len = (i4) STlength(qrytxt);
    rqr_cb.rqr_msg.dd_p2_pkt_p = qrytxt;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;

    /* loop through each site to abort to this savepoint if known to the site */

    lxcb_p = (TPD_LX_CB *) lxlm_p->tlm_3_frst_ptr;
    i = 0;
    while (lxcb_p != (TPD_LX_CB *) NULL && status == E_DB_OK)
    {
	if (i_new_svpt_cnt <= lxcb_p->tlx_sp_cnt)   /* LDB has knowledge of this
						    ** savepoint */
	{
	    /* set up to inform of rollback to this savepoint */

	    rqr_cb.rqr_1_site = & lxcb_p->tlx_site;

	    /* skip CDB if DDL_CONCURRENCY is ON */

	    if (! (sscb_p->tss_ddl_cc && rqr_cb.rqr_1_site->dd_l1_ingres_b))
	    {
		rqr_cb.rqr_dv_cnt = 0;
		rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
		rqr_cb.rqr_timeout = 0;

		if (trace_svpt_103)
		    tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, & lxcb_p->tlx_site,
			TPD_0TO_FE);

		status = tpd_u4_rqf_call(RQR_QUERY, & rqr_cb, v_tpr_p);
		if (status)
		{
		    if (trace_err_104)
			tpd_p5_prt_tpfqry(v_tpr_p, qrytxt, 
			    & lxcb_p->tlx_site, TPD_0TO_FE);
		    STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, save_err);
		    ok = FALSE;
		}
		lxcb_p->tlx_20_flags |= LX_01FLAG_REG;  /* Register.. */
	    }
	    lxcb_p->tlx_sp_cnt = i_new_svpt_cnt; /* these are all savepoints 
					    ** the LDB has knowledge of */
	}
	lxcb_p = lxcb_p->tlx_22_next_p;
	++i;				    /* debugging aid */
    }
    /* update to reflect fewer outstanding session savepoints if necessary */

    if (i_new_svpt_cnt < splm_p->tlm_2_cnt)
    {
	spcb_p = (TPD_SP_CB *) splm_p->tlm_3_frst_ptr;
	for (i = 1; (spcb_p != (TPD_SP_CB *) NULL) && (i < i_new_svpt_cnt); ++i)
	{
	    if (spcb_p != (TPD_SP_CB *) NULL)
		spcb_p = spcb_p->tsp_4_next_p;
	}
	if (i != i_new_svpt_cnt)
	    ok = FALSE;
	else
	{
	    /* update to reflect */

	    spcb_p->tsp_4_next_p = (TPD_SP_CB *) NULL;
	    splm_p->tlm_4_last_ptr = (PTR) spcb_p;
	    splm_p->tlm_2_cnt = i_new_svpt_cnt;
	}
    }
    if (ok)
	return(E_DB_OK);
    else
    {
	STRUCT_ASSIGN_MACRO(save_err, v_tpr_p->tpr_error);
	return(E_DB_ERROR);
    }    
}

