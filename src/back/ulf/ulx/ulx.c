/*
**Copyright (c) 2004 Ingres Corporation
*/
#include    <compat.h>
#include    <gl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <cs.h>
#include    <ex.h>
#include    <er.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <ulx.h>
#include    <adf.h>
#include    <scf.h>
#include    <generr.h>
#include    <sqlstate.h>

/*
**
**  Name: ULX.C - General-purpose error handling routines
**
**  Description:
**	This file contains utility routines for handling exceptions and
**      reporting errors
**	    ulx_sccerror - report message to user and/or log message
**	    ulx_rverror	- this routine will report another facility's error
**	     along with a "no parameter" error
**
**
**  History:
**      09-apr-88 (seputis)
**	  initial creation
**	13-aug-92 (rog)
**	    Removed ulx_declare, make ulx_exception more generic, prototyped.
**	12-mar-1993 (rog)
**	    Included <me.h>.
**	29-jun-1993 (rog)
**	    Include <tr.h> to pick up prototypes.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	09-aug-1993 (walt)
**	    In ulx_exception, the newly added exception number needs its
**	    address passed to ule_format rather than its value.
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
**      28-Oct-1996 (hanch04)
**          Changed SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS
**      24-Sep-1999 (hanch04)
**          Call ERmsg_hdr to format error message header and print out
**	    more info for exception.
**	    
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Nov-2010 (frima01) SIR 124685
**	    Added include of er.h with ERmsg_hdr prototype.
*/


/*{
** Name: ulx_sccerror	- report message to user
**
** Description:
**      Report a message to the user
**
** Inputs:
**      status			  status of error
**	sqlstate		  sqlstate status code
**      msg_buffer		      ptr to message
**      error			   ingres error number
**
** Outputs:
**	Returns:
**	    DB_STATUS
**	Exceptions:
**	    none
**
** Side Effects:
**	  Fatal status will cause query to be aborted, and previous text sent
**	  to user to be flushed.
**
** History:
**      15-apr-87 (seputis)
**	  initial creation
**	21-may-89 (jrb)
**	    added "generic_error" to the interface
**	13-aug-92 (rog)
**	    Prototyped.
**	25-oct-92 (andre)
**	    replaced generic_error with sqlstate in the function's interface +
**	    in SCF_CB, scf_generic_error has been replaced with scf_sqlstate
**	13-nov-92 (ed)
**	    In ulx_sccerror(), if not passed sqlstate, set scf_sqlstate to
**	    MISC_INGRES_ERRORS sqlstate.
[@history_template@]...
*/
DB_STATUS
ulx_sccerror( DB_STATUS status, DB_SQLSTATE *sqlstate, DB_ERRTYPE error,
	      char *msg_buffer, i4  msg_length, DB_FACILITY facility )
{
    SCF_CB		scf_cb;
    DB_STATUS		scf_status;
    i4		scc_operation;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = facility;
    scf_cb.scf_nbr_union.scf_local_error = error;
    if (sqlstate)
	STRUCT_ASSIGN_MACRO((*sqlstate), scf_cb.scf_aux_union.scf_sqlstate);
    else
        MEcopy((PTR) SS50000_MISC_ERRORS, DB_SQLSTATE_STRING_LEN,
	       (PTR) scf_cb.scf_aux_union.scf_sqlstate.db_sqlstate);
    scf_cb.scf_len_union.scf_blength = msg_length;
    scf_cb.scf_ptr_union.scf_buffer = msg_buffer;
    scf_cb.scf_session = DB_NOSESSION;

    if (DB_SUCCESS_MACRO(status))
	/* Print warnings to user, but do not terminate processing */
	scc_operation = SCC_TRACE;
    else
	scc_operation = SCC_ERROR;

    scf_status = scf_call(scc_operation, &scf_cb);

    if (DB_FAILURE_MACRO(scf_status))
    {
	TRdisplay("SCF error displaying message to user or log\n");
	TRdisplay("message is :%s",msg_buffer);
    }
    return (scf_status);
}

/*{
** Name: ulx_rverror	- this routine will report another facility's error
**
** Description:
**      This routine will log an error from another facility and the related
**      ULF error.  
**
** Inputs:
**      errorid			 null terminated identification string
**				      e.g. "Optimizer" or "RDF"
**      error			   error code to report
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**
** History:
**	08-apr-88 (seputis)
**	    initial creation
**	21-may-89 (jrb)
**	    changed ule_format call interface
**	13-aug-92 (rog)
**	    Prototyped.
**	25-oct-92 (andre)
**	    generic_error has been replaced with sqlstate in the interface of
**	    both ulx_sccerror() and ule_format()
**	20-sep-1993 (andys)
**	    Fix arguments to ule_format call.
[@history_line@]...
*/
VOID
ulx_rverror(char *errorid, DB_STATUS status, DB_ERRTYPE error,
	    DB_ERRTYPE foreign, ULF_LOG logmessage, DB_FACILITY facility)
{
    DB_STATUS	ule_status;		  /* return status from ule_format */
    i4	ule_error;		  /* ule cannot format message */
    char	msg_buffer[DB_ERR_SIZE];  /* error message buffer */
    i4	msg_buf_length;		  /* length of message returned */
    DB_SQLSTATE	sqlstate;		  /* sqlstate status code */

    ule_status = ule_format( error, (CL_SYS_ERR *)NULL, logmessage,
	&sqlstate, msg_buffer, (i4) (sizeof(msg_buffer)-1),
	&msg_buf_length, &ule_error, 0);
    if (DB_FAILURE_MACRO(ule_status))
    {
	STprintf(msg_buffer, 
"ULE error = %x\n %s message cannot be found - error no = %x Status = %x Facility error = %x \n", 
		 ule_error, errorid, error , status, foreign);
    }
    else
    {
	msg_buffer[msg_buf_length] = 0;	/* null terminate */
    }

    {   /* report any errors in the range 0 to 127 to the user */
	DB_STATUS	scf_status, ulx_status;

	/*
	** Generate a warning, so that SCC_TRACE is used, this will ensure that
	** SCC_ERROR will be called with the "foreign" error code which will be
	** the ingres user code which will be returned to the equel program
	*/
	if (foreign)
	    ulx_status = E_DB_WARN;
	else
	    ulx_status = status;

	scf_status = ulx_sccerror(ulx_status, &sqlstate, error, msg_buffer,
				  (i4) msg_buf_length, facility);
	if (DB_FAILURE_MACRO(scf_status))
	{
	    TRdisplay( 
	      "%s error reporting error= %x Status = %x Facility error = %x \n",
	      errorid, error , status, foreign);
	}
    }

    /* if foreign error exists then report it in the same way */
    if (foreign)
    {
	DB_STATUS	scf1_status;

	ule_status = ule_format( foreign,  (CL_SYS_ERR *)NULL, logmessage,
	    &sqlstate, msg_buffer, (i4) (sizeof(msg_buffer)-1),
	    &msg_buf_length, &ule_error, 0);
	if (DB_FAILURE_MACRO(ule_status))
	{
	    STprintf(msg_buffer, 
"ULE error = %x\n%s error = %x Status = %x Facility error cannot be found - error no = %x \n", 
		ule_error, errorid, error, status, foreign);
	}
	else
	    msg_buffer[msg_buf_length] = 0;	/* null terminate */

	scf1_status = ulx_sccerror(status, &sqlstate, foreign, msg_buffer,
				   (i4) msg_buf_length, facility);
	if (DB_FAILURE_MACRO(scf1_status))
	{
	    TRdisplay("%s error = %x Status = %x Facility error = %x \n", 
		      errorid, error , status, foreign);
	}
    }
}

/*{
** Name: ulx_exception	- generic exception handler function
**
** Description:
**      This routine handles all of the common tasks that exception
**	handlers normally perform.
**
** Inputs:
**      args		Argument list from exception handler
**      facility	Caller identification ID
**      fac_error	Facility-specific error to be logged
**	dump_session	Boolean that specifies whether scs_avformat() should
**			be called
**
** Outputs:
**	Returns:
**	    OK
**	Exceptions:
**	    None
**
** Side Effects:
**
**
** History:
**	24-jul-1992 (rog)
**	    Initial creation.
**	02-jul-1993 (rog)
**	    Add the exception number to the ule_format() call that reports the 
**	    facility error.  It should not affect parameterless errors, and will
**	    allow us to use error messages that expect one parameter.
**	09-aug-1993 (walt)
**	    The newly added exception number needs its address passed to
**	    ule_format rather than its value.
**	18-Nov-2010 (kschendel) SIR 124685
**	    Correct type of sid, it's not a pointer.
*/
STATUS
ulx_exception( EX_ARGS *args, DB_FACILITY facility, i4 fac_error,
	       bool dump_session )
{
    i4	error;
    char	err_msg[EX_MAX_SYS_REP];
    char	server_id[64];
    CS_SID	sid;

    /*
    ** Report the facility-specific error message, e.g., "There was an
    ** exception in PSF..."
    */
    ule_format( fac_error, NULL, (i4) ULE_LOG, (DB_SQLSTATE *) NULL, NULL,
		(i4) 0, NULL, &error, 1,
		(i4) sizeof(args->exarg_num),
		(i4 *) &args->exarg_num );
    
    CSget_svrid( server_id );
    CSget_sid( &sid );
    ERmsg_hdr( server_id, sid, err_msg);

    /* Dump session info to the logfile */
    if (dump_session)
	scs_avformat();

    /* 
    ** If we can EXsys_report() it, fine;
    ** otherwise, try to dump whatever info we have.
    */
    if (!(EXsys_report(args, err_msg)))
    {
	ule_format( E_UL0306_EXCEPTION, NULL, (i4) ULE_LOG,
		    (DB_SQLSTATE *) NULL, NULL,
		    (i4) 0, NULL, &error, 1,
		    (i4) sizeof(args->exarg_num),
		    (i4 *) &args->exarg_num );
    }

    return(OK);
}
