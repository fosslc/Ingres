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
**  Name: qeucomm.c - Provide add and delete comment support for RDF and PSF.
**
**  Description:
**      The two routines defined in this file provide the catalog update
**	support for the COMMENT ON statement.
**	
**	COMMENT ON results in the addition of a row into the iidbms_comment
**	catalog.  This is affected by the qeu_ccomment.c routine.
**
**	COMMENT ON can also result in the deletion of a row from iidbms_comment,
**	if the comment text is set to an empty string.  This is also affected
**	by qeu_ccomment.c.
**
**	DROP (table, view, or index) results in the deletion of all comments on
**	the table, view, or index, or any of its columns.  qeu_dcomment.c is
**	the routine provided for this.  ALTER TABLE/DROP COLUMN also results
**      in the deletion of a comment on the particular column.
**
**  Defines:
**      qeu_ccomment		- Add a comment
**      qeu_dcomment		- Delete comment(s)
**      qeu_fcomment            - Find comment
**
**  History:
** 	26-Jan-90 (carol)
**	   Written for support of COMMENT ON statement in Terminator phase II.
**	08-may-90 (carol)
**	    Added qeu_dcomment for cascading drops.
**
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	2-sep-93 (stephenb)
**	    Added call to qeu_secaudit(), creating/deleting a comment is an
**	    auditable action.
**      14-sep-93 (smc)
**          Fixed up casts of qeuq_d_id now session ids are CS_SID.
**	8-oct-93  (robf)
**	    Updated qeu_secaudit calls to new interface
**      27-jul-96 (ramra01 for bryanp)
**          Alter table drop column cascade option.
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
**	4-Jun-2009 (kschendel) b122118
**	    Make sure dmt-cb doesn't have junk in it.
*/

/*{
** Name: qeu_ccomment	- Update catalog to add or delete one comment
**
** External QEF call:   status = qef_call(QEU_CCOMMENT, &qeuq_cb);
**
** Description:
**	Respond to COMMENT ON statement by adding a row to the iidbms_comment
**	catalog.  If a comment already exists for the object, replace it with
**	this new one.  If this COMMENT ON statement contains an empty string
**	for the comment, that is the mechanism to delete an existing comment
**	on the object.  However, if no comment already exists, it is not an 
**	error condition;  simply ignore it.
**
**	The relstat field of iirelation is also updated to indicate that 
**	at least one comment exists for the object.  Note that (analogous to
**	some other relstat bits) this bit is not turned off when the last 
**      comment is deleted;  it simply reports that a comment did exist at one 
**	time.  This is used to signal cascading delete of comments when the 
**	object is deleted.
**
**	Note that at this time, the optional WITH SHORT_REMARK clause is not
**	supported.  The long remark is all that is supported, and it is not
**	optional.  Support for short remarks will be added at a later time.
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		How many comment tuples --- always 1.
**	    .qeuq_uld_tup	Comment tuple.
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
**	31-jan-90 (carol)
**	    Written for comments feature of Terminator II.
**	2-sep-93 (stephenb)
**	    Added call to qeu_secaudit(), creating/deleting a comment is an
**	    auditable action.
**	18-feb-95 (forky01)
**	    qeu_open was getting uninitialized qeu_mask which caused random
**	    maxlocks and lock timeout.
**      27-nov-09 (coomi01) b122958
**          Toggle comments bit in relstat rather than have it sticky.
**
*/
DB_STATUS
qeu_ccomment(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DB_IICOMMENT	*ctuple;	/* New Comment tuple */
    DB_IICOMMENT	ctuple_temp;	/* Comment tuple to check uniqueness */
    DMR_ATTR_ENTRY	ckey_array[3];	/* Keys for uniqueness check         */
    DMR_ATTR_ENTRY	*ckey_ptr_array[3];/* Keys for uniqueness check      */
    DB_STATUS	    	status, local_status;
    bool                otherCols;
    i4	      	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    i4		    	i;		/* Querytext and tree tuple counter */
    QEF_DATA	    	*next;		/*	     and data pointer       */
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMT_CHAR_ENTRY  	char_spec;	/* DMT_ALTER specification */
    DMT_CB	    	dmt_cb;
    DB_ERROR		e_error;

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

	if ((qeuq_cb->qeuq_culd != 1	    /*  must be one comment tuple */
	    || qeuq_cb->qeuq_uld_tup == NULL)
            || (qeuq_cb->qeuq_db_id == NULL)
            || (qeuq_cb->qeuq_d_id == 0))
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}
	
	/* Set local pointer to the comment tuple, and validate some of the
	** fields in it.
	*/

	ctuple = (DB_IICOMMENT *)qeuq_cb->qeuq_uld_tup->dt_data; /* point to
						** the new comment tuple,
						** which is the first thing
						** in the DB_1_IICOMMENT
						** struct that dt_data points
						** to.
						*/
	/* xxxxxxxx */	

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

	/* Check if there is an existing comment for this table or column */

	/* Open the comment catalog */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_COMMENT_TAB_ID; /* comment catalog */
        qeu.qeu_tab_id.db_tab_index  = DM_I_COMMENT_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);	/* open the comment catalog */
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/*
	** Set up data ouput descriptor
	*/
	qef_data.dt_next  = 0;
	qef_data.dt_size  = sizeof(DB_IICOMMENT);
	qef_data.dt_data  = (PTR) &ctuple_temp;
	
	/*
	** Set up QEU control block 
	*/
	qeu.qeu_count      = 1;
	qeu.qeu_getnext    = QEU_REPO;
	qeu.qeu_output     = &qef_data;
	qeu.qeu_tup_length = qef_data.dt_size;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;
	qeu.qeu_klen = 2;
	qeu.qeu_key  = ckey_ptr_array;

	/*
	** Set up items we are looking for match upon
	*/
	ckey_ptr_array[0] = &ckey_array[0];
	ckey_ptr_array[0]->attr_number = DM_1_COMMENT_KEY;
	ckey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	ckey_ptr_array[0]->attr_value = (char *)&ctuple->dbc_tabid.db_tab_base; /* comtabbase   */
	
	ckey_ptr_array[1] = &ckey_array[1];
	ckey_ptr_array[1]->attr_number = DM_2_COMMENT_KEY;
	ckey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	ckey_ptr_array[1]->attr_value = (char *)&ctuple->dbc_tabid.db_tab_index; /* comtabidx   */
	    
	/*
	** Walk the current comments matching this table, looking for other column comments
	*/
	otherCols = FALSE;
	for(;;)
	{
	    status = qeu_get(qef_cb, &qeu);
	    
	    if ( (status != E_DB_OK) || 
		 (qeu.error.err_code == E_QE0015_NO_MORE_ROWS)
		)
	    {
		/** DONE this quick scan **/
		break;
	    }

	    if ( ctuple_temp.dbc_attid != ctuple->dbc_attid )
	    {
		/* 
		** Other columns present, go no further.
		*/
		otherCols = TRUE;
		break;
	    }
	    
	    qeu.qeu_getnext    = QEU_NOREPO;
	}

	/* 
        ** Alter the table relstat to flag comments exist. This validates that
        ** the table exists and ensures that we get an exclusive lock on it.
        */
	char_spec.char_id = DMT_C_COMMENT;      /* create comment code */

	if (!otherCols && (0 == (ctuple -> dbc_len_long)))
	{
	    /* 
	    ** No others, and, we are going to delete current 
	    */
	    char_spec.char_value = DMT_C_OFF;
	}
	else
	{
	    /*
	    ** Others present anyway, our addtion/deletion irrelevent.
	    */
	    char_spec.char_value = DMT_C_ON;
	}

	MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
	dmt_cb.dmt_flags_mask = 0;
	dmt_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
	dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
	dmt_cb.dmt_char_array.data_address = (PTR)&char_spec;
	dmt_cb.length = sizeof(DMT_CB);
	dmt_cb.type = DMT_TABLE_CB;
	dmt_cb.dmt_id.db_tab_base  = ctuple->dbc_tabid.db_tab_base;
	dmt_cb.dmt_id.db_tab_index = ctuple->dbc_tabid.db_tab_index;
	dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
	status = dmf_call(DMT_ALTER, &dmt_cb);
	if (status != E_DB_OK)
	{
	    error = dmt_cb.error.err_code;
	    break;
	}

	/* Delete a comment on this object.  If there is one,  it will
	** be replaced by this new one.  If there isn't one, no problem.
	** Either way, this new comment is going to be added.
        */
	qeu.qeu_count = 1;
        qeu.qeu_tup_length = sizeof(DB_IICOMMENT);
	qeu.qeu_output = &qef_data;
	qef_data.dt_next = NULL;
        qef_data.dt_size = sizeof(DB_IICOMMENT);
        qef_data.dt_data = (PTR)&ctuple_temp;
	qeu.qeu_getnext = QEU_REPO;
	qeu.qeu_klen = 3;			/* keyed on table id (two
						** columns), and attid   */
	qeu.qeu_key = ckey_ptr_array;
	ckey_ptr_array[0] = &ckey_array[0];
	ckey_ptr_array[0]->attr_number = DM_1_COMMENT_KEY;
	ckey_ptr_array[0]->attr_operator = DMR_OP_EQ;
    	ckey_ptr_array[0]->attr_value = 
	    (char *)&ctuple->dbc_tabid.db_tab_base; /* comtabbase   */
	ckey_ptr_array[1] = &ckey_array[1];
	ckey_ptr_array[1]->attr_number = DM_2_COMMENT_KEY;
	ckey_ptr_array[1]->attr_operator = DMR_OP_EQ;
    	ckey_ptr_array[1]->attr_value = 
	    (char *)&ctuple->dbc_tabid.db_tab_index; /* comtabidx   */
	ckey_ptr_array[2] = &ckey_array[2];
	ckey_ptr_array[2]->attr_number = DM_3_COMMENT_KEY;
	ckey_ptr_array[2]->attr_operator = DMR_OP_EQ;
    	ckey_ptr_array[2]->attr_value = 
	    (char *)&ctuple->dbc_attid;		     /* comattid    */
	qeu.qeu_qual = NULL;		    /* qualification is done by key */
	qeu.qeu_qarg = NULL;
	status = qeu_delete(qef_cb, &qeu);  /* DELETE */

	/* if a row was deleted (OK), or if no row was deleted (NO_MORE_ROWS),
	** those are the expected conditions.  If something else happened,
	** it's an error.
	*/
	if (status != E_DB_OK && qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}


	status = E_DB_OK;

	/* Check that the specified comment is not a zero-length string.
	** If it is, that is equivalent to a drop comment operation.  The
	** existing comment (if it did exist) has already been dropped, just
	** skip the insert step   
	*/

	if (ctuple -> dbc_len_long != 0)
	    {
		/* Insert single comment tuple */
		qeu.qeu_count = 1;
	        qeu.qeu_tup_length = sizeof(DB_IICOMMENT);
		qeu.qeu_input = qeuq_cb->qeuq_uld_tup;
		status = qeu_append(qef_cb, &qeu);
		if (status != E_DB_OK)
		{
		    error = qeu.error.err_code;
		    break;
		}
	    }
	/* Close the comment catalog */
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;
	/*
	** We now have a new comment in the catalog, we need to audit
	*/
	if ( Qef_s_cb->qef_state & QEF_S_C2SECURE )
	{
	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		((DB_1_IICOMMENT *)ctuple)->dbc_tabname.db_tab_name,
		&qef_cb->qef_user,
		sizeof( ((DB_1_IICOMMENT *)ctuple)->dbc_tabname ),
		SXF_E_TABLE, I_SX2039_TBL_COMMENT, SXF_A_SUCCESS | SXF_A_CONTROL,
		&e_error);
	    if (status != E_DB_OK)
	    {
		error = e_error.err_code;
		break;
	    }
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
	return (E_DB_OK);
    } /* End dummy for */

    /* call qef_error to handle error messages */
    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off the comment catalog, if still open.  */
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
} /* qeu_ccomment */

/*{
** Name: qeu_dcomment	- Update catalog to cascade delete comments.
**
** External QEF call:   status = qef_call(QEU_DCOMMENT, &qeuq_cb);
**
** Description:
**	Respond to the DROP TABLE or ALTER TABLE...DROP COLUMN statement
**	by dropping all comments on the dropped object.
**	For tables (view or indexes), all comments on the table and all
**	comments of the table are dropped.  For a column, the comment on
**	the column is dropped.
**
** Inputs:
**      qef_cb				QEF session control block
**      qeuq_cb
**	    .qeuq_eflag			Designate error handling for user
**					errors.
**		QEF_INTERNAL		Return error code.
**		QEF_EXTERNAL		Send message to user.
**	    .qeuq_uld_tup		NULL.
**	    .qeuq_rtbl			id of the table all of whose comments
**					are to be dropped.
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
**	07-may-90 (carol)
**	    Written for comments feature.
**	18-feb-95 (forky01)
**	    qeu_open was getting uninitialized qeu_mask which caused random
**	    maxlocks and lock timeout.
**	27-jul-96 (ramra01 for bryanp)
**	    Alter table drop column cascade option.
**	14-Jan-2004 (jenjo02)
**	    Check db_tab_index > 0 for index, not simply != 0.
*/

DB_STATUS
qeu_dcomment(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb)
{
    DMR_ATTR_ENTRY	ckey_array[3];	/* Keys for lookup         */
    DMR_ATTR_ENTRY	*ckey_ptr_array[3];/* Keys for lookup      */
    DB_STATUS	    	status, local_status;
    i4	    	error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;
    DMT_CHAR_ENTRY	char_spec;
    DMT_CB		dmt_cb;
    bool		comment_exists;

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

	if ((qeuq_cb->qeuq_uld_tup != NULL)
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

	/* Open the comment catalog */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_COMMENT_TAB_ID; /* comment catalog */
        qeu.qeu_tab_id.db_tab_index  = DM_I_COMMENT_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);	/* open the comment catalog */
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/* Delete any comment[s] on this object.  For a table, this includes
	** all column comments on the table as well.  For a column only
	** the one column comment will be deleted.
        */
        qeu.qeu_tup_length = sizeof(DB_IICOMMENT);
	qeu.qeu_output = (QEF_DATA *) NULL;
	qef_data.dt_next = NULL;
	qeu.qeu_key = ckey_ptr_array;
	ckey_ptr_array[0] = &ckey_array[0];
	ckey_ptr_array[0]->attr_number = DM_1_COMMENT_KEY;
	ckey_ptr_array[0]->attr_operator = DMR_OP_EQ;
    	ckey_ptr_array[0]->attr_value = 
	    (char *)&qeuq_cb->qeuq_rtbl->db_tab_base; /* comtabbase   */
	ckey_ptr_array[1] = &ckey_array[1];
	ckey_ptr_array[1]->attr_number = DM_2_COMMENT_KEY;
	ckey_ptr_array[1]->attr_operator = DMR_OP_EQ;
    	ckey_ptr_array[1]->attr_value = 
	    (char *)&qeuq_cb->qeuq_rtbl->db_tab_index; /* comtabidx   */
	ckey_ptr_array[2] = &ckey_array[2];
	ckey_ptr_array[2]->attr_number = DM_3_COMMENT_KEY;
	ckey_ptr_array[2]->attr_operator = DMR_OP_EQ;
    	ckey_ptr_array[2]->attr_value = 
	    (char *)&qeuq_cb->qeuq_aid;	 		/* comattid    */
	qeu.qeu_qual = NULL;		    /* qualification is done by key */
	qeu.qeu_qarg = NULL;

	/* Set up for the search.  If this table is not an index, do the
	** search on db_tab_base (reltid) only.  This will take care of
	** dropping any index or index column comments that may exist.
	** Since the cascading drop of indexes when a table is dropped is
	** handled down in dmf, this is the easiest way to catch these.
	** If the table is an index, then do the search on both fields of
	** the table id (reltid and reltidx).
	*/
	if (qeuq_cb->qeuq_flag_mask & QEU_DROP_COLUMN)
	    qeu.qeu_klen = 3;

	else if (qeuq_cb->qeuq_rtbl->db_tab_index > 0L)    /* index */
	    qeu.qeu_klen = 2;			/* keyed on table id (two
						** columns)               */
	else					/* table */
	    qeu.qeu_klen = 1;			/* keyed on table part of
						** table id (one column)  */

	/* Do the delete */

	status = qeu_delete(qef_cb, &qeu);  /* DELETE */

	/* if rows were deleted (OK), or if no row was deleted (NO_MORE_ROWS),
	** those are the expected conditions.  If something else happened,
	** it is an error.
	*/
	if (status != E_DB_OK && qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	{
	    /* Some other error */
	    error = qeu.error.err_code;
	    break;
	}

	comment_exists = FALSE;

	if (qeuq_cb->qeuq_flag_mask & QEU_DROP_COLUMN)
	{
	   qeu.qeu_getnext = QEU_REPO;
	   qeu.qeu_klen = 1;
	   qeu.qeu_qual = NULL;
	   qeu.qeu_qarg = NULL;
	   qeu.qeu_flag |= QEU_POSITION_ONLY;

	   status = qeu_get(qef_cb, &qeu); 

	   if ( (status != E_DB_OK) &&
	   	(qeu.error.err_code == E_QE0015_NO_MORE_ROWS) )
	   {
	      comment_exists = FALSE;
	   }
	   else if (status != E_DB_OK)
	   {
	      error = qeu.error.err_code;
	      break;
	   }
	   else
	   {
	      comment_exists = TRUE;
	   }
	}

	if (comment_exists == FALSE)
	{
	   char_spec.char_id = DMT_C_COMMENT;
	   char_spec.char_value = DMT_C_OFF;
	   MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
	   dmt_cb.dmt_flags_mask = 0;
	   dmt_cb.dmt_db_id = qeuq_cb->qeuq_db_id;
	   dmt_cb.dmt_char_array.data_in_size = sizeof(DMT_CHAR_ENTRY);
	   dmt_cb.dmt_char_array.data_address = (PTR) &char_spec;
	   dmt_cb.length = sizeof(DMT_CB);
	   dmt_cb.type = DMT_TABLE_CB;
	   dmt_cb.dmt_id.db_tab_base = qeuq_cb->qeuq_rtbl->db_tab_base;
	   dmt_cb.dmt_id.db_tab_index = qeuq_cb->qeuq_rtbl->db_tab_index;
	   dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;

	   status = dmf_call(DMT_ALTER, &dmt_cb);

	   if (status != E_DB_OK)
	   {
	      error = dmt_cb.error.err_code;
	      break;
	   }
	}

	/* Close the comment catalog */
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

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
	return (E_DB_OK);
    } /* End dummy for */

    /* call qef_error to handle error messages */
    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    
    /* Close off the comment catalog, if still open.  */
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
} /* qeu_dcomment */



/*{
** Name: qeu_fcomment	- Find comment in catalog.
**
**
** Description:
**      Search through comments catalog looking for a comment 
**      on a specific column.
**
** Inputs:
**      qef_cb				QEF session control block
**      qeuq_cb
**	    .qeuq_eflag			Designate error handling for user
**					errors.
**		QEF_INTERNAL		Return error code.
**		QEF_EXTERNAL		Send message to user.
**	    .qeuq_uld_tup		NULL.
**	    .qeuq_rtbl			id of the table all of whose comments
**					are to be dropped.
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
**      History:
**      27-nov-09 (coomi01) b122958
**          Created.
**      
*/
DB_STATUS
qeu_fcomment(
QEF_CB          *qef_cb,
QEUQ_CB		*qeuq_cb,
DB_IICOMMENT    **outBufPtr)
{
    DMR_ATTR_ENTRY	ckey_array[3];	/* Keys for lookup         */
    DMR_ATTR_ENTRY	*ckey_ptr_array[3];/* Keys for lookup      */
    DB_STATUS	    	status, local_status;
    i4	    	        error;
    bool	    	transtarted = FALSE;	    
    bool	    	tbl_opened = FALSE;
    QEU_CB	    	tranqeu;
    QEU_CB	    	qeu;
    QEF_DATA	    	qef_data;

    for (;;)	/* Dummy for loop for ERROR breaks */
    {
	/* Validate CB and parameters */
	if (qeuq_cb->qeuq_type != QEUQCB_CB ||
            qeuq_cb->qeuq_length != sizeof(QEUQ_CB))	
	{
	    status = E_DB_ERROR;
	    error = E_QE0017_BAD_CB;
	    break;
	}

	if ((qeuq_cb->qeuq_uld_tup != NULL)
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



	/* OK, Open the comment catalog */
	qeu.qeu_type = QEUCB_CB;
        qeu.qeu_length = sizeof(QEUCB_CB);
        qeu.qeu_tab_id.db_tab_base  = DM_B_COMMENT_TAB_ID; /* comment catalog */
        qeu.qeu_tab_id.db_tab_index  = DM_I_COMMENT_TAB_ID;
        qeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        qeu.qeu_lk_mode = DMT_IX;
        qeu.qeu_flag = DMT_U_DIRECT;
        qeu.qeu_access_mode = DMT_A_WRITE;
	qeu.qeu_mask=0;
	status = qeu_open(qef_cb, &qeu);	/* open the comment catalog */
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = TRUE;

	/*
	** Set up data ouput descriptor
	** ~ Space was passed in from caller
	*/
	qef_data.dt_next  = 0;
	qef_data.dt_size  = sizeof(DB_IICOMMENT);
	qef_data.dt_data  = (PTR)*outBufPtr;
	
	/*
	** Set up QEU control block 
	*/
	qeu.error.err_code = 0;
	qeu.qeu_count      = 1;
	qeu.qeu_getnext    = QEU_REPO;
	qeu.qeu_output     = &qef_data;
	qeu.qeu_tup_length = qef_data.dt_size;
	qeu.qeu_qual = 0;
	qeu.qeu_qarg = 0;
	qeu.qeu_klen = 3;
	qeu.qeu_key  = ckey_ptr_array;

	/*
	** Set up item we are looking for match upon
	** 0 ~ base table
	** 1 ~ index
	** 2 ~ column ident
	*/
	ckey_ptr_array[0] = &ckey_array[0];
	ckey_ptr_array[0]->attr_number = DM_1_COMMENT_KEY;
	ckey_ptr_array[0]->attr_operator = DMR_OP_EQ;
	ckey_ptr_array[0]->attr_value =
	    (char *)&qeuq_cb->qeuq_rtbl->db_tab_base; /* comtabbase   */	

	ckey_ptr_array[1] = &ckey_array[1];
	ckey_ptr_array[1]->attr_number = DM_2_COMMENT_KEY;
	ckey_ptr_array[1]->attr_operator = DMR_OP_EQ;
	ckey_ptr_array[1]->attr_value = 
	    (char *)&qeuq_cb->qeuq_rtbl->db_tab_index; /* comtabidx   */

	ckey_ptr_array[2] = &ckey_array[2];
	ckey_ptr_array[2]->attr_number = DM_3_COMMENT_KEY;
	ckey_ptr_array[2]->attr_operator = DMR_OP_EQ;
	ckey_ptr_array[2]->attr_value = 
	    (char *)&qeuq_cb->qeuq_aid;	 		/* comattid    */	    

	status = qeu_get(qef_cb, &qeu); 
	    
	if (status != E_DB_OK)
	{
	    /* 
	    ** Item not there, so null the buffer pointer
	    ** ~ This effects an output indicator.
	    */
	    *outBufPtr = NULL;

	    /*
	    ** Have to deal with any errors other than no-more-rows
	    */
	    if (qeu.error.err_code != E_QE0015_NO_MORE_ROWS)
	    {
		error = qeu.error.err_code;
		break;
	    }
	}

	/* Now Close the comment catalog */
	status = qeu_close(qef_cb, &qeu);    
	if (status != E_DB_OK)
	{
	    error = qeu.error.err_code;
	    break;
	}
	tbl_opened = FALSE;

	/* take care of the transaction state */
	if (transtarted)		   /* If we started a transaction */
	{
	    status = qeu_etran(qef_cb, &tranqeu);
	    if (status != E_DB_OK)
	    {
		error = tranqeu.error.err_code;
		break;
	    }
	    transtarted = FALSE;
	}
	qeuq_cb->error.err_code = E_QE0000_OK;

	/*
	** If all goes well, even if no comment present, we return here
	*/
	return (E_DB_OK);

    } 

    /* End dummy for-loop, 
    ** Exiting via a break MEANS an error occured. 
    ** Handle error, close catalog, and transaction if required.
    */


    /* call qef_error to handle error messages */
    (VOID) qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    

    /* Close off the comment catalog, if still open.  */
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

}  /* qeu_fcomment */
