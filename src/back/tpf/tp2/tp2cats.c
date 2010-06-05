/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <st.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <tm.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpfqry.h>
#include    <tpfddb.h>
#include    <tpferr.h>
#include    <tpfproto.h>

/**
**
** Name: TP2CATS.C - TPF's 2PC catalog routines
** 
** Description: 
**	This file contains all the 2PC catalog routines.
** 
**	    tp2_c0_log_lx	- log the LX of a DX
**	    tp2_c1_log_dx	- log the DX and its constituent LXs
**	    tp2_c2_log_dx_state	- log the current DX state during recovery
**	    tp2_c3_delete_dx	- delete a DX from iidd_ddb_dxlog
**	    tp2_c4_loop_to_log_dx_state	
**				- loop to log the current DX state until
**				  successful
**	    tp2_c5_loop_to delete_dx	
**				- loop to delete a DX until successful
**
**  History:    $Log-for RCS$
**      nov-19-89 (carl)
**          created
**      07-jul-90 (carl)
**	    changed tp2_c0_log_lx to set up new fields for TPC_D2_DXLDBS
**      13-oct-90 (carl)
**	    modified header of logged messages
**      28-oct-90 (carl)
**	    updated to use new and more meaningful defines for 
**	    TPC_D1_DXLOG.d1_5_dx_state to avoid confusion errors
**	01-may-91 (fpang)
**	    Reversed order of delete's in tp2_c3_delete_dx().
**	    If we do delete's in the same order as inserts, we
**	    will deadlock if two sessions are both using the cdb
**	    to log 2pc. Reversing the order will cause one to block
**	    until the other is finished. May slow things down a little,
**	    but should not have a major impact.
**	    Fixes B37191.
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**      06-apr-1993 (smc)
**          Removed double ';' to clear compiler noise.
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

GLOBALREF   char                *IITP_02_commit_p;

GLOBALREF   char                *IITP_03_abort_p;

GLOBALREF   char                *IITP_20_2pc_recovery_p;


/*{
** Name: tp2_c0_log_lx - log the LX of a DX 
** 
**  Description:    
**	Log an LX.  Will not commit the insert.
**
**	Failure policy: return upon error to fail DX.
**
** Inputs:
**	i1_dxcb_p			ptr to the DX CB in concern
**	i2_state			DX state to log to iidd_ddb_dxlog
**	v_rcb_p				ptr to the session CB
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
**      07-jul-90 (carl)
**	    changed to set up new fields for TPC_D2_DXLDBS
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
tp2_c0_log_lx(
	TPD_DX_CB   *i1_dxcb_p,
	TPD_LX_CB   *i2_lxcb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    DD_LDB_DESC	*cdb_p = 
		    & i1_dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
    TPC_D2_DXLDBS
		dxldbs,
		*dxldbs_p = & dxldbs;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;


    if (! (cdb_p->dd_l6_flags & DD_02FLAG_2PC) )	
				/* make sure the 2PC association is used */
	return( tpd_u2_err_internal(v_rcb_p) );

    MEfill(sizeof(dxldbs), '\0', (PTR) & dxldbs);

    if (! (i2_lxcb_p->tlx_20_flags & LX_01FLAG_REG))   /* must be registered */
	return( tpd_u2_err_internal(v_rcb_p) );

    if (! i2_lxcb_p->tlx_write)	/* must be a write site */
	return( tpd_u2_err_internal(v_rcb_p) );

    /* fill DXLDBS data */

    dxldbs_p->d2_1_ldb_dxid1 = i1_dxcb_p->tdx_id.dd_dx_id1;
    dxldbs_p->d2_2_ldb_dxid2 = i1_dxcb_p->tdx_id.dd_dx_id2;

    STmove(i2_lxcb_p->tlx_site.dd_l2_node_name, ' ', (u_i4) DB_NODE_MAXNAME,
	dxldbs_p->d2_3_ldb_node);
	
    STmove(i2_lxcb_p->tlx_site.dd_l3_ldb_name, ' ', (u_i4) DD_256_MAXDBNAME,
	dxldbs_p->d2_4_ldb_name);

    STmove(i2_lxcb_p->tlx_site.dd_l4_dbms_name, ' ', (u_i4) DB_TYPE_MAXLEN,
	dxldbs_p->d2_5_ldb_dbms);

    dxldbs_p->d2_6_ldb_id = i2_lxcb_p->tlx_site.dd_l5_ldb_id;
    dxldbs_p->d2_7_ldb_lxstate = i2_lxcb_p->tlx_state;
    dxldbs_p->d2_8_ldb_lxid1 = i2_lxcb_p->tlx_23_lxid.dd_dx_id1;
    dxldbs_p->d2_9_ldb_lxid2 = i2_lxcb_p->tlx_23_lxid.dd_dx_id2;

    STmove(i2_lxcb_p->tlx_23_lxid.dd_dx_name, ' ', (u_i4) DB_DB_MAXNAME,
	dxldbs_p->d2_10_ldb_lxname);

    if (i2_lxcb_p->tlx_20_flags & LX_02FLAG_2PC)
	dxldbs_p->d2_11_ldb_lxflags = TPC_LX_02FLAG_2PC;
    else
	dxldbs_p->d2_11_ldb_lxflags = 0;

    /* 1.  send insert query for iidd_ddb_dxldbs */
	
    qcb_p->qcb_1_can_id = INS_20_DXLDBS;
    qcb_p->qcb_3_ptr_u.u2_dxldbs_p = dxldbs_p;
    qcb_p->qcb_4_ldb_p = cdb_p;
    qcb_p->qcb_5_eod_b = FALSE; 
    qcb_p->qcb_6_col_cnt = 0;	/* no result expected */
    qcb_p->qcb_7_qry_p = (char *) NULL;
    qcb_p->qcb_8_dv_cnt = 0;    /* assume */
    qcb_p->qcb_10_total_cnt = 0;
    qcb_p->qcb_12_select_cnt = 0;

    status = tpq_i1_insert(v_rcb_p, qcb_p);
    return(status);
}


/*{
** Name: tp2_c1_log_dx - log the DX and its constituent LXs
** 
**  Description:    
**	Log the DX and its constituent LXs as a transaction to the CDB.  
**	Any communication failure will result in the CDB's aborting the 
**	transaction.  
**
**	Successful logging will be committed.
**
**	Failure policy: return upon error to fail DX.
**
** Inputs:
**	i1_dxcb_p			ptr to the DX CB in concern
**	v_rcb_p				ptr to the session CB
**	
** Outputs:
**	none
**	    
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
**      13-oct-90 (carl)
**	    modified header of logged messages
**      28-oct-90 (carl)
**	    fixed to use LOG_STATE1_SECURE instead of DX_2STATE_SECURE
*/


DB_STATUS
tp2_c1_log_dx(
	TPD_DX_CB   *i1_dxcb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status_1 = E_DB_OK,
		status_2 = E_DB_OK;
    DB_ERROR	err_1;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & i1_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = NULL;
    DD_DDB_DESC *ddb_p = & i1_dxcb_p->tdx_ddb_desc;
    DD_LDB_DESC *cdb_p = & ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    TPC_D1_DXLOG
		dxlog,
		*dxlog_p = & dxlog;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;
    bool	commit_b = TPD_1DO_COMMIT;  /* assume */
    

    if (! (cdb_p->dd_l6_flags & DD_02FLAG_2PC) )	
			    /* make sure the 2PC association is used */
	return( tpd_u2_err_internal(v_rcb_p) );

    switch (sscb_p->tss_4_dbg_2pc)
    {
    case TSS_60_CRASH_BEF_SECURE:
        status_1 = tp2_d0_crash(v_rcb_p);
        return(status_1);
	break;

    case TSS_65_SUSPEND_BEF_SECURE:
	tp2_d1_suspend(v_rcb_p);
	break;

    default:
	break;
    }

    MEfill(sizeof(dxlog), '\0', (PTR) & dxlog);

    /* fill DXLOG data */

    dxlog_p->d1_1_dx_id1 = i1_dxcb_p->tdx_id.dd_dx_id1;
    dxlog_p->d1_2_dx_id2 = i1_dxcb_p->tdx_id.dd_dx_id2;
    
    STmove(i1_dxcb_p->tdx_id.dd_dx_name, ' ', (u_i4) DB_DB_MAXNAME,
	dxlog_p->d1_3_dx_name);

    dxlog_p->d1_4_dx_flags = 0;
    dxlog_p->d1_5_dx_state = LOG_STATE1_SECURE;

    status_1 = tpd_u5_gmt_now(v_rcb_p, dxlog_p->d1_6_dx_create);
    if (status_1)
	return(status_1);

    dxlog_p->d1_6_dx_create[DD_25_DATE_SIZE] = EOS;

    STcopy(dxlog_p->d1_6_dx_create, dxlog_p->d1_7_dx_modify);

    dxlog_p->d1_8_dx_starid1 = 0;
    dxlog_p->d1_9_dx_starid2 = 0;

    STmove(cdb_p->dd_l2_node_name, ' ', (u_i4) DB_NODE_MAXNAME,
	dxlog_p->d1_10_dx_ddb_node);

    STmove(ddb_p->dd_d1_ddb_name, ' ', (u_i4) DB_DB_MAXNAME,
	dxlog_p->d1_11_dx_ddb_name);

    STmove(cdb_p->dd_l4_dbms_name, ' ', (u_i4) DB_TYPE_MAXLEN,
	dxlog_p->d1_12_dx_ddb_dbms);

    dxlog_p->d1_13_dx_ddb_id = ddb_p->dd_d7_uniq_id;

    /* 1.  send insert query for iidd_ddb_dxlog */
	
    qcb_p->qcb_1_can_id = INS_10_DXLOG;
    qcb_p->qcb_3_ptr_u.u1_dxlog_p = & dxlog;
    qcb_p->qcb_4_ldb_p = cdb_p;
    qcb_p->qcb_5_eod_b = FALSE; 
    qcb_p->qcb_6_col_cnt = 0;	/* no result expected */
    qcb_p->qcb_7_qry_p = (char *) NULL;
    qcb_p->qcb_8_dv_cnt = 0;    /* assume */
    qcb_p->qcb_10_total_cnt = 0;
    qcb_p->qcb_12_select_cnt = 0;

    status_1 = tpq_i1_insert(v_rcb_p, qcb_p);
    if (status_1)
	return(status_1);

    /* 2.  loop to log the LXs */

    if (lmcb_p->tlm_3_frst_ptr == NULL)
        return( tpd_u2_err_internal(v_rcb_p) );
    
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;	    

    while (lxcb_p != (TPD_LX_CB *) NULL && status_1 == E_DB_OK)
    {
	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG)
	{
	    if (! lxcb_p->tlx_write)	/* must be a write site */
		return( tpd_u2_err_internal(v_rcb_p) );

	    /* log the lx */

	    status_1 = tp2_c0_log_lx(i1_dxcb_p, lxcb_p, v_rcb_p);
	    if (status_1)
	    {
		STRUCT_ASSIGN_MACRO(v_rcb_p->tpr_error, err_1);
	    }
	}
	lxcb_p = lxcb_p->tlx_22_next_p;	/* advance to next LX CB */
    }

    /* 3.  abort/commit the log */

    if (status_1)
	commit_b = TPD_0DO_ABORT;   /* abort all inserts to recover */

    switch (sscb_p->tss_4_dbg_2pc)
    {
    case TSS_61_CRASH_IN_SECURE:
        status_1 = tp2_d0_crash(v_rcb_p);
        return(status_1);
	break;

    case TSS_66_SUSPEND_IN_SECURE:
	tp2_d1_suspend(v_rcb_p);
	break;

    default:
	break;
    }

    status_2 = tp2_u3_msg_to_cdb(commit_b, cdb_p, v_rcb_p);
    if (status_1)
    {
	STRUCT_ASSIGN_MACRO(err_1, v_rcb_p->tpr_error);
	return(status_1);
    }
    /* DX committed with CDB */

    switch (sscb_p->tss_4_dbg_2pc)
    {
    case TSS_62_CRASH_AFT_SECURE:
        status_1 = tp2_d0_crash(v_rcb_p);
        return(status_1);
	break;

    case TSS_67_SUSPEND_AFT_SECURE:
	tp2_d1_suspend(v_rcb_p);
	break;

    default:
	break;
    }

    return(status_2);
}


/*{
** Name: tp2_c2_log_dx_state - log the current DX state during recovery
** 
**  Description:    
**	Log the current DX COMMIT/ABORT state as an internal transaction to 
**	the CDB.
**
**	Failure policy: return upon error to caller.
**
** Inputs:
**	i1_commit_b			TRUE for COMMIT, FALSE for ABORT
**	v1_dxcb_p			ptr to the DX CB in concern
**	v_rcb_p				ptr to the session CB
**	
** Outputs:
**	v1_dxcb_p->
**	    tdx_state			DX_3STATE_ABORT/DX_4STATE_COMMIT  
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
tp2_c2_log_dx_state(
	bool	     i1_commit_b,
	TPD_DX_CB   *v1_dxcb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    DD_LDB_DESC	*cdb_p = 
		    & v1_dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
    TPC_D1_DXLOG
		dxlog,
		*dxlog_p = & dxlog;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;
    bool	done = FALSE;
    char	*state_p = (char *) NULL;


    if (! (cdb_p->dd_l6_flags & DD_02FLAG_2PC) )	
			/* make sure the 2PC association is used */
	return( tpd_u2_err_internal(v_rcb_p) );

    MEfill(sizeof(dxlog), '\0', (PTR) & dxlog);

    /* set up for sending update query */

    dxlog_p->d1_1_dx_id1 = v1_dxcb_p->tdx_id.dd_dx_id1;
    dxlog_p->d1_2_dx_id2 = v1_dxcb_p->tdx_id.dd_dx_id2;

    if (i1_commit_b)
    {
	switch (sscb_p->tss_4_dbg_2pc)
	{
	case TSS_70_CRASH_BEF_COMMIT:
	    status = tp2_d0_crash(v_rcb_p);
	    return(status);
	    break;

        case TSS_75_SUSPEND_BEF_COMMIT:
	    tp2_d1_suspend(v_rcb_p);
	    break;

	default:
	    break;
	}

	dxlog_p->d1_5_dx_state = LOG_STATE2_COMMIT;
	state_p = IITP_02_commit_p;
    }
    else
    {
	switch (sscb_p->tss_4_dbg_2pc)
	{
	case TSS_80_CRASH_BEF_ABORT:
	    status = tp2_d0_crash(v_rcb_p);
	    return(status);
	    break;

	case TSS_85_SUSPEND_BEF_ABORT:
	    tp2_d1_suspend(v_rcb_p);
	    break;

	default:
	    break;
	}

	dxlog_p->d1_5_dx_state = LOG_STATE3_ABORT;
	state_p = IITP_03_abort_p;
    }
	
    qcb_p->qcb_1_can_id = UPD_10_DXLOG_STATE;
    qcb_p->qcb_3_ptr_u.u1_dxlog_p = & dxlog;
    qcb_p->qcb_4_ldb_p = cdb_p;
    qcb_p->qcb_5_eod_b = FALSE; 
    qcb_p->qcb_6_col_cnt = 0;	/* no result expected */
    qcb_p->qcb_7_qry_p = (char *) NULL;
    qcb_p->qcb_8_dv_cnt = 0;    /* assume */
    qcb_p->qcb_10_total_cnt = 0;
    qcb_p->qcb_12_select_cnt = 0;

    status = tpq_u1_update(v_rcb_p, qcb_p);
    if (status)
    {
	/* failed to log state, report error */

	TRdisplay("%s: ...Logging %s state of DX %x %x\n",
		IITP_20_2pc_recovery_p,
		state_p,
		v1_dxcb_p->tdx_id.dd_dx_id1, 
		v1_dxcb_p->tdx_id.dd_dx_id2);
	TRdisplay("%s:    failed with ERROR %x\n",
		IITP_20_2pc_recovery_p,
		v_rcb_p->tpr_error.err_code);
    }
    else
    {
	/* state logged successfully */

	if (i1_commit_b)
	{
	    switch (sscb_p->tss_4_dbg_2pc)
	    {
	    case TSS_71_CRASH_IN_COMMIT:
		status = tp2_d0_crash(v_rcb_p);
		return(status);
		break;

	    case TSS_76_SUSPEND_IN_COMMIT:
		tp2_d1_suspend(v_rcb_p);
		break;

	    default:
		break;
	    }
	}
	else 
	{
	    switch (sscb_p->tss_4_dbg_2pc)
	    {
	    case TSS_81_CRASH_IN_ABORT:
		status = tp2_d0_crash(v_rcb_p);
		return(status);
		break;

	    case TSS_86_SUSPEND_IN_ABORT:
		tp2_d1_suspend(v_rcb_p);
		break;

	    default:
		break;
	    }
	}

	if (Tpf_svcb_p->tsv_1_printflag)
	{
	    TRdisplay("%s: ...%s state of DX %x %x logged\n",
		IITP_20_2pc_recovery_p,
		state_p,
		v1_dxcb_p->tdx_id.dd_dx_id1, 
		v1_dxcb_p->tdx_id.dd_dx_id2);
	}

	/* commit the update */

	status = tp2_u3_msg_to_cdb(TPD_1DO_COMMIT, cdb_p, v_rcb_p);
	if (status)
	{
	    /* failed to commit state logging */

	    TRdisplay("%s: ...Committing %s state of DX %x %x\n",
		IITP_20_2pc_recovery_p,
		state_p,
		v1_dxcb_p->tdx_id.dd_dx_id1, 
		v1_dxcb_p->tdx_id.dd_dx_id2,
		v_rcb_p->tpr_error.err_code);
	    TRdisplay("%s:    failed with ERROR %x\n",
		IITP_20_2pc_recovery_p,
		v_rcb_p->tpr_error.err_code);
	}
	else
	{
	    /* state comitted successfully */

	    if (Tpf_svcb_p->tsv_1_printflag)
		TRdisplay("%s: ...Logging %s state of DX %x %x committed\n",
		    IITP_20_2pc_recovery_p,
		    state_p,
		    v1_dxcb_p->tdx_id.dd_dx_id1, 
		    v1_dxcb_p->tdx_id.dd_dx_id2);

	    if (i1_commit_b)
	    {
		switch (sscb_p->tss_4_dbg_2pc)
		{
		case TSS_72_CRASH_AFT_COMMIT:
		    status = tp2_d0_crash(v_rcb_p);
		    return(status);
		    break;

		case TSS_77_SUSPEND_AFT_COMMIT:
		    tp2_d1_suspend(v_rcb_p);
		    break;

		default:
		    break;
		}
	    }
	    else 
	    {
		switch (sscb_p->tss_4_dbg_2pc)
		{
		case TSS_82_CRASH_AFT_ABORT:
		    status = tp2_d0_crash(v_rcb_p);
		    return(status);
		    break;

		case TSS_87_SUSPEND_AFT_ABORT:
		    tp2_d1_suspend(v_rcb_p);
		    break;

		default:
		    break;
		}
	    }

	    if (i1_commit_b)
		v1_dxcb_p->tdx_state = DX_4STATE_COMMIT;
	    else
		v1_dxcb_p->tdx_state = DX_3STATE_ABORT;
	}
    }

    return(status);
}


/*{
** Name: tp2_c3_delete_dx - delete a DX from iidd_ddb_dxlog
** 
**  Description:    
**	Delete a DX from iidd_ddb_dxlog and its LXs from iidd_ddb_dxldbs as
**	a transaction to the CDB.
**
**	Successful deletion will be committed.
**
**	Failure policy: return upon error to caller.
**
** Inputs:
**	i1_dxcb_p			ptr to the DX CB in concern
**	v_rcb_p				ptr to the session CB
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
**	01-may-91 (fpang)
**	    Reversed order of delete's in tp2_c3_delete_dx().
**	    If we do delete's in the same order as inserts, we
**	    will deadlock if two sessions are both using the cdb
**	    to log 2pc. Reversing the order will cause one to block
**	    until the other is finished. May slow things down a little,
**	    but should not have a major impact.
*/


DB_STATUS
tp2_c3_delete_dx(
	TPD_DX_CB   *i1_dxcb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    DD_LDB_DESC	*cdb_p = 
		    & i1_dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc;
    TPC_D2_DXLDBS
		dxldbs,
		*dxldbs_p = & dxldbs;
    TPC_D1_DXLOG
		dxlog,
		*dxlog_p = & dxlog;
    TPQ_QRY_CB	qcb1,
		qcb2,
		*qcb1_p = & qcb1,
		*qcb2_p = & qcb2;
    bool	done = FALSE;


    if (! (cdb_p->dd_l6_flags & DD_02FLAG_2PC) )
			    /* make sure the 2PC association is used */
	return( tpd_u2_err_internal(v_rcb_p) );

    qcb1_p->qcb_5_eod_b = qcb2_p->qcb_5_eod_b = FALSE; 
    qcb1_p->qcb_6_col_cnt = qcb2_p->qcb_6_col_cnt = 0;	/* no result expected */
    qcb1_p->qcb_7_qry_p = qcb2_p->qcb_7_qry_p = (char *) NULL;
    qcb1_p->qcb_8_dv_cnt = qcb2_p->qcb_8_dv_cnt = 0; 
    qcb1_p->qcb_10_total_cnt = qcb2_p->qcb_10_total_cnt = 0;
    qcb1_p->qcb_12_select_cnt = qcb2_p->qcb_12_select_cnt = 0;

    /* 1.  delete all the LDB information from iidd_ddb_dxldbs */

    MEfill(sizeof(dxldbs), '\0', (PTR) & dxldbs);

    dxldbs_p->d2_1_ldb_dxid1 = i1_dxcb_p->tdx_id.dd_dx_id1;
    dxldbs_p->d2_2_ldb_dxid2 = i1_dxcb_p->tdx_id.dd_dx_id2;

    qcb1_p->qcb_1_can_id = DEL_20_DXLDBS_BY_DXID;
    qcb1_p->qcb_3_ptr_u.u2_dxldbs_p = & dxldbs;
    qcb1_p->qcb_4_ldb_p = cdb_p;

    /* 2.  delete the DX from iidd_ddb_dxlog */

    MEfill(sizeof(dxlog), '\0', (PTR) & dxlog);

    dxlog_p->d1_1_dx_id1 = i1_dxcb_p->tdx_id.dd_dx_id1;
    dxlog_p->d1_2_dx_id2 = i1_dxcb_p->tdx_id.dd_dx_id2;

    qcb2_p->qcb_1_can_id = DEL_10_DXLOG_BY_DXID;
    qcb2_p->qcb_3_ptr_u.u1_dxlog_p = & dxlog;
    qcb2_p->qcb_4_ldb_p = cdb_p;

    status = tpq_d1_delete(v_rcb_p, qcb2_p);/* iidd_ddb_dxlog */
    if (status == E_DB_OK)
    {
        status = tpq_d1_delete(v_rcb_p, qcb1_p);    /* iidd_ddb_dxldbs */
	if (status == E_DB_OK)
	{
	    status = tp2_u3_msg_to_cdb(TPD_1DO_COMMIT, cdb_p, v_rcb_p);
	}
    }

    return(status);
}


/*{
** Name: tp2_c4_loop_to_log_dx_state - loop until the DX state is logged
**
** Description:
**	Loop until the DX state is logged successfully to iidd_ddb_dxlog 
**	in the CDB:
**
**	    1.  Loop 20 times with no timer in between.
**	    2.  Loop 20 times with a 1-second timer.
**	    3.  Loop forever with a 5-second timer.
**
**	This routine returns only E_DB_OK or E_DB_FATAL.
**
**	Failure policy: retry upon error.
**
** Inputs:
**	i1_commit_b			TRUE if COMMIT, FALSE if ABORT
**	v1_dxcb_p			ptr to TPD_DX_CB
**	v_rcb_p				ptr to session context
**
** Outputs:
**	v1_dxcb_p->
**	    tdx_state			DX_3STATE_ABORT/DX_4STATE_COMMIT  
**
**	Returns:
**	    E_DB_OK or E_DB_FATAL
**					
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-16-89 (carl)
**          created
*/


DB_STATUS
tp2_c4_loop_to_log_dx_state(
	bool		i1_commit_b,
	TPD_DX_CB	*v1_dxcb_p,
	TPR_CB		*v_rcb_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    i4		cs_mask = CS_TIMEOUT_MASK,
		timer = 0,
		i = 0;


    switch (sscb_p->tss_4_dbg_2pc)
    {
    case TSS_90_CRASH_STATE_LOOP:
	status = tp2_d0_crash(v_rcb_p);
	return(status);
	break;

    case TSS_95_SUSPEND_STATE_LOOP:
	tp2_d1_suspend(v_rcb_p);
	break;

    default:
	break;
    }

    /* 1.  Loop 20 times with no timer in between */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	status = tp2_c2_log_dx_state(i1_commit_b, v1_dxcb_p, v_rcb_p);
	if (status)
	{
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_rcb_p->tpr_error, prev_err);
	}
	else
	    return(E_DB_OK);
    }

    if (prev_sts == E_DB_OK)
	return(prev_sts);		/* return OK */

    prev_sts = E_DB_OK;	   /* reset for the next loop */

    /* 2.  Loop 20 times with a 1-second timer */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	/* do suspend for 1 second and then resume */

	timer = 1;
	
	status = CSsuspend(cs_mask, timer, (PTR) NULL);
		/* suspend 1 second */

	status = tp2_c2_log_dx_state(i1_commit_b, v1_dxcb_p, v_rcb_p);
	if (status)
	{
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_rcb_p->tpr_error, prev_err);
	}
	else
	    return(E_DB_OK);
    }

    if (prev_sts == E_DB_OK)
	return(prev_sts);		/* return OK */

    /* 3.  Loop forever with a 5-second timer (termination condtition:
	   prev_sts == E_DB_OK) */

    while (TRUE && prev_sts)
    {
        prev_sts = E_DB_OK;	   /* preset termination condition */

	/* do suspend for 5 seconds and then resume */

	timer = 5;

	status = CSsuspend(cs_mask, timer, (PTR) NULL);	
		/* suspend 5 seconds */

	status = tp2_c2_log_dx_state(i1_commit_b, v1_dxcb_p, v_rcb_p);
	if (status)
	{
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_rcb_p->tpr_error, prev_err);
	}
	else
	    return(E_DB_OK);
    }

    return(E_DB_OK);
}


/*{
** Name: tp2_c5_loop_to_delete_dx - loop until the DX is deleted
**
** Description:
**	Loop until the DX is deleted successfully from iidd_ddb_dxlog and
**	iidd_ddb_dxldbs in the CDB:
**
**	    1.  Loop 20 times with no timer in between.
**	    2.  Loop 20 times with a 1-second timer.
**	    3.  Loop forever with a 5-second timer.
**
**	This routine returns only E_DB_OK or E_DB_FATAL.
**
**	Failure policy: retry upon error.
**
** Inputs:
**	v1_dxcb_p			ptr to TPD_DX_CB
**	v_rcb_p				ptr to session context
**
** Outputs:
**	none
**
**	Returns:
**	    E_DB_OK or E_DB_FATAL
**					
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-16-89 (carl)
**          created
*/


DB_STATUS
tp2_c5_loop_to_delete_dx(
	TPD_DX_CB	*v1_dxcb_p,
	TPR_CB		*v_rcb_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    i4		cs_mask = CS_TIMEOUT_MASK,
		timer = 0,
		i = 0;


    switch (sscb_p->tss_4_dbg_2pc)
    {
    case TSS_91_CRASH_DX_DEL_LOOP:
	status = tp2_d0_crash(v_rcb_p);
	return(status);
	break;

    case TSS_96_SUSPEND_DX_DEL_LOOP:
	tp2_d1_suspend(v_rcb_p);
	break;

    default:
	break;
    }

    /* 1.  Loop 20 times with no timer in between */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	status = tp2_c3_delete_dx(v1_dxcb_p, v_rcb_p);
	if (status)
	{
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_rcb_p->tpr_error, prev_err);
	}
	else
	    return(E_DB_OK);
    }

    if (prev_sts == E_DB_OK)
	return(prev_sts);		/* return OK */

    prev_sts = E_DB_OK;	   /* reset for the next loop */

    /* 2.  Loop 20 times with a 1-second timer */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	/* do suspend for 1 second and then resume */

	timer = 1;
	
	status = CSsuspend(cs_mask, timer, (PTR) NULL);
		/* suspend 1 second */

	status = tp2_c3_delete_dx(v1_dxcb_p, v_rcb_p);
	if (status)
	{
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_rcb_p->tpr_error, prev_err);
	}
	else
	    return(E_DB_OK);
    }

    if (prev_sts == E_DB_OK)
	return(prev_sts);		/* return OK */

    /* 3.  Loop forever with a 5-second timer (termination condtition:
	   prev_sts == E_DB_OK) */

    while (TRUE && prev_sts)
    {
        prev_sts = E_DB_OK;	   /* preset termination condition */

	/* do suspend for 5 seconds and then resume */

	timer = 5;

	status = CSsuspend(cs_mask, timer, (PTR) NULL);	
		/* suspend 5 seconds */

	status = tp2_c3_delete_dx(v1_dxcb_p, v_rcb_p);
	if (status)
	{
	    if (status == E_DB_FATAL)
		return(E_DB_FATAL);
	    prev_sts = status;
	    STRUCT_ASSIGN_MACRO(v_rcb_p->tpr_error, prev_err);
	}
	else
	    return(E_DB_OK);
    }

    return(E_DB_OK);
}

