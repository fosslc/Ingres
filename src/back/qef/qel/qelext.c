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
#include    <qefptr.h>
#include    <qefcat.h>
#include    <qefgbl.h>
#include    <tm.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: QELEXT.C - Routine processing CREATE/DEFINE LINK
**
**  Description:
**      This file contains 1 routine for processing an external request.
**
**	    qel_e0_cre_lnk  - process CREATE/DEFINE LINK statement
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	25-dec-88 (carl)
**	    added transaction semantics
**	13-mar-89 (carl)
**	    added message commit for CDB association 
**	08-jan-93 (davebf)
**	    Converted to function prototypes
**	29-jan-1993 (fpang)
**	    Compare qef_auto against QEF_ON or QEF_OFF instead of 0 or !0.
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
** Name: qel_e0_cre_lnk - Create/define a link.
**
** 1. External call: qef_call(QED_CLINK, & qef_rcb);
**
** Description:
**	This function assumes that PSF has verified that the link's name 
**  does not yet exist and any link-to-table column mapping is complete 
**  and correct.  It creates a link on a local table in the following steps:
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
**      i_qer_p			    QEF_RCB
**	    .qef_eflag		    designate error handling semantics
**				    for user errors
**		QEF_EXTERNAL	    send error message to user; exception:
**				    forced abort errors
**		QEF_INTERNAL	    return error to sender
**	    .qef_qp		    name of QEF structure in QSF storage
**	    .qef_qso_handle	    QSF handle to above structure, NULL if none
**		<QED_DDL_INFO>	    structure at handle
**		    .qed_d1_obj_name	object's name at DDB level or link name
**		    .qed_d2_obj_owner	object owner name
**		    .qed_d3_col_count	number of columns for object
**		    .qed_d4_ddb_cols_pp	ptr to array of ptrs to DDB columns
**		    .qed_d5_qry_info_p	NULL
**		    .qed_d6_tab_info_p	ptr to LDB table information
**			.dd_t1_tab_name	    same as qed_d3_obj_ldbname above
**			.dd_t2_tab_owner    owner of LDB table
**			.dd_t3_tab_type	    type of LDB table
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
**		    .qed_d9_reg_info_p	ptr to iiregistrations query text 
**			.qed_i1_qry_info
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
**	25-dec-88 (carl)
**	    added transaction semantics
**	13-mar-89 (carl)
**	    added message commit for CDB association 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_e0_cre_lnk(
QEF_RCB		*i_qer_p )
{
    DB_STATUS		status,
			sav_status = E_DB_OK;
    DB_ERROR		sav_error;
    DB_ERROR		err_stub;
    i4		ignore;
    QES_DDB_SES		*dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
/*
    DD_LDB_DESC		*cdb_p = & dds_p->qes_d4_ddb_p->
			    dd_d3_cdb_info.dd_i1_ldb_desc;
*/
    QEP_PTR_UNION	ptr_union;
    QSF_RCB		qsf_rcb;
    QEF_CB		*qcb_p = i_qer_p->qef_cb;
    bool		xact_b = FALSE;		/* assume no begin transaction 
						*/
    

    /* transaction semantics
    **	 
    **	  1.	pre-processing : 
    **	    
    **		1.1  if no outstanding transaction, begin a transaction 
    **		     and set xact_b to TRUE;
    **		1.2  if not in auto-commit mode, escalate transaction to MST;
    **
    **	  2.	post-processing:
    **	    
    **		2.1  if processing terminates normally, commit transaction
    **		     only if xact_b is TRUE and not in auto-commit mode;
    **		2.2  if processing terminates abnormally, abort transaction
    **		     only if xact_b is TRUE;
    */

    /* 1.  set up transaction if none exists */

    if (qcb_p->qef_stat == QEF_NOTRAN)
    {
	status = qet_begin(qcb_p);
	if (status)
	    return(status);

	xact_b = TRUE;
    }

    if (qcb_p->qef_auto == QEF_OFF)
	qcb_p->qef_stat = QEF_MSTRAN;		/* escalate to MST */

    /* 2.  get query control block from QSF */

    qsf_rcb.qsf_lk_state = QSO_EXLOCK;
    
    status = qed_u4_qsf_acquire(i_qer_p, & qsf_rcb);
    if (status)
    {
	STRUCT_ASSIGN_MACRO(qsf_rcb.qsf_error, i_qer_p->error);
	return(status);
    }

    /* QSF handle acquired */

    /* 3.  get at the QSF object root */

    status = qed_u6_qsf_root(i_qer_p, & qsf_rcb);
    if (status)
    {
	sav_status = status;
	STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
		/* cannot return until object is destroyed */
    }
    else
    {
	/* 4.  perform CREATE */

	ptr_union.qep_ptr_u.qep_1_ptr = qsf_rcb.qsf_root;
	STRUCT_ASSIGN_MACRO(
	    *ptr_union.qep_ptr_u.qep_2_ddl_info_p, 
	    i_qer_p->qef_r3_ddb_req.qer_d7_ddl_info);

	status = qel_c0_cre_lnk(i_qer_p);
	if (status)
	{
	    sav_status = status;
	    STRUCT_ASSIGN_MACRO(i_qer_p->error, sav_error);
		/* cannot return until object is destroyed */
	}
    }

    /* 5.  must destroy QSF object */

    status = qed_u5_qsf_destroy(i_qer_p, & qsf_rcb);

    /* 6.  post-processing*/

    if (status || sav_status)
    {
	/* error condition */

	if (xact_b)				/* abort SST */
	    status = qet_abort(qcb_p);
    }
    else
    {
	/* ok condition */

	if (xact_b && qcb_p->qef_auto == QEF_ON)		
	    status = qet_commit(qcb_p);		/* commit SST */
	else
	{
	    /* send message to commit CDB association to avoid deadlocks */

	    status = qed_u11_ddl_commit(i_qer_p);
	}
    }

    if (sav_status)
    {
	/* 7.  return first error */

	STRUCT_ASSIGN_MACRO(sav_error, i_qer_p->error);
	status = sav_status;
    }
/*
    if (status)
	qef_error(i_qer_p->error.err_code, 0L, E_DB_ERROR, &ignore, &err_stub, 0);
*/	
    return(status);
}    

