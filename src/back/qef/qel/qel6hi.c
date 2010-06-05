/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <sl.h>
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
**  Name: QEL6HI.C - Routines deleting from HISTOGRAMS and STATISTICS catalogs 
**		     for DESTROY LINK
**
**  Description:
**      This file contains routine for deleting from above catalogs for 
**  CREATE LINK.
**
**	    qel_61_his_cats	- delete from IIDD_STATS and IIDD_HISTOGRAMS
**				  catalogs for CREATE LINK
**	    qel_62_stats	- delete from IIDD_STATS
**	    qel_63_histograms	- delete from IIDD_HISTOGRAMS
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	30-apr-89 (carl)
**          modified qel_61_his_cats to save statistics for
**	    REGISTER WITH REFRESH
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
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/


/*{
** Name: qel_61_his_cats - Delete from IIDD_STATS and IIDD_HISTOGRAMS 
**			   catalogs.
**
** Description:
**	This function deletes from above catalogs for DESTROY LINK.
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
**		.err_code		    E_QE0000_OK
**      Returns:
**          E_DB_OK                 
**          E_DB_ERROR			    caller error
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	30-apr-89 (carl)
**          modified to save statistics for REGISTER WITH REFRESH
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_61_his_cats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QED_DDL_INFO    *ddl_p = & i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info;


    if (dds_p->qes_d7_ses_state == QES_4ST_REFRESH
	&&
	!( ddl_p->qed_d10_ctl_info & QED_1DD_DROP_STATS))
				    /* save statistics at STAR's level */
	return(E_DB_OK);

    status = qel_62_stats(i_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return(status);
    }

    status = qel_63_histograms(i_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return(status);
    }

    return(E_DB_OK);
}    


/*{
** Name: qel_62_stats - Delete from IIDD_STATS.
**
** Description:
**	This routine deletes a link's information from above catalog.
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
*/


DB_STATUS
qel_62_stats(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L15_STATS   stats,
		    *stats_p = & stats;		/* tuple struct */
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /* 1.  set up for deletion */

    qed_u0_trimtail( objects_p->d6_1_obj_name, (u_i4) DB_OBJ_MAXNAME,
		stats_p->l15_1_tab_name);

    qed_u0_trimtail( objects_p->d6_2_obj_owner, (u_i4) DB_OWN_MAXNAME,
		stats_p->l15_2_tab_owner);

    del_p->qeq_c1_can_id = DEL_530_DD_STATS;
    del_p->qeq_c3_ptr_u.l15_stats_p = stats_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_63_histograms - Delete from IIDD_HISTOGRAMS.
**
** Description:
**	This routine deletes a link's information from above catalog.
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
*/


DB_STATUS
qel_63_histograms(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L5_HISTOGRAMS
		    his,
		    *his_p = & his;	/* tuple struct */
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /* 1.  set up for deletion */

    qed_u0_trimtail( objects_p->d6_1_obj_name, (u_i4) DB_OBJ_MAXNAME,
		his_p->l5_1_tab_name);

    qed_u0_trimtail( objects_p->d6_2_obj_owner, (u_i4) DB_OWN_MAXNAME,
		his_p->l5_2_tab_owner);

    del_p->qeq_c1_can_id = DEL_520_DD_HISTOGRAMS;
    del_p->qeq_c3_ptr_u.l5_histograms_p = his_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    

