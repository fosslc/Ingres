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
#include    <cs.h>
#include    <lk.h>
#include    <tm.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
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
**  Name: QEULOC.C - routines for maintaining locations
**
**  Description:
**	The routines defined in this file provide an interface for
**	qyrmod to manipulate the contents of the iilocations catalog.
**      Note, you can never delete a location, since old backups
**      may refer to it.
**
**	The following routine(s) are defined in this file:
**
**	    qeu_cloc	- appends a location tuple
**
**  History:
**      25-jun-89 (jennifer)
**          Created for Security.
**	29-sep-89 (ralph)
**	    Fixed qeu_cloc to use varchar key.
**	28-oct-89 (ralph)
**	    Check for location name in iidatabase and iiextend before drop.
**	24-sep-90 (ralph)
**	    Correct audit outcome
**	30-nov-1992 (pholman)
**	    C2: Replace obsolete qeu_audit with qeu_secaudit
**	6-jul-93 (robf)
**	    Pass security label to qeu_secaudit()
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	3-nov-93 (robf)
**          Audit location security label.
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
**	02-Apr-2001 (jenjo02)
**	    Prevent altering raw locations to anything other than DATABASE.
**	11-May-2001 (jenjo02)
**	    Location's du_raw_blocks changed to du_raw_pct.
**	15-Oct-2001 (jenjo02)
**	    Added static qeu_mklocarea() to make missing Area
**	    directories.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**    25-Oct-2005 (hanje04)
**        Correct prototype for qeu_mklocarea() to prevent compiler
**        errors with GCC 4.0 due to conflict with static function.
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**	11-Apr-2008 (kschendel)
**	    Update DMF-qualification mechanism.
**/

/*
**  Definition of static variables and forward static functions 
*/

static DB_STATUS qeu_qdatabase(
	void		*toss,
	QEU_QUAL_PARAMS	*qparams);

static DB_STATUS qeu_qextend(
	void		*toss,
	QEU_QUAL_PARAMS	*qparams);

static DB_STATUS
qeu_mklocarea( QEF_CB		*qef_cb,
		DU_LOCATIONS	*loc,
		i4		*error );


/*{
** Name: qeu_cloc	- Store location information for one location.
**
** External QEF call:   status = qef_call(QEU_CLOC, &qeuq_cb);
**
** Description:
**	Add the location tuple to the iilocations table.
**
**	The loclation tuple was filled from the CREATE location statement, 
**	except that since this is the first access to iilocation, the uniqueness
**	of the localtion name is validated here.
**	First the iiloclation table is retrieved by location name to insure
**      that the location name provided is unqiue.
**	If a match is found then the location is a duplicate and an error is
**	returned.  If this is unique then the location is entered into 
**      iilocation.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of location tuples.  Must be 1.
**	    .qeuq_uld_tup	LOCATION tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US18B8_6328_LOC_EXISTS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	25-jun-89 (jennifer)
**	    Written for Security.
**	29-sep-89 (ralph)
**	    Fixed qeu_cloc to use varchar key.
**	23-aug-91 (bonobo)
**	    Removed preceeding "&" from array references to eliminate
**	    su4 compiler warnings.
**	18-feb-95 (forky01)
**	    qeu_open never got an initialized qeu_mask which caused random
**	    maxlocks and lock timeout.
*/

DB_STATUS
qeu_cloc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_LOCATIONS  	*ltuple;	/* New location tuple */
    DU_LOCATIONS 	ltuple_temp;	/* Tuple to check for uniqueness */
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;
    QEF_DATA	    	*next;		
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	lkey_array[2];
    DMR_ATTR_ENTRY  	*lkey_ptr_array[2];
    struct		loc_name
    {
	i2		length;
	char		name[DB_MAXNAME];
    };
    struct loc_name	lname;

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

	ltuple = (DU_LOCATIONS *)qeuq_cb->qeuq_uld_tup->dt_data;

    
	/* Validate that the location name/owner is unique */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named location - if not there then ok */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_LOCATIONS);
        qef_data.dt_data = (PTR)&ltuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = lkey_ptr_array;
	lkey_ptr_array[0] = &lkey_array[0];
	lkey_ptr_array[1] = &lkey_array[1];
	lkey_ptr_array[0]->attr_number = DM_1_LOCATIONS_KEY;
	lkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	lkey_ptr_array[0]->attr_value = (char *)&lname;
	MEcopy((PTR)ltuple->du_lname,
	       sizeof(lname.name),
	       (PTR)lname.name);
	lname.length = sizeof(lname.name);
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);
	if (status == E_DB_OK)		/* Found the same location! */
	{
	    (VOID)qef_error(E_US18B8_6328_LOC_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(ltuple->du_lname),
					  (char *)ltuple->du_lname),
			    (PTR)ltuple->du_lname);
	    error = E_QE0025_USER_ERROR;
	    break;
	}
	if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}
	/* The location is unique - append it */

	/* Insert single location tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
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

	/* This is a security event, must audit. */
	
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		 (char *)ltuple->du_lname, (DB_OWN_NAME *)NULL,
		 sizeof(ltuple->du_lname), SXF_E_LOCATION,
		 I_SX200B_LOCATION_CREATE, SXF_A_SUCCESS | SXF_A_CREATE, 
		 &e_error);

	    if (status != E_DB_OK)
		break;
	}

	/* Lastly, make any missing Area directories */
	if ( status = qeu_mklocarea(qef_cb, ltuple, &error) )
	    break;

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

    /* call qef_error to handle error/warning messages */
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
	if ( DB_FAILURE_MACRO(status) )
	    local_status = qeu_atran(qef_cb, &tranqeu);
	else
	    local_status = qeu_etran(qef_cb, &tranqeu);
        if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_cloc */


/*{
** Name: qeu_aloc	- Alter location information for one location.
**
** External QEF call:   status = qef_call(QEU_ALOC, &qeuq_cb);
**
** Description:
**	Alter a location tuple in the iilocations table.
**
**	The location tuple was filled from the ALTER location statement.
**	Only the usage flags are changed.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of location tuples.  Must be 1.
**	    .qeuq_uld_tup	LOCATION tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	29-sep-89 (ralph)
**	    Written.
**	8-jul-93 (robf)
**	    Add MAC check for B1, must dominate location label to change it.
**	18-feb-95 (forky01)
**	    qeu_open never got an initialized qeu_mask which caused random
**	    maxlocks and lock timeout.
**	02-Apr-2001 (jenjo02)
**	    Prevent altering raw locations to anything other than DATABASE.
*/

DB_STATUS
qeu_aloc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_LOCATIONS  	*ltuple;	/* New location tuple */
    DU_LOCATIONS 	ltuple_temp;	/* Tuple to check for uniqueness */
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;
    QEF_DATA	    	*next;		
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	lkey_array[2];
    DMR_ATTR_ENTRY  	*lkey_ptr_array[2];
    struct		loc_name
    {
	i2		length;
	char		name[DB_MAXNAME];
    };
    struct loc_name	lname;

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

	ltuple = (DU_LOCATIONS *)qeuq_cb->qeuq_uld_tup->dt_data;

    
	/* Validate that the location name exists */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named location - if not there then error */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_LOCATIONS);
        qef_data.dt_data = (PTR)&ltuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = lkey_ptr_array;
	lkey_ptr_array[0] = &lkey_array[0];
	lkey_ptr_array[1] = &lkey_array[1];
	lkey_ptr_array[0]->attr_number = DM_1_LOCATIONS_KEY;
	lkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	lkey_ptr_array[0]->attr_value = (char *)&lname;
	MEcopy((PTR)ltuple->du_lname,
	       sizeof(lname.name),
	       (PTR)lname.name);
	/* lname.length = (i2)STzapblank(&lname.name, &lname.name); */
	lname.length = sizeof(lname.name);
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		(VOID)qef_error(E_US18B9_6329_LOC_ABSENT, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(ltuple->du_lname),
					  (char *)ltuple->du_lname),
			    (PTR)ltuple->du_lname);
		error = E_QE0025_USER_ERROR;
	    }
	    else    /* Other error */
	    {
		error = qeu.error.err_code;
	    }
	    break;
	}

	/* Found the location, merge the information. */
	ltuple_temp.du_status = ltuple->du_status;

	/* Delete the old tuple, append new one */

	qeu.qeu_getnext = QEU_NOREPO;
	qeu.qeu_klen	= 0;

	status = qeu_delete(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}

	/* Insert single location tuple */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	qeu.qeu_input = &qef_data;
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

	/* This is a security event, must audit. */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		 (char *)ltuple->du_lname, (DB_OWN_NAME *)NULL,
		 sizeof(ltuple->du_lname), SXF_E_LOCATION,
		 I_SX202B_LOCATION_ALTER, SXF_A_SUCCESS | SXF_A_ALTER, 
		 &e_error);

	    if (status != E_DB_OK)
	    {
		error = e_error.err_code;
		break;
	    }
	}

	/* Lastly, make any missing Area directories */
	if ( status = qeu_mklocarea(qef_cb, &ltuple_temp, &error) )
	    break;

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

    /* call qef_error to handle error/warning messages */
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
	if ( DB_FAILURE_MACRO(status) )
	    local_status = qeu_atran(qef_cb, &tranqeu);
	else
	    local_status = qeu_etran(qef_cb, &tranqeu);
        if (local_status != E_DB_OK)
	{
	    (VOID) qef_error(tranqeu.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }
    return (status);
} /* qeu_aloc */


/*{
** Name: qeu_dloc	- Drop location.
**
** External QEF call:   status = qef_call(QEU_DLOC, &qeuq_cb);
**
** Description:
**	Delete a location tuple from the iilocations table.
**
**	The location tuple was filled from the DROP location statement.
**	Only the location name is used.
**
**	The iiextend and iidatabase catalogs are checked to ensure no
**	databases occupy the location to be dropped.  If the location
**	is occupied, then the statement is aborted.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of location tuples.  Must be 1.
**	    .qeuq_uld_tup	LOCATION tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	29-sep-89 (ralph)
**	    Written.
**	28-oct-89 (ralph)
**	    Check for location name in iidatabase and iiextend before drop.
**	17-jun-92 (andre)
**	    init qeu_flag before calling qeu_get() since it will be looking for
**	    QEU_BY_TID
**	9-jul-93 (robf)
**	    Check MAC access to location, to drop a location you need to
**	    dominate it.
**	18-feb-95 (forky01)
**	    qeu_open never got an initialized qeu_mask which caused random
**	    maxlocks and lock timeout.
*/

DB_STATUS
qeu_dloc(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_LOCATIONS  	*ltuple;	/* location tuple to be dropped */
    DU_LOCATIONS 	ltuple_temp;	/* Tuple to check for existence */
    DU_DATABASE		db_tuple;	/* To check iidatabase for location */
    DU_EXTEND		ex_tuple;	/* To check iiextend   for location */
    DB_STATUS	    	status, local_status;
    DB_ERROR		e_error;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;
    QEF_DATA	    	*next;		
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEU_QUAL_PARAMS	qparams;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	lkey_array[2];
    DMR_ATTR_ENTRY  	*lkey_ptr_array[2];
    struct		loc_name
    {
	i2		length;
	char		name[DB_MAXNAME];
    };
    struct loc_name	lname;

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

	ltuple = (DU_LOCATIONS *)qeuq_cb->qeuq_uld_tup->dt_data;
    
	/*
	** Check the iidatabase catalog to ensure no database
	** references the location for any usage.
	** If so, abort.
	*/
	
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_DATABASE_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_DATABASE_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_mask=0;
	qeu.qeu_flag=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve databases with the named location - if there then error */

	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_DATABASE);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_DATABASE);
        qef_data.dt_data = (PTR)&db_tuple;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 0;
	qeu.qeu_key = lkey_ptr_array;
	qeu.qeu_flag = 0;

	qparams.qeu_qparms[0] = (PTR)ltuple->du_lname;
	qeu.qeu_qual = qeu_qdatabase;
	qeu.qeu_qarg = &qparams;

	/* Get iidatabase tuple with specified location */

	status = qeu_get(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		error = qeu.error.err_code;
		break;
	    }
	}
	else
	{
	    (VOID)qef_error(E_US18DC_LOC_IN_USE, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(ltuple->du_lname),
			    (char *)ltuple->du_lname),
			    (PTR)ltuple->du_lname);
	    error = E_QE0025_USER_ERROR;
	    break;
	}

	/* Close the iidatabase table */

	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

    
	/*
	** Check the iiextend catalog to ensure no database
	** references the location for any usage.
	** If so, abort.
	*/
	
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_EXTEND_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_EXTEND_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IS;
        qeu.qeu_access_mode = DMT_A_READ;
	qeu.qeu_flag=0;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve databases with the named location - if there then error */

	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_EXTEND);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_EXTEND);
        qef_data.dt_data = (PTR)&ex_tuple;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 0;
	qeu.qeu_key = lkey_ptr_array;

	qparams.qeu_qparms[0] = (PTR)ltuple->du_lname;
	qeu.qeu_qual = qeu_qextend;
	qeu.qeu_qarg = &qparams;

	/* Get iiextend tuple with specified location */

	status = qeu_get(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		error = qeu.error.err_code;
		break;
	    }
	}
	else
	{
	    (VOID)qef_error(E_US18DC_LOC_IN_USE, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(ltuple->du_lname),
			    (char *)ltuple->du_lname),
			    (PTR)ltuple->du_lname);
	    error = E_QE0025_USER_ERROR;
	    break;
	}

	/* Close the iiextend table */

	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;
    
	/* Validate that the location name exists */

	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_LOCATIONS_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_LOCATIONS_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the same named location - if not there then error */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DU_LOCATIONS);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DU_LOCATIONS);
        qef_data.dt_data = (PTR)&ltuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 1;			
	qeu.qeu_key = lkey_ptr_array;
	lkey_ptr_array[0] = &lkey_array[0];
	lkey_ptr_array[1] = &lkey_array[1];
	lkey_ptr_array[0]->attr_number = DM_1_LOCATIONS_KEY;
	lkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	lkey_ptr_array[0]->attr_value = (char *)&lname;
	MEcopy((PTR)ltuple->du_lname,
	       sizeof(lname.name),
	       (PTR)lname.name);
	/* lname.length = (i2)STzapblank(&lname.name, &lname.name); */
	lname.length = sizeof(lname.name);
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    if (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
	    {
		(VOID)qef_error(E_US18B9_6329_LOC_ABSENT, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1,
			    qec_trimwhite(sizeof(ltuple->du_lname),
					  (char *)ltuple->du_lname),
			    (PTR)ltuple->du_lname);
		error = E_QE0025_USER_ERROR;
	    }
	    else    /* Other error */
	    {
		error = qeu.error.err_code;
	    }
	    break;
	}

	/* Delete the tuple */

	qeu.qeu_getnext = QEU_NOREPO;
	qeu.qeu_klen	= 0;

	status = qeu_delete(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}

	/* Close the table */
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	/* This is a security event, must audit. */
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		 (char *)ltuple->du_lname, (DB_OWN_NAME *)NULL,
		 sizeof(ltuple->du_lname), SXF_E_LOCATION,
		 I_SX202C_LOCATION_DROP, SXF_A_SUCCESS | SXF_A_DROP, 
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
} /* qeu_dloc */


/*{
** Name: QEU_QDATABASE - Qualify an iidatabase tuple by location
**
** Description:
**      This procedure qualifies an iidatabase tuple by location name.
**	If the specified qualification parameter appears as a location
**	name in any attribute of the iidatabase tuple, the tuple is
**	accepted; otherwise it is rejected.
**      It is called by DMF as part of tuple qualification when
**      deleting tuples for qeu_dloc.
**
** Inputs:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	The location name for which to scan.
**	    .qeu_rowaddr	The current iidatabase tuple to consider
**
** Outputs:
**	qparams.qeu_retval	Set to ADE_TRUE if match, else ADE_NOT_TRUE
**
**      Returns E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          None.
**
** History:
**      28-oct-89 (ralph)
**          written
**	03-nov-92 (jrb)
**	    Changed du_sortloc to du_workloc for multi-location sorts project.
**	11-Apr-2008 (kschendel)
**	    Revised the call sequence.
*/
static DB_STATUS
qeu_qdatabase(
    void *toss, QEU_QUAL_PARAMS *qparams)
{
    PTR qual_loc = qparams->qeu_qparms[0];
    DU_DATABASE *db_tuple = (DU_DATABASE *) qparams->qeu_rowaddr;

    qparams->qeu_retval = ADE_NOT_TRUE;
    if (qual_loc == NULL || db_tuple == NULL)      /* Bad parm; abort! */

        return(E_DB_ERROR);

    if	((MEcmp(qual_loc,			    /* Database location */
		(PTR)&db_tuple->du_dbloc,
		sizeof(db_tuple->du_dbloc))
		== 0) 
	    ||
	 (MEcmp(qual_loc,			    /* Checkpoint location */
		(PTR)&db_tuple->du_ckploc,
		sizeof(db_tuple->du_ckploc))
		== 0) 
	    ||
	 (MEcmp(qual_loc,			    /* Journal location */
		(PTR)&db_tuple->du_jnlloc,
		sizeof(db_tuple->du_jnlloc))
		== 0) 
	    ||
	 (MEcmp(qual_loc,			    /* Work location */
		(PTR)&db_tuple->du_workloc,
		sizeof(db_tuple->du_workloc))
		== 0) 
	    ||
	 (MEcmp(qual_loc,			    /* Dump location */
		(PTR)&db_tuple->du_dmploc,
		sizeof(db_tuple->du_dmploc))
		== 0)
	)
	qparams->qeu_retval = ADE_TRUE;		/* Qualified */

    return (E_DB_OK);
}


/*{
** Name: QEU_QEXTEND - Qualify an iiextend tuple by location
**
** Description:
**      This procedure qualifies an iiextend tuple by location name.
**	If the specified qualification parameter appears as a location
**	name in the iiextend tuple, the tuple is accepted; otherwise
**	it is rejected.
**      It is called by DMF as part of tuple qualification wwhen
**      deleting tuples for qeu_dloc.
**
** Inputs:
**	qparams			QEU_QUAL_PARAMS info area
**	    .qeu_qparms[0]	The location name for which to scan.
**	    .qeu_rowaddr	The current iiextend tuple to consider
**
** Outputs:
**	qparams.qeu_retval	Set to ADE_TRUE if match, else ADE_NOT_TRUE
**
**      Returns E_DB_OK
**      Exceptions:
**          none
**
** Side Effects:
**          None.
**
** History:
**      28-oct-89 (ralph)
**          written
**	11-Apr-2008 (kschendel)
**	    Revised the calling sequence.
*/
static DB_STATUS
qeu_qextend(
    void *toss,
    QEU_QUAL_PARAMS *qparams)
{
    PTR qual_loc = qparams->qeu_qparms[0];
    DU_EXTEND *ex_tuple = (DU_EXTEND *) qparams->qeu_rowaddr;
    
    char	lname[DB_MAXNAME];	/* Buffer to normalize location name */

    qparams->qeu_retval = ADE_NOT_TRUE;
    if (qual_loc == NULL || ex_tuple == NULL)	    /* Bad parm; abort! */
        return(E_DB_ERROR);

    /* Normalize location name to fixed length */

    MEmove(ex_tuple->du_l_length, (PTR)ex_tuple->du_lname, ' ',
	   DB_MAXNAME, (PTR)lname);

    if	 (MEcmp(qual_loc,
		(PTR)lname, DB_MAXNAME)
		== 0)
	qparams->qeu_retval = ADE_TRUE;		/* Qualified */

    return (E_DB_OK);
}

/*{
** Name: qeu_mklocarea - Make missing Location area directory(s).
**
** Description:
**	Calls DMF to make any missing subdirectories for a 
**	location's area/usage combinations.
**
** Inputs:
**	qef_cb			QEF session control block
**	ltuple			Pointer to DU_LOCATIONS tuple
**	error			Why it failed if it did
**
** Outputs:
**      None.
**
**      Returns:
**	    DB_STATUS
**      Exceptions:
**          none
**
** Side Effects:
**	Area directories may be created.
**
** History:
**	15-Oct-2001 (jenjo02)
**	    Invented
*/
static DB_STATUS
qeu_mklocarea(
QEF_CB		*qef_cb,
DU_LOCATIONS	*ltuple,
i4		*error)
{
    DB_STATUS		status = E_DB_OK;
    DMM_CB		dmm_cb;
    DMM_LOC_LIST	loc_list, *loc = &loc_list;
    i4			usage;

    /* Init DMM_LOC_LIST with Area path */
    MEcopy(ltuple->du_area, ltuple->du_l_area, (char*)&loc->loc_area);
    loc->loc_l_area = ltuple->du_l_area;
    loc->loc_raw_pct = ltuple->du_raw_pct;

    /* Init DMM_CB */
    MEfill(sizeof(DMM_CB), '\0', (PTR)&dmm_cb);
    dmm_cb.length = sizeof(DMM_CB);
    dmm_cb.type = DMM_MAINT_CB;
    dmm_cb.ascii_id = DMM_ASCII_ID;
    dmm_cb.error.err_code = E_DB_OK;
    dmm_cb.dmm_tran_id = qef_cb->qef_dmt_id;
    dmm_cb.dmm_db_location.data_address = loc->loc_area;
    dmm_cb.dmm_db_location.data_in_size = loc->loc_l_area;
    dmm_cb.dmm_loc_list.ptr_address = (PTR)&loc;
    dmm_cb.dmm_loc_list.ptr_in_count = 1;
    dmm_cb.dmm_loc_list.ptr_size = sizeof(*loc);

    /* Make missing directories */
    dmm_cb.dmm_flags_mask = DMM_MAKE_LOC_AREA;

    /* Discard stray du_status bits we don't care about */
    usage = ltuple->du_status & (DU_DBS_LOC  | DU_JNLS_LOC |
				 DU_CKPS_LOC | DU_DMPS_LOC |
				 DU_WORK_LOC | DU_AWORK_LOC);

    /* For each "usage"... */
    while ( usage && status == E_DB_OK )
    {
	if ( usage & DU_DBS_LOC )
	{
	    loc->loc_type = DMM_L_DATA;
	    usage &= ~DU_DBS_LOC;
	}
	else if ( usage & DU_JNLS_LOC )
	{
	    loc->loc_type = DMM_L_JNL;
	    usage &= ~DU_JNLS_LOC;
	}
	else if ( usage & DU_CKPS_LOC )
	{
	    loc->loc_type = DMM_L_CKP;
	    usage &= ~DU_CKPS_LOC;
	}
	else if ( usage & DU_DMPS_LOC )
	{
	    loc->loc_type = DMM_L_DMP;
	    usage &= ~DU_DMPS_LOC;
	}
	else if ( usage & DU_WORK_LOC )
	{
	    loc->loc_type = DMM_L_WRK;
	    usage &= ~DU_WORK_LOC;
	}
	else if ( usage & DU_AWORK_LOC )
	{
	    loc->loc_type = DMM_L_AWRK;
	    usage &= ~DU_AWORK_LOC;
	}

	/*
	** Returns E_DB_OK    if area exists or was made.
	**
	**	   E_DB_WARN  if Raw area does not exist
	**
	**	   E_DB_ERROR if area path is bad or could
	**		      not be created,
	**		or    invalid usage for raw location,
	**		or    area is raw and rawpct is missing,
	**		or    area is cooked and rawpct specified
	*/
	status = dmf_call(DMM_ADD_LOC, &dmm_cb);
    }

    *error = dmm_cb.error.err_code;
    return(status);
}
