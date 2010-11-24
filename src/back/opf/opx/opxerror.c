/*
**Copyright (c) 2004, 2010 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <ex.h>
#include    <ulm.h>
#include    <ulf.h>
#include    <ulx.h>
#include    <adf.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <qsf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <psfparse.h>
#include    <qefnode.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <scf.h>
#include    <generr.h>
#include    <sqlstate.h>
/* beginning of optimizer header files */
#include    <opglobal.h>
#include    <opdstrib.h>
#include    <opfcb.h>
#include    <opgcb.h>
#include    <opscb.h>
#include    <ophisto.h>
#include    <opboolfact.h>
#include    <oplouter.h>
#include    <opeqclass.h>
#include    <opcotree.h>
#include    <opvariable.h>
#include    <opattr.h>
#include    <openum.h>
#include    <opagg.h>
#include    <opmflat.h>
#include    <opcsubqry.h>
#include    <opsubquery.h>
#include    <opcstate.h>
#include    <opstate.h>

/* external routine declarations definitions */
#include    <er.h>
#define             OPX_EXROUTINES      TRUE
#define             OPX_PERROR          TRUE
#define             OPX_VERROR          TRUE
#define             OPX_STATUS          TRUE
#define             OPX_RVERROR         TRUE
#define             OPX_RERROR          TRUE
#define             OPX_LERROR          TRUE
#include    <me.h>
#include    <tr.h>
#include    <st.h>
#include    <opxlint.h>

/**
**
**  Name: OPXERROR.C - routine to report optimizer error
**
**  Description:
**      This routine will report an optimizer error
**
**  History:
**      3-jul-86 (seputis)    
**          initial creation
**	21-may-89 (jrb)
**	    added generic error support, changed ule_format calls for new
**	    interface
**	07-nov-90 (stec)
**	    Added opx_lerror() routine.
**	14-dec-90 (stec)
**	    Added OPX_LERROR define above for lint.
**      16-aug-91 (seputis)
**          add CL include files for PC group
**	30-apr-92 (bryanp)
**	    Translate E_RD002A_DEADLOCK to E_OP0010_RDF_DEADLOCK.
**	8-sep-92 (ed)
**	    remove OPF_DIST ifdefs
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**      16-sep-93 (smc)
**          Moved <cs.h> for CS_SID.
**      17-feb-95 (inkdo01)
**          Added opx_1perror to support 1 token error messages
**	10-jan-1996 (toumi01; from 1.1 axp_osf port)
**	    Cast (long) 3rd... EXsignal args to avoid truncation.
**	28-oct-1996 (hanch04)
**	    Changed SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS
**	11-nov-1998 (nanpr01)
**	    Call ule_doformat instead of ule_format so that
**	    the line numbers without the messages do not get
**	    printed with blank lines.
**      18-Feb-1999 (hweho01)
**          Changed EXCONTINUE to EXCONTINUES to avoid compile error of
**          redefinition on AIX 4.3, it is used in <sys/context.h>.
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes
**/

/* TABLE OF CONTENTS */
i4 opx_status(
	OPF_CB *opf_cb,
	OPS_CB *ops_cb);
static i4 opx_sccerror(
	i4 status,
	DB_SQLSTATE *sqlstate,
	OPX_ERROR error,
	char *msg_buffer,
	i4 msg_length);
void opx_rverror(
	OPF_CB *opfcb,
	i4 status,
	OPX_ERROR error,
	OPX_FACILITY facility);
void opx_verror(
	i4 status,
	OPX_ERROR error,
	OPX_FACILITY facility);
void opx_lerror(
	OPX_ERROR error,
	i4 num_parms,
	PTR p1,
	PTR p2,
	PTR p3,
	PTR p4);
void opx_1perror(
	OPX_ERROR error,
	PTR p1);
void opx_2perror(
	OPX_ERROR error,
	PTR p1,
	PTR p2);
void opx_vrecover(
	i4 status,
	OPX_ERROR error,
	OPX_FACILITY facility);
void opx_rerror(
	OPF_CB *opfcb,
	OPX_ERROR error);
void opx_error(
	OPX_ERROR error);
i4 opx_float(
	OPX_ERROR error,
	bool hard);
static i4 opx_adfexception(
	OPS_CB *ops_cb,
	EX_ARGS *ex_args);
i4 opx_exception(
	EX_ARGS *args);

/*{
** Name: opx_status	- get user return status
**
** Description:
**      This routine will get the user return status from the session control
**      block.  The routine should only be call after an exception has been
**      generated since it assumes that some error has occurred and will
**      update the caller's control block with a default error message if
**      necessary
**
** Inputs:
**      opf_cb                          ptr to caller's control block
**      ops_cb                          ptr to session control block, or NULL
**                                      if not available
**
** Outputs:
**	Returns:
**	    user return status
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	29-jun-86 (seputis)
**          initial creation
**      26-jun-91 (seputis)
**          added check for OPC access violations
[@history_line@]...
*/
STATUS
opx_status(
	OPF_CB             *opf_cb,
	OPS_CB             *ops_cb)
{
    DB_STATUS              status;  /* status from control block which was
                                    ** set by an earlier error exit, prior
                                    ** to signalling an exception
                                    */

    if (opf_cb->opf_errorblock.err_code == E_OP0000_OK)
    {
	if (ops_cb
	    &&
	    (ops_cb->ops_state->ops_gmask & OPS_OPCEXCEPTION)
	    )
	    opf_cb->opf_errorblock.err_code  = E_OP08A2_EXCEPTION; /* 
				    ** check for exception in
				    ** OPC component of OPF */
	else
	    opf_cb->opf_errorblock.err_code = E_OP0082_UNEXPECTED_EX; /* an
				    ** unexpected exception occurred which
                                    ** was not handled */
	return (E_DB_SEVERE);        /* probably should be session fatal */
    }


    if (!ops_cb)
	return (E_DB_FATAL);        /* probably a server initialization error */

    status = ops_cb->ops_retstatus;
# ifdef E_OP0081_NOSTATUS
    if (status == E_DB_OK)
    {
	/* this code should not be executed */
	status = E_DB_SEVERE;
	opf_cb->opf_errorblock.err_code = E_OP0081_NOSTATUS;
    }
# endif
    return (status);

}

/*{
** Name: opx_sccerror	- report error to user
**
** Description:
**      Report a message to the user
**
** Inputs:
**      status                          status of error
**      msg_buffer                      ptr to message
**      error                           ingres error number
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    fatal status will cause query to be aborted, and previous text sent
**          to user to be flushed
**
** History:
**      15-apr-87 (seputis)
**          initial creation
**	21-may-89 (jrb)
**	    changed interface to this routine to accept generic error
**      28-may-92 (seputis)
**          - dump query for any error which is logged
**	24-oct-92 (andre)
**	    replaced generic error (i4) with sqlstate (DB_SQLSTATE *) in
**	    the interface of opx_sccerror()
**	13-nov-92 (andre)
**	    If sqlstate is not passed into opx_sccerror(), set scf_sqlstate 
**	    to MISC_ING_ERRORS"

[@history_line@]...
[@history_template@]...
*/
static DB_STATUS
opx_sccerror(
	DB_STATUS	   status,
	DB_SQLSTATE	   *sqlstate,
	OPX_ERROR          error,
	char               *msg_buffer,
	i4                msg_length)
{
    SCF_CB                 scf_cb;
    DB_STATUS              scf_status;

    scf_cb.scf_length = sizeof(scf_cb);
    scf_cb.scf_type = SCF_CB_TYPE;
    scf_cb.scf_facility = DB_OPF_ID;
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
	scf_status = scf_call(SCC_TRACE, &scf_cb);
    else
    {
	/* Dump the session that caused this error */
	if ((error % 256) >= 128)
	    scs_avformat();	/* dump info on internal
				** consistency checks only,
				** i.e. do not dump for errors like
				** OP0008 */
	scf_status = scf_call(SCC_ERROR, &scf_cb);
    }
    if (scf_status != E_DB_OK)
    {
	TRdisplay("SCF error displaying OPF message to user\n");
	TRdisplay("OPF message is :%s",msg_buffer);
    }
    return (scf_status);
}

/*{
** Name: opx_rverror	- this routine will report another facility's error
**
** Description:
**      This routine will log an error from another facility and the related
**      optimizer error.  
**
** Inputs:
**      opfcb                           ptr to another facility's control block
**      error                           optimizer error code to report
**      facility                        ptr to facility's control block
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**	    the exception handling routines are responsible for
**          deallocating OPF resources bound to this session.
**
** History:
**	3-jul-86 (seputis)
**          initial creation
**	21-may-89 (jrb)
**	    now passes generic_error to opx_sccerror routine
**	24-oct-92 (andre)
**	    interfaces of ule_format() and opx_sccerror() have been changed to
**	    receive (DB_SQLSTATE *)
**	24-Oct-2008 (jonj)
**	    Replace ule_doformat with ule_format.
[@history_line@]...
*/
VOID
opx_rverror(
	OPF_CB             *opfcb,
	DB_STATUS          status,
	OPX_ERROR          error,
	OPX_FACILITY       facility)
{
    DB_STATUS	    ule_status;		/* return status from ule_format */
    i4         ule_error;          /* ule cannot format message */
    char	    msg_buffer[DB_ERR_SIZE]; /* error message buffer */
    i4         msg_buf_length;     /* length of message returned */
    i4         log;                /* logging state */
    DB_SQLSTATE	    sqlstate;

    opfcb->opf_errorblock.err_code = error;	/* save error code */
    opfcb->opf_errorblock.err_data = facility;	/* save code 
                                                ** from facility */
    if ((error % 256) < 128)
	log = ULE_LOOKUP;		/* lookup only user errors with range
					** of 0-127 */
    else
	log = ULE_LOG;			/* log errors with range 128-255 */

#ifdef    OPT_F031_WARNINGS
    /* if trace flag is set then warning or info errors will be reported */
    if ( DB_SUCCESS_MACRO(status))
    {
	OPS_CB	    *ops_cb;
	ops_cb = ops_getcb();
	if (ops_cb
	    &&
	    (   !ops_cb->ops_check 
		||
		!opt_strace( ops_cb, OPT_F031_WARNINGS)
	    ))
		return;
    }
#endif
    ule_status = ule_format( error, (CL_SYS_ERR *)NULL, log, &sqlstate,
	msg_buffer, (i4) (sizeof(msg_buffer)-1), &msg_buf_length,
	&ule_error, 0);
    if (ule_status != E_DB_OK)
    {
        (VOID)STprintf(msg_buffer, 
"ULE error = %x\nOptimizer cannot be found - error no = %x Status = %x Facility error = %x \n", 
	    ule_error, error , status, facility);
    }
    else
	msg_buffer[msg_buf_length] = 0;	/* null terminate */
    /* FIXME - should check possible return status and generate a severe error
    ** unless it is simple (like cannot find message) */
    {   /* report any errors in the range 0 to 127 to the user */
        DB_STATUS		scf_status;
	if (facility)
	    scf_status = opx_sccerror(E_DB_WARN, &sqlstate, error,
		msg_buffer,(i4)msg_buf_length);	/* generate
					    ** a warning, so that SCC_TRACE is
                                            ** used, this will ensure that
                                            ** SCC_ERROR will be called with
		                            ** the "facility" error code which
                                            ** will be the ingres user code
                                            ** which will be returned to the
                                            ** equel program */
	else
	    scf_status = opx_sccerror(status, &sqlstate, error, msg_buffer,
		(i4)msg_buf_length);
	if (scf_status != E_DB_OK)
	{
	    TRdisplay( 
		"Optimizer error = %x Status = %x Facility error = %x \n", 
                error , status, facility);
	}
    }
    if (!facility)
	return;				    /* if facility error exists then
					    ** report it in the same way */
    ule_status = ule_format( facility,  (CL_SYS_ERR *)NULL, log, &sqlstate,
	msg_buffer, (i4) (sizeof(msg_buffer)-1), &msg_buf_length,
	&ule_error, 0);
    if (ule_status != E_DB_OK)
    {
        (VOID)STprintf(msg_buffer, 
"ULE error = %x\nOptimizer error = %x Status = %x Facility error cannot be found - error no = %x \n", 
	    ule_error, error , status, facility);
    }
    else
	msg_buffer[msg_buf_length] = 0;	/* null terminate */
    {   
        DB_STATUS		scf1_status;
	scf1_status = opx_sccerror(status, &sqlstate, facility, msg_buffer,
	    (i4)msg_buf_length);
	if (scf1_status != E_DB_OK)
	{
	    TRdisplay( 
		"Optimizer error = %x Status = %x Facility error = %x \n", 
                error , status, facility);
	}
    }
}

/*{
** Name: opx_verror	- this routine will report another facility's error
**
** Description:
**      This routine will report an error from another facility
**      optimizer error.  It will check the status and see the error
**      is recoverable.  The routine will not return if the status is fatal
**      but instead generate an internal exception and
**      exit via the exception handling mechanism.  The error code will be
**      placed directly in the user's calling control block.  A ptr to this
**      block was placed in the session control block (which can be requested
**      from SCF)
**
** Inputs:
**      error                           optimizer error code to report
**      facility                        facility's error code
**      status                          facility's return status
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**	    the exception handling routines are responsible for
**          deallocating OPF resources bound to this session.
**
** History:
**	3-jul-86 (seputis)
**          initial creation
**	28-jan-91 (seputis)
**          added support for OPF ACTIVE flag
**	30-apr-92 (bryanp)
**	    Translate E_RD002A_DEADLOCK to E_OP0010_RDF_DEADLOCK.
[@history_line@]...
*/
VOID
opx_verror(
	DB_STATUS          status,
	OPX_ERROR          error,
	OPX_FACILITY       facility)
{
    OPS_CB              *opscb;			/* ptr to session control block
                                                ** for this optimization */
    opscb = ops_getcb();
    opscb->ops_retstatus = status;		/* save status code */
    if (facility == E_RD002A_DEADLOCK)
	error = E_OP0010_RDF_DEADLOCK;
    if ((facility == E_RD002B_LOCK_TIMER_EXPIRED)
	&&
	(opscb->ops_smask & OPS_MCONDITION))
    {
	error = E_OP000D_RETRY;			/* when an attempt is made to
						** report the RDF timeout error
						** then it should be translated
						** into a retry error, since an
						** undetected deadlock may have
						** occurred due to the ACTIVE flag */
	opscb->ops_server->opg_sretry++;	/* track number of retrys */
	opx_vrecover(status, E_OP000D_RETRY, facility); /* this routine will not
						** return */
    }
    else
	opx_rverror(opscb->ops_callercb, status, error, facility); 

    if (DB_SUCCESS_MACRO(status))
    {
	return;					/* return if error is not
                                                ** serious */
    }
    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error); /* signal an optimizer long
                                                ** jump with error code */
}
/*{
** Name: opx_lerror	- an error handling routine, which accepts
**			  variable arguments
**
** Description:
**	This routine logs information in the dbms.log. It accepts up
**	to 4 variable arguments.
**
** Inputs:
**      error		    optimizer error code to report
**	p1 .. p4	    pointers to variable arguments
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    the exception handling routines are responsible for
**          deallocating OPF resources bound to this session.
**
** History:
**	07-nov-90 (stec)
**          initial creation
**	24-oct-92 (andre)
**	    interface of ule_format() has changed to accept (DB_SQLSTATE *).
**	    Since we are not using the sqlstate value here, simply pass NULL to
**	    ule_format()
*/
VOID
opx_lerror(
	OPX_ERROR   error,
	i4	    num_parms,
	PTR         p1,
	PTR         p2,
	PTR         p3,
	PTR         p4)
{
    i4             uletype;
    i4		ulecode;
    i4		msglen;
    char		errbuf[DB_ERR_SIZE];

    if (num_parms > 4)
	return;

    uletype = ULE_LOG;

    (VOID) ule_format(error, 0L, uletype, (DB_SQLSTATE *) NULL, errbuf,
	(i4) sizeof(errbuf), &msglen, &ulecode, num_parms,
	(i4)0, p1, (i4)0, p2, (i4)0, p3, (i4)0, p4); 

    return;
}

/*{
** Name: opx_1perror	- print error with 1 parameter 
**
** Description:
**	Print error with exactly 1 parameter, do not use variable parameters to
**	avoid lint errors
**
** Inputs:
**      error                           error message number
**      p1                              parameter 1
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    exits via exception handler
**
** Side Effects:
**	    does not return
**
** History:
**      17-feb-95 (inkdo01) 
**          initial creation 
[@history_template@]...
*/
VOID
opx_1perror(
	OPX_ERROR           error,
	PTR		    p1)
{
    DB_STATUS	    ule_status;		/* return status from ule_format */
    i4         ule_error;          /* ule cannot format message */
    char	    msg_buffer[DB_ERR_SIZE]; /* error message buffer */
    i4         msg_buf_length;     /* length of message returned */
    i4         log;                /* should this error be logged */
    DB_SQLSTATE	    sqlstate;
    OPS_CB              *opscb;			/* ptr to session control block
                                                ** for this optimization */
    opscb = ops_getcb();
    opscb->ops_retstatus = E_DB_ERROR;          /* return status code */
    opscb->ops_callercb->opf_errorblock.err_code = error; /* save error code */

    if ((error % 256) < 128)
	log = ULE_LOOKUP;
    else
	log = ULE_LOG;

    ule_status = ule_format( error,  (CL_SYS_ERR *)NULL, log, &sqlstate,
	msg_buffer, (i4) (sizeof(msg_buffer)-1), &msg_buf_length,
	&ule_error, 1 /* 1 parameters */, (i4)0, p1);
    if (ule_status != E_DB_OK)
    {
        (VOID)STprintf(msg_buffer, 
	    "ULE error = %x, Optimizer message cannot be found - error no. = %x\n", 
	    ule_error, error );
    }
    else
	msg_buffer[msg_buf_length] = 0;	/* null terminate */
    (VOID) opx_sccerror(E_DB_ERROR, &sqlstate, error, msg_buffer,
						(i4)msg_buf_length);
    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error);/* signal an optimizer long
                                                ** jump */
}

/*{
** Name: opx_2perror	- print error with 2 parameters
**
** Description:
**	Print error with exactly 2 parameters, do not use variable parameters to
**	avoid lint errors
**
** Inputs:
**      error                           error message number
**      p1                              parameter 1
**      p2                              parameter 2
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    exits via exception handler
**
** Side Effects:
**	    does not return
**
** History:
**      18-may-92 (seputis)
**          initial creation for tech support
**	24-oct-92 (andre)
**	    interfaces of ule_format() and opx_sccerror() have been changed to
**	    receive (DB_SQLSTATE *)
[@history_template@]...
*/
VOID
opx_2perror(
	OPX_ERROR           error,
	PTR		    p1,
	PTR		    p2)
{
    DB_STATUS	    ule_status;		/* return status from ule_format */
    i4         ule_error;          /* ule cannot format message */
    char	    msg_buffer[DB_ERR_SIZE]; /* error message buffer */
    i4         msg_buf_length;     /* length of message returned */
    i4         log;                /* should this error be logged */
    DB_SQLSTATE	    sqlstate;
    OPS_CB              *opscb;			/* ptr to session control block
                                                ** for this optimization */
    opscb = ops_getcb();
    opscb->ops_retstatus = E_DB_ERROR;          /* return status code */
    opscb->ops_callercb->opf_errorblock.err_code = error; /* save error code */

    if ((error % 256) < 128)
	log = ULE_LOOKUP;
    else
	log = ULE_LOG;

    ule_status = ule_format( error,  (CL_SYS_ERR *)NULL, log, &sqlstate,
	msg_buffer, (i4) (sizeof(msg_buffer)-1), &msg_buf_length,
	&ule_error, 2 /* 2 parameters */, (i4)0, p1, (i4)0, p2); 
    if (ule_status != E_DB_OK)
    {
        (VOID)STprintf(msg_buffer, 
	    "ULE error = %x, Optimizer message cannot be found - error no. = %x\n", 
	    ule_error, error );
    }
    else
	msg_buffer[msg_buf_length] = 0;	/* null terminate */
    (VOID) opx_sccerror(E_DB_ERROR, &sqlstate, error, msg_buffer,
						(i4)msg_buf_length);
    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error);/* signal an optimizer long
                                                ** jump */
}

/*{
** Name: opx_vrecover	- this routine return an error code without reporting
**
** Description:
**	This routine will not log or report to the user this recoverable error
**      to but instead exit to the caller who is expected to take corrective
**      action.
**
** Inputs:
**      error                           optimizer error code to report
**      facility                        facility's error code
**      status                          facility's return status
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**	    the exception handling routines are responsible for
**          deallocating OPF resources bound to this session.
**
** History:
**	3-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
VOID
opx_vrecover(
	DB_STATUS          status,
	OPX_ERROR          error,
	OPX_FACILITY       facility)
{
    OPS_CB              *opscb;			/* ptr to session control block
                                                ** for this optimization */
    opscb = ops_getcb();
    opscb->ops_retstatus = status;		/* save status code */
    opscb->ops_callercb->opf_errorblock.err_code = error; /* save error code */
    opscb->ops_callercb->opf_errorblock.err_data = facility; /* save code 
                                                ** from facility */
    if (DB_SUCCESS_MACRO(status))
    {
	return;					/* return if error is not
                                                ** serious */
    }
    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error); /* signal an optimizer long
                                                ** jump with error code */
}


/*{
** Name: opx_rerror	- this routine will report an optimizer error
**
** Description:
**      This routine will log an optimizer error.  
**
** Inputs:
**      opfcb                           ptr to caller's control block
**      error                           optimizer error code to log
**
** Outputs:
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**	3-jul-86 (seputis)
**          initial creation
**	21-may-89 (jrb)
**	    now passes generic error to opx_sccerror
**	24-oct-92 (andre)
**	    interfaces of ule_format() and opx_sccerror() have been changed to
**	    receive (DB_SQLSTATE *)
**	24-june-03 (inkdo01)
**	    Add OP04C0 to list for which error is NOT returned to user.
**	22-june-06 (dougi)
**	    Make E_OP0018 (timeoutabort) go to log where the query text also
**	    gets dumped.
**	24-jun-08 (hayke02)
**	    Treat E_OP04C1 like E_OP04C0: soft failure, error reported to
**	    errlog.log and retry using non-greedy enumeration, unless op247 is
**	    set.
[@history_line@]...
*/
VOID
opx_rerror(
	OPF_CB             *opfcb,
	OPX_ERROR          error)
{
    DB_STATUS	    ule_status;		/* return status from ule_format */
    i4         ule_error;          /* ule cannot format message */
    char	    msg_buffer[DB_ERR_SIZE]; /* error message buffer */
    i4         msg_buf_length;     /* length of message returned */
    i4         log;                /* should this error be logged */
    DB_SQLSTATE	    sqlstate;
    bool	enumnomsg = FALSE;

    opfcb->opf_errorblock.err_code = error; /* save error code */

    if ((error % 256) < 128 && error != E_OP0018_TIMEOUTABORT)
	log = ULE_LOOKUP;
    else
	log = ULE_LOG;

    ule_status = ule_format( error,  (CL_SYS_ERR *)NULL, log, &sqlstate,
	msg_buffer, (i4) (sizeof(msg_buffer)-1), &msg_buf_length,
	&ule_error, 0);
    if (ule_status != E_DB_OK)
    {
        (VOID)STprintf(msg_buffer, 
	    "ULE error = %x, Optimizer message cannot be found - error no. = %x\n", 
	    ule_error, error );
    }
    else
	msg_buffer[msg_buf_length] = 0;	/* null terminate */
    if ((error == E_OP04C0_NEWENUMFAILURE)
	||
	(error == E_OP04C1_GREEDYCONSISTENCY))
    {
	OPS_CB              *opscb;
	OPS_STATE	*global;
	i4	temp;

	/* Check if new enumeration is result of tp op247 - if not, 
	** message isn't displayed. */
	opscb = ops_getcb();
	global = opscb->ops_state;
	if (!global->ops_cb->ops_check ||
	    !opt_svtrace( global->ops_cb, OPT_F119_NEWENUM, &temp, &temp))
	 enumnomsg = TRUE;
    }
    if (error != E_OP0003_ASYNCABORT && !enumnomsg)
					    /* do not report control C errors
					    ** or "new enumeration" errors
					    ** to the user */
    {
	(VOID) opx_sccerror(E_DB_ERROR, &sqlstate, error, msg_buffer,
						(i4)msg_buf_length);
    }
}

/*{
** Name: opx_error	- this routine will report an optimizer error
**
** Description:
**      This routine will report a E_DB_ERROR optimizer error.  The routine
**      will not return but instead generate an internal exception and
**      exit via the exception handling mechanism.  The error code will be
**      placed directly in the user's calling control block.  A ptr to this
**      block was placed in the session control block (which can be requested
**      from SCF)
**
** Inputs:
**      error                           optimizer error code to report
**
** Outputs:
**	Returns:
**	    - routine does not return
**	Exceptions:
**	    - internal exception generated
**
** Side Effects:
**	    the exception handling routines are responsible for
**          deallocating OPF resources bound to this session.
**
** History:
**	3-jul-86 (seputis)
**          initial creation
[@history_line@]...
*/
VOID
opx_error(
	OPX_ERROR          error)
{
    OPS_CB              *opscb;			/* ptr to session control block
                                                ** for this optimization */
    opscb = ops_getcb();
    opscb->ops_retstatus = E_DB_ERROR;          /* return status code */
    opx_rerror( opscb->ops_callercb, error);    /* log error */
    EXsignal( (EX)EX_JNP_LJMP, (i4)1, (long)error);/* signal an optimizer long
                                                ** jump */
}

/*{
** Name: opx_float	- handle floating point exception in OPF
**
** Description:
**      This routine will process a floating point exception error
**	within the optimizer.  This will normally be done by skipping
**	the part of the search space which causes the floating point
**	exception. 
**
** Inputs:
**      error                           floating point error code
**	hard				TRUE - if HARD error occurs
**					which cannot be continued from
**
** Outputs:
**
**	Returns:
**	    VOID
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      28-nov-90 (seputis)
**          initial creation
**	21-jan-91 (jkb) submitted for seputis
**	    correct potential UNIX floating point problem
**	18-apr-91 (seputis)
**	    - fix bug 36920
**	31-mar-92 (rog)
**	    Remove underscores from EX return statuses.
[@history_template@]...
*/
STATUS
opx_float(
	OPX_ERROR	    error,
	bool		    hard)
{
    OPS_CB             *opscb; /* session control block */
    OPF_CB             *opfcb; /* caller's control block */

    opscb = ops_getcb();	/* get optimizer session control block */
    opfcb = opscb->ops_state->ops_caller_cb; /* report unexpected exception
                                ** otherwise */
    if (opscb->ops_check 
	&&
	opt_strace( opscb, OPT_F048_FLOAT)
	&&
	!hard
	&&
	(opscb->ops_state->ops_gmask & OPS_BFPEXCEPTION)
	)
    {
	opscb->ops_state->ops_gmask |= OPS_FPEXCEPTION; /* mark
				** plan as having a floating point
				** exception */
	return(EXCONTINUES);	/* try to recover from floating point
				** exception unless a hard error occurs
				** or a plan has been found which did
				** not have a floating point exception */
    }
    if (DB_SUCCESS_MACRO(opscb->ops_retstatus))
    {
	opfcb->opf_errorblock.err_code = error; /* save error code */
	opscb->ops_retstatus = E_DB_WARN; /* change status only if more
				** severe */
    }
    opscb->ops_state->ops_gmask |= (OPS_FLSPACE | OPS_FLINT); /* indicate that
			    ** search space has been reduced */
    return(EXDECLARE);
}

/*{
** Name: opx_adfexception	- handle ADF exceptions if possible
**
** Description:
**      Routine will be called by exception handlers in the optimizer to
**	handle arithmetic exceptions if possible
**
** Inputs:
**      ops_cb                          OPF session control block
**      ex_args                         exception handler arguments
**
** Outputs:
**	Returns:
**	    exception status - EXCONTINUES, or EXRESIGNAL
**		where EXRESIGNAL means that the exception is unknown
**	Exceptions:
**	    OPF exception is generated for a fatal ADF exception
**
** Side Effects:
**	    none
**
** History:
**      20-jan-86 (seputis)
**          initial creation
**	31-mar-92 (rog)
**	    Remove underscores from EX return statuses.
**	24-oct-92 (andre)
**	    interface of opx_sccerror() has been changed to receive
**	    (DB_SQLSTATE *)
[@history_template@]...
*/
static STATUS
opx_adfexception(
	OPS_CB		   *ops_cb,
	EX_ARGS            *ex_args)
{
    DB_STATUS           status;
    ADF_CB              *adf_scb;

    adf_scb = ops_cb->ops_adfcb;
    status = adx_handler(adf_scb, ex_args);
    if ((status == E_DB_WARN) 
	&& 
	(adf_scb->adf_errcb.ad_errcode == E_AD0116_EX_UNKNOWN)
	)
    {	/* adf does not know about this error so resignal it */
	return (EXRESIGNAL);
    }
    if (DB_SUCCESS_MACRO(status))
    {	/* adf will return OK status if it knowns about the error and the
        ** correct action is to continue */
#ifdef    OPT_F031_WARNINGS
	/* if trace flag is set then warning or info errors will be reported */
	if ( DB_SUCCESS_MACRO(status)
	    &&
	    ops_cb->ops_check 
	    &&
	    opt_strace( ops_cb, OPT_F031_WARNINGS)
	   )
	    opx_rverror( ops_cb->ops_callercb, 
		E_DB_WARN, E_OP0701_ADF_EXCEPTION, (OPX_FACILITY)0); /* report 
						** warning is 
						** trace flag is set */
#endif
	return(EXCONTINUES);			/* return if error is not
                                                ** serious */
    }

    {
	OPX_ERROR	adf_error;
	OPX_ERROR	opf_error;
	ops_cb->ops_retstatus = status;
	if (adf_scb->adf_errcb.ad_errclass == ADF_USER_ERROR)
	{	/* report the error message to the user */
	    adf_error = adf_scb->adf_errcb.ad_usererr;
	    opf_error = E_OP0702_ADF_EXCEPTION;
	}
	else
	{	/* report non-user error code */
	    adf_error = adf_scb->adf_errcb.ad_errcode;
	    opf_error = E_OP0795_ADF_EXCEPTION;
	}
	opx_rverror(ops_cb->ops_callercb, E_DB_WARN, opf_error,(OPX_FACILITY)0);
					/* do not report facility error
					** since opx_sccerror will report it
					** in the next statement
                                        ** - need to use the formatted message
                                        ** the ADF provides since it may contain
                                        ** parameters */
	(VOID) opx_sccerror(status, &adf_scb->adf_errcb.ad_sqlstate,
	    adf_error, adf_scb->adf_errcb.ad_errmsgp,
	    adf_scb->adf_errcb.ad_emsglen);
    }
    return (EXDECLARE);		    /* return to declaration point of
					    ** exception handler */
}

/*{
** Name: opx_exception	- handle unknown exception for optimizer
**
** Description:
**      This routine will handle unknown exceptions for the optimizer
**
** Inputs:
**      ex_args                         argument list from exception handler
**
** Outputs:
**	Returns:
**	    STATUS - appropriate CL return code for exception handler
**	Exceptions:
**
**
** Side Effects:
**
**
** History:
**      20-jan-87 (seputis)
**	    initial creation
**      7-nov-88 (seputis)
**          use EXsys_report
**      26-jun-91 (seputis)
**          added check for OPC access violations
**	11-mar-92 (rog)
**	    Use EXsys_report for everything, not just EXSEGVIO.  Call
**	    scs_avformat to dump info about the offending session.  Remove
**	    underscores from EX return statuses.
**	02-jul-1993 (rog)
**	    Call ulx_exception() to handle all of the reporting duties.
**	    OPF should need to call adx_handler, so remove call to
**	    opx_adfexception().
[@history_line@]...
[@history_template@]...
*/
STATUS
opx_exception(
	EX_ARGS            *args)
{
    /* unexpected exception so record the error and return to
    ** deallocate resources */

    OPS_CB	*opscb; /* session control block */
    OPF_CB	*opfcb; /* caller's control block */
    OPX_ERROR	errorcode; /* error code for exception */

    opscb = ops_getcb();	/* get optimizer session control block */
    opfcb = opscb->ops_state->ops_caller_cb; /* report unexpected exception
                                ** otherwise */
    if (opfcb->opf_errorblock.err_code == E_OP0082_UNEXPECTED_EX)
    {	/* try to check for exception within an exception as soon as possible */
	errorcode = E_OP0083_UNEXPECTED_EXEX; /* second exception
				** occurred so do not deallocate
				** any resources */
	opscb->ops_retstatus = E_DB_SEVERE; /* make sure an high enough priority
                                ** error is reported */
    }
    else
    {
	if (opscb->ops_state->ops_gmask & OPS_OPCEXCEPTION)
	    errorcode = E_OP08A2_EXCEPTION; /* check for exception in
				** OPC component of OPF */
	else
	    errorcode = E_OP0082_UNEXPECTED_EX; /* check for exception
				** in non-OPC components of OPF */
    }

    ulx_exception( args, DB_OPF_ID, E_OP0901_UNKNOWN_EXCEPTION, TRUE);

    if (opscb->ops_retstatus != E_DB_SEVERE
	&&
	opscb->ops_retstatus != E_DB_FATAL)
	opscb->ops_retstatus = E_DB_ERROR; /* make sure an high enough priority
                                ** error is reported */
    opx_rerror(opscb->ops_state->ops_caller_cb, errorcode);/*log error*/
    return (EXDECLARE);        /* return to exception handler declaration pt */
}
