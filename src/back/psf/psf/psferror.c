/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cs.h>
#include    <qu.h>
#include    <sl.h>
#include    <st.h>
#include    <iicommon.h>
#include    <sqlstate.h>
#include    <dbdbms.h>
#include    <ddb.h>
#include    <dmf.h>
#include    <dmtcb.h>
#include    <adf.h>
#include    <ulf.h>
#include    <qsf.h>
#include    <scf.h>
#include    <qefrcb.h>
#include    <rdf.h>
#include    <qefmain.h>
#include    <psfparse.h>
#include    <psfindep.h>
#include    <pshparse.h>
/**
**
**  Name: PSFERROR.C - Error formatting and reporting functions.
**
**  Description:
**      This file contains the error formatting and reporting functions for 
**      the parser facility.
**
**          psfErrorFcn - Format and report a parser facility error.
**          psf_adf_error - Format and report an ADF error in PSF.
**          psf_rdf_error - Format and report an RDF error in PSF.
**	    psf_qef_error - Format and report a QEF error in PSF.
**
**
**  History:
**      30-dec-85 (jeff)    
**          written
**	20-oct-87 (stec)
**	    When PSF_INTERR and detail != 0, the problem caused
**	    by other facility.
**	25-oct-87 (stec)
**	    Added psf_adf_error
**	07-dec-87 (stec)
**	    Added psf_rdferror routine.
**	17-feb-88 (stec)
**	    Correct coding error.
**	16-mar-88 (stec)
**	    Modified psf_rdferror routine.
**	24-mar-88 (stec)
**	    Implement RETRY.
**	04-auf-88 (stec)
**	    Changed the define for E_PS0904.
**	21-may-89 (jrb)
**	    Generic error support.  Now all calls to scc_error first set the
**	    scf_generic_error field of the scf_cb.
**	13-jun-89 (fred)
**	    If error class in ADF error block is EXTERNAL_ERROR, then
**	    send back a 'raise error' style message so that the FE's won't
**	    attempt to enforce their brand of aesthetics on the message.
**	11-sep-90 (teresa)
**	    Fix pss_retry logic.
**	14-jan-92 (barbara)
**	    Included ddb.h for Star.
**	25-mar-92 (barbara)
**	    Merged Star code for Sybil.    
**	30-apr-1992 (bryanp)
**	    Translate E_RD002A_DEADLOCK into E_PS0009_DEADLOCK.
**	29-jun-93 (andre)
**	    #included ST.H
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	11-aug-93 (andre)
**	    removed declarations of external functions whose prototype 
**	    declarations are in included header files + fixed cause of compiler
**	    warning
**      16-sep-93 (smc)
**          Added/moved <cs.h> for CS_SID. Added history_template so we
**          can automate this next time.
**      10-nov-93 (cohmi01)
**          reduce 'strength' of error from INT_OTHER to USER if lock timeout.
**	28-Oct-96 (hanch04)
**	    Changed SS50000_MISC_ING_ERRORS to SS50000_MISC_ERRORS
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Handle E_QE005B_TRAN_ACCESS_CONFLICT.
**	14-Aug-1998 (hanch04)
**	    Make direct call to ule_doformat because to output is sent to II_DBMS_LOG
**	10-Jan-2001 (jenjo02)
**	    Added *PSS_SELBLK parm to psf_mopen(), psf_mlock(), psf_munlock(),
**	    psf_malloc(), psf_mclose(), psf_mroot(), psf_mchtyp().
**	    Changed psf_sesscb() prototype to return PSS_SESBLK* instead
**	    of DB_STATUS. Supply scf_session to SCF when it's known.
**      17-April-2003 (inifa01) bugs 110096 & 109815 INGREP 133.
**          E_DM004D and E_RD002B should not be returned to the user or printed
**          to the error log.  This change suppresses printing of E_DM004D
**          in rdu_ferror() and re-mapps E_RD002B to E_US125E in psf_rdf_error().
**	10-Sep-2008 (jonj)
**	    SIR 120874: Use CLRDBERR, SETDBERR to value DB_ERROR structure.
**	    Use new form of uleFormat().
**	08-Nov-2010 (kiria01) SIR 124685
**	    Rationalise function prototypes. As part of this, introduce psf_verror
**	    to allow simpler calling from other handlers.
**/

/* TABLE OF CONTENTS */
i4 psf_verror(
	i4 errorno,
	i4 detail,
	i4 err_type,
	i4 *err_code,
	DB_ERROR *err_blk,
	PTR FileName,
	i4 LineNumber,
	i4 num_parms,
	va_list);
i4 psfErrorFcn(
	i4 errorno,
	i4 detail,
	i4 err_type,
	i4 *err_code,
	DB_ERROR *err_blk,
	PTR FileName,
	i4 LineNumber,
	i4 num_parms,
	...);
i4 psf_errorNV(
	i4 errorno,
	i4 detail,
	i4 err_type,
	i4 *err_code,
	DB_ERROR *err_blk,
	i4 num_parms,
	...);
void psf_adf_error(
	ADF_ERROR *adf_errcb,
	DB_ERROR *err_blk,
	PSS_SESBLK *sess_cb);
void psf_rdf_error(
	i4 rdf_opcode,
	DB_ERROR *rdf_errblk,
	DB_ERROR *err_blk);
void psf_qef_error(
	i4 qef_opcode,
	DB_ERROR *qef_errblk,
	DB_ERROR *err_blk);

/*{
** Name: psfErrorFcn	- Format and report a parser facility error.
**
** Description:
**      This function formats and reports a parser facility error.  There are 
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
**      err_type                        Type of error
**					    PSF_USERERR - user error
**					    PSF_INTERR  - internal error
**					    PSF_CALLERR - caller error
**	err_code			Ptr to reason for error return, if any
**	err_blk				Pointer to error block
**	FileName			File name of function caller
**	LineNumber			Line number within that file
**	num_parms			Number of parameter pairs to follow.
** NOTE: The following parameters are optional, and come in pairs.  The first
**  of each pair gives the length of a value to be inserted in the error
**  message.  The second of each pair is a pointer to the value to be inserted.
**      param1                          First error parameter, if any
**      param2                          Second error parameter, if any
**      param3                          Third error parameter, if any
**      param4                          Fourth error parameter, if any
**      param5                          Fifth error parameter, if any
**      param6                          Sixth error parameter, if any
**	param7				Seventh error parameter, if any
**	param8				Eighth error parameter, if any
**	param9				Ninth error parameter, if any
**	param10				Tenth error parameter, if any
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
**	30-dec-85 (jeff)
**          written
**	03-jun-87 (daved)
**	    if sess_cb->pss_retry is set, don't print error.
**	7-aug-87 (daved)
**	    if logging 'errorno', log 'detail'
**	20-oct-87 (stec)
**	    When PSF_INTERR and detail != 0, the problem caused
**	    by other facility.
**	17-feb-88 (stec)
**	    Correct coding error.
**	28-jan-89 (jrb)
**	    Added num_parms to interface.
**	14-mar-89 (jrb)
**	    Now passes num_parms thru to ule_format.
**	21-may-89 (jrb)
**	    Generic errors now obtained via ule_format and passed to scc_error.
**      11-sep-90 (teresa)
**          fix pss_retry logic.
**	12-nov-91 (rblumer)
**	    change parameters to be of correct type, i.e. change
**	    odd paramX's to i4s and even paramX's to PTR 
**	    (now it matches ule_format()).
**	03-aug-92 (barbara)
**	    PSF retry processing: PSS_RETRY has been renamed PSS_ERR_ENCOUNTERED
**	    and it is now a bit flag (together with PSS_REPORT_MSGS).
**	24-oct-92 (andre)
**	    interface of ule_format() has changed to receive a (DB_SQLSTATE *) +
**	    SCF_CB now contains an SQLSTATE (DB_SQLSTATE) instead of generic
**	    error (i4)
**	28-oct-92 (andre)
**	    if we are processing a DSQL query and sqlstate status code returned
**	    by ule_format() starts with 42, overwrite 42 with 37 (SQLSTATE class
**	    for DSQL "syntax or access rule violation"s)
**	26-feb-93 (andre)
**	    PSF_USERERR messages issued in the course of reparsing a dbproc to
**	    determine whether it it is active or grantable will be logged since
**	    they contain useful information without which the subsequent
**	    E_PS042C is not very useful
**	03-mar-93 (rblumer)
**	    change psq_cb to err_blk in psf_adf_error
**	15-mar-94 (andre)
**	    we'll no longer try to map 42000-series SQLSTATEs into 37000-series
**	    SQLSTATEs because 37000- and 2A000-series SQLSTATEs have been 
**	    removed from SQL92
**      10-nov-93 (cohmi01)
**          reduce 'strength' of error from INT_OTHER to USER if lock timeout.
**	10-Sep-2008 (jonj)
**	    SIR 120874: Renamed to psfErrorFcn and prototyped, called via 
**	    psf_error macro. Use SETDBERR macro to value DB_ERROR structures.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
*/

DB_STATUS
psf_verror(
    i4		errorno,
    i4		detail,
    i4		err_type,
    i4		*err_code,
    DB_ERROR	*err_blk,
    PTR		FileName,
    i4		LineNumber,
    i4		num_parms,
    va_list	ap)
{
    i4             uletype;
    DB_STATUS           uleret;
    i4		ulecode;
    DB_SQLSTATE		sqlstate;
    i4		msglen;
    DB_STATUS		ret_val = E_DB_OK;
    char		errbuf[DB_ERR_SIZE];
    SCF_CB		scf_cb;
    PSS_SESBLK		*sess_cb;
    bool		leave_loop = TRUE;

#define	MAX_PARMS 5
    i4		psize[MAX_PARMS];
    PTR		pvalue[MAX_PARMS];
    i4		NumParms;
    DB_ERROR	localDBerror;

    for ( NumParms = 0; NumParms < num_parms && NumParms < MAX_PARMS; NumParms++ )
    {
        psize[NumParms] = (i4)va_arg(ap, i4);
	pvalue[NumParms] = (PTR)va_arg(ap, PTR);
    }

    do			/* Not a loop, just gives a place to break to */
    {
	/* Record the error number */
	if (err_type == PSF_CALLERR)
	{
	    err_blk->err_code = errorno;
	    err_blk->err_data = detail;
	    /* ...and whence it came */
	    err_blk->err_file = FileName;
	    err_blk->err_line = LineNumber;
	}
	else if (err_type == PSF_INTERR)
	{
	    if (detail)
	    {
		SETDBERR(err_blk, detail, E_PS0007_INT_OTHER_FAC_ERR);
	    }
	    else
	    {
		SETDBERR(err_blk, detail, E_PS0002_INTERNAL_ERROR);
	    }
	}
	else if (err_type == PSF_USERERR)
	{
	    SETDBERR(err_blk, detail, E_PS0001_USER_ERROR);
	}
	else	/* Unknown error type */
	{
	    *err_code = E_PS0101_BADERRTYPE;
	    ret_val = E_DB_ERROR;
	    break;
	}

	/*
	** Log internal errors, don't log user errors or caller errors.
	*/
	if (err_type == PSF_INTERR)
	    uletype = ULE_LOG;
	else
	    uletype = 0L;

	/*
	** Get error message text.  Don't bother with caller errors, since
	** we don't want to report them or log them.
	*/
	if (err_type != PSF_CALLERR)
	{
	    /* Package error info in a DB_ERROR for uleFormat */
	    localDBerror.err_code = errorno;
	    localDBerror.err_data = 0;
	    localDBerror.err_file = FileName;
	    localDBerror.err_line = LineNumber;

	    /* log the detail if logging 'errorno' */
	    if (uletype == ULE_LOG && detail > 0)
	    {
		uleret = uleFormat(NULL, detail, NULL, ULE_LOG, (DB_SQLSTATE *) NULL,
		    errbuf, (i4) sizeof(errbuf), &msglen, &ulecode,
		    NumParms,
		    psize[0], pvalue[0],
		    psize[1], pvalue[1],
		    psize[2], pvalue[2],
		    psize[3], pvalue[3],
		    psize[4], pvalue[4]);

		/*
		** If ule_format failed, we probably can't report any error.
		** Instead, just propagate the error up to the user.
		*/
		if (uleret != E_DB_OK)
		{
		    STprintf(errbuf, "Error message corresponding to %d (%x) \
			not found in INGRES error file", detail, detail);
		    msglen = STlength(errbuf);
		    uleFormat(NULL, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL,
			errbuf, msglen, 0, &ulecode, 0);
		    *err_code = ulecode;
		}
	    }
	    uleret = uleFormat(&localDBerror, 0, NULL, uletype, &sqlstate, errbuf,
		(i4) sizeof(errbuf), &msglen, &ulecode,
		NumParms,
		psize[0], pvalue[0],
		psize[1], pvalue[1],
		psize[2], pvalue[2],
		psize[3], pvalue[3],
		psize[4], pvalue[4]);

	    /*
	    ** If ule_format failed, we probably can't report any error.
	    ** Instead, just propagate the error up to the user.
	    */
	    if (uleret != E_DB_OK)
	    {
		char	    *misc_ing_err_sqlstate = SS50000_MISC_ERRORS;
		i4	    i;
		
		STprintf(errbuf, "Error message corresponding to %d (%x) \
		    not found in INGRES error file", errorno, errorno);
		msglen = STlength(errbuf);
		uleFormat(NULL, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL, errbuf,
		    msglen, 0, &ulecode, 0);
		*err_code = ulecode;

		/* store something relatively meaningful into sqlstate */
		for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
		    sqlstate.db_sqlstate[i] = misc_ing_err_sqlstate[i];
	    }
	}

	/*
	** Only send text of user errors to user.  Internal error has already
	** been logged by ule_format.
	*/
	if (err_type == PSF_USERERR)
	{
	    /* get session control block */
	    sess_cb = psf_sesscb();
	    if ( sess_cb->pss_retry & PSS_REPORT_MSGS)
	    {
		scf_cb.scf_length = sizeof(scf_cb);
		scf_cb.scf_type = SCF_CB_TYPE;
		scf_cb.scf_facility = DB_PSF_ID;
		scf_cb.scf_session = sess_cb->pss_sessid;
		scf_cb.scf_nbr_union.scf_local_error = errorno;

		STRUCT_ASSIGN_MACRO(sqlstate,
				    scf_cb.scf_aux_union.scf_sqlstate);
		scf_cb.scf_len_union.scf_blength = msglen;
		scf_cb.scf_ptr_union.scf_buffer = errbuf;
		ret_val = scf_call(SCC_ERROR, &scf_cb);
		if (ret_val != E_DB_OK)
		{
		    *err_code = scf_cb.scf_error.err_code;
		    break;
		}
	    }
	    else
	    {
		/* set to indicate caller should retry if caller is so
		** inclined.  However, messages will continue to be
		** suppressed. */
		sess_cb->pss_retry |= PSS_ERR_ENCOUNTERED;

		/*
		** if the message is being issued in the course of reparsing a
		** dbproc to determine whether it is active or grantable, log
		** the message
		*/
		if (   sess_cb->pss_dbp_flags & PSS_RECREATE
		    && sess_cb->pss_dbp_flags &
		           (PSS_0DBPGRANT_CHECK | PSS_CHECK_IF_ACTIVE)
		   )
		{
		    uleFormat(&localDBerror, 0, NULL, ULE_MESSAGE, (DB_SQLSTATE *) NULL,
			errbuf, msglen, &msglen, &ulecode, 0);
		}
	    }
	}

	*err_code = 0L;
	
	/* leave loop has already been set to TRUE */
    } while (!leave_loop);

    return (ret_val);
}

DB_STATUS
psfErrorFcn(
    i4		errorno,
    i4		detail,
    i4		err_type,
    i4		*err_code,
    DB_ERROR	*err_blk,
    PTR		FileName,
    i4		LineNumber,
    i4		num_parms,
		...)
{
    DB_STATUS status;
    va_list	ap;
    va_start(ap, num_parms);
    status = psf_verror(errorno, detail, err_type, err_code, err_blk,
		FileName, LineNumber, num_parms, ap);
    va_end(ap);
    return status;
}

/* Non-variadic function forms, insert __FILE__, __LINE__ manually */
DB_STATUS
psf_errorNV(
    i4		errorno,
    i4		detail,
    i4		err_type,
    i4		*err_code,
    DB_ERROR	*err_blk,
    i4		num_parms,
		...)
{
    DB_STATUS status;
    va_list	ap;

    va_start(ap, num_parms);
    status = psf_verror(errorno, detail, err_type, err_code, err_blk,
		__FILE__, __LINE__, num_parms, ap);
    va_end(ap);
    return status;
}

/*{
** Name: PSF_ADF_ERROR	- display ADF errors.
**
** Description:
**      This routine takes an ADF error block and processes it properly for
**  PSF. As a result an error message is diplayed to the user.
**
** Inputs:
**      adf_errcb                       the adf error block
**	err_blk				error block (usually from psq_cb)
**
** Outputs:
**      err_blk
**	    err_code			one of the following
**						E_PS0001_USER_ERROR
**	Returns:
**	    None.
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      25-oct-87 (stec)
**          Written
**	21-may-89 (jrb)
**	    Now sends generic error code from adf cb through to scc_error.
**	13-jun-89 (fred)
**	    If error class in ADF error block is EXTERNAL_ERROR, then
**	    send back a 'raise error' style message so that the FE's won't
**	    attempt to enforce their brand of aesthetics on the message.
**	12-sep-90 (teresa)
**	    fix faulty pss_retry logic.
**	14-aug-92 (barbara)
**	    pss_retry is now a bit field, so don't use equality to test it.
**	24-oct-92 (andre)
**	    both SCF_CB and ADF_ERROR now contain an SQLSTATE (DB_SQLSTATE)
**	    instead of generic error (i4)
**	03-mar-93 (rblumer)
**	    change psq_cb to err_blk
*/
VOID
psf_adf_error(
	ADF_ERROR   *adf_errcb,
	DB_ERROR    *err_blk,
	PSS_SESBLK  *sess_cb)
{
    SCF_CB		scf_cb;
    i4			op_code;

    if (sess_cb->pss_retry & PSS_REPORT_MSGS)
    {
	scf_cb.scf_facility = DB_PSF_ID;
	scf_cb.scf_session = sess_cb->pss_sessid;
	scf_cb.scf_length = sizeof(SCF_CB);
	scf_cb.scf_type = SCF_CB_TYPE;

	if (adf_errcb->ad_errclass != ADF_INTERNAL_ERROR)
	{
	    SETDBERR(err_blk, 0, E_PS0001_USER_ERROR);
	    scf_cb.scf_ptr_union.scf_buffer  = adf_errcb->ad_errmsgp;
	    scf_cb.scf_len_union.scf_blength = adf_errcb->ad_emsglen;
	    scf_cb.scf_nbr_union.scf_local_error = adf_errcb->ad_usererr;
	    STRUCT_ASSIGN_MACRO(adf_errcb->ad_sqlstate,
				scf_cb.scf_aux_union.scf_sqlstate);
	    if (adf_errcb->ad_errclass == ADF_USER_ERROR)
	    {
		op_code = SCC_ERROR;
	    }
	    else
	    {
		op_code = SCC_RSERROR;
	    }
	    (VOID) scf_call(op_code, &scf_cb);
	}
    }
    else
    {
	/* set to indicate caller should retry if caller is so
	** inclined.  However, messages will continue to be suppressed. */

	sess_cb->pss_retry |= PSS_ERR_ENCOUNTERED;
    }
}

/*{
** Name: psf_rdf_error	- Format and report an RDF error in PSF.
**
** Description:
**
**      This procedure formats and reports an RDF error in PSF. The main
**	purpose of this routine is to provide a place where RDF error
**	processing within PSF will occur, so that when new RDF error return
**	codes are added only one routine needs to be modified.
**	It is important to note, however, that this routine is not `aware'
**	of the context of the RDF call, therefore it may only handle general
**	type of errors; routines calling RDF still need to handle context
**	specific type of errors.
**
** Inputs:
**      rdf_opcode			RDF call type
**      rdf_errblk			RDF error block
**	err_blk				Pointer to error block
**
** Outputs:
**	err_blk				Filled in with error
** Returns:
**	None
** Exceptions:
**	None
**
** Side Effects:
**	Can cause message to be logged or sent to user.
**
** History:
**	08-dec-85 (stec)
**          Written.
**	21-dec-87 (stec)
**	    Added handling for E_RD0025_USER_ERROR (maps from E_QE0025...).
**	16-mar-88 (stec)
**	    Added handling for E_RD0200_NO_TUPLES.
**	04-auf-88 (stec)
**	    Changed the define for E_PS0904.
**	04-may-90 (andre)
**	    Add translation for E_RD0145_ALIAS_MEM.
**	25-mar-92 (barbara)
**	    Merge of Star RDF error code for Sybil and comment:
**	    12-jun-89 (andre)
**		handle E_RDO277_CANNOT_GET_ASSOCIATION
**	30-apr-1992 (bryanp)
**	    Translate E_RD002A_DEADLOCK into E_PS0009_DEADLOCK.
**      10-feb-93 (teresa)
**          Added case to handle RDF_READTUPLES errors.
**	02-dec-1996 (nanpr01)
**	    Mapped the E_RD002B_LOCK_TIMER_EXPIRED in correct place.
**    17-April-2003 (inifa01) bugs 110096 & 109815 INGREP 133.
**        E_DM004D and E_RD002B should not be returned to the user or 
**        printed to the error log.  This change suppresses printing of
**        E_DM004D in rdu_ferror() and re-mapps E_RD002B here to E_US125E.
*/

VOID
psf_rdf_error(
	i4	    rdf_opcode,
	DB_ERROR    *rdf_errblk,
	DB_ERROR    *err_blk)
{
    i4 err_code;
    i4 psferror;

    switch (rdf_errblk->err_code)
    {
	case E_RD002B_LOCK_TIMER_EXPIRED:
           /* Re-mapped E_RD002B to E_US125E (4702L) instead of 
           ** E_PS0010.
           */
           (VOID) psf_error(4702L, 0L,
                 PSF_USERERR, &err_code, err_blk, 0);
            break;

	case E_RD000C_USER_INTR:
	    SETDBERR(err_blk, 0, E_PS0003_INTERRUPTED);
	    break;

	case E_RD000D_USER_ABORT:
	    SETDBERR(err_blk, 0, E_PS0005_USER_MUST_ABORT);
	    break;

	case E_RD0050_TRANSACTION_ABORTED:
	    (VOID) psf_error(E_PS0F03_TRANSACTION_ABORTED, 0L,
		PSF_USERERR, &err_code, err_blk, 0);
	    break;

	case E_RD0043_CACHE_FULL:
	    (VOID) psf_error(E_PS0F01_CACHE_FULL, 0L,
		PSF_USERERR, &err_code, err_blk, 0);
	    break;

	case E_RD0145_ALIAS_MEM:
	    (VOID) psf_error(E_PS0458_ULH_ALIAS_ERROR, 0L,
		PSF_USERERR, &err_code, err_blk, 0);
	    break;

	case E_RD001A_RESOURCE_ERROR:
	    (VOID) psf_error(E_PS0F04_RDF_RESOURCE_ERR, 0L,
		PSF_USERERR, &err_code, err_blk, 0);
	    break;

	case E_RD0025_USER_ERROR:
	    SETDBERR(err_blk, 0, E_PS0001_USER_ERROR);
	    break;

	case E_RD002A_DEADLOCK:
	    SETDBERR(err_blk, 0, E_PS0009_DEADLOCK);
	    break;

	case E_RD0200_NO_TUPLES:
	    (VOID) psf_error(E_PS0D31_NO_TUPLES, 0L,
		PSF_USERERR, &err_code, err_blk, 0);
	    break;

	case E_RD0277_CANNOT_GET_ASSOCIATION:
	    (VOID) psf_error(E_PS091B_NO_CONNECTION, 0L,
		PSF_USERERR, &err_code, err_blk, 0);
	    break;

	default:
	    switch (rdf_opcode)
	    {
		case RDF_GETDESC:
		    psferror = E_PS0904_BAD_RDF_GETDESC;
		    break;

		case RDF_GETINFO:
		    psferror = E_PS0D15_RDF_GETINFO;
		    break;

		case RDF_READTUPLES:
		    psferror = E_PS0D2A_RDF_READTUPLES;
		    break;

		case RDF_UNFIX:
		    psferror = E_PS0D17_RDF_UNFIX;
		    break;

		case RDF_UPDATE:
		    psferror = E_PS0D16_RDF_UPDATE;
		    break;

		case RDF_INVALIDATE:
		    psferror = E_PS0D29_RDF_INVALIDATE;
		    break;

		case RDF_INITIALIZE:
		    psferror = E_PS0D27_RDF_INITIALIZE;
		    break;

		case RDF_TERMINATE:
		    psferror = E_PS0D28_RDF_TERMINATE;
		    break;

		default:
		    psferror = E_PS0703_RDF_ERROR;
		    break;
	    }

	    (VOID) psf_error(psferror, rdf_errblk->err_code, 
		PSF_INTERR, &err_code, err_blk, 0);
	    break;
    }
}

/*{
** Name: psf_qef_error	- Format and report a QEF error in PSF.
**
** Description:
**      This procedure formats and reports a QEF error in PSF. The main
**	purpose of this routine is to provide a place where QEF error
**	processing within PSF will occur, so that when new QEF error return
**	codes are added only one routine needs to be modified.
**	It is important to note, however, that this routine is not `aware'
**	of the context of the QEF call, therefore it may only handle general
**	type of errors; routines calling QEF still need to handle context
**	specific type of errors.
**
** Inputs:
**      qef_opcode			QEF call type
**      qef_errblk			QEF error block
**	err_blk				Pointer to error block
**
** Outputs:
**	err_blk				Filled in with error
** Returns:
**	None
** Exceptions:
**	None
**
** Side Effects:
**	Can cause message to be logged or sent to user.
**
** History:
**	29-jun-90 (andre)
**	    written
**      17-oct-92 (robf)
**	    Added handling for E_QE0400_GATEWAY_ERROR so its reported
**          corrected as a GWF error rather than a misc. QEF error.
**	28-sep-93 (robf)
**          Added I_QE2033_SEC_VIOLATION 
**	16-nov-93  (robf)
**          Allow for 6500 (Security violation) to be returned from QEF
**	    as part of general error processing.
**	27-Feb-1997 (jenjo02)
**	    Extended <set transaction statement> to support <transaction access mode>
**	    (READ ONLY | READ WRITE) per SQL-92.
**	    Handle E_QE005B_TRAN_ACCESS_CONFLICT.
*/
VOID
psf_qef_error(
	i4	    qef_opcode,
	DB_ERROR    *qef_errblk,
	DB_ERROR    *err_blk)
{
    i4		user_err_no = 0L;
    i4		err;
    i4		err_code;

    switch (qef_errblk->err_code)
    {
	/* interrupt */
	case E_QE0022_QUERY_ABORTED:
	{
	    user_err_no = E_PS0003_INTERRUPTED;
	    err = 0L;
	    break;
	}
	/* should be retried */
	case E_QE0023_INVALID_QUERY:
	{
	    SETDBERR(err_blk, 0, E_PS0008_RETRY);
	    break;
	}
	/* user has some other locks on the object */
	case E_QE0051_TABLE_ACCESS_CONFLICT:
	{
	    user_err_no = E_PS0D21_QEF_ERROR;
	    err = 0L;
	    break;
	}
	/* conflict with transaction access mode */
	case E_QE005B_TRAN_ACCESS_CONFLICT:
	{
	    user_err_no = 5007L;
	    err = 0L;
	    break;
	}
	/* resource related */
	case E_QE000D_NO_MEMORY_LEFT:
	case E_QE000E_ACTIVE_COUNT_EXCEEDED:
	case E_QE000F_OUT_OF_OTHER_RESOURCES:
	case E_QE001E_NO_MEM:
	{
	    user_err_no = E_PS0D23_QEF_ERROR;
	    err = qef_errblk->err_code;
	    break;
	}
	/* lock timer */
	case E_QE0035_LOCK_RESOURCE_BUSY:
	case E_QE0036_LOCK_TIMER_EXPIRED:
	{
	    user_err_no = 4702L;
	    err = qef_errblk->err_code;
	    break;
	}
	/* resource quota */
	case E_QE0052_RESOURCE_QUOTA_EXCEED:
	case E_QE0067_DB_OPEN_QUOTA_EXCEEDED:
	case E_QE0068_DB_QUOTA_EXCEEDED:
	case E_QE006B_SESSION_QUOTA_EXCEEDED:
	{
	    user_err_no = 4707L;
	    err = qef_errblk->err_code;
	    break;
	}
	/* log full */
	case E_QE0024_TRANSACTION_ABORTED:
	{
	    user_err_no = 4706L;
	    err = qef_errblk->err_code;
	    break;
	}
	/* deadlock */
	case E_QE002A_DEADLOCK:
	{
	    user_err_no = 4700L;
	    err = qef_errblk->err_code;
	    break;
	}
	/* lock quota */
	case E_QE0034_LOCK_QUOTA_EXCEEDED:
	{
	    user_err_no = 4705L;
	    err = qef_errblk->err_code;
	    break;
	}
	/* Security violation */
	case I_QE2033_SEC_VIOLATION:
	case 6500:
	{
	    user_err_no = 6500;
	    err = qef_errblk->err_code;
	    break;
	}
	/* Gateway error */
	case E_QE0400_GATEWAY_ERROR:
	{
	    user_err_no = E_PS110E_GATEWAY_ERROR;
	    err = qef_errblk->err_code;
	    break;
	}
	/* inconsistent database */
	case E_QE0099_DB_INCONSISTENT:
	{
	    user_err_no = 38L;
	    err = qef_errblk->err_code;
	    break;
	}
	case E_QE0025_USER_ERROR:
	{
	  SETDBERR(err_blk, 0, E_PS0001_USER_ERROR);
	  break;
	}
	default:
	{
	    (VOID) psf_error(E_PS0D20_QEF_ERROR, qef_errblk->err_code,
		PSF_INTERR, &err_code, err_blk, 0);
	    break;
	}
    }

    if (user_err_no != 0L)
    {
	(VOID) psf_error(user_err_no, err, PSF_USERERR,
	    &err_code, err_blk, 0);
    }

    return;
}

