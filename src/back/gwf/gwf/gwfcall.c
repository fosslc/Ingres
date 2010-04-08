/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <ex.h>
#include    <me.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ulf.h>
#include    <adf.h>
#include    <scf.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <ulm.h>
#include    <ulx.h>
#include    <gwf.h>
#include    <gwfint.h>

/**
** Name: GWFCALL.C	- The main GWF entry point
**
** Description:
**	This file contains gwf_call(), which is the main entry point to
**	GWF and the means by which all other facilities invoke GWF
**	services.
**
**	Routines:
**
**	gwf_call	    - Primary GWF entry point.
**
** History:
**	6-apr-90 (bryanp)
**	    Created.
**	4-feb-91 (linda)
**	    Code cleanup -- in particular, update structure member names.
**      18-may-92 (schang)
**          GW merge
**	8-oct-92 (daveb)
**	    include <gwfint.h> for prototypes of called functions.
**      16-nov-92 (schang)
**          prototype
**	18-dec-92 (robf)
**	    Add call to verify gateway id/name (GWU_VALIDGW).
**	    Improve exception handling to new standard by calling
**	    ulx_exception().
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      26-jul-93 (schang)
**          reset gwf interrupt handler when enter GWF
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Add support for GWS_ALTER opcode to alter the session's
**	    case translation semantics.
**	28-Jul-1993 (daveb)
**	    Work around segv on init from checking interrupt handling
**	    when there's no session.
**	26-Oct-1993 (daveb) 55487
**	    Fixed interrupt handling problems.   If interrupt is set in
**	    one query, it is not reset, and entry to second query can
**	    return interrupted.  (For instance, 'help' followed by a
**	    select from a GWF table.)  We need to reset the interrupt
**	    field on entry in certain operations, and check them later.
**	14-Apr-2008 (kschendel)
**	    Teach GWF about handling ADF interrupts and errors from
**	    row-qualifier functions.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Use new form of uleFormat().
*/

/*
** Definition of static variables and forward static functions.
*/

static	STATUS	gwf_handler();	/* GWF catch-all exception handler. */

/*{
** Name: gwf_call	- Primary GWF entry point.
**
** Description:
**	This routine checks that the arguments to gwf_call look reasonable
**	and that the appropriate control block type and size is passed.
**	Then it calls the appropriate GWF function to execute the desired
**	operation and the operation completion status is returned to the
**	caller.
**
** Inputs:
**	operation	    - The GWF operation code
**	control_block	    - The GWF control block (currently GW_RCB always)
**
** Outputs:
**	None
**
** Returns:
**	DB_STATUS
**
** Exceptions & Side Effects:
**	None
**
** History:
**	6-apr-90 (bryanp)
**	    Created.
**	8-oct-92 (daveb)
**	    removed dead variable "err_code"
**	18-dec-92 (robf)
**	    Add operation gwu_validgw.
**      06-jul-93 (schang)
**          reset gwf interrupt handler when enter GWF
**	27-jul-93 (ralph)
**	    DELIM_IDENT:
**	    Add support for GWS_ALTER opcode to alter the session's
**	    case translation semantics.
**	28-Jul-1993 (daveb)
**	    Work around segv on init from checking interrupt handling
**	    when there's no session.
**	26-Oct-1993 (daveb) 55487
**	    Fixed interrupt handling problems correctly, solving
**	    riddled left by the 06-jul-92 and 28-jul-92 change and
**	    backout.  If interrupt is set in one query, it is not
**	    reset, and entry to second query can return interrupted.
**	    (For instance, 'help' followed by a select from a GWF
**	    table.)  We need to reset the interrupt field on entry in
**	    certain operations, and check them later.  That's why we
**	    tried to call gws_check_interupt on entry.  But this is
**	    only correct when there is a session already; so call
**	    check in the likely places inside the switch instead of
**	    once for everyone.
*/
DB_STATUS
gwf_call
(
    GWF_OPERATION	operation,
    GW_RCB		*control_block
)
{
    EX_CONTEXT	context;
    DB_STATUS	status;

    /*
    ** Make some simple checks for bad parameters and incorrect operations.
    */

    if (control_block == (GW_RCB *)0)
    {
	/* not much we can do to log an error... */
	return (E_DB_ERROR);
    }

    if (operation < GWF_MIN_OPERATION || operation > GWF_MAX_OPERATION)
    {
	SETDBERR(&control_block->gwr_error, 0, E_GW0660_BAD_OPERATION_CODE);
	return (E_DB_ERROR);
    }

    if (control_block->gwr_type != GWR_CB_TYPE)
    {
	SETDBERR(&control_block->gwr_error, 0, E_GW0661_BAD_CB_TYPE);
	return (E_DB_ERROR);
    }

    if (control_block->gwr_length != sizeof(GW_RCB))
    {
	SETDBERR(&control_block->gwr_error, 0, E_GW0662_BAD_CB_LENGTH);
	return (E_DB_ERROR);
    }

    /*
    ** Plant an exception handler so that if any lower-level GWF routine
    ** aborts, we can attempt to trap the error and "do the right thing".
    */
    CLRDBERR(&control_block->gwr_error);
    if (EXdeclare( gwf_handler, &context) != OK)
    {
	/*
	** We get here if an exception occurs and the handler longjumps
	** back to us (returns EXDECLARE). In this case, something HORRID
	** has occurred, so let's complain loudly to our caller:
	*/
	EXdelete();
	if (control_block->gwr_error.err_code == 0)
	{
	    SETDBERR(&control_block->gwr_error, 0, E_GW0663_INTERNAL_ERROR);
	    return (E_DB_FATAL);
	}
	/* Well, maybe not horrid, maybe an arithmetic exception.
	** Return these to the caller, although less violently.
	*/
	return (E_DB_ERROR);
    }

    /*
    ** Now call the appropriate GWF routine based on the operation code.
    ** Return the status from the GWF routine to the caller.
    */
    switch( operation )
    {
	case GWF_INIT:
	    status = gwf_init( control_block );
	    break;

	case GWF_TERM:
	    status = gwf_term( control_block );
	    break;

	case GWF_SVR_INFO:
	    status = gwf_svr_info( control_block );
	    break;

	case GWS_INIT:
	    status = gws_init( control_block ); break;

	case GWS_TERM:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gws_term( control_block );
	    break;

	case GWS_ALTER:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gws_alter( control_block );
	    break;

	case GWT_REGISTER:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwt_register( control_block );
	    break;

	case GWT_REMOVE:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwt_remove( control_block );
	    break;

	case GWT_OPEN:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwt_open( control_block );
	    break;

	case GWT_CLOSE:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwt_close( control_block );
	    break;

	case GWI_REGISTER:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwi_register( control_block );
	    break;

	case GWI_REMOVE:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwi_remove( control_block );
	    break;

	case GWR_POSITION:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwr_position( control_block );
	    break;

	case GWR_GET:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwr_get( control_block );
	    break;

	case GWR_PUT:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwr_put( control_block );
	    break;

	case GWR_REPLACE:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwr_replace( control_block );
	    break;

	case GWR_DELETE:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwr_delete( control_block );
	    break;

	case GWX_BEGIN:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwx_begin( control_block );
	    break;

	case GWX_ABORT:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwx_abort( control_block );
	    break;

	case GWX_COMMIT:
	    (void) gws_check_interrupt(
			(GW_SESSION *)control_block->gwr_session_id);
	    status = gwx_commit( control_block );
	    break;

	case GWU_INFO:
	    status = gwu_info( control_block );
	    break;
	
	case GWU_VALIDGW:
	    status = gwu_validgw( control_block );
	    break;

	default:
	    /*
	    ** Falling through means that our case statement is out of sync with
	    ** the known GWF operations and indicates a severe coding error.
	    */
	    SETDBERR(&control_block->gwr_error, 0, E_GW0664_UNKNOWN_OPERATION);
	    status = E_DB_ERROR;
	    break;
    }

    EXdelete();
    return (status);
}

/*{
** Name: gwf_handler	    - GWF exception handler
**
** Description:
**	This routine will catch all GWF exceptions.
**
**	If we might be executing a row-qualifier function, we'll check
**	for arithmetic exceptions and handle them
**
** Inputs:
**	ex_args		    - Exception arguments, as called by EX
**
** Outputs:
**	None
**
** Returns:
**	EXDECLARE
**
** Exceptions and Side Effects:
**	The EXDECLARE return causes an abrupt return to gwf_call()
**
** History:
**	6-apr-90 (bryanp)
**	    Created.
**	10-jul-91 (rickh)
**	    Corrected bug in buffer length found by the ICL port.
**	18-dec-92 (robf)
**	    Upgrade to new handling by calling ulx_exception.
**	14-Apr-2008 (kschendel)
**	    Catch ADF errors if we have an adf-cb around, and we're calling
**	    a row qualification function.  Arithmetic errors should not
**	    be fatal in that case.  (We're no longer switching to QEF
**	    signal context for every qualification call.)
*/

static STATUS
gwf_handler
(
    EX_ARGS	    *ex_args
)
{
    CS_SID	sid;
    DB_STATUS	sts;
    GW_SESSION	*scb;
    SCF_CB	scf_cb;
    SCF_SCI	sci_list[1];

    /* Get the GWF session block for the current session.  GWF doesn't
    ** do the clever stuff that allows GET_DML_SCB in DMF (for example),
    ** so do it the hard way by asking SCF.
    */

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_ptr_union.scf_sci = (SCI_LIST *) sci_list;
    scf_cb.scf_len_union.scf_ilength = 1;
    sci_list[0].sci_length = sizeof(PTR);
    sci_list[0].sci_code = SCI_SCB;
    sci_list[0].sci_aresult = (PTR)(&scb);
    sci_list[0].sci_rlength = NULL;
    sts = scf_call(SCU_INFORMATION, &scf_cb);
    if (sts == E_DB_OK && scb->gws_qfun_adfcb != NULL)
    {
	/* Qualifier execution context, try arithmetic exceptions */

	ADF_CB *adfcb = scb->gws_qfun_adfcb;
	i4 err_code;

	sts = adx_handler(adfcb, ex_args);
	err_code = adfcb->adf_errcb.ad_errcode;
	if (sts == E_DB_OK || err_code == E_AD0001_EX_IGN_CONT
	    || err_code == E_AD0115_EX_WRN_CONT) /* handle warning messages */
	{
	    return(EXCONTINUES);
	}
	/* Tell GWFCALL so that it doesn't crash out */
	scb->gws_qfun_errptr->err_code = err_code;
	if (adfcb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	{
	    SCF_CB		scf_cb;

	    scf_cb.scf_facility = DB_GWF_ID;
	    scf_cb.scf_session = DB_NOSESSION;
	    scf_cb.scf_length = sizeof(SCF_CB);
	    scf_cb.scf_type = SCF_CB_TYPE;

	    scf_cb.scf_ptr_union.scf_buffer = adfcb->adf_errcb.ad_errmsgp;
	    scf_cb.scf_len_union.scf_blength = adfcb->adf_errcb.ad_emsglen;
	    scf_cb.scf_nbr_union.scf_local_error = adfcb->adf_errcb.ad_usererr;
	    STRUCT_ASSIGN_MACRO(
		adfcb->adf_errcb.ad_sqlstate,
		scf_cb.scf_aux_union.scf_sqlstate);
			
	    (VOID) scf_call(SCC_ERROR, &scf_cb);
	    scb->gws_qfun_errptr->err_code = E_GW0023_USER_ERROR;
	    /* The errdata is so that QEF can set iierrornumber. */
	    scb->gws_qfun_errptr->err_data = adfcb->adf_errcb.ad_usererr;
	}
	/* Session-CB error members are no longer of interest as we're about
	** to exit GWF, clean them up now.
	*/
	scb->gws_qfun_adfcb = NULL;
	scb->gws_qfun_errptr = NULL;
	return (EXDECLARE);
    }

    /*
    **	Report the exception
    */
    ulx_exception (ex_args, DB_GWF_ID, E_GW0668_UNEXPECTED_EXCEPTION, TRUE);

    return (EXDECLARE);
}

/*
** Name: gwf_adf_error -- Handle ADF error from user CX in GWF
**
** Description:
**
**	This routine is analogous to qef_adf_error, and in fact was
**	cut-n-pasted from it.  The idea is to analyze an ADF error
**	returned from executing a row qualifier function, probably
**	a CX but not necessarily.  Normal user-level errors are
**	issued to the user now, so that they don't get turned into
**	scary looking (and information-free) GWF generic errors.
**
** Inputs:
**	adf_errcb		The ADF_ERROR part of the ADF cb used during
**				execution of the qualifier function.
**	status			The status returned by the qualifier.
**	scb			The thread's GW_SESSION session control block
**	err_code		An output
**
** Outputs:
**	err_code		Set to translated error to be passed back up.
**				If "just" a user error, return GW0023,
**				which QEF translates to QE0025, which means
**				error message was already issued.
**
**	If GW0023, also set scb->gws_qfun_errptr->err_data to the ADF
**	"user" error code, which ultimately gets sent back to QEF who
**	uses it to set the DBP iierrornumber variable.
**
**	Returns E_DB_OK or E_DB_ERROR
**
** History:
**	14-Apr-2008 (kschendel)
**	    Trying to get GWF/DMF row qualifier out of QEF context.
*/

DB_STATUS
gwf_adf_error(
	ADF_ERROR	*adf_errcb,
	DB_STATUS	status,
	GW_SESSION	*scb,
	i4		*err_code)
{
    DB_STATUS	ret_status;
    i4		error;
    i4		temp;
    SCF_CB	scf_cb;

    /* Make sure it was an ADF error, since unlike QEF we're not always
    ** 100% sure that we're calling a CX for qualification.
    ** Also make sure we have some sort of user context.
    */
    if (scb == NULL
      || (adf_errcb->ad_errclass == 0 && adf_errcb->ad_errcode == 0) )
    {
	*err_code = E_GW020B_GWR_GET_ERROR;
	return (E_DB_ERROR);
    }

    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = DB_NOSESSION;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_type = SCF_CB_TYPE;

    if (adf_errcb->ad_errclass == ADF_INTERNAL_ERROR)
    {
	if ( (adf_errcb->ad_errcode == E_AD550A_RANGE_FAILURE) ||
	     (adf_errcb->ad_errcode == E_AD2005_BAD_DTLEN ) )
	{
	    *err_code = E_AD2005_BAD_DTLEN;
	    if (adf_errcb->ad_errcode != E_AD2005_BAD_DTLEN)
		*err_code = E_GW020B_GWR_GET_ERROR;

	    ret_status = E_DB_ERROR;
	}
	else if (adf_errcb->ad_errcode == E_AD7010_ADP_USER_INTR)
	{
	    *err_code = E_GW0341_USER_INTR;
	    ret_status = E_DB_ERROR;
	}
	else
	{
	    if (adf_errcb->ad_emsglen == 0)
		TRdisplay("%@ gwf_adf_error: ADF returned %d (%x) with no message\n",
				adf_errcb->ad_errcode, adf_errcb->ad_errcode);
	    uleFormat(NULL, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL,
		adf_errcb->ad_errmsgp, adf_errcb->ad_emsglen, &temp, &error, 0);
	    *err_code = E_GW020B_GWR_GET_ERROR;
	    ret_status = E_DB_ERROR;
	}
    }
    else
    {
	if (status == E_DB_WARN)
	    ret_status = E_DB_OK;
	else
	    ret_status = E_DB_ERROR;
	scf_cb.scf_ptr_union.scf_buffer  = adf_errcb->ad_errmsgp;
	scf_cb.scf_len_union.scf_blength = adf_errcb->ad_emsglen;
	scf_cb.scf_nbr_union.scf_local_error = adf_errcb->ad_usererr;
	STRUCT_ASSIGN_MACRO(adf_errcb->ad_sqlstate,
			    scf_cb.scf_aux_union.scf_sqlstate);
	if (adf_errcb->ad_errclass == ADF_USER_ERROR)
	{
	    (VOID) scf_call(SCC_ERROR, &scf_cb);
	}
	else
	{
	    (VOID) scf_call(SCC_RSERROR, &scf_cb);
	}
	*err_code = E_GW0023_USER_ERROR;
	if (scb->gws_qfun_errptr != NULL)
	{
	    scb->gws_qfun_errptr->err_code = E_GW0023_USER_ERROR;
	    scb->gws_qfun_errptr->err_data = adf_errcb->ad_usererr;
	}
    }

    return (ret_status);
}
