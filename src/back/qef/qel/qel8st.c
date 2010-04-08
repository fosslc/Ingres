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
**  Name: QEL8ST.C - Routines deleting from or updating STANDARD catalogs 
**		     for DESTROY LINK
**
**  Description:
**      These routines perform STANDARD catalog deletion for DESTROY LINK.
**
**	    qel_81_std_cats	    - perform deletions on STANDARD catalogs
**	    qel_82_tables	    - delete from IIDD_TABLES
**	    qel_83_columns	    - delete from IIDD_COLUMNS and
**				      IIDD_DDB_LDB COLUMNS if mapping columns
**	    qel_84_altcols	    - delete from IIDD_ALT_COLUMNS
**	    qel_85_registrations    - delete from IIDD_REGISTRATIONS
**
**  History:    $Log-for RCS$
**	08-jun-88 (carl)
**          written
**	10-dec-88 (carl)
**          modified for catalog redefinition
**	12-may-89 (carl)
**	    modified qel_81_std_cats and qel_85_registrations to delete
**	    from iiregistrations in the case of tables
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
** Name: qel_81_std_cats - Perform deletion on non-index STANDARD catalogs.
**
** Description:
**	This function removes information from the non-index STANDARD catalogs 
**  using supplied information for DESTROY LINK.
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
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**	08-jun-88 (carl)
**          written
**	12-may-89 (carl)
**	    modified to delete from iiregistrations in the case of tables
**	21-dec-92 (teresa)
**	    Added support to remove registered procedures, which do not have
**	    any entries in iidd_columns or iidd_altcolumns.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
*/


DB_STATUS
qel_81_std_cats(
QEF_RCB		*i_qer_p, 
QEC_LINK	*v_lnk_p )
{
    DB_STATUS		status;
/*
    QED_DDL_INFO	*ddl_p = v_lnk_p->qec_1_ddl_info_p;
*/
    QEC_D6_OBJECTS	*objects_p = v_lnk_p->qec_13_objects_p;
    QEC_D9_TABLEINFO	*table_p = v_lnk_p->qec_2_tableinfo_p;
    bool		regproc;

    regproc = (objects_p->d6_6_obj_type[0]=='L') 
	      && 
	      (table_p->d9_2_lcl_type[0]=='P');

    if ((objects_p->d6_6_obj_type[0] != 'V') && !regproc)
    {
	/* not a distributed view or registered procedure */

	/* 1.  delete from IIDD_ALT_COLUMNS */

	status = qel_84_altcols(i_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}
    }

    /* 5.  delete from IIDD_TABLES */

    status = qel_82_tables(i_qer_p, v_lnk_p);
    if (status)
    {
	status = qed_u9_log_forgiven(i_qer_p);
	if (status)
	    return(status);
    }

    /* 6.  delete from IIDD_COLUMNS, unless its a registered procedure */
    if (! regproc)
    {
	status = qel_83_columns(i_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}
    }

    /* 7.  delete from IIDD_REGISTRATIONS */

    if (objects_p->d6_6_obj_type[0] == 'L'
	||
	objects_p->d6_6_obj_type[0] == 'T'
	||
	regproc /* redundant test since obj type is 'L' for regproc, but
		** stated explicitely here makes logic easier to understand */
	)    
    {
	status = qel_85_registrations(i_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}
    }

    /* 8.  delete from IIDD_PROCEDURES if a procedure */

    if (regproc)
    {
	status = qel_86_proceduress(i_qer_p, v_lnk_p);
	if (status)
	{
	    status = qed_u9_log_forgiven(i_qer_p);
	    if (status)
		return(status);
	}
    }

    return(E_DB_OK);
}    


/*{
** Name: qel_82_tables - Delete link's entry from IIDD_TABLES.
**
** Description:
**	This routine delete the link's entry from IIDD_TABLES.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
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
qel_82_tables(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L16_TABLES  *tables_p = v_lnk_p->qec_9_tables_p;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;
						/* working structure */


    /* 1.  set up keying information */

    qed_u0_trimtail(
		objects_p->d6_1_obj_name,
		(u_i4) DB_MAXNAME,
		tables_p->l16_1_tab_name);

    qed_u0_trimtail(
		objects_p->d6_2_obj_owner,
		(u_i4) DB_MAXNAME,
		tables_p->l16_2_tab_owner);

    /* 2.  send DELETE query */

    del_p->qeq_c1_can_id = DEL_531_DD_TABLES;	
    del_p->qeq_c3_ptr_u.l16_tables_p = tables_p;	
    del_p->qeq_c4_ldb_p = cdb_p;
    status = qel_r1_remove(i_qer_p, v_lnk_p);

    return(status);
}    


/*{
** Name: qel_83_columns - Delete link's entries from IIDD_COLUMNS.
**
** Description:
**	This function cleans up a link's entries from IIDD_COLUMNS.
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
qel_83_columns(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
/*
    QEC_D9_TABLEINFO
		    *tabinfo_p = v_lnk_p->qec_2_tableinfo_p;
*/
    QEC_L3_COLUMNS  cols,
		    *cols_p = & cols;	/* tuple struct */
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /* 1.  set up keying information */

    qed_u0_trimtail(
		objects_p->d6_1_obj_name,
		(u_i4) DB_MAXNAME,
		cols_p->l3_1_tab_name);

    qed_u0_trimtail(
		objects_p->d6_2_obj_owner,
		(u_i4) DB_MAXNAME,
		cols_p->l3_2_tab_owner);

    del_p->qeq_c1_can_id = DEL_502_DD_COLUMNS;
    del_p->qeq_c3_ptr_u.l3_columns_p = cols_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_84_altcols - Delete a link's entries from IIDD_ALT_COLUMNS.
**
** Description:
**	This routine cleans up a link's entries from above catalog using
**  provided key id information.
**
** Inputs:
**      i_qer_p			    QEF_RCB
**	v_lnk_p			    ptr to QEC_LINK
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
qel_84_altcols(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L2_ALT_COLUMNS
		    alt_cols,
		    *altcols_p = & alt_cols;	/* tuple struct */
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;


    /* 1.  set up for deleting from IIDD_ALT_COLUMNS */

    qed_u0_trimtail(
		objects_p->d6_1_obj_name,
		(u_i4) DB_MAXNAME,
		altcols_p->l2_1_tab_name);

    qed_u0_trimtail(
		objects_p->d6_2_obj_owner,
		(u_i4) DB_MAXNAME,
		altcols_p->l2_2_tab_owner);

    del_p->qeq_c1_can_id = DEL_501_DD_ALT_COLUMNS;
    del_p->qeq_c3_ptr_u.l2_alt_columns_p = altcols_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2. send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    


/*{
** Name: qel_85_registrations - Delete link's entries from IIDD_REGISTRATIONS.
**
** Description:
**	This routine cleans a link's entries from above catalog.
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
**	07-nov-88 (carl)
**          written
**	12-may-89 (carl)
**	    modified to delete from iiregistrations in the case of tables
*/


DB_STATUS
qel_85_registrations(
QEF_RCB		*i_qer_p,
QEC_LINK	*v_lnk_p )
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
/*
    QED_DDL_INFO    *ddl_p = v_lnk_p->qec_1_ddl_info_p;
*/
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEC_L14_REGISTRATIONS
		    regs,
		    *regs_p= & regs;		/* tuple struct */
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;



    /* 1.  set up keying information */

    qed_u0_trimtail(
		objects_p->d6_1_obj_name,
		(u_i4) sizeof(regs_p->l14_1_obj_name),
		regs_p->l14_1_obj_name);

    qed_u0_trimtail(
		objects_p->d6_2_obj_owner,
		(u_i4) sizeof(regs_p->l14_2_obj_owner),
		regs_p->l14_2_obj_owner);
	
    del_p->qeq_c1_can_id = DEL_529_DD_REGISTRATIONS;
    del_p->qeq_c3_ptr_u.l14_registrations_p = regs_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    

/*{
** Name: qel_86_proceduress - Delete link's entries from IIDD_PROCEDURES.
**
** Description:
**	This routine cleans a link's entries from above catalog.
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
**	23-dec-92 (teresa)
**	    Initial creation to remove registered procedures.
*/


DB_STATUS
qel_86_proceduress(i_qer_p, v_lnk_p)
QEF_RCB		*i_qer_p;
QEC_LINK	*v_lnk_p;
{
    DB_STATUS	    status;
    QES_DDB_SES	    *dds_p = & i_qer_p->qef_cb->qef_c2_ddb_ses;
    QEC_D6_OBJECTS  *objects_p = v_lnk_p->qec_13_objects_p;
    DD_LDB_DESC	    *cdb_p = 
			& dds_p->qes_d4_ddb_p->dd_d3_cdb_info.dd_i1_ldb_desc;
    QEQ_1CAN_QRY    *del_p = v_lnk_p->qec_21_delete_p;
    QEC_L18_PROCEDURES
		    procs,
		    *procs_p = &procs;	    /* tuple struct */


    /* 1.  set up keying information */

    qed_u0_trimtail(
		objects_p->d6_1_obj_name,
		(u_i4) sizeof(procs_p->l18_1_proc_name),
		procs_p->l18_1_proc_name);

    qed_u0_trimtail(
		objects_p->d6_2_obj_owner,
		(u_i4) sizeof(procs_p->l18_2_proc_owner),
		procs_p->l18_2_proc_owner);
	
    del_p->qeq_c1_can_id = DEL_528_DD_PROCEDURES;
    del_p->qeq_c3_ptr_u.l18_procedures_p = procs_p;
    del_p->qeq_c4_ldb_p = cdb_p;

    /* 2.  send DELETE query */

    status = qel_r1_remove(i_qer_p, v_lnk_p);
    return(status);
}    
