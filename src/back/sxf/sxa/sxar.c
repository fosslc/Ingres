/*
**Copyright (c) 2004 Ingres Corporation
*/
 
# include    <compat.h>
# include    <gl.h>
# include    <pc.h>
# include    <iicommon.h>
# include    <dbdbms.h>
# include    <cs.h>
# include    <lk.h>
# include    <cv.h>
# include    <tm.h>
# include    <st.h>
# include    <me.h>
# include    <ulf.h>
# include    <scf.h>
# include    <sxf.h>
# include    <sxfint.h>
# include    <sxap.h>
 
/*
** Name: SXAR.C - SXA record management routines
**
** Description:
**      This file contains all of the audit record management routines
**      used by the SXF auditing system, SXA. The routines contained in 
**	this file are:-
**
**          sxar_position - position a record stream for reading
**          sxar_read 	  - read the next audit record from the stream.
**          sxar_write    - write an audit record to the current audit file.
**          sxar_flush    - flush any buffered audit records to disk.
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation.
**	10-dec-1992 (markg)
**	    Updated sxar_write to handle changes in effective user name in
**	    mid-session.
**	3-feb-1993 (markg)
**	    Included additional validation of parameters passed to
**	    sxar_write().
**	10-may-1993 (markg)
**	    Fixed a bug in sxar_write() which caused the audit_all user
**	    bit to get ignored sometimes.
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-mar-94 (stephenb)
**	    Add option to use dbname passed in rcb when writing an audit in
**	    sxar_write().
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-Jul-2003 (ashco01) Bug#110465
**          Real and effective user of audited event was being logged as 
**          NULLs for dmfjsp operations on the iidbdb. Changed sxar_write() 
**          to detect this and log operation to $ingres.
**      13-Oct-2003 (ashco01) Problem INGSRV#2548 (Bug: 111079)
**          Security audit log fields 'sessionid' and 'objectowner' contain
**          trailing spaces when iidbdb operations are logged. sxar_write()
**          amended to handle these stings correctly.           
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/

/*
** Name: SXAR_POSITION - position a record stream for reading
**
** Description:
**	This routine positions an audit record stream for reading. The 
**	record stream must have been created by a call to sxaf_open. All 
**	record streams must be positioned before they can be used for reading.
**
**	NOTE:
**	The current version of SXF does not have any audit file access
**	mechanisms that support keyed record access. For the moment all
**	sxar_position calls must specify the SXF_SCAN type.
**
** 	Overview of algorithm:-
**
**	Validate the contents of the SXF_RSCB.
**	Calls SXAP routine to perform low level position operation.
**	Mark the RSCB as being positioned.
**
**
** Inputs:
**	rcb.sxf.access_id	SXF audit file access identifier.
**
** Outputs:
**	rcb.sxf_error		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation.
*/
DB_STATUS
sxar_position(	
    SXF_RCB	*rcb)
{
    DB_STATUS	status = E_DB_OK;
    i4	err_code = E_SX0000_OK;
    i4	local_err;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_RSCB	*rscb = (SXF_RSCB *) rcb->sxf_access_id;
    SXF_SCB	*scb = rscb->sxf_scb_ptr;

    rcb->sxf_error.err_code = E_SX0000_OK;

    for (;;)
    {
        if ((sxf_status & (SXF_C2_SECURE)) == 0)
        {
            err_code = E_SX0010_SXA_NOT_ENABLED;
            break;
	}

	if ((rscb->sxf_rscb_status & SXRSCB_OPENED) == 0)
	{
	    err_code = E_SX0012_FILE_NOT_OPEN;
	    break;
	}

	status = (*Sxap_vector->sxap_pos)(
			scb, rscb, SXF_SCAN, NULL, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	rscb->sxf_rscb_status |= SXRSCB_POSITIONED;

	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX100F_SXA_POSITION;

	rcb->sxf_error.err_code = err_code;
	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}

/*
** Name: SXAR_READ - read the next record from a record stream.
**
** Description:
**	This routine reads the next audit record from a record stream and
**	returns it to the caller. The record stream must have been previously
**	created by a sxaf_open call. Also the record stream must also have 
**	been previously positionsed by a sxar_position call. If no more 
**	audit records exist in the record stream this routine will return
**	a status of E_DB_WARN and an error code of E_SX0007_NO_ROWS.
**
** 	Overview of functions performed in this routine:-
**
**	Validate the contents of the SXF_RSCB.
**	Call SXAP_VREAD routine to perform the physical read operation.
**
** Inputs:
**	scb			SXF session control block.
**	rscb			Record stream control block for record stream
**				to be read from. 
**	audit_rec		Audit record buffer.
**
** Outputs:
**	audit_rec		The audit record to be returned to the caller
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation.
*/
DB_STATUS
sxar_read(
    SXF_RCB	*rcb)
{
    DB_STATUS	status = E_DB_OK;
    i4	err_code = E_SX0000_OK;
    i4	local_err;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_RSCB	*rscb = (SXF_RSCB *) rcb->sxf_access_id;
    SXF_SCB	*scb = rscb->sxf_scb_ptr;

    rcb->sxf_error.err_code = E_SX0000_OK;

    for (;;)
    {
        if ((sxf_status & (SXF_C2_SECURE)) == 0)
        {
            err_code = E_SX0010_SXA_NOT_ENABLED;
            break;
	}

	if (rcb->sxf_record == NULL || rscb == NULL)
	{
	    err_code = E_SX0004_BAD_PARAMETER;
	    break;
	}

	if ((rscb->sxf_rscb_status & SXRSCB_POSITIONED) == 0)
	{
	    err_code = E_SX0013_FILE_NOT_POSITIONED;
	    break;
	}

	status = (*Sxap_vector->sxap_read)(
			scb, rscb, rcb->sxf_record, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    if (err_code > E_SXF_INTERNAL)
	        _VOID_ ule_format(err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	if (err_code > E_SXF_INTERNAL)
	    err_code = E_SX1010_SXA_READ;

	rcb->sxf_error.err_code = err_code;
	if (status == E_DB_OK)
	    status = E_DB_ERROR;	
    }

    return (status);
}

/*
** Name: SXAR_WRITE - write a record to the current audit file
**
** Description:
**	This routine is used to write an audit record to the current audit 
**	file. Audit records written using this rotine may be buffered before 
**	they are written to disk. A subsequent call to sxf_flush will 
**	ensure that the audit record has been safely written to disk.
**
** 	Overview of functions performed in this routine:-
**
**	Validate the contents of the RCB.
**	Locate the session control block for the caller.
**	Call sxas_check to check if this record is required to be written.
**	Refresh the user name and status fields if necessary.
**	Build the audit record from the data supplied in the RCB.
**	Call the SXAP routine to perform the physical write operation.
**	If the sxf_force flag is set, call the SXAP to flush the audit record.
**	If the effective userid may have changed, get new value from SCF.
**
** Inputs:
*	rcb
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation.
**	10-dec-1992 (markg)
**	    Updated to handle changes in effective user name in
**	    mid-session. Now if a request to audit a successful change
**	    in user name is received, we call SCF to get the new user name.
**	11-jan-1993 (robf)
**	    Added sxf_detail handling, plus comment about sec label/id when
**	    we handle this.
**	3-feb-1993 (markg)
**	    Included additional validation of parameters passed.
**	10-may-1993 (markg)
**	    Fixed a bug which caused the audit_all user bit to get 
**	    ignored sometimes.
**	7-jul-93 (robf)
**	     Initialize session id for audit record
**	3-aug-93 (robf)
**	     Move logfull processing to logical layer, check
**	     for stop/suspend errors when E_DB_WARN status on write
**	17-sep-93 (robf)
**	     Add basic tracing. 
**	30-dec-93 (robf)
**           Use passed session cb is passed in, otherwise look up as
**	     previously.
**	9-mar-94 (stephenb)
**	     Add option to use dbname passed in rcb rather than session dbname.
**	10-mar-1994 (robf)
**	     Move sxf_db_name into dbinfo structure, updated reference to it
**	 9-may-1994 (robf)
**	     Put back 10-mar-94 change, lost after integration.
**	27-may-94 (robf)
**           When doing an SXAP FLUSH warnings may be returned. Handle this
**	     by calling sxar_flush() (which has the necessary processing) rather
**	     than directly calling the SXAP flush routine.
**      22-Jul-2003 (ashco01) Bug#110465
**           Real and effective user of audited event was being logged as 
**           NULLs for dmfjsp operations on the iidbdb. Changed sxar_write() 
**           to detect this and log operation to $ingres.
**      13-Oct-2003 (ashco01) Problem INGSRV#2548 (Bug: 111079)
**          Security audit log fields 'sessionid' and 'objectowner' contain
**          trailing spaces when iidbdb operations are logged. sxar_write()
**          amended to handle these stings correctly. 	     
*/
DB_STATUS
sxar_write(
    SXF_RCB	*rcb)
{
    DB_STATUS		status = E_DB_OK;
    i4 		err_code = E_SX0000_OK;
    i4		local_err;
    SXF_SCB		*scb;
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[2];
    DB_OWN_NAME		user_name;
    i4			user_stat;
    SXF_AUDIT_REC	audit_rec;
    bool		audit_reqd;
    i4		sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_RSCB		*rscb = Sxf_svcb->sxf_write_rscb;
    SXF_ASCB		*ascb = Sxf_svcb->sxf_aud_state;

    rcb->sxf_error.err_code = E_SX0000_OK;

    for (;;)
    {
        if ((sxf_status & (SXF_C2_SECURE)) == 0)
        {
            break;
        }

	if (rcb->sxf_auditevent == 0 ||
	    rcb->sxf_accessmask == 0 ||
	    rcb->sxf_msg_id == 0 ||
	    rcb->sxf_objlength < 0 ||
	    (rcb->sxf_objlength > 0 && rcb->sxf_objname == NULL))
	{
	    err_code = E_SX0004_BAD_PARAMETER;
	    _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	if(rcb->sxf_scb!=NULL)
	{
	    /*
	    ** Passed SXF session id, so just use that,
	    ** with quick check for OK
	    */
	    scb= (SXF_SCB*)rcb->sxf_scb;
	    if(scb->sxf_cb_type!=SXFSCB_CB ||
	       scb->sxf_ascii_id!=SXFSCB_ASCII_ID)
	    {
	        err_code = E_SX0004_BAD_PARAMETER;
	        _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	}
	else
	{
	    /*
	    ** No SCB passed in so look it  up
	    */
	    status = sxau_get_scb(&scb, &local_err);
	    if (status != E_DB_OK)
	    {
	        err_code = local_err;
	        _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	        break;
	    }
	}

	/*
	** Because we have to start up SXF sessions before the 
	** user status and effective user names have been 
	** determined (see scf!scs!scsinit.c), the effective
	** user name and user status fields may be empty now.
	** If they are empty, try and get the information from
	** SCF.
	*/
	if ((Sxf_svcb->sxf_svcb_status & SXF_SNGL_USER) == 0 &&
	     STskipblank(scb->sxf_user.db_own_name, 
	     sizeof(DB_OWN_NAME)) == NULL)
	{
            scf_cb.scf_length = sizeof(SCF_CB);
            scf_cb.scf_type = SCF_CB_TYPE;
            scf_cb.scf_facility = DB_SXF_ID;
            scf_cb.scf_session = DB_NOSESSION;
            scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
            scf_cb.scf_len_union.scf_ilength = 2;
            sci_list[0].sci_length  = sizeof (i4);
            sci_list[0].sci_code    = SCI_USTAT;
            sci_list[0].sci_aresult = (char *) &user_stat;
            sci_list[0].sci_rlength = NULL;
            sci_list[1].sci_length  = sizeof (DB_OWN_NAME);
            sci_list[1].sci_code    = SCI_USERNAME;
            sci_list[1].sci_aresult = (char *) &user_name;
            sci_list[1].sci_rlength = NULL;
            status = scf_call(SCU_INFORMATION, &scf_cb);
	    if (status != E_DB_OK)
	    {
		err_code = scf_cb.scf_error.err_code;
	        _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		err_code  = E_SX1005_SESSION_INFO;
	        _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }

	    if (STlength(user_name.db_own_name) != 0)
	    {
		STRUCT_ASSIGN_MACRO(user_name, scb->sxf_user);
		scb->sxf_ustat = user_stat;
	    }
	}

	status = sxas_check(
		scb, 
		rcb->sxf_auditevent, 
		&audit_reqd, 
		&local_err);
	if (status != OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}
	if (audit_reqd == FALSE)
	    break;
	/*
	** If auditing is disabled break, we check this AFTER
	** sxas_check to allow restart processing  to operate
	*/
	if((sxf_status & SXF_AUDIT_DISABLED))
	{
		if(SXS_SESS_TRACE(SXS_TAUD_LOG))
			SXF_DISPLAY("SXA_WRITE - Auditing disabled so record not written\n");
		break;
	}
	/* 
	** Build the audit record and pass it to the
	** SXAP level to get written.
	*/
	audit_rec.sxf_eventtype  = rcb->sxf_auditevent;
	audit_rec.sxf_messid     = rcb->sxf_msg_id; 
	audit_rec.sxf_userpriv   = scb->sxf_ustat;
        /* dmfjsp operations on iidbdb supply null usernames, in this case
           record iidbdb owner as real and effective users */
        if (scb->sxf_ruser.db_own_name[0] != EOS)
        {
             STRUCT_ASSIGN_MACRO(scb->sxf_ruser, audit_rec.sxf_ruserid);
             STRUCT_ASSIGN_MACRO(scb->sxf_user, audit_rec.sxf_euserid);
        }
        else
        {
             /* real and effective users are iidbdb owner */
             STRUCT_ASSIGN_MACRO(*rcb->sxf_objowner, audit_rec.sxf_ruserid);
             STRUCT_ASSIGN_MACRO(*rcb->sxf_objowner, audit_rec.sxf_euserid); 
        }
	/*
	** If dbname has been passed then we'll use it, this is so that we
	** can have an SXF session in one database (iidbdb) and audit access
	** to another, this is done so that a single user session in the iidbdb
	** may audit access to another database (e.g. for ckpdb).
	*/
	if (rcb->sxf_db_name)
	{
	    STRUCT_ASSIGN_MACRO(*rcb->sxf_db_name, audit_rec.sxf_dbname);
	}
	else
	{
 	    STRUCT_ASSIGN_MACRO(scb->sxf_db_info->sxf_db_name, 
 		audit_rec.sxf_dbname);
	}

	MEmove(STlength(scb->sxf_sessid) , (PTR)scb->sxf_sessid, ' ', 
                SXF_SESSID_LEN, (PTR)&audit_rec.sxf_sessid);
	audit_rec.sxf_detail_txt = rcb->sxf_detail_txt;
	audit_rec.sxf_detail_int = rcb->sxf_detail_int;
	audit_rec.sxf_detail_len = rcb->sxf_detail_len;

	MEmove(rcb->sxf_objlength, (PTR)rcb->sxf_objname, ' ', 
	       DB_MAXNAME, (PTR)audit_rec.sxf_object);

	if (rcb->sxf_objowner != NULL)
	{
	    STRUCT_ASSIGN_MACRO(*rcb->sxf_objowner, audit_rec.sxf_objectowner);
	}
	else
	{
	    MEfill(DB_OWN_MAXNAME, ' ', (PTR) &audit_rec.sxf_objectowner);
	}

	TMnow(&audit_rec.sxf_tm);
	audit_rec.sxf_auditstatus = 
	    (rcb->sxf_accessmask & SXF_A_SUCCESS) ? SUCCEED : FAIL;
	audit_rec.sxf_objpriv =
	    (rcb->sxf_auditevent == SXF_E_USER) ? rcb->sxf_ustat : 0;
	audit_rec.sxf_accesstype = 
	    rcb->sxf_accessmask & ~(SXF_A_SUCCESS | SXF_A_FAIL);

	if(SXS_SESS_TRACE(SXS_TAUD_LOG))
	{
		char mesgid[20];
		CVlx(rcb->sxf_msg_id, mesgid);
		SXF_DISPLAY("=============  SXA_WRITE Writing audit record =======================\n");

		SXF_DISPLAY4("Event: %d Action: %d MesgId: I_SX%s Status: %s\n",
			(i4)rcb->sxf_auditevent,
			(i4)audit_rec.sxf_accesstype,
			mesgid,
			audit_rec.sxf_auditstatus==SUCCEED?"Success":"Fail");
		SXF_DISPLAY2("Object Name: '%t'\n",
			sizeof(audit_rec.sxf_object), &audit_rec.sxf_object);
	}
	status = (*Sxap_vector->sxap_write)(scb, rscb, &audit_rec, &local_err);
	if(status==E_DB_WARN)
	{
		/*
		** Some unusual condition
		*/
		if(local_err==E_SX102D_SXAP_STOP_AUDIT)
		{
			/*
			** Auditing should be stopped, so set stopped bit
			** and break out.
			*/
			if(SXS_SESS_TRACE(SXS_TAUD_LOG))
				SXF_DISPLAY("SXA_WRITE - Stop audit request detected\n");
			status=sxas_stop_audit(FALSE, &err_code);
			if(status!=E_DB_OK)
			{
				status=E_DB_FATAL;
				break;
			}
			break;
		}
		else if(local_err==E_SX10AE_SXAP_AUDIT_SUSPEND)
		{
			/*
			** Auditing should be suspended, so suspend audit
			** and continue.
			*/
			if(SXS_SESS_TRACE(SXS_TAUD_LOG))
				SXF_DISPLAY("SXA_WRITE - Suspend audit request detected\n");
			status=sxas_suspend_audit(FALSE, &local_err);
			if(status!=E_DB_OK)
			{
			    status=E_DB_ERROR;
			    err_code = local_err;
			    _VOID_ ule_format(err_code, NULL,
				    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
			    break;
			}
			else
			{
				/*
				** Now auditing is suspended, we loop
				** again to write, and so suspend ourselves
				*/
				continue;
			}
			break;
		}
		else
		{
		    /*
		    ** Some other unknown warning, log as an error
		    */
		    status=E_DB_ERROR;
		    err_code = local_err;
		    _VOID_ ule_format(err_code, NULL,
			    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
		    break;
		}
	}
	else if (status != E_DB_OK)
	{
	    err_code = local_err;
	    _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    break;
	}

	/* 
	** If the sxf_force flag is set, flush any buffered audit records.
	** Use sxar_flush() to handle any conditions returns from  SXAP
	*/
	if (rcb->sxf_force)
	{
	    status = sxar_flush(rcb);
	    if (status != E_DB_OK)
	    {
		err_code = local_err;
	        _VOID_ ule_format(err_code, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);
		break;
	    }
	}

	break;
    }

    /*
    ** If we have just processed a successful "CHANGE USER" operation
    ** the effective user id for this session may have changed. We 
    ** should get the new value from SCF now, and store it in the SCB.
    */
    if (err_code == E_SX0000_OK &&
       (rcb->sxf_accessmask & SXF_A_SETUSER) &&
       (rcb->sxf_accessmask & SXF_A_SUCCESS))
    {
        scf_cb.scf_length = sizeof(SCF_CB);
        scf_cb.scf_type = SCF_CB_TYPE;
        scf_cb.scf_facility = DB_SXF_ID;
        scf_cb.scf_session = DB_NOSESSION;
        scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
        scf_cb.scf_len_union.scf_ilength = 1;
        sci_list[0].sci_length  = sizeof (DB_OWN_NAME);
        sci_list[0].sci_code    = SCI_USERNAME;
        sci_list[0].sci_aresult = (char *) &user_name;
        sci_list[0].sci_rlength = NULL;
        status = scf_call(SCU_INFORMATION, &scf_cb);
	if (status != E_DB_OK)
	{
	    err_code = scf_cb.scf_error.err_code;
	    _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	    err_code  = E_SX1005_SESSION_INFO;
	    _VOID_ ule_format(err_code, NULL,
		    ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);
	}
	else
	{
	    STRUCT_ASSIGN_MACRO(user_name, scb->sxf_user);
	}
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
	_VOID_ ule_format(E_SX1011_SXA_WRITE, NULL, ULE_LOG, 
			NULL, NULL, 0L, NULL, &local_err, 0);

	status = sxac_audit_error();
	if (status != E_DB_OK) 
            rcb->sxf_error.err_code = err_code;
    }
 
    return (status);
}

/*
** Name: SXAR_FLUSH - flush all audit records for this session to disk
**
** Description:
**	This routine is used to flush any buffered audit records previously
**	written by this session to disk. It will not complete until all the
**	relevant audit records are safely on disk.
**
** 	Overview of functions performed in this routine:-
**
**	Locate the SCB for the caller.
**	Call the SXAP routine to perform the physical flush operation.
**
** Inputs:
**	scb			SXF session control block.
**
** Outputs:
**	err_code		Error code returned to the caller.
**
** Returns:
**	DB_STATUS
**
** History:
**	20-sep-1992 (markg)
**	    Initial creation.
**	12-jan-94 (stephenb)
**	    Integrate (robf) fix:
**
**          Failure to flush audit records is a serious problem, since
**          events may not be written to the audit log, so call
**          sxac_audit_error() to determine how to handle the error condition.
**	15-apr-94 (robf) 
**	    SXAP may now return E_DB_WARN if auditing should be  
**	    stopped/suspended, so check for this
**	26-may-94 (robf)
**          Improve handling of returns from sxap_flush(), notably warning
**	    conditions  could be logged unnecessarily and internal error
**	    status codes could be lost sometimes. No reported bug on this,
**	    found during code inspection.
*/
DB_STATUS
sxar_flush(
    SXF_RCB	*rcb)
{
    DB_STATUS	status = E_DB_OK;
    i4	err_code = E_SX0000_OK;
    i4	local_err;
    i4	sxf_status = Sxf_svcb->sxf_svcb_status;
    SXF_SCB	*scb;

    rcb->sxf_error.err_code = E_SX0000_OK;

    for (;;)
    {
	/* If nothing to do we are done */
        if ((sxf_status & (SXF_C2_SECURE)) == 0)
        {
            break;
        }
	/*
	** If auditing is disabled break.
	*/
	if((sxf_status & SXF_AUDIT_DISABLED))
		break;

	status = sxau_get_scb(&scb, &local_err);
	if (status != E_DB_OK)
	{
	    err_code = local_err;
	    break;
	}
	/*
	** Call SXAP_FLUSH to physically perform the flush
	*/
	status = (*Sxap_vector->sxap_flush)(scb, &err_code);
	/*
	** Check for exceptional conditions
	*/
	if(status==E_DB_WARN)
	{
	    /*
	    ** Some unusual condition
	    */
	    if(err_code==E_SX102D_SXAP_STOP_AUDIT)
	    {
		/*
		** Auditing should be stopped, so set stopped bit
		** and break out.
		*/
		if(SXS_SESS_TRACE(SXS_TAUD_LOG))
			SXF_DISPLAY("SXA_FLUSH - Stop audit request detected\n");
		status=sxas_stop_audit(FALSE, &err_code);
		if(status!=E_DB_OK)
			status=E_DB_FATAL;
		else
			err_code=E_SX0000_OK;
	    }
	    else if(err_code==E_SX10AE_SXAP_AUDIT_SUSPEND)
	    {
	     	/*
	     	** Auditing should be suspended, so suspend auditing
	     	*/
		if(SXS_SESS_TRACE(SXS_TAUD_LOG))
			SXF_DISPLAY("SXA_FLUSH - Suspend audit request detected\n");
	     	status=sxas_suspend_audit(FALSE, &err_code);
	     	if(status!=E_DB_OK)
	     	{
			status=E_DB_FATAL;
			break;
		}
	    	else
	    	{
			bool audit_reqd;
			err_code=E_SX0000_OK;
			/*
			** Since auditing is suspended then maybe the
			** flush didn't complete, so suspend ourselves 
			** via sxas_check() until the flush can complete.
			*/
			if(SXS_SESS_TRACE(SXS_TAUD_LOG))
				SXF_DISPLAY("SXA_FLUSH - Auditing suspended, calling sxas_check to suspend us if necessary\n");
			status = sxas_check(
				scb, 
				SXF_E_SECURITY, /* Dummy */
				&audit_reqd,
				&err_code);
			if (status != OK)
			{
				/*
				** Error checking state
				*/
				if(SXS_SESS_TRACE(SXS_TAUD_LOG))
					SXF_DISPLAY("SXA_FLUSH - Error checking audit state\n");
				break;
			}
			else
			{
				/*
				** Auditing is OK, so
				** loop again to flush
				*/
				if(SXS_SESS_TRACE(SXS_TAUD_LOG))
					SXF_DISPLAY("SXA_FLUSH - Auditing check suceeded so continuing flush\n");
				continue;
			}
		}
	    }
	    else
	    {
		/* Unknown code for warning, so turn into error */
		status=E_DB_ERROR;
	    }
	}
	break;
    }

    /* Handle any errors */
    if (err_code != E_SX0000_OK)
    {
        status = E_DB_ERROR;
 
        if (err_code > E_SXF_INTERNAL)
        {
	    /* Log internal error */
	    _VOID_ ule_format(err_code, NULL,
			ULE_LOG, NULL, NULL, 0L, NULL, &local_err, 0);

            err_code = E_SX1012_SXA_FLUSH;
 
            /*
            ** Internal errors when attempting to flush
            ** audit records are considered to be severe
            ** errors.
            */
	    status = E_DB_SEVERE;
        }
	status = sxac_audit_error();
	if( status!=E_DB_OK)
            rcb->sxf_error.err_code = err_code;
    }
 
    return (status);
}
