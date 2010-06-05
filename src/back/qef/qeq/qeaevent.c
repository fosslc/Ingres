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
#include    <tm.h>
#include    <sxf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefdsh.h>
#include    <qefscb.h>

#include    <qefqp.h>
#include    <dmmcb.h>
#include    <dmucb.h>
#include    <ex.h>

#include    <dudbms.h>
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
**  Name: qeaevent.c - QEF action routines for events.
**
**  Description:
**      This module contains the QEF action routines to process the 
**      QEF_EVENT actions in the QP.
**
**  Defines:
**      qea_event  	- Register, remove, raise an event.
**
**  Locals:
**	qea_evtrace 	- Handle the user trace points for events.
**	
**  History:
**	07-sep-89 (neil)
**          Written for Terminator II/alerters.
**	27-oct-89 (neil)
**	    Added timestamp to RAISE.
**	09-feb-90 (neil)
**	    Auditing of event access.
**	01-mar-90 (neil)
**	    New event error message and made some scfa fields pointers.
**	16-jul-92 (rogerk)
**	    Fix overwriting of error status in security auditing code.
**	27-oct-92 (robf)
**	    Updated auditing to distinguish between REGISTER/REMOVE/RAISE
**	    event (previously all had been logged as "EXECUTE"). Also
**	    changed to use SXF call directly.
**	08-dec-92 (anitap)
**	    Included <qsf.h> and <psfparse.h> for CREATE SCHEMA.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	08-oct-93 (swm)
**	    Bug #56448
**	    Altered qec_tprintf calls to conform to new portable interface.
**	    Each call is replaced with a call to STprintf to deal with the
**	    variable number and size of arguments issue, then the resulting
**	    formatted buffer is passed to qec_tprintf. This works because
**	    STprintf is a varargs function in the CL. Unfortunately,
**	    qec_tprintf cannot become a varargs function as this practice is
**	    outlawed in main line code.
**	01-nov-93 (johnst)
**	    Bug #56447
**	    Deleted i4 cast on scf_cb.scf_session = qef_cb->qef_ses_id
**	    assignment as both sides are type CS_SID now. On axp_osf, CS_SIDs
**	    are longs, so i4 casts cause 64-to-32-bit truncations.
**	18-mar-94 (swm)
**	    Bug #59883
**	    Remove format buffers declared as automatic data on the stack with
**	    references to per session buffer allocated from ulm memory at
**	    QEF session startup time.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	23-feb-04 (inkdo01)
**	    Changed qef_rcb->error to dsh->dsh_error for || thread safety.
**	28-Dec-2005 (kschendel)
**	    Use DSH references whenever possible instead of QEF RCB
**	    references.  (This simplifies the code, eliminating many
**	    RCB references, and reflects the new primacy of the DSH
**	    thanks to the parallel query changes.)
**      09-jan-2009 (stial01)
**          Fix buffers that are dependent on DB_MAXNAME
**      01-apr-2010 (stial01)
**          Changes for Long IDs
*/



GLOBALREF       QEF_S_CB *Qef_s_cb;		/* To check if audit required */

/*{
** Name: qea_event - Register, Remove or Raise an Event.
**
** Description:
**	This routine is the last stop of events before SCF.  Until this routine
**	events are just simple data objects.  This routine will convert the
**	event action into an alert which then becomes a real SCF server
**	(or inter-server) operation.
**
**	Event data is moved into an SCF_ALERT and the following calls are made:
**	Action:		Call:		Data:
**	QEA_EVREGISTER	SCE_REGISTER	DB_ALERT_NAME
**	QEA_EVDEREG	SCE_REMOVE	DB_ALERT_NAME
**	QEA_EVRAISE	SCE_RAISE	DB_ALERT_NAME +time [+flags] [+text] 
**
**	If the action is QEA_EVRAISE then an extra check is made to see if
**	event tracing or logging is required:
**	Trace Flag:	Tracer:
**	QEF_T_EVENTS 	qec_tprintf
**	QEF_T_LGEVENTS 	ule_format + ULE_MESSAGE
**
** Inputs:
**	action			Current action:
**	   .ahd_type		The action type:
**				   QEA_EVREGISTER, QEA_EVDEREG, QEA_EVRAISE
**	   .qhd_event		Event data:
**		.ahd_event	Full uniquely qualified event name.
**		.ahd_evflags	Ignored if not QEA_RAISE, otherwise:
**				   QEF_EVNOSHARE	No share
**		.ahd_evvalue	Ignored if not QEA_RAISE, otherwise if not null
**				then this must be user text.  Truncation of
**				user text longer than DB_EVDATA_MAX is done
**				silently, though more than that may be copied
**				locally (to use the fastest CX - ADE_MECOPY).
**	qef_rcb			Ptr to QEF request block.  Contains other
**				CB's through which SCF can be accessed.
**
** Outputs:
**      qef_rcb
**	    .error.err_code	one of the following:
**				E_QE0000_OK
**                              E_QE0025_USER_ERROR - On all user errors
**				E_QE019A_EVENT_MESSAGE - No GCA event protocol
**				E_QE0200_NO_EVENTS - No event subsystem
**				E_QE020C_EV_REGISTER_EXISTS - Already registered
**				E_QE020D_EV_REGISTER_ABSENT - Not yet registered
**                              E_QE020F_EVENT_SCF_FAIL - Internal failure
**	Returns:
**	    E_DB_{OK, WARN, ERROR, FATAL}
**	Exceptions:
**	    none
**
** Side Effects:
**	1. SCF and EV have some global side effects.
**
** History:
**	01-sep-89 (neil)
**	    Written for Terminator II/alerters.
**	27-oct-89 (neil)
**	    Added timestamp to RAISE.
**	09-feb-90 (neil)
**	    Auditing of event access.
**	01-mar-90 (neil)
**	    New event error message.
**	16-jul-92 (rogerk)
**	    Fix overwriting of error status in security auditing code.
**	    Added local status variable for return code from qeu_audit call.
**	27-oct-92 (robf)
**	    Updated auditing to distinguish between REGISTER/REMOVE/RAISE
**	    event (previously all had been logged as "EXECUTE"). Also
**	    changed to use SXF call directly.
**	08-dec-1992 (pholman)
**	    C2: replace obsolete qeu_audit with qeu_secudit.
**	03-nov-93 (robf)
**	    Pass security label to qeu_secaudit().
**	6-Apr-2004 (schka24)
**	    Adjust rowcount in dsh, not rcb. 
**	26-aug-04 (inkdo01)
**	    Support global base array for EVRAISE result.
**	16-dec-04 (inkdo01)
**	    Init. a collID.
**	13-Dec-2005 (kschendel)
**	    Inlined QEN_ADF changed to pointer, fix here.
*/
DB_STATUS
qea_event(
QEF_AHD		*action,
QEF_RCB		*qef_rcb,
QEE_DSH		*dsh )
{
    QEF_CB	*qef_cb = dsh->dsh_qefcb;		/* Session CB */
    QEF_EVENT	*ev;					/* Current event     */
    SCF_ALERT	scfa;					/*   and SCF's event */
    SCF_CB	scf_cb;					/* SCF request block */
    i4		scf_opcode;				/*  and request code */
    QEN_ADF	*qen_adf;
    i4	access_type;
    i4	audit_msg;
    ADE_EXCB	*ade_excb;
    struct  {						/* User text data */
	i2	txtlen;
	char	text[DB_MAXSTRING];
    } txt;
    DB_DATA_VALUE tm;					/* Timestamp */
    DB_DATE	tm_date;
    ADF_CB	*adf_cb;
    DB_STATUS	status;
    i4	err;
    char	*errstr;
    i4	tr1 = 0, tr2 = 0;			/* Dummy trace values */
    DB_STATUS	audit_status;
    i4	error;

    dsh->dsh_error.err_code = E_QE0000_OK;
    dsh->dsh_qef_rowcount = -1;
    ev = &action->qhd_obj.qhd_event;
    scf_cb.scf_ptr_union.scf_alert_parms = &scfa;

    /* Format SCF request block */
    scf_cb.scf_facility = DB_QEF_ID;
    scf_cb.scf_session = qef_cb->qef_ses_id;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;

    scfa.scfa_name = &ev->ahd_event;
    scfa.scfa_text_length = 0;
    scfa.scfa_user_text = NULL;
    scfa.scfa_flags = 0;
    switch (action->ahd_atype)
    {
      case QEA_EVREGISTER:
	scf_opcode = SCE_REGISTER;
	errstr = "REGISTER";
	access_type=SXF_A_REGISTER;
	audit_msg=I_SX2705_REGISTER_DBEVENT;
	break;

      case QEA_EVDEREG:
	scf_opcode = SCE_REMOVE;
	errstr = "REMOVE";
	access_type=SXF_A_REMOVE;
	audit_msg=I_SX2706_REMOVE_DBEVENT;
	break;

      case QEA_EVRAISE:
	scf_opcode = SCE_RAISE;
	errstr = "RAISE";
	access_type=SXF_A_RAISE;
	audit_msg=I_SX2704_RAISE_DBEVENT;
	adf_cb = dsh->dsh_adf_cb;
	/* Materialize user-specified event text */
        txt.txtlen = 0;
	qen_adf = ev->ahd_evvalue;
	if (qen_adf != NULL)	/* If there is event text */
	{
	    ade_excb = (ADE_EXCB*)dsh->dsh_cbs[qen_adf->qen_pos];
	    ade_excb->excb_seg = ADE_SMAIN;
	    if (dsh->dsh_qp_ptr->qp_status & QEQP_GLOBAL_BASEARRAY)
		dsh->dsh_row[qen_adf->qen_uoutput] = (PTR)&txt;
	    else ade_excb->excb_bases[ADE_ZBASE+qen_adf->qen_uoutput] = (PTR)(&txt);
	    status = ade_execute_cx(adf_cb, ade_excb);
	    if (status != E_DB_OK)
	    {
		if ((status = qef_adf_error(&adf_cb->adf_errcb,
			  status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		    return (status);
	    }
	    if (txt.txtlen > 0)		/* No need for empty strings */
	    {
		if (txt.txtlen > DB_EVDATA_MAX)
		    txt.txtlen = DB_EVDATA_MAX;
		scfa.scfa_text_length = txt.txtlen;
		scfa.scfa_user_text = txt.text;
	    }
	} /* If event text */
	tm.db_datatype = DB_DTE_TYPE; 	/* Get time stamp for the event */
	tm.db_prec = 0;
	tm.db_collID = -1;
	tm.db_length = sizeof(tm_date);
	tm.db_data = (PTR)&tm_date;
	status = adu_datenow(adf_cb, &tm);
	if (status != E_DB_OK)
	{
	    if ((status = qef_adf_error(&adf_cb->adf_errcb,
			status, qef_cb, &dsh->dsh_error)) != E_DB_OK)
		return (status);
	}
	scfa.scfa_when = &tm_date;
	if (ev->ahd_evflags & QEF_EVNOSHARE)
	    scfa.scfa_flags = SCE_NOSHARE;

	/* If tracing and/or logging events then display event information */
	if (ult_check_macro(&qef_cb->qef_trace, QEF_T_EVENTS, &tr1, &tr2))
	    qea_evtrace(qef_rcb, QEF_T_EVENTS, &ev->ahd_event, 
			&tm, txt.txtlen, txt.text);
	if (ult_check_macro(&qef_cb->qef_trace, QEF_T_LGEVENTS, &tr1, &tr2))
	    qea_evtrace(qef_rcb, QEF_T_LGEVENTS, &ev->ahd_event, 
			&tm, 0, (char *)NULL);
	break;
    } /* end switch */

    status = scf_call(scf_opcode, &scf_cb);
    if (status != E_DB_OK)
    {
	char	*enm, *onm;	/* Event and owner names */

	enm = (char *)&ev->ahd_event.dba_alert;
	onm = (char *)&ev->ahd_event.dba_owner;
	switch (scf_cb.scf_error.err_code)
	{
	  case E_SC0270_NO_EVENT_MESSAGE:
	    _VOID_ qef_error(E_QE019A_EVENT_MESSAGE, 0L, status, &err,
			     &dsh->dsh_error, 1,
			     (i4)STlength(errstr), errstr);
	    break;
	  case E_SC0280_NO_ALERT_INIT:
	    _VOID_ qef_error(E_QE0200_NO_EVENTS, 0L, status, &err,
			     &dsh->dsh_error, 1,
			     (i4)STlength(errstr), errstr);
	    break;
	  case E_SC002B_EVENT_REGISTERED:
	    _VOID_ qef_error(E_QE020C_EV_REGISTER_EXISTS, 0L, status, &err,
			     &dsh->dsh_error, 2,
			     qec_trimwhite(DB_OWN_MAXNAME, onm), onm,
			     qec_trimwhite(DB_EVENT_MAXNAME, enm), enm);
	    break;
	  case E_SC002C_EVENT_NOT_REGISTERED:
	    _VOID_ qef_error(E_QE020D_EV_REGISTER_ABSENT, 0L, status, &err,
			     &dsh->dsh_error, 2,
			     qec_trimwhite(DB_OWN_MAXNAME, onm), onm,
			     qec_trimwhite(DB_EVENT_MAXNAME, enm), enm);
	    break;
	  default:
	    _VOID_ qef_error(E_QE020F_EVENT_SCF_FAIL, 0L, status, &err,
			     &dsh->dsh_error, 4,
			     (i4)STlength(errstr), errstr,
			     (i4)sizeof(i4),
					(PTR)&scf_cb.scf_error.err_code,
			     qec_trimwhite(DB_OWN_MAXNAME, onm), onm,
			     qec_trimwhite(DB_EVENT_MAXNAME, enm), enm);
	    break;
	}
	dsh->dsh_error.err_code = E_QE0025_USER_ERROR;
	status = E_DB_ERROR;
    } /* If SCF not ok */

    /*
    ** Audit the event operation: since the object (event) was accessed
    ** we must always audit even if the operation failed.  For 
    ** performance confirm that auditing is really on.
    */
    if (Qef_s_cb->qef_state & QEF_S_C2SECURE)
    {
        DB_ERROR	e_error;

	if(status==E_DB_OK)
	    access_type |=SXF_A_SUCCESS;
	else
	    access_type |=SXF_A_FAIL;

	audit_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
	    		(char *)&ev->ahd_event.dba_alert,
			&ev->ahd_event.dba_owner,
	    		DB_EVENT_MAXNAME, SXF_E_EVENT,
	      		audit_msg, access_type, &e_error);

	if ((audit_status != E_DB_OK) && (status == E_DB_OK))
	{
	    /*
	    ** Return audit error only if we are not already returning an
	    ** error to the caller.  The user error code takes precedence
	    ** over the audit error status.
	    */
	    dsh->dsh_error.err_code = e_error.err_code;
	    status = audit_status;
	}
    }
    return (status);
} /* qea_event */

/*{
** Name: qea_evtrace - Trace/log RAISE EVENT.
**
** Description:
**	This routine acts as a trace utility for RAISE EVENT.
**
**	Note that we can be called in a non-query context (from qeu), so
**	don't expect a DSH to be available here!
**
** Inputs:
**	qef_rcb			Ptr to QEF request block to pass on.
**	printf			Flag: QEF_T_EVENTS 	- Trace events
**				      QEF_T_LGEVENTS	- Log events
**	ev			Action event data.
**	tm			Time stamp.
**	txtlen			Length of RAISE text (may be zero).
**	txt			RAISE text (may be null - ignored if log events)
**
** Outputs:
**      None
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	1. May send trace data to FE.
**
** History:
**	01-sep-89 (neil)
**	    Written for Terminator II/alerters.
**	27-oct-89 (neil)
**	    Added timestamp tracing.
**	26-nov-93 (robf) 
**	    Made global, updated interface to allow calling from 
**	    qeu_event.c
*/

VOID
qea_evtrace(
QEF_RCB		*qef_rcb,
i4		print,
DB_ALERT_NAME   *alert,
DB_DATA_VALUE	*tm,
i4		txtlen,
char		*txt )
{
    char		*enm, *onm, *dnm;	/* Event, owner, db names */
    char		buf[200 + (3*DB_MAXNAME)];
    i4		error;
    DB_DATA_VALUE	tm_char;		/* Text for log-time stamp */
    char		tm_buf[27];
    char		*cbuf = qef_rcb->qef_cb->qef_trfmt;
    i4			cbufsize = qef_rcb->qef_cb->qef_trsize;

    enm = (char *)&alert->dba_alert;
    onm = (char *)&alert->dba_owner;
    dnm = (char *)&alert->dba_dbname;
    STprintf(buf, "%s: RAISE '%.*s', owned by '%.*s', in database '%.*s'",
	     print == QEF_T_EVENTS ? "PRINTEVENTS" : "LOGEVENTS",
	     qec_trimwhite(DB_EVENT_MAXNAME, enm), enm,
	     qec_trimwhite(DB_OWN_MAXNAME, onm), onm,
	     qec_trimwhite(DB_DB_MAXNAME, dnm), dnm);
    if (print == QEF_T_EVENTS)	/* Print RAISE + values */
    {
	STprintf(cbuf, "%s\n", buf);
        qec_tprintf(qef_rcb, cbufsize, cbuf);
	if (txtlen > 0)
	{
	    STprintf(cbuf, "PRINTEVENTS Values: %.*s\n", txtlen, txt);
            qec_tprintf(qef_rcb, cbufsize, cbuf);
	}
    }
    else			/* Log RAISE */
    {
	tm_char.db_datatype = DB_CHA_TYPE;
	tm_char.db_length = 26;
	tm_char.db_data = (PTR)tm_buf;
	tm_buf[0] = EOS;
	tm_buf[26] = EOS;
	_VOID_ adc_cvinto(qef_rcb->qef_cb->qef_adf_cb, tm, &tm_char);
	STprintf(buf + STlength(buf), " (%s)", tm_buf);
	ule_format(0, 0, ULE_MESSAGE, NULL, buf, STlength(buf), 0, &error, 0);
    }
} /* qea_evtrace */
