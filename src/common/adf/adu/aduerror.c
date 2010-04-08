/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <er.h>
#include    <cm.h>
#include    <me.h>
#include    <st.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <ulf.h>
#include    <adftrace.h>
#include    <adfint.h>
#include    <usererror.h>
#include    <generr.h>
#include    <sqlstate.h>

/**
**
**  Name: ADUERROR.C -	    contains on the support routines necessary for
**			    error-handling in ADF.
**
**  Description:
**        This file contains all the routines necessary to do error-handling
**	in ADF. 
**
**          aduErrorFcn() -	format an error msg for an ADF error code
**				and translate it to the appropriate DB_STATUS.
**
**
**  History:
**      15-jul-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	07-aug-86 (ericj)
**	    Updated.  See adu_error() history.
**      02-aug-86 (thurston)
**          Added error codes E_AD550A through E_AD550E.
**	03-nov-86 (ericj)
**	    Added error code E_AD1025_BAD_ERROR_NUM.
**	20-nov-86 (thurston)
**	    Added error code E_AD2030_LIKE_ONLY_FOR_SQL.
**	06-jan-87 (thurston)
**	    Added error codes E_AD0101_EMBEDDED_CHAR_TRUNC and
**	    E_AD1013_EMBEDDED_NUMOVF.
**	09-jan-87 (thurston)
**	    Added error codes E_AD1006_BAD_OUTARG_VAL, E_AD1007_BAD_QLANG,
**	    and E_AD1008_BAD_MATHEX_OPT.
**      28-jan-87 (thurston)
**          In adu_error(), I added a check at the front of the routine to just
**	    return E_DB_OK if the E_AD0000_OK error code was supplied.  Without
**	    this, the routine would return something about an unknown error.
**	23-feb-87 (thurston)
**	    Added error code E_AD550F_BAD_CX_REQUEST.
**	24-feb-87 (thurston)
**	    Added error code E_AD1009_BAD_SRVCB.
**	06-mar-87 (thurston)
**	    Added error code E_AD100A_BAD_NULLSTR.
**      16-mar-87 (thurston)
**	    Added error code E_AD1050_NULL_HISTOGRAM.
**      16-mar-87 (thurston)
**          Added new language param to ERlookup() calls.
**      06-apr-87 (thurston)
**          Made `null to non-null' a user error.  Also got rid of the
**          E_AD0FFF.... mappings that made it look like certain ADF routines
**          were not implemented yet, when in fact it was supposedly the error
**          message that hadn't been implemented, yet.
**      06-apr-87 (thurston)
**          Added E_AD1014_BAD_VALUE_FOR_DT.
**      21-apr-87 (thurston)
**          Added E_AD1015_BAD_RANGE.
**      29-apr-87 (thurston)
**          Added E_AD6000_BAD_MATH_ARG.
**      04-may-87 (thurston)
**          Added a check to see if trace point ADF_008_NO_ERLOOKUP is set, and
**          if so, avoid calling ERlookup().
**	07-jun-87 (thurston)
**	    Added user error E_AD5061_DGMT_ON_INTERVAL.
**      07-jun-87 (thurston)
**          Now omits error message lookup on expected ADF internal errors.
**      24-jun-87 (thurston)
**          Added E_AD200A_NOCOPYCOERCION.
**      01-jul-87 (thurston)
**          Added E_AD5002_BAD_NUMBER_TYPE and E_AD1030_F_COPY_STR_TOOSHORT.
**      29-jul-87 (thurston)
**          Added E_AD0102_NULL_IN_TEXT.
**      08-oct-87 (thurston)
**          Added E_AD3050_BAD_CHAR_IN_STRING.
**      29-dec-87 (thurston)
**          Made E_AD1014_BAD_VALUE_FOR_DT be an expected error to avoid message
**	    lookup cost.
**      04-feb-88 (thurston)
**          Re-coded the calls to ERlookup() to meet spec.
**      04-feb-88 (thurston)
**          Added code to check for a NULL adf_scb, and if NULL, just return the
**	    appropriate DB_STATUS.
**      03-mar-88 (thurston)
**          Changed the declaration of p1, p3, and p5 to be i4s instead of
**	    pointers to i4s in adu_error().
**      28-jun-88 (thurston)
**          Added the following new error codes for the ESCAPE clause on the
**	    LIKE and NOT LIKE operators:
**			E_AD1016_PMCHARS_IN_RANGE
**			E_AD1017_ESC_AT_ENDSTR
**			E_AD1018_BAD_ESC_SEQ
**			E_AD5510_BAD_DTID_FOR_ESCAPE
**			E_AD5511_BAD_DTLEN_FOR_ESCAPE
**      09-aug-88 (thurston)
**          Added the following error codes as internal errors for the GCA-ADF
**	    work, and a couple of up-and-coming DECIMAL errors:
**			E_AD200B_BAD_PREC
**			E_AD200C_BAD_SCALE
**			E_AD200D_BAD_BASE_DTID
**			E_AD2040_INCONSISTENT_TPL_CNT
**			E_AD2041_TPL_ARRAY_TOO_SMALL
**			E_AD2042_MEMALLOC_FAIL
**			E_AD2050_NO_COMVEC_FUNC
**      12-aug-88 (thurston)
**          Added E_AD2043_ATOMIC_TPL.
**      01-sep-88 (thurston)
**          Added E_AD100B_BAD_MAXSTRING.
**      26-sep-88 (thurston)
**          Added E_AD5005_BAD_DI_FILENAME.
**	06-oct-88 (thurston)
**	    Made error codes E_AD2009_NOCOERCION and E_AD200A_NOCOPYCOERCION be
**	    considered `expected' errors to avoid looking up messages every time
**	    we don't find a coercion.
**	01-dec-88 (jrb)
**	    Added E_AD505E_NOABSDATES and mapped to E_US10DC_NOABSDATES.
**	05-dec-88 (jrb)
**	    Added E_AD0500_ABS_DATE_IN_AG and mapped to E_US10DD_ABSDATEINAG.
**	08-feb-89 (greg)
**	    add ERlookup num_params argument
**	22-may-89 (jrb)
**	    Changed for generic error support: generic error from ERlookup is
**	    placed into ad_generic_err field of ADF_ERROR struct.
**	01-jun-89 (jrb)
**	    Set ad_generic_err field for expected errors too.  Always set to
**	    E_GE98BC_OTHER_ERROR.
**	06-jun-89 (jrb)
**	    Added errors 2060-2063 for datatype resolution move to ADF.
**	24-jul-89 (jrb)
**	    Added new decimal-related errors and put ER_TIMESTAMP into ERlookup
**	    calls so time and date would show up in the front-ends.
**	28-aug-89 (mikem)
**	    Adding E_AD5080_LOGKEY_BAD_CVT_LEN error.
**      05-mar-1990 (fred)
**	    Added peripheral datatype error support -- ad2002
**      21-jan-1991 (jennifer)
**	    Added error code E_AD1061_BAD_SECURITY_LABEL.
**	09-mar-92 (jrb, merged by stevet)
**	    Added E_AD2080_IFTRUE_NEEDS_INT and E_AD2085_LOCATE_NEEDS_STR.
**	09-mar-92 (jrb, merged by stevet)
**	    Added E_AD1019, E_AD2100, E_AD2101.
**	03-oct-1992 (fred)
**	    Added bit errors (1070-1072).
**	24-oct-92 (andre)
**	    replaced calls to ERlookup() to calls to ERslookup()
**
**	    in ADF_ERROR, ad_generic_err got replaced with ad_sqlstate
**	30-nov-1992 (rmuth)
**	    File Extend V, add the error messages E_AD2102,E_AD2103,
**	    E_AD2104 and E_AD2105. These are related to the FEXI functions
**	    adu_allocated_pages and adu_overflow_pages.
**      11-jan-1993 (stevet)
**          Added E_AD2090_BAD_DT_FOR_STRFUNC, E_AD1080_STR_TO_DECIMAL and 
**          E_AD1081_STR_TO_FLOAT.
**      03-feb-1993 (stevet)
**          Added E_AD1090_BAD_DTDEF for invalid DEFAULT values.
**      08-mar-1993 (stevet)
**          Changed to allow adf_errcb->ad_ebuflen to be negative values,
**          which behaves the same way when this value is zero.  This is
**          done to support ADF_ERRMSG_OFF and ADF_ERRMSG_ON macros to
**          temporary disable the ADF message formatting option.
**	21-mar-1993 (robf)
**	    Added handling for Security labels
**       6-Jul-1993 (fred)
**          Added adu_ome_error() routine.  This routine is called by
**          users to place their errors in the ADF scb.  These can
**          then be returned to users, or dealt with by whomever.
**	14-jul-93 (ed)
**	    replacing <dbms.h> by <gl.h> <sl.h> <iicommon.h> <dbdbms.h>
**       5-Aug-1993 (fred)
**          Fixed use of un initialized variable in adu_ome_error().
**      11-aug-93 (ed)
**	    added missing includes
**      12-aug-1993 (stevet)
**          Added E_AD1082_STR_TRUNCATE error.
**	25-aug-93 (ed)
**	    remove dbdbms.h
**      12-aug-1993 (stevet)
**          Added E_AD1083_UNTERM_C_STR.
**	1-nov-93 (robf)
**          Added E_AD1098_BAD_LEN_SEC_INTRNL
**      13-Apr-1994 (fred)
**          Added handling for E_AD7010_ADP_USER_INTR (user interrupt)
**          and E_AD7011_BAD_CPNIFY_OVER (coupon size error).
**      22-apr-1994 (stevet/rog)
**          Added E_AD7012_ADP_WORKSPACE_TOOLONG;
**      21-jan-1999 (hanch04)
**          replace nat and longnat with i4
**	26-May-1999 (shero03)
**	    Added E_AD2092_BAD_START_FOR_SUBSTR;
**	09-Jun-1999 (shero03)
**	    Added E_AD5007_BAD_HEX_CHAR
**      22-Jul-1999 (hanal04)
**          Added E_AD101B_BAD_FLT_VALUE. b98034.
**      04-Feb-2000 (gupsh01)
**          Added handling of error E_AD5008_BAD_IP_ADDRESS. (bug 100265) 
**      16-apr-2001 (stial01)
**          Added support for tableinfo function
**      27-jun-2001 (gupsh01)
**          Added E_AD2093_BAD_LEN_FOR_SUBSTR.
**	16-apr-2002 (gupsh01)
**	    Added E_AD5010_SOURCE_STRING_TRUNC.
**      16-oct-2006 (stial01)
**          Added locator errors.
**      12-dec-2006 (gupsh01)
**          Added errors for bad ANSI date/time format.
**      20-jul-2007 (stial01)
**          Added locator errors.
**      22-Apr-2009 (Marty Bowes) SIR 121969
**          Add E_AD2055_CHECK_DIGIT_SCHEME, E_AD2056_CHECK_DIGIT_STRING
**          and E_AD2057_CHECK_DIGIT_EMPTY
**          and E_AD2058_CHECK_DIGIT_STRING_LENGTH:
**          for the generation and validation of checksum values.
**      26-oct-2009 (joea)
**          Add cases for E_AD106A_INV_STR_FOR_BOOL_CAST and
**          E_AD106B_INV_INT_FOR_BOOL_CAST in aduErrorFcn.
**/
    

/*
**  Defines of other constants.
*/

/*  Define the maximum number of error parameters allowed. */
#define			    MAX_ERR_PARAMS	6


/* {@fix_me@} */
/*  The following define is only temporary.  It should really be formalized
**  in adf.h.
*/
#define			ADF_WARN_BOUNDARY		(E_AD_MASK + 0x0100L)
#define			ADF_ERR_BOUNDARY		(E_AD_MASK + 0x1000L)
#define			ADF_FATAL_BOUNDARY		(E_AD_MASK + 0x9000L)

/*
**  Dummies to hand to ult_check_macro():
*/

static	i4	    dum1;	/* Dummy to give to ult_check_macro() */
static	i4	    dum2;	/* Dummy to give to ult_check_macro() */


/*{
** Name: aduErrorFcn()	-   format an error message for an ADF error code and
**			    translate to appropriate DB_STATUS code.
**
** Description:
**        This routine is used to format an error message for an ADF error
**	code and then translates the error code to a DB_STATUS code to
**	be returned.  Note, the return status in this procedure is overloaded.
**	It is intended to map an ADF error code to the appropriate DB_STATUS
**	error code.  However, if an error occurs in executing this routine,
**	it will be signaled by returning an E_DB_ERROR code.  This case
**	can be determined by looking at the contents of the ad_errcode field
**	in the ADF error block, adf_errcb.
**	  This routine will also format ADF warning messages.
[@comment_line@]...
**
** Inputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.
**		.ad_ebuflen		The length, in bytes, of the buffer
**					pointed to by ad_errmsgp.  If <= 0, no
**					error message will be formatted.
**		.ad_errmsgp		Pointer to a buffer to put formatted
**					error message in, if necessary.  If
**					NULL, no error message will be
**					formatted.
**	    .adf_slang			Spoken language in use.
**      adf_errorcode                   The ADF error code to format a message
**					for and translate to a DB_STATUS code.
**	FileName			Pointer to filename of the adu_error
**					macro making this call.
**	LineNumber			Line number within that file.
**	pcnt				This is a count of the optional
**					parameters which should be formated
**					into the error message.  If no
**					parameters are necessary, this should
**					be zero.
**	[p1 ... p6]			Optional parameter pairs.  Optional
**					parameters should come in pairs where
**					the first member designates the second
**					values' length as specified by the
**					ERslookup() protocol and the second
**					member should be the address of the
**					value to be interpreted.
**
** Outputs:
**	adf_scb				Pointer to an ADF session control block.
**	    .adf_errcb			ADF_ERROR struct.  If an
**					error occurs the following fields will
**					be set.  NOTE: if .ad_ebuflen <= 0 or
**					.ad_errmsgp = NULL, no error message
**					will be formatted.
**		.ad_errcode		ADF error code for the error.
**		.ad_errclass		Signifies the ADF error class.
**		.ad_usererr		If .ad_errclass is ADF_USER_ERROR,
**					this field is set to the corresponding
**					user error which will either map to
**					an ADF error code or a user-error code.
**		.ad_sqlstate		This field is set to the SQLSTATE status
**					code this error maps to.
**		.ad_emsglen		The length, in bytes, of the resulting
**					formatted error message.
**		.adf_errmsgp		Pointer to the formatted error message.
**		.adf_dberror
**		    .err_code		Duplicates ad_errcode
**		    .err_data		0
**		    .err_file		FileName
**		    .err_line		LineNumber
**
**	Returns:
**	      The following DB_STATUS codes may be returned:
**	    E_DB_OK, E_DB_WARN, E_DB_ERROR, E_DB_SEVERE, E_DB_FATAL
**
**	      If a DB_STATUS code other than E_DB_OK is returned, the caller
**	    can look in the field adf_scb.adf_errcb.ad_errcode to determine
**	    the ADF error code.  The following is a list of possible ADF error
**	    codes that can be returned by this routine:
**
**	    E_AD0000_OK			If the ADF error was actually OK.
**	    E_AD1020_BAD_ERROR_LOOKUP	If ERlookup could not find the error
**					code that was passed to it in the
**					error message file.
**	    E_AD1021_BAD_ERROR_PARAMS	If the too many parameter arguments
**					are passed in or an odd number of
**					parameter arguments is passed in.
**	    E_AD1025_BAD_ERROR_NUM	The ADF error code passed in was
**					unknown to adu_error().
**
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      17-jul-86 (ericj)
**          Initial creation.
**      27-jul-86 (ericj)
**	    Converted for new ADF error handling.
**	07-aug-86 (ericj)
**	    Added cases to handle networking errors and to handle ADF
**	    exception errors and warnings.
**      02-aug-86 (thurston)
**          Added error codes E_AD550A through E_AD550E.
**	03-nov-86 (ericj)
**	    Added error code E_AD1025_BAD_ERROR_NUM.
**	20-nov-86 (thurston)
**	    Added error code E_AD2030_LIKE_ONLY_FOR_SQL.
**	06-jan-87 (thurston)
**	    Added error codes E_AD0101_EMBEDDED_CHAR_TRUNC and
**	    E_AD1013_EMBEDDED_NUMOVF.
**      28-jan-87 (thurston)
**          Added a check at the front of the routine to just return E_DB_OK
**          if the E_AD0000_OK error code was supplied.  Without this, the
**          routine would return something about an unknown error.
**	23-feb-87 (thurston)
**	    Added error code E_AD550F_BAD_CX_REQUEST.
**	24-feb-87 (thurston)
**	    Added error code E_AD1009_BAD_SRVCB.
**	06-mar-87 (thurston)
**	    Added error code E_AD100A_BAD_NULLSTR.
**      16-mar-87 (thurston)
**	    Added error code E_AD1050_NULL_HISTOGRAM.
**      16-mar-87 (thurston)
**          Added new language param to ERlookup() calls.
**      06-apr-87 (thurston)
**          Made `null to non-null' a user error.  Also got rid of the
**          E_AD0FFF.... mappings that made it look like certain ADF routines
**          were not implemented yet, when in fact it was supposedly the error
**          message that hadn't been implemented, yet.
**      06-apr-87 (thurston)
**          Added E_AD1014_BAD_VALUE_FOR_DT.
**      21-apr-87 (thurston)
**          Added E_AD1015_BAD_RANGE.
**      29-apr-87 (thurston)
**          Added E_AD6000_BAD_MATH_ARG.
**      04-may-87 (thurston)
**          Added a check to see if trace point ADF_008_NO_ERLOOKUP is set, and
**          if so, avoid calling ERlookup().
**	07-jun-87 (thurston)
**	    Added user error E_AD5061_DGMT_ON_INTERVAL.
**      07-jun-87 (thurston)
**          Now omits error message lookup on expected ADF internal errors.
**      24-jun-87 (thurston)
**          Added E_AD200A_NOCOPYCOERCION.
**      01-jul-87 (thurston)
**          Added E_AD5002_BAD_NUMBER_TYPE and E_AD1030_F_COPY_STR_TOOSHORT.
**      29-jul-87 (thurston)
**          Added E_AD0102_NULL_IN_TEXT.
**      29-dec-87 (thurston)
**          Made E_AD1014_BAD_VALUE_FOR_DT be an expected error to avoid message
**	    lookup cost.
**      04-feb-88 (thurston)
**          Re-coded the calls to ERlookup() to meet spec.
**      04-feb-88 (thurston)
**          Added code to check for a NULL adf_scb, and if NULL, just return the
**	    appropriate DB_STATUS.
**      03-mar-88 (thurston)
**          Changed the declaration of p1, p3, and p5 to be i4s instead of
**	    pointers to i4s.
**      28-jun-88 (thurston)
**          Added the following new error codes for the ESCAPE clause on the
**	    LIKE and NOT LIKE operators:
**			E_AD1016_PMCHARS_IN_RANGE
**			E_AD1017_ESC_AT_ENDSTR
**			E_AD1018_BAD_ESC_SEQ
**			E_AD5510_BAD_DTID_FOR_ESCAPE
**			E_AD5511_BAD_DTLEN_FOR_ESCAPE
**      09-aug-88 (thurston)
**          Added the following error codes as internal errors for the GCA-ADF
**	    work, and a couple of up-and-coming DECIMAL errors:
**			E_AD200B_BAD_PREC
**			E_AD200C_BAD_SCALE
**			E_AD200D_BAD_BASE_DTID
**			E_AD2040_INCONSISTENT_TPL_CNT
**			E_AD2041_TPL_ARRAY_TOO_SMALL
**			E_AD2042_MEMALLOC_FAIL
**			E_AD2050_NO_COMVEC_FUNC
**      12-aug-88 (thurston)
**          Added E_AD2043_ATOMIC_TPL.
**      01-sep-88 (thurston)
**          Added E_AD100B_BAD_MAXSTRING.
**      26-sep-88 (thurston)
**          Added E_AD5005_BAD_DI_FILENAME.
**	06-oct-88 (thurston)
**	    Made error codes E_AD2009_NOCOERCION and E_AD200A_NOCOPYCOERCION be
**	    considered `expected' errors to avoid looking up messages every time
**	    we don't find a coercion.
**      26-sep-88 (thurston)
**          Added E_AD8999_FUNC_NOT_IMPLEMENTED.
**	02-dec-88 (jrb)
**	    Added E_AD505E_NOABSDATES no absolute dates allowed for interval()
**	    function.
**	05-dec-88 (jrb)
**	    Added E_AD0500_ABS_DATE_IN_AG and mapped to E_US10DD_ABSDATEINAG.
**	22-may-89 (jrb)
**	    Changed for generic error support: generic error from ERlookup is
**	    placed into ad_generic_err field of ADF_ERROR struct.
**	01-jun-89 (jrb)
**	    Set ad_generic_err field for expected errors too.  Always set to
**	    E_GE98BC_OTHER_ERROR.
**	06-jun-89 (jrb)
**	    Added errors 2060-2063 for datatype resolution move to ADF.
**	24-jul-89 (jrb)
**	    Added new decimal errors and put ER_TIMESTAMP into ERlookup calls.
**	28-aug-89 (mikem)
**	    Adding E_AD5080_LOGKEY_BAD_CVT_LEN error.
**	24-oct-92 (andre)
**	    replaced calls to ERlookup() to calls to ERslookup()
**
**	    in ADF_ERROR, ad_generic_err got replaced with ad_sqlstate
**      03-nov-1992 (stevet)
**          Added E_AD101A_DT_NOT_SUPPORTED error. 
**	30-nov-1992 (rmuth)
**	    File Extend V, add the error messages E_AD2102,E_AD2103,
**	    E_AD2104 and E_AD2105. These are related to the FEXI functions
**	    adu_allocated_pages and adu_overflow_pages.
**	21-mar-1993 (robf)
**	    Added handling for E_AD1061_BAD_SECURITY_LABEL
**	25-nov-1998 (nanpr01)
**	    Map E_AD5062_DATEOVFLO to usererror message.
**      22-Jul-1999 (hanal04)
**          Handle E_AD101B so that it can be passed back to caller. b98034.
**      04-Feb-2000 (gupsh01)
**          Added handling of error E_AD5008_BAD_IP_ADDRESS. (bug 100265)
**	03-nov-2000 (somsa01)
**	    Added E_AD1074_SOP_NOT_ALLOWED.
**	19-Feb-2004 (gupsh01)
**	    Added E_AD5011_NOCHARMAP_FOUND and E_AD5012_UCETAB_NOT_EXISTS.
**	11-apr-05 (inkdo01)
**	    Added E_AD50A0_BAD_UUID_PARM.
**	22-apr-05 (inkdo01)
**	    Added E_AD5081_UNICODE_FUNC_PARM.
**	05-sep-06 (gupsh01)
**	    Added E_AD5064_DATEANSI.
**	14-sep-06 (gupsh01)
**	    Added E_AD5065_DATE_ALIAS_NOTSET.
**	16-nov-06 (gupsh01)
**	    Added E_AD5066_DATE_COERCION.
**	22-nov-06 (gupsh01)
**	    Added E_AD5067_ANSIDATE_4DIGYR for ansidates having less than 
**	    4 digits in year component.
**	10-dec-2006 (gupsh01)
**	    Added E_AD5068_ANSIDATE_DATESUB and E_AD5069_ANSIDATE_DATEADD.
**	12-dec-2006 (gupsh01)
**	    Added Ansi date/time formatting errors.
**	09-jan-2007 (gupsh01)
**	    Added E_AD5078_ANSITIME_INTVLADD error.
**	10-Feb-2008 (kiria01) b120043
**	    Added E_US10F1_DATE_DIV_BY_ZERO, E_US10F2_DATE_DIV_ANSI_INTV
**	    and E_US10F3_DATE_ARITH_NOABS to map AD5090/1/2/3/4
**      10-Mar-2008 (coomi01) : b119995
**          Map E_AD2051_BAD_ANSITRIM_LEN onto E_US10F4_4340_TRIM_SPEC
**	25-Jun-2008 (kiria01) SIR120473
**	    Added E_AD5512_BAD_DTID2_FOR_ESCAPE and
**	    E_AD5513_BAD_DTLEN2_FOR_ESCAPE
**	18-Sep-2008 (jonj)
**	    SIR 120874: Renamed from adu_error() to aduErrorFcn(), invoked
**	    by adu_error macro to include FileName, LineNumber source
**	    information.
**	29-Oct-2008 (jonj)
**	    SIR 120874: Add non-variadic macros and functions for
**	    compilers that don't support them.
**	07-Jan-2009 (kiria01) SIR120473
**	    Added E_AD1026_PAT_TOO_CPLX & E_AD1027_PAT_LEN_LIMIT
**	9-sep-2009 (stephenb)
**	    Add E_AD2107_LASTID_FCN_ERR internal error case.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Add E_AD7016_ADP_MVCC_INCOMPAT user error.
**	18-Mar-2010 (kiria01) b123438
**	    Added E_AD1028_NOT_ZEROONE_ROWS mapping to user error
**	    E_US1196_NOT_ZEROONE_ROWS for SINGLETON aggregate.
**      23-Mar-2010 (hanal04) Bug 122436
**          Map E_AD2085_LOCATE_NEEDS_STR to E_US1072_BAD_LOCATE_ARG.
**          
[@history_template@]...
*/

/*VARARGS3*/
DB_STATUS
aduErrorFcn(
ADF_CB		    *adf_scb,
i4         	    adf_errorcode,
PTR		    FileName,
i4		    LineNumber,
i4		    pcnt,
		    ...)
{

    DB_STATUS           ret_status = E_DB_OK;
    DB_STATUS		tmp_status;
    CL_SYS_ERR		sys_err;
    i4			final_msglen;
#define MAX_ER_ARGS MAX_ERR_PARAMS / 2
    ER_ARGUMENT		params[MAX_ER_ARGS];
    ADF_ERROR		*adf_errcb;
    i4			msg_lkup;
    i4			num_params;
    va_list		ap;


    /* Set the DB_STATUS error code */

    if (adf_errorcode < ADF_WARN_BOUNDARY)
	ret_status = E_DB_OK;
    else if (adf_errorcode < ADF_ERR_BOUNDARY)
	ret_status = E_DB_WARN;
    else if (adf_errorcode < ADF_FATAL_BOUNDARY)
	ret_status = E_DB_ERROR;
	
	/* {@fix_me@} */
	/* maybe a severe boundary check should be done here. */

    else
	ret_status = E_DB_FATAL;


    if (adf_scb == NULL)
	return (ret_status);

    adf_errcb = &adf_scb->adf_errcb;

    if (adf_errorcode == E_AD0000_OK)
    {
	CLRDBERR(&adf_errcb->ad_dberror);
	adf_errcb->ad_errcode = adf_errorcode;
	return (E_DB_OK);
    }

    /*
    ** Plant caller's source information in the ADF_SCB's
    ** DB_ERROR.
    **
    ** We guarantee that err_code will match
    ** what ends up in adf_errcb.ad_errcode.
    */
    adf_errcb->ad_dberror.err_code = adf_errorcode;
    adf_errcb->ad_dberror.err_data = 0;
    adf_errcb->ad_dberror.err_file = FileName;
    adf_errcb->ad_dberror.err_line = LineNumber;



    /* Set the ADF error block's fields correctly */

    /* By default, set the error class to "user error".
    ** This is done because all of the internal errors in the switch
    ** stmt below perform the same action and it can easily be reset
    ** there for internal errors.
    */
    adf_errcb->ad_errclass  = ADF_USER_ERROR;
    adf_errcb->ad_errcode   = adf_errorcode;

    /* Set message lookup to TRUE.  Some ADF internal errors will want to
    ** avoid having to look up an error message, since these are `expected'
    ** errors that the calling is ready to handle.  For those errors, message
    ** lookup will be reset to FALSE.
    */
    msg_lkup = TRUE;


    switch (adf_errorcode)
    {
      case E_AD0002_INCOMPLETE:
	ret_status = E_DB_INFO;
	adf_errcb->ad_errclass	= ADF_INTERNAL_ERROR;
	break;

      /* Internal ADF error and warnings that do NOT look up msgs follow: */
      case E_AD0001_EX_IGN_CONT:
      case E_AD0115_EX_WRN_CONT:
      case E_AD0116_EX_UNKNOWN:
      case E_AD1014_BAD_VALUE_FOR_DT:
      case E_AD1050_NULL_HISTOGRAM:
      case E_AD2009_NOCOERCION:
      case E_AD200A_NOCOPYCOERCION:
      case E_AD2022_UNKNOWN_LEN:
      case E_AD5505_UNALIGNED:
      case E_AD5506_NO_SPACE:
      case E_AD550A_RANGE_FAILURE:
	msg_lkup = FALSE;
	/* specifically no `break;' stmt here */

      /* Internal ADF error and warnings that DO look up msgs follow: */
      case E_AD0101_EMBEDDED_CHAR_TRUNC:
      case E_AD1001_BAD_DATE:
      case E_AD1002_BAD_MONY_SIGN:
      case E_AD1003_BAD_MONY_LORT:
      case E_AD1004_BAD_MONY_PREC:
      case E_AD1005_BAD_DECIMAL:
      case E_AD1006_BAD_OUTARG_VAL:
      case E_AD1007_BAD_QLANG:
      case E_AD1008_BAD_MATHEX_OPT:
      case E_AD1009_BAD_SRVCB:
      case E_AD100A_BAD_NULLSTR:
      case E_AD100B_BAD_MAXSTRING:
      case E_AD1010_BAD_EMBEDDED_TYPE:
      case E_AD1011_BAD_EMBEDDED_LEN:
      case E_AD1019_NULL_FEXI_PTR:
      case E_AD1020_BAD_ERROR_LOOKUP:
      case E_AD1021_BAD_ERROR_PARAMS:
      case E_AD1025_BAD_ERROR_NUM:
      case E_AD1040_CV_DVBUF_TOOSHORT:
      case E_AD1041_CV_NETBUF_TOOSHORT:
      case E_AD1080_STR_TO_DECIMAL:
      case E_AD1081_STR_TO_FLOAT:
      case E_AD2002_BAD_OPID:
      case E_AD2004_BAD_DTID:
      case E_AD2005_BAD_DTLEN:
      case E_AD200B_BAD_PREC:
      case E_AD200C_BAD_SCALE:
      case E_AD200D_BAD_BASE_DTID:
      case E_AD2010_BAD_FIID:
      case E_AD2020_BAD_LENSPEC:
      case E_AD2021_BAD_DT_FOR_PRINT:
      case E_AD2030_LIKE_ONLY_FOR_SQL:
      case E_AD2040_INCONSISTENT_TPL_CNT:
      case E_AD2041_TPL_ARRAY_TOO_SMALL:
      case E_AD2042_MEMALLOC_FAIL:
      case E_AD2043_ATOMIC_TPL:
      case E_AD2050_NO_COMVEC_FUNC:
      case E_AD2060_BAD_DT_COUNT:
      case E_AD2061_BAD_OP_COUNT:
      case E_AD2062_NO_FUNC_FOUND:
      case E_AD2063_FUNC_AMBIGUOUS:
      case E_AD2100_NULL_RESTAB_PTR:
      case E_AD2101_RESTAB_FCN_ERROR:
      case E_AD2102_NULL_ALLOCATED_PTR:
      case E_AD2103_ALLOCATED_FCN_ERR:
      case E_AD2104_NULL_OVERFLOW_PTR:
      case E_AD2105_OVERFLOW_FCN_ERR:
      case E_AD2106_TABLEINFO_FCN_ERR:
      case E_AD2107_LASTID_FCN_ERR:
      case E_AD3001_DTS_NOT_SAME:
      case E_AD3002_BAD_KEYOP:
      case E_AD3003_DLS_NOT_SAME:
      case E_AD3005_BAD_EQ_DTID:
      case E_AD3006_BAD_EQ_DTLEN:
      case E_AD3007_BAD_DS_DTID:
      case E_AD3008_BAD_DS_DTLEN:
      case E_AD3009_BAD_HP_DTID:
      case E_AD3010_BAD_HP_DTLEN:
      case E_AD3011_BAD_HG_DTID:
      case E_AD3012_BAD_HG_DTLEN:
      case E_AD4001_FIID_IS_AG:
      case E_AD4002_FIID_IS_NOT_AG:
      case E_AD4003_AG_WORKSPACE_TOO_SHORT:
      case E_AD4004_BAD_AG_DTID:
      case E_AD4005_NEG_AG_COUNT:
      case E_AD5001_BAD_STRING_TYPE:
      case E_AD5002_BAD_NUMBER_TYPE:
      case E_AD5004_OVER_MAXTUP:
      case E_AD5012_UCETAB_NOT_EXISTS:
      case E_AD5017_NOMAPTBL_FOUND:
      case E_AD5019_UDECOMP_COL:
      case E_AD5060_DATEFMT:
      case E_AD5500_BAD_SEG:
      case E_AD5501_BAD_SEG_FOR_ICODE:
      case E_AD5502_WRONG_NUM_OPRS:
      case E_AD5503_BAD_DTID_FOR_FIID:
      case E_AD5504_BAD_RESULT_LEN:
      case E_AD5507_BAD_DTID_FOR_KEYBLD:
      case E_AD5508_BAD_DTLEN_FOR_KEYBLD:
      case E_AD5509_BAD_RANGEKEY_FLAG:
      case E_AD550B_TOO_FEW_BASES:
      case E_AD550C_COMP_NOT_IN_PROG:
      case E_AD550D_WRONG_CX_VERSION:
      case E_AD550E_TOO_MANY_VLTS:
      case E_AD550F_BAD_CX_REQUEST:
      case E_AD5510_BAD_DTID_FOR_ESCAPE:
      case E_AD5511_BAD_DTLEN_FOR_ESCAPE:
      case E_AD5512_BAD_DTID2_FOR_ESCAPE:
      case E_AD5513_BAD_DTLEN2_FOR_ESCAPE:
      case E_AD6001_BAD_MATHOPT:
      case E_AD7000_ADP_BAD_INFO:
      case E_AD7001_ADP_NONEXT:
      case E_AD7002_ADP_BAD_GET:
      case E_AD7003_ADP_BAD_RECON:
      case E_AD7004_ADP_BAD_BLOB:
      case E_AD7005_ADP_BAD_POP:
      case E_AD7006_ADP_PUT_ERROR:
      case E_AD7007_ADP_GET_ERROR:
      case E_AD7008_ADP_DELETE_ERROR:
      case E_AD7009_ADP_MOVE_ERROR:
      case E_AD700A_ADP_REPLACE_ERROR:
      case E_AD700B_ADP_BAD_POP_CB:
      case E_AD700C_ADP_BAD_POP_UA:
      case E_AD700D_ADP_NO_COUPON:
      case E_AD700E_ADP_BAD_COUPON:
      case E_AD700F_ADP_BAD_CPNIFY:
      case E_AD7010_ADP_USER_INTR:
      case E_AD7011_ADP_BAD_CPNIFY_OVER:
      case E_AD7012_ADP_WORKSPACE_TOOLONG:
      case E_AD7013_ADP_CPN_TO_LOCATOR:
      case E_AD7014_ADP_LOCATOR_TO_CPN:
      case E_AD7015_ADP_FREE_LOCATOR:
      case E_AD9004_SECURITY_LABEL:
      case E_AD9998_INTERNAL_ERROR:
      case E_AD9999_INTERNAL_ERROR:
	adf_errcb->ad_errclass	= ADF_INTERNAL_ERROR;
	break;

      /* 
      ** User error codes that get mapped to an old 4.0
      ** (or new 6.0) user error code in ERUSF.MSG follow.
      */
    case E_AD0100_NOPRINT_CHAR:
	adf_errcb->ad_usererr	= E_US1080_NOPRINT_CHAR;
	break;

      case E_AD0120_INTDIV_WARN:
	adf_errcb->ad_usererr	= E_US1078_INTDIV_WARN;
	break;

      case E_AD0121_INTOVF_WARN:
	adf_errcb->ad_usererr	= E_US1077_INTOVF_WARN;
	break;

      case E_AD0122_FLTDIV_WARN:
	adf_errcb->ad_usererr	= E_US107A_FLTDIV_WARN;
	break;

      case E_AD0123_FLTOVF_WARN:
	adf_errcb->ad_usererr	= E_US1079_FLTOVF_WARN;
	break;

      case E_AD0124_FLTUND_WARN:
	adf_errcb->ad_usererr	= E_US107B_FLTUND_WARN;
	break;

      case E_AD0125_MNYDIV_WARN:
	adf_errcb->ad_usererr	= E_US113A_BAD_MNYDIV;
	break;

      case E_AD0500_ABS_DATE_IN_AG:
	adf_errcb->ad_usererr	= E_US10DD_ABSDATEINAG;
	break;

      case E_AD1120_INTDIV_ERROR:
	adf_errcb->ad_usererr	= E_US1069_INTDIV_ERROR;
	break;

      case E_AD1121_INTOVF_ERROR:
	adf_errcb->ad_usererr	= E_US1068_INTOVF_ERROR;
	break;

      case E_AD1122_FLTDIV_ERROR:
	adf_errcb->ad_usererr	= E_US106B_FLTDIV_ERROR;
	break;

      case E_AD1123_FLTOVF_ERROR:
	adf_errcb->ad_usererr	= E_US106A_FLTOVF_ERROR;
	break;

      case E_AD1124_FLTUND_ERROR:
	adf_errcb->ad_usererr	= E_US106C_FLTUND_ERROR;
	break;

      case E_AD1125_MNYDIV_ERROR:
	adf_errcb->ad_usererr	= E_US113A_BAD_MNYDIV;
	break;

      case E_AD2051_BAD_ANSITRIM_LEN:
	adf_errcb->ad_usererr = E_US10F4_4340_TRIM_SPEC;
	break;

      case E_AD3050_BAD_CHAR_IN_STRING:
	adf_errcb->ad_usererr	= E_US0A96_BAD_CHAR_IN_STR;
	break;

      case E_AD5003_BAD_CVTONUM:
	adf_errcb->ad_usererr	= E_US100F_BAD_CVTONUM;
	break;

      case E_AD5005_BAD_DI_FILENAME:
	adf_errcb->ad_usererr	= E_US1036_BAD_DI_FILENAME;
	break;

      case E_AD5007_BAD_HEX_CHAR:
	adf_errcb->ad_usererr	= E_US113B_BAD_HEX_CHAR;
	break;

      case E_AD5020_BADCH_MNY:
	adf_errcb->ad_usererr	= E_US1130_BADCH_MNY;
	break;

      case E_AD5021_MNY_SIGN:
	/* {@fix_me@} */
	/* Is this really the right mapping here? */
	adf_errcb->ad_usererr	= E_US1138_SIGN_MNY;
	break;

      case E_AD5022_DECPT_MNY:
       adf_errcb->ad_usererr  = E_US1136_DECPT_MNY;
	break;

      case E_AD5031_MAXMNY_OVFL:
       adf_errcb->ad_usererr  = E_US1131_MAXMNY_OVFL;
	break;

      case E_AD5032_MINMNY_OVFL:
       adf_errcb->ad_usererr	= E_US1132_MINMNY_OVFL;
	break;

      case E_AD5050_DATEADD:
	adf_errcb->ad_usererr	= E_US10CC_DATEADD;
	break;

      case E_AD5051_DATESUB:
	adf_errcb->ad_usererr	= E_US10CD_DATESUB;
	break;

      case E_AD5052_DATEVALID:
	adf_errcb->ad_usererr	= E_US10CE_DATEVALID;
	break;

      case E_AD5053_DATEYEAR:
	adf_errcb->ad_usererr	= E_US10CF_DATEYEAR;
	break;

      case E_AD5054_DATEMONTH:
	adf_errcb->ad_usererr	= E_US10D0_DATEMONTH;
	break;

      case E_AD5055_DATEDAY:
	adf_errcb->ad_usererr	= E_US10D1_DATEDAY;
	break;

      case E_AD5056_DATETIME:
	adf_errcb->ad_usererr	= E_US10D2_DATETIME;
	break;

      case E_AD5058_DATEBADCHAR:
	adf_errcb->ad_usererr	= E_US10D4_DATEBADCHAR;
	break;

      case E_AD5059_DATEAMPM:
	adf_errcb->ad_usererr	= E_US10D5_DATEAMPM;
	break;

      case E_AD505A_DATEYROVFLO:
	adf_errcb->ad_usererr	= E_US10D6_DATEYROVFLO;
	break;

      case E_AD505B_DATEYR:
	adf_errcb->ad_usererr	= E_US10D7_DATEYR;
	break;

      case E_AD505C_DOWINVALID:
	adf_errcb->ad_usererr	= E_US10D8_DOWINVALID;
	break;

      case E_AD505D_DATEABS:
	adf_errcb->ad_usererr	= E_US10D9_DATEABS;
	break;

      case E_AD505E_NOABSDATES:
	adf_errcb->ad_usererr	= E_US10DC_NOABSDATES;
	break;

      case E_AD505F_DATEINTERVAL:
	adf_errcb->ad_usererr	= E_US10DB_DATEINTERVAL;
	break;

      case E_AD5062_DATEOVFLO:
	adf_errcb->ad_usererr	= E_US10DE_DATEOVFL;
	break;

      case E_AD5064_DATEANSI:
	adf_errcb->ad_usererr	= E_US10DF_DATEANSI;
	break;

      case E_AD5065_DATE_ALIAS_NOTSET: 
	adf_errcb->ad_usererr	= E_US10E0_DATE_NOALIAS;
	break;

      case E_AD5066_DATE_COERCION: 
	adf_errcb->ad_usererr	= E_US10E1_DATE_COERCE;
	break;

      case E_AD5067_ANSIDATE_4DIGYR: 
	adf_errcb->ad_usererr	= E_US10E2_ANSIDATE_4DIGYR;
	break;

      case E_AD5068_ANSIDATE_DATESUB: 
	adf_errcb->ad_usererr	= E_US10E3_ANSIDATE_DATESUB;
	break;

      case E_AD5069_ANSIDATE_DATEADD: 
	adf_errcb->ad_usererr	= E_US10E4_ANSIDATE_DATEADD;
	break;

      case E_AD5070_ANSIDATE_BADFMT:
	adf_errcb->ad_usererr	= E_US10E5_ANSIDATE_BADFMT;
	break;

      case E_AD5071_ANSITMWO_BADFMT:
	adf_errcb->ad_usererr	= E_US10E6_ANSITMWO_BADFMT;
	break;

      case E_AD5072_ANSITMW_BADFMT:
	adf_errcb->ad_usererr	= E_US10E7_ANSITMW_BADFMT;
	break;

      case E_AD5073_ANSITSWO_BADFMT:
	adf_errcb->ad_usererr	= E_US10E8_ANSITSWO_BADFMT;
	break;

      case E_AD5074_ANSITSW_BADFMT:
	adf_errcb->ad_usererr	= E_US10E9_ANSITSW_BADFMT;
	break;

      case E_AD5075_ANSIINYM_BADFMT:
	adf_errcb->ad_usererr	= E_US10EA_ANSIINYM_BADFMT;
	break;

      case E_AD5076_ANSIINDS_BADFMT:
	adf_errcb->ad_usererr	= E_US10EB_ANSIINDS_BADFMT;
	break;

      case E_AD5077_ANSITMZONE_BADFMT:
	adf_errcb->ad_usererr	= E_US10EC_ANSITMZONE_BADFMT;
	break;

      case E_AD5078_ANSITIME_INTVLADD:
	adf_errcb->ad_usererr	= E_US10ED_ANSITIME_INTVLADD;
	break;

      case E_AD5092_DATE_DIV_BY_ZERO:
	adf_errcb->ad_usererr	= E_US10F1_DATE_DIV_BY_ZERO;
	break;

      case E_AD5093_DATE_DIV_ANSI_INTV:
	adf_errcb->ad_usererr	= E_US10F2_DATE_DIV_ANSI_INTV;
	break;

      case E_AD5094_DATE_ARITH_NOABS:
	adf_errcb->ad_usererr	= E_US10F3_DATE_ARITH_NOABS;
	break;

      case E_AD6000_BAD_MATH_ARG:
	adf_errcb->ad_usererr	= E_US1071_BAD_MATH_ARG;
	break;

      case E_AD5008_BAD_IP_ADDRESS:
        adf_errcb->ad_usererr   = E_US1081_BAD_IP_ADDRESS;
        break;

      case E_AD2085_LOCATE_NEEDS_STR:
        adf_errcb->ad_usererr   = E_US1072_BAD_LOCATE_ARG;
        break;

      case E_AD7016_ADP_MVCC_INCOMPAT:
        adf_errcb->ad_usererr   = E_US120C_INVALID_LOCK_LEVEL;
        break;

      case E_AD1028_NOT_ZEROONE_ROWS:
        adf_errcb->ad_usererr   = E_US1196_NOT_ZEROONE_ROWS;
        break;

      /* User error codes that keep their ADF error numbers */

      case E_AD0102_NULL_IN_TEXT:
      case E_AD0126_DECDIV_WARN:
      case E_AD0127_DECOVF_WARN:
      case E_AD1012_NULL_TO_NONNULL:
      case E_AD1013_EMBEDDED_NUMOVF:
      case E_AD1015_BAD_RANGE:
      case E_AD1016_PMCHARS_IN_RANGE:
      case E_AD1017_ESC_AT_ENDSTR:
      case E_AD1018_BAD_ESC_SEQ:
      case E_AD101A_DT_NOT_SUPPORTED:
      case E_AD101B_BAD_FLT_VALUE:
      case E_AD101C_BAD_ESC_SEQ:
      case E_AD101D_BAD_ESC_SEQ:
      case E_AD101E_ESC_SEQ_CONFLICT:
      case E_AD101F_REG_EXPR_SYN:
      case E_AD1026_PAT_TOO_CPLX:
      case E_AD1027_PAT_LEN_LIMIT:
      case E_AD1030_F_COPY_STR_TOOSHORT:
      case E_AD1061_BAD_SECURITY_LABEL:
      case E_AD1062_SECURITY_LABEL_LUB:
      case E_AD1063_SECURITY_LABEL_EXTERN:
      case E_AD1064_SECURITY_LABEL_EXTLEN:
      case E_AD1065_SESSION_LABEL:
      case E_AD1066_SOP_NOT_AVAILABLE:
      case E_AD1067_SOP_FIND:
      case E_AD1068_SOP_COMPARE:
      case E_AD1069_SOP_COLLATE:
      case E_AD106A_INV_STR_FOR_BOOL_CAST:
      case E_AD106B_INV_INT_FOR_BOOL_CAST:
      case E_AD1070_BIT_TO_STR_OVFL:
      case E_AD1071_NOT_BIT:
      case E_AD1072_STR_TO_BIT_OVFL:
      case E_AD1073_SOP_MAC_READ:
      case E_AD1074_SOP_NOT_ALLOWED:
      case E_AD1082_STR_TRUNCATE:
      case E_AD1083_UNTERM_C_STR:
      case E_AD1090_BAD_DTDEF:
      case E_AD1095_BAD_INT_SECLABEL:
      case E_AD1096_SESSION_PRIV:
      case E_AD1097_BAD_DT_FOR_SESSPRIV:
      case E_AD1098_BAD_LEN_SEC_INTRNL:
      case E_AD1126_DECDIV_ERROR:
      case E_AD1127_DECOVF_ERROR:
      case E_AD2001_BAD_OPNAME:
      case E_AD2003_BAD_DTNAME:
      case E_AD2006_BAD_DTUSRLEN:
      case E_AD2007_DT_IS_FIXLEN:
      case E_AD2008_DT_IS_VARLEN:
      case E_AD2055_CHECK_DIGIT_SCHEME:
      case E_AD2056_CHECK_DIGIT_STRING:
      case E_AD2057_CHECK_DIGIT_EMPTY:
      case E_AD2058_CHECK_DIGIT_STRING_LENGTH:
      case E_AD2070_NO_EMBED_ALLOWED:
      case E_AD2071_NO_PAREN_ALLOWED:
      case E_AD2072_NO_PAREN_AND_EMBED:
      case E_AD2073_BAD_USER_PREC:
      case E_AD2074_BAD_USER_SCALE:
      case E_AD2080_IFTRUE_NEEDS_INT:
      case E_AD2090_BAD_DT_FOR_STRFUNC:
      case E_AD2092_BAD_START_FOR_SUBSTR:
      case E_AD2093_BAD_LEN_FOR_SUBSTR:
      case E_AD2094_INVALID_LOCATOR:
      case E_AD20A0_BAD_LEN_FOR_PAD:
      case E_AD20B0_REPLACE_NEEDS_STR:
      case E_AD5010_SOURCE_STRING_TRUNC:
      case E_AD5011_NOCHARMAP_FOUND: 
      case E_AD5015_INVALID_BYTESEQ:
      case E_AD5016_NOUNIMAP_FOUND:
      case E_AD5061_DGMT_ON_INTERVAL:
      case E_AD5080_LOGKEY_BAD_CVT_LEN:
      case E_AD5081_UNICODE_FUNC_PARM:
      case E_AD5082_NON_UNICODE_DB:
      case E_AD5090_DATE_MUL:
      case E_AD5091_DATE_DIV:
      case E_AD50A0_BAD_UUID_PARM:
      case E_AD8999_FUNC_NOT_IMPLEMENTED:
	adf_errcb->ad_usererr	= adf_errorcode;
	break;

      default:
	/* Call adu_error() recursively to handle an unknown ADF error code. */
	return (aduErrorFcn(adf_scb, E_AD1025_BAD_ERROR_NUM, 
			  adf_errcb->ad_dberror.err_file,
			  adf_errcb->ad_dberror.err_line,
			  2,
			  (i4) sizeof(adf_errorcode),
			  (i4 *) &adf_errorcode));
    }


    /* If no error msg formating set generic error to internal error and    */
    /* set msg length to 0						    */
    if (    !msg_lkup
	||  adf_errcb->ad_errmsgp == NULL
	||  adf_errcb->ad_ebuflen <= 0
       )
    {
	char	    *src = SS5000B_INTERNAL_ERROR;
	char	    *dest = adf_errcb->ad_sqlstate.db_sqlstate;
	i4	    i;

	for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	    CMcpyinc(src, dest);

	adf_errcb->ad_emsglen = 0;
	return (ret_status);
    }


    /* If trace point to avoid ERslookup() calls is set, just return */

    if (ult_check_macro(&Adf_globs->Adf_trvect,ADF_008_NO_ERLOOKUP,&dum1,&dum2))
    {
	adf_errcb->ad_emsglen = 0;
	return (ret_status);
    }



    /* Format the error msg */

    /*
    ** Note that unlike everywhere else, "pcnt" is the number of things,
    ** not the number of pairs of things, a little odd...
    */


    /* Check for bad parameters being passed to this routine */

    if ((pcnt % 2) == 1  ||  pcnt > MAX_ERR_PARAMS)
    {
	adf_errcb->ad_errclass	= ADF_INTERNAL_ERROR;
	adf_errcb->ad_errcode	= E_AD1021_BAD_ERROR_PARAMS;
	adf_errorcode		= E_AD1021_BAD_ERROR_PARAMS;
	pcnt			= 0;
    }

    /* If this is a user error, we want to format the user error message
    ** and not the internal ADF error number.
    */
    if (adf_errcb->ad_errclass == ADF_USER_ERROR)
	adf_errorcode =adf_errcb->ad_usererr;

    /* Extract variable parameters */
    va_start( ap, pcnt );
    for ( num_params = 0; num_params < pcnt/2; num_params++ )
    {
        params[num_params].er_size = (i4) va_arg( ap, i4 );
        params[num_params].er_value = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    tmp_status  = ERslookup(adf_errorcode, 0, ER_TIMESTAMP,
			    adf_errcb->ad_sqlstate.db_sqlstate,
			    adf_errcb->ad_errmsgp,adf_errcb->ad_ebuflen,
			    adf_scb->adf_slang, &final_msglen, &sys_err,
			    num_params, &params[0]);

    if (tmp_status != E_DB_OK)
    {
	/* Start processing an error for a bad ERslookup() call rather
	** than the original error that generated the call to this routine.
	*/
	adf_errcb->ad_errcode	= E_AD1020_BAD_ERROR_LOOKUP;
	adf_errcb->ad_errclass	= ADF_INTERNAL_ERROR;
	/* It is assumed that the following call will not produce an error.
	** It had better be tested.
	*/

	params[0].er_size = 4;
	params[0].er_value = (PTR)&adf_errorcode;
	tmp_status  = ERslookup(E_AD1020_BAD_ERROR_LOOKUP, 0, ER_TIMESTAMP,
				adf_errcb->ad_sqlstate.db_sqlstate,
				adf_errcb->ad_errmsgp, adf_errcb->ad_ebuflen,
				adf_scb->adf_slang, &final_msglen, &sys_err,
				1, &params[0]);
	ret_status  = E_DB_ERROR;
    }

    adf_errcb->ad_emsglen	= final_msglen;
    adf_errcb->ad_dberror.err_code = adf_errorcode;

    return (ret_status);
}

/* Non-variadic function form, insert __FILE__, __LINE__ manually */
DB_STATUS
adu_errorNV(
ADF_CB		    *adf_scb,
i4         	    adf_errorcode,
i4		    pcnt,
		    ...)
{
    i4			ps[MAX_ER_ARGS];
    PTR			pv[MAX_ER_ARGS];
    i4			i;
    va_list		ap;

    va_start( ap, pcnt );
    for ( i = 0; i < pcnt/2 && i < MAX_ERR_PARAMS; i++ )
    {
        ps[i] = (i4) va_arg( ap, i4 );
        pv[i] = (PTR) va_arg( ap, PTR );
    }
    va_end( ap );

    return( aduErrorFcn(
    			adf_scb,
			adf_errorcode,
			__FILE__,
			__LINE__,
			pcnt,
			ps[0], pv[0],
			ps[1], pv[1],
			ps[2], pv[2] ) );
}

/*
** {
** Name:	adu_ome_error	- Insert user error in scb
**
** Description:
**	This routine is provided so that users of the OME package can
**	call it to insert error messages and/or numbers into the scb.
**	Using this routine removes the necessity of documenting either
**	all or part of the scb's internal structure.
**
** Re-entrancy:
**	Re-entrant.  Uses only passed parameters.
**
** Inputs:
**	scb             The ADF session control block.
**	e_number        The error number.
**	e_text          The error text itself (ptr).
**
** Outputs:
**	scb             Same control block filled with error as appropriate.
**
** Returns:
**	DB_STATUS
**
** Exceptions: (if any)
**      None.
**
** Side Effects: (if not obvious from description)
**	None.
**
** History:
**       6-Jul-1993 (fred)
**          Created.
**       5-Aug-1993 (fred)
**          Fixed use of uninitialized variable... :-(
**	18-Sep-2008 (jonj)
**	    Duplicate ad_errcode into ad_dberror.
*/
DB_STATUS
adu_ome_error(ADF_CB	    *scb,
	      i4       e_number,
	      char          *e_text)
{
    ADF_ERROR		*errcb;
    char	    	*src = SS5000B_INTERNAL_ERROR;
    char	    	*dest;
    i4	    	    	i;
    
    if (scb == NULL)
	return (E_DB_ERROR);


    errcb = &scb->adf_errcb;
    errcb->ad_errclass  = ADF_USER_ERROR;
    errcb->ad_errcode   = e_number;
    errcb->ad_dberror.err_code = e_number;
    errcb->ad_dberror.err_data = 0;
    errcb->ad_dberror.err_file = NULL;
    errcb->ad_dberror.err_line = 0;
    dest = errcb->ad_sqlstate.db_sqlstate;
    for (i = 0; i < DB_SQLSTATE_STRING_LEN; i++)
	CMcpyinc(src, dest);

    if (errcb->ad_errmsgp == NULL ||  errcb->ad_ebuflen <= 0)
	return(E_DB_OK);

    if (e_text)
    {
	errcb->ad_emsglen = min(STlength(e_text), errcb->ad_ebuflen);
	MEcopy(e_text, errcb->ad_emsglen, errcb->ad_errmsgp);
    }
    else
    {
	errcb->ad_emsglen = 0;
    }
    return(E_DB_OK);
}

