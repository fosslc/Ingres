/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <ddb.h>
#include    <tm.h>
#include    <er.h>
#include    <cs.h>
#include    <scf.h>
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
#include    <sxf.h>

#include    <ade.h>
#include    <dmmcb.h>
#include    <dmucb.h>
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
#include    <qefprotos.h>
/**
**
**  Name: QEUSEC.C - routines for maintaining security state
**
**  Description:
**	The routines defined in this file provide an interface for
**	qyrmod to manipulate the contents of the iisecuritystate catalog.
**
**	The following routine(s) are defined in this file:
**
**	    qeu_msec	 - modifies a security state record
**
**  History:
**      25-jun-89 (jennifer)
**          Created for Security.
**	09-feb-90 (neil)
**	    Modified to include rules and alerts.
**	08-aug-90 (ralph)
**	    Added SUCCESS to access field for successful operation (20659)
**	    Added support for auditing changes to ALARM auditing (DU_SALARM)
**	    Audit DISABLE SECURITY_AUDIT in all cases (20482)
**	03-nov-92 (robf)
**	    Updated to use SXF discrete audit message ids
**	20-nov-92 (markg)
**	    Updated to call SXF for audit state manipulation.
**	11-jan-1993 (pholman)
**	    Replace obsolete use of qeu_audit with qeu_secaudit.
**	6-jul-93 (robf)
**	    Pass security label to qeu_secaudit()
**	12-jul-93 (robf)
**	    Added  handling for security label auditing. 
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	30-mar-04 (toumi01)
**	    move qefdsh.h below qefact.h for QEF_VALID definition
**	13-Feb-2006 (kschendel)
**	    Fix a couple annoying int == NULL warnings.
**/

/*
**  Definition of static variables and forward static functions 
*/

/*{
** Name: qeu_msec	- Store security state information.
**
** External QEF call:   status = qef_call(QEU_MSEC, &qeuq_cb);
**
** Description:
**	Add the security state tuple to the iisecuritystate table.
**
**      The security state tuple was filled in from the 
**      ENABLE AUDIT statement.
**      If a record already exists for the specified event or level
**      then replace it, otherwise insert the new one.
**
** Inputs:
**      qef_cb                  QEF session control block
**      qeuq_cb
**	    .qeuq_eflag	        Designate error handling for user errors.
**		QEF_INTERNAL	Return error code.
**		QEF_EXTERNAL	Send message to user.
**	    .qeuq_culd		Number of security state tuples.  Must be 1.
**	    .qeuq_uld_tup	Security state tuple.
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
**	25-jun-89 (jennifer)
**	    Written for Security.
**	09-feb-90 (neil)
**	    Modified to include rules and alerts.
**	08-aug-90 (ralph)
**	    Added SUCCESS to access field for successful operation (20659)
**	    Added support for auditing changes to ALARM auditing (DU_SALARM)
**	    Audit DISABLE SECURITY_AUDIT in all cases (20482)
**	03-nov-92 (robf)
**	    Updated to use SXF discrete audit message ids, with no 
**	    parameters.
**	20-nov-92 (markg)
**	    Updated to call SXF for audit state manipulation.
**	11-jan-93 (robf)
**	    Corrected problem with DBEVENT change, needed to be 8 rather than
**	    6 bytes for text.
**	14-jan-1993 (pholman)
**	    C2: Replace obsolete qeu_audit with qeu_secaudit.
**	2-jul-93 (robf)
**	    Added RESOURCE, QRYTEXT, auditing, rewrote many if-compares
**	    to  be table driven instead.
**	    If SXC_ALTER fails break out rather  than continuing (this
**	    prevents the error getting lost)
**	12-jul-93 (robf)
**	    Add handling for security label auditing
*/

DB_STATUS
qeu_msec(
QEF_CB          *qef_cb,
QEUQ_CB		    *qeuq_cb)
{
    DU_SECSTATE     *stuple;	/* New security tuple */
    DB_STATUS	    status = E_DB_OK, local_status;
    i4	    error=E_QE0000_OK;
    i4              access;
    char            *p;
    char            *objname;
    i4              l_objname;
    i4		    i;
    i4         msgid;
    SXF_RCB	    sxf;
    static struct {
	i4 type;
	i4 msgenable;
	i4 msgdisable;
	char	*objname;
    } audtypes [] = {
	SXF_E_DATABASE, 
		I_SX2508_ENAB_AUDIT_DATABASE, 
		I_SX2509_DISB_AUDIT_DATABASE,
		"DATABASE",
	SXF_E_APPLICATION, 
		I_SX2501_CHANGE_AUDIT_PROFILE, 
		I_SX2501_CHANGE_AUDIT_PROFILE,
		"ROLE",
	SXF_E_PROCEDURE,
		I_SX250C_ENAB_AUDIT_PROCEDURE,
		I_SX250D_DISB_AUDIT_PROCEDURE,
		"PROCEDURE",
	SXF_E_TABLE,	
		I_SX2504_ENAB_AUDIT_TABLE,
		I_SX2505_DISB_AUDIT_TABLE,
		"TABLE",
	SXF_E_LOCATION,
		I_SX2516_ENAB_AUDIT_LOCATION,
		I_SX2517_DISB_AUDIT_LOCATION,
		"LOCATION",
	SXF_E_VIEW,
		I_SX2506_ENAB_AUDIT_VIEW,
		I_SX2507_DISB_AUDIT_VIEW,
		"VIEW",
	SXF_E_RECORD,
		I_SX2501_CHANGE_AUDIT_PROFILE,
		I_SX2501_CHANGE_AUDIT_PROFILE,
		"ROW",
	SXF_E_SECURITY,
		I_SX250E_ENAB_AUDIT_SECURITY,
		I_SX250F_DISB_AUDIT_SECURITY,
		"SECURITY",
	SXF_E_USER,
		I_SX250A_ENAB_AUDIT_USER,
		I_SX250B_DISB_AUDIT_USER,
		"USER",
	SXF_E_LEVEL,
		I_SX2501_CHANGE_AUDIT_PROFILE,
		I_SX2501_CHANGE_AUDIT_PROFILE,
		"LEVEL",
	SXF_E_RULE,
		I_SX2514_ENAB_AUDIT_RULE,
		I_SX2515_DISB_AUDIT_RULE,
		"RULE",
	SXF_E_ALARM,
		I_SX2510_ENAB_AUDIT_ALARM,
		I_SX2511_DISB_AUDIT_ALARM,
		"ALARM",
	SXF_E_EVENT,
		I_SX2512_ENAB_AUDIT_DBEVENT,
		I_SX2513_DISB_AUDIT_DBEVENT,
		"DBEVENT",
	SXF_E_ALL,
		I_SX2502_ENAB_AUDIT_ALL,
		I_SX2503_DISB_AUDIT_ALL,
		"ALL",
	SXF_E_RESOURCE,
		I_SX2519_ENAB_AUDIT_RESOURCE,
		I_SX251A_DISB_AUDIT_RESOURCE,
		"RESOURCE",
	SXF_E_QRYTEXT,
		I_SX251B_ENAB_AUDIT_QRYTEXT,
		I_SX251C_DISB_AUDIT_QRYTEXT,
		"QUERY_TEXT",
	-1, 0, 0, NULL
    };

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
		
	if(qeuq_cb->qeuq_sec_mask & QEU_SEC_NORM)
	{
		stuple = (DU_SECSTATE *)qeuq_cb->qeuq_uld_tup->dt_data;
		if (stuple->du_state == 0)
		    sxf.sxf_auditstate = SXF_S_DISABLE;
		else
		    sxf.sxf_auditstate = SXF_S_ENABLE;

		switch (stuple->du_id)
		{
		case DU_SDATABASE:
		    sxf.sxf_auditevent = SXF_E_DATABASE;
		    break;
		case DU_SAPPLICATION:
		    sxf.sxf_auditevent = SXF_E_APPLICATION;
		    break;
		case DU_SPROCEDURE:
		    sxf.sxf_auditevent = SXF_E_PROCEDURE;
		    break;
		case DU_STABLE:
		    sxf.sxf_auditevent = SXF_E_TABLE;
		    break;
		case DU_SLOCATION:
		    sxf.sxf_auditevent = SXF_E_LOCATION;
		    break;
		case DU_SVIEW:
		    sxf.sxf_auditevent = SXF_E_VIEW;
		    break;
		case DU_SRECORD:
		    sxf.sxf_auditevent = SXF_E_RECORD;
		    break;
		case DU_SSECURITY:
		    sxf.sxf_auditevent = SXF_E_SECURITY;
		    break;
		case DU_SUSER:
		    sxf.sxf_auditevent = SXF_E_USER;
		    break;
		case DU_SALARM:
		    sxf.sxf_auditevent = SXF_E_ALARM;
		    break;
		case DU_SRULE:
		    sxf.sxf_auditevent = SXF_E_RULE;
		    break;
		case DU_SALERT:
		    sxf.sxf_auditevent = SXF_E_EVENT;
		    break;
		case DU_SRESOURCE:
		    sxf.sxf_auditevent = SXF_E_RESOURCE;
		    break;
		case DU_SQRYTEXT:
		    sxf.sxf_auditevent = SXF_E_QRYTEXT;
		    break;
		case DU_SALL:
		    sxf.sxf_auditevent = SXF_E_ALL;
		    break;
		default:
		    status = E_DB_ERROR;
		    error = E_QE0018_BAD_PARAM_IN_CB;
		    break;
		}
		if(status!=E_DB_OK)
			break;
		if (stuple->du_state == 0)
		    access = SXF_A_DROP;
		else
		    access = SXF_A_CREATE;
	}
	else
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}

	sxf.sxf_length = sizeof(sxf);
        sxf.sxf_cb_type = SXFRCB_CB;

	if (status != E_DB_OK)
	    break;

	/* Prepare audit information */

	for(i=0; audtypes[i].type!= -1; i++)
	{
		if(audtypes[i].type==sxf.sxf_auditevent)
		{
		    if(sxf.sxf_auditstate == SXF_S_ENABLE)
			msgid=audtypes[i].msgenable;
		    else
			msgid=audtypes[i].msgdisable;
		    objname=audtypes[i].objname;
		    l_objname=STlength(objname);
		    break;
		}
	}
	if (audtypes[i].type== -1)
	{
	    status = E_DB_ERROR;
	    error = E_QE0018_BAD_PARAM_IN_CB;
	    break;
	}
        access |= SXF_A_SUCCESS;

	/* If this is a DISABLE request, audit now */

	if (sxf.sxf_auditstate == SXF_S_DISABLE)
	{
	    DB_ERROR e_error;

	    status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		  objname,(DB_OWN_NAME *)NULL,
		  l_objname, SXF_E_SECURITY,
		  msgid, access, 
		  &e_error);

	    error = e_error.err_code;

	    if (status != E_DB_OK)
	    {
		break;
	    }
	}

	/* Change audit status */

	status = sxf_call(SXS_ALTER, &sxf);
	if (status != E_DB_OK)
	{
	    error = sxf.sxf_error.err_code;
	    if (status != E_DB_OK)
	    {
		break;
	    }
	}

	/* If this was not DISABLE, audit now */

	if (sxf.sxf_auditstate != SXF_S_DISABLE)
	{
	    DB_ERROR e_error;

	    local_status = qeu_secaudit(FALSE, qef_cb->qef_ses_id,
		  objname,(DB_OWN_NAME *)NULL,
		  l_objname, SXF_E_SECURITY,
		  msgid, access, 
		  &e_error);

	    error = e_error.err_code;
	    if (local_status != E_DB_OK)
	    {
		status = (status > local_status) ?
			  status : local_status;

		break;
	    }
	}

	break;
    }

    if (status)
    {
        /* call qef_error to handle error messages */
        _VOID_ qef_error(error, 0L, status, &error, &qeuq_cb->error, 0);
    }
    else
    {
	qeuq_cb->error.err_code = E_QE0000_OK;
    }
    
    return (status);
} /* qeu_msec */

