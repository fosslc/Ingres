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
**  Name: QEL1HI.C - Routines updating HISTOGRAMS and STATISTICS catalogs 
**		     for CREATE LINK
**
**  Description:
**      This file contains routine for updating above catalogs for CREATE LINK.
**
**	    qel_11_his_cats	- update above catalogs for CREATE LINK
**	    qel_12_stats	- update IIDD_STATISTICS
**	    qel_13_histograms	- update IIDD_HISTOGRAMS
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	08-dec-92 (anitap)
**	    Included <qsf.h> for CREATE SCHEMA.
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**      14-sep-93 (smc)
**          Added <cs.h> for CS_SID.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/


/*{
** Name: QEL_11_HIS_CATS - Perform updates on STATISTICS AND HISTOGRAMS 
**			   catalogs.
**
** Description:
**	This function performs above updates for CREATE LINK.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**
** Outputs:
**	none
**	
**	i_qer_p				
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
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_11_his_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS		status;
    DD_2LDB_TAB_INFO	*tabinfo_p = 
			    v_lnk_p->qec_1_ddl_info_p->qed_d6_tab_info_p;


    if (tabinfo_p->dd_t3_tab_type != DD_2OBJ_TABLE
	||
	v_lnk_p->qec_10_haves & QEC_11_LDB_DIFF_ARCH)
	return(E_DB_OK);

    status = qel_12_stats(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    status = qel_13_histograms(i_qer_p, v_lnk_p);

    return(status);
}    


/*{
** Name: QEL_12_STATS - Insert to IIDD_STATS.
**
** Description:
**	This routine retrieves from the LDB and inserts the data into above 
**  catalog under the link name and owner.  
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**
** Outputs:
**	i_qer_p				
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
**	01-mar-93 (barbara)
**	    Delimited id support.  Always assume there are mapped column
**	    names.  Even if user didn't specify a mapping, PSF will provide
**	    a mapping in case of incompatible case translation semantics
**	    between DDB and LDB.
**	04-jun-93 (barbara)
**	    Changed my mind about the above!  PSF was unable to provide
**	    a mapping in all cases (e.g. on CREATE AS SELECT) so QEF now
**	    maps LDB columns into DDB columns, case translating if necessary.
**	22-jun-93 (barbara)
**	    Fixed typo in dd_p3_ldb_caps reference.
*/


DB_STATUS
qel_12_stats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *tabinfo_p = ddl_p->qed_d6_tab_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_L15_STATS   stats,
		    *stats_p = & stats;		/* tuple struct */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p;
    bool	    found_b;
    DD_COLUMN_DESC  *from_p;


    /* 1.  set up to retrieve from LDB */

    qed_u0_trimtail(
		    tabinfo_p->dd_t1_tab_name,
		    (u_i4) DB_MAXNAME,
		    stats_p->l15_1_tab_name);
    qed_u0_trimtail(
		    tabinfo_p->dd_t2_tab_owner,
		    (u_i4) DB_MAXNAME,
		    stats_p->l15_2_tab_owner);

    sel_p->qeq_c1_can_id = SEL_116_II_STATS;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;
    sel_p->qeq_c3_ptr_u.l15_stats_p = stats_p;
    sel_p->qeq_c4_ldb_p = ldb_p;

    /* 2.  send SELECT query */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (sel_p->qeq_c5_eod_b)
	return(E_DB_OK);

    v_lnk_p->qec_10_haves |= QEC_05_STATS;

    while (! status && ! sel_p->qeq_c5_eod_b)
    {
	/* 3.  replace with link name and link owner */

	qed_u0_trimtail(
			ddl_p->qed_d1_obj_name,
			(u_i4) DB_MAXNAME,
			stats_p->l15_1_tab_name);
	qed_u0_trimtail(
			ddl_p->qed_d2_obj_owner,
			(u_i4) DB_MAXNAME,
			stats_p->l15_2_tab_owner);
	
	/* 4.  if mapping columns, use link column name */

	if (tabinfo_p->dd_t6_mapped_b)
	{
	    found_b = qel_a0_map_col(i_qer_p, v_lnk_p,
			    stats_p->l15_3_col_name, & from_p);
	    if (! found_b)			/* should never happen */
	    {
		status = qed_u1_gen_interr(& i_qer_p->error);
		STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
		ignore = qel_s3_flush(i_qer_p, v_lnk_p);
		STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
		return(status);
	    }
	    /* replace with local column name */

	    MEcopy(from_p->dd_c1_col_name, sizeof(DD_NAME), 
		stats_p->l15_3_col_name);
	}
	else
	{
	    /* May need to massage case of LDB column name */
	    qel_a2_case_col(i_qer_p, stats_p->l15_3_col_name,
			*i_qer_p->qef_cb->qef_dbxlate,
			&tabinfo_p->dd_t9_ldb_p->dd_i2_ldb_plus.
						    dd_p3_ldb_caps);
	}

	/* 5.  insert into IIDD_STATS */

	ins_p->qeq_c1_can_id = INS_630_DD_STATS;
	ins_p->qeq_c3_ptr_u.l15_stats_p = stats_p;
	ins_p->qeq_c4_ldb_p = cdb_p;

	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	{
	    /* must flush before returning */

	    STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
	    sel_p->qeq_c4_ldb_p = ldb_p;
	    ignore = qel_s3_flush(i_qer_p, v_lnk_p);
	    STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
	    return(status);
	}

	/* 5.  read next tuple from stream */

	sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);

    }
    return(status);
}    


/*{
** Name: QEL_13_HISTOGRAMS - Insert to IIDD_HISTOGRAMS.
**
** Description:
**	This routine retrieves from the LDB and inserts the data into above 
**  catalog under the link name and owner.  
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
**
** Outputs:
**	i_qer_p				
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
**	01-mar-93 (barbara)
**	    Delimited id support.  Always assume there are mapped column
**	    names.  Even if user didn't specify a mapping, PSF will provide
**	    a mapping in case of incompatible case translation semantics
**	    between DDB and LDB.
**	04-jun-93 (barbara)
**	    Changed my mind about the above!  PSF was unable to provide
**	    a mapping in all cases (e.g. on CREATE AS SELECT) so QEF now
**	    maps LDB columns into DDB columns, case translating if necessary.
*/


DB_STATUS
qel_13_histograms(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status,
		    ignore;
    DB_ERROR	    sav_error;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
    DD_2LDB_TAB_INFO
		    *tabinfo_p = ddl_p->qed_d6_tab_info_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc,
		    *ldb_p = v_lnk_p->qec_19_ldb_p;
    QEC_L5_HISTOGRAMS
		    his,
		    *his_p = & his;	/* tuple struct */
    QEQ_1CAN_QRY    *sel_p = v_lnk_p->qec_6_select_p,
		    *ins_p = v_lnk_p->qec_22_insert_p;
    u_i4	    tupcnt;
    bool	    found_b;
    DD_COLUMN_DESC  *from_p;


    /* 1.  set up for looping through each retrieved IIHISTOGRAM tuple */

    qed_u0_trimtail(
		    tabinfo_p->dd_t1_tab_name,
		    (u_i4) DB_MAXNAME,
		    his_p->l5_1_tab_name);
    qed_u0_trimtail(
		    tabinfo_p->dd_t2_tab_owner,
		    (u_i4) DB_MAXNAME,
		    his_p->l5_2_tab_owner);

    sel_p->qeq_c1_can_id = SEL_106_II_HISTOGRAMS;
    sel_p->qeq_c3_ptr_u.l5_histograms_p = his_p;
    sel_p->qeq_c4_ldb_p = ldb_p;
    sel_p->qeq_c2_rqf_bind_p = v_lnk_p->qec_20_rqf_bind_p;

    /* 2.  send SELECT query and read first tuple */

    status = qel_s4_prepare(i_qer_p, v_lnk_p);
    if (status)
	return(status);

    if (sel_p->qeq_c5_eod_b)
	return(E_DB_OK);

    v_lnk_p->qec_10_haves |= QEC_05_STATS;

    tupcnt = 0;

    while (! sel_p->qeq_c5_eod_b && ! status)
    {
	/* 4.  replace with link name and link owner */

	qed_u0_trimtail(
			ddl_p->qed_d1_obj_name,
			(u_i4) DB_MAXNAME,
			his_p->l5_1_tab_name);
	qed_u0_trimtail(
			ddl_p->qed_d2_obj_owner,
			(u_i4) DB_MAXNAME,
			his_p->l5_2_tab_owner);

	/* 4.  if mapping columns, use link column name */

	if (tabinfo_p->dd_t6_mapped_b)
	{
	    found_b = qel_a0_map_col(i_qer_p, v_lnk_p,
			    his_p->l5_3_col_name, & from_p);
	    if (! found_b)			/* should never happen */
	    {
		status = qed_u1_gen_interr(& i_qer_p->error);
		STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
		ignore = qel_s3_flush(i_qer_p, v_lnk_p);
		STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
		return(status);
	    }
	    /* replace with local column name */

	    MEcopy(from_p->dd_c1_col_name, sizeof(DD_NAME), 
						his_p->l5_3_col_name);
	}
	else
	{
	    /* May need to massage case of LDB column name */
	    qel_a2_case_col(i_qer_p, his_p->l5_3_col_name,
			*i_qer_p->qef_cb->qef_dbxlate,
			&tabinfo_p->dd_t9_ldb_p->dd_i2_ldb_plus.
						    dd_p3_ldb_caps);
	}
	
	/* 5.  insert into IIDD_DDB_HISTOGRAMS */

	ins_p->qeq_c1_can_id = INS_620_DD_HISTOGRAMS;
	ins_p->qeq_c3_ptr_u.l5_histograms_p = his_p;
	ins_p->qeq_c4_ldb_p = cdb_p;

	status = qel_i1_insert(i_qer_p, v_lnk_p);
	if (status)
	{
	    /* must flush data stream before returning */

	    STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
	    sel_p->qeq_c4_ldb_p = ldb_p;
	    ignore= qel_s3_flush(i_qer_p, v_lnk_p);
	    STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
	    return(status);
	}

	tupcnt++;

	/* 3.  retrieve next tuple from LDB */

	sel_p->qeq_c4_ldb_p = ldb_p;
	status = qel_s2_fetch(i_qer_p, v_lnk_p);
	if (status)
	    return(status);
    }

    return(status);
}    

