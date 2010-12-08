/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <cs.h>
#include    <scf.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qefscb.h>

#include    <ade.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <sxf.h>
#include    <qefprotos.h>

/**
**
**  Name: QEUQ.C - record level utility functions for QEF
**
**  Description:
**      These are the external entry points for functions that
**  provide query-like and transaction dependent operations.
**  These functions don't fit into the other QEF catagories
**  (transaction, control or query). 
**
**          qeu_append  - append rows to table
**          qeu_close   - close table
**          qeu_delete  - delete rows from a table
**          qeu_get     - fetch rows from a table
**	    qeu_replace - replace a row in a table
**          qeu_open    - open table
**
**
**
**  History:    $Log-for RCS$
**      30-apr-86 (daved)    
**          written
**      31-jul-86 (jennifer)
**          Modified qeu_get to not allocate output
**          from memory stream, but to assume output
**          array is already initialized.
**      02-sep-86 (jennifer)
**          Modified qeu_delete to return a count of tuples 
**          deleted.
**	12-oct-87 (puree)
**	    Modified qeu_get to map DM0055 to QE0015 during positioning.
**      10-dec-87 (puree)
**          Converted all ulm_palloc to qec_palloc
**	10-feb-88 (puree)
**	    Converted ulm_openstream to qec_mopen.  Rename qec_palloc to
**	    qec_malloc.
**	29-aug-88 (puree)
**	    Ignore duplicate records/keys in qeu_load.
**      18-jul-89 (jennifer)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**	04-jan-91 (andre)
**	    modified qeu_delete() to allow removal of tuples by TID
**	23-jan-91 (seputis)
**	    added support for timeout and maxlocks
**	18-jun-92 (andre)
**	    defined qeu_replace()
**	08-dec-92 (anitap)
**	    Added #include <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	26-jul-93 (walt)
**	    Initialize the local variable "status" in qeu_append to E_DB_OK.
**	    If there are no rows to append it's possible to hit the closing
**	    return(status) statement without ever assigning status a
**	    non-garbage value.
**	13-sep-93 (robf)
**	    Extend qeu_replace() to handle update multiple rows without
**	    requiring a key position.
**      14-sep-93 (smc)
**          removed redundant &.
**      01-feb-95 (stial01)
**          BUG 66409: qeu_append() copy from is hanging if 
**          DM0151_SI_DUPLICATE_KEY_STMT. If this error occurs, process it 
**          like the other duplicate errors.
**	06-mar-96 (nanpr01)
**	    removed the dependency on DB_MAXTUP for increased tuple size
**	    project.
**	19-Aug-1997 (jenjo02)
**	    Distinguish between private and shared ULM streams.
**	    Piggyback ulm_palloc() calls with ulm_openstream() where appropriate.
**	25-mar-99 (shust01)
**	    Initialized dmt_mustlock.  On rare occasions storage happened
**	    to be equal to a 1 (TRUE), causing locks to be taken out even
**	    though readlock=nolock was set.
**	16-oct-1999 (nanpr01)
**	    Initialize the dmr_s_estimated_records for not doing the intelligent
**	    read ahead.
**      07-mar-2000 (gupsh01)
**          qeu_open now does not handle error E_DM006A_TRAN_ACCESS_CONFLICT, as 
**          it is being handled by the calling facility. (BUG 100777).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Jan-2001 (jenjo02)
**	    Fix memory leaks in qeu_delete(), qeu_replace() that
**	    occur if E_DM0055_NONEXT is returned after
**	    DMR_POSITION.
**	26-Apr-2004 (schka24)
**	    Remove qeu_load, adds no value.
**	11-Apr-2008 (kschendel)
**	    Rework dmf qualification call sequence.
**	12-May-2009 (kiria01) b122074
**	    Initialise dmr_cb.dmr_q_fcn so not called
**	    by mistake.
**	4-Jun-2009 (kschendel) b122118
**	    Make sure dmt-cb doesn't have junk in it.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Prototype fixes.
**/

/*{
** Name: QEU_APPEND     - append rows to table
**
** External QEF call:   status = qef_call(QEU_APPEND, &qeu_cb);
**
** Description:
**      This routine appends N tuples to an opened table. The table
** must have been opened with the QEU_OPEN command. This operation
** is valid only in an internal transaction (see QET.C).
**
** Inputs:
**       qeu_cb
**	    .qeu_eflag			    designate error handling semantis
**					    for user errors.
**		QEF_INTERNAL		    return error code.
**		QEF_EXTERNAL		    send message to user.
**          .qeu_acc_id                     table access id
**          .qeu_count                      number of tuples to append
**          .qeu_tup_length                 lenth of each tuple
**          .qeu_input                      input buffer
**					    
**
** Outputs:
**      qeu_cb
**          .qeu_count                      Number of tuples appended
**          .error.err_code                 One of the following
**                                          E_QE0000_OK
**                                          E_QE0002_INTERNAL_ERROR
**                                          E_QE0004_NO_TRANSACTION
**                                          E_QE0007_NO_CURSOR
**                                          E_QE0017_BAD_CB
**                                          E_QE0018_BAD_PARAM_IN_CB
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-may-86 (daved)
**          written
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	26-jul-93 (walt)
**	    Initialize the local variable "status" in qeu_append to E_DB_OK.
**	    If there are no rows to append it's possible to hit the closing
**	    return(status) statement without ever assigning status a
**	    non-garbage value.
**      01-feb-94 (stial01)
**          BUG 66409: copy from is hanging if DM0151_SI_DUPLICATE_KEY_STMT,
**          If this error occurs process it like the other duplicate errors. 
*/
DB_STATUS
qeu_append(
QEF_CB      *qef_cb,
QEU_CB      *qeu_cb)
{
    i4     err;
    DMR_CB      dmr_cb;
    i4     count;              /* number of tuples to appended */
    DB_STATUS   status = E_DB_OK;
    QEF_DATA    *dataptr;
    i4          i;
    i4		num_appended;	    /* number of tuples appended */

    count = qeu_cb->qeu_count;
    qeu_cb->qeu_count = 0;

    /* QEU_APPEND is only valid in a transaction */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
        qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
            &qeu_cb->error, 0);
        return (E_DB_ERROR);
    }
    dmr_cb.type                 = DMR_RECORD_CB;
    dmr_cb.length               = sizeof(DMR_CB);
    dmr_cb.dmr_flags_mask	= 0;
    dmr_cb.dmr_access_id        = qeu_cb->qeu_acc_id;
    dmr_cb.dmr_tid              = 0;                    /* not used */
    dataptr                     = qeu_cb->qeu_input;    
    /* start appending rows */
    num_appended = 0;
    for (i = 0; i < count; i++)
    {
        dmr_cb.dmr_data.data_address = dataptr->dt_data;
        dmr_cb.dmr_data.data_in_size = dataptr->dt_size;
        status = dmf_call(DMR_PUT, &dmr_cb);
        if (status != E_DB_OK)
        {
	    /* *** FIXME *** This is insanely stupid -- dup rows/keys
	    ** are just ignored.  That was bad enough for COPY FROM,
	    ** which no longer comes here, but it seems that a bunch of
	    ** the QEU hurlers EXPECT a silent success on dup key/row.
	    ** Someday, root that crap out, and allow this routine to
	    ** treat errors in some rational manner...
	    */

	    if (dmr_cb.error.err_code == E_DM0046_DUPLICATE_RECORD ||
		  dmr_cb.error.err_code == E_DM0045_DUPLICATE_KEY ||
		  dmr_cb.error.err_code == E_DM0048_SIDUPLICATE_KEY ||
		  dmr_cb.error.err_code == E_DM0150_DUPLICATE_KEY_STMT ||
		  dmr_cb.error.err_code == E_DM0151_SIDUPLICATE_KEY_STMT)
	    {
		status = E_DB_OK;
		qeu_cb->error.err_code = dmr_cb.error.err_code;
	    }
	    else
	    {
		qef_error(dmr_cb.error.err_code, 0L, status, 
					&err, &qeu_cb->error, 0);
		return (status);
	    }
        }
        else
        {
            /* successful append */
            num_appended++;
        }
        /* next next record */
        dataptr = dataptr->dt_next;
    }
    /* tell user how many rows were appended */
    qeu_cb->qeu_count = num_appended;
    return (status);
}

/*{
** Name: QEU_CLOSE      - close an opened table
**
** External QEF call:       status = qef_call(QEU_CLOSE, &qeu_cb);
**
** Description:
**      Close a table opened through the QEU_OPEN command. 
**
** Inputs:
**      qeu_cb
**	    .qeu_eflag			    designate error handling semantis
**					    for user errors.
**		QEF_INTERNAL		    return error code.
**		QEF_EXTERNAL		    send message to user.
**          .qeu_acc_id			    table access id
**
** Outputs:
**      qeu_cb
**          .error.err_code                 One of the following
**                                          E_QE0000_OK
**                                          E_QE0002_INTERNAL_ERROR
**                                          E_QE0004_NO_TRANSACTION
**                                          E_QE0007_NO_CURSOR
**                                          E_QE0017_BAD_CB
**                                          E_QE0018_BAD_PARAM_IN_CB
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-may-86 (daved)
**          written
**	14-may-87 (daved)
**	    zero the access id to cause an error in DMF if a table is
**	    closed twice.
*/
DB_STATUS
qeu_close(
QEF_CB      *qef_cb,
QEU_CB      *qeu_cb)
{
    i4     err;
    DMT_CB      dmt_cb;
    DB_STATUS   status;

    /* QEU_CLOSE is only valid in a transaction */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
        qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
            &qeu_cb->error, 0);
        return (E_DB_ERROR);
    }
    MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
    dmt_cb.type         = DMT_TABLE_CB;
    dmt_cb.length       = sizeof(DMT_CB);
    dmt_cb.dmt_flags_mask   = 0;
    dmt_cb.dmt_record_access_id = qeu_cb->qeu_acc_id;
    status = dmf_call(DMT_CLOSE, &dmt_cb);
    if (status != E_DB_OK)
    {
        qef_error(dmt_cb.error.err_code, 0L, status, &err, &qeu_cb->error, 0);
    }
    else
	qef_cb->qef_open_count--;
    qeu_cb->qeu_acc_id = (PTR) NULL;
    return (status);
}

/*{
** Name: QEU_DELETE     - delete tuples from a table
**
** External QEF call:   status = qef_call(QEU_DELETE, &qeu_cb);
**
** Description:
**      Tuples are deleted from a table opened with the QEU_OPEN
** command. If no qualification is given, the current tuple is 
** deleted.
**
** Inputs:
**       qeu_cb
**	    .qeu_eflag			    designate error handling semantis
**					    for user errors.
**		QEF_INTERNAL		    return error code.
**		QEF_EXTERNAL		    send message to user.
**          .qeu_acc_id                     table access id
**          .qeu_tup_length                 tuple length of the table
**					    Only required for keyed delete
**          .qeu_qual                       qualification function
**          .qeu_qarg                       argument to qualification function
**          .qeu_klen                       length of key - number of entries
**          .qeu_key                        key for delete
**      <qeu_key> is a pointer to an array of pointers of type DMR_ATTR_ENTRY
**              .attr_number
**              .attr_operation
**              .attr_value_ptr
**	    .qeu_flag			    operation qualifier
**		QEU_BY_TID		    remove tuple whose TID is in qeu_tid
**	    .qeu_tid			    contains TID of the tuple to be
**					    removed if (qeu_flag & QEU_BY_TID)
**
** Outputs:
**      qeu_cb
**          .qeu_count                      number of tuples retrieved
**          .error.err_code                 One of the following
**                                          E_QE0000_OK
**                                          E_QE0002_INTERNAL_ERROR
**                                          E_QE0004_NO_TRANSACTION
**                                          E_QE0007_NO_CURSOR
**                                          E_QE0017_BAD_CB
**                                          E_QE0018_BAD_PARAM_IN_CB
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-may-86 (daved)
**          written
**      02-sep-86 (jennifer)
**          Modified qeu_delete to return a count of tuples 
**          deleted.
**	21-oct-86 (daved)
**	    did above fix for case where dmf returns no more rows.
**	22-dec-86 (daved)
**	    return OK if deleting keyed records
**      10-dec-87 (puree)
**          Converted all ulm_palloc to qec_palloc
**	04-jan-91 (andre)
**	    added functionality to delete tuples by TID
**	18-jun-92 (andre)
**	    do not allocate memory unless we will actually be reading tuples,
**	    i.e. if a key is specified
**	06-mar-96 (nanpr01)
**	    removed the dependency on DB_MAXTUP for increased tuple size
**	    project. Also added the check to make sure tuple size is set
**	    whoever called this routine.  Also tuple size consistency is
**	    checked iff qeu_klen > 0.
**	24-Jan-2001 (jenjo02)
**	    Ensure that memory stream is closed before returning.
**	11-Apr-2008 (kschendel)
**	    Revised DMF qualification requirements, simplified for QEU.
*/
DB_STATUS
qeu_delete(
QEF_CB          *qef_cb,
QEU_CB          *qeu_cb)
{
    i4         err;
    i4              count;
    DMR_CB          dmr_cb;
    DB_STATUS       status;
    GLOBALREF QEF_S_CB *Qef_s_cb;
    ULM_RCB         ulm;        /* so we don't need to allocate a tuple buffer
                                ** as a stack variable, use dynamic memory
                                */

    count = 0;
    qeu_cb->qeu_count = 0;
    qeu_cb->error.err_code = E_QE0000_OK;

    /* QEU_DELETE is only valid in a transaction */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
        qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
            &qeu_cb->error, 0);
        return (E_DB_ERROR);
    }
    dmr_cb.type                 = DMR_RECORD_CB;
    dmr_cb.length               = sizeof(DMR_CB);
    dmr_cb.dmr_access_id        = qeu_cb->qeu_acc_id;
    dmr_cb.dmr_q_fcn		= NULL;

    /* position the cursor */
    /* if there is a key, position by qual. Else, position all records */
    if (qeu_cb->qeu_klen)
    {
        if (qeu_cb->qeu_tup_length <= 0)
        {
            qef_error(E_QE0018_BAD_PARAM_IN_CB, 0L, E_DB_ERROR, &err,
                &qeu_cb->error, 0);
            return (E_DB_ERROR);
        }
	/* allocate memory for a tuple only if planning to call qeu_get() */
	
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
	/* Open stream and allocate tuple memory in one action */
	ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm.ulm_psize = ulm.ulm_blocksize = qeu_cb->qeu_tup_length;

	if ((status = qec_mopen(&ulm)) != E_DB_OK)
	{
	    qef_error(E_QE001E_NO_MEM, 0L, E_DB_ERROR, &err,
		&qeu_cb->error, 0);
	    return (status);
	}

        /* set the keys */
        dmr_cb.dmr_position_type = DMR_QUAL;
        dmr_cb.dmr_attr_desc.ptr_address    = (PTR) qeu_cb->qeu_key;
        dmr_cb.dmr_attr_desc.ptr_in_count   = qeu_cb->qeu_klen;
	dmr_cb.dmr_attr_desc.ptr_size	    = sizeof (DMR_ATTR_ENTRY);
	dmr_cb.dmr_s_estimated_records	    = -1;

        /* row qualifier */
        dmr_cb.dmr_q_fcn = (DB_STATUS (*)(void *,void *)) qeu_cb->qeu_qual;
        dmr_cb.dmr_q_arg = (PTR) qeu_cb->qeu_qarg;
	if (qeu_cb->qeu_qual != NULL)
	{
	    dmr_cb.dmr_q_rowaddr = &qeu_cb->qeu_qarg->qeu_rowaddr;
	    dmr_cb.dmr_q_retval = &qeu_cb->qeu_qarg->qeu_retval;
	}
	dmr_cb.dmr_flags_mask = 0;
        status = dmf_call(DMR_POSITION, &dmr_cb);
        if (status != E_DB_OK)
        {
	    ulm_closestream(&ulm);
	    if (dmr_cb.error.err_code == E_DM0055_NONEXT)
		return (E_DB_OK);

            qef_error(dmr_cb.error.err_code, 0L, status, &err, &qeu_cb->error, 0);
            return (status);
        }
        /* the tuple length will not change. Tell DMF about it */
	dmr_cb.dmr_data.data_in_size = qeu_cb->qeu_tup_length;
	dmr_cb.dmr_data.data_address = ulm.ulm_pptr;
    }

    for (;;)
    {
        if (qeu_cb->qeu_klen)
	{
            /* get the tuple */
            dmr_cb.dmr_flags_mask = DMR_NEXT;
            status = dmf_call(DMR_GET, &dmr_cb);
            if (status != E_DB_OK)
            {
                if (dmr_cb.error.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
                    break;
		}
                else
                {
                    qef_error(dmr_cb.error.err_code, 0L, status, 
                        &err, &qeu_cb->error, 0);
                }
                break;
            }
	}
	/*
	** delete the tuple: if deleting by TID, copy qeu_tid to dmr_tid and set
	** dmr_flags_mask to DMR_BY_TID
	*/
	if (qeu_cb->qeu_flag & QEU_BY_TID)
	{
	    dmr_cb.dmr_flags_mask = DMR_BY_TID;
	    dmr_cb.dmr_tid = qeu_cb->qeu_tid;
	}
	/* otherwise delete the current tuple */
	else
	{
	    dmr_cb.dmr_flags_mask = DMR_CURRENT_POS;
	}

        status = dmf_call(DMR_DELETE, &dmr_cb);
        if (status != E_DB_OK)
        {
            qef_error(dmr_cb.error.err_code, 0L, status, 
                &err, &qeu_cb->error, 0);
            break;
        }
	count++;
	if (qeu_cb->qeu_klen == 0)
	{
	    /* we are only deleting current tuple */
	    break;        
        }
    }
    
    if (qeu_cb->qeu_klen)
    {
	/* don't try to close a stream unless it was opened */
	ulm_closestream(&ulm);
    }
    
    qeu_cb->qeu_count = count;
    return (status);
}

/*{
** Name: QEU_GET        - get tuples from an open table
**
** External QEF call:   status = qef_call(QEU_GET, &qeu_cb);
**
** Description:
**      Tuples are retrieved from an open table. 
**
** Inputs:
**       qeu_cb
**	    .qeu_eflag			    designate error handling semantis
**					    for user errors.
**		QEF_INTERNAL		    return error code.
**		QEF_EXTERNAL		    send message to user.
**          .qeu_acc_id                     table access id
**          .qeu_tup_length                 length of retrieved tuple
**          .qeu_count                      number of tuples to retrieve
**          .qeu_getnext                    reposition	
**              QEU_REPO		    repostion using key
**		QEU_NOREPO		    continue from previous call to
**					    this routine
**          .qeu_klen                       length of key - number of entries
**          .qeu_key                        key for retrieve
**          .qeu_qual                       qualification function
**          .qeu_qarg                       argument to qualification function
**	    .qeu_flag			    operation qualifier
**		QEU_BY_TID		    get tuple by TID - will only be
**					    considered if !QEU_REPO
**	    .qeu_tid                        contains TID of the tuple to be
**					    retrieved
**          .qeu_output                     output buffer
**      <qeu_key> is a pointer to an array of pointers of type DMR_ATTR_ENTRY
**              .attr_number
**              .attr_operation
**              .attr_value_ptr
**
** Outputs:
**      qeu_cb
**          .qeu_count                      number of tuples retrieved
**          .error.err_code                 One of the following
**                                          E_QE0000_OK
**                                          E_QE0002_INTERNAL_ERROR
**                                          E_QE0004_NO_TRANSACTION
**                                          E_QE0007_NO_CURSOR
**                                          E_QE0017_BAD_CB
**                                          E_QE0018_BAD_PARAM_IN_CB
**                                          E_QE0015_NO_MORE_ROWS
**                                          E_QE0022_ABORTED
**					    E_QE0015_NO_MORE_ROWS
**          .qeu_output                     output buffer
**      Returns:
**          E_DB_OK
**          E_DB_WARN
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          the output buffers are filled in
**
** History:
**      27-may-86 (daved)
**          written
**      31-jul-86 (jennifer)
**          Modified qeu_get to not allocate output
**          from memory stream, but to assume output
**          array is already initialized.
**      20-aug-86 (jennifer)
**          When dmf returns nonext, then qef must
**          return no-more-rows.
**	10-oct-87 (puree)
**	    Same as above even during positioning.
**	04-jan-90 (andre)
**	    return tid of the last tuple which was successfully read in
**	    QU_CB->qeu_tid
**	17-jun-92 (andre)
**	    add support for retrieval by TID - qeu_gentext should NOT be set to
**	    QEU_REPO
**	27-jul-96 (ramra01)
**	    Alter table project: Position only flag on qeu_get.
**	11-Apr-2008 (kschendel)
**	    Revised DMF qualification requirements, simplified for QEU.
*/
DB_STATUS
qeu_get(
QEF_CB          *qef_cb,
QEU_CB          *qeu_cb)
{
    i4     err;
    DMR_CB      dmr_cb;
    i4     count;          /* number of tuples returned */
    i4		i;
    DB_STATUS   status;
    PTR         tuple;
    QEF_DATA    *dataptr;

    count = qeu_cb->qeu_count;		/* number of rows to get */
    qeu_cb->qeu_count = 0;

    /* QEU_GET is only valid in a transaction */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
        qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
            &qeu_cb->error, 0);
        return (E_DB_ERROR);
    }
    dmr_cb.type                 = DMR_RECORD_CB;
    dmr_cb.length               = sizeof(DMR_CB);
    dmr_cb.dmr_access_id        = qeu_cb->qeu_acc_id;
    dmr_cb.dmr_tid              = 0;                    /* not used */
    dmr_cb.dmr_q_fcn		= NULL;
    /* if positioning the cursor */
    if (qeu_cb->qeu_getnext == QEU_REPO)
    {
        /* if there is a key, position by qual. Else, position all records */
        if (qeu_cb->qeu_klen == 0)
            dmr_cb.dmr_position_type = DMR_ALL;
        else
        {
            /* set the keys */
            dmr_cb.dmr_position_type = DMR_QUAL;
	    dmr_cb.dmr_attr_desc.ptr_address    = (PTR) qeu_cb->qeu_key;
	    dmr_cb.dmr_attr_desc.ptr_in_count   = qeu_cb->qeu_klen;
	    dmr_cb.dmr_attr_desc.ptr_size	= sizeof (DMR_ATTR_ENTRY);
	    dmr_cb.dmr_s_estimated_records	= -1;
        }
        /* row qualifier */
        dmr_cb.dmr_q_fcn = (DB_STATUS (*)(void *,void *)) qeu_cb->qeu_qual;
        dmr_cb.dmr_q_arg = (PTR) qeu_cb->qeu_qarg;
	if (qeu_cb->qeu_qual != NULL)
	{
	    dmr_cb.dmr_q_rowaddr = &qeu_cb->qeu_qarg->qeu_rowaddr;
	    dmr_cb.dmr_q_retval = &qeu_cb->qeu_qarg->qeu_retval;
	}
        dmr_cb.dmr_flags_mask = 0;
        status = dmf_call(DMR_POSITION, &dmr_cb);
        if (status != E_DB_OK)
        {
            if (dmr_cb.error.err_code == E_DM0055_NONEXT)
                qeu_cb->error.err_code = E_QE0015_NO_MORE_ROWS;
            else
            {
                qef_error(dmr_cb.error.err_code, 0L, status, &err, 
		    &qeu_cb->error, 0);
            }
            return (status);
        }
    }

    if (qeu_cb->qeu_flag & QEU_POSITION_ONLY)
       return (E_DB_OK);

    /* We can now start getting records */
    /* point to space for the first tuple */
    /* the tuple length will not change. Tell DMF about it */
    dmr_cb.dmr_data.data_in_size = qeu_cb->qeu_tup_length;

    /* assign the beginning of the output chain */
    dataptr = qeu_cb->qeu_output;
    tuple = dataptr->dt_data;
    i = 0;

    /*
    ** if qeu_getnexdt was not set to QEU_REPO and QEU_BY_TID was set in
    ** qeu_flag, tell dmr_get() to get a tuple with specified TID; otherwise
    ** tell it to get the next row
    */
    if (qeu_cb->qeu_getnext != QEU_REPO && qeu_cb->qeu_flag & QEU_BY_TID)
    {
	dmr_cb.dmr_flags_mask = DMR_BY_TID;
	dmr_cb.dmr_tid = qeu_cb->qeu_tid;
    }
    else
    {
	dmr_cb.dmr_flags_mask = DMR_NEXT;
    }

    for (;;)
    {
        /* get the tuple */
        dmr_cb.dmr_data.data_address = tuple;
        status = dmf_call(DMR_GET, &dmr_cb);
        if (status != E_DB_OK)
        {
            if (dmr_cb.error.err_code == E_DM0055_NONEXT)
	    {
		qeu_cb->error.err_code = E_QE0015_NO_MORE_ROWS;
                break;
	    }
            else
            {
                qef_error(dmr_cb.error.err_code, 0L, status, &err, 
		    &qeu_cb->error, 0);
                return (status);
            }
        }

	/* save TID of the last tuple read */
	qeu_cb->qeu_tid = dmr_cb.dmr_tid;

        /*
	** get space for next tuple - if retrieving by TID we never want to get
	** more than one tuple
	*/
        if (++i < count && dmr_cb.dmr_flags_mask != DMR_BY_TID)
        {
            /* move to next entry in the output chain */
            dataptr = dataptr->dt_next;
            tuple = dataptr->dt_data;
        }
        else
            break;
    }
    /* assign output value */
    qeu_cb->qeu_count = i;
    return (status);
}

/*{
** Name: QEU_OPEN       - open a table
**
** External QEF call:   status = qef_call(QEU_OPEN, &qeu_cb);
**
** Description:
**      A table is opened. A transaction must be in progress when
** this routine is called. This routine must be called before any
** of qeu_delete, qeu_append or qeu_get is called.
** Tables are opened without DMT_NOWAIT being set and are opened for
** direct update.
**
** Inputs:
**       qeu_cb
**          .qeu_flag                       zero or QEU_SHOW_STAT
**                                          indicates if internal request for 
**                                          statistics.
**	    .qeu_eflag			    designate error handling semantis
**					    for user errors.
**		QEF_INTERNAL		    return error code.
**		QEF_EXTERNAL		    send message to user.
**          .qeu_tab_id                     table id
**          .qeu_db_id                      database id
**          .qeu_lk_mode                    lock mode
**              DMT_X                       exclusive table lock
**              DMT_S                       shared table lock
**              DMT_IX                      exclusive page lock
**              DMT_IS                      shared table lock
**              DMT_SIX                     shared table lock with ability
**                                          to exclusive lock pages
**          .qeu_access_mode                access mode
**              DMT_A_READ
**              DMT_A_WRITE
**
** Outputs:
**      qeu_cb
**          .qeu_acc_id                     table access id
**          .error.err_code                 One of the following
**                                          E_QE0000_OK
**                                          E_QE0002_INTERNAL_ERROR
**                                          E_QE0004_NO_TRANSACTION
**                                          E_QE0017_BAD_CB
**                                          E_QE0018_BAD_PARAM_IN_CB
**                                          E_QE0022_ABORTED
**                                          <DMF error messages>
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-may-86 (daved)
**          written
**      18-jul-89 (jennifer)
**          Added a flag to indicate that an internal request 
**          for statistical information is being requested.
**          This solves a B1 security problem where statistics
**          could be unpredicatable if B1 labels were checked.
**          For an internal request they aren't checked, for a 
**          normal query on the iihistogram and iistatistics 
**          table they are checked.
**	23-jan-91 (seputis)
**	    added support for timeout and maxlocks
**	25-mar-99 (shust01)
**	    Initialized dmt_mustlock.  On rare occasions storage happened
**	    to be equal to a 1 (TRUE), causing locks to be taken out even
**	    though readlock=nolock was set.
**	07-mar-2000 (gupsh01)
**	    Do not handle error E_DM006A_TRAN_ACCESS_CONFLICT here as it will
**	    be handled by the calling facility. (BUG 100777).
**	14-feb-03 (inkdo01)
**	    Special check for non-existant iisequence catalog, so message
**	    can be issued that database doesn't support sequences.
**	    
*/
DB_STATUS
qeu_open(
QEF_CB          *qef_cb,
QEU_CB         *qeu_cb)
{
    DMT_CB      dmt_cb;
    DB_STATUS   status;
    i4     err;
    DMT_CHAR_ENTRY  lockmode[2];
    i4		lockcount;

    MEfill(sizeof(DMT_CB), 0, (PTR) &dmt_cb);
    dmt_cb.type         = DMT_TABLE_CB;
    dmt_cb.length       = sizeof(DMT_CB);
    dmt_cb.dmt_db_id    = qeu_cb->qeu_db_id;
    STRUCT_ASSIGN_MACRO(qeu_cb->qeu_tab_id, dmt_cb.dmt_id);
    dmt_cb.dmt_flags_mask  = 0;
    if (qeu_cb->qeu_flag & QEU_SHOW_STAT)
	dmt_cb.dmt_flags_mask |= DMT_SHOW_STAT;
    if (qeu_cb->qeu_flag & QEU_BYPASS_PRIV)
	dmt_cb.dmt_flags_mask |= DMT_DBMS_REQUEST;
    dmt_cb.dmt_lock_mode   = qeu_cb->qeu_lk_mode;
    dmt_cb.dmt_update_mode = DMT_U_DIRECT;
    dmt_cb.dmt_mustlock    = FALSE;
    dmt_cb.dmt_access_mode = qeu_cb->qeu_access_mode;
    lockcount = 0;
    if (qeu_cb->qeu_mask & QEU_TIMEOUT)
    {
	lockmode[lockcount].char_id = DMT_C_TIMEOUT_LOCK;
	lockmode[lockcount].char_value = qeu_cb->qeu_timeout;
	lockcount++;
    }
    if (qeu_cb->qeu_mask & QEU_MAXLOCKS)
    {
	lockmode[lockcount].char_id = DMT_C_PG_LOCKS_MAX;
	lockmode[lockcount].char_value = qeu_cb->qeu_maxlocks;
	lockcount++;
    }
    if (lockcount > 0)
    {
	dmt_cb.dmt_char_array.data_address = (PTR)lockmode;
	dmt_cb.dmt_char_array.data_in_size = sizeof(lockmode[0]) * lockcount;
	dmt_cb.dmt_char_array.data_out_size = 0;
    }
    else
	dmt_cb.dmt_char_array.data_in_size = 0;

    dmt_cb.dmt_sequence	   = qef_cb->qef_stmt;

    /* QEU_OPEN is only valid in a transaction */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
        qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
            &qeu_cb->error, 0);
        return (E_DB_ERROR);
    }
    dmt_cb.dmt_tran_id = qef_cb->qef_dmt_id;
    status = dmf_call(DMT_OPEN, &dmt_cb);
    if (status == E_DB_OK)
    {
        qeu_cb->qeu_acc_id = dmt_cb.dmt_record_access_id;
        qef_cb->qef_open_count++;
    }
    else 
    {
	/* 
	** Do not handle the dmf error E_DM006A_TRAN_ACCESS_CONFLICT here  
	** as it is being handled by the calling facility BUG: 100777 
	**/
	if (dmt_cb.error.err_code == E_DM006A_TRAN_ACCESS_CONFLICT)
	{
	    qeu_cb->error.err_code = dmt_cb.error.err_code;
	    qeu_cb->error.err_data = dmt_cb.error.err_data;
	}
	else if (dmt_cb.error.err_code == E_DM0054_NONEXISTENT_TABLE &&
	    qeu_cb->qeu_tab_id.db_tab_base == DM_B_SEQ_TAB_ID)
         qef_error(E_QE00A1_IISEQ_NOTDEFINED, 0L, status, &err, 
						&qeu_cb->error, 0);
	else qef_error(dmt_cb.error.err_code, 0L, status, &err, 
						&qeu_cb->error, 0);
    }
    return (status);
}

/*{
** Name: QEU_REPLACE     - replace tuple(s) in a table
**
** External QEF call:   status = qef_call(QEU_REPLACE, &qeu_cb);
**
** Description:
**      A tuple is replaced in a table opened with the QEU_OPEN
** command.
** If QEU_BY_TID is specified,
**	the tuple with specified TID is repalced
** else if qeu_cb->kqeu_klen > 0
**	qeu_f_qual points to a function which will both determine whether the
**	tuple should be updated and make appropriate changes to the tuple
**      passed to it
** else
**	replace the tuple which was read by position as specified by the caller
**
** Inputs:
**       qeu_cb
**	    .qeu_eflag			    designate error handling semantis
**					    for user errors.
**		QEF_INTERNAL		    return error code.
**		QEF_EXTERNAL		    send message to user.
**          .qeu_acc_id                     table access id
**          .qeu_tup_length                 lenth of a tuple
**          .qeu_input                      buffer containing new values for the
**					    tuple being replaced
**	    .qeu_qual			    qualification function for DMF
**	    .qeu_qarg			    argument to (*qeu_qual) ()
**          .qeu_klen                       length of key 
**          .qeu_key                        key for update
**      <qeu_key> is a pointer to an array of pointers of type DMR_ATTR_ENTRY
**              .attr_number
**              .attr_operation
**              .attr_value_ptr
**	    .qeu_flag			    operation qualifier
**		QEU_BY_TID		    replace tuple whose TID is in
**					    qeu_tid
**	    .qeu_tid			    contains TID of the tuple to be
**					    replaced if (qeu_flag & QEU_BY_TID)
**	    .qeu_f_qual			    if !(qeu_klen > 0) this must point
**					    at a function which will decide
**					    whether and how the current tuple
**					    must be replaced
**	    .qeu_f_qarg			    argument to (*qeu_f_qual) ()
**	    
** Outputs:
**      qeu_cb
**	    .qeu_count			    number of tuples updated
**          .error.err_code                 One of the following
**                                          E_QE0000_OK
**                                          E_QE0002_INTERNAL_ERROR
**                                          E_QE0004_NO_TRANSACTION
**                                          E_QE0007_NO_CURSOR
**                                          E_QE0017_BAD_CB
**                                          E_QE0018_BAD_PARAM_IN_CB
**      Returns:
**          E_DB_OK
**          E_DB_ERROR                      caller error
**          E_DB_FATAL                      internal error
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      18-jun-92 (andre)
**          written
**	22-jun-92 (andre)
**	    changed the function to allow update one row by tid or to update
**	    multiple rows subject to qualification function
**	24-jun-92 (andre)
**	    another change to allow update of prepositioned tuple
**	13-sep-93 (robf)
**	    Extended to handle update multiple rows without
**	    requiring a key position. This is signalled when qeu_klen==0
**	    and qeu_qual != NULL and qeu_f_qual!=NULL
**	30-nov-93 (robf)
**          When updating multiple rows, keep going after first update.
**	24-Jan-2001 (jenjo02)
**	    Ensure that memory stream is closed before returning.
**	11-Apr-2008 (kschendel)
**	    Revised DMF qualification requirements, simplified for QEU.
*/
DB_STATUS
qeu_replace(
QEF_CB          *qef_cb,
QEU_CB          *qeu_cb)
{
    i4		    err = 0L;
    DMR_CB		    dmr_cb;
    DB_STATUS		    status;
    GLOBALREF QEF_S_CB	    *Qef_s_cb;
    ULM_RCB		    ulm;
    i4			    (*repl_func)(void *, void *);
    bool		    position_all=FALSE;
    bool		    mem_opened=FALSE;

    if (qeu_cb->qeu_flag & QEU_BY_TID)
    {
	/*
	** if update_by_tid, no key(s), qualification function, or qualification
	** function parameters should be passed;
	** qeu_cb->qeu_input must point at a structure containing a ptr to the
	** new value for the tuple of specified size
	*/
	if (qeu_cb->qeu_klen || qeu_cb->qeu_qual || qeu_cb->qeu_qarg ||
	    qeu_cb->qeu_f_qual || qeu_cb->qeu_f_qarg || !qeu_cb->qeu_input ||
	    !qeu_cb->qeu_input->dt_data || !qeu_cb->qeu_input->dt_size)
	{
	    err = E_QE0018_BAD_PARAM_IN_CB;
	    status = E_DB_ERROR;
	}
    }
    else if (qeu_cb->qeu_klen==0 &&  
	    (repl_func = qeu_cb->qeu_f_qual) != NULL &&
	    qeu_cb->qeu_qual)
    {
	position_all=TRUE;
	/*
	** Request to replace all qualifying in table, without key 
	** positioning
	*/
	if (!qeu_cb->qeu_qarg || 
	    !qeu_cb->qeu_f_qarg 
	    )
	{
	    err = E_QE0018_BAD_PARAM_IN_CB;
	    status = E_DB_ERROR;
	}
    }
    else if (qeu_cb->qeu_klen == 0)
    {
	/*
	** if asked to update the current tuple, no qualification function, or
	** qualification function parameters should be passed;
	** qeu_cb->qeu_input must point at a structure containing a ptr to
	** the new value for the tuple of specified size
	*/
	if (qeu_cb->qeu_qual || qeu_cb->qeu_qarg || qeu_cb->qeu_f_qual ||
	    qeu_cb->qeu_f_qarg || !qeu_cb->qeu_input ||
	    !qeu_cb->qeu_input->dt_data || !qeu_cb->qeu_input->dt_size)
	{
	    err = E_QE0018_BAD_PARAM_IN_CB;
	    status = E_DB_ERROR;
	}
    }
    else if ((repl_func = qeu_cb->qeu_f_qual) == NULL)
    {
	/*
	** if caller supplied a key, must specify the function which
	** will determine whether and how the tuple must be updated
	*/
	err = E_QE0018_BAD_PARAM_IN_CB;
	status = E_DB_ERROR;
    }

    if (err)
    {
	(VOID) qef_error(err, 0L, status, &err, &qeu_cb->error, 0);
	return(status);
    }

    qeu_cb->qeu_count = 0;
    qeu_cb->error.err_code = E_QE0000_OK;

    /* QEU_REPLACE is only valid in a transaction */
    if (qef_cb->qef_stat == QEF_NOTRAN)
    {
        qef_error(E_QE0004_NO_TRANSACTION, 0L, E_DB_ERROR, &err,
            &qeu_cb->error, 0);
        return (E_DB_ERROR);
    }

    dmr_cb.type                 = DMR_RECORD_CB;
    dmr_cb.length               = sizeof(DMR_CB);
    dmr_cb.dmr_access_id        = qeu_cb->qeu_acc_id;
    dmr_cb.dmr_q_fcn		= NULL;

    /* if replacing by key, position the cursor */
    if (qeu_cb->qeu_klen || position_all)
    {
	/* allocate memory for a tuple only if planning to call qeu_get() */
	
	STRUCT_ASSIGN_MACRO(Qef_s_cb->qef_d_ulmcb, ulm);
	/* Open stream and allocate tuple memory in one action */
	ulm.ulm_flags = ULM_PRIVATE_STREAM | ULM_OPEN_AND_PALLOC;
	ulm.ulm_psize = ulm.ulm_blocksize = qeu_cb->qeu_tup_length;

	if ((status = qec_mopen(&ulm)) != E_DB_OK)
	{
	    qef_error(E_QE001E_NO_MEM, 0L, E_DB_ERROR, &err,
		&qeu_cb->error, 0);
	    return (status);
	}
	mem_opened=TRUE;

        /* set the keys */
	if(position_all)
	{
		dmr_cb.dmr_position_type = DMR_ALL;
	}
	else
	{
		dmr_cb.dmr_position_type = DMR_QUAL;
		dmr_cb.dmr_attr_desc.ptr_address    = (PTR) qeu_cb->qeu_key;
		dmr_cb.dmr_attr_desc.ptr_in_count   = qeu_cb->qeu_klen;
		dmr_cb.dmr_attr_desc.ptr_size	    = sizeof (DMR_ATTR_ENTRY);
		dmr_cb.dmr_s_estimated_records	    = -1;
	}
        /* row qualifier */
        dmr_cb.dmr_q_fcn = (DB_STATUS (*)(void *,void *)) qeu_cb->qeu_qual;
        dmr_cb.dmr_q_arg = (PTR) qeu_cb->qeu_qarg;
	if (qeu_cb->qeu_qual != NULL)
	{
	    dmr_cb.dmr_q_rowaddr = &qeu_cb->qeu_qarg->qeu_rowaddr;
	    dmr_cb.dmr_q_retval = &qeu_cb->qeu_qarg->qeu_retval;
	}
	dmr_cb.dmr_flags_mask = 0;
        status = dmf_call(DMR_POSITION, &dmr_cb);
        if (status != E_DB_OK)
        {
	    ulm_closestream(&ulm);

	    if (dmr_cb.error.err_code == E_DM0055_NONEXT)
		return (E_DB_OK);
            qef_error(dmr_cb.error.err_code, 0L, status, &err, &qeu_cb->error, 0);
            return (status);
        }

	/* the tuple length will not change. Tell DMF about it */
	dmr_cb.dmr_data.data_in_size = qeu_cb->qeu_tup_length;
	dmr_cb.dmr_data.data_address = ulm.ulm_pptr;
    }
    else
    {
	/*
	** make dmr_data point at the new value for the tuple
	*/
	dmr_cb.dmr_data.data_address = qeu_cb->qeu_input->dt_data;
	dmr_cb.dmr_data.data_in_size = qeu_cb->qeu_input->dt_size;

	/*
	** if (qeu_flag & QEU_BY_TID)
	**    indicate to dmr_replace() that we are replacing by tid
	** else
	**    indicate to dmr_replace to replace the current tuple
	*/

	if (qeu_cb->qeu_flag & QEU_BY_TID)
	{	
	    dmr_cb.dmr_flags_mask = DMR_BY_TID;
	    dmr_cb.dmr_tid = qeu_cb->qeu_tid;
	}
	else
	{
	    dmr_cb.dmr_flags_mask = DMR_CURRENT_POS;
	}
    }

    for (;;)
    {
        if (qeu_cb->qeu_klen || position_all)
	{
	    i4	ret_val;

            /* get the tuple */
            dmr_cb.dmr_flags_mask = DMR_NEXT;
            status = dmf_call(DMR_GET, &dmr_cb);
            if (status != E_DB_OK)
            {
                if (dmr_cb.error.err_code == E_DM0055_NONEXT)
		{
		    status = E_DB_OK;
		}
		else
		{
		    err = dmr_cb.error.err_code;
		}
                break;
            }

	    /*
	    ** let caller's function determine whether and how the tuple must be
	    ** changed; QEU_F_RETURN indicates that we must proceed; QEU_F_NEXT
	    ** says "skip this tuple"
	    */
	    ret_val = (*repl_func) (dmr_cb.dmr_data.data_address,
		qeu_cb->qeu_f_qarg);

	    if (ret_val != QEU_F_RETURN)
	    {
		continue;
	    }

	    /* will replace the current tuple */
	    dmr_cb.dmr_flags_mask = DMR_CURRENT_POS;
	}

	dmr_cb.dmr_attset = NULL;
	status = dmf_call(DMR_REPLACE, &dmr_cb);
	if (status != E_DB_OK)
	{
	    err = dmr_cb.error.err_code;
	    break;
	}
	else
	{
	    qeu_cb->qeu_count++;
	}

	if (qeu_cb->qeu_klen == 0 && !position_all)
	{
	    /* we were replacing a tuple by tid and are done */
	    break;
	}
    }

    if (status != E_DB_OK)
    {
	qef_error(err, 0L, status, &err, &qeu_cb->error, 0);
    }

    if (mem_opened)
    {
	/* don't try to close a stream unless it was opened */
	ulm_closestream(&ulm);
    }
    
    return (status);
}
