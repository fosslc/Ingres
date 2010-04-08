/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <pc.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <usererror.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
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

#include    <ade.h>
#include    <dudbms.h>
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
#include    <sxf.h>
#include    <qefprotos.h>

/**
**  Name: qeusyn.c - Provide CREATE and DROP synonym support for RDF and PSF.
**
**  Description:
**      The two routines defined in this file provide the catalog update
**	support for the CREATE/DROP SYNONYM statements.  Routine responsible for
**	DROP SYNONYM support will also provide support for deletion of all
**	synonyms which would take place whenever an object (table, view, or
**	index) is dropped.
**	
**	CREATE SYNONYM results in the addition of a row into the IISYNONYM
**	catalog.  This is affected by qeu_create_synonym() routine.
**
**	DROP SYNONYM results in deletion of a row from IISYNONYM,
**	This is affected by qeu_drop_synonym().
**
**	DROP (table, view, or index) results in the deletion of all synonyms on
**	the table, view, or index.  qeu_drop_synonym() will handle this as well.
**
**  Defines:
**      qeu_create_synonym	- Create a synonym
**      qeu_drop_synonym	- Drop synonym(s).
**
**  History:
** 	19-apr-90 (andre)
**	   Written for support of CREATE/DROP SYNONYM statements.
**	04-jan-91 (andre)
**	    changed qeu_drop_synonym() for the case when we are dropping all
**	    synonyms defined on a view or a table (and its indices).  Rather
**	    than opening IISYNONYM in IX mode and scanning the whole table
**	    (which results in IISYNONYM being locked up in exclusive mode), we
**	    will open it twice: for read (with lockmode set to S) and for write
**	    (with lock_mode set to IX).  We will hold shared lock on the table,
**	    but will minimize the number of pages locked exclusively.
**	19-may-93 (anitap)
**	    Added support for creation of implicit schema for synonyms.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	07-jul-93 (anitap)
**	    Added arguments qef_rcb & qeuq_cb to qea_schema() instead of PTR in
**	    qeu_create_synonym().
**	    Changed assignment of flag to IMPLICIT_SCHEMA.
**	24-aug-93 (stephenb)
**	    We should audit succesfull create/drop synonym here for C2, 
**	    added calls to qeu_secaudit() in both routines.
**      14-sep-93 (smc)
**          Removed (PTR) cast of qeuq_d_id now session ids are CS_SID.
**	8-oct-93 (robf)
**          Updated qeu_secaudit calls to new format
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
**	11-Apr-2008 (kschendel)
**	    Fix type mismatch caused by change to QEU qualification type.
*/

/*{
** Name: qeu_create_synonym	- Update catalog to create a synonym.
**
** External QEF call:   status = qef_call(QEU_CSYNONYM, &qeuq_cb);
**
** Description:
**	Respond to CREATE SYNONYM statement by adding a row to the iisynonym
**	catalog.  
**
**	The relstat field of iirelation is also updated to indicate that 
**	at least one synonym exists for the object.  Note that (analogous to
**	some other relstat bits) this bit is not turned off when the last 
**      synonym is deleted;  it simply reports that a synonym did exist at one 
**	time.  This is used to signal cascading delete of synonyms when the 
**	object is deleted.
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		How many synonym tuples --- always 1.
**	    .qeuq_uld_tup
**		dt_data		Synonym tuple.
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
**	19-apr-90 (andre)
**	    Written for synonyms feature.
**	19-may-93 (anitap)
**	    Added support for creation of implicit schemas for synonyms.
**	07-jul-93 (anitap)
**	    Added arguments qef_rcb & qeuq_cb to qea_schema() instead of PTR.
**	    Changed assignment of flag to IMPLICIT_SCHEMA.
**	24-aug-93 (stephenb)
**	    Added call to qeu_secaudit() to audit successful synonym creation.
**	15-sep-93 (andre)
**	    Two new attributes - synid and synidx - have been added to IISYNONYM
**	    in order to make it possible to express dependencies of dbprocs on 
**	    synonyms.  Code to populate these new attributes will be added.
**	18-feb-95 (forky01)
**	    qeu_open was getting qeucb passed with uninitialized qeu_mask
**	    causing random maxlocks and lock timeouts.
*/
DB_STATUS
qeu_create_synonym(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    DB_IISYNONYM    	*syn_tuple;	/* New SYNONYM tuple */
    DB_STATUS	    	status, local_status;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    DMT_CHAR_ENTRY  	char_spec;	/* DMT_ALTER specification */
    DMT_CB	    	dmt_cb;
    i4			flag = IMPLICIT_SCHEMA;	
    QEF_RCB		*qef_rcb = (QEF_RCB *)NULL;
    DMU_CB	        dmu_cb;
    bool		leave_loop = FALSE;

    while (!leave_loop)	/* Dummy for loop for error breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}

	if ((qeuq_cb->qeuq_culd != 1	    /*  must be one synonym tuple */
	    || qeuq_cb->qeuq_uld_tup == NULL)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}
	
	/*
	** point to the new synonym tuple.
	*/
	
	syn_tuple = (DB_IISYNONYM *) qeuq_cb->qeuq_uld_tup->dt_data; 

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

	/* Get the synonym id from DMF. */
	dmu_cb.type = DMU_UTILITY_CB;
	dmu_cb.length = sizeof(DMU_CB);
	dmu_cb.dmu_flags_mask = 0;
	dmu_cb.dmu_tran_id = qef_cb->qef_dmt_id;
	dmu_cb.dmu_db_id = qeuq_cb->qeuq_db_id;

	status = dmf_call(DMU_GET_TABID, &dmu_cb);
	if (status != E_DB_OK)
	{
	    error = dmu_cb.error.err_code;
	    break;
	}	
	
	/* store the synonym id in the new tuple */
	syn_tuple->db_syn_id.db_tab_base = dmu_cb.dmu_tbl_id.db_tab_base;
	syn_tuple->db_syn_id.db_tab_index = dmu_cb.dmu_tbl_id.db_tab_index;

	/* 
        ** Alter the table relstat to flag existence of synonyms.
	** This validates that the table exists and ensures that we get an
	** exclusive lock on it.
        */
	char_spec.char_id = DMT_C_SYNONYM;      /* create synonym code */
	char_spec.char_value = DMT_C_ON;
	dmt_cb.dmt_flags_mask = 0;
	dmt_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
	dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
	dmt_cb.dmt_char_array.data_address = (PTR)&char_spec;
	dmt_cb.length = sizeof(DMT_CB);
	dmt_cb.type = DMT_TABLE_CB;
	STRUCT_ASSIGN_MACRO(syn_tuple->db_syntab_id, dmt_cb.dmt_id);
	dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
	status = dmf_call(DMT_ALTER, &dmt_cb);
	if (status != E_DB_OK)
	{
	    error = dmt_cb.error.err_code;
	    break;
	}
    
	/* Open the synonym catalog */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_SYNONYM_TAB_ID; /* synonym catalog */
        qeu.qeu_tab_id.db_tab_index  = DM_I_SYNONYM_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
	qeu.qeu_mask = 0;
        qeu.qeu_access_mode = DMT_A_WRITE;
	status = qeu_open(qef_cb, &qeu);	/* open the synonym catalog */
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* now insert the synonym tuple */

	qeu.qeu_count = 1;
	qeu.qeu_tup_length = sizeof(DB_IISYNONYM);
	qeu.qeu_input = qeuq_cb->qeuq_uld_tup;
	status = qeu_append(qef_cb, &qeu);
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	/*
	** we now have a synonym tuple, must audit successful synonym create
	*/
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    DB_ERROR	e_error;

	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		    syn_tuple->db_synname.db_synonym,
		    (DB_OWN_NAME *)syn_tuple->db_synowner.db_syn_own,
		    sizeof(syn_tuple->db_synname.db_synonym),
		    SXF_E_TABLE, I_SX272A_TBL_SYNONYM_CREATE,
		    SXF_A_SUCCESS | SXF_A_CONTROL, &e_error);
	    if (status != E_DB_OK)
	    {
		error = e_error.err_code;
		break;
	    }
	}

	/* Close the synonym catalog */
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	/* call qea_cschema() to create implicit schema if it isn't already
        ** present in the iischema catalog.
        */
        status = qea_schema(qef_rcb, qeuq_cb, qef_cb, 
			(DB_SCHEMA_NAME *)&syn_tuple->db_synowner, flag);
        if (status != E_DB_OK)
        {
           error = qeuq_cb->error.err_code;
           break;
        }


	/* take care of the transaction state */
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

	leave_loop = TRUE;
    } /* End dummy while() */

    /*
    ** if we left the loop because we finished processing successfully, return
    ** now; otherwise, proceed on to error handling code
    */
    if (status == E_DB_OK)
	return (E_DB_OK);

    /* call qef_error to handle error messages */
    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off the synonym catalog, if still open.  */
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
} /* qeu_create_synonym */

/*{
** Name: qeu_drop_synonym	- Update catalog to destroy synonym(s).
**
** External QEF call:   status = qef_call(QEU_DSYNONYM, &qeuq_cb);
**
** Description:
**	Delete a row from IISYNONYM for DROP SYNONYM statement.
**	This function will also handle destroying of all synonyms associated
**	with a given table.
**
** Inputs:
**      qef_cb				QEF session control block
**      qeuq_cb
**	    .qeuq_eflag			Designate error handling for user
**					errors.
**		QEF_INTERNAL		Return error code.
**		QEF_EXTERNAL		Send message to user.
**	    .qeuq_uld_tup		NULL if all synonyms on a table are to
**					be dropped; otherwise
**		dt_data			Synonym tuple.
**	    .qeuq_rtbl			id of the table all of whose synonyms
**					are to be dropped (will only be used
**					whenever qeuq_uld_tup == NULL)
**	    .qeuq_flag_mask		
**		QEU_DROP_IDX_SYN	object being dropped is a table on 
**					which some indices have been defined - 
**					we must destroy synonyms on the table 
**					being dropped as well as on any indices
**					defined on it
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
**	20-apr-90 (andre)
**	    Written for synonyms feature.
**	08-may-90 (andre)
**	    If caller set QEU_DROP_IDX_SYN bit in qeuq_flag_mask, destroy
**	    synonyms created for indices which may have been created on this
**	    table.
**	12-dec-90 (andre)
**	    we don't need to call dmt_alter() when dropping all synonyms since
**	    this occurs only when we drop the object.
**	04-jan-91 (andre)
**	    changed qeu_drop_synonym() for the case when we are dropping all
**	    synonyms defined on a view or a table (and its indices).  Rather
**	    than opening IISYNONYM in IX mode and scanning the whole table
**	    (which results in IISYNONYM being locked up in exclusive mode), we
**	    will open it twice: for read (with lockmode set to S) and for write
**	    (with lock_mode set to IX).  We will hold shared lock on the table,
**	    but will minimize the number of pages locked exclusively.
**	27-feb-92 (rickh)
**	    Initialize qeu_flag to 0 on single synonym delete.  Stack noise
**	    isn't good enough.
**	17-jun-92 (andre)
**	    init qeu_flag before calling qeu_get() which will be looking for
**	    QEU_BY_TID
**	24-aug-93 (stephenb)
**	    Added calls to qeu_secaudit() to audit drop synonym action.
**	13-sep-93 (andre)
**	    When dropping synonym(s) the caller may request that we force 
**	    invalidation of QPs which may depend on these synonyms:
**	      - if dropping synonym(s) on a table, this will be accomplished 
**	        by altering table's timestamp
**	      - if dropping synonym(s) on a view, this will be accomplished by 
**		altering timestamp of a base table on which the view was 
**		defined (id of which has been recorded in the view tree at 
**		view creation time)
**	15-oct-93 (andre)
**	    When dropping synonyms as a part of dropping the object on which 
**	    they were defined, instead of scanning through the entire IISYNONYM
**	    table, we will key into IIXSYNONYM by table id and then delete 
**	    IISYNONYM tuple by tid
**	22-oct-93 (andre)
**	    When called to drop a specific synonym, we will delete the IISYNONYM
**	    tuple and then invoke qeu_d_cascade() to check for, and mark 
**	    dormant, any dbprocs which depend on this synonym
**	04-nov-93 (andre)
**	    if dropping a specific permit, we will ask 
**	    qeu_force_qp_invalidation() to let us know whether the QPs affected 
**	    by the destruction have been invalidated.  If not, we will set 
**	    QEU_FORCE_QP_INVALIDATION in qeuq_cb.qeuq_flag_mask before calling
**	    qeu_d_cascade().  This will cause it to attempt to alter timestamps 
**	    of UBTs of dbprocs dependent on the synonym.
**	24-nov-93 (andre)
**	    qeu_force_qp_invalidation() will report its own errors; this 
**	    requires that we supply it with address of DB_ERROR instead of 
**	    space to store error number
**	18-feb-95 (forky01)
**	    qeu_open was getting qeucb passed with uninitialized qeu_mask
**	    causing random maxlocks and lock timeouts.
*/
DB_STATUS
qeu_drop_synonym(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    DB_IISYNONYM    	*syn_tuple;
    DB_STATUS	    	status, local_status;
    i4	    	error = E_QE0000_OK;
    bool	    	drop_all_synonyms,
			transtarted = FALSE,
		    	read_open = FALSE,
			write_open = FALSE;
    QEU_CB	    	tran_qeu,  *tqeu = &tran_qeu,
		    	read_qeu,  *rqeu = &read_qeu,
			write_qeu, *wqeu = &write_qeu;
    DMT_CB	    	dmt_cb;
    DMR_ATTR_ENTRY	synkey_array[2];
    DMR_ATTR_ENTRY	*synkey_ptr_array[2];
    QEF_DATA	    	qef_data;
    bool		leave_loop = FALSE;
    bool		qps_invalidated;
    bool		err_already_reported = FALSE;

    while (!leave_loop)	/* something to break out of */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}

	if (   qeuq_cb->qeuq_db_id == NULL 
	    || qeuq_cb->qeuq_d_id == 0
	    || (   qeuq_cb->qeuq_uld_tup == NULL
		&& qeuq_cb->qeuq_flag_mask & QEU_FORCE_QP_INVALIDATION)
	   )
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}

	if (qeuq_cb->qeuq_uld_tup == NULL)
	{
	    drop_all_synonyms = TRUE;
	    /*
	    ** qeuq_cb->qeuq_rtbl points to the ID of the table all of whose
	    ** synonyms will be dropped
	    */
	}
	else
	{
	    /*
	    ** point to the synonym tuple containing the name and the
	    ** owner of the synonym to be dropped 
	    */
	    syn_tuple = (DB_IISYNONYM *) qeuq_cb->qeuq_uld_tup->dt_data;
	    drop_all_synonyms = FALSE;
	}

	/* 
	** Check to see if transaction is in progress, if so, set a local
	** transaction, otherwise we'll use the user's transaction.
	*/
	if (qef_cb->qef_stat == QEF_NOTRAN)
	{
	    tqeu->qeu_type   = QEUCB_CB;
	    tqeu->qeu_length = sizeof(QEUCB_CB);
	    tqeu->qeu_db_id  = qeuq_cb->qeuq_db_id;
	    tqeu->qeu_d_id   = qeuq_cb->qeuq_d_id;
	    tqeu->qeu_flag   = 0;
	    status = qeu_btran(qef_cb, tqeu);
	    if (status != E_DB_OK)
	    {	
		error = tqeu->error.err_code;
		break;	
	    }	    
	    transtarted = TRUE;
	}

	/* Escalate the transaction to MST */
	if (qef_cb->qef_auto == QEF_OFF)
	    qef_cb->qef_stat = QEF_MSTRAN;

	/*
	** if deleting a specific synonym, pass its name and owner to
	** qeu_delete(); otherwise we have to key into IIXSYNONYM by table id 
	** and then delete IISYNONYM tuple by TID
        */

	if (drop_all_synonyms)
	{
	    DB_IIXSYNONYM	xsyn_tup;

	    /*
	    ** first open IIXSYNONYM for reading; 
	    */
	    rqeu->qeu_type = QEUCB_CB;
	    rqeu->qeu_length = sizeof(QEUCB_CB);
						    /* IIXSYNONYM catalog */
	    rqeu->qeu_tab_id.db_tab_base  = DM_B_XSYN_TAB_ID; 
	    rqeu->qeu_tab_id.db_tab_index = DM_I_XSYN_TAB_ID;

	    rqeu->qeu_db_id = qeuq_cb->qeuq_db_id;
	    rqeu->qeu_lk_mode = DMT_IS;
	    rqeu->qeu_access_mode = DMT_A_READ;
	    rqeu->qeu_flag = 0;
	    rqeu->qeu_mask = 0;
	    status = qeu_open(qef_cb, rqeu);   /* open IIXSYNONYM for reading */
	    if (status != E_DB_OK)
	    {
		error = rqeu->error.err_code;
		break;
	    }
	    read_open = TRUE;

	    rqeu->qeu_tup_length = sizeof(DB_IIXSYNONYM);
	    rqeu->qeu_getnext = QEU_REPO;   /*
					    ** this will be reset to QEU_NOREPO
					    ** after the first qeu_get()
					    */

	    rqeu->qeu_count = 1;	    /* will look up a tuple at a time */

	    rqeu->qeu_key = synkey_ptr_array;

	    synkey_ptr_array[0] = synkey_array;
	    synkey_ptr_array[0]->attr_number = DM_1_XSYN_KEY;
	    synkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    synkey_ptr_array[0]->attr_value = 
		(char *) &qeuq_cb->qeuq_rtbl->db_tab_base;

	    /*
	    ** if we were instructed to drop synonyms on a table and all 
	    ** indices defined over it, we will read IIXSYNONYM using the first
	    ** half of table id; otherwise we will use the entire id
	    */
	    if (qeuq_cb->qeuq_flag_mask & QEU_DROP_IDX_SYN)
	    {
		rqeu->qeu_klen = 1;
	    }
	    else
	    {
	        rqeu->qeu_klen = 2;

	        synkey_ptr_array[1] = synkey_array + 1;
	        synkey_ptr_array[1]->attr_number = DM_2_XSYN_KEY;
	        synkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	        synkey_ptr_array[1]->attr_value = 
		    (char *) &qeuq_cb->qeuq_rtbl->db_tab_index;
	    }

	    rqeu->qeu_output = &qef_data;
	    qef_data.dt_next = NULL;
	    qef_data.dt_size = sizeof(DB_IIXSYNONYM);
	    qef_data.dt_data = (PTR) &xsyn_tup;

	    rqeu->qeu_qual = NULL;
	    rqeu->qeu_qarg = NULL;

	    if ((status = qeu_get(qef_cb, rqeu)) != E_DB_OK)
	    {
		error = rqeu->error.err_code;
	    }
	    else
	    {
		rqeu->qeu_getnext = QEU_NOREPO;
	    }

	    for (; status == E_DB_OK;)
	    {
		if (!write_open)
		{
		    /* open IISYNONYM for write in IX mode */

		    wqeu->qeu_type = QEUCB_CB;
		    wqeu->qeu_length = sizeof(QEUCB_CB);
		    
						    /* synonym catalog */
		    wqeu->qeu_tab_id.db_tab_base  = DM_B_SYNONYM_TAB_ID; 
		    wqeu->qeu_tab_id.db_tab_index  = DM_I_SYNONYM_TAB_ID;

		    wqeu->qeu_db_id = qeuq_cb->qeuq_db_id;
		    wqeu->qeu_lk_mode = DMT_IX;
		    wqeu->qeu_mask=0;
		    wqeu->qeu_flag=0;
		    wqeu->qeu_access_mode = DMT_A_WRITE;

		    /* open the synonym catalog */
		    status = qeu_open(qef_cb, wqeu);
		    if (status != E_DB_OK)
		    {
			error = wqeu->error.err_code;
			break;
		    }

		    /*
		    ** TID will be used to specify the tuple to be deleted;
		    ** set qeu_klen to 0 to indicate to qeu_delete() that we do
		    ** not delete by primary key
		    */
		    wqeu->qeu_klen = 0;
		    wqeu->qeu_key = (DMR_ATTR_ENTRY **) NULL;
		    wqeu->qeu_flag = QEU_BY_TID;
		    wqeu->qeu_getnext = QEU_NOREPO;

		    write_open = TRUE;
		}

		/* use TIDP from IIXSYNONYM tuple to delete IISYNONYM tuple */
		wqeu->qeu_tid = xsyn_tup.dbx_tidp;

		if ((status = qeu_delete(qef_cb, wqeu)) != E_DB_OK)
		{
		    error = wqeu->error.err_code;
		}
		else if ((status = qeu_get(qef_cb, rqeu)) != E_DB_OK)
		{
		    error = rqeu->error.err_code;
		}
	    }

            /* reset status to OK if error was that we ran out of rows */
            if (status != E_DB_OK && error == E_QE0015_NO_MORE_ROWS)
            {
                status = E_DB_OK;
            }
	}
	else
	{
            DB_IISYNONYM	read_syn_tuple;
	    QEUQ_CB		dqeuq_cb;

	    /* Open the synonym catalog for writing */
	    wqeu->qeu_type = QEUCB_CB;
	    wqeu->qeu_length = sizeof(QEUCB_CB);
						    /* synonym catalog */
	    wqeu->qeu_tab_id.db_tab_base  = DM_B_SYNONYM_TAB_ID; 
	    wqeu->qeu_tab_id.db_tab_index  = DM_I_SYNONYM_TAB_ID;

	    wqeu->qeu_db_id = qeuq_cb->qeuq_db_id;
	    wqeu->qeu_lk_mode = DMT_IX;
	    wqeu->qeu_access_mode = DMT_A_WRITE;
	    wqeu->qeu_mask=0;
	    wqeu->qeu_flag=0;
	    status = qeu_open(qef_cb, wqeu);	/* open the synonym catalog */
	    if (status != E_DB_OK)
	    {
		error = wqeu->error.err_code;
		break;
	    }

	    write_open = TRUE;

            wqeu->qeu_count = 1;
	    wqeu->qeu_tup_length = sizeof(DB_IISYNONYM);

	    wqeu->qeu_output = &qef_data;
	    qef_data.dt_next = 0;
	    qef_data.dt_size = sizeof(DB_IISYNONYM);
	    qef_data.dt_data = (PTR) &read_syn_tuple;

	    /* we will get the IISYNONYM tuple by key and then delete it */
	    wqeu->qeu_getnext = QEU_REPO;

	    wqeu->qeu_klen = 2;			/* keyed on name and owner */
	    wqeu->qeu_key = synkey_ptr_array;

	    synkey_ptr_array[0] = synkey_array;
	    synkey_ptr_array[0]->attr_number = DM_1_SYNONYM_KEY;
	    synkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	    synkey_ptr_array[0]->attr_value = 
		(char *) &syn_tuple->db_synname; /* synonym name */

	    synkey_ptr_array[1] = synkey_array + 1;
	    synkey_ptr_array[1]->attr_number = DM_2_SYNONYM_KEY;
	    synkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	    synkey_ptr_array[1]->attr_value = 
		(char *) &syn_tuple->db_synowner; /* name of synonym owner */

	    wqeu->qeu_qual = NULL;	    /* qualification is done by key */
	    wqeu->qeu_qarg = NULL;
	    wqeu->qeu_flag = 0;

	    if ((status = qeu_get(qef_cb, wqeu)) != E_DB_OK)
	    {
		if (wqeu->error.err_code == E_QE0015_NO_MORE_ROWS)
		{
		    error = E_QE011C_NONEXISTENT_SYNONYM;
		}
		else
		{
		    error = wqeu->error.err_code;
		}

		break;
	    }

	    /* 
	    ** now that we got a copy of the IISYNONYM tuple in 
	    ** read_syn_tuple, we can delete it - this requires that we 
	    ** reset wqeu->qeu_klen to 0 so that qeu_delete() would delete 
	    ** the "current" tuple
	    */
	    wqeu->qeu_klen = 0;
	    wqeu->qeu_getnext = QEU_NOREPO;
	    wqeu->qeu_key = (DMR_ATTR_ENTRY **) NULL;
    
	    if ((status = qeu_delete(qef_cb, wqeu)) != E_DB_OK)
	    {
	        error = wqeu->error.err_code;
	        break;
	    }

	    if (qeuq_cb->qeuq_flag_mask & QEU_FORCE_QP_INVALIDATION)
	    {
		/* 
		** force invalidation of QPs that may depend on existence of 
		** this synonym; if the synonym was defined on a base table,
		** we will alter timestamp of that table; otherwise (if it 
		** was defined on a view), we will alter timestamp of a base 
		** table used in the view's definition
		*/
		status = qeu_force_qp_invalidation(qef_cb, 
		    &syn_tuple->db_syntab_id, (bool) FALSE, 
		    &qps_invalidated, &qeuq_cb->error);
		
		if (DB_FAILURE_MACRO(status))
		{
		    err_already_reported = TRUE;
		    break;
		}
	    }

	    /* 
	    ** now the final step: call qeu_d_cascade() to check for, and 
	    ** mark dormant, any dbprocs which depend on this synonym
	    */

	    STRUCT_ASSIGN_MACRO(*qeuq_cb, dqeuq_cb);

	    dqeuq_cb.qeuq_rtbl = &read_syn_tuple.db_syn_id;
	    dqeuq_cb.qeuq_dbp_tup = (QEF_DATA *) NULL;
	    dqeuq_cb.qeuq_qid.db_qry_high_time = 
		dqeuq_cb.qeuq_qid.db_qry_low_time = 0;

	    dqeuq_cb.qeuq_flag_mask = 
		qps_invalidated ? 0 : QEU_FORCE_QP_INVALIDATION;
	    
	    status = qeu_d_cascade(qef_cb, &dqeuq_cb, (DB_QMODE) DB_SYNONYM,
		(DMT_TBL_ENTRY *) NULL);
	    if (status != E_DB_OK)
	    {
		/* qeu_d_cascade() reports its own errors */
		error = 0L;

		STRUCT_ASSIGN_MACRO(dqeuq_cb.error, qeuq_cb->error);
		break;
	    }

	    /* success, we need to audit synonym drop */
	    if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	    {
	        DB_ERROR	e_error;

	    	status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		    	syn_tuple->db_synname.db_synonym,
		    	(DB_OWN_NAME *)syn_tuple->db_synowner.db_syn_own,
		    	sizeof(syn_tuple->db_synname.db_synonym),
		    	SXF_E_TABLE, I_SX272B_TBL_SYNONYM_DROP,
		    	SXF_A_SUCCESS | SXF_A_CONTROL, &e_error); 
	    	if (status != E_DB_OK)
	    	{
		    error = e_error.err_code;
		    break;
	    	}
	    }
	}

	leave_loop = TRUE;
    } /* End dummy while() */

    if (status != E_DB_OK && !err_already_reported)
    {
	if (error == E_QE011C_NONEXISTENT_SYNONYM)
	{
	    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 2,
		qec_trimwhite(sizeof(syn_tuple->db_synowner),
		    (char *) &syn_tuple->db_synowner),
		(PTR) &syn_tuple->db_synowner,
		qec_trimwhite(sizeof(syn_tuple->db_synname),
		    (char *) &syn_tuple->db_synname),
		(PTR) &syn_tuple->db_synname);
	}
	else if (error != 0L)
	{
	    /* call qef_error to handle error messages */
	    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
	}
    }
    else
    {
	/* reset err_code to OK in case it was set to E_QE0015_NO_MORE_ROWS */
	qeuq_cb->error.err_code = E_QE0000_OK;
    }

    local_status = E_DB_OK;

    /* close opened tables */
    if (read_open)
    {
	if ((local_status = qeu_close(qef_cb, rqeu)) != E_DB_OK)
	{
	    (VOID) qef_error(rqeu->error.err_code, 0L, local_status, &error, 
		    	     &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (write_open)
    {
	if ((local_status = qeu_close(qef_cb, wqeu)) != E_DB_OK)
	{
	    (VOID) qef_error(wqeu->error.err_code, 0L, local_status, &error, 
		    	     &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    if (transtarted)		   /* If we started a transaction */
    {
	if (status == E_DB_OK &&    /* everything looks OK so far -- commit */
	    (status = qeu_etran(qef_cb, tqeu)) != E_DB_OK)
	{
	    /*
	    ** failed to commit internal transaction; will abort, but first
	    ** report the error;
	    **
	    ** notice that status now contains the return value from 
	    ** qeu_etran() - it was OK to reset status since we knew that it
	    ** was OK and we would not be covering up any previous errors.
	    */
	    (VOID) qef_error(tqeu->error.err_code, 0L, status,
			     &error, &qeuq_cb->error, 0);
	}

	if (status != E_DB_OK &&    /* we got an error -- abort */
	    (local_status = qeu_atran(qef_cb, tqeu)) != E_DB_OK)
	{
	    (VOID) qef_error(tqeu->error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
	    if (local_status > status)
		status = local_status;
	}
    }

    return (status);
} /* qeu_drop_synonym */
