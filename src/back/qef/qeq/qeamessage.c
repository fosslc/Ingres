/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <ade.h>
#include    <cs.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefdsh.h>
#include    <generr.h>
#include    <sqlstate.h>

#include    <qefqp.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>
#include    <tm.h>

#include    <dudbms.h>
#include    <qefqeu.h>
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
**  Name: QEAMESSAGE.C - QEF action routine for QEF_MESSAGE.
**
**  Description:
**	This module contains the QEF action routines to process the 
**	QEF_MESSAGE action in the QP.  The routines included in this
**	module are:
**
**          qea_message	 - return user-define message to front end
**          qea_emessage - return user-define error to front end
**
**  History:
**	10-may-88 (puree)
**	    created qea_message for DB procedures.
**	24-mar-89 (paul)
**	    Added qea_emessage in support of rules.
**	11-apr-89 (neil)
**	    Removed an old hack to support rules.
**	17-apr-89 (paul)
**	    Corrected unix compiler warnings.
**	24-apr-89 (neil)
**	    Modified arguments to SCC to pass user errors.
**	03-may-89 (neil)
**	    RAISE ERROR uses SCC_RSERROR operation instead of the "stype".
**	    If there is no text returns RAISE ERROR # - because there
**	    is no protocol to send an error w/o error message text.
**	21-may-89 (jrb)
**	    Changed scf_enumber to scf_local_error and added support for a
**	    new generic error code for raised errors.
**	30-may-89 (jrb)
**	    Set scf_generic_error to message number in qea_message.
**	14-jun-89 (neil)
**	    Made both use common local routine qea_procmessage.
**	24-apr-90 (davebf)
**	    return blank message properly
**	03-mar-91 (ralph)
**	    Add support for DESTINATION clause to the MESSAGE and RAISE ERROR 
**	    statements.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	15-jul-93 (robf)
**	    Add handling to send messages to security audit log
**      16-sep-1993 (pearl)
**          Bug 54772.  In qea_procmessage(), if DB_MSGLOG is set and iserror,
**          make sure we return E_DB_ERROR, or else the transaction isn't
**          rolled back.
**	02-oct-93 (swm)
**	    Bug #56447
**	    Removed truncating cast of qef_ses_id.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	18-Jan-2001 (jenjo02)
**	    Subvert calls to qeu_secaudit(), qeu_secaudit_detail()
**	    if not C2SECURE.
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	13-May-2005 (kodse01)
**	    replace %ld with %d for old nat and long nat variables.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**      20-Apr-2009 (horda03) Bug 121826
**          Abort if a message send is interrupted by user.
**/

/*	static functions	*/

static DB_STATUS
qea_procmessage(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
bool		iserror );




/*{
** Name: qea_message	- Send DB procedure message to the user.
**
** Description:
**	This routine materializes user's message and sends it to the
**	application in the front end.  A user message consists of the
**	message number and/or the message text.  A message number is
**	a non-nullable i4 and a message text sent to the application is
**	a variable-length string with a null terminator.
**
** Inputs:
**      action			ptr to action descriptor for the 
**				QEF_MESSAGE.
**	qef_rcb			ptr to QEF request block.
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**				E_QE0111_ERROR_SENDING_MESSAGE
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	02-feb-88 (puree)
**	    created for DB procedure.
**	13-jan-89 (paul)
**	    Added diagnostic capability for nested procedures.
**	    This should be removed before delivery.
**	11-apr-89 (neil)
**	    Removed above check as we now have RAISE ERROR.
**	17-apr-89 (paul)
**	    Corrected unix compiler warnings.
**	21-may-89 (jrb)
**	    Changed scf_enumber to scf_local_error.
**	30-may-89 (jrb)
**	    Set scf_generic_error to message number as well so that it will get
**	    into the gca_id_error which is where LIBQ expects it.
**	14-jun-89 (neil)
**	    Made both use common local routine qea_procmessage.
*/
DB_STATUS
qea_message(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb )
{
    return (qea_procmessage(action, qef_rcb, (bool)FALSE));
}

/*{
** Name: qea_emessage	- Send DB procedure user defined error to the user.
**
** Description:
**	This routine materializes user's error message and sends it to the
**	application in the front end.  A user error message consists of the
**	message number and, optionally, the message text.  A message number is
**	a non-nullable i4 and a message text sent to the application is
**	a variable-length string with a null terminator.
**
** Inputs:
**      action			ptr to action descriptor for the 
**				QEF_EMESSAGE.
**	qef_rcb			ptr to QEF request block.
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following
**				E_QE0025_USER_ERROR
**				E_QE0111_ERROR_SENDING_MESSAGE
**	Returns:
**	    E_DB_ERROR
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
** History:
**	24-mar-89 (paul)
**	    Created for rules support
**	24-apr-89 (neil)
**	    Change to SCC - SCC_MESSAGE + SCU_USERERROR = user error.
**	03-may-89 (neil)
**	    RAISE ERROR uses SCC_RSERROR operation instead of the "stype".
**	    If there is no text returns RAISE ERROR # - because there
**	    is no protocol to send an error w/o error message text.
**	21-may-89 (jrb)
**	    Renamed scf_enumber to scf_local_error and added support for generic
**	    error code on raised errors.
**	14-jun-89 (neil)
**	    Made both use common local routine qea_procmessage.
*/
DB_STATUS
qea_emessage(
QEF_AHD		    *action,
QEF_RCB		    *qef_rcb )
{
    return (qea_procmessage(action, qef_rcb, (bool)TRUE));
}

/*{
** Name: qea_procmessage - Send DB procedure message/error to the user.
**
** Description:
**	This routine materializes user's message and sends it to the
**	application in the front end.  A message consists of the
**	number and/or text.
**
** Inputs:
**      action			ptr to action descriptor for the QEF_MESSAGE.
**	qef_rcb			ptr to QEF request block.
**	iserror			TRUE/FALSE - user-defined error or just message
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following
**				E_QE0000_OK
**                              E_QE0025_USER_ERROR
**				E_QE0111_ERROR_SENDING_MESSAGE
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	14-jun-89 (neil)
**	    Extracted common code from above 2 routines.
**	24-apr-90 (davebf)
**	    return blank message properly
**	03-mar-91 (ralph)
**	    Add support for DESTINATION clause to the MESSAGE and RAISE ERROR 
**	    statements.
**	25-oct-92 (andre)
**	    in SCF_CB, generic_error has been replaced with sqlstate; if
**	    processing an error raised by the user, SQLSTATE will be set to
**	    SS50009_ERROR_RAISED_IN_DBPROC; if processing a message, sqlstate
**	    will be set to SS5000G_DBPROC_MESSAGE
**	15-jul-93 (robf)
**	    Add handling to send messages to security audit log
**      16-sep-1993 (pearl)
**          Bug 54772.  In qea_procmessage(), if DB_MSGLOG is set and iserror,
**          make sure we return E_DB_ERROR, or else the transaction isn't
**          rolled back.
**	3-nov-93 (robf)
**	    Correct merge errors, error handling cleanups.
**	26-aug-04 (inkdo01)
**	    Added global base array support for message results.
**	13-Dec-2005 (kschendel)
**	    Remaining inlined QEN_ADF structs changed to pointers, fix here.
*/
static DB_STATUS
qea_procmessage(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
bool		iserror )
{
    QEF_CB	*qef_cb = qef_rcb->qef_cb;
    QEE_DSH	*dsh = (QEE_DSH *)qef_cb->qef_dsh;
    ADF_CB	*adfcb;
    PTR		*cbs = dsh->dsh_cbs;
    QEN_ADF	*qen_adf;
    ADE_EXCB	*ade_excb;
    DB_STATUS	status, local_status;
    i4	msgnumber;
    SCF_CB	scf_cb;
    SCF_SCI	sci_list[1];
    char	username[DB_MAXNAME+1];
    i4		scf_type = iserror ? SCC_RSERROR : SCC_MESSAGE;
    struct  {
	i2	msglength;
	char	msgtext[DB_MAXSTRING];
    } msg;
    i4	error;
    char	*sqlstate_str;
    i4		i;
    DB_ERROR	local_error;

    adfcb = dsh->dsh_adf_cb;

    /* Materialize the user-specified message number */

    qen_adf = action->qhd_obj.qhd_message.ahd_mnumber;
    if (qen_adf != NULL)	/* Check if there is a message number */
    {				/* Yes, */
	ade_excb = (ADE_EXCB*) cbs[qen_adf->qen_pos];
	ade_excb->excb_seg = ADE_SMAIN;
	if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
	    dsh->dsh_row[qen_adf->qen_uoutput] = (PTR)(&msgnumber);
	else ade_excb->excb_bases[ADE_ZBASE + qen_adf->qen_uoutput] = 
						(PTR)(&msgnumber);
	status = ade_execute_cx(adfcb, ade_excb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb, 
		    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}
    }
    else
    {				/* no message number specified */
	msgnumber = 0L;
    }

    /* Materialize the user-specified message text */

    qen_adf = action->qhd_obj.qhd_message.ahd_mtext;
    if (qen_adf != NULL)	/* Check if there is a message text */
    {				/* Yes, */
	ade_excb = (ADE_EXCB*) cbs[qen_adf->qen_pos];
	ade_excb->excb_seg = ADE_SMAIN;
	if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
	    dsh->dsh_row[qen_adf->qen_uoutput] = (PTR)(&msg);
	else ade_excb->excb_bases[ADE_ZBASE + qen_adf->qen_uoutput] = (PTR)(&msg);
	status = ade_execute_cx(adfcb, ade_excb);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adfcb->adf_errcb, 
		    status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}

	if(msg.msglength == 0)	    /* if blank message */
	{
	    msg.msgtext[0] = EOS;   /* return blank message */
	    msg.msglength = 1;
	}
    }
    else				/* no message text specified */
    {
	/* Currently client has no way to process errors w/o text so provide */
	if (iserror)
	{
	    STprintf(msg.msgtext, "RAISE ERROR %d", msgnumber);
	    msg.msglength = STlength(msg.msgtext);
	}
	else
	{
	    msg.msgtext[0] = EOS;
	    msg.msglength = 1;
	}
    }

    /* Format SCF request block */
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_session = qef_cb->qef_ses_id;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_nbr_union.scf_local_error = msgnumber;
    sqlstate_str = (iserror) ? SS50009_ERROR_RAISED_IN_DBPROC
			     : SS5000G_DBPROC_MESSAGE;
    for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate[i] = sqlstate_str[i];
    scf_cb.scf_ptr_union.scf_buffer = &msg.msgtext[0];
    scf_cb.scf_len_union.scf_blength = msg.msglength;
    dsh->dsh_error.err_code = E_QE0000_OK;
    if ((action->qhd_obj.qhd_message.ahd_mdest == 0) ||
	(action->qhd_obj.qhd_message.ahd_mdest & DB_MSGUSER))
    {
	status = scf_call(scf_type, &scf_cb);
	if (DB_FAILURE_MACRO(status))
	{
            if (scf_cb.scf_error.err_code == E_CS0008_INTERRUPTED)
            {
               qef_error( E_QE0022_QUERY_ABORTED, 0L, E_DB_OK, &error, &local_error, 0);
            }

	    dsh->dsh_error.err_code = E_QE0111_ERROR_SENDING_MESSAGE;
	}
	else if (iserror)
	{
	    dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	    status = E_DB_ERROR;
	}
    }
    if (action->qhd_obj.qhd_message.ahd_mdest & DB_MSGLOG)
    {
	scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *)sci_list;
	scf_cb.scf_len_union.scf_ilength = 1;
	sci_list[0].sci_length	= DB_MAXNAME;
	sci_list[0].sci_code	= SCI_USERNAME;
	sci_list[0].sci_aresult = (char *)username;
	sci_list[0].sci_rlength = NULL;
	local_status = scf_call(SCU_INFORMATION, &scf_cb);
	if (local_status != E_DB_OK)
	{
	    dsh->dsh_error.err_code = E_QE022F_SCU_INFO_ERROR;
	    status = (status > local_status) ? status : local_status;
	}
	else
	{
	    username[DB_MAXNAME] = '\0';
	    _VOID_ STtrmwhite(username);
	    (VOID)qef_error(E_QE0300_USER_MSG, 0L, E_DB_OK, 
			&error, &local_error, 4,
			iserror ? 5 : 7, iserror ? "ERROR" : "MESSAGE",
			sizeof(msgnumber), &msgnumber, 
			STlength(username), username,
			(i4)msg.msglength, msg.msgtext);
           if (!status && iserror)
	   {
	       dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	       status = E_DB_ERROR;
	   }
	}
    }
    if ( action->qhd_obj.qhd_message.ahd_mdest & DB_MSGAUDIT &&
	 Qef_s_cb->qef_state & QEF_S_C2SECURE )
    {
	/*
	** Send to secuirty audit log if requested
	*/
	status =qeu_secaudit_detail( 
    		FALSE,
    		qef_cb->qef_ses_id,
		iserror? "USER ERROR": "USER MESSAGE",
		NULL,
		iserror? 10 : 12,
		SXF_E_SECURITY,
	 	I_SX2834_USERMESG,
		SXF_A_USERMESG,
		&local_error,
		msg.msgtext,
		msg.msglength,
		msgnumber);
         if (!status && iserror)
	 {
	       dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	       status = E_DB_ERROR;
	 }
	 else if (status!=E_DB_OK)
	 {
	       dsh->dsh_error.err_code = local_error.err_code;
	 }
    }
    return (status);
}
