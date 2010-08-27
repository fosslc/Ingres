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
#include    <dmscb.h>
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
**  Name: QEUSEQ.C - routines for maintaining sequence generators
**
**  Description:
**	The routines defined in this file provide an interface for
**	qyrmod to manipulate the contents of the iisequence catalog.
**
**	The following routine(s) are defined in this file:
**
**	    qeu_cseq	- appends an iisequence tuple
**	    qeu_aseq	- alters an iisequence tuple
**	    qeu_dseq	- drops an iisequence tuple
**
**  History:
**	05-Mar-2002 (jenjo02)
**	    Invented.
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	14-Mar-2005 (thaju02)
**	    Modified qeu_seq(). For drop sequence perform a 
**	    cascading destroy. (B112545)
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**/

/*
**  Definition of static variables and forward static functions 
*/
static DB_STATUS qeu_seq(
		    i4		action,
		    QEF_CB      *qef_cb,
		    QEUQ_CB     *qeuq_cb);


/*{
** Name: qeu_cseq	- Store sequence information for one sequence.
**
** External QEF call:   status = qef_call(QEU_CSEQ, &qeuq_cb);
**
** Description:
**	Add the sequencer tuple to the iisequence table.
**
**	The sequence tuple was filled from the CREATE sequence statement, 
**	except that since this is the first access to iisequence, the
**	uniqueness of the sequence name is validated here.
**	First the iisequence table is retrieved by sequence name+owner
**	to insure that the sequence name+owner is unique.
**	If a match is found then the sequence is a duplicate and an error is
**	returned.  If this is unique then the sequence is entered into 
**      iisequence.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of sequence tuples.  Must be 1.
**	    .qeuq_uld_tup	SEQUENCE tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US1912_6418_SEQ_EXISTS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Invented.
**	20-mar-02 (inkdo01)
**	    Added create date to iisequence row.
*/

DB_STATUS
qeu_cseq(
QEF_CB      *qef_cb,
QEUQ_CB	    *qeuq_cb)
{
    DB_IISEQUENCE	*stuple;
    DB_DATA_VALUE	seqcreate_dv;
    DB_STATUS		status;
    i4			error;
    
    /* Get the create date of the sequence */
    stuple = (DB_IISEQUENCE *)qeuq_cb->qeuq_uld_tup->dt_data;
    seqcreate_dv.db_datatype = DB_DTE_TYPE;
    seqcreate_dv.db_prec = 0;
    seqcreate_dv.db_length = sizeof(DB_DATE);
    seqcreate_dv.db_data = (PTR)&stuple->dbs_create;
    qef_cb->qef_adf_cb->adf_constants = NULL;
    
    status = adu_datenow(qef_cb->qef_adf_cb, &seqcreate_dv);
    if (status != E_DB_OK)
    {
	error = qef_cb->qef_adf_cb->adf_errcb.ad_errcode;
	/* call qef_error to handle error/warning messages */
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
	return(status);
    }

    return( qeu_seq(QEU_CSEQ, qef_cb, qeuq_cb) );
}


/*{
** Name: qeu_aseq	- Alter sequence information for one sequence.
**
** External QEF call:   status = qef_call(QEU_ASEQ, &qeuq_cb);
**
** Description:
**	Alter a sequence tuple in the iisequences table.
**
**	The sequence tuple was filled from the ALTER sequence statement.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of sequence tuples.  Must be 1.
**	    .qeuq_uld_tup	SEQUENCE tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US1913_6419_SEQ_NOT_EXISTS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Invented.
**	20-mar-02 (inkdo01)
**	    Added modify date to iisequence row.
*/

DB_STATUS
qeu_aseq(
QEF_CB      *qef_cb,
QEUQ_CB	    *qeuq_cb)
{
    DB_IISEQUENCE	*stuple;
    DB_DATA_VALUE	seqmodify_dv;
    DB_STATUS		status;
    i4			error;
    
    /* Get the modify date of the sequence */
    stuple = (DB_IISEQUENCE *)qeuq_cb->qeuq_uld_tup->dt_data;
    seqmodify_dv.db_datatype = DB_DTE_TYPE;
    seqmodify_dv.db_prec = 0;
    seqmodify_dv.db_length = sizeof(DB_DATE);
    seqmodify_dv.db_data = (PTR)&stuple->dbs_modify;
    qef_cb->qef_adf_cb->adf_constants = NULL;
    
    status = adu_datenow(qef_cb->qef_adf_cb, &seqmodify_dv);
    if (status != E_DB_OK)
    {
	error = qef_cb->qef_adf_cb->adf_errcb.ad_errcode;
	/* call qef_error to handle error/warning messages */
	(VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
	return(status);
    }

    return( qeu_seq(QEU_ASEQ, qef_cb, qeuq_cb) );
}


/*{
** Name: qeu_dseq	- Drop sequence.
**
** External QEF call:   status = qef_call(QEU_DSEQ, &qeuq_cb);
**
** Description:
**	Delete a sequence tuple from the iisequences table.
**
**	The sequence tuple was filled from the DROP sequence statement.
**	Only the sequence name is used.
**
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of sequence tuples.  Must be 1.
**	    .qeuq_uld_tup	SEQUENCE tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US1913_6419_SEQ_NOT_EXISTS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Invented.
*/

DB_STATUS
qeu_dseq(
QEF_CB      *qef_cb,
QEUQ_CB	    *qeuq_cb)
{
    return( qeu_seq(QEU_DSEQ, qef_cb, qeuq_cb) );
}

/*{
** Name: qeu_seq	- iisequence common code.
**
** Description:
**	Create/Alter/Drop a sequence tuple from iisequences table.
**
**
** Inputs:
**	action			QEU_CSEQ || QEU_ASEQ || QEU_DSEQ
**
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of sequence tuples.  Must be 1.
**	    .qeuq_uld_tup	SEQUENCE tuple.
**	    .qeuq_db_id		Database id.
**	    .qeuq_d_id		DMF session id.
**	
** Outputs:
**      qeuq_cb
**	    .error.err_code	E_QE0002_INTERNAL_ERROR
**				E_QE0017_BAD_CB
**				E_QE0018_BAD_PARAM_IN_CB
**				E_QE0022_ABORTED
**				E_US1912_6418_SEQ_EXISTS
**				E_US1913_6419_SEQ_NOT_EXISTS
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	05-Mar-2002 (jenjo02)
**	    Invented.
**	14-Mar-2005 (thaju02)
**	    For drop sequence perform a cascading destroy. (B112545)
**	13-Jul-2006 (kiria01)  b112605
**	    Don't increment the db_tab_index to mark cache to be
**	    refreshed as will be noted in the dms_end_seq() call and
**	    cache subsequently flushed.
**	11-Jul-2010 (jonj) Bug 123896
**	    Acquire sequence id before opening newly created
**	    sequence, rather than after.
**	    dms_open() now forms LK_SEQUENCE locks using this unique
**	    id instead of sequence owner and name, which cannot
**	    be made into a unique lock key.
*/
static DB_STATUS
qeu_seq(
i4		action,
QEF_CB          *qef_cb,
QEUQ_CB	        *qeuq_cb)
{
    DB_IISEQUENCE  	*stuple;	/* New sequence tuple */
    DB_IISEQUENCE 	stuple_temp;	/* Tuple to check for uniqueness */
    DB_STATUS	    	status, local_status;
    i4	    		error;
    i4		    	transtarted = FALSE;	    
    i4		    	tbl_opened = FALSE;
    i4		    	seq_opened = FALSE;
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMR_ATTR_ENTRY  	skey_array[2];
    DMR_ATTR_ENTRY  	*skey_ptr_array[2];
    DMU_CB		dmu_cb;
    DMS_CB		dms_cb;
    DMS_SEQ_ENTRY	dms_seq;

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
	if ( (qeuq_cb->qeuq_culd != 1 || qeuq_cb->qeuq_uld_tup == NULL)
            || qeuq_cb->qeuq_db_id == NULL
            || qeuq_cb->qeuq_d_id == 0 )
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

	stuple = (DB_IISEQUENCE *)qeuq_cb->qeuq_uld_tup->dt_data;

    
	/* Open iisequences catalog */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_SEQ_TAB_ID;
        qeu.qeu_tab_id.db_tab_index  = DM_I_SEQ_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = QEU_BYPASS_PRIV;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);
	if ( status )
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Retrieve the named sequence */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_IISEQUENCE);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DB_IISEQUENCE);
        qef_data.dt_data = (PTR)&stuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 2;			
	qeu.qeu_key = skey_ptr_array;
	skey_ptr_array[0] = &skey_array[0];
	skey_ptr_array[1] = &skey_array[1];
	skey_ptr_array[0]->attr_number = DM_1_SEQ_KEY;
	skey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	skey_ptr_array[0]->attr_value = (char *)&stuple->dbs_name;
	skey_ptr_array[1]->attr_number = DM_2_SEQ_KEY;
	skey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	skey_ptr_array[1]->attr_value = (char *)&stuple->dbs_owner;
	qeu.qeu_qual = NULL;
	qeu.qeu_qarg = NULL;
	status = qeu_get(qef_cb, &qeu);

	if (status == E_DB_OK)		/* Found duplicate sequence */
	{
	    /* If create, that's an error */
	    if ( action == QEU_CSEQ )
	    {
		(VOID)qef_error(E_US1912_6418_SEQ_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1, 
			    qec_trimwhite(sizeof(stuple->dbs_name),
					  (PTR)&stuple->dbs_name),
					  (PTR)&stuple->dbs_name);
		qeu.error.err_code = E_QE0025_USER_ERROR;
		status = E_DB_ERROR;
	    }
		
	}
	else if ( qeu.error.err_code == E_QE0015_NO_MORE_ROWS )
	{
	    /* If alter/drop, that's an error */
	    if ( action == QEU_ASEQ || action == QEU_DSEQ )
	    {
		(VOID)qef_error(E_US1913_6419_SEQ_NOT_EXISTS, 0L, E_DB_ERROR,
			    &error, &qeuq_cb->error, 1, 
			    qec_trimwhite(sizeof(stuple->dbs_name),
					  (PTR)&stuple->dbs_name),
					  (PTR)&stuple->dbs_name);
		qeu.error.err_code = E_QE0025_USER_ERROR;
	    }
	    else
	    {
		/*
		** CREATE:
		** Get unique sequence id (a table id) from DMF
		** to be used as an internal identifer
		** and lock key element.
		*/
		dmu_cb.type = DMU_UTILITY_CB;
		dmu_cb.length = sizeof(DMU_CB);
		dmu_cb.dmu_flags_mask = 0;
		dmu_cb.dmu_tran_id = qef_cb->qef_dmt_id;
		dmu_cb.dmu_db_id = qeuq_cb->qeuq_db_id;
		status = dmf_call(DMU_GET_TABID, &dmu_cb);
		if (status == E_DB_OK)
		    stuple->dbs_uniqueid = dmu_cb.dmu_tbl_id;
		else
		    qeu.error.err_code = dmu_cb.error.err_code;
	    }
	}

	if ( status == E_DB_OK )
	{
	    /*
	    ** Open the Sequence. For DDL, this involves
	    ** placing an exclusive lock on the 
	    ** database id and sequence id.
	    ** 
	    ** DML Queries which have opened the Sequence hold
	    ** at least an IX lock on the sequence, so we'll be
	    ** blocked until they commit.
	    */
	    dms_cb.type = DMS_SEQ_CB;
	    dms_cb.length = sizeof(DMS_CB);
	    dms_cb.dms_flags_mask = (action == QEU_CSEQ) ? DMS_CREATE 
				   :(action == QEU_ASEQ) ? DMS_ALTER
				   : DMS_DROP;
	    dms_cb.dms_tran_id = qef_cb->qef_dmt_id;
	    dms_cb.dms_db_id = qeuq_cb->qeuq_db_id;
	    dms_cb.dms_seq_array.data_address = (PTR)&dms_seq;
	    dms_cb.dms_seq_array.data_in_size = sizeof(DMS_SEQ_ENTRY);
	    STRUCT_ASSIGN_MACRO(stuple->dbs_name, dms_seq.seq_name);
	    STRUCT_ASSIGN_MACRO(stuple->dbs_owner, dms_seq.seq_owner);
	    STRUCT_ASSIGN_MACRO(stuple->dbs_uniqueid, dms_seq.seq_id);

	    status = dmf_call(DMS_OPEN_SEQ, &dms_cb);
	    if ( status )
		qeu.error.err_code = dms_cb.error.err_code;
	    else
		seq_opened = TRUE;
	}

	if ( status == E_DB_OK && action == QEU_ASEQ )
	{
	    /* Alter: delete the old tuple */
	    qeu.qeu_getnext = QEU_NOREPO;
	    qeu.qeu_klen	= 0;
	    status = qeu_delete(qef_cb, &qeu);
	}

	if ( status == E_DB_OK && action == QEU_DSEQ )
	{
	    QEUQ_CB        dqeuq;

	    /* Delete: perform cascading destruction */
	    STRUCT_ASSIGN_MACRO(*qeuq_cb, dqeuq);
	    dqeuq.qeuq_flag_mask = 0;
	    dqeuq.qeuq_uld_tup = &qef_data;
	    dqeuq.qeuq_rtbl = &stuple->dbs_uniqueid;

	    status = qeu_d_cascade(qef_cb, &dqeuq, 
		(DB_QMODE) DB_SEQUENCE, (DMT_TBL_ENTRY *) NULL);

	    if (status != E_DB_OK)
		qeu.error.err_code = dqeuq.error.err_code;
	}

	if ( status == E_DB_OK && action != QEU_DSEQ )
	{
	    /* Create/Alter: Insert new sequence tuple */
	    dms_seq.seq_id = stuple->dbs_uniqueid;

	    qeu.qeu_count = 1;
	    qeu.qeu_tup_length = sizeof(DB_IISEQUENCE);
	    qeu.qeu_input = &qef_data;
	    qef_data.dt_data = (PTR)stuple;
	    status = qeu_append(qef_cb, &qeu);
	}

	if ( status == E_DB_OK )
	{
	    tbl_opened = FALSE;
	    status = qeu_close(qef_cb, &qeu);    

	    /* Close the DMF Sequence */
	    if ( status == E_DB_OK )
	    {
		seq_opened = FALSE;
		dms_seq.seq_status = status;
		status = dmf_call(DMS_CLOSE_SEQ, &dms_cb);
		qeu.error.err_code = dms_cb.error.err_code;
	    }

	    if ( status == E_DB_OK && transtarted )
	    {
		status = qeu_etran(qef_cb, &tranqeu);
		qeu.error.err_code = tranqeu.error.err_code;
	    }
	    if ( status == E_DB_OK )
	    {
		qeuq_cb->error.err_code = E_QE0000_OK;
		return (E_DB_OK);
	    }
	}
	error = qeu.error.err_code;
	break;
    } /* End dummy for */

    /* call qef_error to handle error/warning messages */
    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off all the tables. */
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

    if ( seq_opened )
    {
	dms_seq.seq_status = status;
	local_status = dmf_call(DMS_CLOSE_SEQ, &dms_cb);
	if ( local_status )
	{
	    (VOID) qef_error(dms_cb.error.err_code, 0L, local_status, 
			     &error, &qeuq_cb->error, 0);
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
}
