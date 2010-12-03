/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <ex.h>
#include    <cs.h>
#include    <cv.h>
#include    <tm.h>
#include    <tr.h>
#include    <pc.h>
#include    <st.h>
#include    <lk.h>
#include    <iicommon.h>
#include    <dbdbms.h>

#include    <ddb.h>
#include    <ulf.h>
#include    <ulx.h>
#include    <scf.h>
#include    <qsf.h>	/* for scs control block */
#include    <adf.h>	/* for scs cb */
#include    <gca.h>
#include    <generr.h>
#include    <sqlstate.h>

/* added for scs.h prototypes, ugh! */
#include <dudbms.h>
#include <ulm.h>
#include <dmf.h>
#include <dmccb.h>
#include <dmrcb.h>
#include <copy.h>
#include <qefrcb.h>
#include <qefqeu.h>
#include <qefcopy.h>

#include    <sc.h>
#include    <scc.h>
#include    <scs.h>
#include    <scd.h>
#include    <sc0e.h>
#include    <sc0m.h>
#include    <sce.h>
#include    <scfcontrol.h>

/**
**
**  Name: SC0E.C - Log error messages from SCF
**
**  Description:
**      This routine writes (via ule_format()) error messages to the Ingres 
**      error log.  It is provided as a common interface so that errors may 
**      be related to one another.
**
**          sc0ePutFcn  - write an error message.
**	    sc0e_uput   - write an error message to the user
**
**
**  History:
**      30-jun-1986 (fred)    
**          Created on Jupiter.
**	21-may-89 (jrb)
**	    New ule_format interface changes, and passes generic error code to
**	    scc_error.
**	13-jul-92 (rog)
**	    Changed sc0e_tput so that it can send its output to either
**	    SCC_TRACE or the errlog depending on a mask passed in.
**	12-Mar-1993 (daveb)
**	    Add include <st.h>
**	24-may-1993 (bryanp)
**	    Include <pc.h> before <lk.h> to pick up PID definition.
**	29-Jun-1993 (daveb)
**	    Include <tr.h>, correctly cast arg to CSget_scb.
**	30-jun-1993 (shailaja)
**	    Fixed prototype incompatibilities.
**	2-Jul-1993 (daveb)
**	    prototyped, add sc0e_0_pyt and sc0e_0_uput.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	13-sep-93 (swm)
**	    Include cv.h to pickup CV_HEX_PTR_SIZE define which is now
**	    needed by scs.h.
**      28-Oct-1996 (hanch04)
**          Changed SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS
**	22-Oct-1998 (jenjo02)
**	    Corrected miscasts of CS_SCB to SCF_SESSION. Caused BUS
**	    errors on HP-11.
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**      04-may-1999 (hanch04)
**          Change TRformat's print function to pass a PTR not an i4.
**	30-jun-1999 (hanch04)
**	    Corrected casts removed by cleanup change.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	04-Jan-2001 (jenjo02)
**	    Pass CS_SCB* in scf_session to scf_call() rather than
**	    DB_NOSESSION.
**      23-Apr-2001 (hweho01)
**          Changed sc0e_put() to a varargs function, avoid stack
**          corruptions.
**	10-Jun-2004 (schka24)
**	    Don't use ule-format macro for user message lookups, leaves
**	    little TRdisplay droppings in the DBMS log file.
**	    FIXME there are lots more of these, but I don't have time
**	    to fix them all -- just getting the most annoying one.
**	30-Nov-2006 (kschendel) b122041
**	    Fix ult_check_macro usage.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Deleted sc0e_0_put, sc0e_0_uput functions which
**	    are now macroized to simply pass 0 params to the corresponding
**	    function.
**	30-Mar-2010 (kschendel) SIR 123485
**	    Re-type some ptr's as the proper struct pointer.
**/

/*
**  Forward and/or External function references.
*/

GLOBALREF SC_MAIN_CB *Sc_main_cb;        /* core struct of scf */    

/*{
** Name: sc0ePutFcn	- write an error message to the log
**
** Description:
**      This routine writes an error message to the Ingres error log. 
**      The message, along with its parameters, is sent to ule_format() 
**      augmented with the server id, the session id, and the error sequence 
**      number for the server.
**
** Inputs:
**	dberror				DB_ERROR containing error source info
**      err_code                        an error to override what's in dberror.
**      os_error                        the os error if appropriate
**	FileName			error source file name
**	LineNumber			line number within that file
**	num_params			the number of param pairs to follow
**      param_1_length                  length of ...
**      param_1                         the first parameter
**      ...                             more param pairs
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	30-Jun-1986 (fred)
**          Created on Jupiter
**	28-jan-1989 (jrb)
**	    Added num_params to sc0e_put.
**	09-mar-1989 (jrb)
**	    Now passes num_params arg thru to ule_format.
**	21-may-89 (jrb)
**	    Changed calls to ule_format to pass NULL as 4th parm.
**	13-jul-92 (rog)
**	    Result of call to CSget_sid was never used.  Call and otherwise
**	    unused local variable eliminated.  (Integrating seg's change from
**	    6.4.)
**	25-oct-92 (andre)
**	    interface of ule_format() has changed: instead of generic_error
**	    (i4 *), it expects sqlstate (DB_SQLSTATE *)
**      06-apr-1993 (smc)
**          Cast parameters to match prototypes.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    prototyped.  os_error is properly a CL_ERR_DESC *, not a i4.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Renamed from sc0e_put(), invoked by sc0ePut, sc0e_put,
**	    sc0e_0_put macros.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
[@history_template@]...
*/

VOID
sc0ePutFcn(
	 DB_ERROR *dberror,
	 i4	err_code,
         CL_ERR_DESC *os_error,
	 PTR	FileName,
	 i4	LineNumber,
         i4 num_params,
         ... )
{
    DB_STATUS           status;
    i4             	error;
    DB_ERROR		localDBerror, *DBerror;
#define MAX_PARMS	6
    i4	 		param_size[MAX_PARMS];
    PTR 		param_value[MAX_PARMS];
    i4	 		i; 
    va_list 		ap;

    va_start( ap , num_params );

    for( i = 0;  i < num_params && i < MAX_PARMS; i++ )
    {
        param_size[i] = (i4) va_arg( ap, i4 );
        param_value[i] = ( PTR ) va_arg( ap, PTR );
    }
    va_end( ap );

    /* Check for overriding err_code */
    if ( !dberror || err_code )
    {
        DBerror = &localDBerror;
	DBerror->err_file = FileName;
	DBerror->err_line = LineNumber;
	DBerror->err_code = err_code;
	DBerror->err_data = 0;

	/* Fill caller's dberror with that used */
	if ( dberror )
	    *dberror = *DBerror;
    }
    else
        DBerror = dberror;

    status = uleFormat(DBerror, 0, os_error, ULE_LOG, (DB_SQLSTATE *) NULL, 0,
	0, 0, &error, i, param_size[0], param_value[0], 
        param_size[1], param_value[1], param_size[2], param_value[2],
        param_size[3], param_value[3], param_size[4], param_value[4],
        param_size[5], param_value[5]);

    if (Sc_main_cb)
    {
	if (DB_FAILURE_MACRO(status))
	{
	    status = uleFormat(NULL, E_SC012A_BAD_MESSAGE_FOUND, NULL, ULE_LOG,
		(DB_SQLSTATE *) NULL, 0, 0, 0, &error, 1,
		(DBerror->err_code ? sizeof(DBerror->err_code) : sizeof(os_error)),
		(DBerror->err_code ? (PTR)&DBerror->err_code : (PTR)&os_error));
	    if (DB_FAILURE_MACRO(status))
	    {
		uleFormat(NULL, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL,
		    "E_????_ERROR_LOGGING_&_FORMATTING_MESSAGE",
		     41, 0, &error, 0);
	    }
	}
	if (ult_check_macro(&Sc_main_cb->sc_trace_vector, SCT_SCRLOG, NULL, NULL))
	{
	    SCF_CB			*scf_cb;
	    char			msg_area[DB_ERR_SIZE];    
	    SCD_SCB		*scb;

	    CSget_scb((CS_SCB **)&scb);
	
	    if ((scb) && (scb->scb_sscb.sscb_scccb))
	    {
		scf_cb = scb->scb_sscb.sscb_scccb;
		if (scf_cb)
		{
		    scf_cb->scf_facility = DB_SCF_ID;
		    scf_cb->scf_session = (SCF_SESSION)scb;
		    scf_cb->scf_length = sizeof(SCF_CB);
		    scf_cb->scf_type = SCF_CB_TYPE;
		    status = uleFormat(DBerror, 0, os_error, ULE_LOOKUP,
			(DB_SQLSTATE *) NULL, msg_area, sizeof(msg_area),
			&scf_cb->scf_len_union.scf_blength, 
			&error, num_params, param_size[0],
                        param_value[0], param_size[1], param_value[1], 
                        param_size[2], param_value[2], param_size[3],
                        param_value[3], param_size[4], param_value[4],
                        param_size[5], param_value[5]);
		    if (status == E_DB_OK)
		    {
			scf_cb->scf_ptr_union.scf_buffer = msg_area;
			scf_cb->scf_nbr_union.scf_local_error = DBerror->err_code;
			scf_call(SCC_TRACE, scf_cb);
		    }
		}
	    }
	}
    }
}


/* Used where function pointer needed and FileName, LineNumber unknown */
VOID
sc0e_putAsFcn(i4 err_code,
         CL_ERR_DESC *os_error,
         i4 num_params,
         ... )
{
    i4			ps[MAX_PARMS];
    PTR			pv[MAX_PARMS];
    i4	 		i; 
    va_list 		ap;

    va_start( ap , num_params );

    for( i = 0;  i < num_params && i < MAX_PARMS; i++ )
    {
        ps[i] = (i4) va_arg( ap, i4 );
        pv[i] = ( PTR ) va_arg( ap, PTR );
    }
    va_end( ap );

    sc0ePutFcn(NULL,
	       err_code,
	       os_error,
	       __FILE__,
	       __LINE__,
	       i,
	       ps[0], pv[0],
	       ps[1], pv[1],
	       ps[2], pv[2],
	       ps[3], pv[3],
	       ps[4], pv[4],
	       ps[5], pv[5]);
}

/* Non-variadic form of sc0ePut, insert __FILE__, __LINE__ manually */
VOID
sc0ePutNV(DB_ERROR *dberror,
	 i4 err_code,
         CL_ERR_DESC *os_error,
         i4 num_params,
         ... )
{
    i4			ps[MAX_PARMS];
    PTR			pv[MAX_PARMS];
    i4	 		i; 
    va_list 		ap;

    va_start( ap , num_params );

    for( i = 0;  i < num_params && i < MAX_PARMS; i++ )
    {
        ps[i] = (i4) va_arg( ap, i4 );
        pv[i] = ( PTR ) va_arg( ap, PTR );
    }
    va_end( ap );

    sc0ePutFcn(dberror,
	       err_code,
	       os_error,
	       __FILE__,
	       __LINE__,
	       i,
	       ps[0], pv[0],
	       ps[1], pv[1],
	       ps[2], pv[2],
	       ps[3], pv[3],
	       ps[4], pv[4],
	       ps[5], pv[5]);
}


/*{
** Name: sc0e_uput	- write an error message to the user
**
** Description:
**      This routine writes an error message to the Ingres user.
**      The message, along with its parameters, is sent to ule_format() 
**      augmented with the server id, the session id, and the error sequence 
**      number for the server.
**
** Inputs:
**      err_code                        error code to be reported to user
**      os_error                        the os error if appropriate
**	num_params			the number of param pairs to follow
**      param_1_length                  length of ...
**      param_1                         the first parameter
**      ...                             more param pairs
**
** Outputs:
**      none
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	24-Jul-1986 (fred)
**          Created on Jupiter
**	28-jan-1989 (jrb)
**	    Added num_params to sc0e_uput.
**	09-mar-1989 (jrb)
**	    Now passes num_params arg thru to ule_format.
**	21-may-89 (jrb)
**	    Now passes generic error code to scc_error after looking up thru
**	    ule_format.
**	13-jul-92 (rog)
**	    Added an (unsigned) to the 0xffff0000 mask.
**	25-oct-92 (andre)
**	    interface of ule_format() has changed: instead of generic_error
**	    (i4 *), it expects sqlstate (DB_SQLSTATE *) +
**	    in SCF_CB, scf_generic_error has been replaced with scf_sqlstate
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    Prototyped.  Pass correct type/number of args to sc0e_put.
**	    os_error is CL_ERR_DESC *, not i4.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use new form of uleFormat().
**	    Note that ULE_LOOKUP does not prefix the message text with
**	    the ULE header, hence there's no need for FileName, LineNumber,
**	    or input DB_ERROR.
[@history_template@]...
*/
VOID
sc0e_uput(i4 err_code,
	  CL_ERR_DESC *os_error,
	  i4  num_params,
	  ...)
{
    DB_STATUS           status;
    i4             error;
    i4			msg_owner;
    SCF_CB		*scf_cb;
    char		msg_area[DB_ERR_SIZE];    
    SCD_SCB		*scb;
#define MAX_PARMS	6
    i4	 		param_size[MAX_PARMS];
    PTR 		param_value[MAX_PARMS];
    i4	 		i; 
    va_list 		ap;

    va_start( ap , num_params );

    for( i = 0;  i < num_params && i < MAX_PARMS; i++ )
    {
        param_size[i] = (i4) va_arg( ap, i4 );
        param_value[i] = ( PTR ) va_arg( ap, PTR );
    }
    va_end( ap );

    CSget_scb((CS_SCB **)&scb);
    
    if ((scb) && (scb->scb_sscb.sscb_scccb))
    {
	scf_cb = scb->scb_sscb.sscb_scccb;
	scf_cb->scf_facility = DB_SCF_ID;
	scf_cb->scf_session = (SCF_SESSION)scb;
	scf_cb->scf_length = sizeof(SCF_CB);
	scf_cb->scf_type = SCF_CB_TYPE;
	msg_owner = (err_code & (unsigned)0xffff0000) >> 16;
	status = uleFormat(NULL, err_code, os_error, ULE_LOOKUP,
		&scf_cb->scf_aux_union.scf_sqlstate, msg_area,
		sizeof(msg_area),
		&scf_cb->scf_len_union.scf_blength, 
		&error, i,
		param_size[0], param_value[0], 
		param_size[1], param_value[1], param_size[2], param_value[2],
		param_size[3], param_value[3], param_size[4], param_value[4],
		param_size[5], param_value[5]);
	if (DB_SUCCESS_MACRO(status))
	{
	    scf_cb->scf_ptr_union.scf_buffer = msg_area;
	    scf_cb->scf_nbr_union.scf_local_error = err_code;
	    status = scf_call(SCC_ERROR, scf_cb);
	    if (status != E_DB_OK)
	    {
		sc0ePut(&scf_cb->scf_error, 0, NULL, 0);
	    }
	}
	else
	{
	    char	*sqlstate_str = SS50000_MISC_ERRORS;
	    i4		i;
	    
	    scf_cb->scf_ptr_union.scf_buffer = "Text unavailable";
	    scf_cb->scf_len_union.scf_blength =
		STlength(scf_cb->scf_ptr_union.scf_buffer);
	    scf_cb->scf_nbr_union.scf_local_error = err_code;
	    for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
		scf_cb->scf_aux_union.scf_sqlstate.db_sqlstate[i] =
		    sqlstate_str[i];
	    status = scf_call(SCC_ERROR, scf_cb);
	    if (status != E_DB_OK)
	    {
		sc0ePut(&scf_cb->scf_error, 0, NULL, 0);
	    }
	    sc0e_put(E_SC012A_BAD_MESSAGE_FOUND, 0, 1,
		     (err_code ? sizeof(err_code) : sizeof(os_error)),
		     (err_code ? (PTR)&err_code : (PTR)&os_error),
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0,
		     0, (PTR)0 );
	}
    }
}


/*{
** Name: sc0e_trace	- Send trace statements to the FE
**
** Description:
**      This routine is called similar to TRdisplay(), but 
**      sends the string to the FE.
**
** Inputs:
**	scb				Scb to trace
**      fmt                             Format string (ala TRdisplay())
**      p1, p2, ...                     Parameters
**
** Outputs:
**      None.
**	Returns:
**	    Void
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      20-Jan-88 (fred)
**          Created.
**	13-jul-92 (rog)
**	    Changed sc0e_tput so that it can send its output to either
**	    SCC_TRACE or the errlog depending on a mask passed in.
**	03-jun-1993 (rog)
**	    If sc0e_tput() is sending output to the errlog, it first calls
**	    ulx_format_query() to break up the text in <80 character lines
**	    that are split at reasonable places.
**	29-Jun-1993 (daveb)
**	    correctly cast arg to CSget_scb.
**	2-Jul-1993 (daveb)
**	    Tried to prototype, but callers's are type incorrect.
**	    Easier for now to leave it old-style.
**	13-jul-1993 (rog)
**	    print_mask must be initialized to the correct flags or the trace
**	    output from sc0e_trace() will be silently discarded.  (bug 53118)
**	11-oct-93 (swm)
**	    Bug #56448
**	    Altered sc0e_trace calls to conform to new portable interface.
**	    It is no longer a "pseudo-varargs" function. It cannot become a
**	    varargs function as this practice is outlawed in main line code.
**	    Instead it takes a formatted buffer as its only parameter.
**	    Each call which previously had additional arguments has an
**	    STprintf embedded to format the buffer to pass to sc0e_trace.
**	    This works because STprintf is a varargs function in the CL.
**      27-Jun-2007 (kiria01) b118501
**          The previous change did not consider calls to this function where
**          it was used as a parameter function such as with calls from
**          adu_2prvalue - not all of these explicitly pass through STprintf.
**          Specificly it is possible for a raw databuffer to be presented which
**          might have character sequences that could be mistaken for format
**          specifiers. To make sure this is handled, we now pass via "%s".
**	17-Nov-2010 (kschendel) SIR 124685
**	    Drop the add-newline stuff from sc0e_tput, it was never called
**	    that way, and with the prototype cleanups, we can now get NULL
**	    to it.  The client side (almost???) always appends its own newline,
**	    so no need to do it here anyway.
**	    The whole newline handling for messages and tracing is a mess!
*/
VOID
sc0e_trace( char *fmt )
{
    char		buffer[SC0E_FMTSIZE]; /* last char for `\n' */
    i4		print_mask = SCE_FORMAT_NEEDNUL | SCE_FORMAT_GCA;

    /* TRformat removes `\n' chars, so to ensure that psf_scctrace()
    ** outputs a logical line (which it is supposed to do), we allocate
    ** a buffer with one extra char for NL and will hide it from TRformat
    ** by specifying length of 1 byte less. The NL char will be inserted
    ** at the end of the message by scs_tput().
    */
    TRformat(sc0e_tput, &print_mask, buffer, sizeof(buffer) - 1, "%s", fmt);
}

/*
** History:
**	2-Jul-1993 (daveb)
**	    prototyped.
*/
i4
sc0e_tput( PTR arg1, i4 msg_length, char *msg_buffer )
{
    SCF_CB       *scf_cb;
    SCD_SCB	 *scb;
    DB_STATUS    status = E_DB_OK;
    i4	 mask;
    i4		 err_code;

    if (msg_length == 0)
	return;

    if (arg1 == (PTR)0 || arg1 == (PTR)1)
	mask = SCE_FORMAT_GCA;
    else
	mask = *((i4 *) arg1);

    if (mask & SCE_FORMAT_GCA)
    {
	if (mask & SCE_FORMAT_NEEDNUL)
	{
	    /* Add a NL character */
	    *((char *)(msg_buffer + msg_length)) = '\n';
	    msg_length++;
	}    
    
	CSget_scb((CS_SCB **)&scb);

	scf_cb = scb->scb_sscb.sscb_scccb;
	scf_cb->scf_length = sizeof(*scf_cb);
	scf_cb->scf_type = SCF_CB_TYPE;
	scf_cb->scf_facility = DB_PSF_ID;
	scf_cb->scf_len_union.scf_blength = (u_i4)msg_length;
	scf_cb->scf_ptr_union.scf_buffer = msg_buffer;
	scf_cb->scf_session = (SCF_SESSION)scb;	/* print to current session */

	status = scf_call(SCC_TRACE, scf_cb);
	err_code = scf_cb->scf_error.err_code;
    }
    else if (mask & SCE_FORMAT_ERRLOG)
    {
	char		out_buffer[2048];
	i4		err_code;
    
	if (mask & SCE_FORMAT_NEEDNUL)
	{
	    /* Add a NL character */
	    *((char *)(msg_buffer + msg_length)) = '\n';
	    msg_length++;
	}    

	ulx_format_query(msg_buffer, msg_length, out_buffer,
			 sizeof(out_buffer));

	msg_length = STlength(out_buffer);

	status = uleFormat(NULL, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL, out_buffer,
	    msg_length, 0, &err_code, 0);
    }

    if (status != E_DB_OK)
    {
	TRdisplay("SCS_TPUT: SCF error %d displaying query text to user\n",
	    err_code);
    }
}
