/*
**	Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <mh.h>
#include    <ex.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adfint.h>
/**
**
**  Name: ADXHANDLER.C - Handling of ADF exceptions and warnings.
**
**  Description:
**      This file contains all of the routines necessary to
**	handle any ADF known exception or warning.
**
**      This file defines:
**
**          adx_chkwarn() - Check for ADF warnings to be processed (mostly used
**			    for exception warnings).
**          adx_handler() - ADF exception handling routine.
**
**  History:
**      27-feb-86 (thurston)
**          Initial creation.
**      13-may-86 (thurston)
**          Initial coding.
**      16-may-86 (thurston)
**          Made return statuses more generic for the adx_handler() routine.
**	02-jun-86 (thurston)
**	    Changed comments in the adx_handler() routine for the return
**	    statuses so that they say "ADF exception" instead of "math
**	    exception."
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	09-aug-86 (ericj)
**	    Redesigned interface to accomodate math warning messages and
**	    implemented.  Added adx_chkwarn() to this file.
**      13-feb-87 (thurston)
**          In adx_chkwarn():  Added checking for the .ad_noprint_cnt warning.
**          Also, fixed bug whereby the warning counts were not being reset to
**          zero by this routine.  Also fixed bug where the .ad_fltund_cnt
**          warnint was never being checked (.ad_fltdiv_cnt was being checked
**          twice). 
**      23-feb-87 (thurston)
**          Fixed bug in the way adx_chkwarn() was calling adu_error() with
**	    parameters.  The value of the parameter was being passed not the
**	    address.
**      29-jul-87 (thurston)
**          Added code to adx_chkwarn() to handle the null char cvt'ed to blank
**	    in text warning. 
**	02-dec-88 (jrb)
**	    Now handles ad_agabsdate_cnt which tells how many times an absolute
**	    date was encountered while doing the avg() or sum() aggregates.
**	18-jul-89 (jrb)
**	    Added support for new decimal exceptions.
**      16-may-90 (jkb)
**          Integrate jrb's changes for handling hard errors
**      29-dec-1992 (stevet)
**          Added function prototype.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**	25-aug-93 (ed)
**	    remove dbdbms.h
**	26-feb-96 (thoda04)
**	    Added adfint.h for function prototype for adu_error().
**	    Added ulf.h for adfint.h. 
**      19-feb-1999 (matbe01)
**          Move include for ex.h after include for mh.h. The check for "ifdef
**          EXCONTINUE" in ex.h does no good if sys/context.h (via mh.h) is
**          included after ex.h.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/


/*{
** Name: adx_chkwarn() - Check for ADF warnings to be processed (used mostly
**			 for exception warnings).
**
** Description:
**      This routine checks if there are any ADF warnings to be processed.
**      This is done by scanning the adf_warncb field, an ADI_WARN struct,
**      in the ADF_CB.  If a non-zero value is encountered during the scan,
**      a warning message is generated and placed in the ADF_ERROR struct of
**      the ADF_CB, the value of the field is reset to zero, and a DB_STATUS
**      of E_DB_WARN is returned.  If no warning is encountered, a DB_STATUS
**      of E_DB_OK is returned.  The caller is expected to call this routine
**      iteratively, saving or sending the warning message each time, until
**      the DB_STATUS of E_DB_OK is returned. 
**
** Inputs:
**      adf_scb                         Ptr to an ADF_CB struct.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_warncb			An ADI_WARN struct.
**		.ad_intdiv_cnt		Number of integer divisions by zero
**					that occurred.
**		.ad_intovf_cnt		Number of integer overflows.
**		.ad_fltdiv_cnt		Number of float divisions by zero.
**		.ad_fltovf_cnt		Number of float overflows.
**		.ad_fltund_cnt		Number of float underflows.
**		.ad_mnydiv_cnt		Number of money divisions by zero.
**		.ad_noprint_cnt		Number of times the non-print char
**					cvt'ed to blanks in the `c' datatype
**					warning occured.
**		.ad_textnull_cnt	Number of times the null char
**					cvt'ed to blank in the `text' datatype
**					warning occured.
**		.ad_agabsdate_cnt	Number of times an abs date was
**					aggregated into sum() or avg()
**
** Outputs:
**      adf_scb
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**	    .adf_warncb			An ADI_WARN struct.  The fields listed
**					above for adf_warncb will be reset to
**					zero as non-zero values are encountered
**					on iterative calls to this routine.
**
**	Returns:
**	    E_DB_OK			If no ADF exception warning was found.
**	    E_DB_WARN			If an ADF exception warning was found
**					and a warning message was formatted.
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**		E_AD0100_NOPRINT_CHAR
**		E_AD0102_NULL_IN_TEXT
**		E_AD0120_INTDIV_WARN
**		E_AD0121_INTOVF_WARN
**		E_AD0122_FLTDIV_WARN
**		E_AD0123_FLTOVF_WARN
**		E_AD0124_FLTUND_WARN
**		E_AD0125_MNYDIV_WARN
**		E_AD0125_DECDIV_WARN
**		E_AD0125_DECOVF_WARN
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      07-aug-86 (ericj)
**          Initial creation.
**      13-feb-87 (thurston)
**          Added checking for the .ad_noprint_cnt warning.  Also, fixed bug
**          whereby the warning counts were not being reset to zero by this
**          routine.  Also fixed bug where the .ad_fltund_cnt warnint was never
**          being checked (.ad_fltdiv_cnt was being checked twice).
**      23-feb-87 (thurston)
**          Fixed bug in the way adu_error() was being called with parameters.
**          The value of the parameter was being passed not the address.
**      29-jul-87 (thurston)
**          Added code to handle the null char cvt'ed to blank in text warning.
**	02-dec-88 (jrb)
**	    Now handles ad_agabsdate_cnt which tells how many times an absolute
**	    date was encountered while doing the avg() or sum() aggregates.
**	18-jul-89 (jrb)
**	    Added support for new decimal exceptions.
**	11-Apr-2008 (kschendel)
**	    Quick check for no warnings if warncount is zero.
**      09-nov-2010 (gupsh01) SIR 124685
**          Protype cleanup.
[@history_template@]...
*/

DB_STATUS
adx_chkwarn(
ADF_CB             *adf_scb)
{
    DB_STATUS		db_stat;

    if (adf_scb->adf_warncb.ad_anywarn_cnt == 0)
	return (E_DB_OK);
    if (adf_scb->adf_warncb.ad_intdiv_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0120_INTDIV_WARN, 2,
			    (i4) sizeof(adf_scb->adf_warncb.ad_intdiv_cnt),
			    (i4 *) &adf_scb->adf_warncb.ad_intdiv_cnt);
	adf_scb->adf_warncb.ad_intdiv_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_intovf_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0121_INTOVF_WARN, 2,
			    (i4) sizeof(adf_scb->adf_warncb.ad_intovf_cnt),
			    (i4 *) &adf_scb->adf_warncb.ad_intovf_cnt);
	adf_scb->adf_warncb.ad_intovf_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_fltdiv_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0122_FLTDIV_WARN, 2,
			    (i4) sizeof(adf_scb->adf_warncb.ad_fltdiv_cnt),
			    (i4 *) &adf_scb->adf_warncb.ad_fltdiv_cnt);
	adf_scb->adf_warncb.ad_fltdiv_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_fltovf_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0123_FLTOVF_WARN, 2,
			    (i4) sizeof(adf_scb->adf_warncb.ad_fltovf_cnt),
			    (i4 *) &adf_scb->adf_warncb.ad_fltovf_cnt);
	adf_scb->adf_warncb.ad_fltovf_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_fltund_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0124_FLTUND_WARN, 2,
			    (i4) sizeof(adf_scb->adf_warncb.ad_fltdiv_cnt),
			    (i4 *) &adf_scb->adf_warncb.ad_fltdiv_cnt);
	adf_scb->adf_warncb.ad_fltund_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_mnydiv_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0125_MNYDIV_WARN, 0);
	adf_scb->adf_warncb.ad_mnydiv_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_decdiv_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0126_DECDIV_WARN, 2,
			    (i4) sizeof(adf_scb->adf_warncb.ad_decdiv_cnt),
			    (i4 *) &adf_scb->adf_warncb.ad_decdiv_cnt);
	adf_scb->adf_warncb.ad_decdiv_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_decovf_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0127_DECOVF_WARN, 2,
			    (i4) sizeof(adf_scb->adf_warncb.ad_decovf_cnt),
			    (i4 *) &adf_scb->adf_warncb.ad_decovf_cnt);
	adf_scb->adf_warncb.ad_decovf_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_noprint_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0100_NOPRINT_CHAR, 0);
	adf_scb->adf_warncb.ad_noprint_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_textnull_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0102_NULL_IN_TEXT, 0);
	adf_scb->adf_warncb.ad_textnull_cnt = 0;
	return (db_stat);
    }

    if (adf_scb->adf_warncb.ad_agabsdate_cnt)
    {
	db_stat = adu_error(adf_scb, E_AD0500_ABS_DATE_IN_AG, 2,
		(i4) sizeof(adf_scb->adf_warncb.ad_agabsdate_cnt),
		(i4 *) &adf_scb->adf_warncb.ad_agabsdate_cnt);
	adf_scb->adf_warncb.ad_agabsdate_cnt = 0;
	return (db_stat);
    }

    return (E_DB_OK);
}



/*{
** Name: adx_handler()  - ADF exception handling.
**
** Description:
**	This function is the external ADF call "adx_handler()" which will look
**	at the exception that has ocurred and, if it was an ADF known
**	exception, take the appropriate actions.  If this exception is one of
**	the math exceptions (EXFLTUND, EXFLTOVF, EXFLTDIV, EXINTOVF, EXINTDIV,
**	EXMNYDIV, EXDECOVF, EXDECDIV) the routine will handle it in a manner
**	that is consistent with the supplied math exception option (the setting
**	of the `-x' flag, found in the ADF session control block).
**
**      NOTE: This routine is not designed to be "declared" as an exception
**      handler.  Instead, adx_handler() is designed to be called by
**      someone else's exception handler during exception processing to
**      see if the exception can be handled by ADF.  The .adf_exmathopt code
**	in the ADF_CB will have one of three values:  ADX_IGN_MATHEX,
**	ADX_WRN_MATHEX, or ADX_ERR_MATHEX.  In the INGRES DBMS,
**	during query processing, the value of this should be determined
**	by the user settable option for math exception handling.
**	If called during constant folding the .adf_exmathopt may also have
**	the value ADX_IGNALL_MATHEX meaning to ignore those exceptions that
**	would usually signal past ADX_IGN_MATHEX.
**
**      Note that the existence of this routine does not preclude
**      ADF declaring an exception handler if it needs to for some
**      obscure reason.  However, this is very unlikely.
**
**      Here is how I envision ADF exception handling:  Say that QEF is
**      processing a query, calls ADF to perform a division, and BOOM:
**      EXFLTDIV occurs!  ADF has not declared an exception handler, so
**      the last one declared by QEF takes over.  It first wants to check
**      if the exception was an ADF exception, so it decides to call
**      adx_handler().  This routine is going to need to know what to
**      do with math exceptions (like the one that has just occurred).
**	There are three options that are user settable (that is, can be
**	set for each session):
**          (1) IGNORE:  React as if nothing ever happened.
**        * (2) WARN:    Give user a warning message, then continue.
**        * (3) ERROR:   Give user an error message, then abort the query.
**	This option is set in the .adf_exmathopt field of the ADF_CB.
**	Since the QEF handler will not have a pointer to the ADF_CB
**	for this session, it will have to call the SCF routine,
**	scu_information(), to get this pointer.  Frontends will have
**	this address by referencing the ADF_CB as a global variable.
**
**      (Note that other ADF callers, such as DMF, might want to always
**      generate an error on math exceptions, and never need to look at
**      the setting of this option.  Only during query execution should
**      this user settable flag be used.)
**
**	    *   A related issue here:  adx_handler() will only format the error
**	      messages, and will not be responsible for issuing them to the
**	      user, since this would imply an SCF call, and we have already
**	      ascertained that ADF can not use SCF since various front ends
**	      will use ADF and not SCF.
**		Also, if the warning option is set and one or more warnings
**	      have occurred during the execution of the query, the ADF routine,
**	      adx_chkwarn(), should be called at the end of the query.  This
**	      routine will scan the ADF_CB checking for various warnings that
**	      might have occurred.  When it finds a warning count in the
**	      ADF_CB, it will format a warning message, reset the warning count
**	      to zero, and return a DB_STATUS of E_DB_WARN.  This routine
**	      should be called iteratively until it returns E_DB_OK signaling
**	      that no more warnings have been found.
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.
**	    .adf_exmathopt		This is a flag to tell ADF how
**					to handle the exception if it is
**					a math exception.  It must have
**					one of the following values:
**						ADX_IGN_MATHEX
**						ADX_WRN_MATHEX
**						ADX_ERR_MATHEX
**						ADX_IGNALL_MATHEX
**	adx_exstruct			This is a pointer to the EX_ARGS
**					structure containing the
**					exception that has just ocurred.
**		.exarg_count		# of i4's in this structure.
**		.exarg_num		The exception that just ocurred.
**		.exarg_array[]		Optional arguments, as many as
**					specified by .exarg_count - 2.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen = 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**
**      Returns:
**	    E_DB_OK			If the exception is to be ignored.
**					In this case the ad_errcode field
**					in the ADF_ERRCB will be set to
**					E_AD0001_EX_IGN_CONT.
**	    E_DB_WARN			If the exception is to be warned,
**					the ad_errcode field will be set
**					E_AD0115_EX_WRN_CONT, or if the caller
**					is to look elsewhere for the exception,
**					ad_errcode field will be set to
**					E_AD0116_EX_UNKNOWN.
**	    E_DB_ERROR			If the exception is to be query fatal,
**					ad_errcode field is set to an error
**					number in the range of E_AD1120_... to
**                                      E_AD1170_..., or if an internal error
**					occurred in this routine, ad_errcode is
**					set to some appropriate ADF internal
**					error.
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**          E_AD0001_EX_IGN_CONT        This was an ADF exception, and I have
**                                      handled it by ignoring it and now
**                                      control should resume at the instruction
**                                      after the one that caused the exception
**                                      to be raised (EXCONTINUE). 
**
**          E_AD0115_EX_WRN_CONT        This was an ADF exception, and I have
**                                      handled it by marking a warning in the
**                                      ADI_WARN block of the ADF_CB.  Control
**                                      should now resume at the instruction
**                                      after the one that caused the exception
**                                      to be raised (EXCONTINUE). 
**
**          E_AD0116_EX_UNKNOWN         I don't know about this exception, so
**                                      don't bug me.  If you don't know about
**                                      it either, you had better do an
**                                      EXRESIGNAL. 
**
**	    E_AD1120_INTDIV_ERROR	Integer divide by zero ...
**	    E_AD1121_INTOVF_ERROR	Integer overflow ...
**	    E_AD1122_FLTDIV_ERROR	Floating point divide by zero ...
**	    E_AD1123_FLTOVF_ERROR	Floating point overflow ...
**	    E_AD1124_FLTUND_ERROR	Floating point underflow ...
**	    E_AD1125_MNYDIV_ERROR	Money divide by zero ...
**	    E_AD1126_DECDIV_ERROR	Decimal divide by zero...
**	    E_AD1127_DECOVF_ERROR	Decimal overflow...
**					All of these mean:
**                                      This was an ADF exception, and I have
**                                      handled it by formatting an error
**                                      message for you to issue to the user
**                                      and, after you have done this, control
**                                      should resume at the instruction
**                                      immediately after the one that declared
**                                      you (i.e. the exception handler)
**                                      (EXDECLARE).  The code there should
**                                      cause the routine to clean up, if
**                                      necessary, and return with an
**                                      appropriate error condition. 
**
**	    E_AD6000_BAD_MATH_ARG	Bad argument to an MH routine ... always
**					generates an ERROR condition.
**	    E_AD6001_BAD_MATHOPT	The ADX_MATHEX_OPT found in the ADF_CB
**					in not a valid one.
**
**          (If other ADF exceptions are defined,
**              more status codes may also be
**               defined to deal with them.)
**
**      Exceptions:
**          none
**
** Side Effects:
**          none
**
** History:
**      27-feb-86 (thurston)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	09-aug-86 (ericj)
**	    Redesigned interface to accomodate math warning messages and
**	    implemented.
**	18-jul-89 (jrb)
**	    Added support for new decimal exceptions.
**      16-may-90 (jkb)
**          Integrate jrb's changes for handling hard errors
**	11-Apr-2008 (kschendel)
**	    Count any-warnings as well as individual counters.
**	27-Nov-2009 (kiria01) b122966
**	    Allow ADX_IGNALL_MATHEX to ignore more exceptions
**	    than ADX_IGN_MATHEX.
*/

DB_STATUS
adx_handler(
ADF_CB             *adf_scb,
EX_ARGS            *adx_exstruct)
{
    i4		error_code;
    bool		hard = FALSE;
    ADX_MATHEX_OPT exmathopt = adf_scb->adf_exmathopt;

    switch (adx_exstruct->exarg_num)
    {
	case MH_BADARG:
	    /* Bad arg to MH routine ... always generate an error unless
	    ** IGNALL set */
	    if (exmathopt != ADX_IGNALL_MATHEX)
	    {
		error_code = E_AD6000_BAD_MATH_ARG;
		break;
	    }
	    /*FALLTHROUGH*/
	/*
	**  The following are math exceptions recognized by ADF.
	*/
	case EXHFLTOVF:
	case EXHFLTUND:
	case EXHFLTDIV:
	case EXHINTOVF:
	case EXHINTDIV:
			hard = TRUE;
	case EXFLTOVF:
	case EXFLTUND:
	case EXFLTDIV:
	case EXINTOVF:
	case EXINTDIV:
	case EXMNYDIV:
	case EXDECOVF:
	case EXDECDIV:

	    /* Set up Warning or Error in the internal message buffer */

	    if (!hard && exmathopt == ADX_IGN_MATHEX ||
		exmathopt == ADX_IGNALL_MATHEX)
	    {
		adf_scb->adf_errcb.ad_errcode = E_AD0001_EX_IGN_CONT;
		return (E_DB_OK);
	    }
	    else if (!hard && exmathopt == ADX_WRN_MATHEX)
	    {
		error_code  = E_AD0115_EX_WRN_CONT;
		switch (adx_exstruct->exarg_num)
		{
		  case EXFLTOVF:
		    adf_scb->adf_warncb.ad_fltovf_cnt++;
		    break;

		  case EXFLTUND:
		    adf_scb->adf_warncb.ad_fltund_cnt++;
		    break;

		  case EXFLTDIV:
		    adf_scb->adf_warncb.ad_fltdiv_cnt++;
		    break;

		  case EXINTOVF:
		    adf_scb->adf_warncb.ad_intovf_cnt++;
		    break;

		  case EXINTDIV:
		    adf_scb->adf_warncb.ad_intdiv_cnt++;
		    break;

		  case EXMNYDIV:
		    adf_scb->adf_warncb.ad_mnydiv_cnt++;
		    break;

		  case EXDECOVF:
		    adf_scb->adf_warncb.ad_decovf_cnt++;
		    break;

		  case EXDECDIV:
		    adf_scb->adf_warncb.ad_decdiv_cnt++;
		    break;

		  default:
		    error_code	= E_AD9999_INTERNAL_ERROR;
		    break;
		}
		if (error_code == E_AD0115_EX_WRN_CONT)
		    ++adf_scb->adf_warncb.ad_anywarn_cnt;
	    }
	    else if (hard || exmathopt == ADX_ERR_MATHEX)
	    {
		switch (adx_exstruct->exarg_num)
		{
		  case EXFLTOVF:
		  case EXHFLTOVF:
		    error_code	= E_AD1123_FLTOVF_ERROR;
		    break;

		  case EXFLTUND:
		  case EXHFLTUND:
		    error_code	= E_AD1124_FLTUND_ERROR;
		    break;

		  case EXFLTDIV:
		  case EXHFLTDIV:
		    error_code	= E_AD1122_FLTDIV_ERROR;
		    break;

		  case EXINTOVF:
		  case EXHINTOVF:
		    error_code	= E_AD1121_INTOVF_ERROR;
		    break;

		  case EXINTDIV:
		  case EXHINTDIV:
		    error_code	= E_AD1120_INTDIV_ERROR;
		    break;

		  case EXMNYDIV:
		    error_code	= E_AD1125_MNYDIV_ERROR;
		    break;

		  case EXDECOVF:
		    error_code	= E_AD1127_DECOVF_ERROR;
		    break;

		  case EXDECDIV:
		    error_code	= E_AD1126_DECDIV_ERROR;
		    break;

		  default:
		    error_code	= E_AD9999_INTERNAL_ERROR;
		    break;
		}
	    }
	    else
	    {
		error_code  = E_AD6001_BAD_MATHOPT;
	    }

	    break;

	default:
	    error_code = E_AD0116_EX_UNKNOWN;
	    break;
    }

    return (adu_error(adf_scb, error_code, 0));
}
