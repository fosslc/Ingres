/*
**Copyright (c) 2004 Ingres Corporation
*/

# include    <compat.h>
# include    <gl.h>
# include    <sl.h>
# include    <cs.h>
# include    <me.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <pc.h>
# include    <tm.h>
# include    <st.h>
# include    <lk.h>
# include    <sa.h>
# include    <ulf.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    "sxapoint.h"

/*
** Name: SXAPOR.C - Record access routines for the SXAPO auditing system.
**
** Description:
**	This file contains all of the record access routines used by the 
**	SXF low level operating system version of the auditing system, SXAPO
**
**	sxapo_position - position a record stream for reading
**	sxapo_read - read an audit record from the record stream
**	sxapo_write - write an audit record to the OS audit trail
**	sxapo_flush - flush audit records to the OS auditor
**
** History:
**	6-dec-93 (stephenb)
**	    First written.
**	11-feb-94 (stephenb)
**	    Update SA calls to current SA spec and add functionality to flush
**	    after 100 writes.
**	28-feb-94 (stephenb)
**	    Clean up additional text field writes in sxapo_write.
**	31-may-94 (stephenb)
**	    Include me.h for a clean build on VMS
**	04-Aug-1997 (jenjo02)
**	    Removed sxf_incr(). It's gross overkill to semaphore-protect a
**	    simple statistic.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	13-May-2009 (kschendel) b122041
**	    Compiler warning fixes.
*/

/*
** Name: sxapo_position
**
** Description - position an audit logfile for reading
**	This routine positions an audit logfile for reading, it is included as
**	a no-op routine because reading the audit logs is not a supported
**	action for the opoerating system implimentation of the auditing system.
**
**	The SXA may attempt to call this routine when the user performs
**	unsupported actions, so the routine will always return a bad status
**	and error code in order to warn the user of the problem.
**
** Inputs:
**	scb		SXF session control block
**	rscb		the SXF_RSCB for this audit file
**	pos_type	the type of position operation to be performed.
**	pos_key		the key to use for the position operation.
**
** Outputs:
**	err_code	Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	6-dec-93 (stephenb)
**	    Initial creation.
*/
DB_STATUS
sxapo_position(
    SXF_SCB	*scb,
    SXF_RSCB	*rscb,
    i4		pos_type,
    PTR		pos_key,
    i4	*err_code)
{
    /*
    ** This action is not supported, so return an error and bad status.
    */
    *err_code = E_SX002D_OSAUDIT_NO_POSITION;
    return(E_DB_ERROR);
}

/*
** Name: sxapo_read - read the next record from the audit file.
**
** Description:
**	This routine should get the next record from the audit file, since
**	reading of the operating system audit files will not be supported
**	this routine will simply return an error and a bad status.
**
** Inputs:
**	scb		SXF session control block.
**	rscb		The SXF_RSCB for the audit file.
**
** Outputs:
**	audit_rec	The audit record to be returned to the caller
**	err_code	Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	22-dec-93 (stephenb)
**	    Initial creation.
*/
DB_STATUS
sxapo_read(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code)
{
    *err_code = E_SX002E_OSAUDIT_NO_READ;
    return(E_DB_ERROR);
}

/*
** Name: sxapo_write - write a record to the operating system audit trail
**
** Description:
**	This routine is used to write an audit record to the operating system
**	audit trail, it will use the CL routine SAwrite() to perform the
**	operating system specific tasks. The audits may be buffered by SAwrite()
**	and a subsequent call to sxapo_flush will ensure that they are flushed
**	to the operating system for auditing.
** Inputs:
**	scb		SXF session control block
**	rscb		Record stream control block for current audit trail
**	audit_rec	the audit record to be written to the audit trail
**
** Outputs:
**	err_code	Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	22-dec-93 (stephenb)
**	    Initial creation.
**	11-feb-94 (stephenb)
**	    Update SA call to current spec and add functionality to flush
**	    after 100 writes.
**	28-feb-94 (stephenb)
**	    Clean up writes of additional text field by using the given text
**	    length.
**	3-apr-94 (robf)
**          Use SXAPO_SCB for consistency with SXAP
*/
DB_STATUS
sxapo_write(
    SXF_SCB		*scb,
    SXF_RSCB		*rscb,
    SXF_AUDIT_REC	*audit_rec,
    i4		*err_code)
{
    SA_AUD_REC		sa_aud_rec;
    i4		msgid;
    DB_STATUS		status;
    DB_DATA_VALUE	msg_text;
    SXAPO_RSCB		*sxapo_rscb = (SXAPO_RSCB *)rscb->sxf_sxap;
    CL_ERR_DESC		cl_err;
    STATUS		cl_stat;
    i4		local_err;
    SXAPO_SCB		*sxapo_scb = (SXAPO_SCB*)scb->sxf_sxap_scb;
    SXAPO_RECID		*scb_last_rec = &sxapo_scb->sxapo_recid;

    /* to store converted text */

    char		evtype[SA_MAX_EVENTLEN]; /* event type */
    char		*bad_event = "BAD EVENT TYPE";
    char		message[SA_MAX_MESSTXT];  /* message text */
    char		*bad_msg = "BAD MESSAGE ID";
    char		user_priv[SA_MAX_PRIVLEN]; /* user privileges */
    char		object_priv[SA_MAX_PRIVLEN]; /* object privileges */
    char		acc_type[SA_MAX_ACCLEN]; /* access type */
    char		*bad_access = "BAD ACCESS TYPE";
    char		detail_text[257];

    /* initialize vars (zero fill)*/

    MEfill(SA_MAX_EVENTLEN, 0, evtype);
    MEfill(SA_MAX_MESSTXT, 0, message);
    MEfill(SA_MAX_PRIVLEN, 0, user_priv);
    MEfill(SA_MAX_PRIVLEN, 0, object_priv);
    MEfill(SA_MAX_ACCLEN, 0, acc_type);

    *err_code = E_SX0000_OK;
    for(;;)
    {
	/*
	** Reformat the audit record so that it fits into SA_AUD_REC and then
	** make a call to SAwrite() to write the record out
	*/

	/*
	** System time is NULL for writes
	*/
	sa_aud_rec.sa_evtime = NULL;

	/* 
	** the event type 
	*/
	msgid = sxapo_evtype_to_msgid( audit_rec->sxf_eventtype );
	msg_text.db_data = evtype;
	msg_text.db_datatype = DB_CHA_TYPE;
	msg_text.db_length = SA_MAX_EVENTLEN;
	status = sxapo_msgid_to_desc(msgid, &msg_text);
	if (status != E_DB_OK)
	    sa_aud_rec.sa_eventtype = bad_event;
	else
	    sa_aud_rec.sa_eventtype = evtype;
	/*
	** user id's and dbname
	*/
	sa_aud_rec.sa_ruserid = audit_rec->sxf_ruserid.db_own_name;
	sa_aud_rec.sa_euserid = audit_rec->sxf_euserid.db_own_name;
	sa_aud_rec.sa_dbname = audit_rec->sxf_dbname.db_db_name;
	/*
	** message text
	*/
	msg_text.db_data = message;
	msg_text.db_length = SA_MAX_MESSTXT;
	status = sxapo_msgid_to_desc(audit_rec->sxf_messid, &msg_text);
	if (status != E_DB_OK)
	    sa_aud_rec.sa_messtxt = bad_msg;
	else
	    sa_aud_rec.sa_messtxt = message;

	/*
	** status of operation
	*/
	if (audit_rec->sxf_auditstatus == SUCCEED)
	    sa_aud_rec.sa_status = FALSE;
	else
	    sa_aud_rec.sa_status = TRUE;
	/*
	** user privs
	*/
	msg_text.db_data = user_priv;
	msg_text.db_length = SA_MAX_PRIVLEN;
	sxapo_priv_to_str(audit_rec->sxf_userpriv, &msg_text, FALSE);
	sa_aud_rec.sa_userpriv = user_priv;
	/*
	** Object privs
	*/
	msg_text.db_data = object_priv;
	sxapo_priv_to_str(audit_rec->sxf_objpriv, &msg_text, FALSE);
	sa_aud_rec.sa_objpriv = object_priv;
	/*
	** Access type
	*/
	msgid = sxapo_acctype_to_msgid( audit_rec->sxf_accesstype );
	msg_text.db_data = acc_type;
	msg_text.db_length = SA_MAX_ACCLEN;
	status = sxapo_msgid_to_desc(msgid, &msg_text);
	if (status != E_DB_OK)
	    sa_aud_rec.sa_accesstype = bad_access;
	else
	    sa_aud_rec.sa_accesstype = acc_type;
	/*
	** object owner
	*/
	sa_aud_rec.sa_objowner = audit_rec->sxf_objectowner.db_own_name;
	/*
	** object name
	*/
	sa_aud_rec.sa_objname = audit_rec->sxf_object;
	/*
	** optional detail fields
	*/
	if (audit_rec->sxf_detail_txt)
	{
	    MEcopy(audit_rec->sxf_detail_txt, audit_rec->sxf_detail_len, 
		detail_text);
	    detail_text[audit_rec->sxf_detail_len] = '\0';
	    sa_aud_rec.sa_detail_txt = detail_text;
	}
	else
	{
	    sa_aud_rec.sa_detail_txt = NULL;
	}
	sa_aud_rec.sa_detail_int = audit_rec->sxf_detail_int;
	/*
	** Session id
	*/
	sa_aud_rec.sa_sess_id = audit_rec->sxf_sessid;
	/*
	** Now call SA to write the record
	*/
	cl_stat = SAwrite(&sxapo_rscb->sxapo_desc, &sa_aud_rec, &cl_err);
	/*
	** handle any errors
	*/
	if (cl_stat != OK)
	{
	    *err_code = E_SX10B4_OSAUDIT_WRITE_FAILED;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, NULL, NULL,
		0L, NULL, &local_err, 0);
	    /*
	    ** Failure to write an audit record is severe
	    */
	    status = E_DB_SEVERE;
	}
	else
	{
	    /* A record has been successfully written, incriment counts */
	    scb_last_rec->recno++;
	    Sxapo_cb->sxapo_writes++;
	}
	/*
	** Flush after 100 writes
	*/
	if (Sxapo_cb->sxapo_writes >= 100)
	{
	    status = sxapo_flush(scb, err_code);
	    if (status != E_DB_OK)
		break;
	}
	break;
    }
    return(status);
}

/*
** Name: sxapo_flush - flush audit records for this session to OS audit trail
**
** Description:
**	This routine is used to flush any audit records that may be buffered
**	by SA for this session to the OS audit trail. Unlike sxap, sxapo does
**	not buffer audit records it simply passes them to SA where they may or
**	may not be buffered.
**
**	Overview of alorythmn:-
**
**	If (last record written to audit trail is prior to
**	    last record written by session)
**		call SAflush()
**
** Inputs:
**	scb		SXF session control block.
**
** Outputs:
**	err_code
**
** Returns:
**	DB_STATUS
**
** History:
**	6-jan-94 (stephenb)
**	    Initial creation.
**	11-feb-94 (stephenb)
**	    Update SA call to current spec and add functionality to flush
**	    after 100 writes.
**       3-apr-94 (robf)
**	    Updated to use SXAPO_SCB
**      28-Jun-2001 (hweho01)
**          Added the missing argument num_parms to ule_format() call.
*/
DB_STATUS
sxapo_flush(
    SXF_SCB		*scb,
    i4		*err_code)
{
    SXAPO_SCB		*sxapo_scb = (SXAPO_SCB*)scb->sxf_sxap_scb;
    SXAPO_RECID		*scb_last_rec = &sxapo_scb->sxapo_recid;
    CL_ERR_DESC		cl_err;
    STATUS		cl_stat;
    i4		local_err;
    DB_STATUS		status = E_DB_OK;

    *err_code = E_SX0000_OK;
    /*
    ** Incriment flush count for stats
    */
    Sxf_svcb->sxf_stats->flush_count++;
    for (;;)
    {
	/*
	** If there are no records written since last flush then we have
	** nothing to do
	*/
	if (scb_last_rec->recno == 0)
	    break;
	/*
	** Otherwise call SAflush() to flush the buffer, and re-set recno
	*/
	cl_stat = SAflush(&Sxapo_cb->sxapo_curr_rscb->sxapo_desc, &cl_err);
	scb_last_rec->recno = 0;
	/*
	** Handle any errors
	*/
	if (cl_stat != OK)
	{
	    *err_code = E_SX10B5_OSAUDIT_FLUSH_FAILED;
	    _VOID_ ule_format(*err_code, &cl_err, ULE_LOG, NULL, NULL, 0L,
		&local_err, 0, 0);
	    status = E_DB_ERROR;
	}
	else
	{
	    /*
	    ** Successful flush, reset write count
	    */
	    Sxapo_cb->sxapo_writes--;
	}
	break;
    }
    return (status);
}
