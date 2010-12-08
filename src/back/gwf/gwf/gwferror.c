/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <qu.h>
#include    <sl.h>
#include    <st.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <dmf.h>
#include    <dmrcb.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <er.h>
#include    <me.h>
#include    <scf.h>
#include    <ulm.h>
#include    <gwf.h>
#include    <gwfint.h>

/**
**
**  Name: GWFERROR.C - Error formatting and reporting functions.
**
**  Description:
**      This file contains the error formatting and reporting functions for 
**      the gateway (GWF) facility.
**
**          gwfErrorFcn - Format and report a gateway facility error.
**	    gwf_errsend - Format error message and send to frontend
**
**  History:
**      15-mar-90 (linda)
**          written -- modeled on psferror.c
**	14-jun-1990 (bryanp)
**	    Added gwf_errsend. This routine allows the caller to send an error
**	    message to the frontend, which is useful when we want to report the
**	    error at a low (GWF) level and return "ALREADY_REPORTED" to our
**	    caller (DMF).
**      18-may-92 (schang)
**          GW merge
**	8-oct-92 (daveb)
**	    prototyped
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      23-jul-93 (schang)
**          add st.h
**	11-Aug-1998 (hanch04)
**	    Removed declaration of ule_format
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: gwfErrorFcn	- Format and report a gateway facility error.
**
** Description:
**      This function formats and reports a gwf facility error.  There are 
**      three types of errors: user errors, internal errors, and caller errors.
**	User errors are not logged, but the text is sent to the user.  Internal
**	errors are logged , but the text is not sent to the user.  Caller errors
**	are neither logged nor sent to the user.  Sometimes there will be
**      an operating system error associated with the error; this can be passed 
**      as a separate parameter.
**
**	The error number will be put into the error control block in the
**	control block used to call the parser facility.  User errors will be
**	recorded in the error block as E_PS0001_USER_ERROR, because the caller
**	wants to know if the user made a mistake, but doesn't particularly care
**	what the mistake was.  Internal errors are recorded in the error block
**	as E_PS0002_INTERNAL_ERROR, because the caller doesn't need to know
**	the particulars of why the parser facility screwed up (more specific
**	information will be recorded in the log.  Caller errors will be recorded
**	in the error block as is.
**
** Inputs:
**      errorno                         Error number
**      detail				Error number that caused this error
**					(if, e.g., the error is of type
**					GWF_INTERR, generated after a bad
**					status return from a call to another
**					facility, then this would be the error
**					code from the control block for that
**					facility)
**      err_type                        Type of error
**					    GWF_USERERR - user error
**					    GWF_INTERR  - internal error
**					    GWF_CALLERR - caller error
**	err_code			Ptr to reason for error return, if any
**	err_blk				Pointer to error block
**	num_parms			Number of parameter pairs to follow.
** NOTE: The following parameters are optional, and come in pairs.  The first
**  of each pair gives the length of a value to be inserted in the error
**  message.  The second of each pair is a pointer to the value to be inserted.
**      plen1				First length parameter, if any
**      parm1				First error parameter, if any
**      plen2				Second lengthparameter, if any
**      parm2				Second error parameter, if any
**      plen3				Third length parameter, if any
**      parm3				Third error parameter, if any
**	plen4				Fourth length parameter, if any
**	parm4				Fourth error parameter, if any
**	plen5				Fifth length parameter, if any
**	parm5				Fifth error parameter, if any
**
** Outputs:
**      err_code                        Filled in with reason for error return
**	err_blk				Filled in with error
**	Returns:
**	    E_DB_OK			Success
**	    E_DB_ERROR			Error by caller
**	    E_DB_FATAL			Couldn't format or report error
**	Exceptions:
**	    none
**
** Side Effects:
**	    Can cause message to be logged or sent to user.
**
** History:
**      15-mar-90 (linda)
**          written -- modeled on psf_error()
**	10-jul-91 (rickh)
**	    added support for user errors.
**	24-oct-92 (andre)
**	    ule_format() will return an SQLSTATE status code instead of generic
**	    error message.  SCC_ERROR will know how to handle SQLSTATEs
**	03-Nov-2010 (jonj) SIR 124685 Prototype Cleanup
**	    Rewrote gwf_error() with variable parms for prototyping.
**	    Replace ule_format with uleFormat.
**	    gwfErrorFcn() called by way of gwf_error macro.
*/
/* VARARGS5 */
VOID
gwfErrorFcn(
i4		err_code,
i4		err_type,
PTR		FileName,
i4		LineNumber,
i4		num_parms,
... )
{
#define	NUM_ER_ARGS	12
    i4		    uletype;
    DB_STATUS		    uleret;
    i4		    ulecode;
    i4		    msglen;
    DB_STATUS		    ret_val = E_DB_OK;
    char		    errbuf[DB_ERR_SIZE];
    SCF_CB		    scf_cb;
    DB_ERROR	    localDBerror, *DBerror;
    i4		    i;
    va_list	    ap;
    ER_ARGUMENT	    er_args[NUM_ER_ARGS];

    va_start( ap, num_parms );
    for ( i = 0; i < num_parms && i < NUM_ER_ARGS; i++ )
    {
        er_args[i].er_size = (i4) va_arg( ap, i4 );
	er_args[i].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    DBerror = &localDBerror;
    DBerror->err_file = FileName;
    DBerror->err_line = LineNumber;
    DBerror->err_code = err_code;
    DBerror->err_data = 0;

    for (;;)	/* Not a loop, just gives a place to break to */
    {
	/*
	** Log internal errors, don't log user errors or caller errors.
	*/
	if (err_type == GWF_INTERR)
	    uletype = ULE_LOG;
	else
	    uletype = 0L;

	/*
	** Get error message text.  Don't bother with caller errors, since
	** we don't want to report them or log them.
	*/
	if (err_type != GWF_CALLERR)
	{
	    uleret = uleFormat(DBerror, 0, 0L, uletype,
		&scf_cb.scf_aux_union.scf_sqlstate, errbuf,
		(i4) sizeof(errbuf), &msglen, &ulecode, i,
		er_args[0].er_size, er_args[0].er_value,
		er_args[1].er_size, er_args[1].er_value,
		er_args[2].er_size, er_args[2].er_value,
		er_args[3].er_size, er_args[3].er_value,
		er_args[4].er_size, er_args[4].er_value,
		er_args[5].er_size, er_args[5].er_value,
		er_args[6].er_size, er_args[6].er_value,
		er_args[7].er_size, er_args[7].er_value,
		er_args[8].er_size, er_args[8].er_value,
		er_args[9].er_size, er_args[9].er_value,
		er_args[10].er_size, er_args[10].er_value,
		er_args[11].er_size, er_args[11].er_value );

	    /*
	    ** If ule_format failed, we probably can't report any error.
	    ** Instead, just propagate the error up to the user.
	    */
	    if (uleret != E_DB_OK)
	    {
		STprintf(errbuf, "Error message corresponding to %d (%x) \
		    not found in INGRES error file", err_code, err_code);
		msglen = STlength(errbuf);
		uleFormat(NULL, 0L, 0L, ULE_MESSAGE, NULL, errbuf, msglen, 0,
		    &ulecode, 0);
		break;
	    }
	}

	/*
	** Only send text of user errors to user.  Internal error has already
	** been logged by ule_format.  This logic keys off of err_type, which
	** must be GWF_USERERR.  If a further error occurs, it calls this
	** routine again with err_type = GWF_INTERR.  If you change this,
	** ponder the possibilities of infinite recursion.
	*/

	if (err_type == GWF_USERERR)
	{

	    scf_cb.scf_length = sizeof(scf_cb);
	    scf_cb.scf_type = SCF_CB_TYPE;
	    scf_cb.scf_facility = DB_GWF_ID;
	    scf_cb.scf_session = DB_NOSESSION;
	    scf_cb.scf_nbr_union.scf_local_error = err_code;
	    scf_cb.scf_len_union.scf_blength = msglen;
	    scf_cb.scf_ptr_union.scf_buffer = errbuf;
	    ret_val = scf_call(SCC_ERROR, &scf_cb);

	    if ( ret_val != E_DB_OK)
	    {
		gwf_error( scf_cb.scf_error.err_code, GWF_INTERR, 0 );
	    }

	}	/* end if err_type	*/

	break;
    }

    return ;
}
/* Non-variadic form of gwf_error, insert __FILE__, __LINE__ manually */
VOID
gwfErrorNV(
	 i4 err_code,
         i4 err_type,
         i4 num_parms,
         ... )
{
    i4	 		i; 
    va_list 		ap;
    ER_ARGUMENT	        er_args[NUM_ER_ARGS];

    va_start( ap , num_parms );

    for( i = 0;  i < num_parms && i < NUM_ER_ARGS; i++ )
    {
        er_args[i].er_size = (i4) va_arg( ap, i4 );
	er_args[i].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    gwfErrorFcn(
	        err_code,
	        err_type,
	        __FILE__,
	        __LINE__,
	        i,
		er_args[0].er_size, er_args[0].er_value,
		er_args[1].er_size, er_args[1].er_value,
		er_args[2].er_size, er_args[2].er_value,
		er_args[3].er_size, er_args[3].er_value,
		er_args[4].er_size, er_args[4].er_value,
		er_args[5].er_size, er_args[5].er_value,
		er_args[6].er_size, er_args[6].er_value,
		er_args[7].er_size, er_args[7].er_value,
		er_args[8].er_size, er_args[8].er_value,
		er_args[9].er_size, er_args[9].er_value,
		er_args[10].er_size, er_args[10].er_value,
		er_args[11].er_size, er_args[11].er_value );
}

/*
** Name: gwf_errsend	- send Ingres error message to front end
**
** Description:
**	This routine sends a Ingres error message to the frontend. We format
**	the msg with ule_format, then send it to the frontend using scc_error().
**
** Inputs:
**	err_code		- the error message number
**
** Outputs:
**	None
**
** Returns:
**	E_DB_OK			- message sent successfully.
**	E_DB_ERROR		- something went wrong.
**
** History:
**	14-jun-1990 (bryanp)
**	    Created.
**	8-oct-92 (daveb)
**	    prototyped
**	24-oct-92 (andre)
**	    replace call to ERlookup() with a call to ERslookup()
**	08-sep-93 (swm)
**	    Changed sid type from i4  to CS_SID to match recent CL
**	    interface revision.
**	    moved cs.h include higher up above other header files that
**	    need the new CS_SID definition.
**
*/
DB_STATUS
gwf_errsend( i4 err_code )
{
    CS_SID	sid;
    SCF_CB	scf_cb;
    STATUS	tmp_status;
    i4		msglen;
    CL_ERR_DESC	sys_err;
    char	errmsg[ER_MAX_LEN];
    DB_STATUS	status;

    tmp_status  = ERslookup(err_code, 0, ER_TIMESTAMP,
				scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate,
				errmsg, sizeof(errmsg), -1,
				&msglen, &sys_err, (i4)0,
				(ER_ARGUMENT *)0);

    if (tmp_status != OK)
    {
	return (E_DB_ERROR);
    }

    CSget_sid( &sid );

    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_length = sizeof(SCF_CB);
    scf_cb.scf_facility = DB_GWF_ID;
    scf_cb.scf_session = sid;	    /* was: DB_NOSESSION */
    scf_cb.scf_nbr_union.scf_local_error = err_code;
    scf_cb.scf_len_union.scf_blength = msglen;
    scf_cb.scf_ptr_union.scf_buffer = errmsg;
    status = scf_call(SCC_ERROR, &scf_cb);
    if (status != E_DB_OK)
    {
	gwf_error(scf_cb.scf_error.err_code, GWF_INTERR, 0);
	gwf_error(E_GW0303_SCC_ERROR_ERROR, GWF_INTERR, 0);

	return (E_DB_ERROR);
    }

    return (E_DB_OK);
}
