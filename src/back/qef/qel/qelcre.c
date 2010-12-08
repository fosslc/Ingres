/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
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
#include    <cv.h>
#include    <me.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QELCRE.C - Contains routines for CREATE/DEFINE LINK
**
**  Description:
**      This file contains routines for CREATE LINK.
**
**	    qel_c0_cre_lnk	- create/define or refresh a link
**	    qel_c1_cre_p1	- phase 1 of CREATE LINK
**	    qel_c2_cre_p2	- phase 2 of CREATE LINK
**	    qel_c3_obj_base	- retrieve current object base
**	    qel_c4_ldbid_short	- retrieve the LDB's id if it exists
**	    qel_c5_ldbid_long	- retrieve the LDB's alias if it exists or
**				  else generate one from its long name
**	    qel_c6_ldb_tables	- retrieve the table's information from
**				  the LDB's IITABLES
**	    qel_c7_ldb_chrs	- retrieve the LDB's characteristics from
**				  the LDB's IIDBCONSTANTS
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	25-jan-89 (carl)
**          modified for new iidd_ddb_ldbids format
**	02-apr-89 (carl)
**          modified to process the CREATE LINK part of REGISTER WITH REFRESH
**	02-aug-89 (carl)
**	    modified qel_c0_cre_lnk to increment object_base in 
**	    ii_dd_ddb_object_base to synchronize threads doing
**	    CREATE LINK 
**	23-sep-90 (carl)
**	    modified qel_c0_cre_lnk to process trace point
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	20-jan-1993 (fpang)
**	    In qel_c0_cre_lnk(), register with TPF as an updater if ddl 
**	    concurrency is off.
**	30-jun-93 (rickh)
**	    Corrected a call to CVla.  The second argument should have been
**	    an address.
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
**	    QEF session startup time.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
**/


GLOBALREF   char	IIQE_c5_low_ingres[];
GLOBALREF   char	IIQE_c6_cap_ingres[];
GLOBALREF   char	IIQE_30_alias_prefix[];
GLOBALREF   char	IIQE_61_qefsess[];
GLOBALREF   char	IIQE_62_3dots[];
GLOBALREF   char	IIQE_65_tracing[];



/*{
** Name: qel_c0_cre_lnk - Create/define or refresh a link.
**
** Description:
**	This function creates a link on a local table in the following steps:
**
**	1.  Insert the link's information into STAR catalogs.
**	2.  Promote the table's standard catalog information from the LDB into 
**	    the STAR's equivalent catalogs in the CDB under the link's name.
**	3.  If this is the first time that the LDB participates in the DDB, 
**	    incorporate its information by:
**
**	    3.1  Assigning an id and insert its information into STAR's LDB id 
**		 catalog.
**	    3.2  Promoting the LDB's capabilitiies into STAR's LDB capability
**		 catalog.
**	    3.3  Rederiving the lowest common LDB capabilities and insert
**		 them into STAR's own capability catalog.
**
**  Notes:
**	1) All updates on the CDB are done under $ingres for Titan.
**	2) Until 2PC is implemented, CDB updates are registered with TPF as 
**	   READ-ONLY operations to avoid jeopardizing the single-site update 
**	   requirement imposed on the user.
**  
** Inputs:
**      qef_rcb			    QEF_RCB
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qef_cb
**		.qef_c2_ddb_ses
**		    .qes_d7_ses_state    
**				    QES_4ST_REFRESH for REGISTER WITH REFRESH
**	    .qef_r3_ddb_req	    QEF_DDB_REQ
**		.qer_d7_ddl_info    QED_DDL_INFO
**		    .qed_d1_obj_name	    object's name at DDB level or 
**					    link name
**		    .qed_d2_obj_owner	    object owner name
**		    .qed_d3_col_count	    number of columns for object
**		    .qed_d4_ddb_cols_pp	    ptr to array of ptrs to DDB columns
**		    .qed_d5_qry_info_p	    NULL
**		    .qed_d6_tab_info_p	    ptr to LDB table information
**			.dd_t1_tab_name	    same as qed_d3_obj_ldbname above
**			.dd_t2_tab_owner    owner of LDB table
**			.dd_t3_tab_type	    type of LDB table:
**					    DD_4OBJ_INDEX for index
**			.dd_t6_mapped_b	    TRUE if column mapping is specified
**			.dd_t7_col_cnt	    same as qed_d5_col_count above
**			.dd_t8_cols_pp	    ptr to array of ptrs to columns
**			.dd_t9_ldb_p	    ptr to LDB information
**			    .dd_i1_ldb_desc	    LDB descriptor
**				.dd_l1_ingres_b	    FALSE for non-$ingres
**				.dd_l2_node_name    node name
**				.dd_l3_ldb_name	    LDB name
**				.dd_l4_dbms_name    INGRES if default
**				.dd_l5_ldb_id	    0 for unknown
**		    .qed_d7_obj_id	    for passing object id, e.g., to 
**					    identify a link 
**		    .qed_d8_obj_type	    DD_1OBJ_LINK for REGISTER/CREATE,
**		    .qed_d9_reg_info_p	    ptr to iiregistrations query text 
**					    NULL if none
**			.qed_i1_qry_info
** Outputs:
**	qef_rcb				
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
**	02-apr-89 (carl)
**          modified to process the CREATE LINK part of REGISTER WITH REFRESH
**	02-aug-89 (carl)
**	    modified qel_c0_cre_lnk to increment object_base in 
**	    ii_dd_ddb_object_base to synchronize threads doing
**	    CREATE LINK 
**	23-sep-90 (carl)
**	    modified to process trace point
**	20-jan-1993 (fpang)
**	    Register with TPF as an updater if ddl concurrency is off.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_c0_cre_lnk(
QEF_RCB		*v_qer_p ) 
{
    DB_STATUS		status;
    QEF_CB		*qef_cb = v_qer_p->qef_cb;
    QES_DDB_SES		*dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO	*ddl_p = & v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_0LDB_PLUS	*plus_p = & ddl_p->qed_d6_tab_info_p->
			    dd_t9_ldb_p->dd_i2_ldb_plus;
    DD_CAPS		*caps_p = & plus_p->dd_p3_ldb_caps;
    DD_LDB_DESC		ldb,		    /* for replicating LDB desc */
			*ldb_p = & ldb,
			*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc,
			*ddl_ldb_p = & ddl_p->qed_d6_tab_info_p->
			    dd_t9_ldb_p->dd_i1_ldb_desc;
    QEC_D9_TABLEINFO	tabinfo;	    /* used for table info of link */
    QEC_D2_LDBIDS	ldbids;		    /* used for internal LDB id info */
    QEC_D5_LONG_LDBNAMES
			longnames;	    /* used for excessively long LDB
					    ** name */
    QEC_L16_TABLES	tables;		    /* used for IITABLES-style info */
    QEC_D6_OBJECTS	objects;	    /* used for object info */
    QEC_L6_INDEXES	indexes;
    QEC_MIN_CAP_LVLS	pre_mins,
			aft_mins;
    QEC_LINK		link,		    /* used as global control block */
			*link_p = & link;
    QEQ_1CAN_QRY	del,
			ins,
			sel,
			upd,		    /* used for ordering canned query */
			*upd_p = & upd;
    u_i4		len;
    SYSTIME		now;
    bool		log_ddl_56 = FALSE,
			log_err_59 = FALSE;
    i4		i4_1,
			i4_2;
    QEC_INDEX_ID	ndx_ids[QEK_0MAX_NDX_COUNT + 1];
					    /* working space for index ids */
    RQB_BIND		rq_bind[QEC_CAT_COL_COUNT_MAX + 1];
    QEC_D7_OBJECT_BASE	obj_base,
			*base_p = & obj_base;
    i4			xact_mode;
    char		*cbuf = v_qer_p->qef_cb->qef_trfmt;
    i4			cbufsize = v_qer_p->qef_cb->qef_trsize;

    
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
    ins.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;
    upd.qeq_c2_rqf_bind_p = (RQB_BIND *) NULL;

    /* 1.  set up control block */

    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH)	
						/* REGISTER WITH REFRESH ? */
	STRUCT_ASSIGN_MACRO(ddl_p->qed_d7_obj_id, tabinfo.d9_1_obj_id);
						/* yes */
    else					/* no */
    {
	tabinfo.d9_1_obj_id.db_tab_base = 0;	/* unknown */
	tabinfo.d9_1_obj_id.db_tab_index = 0;	/* always 0 for non-index */
    }
    link_p->qec_1_ddl_info_p = ddl_p;
    link_p->qec_2_tableinfo_p = & tabinfo;
    link_p->qec_3_ldb_id = 0;			
    link_p->qec_4_col_cnt = 0;
    MEfill(DB_DB_MAXNAME, ' ', link_p->qec_5_ldb_alias);
    link_p->qec_6_select_p = & sel;
    link_p->qec_7_ldbids_p = & ldbids;
    link_p->qec_8_longnames_p = & longnames;
    link_p->qec_9_tables_p = & tables;
    link_p->qec_10_haves = QEC_07_CREATE;
    if (caps_p->dd_c6_name_case == DD_2CASE_UPPER)
	link_p->qec_10_haves |= QEC_06_UPPER_LDB;
    if (caps_p->dd_c1_ldb_caps & DD_7CAP_USE_PHYSICAL_SOURCE)
	link_p->qec_10_haves |= QEC_10_USE_PHY_SRC;
    if (caps_p->dd_c2_ldb_caps & DD_201CAP_DIFF_ARCH)
	link_p->qec_10_haves |= QEC_11_LDB_DIFF_ARCH;

    /* make a copy of the LDB descriptor for non-$ingrs access if necessary */

    if (ddl_ldb_p->dd_l1_ingres_b)
    {
	/* make a copy for non-$ingres access */

	STRUCT_ASSIGN_MACRO(*ddl_ldb_p, ldb);
	ldb_p->dd_l1_ingres_b = FALSE;
	ldb_p->dd_l5_ldb_id = DD_0_IS_UNASSIGNED;
    }
    else
    {	
	/* use provided LDB descriptor but determine if LDB has a long name */

	qed_u0_trimtail(
		ddl_ldb_p->dd_l3_ldb_name, 
		(u_i4) DD_256_MAXDBNAME,
		ldb_p->dd_l3_ldb_name);

	len = STlength(ldb_p->dd_l3_ldb_name);
	if (len > DB_DB_MAXNAME)
	    link_p->qec_10_haves |= QEC_04_LONG_LDBNAME;

	ldb_p = ddl_ldb_p;
    }
    link_p->qec_11_ldb_obj_cnt = 0;
    link_p->qec_12_indexes_p = & indexes;
    link_p->qec_13_objects_p = & objects;
    link_p->qec_15_ndx_cnt = 0;
    link_p->qec_16_ndx_ids_p = ndx_ids;
    link_p->qec_19_ldb_p = ldb_p;
    link_p->qec_20_rqf_bind_p = rq_bind;
    link_p->qec_21_delete_p = & del;
    link_p->qec_22_insert_p = & ins;
    link_p->qec_23_update_p = & upd;

    status = qed_u8_gmt_now(v_qer_p, link_p->qec_24_cur_time);
    if (status)
	return(status);

    link_p->qec_24_cur_time[DD_25_DATE_SIZE] = EOS;
    link_p->qec_25_pre_mins_p = & pre_mins;
    link_p->qec_26_aft_mins_p = & aft_mins;
    link_p->qec_27_select_cnt = 0;
    link_p->qec_28_iistats_cnt = 0;
    
    TMnow(& now);
    link_p->qec_17_ts1 = now.TM_secs;
    link_p->qec_18_ts2 = now.TM_msecs;
    
    /* 2.  inform TPF of update intention on CDB (note that only CREATE does
    **	   LDB retrievals */

    /* 2.1 2PC is required if DDL concurrency is off. */
    if (dds_p->qes_d9_ctl_info & QES_01CTL_DDL_CONCURRENCY_ON)
	xact_mode = QEK_4TPF_1PC;
    else
	xact_mode = QEK_3TPF_UPDATE;

    status = qet_t5_register(v_qer_p, cdb_p, DB_SQL, xact_mode);
    if (status)
	return(status);

    if (log_ddl_56)
    {
	/* display beginning, CDB and LDB info */

	STprintf(cbuf, "\n%s %p: %s REGISTRATION begins%s\n",
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

	STprintf(cbuf, 
	    "%s %p: ...the LDB:\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
	qed_w1_prt_ldbdesc(v_qer_p, ldb_p);

	STprintf(cbuf, "\n");
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }
    /* 3.  update OBJECT_BASE in II_DD_DB_OBJECT_BASE */

    upd_p->qeq_c1_can_id = UPD_716_DD_DDB_OBJECT_BASE;
    upd_p->qeq_c3_ptr_u.d7_object_base_p = base_p;
    upd_p->qeq_c4_ldb_p = cdb_p;

    status = qel_u1_update(v_qer_p, link_p);
    if (status)
	goto END_TRACING;

    /* 4.  enter phase 1: set up necessary information */

    status = qel_c1_cre_p1(v_qer_p, link_p);
    if (status)
	goto END_TRACING;

    /* 2.  enter phase 2: promote information into STAR catalogs */

    status = qel_c2_cre_p2(v_qer_p, link_p);
    if (status)
	v_qer_p->qef_cb->qef_abort = TRUE;

END_TRACING:

    if (log_ddl_56)
    {
	/* display end tracing message */

	STprintf(cbuf, 
	    "%s %p: %s REGISTRATION ends\n\n",
	    IIQE_61_qefsess,
	    (PTR) v_qer_p->qef_cb,
	    IIQE_65_tracing);
        qec_tprintf(v_qer_p, cbufsize, cbuf);
    }

    return(status);
}    


/*{
** Name: QEL_C1_CRE_P1 - Phase 1 of CREATE/DEFINE LINK.
**
** Description:
**	This function sets up all necessary information to facilitate phase 2 
**  for updating STAR catalogs.
**
**  
** Inputs:
**      v_qer_p			    QEF_RCB
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
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
**	25-jan-89 (carl)
**          modified for new iidd_ddb_ldbids format
**	02-apr-89 (carl)
**          modified to process the CREATE LINK part of REGISTER WITH REFRESH
*/


DB_STATUS
qel_c1_cre_p1(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = & v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_LDB_DESC	    *ldb_p = v_lnk_p->qec_19_ldb_p;
    bool	    obj_id_b = TRUE;	/* assume new object id required */


    /*  1.  determine if LDB is INGRES */

    if (
	(MEcmp(
		ldb_p->dd_l4_dbms_name, 
		IIQE_c6_cap_ingres, 
		QEK_005_LEN) == 0)
	||
	(MEcmp(
		ldb_p->dd_l4_dbms_name, 
		IIQE_c5_low_ingres, 
		QEK_005_LEN) == 0)
       )
	v_lnk_p->qec_10_haves |= QEC_03_INGRES;

    /*	2.  retrieve existing OBJECT_BASE from IIDD_DDB_OBJECT_BASE */

    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH
	&&
	ddl_p->qed_d6_tab_info_p->dd_t3_tab_type != DD_4OBJ_INDEX)
				/* REFRESH LINK ? */
	obj_id_b = FALSE;	/* Yes, retain original object id */

    if (obj_id_b)
    {
	/* retrieve the new object id as required */

	status = qel_c3_obj_base(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /*	3.  retrieve the LDB's id from IIDD_DDB_LDBIDS if it exists */

    if (v_lnk_p->qec_10_haves & QEC_04_LONG_LDBNAME)
	status = qel_c5_ldbid_long(v_qer_p, v_lnk_p);	
    else
	status = qel_c4_ldbid_short(FALSE, v_qer_p, v_lnk_p);	
					    /* FALSE for no database alias */
    if (status)
	return(status);

    v_lnk_p->qec_3_ldb_id = v_lnk_p->qec_7_ldbids_p->d2_5_ldb_id;

    /*	4.  retrieve the table's information from the LDB's IITABLES */

    status = qel_c6_ldb_tables(v_qer_p, v_lnk_p);	
    if (status)
	return(status);

    /*	5.  if the LDB is a new participant, do:
    **	
    **	    determine if it has an excessively long name, 
    **	    retrieve information about existence of user names, dba name, and
    **	    compute the minimum levels of LDB capabilities existing
    **	    in IIDD_DDB_LDB_DBCAPS */

    if (v_lnk_p->qec_3_ldb_id == 0)
    {
	if (v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p->
	    dd_t9_ldb_p->dd_i1_ldb_desc.dd_l3_ldb_name[0] == ' ')
	{
	    status = qed_u1_gen_interr(& v_qer_p->error);
	    return(status);
	}

	status = qel_c7_ldb_chrs(v_qer_p, v_lnk_p);
	if (status)
	    return(status);

	status = qel_a1_min_cap_lvls(TRUE, v_qer_p, v_lnk_p); 
						/* TRUE for filling what
						** qec_25_pre_mins_p points
						** at */
    }

    return(status);
}


/*{
** Name: QEL_C2_CRE_P2 - Phase 2 of CREATE/DEFINE LINK.
**
** Description:
**	This function updates all STANDARD, INDEX-related, STATISTICS-related
**  and STAR-specific catalogs.
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
**	19-nov-92 (teresa)
**	    support for procedures.
*/


DB_STATUS
qel_c2_cre_p2(
QEF_RCB		*v_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS		status;
    DD_2LDB_TAB_INFO	*tabinfo_p = 
			    v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p;

    /* 
    **	1.  update all DDB-specific catalogs first to allow DESTROY LINK
    **	    to work repeatedly for robustness  
    */

    status = qel_01_ddb_cats(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    /* 2.  update all STANDARD catalogs */

    status = qel_31_std_cats(v_qer_p, v_lnk_p);	
    if (status)
	return(status);

    /* 3.  probe and update all HISTOGRAMS/STATISTICS catalogs */

    if (tabinfo_p->dd_t3_tab_type == DD_2OBJ_TABLE
	&&
	! (v_lnk_p->qec_10_haves & QEC_11_LDB_DIFF_ARCH))
    {
	status = qel_11_his_cats(v_qer_p, v_lnk_p);	
	if (status)
	    return(status);
    }
    /* 4.  promote index information and create a link for each index */

    /* not a view, not a registered procedure and is ingres (i.e., not gateway)*/
    if (tabinfo_p->dd_t3_tab_type != DD_3OBJ_VIEW 
	&&
	tabinfo_p->dd_t3_tab_type != DD_5OBJ_REG_PROC
	&&
	v_lnk_p->qec_10_haves & QEC_03_INGRES)

	    status = qel_21_ndx_cats(v_qer_p, v_lnk_p);	

    return(status);
}


/*{
** Name: QEL_C3_OBJ_BASE - Retrieve current object base 
**
** Description:
**      This routine retrieves the current object base value from 
**  IIDD_DDB_OBJECT_BASE.
**
** Inputs:
**	    v_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
** Outputs:
**	    v_lnk_p			QEC_LINK
**		.qec_c2_tab_info_p
**		    .q7_1_obj_id	
**					link/table	index
**					----------	-----
**			.db_tab_base	current value	unchanged
**				        + 1
**			.db_tab_index	0		current value + 1
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
**	02-aug-89 (carl)
**	    modified to use the already incremented object_base read from
**	    ii_dd_ddb_object_base 
*/


DB_STATUS
qel_c3_obj_base(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc;
    QED_DDL_INFO    *ddl_p = & v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;
    DD_2LDB_TAB_INFO	
		    *tab_p = ddl_p->qed_d6_tab_info_p;
    QEC_D7_OBJECT_BASE
		    obj_base;	    /* tuple structure */
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;



    /* 1.  retrieve IIDD_DDB_OBJECT_BASE */

    sel_p->qeq_c1_can_id = SEL_016_DD_DDB_OBJECT_BASE;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.d7_object_base_p = & obj_base; 
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    /* 2.  retrieve current OBJECT_BASE value */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {	
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    /* 3.  use the already incremented object_base */

    if (tab_p->dd_t3_tab_type == DD_4OBJ_INDEX)
    {
	tabinfo_p->d9_1_obj_id.db_tab_base = 
	    ddl_p->qed_d7_obj_id.db_tab_base;
	tabinfo_p->d9_1_obj_id.db_tab_index = obj_base.d7_1_obj_base; 
						/* creating link for index */
    }
    else
    {    
	tabinfo_p->d9_1_obj_id.db_tab_base = obj_base.d7_1_obj_base;
	tabinfo_p->d9_1_obj_id.db_tab_index = 0;
    }
    return(E_DB_OK);
}


/*{
** Name: qel_c4_ldbid_short - Retrieve the LDB's id if it exists
**
** Description:
**      This routine retrieves the LDB's id from the CDB if it exists.
**
** Inputs:
**	    i_alias_b			TRUE if using alias, FALSE otherwise
**	    v_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
** Outputs:
**	    v_lnk_p			QEC_LINK
**		.qec_2_ldbid		0 if LDB's id does not exist,
**					> 0, if it exists
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
qel_c4_ldbid_short(
bool		    i_alias_b,
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;

    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    QEC_D2_LDBIDS
		    *ldbids_p = v_lnk_p->qec_7_ldbids_p;


    /* 1.  must initialize ldb_id to unknown */

    ldbids_p->d2_5_ldb_id = 0;

    /* 2.  set up for accessing IIDD_DDB_LDBIDS */

    sel_p->qeq_c3_ptr_u.d2_ldbids_p = ldbids_p; 
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    qed_u0_trimtail( ldb_p->dd_l2_node_name, (u_i4) DB_NODE_MAXNAME,
		ldbids_p->d2_1_ldb_node);
    qed_u0_trimtail( ldb_p->dd_l4_dbms_name, (u_i4) DB_TYPE_MAXLEN,
		ldbids_p->d2_2_ldb_dbms);

    if (i_alias_b)
	qed_u0_trimtail( v_lnk_p->qec_5_ldb_alias, (u_i4) DB_DB_MAXNAME,
		ldbids_p->d2_3_ldb_database);
					    /* use alias as database name */
    else
	qed_u0_trimtail(
		ldb_p->dd_l3_ldb_name,
		(u_i4) DD_256_MAXDBNAME,
		ldbids_p->d2_3_ldb_database);

    /* 4.  send SELECT query to retrieve ldb id if it exists */

    sel_p->qeq_c1_can_id = SEL_005_DD_DDB_LDBIDS_BY_NAME;

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status) 
	return(status);

    if (! sel_p->qeq_c5_eod_b)
	status = qel_s3_flush(v_qer_p, v_lnk_p);

    return(status);
}


/*{
** Name: qel_c5_ldbid_long - Retrieve the LDB's alias if it exists or
**			     else generate one from its long name 
**
** Description:
**      This routine retrieves the LDB's alias from IIDD_DDB_LONG_LDBNAMES 
**  if it exists or else generates one before calling above routine to 
**  obtain the LDB's id if it exists.
**
** Inputs:
**	    v_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
** Outputs:
**	    v_lnk_p			QEC_LINK
**		.qec_2_ldbid		0 if LDB's id does not exist,
**					> 0, if it exists
**		.ldbids_p
**		.qec_5_ldb_alias	
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
**	30-jun-93 (rickh)
**	    Corrected a call to CVla.  The second argument should have been
**	    an address.
*/


DB_STATUS
qel_c5_ldbid_long(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & v_qer_p->qef_cb->qef_c2_ddb_ses;
    DD_LDB_DESC	    *cdb_p = & dds_p->qes_d4_ddb_p->
			dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;

    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;
    QEC_D5_LONG_LDBNAMES
		    *long_p = v_lnk_p->qec_8_longnames_p;


    /* 1.  set up for accessing IIDD_DDB_LDB_LONGNAMES */

    sel_p->qeq_c3_ptr_u.d5_long_ldbnames_p = long_p; 
    sel_p->qeq_c4_ldb_p = cdb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    qed_u0_trimtail(
	ldb_p->dd_l3_ldb_name,
	(u_i4) DD_256_MAXDBNAME,
	long_p->d5_1_ldb_name);

    /* 3.  retrieve the LDB's 32-char alias if it exists */

    sel_p->qeq_c1_can_id = SEL_014_DD_DDB_LONG_LDBNAMES;

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)	    
    {
	/* no data */
    
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status) 
	    return(status);

	/* generate alias */

	STcopy(IIQE_30_alias_prefix, v_lnk_p->qec_5_ldb_alias);   /* prefix */
	CVla(v_lnk_p->qec_2_tableinfo_p->d9_1_obj_id.db_tab_base,
	     &v_lnk_p->qec_5_ldb_alias[QEK_5_SIZE_OF_PREFIX]);

	return(E_DB_OK);
    }

    /* 4.  send SELECT query to retrieve ldb id if it exists */

    status = qel_c4_ldbid_short(TRUE, v_qer_p, v_lnk_p);
						/* TRUE for using alias */
    return(status);
}


/*{
** Name: qel_c6_ldb_tables - Retrieve a table's information from the 
**			     LDB's IITABLES
**
** Description:
**      This routine retrieves information from the given LDB's IITABLES.
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
qel_c6_ldb_tables(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS		status;
    DD_2LDB_TAB_INFO	*tabinfo_p = v_lnk_p->qec_1_ddl_info_p->
			    qed_d6_tab_info_p;
    DD_LDB_DESC		*ldb_p = v_lnk_p->qec_19_ldb_p;
    QEQ_1CAN_QRY	*sel_p = v_lnk_p->qec_6_select_p;
    QEC_L16_TABLES	*tables_p = v_lnk_p->qec_9_tables_p;



    /* 1.  set up for retrieving from IITABLES */

    MEcopy(tabinfo_p->dd_t1_tab_name, DB_TAB_MAXNAME, 
	tables_p->l16_1_tab_name);
    MEcopy(tabinfo_p->dd_t2_tab_owner, DB_OWN_MAXNAME, 
	tables_p->l16_2_tab_owner);
/*    if (v_qer_p->qef_r3_ddb_req.qer_d13_ctl_info & QEF_06_REG_PROC) */
    if (v_qer_p->qef_r3_ddb_req.qer_d7_ddl_info.qed_d6_tab_info_p->
	dd_t3_tab_type == DD_5OBJ_REG_PROC
       )
        sel_p->qeq_c1_can_id = SEL_118_II_PROCEDURES;
    else
	sel_p->qeq_c1_can_id = SEL_117_II_TABLES;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l16_tables_p = tables_p;
    sel_p->qeq_c4_ldb_p = ldb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    /* 2.  generate and send retrieve query */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
	status = qel_s3_flush(v_qer_p, v_lnk_p);

    if (v_lnk_p->qec_27_select_cnt == 0)
    {
	if ((v_lnk_p->qec_10_haves & ~QEC_03_INGRES)
	    &&
	    (tabinfo_p->dd_t3_tab_type == DD_4OBJ_INDEX))
	    v_lnk_p->qec_10_haves |= QEC_08_NO_IITABLES;
	else
	    status = qed_u2_set_interr(
			E_QE0902_NO_LDB_TAB_ON_CRE_LINK,
			& v_qer_p->error);
    }

    return(status);
}


/*{
** Name: QEL_C7_LDB_CHRS - Retrieve the LDB's characteristics.
**
** Description:
**      This routine retrieves the user name and dba name from the LDB's
**	IIDBCONSTANTS to determine if it supports the notion of user and 
**	dba names.
**
** Inputs:
**	    v_qer_p			
**		.qef_cb			QEF session cb
**		    .qef_c2_ddb_ses	DDB session cb
** Outputs:
**	    v_lnk_p			QEC_LINK
**		.qec_c7_ldb_ids_p
**		    .q2_6_ldb_user	'Y' or 'N'
**		    .q2_7_ldb_dba	'Y' or 'N'
**		    .q2_8_ldb_dbaname	DBA name if above is 'Y', blanks 
**					otherwise	
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
**      25-jan-89 (carl)
**          written
*/


DB_STATUS
qel_c7_ldb_chrs(
QEF_RCB		    *v_qer_p,
QEC_LINK	    *v_lnk_p )
{
    DB_STATUS	    status;
    DD_LDB_DESC	    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_D2_LDBIDS   *ldbids_p = v_lnk_p->qec_7_ldbids_p;
    QEC_L1_DBCONSTANTS
		    dbconsts;	    /* tuple structure */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p;


    /* 1.  set up to retrieve from IIDBCONSTANTS */

    sel_p->qeq_c1_can_id = SEL_105_II_DBCONSTANTS;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l1_dbconstants_p = & dbconsts; 
    sel_p->qeq_c4_ldb_p = ldb_p;
    sel_p->qeq_c5_eod_b = FALSE;
    sel_p->qeq_c6_col_cnt = 0;

    /* 2.  retrieve data */

    status = qel_s4_prepare(v_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (! sel_p->qeq_c5_eod_b)
    {	
	status = qel_s3_flush(v_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    if (dbconsts.l1_1_usr_name[0] == '$')
	ldbids_p->d2_6_ldb_user[0] = 'N';	/* no user name */
    else
	ldbids_p->d2_6_ldb_user[0] = 'Y'; 
    ldbids_p->d2_6_ldb_user[1] = EOS;		/* null terminate */

    if (dbconsts.l1_2_dba_name[0] == '$')   
    {
	ldbids_p->d2_7_ldb_dba[0] = 'N';	/* no DBA name */
	ldbids_p->d2_8_ldb_dbaname[0] = EOS;	/* set to null string */
    }
    else
    {
	ldbids_p->d2_7_ldb_dba[0] = 'Y'; 
	qed_u0_trimtail( dbconsts.l1_2_dba_name, (u_i4) DB_OWN_MAXNAME,
		ldbids_p->d2_8_ldb_dbaname);
    }
    ldbids_p->d2_7_ldb_dba[1] = EOS;	/* null terminate */

    return(E_DB_OK);
}

						/* INGRES, i.e., not gateway */
