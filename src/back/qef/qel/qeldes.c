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
#include    <qefmain.h>
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
#include    <qefgbl.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QELDES.C - Contains routines for DROP/DESTROY and REGISTER WITH 
**		     REFRESH
**
**  Description:
**      This file contains routines for DESTROY LINK.
**
**	    qel_d0_des_lnk	- drop/destroy a link.
**	    qel_d1_des_p1	- phase 1 of DESTROY LINK
**	    qel_d2_des_p2	- phase 2 of DESTROY LINK
**	    qel_d3_objects	- retrieve from IIDD_DDB_OBJECTS
**	    qel_d4_tableinfo	- retrieve the table's information from
**				  IIDD_DDB_TABLEINFO
**	    qel_d5_ldbinfo	- retrieve the LDB's information, number of
**				  objects of LDB, and determine if has an
**				  excessively long name to be deleted
**	    qel_d6_ndxinfo	- retrieve all index names from 
**				  IIDD_DDB_LDB_INDEXES for column deletion
**	    qel_d7_iistatscnt	- retrieve number of IISTATS tuples for
**				  current LDB table 
**	    qel_d8_des_p2	- new phase 2 of DESTROY LINK
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog redefinition
**	02-apr-89 (carl)
**          modified to process REGISTER WITH REFRESH
**	30-apr-89 (carl)
**          modified qel_d2_des_p2 to implement new semantics for 
**	    REGISTER WITH REFRESH
**	20-jul-89 (carl)
**	    adapted qel_d1_des_p2 to qel_d8_des_p2 to destroy in the order 
**	    as CREATE LINK
**	28-aug-90 (carl)
**	    changed all occurrences of tpf_call to qed_u17_tpf_call
**	23-sep-90 (carl)
**	    modified qel_d0_des_lnk to process trace point
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      10-sep-93 (smc)
**          Corrected cast of qec_tprintf to PTR & used %p.
**          Added <cs.h> for CS_SID.
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
**      09-nov-94 (brogr02)
**          Register with TPF as an updater if ddl concurrency is off
**	    QEF session startup time.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/

GLOBALREF   char	IIQE_61_qefsess[];
GLOBALREF   char	IIQE_62_3dots[];
GLOBALREF   char	IIQE_65_tracing[];



/*{
** Name: qel_d0_des_lnk - Drop/destroy a link.
**
**  External call:  qef_call(QEU_DBU, & rcb);
**
** Description:
**	This function destroys a link's information from all STANDARD, 
**  INDEX-related, STATISTICS-related and STAR-specific catalogs.
**
**  Notes:
**	1) All updates on the CDB are done under $ingres for Titan.
**	2) Until 2PC is implemented, CDB updates are registered with TPF as 
**	   READ-ONLY operations to avoid jeopardizing the single-site update 
**	   requirement imposed on the user.
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qef_cb
**		.qef_c2_ddb_ses
**		    .qes_d7_ses_state
**				    QES_4ST_REFRESH if REGISTER WITH REFRESH
**	    .qef_r3_ddb_req
**		.qer_d7_ddl_info    QED_DDL_INFO
**		    .qed_d7_obj_id  object id of link to be destroyed
**		    .qed_d8_obj_type	
**				    DD_1OBJ_LINK if LINK
**				    DD_2OBJ_TABLE if TABLE
**
**	
** Outputs:
**      v_qer_p
**          .error
**		.err_code	    One of the following:
**                                  E_QE0000_OK
**                                  (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR              caller error
**          E_DB_FATAL              internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	02-apr-89 (carl)
**          modified to process REGISTER WITH REFRESH
**	20-jul-89 (carl)
**	    changed to call qel_d8_des_p2 which destroys in the same order 
**	    as CREATE LINK
**	23-sep-90 (carl)
**	    modified to process trace point
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      09-nov-94 (brogr02)
**          Register with TPF as an updater if ddl concurrency is off
*/


DB_STATUS
qel_d0_des_lnk(
QEF_RCB		*v_qer_p ) 
{
    DB_STATUS		status;
    QEF_CB		*qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO	*ddl_p = & v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D9_TABLEINFO	tabinfo;	    /* used for table info of link */
    QEC_D2_LDBIDS	ldbids;		    /* used for internal LDB id info */
    QEC_D5_LONG_LDBNAMES
			longnames;	    /* used for excessively long LDB
					    ** name */
    QEC_L16_TABLES	tables;		    /* used for IITABLES-style info */
    QEC_D6_OBJECTS	objects;	    /* used object info */
    QEC_L6_INDEXES	indexes;
    QEC_MIN_CAP_LVLS	pre_mins,
			aft_mins;
    QEC_LINK		link,		    /* used as global control block */
			*link_p = & link;
    QEQ_1CAN_QRY	del,
			sel,
			upd;		    /* used for ordering canned query */
    bool		log_ddl_56 = FALSE,
			log_err_59 = FALSE;
    i4		i4_1,
			i4_2;
    QEC_INDEX_ID	ndx_ids[QEK_0MAX_NDX_COUNT + 1];
					    /* working space for index ids */
    RQB_BIND		rq_bind[QEC_CAT_COL_COUNT_MAX + 1];
    char		*cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4			cbufsize = v_qer_p->qef_cb->qef_trsize;
    i4                  xact_mode;


    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_DDL_56,
	    & i4_1, & i4_2))
    {
        log_ddl_56 = TRUE;
    }
    
    if (ult_check_macro(& qef_cb->qef_trace, QEF_TRACE_DDB_LOG_ERR_59,
	    & i4_1, & i4_2))
    {
        log_err_59 = TRUE;
    }
    
    sel.qeq_c2_rqf_bind_p = rq_bind;		/* must set up */
    del.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;
    upd.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;
    
    /* 1.  set up control block */

    link_p->qec_1_ddl_info_p = ddl_p;
    link_p->qec_2_tableinfo_p = & tabinfo;
    link_p->qec_3_ldb_id = 0;			
    link_p->qec_4_col_cnt = 0;
    MEfill(DB_MAXNAME, ' ', link_p->qec_5_ldb_alias);
    link_p->qec_6_select_p = & sel;
    link_p->qec_7_ldbids_p = & ldbids;
    link_p->qec_8_longnames_p = & longnames;
    link_p->qec_9_tables_p = & tables;
    link_p->qec_10_haves = 0;
    link_p->qec_11_ldb_obj_cnt = 0;
    link_p->qec_12_indexes_p = & indexes;
    link_p->qec_13_objects_p = & objects;
    link_p->qec_15_ndx_cnt = 0;
    link_p->qec_16_ndx_ids_p = ndx_ids;

    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH)
	link_p->qec_19_ldb_p = 
	    & ddl_p->qed_d6_tab_info_p->dd_t9_ldb_p->dd_i1_ldb_desc;
    else
	link_p->qec_19_ldb_p = (DD_LDB_DESC *) NULL;

    link_p->qec_20_rqf_bind_p = rq_bind;
    link_p->qec_21_delete_p = & del;
    link_p->qec_22_insert_p = NULL;
    link_p->qec_23_update_p = & upd;

    status = qed_u8_gmt_now(v_qer_p, link_p->qec_24_cur_time);
    if (status)
	return(status);

    link_p->qec_24_cur_time[DD_25_DATE_SIZE] = EOS;
    link_p->qec_25_pre_mins_p = & pre_mins;
    link_p->qec_26_aft_mins_p = & aft_mins;
    link_p->qec_28_iistats_cnt = 0;		
    
    /* 2. inform TPF of update intention of CDB */

    if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON)
        xact_mode = QEK_4TPF_1PC;
    else
        xact_mode = QEK_3TPF_UPDATE;

    status = qet_t5_register(v_qer_p, cdb_p, DB_SQL, xact_mode);
    if (status)
    {
	return(status);
    }

    if (log_ddl_56)
    {
	/* display beginning and CDB info */

	STprintf(cbuf, 
	    "\n%s %p: %s DE-REGISTRATION begins%s\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    IIQE_65_tracing,
	    IIQE_62_3dots);
        qec_tprintf(v_qer_p, cbufsize, cbuf);

	STprintf(cbuf, 
	    "%s %p: ...the CDB:\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
	qed_w1_prt_ldbdesc(v_qer_p, cdb_p);

	STprintf(cbuf, "\n");
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    /*	3.  retrieve object's information from IIDD_DDB_OBJECTS */

    status = qel_d3_objects(v_qer_p, link_p);
    if (status)
	goto END_TRACING;

    if (link_p->qec_27_select_cnt <= 0)
    {
	/* the object no longer exists possibly a casualty of cascading 
	** delete, hence do forgiving return */

	goto END_TRACING;
    }

    /*  4.  enter phase 1: set up necessary information */

    status = qel_d1_des_p1(v_qer_p, link_p);
    if (status)
	goto END_TRACING;

    /*  5.  enter phase 2: remove information from CDB catalogs */

    status = qel_d8_des_p2(v_qer_p, link_p);
    if (status)
    {
	/* forgive error */

	status = qed_u9_log_forgiven(v_qer_p);
    }

END_TRACING:

    if (log_ddl_56)
    {
	/* display end tracing message */

	STprintf(cbuf, 
	    "%s %p: %s DE-REGISTRATION ends\n\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    IIQE_65_tracing);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    return(status);
}


/*{
** Name: qel_d1_des_p1 - Phase 1 of DESTROY/DROP LINK.
**
** Description:
**	This function sets up all necessary information to facilitate phase 2 
**  for deleting from STAR catalogs.
**
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
** Outputs:
**	v_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	10-mar-89 (carl)
**	    moved call to retrieve object id to qel_d0_des_lnk
**	02-apr-89 (carl)
**          modified to process REGISTER WITH REFRESH
**	21-dec-92 (teresa)
**	    support remove of regproc.
*/


DB_STATUS
qel_d1_des_p1(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
/*
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
*/
    QEC_D2_LDBIDS   *ldbids_p = v_lnk_p->qec_7_ldbids_p;
    bool	    regproc;


    /*	1.  retrieve from IIDD_DDB_TABLEINFO */

    status = qel_d4_tableinfo(v_qer_p, v_lnk_p);	
    if (status)
	return(status);
    else if (v_lnk_p->qec_27_select_cnt > 0)
	v_lnk_p->qec_3_ldb_id = v_lnk_p->qec_2_tableinfo_p->d9_10_ldb_id;
    else
	v_lnk_p->qec_3_ldb_id = 0;		/* inconsistent, press on */

    /*	2.  retrieve from IIDD_DDB_LDBIDS */

    status = qel_d5_ldbinfo(v_qer_p, v_lnk_p);	
    if (status)
	return(status);
    else if (v_lnk_p->qec_27_select_cnt > 0)
    {
	/*  3.1  determine if LDB is INGRES */

	if (
	    (MEcmp(
		ldbids_p->d2_2_ldb_dbms, 
		IIQE_c6_cap_ingres, 
		QEK_6_SIZE_OF_INGRES) == 0)
	    ||
	    (MEcmp(
		ldbids_p->d2_2_ldb_dbms, 
		IIQE_c5_low_ingres, 
		QEK_6_SIZE_OF_INGRES) == 0)
	    )
	    v_lnk_p->qec_10_haves |= QEC_03_INGRES;

	/*  3.2  determine if LDB has a long name */

	if (ldbids_p->d2_4_ldb_longname[0] == 'Y')
	    v_lnk_p->qec_10_haves |= QEC_04_LONG_LDBNAME;
    }

    /*	4.  retrieve index information from IIDD_DDB_LDB_INDEXES */

    status = qel_d6_ndxinfo(v_qer_p, v_lnk_p);	
    if (status)
	return(status);

    /*	5.  determine if LDB table has any statistics to refresh */

    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH)
    {
	status = qel_d7_iistatscnt(v_qer_p, v_lnk_p);	
	if (status)
	    return(status);

	if (v_lnk_p->qec_28_iistats_cnt > 0)
	    v_lnk_p->qec_10_haves |= QEC_05_STATS;
    }

    /* Determine if this is a registered procedure */
    regproc = (v_lnk_p->qec_13_objects_p->d6_6_obj_type[0]=='L') 
	      && 
	      (v_lnk_p->qec_2_tableinfo_p->d9_2_lcl_type[0]=='P');

    return(status);
}


/*{
** Name: qel_d2_des_p2 - Phase 2 of DESTROY/DROP LINK.
**
** Description:
**	This function deletes from all INDEX-related, STATISTICS-related,
**  STANDARD and STAR-specific catalogs for a link in a very forgiving 
**  manner.  Such a policy is necessary to make DESTROY/DROP LINK to
**  get rid of problematic or corrupted links that may have inconsistent 
**  or incomplete information.
**
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
** Outputs:
**	v_qer_p				
**	    .error
**		.err_code		    always E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	02-apr-89 (carl)
**          modified to process REGISTER WITH REFRESH
**	30-apr-89 (carl)
**          more modification to implement new semantics for 
**	    REGISTER WITH REFRESH
*/


DB_STATUS
qel_d2_des_p2(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	     status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = & v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    bool	    drop_stats_b = TRUE;	/* assume NOT refresh */


    /* 1.  always delete from index catalogs */

    if (v_lnk_p->qec_15_ndx_cnt > 0)
    {
	status = qel_71_ndx_cats(v_qer_p, v_lnk_p);	
	if (status)
	{
	    status = qed_u9_log_forgiven(v_qer_p);
	    if (status)
		return(status);
	}
    }

    /* 2.  drop current statistics and refresh HISTOGRAMS/STATISTICS catalogs 
    **	   only if the LDB table comes with statistics as informed */

    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH
	&&
	!( ddl_p->qed_d10_ctl_info & QED_1DD_DROP_STATS))
	drop_stats_b = FALSE;		/* LDB table has no statistics,
					** save whatever is in STAR */

    if (drop_stats_b)
    {
	status = qel_61_his_cats(v_qer_p, v_lnk_p);	
	if (status)
	{
	    status = qed_u9_log_forgiven(v_qer_p);
	    if (status)
		return(status);
	}
    }

    /* 3.  always delete from all STANDARD catalogs */

    status = qel_81_std_cats(v_qer_p, v_lnk_p);	
    if (status)
    {
	status = qed_u9_log_forgiven(v_qer_p);
	if (status)
	    return(status);
    }

    /* 4.  always delete from all DDB-specific catalogs */

    status = qel_51_ddb_cats(v_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(v_qer_p);
	if (status)
	    return(status);
    }

    /* 5.  delete all dependent views if not REGISTER WITH REFRESH */

    if (! (dds_p->qes_d7_ses_state == QES_4ST_REFRESH))
    {
	/* not REGISTER WITH REFRESH */

	status = qeu_21_des_depends(v_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(v_qer_p);
	    if (status)
		return(status);
	}
    }

    return(E_DB_OK);
}


/*{
** Name: qel_d3_objects - Retrieve a link's object information
**
** Description:
**      This routine retrieves the link's information from IIDD_DDB_OBJECTS.
**
** Inputs:
**	    v_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
**	    v_lnk_p			QEC_LINK
**		.qec_1_ddl_info_p	QED_DDL_INFO
**		    .qed_d7_obj_id	link's id
** Outputs:
**	    v_lnk_p			QEC_LINK
**		.qec_13_objects_p
**		    .d6_1_obj_name
**		    .d6_2_obj_owner
**	    v_qer_p
**		.error
**		    .err_code		E_QE0002_INTERNAL_ERROR
**					(to be specified)
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jun-88 (carl)
**          written
*/


DB_STATUS
qel_d3_objects(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;


    /* 1.  retrieve from IIDD_DDB_OBJECTS */

    sel_p->qeq_c1_can_id = SEL_035_DD_DDB_OBJECTS_BY_ID;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.d6_objects_p = objects_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    /* 2.  provide object id as keying information */

    STRUCT_ASSIGN_MACRO(ddl_p->qed_d7_obj_id, objects_p->d6_3_obj_id);

    /* 3.  retrieve information of link being destroyed */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {	
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    return(E_DB_OK);
}


/*{
** Name: qel_d4_tableinfo - Retrieve the link's table information from 
**			    IIDD_DDB_TABLEINFO
** Description:
**      This routine retrieves information from above catalog.
**
** Inputs:
**	    v_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
** Outputs:
**	    v_lnk_p			QEC_LINK
**		
**	    v_qer_p
**		.error
**		    .err_code		E_QE0002_INTERNAL_ERROR
**					(to be specified)
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jun-88 (carl)
**          written
*/


DB_STATUS
qel_d4_tableinfo(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;


    /* 1.  set up for retrieving from IIDD_DDB_TABLEINFO */

    sel_p->qeq_c1_can_id = SEL_018_DD_DDB_TABLEINFO;
    sel_p->qeq_c3_ptr_u.d9_tableinfo_p = tabinfo_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;

    /* 2.  provide keying information */

    STRUCT_ASSIGN_MACRO(
	v_lnk_p->qec_13_objects_p->d6_3_obj_id,
	tabinfo_p->d9_1_obj_id);

    /* 3.  generate and send retrieve query */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
/*


    if (v_lnk_p->qec_27_select_cnt != 1)
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }
*/
    return(E_DB_OK);
}


/*{
** Name: qel_d5_ldbinfo	- Retrieve LDB's long name info from 
**			  IIDD_DDB_LONG_LDBNAMES and its number of objects 
**			  remaining in the DDB.
**
** Description:
**      This routine retrieves the LDB's long name information from
**  IIDD_DDB_LDBIDS and LDB's number of objects in the DDB from
**  IIDD_DDB_TABLEINFO.
**
** Inputs:
**	    v_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
** Outputs:
**	    v_lnk_p			QEC_LINK
**		
**	    v_qer_p
**		.error
**		    .err_code		E_QE0002_INTERNAL_ERROR
**					(to be specified)
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      30-jun-88 (carl)
**          written
*/


DB_STATUS
qel_d5_ldbinfo(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_D2_LDBIDS   *ldbids_p = v_lnk_p->qec_7_ldbids_p;
    QEC_D9_TABLEINFO
		    tab_info,
		    *tabinfo_p = & tab_info;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;


    /* 1.1  set up for retrieving from IIDD_DDB_LDBIDS */

    sel_p->qeq_c1_can_id = SEL_006_DD_DDB_LDBIDS_BY_ID;
    sel_p->qeq_c3_ptr_u.d2_ldbids_p = ldbids_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;

    /* 1.2  provide ldb id as keying information */

    ldbids_p->d2_5_ldb_id = v_lnk_p->qec_2_tableinfo_p->d9_10_ldb_id;

    /* 1.3  generate and send retrieve query */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    
    if (v_lnk_p->qec_7_ldbids_p->d2_4_ldb_longname[0] == 'Y'
	||
	v_lnk_p->qec_7_ldbids_p->d2_4_ldb_longname[0] == 'y')
	v_lnk_p->qec_10_haves |= QEC_04_LONG_LDBNAME;

    /* 2.1  set up for retrieving from IIDD_DDB_TABLEINFO */

    sel_p->qeq_c1_can_id = SEL_036_DD_LDB_OBJECT_COUNT;
    sel_p->qeq_c3_ptr_u.d9_tableinfo_p = tabinfo_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;

    /* 1.2  provide ldb id as keying information */

    tabinfo_p->d9_10_ldb_id = v_lnk_p->qec_2_tableinfo_p->d9_10_ldb_id;

    /* 1.3  generate and send retrieve query */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }
    
    if (v_lnk_p->qec_27_select_cnt != 1)
    {
	status = qed_u1_gen_interr(& v_qer_p->error);
	return(status);
    }

    return(E_DB_OK);
}


/*{
** Name: qel_d6_ndxinfo - Retrieve information about all indexes of a link.
**
** Description:
**	This routine retrieves for a link all the index information for 
**  deletion.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**
** Outputs:
**	v_lnk_p
**	    .qec_15_item_cnt	    number of indexes of table
**	    .qec_16_ndx_ids_p	    index ids stored in array
**
**	v_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
*/


DB_STATUS
qel_d6_ndxinfo(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L6_INDEXES  *indexes_p = v_lnk_p->qec_12_indexes_p; 
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    QEC_INDEX_ID    *xid_p = v_lnk_p->qec_16_ndx_ids_p;


    /* 1.  set up for retrieveing from IIDD_INDEXES */

    v_lnk_p->qec_15_ndx_cnt = 0;

    sel_p->qeq_c1_can_id = SEL_021_DD_INDEXES;
    sel_p->qeq_c3_ptr_u.l6_indexes_p = indexes_p;
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;

    qed_u0_trimtail(
	    v_lnk_p->qec_13_objects_p->d6_1_obj_name,
	    (u_i4) DB_MAXNAME,
	    indexes_p->l6_4_base_name);

    qed_u0_trimtail(
	    v_lnk_p->qec_13_objects_p->d6_2_obj_owner,
	    (u_i4) DB_MAXNAME,
	    indexes_p->l6_5_base_owner);

    /* 2.  send SELECT query */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);
        
    if (sel_p->qeq_c5_eod_b)
	return(status);

    while (! sel_p->qeq_c5_eod_b && ! status)
    {
	/* 2.1  save index name and owner */

	MEcopy(
	    indexes_p->l6_1_ind_name,
	    sizeof(indexes_p->l6_1_ind_name),
	    xid_p->qec_i1_name);

	MEcopy(
	    indexes_p->l6_2_ind_owner,
	    sizeof(indexes_p->l6_2_ind_owner),
	    xid_p->qec_i2_owner);

	v_lnk_p->qec_15_ndx_cnt++;		/* increment count */
	xid_p++;				/* pt to next slot */

	/* 2.2  read next tuple from catalog */

	status = qel_s2_fetch(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
	
    }

    if (v_lnk_p->qec_15_ndx_cnt > 0)
	v_lnk_p->qec_10_haves |= QEC_02_INDEXES;

    return(E_DB_OK);
}    

/*{
** Name: qel_d7_iistatscnt - retrieve number of IISTATS tuples for
**			     current LDB table 
**
** Description:
**	This routine retrieves from the LDB's IISTATS the count of tuples
**  for the table.
**
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**
** Outputs:
**	v_lnk_p
**	    .qec_28_iistats_cnt
**
**	v_qer_p				
**	    .error
**		.err_code		    One of the following:
**					    E_QE0000_OK
**					    (to be specified)
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	06-apr-89 (carl)
**          written
*/


DB_STATUS
qel_d7_iistatscnt(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
/*
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
*/
    DD_LDB_DESC	    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;



    v_lnk_p->qec_28_iistats_cnt = 0;

    /* 1.  set up for retrieveing from IISTATS */

    sel_p->qeq_c1_can_id = SEL_120_II_STATS_COUNT;
    sel_p->qeq_c4_ldb_p = ldb_p;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;


    /* 2.  send SELECT query */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);
        
    if (! sel_p->qeq_c5_eod_b)
    {
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    return(E_DB_OK);
}    

/*{
** Name: qel_d8_des_p2 - Phase 2 of DESTROY/DROP LINK.
**
** Description:
**	This function deletes from all INDEX-related, STATISTICS-related,
**  STANDARD and STAR-specific catalogs for a link in a very forgiving 
**  manner.  Such a policy is necessary to make DESTROY/DROP LINK to
**  get rid of problematic or corrupted links that may have inconsistent 
**  or incomplete information.
**
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
** Outputs:
**	v_qer_p				
**	    .error
**		.err_code		    always E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	20-jul-89 (carl)
**	    adapted from qel_d1_des_p2 to destroy in the same order as 
**	    CREATE LINK
**	21-dec-92 (teresa)
**	    Added support to drop registered procedures.
*/


DB_STATUS
qel_d8_des_p2(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	     status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = & v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    bool	    drop_stats_b = TRUE;	/* assume NOT refresh */
    bool	    regproc;

    /* 1.  always delete from all DDB-specific catalogs */

    status = qel_51_ddb_cats(v_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(v_qer_p);
	if (status)
	    return(status);
    }

    /* 2.  always delete from all STANDARD catalogs */

    status = qel_81_std_cats(v_qer_p, v_lnk_p);	
    if (status)
    {
	status = qed_u9_log_forgiven(v_qer_p);
	if (status)
	    return(status);
    }

    /* 3.  delete from index catalogs if theres any indexes */

    if (v_lnk_p->qec_15_ndx_cnt > 0)
    {
	status = qel_71_ndx_cats(v_qer_p, v_lnk_p);	
	if (status)
	{
	    status = qed_u9_log_forgiven(v_qer_p);
	    if (status)
		return(status);
	}
    }


    /* Determine if this is a registered procedure */
    regproc = (v_lnk_p->qec_13_objects_p->d6_6_obj_type[0]=='L') 
	      && 
	      (v_lnk_p->qec_2_tableinfo_p->d9_2_lcl_type[0]=='P');

    /* 4.  drop current statistics and refresh HISTOGRAMS/STATISTICS catalogs 
    **	   only if the LDB table comes with statistics as informed.  Also,
    **	   never try to update any statistics/histogram data for a registered
    **	   procedure.
    */

    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH
	&&
	!( ddl_p->qed_d10_ctl_info & QED_1DD_DROP_STATS))
	drop_stats_b = FALSE;		/* LDB table has no statistics,
					** save whatever is in STAR */
    
    if (drop_stats_b && !regproc)
    {
	status = qel_61_his_cats(v_qer_p, v_lnk_p);	
	if (status)
	{
	    status = qed_u9_log_forgiven(v_qer_p);
	    if (status)
		return(status);
	}
    }

    /* 5.  delete all dependent views if not REGISTER WITH REFRESH */

    if ( (dds_p->qes_d7_ses_state != QES_4ST_REFRESH) && (!regproc) )
    {
	/* not REGISTER WITH REFRESH and not REGPROC */

	status = qeu_21_des_depends(v_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(v_qer_p);
	    if (status)
		return(status);
	}
    }

    return(E_DB_OK);
}
