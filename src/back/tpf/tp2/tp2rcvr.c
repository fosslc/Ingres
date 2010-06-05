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
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <cv.h>
#include    <me.h>
#include    <nm.h>
#include    <st.h>
#include    <tm.h>
#include    <tr.h>
#include    <scf.h>
#include    <qsf.h>
#include    <adf.h>
#include    <qefrcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <tpfcat.h>
#include    <tpferr.h>
#include    <tpfddb.h>
#include    <tpfqry.h>
#include    <tpfproto.h>

/**
**
** Name: TP2RCVR.C - TPF's 2PC primitives
** 
** Description: 
**
**	tp2_r0_recover	    - recover all the DDBs of the current installation
**	tp2_r1_recover_all_ddbs
**			    - iterate over each CDB to recover its DDB
**	tp2_r2_recover_1_ddb 
**			    - recover a DDB given its CDB
**	tp2_r3_bld_cdb_list - build a CDB list given an installation's iidbdb 
**	tp2_r4_bld_odx_list - build an orphan DX list for a given CDB
**	tp2_r5_bld_lx_list  - build an LX list for a given DX
**	tp2_r6_close_odx    - close an orphan DX
**	tp2_r7_close_all_lxs- close all the LXs of given orphan DX 
**	tp2_r8_p1_recover   - recovery phase 1
**	tp2_r9_p2_recover   - recovery phase 2
**	tp2_r10_get_dx_cnts - query iidd_ddb_dxlog for DX count on each CDB
**	tp2_r11_bld_srch_list
**			    - duplicate CDBs in a search list
**	tp2_r12_set_ddb_recovered
**			    - set DDB's status to recovered in search list
**	tp2_r13_resecure_all_ldbs 
**			    - resecure all LDBs of given orphan DX
**
**  History:    $Log-for RCS$
**      nov-07-89 (carl)
**          created
**      jul-07-89 (carl)
**	    changed tp2_r13_resecure_all_ldbs to use LX id instead of DX id
**      jul-12-89 (carl)
**	    changed tp2_r5_bld_lx_list to use set up LX id using retrieved
**	    data from iidd_ddb_dxldbs
**      oct-19-90 (carl)
**	    cleanup 
**      oct-28-90 (carl)
**	    updated to use new and more meaningful defines for 
**	    TPC_D1_DXLOG.d1_5_dx_state to avoid confusion errors
**	apr-29-92 (fpang)
**	    Changed TRdisplay() calls to tp2_u13_log().
**	    Fixes B43851 (DX Recovery thread is not logging recovery status).
**	19-jan-92 (fpang)
**	    Included qefrcb.h, scf.h and adf.h
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	22-Jul-1993 (fpang)
**	    Include missing header files.
**	11-Aug-1997 (jenjo02)
**	    Changed ulm_streamid from (PTR) to (PTR*) so that ulm
**	    can destroy those handles when the memory is freed.
**	15-Aug-1997 (jenjo02)
** 	    New field, ulm_flags, added to ULM_RCB in which to specify the
**	    type of stream to be opened, shared or private.
**      24-Sep-1999 (hanch04)
**	    Replace tp2_u13_log with tp2_put.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	06-Nov-2001 (jenjo02)
**	    Don't call tp2_u3_msg_to_cdb(COMMIT) if not 2PC, avoiding
**	    bogus E_TP0020_INTERNAL error in log, which we don't care
**	    about anyway. This is something of a hack, but it's STAR.
**	01-May-2002 (bonro01)
**	    Fix overlay caused by using wrong variable to set EOS
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


GLOBALREF   char                *IITP_02_commit_p;

GLOBALREF   char                *IITP_03_abort_p;
 
GLOBALREF   char                *IITP_04_unknown_p;

GLOBALREF   char                *IITP_06_updater_p;
 
GLOBALREF   char                *IITP_20_2pc_recovery_p;
 
GLOBALREF   char                *IITP_24_ends_ok_p;
 
GLOBALREF   char                *IITP_25_78_dashes_p;
 
GLOBALREF   char                *IITP_26_completes_p;
 
GLOBALREF   char                *IITP_31_phase_1_p;
 
GLOBALREF   char                *IITP_32_phase_2_p;
 

/*{
** Name: tp2_r0_recover - recover all the DDBs of the current installation
**
**  External call: tpf_call(TPF_RECOVER, & tpr_cb);
** 
** Description: 
**	This routine is the recovery driver.  It builds a CDB list from 
**	the iistar_cdbs in iidbdb as a basis for recovery.  When recovery
**	is done, all the acquired memory resources are released.
**
**	Recovery Algorithm:
**
**	1.  Retrieve from iistar_cdbs in iidbdb and buffer the information of 
**	    all the CDBs.
**	    
**	2.  Loop at most 3 times through the CDB list looking for orphan DXs to
**	    perform recovery:
**
**	    2.1  Restart all the sites involved in the current orphan DX.
**	    2.2  Close current orphan DX only all/some sites have been 
**		 restarted and recovery is possible; give up and move on 
**		 to the next DX otherwise.
**
** Inputs: 
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	tpr_cb->
**	    tpf_error			tpf error info
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    Orphan DXs closed.
**
** History:
**      nov-28-89 (carl)
**          created
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
tp2_r0_recover(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
    ULM_RCB	ulm;
    i4		i = 0;
    bool	retry_b = TRUE;	/* assume retry necessary */


    if (Tpf_svcb_p->tsv_2_skip_recovery)
	return(E_DB_OK);		/* don't do recovery */

    /* 1.  build a CDB list for this installation if iidbdb is accessible;
    **     result in Tpf_svcb_p->tsv_8_cdb_ulmcb */

    status = tp2_r3_bld_cdb_list(v_tpr_p);
    if (status)
    {
	/*
	** inaccessible iidbdb or resource problem cannot be dealt with;
	** error policy: abort server startup 
	*/

	return(status);
    }
    if (lmcb_p->tlm_2_cnt <= 0)
    {
	if (lmcb_p->tlm_1_streamid != NULL) /* assert */
	    return( tpd_u2_err_internal(v_tpr_p) );

	return(E_DB_OK);
    }

    /* 2.  loop at most 3 times to recover the DDBs */

    retry_b = TRUE;   /* assume necessary */
    for (i = 1; i < (TPD_0SVR_RECOVER_MAX + 1) && retry_b; ++i)
    {
	tp2_put(I_TP0200_BEGIN_ROUND_REC, 0, 1, sizeof(i), (PTR)&i, 
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	status = tp2_r1_recover_all_ddbs(v_tpr_p);
	if (status == E_DB_OK)
	{
	    tp2_put(I_TP0201_END_ROUND_REC, 0, 1, sizeof(i), (PTR)&i,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	    retry_b = FALSE;	/* terminate loop */
	}
	else 
	{
	    if (status == E_DB_FATAL)
	    {
		tp2_put(E_TP0202_FAIL_FATAL_REC, 0, 2, sizeof(i), (PTR)&i,
		    sizeof(v_tpr_p->tpr_error.err_code),
		    (PTR)&v_tpr_p->tpr_error.err_code ,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
 
		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		goto dismantle_0;
	    }
	    else
	    {
		tp2_put(E_TP0203_FAIL_REC, 0, 2, sizeof(i), (PTR)&i,
		    sizeof(v_tpr_p->tpr_error.err_code),
		    (PTR)&v_tpr_p->tpr_error.err_code ,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
 		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
	    }
	}
    }   

dismantle_0:

    /* 3.  dismantle the CDB list and hence the stream */

    status = tp2_u7_term_lm_cb(lmcb_p, v_tpr_p);
/*
    ** commented code left here for reference **

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    status = ulm_closestream(& ulm);
*/
    if (prev_sts)
    {
	/* here if previous error recorded */

	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }

    return(E_DB_OK);	/* always return OK to open up for business */
}  


/*{
** Name: tp2_r1_recover_all_ddbs - try to recover all the DDBs of the 
**				   current installation
** 
** Description: 
**	This routine does the hard work of closing all the orphan DXs that are
**	left behind by defunct STAR server(s) in the current installation.
**
**	Assumption:
**	    The CDB list has already been built for use.
**
**	Algorithm:
**
**	Loop through every CDB to look for orphan DXs to recover.  
**	For each such DX, do:
**	    1.  Restart all the sites involved in current orphan DX.
**	    2.  Close current orphan DX only if all sites have been restarted;
**	        give up and move on to the next DX otherwise.
**
** Inputs: 
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	tpr_cb->
**	    tpf_error			tpf error info
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    Orphan DXs closed.
**
** History:
**      nov-28-89 (carl)
**          created
*/


DB_STATUS
tp2_r1_recover_all_ddbs(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK, /* assume */
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
    TPD_CD_CB	*cdcb_p = (TPD_CD_CB *) NULL;	    /* ptr to cdb CB */
    DB_DB_STR	ddb_name;
/*
    DB_OWN_STR	ddb_owner;
    DB_DB_STR   cdb_name;
    DB_OWN_STR	cdb_owner;
    DB_TYPE_STR	cdb_dbms;
    DB_NODE_STR cdb_node;
*/


    /* iterate over all the CDBs (DDBs equivalently) once every time thru loop 
    */

    cdcb_p = (TPD_CD_CB *) lmcb_p->tlm_3_frst_ptr;
    while (cdcb_p != (TPD_CD_CB *) NULL && status != E_DB_FATAL)
    {
	if ((cdcb_p->tcd_2_flags & TPD_1CD_TO_RECOVER)
	    &&
	    (! (cdcb_p->tcd_2_flags & TPD_2CD_RECOVERED)))
	{
	    MEcopy(cdcb_p->tcd_1_starcdbs.i1_1_ddb_name,
		DB_DB_MAXNAME, ddb_name);
	    ddb_name[DB_DB_MAXNAME] = EOS;
	    STtrmwhite(ddb_name);
/*
	    MEcopy(cdcb_p->tcd_1_starcdbs.i1_2_ddb_owner,
		DB_OWN_MAXNAME, ddb_owner);
	    ddb_owner[DB_OWN_MAXNAME] = EOS;
	    STtrmwhite(ddb_owner);

	    MEcopy(cdcb_p->tcd_1_starcdbs.i1_3_cdb_name,
		DB_DB_MAXNAME, cdb_name);
	    cdb_name[DB_DB_MAXNAME] = EOS;
	    STtrmwhite(cdb_name);

	    MEcopy(cdcb_p->tcd_1_starcdbs.i1_5_cdb_node,
		DB_NODE_MAXNAME, cdb_node);
	    cdb_node[DB_NODE_MAXNAME] = EOS;
	    STtrmwhite(cdb_node);

	    MEcopy(cdcb_p->tcd_1_starcdbs.i1_6_cdb_dbms,
		DB_TYPE_MAXLEN, cdb_dbms);
	    cdb_dbms[DB_TYPE_MAXLEN] = EOS;
	    STtrmwhite(cdb_dbms);
*/
	    tp2_put(E_TP0204_BEGIN_DDB_REC, 0, 1, sizeof(ddb_name), (PTR)ddb_name,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
/*
	    tp2_u13_log("%s: ...DDB owner %s\n", 
		    IITP_20_2pc_recovery_p,
		    ddb_owner);
	    tp2_u13_log("%s: ...CDB name %s\n", 
		    IITP_20_2pc_recovery_p,
		    cdb_name);
	    tp2_u13_log("%s: ...CDB node %s\n", 
		    IITP_20_2pc_recovery_p,
		    cdb_node);
	    tp2_u13_log("%s: ...CDB DBMS %s\n", 
		    IITP_20_2pc_recovery_p,
		    cdb_dbms);
*/
	    tpd_p8_prt_ddbdesc(v_tpr_p, & cdcb_p->tcd_1_starcdbs, 
		    TPD_1TO_TR);
	    status = tp2_r2_recover_1_ddb(cdcb_p, v_tpr_p);
	    if (status)
	    {
		/* must save error */

		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		tp2_put(I_TP0205_DDB_FAIL_REC, 0, 2, sizeof(ddb_name),
		    (PTR)ddb_name, sizeof(v_tpr_p->tpr_error.err_code), 
		    (PTR)&v_tpr_p->tpr_error.err_code,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );

		lmcb_p->tlm_2_cnt--;
	    }
	    else    
	    {
		/* no error */

		cdcb_p->tcd_2_flags |= TPD_2CD_RECOVERED;

		tp2_put(I_TP0206_DDB_OK_REC, 0, 1, sizeof(ddb_name),
		    (PTR)ddb_name,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    }
	}
	 cdcb_p = cdcb_p->tcd_4_next_p;
    }
	
    if (prev_sts)
    {
    	status = prev_sts;
    	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }

    return(status);
}  


/*{
** Name: tp2_r2_recover_1_ddb - recover a DDB given its CDB
**
** Description: 
**	Given a CDB in the recovery list, this routine reads iidd_ddb_dxlog
**	to determine if there are any orphan DXs to be recovered.
**	
**	A stream (tsv_9_odx_ulmcb) is opened for allocating space for the 
**	orphan-DX list.  It will be closed when this routine returns.
**
**	CDB recovery algorithm:
**
**	1.  Retrieve from iidd_ddb_dxlog in the CDB the information of all the 
**	    orphan DXs.
**	    
**	2.  Recover each DX by calling the DX terminator.
**
** Inputs: 
**	v1_cdcb_p->			ptr to the CDB CB
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
** Outputs:
**	v_tpr_p->
**	    tpf_error			tpf error info
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    Orphan DXs closed.
**
** History:
**      nov-28-89 (carl)
**          created
**	30-jul-97 (nanpr01)
**	    After deallocating the memory, set the streamid to NULL.
**	07-Aug-1997 (jenjo02)
**	    BEFORE deallocating the memory, set the streamid to NULL.
*/


DB_STATUS
tp2_r2_recover_1_ddb(
	TPD_CD_CB	*v1_cdcb_p,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK,
		sts_flush = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_9_odx_ulmcb,
		*lxlm_p = NULL;
    TPD_OX_CB	*oxcb_p = NULL;	    /* ptr to ODX CB */
    ULM_RCB     ulm;	
    DD_LDB_DESC	cdb,
		*cdb_p = & cdb;
    RQR_CB	rqr_cb;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;
    i4		dxlog_cnt;
/*
    DB_DB_STR	ddb_name;
    DB_OWN_STR	ddb_owner;
    DB_DB_STR   cdb_name;
    DB_OWN_STR	cdb_owner;
    DB_TYPE_STR	cdb_dbms;
    DB_NODE_STR cdb_node;
*/


    if (! (v1_cdcb_p->tcd_2_flags & TPD_1CD_TO_RECOVER))
	return(E_DB_OK);			/* no recovery necessary */

    if (v1_cdcb_p->tcd_2_flags & TPD_2CD_RECOVERED)
	return(E_DB_OK);			/* already recovered */
/*
    ** 0.  make sure CDB has the 2PC iidd_ddb_dxlog catalog **

    (VOID) tp2_u6_cdcb_to_ldb(v1_cdcb_p, cdb_p);

    MEfill(sizeof(qcb), '\0', (PTR) & qcb);

    qcb_p->qcb_1_can_id = SEL_60_TABLES_FOR_DXLOG;
    qcb_p->qcb_3_ptr_u.u0_ptr = (PTR) NULL;
    qcb_p->qcb_4_ldb_p = cdb_p;
    status = tpq_s4_prepare(v_tpr_p, qcb_p);
    if (status)
	return(status);

    if (! qcb_p->qcb_5_eod_b)
	sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

    dxlog_cnt = qcb_p->qcb_10_total_cnt;

    if (dxlog_cnt <= 0)
    {
	** generate a warning message **

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_1_ddb_name, DB_DB_MAXNAME, ddb_name);
	ddb_name[DB_DB_MAXNAME] = EOS;
	STtrmwhite(ddb_name);

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_2_ddb_owner, DB_OWN_MAXNAME, ddb_owner);
	ddb_owner[DB_OWN_MAXNAME] = EOS;
	STtrmwhite(ddb_owner);

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_3_cdb_name, DB_DB_MAXNAME, cdb_name);
	cdb_name[DB_DB_MAXNAME] = EOS;
	STtrmwhite(cdb_name);

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_5_cdb_node, DB_NODE_MAXNAME, cdb_node);
	cdb_node[DB_NODE_MAXNAME] = EOS;
	STtrmwhite(cdb_node);

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_6_cdb_dbms, DB_TYPE_MAXLEN, cdb_dbms);
	cdb_dbms[DB_TYPE_MAXLEN] = EOS;
	STtrmwhite(cdb_dbms);

	tp2_u13_log("%s: ...2PC catalogs NOT found for DDB %s:\n",
		IITP_20_2pc_recovery_p,
		ddb_name);
	tp2_u13_log("%s: ...(1)  DDB owner %s\n", 
		IITP_20_2pc_recovery_p,
		sscb_p.
		ddb_owner);
	tp2_u13_log("%s: ...(2)  CDB name %s\n", 
		IITP_20_2pc_recovery_p,
		cdb_name);
	tp2_u13_log("%s: ...(3)  CDB node %s\n", 
		IITP_20_2pc_recovery_p,
		cdb_node);
	tp2_u13_log("%s: ...(4)  CDB DBMS %s\n", 
		IITP_20_2pc_recovery_p,
		cdb_dbms);
	v1_cdcb_p->tcd_2_flags |= TPD_2CD_RECOVERED;

	status = tpd_u11_term_assoc(cdb_p, v_tpr_p);

	return(status);    ** nothing to recover **
    }
*/
    /* 1.  build a DX list for this CDB; result in Tpf_svcb_p->tsv_9_odx_ulmcb 
    */
    
    status = tp2_r4_bld_odx_list(v1_cdcb_p, v_tpr_p);
    if (status)
    {
	if (lmcb_p->tlm_1_streamid != (PTR) NULL)   /* assert */
	    return( tpd_u2_err_internal(v_tpr_p) );

	return(status);	
    }

    /* 2.  loop thru each of the orphan DXs once and only once */

    oxcb_p = (TPD_OX_CB *) lmcb_p->tlm_3_frst_ptr;
    while (oxcb_p != (TPD_OX_CB *) NULL)
    {
	if (! (oxcb_p->tox_2_flags & TPD_1OX_FLAG_DONE))
	{
	    status = tp2_r6_close_odx(v1_cdcb_p, oxcb_p, v_tpr_p);
				/* should have closed the LX stream */

	    if (oxcb_p->tox_0_sscb.tss_dx_cb.tdx_22_lx_ulmcb.tlm_1_streamid
		!= (PTR) NULL)
	    {
		status = tpd_u2_err_internal(v_tpr_p);

		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		goto dismantle_2;
	    }

	    if (status == E_DB_OK)
	    {
		lmcb_p->tlm_2_cnt--;
		oxcb_p->tox_2_flags |= TPD_1OX_FLAG_DONE;
	    }
	    else 
	    {
		/* must save error */

		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		if (status == E_DB_FATAL)
		    goto dismantle_2;
	    }
	}    
	oxcb_p = oxcb_p->tox_4_next_p;
    }

dismantle_2:

    if (status == E_DB_OK)
    {
	v1_cdcb_p->tcd_2_flags |= TPD_2CD_RECOVERED;
	
	status = tp2_r12_set_ddb_recovered(v1_cdcb_p, v_tpr_p);
/*
	status = tp2_u3_msg_to_cdb(TRUE, cdb_p, v_tpr_p);
			    ** TRUE to commit queries on this CDB **
*/
	status = tpd_u11_term_assoc(cdb_p, v_tpr_p);
    }
    else
    {
	/* save error if any */

	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
    }

    /* 3.  here to dismantle the open ODX stream */

    status = tp2_u7_term_lm_cb(lmcb_p, v_tpr_p);

    /* 4.  here to dismantle the open LX stream if any */

    if (Tpf_svcb_p->tsv_10_slx_ulmcb_p != (TPD_LM_CB *) NULL)
    {
	lmcb_p = Tpf_svcb_p->tsv_10_slx_ulmcb_p;
	Tpf_svcb_p->tsv_10_slx_ulmcb_p = (TPD_LM_CB *) NULL;
				/* must reinitialize */

	if (lmcb_p->tlm_1_streamid != (PTR) NULL)
	{

	    ulm.ulm_facility = DB_QEF_ID;
	    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
	    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
	    status = ulm_closestream(& ulm);
	    if (status != E_DB_OK)
		return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	    /* ULM has nullified tlm_1_streamid */
	}
    }

    if (prev_sts)
    {
	/* restore original error */

	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }

    return(status);
}


/*{
** Name: tp2_r3_bld_cdb_list - build a CDB list given an installation's iidbdb
**
** Description: 
**	This routine reads iistar_cdbs in iidbdb to build a CDB list for
**	the purpose of recovery.
**
**	A stream (tsv_8_cdb_ulmcb) is opened for allocating space for the 
**	CDB list.  It will be closed by the caller when done.
**
** Inputs: 
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	v_tpr_p->
**	    tpf_error			tpf error info
**	Tpf_svcb_p->
**	    tsv_8_cdb_ulmcb
**		.tlm_1_streamid		id of stream opened for list
**		.tlm_2_cnt		number of CDB CBs in the list
**		.tlm_3_frst_ptr		ptr to the first CB in the list
**		.tlm_4_last_ptr		ptr to the last CB in the list
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
**      nov-28-89 (carl)
**          created
*/


DB_STATUS
tp2_r3_bld_cdb_list(
	TPR_CB	*v_tpr_p)
{
    char	dummy_1[20000];

    DB_STATUS	status = E_DB_OK,
		ignore,
		sts_commit = E_DB_OK,
		sts_flush = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
    TPD_CD_CB	*cdcb_p = NULL;	    /* ptr to cdb CB */
    ULM_RCB	ulm;
    i4		cdb_cnt = 0;
    TPC_I1_STARCDBS
		starcdbs,
		*starcdbs_p = & starcdbs;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;


    if (lmcb_p->tlm_1_streamid != (PTR) NULL)
	return( tpd_u2_err_internal(v_tpr_p) );

    tp2_put(I_TP0207_BEGIN_CDB_REC, 0, 0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0 );

    /* 1.1  allocate a private CDB stream */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_blocksize = sizeof(TPC_I1_STARCDBS);    /* preferred size */
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    ulm.ulm_streamid_p = &ulm.ulm_streamid;
    ulm.ulm_flags = ULM_PRIVATE_STREAM;
    status = ulm_openstream(& ulm);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0015_ULM_OPEN_FAILED, v_tpr_p) );

    /* 1.2  initialize the ulm cb */

    lmcb_p->tlm_1_streamid = ulm.ulm_streamid;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = 
	lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    /* 2.  retrieve the entry count from iistar_cdbs */

    MEfill(sizeof(qcb), '\0', (PTR) & qcb);

    qcb_p->qcb_1_can_id = SEL_30_CDBS_CNT;
    qcb_p->qcb_3_ptr_u.u3_starcdbs_p = starcdbs_p;
    qcb_p->qcb_4_ldb_p = v_tpr_p->tpr_site; /* iidbdb */
    status = tpq_s4_prepare(v_tpr_p, qcb_p);
    if (status)
    {
	tp2_put(E_TP0208_FAIL_READ_CDBS, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	/*
	** assume inaccessible iidbdb; abort building CDB list
	*/

	goto dismantle_3;
    }
    cdb_cnt = qcb_p->qcb_10_total_cnt;

    if (! qcb_p->qcb_5_eod_b)
	sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

    if (cdb_cnt <= 0)
    {
	/* no DDBs in this installation */

	/* Don't try COMMIT if not 2PC */
	if ( v_tpr_p->tpr_site->dd_l6_flags & DD_02FLAG_2PC )
	    sts_commit = tp2_u3_msg_to_cdb(TRUE, v_tpr_p->tpr_site, v_tpr_p);
			    /* TRUE to commit all queries on iidd_ddb_dxlog */
	goto dismantle_3;
    }

    /* 3.  retrieve all the entries from iistar_cdbs, and build a linked 
	   list of TPC_I1_STARCDBS */

    qcb_p->qcb_1_can_id = SEL_31_CDBS_ALL;
    qcb_p->qcb_3_ptr_u.u3_starcdbs_p = starcdbs_p;
    qcb_p->qcb_4_ldb_p = v_tpr_p->tpr_site;
    status = tpq_s4_prepare(v_tpr_p, qcb_p);
    if (status)
    {
	tp2_put(E_TP0208_FAIL_READ_CDBS, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	/*
	** assume inaccessible iidbdb; abort building CDB list
	*/

	goto dismantle_3;
    }

    /* 4.  loop on read entries to build linked list */

    while (status == E_DB_OK && (! qcb_p->qcb_5_eod_b))
    {
	/* 4.1  create a CDB CB */

	status = tpd_u6_new_cb(TPD_2LM_CD_CB, v_tpr_p, (PTR *) & cdcb_p);
	if (status == E_DB_OK)
	{
	    /* 4.2  move information from iistar_cdbs entry to CDB CB */

	    STRUCT_ASSIGN_MACRO(starcdbs, cdcb_p->tcd_1_starcdbs);
	
	    /* 4.3  fetch the next if any */

	    status = tpq_s2_fetch(v_tpr_p, qcb_p);
	    if (status)
		tp2_put(E_TP0208_FAIL_READ_CDBS, 0, 0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	}
	else
	    tp2_put( E_TP0209_FAIL_ALLOC_CDBS, 0, 0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
    }

    if (! qcb_p->qcb_5_eod_b)
	sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

    if (status == E_DB_OK)
    {
	tp2_put( I_TP020A_END_CDB_REC, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	return(E_DB_OK);    /* return here to save the CDB list */
    }	    
dismantle_3:

    /* 5.  here to dismantle open CDB stream */

    ignore = tp2_u7_term_lm_cb(lmcb_p, v_tpr_p);

    if (status)
    {
	tp2_put( E_TP020B_FAIL_CDB_REC, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    }
    return(status);
}


/*{
** Name: tp2_r4_bld_odx_list - build an orphan DX list for a given CDB
**
** Description: 
**	This routine reads iistar_cdbs in iidbdb to build a CDB list for
**	the purpose of recovery.
**
**	A stream (tsv_8_cdb_ulmcb) is opened for allocating space for the 
**	CDB list.  It will be closed by the caller when done.
**
** Inputs: 
**	v1_cdcb_p->			ptr to CDB CB
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	v_tpr_p->
**	    tpf_error			tpf error info
**	Tpf_svcb_p->
**	    tsv_8_cdb_ulmcb
**		.tlm_1_streamid		id of stream opened for list
**		.tlm_2_cnt		number of CDB CBs in the list
**		.tlm_3_frst_ptr		ptr to the first CB in the list
**		.tlm_4_last_ptr		ptr to the last CB in the list
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
**      nov-28-89 (carl)
**          created
*/


DB_STATUS
tp2_r4_bld_odx_list(
	TPD_CD_CB   *v1_cdcb_p,
	TPR_CB	    *v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK,
		sts_commit = E_DB_OK,
		sts_flush = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_9_odx_ulmcb;
    TPD_OX_CB	*oxcb_p = NULL;	    /* ptr to ODX CB */
    DD_LDB_DESC	cdb,
		*cdb_p = & cdb;
    ULM_RCB	ulm;
    i4		dx_cnt = 0,
		do_cnt = 0;
    TPC_D1_DXLOG
		dxlog,
		*dxlog_p = & dxlog;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;
    DB_DB_STR   ddb_name;


    if (lmcb_p->tlm_1_streamid != (PTR) NULL)
	return( tpd_u2_err_internal(v_tpr_p) );

    (VOID) tp2_u6_cdcb_to_ldb(v1_cdcb_p, cdb_p);

    /* 1.  retrieve the entry count from iidd_ddb_dxlog */

    MEfill(sizeof(qcb), '\0', (PTR) & qcb);

    qcb_p->qcb_1_can_id = SEL_10_DXLOG_CNT;
    qcb_p->qcb_3_ptr_u.u1_dxlog_p = dxlog_p;
    qcb_p->qcb_4_ldb_p = cdb_p;
    status = tpq_s4_prepare(v_tpr_p, qcb_p);
    if (status)
	return(status);

    if (! qcb_p->qcb_5_eod_b)
	sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

    dx_cnt = qcb_p->qcb_10_total_cnt;

    MEcopy(v1_cdcb_p->tcd_1_starcdbs.i1_1_ddb_name, DB_DB_MAXNAME, ddb_name);
    ddb_name[DB_DB_MAXNAME] = EOS;
    STtrmwhite(ddb_name);

    tp2_put( E_TP020C_ORPHAN_DX_FOUND, 0, 2, sizeof( dx_cnt ), (PTR)&dx_cnt,
	sizeof(ddb_name), (PTR)ddb_name,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

    if (dx_cnt <= 0)
    {
	v1_cdcb_p->tcd_2_flags |= TPD_2CD_RECOVERED;

	status = tpd_u11_term_assoc(cdb_p, v_tpr_p);

	tp2_put(I_TP020D_NO_NEEDED_REC, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	return(E_DB_OK);    /* nothing to recover */
    }
    else
    {
	tp2_put(I_TP020E_BEGIN_DX_LIST, 0, 1, sizeof( ddb_name ), (PTR)ddb_name,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    }

    /* 2.1  allocate a private ODX stream */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_blocksize = sizeof(TPC_D1_DXLOG);    /* preferred size */
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    ulm.ulm_streamid_p = &ulm.ulm_streamid;
    ulm.ulm_flags = ULM_PRIVATE_STREAM;
    status = ulm_openstream(& ulm);
    if (status != E_DB_OK)
	return( tpd_u10_fatal(E_TP0015_ULM_OPEN_FAILED, v_tpr_p) );

    /* 2.2  initialize the ulm cb */

    lmcb_p->tlm_1_streamid = ulm.ulm_streamid;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = 
	lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    /* 3.  retrieve all the entries from iidd_ddb_dxlog, and build a linked 
	   list of TPD_DX_CBs */

    qcb_p->qcb_1_can_id = SEL_11_DXLOG_ALL;
    qcb_p->qcb_3_ptr_u.u1_dxlog_p = dxlog_p;
    qcb_p->qcb_4_ldb_p = cdb_p;
    status = tpq_s4_prepare(v_tpr_p, qcb_p);
    if (status)
    {
	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

	goto dismantle_4;
    }

    if (qcb_p->qcb_5_eod_b)
    {
	tp2_put(I_TP020F_DX_LIST_EMPTY, 0, 1, sizeof( ddb_name ), (PTR)ddb_name,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	
	goto dismantle_4;
    }
    /* 4.  loop on read entries to build linked list */

    while (status == E_DB_OK && (! qcb_p->qcb_5_eod_b))
    {
	/* 4.1  create an OX CB */

	status = tpd_u6_new_cb(TPD_3LM_OX_CB, v_tpr_p, (PTR *) & oxcb_p);
	if (status == E_DB_OK)
	{
	    /* 4.2  move information from iidd_ddb_dxlog entry to OX CB */

	    STRUCT_ASSIGN_MACRO(dxlog, oxcb_p->tox_1_dxlog);

	    /* 4.3  make a copy of the sesssion CB for this DX */

	    STRUCT_ASSIGN_MACRO(*sscb_p, oxcb_p->tox_0_sscb);

	    /* 4.4  initialize for this DX CB */

	    status = tp2_u4_init_odx_sess(cdb_p, v_tpr_p, oxcb_p);

	    if (status == E_DB_OK)
		status = tpq_s2_fetch(v_tpr_p, qcb_p);
				    /* get next if any */
	}
    }

    if (! qcb_p->qcb_5_eod_b)
	sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

    /* Don't try COMMIT if not 2PC */
    if ( cdb_p->dd_l6_flags & DD_02FLAG_2PC )
	sts_commit = tp2_u3_msg_to_cdb(TRUE, cdb_p, v_tpr_p);
			    /* TRUE to commit all queries on iidd_ddb_dxlog */
    if (status == E_DB_OK)
    {
	tp2_put(I_TP0210_END_DX_LIST, 0, 1, sizeof( ddb_name ), (PTR)ddb_name,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	return(E_DB_OK);
    }
dismantle_4:

    if (status)
    {
	tp2_put(E_TP0211_FAIL_END_DX_LIST, 0, 2, sizeof( ddb_name ), (PTR)ddb_name,
	    sizeof(v_tpr_p->tpr_error.err_code), (PTR)&v_tpr_p->tpr_error.err_code,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
    }

    /* 5.  here to dismantle the open ODX stream */

    status = tp2_u7_term_lm_cb(lmcb_p, v_tpr_p);
/*
    ** commented code left here for reference **

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    status = ulm_closestream(& ulm);

    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = lmcb_p->tlm_4_last_ptr = (PTR) NULL;
*/
    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
	return(status);
    }

    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );

    return(E_DB_OK);
}


/*{
** Name: tp2_r5_bld_lx_list - build an LX list for a given DX
**
** Description: 
**	This routine reads iidd_ddb_dxldbs in the given CDB to build an
**	LX list for the purpose of recovering the DX.
**
**	An LX stream is opened in the OX CB for allocating space for the 
**	CDB list.  It will be closed by the caller when done.
**
** Inputs: 
**	i1_cdcb_p                       ptr to the CDB CB
**	v1_oxcb_p			ptr to the ODX CB
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	v_tpr_p->
**	    tpf_error			tpf error info
**	v1_oxcb_p->
**	    tox_0_sscb
**		.tss_dx_cb
**		    .tdx_22_lx_ulmcb
**			.tlm_1_streamid	    id of stream opened for list
**			.tlm_2_cnt	    number of LX CBs in the list
**			.tlm_3_frst_ptr	    ptr to the first CB in the list
**			.tlm_4_last_ptr	    ptr to the last CB in the list
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
**      nov-28-89 (carl)
**          created
**      jul-12-89 (carl)
**	    changed to use set up LX id using retrieved data from 
**	    iidd_ddb_dxldbs
**	30-jul-97 (nanpr01)
**	    After deallocating the memory, set the streamid to NULL.
*/


DB_STATUS
tp2_r5_bld_lx_list(
	TPD_CD_CB   *i1_cdcb_p,
	TPD_OX_CB   *v1_oxcb_p,
	TPR_CB	    *v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK,
		sts_commit = E_DB_OK,
		sts_flush = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = & v1_oxcb_p->tox_0_sscb;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    DD_LDB_DESC	cdb,
		*cdb_p = & cdb;
    ULM_RCB	ulm;
    i4		ldb_cnt = 0,
		do_cnt = 0;
    TPC_D2_DXLDBS
		dxldbs,
		*dxldbs_p = & dxldbs;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;
    char	cdb_node[DB_NODE_MAXNAME + 1],
		cdb_name[DD_256_MAXDBNAME + 1];


    if (lmcb_p->tlm_1_streamid != (PTR) NULL)
	return( tpd_u2_err_internal(v_tpr_p) );

    if (Tpf_svcb_p->tsv_10_slx_ulmcb_p != (TPD_LM_CB *) NULL)
	return( tpd_u2_err_internal(v_tpr_p) );

    (VOID) tp2_u6_cdcb_to_ldb(i1_cdcb_p, cdb_p);

    /* 1.  retrieve the entry count from iidd_ddb_dxldbs */

    MEfill(sizeof(qcb), '\0', (PTR) & qcb);

    dxldbs_p->d2_1_ldb_dxid1 = dxcb_p->tdx_id.dd_dx_id1;
    dxldbs_p->d2_2_ldb_dxid2 = dxcb_p->tdx_id.dd_dx_id2;

    qcb_p->qcb_1_can_id = SEL_20_DXLDBS_CNT_BY_DXID;
    qcb_p->qcb_3_ptr_u.u2_dxldbs_p = dxldbs_p;
    qcb_p->qcb_4_ldb_p = cdb_p;
    status = tpq_s4_prepare(v_tpr_p, qcb_p);
    if (status)
	return(status);

    if (! qcb_p->qcb_5_eod_b)
	sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

    ldb_cnt = qcb_p->qcb_10_total_cnt;

    if (ldb_cnt <= 0)
    {
	/* Don't try COMMIT if not 2PC */
	if ( cdb_p->dd_l6_flags & DD_02FLAG_2PC )
	    sts_commit = tp2_u3_msg_to_cdb(TRUE, cdb_p, v_tpr_p);
			    /* TRUE to commit all queries on iidd_ddb_dxldbs */
	return(E_DB_OK);    /* assume result of manual termination */
    }

    /* 2.1  allocate a private LX stream */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_blocksize = sizeof(TPD_LX_CB);    /* preferred size */
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    ulm.ulm_streamid_p = &ulm.ulm_streamid;
    ulm.ulm_flags = ULM_PRIVATE_STREAM;
    status = ulm_openstream(& ulm);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0015_ULM_OPEN_FAILED, v_tpr_p) );

    /* 2.2  initialize the ulm cb */

    lmcb_p->tlm_1_streamid = ulm.ulm_streamid;
    lmcb_p->tlm_2_cnt = 0;
    lmcb_p->tlm_3_frst_ptr = lmcb_p->tlm_4_last_ptr = (PTR) NULL;

    Tpf_svcb_p->tsv_10_slx_ulmcb_p = lmcb_p;	/* MUST set to pass the ULM CB 
						** to tpd_u6_new_cb */

    /* 3.  retrieve all the entries from iidd_ddb_dxldbs, and build a linked 
	   list of TPD_LX_CBs */

    MEfill(sizeof(qcb), '\0', (PTR) & qcb);

    qcb_p->qcb_1_can_id = SEL_21_DXLDBS_ALL_BY_DXID;
    qcb_p->qcb_3_ptr_u.u2_dxldbs_p = dxldbs_p;
    qcb_p->qcb_4_ldb_p = cdb_p;
    status = tpq_s4_prepare(v_tpr_p, qcb_p);
    if (status)
    {
	tp2_put(E_TP0212_FAIL_READ_DX, 0, 3, 
	    sizeof(v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof(v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    sizeof(v_tpr_p->tpr_error.err_code),
	    (PTR)&v_tpr_p->tpr_error.err_code,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	MEcopy(cdb_p->dd_l2_node_name, DB_NODE_MAXNAME, cdb_node);
	cdb_node[DB_NODE_MAXNAME] = EOS;
	STtrmwhite(cdb_node);
	
	MEcopy(cdb_p->dd_l3_ldb_name, DD_256_MAXDBNAME, cdb_name);
	cdb_name[DD_256_MAXDBNAME] = EOS;
	STtrmwhite(cdb_name);

	tp2_put(I_TP0213_CDB_NODE_NAME, 0, 2, 
	    sizeof(cdb_node), (PTR)cdb_node,
	    sizeof(cdb_name), (PTR)cdb_name,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

	goto dismantle_5;
    }
    /* 4.  loop on read entries to build linked list */

    while (status == E_DB_OK && (! qcb_p->qcb_5_eod_b))
    {
	/* 4.1  create an LX CB */

	status = tpd_u6_new_cb(TPD_4LM_SX_CB, v_tpr_p, (PTR *) & lxcb_p);
	if (status == E_DB_OK)
	{
	    /* 4.2  move information from iidd_ddb_dxldbs entry to LX CB */

	    /* WARNING: must preserve tlx_21_prev_p and tlx_22_next_p */

	    lxcb_p->tlx_txn_type = dxcb_p->tdx_lang_type;
	    lxcb_p->tlx_start_time = 0;
	    lxcb_p->tlx_write = TRUE;
	    lxcb_p->tlx_sp_cnt = 0;
	    lxcb_p->tlx_site.dd_l1_ingres_b = TRUE; /* must connect as STAR */
	    STmove(dxldbs_p->d2_3_ldb_node, ' ', (u_i4) DB_NODE_MAXNAME,
	    	lxcb_p->tlx_site.dd_l2_node_name);
	    STmove(dxldbs_p->d2_4_ldb_name, ' ', (u_i4) DD_256_MAXDBNAME,
	    	lxcb_p->tlx_site.dd_l3_ldb_name); 
	    STmove(dxldbs_p->d2_5_ldb_dbms, ' ', (u_i4) DB_TYPE_MAXLEN,
	    	lxcb_p->tlx_site.dd_l4_dbms_name);
	    lxcb_p->tlx_site.dd_l5_ldb_id = dxldbs_p->d2_6_ldb_id;
	    lxcb_p->tlx_site.dd_l6_flags = LX_01FLAG_REG;
	    lxcb_p->tlx_state = dxldbs_p->d2_7_ldb_lxstate;

	    /* fill in LX id using retrieved data */

	    lxcb_p->tlx_23_lxid.dd_dx_id1 = dxldbs_p->d2_8_ldb_lxid1;
	    lxcb_p->tlx_23_lxid.dd_dx_id2 = dxldbs_p->d2_9_ldb_lxid2;
	    STmove(dxldbs_p->d2_10_ldb_lxname, ' ', (u_i4) DB_DB_MAXNAME,
	    	lxcb_p->tlx_23_lxid.dd_dx_name);

	    lxcb_p->tlx_ldb_status = LX_09LDB_ASSOC_LOST;
	    lxcb_p->tlx_20_flags = LX_02FLAG_2PC | LX_01FLAG_REG;

	    status = tpq_s2_fetch(v_tpr_p, qcb_p);
	}
    }

    if (qcb_p->qcb_5_eod_b)
    {
	sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);
    }
    /* Don't try COMMIT if not 2PC */
    if ( cdb_p->dd_l6_flags & DD_02FLAG_2PC )
	sts_commit = tp2_u3_msg_to_cdb(TRUE, cdb_p, v_tpr_p);
			    /* TRUE to commit all queries on iidd_ddb_dxldbs */

    if (status == E_DB_OK)
    {
	return(E_DB_OK);
    }

dismantle_5:

    if (status)
    {
	tp2_put(E_TP0214_FAIL_END_LX_LIST, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
    }

    /* 5.  here to dismantle the open ODX stream */

    status = tp2_u7_term_lm_cb(lmcb_p, v_tpr_p);

    /* 6.  here to dismantle the open LX stream if any */

    if (Tpf_svcb_p->tsv_10_slx_ulmcb_p != (TPD_LM_CB *) NULL)
    {
	lmcb_p = Tpf_svcb_p->tsv_10_slx_ulmcb_p;
	Tpf_svcb_p->tsv_10_slx_ulmcb_p = (TPD_LM_CB *) NULL;	
				/* must reinitialize */

	if (lmcb_p->tlm_1_streamid != (PTR) NULL)
	{
	    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
	    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
	    status = ulm_closestream(& ulm);
	    if (status != E_DB_OK)
		return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	    /* ULM has nullified tlm_1_streamid */
	}
    }

    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
	return(status);
    }

    return(status);
}


/*{
** Name: tp2_r6_close_odx - terminate an orphan DX
**
** Description: 
**	Given an orphan DX to recover, this routine reads iidd_ddb_dxldbs
**	to determine if there are any LXs to be recovered.
**	
**	A stream (Tpf_svcb_p->tsv_10_slx_ulmcb_p) is opened for allocating 
**	space for the LX list.  It will be closed by this routine upon
**	return.
**
**	ODX recovery algorithm:
**
**	1.  Retrieve from iidd_ddb_dxldbs in the CDB the information of all the 
**	    LDBs involved and reconstruct an LX list.
**	    
**	2.  Restart all the LDBs involved in this DX.  Give up if 1) some 
**	    other STAR server gets to them first and is now recovering this 
**	    same DX or 2) some LDB connection cannot be established.
**
**	3.  Close the each DX according to its STATE:
**
**	    DX State
**	    --------
**
**	    ABORT   Abort all the remaining sites that have NOT aborted.
**
**	    COMMIT  Commit all the remaining sites that have NOT committed.
**
**	    SECURE  If any site does NOT recognize the DX, enter the above 
**		    ABORT state.
**		    Otherwise re-poll all the sites for an ABOR/COMMIT
**		    decision and enter the appropriate DX state after
**		    logging that state.
**
**
** Inputs: 
**	i1_cdcb_p->			ptr to the CDB CB
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
** Outputs:
**	v1_oxcb_p->			ptr to OX CB
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	    Orphan DXs closed.
**
** History:
**      nov-28-89 (carl)
**          created
**	30-jul-97 (nanpr01)
**	    After deallocating the memory, set the streamid to NULL.
*/


DB_STATUS
tp2_r6_close_odx(
	TPD_CD_CB	*i1_cdcb_p,
	TPD_OX_CB	*v1_oxcb_p,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,   /* assume */
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = & v1_oxcb_p->tox_0_sscb;
    TPD_DX_CB	*dxcb_p = & sscb_p->tss_dx_cb;
    TPD_LM_CB	*lmcb_p = & dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    bool	commit_b,
		giveup_b = FALSE,
		recover_b = TRUE;
    ULM_RCB	ulm;
    RQR_CB	rqr_cb;
    i4		dropout_cnt;	/* number of LDBs that have terminated their
				** LXs and hence do not recognize this DX */
    char	*state_p = (char *) NULL;
	

    /* 1.  build the LX list for this DX; result in the ODX CB */

	tp2_put( I_TP0215_BEGIN_LX_LIST, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

    status = tp2_r5_bld_lx_list(i1_cdcb_p, v1_oxcb_p, v_tpr_p);
    if (status)
    {
	tp2_put( E_TP0214_FAIL_END_LX_LIST, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	return(status);	
    }
    /* 2.  must first restart all the LDBs to make sure that no other 
	   STAR server is recovering this same DX */

    tp2_put( I_TP0216_RESTART_LDBS, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

    status = tp2_u8_restart_all_ldbs(recover_b, dxcb_p, v_tpr_p,
		& dropout_cnt, & giveup_b);
    if (status)
    {
	tp2_put( E_TP0217_FAIL_RESTART_LDBS, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

	goto dismantle_6;
    }

    if (giveup_b)
    {
	tp2_put( E_TP0217_FAIL_RESTART_LDBS, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	goto dismantle_6;	    /* nothing to do */
    }

    tp2_put( I_TP0218_END_RESTART_LDBS, 0, 2, 
	sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	(PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	(PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    if (dropout_cnt == 0)
    {
	tp2_put( I_TP0219_END_RESTART_ALL_LDBS, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    }
    else
    {
	tp2_put( E_TP021A_LDB_TERM, 0, 3, 
	    sizeof(dropout_cnt), (PTR)&dropout_cnt,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    }
    /* 3.  all continuing LDBs restarted; terminate the LXs */

    switch (v1_oxcb_p->tox_1_dxlog.d1_5_dx_state)
    {
    case LOG_STATE1_SECURE:
	tp2_put( E_TP021B_DX_SECURE, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	dxcb_p->tdx_state = DX_2STATE_SECURE;
	if (dropout_cnt > 0)	/* some have aborted */
	{
	    commit_b = FALSE;
	    status = tp2_c2_log_dx_state(commit_b, dxcb_p, v_tpr_p);
	    if (status)
	    {
		tp2_put( E_TP021C_FAIL_DX_ABORT, 0, 3, 
		    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
		    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
		    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
		    sizeof( v_tpr_p->tpr_error.err_code ),
		    (PTR)&v_tpr_p->tpr_error.err_code,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );

		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		goto disassoc_6;
	    }

	    tp2_put( I_TP021D_END_DX_ABORT, 0, 2, 
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	    state_p = IITP_03_abort_p;
	    dxcb_p->tdx_state = DX_3STATE_ABORT;
	}
	else		
	{
	    commit_b = TRUE;/* all have RESTARTED and hence MUST BE WILLING */

	    status = tp2_c2_log_dx_state(commit_b, dxcb_p, v_tpr_p);
	    if (status)
	    {
		tp2_put( E_TP021E_FAIL_DX_COMMIT, 0, 3, 
		    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
		    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
		    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
		    sizeof( v_tpr_p->tpr_error.err_code ),
		    (PTR)&v_tpr_p->tpr_error.err_code,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );

		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		goto disassoc_6;
	    }

	    tp2_put( I_TP021F_END_DX_COMMIT, 0, 2, 
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	    state_p = IITP_02_commit_p;
	    dxcb_p->tdx_state = DX_4STATE_COMMIT;
/*
	    ** re-poll for ABORT/COMMIT **

	    status = tp2_r13_resecure_all_ldbs(dxcb_p, v_tpr_p, & commit_b);
	    if (status)
	    {
tp2_u13_log("%s: ...Resecuring DX %x %x failed with ERROR %x\n", 
		    IITP_20_2pc_recovery_p,
		    v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		    v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
		    v_tpr_p->tpr_error.err_code);

		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		goto disassoc_6;
	    }
	    tp2_u13_log("%s: ...Resecuring DX %x %x succeeds\n", 
		    IITP_20_2pc_recovery_p,
		    v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		    v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2);
*/
	}
	break;
    case LOG_STATE2_COMMIT:
	tp2_put( E_TP0220_DX_STATE_COMMIT, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	state_p = IITP_02_commit_p;
	dxcb_p->tdx_state = DX_4STATE_COMMIT;
	commit_b = TRUE;
	break;
    case LOG_STATE3_ABORT:
	tp2_put( E_TP0221_DX_STATE_ABORT, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	state_p = IITP_03_abort_p;
	dxcb_p->tdx_state = DX_3STATE_ABORT;
	commit_b = FALSE;
	break;
    default:
	state_p = IITP_04_unknown_p;
	tp2_put( E_TP0222_DX_STATE_UNKNOWN, 0, 3, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    sizeof(v1_oxcb_p->tox_1_dxlog.d1_5_dx_state),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_5_dx_state,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	prev_sts = tpd_u10_fatal(E_TP0020_INTERNAL, v_tpr_p);
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

	goto disassoc_6;
	break;
    }

    tp2_put(I_TP0223_DX_STATE_REC , 0, 3, 
	sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	(PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	(PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	sizeof( state_p ), (PTR)state_p,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0 );

    status = tp2_r7_close_all_lxs(dxcb_p, commit_b, v_tpr_p);
    if (status == E_DB_OK)
    {
	/* 4.  delete the DX and LDB entries from the CDB */

	status = tp2_c3_delete_dx(dxcb_p, v_tpr_p);
	if (status)
	{
	    tp2_put( E_TP0224_FAIL_DEL_DX, 0, 3, 
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
		sizeof( v_tpr_p->tpr_error.err_code ),
		(PTR)&v_tpr_p->tpr_error.err_code,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	}
	else
	{
	    tp2_put( I_TP0225_END_DX_REC, 0, 2, 
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
		sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
		(PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );
	}
    }
    else
    {
	/* here if error */

	tp2_put( E_TP0226_FAIL_DX_TERM, 0, 2, 
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_1_dx_id1,
	    sizeof( v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2 ),
	    (PTR)&v1_oxcb_p->tox_1_dxlog.d1_2_dx_id2,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	/* save the DX in iidd_ddb_dxlog for future recovery or diagnosis */
    }   
 
disassoc_6:

    /* 6.  terminate all the restarted associations if any */

    if (status)
    {
	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
    }

    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    rqr_cb.rqr_session = v_tpr_p->tpr_rqf;
    rqr_cb.rqr_mod_flags = (DD_MODIFY *) NULL;

    status = E_DB_OK;
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	if ( lxcb_p->tlx_20_flags & LX_03FLAG_RESTARTED)
	{
	    rqr_cb.rqr_1_site = & lxcb_p->tlx_site;
	    status = tpd_u4_rqf_call(RQR_TERM_ASSOC, & rqr_cb, v_tpr_p);
	    /* ignore any error */

	    lxcb_p->tlx_20_flags &= ~LX_03FLAG_RESTARTED;
	}
	lxcb_p = lxcb_p->tlx_22_next_p;	/* advance to next */
    }

dismantle_6:

    /* 7.  close the LX list stream */

    status = tp2_u5_term_odx_sess(v1_oxcb_p, v_tpr_p);

    /* 8.  here to dismantle the open LX stream if any */

    if (Tpf_svcb_p->tsv_10_slx_ulmcb_p != (TPD_LM_CB *) NULL)
    {
	lmcb_p = Tpf_svcb_p->tsv_10_slx_ulmcb_p;
	Tpf_svcb_p->tsv_10_slx_ulmcb_p = (TPD_LM_CB *) NULL;	
				/* must reinitialize */

	if (lmcb_p->tlm_1_streamid != (PTR) NULL)
	{
	    ulm.ulm_facility = DB_QEF_ID;
	    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
	    ulm.ulm_streamid_p = &lmcb_p->tlm_1_streamid;
	    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
	    status = ulm_closestream(& ulm);
	    if (status != E_DB_OK)
		return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );
	    /* ULM has nullified tlm_1_streamid */
	}
    }

    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }

    return(status);
}


/*{
** Name: tp2_r7_close_all_lxs - close all the LXs of given orphan DX 
**
** Description:
**	Close all the LXs of the orphan DX.
**
**	Assumption: they have already been restarted.
**
** Inputs:
**	v1_dxcb_p			ptr to TPD_DX_CB
**	v_tpr_p				ptr to session context
**
** Outputs:
**	    none
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
**      nov-16-89 (carl)
**          created
*/


DB_STATUS
tp2_r7_close_all_lxs(
	TPD_DX_CB	*v1_dxcb_p,
	bool		i2_commit_b,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & v1_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p;
    i4		lx_cnt,
		result;
    bool	done = TRUE;	/* assume */
    char	*state_p = (char *) NULL;
/*
    DB_DB_STR	ddb_name;
    DB_OWN_STR	ddb_owner;
    DB_DB_STR   cdb_name;
    DB_OWN_STR	cdb_owner;
    DB_TYPE_STR	cdb_dbms;
    DB_NODE_STR cdb_node;
*/

    if (i2_commit_b)
	state_p = IITP_02_commit_p;
    else
	state_p = IITP_03_abort_p;

    /* broadcast COMMIT/ABORT message to all the LDBs to close their LXs */

    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    lx_cnt = 0;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG)   /* a registered LX */
	{
/*
	    MEcopy(lxcb_p->tlx_site.dd_l2_node_name, DB_NODE_MAXNAME, ldb_node);
	    ldb_node[DB_NODE_MAXNAME] = EOS;
	    STtrmwhite(ldb_node);

	    MEcopy(lxcb_p->tlx_site.dd_l3_ldb_name, DD_256_MAXDBNAME, ldb_name);
	    ldb_name[DD_256_MAXDBNAME] = EOS;
	    STtrmwhite(ldb_name);

	    MEcopy(lxcb_p->tlx_site.dd_l4_dbms_name, DB_TYPE_MAXLEN, ldb_dbms);
	    ldb_dbms[DB_TYPE_MAXLEN] = EOS;
	    STtrmwhite(ldb_dbms);
*/
	    if (! lxcb_p->tlx_write)		    /* must be an updater */
		return( tpd_u2_err_internal(v_tpr_p) );

	    status = tp2_u2_msg_to_ldb(v1_dxcb_p, i2_commit_b, 
			    lxcb_p, v_tpr_p);
	    if (status == E_DB_OK)
	    {
		lxcb_p->tlx_20_flags &= ~LX_01FLAG_REG; 
					    /* unregister */
		if (i2_commit_b)
		    lxcb_p->tlx_state = LX_5STATE_COMMITTED;
		else
		    lxcb_p->tlx_state = LX_4STATE_ABORTED;

		lxcb_p->tlx_write = FALSE;

		if (i2_commit_b)
		    tp2_put( I_TP0227_COMMIT_UPDATE, 0, 0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0 );
		else
		    tp2_put( E_TP0228_ABORT_UPDATE, 0, 0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0 );
		tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_1TO_TR);
		tp2_put( I_TP0229_DX_OK, 0, 2, 
		    sizeof( v1_dxcb_p->tdx_id.dd_dx_id1 ),
		    (PTR)&v1_dxcb_p->tdx_id.dd_dx_id1,
		    sizeof( v1_dxcb_p->tdx_id.dd_dx_id2 ),
		    (PTR)&v1_dxcb_p->tdx_id.dd_dx_id2,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    }
	    else
	    {
		prev_sts = status;
		STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);

		if (i2_commit_b)
		    tp2_put( I_TP0227_COMMIT_UPDATE, 0, 0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0 );
		else
		    tp2_put( E_TP0228_ABORT_UPDATE, 0, 0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0 );
		tpd_p9_prt_ldbdesc(v_tpr_p, & lxcb_p->tlx_site, TPD_1TO_TR);

		tp2_put( E_TP0230_DX_FAIL, 0, 3, 
		    sizeof( v1_dxcb_p->tdx_id.dd_dx_id1 ),
		    (PTR)&v1_dxcb_p->tdx_id.dd_dx_id1,
		    sizeof( v1_dxcb_p->tdx_id.dd_dx_id2 ),
		    (PTR)&v1_dxcb_p->tdx_id.dd_dx_id2,
		    sizeof( v_tpr_p->tpr_error.err_code ),
		    (PTR)&v_tpr_p->tpr_error.err_code,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );

	    }
	} 
	lxcb_p = lxcb_p->tlx_22_next_p;
	++lx_cnt;
    } 

    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
    }
    return(status);
}


/*{
** Name: tp2_r8_p1_recover - recovery phase 1
**
**  External call: tpf_call(TPF_P1_RECOVER, & tpr_cb);
** 
** Description: 
**	This routine builds the CDB list and queries every CDB for the number
**	of orphan DXs.  If successful, the CDB list is replicated in a second
**	search list.
**
** Inputs: 
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	1.  Tpf_svcb_p->tsv_4_flags
**					TPD_01_IN_RECOVERY ON if recovery is
**					expected, OFF otherwise
**	2.  Tpf_svcb_p->tsv_8_cdb_ulmcb	CDB list built if there is recovery 
**					work to do
**	tpr_cb->
**	    tpf_error			tpf error if any
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	none
**
** History:
**      jan-28-90 (carl)
**          created
**	04-Oct-2001 (jenjo02)
**	    Fixed up error and message handling,
**	    which was wrong and/or misleading.
*/


DB_STATUS
tp2_r8_p1_recover(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		ignore;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
    TPD_LM_CB	*srchcb_p = & Tpf_svcb_p->tsv_12_srch_ulmcb;
    bool	recovery_b = FALSE; /* assume */
    i4		err_code;


    /* Tpf_svcb_p->tsv_4_flags initialized */

    v_tpr_p->tpr_need_recovery = FALSE;	/* assume */

    srchcb_p->tlm_1_streamid = (PTR) NULL;
    srchcb_p->tlm_2_cnt = 0;
    srchcb_p->tlm_3_frst_ptr = 
	srchcb_p->tlm_4_last_ptr = (PTR) NULL;

    if (Tpf_svcb_p->tsv_2_skip_recovery)
    {
	tp2_put( I_TP0231_SKIP_PHASE1, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	return(E_DB_OK);		/* don't do recovery */
    }

    tp2_put( I_TP0232_BEGIN_PHASE1, 0, 0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0 );

    /* 1.  build a CDB list for this installation if iidbdb is accessible;
    **     result in Tpf_svcb_p->tsv_8_cdb_ulmcb */

    if ( (status = tp2_r3_bld_cdb_list(v_tpr_p)) == E_DB_OK )
    {
	if (lmcb_p->tlm_2_cnt <= 0)
	{

	    /* empty CDB list */

	    if (lmcb_p->tlm_1_streamid != NULL) /* assert */
		return( tpd_u2_err_internal(v_tpr_p) );

	    return(E_DB_OK);	/* return for phase 2 */
	}

	/* 2.  query every CDB's iidd_ddb_dxlog to determine the number of
	**	   orphan DXs for recordation in the CDB's CB */

	if ( (status = tp2_r10_get_dx_cnts(v_tpr_p, & recovery_b)) == E_DB_OK )
	{
	    if (! recovery_b)
	    {
		/* no recovery is necessary */

		tp2_put( I_TP0233_NO_REC, 0, 0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    }
	    else
	    {

		/* 4.  replicate the CDB list in a search list (stream released if error)
		*/

		if ( (status = tp2_r11_bld_srch_list(v_tpr_p)) == E_DB_OK )
		{
		    tp2_put( I_TP0235_END_PHASE1, 0, 0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
		    Tpf_svcb_p->tsv_4_flags |= TSV_01_IN_RECOVERY;
		    v_tpr_p->tpr_need_recovery = TRUE;
		    return(E_DB_OK);	/* return for phase 2 */
		}
	    }
	}
    }

    /* 5.  dismantle the CDB and search lists and hence their streams */

    /* Save error code, if any */
    err_code = v_tpr_p->tpr_error.err_code;

    ignore = tp2_u7_term_lm_cb(lmcb_p, v_tpr_p);    /* ignore error */
	
    ignore = tp2_u7_term_lm_cb(srchcb_p, v_tpr_p);  /* ignore error */

    if (status)
	tp2_put( E_TP0234_FAIL_REP_CDB, 0, 1, 
	    sizeof(err_code),(PTR)&err_code,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    else
	tp2_put( I_TP0236_COMPLETE_PHASE1 , 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    return(status);	
}  


/*{
** Name: tp2_r9_p2_recover - recovery phase 2
**
**  External call: tpf_call(TPF_P2_RECOVER, & tpr_cb);
** 
** Description: 
**	This routine does the hard work of closing all the DDBs that are
**	left behind by defunct STAR server(s) in the current installation.
**
**	Algorithm:
**
**	1.  Retrieve from iistar_cdbs in iidbdb and buffer the information of 
**	    all the CDBs.
**	    
**	2.  Loop at most 3 times through every CDB to look for orphan DXs to
**	    perform recovery for each DX:
**
**	    2.1  Restart all the sites involved in the current orphan DX.
**	    2.2  Close current orphan DX only all/some sites have been 
**		 restarted and recovery is possible; give up and move on 
**		 to the next DX otherwise.
** Inputs: 
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	1.  Tpf_svcb_p->tsv_4_flags
**		.ts4_2_i4		TSV_00_FLAG_NIL
**	2.  Tpf_svcb_p->tsv_8_cdb_ulmcb	CDB list stream released
**					
**	tpr_cb->
**	    tpf_error			tpf error if any
**
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    None
**
** Side Effects:
**	none
**
** History:
**      jan-28-90 (carl)
**          created
*/


DB_STATUS
tp2_r9_p2_recover(
	TPR_CB	*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK,
		ignore;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
    TPD_LM_CB	*srchcb_p = & Tpf_svcb_p->tsv_12_srch_ulmcb;
    ULM_RCB	ulm;
    i4		i = 0;
    bool	retry_b = TRUE;	/* assume retry necessary */


    if (Tpf_svcb_p->tsv_2_skip_recovery)
    {
/*
	Tpf_svcb_p->tsv_4_flags.ts4_2_i4 = TSV_00_FLAG_NIL;
*/
	tp2_put( E_TP0237_SKIP_PHASE2, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	return(E_DB_OK);		/* don't do recovery */
    }
/*
    if (! (Tpf_svcb_p->tsv_4_flags.ts4_2_i4 & TSV_01_TO_RECOVER))
	return( tpd_u2_err_internal(v_tpr_p) );
*/
					/* must have already been set */
/*
    if (! (Tpf_svcb_p->tsv_4_flags.ts4_2_i4 & TSV_02_IN_RECOVERY))
	return( tpd_u2_err_internal(v_tpr_p) );
*/
					/* must have already been set */
    /* 1.  CDB list must have already been built */

    if (lmcb_p->tlm_2_cnt <= 0)
	return( tpd_u2_err_internal(v_tpr_p) );

    if (lmcb_p->tlm_1_streamid == NULL) /* assert */
	return( tpd_u2_err_internal(v_tpr_p) );

    tp2_put( I_TP0238_BEGIN_PHASE2, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

    /* 2.  loop at most 3 times to recover the DDBs */

    retry_b = TRUE;   /* assume necessary */
    for (i = 1; i < (TPD_0SVR_RECOVER_MAX + 1) && retry_b; ++i)
    {
	tp2_put( I_TP0200_BEGIN_ROUND_REC, 0, 1,
	    sizeof(i), (PTR)&i,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	status = tp2_r1_recover_all_ddbs(v_tpr_p);
	if (status == E_DB_OK)
	{
	    tp2_put(I_TP0201_END_ROUND_REC, 0, 1, sizeof(i), (PTR)&i,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0,
		0, (PTR)0 );

	    retry_b = FALSE;	/* terminate loop */
	}
	else 
	{
	    if (status == E_DB_FATAL)
	    {
		tp2_put(E_TP0202_FAIL_FATAL_REC, 0, 2, sizeof(i), (PTR)&i,
		    sizeof(v_tpr_p->tpr_error.err_code),
		    (PTR)&v_tpr_p->tpr_error.err_code,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
 
		goto dismantle_9;
	    }
	    else
	    {
		tp2_put(E_TP0203_FAIL_REC, 0, 2, sizeof(i), (PTR)&i,
		    sizeof(v_tpr_p->tpr_error.err_code),
		    (PTR)&v_tpr_p->tpr_error.err_code,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    }
	}
    }   

dismantle_9:

    /* 3.  dismantle the DDB (CDB) list */

    tpf_u1_p_sema4(TPD_1SEM_EXCLUSIVE, & Tpf_svcb_p->tsv_11_srch_sema4);
    status = tp2_u7_term_lm_cb(srchcb_p, v_tpr_p);
    tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);

    ignore = tp2_u7_term_lm_cb(lmcb_p, v_tpr_p);

    if (status)
	tp2_put( I_TP0240_COMPLETE_PHASE2, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
    else
	tp2_put( I_TP0239_END_PHASE2, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

    Tpf_svcb_p->tsv_4_flags &= ~TSV_01_IN_RECOVERY;
    if (status == E_DB_FATAL)
	return(E_DB_FATAL);
    else
	return(E_DB_OK);    /* return OK to open up for business */
}  


/*{
** Name: tp2_r10_get_dx_cnts - query iidd_ddb_dxlog for DX count on each CDB
**
** Description: 
**	A non-zero DX count is reflected on tcd_2_flags in the CDB's CB as 
**	TCD_01_TO_RECOVER.
**
** Inputs: 
**	v1_cdcb_p->			ptr to CDB CB
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	v_tpr_p->
**	    tpf_error			tpf error info
**	Tpf_svcb_p->
**	    tsv_8_cdb_ulmcb
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
**      jan-28-90 (carl)
**          created
**      may-24-90 (carl)
**	    modified to press on if a CDB is inaccesible
*/


DB_STATUS
tp2_r10_get_dx_cnts(
	TPR_CB	    *v_tpr_p,
	bool	    *o1_recovery_p)
{
    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK,
		sts_commit = E_DB_OK,
		sts_flush = E_DB_OK,
		sts_ignore;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
    TPD_CD_CB   *cdcb_p = (TPD_CD_CB *) NULL;
    DD_LDB_DESC	cdb,
		*cdb_p = & cdb;
    i4		dx_cnt = 0,
    		dxlog_cnt = 0;
    RQR_CB	rqr_cb;
    TPC_D1_DXLOG
		dxlog,
		*dxlog_p = & dxlog;
    TPQ_QRY_CB	qcb,
		*qcb_p = & qcb;
    bool	strs_ok = FALSE;	    /* must initialize */
    DB_DB_STR	ddb_name;
/*
    DB_OWN_STR	ddb_owner;
    DB_DB_STR   cdb_name;
    DB_OWN_STR	cdb_owner;
    DB_TYPE_STR	cdb_dbms;
    DB_NODE_STR cdb_node;
*/

    *o1_recovery_p = FALSE;  /* assume */

    if (lmcb_p->tlm_1_streamid == (PTR) NULL)
	return( tpd_u2_err_internal(v_tpr_p) );

    cdcb_p = (TPD_CD_CB *) lmcb_p->tlm_3_frst_ptr;
    while (cdcb_p != (TPD_CD_CB *) NULL && status == E_DB_OK)
    {
	/* must initialize for each loop iteration */

    	strs_ok = FALSE;	
	dxlog_cnt = 0;

	/* 0.  make sure CDB has the 2PC iidd_ddb_dxlog catalog */

	(VOID) tp2_u6_cdcb_to_ldb(cdcb_p, cdb_p);

	MEfill(sizeof(qcb), '\0', (PTR) & qcb);

	qcb_p->qcb_1_can_id = SEL_60_TABLES_FOR_DXLOG;
	qcb_p->qcb_3_ptr_u.u0_ptr = (PTR) NULL;
	qcb_p->qcb_4_ldb_p = cdb_p;
	status = tpq_s4_prepare(v_tpr_p, qcb_p);
	if (status == E_DB_OK)
	{
	    if (! qcb_p->qcb_5_eod_b)
		sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

	    dxlog_cnt = qcb_p->qcb_10_total_cnt;
	}
	MEcopy(cdcb_p->tcd_1_starcdbs.i1_1_ddb_name, DB_DB_MAXNAME, ddb_name);
	ddb_name[DB_DB_MAXNAME] = EOS;
	STtrmwhite(ddb_name);

/*
	MEcopy(cdcb_p->tcd_1_starcdbs.i1_2_ddb_owner, DB_OWN_MAXNAME, ddb_owner);
	ddb_owner[DB_OWN_MAXNAME] = EOS;
	STtrmwhite(ddb_owner);

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_3_cdb_name, DB_DB_MAXNAME, cdb_name);
	cdb_name[DB_DB_MAXNAME] = EOS;
	STtrmwhite(cdb_name);

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_5_cdb_node, DB_NODE_MAXNAME, cdb_node);
	cdb_node[DB_NODE_MAXNAME] = EOS;
	STtrmwhite(cdb_node);

	MEcopy(cdcb_p->tcd_1_starcdbs.i1_6_cdb_dbms, DB_TYPE_MAXLEN, cdb_dbms);
	cdb_dbms[DB_TYPE_MAXLEN] = EOS;
	STtrmwhite(cdb_dbms);
*/
	if (dxlog_cnt <= 0)
	{
/*
	    tpd_u13_trim_ddbstrs(& cdcb_p->tcd_1_starcdbs,
		ddb_name, ddb_owner, cdb_name, cdb_node, cdb_dbms);
*/
	    strs_ok = TRUE;

	    /* generate a warning message */

	    if (status)
		tp2_put( E_TP0241_FAIL_ACCESS_CDB, 0, 0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    else
		tp2_put( E_TP0242_FAIL_CATALOG_DDB, 0, 0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    /* complete the message by displaying the DDB identity */

	    tpd_p8_prt_ddbdesc(v_tpr_p, & cdcb_p->tcd_1_starcdbs, 
		    TPD_1TO_TR);
/*
	    tp2_u13_log("%s: ...(1)  DDB owner %s\n", 
		IITP_20_2pc_recovery_p,
		ddb_owner);
	    tp2_u13_log("%s: ...(2)  CDB name %s\n", 
		IITP_20_2pc_recovery_p,
		cdb_name);
	    tp2_u13_log("%s: ...(3)  CDB node %s\n", 
		IITP_20_2pc_recovery_p,
		cdb_node);
	    tp2_u13_log("%s: ...(4)  CDB DBMS %s\n", 
		IITP_20_2pc_recovery_p,
		cdb_dbms);
*/	    
	    cdcb_p->tcd_2_flags = TPD_0CD_FLAG_NIL;

	    sts_ignore = tpd_u11_term_assoc(cdb_p, v_tpr_p);
						/* no longer needed */
	}
	else
	{	
	    /* 1.  retrieve the entry count from iidd_ddb_dxlog */

	    MEfill(sizeof(qcb), '\0', (PTR) & qcb);

	    qcb_p->qcb_1_can_id = SEL_10_DXLOG_CNT;
	    qcb_p->qcb_3_ptr_u.u1_dxlog_p = dxlog_p;
	    qcb_p->qcb_4_ldb_p = cdb_p;
	    status = tpq_s4_prepare(v_tpr_p, qcb_p);
	    if (status)
	    {
/*
		if (! strs_ok)
		    tpd_u13_trim_ddbstrs(& cdcb_p->tcd_1_starcdbs,
			ddb_name, ddb_owner, cdb_name, cdb_node, cdb_dbms);
*/
		tp2_put( E_TP0241_FAIL_ACCESS_CDB, 0, 0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );

		/* complete the message by displaying the DDB identity */

		tpd_p8_prt_ddbdesc(v_tpr_p, & cdcb_p->tcd_1_starcdbs, 
		    TPD_1TO_TR);
/*
		tp2_u13_log("%s: ...   1) DDB owner %s\n", 
		    IITP_20_2pc_recovery_p,
		    ddb_owner);
		tp2_u13_log("%s: ...   2) CDB name %s\n", 
		    IITP_20_2pc_recovery_p,
		    cdb_name);
		tp2_u13_log("%s: ...   3) CDB node %s\n", 
		    IITP_20_2pc_recovery_p,
		    cdb_node);
		tp2_u13_log("%s: ...   4) CDB DBMS %s\n", 
		    IITP_20_2pc_recovery_p,
		    cdb_dbms);
*/
		cdcb_p->tcd_2_flags = TPD_0CD_FLAG_NIL;

	    }
	    else
	    {
		if (! qcb_p->qcb_5_eod_b)
		    sts_flush = tpq_s3_flush(v_tpr_p, qcb_p);

		dx_cnt = qcb_p->qcb_10_total_cnt;

/*
		if (! strs_ok)
		    tpd_u13_trim_ddbstrs(& cdcb_p->tcd_1_starcdbs,
			    ddb_name, ddb_owner, cdb_name, cdb_node, cdb_dbms);
*/
		tp2_put( E_TP020C_ORPHAN_DX_FOUND, 0, 2,
		    sizeof( dx_cnt ), (PTR)&dx_cnt,
		    sizeof(ddb_name), (PTR)ddb_name,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
	    }
	    if (dx_cnt <= 0)
	    {
		cdcb_p->tcd_2_flags = TPD_0CD_FLAG_NIL;
					/* nothing to recover */
		status = tpd_u11_term_assoc(cdb_p, v_tpr_p);
	    }
	    else
	    {
		cdcb_p->tcd_2_flags |= TPD_1CD_TO_RECOVER;
		*o1_recovery_p = TRUE;
		/* Don't try COMMIT if not 2PC */
		if ( cdb_p->dd_l6_flags & DD_02FLAG_2PC )
		    sts_commit = tp2_u3_msg_to_cdb(TRUE, cdb_p, v_tpr_p);
			    /* TRUE to commit all queries on iidd_ddb_dxlog */
	    }
	}
	cdcb_p = cdcb_p->tcd_4_next_p;
    }

    /* done with CDB list */

    return(E_DB_OK);
}


/*{
** Name: tp2_r11_bld_srch_list - build a CDB search list 
**
** Description: 
**	This routine replicates the CDBs in a search list.
**
**	A stream (tsv_12_srch_ulmcb) is opened for allocating space for the 
**	CDB search list.  It will be closed by the caller when done.
**
** Inputs: 
**	v_tpr_p->			TPR_CB pointer
**	    tpr_session	                TPF's restart session CB
**	    tpr_rqf		        RQF's restart session CB
**	    tpr_site                    LDB descriptor for iidbdb
** Outputs:
**	v_tpr_p->
**	    tpf_error			tpf error info
**	Tpf_svcb_p->
**	    tsv_12_srch_ulmcb
**		.tlm_1_streamid		id of stream opened for list
**		.tlm_2_cnt		number of CDB CBs in the list
**		.tlm_3_frst_ptr		ptr to the first CB in the list
**		.tlm_4_last_ptr		ptr to the last CB in the list
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
**      jan-28-90 (carl)
**          created
*/


DB_STATUS
tp2_r11_bld_srch_list(
	TPR_CB	*v_tpr_p)
{
    char	dummy_1[20000];

    DB_STATUS	status = E_DB_OK,
		prev_sts = E_DB_OK;
    DB_ERROR	prev_err;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*cdblm_p = & Tpf_svcb_p->tsv_8_cdb_ulmcb;
    TPD_LM_CB	*srchcb_p = & Tpf_svcb_p->tsv_12_srch_ulmcb;
    TPD_CD_CB	*cdcb_p1 = (TPD_CD_CB *) NULL;	    /* ptr to CDB list */
    TPD_CD_CB	*cdcb_p2 = (TPD_CD_CB *) NULL;	    /* ptr to search list */
    ULM_RCB	ulm;
    i4		srchcb_cnt = 0;


    if (cdblm_p->tlm_2_cnt <= 0)
	return( tpd_u2_err_internal(v_tpr_p) );

    if (srchcb_p->tlm_1_streamid != (PTR) NULL)
	return( tpd_u2_err_internal(v_tpr_p) );

    tp2_put( I_TP0243_BEGIN_SEARCH_DDB, 0, 0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0 );

    /* 1.1  allocate a private CDB stream */

    MEfill(sizeof(ulm), '\0', (PTR) & ulm);

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_blocksize = sizeof(TPC_I1_STARCDBS);    /* preferred size */
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    ulm.ulm_streamid_p = &ulm.ulm_streamid;
    ulm.ulm_flags = ULM_PRIVATE_STREAM;
    status = ulm_openstream(& ulm);
    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0015_ULM_OPEN_FAILED, v_tpr_p) );

    /* 1.2  initialize the ulm cb */

    srchcb_p->tlm_1_streamid = ulm.ulm_streamid;
    srchcb_p->tlm_2_cnt = 0;
    srchcb_p->tlm_3_frst_ptr = 
	srchcb_p->tlm_4_last_ptr = (PTR) NULL;

    /* 4.  loop to build search list */

    srchcb_cnt = 0;
    cdcb_p1 = (TPD_CD_CB *) cdblm_p->tlm_3_frst_ptr;
    while (cdcb_p1 != (TPD_CD_CB *) NULL && status == E_DB_OK)
    {
	/* 4.1  create a new search CDB CB */

	status = tpd_u6_new_cb(TPD_5LM_SR_CB, v_tpr_p, (PTR *) & cdcb_p2);
	if (status == E_DB_OK)
	{
	    /* 4.2  move information from iistar_cdbs entry to CDB CB */

	    cdcb_p2->tcd_1_starcdbs = cdcb_p1->tcd_1_starcdbs;
	    cdcb_p2->tcd_2_flags = cdcb_p1->tcd_2_flags;
	}
	cdcb_p1 = cdcb_p1->tcd_4_next_p;
	srchcb_cnt++;
    }

    if (status == E_DB_OK)
    {
	if (srchcb_cnt != cdblm_p->tlm_2_cnt)
	    return( tpd_u2_err_internal(v_tpr_p) );

	tp2_put( I_TP0244_END_SEARCH_DDB, 0, 0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );

	return(E_DB_OK);
    }	    

dismantle_11:

    if (status)
    {
	tp2_put( E_TP0245_FAIL_SEARCH_DDB, 0, 1,
	    sizeof( v_tpr_p->tpr_error.err_code ),
	    (PTR)&v_tpr_p->tpr_error.err_code,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0,
	    0, (PTR)0 );
	prev_sts = status;
	STRUCT_ASSIGN_MACRO(v_tpr_p->tpr_error, prev_err);
    }

    /* 5.  here to dismantle open CDB stream */

    status = tp2_u7_term_lm_cb(srchcb_p, v_tpr_p);
/*
    ** commented code left here for reference **

    ulm.ulm_facility = DB_QEF_ID;
    ulm.ulm_poolid = Tpf_svcb_p->tsv_6_ulm_poolid;
    ulm.ulm_streamid_p = &srchcb_p->tlm_1_streamid;
    ulm.ulm_memleft = & Tpf_svcb_p->tsv_7_ulm_memleft.ts4_2_i4;
    status = ulm_closestream(& ulm);

    srchcb_p->tlm_2_cnt = 0;
    srchcb_p->tlm_3_frst_ptr = srchcb_p->tlm_4_last_ptr = NULL;
*/

    if (prev_sts)
    {
	status = prev_sts;
	STRUCT_ASSIGN_MACRO(prev_err, v_tpr_p->tpr_error);
	return(status);
    }

    if (status != E_DB_OK)
	return( tpd_u1_error(E_TP0017_ULM_CLOSE_FAILED, v_tpr_p) );

    return(E_DB_OK);
}


/*{
** Name: tp2_r12_set_ddb_recovered - set DDB's status to recovered in search 
**				     list
**
** Description: 
**	Scan the search CDB list to set the status of given DDB to recovered.
**
** Inputs:
**	i1_cdcb_p		    ptr to CDB CB for searching
**	v_tpr_p			    ptr to TPR_CB
**
** Outputs:
**	none
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
tp2_r12_set_ddb_recovered(
	TPD_CD_CB	*i1_cdcb_p,
	TPR_CB		*v_tpr_p)
{
    DB_STATUS	status = E_DB_OK;   /* assume */
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*srchcb_p = & Tpf_svcb_p->tsv_12_srch_ulmcb;
    TPD_CD_CB	*cdcb_p = (TPD_CD_CB *) NULL;

    
    tpf_u1_p_sema4(TPD_1SEM_EXCLUSIVE, & Tpf_svcb_p->tsv_11_srch_sema4);
					/* get semaphore, a MUST */

    cdcb_p = (TPD_CD_CB *) srchcb_p->tlm_3_frst_ptr;	    
		
    while (cdcb_p != (TPD_CD_CB *) NULL)
    {
	if (MEcmp(i1_cdcb_p->tcd_1_starcdbs.i1_1_ddb_name,
	    cdcb_p->tcd_1_starcdbs.i1_1_ddb_name, DB_DB_MAXNAME) == 0)
	{
	    /* have a match */

	    cdcb_p->tcd_2_flags |= TPD_2CD_RECOVERED;

	    /* give up the semaphore before returning */

	    tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);	/* a MUST */

	    return(E_DB_OK);
	}
	cdcb_p = cdcb_p->tcd_4_next_p;	/* advance to next CDB */
    }
    tpf_u2_v_sema4(& Tpf_svcb_p->tsv_11_srch_sema4);	/* a MUST */

    return(E_DB_OK);
}


/*{
** Name: tp2_r13_resecure_all_ldbs - reseucre all LDBs
**
** Description:
**
**	This routine broadcasts a SECURE message to ALL the orphan DX's 
**	RESTARTED LDBs and return the polling result.  
**
**	There are 2 error situations:
**	
**	1)  An LDB refuses to commit.  ABORT is returned.
**	2)  Any other error will cause an immediate return.  
**
** Inputs:
**	i1_dxcb_p			ptr to DX conotrol block 
**	v_tpr_p				ptr to session context
**
** Outputs:
**	o1_willing_p			ptr to boolean result: TRUE if all
**					sites are willing to commit, FALSE 
**					otherwise implying that the DX must 
**					be 2pc-aborted
**	Returns:
**	    DB_STATUS			E_DB_ERROR implies failure in 1 way or
**					or another
**	Exceptions:
**	    None
**
** Side Effects:
**	    None
**
** History:
**      nov-16-89 (carl)
**          created
**      jul-07-89 (carl)
**	    changed tp2_r13_resecure_all_ldbs to use LX id instead of DX id
**	01-May-2002 (bonro01)
**	    Fix overlay caused by using wrong variable to set EOS
*/


DB_STATUS
tp2_r13_resecure_all_ldbs(
	TPD_DX_CB	*i1_dxcb_p,
	TPR_CB		*v_tpr_p,
	bool		*o1_willing_p)
{
    DB_STATUS	status = E_DB_OK;
    TPD_SS_CB	*sscb_p = (TPD_SS_CB *) v_tpr_p->tpr_session;
    TPD_LM_CB	*lmcb_p = & i1_dxcb_p->tdx_22_lx_ulmcb;
    TPD_LX_CB	*lxcb_p = (TPD_LX_CB *) NULL;
    i4		lx_cnt;
    RQR_CB	rqr_cb;
    DB_NODE_STR node_name;
    char	ldb_name[DD_256_MAXDBNAME + 1];
    DB_TYPE_STR dbms_name;


    *o1_willing_p = FALSE;			/* assume */

    MEfill(sizeof(rqr_cb), '\0', (PTR) & rqr_cb);

    rqr_cb.rqr_session = sscb_p->tss_rqf;

    tp2_put( I_TP023A_BEGIN_LDBS, 0, 2, 
	sizeof( i1_dxcb_p->tdx_id.dd_dx_id1 ),
	(PTR)&i1_dxcb_p->tdx_id.dd_dx_id1,
	sizeof( i1_dxcb_p->tdx_id.dd_dx_id2 ),
	(PTR)&i1_dxcb_p->tdx_id.dd_dx_id2,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0 );

    /* 1.  broadcast the SECURE message to all the writers */

    rqr_cb.rqr_session = sscb_p->tss_rqf;	    /* RQF ssession ptr */
    rqr_cb.rqr_timeout = 0;
/*
    rqr_cb.rqr_2pc_dx_id = & i1_dxcb_p->tdx_id;
*/
    lxcb_p = (TPD_LX_CB *) lmcb_p->tlm_3_frst_ptr;
    lx_cnt = 0;
    while (lxcb_p != (TPD_LX_CB *) NULL)
    {
/*
	** for reference **
    
	if (lxcb_p->tlx_20_flags & LX_01FLAG_REG    ** registered **
	    &&
	    lxcb_p->tlx_write			    ** an updater **
	    &&
	    lxcb_p->tlx_state == LX_7STATE_ACTIVE)
*/
	{
	    MEcopy(lxcb_p->tlx_site.dd_l2_node_name, DB_NODE_MAXNAME, node_name);
	    node_name[DB_NODE_MAXNAME] = EOS;
	    STtrmwhite(node_name);
		
	    MEcopy(lxcb_p->tlx_site.dd_l3_ldb_name, DD_256_MAXDBNAME, ldb_name);
	    ldb_name[DD_256_MAXDBNAME] = EOS;
	    STtrmwhite(ldb_name);
		
	    MEcopy(lxcb_p->tlx_site.dd_l4_dbms_name, DB_TYPE_MAXLEN, dbms_name);
	    dbms_name[DB_TYPE_MAXLEN] = EOS;
	    STtrmwhite(dbms_name);

	    /* set the LX state before polling */

	    lxcb_p->tlx_state = LX_2STATE_POLLED;

	    rqr_cb.rqr_1_site = & lxcb_p->tlx_site;
	    rqr_cb.rqr_q_language = lxcb_p->tlx_txn_type;
	    rqr_cb.rqr_2pc_dx_id = & lxcb_p->tlx_23_lxid;

	    status = tpd_u4_rqf_call(RQR_SECURE, & rqr_cb, v_tpr_p);
	    if (status == E_DB_OK)
		lxcb_p->tlx_state = LX_3STATE_WILLING;
	    else
	    {
		/* process error */

		if (rqr_cb.rqr_error.err_code == E_RQ0048_SECURE_FAILED)
		{
		    /*  SECURE refused  */

		    tp2_put( E_TP023B_REFUSE_LDBS, 0, 2, 
			sizeof( i1_dxcb_p->tdx_id.dd_dx_id1 ),
			(PTR)&i1_dxcb_p->tdx_id.dd_dx_id1,
			sizeof( i1_dxcb_p->tdx_id.dd_dx_id2 ),
			(PTR)&i1_dxcb_p->tdx_id.dd_dx_id2,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0 );

		    (VOID) tpd_u9_2pc_ldb_status(& rqr_cb, lxcb_p);
		    lxcb_p->tlx_state = LX_8STATE_REFUSED;  

		    tp2_put( I_TP023C_NODE_LDBS, 0, 3, 
			sizeof( node_name ), (PTR)node_name,
			sizeof( ldb_name), (PTR)ldb_name,
			sizeof(dbms_name ), (PTR)dbms_name,
			0, (PTR)0,
			0, (PTR)0,
			0, (PTR)0 );

		    status = E_DB_OK;
		}
		else if (rqr_cb.rqr_error.err_code == E_RQ0045_CONNECTION_LOST) 
		{
		    /* connection lost */

		    lxcb_p->tlx_state = LX_7STATE_ACTIVE;   /* restore state */
		    lxcb_p->tlx_ldb_status = LX_09LDB_ASSOC_LOST;
		}
		tp2_put( E_TP023D_FAIL_LDBS, 0, 3, 
		    sizeof( i1_dxcb_p->tdx_id.dd_dx_id1 ),
		    (PTR)&i1_dxcb_p->tdx_id.dd_dx_id1,
		    sizeof( i1_dxcb_p->tdx_id.dd_dx_id2 ),
		    (PTR)&i1_dxcb_p->tdx_id.dd_dx_id2,
		    sizeof( rqr_cb.rqr_error.err_code ),
		    (PTR)&rqr_cb.rqr_error.err_code,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );

		tp2_put( I_TP023C_NODE_LDBS, 0, 3, 
		    sizeof( node_name ), (PTR)node_name,
		    sizeof( ldb_name), (PTR)ldb_name,
		    sizeof(dbms_name ), (PTR)dbms_name,
		    0, (PTR)0,
		    0, (PTR)0,
		    0, (PTR)0 );
		return(status);	/* return error */
	    }
	}
	lxcb_p = lxcb_p->tlx_22_next_p; /* advance to next LX */
	++lx_cnt;
/*
	if (lx_cnt > 0)
	{
	    if (sscb_p->tss_4_dbg_2pc == TSS_61_CRASH_IN_SECURE)
	    {
		status = tp2_d0_crash(v_tpr_p);
		return(status);
	    }
	}
*/
    }
/*
    if (sscb_p->tss_4_dbg_2pc == TSS_62_CRASH_AFT_SECURE)
    {
	status = tp2_d0_crash(v_tpr_p);
	return(status);
    }
*/
    *o1_willing_p = TRUE;

    tp2_put( I_TP023E_END_LDBS, 0, 2, 
	sizeof( i1_dxcb_p->tdx_id.dd_dx_id1 ),
	(PTR)&i1_dxcb_p->tdx_id.dd_dx_id1,
	sizeof( i1_dxcb_p->tdx_id.dd_dx_id2 ),
	(PTR)&i1_dxcb_p->tdx_id.dd_dx_id2,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0,
	0, (PTR)0 );
    return(E_DB_OK);
}

