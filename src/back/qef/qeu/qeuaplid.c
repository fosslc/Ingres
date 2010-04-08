/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <ddb.h>
#include    <cui.h>
#include    <tm.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <dmacb.h>

#include    <ade.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>
#include    <usererror.h>
/**
**
**  Name: QEUAPLID.C - routines for maintaining role identifiers.
**
**  Description:
**      The routines defined in this file provide an interface for
**	qrymod to manipulate the contents of the iirole catalog.
**	The operations supported include CREATE/ALTER/DROP for roles.
**
**	The following routines are defined in this file:
**
**	    qeu_aplid	- dispatches iirole processing routines
**	    qeu_caplid	- appends tuples to iirole
**	    qeu_aaplid	- updates tuples in iirole
**	    qeu_daplid	- deletes tuples from iirole
**	    qeu_gaplid	- gets tuples from iirole for internal QEF routines.
**
**
**  History:
**	17-may-89 (ralph)
**	    Created this file from qeuqmod.c routines.
**	02-feb-90 (davebf)
**	    Added init of qeu_cb.qeu_mask
**	19-sep-90 (ralph)
**	    Audit "success" when CREATE/DROP ROLE (bug 20484)
**	20-nov-1992 (pholman)
**	    Replace obsolete qeu_audit with qeu_secaudit, and add <sxf.h>
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	6-jul-93 (robf)
**	    Pass security label  to qeu_secaudit()
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	3-sep-93 (stephenb)
**	    Modified calls to qeu_secaudit() to correctly reflect the action
**	    SXF_A_CREATE, SXF_A_ALTER, SXF_A_DROP.
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	20-sep-93 (robf)
**          Enforce MAC checks when trying to alter or drop a role.
**	31-mar-94 (rblumer)       in qeu_aplid,
**	    Changed error handling to correctly handle blanks in delimited ids,
**	    by using cus_trmwhite instead of STindex to trim trailing blanks.
**      19-oct-94 (canor01)
**          fixed up casts of queq_d_id to get through AIX 4.1 compiler
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
**	17-mar-1999 (somsa01)
**	    Added include of pc.h and lk.h to resolve references to locking
**	    ids.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**/

/*
**  Definition of static variables and forward static functions 
*/


/*{
** Name: QEU_APLID	- Dispatch QEU routines to manipulate iirole.
**
** External QEF call:   status = qef_call(QEU_APLID, &qeuq_cb);
**
** Description:
**	This procedure checks the qeuq_agid_mask field of the QEUQ_CB
**	to determine which qrymod processing routine to call:
**		QEU_CAPLID results in call to qeu_caplid()
**		QEU_DAPLID results in call to qeu_daplid()
**		QEU_AAPLID results in call to qeu_aaplid()
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_db_id		database id
**	    .qeuq_agid_mask	iirole operation code
**
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE022B_IIAPPLICATION_ID
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0161_INVALID_APLID_OP
**				other error codes from qeu_[cda]aplid
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_SEVERE
**	    Other status codes from qeu_[cda]aplid
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	12-mar-89 (ralph)
**          written
**	31-mar-94 (rblumer)
**	    Changed error handling to correctly handle blanks in delimited ids,
**	    by using cus_trmwhite instead of STindex to trim trailing blanks.
*/
DB_STATUS
qeu_aplid(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DB_APPLICATION_ID *aptuple = NULL;
    DB_STATUS	    status, local_status;
    i4	    error = 0;
    bool	    transtarted = FALSE;
    bool	    tbl_opened = FALSE;
    QEU_CB	    tranqeu;
    QEU_CB	    qeu;
    i4	    alen;

    for (;;)
    {
	/* Check the control block. */

	error = E_QE0017_BAD_CB;
	status = E_DB_SEVERE;
	if (qeuq_cb->qeuq_type != QEUQCB_CB || 
	    qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	

	    break;

	error = E_QE0018_BAD_PARAM_IN_CB;
        if ((qeuq_cb->qeuq_db_id == 0)
            || (qeuq_cb->qeuq_d_id == 0))

	    break;

	if (qeuq_cb->qeuq_cagid != 1 || qeuq_cb->qeuq_agid_tup == 0)

	    break;

	aptuple = (DB_APPLICATION_ID *)qeuq_cb->qeuq_agid_tup->dt_data;

	if (aptuple == NULL)

	    break;

	/* 
	** Check to see if a transaction is in progress,
	** if so, set transtarted flag to FALSE, otherwise
	** set it to TRUE after beginning a transaction.
	** If we started a transaction then we will abort
        ** it if an error occurs, otherwise we will just
        ** return the error and let the caller abort it.
	*/

	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tranqeu.qeu_type = QEUCB_CB;
	    tranqeu.qeu_length = sizeof(QEUCB_CB);
	    tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	    tranqeu.qeu_d_id = qeuq_cb->qeuq_d_id;
	    tranqeu.qeu_flag = 0;
	    tranqeu.qeu_mask = 0;
	    status = qeu_btran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {	
		error = tranqeu.error.err_code;
		break;	
	    }	    
	    transtarted = TRUE;
	}
	/* escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	/* prepare the qeu_cb and open the table for processing */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_APLID_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_APLID_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
	qeu.qeu_mask = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;

	/* Open the iirole catalog */

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error  = E_QE022B_IIAPPLICATION_ID;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}
	tbl_opened = TRUE;

	/* Call the appropriate processing routine */

	switch (qeuq_cb->qeuq_agid_mask)
	{
	    case QEU_CAPLID:
		status = qeu_caplid(qef_cb, qeuq_cb, &qeu);
		error = qeu.error.err_code;
		break;

	    case QEU_AAPLID:
		status = qeu_aaplid(qef_cb, qeuq_cb, &qeu);
		error = qeu.error.err_code;
		break;

	    case QEU_DAPLID:
		status = qeu_daplid(qef_cb, qeuq_cb, &qeu, TRUE);
		error = qeu.error.err_code;
		break;

	    default:
		status = E_DB_SEVERE;
		error = E_QE0161_INVALID_APLID_OP;
		break;
	}
	
	break;

    
    } /* end for */

    /* call qef_error to handle error messages, if any */

    if (status != E_DB_OK)
    {
	if (aptuple != NULL)
	{
	    alen = cus_trmwhite(sizeof(aptuple->dbap_aplid.db_own_name),
				aptuple->dbap_aplid.db_own_name);

	    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, (i4)1,
			     alen, aptuple->dbap_aplid.db_own_name);
	}
	else
	{
	    (VOID) qef_error(error, 0L, status, &error,
			     &qeuq_cb->error, (i4)0);
	}
    }  /* end if status not OK */
    
    /* Close the table. */

    if (tbl_opened)
    {
	local_status = qeu_close(qef_cb, &qeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(qeu.error.err_code, 0L, local_status, 
			&error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    /* End the transaction, if one was started */

    if (transtarted)
    {
	if (status <= E_DB_WARN)
	{
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		(VOID) qef_error(tranqeu.error.err_code, 0L, status, 
			    &error, &qeuq_cb->error, 0);
	    }
	}
	else
	{
	    local_status = qeu_atran(qef_cb, &tranqeu);
	    if (local_status != E_DB_OK)
	    {
		(VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			    &error, &qeuq_cb->error, 0);
		if (local_status > status)
		    status = local_status;
	    }
	}
    }

    return (status);
}

/*{
** Name: QEU_CAPLID	- store application identifier information
**
** Internal	call:   status = qeu_caplid(&qef_cb, &qeuq_cb, &qeu_cb);
**
** Description:
**	Appends tuples to the iirole catalog.
**	If a tuple already exists, an error message is returned.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_cagid		number of application id tuples
**	    .qeuq_agid_tup	application id tuples
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	qeu_cb			pointer to qeu_cb opened on iirole
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0218_APLID_EXISTS
**				E_QE0223_ROLE_USER
**				E_QE0224_ROLE_GROUP
**				E_QE022A_IIUSERGROUP
**				E_QE022B_IIAPPLICATION_ID
**				E_QE022E_IIUSER
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN		Role name conflict
**	    E_DB_ERROR		Unable to process iirole.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in the iirole catalog
**
** History:
**	13-mar-89 (ralph)
**          written.
**	06-may-89 (ralph)
**	    Check for name space collision with iiuser, iiusergroup.
**	12-jan-93
**	    fixed cast
**	3-sep-93 (stephenb)
**	    Modified call to qeu_secaudit() to audit action SXF_A_ALTER
**	    when an alter has triggered a create.
**	7-mar-94 (robf)
**          Create a default grant on the role to the user creating the
**	    role.
**	17-mar-94 (robf)
**          Include limiting label in privilege check for consistency 
**	    with create/alter user/profile
**	6-may-94 (robf)
**          Update check for user name clash to not lose error status.
**	10-Jan-2001 (jenjo02)
**	    We know this session's id; pass it to SCF.
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	26-Oct-2004 (schka24)
**	    Restore check erroneously removed with B1.
*/
DB_STATUS
qeu_caplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb)
{
    DB_APPLICATION_ID *aptuple;
    DB_APPLICATION_ID *atuple;
    DB_STATUS	    status = E_DB_OK;
    i4	    error = 0;
    DB_ERROR	    e_error;
    SCF_CB	    scf_cb;
    SCF_SCI	    sci_list[1];
    char	    username[DB_MAXNAME];

    aptuple = (DB_APPLICATION_ID *)qeuq_cb->qeuq_agid_tup->dt_data;

    /* Ensure no name collision */

    for (;;)
    {
	/* Check the iiusergroup catalog to ensure no name collision 
	** with group
	*/

	status = qeu_ggroup(qef_cb, qeuq_cb,
			   (char *)&aptuple->dbap_aplid, NULL, NULL);

	if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
	    qeu_cb->error.err_code = E_QE022A_IIUSERGROUP;
	else if (status == E_DB_OK)		    /* Group exists! */
	{
	    qeu_cb->error.err_code = E_QE0224_ROLE_GROUP;
	    status = E_DB_WARN;
	}
	else					    /* Group does not exist */
	    status = E_DB_OK;

	/* Terminate if error occurred */

	if (status != E_DB_OK)
	    break;

	/* Check the iiuser catalog to ensure no name collision */

	status = qeu_guser(qef_cb, qeuq_cb,
			   (char *)&aptuple->dbap_aplid, NULL, NULL);

	if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
	    qeu_cb->error.err_code = E_QE022E_IIUSER;
	else if (status == E_DB_OK)		    /* User exists! */
	{
	    qeu_cb->error.err_code = E_QE0223_ROLE_USER;
	    status = E_DB_WARN;
	}
	else					    /* User does not exist */
	    status = E_DB_OK;

	/* Terminate if error occurred */

	if (status != E_DB_OK)
	    break;

        /* Check the iiprofile catalog to ensure no name collision */

        status = qeu_gprofile(qef_cb, qeuq_cb,
                           (char *)&aptuple->dbap_aplid, NULL);

        if (DB_FAILURE_MACRO(status))		    /* Fatal error! */
             qeu_cb->error.err_code = E_QE0282_IIPROFILE;
        else if (status == E_DB_OK)		    /* Profile exists! */
        {
	    (VOID)qef_error(E_QE0283_DUP_OBJ_NAME, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 3,
			    4, "role",
			    qec_trimwhite(sizeof(aptuple->dbap_aplid),
					  (char *)&aptuple->dbap_aplid),
			    (PTR)&aptuple->dbap_aplid,
			    7, "profile" );
	    qeu_cb->error.err_code = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
        }
        else					    /* Profile does not exist */
            status = E_DB_OK;

	/* Terminate if error occurred */

	if (status != E_DB_OK)
	    break;

	/*
	** Don't grant security or audit if you don't own them.
	**
	*/
	if ( ( (aptuple->dbap_status & DU_USECURITY)
		&& !( qef_cb->qef_ustat & DU_USECURITY))
	  || ( (aptuple->dbap_status&DU_UAUDIT_PRIVS)
		&& !( qef_cb->qef_ustat & DU_UALTER_AUDIT)) )
	{
	    /*
	    ** Audit failed attempt to create the role 
	    */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
		    (char *)&aptuple->dbap_aplid,
		    &aptuple->dbap_aplid,
		    sizeof(DB_NAME), 
		    SXF_E_SECURITY,
		    I_SX201B_ROLE_ACCESS, 
		    SXF_A_FAIL | SXF_A_CREATE,
		    &e_error);
	    (VOID)qef_error(E_US18D3_6355_NOT_AUTH, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 1,
                             sizeof("CREATE ROLE")-1,
                             "CREATE ROLE");
	    qeu_cb->error.err_code = E_QE0025_USER_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
	break;
    }
    
    /* If no errors, attempt to add the role */

    if (status == E_DB_OK)
    for (;;)
    {
	i4		msg_id;
	i4		accessmask = SXF_A_SUCCESS;
	/* Like other QEU routines, this assumes one tuple per call */

	qeu_cb->qeu_count = qeuq_cb->qeuq_cagid;
	qeu_cb->qeu_tup_length = sizeof(DB_APPLICATION_ID);
	qeu_cb->qeu_input = qeuq_cb->qeuq_agid_tup;

	/* Add the tuple to iirole */

	status = qeu_append(qef_cb, qeu_cb);
	if (status != E_DB_OK)
	{
	    /* Hmmm.... Why am I here? */
	    qeu_cb->error.err_code  = E_QE022B_IIAPPLICATION_ID;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}
	else
	if (qeu_cb->qeu_count != qeuq_cb->qeuq_cagid)
	{
	    /* The applid must have existed already */
	    qeu_cb->error.err_code  = E_QE0218_APLID_EXISTS;
	    status = E_DB_WARN;
	    break;
	}

	/* This is a security event, audit it. */

	atuple = (DB_APPLICATION_ID *)qeuq_cb->qeuq_agid_tup->dt_data;

	if (!(qeuq_cb->qeuq_agid_mask & QEU_AAPLID))
	{
	    QEUQ_CB gr_qeuqcb;
	    DB_ROLEGRANT rgrtuple;
	    QEF_DATA     qefdata;
	    /*
	    ** If not altering the role then we must grant access
	    ** on the role to the creator
	    */
	    STRUCT_ASSIGN_MACRO(*qeuq_cb, gr_qeuqcb);
	    gr_qeuqcb.qeuq_agid_mask= QEU_GROLE;
	    gr_qeuqcb.qeuq_agid_tup= &qefdata;
	    qefdata.dt_data=(PTR)&rgrtuple;
	    qefdata.dt_size=sizeof(DB_ROLEGRANT);
	    qefdata.dt_next=NULL;
	    rgrtuple.rgr_flags=DBRG_ADMIN_OPTION;
	    MEfill(sizeof(rgrtuple.rgr_reserve),' ',(PTR)&rgrtuple.rgr_reserve);
	    MEmove(sizeof(aptuple->dbap_aplid), 
			(PTR)&aptuple->dbap_aplid, ' ',
			sizeof(rgrtuple.rgr_rolename),
			(PTR)&rgrtuple.rgr_rolename);

	    scf_cb.scf_length	= sizeof (SCF_CB);
	    scf_cb.scf_type	= SCF_CB_TYPE;
	    scf_cb.scf_facility	= DB_QEF_ID;
	    scf_cb.scf_session	= qef_cb->qef_ses_id;
	    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
	    scf_cb.scf_len_union.scf_ilength = 1;
	    sci_list[0].sci_length	= sizeof(username);
	    sci_list[0].sci_code	= SCI_USERNAME;
	    sci_list[0].sci_aresult	= (char *) username;
	    sci_list[0].sci_rlength	= NULL;

	    status = scf_call(SCU_INFORMATION, &scf_cb);
	    if (status != E_DB_OK)
	    {
	    	qeu_cb->error.err_code = E_QE022F_SCU_INFO_ERROR;
	    	status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    	break;
	    }
	    MEmove(sizeof(username), 
			(PTR)username, ' ',
			sizeof(rgrtuple.rgr_grantee),
			(PTR)&rgrtuple.rgr_grantee);

	    rgrtuple.rgr_gtype=DBGR_USER;

	    status=qeu_rolegrant(qef_cb, &gr_qeuqcb);
	    if(status!=E_DB_OK)
	    {
		qeu_cb->error.err_code=gr_qeuqcb.error.err_code;
		break;
	    }
	}
	/*
	** We must distinguish between create role and alter role for auditing
	*/
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    if (qeuq_cb->qeuq_agid_mask & QEU_AAPLID)
	    {
		msg_id = I_SX2021_ROLE_ALTER;
		accessmask |= SXF_A_ALTER;
	    }
	    else
	    {
		msg_id = I_SX2001_ROLE_CREATE;
		accessmask |= SXF_A_CREATE;
	    }
	    status = qeu_secaudit(FALSE, 
		    qef_cb->qef_ses_id, 
		    (char *)&atuple->dbap_aplid,
		    (DB_OWN_NAME *)NULL, 
		    sizeof(atuple->dbap_aplid), 
		    SXF_E_SECURITY, 
		    msg_id, 
		    accessmask, 
		    &e_error);

	    if (status != E_DB_OK)
	    {
		qeu_cb->error.err_code  = E_QE022B_IIAPPLICATION_ID;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	}
	break;
    
    } /* end for */

    return (status);
}

/*{
** Name: QEU_DAPLID	- delete application identifier information
**
** Internal call:   status = qeu_daplid(&qef_cb, &qeuq_cb, &qeu_cb, pdbpriv);
**
** Description:
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_cagid		number of application id tuples
**	    .qeuq_agid_tup	application id tuples
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	qeu_cb			pointer to qeu_cb opened on iirole
**	pdbpriv			TRUE - purge database privileges
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0219_APLID_NOT_EXIST
**				E_QE0228_DBPRIVS_REMOVED
**				E_QE022B_IIAPPLICATION_ID
**				E_QE022C_IIDBPRIV
**	Returns:
**	    E_DB_OK
**	    E_DB_WARN		Role does not exist
**	    E_DB_ERROR		Unable to process iirole.
**	Exceptions:
**	    none
**
** Side Effects:
**	    Removes tuples from the iirole catalog.
**
** History:
**	14-mar-89 (ralph)
**          written.
**	16-may-89 (ralph)
**	    check for dbprivs granted to the role to be deleted
**	12-jan-93
**	    fixed cast
**	3-sep-93 (stephenb)
**	    make sure that we do not call qeu_secaudit() if this is part
**	    of an alter role action.
**	3-nov-93 (robf)
**          Audit security label on drop role
**	7-mar-94 (robf)
**          Purge related grants.
**	28-mar-94 (robf)
**          Update error handling when trying to delete a non-existent role.
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
*/
DB_STATUS
qeu_daplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb,
bool		pdbpriv)
{
    DB_APPLICATION_ID *aptuple, *atuple;
    DB_STATUS	    status = E_DB_OK;
    DMR_ATTR_ENTRY  key_array[1];
    DMR_ATTR_ENTRY  *key_ptr_array[1];
    DB_ERROR	    e_error;
    DB_APPLICATION_ID aplid_temp;
    i4	    error;

    key_ptr_array[0] = &key_array[0];

    aptuple = (DB_APPLICATION_ID *)qeuq_cb->qeuq_agid_tup->dt_data;

    for (;;)
    {
	/* Like other QEU routines, this assumes one tuple per call */

	if(pdbpriv)
	{
	    /* 
	    ** Get the tuple  and see if we have MAC privilege to delete it 
	    ** Only do this if real delete (not via ALTER ROLE which calls
	    ** this during processing
	    */
	    status = qeu_gaplid(qef_cb, qeuq_cb, (char*)&aptuple->dbap_aplid, 	
			&aplid_temp);

	    if(status==E_DB_INFO)
	    {
		/* The role tuple was not found */
		qeu_cb->error.err_code  = E_QE0219_APLID_NOT_EXIST;
		status = E_DB_WARN;
		break;
	    }
	    else if (status!=E_DB_OK)
	    {
		qeu_cb->error.err_code  = E_QE022B_IIAPPLICATION_ID;
		status = E_DB_ERROR;
		break;
	    }

	    /* 
	    ** Purge any related role grants,
	    ** only do this if really dropping the role
	    */
	    status =qeu_prolegrant(qef_cb, qeuq_cb,
	    	(char*)NULL, 
	    	(char*)&aptuple->dbap_aplid);

	    if (status!=E_DB_OK)
	    {
	    	qeu_cb->error.err_code  = E_US247C_9340_IIROLEGRANT_ERROR;
	    	status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    	break;
	    }
	}
	/* Prepare to remove the tuple from iirole */

	qeu_cb->qeu_klen = 1;
	qeu_cb->qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_number = DM_1_APLID_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *) &aptuple->dbap_aplid;
	qeu_cb->qeu_tup_length = sizeof(DB_APPLICATION_ID);
	qeu_cb->qeu_qual = 0;
	qeu_cb->qeu_qarg = 0;

	/* Remove the tuple from ii_role */

	status = qeu_delete(qef_cb, qeu_cb);
	if (status != E_DB_OK)
	{
	    if (qeu_cb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		/* The role tuple was not found */
		qeu_cb->error.err_code  = E_QE0219_APLID_NOT_EXIST;
		status = E_DB_WARN;
	    }
	    else
	    {
		/* I give up.... Why am I here? */
		qeu_cb->error.err_code  = E_QE022B_IIAPPLICATION_ID;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	    break;
	}
	else
	if (qeu_cb->qeu_count != 1)
	{
	    /* The applid tuple was not found? */
	    qeu_cb->error.err_code  = E_QE0219_APLID_NOT_EXIST;
	    status = E_DB_WARN;
	    break;
	}

	/* This is a security event, must audit. */
	atuple = (DB_APPLICATION_ID *)qeuq_cb->qeuq_agid_tup->dt_data;
	/*
	** Only audit if this is not part of a alter role action,
	** if it is we will audit on the create.
	*/
	if ( !(qeuq_cb->qeuq_agid_mask & QEU_AAPLID) &&
	    Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
	      (char *)&atuple->dbap_aplid,(DB_OWN_NAME *)NULL, 
	      sizeof(atuple->dbap_aplid), SXF_E_SECURITY, 
	      I_SX2002_ROLE_DROP, SXF_A_SUCCESS | SXF_A_DROP, &e_error);

	    if (status != E_DB_OK)
	    {
		qeu_cb->error.err_code  = E_QE022B_IIAPPLICATION_ID;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	}
	break;
    
    } /* end for */

    /* Remove any database privileges defined for the role */

    if ((status == E_DB_OK) && pdbpriv)
    {
	i2  gtype = DBGR_APLID;

	status = qeu_pdbpriv(qef_cb, qeuq_cb,
			     (char *)&aptuple->dbap_aplid, (char *)&gtype);

	if (DB_FAILURE_MACRO(status))
	    qeu_cb->error.err_code = E_QE022C_IIDBPRIV;
	else if (status != E_DB_OK)
	    qeu_cb->error.err_code = E_QE0228_DBPRIVS_REMOVED;

    }

    return (status);
}

/*{
** Name: QEU_AAPLID	- alter application identifier information
**
** Internal	call:   status = qeu_aaplid(&qef_cb, &qeuq_cb, &qeu_cb);
**
** Description:
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_cagid		number of application id tuples
**	    .qeuq_agid_tup	application id tuples
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	qeu_cb			pointer to qeu_cb opened on iirole
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	set by qeu_[cd]aplid
**	Returns:
**	    Set bu qeu_[cd]aplid
**	Exceptions:
**	    none
**
** Side Effects:
**	    Alters tuples in the iirole catalog.
**
** History:
**	14-mar-89 (ralph)
**          written.
**	7-jan-94 (robf)
**	    Preserve security label across drop/insert.
**	8-mar-94 (robf)
**          Save change privilege and security audit masks
**	17-mar-94 (robf)
**          Include limiting label in privilege check for consistency 
**	    with create/alter user/profile
**	18-apr-94 (robf)
**          Preserve old privileges when altering.
**	26-Oct-2004 (schka24)
**	    Restore check for security, audit privs lost during B1 removal.
*/
DB_STATUS
qeu_aaplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb)
{
    DB_STATUS	    status, local_status;
    DB_APPLICATION_ID aplid_temp;
    DB_APPLICATION_ID *aptuple = (DB_APPLICATION_ID *)qeuq_cb->qeuq_agid_tup->dt_data;
    i4		error;
    DB_ERROR		e_error;

    /*
    ** I could have been fancy and added a qeu_replace
    ** routine, but it's easier just to delete it,
    ** and then replace it.  
    */

    for (;;)
    {

	/* Get the tuple  and see if we have MAC privilege to delete it */
	status = qeu_gaplid(qef_cb, qeuq_cb, (char*)&aptuple->dbap_aplid, 	
			&aplid_temp);
	if(status==E_DB_INFO)
	{
		/* The applid tuple was not found */
		qeu_cb->error.err_code  = E_QE0219_APLID_NOT_EXIST;
		status = E_DB_WARN;
		break;
	}
	else if(status!=E_DB_OK)
	{
		qeu_cb->error.err_code  = E_QE022B_IIAPPLICATION_ID;
		status = E_DB_ERROR;
		break;
	}

	/*
	** Don't grant security or audit if you don't own them.
	**
	*/
	if ( ( (aptuple->dbap_status & DU_USECURITY)
		&& !( qef_cb->qef_ustat & DU_USECURITY))
	  || ( (aptuple->dbap_status&DU_UAUDIT_PRIVS)
		&& !( qef_cb->qef_ustat & DU_UALTER_AUDIT)) )
	{
	    /*
	    ** Audit failed attempt to alter the role 
	    */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id, 
		    (char *)&aptuple->dbap_aplid,
		    &aptuple->dbap_aplid,
		    sizeof(DB_NAME), 
		    SXF_E_SECURITY,
		    I_SX201B_ROLE_ACCESS, 
		    SXF_A_FAIL | SXF_A_ALTER,
		    &e_error);
	    (VOID)qef_error(E_US18D3_6355_NOT_AUTH, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 1,
                             sizeof("CREATE ROLE")-1,
                             "CREATE ROLE");
	    qeu_cb->error.err_code = E_QE0025_USER_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
	/*
	** If not changing the role password, save the old one
	*/
	if (!(aptuple->dbap_flagsmask & DU_UPASS))
	{
		STRUCT_ASSIGN_MACRO(aplid_temp.dbap_apass, 
			aptuple->dbap_apass);
		/*
		** Include any previous regular or external flags mask
		*/
		aptuple->dbap_flagsmask|=
			 (aplid_temp.dbap_flagsmask&(DU_UPASS|DU_UEXTPASS));
	}
        if (aptuple->dbap_flagsmask & DU_UAPRIV)
	{
	    /*
	    ** Or old with new
	    */
	    aptuple->dbap_status |= aplid_temp.dbap_status; 
	}
        else if (aptuple->dbap_flagsmask & DU_UDPRIV)
	{
	    i4 privs;
	    privs=aplid_temp.dbap_status;	/* Old privs */
	    privs &= ~aptuple->dbap_status;     /* Remove new */
	    aptuple->dbap_status=privs;
	}
	else if (!(aptuple->dbap_flagsmask & DU_UPRIV))
	{
		/* No privileges specified so preserve old ones */
		aptuple->dbap_status=aplid_temp.dbap_status;
	}
	else
	{
		/* 
		** New privileges set, so only copy over any
		** non-privilege bits, like auditing.
		*/
		aptuple->dbap_status|= 
			(aplid_temp.dbap_status & ~DU_UALL_PRIVS);

	}
	/* Check security audit status */
	if (aptuple->dbap_flagsmask & DU_UQRYTEXT)
	    aptuple->dbap_status |= DU_UAUDIT_QRYTEXT;
	
	if (aptuple->dbap_flagsmask & DU_UNOQRYTEXT)
	    aptuple->dbap_status &= ~DU_UAUDIT_QRYTEXT;
	
	if (aptuple->dbap_flagsmask & DU_UALLEVENTS)
	    aptuple->dbap_status |= DU_UAUDIT;
	
	if (aptuple->dbap_flagsmask & DU_UDEFEVENTS)
	    aptuple->dbap_status &= ~DU_UAUDIT;
	/* Delete the applid tuple */

	status = qeu_daplid(qef_cb, qeuq_cb, qeu_cb, FALSE);

	if (status != E_DB_OK)
	    break;

	/* Insert the applid tuple */
	status = qeu_caplid(qef_cb, qeuq_cb, qeu_cb);

	break;
    
    } /* end for */

    /* Let the create/drop routine set the error code.... */

    return (status);
}

/*{
** Name: QEU_GAPLID - Obtain tuple from iirole
**
** Internal QEF call:   status = qeu_gaplid(&qef_cb, &qeuq_cb,
**					    &aplid, &aptuple)
**
** Description:
**	This procedure obtains requested tuple from the iirole catalog.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantics
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	aplid			the role's name
**				if null, blanks are used
**	aptuple			the iirole tuple buffer
**				if null, only status is returned.
**
** Outputs:
**	aptuple			filled with requested iirole tuple
**
**	Returns:
**	    E_DB_OK		Success; tuple was found
**	    E_DB_INFO		Success; tuple was not found.
**	    E_DB_ERROR		Failure
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	18-may-89 (ralph)
**          written
**	06-Aug-2002 (jenjo02)
**	    Init qeu_mask before calling qeu_open() lest DMF think we're
**	    passing bogus lock information (QEU_TIMEOUT,QEU_MAXLOCKS).
*/
DB_STATUS
qeu_gaplid(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*aplid,
DB_APPLICATION_ID   *aptuple)
{
    DB_STATUS	    status = E_DB_OK, local_status;
    QEU_CB	    qeu;
    QEF_DATA	    qef_data;
    DB_APPLICATION_ID altuple;
    DMR_ATTR_ENTRY  key_array[1];
    DMR_ATTR_ENTRY  *key_ptr_array[1];

    key_ptr_array[0] = &key_array[0];

    for (;;)
    {
	/* Open the iirole table */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_APLID_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_APLID_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_lk_mode = DMT_IS;
	qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_flag = 0;
	qeu.qeu_mask = 0;

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	/* Prepare to query the iirole table */

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DB_APPLICATION_ID);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DB_APPLICATION_ID);
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	qeu.qeu_klen = 1;
	qeu.qeu_key = key_ptr_array;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_number   = DM_1_APLID_KEY;
	key_ptr_array[0]->attr_value    = (char *)&altuple.dbap_aplid;

	/*
	** If aplid is null, use blanks.
	*/

	if (aplid == NULL)
	    MEfill(sizeof(altuple.dbap_aplid), (u_char)' ',
		   (PTR)&altuple.dbap_aplid);
	else
	    MEmove(DB_MAXNAME, (PTR) aplid , (char)' ',
		   sizeof(altuple.dbap_aplid),
		   (PTR)&altuple.dbap_aplid);

	/* If output tuple is null, use our own storage */

	if (aptuple == NULL)
	    qef_data.dt_data = (PTR)&altuple;
	else
	    qef_data.dt_data = (PTR) aptuple;

	/* Get a tuple from iirole */

	status = qeu_get(qef_cb, &qeu);
	
	/* Check the result */

	if ((status != E_DB_OK) &&
	    (qeu.error.err_code != E_QE0015_NO_MORE_ROWS))
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	else if (qeu.qeu_count == 0)
	    status = E_DB_INFO;
	else
	    status = E_DB_OK;

	/* Close the iirole table */

	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	    local_status = (local_status > E_DB_ERROR) ?
			    local_status : E_DB_ERROR;

	/* Set the status and exit */

	status = (status > local_status) ? status : local_status;
	
	break;
    }

    return(status);
}
