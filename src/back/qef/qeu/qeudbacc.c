/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
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
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <dmacb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <usererror.h>

#include    <ade.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <tm.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>
/**
**
**  Name: QEUDBACESS.C - routines for maintaining access to databases
**
**  Description:
**	The routines defined in this file provide an interface for
**	qyrmod to manipulate the contents of the iidbaccess catalog.
**
**	The following routine(s) are defined in this file:
**
**	    qeu_cdbacc	- appends iidbaccess tuple
**	    qeu_ddbacc	- deletes iidbaccess tuple
**
**  History:
**      25-jun-89 (jennifer)
**          Created for security.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
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
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**/

/*
**  Definition of static variables and forward static functions 
*/

/*{
** Name: qeu_cdbacc	- Store database access information for one user.
**
** External QEF call:   status = qef_call(QEU_CDBACC, &qeuq_cb);
**
** Description:
**	Add the iidbaccess tuple.
**
**      This procedure insures that the database user combination
**      name provided is unqiue.
**	If a match is found then the entry is a duplicate and an error is
**	returned.  If this is unique then the entry is entered into 
**      iidbaccess.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of tuples.  Must be 1.
**	    .qeuq_uld_tup	iidbaccess tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**                              E_US18BC_6332_DBACC_EXISTS
**                              E_US18BB_6331_USR_UNKNOWN
**                              E_US18BA_6330_DB_UNKNOWN
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	25-jun-89 (jennifer)
**	    Written for Security.
**	08-dec-1992 (pholman)
**	    C2: replace obsolete qeu_audit with qeu_secaudit.
**	12-jan-93 (rickh?)
**	    fixed cast
**	6-jul-93 (robf)
**	    Pass security label to qeu_secaudit()
**	8-oct-93 (stephenb)
**	    eliminated erroneous variable "err" and fixed error handling in 
**	    call to qeu_secaudit().
**	18-feb-95 (forky01)
**	    qeucb getting passed uninitialized qeu_mask which caused random
**	    maxlocks and lock timeout.
*/

DB_STATUS
qeu_cdbacc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_DBACCESS 	*dtuple;	/* New entry tuple */
    DU_DBACCESS 	dtuple_temp;	/* Tuple to check for uniqueness */
    DU_USER             utuple_temp;
    DU_DATABASE         dbtuple_temp;
    DB_STATUS	    	status, local_status;
    DB_ERROR 		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;		
    QEF_DATA	    	*next;		
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	dkey_array[2];
    DMR_ATTR_ENTRY  	*dkey_ptr_array[2];

    for (;;)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   (qeuq_cb->qeuq_culd != 1 || qeuq_cb->qeuq_uld_tup == NULL)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}
		
	/* 
	** Check to see if transaction is in progress, if so set a local
	** transaction, otherwise we'll use the user's transaction.
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
	/* Escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	dtuple = (DU_DBACCESS *)qeuq_cb->qeuq_uld_tup->dt_data;

    
	/* Validate the user name. */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_USER_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_USER_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named user - if not there then error */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_USER);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_USER);
        qef_data.dt_data = (PTR)&utuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = dkey_ptr_array;
	dkey_ptr_array[0] = &dkey_array[0];
	dkey_ptr_array[1] = &dkey_array[1];
	dkey_ptr_array[0]->attr_number = DM_1_USER_KEY ;
	dkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	dkey_ptr_array[0]->attr_value =
				    (char *)&dtuple->du_usrname;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    local_status = qeu_close(qef_cb, &qeu);
	    if (local_status != E_DB_OK)
	    {
		(VOID) qef_error(qeu.error.err_code, 0L, local_status, &error, 
				 &qeuq_cb->error, 0);
		if (local_status > status)
		    status = local_status;
		tbl_opened = FALSE;
		break;
	    }
	}
	if (status == E_QE0015_NO_MORE_ROWS)	/* NO ENTRY! */
	{
	    (VOID)qef_error(E_US18BB_6331_USR_UNKNOWN, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(dtuple->du_usrname),
					  (char *)&dtuple->du_usrname),
			    (PTR)&dtuple->du_usrname);
	    error = E_QE0025_USER_ERROR;
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}


	/* Validate the database name. */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named database - if not there then error*/
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_DATABASE);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_DATABASE);
        qef_data.dt_data = (PTR)&dbtuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = dkey_ptr_array;
	dkey_ptr_array[0] = &dkey_array[0];
	dkey_ptr_array[1] = &dkey_array[1];
	dkey_ptr_array[0]->attr_number = DM_1_DATABASE_KEY ;
	dkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	dkey_ptr_array[0]->attr_value =
				    (char *)dtuple->du_dbname;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);
	if (status == E_DB_OK)
	{
	    local_status = qeu_close(qef_cb, &qeu);
	    if (local_status != E_DB_OK)
	    {
		(VOID) qef_error(qeu.error.err_code, 0L, local_status, &error, 
				 &qeuq_cb->error, 0);
		if (local_status > status)
		    status = local_status;
		tbl_opened = FALSE;
		break;
	    }
	}
	if (status == E_QE0015_NO_MORE_ROWS)	/* NO Match! */
	{
	    (VOID)qef_error(E_US18BA_6330_DB_UNKNOWN, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(dtuple->du_dbname),
					  (char *)dtuple->du_dbname),
			    (PTR)dtuple->du_dbname);
	    error = E_QE0025_USER_ERROR;
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}
	/* Validate that the user name, database name is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DBACCESS_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DBACCESS_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named user/database pair - if not there then ok */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_DBACCESS);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_DBACCESS);
        qef_data.dt_data = (PTR)&dtuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 2;			
	qeu.qeu_key = dkey_ptr_array;
	dkey_ptr_array[0] = &dkey_array[0];
	dkey_ptr_array[1] = &dkey_array[1];
	dkey_ptr_array[0]->attr_number = DM_1_DBACCESS_KEY ;
	dkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	dkey_ptr_array[0]->attr_value =
				    (char *)&dtuple->du_usrname;
	dkey_ptr_array[1]->attr_number = DM_2_DBACCESS_KEY ;
	dkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	dkey_ptr_array[1]->attr_value =
				    (char *)dtuple->du_dbname;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);
	if (status == E_DB_OK)		/* Found the same entry! */
	{
	    (VOID)qef_error(E_US18BC_6332_DBACC_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(dtuple->du_dbname),
					  (char *)dtuple->du_dbname),
			    (PTR)dtuple->du_dbname);
	    error = E_QE0025_USER_ERROR;
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}
	/* The entry is unique - append it */

	status = E_DB_OK;


	/* Insert single iidbaccess tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_DBACCESS);
	qeu.qeu_input = qeuq_cb->qeuq_uld_tup;
	status = qeu_append(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	/* This is a security event, write audit record. */

	/* This is a security event, audit it. */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)dtuple->du_dbname,
			    &dtuple->du_usrname,
			    sizeof(dtuple->du_dbname), SXF_E_DATABASE,
			    I_SX2003_DBACCESS_CREATE, SXF_A_SUCCESS | SXF_A_CREATE,
			    &e_error);

	    if (status != E_DB_OK)
	    {
		error = e_error.err_code;
		break;
	    }
	}

	if (transtarted)		   /* If we started a transaction */
	{
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	}
	qeuq_cb->error.err_code = E_QE0000_OK;
	return (E_DB_OK);
    } /* End dummy for */

    /* call qef_error to handle error messages */
    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off all the tables. */
    local_status = E_DB_OK;
    if (tbl_opened)		    /* If system table opened, close it */
    {
	local_status = qeu_close(qef_cb, &qeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(qeu.error.err_code, 0L, local_status, &error, 
		    	     &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (transtarted)		   /* If we started a transaction */
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
    return (status);
} /* qeu_cuser */

/*{
** Name: qeu_ddbacc	- Drop single iidbaccess entry.
**
** External QEF call:   status = qef_call(QEU_DDBACC, &qeuq_cb);
**
** Description:
**	Drop the iidbaccess tuple.
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Must be 1 if DROP entry.
**	    .qeuq_uld_tup	database access tuple to be deleted.
**		.du_usrname	user name.
**		.du_dbname	database name.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**                              E_US18BD_6333_DBACC_ABSENT
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	25-jun-89 (jennifer)
**	    Written for Security/DBACCESS.
**	12-jan-93
**	    fixed cast
**	18-feb-95 (forky01)
**	    qeucb getting passed uninitialized qeu_mask which caused random
**	    maxlocks and lock timeout.
*/

DB_STATUS
qeu_ddbacc(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    QEU_CB	    	tranqeu;	/* For transaction request */
    bool	    	transtarted = FALSE;	    
    QEU_CB	    	dqeu;		/* For IIDBACCESS table */
    QEF_DATA	    	dqef_data;
    DU_DBACCESS	    	*dtuple_name;	/* Input tuple with name. */
    DU_DBACCESS	    	dtuple;		/* Tuple currently being deleted */
    bool	    	user_opened = FALSE;
    DMR_ATTR_ENTRY  	dkey_array[2];
    DMR_ATTR_ENTRY  	*dkey_ptr_array[2];
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;

    for (;;)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}
	if (   (qeuq_cb->qeuq_culd == 0)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}


	/* 
	** Check to see if transaction is in progress, if so set a local
	** transaction, otherwise we'll use the user's transaction.
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
	/* Escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	/* Open iidbaccess. */
	dqeu.qeu_type = QEUCB_CB;
        dqeu.qeu_length = sizeof(QEUCB_CB);
        dqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        dqeu.qeu_lk_mode = DMT_IX;
        dqeu.qeu_flag = DMT_U_DIRECT;
        dqeu.qeu_access_mode = DMT_A_WRITE;

        dqeu.qeu_tab_id.db_tab_base = DM_B_DBACCESS_TAB_ID; /* Open iidbaccess*/
        dqeu.qeu_tab_id.db_tab_index = DM_I_DBACCESS_TAB_ID; 	
	dqeu.qeu_mask=0;
	status = qeu_open(qef_cb, &dqeu);
	if (status != E_DB_OK)
	{
	    error = dqeu.error.err_code;
	    break;
	}
	user_opened = TRUE;

	dqeu.qeu_count = 1;	   	    /* Initialize user-specific qeu */
        dqeu.qeu_tup_length = sizeof(DU_DBACCESS);
	dqeu.qeu_output = &dqef_data;
	dqef_data.dt_next = NULL;
        dqef_data.dt_size = sizeof(DU_DBACCESS);
        dqef_data.dt_data = (PTR)&dtuple;

	dtuple_name = (DU_DBACCESS *)qeuq_cb->qeuq_uld_tup->dt_data;

	dqeu.qeu_getnext = QEU_REPO;
	dqeu.qeu_klen = 2;		
	dqeu.qeu_key = dkey_ptr_array;
	dkey_ptr_array[0] = &dkey_array[0];
	dkey_ptr_array[1] = &dkey_array[1];
	dkey_ptr_array[0]->attr_number = DM_1_DBACCESS_KEY;
	dkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	dkey_ptr_array[0]->attr_value =
				(char *)&dtuple_name->du_usrname;
	dkey_ptr_array[1]->attr_number = DM_2_DBACCESS_KEY;
	dkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	dkey_ptr_array[1]->attr_value =
				(char *)dtuple_name->du_dbname;
	dqeu.qeu_qual = NULL;
	dqeu.qeu_qarg = NULL;
	for (;;)		
	{
	    status = qeu_get(qef_cb, &dqeu);
	    if (status != E_DB_OK)
	    {
		if (dqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		{
			(VOID)qef_error(E_US18BD_6333_DBACC_ABSENT, 0L,
				    E_DB_ERROR, &error, &qeuq_cb->error, 1,
				    qec_trimwhite(sizeof(dtuple_name->du_dbname),
						(char *)dtuple_name->du_dbname),
				    (PTR)dtuple_name->du_dbname);
			error = E_QE0025_USER_ERROR;
		}
		else	/* Other error */
		{
		    error = dqeu.error.err_code;
		}
		break;
	    } /* If no entry found */
	    dqeu.qeu_getnext = QEU_NOREPO;
            dqeu.qeu_klen = 0;
	    
	    /* Now delete the current tuple */
	    status = qeu_delete(qef_cb, &dqeu);
	    if (status != E_DB_OK)
	    {
		error = dqeu.error.err_code;
		break;
	    }
		
	    /* This is a security event, write audit record. */

	    /* This is a security event, audit it. */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
		status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
			    (char *)dtuple_name->du_dbname,
			    &dtuple_name->du_usrname,
			    sizeof(dtuple_name->du_dbname), SXF_E_DATABASE,
			    I_SX2004_DBACCESS_DROP, SXF_A_SUCCESS | SXF_A_DROP,
			    &e_error);

		error = e_error.err_code;
	    }

	    if (status != E_DB_OK)
	    {
		break;
	    }

	} /* For all entry found */

	if (status != E_DB_OK)
	    break;

    } /* End dummy for loop */

    /* Handle any error messages */
    if (status != E_DB_OK)
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off all the tables and transaction */
    if (user_opened)
    {
	local_status = qeu_close(qef_cb, &dqeu);    
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(dqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    if (transtarted)
    {
	if (status == E_DB_OK)
	    local_status = qeu_etran(qef_cb, &tranqeu);
	else
	    local_status = qeu_atran(qef_cb, &tranqeu);
	if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_duser */
