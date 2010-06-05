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
#include    <er.h>
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
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>

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
**  Name: QEURPRV.C - routines for maintaining role grants
**
**  Description:
**      The routines defined in this file provide an interface for
**	querymod to manipulate the contents of the iirolegrant catalog.
**	The operations supported include GRANT and REVOKE on roles plus
**	purging tuples as necessary
**
**	The following routines are defined in this file:
**
**	    qeu_rolegrant- dispatches iirolegrant processing routines
**	    qeu_rgrant	- appends tuples to iirolegrant
**	    qeu_rrevoke	- deletes member tuples from iirolegrant
**	    qeu_prolegrant- deletes all tuples associated with a specific member
**	    qeu_qrolegrant- qualifies iirolegrant tuples for qeu_prolegrant
**
**
**  History:
**	4-mar-94 (robf)
**	    Created 
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
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	12-Apr-2008 (kschendel)
**	    Rework DMF row qualification interface.
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**/

/*
**  Definition of static variables and forward static functions 
*/
static DB_STATUS qeu_qrolegrant(
    void		*toss,
    QEU_QUAL_PARAMS	*qparams);


/*{
** Name: QEU_ROLEGRANT	- Dispatch QEU routines to manipulate iirolegrant.
**
** External QEF call:   status = qef_call(QEU_ROLEGRANT, &qeuq_cb);
**
** Description:
**	This procedure checks the qeuq_dbpr_mask field of the QEUQ_CB
**	to determine which qrymod processing routine to call:
**		QEU_GROLE results in call to qeu_rgrant()
**		QEU_RROLE results in call to qeu_rrevoke()
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_db_id		database id
**	    .qeuq_agid_mask	iirolegrant operation code
**	    .qeuq_agid_tup	iirolegrant tuple
**
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**				E_QE0017_BAD_CB
**	Returns:
**	    E_DB_OK
**	    E_DB_ERROR
**	    E_DB_SEVERE
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	4-mar-94 (robf)
**          written
**	10-Jan-2001 (jenjo02)
**	    We know this session'd id; pass it to SCF.
*/
DB_STATUS
qeu_rolegrant(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DB_ROLEGRANT    *rgrtuple = NULL;
    DB_STATUS	    status, local_status;
    i4	    error = 0;
    bool	    transtarted = FALSE;
    bool	    tbl_opened = FALSE;
    QEU_CB	    tranqeu;
    QEU_CB	    qeu;
    char	    *tempstr;
    i4	    glen, mlen;
    SCF_CB	    scf_cb;
    SCF_SCI	    sci_list[3];
    i4		    user_status;
    char	    username[DB_OWN_MAXNAME];
    char	    dbname[DB_DB_MAXNAME];
    SXF_ACCESS	    sxfaccess;
   i4	    mesgid;
    char	    *rolename;
    DB_ERROR	    e_error;

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

	if (!qeuq_cb->qeuq_agid_tup)
		break;

        rgrtuple = (DB_ROLEGRANT *)qeuq_cb->qeuq_agid_tup->dt_data;

	if(rgrtuple==NULL)
		break;


	error=0;
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

	/* 
	** Check general access privileges:
	** - must be connected to iidbdb
	** - must be MAINTAIN_USERS privilege
	*/
	/* Get info about the user and current database */

	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type		= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_QEF_ID;
	scf_cb.scf_session	= qef_cb->qef_ses_id;
	scf_cb.scf_ptr_union.scf_sci	 = (SCI_LIST *) sci_list;
	scf_cb.scf_len_union.scf_ilength = 3;
	sci_list[0].sci_length	= sizeof(dbname);
	sci_list[0].sci_code	= SCI_DBNAME;
	sci_list[0].sci_aresult	= (char *) dbname;
	sci_list[0].sci_rlength	= NULL;
	sci_list[1].sci_length	= sizeof(user_status);
	sci_list[1].sci_code	= SCI_USTAT;
	sci_list[1].sci_aresult	= (char *) &user_status;
	sci_list[1].sci_rlength	= NULL;
	sci_list[2].sci_length	= sizeof(username);
	sci_list[2].sci_code	= SCI_USERNAME;
	sci_list[2].sci_aresult	= (char *) username;
	sci_list[2].sci_rlength	= NULL;

	status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    error = E_QE022F_SCU_INFO_ERROR;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;

	}
	
	/* Make sure we are in the iidbdb */

	if (MEcmp((PTR)dbname, (PTR)DB_DBDB_NAME, sizeof(dbname)))
	{
	    (VOID)qef_error(E_US247E_9342_ROLE_RG_DBDB, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 0);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_ERROR;
	    break;

	}

	/* Check status, must have MAINTAIN_USER privileges */
	if (!(user_status & DU_UMAINTAIN_USER))
	{
	    (VOID)qef_error(E_US247F_9343_ROLE_RG_NOT_AUTH, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 0);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_ERROR;
	    break;
	}

	/* prepare the qeu_cb and open the table for processing */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_IIROLEGRANT_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_IIROLEGRANT_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;
         	
	/* open the iirolegrant table */

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    (VOID) qef_error(E_US247B_9339_IIROLEGRANT_OPEN, 
			0L, status, &error, &qeuq_cb->error, (i4)0);
	    error = E_QE0025_USER_ERROR;
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}
	tbl_opened = TRUE;

	/* Call the appropriate group function */

	switch (qeuq_cb->qeuq_agid_mask & (QEU_GROLE | QEU_RROLE ))
	{
	    case QEU_GROLE:
		status = qeu_rgrant(qef_cb, qeuq_cb, &qeu);
		error = qeu.error.err_code;
		break;

	    case QEU_RROLE:
		status = qeu_rrevoke(qef_cb, qeuq_cb, &qeu);
		error = qeu.error.err_code;
		break;

	    default:
		status = E_DB_SEVERE;
		error = E_QE019D_INVALID_ROLEGRANT_OP;
		break;
	}
	
	break;

    
    } /* end for */

    /* call qef_error to handle error messages, if any */

    if (status != E_DB_OK)
    {
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, (i4)0);
    }

    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	/* This is a security event, audit it. */
	if(status==E_DB_OK)
	    sxfaccess=(SXF_A_SUCCESS|SXF_A_CONTROL);
	else
	    sxfaccess=(SXF_A_FAIL|SXF_A_CONTROL);

	if( qeuq_cb->qeuq_agid_mask & QEU_GROLE )
	    mesgid=I_SX274E_ROLE_GRANT;
	else 	
	    mesgid=I_SX274F_ROLE_REVOKE;

	if(rgrtuple)
		rolename= (char*)&rgrtuple->rgr_rolename;
	else
		rolename=NULL;    

	local_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
	      rolename, 
	      (DB_OWN_NAME *)NULL, 
	      DB_OWN_MAXNAME,
	      SXF_E_SECURITY,
	      mesgid,
	      sxfaccess,
	      &e_error);

	if (local_status > status)
	    status = local_status;
    }

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

    /* End the transaction if one was started */

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
** Name: QEU_RGRANT	- Grant on Role
** 
** Internal	call:   status = qeu_rgrant(&qef_cb, &qeuq_cb, &qeu_cb);
**
** Description:
**	Appends tuples to the iirolegrant catalog.
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_agid_tup	rolegrant tuples
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	qeu_cb			pointer to qeu_cb opened on iirolegrant
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**	Returns:
**	Exceptions:
**	    none
**
** Side Effects:
**	    Stores tuples in the iiusergroup catalog.
**
** History:
**	4-mar-94 (robf)
**          written.
**	15-mar-94 (robf)
**          add MAC check for access to the role.
**	26-Oct-2004 (schka24)
**	    Fix IF test broken by B1 removal.
*/
DB_STATUS
qeu_rgrant(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb)
{
    DB_ROLEGRANT    *rgrtuple, rantuple;
    DB_STATUS	    status = E_DB_OK;
    DB_ERROR	    err;
    i4	    error = 0;
    i4		    i;
    QEF_DATA	    qef_data;
    DMR_ATTR_ENTRY  key_array[1];
    DMR_ATTR_ENTRY  *key_ptr_array[1];
    DB_ERROR	     e_error;
    DB_APPLICATION_ID aptuple;
    
    for (i=0; i< 1; i++)
	key_ptr_array[i] = &key_array[i];

    rgrtuple = (DB_ROLEGRANT *)qeuq_cb->qeuq_agid_tup->dt_data;

    for(;;)
    {
	/* Check the iirole catalog to ensure role exists */

	status = qeu_gaplid(qef_cb, qeuq_cb,
			   (char *)&rgrtuple->rgr_rolename, &aptuple);

	if (DB_FAILURE_MACRO(status))		/* Fatal error! */
	    error = E_QE022B_IIAPPLICATION_ID;
	else if (status != E_DB_OK)		/* Role does not exist */
	{
	    (VOID)qef_error(E_US1875_APLID_NOT_EXIST, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(rgrtuple->rgr_rolename),
					(char *)&rgrtuple->rgr_rolename),
			    (PTR)&rgrtuple->rgr_rolename);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
	}
	else					
	    status = E_DB_OK;

	/* Terminate if error occurred */

	if (status != E_DB_OK)
	    break;
	
	/*
	** Check for granting privs or audits to other users  
	** via roles when not security/maintain_audit.
	*/
	if ( ((aptuple.dbap_status & DU_USECURITY) && !( qef_cb->qef_ustat &DU_USECURITY))
	  || ((aptuple.dbap_status&DU_UAUDIT_PRIVS) && !( qef_cb->qef_ustat &DU_UALTER_AUDIT)) )
	{
	    (VOID)qef_error(E_US247F_9343_ROLE_RG_NOT_AUTH, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 0);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_ERROR;
	    break;
	}
	/* If user grantee then check user exists */

	if (rgrtuple->rgr_gtype==DBGR_USER)
	{
		status = qeu_guser(qef_cb, qeuq_cb,
			   (char *)&rgrtuple->rgr_grantee, NULL, NULL);

		if (DB_FAILURE_MACRO(status))		/* Fatal error! */
	    		error = E_QE022E_IIUSER;
		else if (status != E_DB_OK)		/* User not exists */
		{
		      (VOID)qef_error(E_US18B7_6327_USER_ABSENT, 0L,
                                    E_DB_ERROR, &error, &qeuq_cb->error, 1,
                                    qec_trimwhite(sizeof(rgrtuple->rgr_grantee),
                                                (char *)&rgrtuple->rgr_grantee),
                                    (PTR)&rgrtuple->rgr_grantee);
                        error = E_QE0025_USER_ERROR;
                        status = E_DB_WARN;
			break;
		}
	}
	break;
    }

    /*
    ** Now we are done checks, perform the semantic action
    */
    if (status == E_DB_OK)
    for (;;)
    {
	qeu_cb->qeu_count = 1;
	qeu_cb->qeu_tup_length = sizeof(DB_ROLEGRANT);
	qeu_cb->qeu_input = qeuq_cb->qeuq_agid_tup;
	status = qeu_append(qef_cb, qeu_cb);
	if (status != E_DB_OK)
	{
	    error = E_US247C_9340_IIROLEGRANT_ERROR;
	    break;
	}
	else
	if (qeu_cb->qeu_count != 1)
	{
	    /*
	    ** Assume grant already there, so silently succeed
	    */
	    error  = 0;
	    status = E_DB_OK;
	}
	
	break;
    
    } /* end for */

    qeu_cb->error.err_code = error;
    return (status);
}

/*{
** Name: QEU_RREVOKE	- Revoke rolegrants information
**
** Internal	call:   status = qeu_rrevoke(&qef_cb, &qeuq_cb, &qeu_cb);
**
** Description:
**
** Inputs:
**      qef_cb                  qef session control block
**      qeuq_cb
**	    .qeuq_eflag	        designate error handling semantis
**				for user errors.
**		QEF_INTERNAL	return error code.
**		QEF_EXTERNAL	send message to user.
**	    .qeuq_agid_tup	rolegrant tuples
**	    .qeuq_db_id		database id
**	    .qeuq_d_id		dmf session id
**	qeu_cb			pointer to qeu_cb opened on iirolegrant
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	one of the following
**	    none
**
** Side Effects:
**	    Removes tuples from the iirolegrant catalog.
**
** History:
**	4-mar-94 (robf)
**          written.
**	15-mar-94 (robf)
**          Add MAC check
**      06-mar-1996 (nanpr01)
**          Change for variable page size project. Previously qeu_delete
**          was using DB_MAXTUP to get memory. With the bigger pagesize
**          it was unnecessarily allocating much more space than was needed.
**          Instead we initialize the correct tuple size, so that we can
**          allocate the actual size of the tuple instead of max possible
**          size.
*/
DB_STATUS
qeu_rrevoke(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
QEU_CB		*qeu_cb)
{
    DB_ROLEGRANT    *rgrtuple;
    DB_STATUS	    status = E_DB_OK;
    DB_ERROR	    e_error;
    i4	    error = 0;
    i4		    i;
    bool	    purge;
    bool	    drop;
    QEF_DATA	    qef_data;
    DMR_ATTR_ENTRY  key_array[2];
    DMR_ATTR_ENTRY  *key_ptr_array[2];
    DB_APPLICATION_ID aptuple;

    for (i=0; i<2; i++)
	key_ptr_array[i] = &key_array[i];

    rgrtuple = (DB_ROLEGRANT *)qeuq_cb->qeuq_agid_tup->dt_data;

    for (;;)
    {
	/*
	** Check role exists
	*/
	status = qeu_gaplid(qef_cb, qeuq_cb,
			   (char *)&rgrtuple->rgr_rolename, &aptuple);

	if (DB_FAILURE_MACRO(status))		/* Fatal error! */
	    error = E_QE022B_IIAPPLICATION_ID;
	else if (status != E_DB_OK)		/* Role does not exist */
	{
	    (VOID)qef_error(E_US1875_APLID_NOT_EXIST, 0L,
			    E_DB_ERROR, &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(rgrtuple->rgr_rolename),
					(char *)&rgrtuple->rgr_rolename),
			    (PTR)&rgrtuple->rgr_rolename);
	    error = E_QE0025_USER_ERROR;
	    status = E_DB_WARN;
	}
	else					
	    status = E_DB_OK;

	/* Terminate if error occurred */

	if (status != E_DB_OK)
	    break;
	
	/* Like other QEU routines, this assumes one tuple per call */

	qeu_cb->qeu_getnext = QEU_REPO;
        qeu_cb->qeu_tup_length = sizeof(DB_ROLEGRANT);
	qeu_cb->qeu_klen = 2;       
	qeu_cb->qeu_key = key_ptr_array;
	/*
	** Key by grantee, rolename
	*/
	key_ptr_array[0]->attr_number = DM_1_IIROLEGRANT_KEY;
	key_ptr_array[0]->attr_operator = DMR_OP_EQ;
	key_ptr_array[0]->attr_value = (char *) &rgrtuple->rgr_grantee;
	key_ptr_array[1]->attr_number = DM_2_IIROLEGRANT_KEY;
	key_ptr_array[1]->attr_operator = DMR_OP_EQ;
	key_ptr_array[1]->attr_value = (char *) &rgrtuple->rgr_rolename;
	qeu_cb->qeu_qual = 0;
	qeu_cb->qeu_qarg = 0;

	/* Delete the tuple(s) from iirolegrant */

	status = qeu_delete(qef_cb, qeu_cb);
	if (status != E_DB_OK)			
	{
	    if (qeu_cb->error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
	        /* The user tried to drop a non-existing grant*/
		status=E_DB_OK;
	    }
	    else
	    {
		/* Some other error */
		error = qeu_cb->error.err_code;
		status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    }
	    break;	
	}
	break;
    }
    qeu_cb->error.err_code = error;
    return (status);
}

/*{
** Name: QEU_PROLEGRANT - Remove all iirolegrant tuples associated with 
**			  a grantee and/or rolename.
**
** Internal QEF call:   status = qeu_prolegrant(&qef_cb, &qeuq_cb, &grantee)
**
** Description:
**	This function purges requested tuples from the iirolegrant catalog.
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
**	grantee			the grantee's name, may be NULL
**	rolename		the rolename, may be NULL
**
** Outputs:
**	None
**
**	Returns:
**	    E_DB_OK		Success; no tuples were deleted.
**	    E_DB_INFO		Success; one or more tuples deleted.
**	    E_DB_ERROR		Failure.
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	4-mar-94 (robf)
**          written
**	06-Aug-2002 (jenjo02)
**	    Init qeu_mask before calling qeu_open() lest DMF think we're
**	    passing bogus lock information (QEU_TIMEOUT,QEU_MAXLOCKS).
*/
DB_STATUS
qeu_prolegrant(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
char		*grantee,
char		*rolename)
{
    i4		    i;
    DB_STATUS	    status = E_DB_OK, local_status;
    QEU_CB	    qeu;
    QEU_QUAL_PARAMS qparams;
    DB_ROLEGRANT    qualtuple;
    PTR		    qual_parms[5];
    i4		    audit;
    i4	    error;
    DB_ROLEGRANT    rgrtuple;
    QEF_DATA	    qef_data;

    for (;;)
    {
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    audit = TRUE;
	}
	else
	    audit = FALSE;

	/* Open the iirolegrant table */

	qeu.qeu_type = QEUCB_CB;
	qeu.qeu_length = sizeof(QEUQ_CB);
	qeu.qeu_tab_id.db_tab_base  = DM_B_IIROLEGRANT_TAB_ID;
	qeu.qeu_tab_id.db_tab_index = DM_I_IIROLEGRANT_TAB_ID;
	qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
	qeu.qeu_flag = QEU_BYPASS_PRIV;
	qeu.qeu_lk_mode = DMT_IX;
	qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask = 0;

	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
	    break;
	}

	/* Prepare to process the iirolegrant table */

	qeu.qeu_getnext = QEU_REPO;

	qeu.qeu_klen 	= 0;       
	qeu.qeu_key 	= NULL;
	qeu.qeu_qual 	= qeu_qrolegrant;
	qeu.qeu_qarg 	= &qparams;
	qef_data.dt_next = 0;
	qef_data.dt_size = sizeof(DB_ROLEGRANT);
	qef_data.dt_data = (PTR)&rgrtuple;
	qeu.qeu_output = &qef_data;
        qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_ROLEGRANT);

	qparams.qeu_qparms[0]	= (PTR)qef_cb;
	qparams.qeu_qparms[1]	= (PTR)grantee;
	qparams.qeu_qparms[2]	= (PTR)&audit;
	qparams.qeu_qparms[3]   = (PTR)rolename;

	/* Delete tuples from iirolegrant */

	do 
	{
	    /*
	    ** Search for qualifying rows
	    */
	    status = qeu_get(qef_cb, &qeu);
	    if ((status != E_DB_OK) &&
	        (qeu.error.err_code != E_QE0015_NO_MORE_ROWS))
	    {
	        status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		break;
	    }
	    else if (qeu.qeu_count != 1)
	    {
		/*
		** No rows found
		*/
	        status = E_DB_OK;
		break;
	    }

	    qeu.qeu_getnext = QEU_NOREPO;

	    /*
	    ** Now delete the current row
	    */
	    status = qeu_delete(qef_cb, &qeu);

	    /* Check the result */

	    if ((status != E_DB_OK &&
	        (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)) ||
		qeu.qeu_count==0)
	    {
		/*
		** Error deleting or no row found
		*/
	        status = (status > E_DB_ERROR) ? status : E_DB_ERROR;
		break;
	    }

	} while (status==E_DB_OK);
	/* Close the iirolegrantee table */

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

/*{
** Name: QEU_QROLEGRANT- Qualify an iirolegrant tuple by grantee with audit
**
** External QEF call:   status = (*que_qual)(qeu_qarg, &current_tuple)
**
** Description:
**	This procedure qualifies an iirolegrant tuple by grantee name.
**	It is called by DMF as part of tuple qualification when
**	deleting tuples for qeu_duser.
**
** Inputs:
**	qparams.qeu_qparms[]	Array of pointers to qualification parameters:
**	    qef_cb		    pointer to the current qef_cb
**	    grantee		    grantee name
**	    audit		    if TRUE, audit record as being dropped.
**	    rolename		    rolename
**	qparams.qeu_rowaddr	Pointer to iirolegrant row being qualified
**
** Outputs:
**	qparams.qeu_retval	ADE_TRUE if row qualifies, else ADE_NOT_TRUE
**
**	Returns: E_DB_OK or audit error status
**	Exceptions:
**	    none
**
** Side Effects:
**	    Produces audit records indicating the member is dropped 
**	    from the group if audit is TRUE.
**
** History:
**	4-mar-94 (robf)
**          written
**	11-Apr-2008 (kschendel)
**	    Update to new style calling sequence.
*/
static DB_STATUS
qeu_qrolegrant(
    void *toss, QEU_QUAL_PARAMS *qparams)
{
    DB_ROLEGRANT    *rgr_tuple;
    QEF_CB          *qef_cb;
    char	    *grantee;
    char            *rolename;
    i4		    *audit;
    i4	    error = 0;
    DB_ERROR	    err;
    DB_STATUS	    status;

    rgr_tuple = (DB_ROLEGRANT *) qparams->qeu_rowaddr;

    qef_cb	= (QEF_CB *)qparams->qeu_qparms[0];
    grantee	= (char *)qparams->qeu_qparms[1];
    audit	= (i4 *)qparams->qeu_qparms[2];
    rolename	= (char *)qparams->qeu_qparms[3];

    qparams->qeu_retval = ADE_NOT_TRUE;
    if(grantee && MEcmp((PTR)grantee, 
	      (PTR)&rgr_tuple->rgr_grantee,
	      sizeof(rgr_tuple->rgr_grantee))
	    != 0)

	return(E_DB_OK);				/* Unqualified.... */

    if(rolename && MEcmp((PTR)rolename, 
	      (PTR)&rgr_tuple->rgr_rolename,
	      sizeof(rgr_tuple->rgr_rolename))
	    != 0)

	return(E_DB_OK);				/* Unqualified.... */

    /* The tuple is qualified.  Audit if reqested */

    qparams->qeu_retval = ADE_TRUE;
    status = E_DB_OK;
    if ( *audit && Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	DB_ERROR	e_error;

	status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
                rgr_tuple->rgr_grantee.db_own_name,
		(DB_OWN_NAME *)NULL, DB_OWN_MAXNAME,
                SXF_E_SECURITY, 
		I_SX274F_ROLE_REVOKE,
		SXF_A_SUCCESS | SXF_A_CONTROL, 
		&e_error);

    }

    return(status);
}
