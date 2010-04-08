/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <cs.h>
#include    <tm.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <qefmain.h>
#include    <qefscb.h>

#include    <ade.h>
#include    <adf.h>
#include    <dudbms.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <ex.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>
/**
**
**  Name: QEUAUDIT.C - routines for creating and sending audit records.
**
**  Description:
**	The routines defined in this file provide an interface for
**	QEF to create and send audit records to DMF.
**
**	The following routine(s) are defined in this file:
**
**	    qeu_audit	- creates and send audit record to DMF.
**
**  History:
**      25-jun-89 (jennifer)
**          Created for Security.
**	20-nov-1992 (pholman)
**	    This module is now OBSOLETE and will be phased out in favour
**	    of calls to sxf_call with SXR_WRITE.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      13-sep-93 (smc)
**          Made session ids CS_SID.
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
** Name: qeu_audit	- Create and send audit records.
**
** Internal QEF call:   status = qef_audit();
**
** Description:
**      Determine the type of audit record to be built, build it 
**      and send to DMF.
**
** Inputs:
**      session_id              SCF session id.
**      type                    Type of event being audited.
**      access                  Type of access being audited
**      name                    Name of object being manipulated.
**      l_name                  Length of object name.
**      info_number             Number of information error used 
**                              to create audit text.
**      n_args                  Number of arguments in argument array.
**      args                    Pointer to argument array which contains
**                              Length followed by pointer to argument
**                              used by ERlookup to build text portion 
**                              of audit record.
**	
** Outputs:
**      err.err_code            E_QE001F_INVALID_REQUEST or
**                              Errors returned from DMF or ERlookup
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** History:
**	25-jun-89 (jennifer)
**	    Written for Security.
**	20-nov-1992 (pholman)
**	    This module is now OBSOLETE and will be phased out in favour
**	    of calls to sxf_call with SXR_WRITE.
*/

DB_STATUS
qeu_audit(
CS_SID       session_id,
i4           type,
i4           access,
char         *name,
i4           l_name,
i4      info_number,
i4           n_args,
PTR          args,
DB_ERROR     *err)
{
    DB_STATUS	    	status, local_status;
    i4	    	error;
    i4		    	i;		
    i4             err_code;
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    SXF_RCB             sxfrcb;

    if ((Qef_s_cb->qef_state & QEF_S_C2SECURE) == 0)
	return (E_DB_OK);

    MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
    sxfrcb.sxf_cb_type    = SXFRCB_CB;
    sxfrcb.sxf_length     = sizeof(sxfrcb);
    sxfrcb.sxf_force      = TRUE;
    sxfrcb.sxf_sess_id    = session_id;
    sxfrcb.sxf_scb        = NULL;
    sxfrcb.sxf_objowner   = (DB_OWN_NAME *)NULL;
    sxfrcb.sxf_objname    = name;
    sxfrcb.sxf_objlength  = l_name;
    sxfrcb.sxf_auditevent = type;
    sxfrcb.sxf_msg_id     = info_number;
    sxfrcb.sxf_accessmask = access;

    if ((status = sxf_call(SXR_WRITE, &sxfrcb)) == E_DB_OK)
	return (status);

    error = sxfrcb.sxf_error.err_code;
    (VOID) qef_error(error, 0L, status, &error, err, 0);
    return (status);
}

/*{
** Name:	qeu_secaudit	- wrapper for writing security audit records
**
** Description:
**	Internal QEF call.
**	This function is a wrapper for writing security audit records to
**	the Security eXtensions Facility from the QEF.
**
** Inputs:
**	force		Indicates if the SXF should ensure record written
**			to disk before returning.
**	sess_id		The session ID of the user on whose behalf the record
**			is being generated.
**	objname		The name of the object that the user is accessing
**	objowner	The owner (if any) of the object
**	objlength	The length of the object name
**	auditevent	The class of security audit record this belongs in
**	msg_id		The SXF message (description of audit record)
**	accessmask	The type (eg CREATE, DROP) and status (SUCCESS/FAIL) of
**			the operation that caused the record to be written.
**	err		Error id (if return status != E_DB_OK).
**	
**
** Outputs:
**	<param name>	<output value description>
**
** Returns:
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**
** Exceptions: 
**	None
**	
**
** History:
**	27-nov-1992 (pholman)
**	    First written, to allow qeu_audit to be phased out gradually.
**	29-may-1992 (robf)
**	    Cleaned up error handling. qeferror needed a 'known' error to
**	    map, so logged the SXF error here then pass E_QE019C_SEC_WRITE_ERROR
**	    (same mapping as DMF version) to qeferror.
**	14-jul-93 (robf)
**	    Made wrapper for qeu_secaudit_detail()
*/
DB_STATUS
qeu_secaudit( 
    int		force,
    CS_SID	sess_id,
    char	*objname,
    DB_OWN_NAME *objowner,
    i4	objlength,
    SXF_EVENT	auditevent,
    i4	msg_id,
    SXF_ACCESS	accessmask,
    DB_ERROR    *err)
{
	return ( qeu_secaudit_detail( 
	    force,
	    sess_id,
	    objname,
	    objowner,
	    objlength,
	    auditevent,
	    msg_id,
	    accessmask,
	    err,
	    NULL, 0,0 	/* Detail info */
	    ));
}

/*{
** Name:	qeu_secaudit_detail
**
** Description:
**	Internal QEF call.
**	This function is a wrapper for writing security audit records to
**	the Security eXtensions Facility from the QEF, including
**	"detail" information.
**
** Inputs:
**	force		Indicates if the SXF should ensure record written
**			to disk before returning.
**	sess_id		The session ID of the user on whose behalf the record
**			is being generated.
**	objname		The name of the object that the user is accessing
**	objowner	The owner (if any) of the object
**	objlength	The length of the object name
**	auditevent	The class of security audit record this belongs in
**	msg_id		The SXF message (description of audit record)
**	accessmask	The type (eg CREATE, DROP) and status (SUCCESS/FAIL) of
**			the operation that caused the record to be written.
**	err		Error id (if return status != E_DB_OK).
**	detailtext	Detailed info
**	detaillen	Length of detailtext
**	detailnum	Detail number
**	
**
** Outputs:
**	<param name>	<output value description>
**
** Returns:
**
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**
** Exceptions: 
**	None
**	
**
** History:
**	14-jul-93 (robf)
**	    Created from qeu_secaudit()
**	30-dec-93 (robf)
**          Initialize the SXF session to NULL, this was wrong  anyway
**	    and was ignored by SXF.
**	6-may-94 (robf)
**          Quiten VMS compiler, add space between =&
*/
DB_STATUS
qeu_secaudit_detail( 
    int		force,
    CS_SID	sess_id,
    char	*objname,
    DB_OWN_NAME *objowner,
    i4	objlength,
    SXF_EVENT	auditevent,
    i4	msg_id,
    SXF_ACCESS	accessmask,
    DB_ERROR    *err,
    char	*detailtext,
    i4	detaillen,
    i4	detailnum)
{
    DB_STATUS	    	status;
    i4	    	error;
    GLOBALREF QEF_S_CB	*Qef_s_cb;
    SXF_RCB             sxfrcb;
    CL_SYS_ERR	    syserr;
    i4         local_error;

    /*
    ** If we're not doing any C2 security auditing, then just return
    ** with E_DB_OK.
    */
    if ((Qef_s_cb->qef_state & QEF_S_C2SECURE) == 0)
	return (E_DB_OK);

    MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
    sxfrcb.sxf_cb_type    = SXFRCB_CB;
    sxfrcb.sxf_length     = sizeof(sxfrcb);
    sxfrcb.sxf_force      = force;
    sxfrcb.sxf_sess_id    = (CS_SID) 0;
    sxfrcb.sxf_scb        = NULL;
    sxfrcb.sxf_objname    = objname;
    sxfrcb.sxf_objowner   = objowner;
    sxfrcb.sxf_objlength  = objlength;
    sxfrcb.sxf_auditevent = auditevent;
    sxfrcb.sxf_msg_id     = msg_id;
    sxfrcb.sxf_accessmask = accessmask;
    sxfrcb.sxf_detail_txt = detailtext;
    sxfrcb.sxf_detail_len = detaillen;
    sxfrcb.sxf_detail_int = detailnum;

    if ((status = sxf_call(SXR_WRITE, &sxfrcb)) == E_DB_OK)
	return (status);

    /*
    **	Log SXF error in the error log
    */
    (VOID) ule_format(sxfrcb.sxf_error.err_code, &syserr, ULE_LOG, NULL,
	    (char *)0, 0L, (i4 *)0, &local_error, 0);
    /*
    **	Call QEFERROR with standard Security Write Error
    */
    error = E_QE019C_SEC_WRITE_ERROR;
    (VOID) qef_error(error, sxfrcb.sxf_error.err_code, status, &error, err, 0);
    return (status);
}
