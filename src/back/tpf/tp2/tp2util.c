/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <pc.h>
#include    <di.h>
#include    <lo.h>
#include    <tm.h>
#include    <st.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
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
** Name: TP2UTIL.C - TPF's 2PC utilities
** 
** Description: 
**	This file includes the 2PC utilities.
** 
**	    tp2_u1_restart_1_ldb	
**				- restart a lost LDB
**	    tp2_u2_msg_to_ldb	- send a COMMIT/ABORT message to an LDB
**	    tp2_u3_msg_to_cdb	- send a COMMIT/ABORT message to the CDB along
**				  the 2PC association
**	    tp2_u4_init_odx_sess- initialize private copy of session CB for an 
**				  ODX
**	    tp2_u5_term_odx_sess- terminate the private session copy of an ODX
**	    tp2_u6_cdcb_to_ldb	- move LDB information from CDB CB to LDB desc
**	    tp2_u7_term_lm_cb	- close a ulm stream and terminate the LM CB
**	    tp2_u8_restart_all_ldbs
**				- restart all LDBs of given DX
**	    tp2_u9_loop_to_restart_all_ldbs
**				- loop to restart all LDBs of given DX
**	    tp2_u10_loop_to_restart_cdb 
**				- loop, if necessary, to restart a lost CDB
**	    tp2_u11_loop_to_send_msg
**				- loop, if necessary, to send a COMMIT/ABORT 
**				  message to an LDB 
**	    tp2_u12_term_all_ldbs
**				- terminate all the LDBs that have been 
**				  restarted
**	    tp2_u13_log
**				- log message to trace file or errlog
**
**  History:    $Log-for RCS$
**      nov-19-89 (carl)
**          created
**      jul-07-89 (carl)
**	    changed tp2_u1_restart_1_ldb, tp2_u2_msg_to_ldb to use LX id 
**	    instead of DX id
**	apr-29-92 (fpang)
**	    Created tp2_u13_log().
**	    Fixes B43851 (DX Recovery is not logging recovery status).
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**      24-Sep-1999 (hanch04)
**          Created tp2_put to replace tp2_u13_log
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

GLOBALREF   char            *IITP_00_tpf_p;
 

/*{
** Name: tp2_u1_restart_1_ldb - restart an LDB
** 
**  Description:    
**	Restart a lost LDB.
**
** Inputs:
**	i1_dxcb_p			ptr to the DX CB in concern
**	v1_lxcb_p			ptr to the LX CB in concern
**	v_rcb_p				ptr to the session CB
**	
** Outputs:
**	v2_lxcb_p->
**	    tlx_ldb_status		
**	o1_result_p->			restart result
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
**      jul-07-89 (carl)
**	    changed to use LX id instead of DX id
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
tp2_u1_restart_1_ldb(
	TPD_DX_CB   *i1_dxcb_p,
	TPD_LX_CB   *v1_lxcb_p,
	TPR_CB	    *v_rcb_p,
	i4	    *o1_result_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    RQR_CB	rqr_cb;


    *o1_result_p = TPD_0RESTART_NIL;   /* initialize */

    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    /* set up static parameters to call RQF */

    rqr_cb.rqr_session = sscb_p->tss_rqf;
    rqr_cb.rqr_q_language = v1_lxcb_p->tlx_txn_type;
    rqr_cb.rqr_msg.dd_p1_len = 0;
    rqr_cb.rqr_msg.dd_p2_pkt_p = NULL;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;
    rqr_cb.rqr_1_site = & v1_lxcb_p->tlx_site;
    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
    rqr_cb.rqr_timeout = 0;
/*
    rqr_cb.rqr_2pc_dx_id = & i1_dxcb_p->tdx_id;	
*/
    rqr_cb.rqr_2pc_dx_id = & v1_lxcb_p->tlx_23_lxid;	

    status = tpd_u4_rqf_call(RQR_RESTART, & rqr_cb, v_rcb_p);
    if (status == E_DB_OK)
    {
	v1_lxcb_p->tlx_20_flags |= LX_03FLAG_RESTARTED;
	v1_lxcb_p->tlx_ldb_status = LX_00LDB_OK;
    }
    else
    {
	/* possible cases: 

	    1)	LX was terminated, 
	    2)  LDB no longer exists,
	    3)	LDB busy starting up,
	    4)  some other server has taken over the DX and hence all LXs
	*/

	(VOID) tpd_u9_2pc_ldb_status(& rqr_cb, v1_lxcb_p);

	if (rqr_cb.rqr_error.err_code == E_RQ0049_RESTART_FAILED)
	{
	    switch (v1_lxcb_p->tlx_ldb_status)
	    {
	    case LX_03LDB_DX_ID_UNKNOWN:/* DX not recognized */
		v1_lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG;  /* unregister */
		v1_lxcb_p->tlx_state = LX_6STATE_CLOSED;		
		v1_lxcb_p->tlx_write = FALSE;
		*o1_result_p = TPD_4RESTART_UNKNOWN_DX_ID;
		break;

	    case LX_04LDB_DX_ALREADY_ADOPTED:
		*o1_result_p = TPD_2RESTART_NOT_DX_OWNER;
		break;

	    case LX_07LDB_DB_UNKNOWN:	/* LDB not in DX */
		v1_lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG;  /* unregister */
		v1_lxcb_p->tlx_state = LX_6STATE_CLOSED;		
		v1_lxcb_p->tlx_write = FALSE;
		*o1_result_p = TPD_3RESTART_LDB_UNKNOWN;
		break;

	    case LX_08LDB_NO_SUCH_LDB:	/* LDB no no longer exists */
		v1_lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG;  /* unregister */
		v1_lxcb_p->tlx_state = LX_6STATE_CLOSED;		
		v1_lxcb_p->tlx_write = FALSE;
		*o1_result_p = TPD_5RESTART_NO_SUCH_LDB;
		break;

	    default:
		*o1_result_p = TPD_1RESTART_UNEXPECTED_ERR;
		break;
	    }
	}
    }

    return(status);
}


/*{
** Name: tp2_u2_msg_to_ldb - Send a COMMIT/ABORT message to an LDB
** 
**  Description:    
**	Send a COMMIT/ABORT message to an LDB.
**
** Inputs:
**	i1_dxcb_p			ptr to the DX CB in concern
**	i2_commit_b			TRUE if committing, FALSE if aborting
**	v1_lxcb_p			ptr to the LX CB in concern
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
**      jul-07-89 (carl)
**	    changed to use LX id instead of DX id
*/


DB_STATUS
tp2_u2_msg_to_ldb(
	TPD_DX_CB   *i1_dxcb_p,
	bool	     i2_commit_b,
	TPD_LX_CB   *v1_lxcb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    RQR_CB	rqr_cb;
    i4		rqr_request;


    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    /* set up static parameters to call RQF */

    rqr_cb.rqr_q_language = v1_lxcb_p->tlx_txn_type;
    rqr_cb.rqr_session = sscb_p->tss_rqf;
    rqr_cb.rqr_msg.dd_p1_len = 0;
    rqr_cb.rqr_msg.dd_p2_pkt_p = NULL;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;
    rqr_cb.rqr_1_site = & v1_lxcb_p->tlx_site;
    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
    rqr_cb.rqr_timeout = 0;
/*
    rqr_cb.rqr_2pc_dx_id = & i1_dxcb_p->tdx_id;	
*/
    rqr_cb.rqr_2pc_dx_id = & v1_lxcb_p->tlx_23_lxid;	

    if (i2_commit_b)
    	rqr_request = RQR_COMMIT;
    else
	rqr_request = RQR_ABORT;

    status = tpd_u4_rqf_call(rqr_request, & rqr_cb, v_rcb_p);
    return(status);
}


/*{
** Name: tp2_u3_msg_to_cdb - Send a COMMIT/ABORT message to the CDB.
** 
**  Description:    
**	Send a COMMIT/ABORT message to the CDB along the 2PC association.
**
**	This operation is expected to always to succeed.
**
** Inputs:
**	i1_commit_b			TRUE if committing, FALSE if aborting
**	i2_cdb_p			ptr to the CDB in concern
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
*/


DB_STATUS
tp2_u3_msg_to_cdb(
	bool	     i1_commit_b,
	DD_LDB_DESC *i2_cdb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    RQR_CB	rqr_cb;
    i4		rqr_request;


    if (! (i2_cdb_p->dd_l6_flags & DD_02FLAG_2PC))
	return( tpd_u2_err_internal(v_rcb_p) );

    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    /* set up static parameters to call RQF */

    rqr_cb.rqr_2pc_dx_id = (DD_DX_ID *) NULL;
    rqr_cb.rqr_q_language = DB_SQL;
    rqr_cb.rqr_session = sscb_p->tss_rqf;
    rqr_cb.rqr_msg.dd_p1_len = 0;
    rqr_cb.rqr_msg.dd_p2_pkt_p = NULL;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;
    rqr_cb.rqr_1_site = i2_cdb_p;
    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
    rqr_cb.rqr_timeout = 0;

    if (i1_commit_b)
    	rqr_request = RQR_COMMIT;
    else
	rqr_request = RQR_ABORT;

    status = tpd_u4_rqf_call(rqr_request, & rqr_cb, v_rcb_p);
    return(status);
}


/*{
** Name: tp2_u4_init_odx_sess - initialize private copy of session CB for an ODX
** 
** Description:  
**	
** Inputs:
**	i1_cdb_p		ptr to the CDB CB
**	v_rcb_p->		TPF request control block
**
** Outputs:
**	o1_oxb_p->		contents initialized
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
**      dec-02-89 (carl)
**          created
**
*/


DB_STATUS
tp2_u4_init_odx_sess(
	DD_LDB_DESC *i1_cdb_p,
	TPR_CB	    *v_rcb_p,
	TPD_OX_CB   *o1_oxcb_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & o1_oxcb_p->tox_0_sscb.tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPC_D1_DXLOG
		*dxlog_p = & o1_oxcb_p->tox_1_dxlog;
    ULM_RCB	ulm;



    /* 1.  make a private copy of the recovery session for this ODX */

    STRUCT_ASSIGN_MACRO(*sscb_p, o1_oxcb_p->tox_0_sscb);
	
    /* 2.  copy the DX id into the private session CB */
	
    dxcb_p->tdx_id.dd_dx_id1 = dxlog_p->d1_1_dx_id1;
    dxcb_p->tdx_id.dd_dx_id2 = dxlog_p->d1_2_dx_id2;
    STmove(dxlog_p->d1_3_dx_name, ' ', (u_i4) DB_DB_MAXNAME,
	dxcb_p->tdx_id.dd_dx_name);
	

    /* 3.  copy the CDB id into this OX CB */

    STRUCT_ASSIGN_MACRO(*i1_cdb_p, 
	dxcb_p->tdx_ddb_desc.dd_d3_cdb_info.dd_i1_ldb_desc);
        
    /* 4.  open a special stream for the LX CBs */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_blocksize = sizeof(TPD_LX_CB);	/* preferred size */
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
    ulm.ulm_flags = ULM_SHARED_STREAM;
    status = ulm_openstream(& ulm);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0015_ULM_OPEN_FAILED, v_rcb_p) );
	   
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    return(E_DB_OK);
}


/*{
** Name: tp2_u5_term_odx_sess - terminate the private session copy of an ODX
**
**  Description: 
**
** Inputs:
**	v1_oxcb_p		ptr to TPD_OX_CB
**	v_rcb_p
**
** Outputs:
**	v1_oxb_p->		contents reinitialized
**
**	Returns:
**	    DB_STATUS
**
** Side Effects:
**	    None
**
** History:
**      dec-02-89 (carl)
**          created
*/


DB_STATUS
tp2_u5_term_odx_sess(
	TPD_OX_CB   *v1_oxcb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    TPD_DX_CB	*dxcb_p = & v1_oxcb_p->tox_0_sscb.tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    ULM_RCB	ulm;


    if (lmcb_p->tlm_1_streamid != (PTR) NULL)
    {

        /* close stream for the LX CBs */

	MEfill(sizeof(ulm), '\0', (PTR) & ulm);

	ulm.ulm_facility = DB_QEF_ID;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
	ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
        status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_rcb_p) );
	/* ULM has nullified tlm_1_streamid */
    }

    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    return(E_DB_OK);
}


/*{
** Name: tp2_u6_cdcb_to_ldb - move LDB information from CDB CB to LDB desc
**
**  Description: 
**
** Inputs:
**	i1_cdcb_p		ptr to TPD_CD_CB
**
** Outputs:
**	o1_cdb_p->		ptr to DD_LDB_DESC
**
**	Returns:
**	    nothing
**
** Side Effects:
**	    None
**
** History:
**      dec-02-89 (carl)
**          created
*/


VOID
tp2_u6_cdcb_to_ldb(
	TPD_CD_CB   *i1_cdcb_p,
	DD_LDB_DESC *o1_cdb_p)
{


    MEfill(sizeof(*o1_cdb_p), '\0', (PTR) o1_cdb_p);

    o1_cdb_p->dd_l1_ingres_b = TRUE;
    STmove(i1_cdcb_p->tcd_1_starcdbs.i1_5_cdb_node, ' ', (u_i4) DB_NODE_MAXNAME,
		o1_cdb_p->dd_l2_node_name);
    STmove(i1_cdcb_p->tcd_1_starcdbs.i1_6_cdb_dbms, ' ', (u_i4) DB_TYPE_MAXLEN,
		o1_cdb_p->dd_l4_dbms_name);
    STmove(i1_cdcb_p->tcd_1_starcdbs.i1_3_cdb_name, ' ', 
		(u_i4) DD_256_MAXDBNAME, o1_cdb_p->dd_l3_ldb_name);
    o1_cdb_p->dd_l5_ldb_id = 0;	/* unknown */
    o1_cdb_p->dd_l6_flags = DD_02FLAG_2PC;
}


/*{
** Name: tp2_u7_term_lm_cb - close a ulm stream and terminate the LM CB
**
**  Description: 
**
** Inputs:
**	v1_lmcb_p		ptr to TPD_LM_CB
**	v_rcb_p
**
** Outputs:
**	none
**
**	Returns:
**	    DB_STATUS
**
** Side Effects:
**	    None
**
** History:
**      dec-02-89 (carl)
**          created
*/


DB_STATUS
tp2_u7_term_lm_cb(
	TPD_LM_CB   *v1_lmcb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    ULM_RCB	ulm;


    if (v1_lmcb_p->tlm_1_streamid != (PTR) NULL)
    {
        /* close stream for the LX CBs */

	MEfill(sizeof(ulm), '\0', (PTR) & ulm);

	ulm.ulm_facility = DB_QEF_ID;
	ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
	ulm.ulm_streamid_p = &v1_lmcb_p->tlm_1_streamid;
        status = ulm_closestream(& ulm);
	if (status != E_DB_OK)
	    return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_rcb_p) );
	/* ULM has nullified tlm_1_streamid */
    }

    v1_lmcb_p->tlm_2_cnt = 0;
    v1_lmcb_p->tlm_3_frst_ptr = v1_lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    return(E_DB_OK);
}


/*{
** Name: tp2_u8_restart_all_ldbs - restart all LDBs of given DX
**
** Description:
**	Iterate once thru all lost LDBs restarting them.
**
** Inputs:
**	i_recover_b			TRUE if in recovery, FALSE otherwise
**					meaning a regular session
**	v_dxcb_p			ptr to DX conotrol block 
**	v_rcb_p				ptr to session context
**
** Outputs:
**	o1_dropout_cnt_p		number of terminated LDBs
**	o2_giveup_b_p			TRUE if any one has already been started
**					by some other server, FALSE otherwise
**	LX state updated for restarted LDBs
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
**      nov-16-89 (carl)
**          created
*/


DB_STATUS
tp2_u8_restart_all_ldbs(
	bool		i_recover_b,
	TPD_DX_CB	*v_dxcb_p,
	TPR_CB		*v_rcb_p,
	i4		*o1_dropout_cnt_p,
	bool		*o2_giveup_b_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & v_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p;
    i4		lx_cnt,
		result;


    *o1_dropout_cnt_p = 0;	/* assume */
    *o2_giveup_b_p = FALSE;	/* assume */

    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    lx_cnt = 0;
    while (lxcb_p != (TPD_LX_CB *) NULL && (! *o2_giveup_b_p))
    {
	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG    /* registered in the DX */
	    &&
	    lxcb_p->tlx_write)			    /* an updater */
	{
	    /* restart this lost LDB */

	    status = tp2_u1_restart_1_ldb(v_dxcb_p, lxcb_p, v_rcb_p, 
			& result);
	    if (status)
	    {
		switch (result)
		{
		case TPD_3RESTART_LDB_UNKNOWN:
		case TPD_4RESTART_UNKNOWN_DX_ID:
		case TPD_5RESTART_NO_SUCH_LDB:
		    lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG;  /* unregister */
		    lxcb_p->tlx_write = FALSE;
		    *o1_dropout_cnt_p += 1;
		    status = E_DB_OK;
		    break;
		case TPD_2RESTART_NOT_DX_OWNER:

		    *o2_giveup_b_p = TRUE;

		    /* terminate all restarted LDBs */

		    status = tp2_u12_term_all_ldbs(v_dxcb_p, v_rcb_p);

		    /* forgive any error */

		    return(E_DB_OK);
		    break;
		default:
		    return(status);
		}
	    }
	    lxcb_p->tlx_ldb_status = LX_00LDB_OK;
	    lxcb_p->tlx_20_flags |= LX_03FLAG_RESTARTED;
	}
	lxcb_p = lxcb_p->tlx_22_next_p;
	++lx_cnt;
    }

    return(E_DB_OK);
}


/*{
** Name: tp2_u9_loop_to_restart_all_ldbs - loop to restart all LDBs of given DX
**
** Description: 
**	This routine loops over all the lost LDBs to restart them.
**	It uses a timer mechanism and is potenitally an infinite loop.
**
**	Usage:  due to its infinite nature, this routine should be 
**		used only with great care and for ending a 2PC DX.
**
** Inputs: 
**	i_recover_b			TRUE if in recovery, FALSE otherwise
**					meaning a regular session
**	v_dxcb_p
**	v_rcb_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
** Outputs:
**	o1_giveup_p			TRUE if DX is taken over by some other
**					server
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    none
**
** History:
**      nov-28-89 (carl)
**          created
*/


DB_STATUS
tp2_u9_loop_to_restart_all_ldbs(
	bool	    i1_recover_b,
	TPD_DX_CB   *v_dxcb_p,
	TPR_CB	    *v_rcb_p,
	bool	    *o1_giveup_b_p)
{
    DB_STATUS	status_1 = E_DB_OK,
		status_2 = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    i4		cs_mask = CS_TIMEOUT_MASK,
		timer = 0,
		i = 0,
		dropout_cnt = 0;


    /* 1.  loop at most 20 times with no timer in between */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	status_1 = tp2_u8_restart_all_ldbs(i1_recover_b, v_dxcb_p, v_rcb_p,
		    & dropout_cnt, o1_giveup_b_p);
	if (status_1 == E_DB_OK
	    ||
	    status_1 == E_DB_FATAL)
	    return(status_1);
	
	if (*o1_giveup_b_p)
	    return(E_DB_OK);
    }   
	
    status_1 = E_DB_OK;	   /* reset for the next loop */

    /* 2.  loop at most 20 times with a 1-second timer */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	/* do suspend for 1 second and then resume */

	timer = 1;
	
	status_2 = CSsuspend(cs_mask, timer, (PTR) NULL);
		/* suspend 1 second */

	status_1 = tp2_u8_restart_all_ldbs(i1_recover_b, v_dxcb_p, v_rcb_p,
		    & dropout_cnt, o1_giveup_b_p);
	if (status_1 == E_DB_OK
	    ||
	    status_1 == E_DB_FATAL)
	    return(status_1);
	
	if (*o1_giveup_b_p)
	    return(E_DB_OK);
    }

    /* 3.  loop forever with a 5-second timer (termination condtition:
	   status_2 == E_DB_OK */

    while (TRUE && status_2)
    {
	/* do suspend for 5 seconds and then resume */

	timer = 5;

	status_2 = CSsuspend(cs_mask, timer, (PTR) NULL);	
		/* suspend 5 seconds */

	status_1 = tp2_u8_restart_all_ldbs(i1_recover_b, v_dxcb_p, v_rcb_p,
		    & dropout_cnt, o1_giveup_b_p);
	if (status_1 == E_DB_OK
	    ||
	    status_1 == E_DB_FATAL)
	    return(status_1);
	
	if (*o1_giveup_b_p)
	    return(E_DB_OK);
    }
    return(E_DB_OK);
}  


/*{
** Name: tp2_u10_loop_to_restart_cdb - restart a lost CDB
** 
**  Description:    
**	Restart a lost CDB.
**
**	Usage:  due to its infinite nature, this routine should be 
**		used only with great care and for ending a 2PC DX.
**
** Inputs:
**	i1_cdb_p			ptr to the CDB in concern
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
*/


DB_STATUS
tp2_u10_loop_to_restart_cdb(
	DD_LDB_DESC *i1_cdb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status_1 = E_DB_OK,
		status_2 = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    i4		cs_mask = CS_TIMEOUT_MASK,
		timer = 0,
		i = 0;
    RQR_CB	rqr_cb;


    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    /* set up static parameters to call RQF */

    rqr_cb.rqr_session = sscb_p->tss_rqf;
    rqr_cb.rqr_2pc_dx_id = (DD_DX_ID *) NULL;	/* no DX id */
    rqr_cb.rqr_q_language = DB_NOLANG;
    rqr_cb.rqr_msg.dd_p1_len = 0;
    rqr_cb.rqr_msg.dd_p2_pkt_p = NULL;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;
    rqr_cb.rqr_1_site = i1_cdb_p;
    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
    rqr_cb.rqr_timeout = 0;

    /* 1.  loop at most 20 times with no timer in between */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	status_1 = tpd_u4_rqf_call(RQR_RESTART, & rqr_cb, v_rcb_p);
	if (status_1 == E_DB_OK
	    ||
	    status_1 == E_DB_FATAL)
	    return(status_1);
    }   
	
    status_1 = E_DB_OK;	   /* reset for the next loop */

    /* 2.  loop at most 20 times with a 1-second timer */

    for (i = 0; i < TPD_0TIMER_MAX; ++i)
    {
	/* do suspend for 1 second and then resume */

	timer = 1;
	
	status_2 = CSsuspend(cs_mask, timer, (PTR) NULL);
		/* suspend 1 second */

	status_1 = tpd_u4_rqf_call(RQR_RESTART, & rqr_cb, v_rcb_p);
	if (status_1 == E_DB_OK
	    ||
	    status_1 == E_DB_FATAL)
	    return(status_1);
    }

    /* 3.  loop forever with a 5-second timer (termination condtition:
	   status_2 == E_DB_OK */

    while (TRUE && status_2)
    {
	/* do suspend for 5 seconds and then resume */

	timer = 5;

	status_2 = CSsuspend(cs_mask, timer, (PTR) NULL);	
		/* suspend 5 seconds */

	status_1 = tpd_u4_rqf_call(RQR_RESTART, & rqr_cb, v_rcb_p);
	if (status_1 == E_DB_OK
	    ||
	    status_1 == E_DB_FATAL)
	    return(status_1);
    }
    return(E_DB_OK);
}


/*{
** Name: tp2_u11_loop_to_send_msg - Send a COMMIT/ABORT message to an LDB
** 
**  Description:    
**	Send a COMMIT/ABORT message to an LDB, loop if necessary.
**
**	This atomic operation is expected to always succeed.
**
** Inputs:
**	i1_commit_b			TRUE if committing, FALSE if aborting
**	i2_ldb_p			ptr to the LDB
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
*/


DB_STATUS
tp2_u11_loop_to_send_msg(
	bool	     i1_commit_b,
	DD_LDB_DESC *i2_ldb_p,
	TPR_CB	    *v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    RQR_CB	rqr_cb;
    i4		rqr_request;
    bool	done = FALSE;


    if (! (i2_ldb_p->dd_l6_flags & DD_02FLAG_2PC))
	return( tpd_u2_err_internal(v_rcb_p) );

    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    /* set up static parameters to call RQF */

    rqr_cb.rqr_2pc_dx_id = (DD_DX_ID *) NULL;
    rqr_cb.rqr_q_language = DB_SQL;
    rqr_cb.rqr_session = sscb_p->tss_rqf;
    rqr_cb.rqr_msg.dd_p1_len = 0;
    rqr_cb.rqr_msg.dd_p2_pkt_p = NULL;
    rqr_cb.rqr_msg.dd_p3_nxt_p = NULL;
    rqr_cb.rqr_1_site = i2_ldb_p;
    rqr_cb.rqr_dv_cnt = 0;
    rqr_cb.rqr_dv_p = (DB_DATA_VALUE *) NULL;
    rqr_cb.rqr_timeout = 0;

    if (i1_commit_b)
    	rqr_request = RQR_COMMIT;
    else
	rqr_request = RQR_ABORT;

    done = FALSE;
    while (! done)
    {
	status = tpd_u4_rqf_call(rqr_request, & rqr_cb, v_rcb_p);
	if (status == E_DB_OK)
	    return(E_DB_OK);
	if (rqr_cb.rqr_error.err_code == E_RQ0045_CONNECTION_LOST)
	    status = tp2_u10_loop_to_restart_cdb(i2_ldb_p, v_rcb_p);
	else
	    return(E_DB_FATAL);
    }
    return(E_DB_OK);
}


/*{
** Name: tp2_u12_term_all_ldbs - terminate all LDBs of given DX that have been
**				 restarted
**
** Description:
**	Iterate once thru all LDBs that have been restarted
**
** Inputs:
**	v_dxcb_p			ptr to DX conotrol block 
**	v_rcb_p				ptr to session context
**
** Outputs:
**	none
**					by some other server, FALSE otherwise
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
**      nov-16-89 (carl)
**          created
*/


DB_STATUS
tp2_u12_term_all_ldbs(
	TPD_DX_CB	*v_dxcb_p,
	TPR_CB		*v_rcb_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_rcb_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & v_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = NULL;
    DD_LDB_DESC *ldb_p = NULL;
    i4		lx_cnt = 0;


    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    lx_cnt = 0;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	if (lxcb_p->tlx_20_flags & LX_03FLAG_RESTARTED)
	{
	    ldb_p = & lxcb_p->tlx_site;

	    /* terminate this restarted LDB */

	    status = tpd_u11_term_assoc(ldb_p, v_rcb_p);

	    /* forgive any error */

	    lxcb_p->tlx_20_flags &= ~LX_03FLAG_RESTARTED;
	}
	lxcb_p = lxcb_p->tlx_22_next_p;
	++lx_cnt;
    }

    return(E_DB_OK);
}


/*{
** Name: tp2_put - log message to trace file or errlog.
**
** Description:
**	If Tpf_svcb_p->tsv_1_printflag) is true, then log message to
**	the current trace file, log to errlog.log otherwise.
**
** Inputs:
**	p1-p6  -   args to control string
**
** Outputs:
**	none
**
**	Returns:
**	    none
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      24-Sep-1999 (hanch04)
**          Created to replace tp2_u13_log
*/


VOID
tp2_put(i4 tpf_error,
         CL_ERR_DESC *os_error,
         i4 num_params,
         i4 p1l,
         PTR p1,
         i4 p2l,
         PTR p2,
         i4 p3l,
         PTR p3,
         i4 p4l,
         PTR p4,
         i4 p5l,
         PTR p5,
         i4 p6l,
         PTR p6 )
{
    DB_STATUS	status;
    i4		error;
    char        buf[ DB_ERR_SIZE ];
    i4		msg_len = 0;

    if (Tpf_svcb_p->tsv_1_printflag)
    {
	status = ule_format(tpf_error, os_error, ULE_LOOKUP,
	(DB_SQLSTATE *) NULL, buf, sizeof(buf), 0, &error, num_params,
	p1l, p1, p2l, p2, p3l, p3, p4l, p4, p5l, p5, p6l, p6);
	TRdisplay("%s", buf);
    }
    else
	status = ule_format(tpf_error, os_error, ULE_LOG,
	(DB_SQLSTATE *) NULL, 0, 0, 0, &error, num_params,
	p1l, p1, p2l, p2, p3l, p3, p4l, p4, p5l, p5, p6l, p6);
    return;
}
