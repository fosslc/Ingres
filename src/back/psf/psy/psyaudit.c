/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <er.h>
#include    <qu.h>
#include    <me.h>
#include    <tm.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <dmacb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <ulm.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <qefqeu.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
#include    <usererror.h>
#include    <scf.h>
#include    <sxf.h>
#include    <dudbms.h>
#include    <psyaudit.h>

/**
**
**  Name: PSYAUDIT.C - Functions to manipulate the iisecuritystate catalog.
**
**  Description:
**      This file contains functions necessary to enable and disable
**	security auditing:
**
**          psy_audit - Enables and disables security auditing
**
**
**  History:
**	07-sep-89 (ralph)
**	    written
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**      2-oct-1992 (ed)
**          Use DB_MAXNAME to replace hard coded numbers
**          - also created defines to replace hard coded character strings
**          dependent on DB_MAXNAME
**	06-nov-1992 (rog)
**	    <ulf.h> must be included before <qsf.h>.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	10-aug-93 (andre)
**	    fixed causes of compiler warnings
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time. Made sess_id a CS_SID.
**      22-feb-95 (newca01)
**          Added new IF stmt to check for C2 security before
**          ALTER SECURITY_AUDIT for B66592.
**	13-jun-96 (nick)
**	    LINT directed changes.
**	10-Jan-2001 (jenjo02)
**	    Supply session id to SCF instead of DB_NOSESSION.
**      11-nov-2002 (stial01)
**         psy_audit() check if security enabled before update security state
**      15-Apr-2003 (bonro01)
**          Added include <psyaudit.h> for prototype of psy_secaudit()
**      01-apr-2010 (stial01)
**          Changes for Long IDs
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 psy_audit(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);
i4 psy_secaudit(
	int force,
	PSS_SESBLK *sess_cb,
	char *objname,
	DB_OWN_NAME *objowner,
	i4 objlength,
	SXF_EVENT auditevent,
	i4 msg_id,
	SXF_ACCESS accessmask,
	DB_ERROR *err);
static i4 psy_altaudit(
	PSY_CB *psy_cb,
	PSS_SESBLK *sess_cb);

/*{
** Name: psy_audit - Enable/disable security auditing
**
**  INTERNAL PSF call format: status = psy_audit(&psy_cb, sess_cb);
**  EXTERNAL     call format: status = psy_call (PSY_AUDIT,&psy_cb, sess_cb);
**
** Description:
**	This procedure builds an iisecuritystate tuple from PSY_CB
**	fields, and passes this tuple thru RDF to QEF to update
**	the server's security auditing state.
**
**	General audit state alter processing is also performed here
**
**	This procedure is called for SQL language only.
**
** Inputs:
**      psy_cb
**	    .psy_auflag			auditing operation
**	    .psy_autype			auditing type
**	    .psy_aulevel		auditing level
**	sess_cb				Pointer to session control block
**					(Can be NULL)
**
** Outputs:
**      psy_cb
**	    .psy_error			Filled in if error happens
**	Returns:
**	    E_DB_OK			Function completed normally.
**	    E_DB_WARN			Function completed with warning(s);
**	    E_DB_ERROR			Function failed; non-catastrophic error
**	    E_DB_SEVERE			Function failed; catastrophic error
**	Exceptions:
**	    none
**
** Side Effects:
**	    None.
**
** History:
**	07-sep-89 (ralph)
**          written
**	24-oct-89 (ralph)
**	    Moved call to qeu_audit() & authorization check to pslsgram.yi.
**	12-mar-90 (andre)
**	    set rdr_2types_mask to 0.
**	22-may-90 (teg)
**	    init rdr_instr to RDF_NO_INSTR
**	03-aug-92 (barbara)
**	    initialize RDF_CB by calling pst_rdfcb_init() before rdf_call().
**      30-jul-93 (robf)
**	    Add support for PSQ_ALTAUDIT, new audit level  processing
**	15-feb-94 (robf)
**          Add (PTR) cast to silence compiler warnings
*/
DB_STATUS
psy_audit(
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb)
{
    DB_STATUS		status;
    i4		err_code;
    char		dbname[DB_DB_MAXNAME];
    SCF_CB		scf_cb;
    SCF_SCI		sci_list[1];
    RDF_CB		rdf_cb;
    register RDR_RB	*rdf_rb = &rdf_cb.rdf_rb;
    DU_SECSTATE		sectuple;
    bool		leave_loop = TRUE;
    register DU_SECSTATE    *sectup = &sectuple;
    i4			local_error;

    /* This code is called for SQL only */

    do
    {
	/* Make sure user is connected to iidbdb */

	scf_cb.scf_length	= sizeof (SCF_CB);
	scf_cb.scf_type	= SCF_CB_TYPE;
	scf_cb.scf_facility	= DB_PSF_ID;
        scf_cb.scf_session	= sess_cb->pss_sessid;
        scf_cb.scf_ptr_union.scf_sci     = (SCI_LIST *) sci_list;
        scf_cb.scf_len_union.scf_ilength = 1;
        sci_list[0].sci_length  = sizeof(dbname);
        sci_list[0].sci_code    = SCI_DBNAME;
        sci_list[0].sci_aresult = (char *) dbname;
        sci_list[0].sci_rlength = NULL;

        status = scf_call(SCU_INFORMATION, &scf_cb);
        if (status != E_DB_OK)
        {
	    err_code = scf_cb.scf_error.err_code;
	    (VOID) psf_error(E_PS0D13_SCU_INFO_ERR, 0L,
			     PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
	    status = (status > E_DB_SEVERE) ? status : E_DB_SEVERE;
	    break;
        }

	if (MEcmp((PTR)dbname, (PTR)DB_DBDB_NAME, sizeof(dbname)))
	{
	    /* Session not connected to iidbdb */
	    err_code = E_US18D4_6356_NOT_DBDB;
	    (VOID) psf_error(E_US18D4_6356_NOT_DBDB, 0L,
			     PSF_USERERR, &err_code, &psy_cb->psy_error, 1,
			     sizeof("ENABLE/DISABLE/ALTER SECURITY_AUDIT")-1,
			     "ENABLE/DISABLE/ALTER SECURITY_AUDIT");
	    status   = E_DB_ERROR;
	    break;
	}

	if ( (Psf_srvblk->psf_capabilities & PSF_C_C2SECURE) == 0)
	{ 
		(VOID) psf_error(E_SX002B_AUDIT_NOT_ACTIVE, 0, 
			PSF_USERERR, 
			&local_error, &psy_cb->psy_error, 0); 
		status = E_DB_ERROR; 
		break;
	}

	if (psy_cb->psy_auflag & PSY_ALTAUDIT)
	{
		/*
		** ALTER SECURITY_AUDIT...
		*/
		status=psy_altaudit( psy_cb, sess_cb);
		break;
	}
	/*
	** ENABLE/DISABLE SECURITY_AUDIT...
	*/
	/*
	** Fill in the part of RDF request block that will be constant.
	*/
	pst_rdfcb_init(&rdf_cb, sess_cb);
	rdf_rb->rdr_status	    = DB_SQL;

	if (psy_cb->psy_autype == DU_SLVL)  
	{
		/* ENABLE/DISABLE AUDIT LEVEL(nnn) */
		rdf_rb->rdr_qtuple_count = 1;

		if (psy_cb->psy_auflag == PSY_ENAUDIT)
			rdf_rb->rdr_update_op  = RDR_APPEND;

		else if (psy_cb->psy_auflag == PSY_DISAUDIT)
			rdf_rb->rdr_update_op  = RDR_DELETE;

		else
		{
		    err_code = E_PS0D48_INVALID_AUDIT_OP;
		    (VOID) psf_error(E_PS0D48_INVALID_AUDIT_OP, 0L,
				     PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		    status = E_DB_SEVERE;
		    break;
		}

		/* Call RDF to update the iisecuritystate tuple */

		status = rdf_call(RDF_UPDATE, (PTR)&rdf_cb);

		if (DB_FAILURE_MACRO(status))
		    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
					 &psy_cb->psy_error);
	}
	else	    /* ENABLE/DISABLE AUDIT other....  */
	{
		rdf_rb->rdr_update_op   = RDR_REPLACE;
		rdf_rb->rdr_types_mask  = RDR_SECSTATE;
		rdf_rb->rdr_qrytuple    = (PTR) sectup;
		rdf_rb->rdr_qtuple_count = 1;

		/* Set du_secstate.du_state */

		if (psy_cb->psy_auflag == PSY_ENAUDIT)
		    sectup->du_state = DU_STRUE;

		else if (psy_cb->psy_auflag == PSY_DISAUDIT)
		    sectup->du_state = DU_SFALSE;

		else
		{
		    err_code = E_PS0D48_INVALID_AUDIT_OP;
		    (VOID) psf_error(E_PS0D48_INVALID_AUDIT_OP, 0L,
				     PSF_INTERR, &err_code, &psy_cb->psy_error, 0);
		    status = E_DB_SEVERE;
		    break;
		}

		sectup->du_type	= DU_SEVENT;
		sectup->du_id	= psy_cb->psy_autype;

		/* Call RDF to update the iisecuritystate tuple */

		status = rdf_call(RDF_UPDATE, (PTR)&rdf_cb);

		if (DB_FAILURE_MACRO(status))
		    (VOID) psf_rdf_error(RDF_UPDATE, &rdf_cb.rdf_error, 
					 &psy_cb->psy_error);
	}
	break;
	/* leave_loop has already been set to TRUE */
    } while (!leave_loop);

    /* We are done */

    return    (status);
}


/*{
** Name:	psy_secaudit	- wrapper for writing security audit records
**
** Description:
**	Internal QEF call.
**	This function is a wrapper for writing security audit records to
**	the Security eXtensions Facility from the QEF.
**
** Inputs:
**	force		Indicates if the SXF should ensure record written
**			to disk before returning.
**	sess_cb		PSF session CB
**	    pss_sessid	The session ID of the user on whose behalf the record
**			is being generated.
**	    pss_retry	flag field containing an indicator of whether we are in
**			retry mode
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
**	    First written, to allow qeu_audit to be replaced in PSF
**	17-jun-93 (andre)
**	    Changed the interface to accept a ptr to PSF session CB instead of
**	    session id (which can be found inside sess_cb)
**	    
**	    in case of failure we would like to avoid producing duplicate
**	    audit records (due to PSF internal retry mechanism.)  Accordingly,
**	    we will avoid creating a SXF_A_FAIL record unless PSS_REPORT_MSGS
**	    bit is set in sess_cb->pss_retry.  PSS_REPORT_MSGS gets set if we
**	    are trying to reparse a query (the first attempt failed) or if we
**	    are creating/destryoing "qrymod" object(s) in which case we only try
**	    once.
**	30-dec-93 (robf)
**          Set SXF session id to NULL, since it was wrong before and
**	    ignored by SXF anyway.
**	6-may-94 (robf)
**          Add space between =& to quieten VMS compiler
*/
DB_STATUS
psy_secaudit( 
    int		force,
    PSS_SESBLK	*sess_cb,
    char	*objname,
    DB_OWN_NAME *objowner,
    i4	objlength,
    SXF_EVENT	auditevent,
    i4	msg_id,
    SXF_ACCESS	accessmask,
    DB_ERROR    *err)
{
    DB_STATUS	    	  status;
    GLOBALREF PSF_SERVBLK *Psf_srvblk;
    SXF_RCB               sxfrcb;
    CL_SYS_ERR	          syserr;
    i4               local_error;

    /*
    ** If we're not doing any C2 security auditing, then just return
    ** with E_DB_OK.
    **
    ** If we were called to audit failure and we are going to retry, return
    ** E_DB_OK as well
    */
    if (   (Psf_srvblk->psf_capabilities & PSF_C_C2SECURE) == 0
	|| (   accessmask & SXF_A_FAIL
	    && ~sess_cb->pss_retry & PSS_REPORT_MSGS)
       )
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

    if ((status = sxf_call(SXR_WRITE, &sxfrcb)) == E_DB_OK)
	return (status);

    /*
    **	Log SXF error in the error log
    */
    (VOID) ule_format(sxfrcb.sxf_error.err_code, &syserr, ULE_LOG, NULL,
	    (char *)0, 0L, (i4 *)0, &local_error, 0);
    /*
    **	Call PSFERROR with standard Security Write Error
    */
    (VOID) psf_error(E_PS1005_SEC_WRITE_ERROR, sxfrcb.sxf_error.err_code, PSF_INTERR, &local_error, err, 0);
    return (status);
}

/*
** Name: psy_altaudit
**
** Description:
**	Alter the current security audit state. This is used for the
**	ALTER SECURITY_AUDIT statement, and implements calls to SXF
**	to change the audit state.
**
** Inputs:
**      psy_cb
**	    .psy_auflag			auditing op mask
**	    .psy_aufile			new audit file
**	sess_cb				Pointer to session control block
**					(Can be NULL)
** Outputs:
**	None
**
** Returns
**	DB_STATUS
**
** Side effects:
**	Security audit state is changed.
**
** History:
**	30-jul-93 (robf)
**	   Created
**	20-oct-93 (robf)
**         Updated to allow resume/restart plus change log in a single
**	   SXF call.
**      22-feb-95 (newca01)
**          Added new IF stmt to check for C2 security before
**          ALTER SECURITY_AUDIT for B66592.
*/
static DB_STATUS
psy_altaudit( 
	PSY_CB             *psy_cb,
	PSS_SESBLK	   *sess_cb
)
{
    GLOBALREF PSF_SERVBLK *Psf_srvblk;
    DB_STATUS	    	  status=E_DB_OK;
    DB_STATUS	    	  s;
    SXF_RCB               sxfrcb;
    i4               local_error;
    DB_ERROR    	  err;
    SXF_ACCESS	  	  access;

    for(;;)
    {
	MEfill(sizeof(sxfrcb), NULLCHAR, (PTR)&sxfrcb);
	sxfrcb.sxf_cb_type    = SXFRCB_CB;
	sxfrcb.sxf_length     = sizeof(sxfrcb);

        sxfrcb.sxf_auditstate=0;
	/*
	** Check for any alter operations, only one (or none) of
	** SUSPEND/RESUME/STOP/RESTART may be specified
	*/
	if(psy_cb->psy_auflag & PSY_AUSUSPEND)
	{
	    /* SUSPEND */
	    sxfrcb.sxf_auditstate=SXF_S_SUSPEND_AUDIT;
	}
	else if(psy_cb->psy_auflag & PSY_AURESUME)
	{
	    /* RESUME */
	    sxfrcb.sxf_auditstate=SXF_S_RESUME_AUDIT;
	}
	else if(psy_cb->psy_auflag & PSY_AUSTOP)
	{
	    /* STOP */
	    sxfrcb.sxf_auditstate=SXF_S_STOP_AUDIT;
	}
	else if(psy_cb->psy_auflag & PSY_AURESTART)
	{
	    /* RESTART */
	    sxfrcb.sxf_auditstate=SXF_S_RESTART_AUDIT;
	}
	/*
	** Add change log request if appropriate
	*/
	if(psy_cb->psy_auflag&PSY_AUSETLOG)
	{
		/* WITH AUDIT_LOG */
		sxfrcb.sxf_auditstate|=SXF_S_CHANGE_FILE;
		sxfrcb.sxf_filename=(PTR)psy_cb->psy_aufile;
	}
	/* 22-feb-95 (newca01) B66592 following if stmt added */
	if ( (Psf_srvblk->psf_capabilities & PSF_C_C2SECURE) == 0 )
	{ 
		(VOID) psf_error(E_SX002B_AUDIT_NOT_ACTIVE, 0, 
			PSF_USERERR, 
			&local_error, &psy_cb->psy_error, 0); 
		status = E_DB_ERROR; 
		break;
	}
	/*
	** Call SXF to make the change
	*/
        status = sxf_call(SXS_ALTER, &sxfrcb);
	/*
	** Handle any errors from the call
	*/
	if(status!=E_DB_OK)
	{
	    /*
	    ** Report the SXF error directly
	    */
	    (VOID) psf_error( sxfrcb.sxf_error.err_code, 0, 
		PSF_USERERR, 
		&local_error, &psy_cb->psy_error, 0);
	    break;
	}
	break;
    }
    /*
    ** Audit changing security audit state
    */
    if(status==E_DB_OK)
	access=SXF_A_SUCCESS|SXF_A_ALTER;
    else
	access=SXF_A_FAIL|SXF_A_ALTER;
    s=psy_secaudit(
	FALSE,
	sess_cb,
	"SECURITY_AUDIT",
    	(DB_OWN_NAME *)0,
	sizeof("SECURITY_AUDIT")-1,
	SXF_E_SECURITY,
	I_SX272D_ALTER_SECAUDIT,
	access,
	&err);
    if(s>status)
    {
 	status=s;
    }
   return status;
}
