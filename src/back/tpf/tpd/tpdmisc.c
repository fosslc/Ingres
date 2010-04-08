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
** Name: TPDMISC.C - TPF's distributed transaction primitives
** 
** Description: 
**	This file includes all the primitives for manipulating the distributed 
**	transacion table.
** 
**	    tpd_m1_set_dx_id 	- transaction id generator
**	    tpd_m2_init_dx_cb	- initialize a DX CB
**	    tpd_m3_term_dx_cb	- terminate a DX CB
**	    tpd_m4_ok_ldb_for_dx- decide if an updating LDB can join the DX
**	    tpd_m5_ok_write_ldbs- validate the updating LDBs of a query for 
**				  the DX
**	    tpd_m6_register_ldb - register an LDB with a DX
**	    tpd_m7_srch_ldb	- search an LDB's LX CB in the (pre-)registered
**	    tpd_m8_new_dx_mode	- determine DX's mode after transition 
**	    tpd_m9_ldb_to_lxcb	- return an LX CB that matches the LDB 
**	    tpd_m10_is_ddb_open	- return TRUE if a DDB in question is open 
**	    tpd_m11_is_ddb_open	- return TRUE if a DDB in question is open 
**
**  History:    $Log-for RCS$
**      mar-11-88 (alexh)
**          created
**      may-20-89 (carl)
**	    changed tpd_m12_send_abort_qry to add parameter session and to 
**	    skip sending savepoint to the CDB if DDL_CONCURRENCY is ON to
**	    hide all the savepoints from the CDB
**      jun-01-89 (carl)
**	    overhauled to handle savepoints and their aborts
**      jun-22-89 (carl)
**	    fixed return inconsistencies discovered by lint in tpd_m6_commit_dx
**      oct-24-89 (carl)
**	    adapted from TPDTABLE.C and reorganized for 2PC
**      jun-23-90 (carl)
**	    changed tpd_m9_ldb_to_lxcb to set SLAVE 2PC protocol flag in 
**	    an LDB's descriptor
**      oct-11-90 (carl)
**	    process trace point
**      nov-20-92 (terjeb)
**          Included <tpfproto.h> in order to declare tpd_p6_prt_qrytxt
**          and tpd_p9_prt_ldbdesc.
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	10-oct-93 (swm)
**	    Bug #56448
**	    Altered tpf_u0_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to tpf_u0_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    tpf_u0_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

GLOBALREF   char    *IITP_00_tpf_p;/* "TPF" */

GLOBALREF   char    *IITP_06_updater_p;	/* "updater" */

GLOBALREF   char    *IITP_13_allowed_in_dx_p;	/* "allowed in DX" */


/*{
** Name: tpd_m1_set_dx_id - transaction id generator
**
** Description:  
**	A distributed transaction id is generated from the current time.
**
** Inputs:
**		v_sscb_p			facility session control block
** Outputs:
**		v_sscb_p->ses_txn.tpd_id.
**			tpf_txn1	set to server PID
**			tpf_txn2	set to (PTR)txnp
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      apr-25-88 (alexh)
**          created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


VOID
tpd_m1_set_dx_id(
	TPD_SS_CB	*v_sscb_p)
{
    TPD_DX_CB	*dxcb_p = & v_sscb_p->tss_dx_cb;
    SYSTIME	tm_now1,
		tm_now2;
    char	hi_ascii[DB_MAXNAME],
		lo_ascii[DB_MAXNAME],
		buff[DB_MAXNAME + DB_MAXNAME];


    TMnow(& tm_now1);
    STRUCT_ASSIGN_MACRO(tm_now1, tm_now2);
    
    while ( tm_now2.TM_secs == tm_now1.TM_secs
	    &&
	    tm_now2.TM_msecs == tm_now1.TM_msecs )
	TMnow(& tm_now2);
	
    dxcb_p->tdx_id.dd_dx_id1 = tm_now2.TM_secs;
    dxcb_p->tdx_id.dd_dx_id2 = tm_now2.TM_msecs;

    /* construct a name from tm_now */

    CVla(tm_now2.TM_secs, hi_ascii);
    CVla(tm_now2.TM_msecs, lo_ascii);
    STpolycat((i4) 3,			/* 3 constituent pieces: "dd_", 2
					** ascii representations of the
					** timestamp */
	"dd_", lo_ascii, hi_ascii, buff);
    MEmove(STlength(buff), buff, ' ', 
	DB_MAXNAME, dxcb_p->tdx_id.dd_dx_name);
}


/*{
** Name: tpd_m2_init_dx_cb	- initialize a transaction CB
**
** Description: Entry  state is initialized  to DX_0STATE_INIT. Entry
**	type is set to  DX_0QRY_NO_ACTION.  LDB count is  set to 0 and
**	transaction id is generated.
**
** Inputs:
**	v_tpr_p			TPR_CB
**	o1_sscb_p		facility session control block
**
** Outputs:
**	o1_sscb_p->
**		tss_dx_cb	initialized
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      11-mar-88 (alexh)
**          created
**	08-Nov-88 (alexh)
**	    added session language parameter and auto commit initialization
**	27-Dec-88 (alexh)
**	    initialize savepoint list - tpd_sp_list
*/


VOID
tpd_m2_init_dx_cb(
	TPR_CB		*v_tpr_p,
	TPD_SS_CB	*o1_sscb_p)
{
    TPD_DX_CB	*dxcb_p = & o1_sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p;


    dxcb_p->tdx_lang_type = v_tpr_p->tpr_lang_type;
    dxcb_p->tdx_state = DX_0STATE_INIT;
    dxcb_p->tdx_mode = DX_0MODE_NO_ACTION;
    dxcb_p->tdx_flags = DX_00FLAG_NIL;

    /* set transaction id */

    tpd_m1_set_dx_id(o1_sscb_p);

/*
    STRUCT_ASSIGN_MACRO(*v_tpr_p->tpr_site, dxcb_p->tdx_cdb);
*/
    dxcb_p->tdx_ldb_cnt = 0;
/*
    ** must initialize the ULM CB for the pre-registered LX CB list **

    lmcb_p = & dxcb_p->tdx_21_pre_lx_ulmcb;
    lmcb_p->tlm_1_streamid = NULL;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = 
	lmcb_p->tlm_4_last_ptr = (PTR) NULL;
*/
    /* must initialize the ULM CB for the registered LX CB list */

    lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    lmcb_p->tlm_1_streamid = NULL;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = 
	lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    /* must initialize the SP ULM CB */

    lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
    lmcb_p->tlm_1_streamid = NULL;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = 
	lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    dxcb_p->tdx_24_1pc_lxcb_p = (TPD_LX_CB *) NULL;
}


/*{
** Name: tpd_m3_term_dx_cb - terminate a DX CB
**
** Description: 
**	DX CB termination involves resetting the LX_01FLAG_REG bit in 
**	tlx_20_flags field in all LX CBs and closing the savepoint stream 
**	if any .
**
** Inputs:
**	v_tpr_p			TPR_CB
**	o1_sscb_p		facility session control block
**
** Outputs:
**	o1_sscb_p->
**		tss_dx_cb	reinitialized
**	Returns:
**	    None
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**	16-nov-89 (carl)
**	    creation
*/


DB_STATUS
tpd_m3_term_dx_cb(
	TPR_CB		*v_tpr_p,
	TPD_SS_CB	*o1_sscb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_DX_CB	*dxcb_p = & o1_sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = (TPD_LM_CB *) NULL;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    ULM_RCB	ulm;


    dxcb_p->tdx_state = DX_0STATE_INIT;
    dxcb_p->tdx_mode = DX_0MODE_NO_ACTION;
    dxcb_p->tdx_flags = DX_00FLAG_NIL;

    /* generate new transaction id */

    tpd_m1_set_dx_id(o1_sscb_p);

    dxcb_p->tdx_ldb_cnt = 0;
    dxcb_p->tdx_24_1pc_lxcb_p = (TPD_LX_CB  *) NULL;

    lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG;	/* de-register */
	lxcb_p->tlx_state = LX_0STATE_NIL;
	lxcb_p->tlx_write = FALSE;
	lxcb_p->tlx_sp_cnt = 0;

	lxcb_p = lxcb_p->tlx_22_next_p;	/* advance to next LX CB */
    }

    /* close stream for session savepoints if any */

    lmcb_p = & dxcb_p->tdx_23_sp_ulmcb;
    if (lmcb_p->tlm_1_streamid != NULL)
    {
        ulm.ulm_facility = DB_QEF_ID;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
	ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;

	status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	/* ULM has nullified tlm_1_streamid */

        /* must reinitialize the SP ULM CB */
    
	lmcb_p->tlm_2_cnt = 0;
	lmcb_p->tlm_3_frst_ptr = 
	    lmcb_p->tlm_4_last_ptr = (PTR) NULL;
    }

    return(E_DB_OK);
}


/*{
** Name: tpd_m4_ok_ldb_for_dx - decide if the updating LDB fits in the DX
**
** Description: 
**	This routine computes if the acceptance of the updating LDB is ok
**	for the DX in terms of 1PC/2PC compatibility.
**
** Inputs: 
**	i1_cur_dxmode			current DX mode
**	i2_lxcb_p                       ptr to LX CB
**	i3_lxcb_1pc_p			ptr to the 1PC LX CB, NULL if none
**	v_rcbp_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    
** Outputs:
**	o1_ok_p->			ptr to returned boolean
**	o1_new_dxmode_p->		ptr to returned new mode
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    none
**
** History:
**      nov-09-89 (carl)
**          created
**	1-oct-98 (stephenb)
**	    Update code logic to support concept of 1PC Plus.
*/



DB_STATUS
tpd_m4_ok_ldb_for_dx(
	i4	    i1_cur_dxmode,
	TPD_LX_CB   *i2_lxcb_p,
	TPD_LX_CB   *i3_1pc_lxcb_p,
	TPR_CB	    *v_tpr_p,
	bool	    *o1_ok_p,
	i4	    *o2_new_dxmode_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB* ) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    DD_LDB_DESC	*cdb_p = & dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;


    *o1_ok_p = FALSE;
    *o2_new_dxmode_p = DX_0MODE_NO_ACTION;

    switch (i1_cur_dxmode)
    {
    case DX_0MODE_NO_ACTION:
    case DX_1MODE_READ_ONLY:
	if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	    *o2_new_dxmode_p = DX_4MODE_SOFT_1PC;
	else
	    *o2_new_dxmode_p = DX_3MODE_HARD_1PC;
	*o1_ok_p = TRUE;
	break;
    case DX_2MODE_2PC:
	if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	{
	    /* compatible */

	    *o2_new_dxmode_p = i1_cur_dxmode;
	    *o1_ok_p = TRUE;
	}
	/* else demote to 1PC+ */
	else
	{
	    *o2_new_dxmode_p = DX_5MODE_1PC_PLUS;
	    *o1_ok_p = TRUE;
	}
	break;
    case DX_3MODE_HARD_1PC:
	if (i2_lxcb_p == i3_1pc_lxcb_p)
	{
	    /* same LDB, ok */

	    *o2_new_dxmode_p = i1_cur_dxmode;
	    *o1_ok_p = TRUE;
	}
	else if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	{
	    /* go to 1PC+ */
	    *o2_new_dxmode_p = DX_5MODE_1PC_PLUS;
	    *o1_ok_p = TRUE;
	}
	/* else fail */
	break;
    case DX_4MODE_SOFT_1PC:
	if (i2_lxcb_p == i3_1pc_lxcb_p)
	{
	    /* same LDB, ok */

	    *o2_new_dxmode_p = i1_cur_dxmode;
	    *o1_ok_p = TRUE;
	}
	else if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	{
	    /* another 2PC LDB, go to 2PC */
	    *o2_new_dxmode_p = DX_2MODE_2PC;
	    *o1_ok_p = TRUE;
	}
	/* else go to 1PC Plus */
	else
	{
	    *o2_new_dxmode_p = DX_5MODE_1PC_PLUS;
	    *o1_ok_p = TRUE;
	}
	break;

    case DX_5MODE_1PC_PLUS:
	if (i2_lxcb_p == i3_1pc_lxcb_p)
	{
	    /* same LDB OK */
	    *o2_new_dxmode_p = i1_cur_dxmode;
	    *o1_ok_p = TRUE;
	}
	if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	{
	    /* 2PC compatible */

	    *o2_new_dxmode_p = i1_cur_dxmode;    /* leave in 1PC+ */
	    *o1_ok_p = TRUE;
	}
	/* else not ok */

	break;
    default:
	return( tpd_u2_err_internal(v_tpr_p) );	
    }

    return(E_DB_OK);
}


/*{
** Name: tpd_m5_ok_write_ldbs - validate updating LDBs for the DX
**
** Description: 
**	This routine validates updating LDBs by pre-computing the 1PC/2PC
**	mode transition that would be caused by the execution of each subquery
**	on such an LDB.  
**
**	A traversal of the LDB list is performed to compute the mock mode 
**	that would cause the DX to enter by the subquery's execution at
**	the LDB.
**
**	Assumption: The LDB list is not unique in that the same LDB ptr
**	may appear multiple times.
**
**	If the executing LDB is joining the DX for the first time, allocate 
**	an LX CB in the LX list and execute a canned query to read its 2PC 
**	capability into this CB.
**
**	Transition rules for the current LDB in the list:
**
**	0.  Case where the DX is in initial or read-only mode
**
**	    set the mock mode of the DX to:
**
**		o soft 1PC if the LDB is 2PC-capable
**		o hard 1PC if the LDB is only 1PC-capable
**
**	1.  Case where the DX is in 2PC
**
**	    if the joining LDB is only 1PC-capable, demote to 1PC+
**	    else continue with the next LDB in the list
**	    
**	2.  Case where the DX is in 1PC
**
**	    If the LDB is the owner of the 1PC DX, continue with the list
**	    else (the 1PC+ rule for a mix of updating LDBs)
**	    {	
**		if the owner LDB does not support 2PC
**		{
**		    if the non-owner does
**		        go to 1PC Plus mode
**		    if the non-oner also does not support 2PC
**			return not OK
**		}
**		else (the owner LDB does support 2PC)
**		{
**		    if the non-owner LDB does not support 2PC,
**			go to 1PC Plus mode
**		    else (both the 1PC owner and non-owner LDB support 2PC)
**		    {
**			go to 2PC mode
**		    }
**		}
**	    }
**
**	 3. Case where the DX is in 1PC Plus
**
**	    if the LDB is the owner of the 1PC+ DX, continue with the list
**	    else (the 1PC+ rule for a mix of updating LDBs)
**	    {
**		if the joining LDB does not support 2PC
**		    Return Not OK
**		else (the joining LDB does support 2PC)
**		    remain in 1PC+ mode
**	    }
**
**	    return OK in tpr_14_w_ldbs_ok if at end of list.
**
** Inputs: 
**	tpr_cb->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_15_w_ldb_p		ptr to first of list of LDB CBs
** Outputs:
**	tpr_cb->
**	    tpr_14_w_ldbs_ok		TRUE if LDBs are allowed to proceed
**					with their write subqueries, FALSE
**					otherwise
**	    tpf_error			tpf error info
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    none
**
** History:
**      nov-07-89 (carl)
**          created
**      oct-11-90 (carl)
**	    process trace point
**	12-aug-93 (swm)
**	    Specify %p print option when printing the sscb_p pointer and
**	    elminate truncating cast to nat.
**	1-oct-98 (stephenb)
**	    Update code logic to support concept of 1PC Plus.
*/


DB_STATUS
tpd_m5_ok_write_ldbs(
	TPR_CB	    *v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB* ) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    i4		mock_mode = dxcb_p->tdx_mode,
		new_mode;
    TPR_W_LDB	*w_ldb_p = v_tpr_p->tpr_15_w_ldb_p;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL,
		*lxcb_1pc_p = dxcb_p->tdx_24_1pc_lxcb_p;
    ULM_RCB	ulm,
		*ulm_p = & ulm;
    bool	ok = FALSE,
		new_lxcb_b = FALSE,
                trace_err_104 = FALSE,
		trace_ldbs_105 = FALSE;
    i4     i4_1,
                i4_2;
    char	*cbuf = v_tpr_p->tpr_trfmt;
    i4		cbufsize = v_tpr_p->tpr_trsize;


    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_ERROR_104,
            & i4_1, & i4_2))
    {
        trace_err_104 = TRUE;
    }

    if (ult_check_macro(& sscb_p->tss_3_trace_vector, TSS_TRACE_LDBS_105,
            & i4_1, & i4_2))
    {
        trace_ldbs_105 = TRUE;
    }

    v_tpr_p->tpr_14_w_ldbs_ok = TRUE;	/* assume ok */

    while (w_ldb_p != (TPR_W_LDB *) NULL)
    {
	/* match the LDB with an LX CB */

	status = tpd_m9_ldb_to_lxcb(w_ldb_p->tpr_3_ldb_p, v_tpr_p, & lxcb_p,
		    & new_lxcb_b);
	if (status)
	    return(status);

	/* have an LX CB */

	if (trace_ldbs_105)
	{
	    STprintf(cbuf, "%s %p: %s %s:\n",
		IITP_00_tpf_p,
		(PTR) sscb_p,
		IITP_06_updater_p,
		IITP_13_allowed_in_dx_p);
	    tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

	    tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
	}
	    
	status = tpd_m4_ok_ldb_for_dx(mock_mode, lxcb_p, lxcb_1pc_p, v_tpr_p, 
		    & ok, & new_mode);
	if (status)
	{
	    if (! trace_ldbs_105 && trace_err_104)
	    {
		STprintf(cbuf, "%s %p: %s NOT %s:\n",
		    IITP_00_tpf_p,
		    (PTR) sscb_p,
		    IITP_06_updater_p,
		    IITP_13_allowed_in_dx_p);
		tpf_u0_tprintf(v_tpr_p, cbufsize, cbuf);

		tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_0TO_FE);
	    }
	    return(status);
	}
	if (! ok)
	{
	    v_tpr_p->tpr_14_w_ldbs_ok = FALSE;
	    return(E_DB_OK);
	}

        switch (mock_mode)
	{
	case DX_0MODE_NO_ACTION:
	case DX_1MODE_READ_ONLY:
	    if (new_mode == DX_4MODE_SOFT_1PC 
		|| 
		new_mode == DX_3MODE_HARD_1PC)
	    lxcb_1pc_p = lxcb_p;	/* remember this 1PC LDB */
	    break;
	case DX_5MODE_1PC_PLUS:
	case DX_3MODE_HARD_1PC:
	    /* no action, 1PC LDB is already saved */
	    break;
	default:
	    if (new_mode == DX_5MODE_1PC_PLUS)
		/* went to 1PC+, save 1PC LDB */
		lxcb_1pc_p = lxcb_p;
	    break;
	}
	mock_mode = new_mode;		/* save mock mode for next iteration */

	w_ldb_p = (TPR_W_LDB *)w_ldb_p->tpr_2_next_p;	/* advance to next LDB */
    }

    /* ok if here */

    return(E_DB_OK);
}


/*{
** Name: tpd_m6_register_ldb - register an LDB with a DX
** 
** Description: 
**	Register the fact that the typed action on the specified LDB has 
**	occurred.  Proper state transition and action are made.  
**
** Inputs:
**	i1_update_b		TRUE if an update subquery for the LDB
**	v_tpr_p			TPF_CB
** Outputs:
**	v_tpr_p->
**	    session
**		.tss_dx_cb
**		    tdx_state	may change to DX_3STATE_BEGIN
**		    tdx_mode	DX_1MODE_READ_ONLY, DX_3MODE_HARD_1PC or 
**				DX_3_MODE_N_WRITE
**		    tdx_ldb_cnt	may increment
**		    tdx_22_lx_ulmcb
**				new LX CB linked in if not already present
**	Returns:
**	    DB_STATUS
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-05-89 (carl)
**          created
**	1-oct-98 (stephenb)
**	    Update code logic to support concept of 1PC plus.
**	04-Oct-2001 (jenjo02)
**	    1993 change 269144 for BUG 49222 and 1PC plus changes
**	    cause first updating DML after rollback-to-savepoint to fail
**	    with E_TP0020_INTERNAL.
*/


DB_STATUS
tpd_m6_register_ldb(
	bool	i1_update_b,
	TPR_CB	*v_tpr_p)
{
    DB_ERRTYPE	tpf_err;
    DB_STATUS	status = E_DB_OK;   /* assume */
    DD_LDB_DESC	*ldb_p = v_tpr_p->tpr_site;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB* ) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LX_CB	*lxcb_p = NULL;
    i4		lxmode,
		dxmode;
    bool	new_lxcb_b = FALSE,
		mode_ok = FALSE;
    RQR_CB	rqr_cb;
		

    status = tpd_m9_ldb_to_lxcb(ldb_p, v_tpr_p, & lxcb_p, & new_lxcb_b);
    if (status)
	return(status);

    if (lxcb_p == (TPD_LX_CB *) NULL)
	return( tpd_u2_err_internal(v_tpr_p) );   /* internal error */

    if (new_lxcb_b)
    {
        lxcb_p->tlx_txn_type = v_tpr_p->tpr_lang_type;
	lxcb_p->tlx_state = LX_7STATE_ACTIVE;   /* consider the LX active */
	lxcb_p->tlx_write = i1_update_b;
	lxcb_p->tlx_sp_cnt = 0;
	STRUCT_ASSIGN_MACRO(*ldb_p, lxcb_p->tlx_site);
	lxcb_p->tlx_write = i1_update_b;
    
	if (v_tpr_p->tpr_lang_type == DB_QUEL)
	{
	    /* send begin transaction to this new joining LDB */

	    rqr_cb.rqr_session = sscb_p->tss_rqf;
	    rqr_cb.rqr_1_site = ldb_p;
	    rqr_cb.rqr_q_language = v_tpr_p->tpr_lang_type;
	    rqr_cb.rqr_timeout = 0;
	    status = tpd_u4_rqf_call(RQR_BEGIN, & rqr_cb, v_tpr_p);
	    if (status)
		return(status);
	}
    }
    else
    {
	if (i1_update_b)
	    lxcb_p->tlx_write = i1_update_b;
    }

    /* give all the outstanding savepoints to the LDB before returning */

    if (dxcb_p->tdx_23_sp_ulmcb.tlm_2_cnt > 0)
    {
	status = tpd_s3_send_sps_to_ldb(v_tpr_p, lxcb_p);
	if (status)
	    return(status);
    }

    /* make a DX mode transition */

    status = tpd_m8_new_dx_mode(i1_update_b, lxcb_p, v_tpr_p, & dxmode, 
		& mode_ok);
    if (status)
	return(status);

    if (! mode_ok)
    {
	/* an internal error because the mode should have been pre-computed 
	** to be ok */

	return( tpd_u2_err_internal(v_tpr_p) );
    }

    /*
    ** Note: We now use tdx_24_1pc_lxcb_p to hold the LXCB of the 1PC capable
    ** database when we go into 1PC Plus mode. This pointer used to be NULL
    ** unles we were in soft or hard 1PC mode, when it held the LXCB of the
    ** original 1PC database.
    */
    switch (dxcb_p->tdx_mode)
    {
    case DX_0MODE_NO_ACTION:
    case DX_1MODE_READ_ONLY:
	
	/* make transition to 1PC */

	/*
	** After rollback-to-savepoint, tdx_mode == READ_ONLY
	** and LXCB remains the same, not an error.
	*/
	if ( dxcb_p->tdx_24_1pc_lxcb_p != lxcb_p &&
	       (dxmode == DX_3MODE_HARD_1PC
		||
		dxmode == DX_4MODE_SOFT_1PC) )
	{

	    if (dxcb_p->tdx_24_1pc_lxcb_p != (TPD_LX_CB *) NULL)
		return( tpd_u2_err_internal(v_tpr_p) );

	    dxcb_p->tdx_24_1pc_lxcb_p = lxcb_p;
	}
	break;

    case DX_5MODE_1PC_PLUS:
    case DX_3MODE_HARD_1PC:
	
	/* no action, LXCB already saved */
	break;

    default:
	if (dxmode == DX_5MODE_1PC_PLUS)
	    dxcb_p->tdx_24_1pc_lxcb_p = lxcb_p;
	break;
    }

    dxcb_p->tdx_mode = dxmode;

    if (i1_update_b)
	lxcb_p->tlx_write = i1_update_b;    /* LX in update mode */

    if (! (lxcb_p->tlx_20_flags & LX_01FLAG_REG))
    {
	lxcb_p->tlx_20_flags |= LX_01FLAG_REG;  /* register with DX */
	lxcb_p->tlx_state = LX_7STATE_ACTIVE;   /* set active state */
	++dxcb_p->tdx_ldb_cnt;
    }
    return(E_DB_OK);
}

/*{
** Name: tpd_m7_srch_ldb - search an LDB's LX CB in the (pre-)registered
**			   list
** Description: 
**	Search for an LDB's LX CB in either the pre- or registered list.
**
** Inputs:
**	i1_which	    TPD_0LM_LX_CB
**	i2_ldb_p	    ptr to LDB for searching
**	v_tpr_p		    ptr to TPR_CB
**
** Outputs:
**	o1_lxcb_pp          ptr to ptr to TPD_LX_CB
**			    If the LDB is not found NULL is returned, else
**			    the LDB's LX CB pointer is returned.
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**	nov-04-89 (carl)
**          created
*/


DB_STATUS
tpd_m7_srch_ldb(
	i4		i1_which,
	DD_LDB_DESC	*i2_ldb_p,
	TPR_CB		*v_tpr_p,
	TPD_LX_CB	**o1_lxcb_pp)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LX_CB	*lxcb_p = NULL;
    TPD_LM_CB	*lmcb_p = NULL;

    
    *o1_lxcb_pp = (TPD_LX_CB *) NULL;
/*
    if (dxcb_p->tdx_ldb_cnt <= 0)
	return(E_DB_OK);		* empty lists *
*/
    switch(i1_which)
    {
    case TPD_0LM_LX_CB:
	lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
	break;
    default:
	return( tpd_u2_err_internal(v_tpr_p) );
    }
    if (lmcb_p->tlm_3_frst_ptr == NULL)
        return(E_DB_OK);
    
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;	    
		
    /* full matching required */

    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	/* nested ifs are used for easy debugging. It is not required. */

	if (lxcb_p->tlx_site.dd_l1_ingres_b == i2_ldb_p->dd_l1_ingres_b)
	    if (MEcmp(i2_ldb_p->dd_l2_node_name, 
		lxcb_p->tlx_site.dd_l2_node_name,
		sizeof(i2_ldb_p->dd_l2_node_name)) == 0)
		if (MEcmp(i2_ldb_p->dd_l3_ldb_name, 
		    lxcb_p->tlx_site.dd_l3_ldb_name,
		    sizeof(i2_ldb_p->dd_l3_ldb_name)) == 0)
		    if (MEcmp(i2_ldb_p->dd_l4_dbms_name, 
			lxcb_p->tlx_site.dd_l4_dbms_name,
			sizeof(i2_ldb_p->dd_l4_dbms_name)) == 0)
			{
			    /* have a match */

			    *o1_lxcb_pp = lxcb_p;
			    return(E_DB_OK);
			}
	lxcb_p = lxcb_p->tlx_22_next_p;	/* advance to next LX CB */
    }
    return(E_DB_OK);
}


/*{
** Name: tpd_m8_new_dx_mode - determine DX's mode after transition 
** 
** Description: 
**	Given the current DX mode, determine the resultant state by
**	including the currently joining LDB and its subquery mode.
**
**	It is assumed that the transition has been validated and no
**	checking needs to be done.
**
** Inputs:
**	i1_update_b		TRUE if an update subquery for the LDB
**	i2_lxcb_p               ptr to the LX CB for the LDB
**	v_tpr_p			TPF_CB
** Outputs:
**	o1_newmode_p->		ptr to new mode
**	o2_ok_p->		ptr to boolean, TRUE if OK, FALSE otherwise
**
**	Returns:
**	    DB_STATUS
**
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-05-89 (carl)
**          created
**	1-oct-98 (stephenb)
**	    Update code logic to support concept of 1PC plus
**      13-jan-2004 (huazh01)
**          If we are in DX_5MODE_1PC_PLUS state, and the 
**          next update is still on the same site that does
**          not support 2PC protocols, then remains on 
**          DX_5MODE_1PC_PLUS state. 
**          This fixes b111441, INGSTR62.
*/


DB_STATUS
tpd_m8_new_dx_mode(
	bool	    i1_update_b,
	TPD_LX_CB   *i2_lxcb_p,
	TPR_CB	    *v_tpr_p,
	i4	    *o1_newmode_p,
	bool	    *o2_ok_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB* ) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    DD_LDB_DESC	*cdb_p = & dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
    TPD_LM_CB   *lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB   *lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
		

    *o2_ok_p = TRUE;	    /* assume */

    if (! i1_update_b)	    
    {
	if (dxcb_p->tdx_mode == DX_0MODE_NO_ACTION)
	    *o1_newmode_p = DX_1MODE_READ_ONLY;
	else
	    *o1_newmode_p = dxcb_p->tdx_mode;   /* mode does not change */
	return(E_DB_OK);
    }

    /* fall thru for update subquery mode */

    switch (dxcb_p->tdx_mode)
    {
    case DX_0MODE_NO_ACTION:
    case DX_1MODE_READ_ONLY:

	if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	    *o1_newmode_p = DX_4MODE_SOFT_1PC;
	else
	    *o1_newmode_p = DX_3MODE_HARD_1PC;

	break;

    case DX_2MODE_2PC:

	if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	    *o1_newmode_p = DX_2MODE_2PC;   /* alerady in 2PC mode */
	else
	    /* joining database is not 2PC capable, demote to 1PC+ */
	    *o1_newmode_p = DX_5MODE_1PC_PLUS;
	break;

    case DX_3MODE_HARD_1PC:

	if (dxcb_p->tdx_24_1pc_lxcb_p == (TPD_LX_CB *) NULL)
	    return( tpd_u2_err_internal(v_tpr_p) );

	if (i2_lxcb_p == dxcb_p->tdx_24_1pc_lxcb_p)
	    *o1_newmode_p = DX_3MODE_HARD_1PC; 
					    /* the same LDB, same hard mode */
	else if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	    /* joing LDB is 2PC capable, switch to 1PC+ */
	    *o1_newmode_p = DX_5MODE_1PC_PLUS;
	else
	{
	    *o2_ok_p = FALSE;		    /* cannot switch DX's mode from 
					    ** hard 1PC */
	    return(E_DB_OK);		    /* NOT considered an error */
	}
	break;

    case DX_4MODE_SOFT_1PC:

	if (dxcb_p->tdx_24_1pc_lxcb_p == (TPD_LX_CB *) NULL)
	    return( tpd_u2_err_internal(v_tpr_p) );

	if (i2_lxcb_p == dxcb_p->tdx_24_1pc_lxcb_p)
	    *o1_newmode_p = DX_4MODE_SOFT_1PC; 
					    /* the same LDB, same soft mode */
	else
	{
	    if (cdb_p->dd_l6_flags & DD_02FLAG_2PC)
	    {
		/* 2PC catalogs exist in CDB, hence 2PC or 1PC+ possible */
		if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
		    /* go to 2PC */
		    *o1_newmode_p = DX_2MODE_2PC;
		else
		    /* go to 1PC+ */
		    *o1_newmode_p = DX_5MODE_1PC_PLUS;
	    }
	    else
		return( tpd_u2_err_internal(v_tpr_p) );	
	}
	break;

    case DX_5MODE_1PC_PLUS:

	if (dxcb_p->tdx_24_1pc_lxcb_p == (TPD_LX_CB *) NULL)
	    return( tpd_u2_err_internal(v_tpr_p) );

	if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	{
	    /* stay in 1PC+ */
	    *o1_newmode_p = DX_5MODE_1PC_PLUS;
	}
	else
	{

           /* b111441: check if there is more than one site that
           ** does not support 2PC, if there is just one site and 
           ** if it is the same as i2_lxcb_p, then stay at 1PC+
           */
           while (lxcb_p != (TPD_LX_CB *)NULL)
           {
               if (lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
               {
                   lxcb_p = lxcb_p->tlx_22_next_p;
                   continue; 
               }

               if ((lxcb_p->tlx_site.dd_l1_ingres_b != 
                   i2_lxcb_p->tlx_site.dd_l1_ingres_b)               ||
                   (MEcmp(i2_lxcb_p->tlx_site.dd_l2_node_name,
                       lxcb_p->tlx_site.dd_l2_node_name,
                       sizeof(i2_lxcb_p->tlx_site.dd_l2_node_name))) ||
                   (MEcmp(i2_lxcb_p->tlx_site.dd_l3_ldb_name,
                       lxcb_p->tlx_site.dd_l3_ldb_name,
                       sizeof(i2_lxcb_p->tlx_site.dd_l3_ldb_name)))  ||
                   (MEcmp(i2_lxcb_p->tlx_site.dd_l4_dbms_name,
                       lxcb_p->tlx_site.dd_l4_dbms_name,
                       sizeof(i2_lxcb_p->tlx_site.dd_l4_dbms_name))))
               {
                   /* find another site that does not support 2PC. 
                   ** cannot switch DX's mode
                   */
                   *o2_ok_p = FALSE;
                   return (E_DB_OK);         /* NOT considered an error */
               } 

               lxcb_p = lxcb_p->tlx_22_next_p;  
           }
           *o1_newmode_p = DX_5MODE_1PC_PLUS;      /* stay in 1PC+ */
	}
	break;

    default:

	return( tpd_u1_error(E_TP0005_UNKNOWN_STATE, v_tpr_p) );

    }
    /* ok if here */

    return(E_DB_OK);
}


/*{
** Name: tpd_m9_ldb_to_lxcb - return an LX CB that matches the LDB 
**
** Description: 
**	This routine searches both the registered list to find an existing 
**	LX CB and failing to find one generates one in the LX list.
**
** Inputs: 
**	i1_ldb_p			ptr to LDB descriptor
**	v_rcbp_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    
** Outputs:
**	o1_lxcb_pp->			LX CB ptr
**	o2_new_cb_p->			TRUE if CB is new, FALSE otherwise
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    none
**
** History:
**      nov-09-89 (carl)
**          created
**      jun-23-90 (carl)
**	    set SLAVE 2PC protocol flag in LDB's descriptor
*/



DB_STATUS
tpd_m9_ldb_to_lxcb(
	DD_LDB_DESC *i1_ldb_p,
	TPR_CB	    *v_tpr_p,
	TPD_LX_CB   **o1_lxcb_pp,
	bool	    *o2_new_cb_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB* ) v_tpr_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    TPC_L1_DBCAPS
		dbcaps,
		*dbcaps_p = & dbcaps;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;
    i4		result;
    bool	commit_b = TRUE;


    *o1_lxcb_pp = (TPD_LX_CB *) NULL;
    *o2_new_cb_p = FALSE;		    /* assume */

    /* look up the LDB in the registered list */

    status = tpd_m7_srch_ldb(TPD_0LM_LX_CB, i1_ldb_p, v_tpr_p, & lxcb_p);
    if (status)
	return(status);

    if (lxcb_p != (TPD_LX_CB *) NULL)
    {
	*o1_lxcb_pp = lxcb_p;
	return(E_DB_OK);
    }

    /* not in the list, allocate and fill a new LX CB for caller */

    *o2_new_cb_p = TRUE;
    status = tpd_u6_new_cb(TPD_0LM_LX_CB, v_tpr_p, (PTR *) & lxcb_p);
    if (status)
	return(status);

    STRUCT_ASSIGN_MACRO(*i1_ldb_p, lxcb_p->tlx_site);	/* must fill */

    if (i1_ldb_p->dd_l6_flags & DD_03FLAG_SLAVE2PC)
	lxcb_p->tlx_20_flags |= LX_02FLAG_2PC;		/* must set */
    else
    {
	/* retrieve the LDB's 2PC capability */

	qcb_p->qcb_1_can_id = SEL_40_DBCAPS_BY_2PC;
	qcb_p->qcb_3_ptr_u.u4_dbcaps_p = & dbcaps;
	qcb_p->qcb_4_ldb_p = i1_ldb_p;
	qcb_p->qcb_5_eod_b = FALSE; /* assume */
	qcb_p->qcb_6_col_cnt = TPC_03_CC_DBCAPS_2;
	qcb_p->qcb_7_qry_p = (char *) NULL;
	qcb_p->qcb_8_dv_cnt = 0;    /* assume */
	qcb_p->qcb_10_total_cnt = 0;
	qcb_p->qcb_12_select_cnt = 0;

	status = tpq_s4_prepare(v_tpr_p, qcb_p);
	if (status)
	{
	    tpd_u7_term_any_cb(TPD_0LM_LX_CB, (PTR *) & lxcb_p, v_tpr_p);
					/* remove the CB from the list */
	    return(status);
	}

	if (qcb_p->qcb_5_eod_b)
	{
	    /* assume LDB supports 2PC */

	    lxcb_p->tlx_20_flags |= LX_02FLAG_2PC;
	}
	else
	{
	    /* read and analyze */

	    result = MEcmp(dbcaps_p->l1_2_cap_val, "Y", DB_MAXNAME);
	    if (result == 0)
		lxcb_p->tlx_20_flags = LX_02FLAG_2PC;		    
	    else
		lxcb_p->tlx_20_flags = LX_00FLAG_NIL;		    

	    status = tpq_s3_flush(v_tpr_p, qcb_p);
	    if (status)
	    {
		tpd_u7_term_any_cb(TPD_0LM_LX_CB, (PTR *) & lxcb_p, v_tpr_p);
					/* remove the CB from the list */
		return(status);
	    }
	}
	/* commit this query */

	status = tpf_u3_msg_to_ldb(i1_ldb_p, commit_b, v_tpr_p);
	if (status)
	{
	    tpd_u7_term_any_cb(TPD_0LM_LX_CB, (PTR *) & lxcb_p, v_tpr_p);
					/* remove the CB from the list */
	    return(status);
	}
    }
    *o1_lxcb_pp = lxcb_p;

    return(E_DB_OK);
}


/*{
** Name: tpd_m10_is_ddb_open - return TRUE if a DDB in question is open 
**
** Description: 
**	Scan the search CDB list to determine if a DDB is being recovered.
**
** Inputs:
**	i1_ddb_p		    ptr to DDB descriptor for searching
**	v_tpr_p			    ptr to TPR_CB
**
** Outputs:
**	o1_ddb_open_p		    ptr to boolean
**				    TRUE if DDB is NOT under recovery,
**				    FALSE otherwise
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**	jan-28-90 (carl)
**          created
*/


DB_STATUS
tpd_m10_is_ddb_open(
	DD_DDB_DESC	*i1_ddb_p,
	TPR_CB		*v_tpr_p,
	bool		*o1_ddb_open_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    DD_LDB_DESC	*cdb_p = & i1_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    TPD_LM_CB	*srchcb_p = & Tpf_svcb_p->tsv_12_srch_ulmcb;
    TPD_CD_CB	*cdcb_p = (TPD_CD_CB *) NULL;

    
    *o1_ddb_open_p = TRUE;		/* assume NOT in recovery */
    
    tpf_u1_p_sema4(TPD_0SEM_SHARED, & Tpf_svcb_p->tsv_11_srch_sema4);	
					/* get semaphore, a MUST */

    cdcb_p = (TPD_CD_CB *) srchcb_p->tlm_3_frst_ptr;	    
		
    while (cdcb_p != (TPD_CD_CB *) NULL)
    {
	if (MEcmp(i1_ddb_p->dd_d1_ddb_name,
	    cdcb_p->tcd_1_starcdbs.i1_1_ddb_name,
	    DB_MAXNAME) == 0)
	{
	    /* have a match */

	    if (cdcb_p->tcd_2_flags & TPD_1CD_TO_RECOVER)
	    {
		if (! (cdcb_p->tcd_2_flags & TPD_2CD_RECOVERED))
		    *o1_ddb_open_p = FALSE; /* under recovery */
	    }
	    /* MUST give up the semaphore before returning */

	    tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);	/* a MUST */

	    return(E_DB_OK);
	}
	cdcb_p = cdcb_p->tcd_4_next_p;	/* advance to next CDB */
    }
    tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);	/* a MUST */

    return(E_DB_OK);
}


/*{
** Name: tpd_m11_is_ddb_open - return TRUE if a DDB in question is open 
**
** Description: 
**	Scan the search CDB list to determine if a DDB is being recovered.
**
** Inputs:
**	i1_ddb_p		    ptr to DDB descriptor for searching
**	v_tpr_p			    ptr to TPR_CB
**
** Outputs:
**	o1_ddb_open_p		    ptr to boolean
**				    TRUE if DDB is NOT under recovery,
**				    FALSE otherwise
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**	jan-28-90 (carl)
**          created
*/


DB_STATUS
tpd_m11_is_ddb_open(
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_DX_CB   *dxcb_p = (TPD_DX_CB *) & sscb_p->tss_dx_cb;
    DD_DDB_DESC	*ddb_p = & dxcb_p->tdx_ddb_desc;
    DD_LDB_DESC	*cdb_p = & dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
    TPD_LM_CB	*srchcb_p = & Tpf_svcb_p->tsv_12_srch_ulmcb;
    TPD_CD_CB	*cdcb_p = (TPD_CD_CB *) NULL;
    

    v_tpr_p->tpr_17_ddb_open = TRUE;	/* assume NOT in recovery */

    tpf_u1_p_sema4(TPD_0SEM_SHARED, & Tpf_svcb_p->tsv_11_srch_sema4);	
					/* get semaphore, a MUST */

    cdcb_p = (TPD_CD_CB *) srchcb_p->tlm_3_frst_ptr;	    
		
    while (cdcb_p != (TPD_CD_CB *) NULL)
    {
	if (MEcmp(ddb_p->dd_d1_ddb_name,
	    cdcb_p->tcd_1_starcdbs.i1_1_ddb_name,
	    DB_MAXNAME) == 0)
	{
	    /* have a match */

	    if (cdcb_p->tcd_2_flags & TPD_1CD_TO_RECOVER)
	    {
		if (! (cdcb_p->tcd_2_flags & TPD_2CD_RECOVERED))
		    v_tpr_p->tpr_17_ddb_open = FALSE; /* under recovery */
	    }
	    /* MUST give up the semaphore before returning */

	    tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);	/* a MUST */

	    return(E_DB_OK);
	}
	cdcb_p = cdcb_p->tcd_4_next_p;	/* advance to next CDB */
    }
    tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);	/* a MUST */

    return(E_DB_OK);
}

