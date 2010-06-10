SQLSTATE_MAPPING_REQUIRED

/*
**    Copyright (c) 2004 Ingres Corporation 
**    Message file for Facility code: _AD
**
** History:
**
**      16-feb-1993 (dianeh)
**              Added NO_SQLSTATE_MAPPING to end of file.
**	22-mar-1993 (robf)
**		Added E_AD1062_SECURITY_LABEL_LUB, 
**		      E_AD1063_SECURITY_LABEL_EXTERN
**		      E_AD1064_SECURITY_LABEL_EXTLEN
**		      E_AD1065_SESSION_LABEL
**                    E_AD9004_SECURITY_LABEL
**	18-may-1993 (robf)
**		Added 	E_AD1066_SOP_NOT_AVAILABLE
**			E_AD1067_SOP_FIND
**			E_AD1068_SOP_COMPARE
**			E_AD1069_SOP_COLLATE
**      12-aug-1993 (stevet)
**             Added E_AD1082_STR_TRUNCATE.
**	8-sep-93 (robf)
**		Added	E_AD1095_BAD_INT_SECLABEL
**			E_AD1096_SESSION_PRIV, 
**			E_AD1097_BAD_DT_FOR_SESSPRIV
**	1-oct-93 (dianeh)
**		Cleaned up text destined for messages.txt
**	1-nov-93 (robf)
**		Added E_AD1098_BAD_LEN_SEC_INTRNL
**	06-dec-93 (dianeh)
**		Cleaned up text destined for messages.txt added after the
**		last cleanup.
**      20-jan-1994 (stevet)
**              Added E_AD1083_UNTERM_C_STR.
**      13-Apr-1994 (fred)
**              Added E_AD700F_ADP_USER_INTR message to propagate dmf
**  	    	messages about interrupts to QEF so that they don't
**  	    	get logged (sort of backwards, but that's the way it
**  	    	works). 
**      25-apr-1994 (stevet/rog)
**              Added E_AD7012_ADP_WORKSPACE_TOOLONG.
**	9-may-94 (robf)
**              Added suggestion to check error log for SOP errors.
**	18-june-1996 (sweeney)
**		Fix some typos, anglicisms.
**	09-oct-96 (mcgem01)
**		Remove hard-coded Ingres references so that these 
**		messages can be shared by both Ingres and Jasmine.
**	26-nov-1998 (nanpr01)
**		Added E_AD5062_DATEOVFLO for day interval overflow.
**	18-jan-99 (stephenb)
**		Added E_AD5063_BAD_SYSTIME
**      12-Jul-1999 (hanal04)
**          Added E_AD101B_BAD_FLT_VALUE for out of range floating point
**          values. b85041.
**	03-nov-2000 (somsa01)
**	    Added E_AD1074_SOP_NOT_ALLOWED.
**      16-apr-2001 (stial01)
**          Added support for iitableinfo function
**      27-jun-2001 (gupsh01)
**          Added E_AD2093_BAD_LEN_FOR_SUBSTR.
**	18-sept-01 (inkdo01)
**	    Added E_AD5008 for trim/pad/squeeze funcs with non-varying 
**	    result type.
**	18-sept-01 (inkdo01)
**	    Changed 5008 to 5009 because 5008 was already in use (though
**	    not in this file)
**      21-mar-02 (gupsh01)
**          Added new error E_AD5010_SOURCE_STRING_TRUNC to signify
**          trucated source string.
**	21-may-02 (gupsh01)
**	    Added new errors E_AD5011 and E_AD5012 for coercion from / to
**	    unicode.
**	01-aug-02 (gupsh01)
**	    Added new error E_AD5019_UDECOMP_COL to signify error in 
**	    attempting to carry out unicode decomposition. 
**      23-jan-2004 (gupsh01)
**          Added errors E_AD5015_INVALID_BYTESEQ and 
**	    E_AD5016_NOUNIMAP_FOUND for unicode coercion.
**	29-jan-2004 (gupsh01)
**	    Added error E_AD5017_NOMAPTBL_FOUND for error in finding
**	    the specified mapping tables for unicode conversion.
**	19-feb-2004 (gupsh01)
**	    Fixed value reporting in E_AD5011_NOCHARMAP_FOUND err message.
**	09-apr-2004 (abbjo03)
**	    Add text to E_AD6000.
**	15-Feb-2005 (schka24)
**	    AD9999 is worthless, add AD9998 which has a "where am I" param.
**	11-apr-05 (inkdo01)
**	    Added E_AD50A0 to document parm errors to uuid_from_char() func.
**	22-apr-05 (inkdo01)
**	    Added AD5081 to tidy up Unicode func errors because of bad 
**	    parm data types.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	05-sep-06 (gupsh01)
**	    Added E_AD5064_DATEANSI. 
**	14-sep-06 (gupsh01)
**	    Added E_AD5065_DATE_ALIAS_NOTSET. 
**      16-oct-2006 (stial01)
**          Added locator messages.
**	16-nov-2006 (gupsh01)
**	    Added E_AD5066_DATE_COERCION.
**	27-dec-06 (kibro01)
**	    Change incorrect '%s' to '%<n>c' in error messages
**      20-jul-07 (stial01)
**          Added locator errors.
**	10-Feb-2008 (kiria01) b120043
**		Added E_AD5090_DATE_MUL, E_AD5091_DATE_DIV, E_AD5092_DATE_DIV_BY_ZERO,
**		E_AD5093_DATE_DIV_ANSI_INTV and E_AD5094_DATE_ARITH_NOABS
**		in support of date arithmetic.
**	25-Jun-2008 (kiria01) SIR120473
**	    Added E_AD5512_BAD_DTID2_FOR_ESCAPE, E_AD5513_BAD_DTLEN2_FOR_ESCAPE,
**	    for changes to ADE_ESC_CHAR  and E_AD101C_BAD_ESC_SEQ,
**	    E_AD101D_BAD_ESC_SEQ and E_AD101E_ESC_SEQ_CONFLICT for
**	    further LIKE family errors.
**  22-Apr-2009 (Marty Bowes) SIR 121969
**      Add E_AD2055_CHECK_DIGIT_SCHEME, E_AD2056_CHECK_DIGIT_STRING
**      and E_AD2057_CHECK_DIGIT_EMPTY
**      and E_AD2058_CHECK_DIGIT_STRING_LENGTH
**      for the generation and validation of checksum values.
**	9-sep-2009 (stephenb)
**	    Add E_AD2107_LASTID_FCN_ERR
**      26-oct-2009 (joea)
**          Add E_AD106A_INV_STR_FOR_BOOL_CAST, E_AD106B_INV_INT_FOR_BOOL_CAST.
**	03-Mar-2010 (jonj)
**	    SIR 121619 MVCC, blob support:
**	    Add E_AD7016_ADP_MVCC_INCOMPAT
**	18-Mar-2010 (kiria01) b123438
**	    Added E_AD1028_NOT_ZEROONE_ROWS for SINGLETON aggregate
*/

#define _AD_CLASS 2


E_AD0000_OK:SS00000_SUCCESS
"ADF successful return status."
E_AD0001_EX_IGN_CONT:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 An ADF math exception has occurred with the math exception option set to\
 ignore."
E_AD0002_INCOMPLETE:SS5000B_INTERNAL_ERROR
"An operation on a peripheral datatype requires more space and/or data;\n\
please call again.  This message is expected and should not appear in a log\n\
file."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is an expected message.  However, if this message appears in a
**	log file, then something is wrong.
**
** System Status:
**	All is normal.
**
** Recommendation:
**	Call Ingres Corporation Technical Support if this message appears. 
**	It is an expected event internally; however, it should never appear 
**	externally.
*/
E_AD0101_EMBEDDED_CHAR_TRUNC:SS01004_STRING_RIGHT_TRUNC
"Truncation has occurred while converting a string type data base value into\
 a string type embedded value."
E_AD0102_NULL_IN_TEXT:SS01000_WARNING
"Warning: Null character(s) converted to blank(s) in text string."
E_AD0115_EX_WRN_CONT:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 An ADF math exception has occurred with the math exception option set to\
 warn."
E_AD0116_EX_UNKNOWN:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 The current exception is not one that ADF recognizes."
E_AD0120_INTDIV_WARN:SS01000_WARNING
""      /* Maps to user warning message in ERUSF.MSG */
E_AD0121_INTOVF_WARN:SS5000B_INTERNAL_ERROR
""      /* Maps to user warning message in ERUSF.MSG */
E_AD0122_FLTDIV_WARN:SS5000B_INTERNAL_ERROR
""      /* Maps to user warning message in ERUSF.MSG */
E_AD0123_FLTOVF_WARN:SS5000B_INTERNAL_ERROR
""      /* Maps to user warning message in ERUSF.MSG */
E_AD0124_FLTUND_WARN:SS5000B_INTERNAL_ERROR
""      /* Maps to user warning message in ERUSF.MSG */
E_AD0125_MNYDIV_WARN:SS5000B_INTERNAL_ERROR
""      /* Maps to user warning message in ERUSF.MSG */
E_AD0126_DECDIV_WARN:SS01000_WARNING
"Warning: %0d Packed Decimal divide by zero operation(s) detected."
/*%
** Parameters:
**	%0d - number of attempted divisions-by-zero detected
**
** Explanation:
**	The operations ADF was asked to perform resulted in a division where
**	the divisor was a packed decimal number with a zero value.  ADF was
**	set to generate warnings for all arithmetic exceptions, so no error
**	was generated.
**
** System Status:
**	This is a warning message.  The results of all attempted divisions
**	with a zero divisor were set to zero.
**
** Recommendation:
**	None required.  Alternative handling for arithmetic exceptions is
**	available and fully explained in the documentation set.
**
**	If these warnings were unexpected, they may be occurring because a
**	small value was truncated to zero while being coerced or converted
**	to a packed decimal number with insufficient scale to hold this small
**	number.  Using the decimal() conversion function to convert the
**	intermediate result to a value with more scale digits might help.
*/
E_AD0127_DECOVF_WARN:SS5000B_INTERNAL_ERROR
"Warning: %0d Packed Decimal overflow(s) detected."
/*%
** Parameters:
**	%0d - number of overflows detected
**
** Explanation:
**	The operations ADF was asked to perform resulted in a packed decimal
**	expression or sub-expression whose result exceeded the number of
**	integral digits available in the result value.  Packed decimal result
**	precision and scale are calculated according to the formulas shown
**	in the documentation.  If an intermediate result has more
**	significant integral (i.e., to the left of the decimal point) digits
**	than were available in the result, overflow will occur.  Another
**	possibility is that an attempt was made to coerce or convert a number
**	with more significant integral digits to a packed decimal number
**	which did not have space for these integral digits.  This could occur
**	when assigning a value to a column in a table, or when using the
**	decimal() conversion function.
**
**	An example of overflow is easy to construct: decimal(123.1, 4, 2) will
**	cause packed decimal overflow, because the number of significant
**	integral digits in the number being converted exceeds the number
**	allowed for in the result.  (123.1 has 3 significant non-zero digits,
**	the resulting packed decimal number will have space for 4-2=2 integral
**	digits, which is insufficient.)
**
**	ADF was set to generate warnings for all arithmetic exceptions, so no
**	error was generated.
**
** System Status:
**	This is a warning message.  The results of all overflowed expressions
**	were set to zero.
**
** Recommendation:
**	None required.  Alternative handling for arithmetic exceptions is
**	available and fully explained in the documentation set.
**
**	If these warnings were unexpected, try simplifying the expression which
**	is generating this overflow until you can reproduce the problem with as
**	few variables, column names, arithmetic operators, etc. as possible.
**	Then, using the formulas given in the documentation, calculate
**	the result size for your simplified expression.  It should become clear
**	why the expression is overflowing.  Once you determine why the overflow
**	is occurring, you may want to use the decimal() conversion function to
**	change the precision and scale of your intermediate result, or if the
**	result is very large, you may want to convert to a floating point
**	datatype using the float4() or float8() conversion function.
*/
E_AD0500_ABS_DATE_IN_AG:SS5000B_INTERNAL_ERROR
""      /* Maps to user warning message in ERUSF.MSG */
E_AD0FFF_NOT_IMPLEMENTED_YET:SS5000B_INTERNAL_ERROR
"ADF function has not been implemented yet."
E_AD1001_BAD_DATE:SS22008_DATETIME_FLD_OVFLOW
"ADF date format, %0d, is invalid."
E_AD1002_BAD_MONY_SIGN:SS50004_SYS_CONFIG_ERROR
"ADF money sign, %0c, is invalid."
E_AD1003_BAD_MONY_LORT:SS50004_SYS_CONFIG_ERROR
"ADF money sign position designator, %0d, is invalid."
E_AD1004_BAD_MONY_PREC:SS50004_SYS_CONFIG_ERROR
"ADF money precision specification, %0d, is invalid."
E_AD1005_BAD_DECIMAL:SS50004_SYS_CONFIG_ERROR
"Bad decimal character specification, '%0x'."
E_AD1006_BAD_OUTARG_VAL:SS50004_SYS_CONFIG_ERROR
"Bad output argument specification (i.e. one of your OUTARG members is bogus)."
E_AD1007_BAD_QLANG:SS0A500_INVALID_QRY_LANG
"Unknown query language ... query language ID = %0d."
E_AD1008_BAD_MATHEX_OPT:SS5000B_INTERNAL_ERROR
"Unknown math exception option, %0d."
E_AD1009_BAD_SRVCB:SS5000B_INTERNAL_ERROR
"ADF Server control block passed into adg_init() or adg_add_fexi() is invalid."
E_AD100A_BAD_NULLSTR:SS50004_SYS_CONFIG_ERROR
"The `NULLstring' passed into adg_init() is invalid."
E_AD100B_BAD_MAXSTRING:SS5000B_INTERNAL_ERROR
"The max size string given to ADF at session init time (%0d) is invalid.\
  It must be between >= 1 and <= %1d."
E_AD1010_BAD_EMBEDDED_TYPE:SS22500_INVALID_DATA_TYPE
"ADF routine was passed an invalid embedded type id."
E_AD1011_BAD_EMBEDDED_LEN:SS5000B_INTERNAL_ERROR
"ADF routine found DB_EMBEDDED_DATA with an invalid length."
E_AD1012_NULL_TO_NONNULL:SS5000A_QUERY_ERROR
"An attempt to place a null value in a non-nullable datatype."
E_AD1013_EMBEDDED_NUMOVF:SS22003_NUM_VAL_OUT_OF_RANGE
"Numeric overflow while converting a data base value into an embedded value."
E_AD1014_BAD_VALUE_FOR_DT:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 Invalid data for datatype found while doing value checking."
E_AD1015_BAD_RANGE:SS42000_SYN_OR_ACCESS_ERR
"Illegal pattern match specified.  One of the following:\n\
    o  An opening `[' without a closing `]'\n\
    o  Vice-versa\n\
    o  Range ending with `-]'\n\
    o  Range of the form `[x-y]' where char `y' comes before `x'."
E_AD1016_PMCHARS_IN_RANGE:SS42000_SYN_OR_ACCESS_ERR
"Illegal pattern match specified:  You cannot have pattern match\
 characters included in a `[ ]' range."
E_AD1017_ESC_AT_ENDSTR:SS22025_INV_ESCAPE_SEQ
"Illegal pattern match specified:  ESCAPE character at end of string."
E_AD1018_BAD_ESC_SEQ:SS22025_INV_ESCAPE_SEQ
"Illegal pattern match specified:  Illegal ESCAPE sequence.\n\
 The ESCAPE char must be followed by one of:\n\
    `\%' (percent)\n\
    `_' (underscore)\n\
    `[' (left square bracket)\n\
    `]' (right square bracket)\n\
    another ESCAPE char."
E_AD1019_NULL_FEXI_PTR:SS5000B_INTERNAL_ERROR
"The adg_add_fexi() routine was called with a NULL pointer given as the address
 of the callback routine.  This is an internal error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	There was an error in the adg_add_fexi() routine.  This means that
**	ADF was unable to successfully obtain the proper callback address for
**	certain external functions it requires.  This is an internal error.
**
** System Status:
**	This usually occurs during startup.  This error will cause startup
**	failure.
**
** Recommendation:
**	Call Ingres Corporation Technical Support with this information.
*/
E_AD101A_DT_NOT_SUPPORTED:SS42000_SYN_OR_ACCESS_ERR
"Datatype of the selected column cannot be supported by your current \
 configurations of Front Ends and %0c Server."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The datatype of the selected column cannot be supported by the
**	FE/GCA/DBMS configurations.  For example, if a 6.4 frontend
**	selects a decimal column from a 6.5 server, this error will be
**	returned. 
**
** System Status:
**	The query will abort.
**
** Recommendation:
**	Fix the query or change the datatype of the selected column to
**	the one that can be supported by the FE/GCA/DBMS configuration.
*/
E_AD101B_BAD_FLT_VALUE:SS5000B_INTERNAL_ERROR
"A bad floating point value has caused an exception in ADF."
E_AD101C_BAD_ESC_SEQ:SS22025_INV_ESCAPE_SEQ
"Illegal pattern match specified:  Illegal ESCAPE sequence.\n\
 The ESCAPE char must be followed by one of:\n\
    `\|' (percent)\n\
    another ESCAPE char."
E_AD101D_BAD_ESC_SEQ:SS22025_INV_ESCAPE_SEQ
"Illegal pattern match specified:  Illegal ESCAPE sequence.\n\
 The ESCAPE char must be followed by one of:\n\
    `[' (left bracket)\n\
    `]' (right bracket)\n\
    `(' (left paren)\n\
    `)' (right paren)\n\
    `\|' (vertical bar)\n\
    `\^' (circumflex)\n\
    `\-' (minus sign)\n\
    `\+' (plus sign)\n\
    `\*' (asterisk)\n\
    `_' (underscore)\n\
    `\%' (percent)\n\
    `\?' (question mark)\n\
    `\{' (left brace)\n\
    another ESCAPE char."
E_AD101E_ESC_SEQ_CONFLICT:SS22025_INV_ESCAPE_SEQ
"Illegal pattern match specified: Escape character conflict.\n\
 Named character classes cannot be used if the ESCAPE\n\
 char is set to ':' (colon)."
E_AD101F_REG_EXPR_SYN:SS22025_INV_ESCAPE_SEQ
"Illegal pattern match specified: Regular expression syntax.\n\
 A syntax error has been found in the regular expression.\n\
 A character set expression must be enclosed with a pair of\n\
 '[', ']' (square brackets) and may contain a list of named\n\
 character classes, individual characters or inclusive ranges\n\
 specified with a '-' (minus sign). Character class names are\n\
 themselves enclosed with '[:' and ':]'. Supported classes are:\n\
 ALPHA, UPPER, LOWER, DIGIT, ALNUM, SPACE, and WHITESPACE.\n\
 The pattern operators '?', '*', '+' and repeat counts affect\n\
 the preceding pattern element, a literal character, a '_'\n\
 (underscore), a bracketed character set or a sequence of\n\
 these collected into one unit with the '(', ')' (parens).\n\
    '?' means 'match zero or one of'\n\
    '*' means 'match zero or more of'\n\
    '+' means 'match one or one of'\n\
    '{n}' means 'match n of'\n\
    '{n,m}' means 'match between n and m of'\n\
    '{,n}' means 'match n or more'.\n\
 Sub-patterns collected with '(', ')' may only be nested upto\n\
 10 levels deep."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Uninitialised floats passed from ESQL applications may cause
**	floating point exceptions in adt_compute_change(). This error
**	is used to report these exceptions.
**
** System Status:
**	The query will abort.
**
** Recommendation:
**	Fix the query to ensure a valid float value is used.
*/
E_AD1020_BAD_ERROR_LOOKUP:SS5000B_INTERNAL_ERROR
"ADF was unable to lookup error code: %0x"
E_AD1021_BAD_ERROR_PARAMS:SS5000B_INTERNAL_ERROR
"adu_error() was called with a bad argument list."
E_AD1022_BAD_USER_LOOKUP:SS5000B_INTERNAL_ERROR
"ADF was unable to lookup the USER error code, %0d"
E_AD1025_BAD_ERROR_NUM:SS5000B_INTERNAL_ERROR
"adu_error() was called with an unknown error code, %0x"
E_AD1026_PAT_TOO_CPLX:SS5000B_INTERNAL_ERROR
"Pattern specified is too complex. Simplify the pattern."
E_AD1027_PAT_LEN_LIMIT:SS5000B_INTERNAL_ERROR
"Pattern specified is too long. Shorten the pattern."
E_AD1028_NOT_ZEROONE_ROWS:SS21000_CARD_VIOLATION
"Subselect returned more that one row."
E_AD1030_F_COPY_STR_TOOSHORT:SS50004_SYS_CONFIG_ERROR
"The floating point number being copied will not fit in the field specified. \
 Either give a longer field, or use a different setting of the -f flag on the\
 command line."
E_AD1040_CV_DVBUF_TOOSHORT:SS5000B_INTERNAL_ERROR
"ADF routine, adc_tolocal(), was passed a data value buffer that was too small\
 to do the conversion from the network standard form."
E_AD1041_CV_NETBUF_TOOSHORT:SS5000B_INTERNAL_ERROR
"ADF routine, adc_tonet(), was passed a buffer that was too small\
 to do the conversion to the network standard form."
E_AD1050_NULL_HISTOGRAM:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 Attempt to create a histogram element for a NULL value."
E_AD1061_BAD_SECURITY_LABEL:SS50004_SYS_CONFIG_ERROR
"Bad security label specification.  The string you specified for a security
label could not be translated by the secure operating system.  Check the
security documentation for your operating system to determine the correct
external format for a security label."
E_AD1062_SECURITY_LABEL_LUB:SS50004_SYS_CONFIG_ERROR
"Security Label LUB error. An error was detected while trying to generate the
Lowest Upper Bound of two security labels. Check the security documentation
for your operating system to determine the allowed LUB operations."
/*%
** Parameters:
**	None.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Check the specification of the two security labels used in the query.
*/
E_AD1063_SECURITY_LABEL_EXTERN:SS50004_SYS_CONFIG_ERROR
"An ADF error occurred while attempting to convert a security label to external
format. This could be caused by the operating system no longer recognizing
the internal security label, or by a corrupted internal security label."
/*%
** Parameters:
**	None.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Check the specification of the security label being converted.
*/
E_AD1064_SECURITY_LABEL_EXTLEN:SS50004_SYS_CONFIG_ERROR
"An ADF error occurred while attempting to calculate the length of a
security label external value. This could be caused by the operating 
system no longer recognizing the internal security label, or by a corrupted 
internal security label."
/*%
** Parameters:
**	None.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Check the specification of the security label being converted.
*/
E_AD1065_SESSION_LABEL:SS50004_SYS_CONFIG_ERROR
"An ADF error occurred while attempting to determine the current security label
for the session."
/*%
** Parameters:
**	None.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Check the session security label.
*/
E_AD1066_SOP_NOT_AVAILABLE:SS50004_SYS_CONFIG_ERROR
"An attempt was made to use the extended Security Operation interface in a
context where this interface is not available."
/*%
** Parameters:
**	None.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Ingres Corporation Technical Support.
*/
E_AD1067_SOP_FIND:SS50004_SYS_CONFIG_ERROR
"An error was returned while using the extended Security Operations interface
to Find a security label/id. Check the %0c error log (errlog.log) for more 
information why this condition occurred."
/*%
** Parameters:
**	None.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Ingres Corporation Technical Support.
*/
E_AD1068_SOP_COMPARE:SS50004_SYS_CONFIG_ERROR
"An error was returned while using the extended Security Operations interface
to Compare two security labels/ids. Check the %0c error log (errlog.log) 
for more information why this condition occurred."
/*%
** Parameters:
**	None.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Ingres Corporation Technical Support.
*/
E_AD1069_SOP_COLLATE:SS50004_SYS_CONFIG_ERROR
"An error was returned while using the extended Security Operations interface
to Collate two security labels/ids. Check the %0c error log (errlog.log) 
for more information why this condition occurred."
/*%
** Parameters:
**	None.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. If the error persists, contact
**	Ingres Corporation Technical Support.
*/
E_AD106A_INV_STR_FOR_BOOL_CAST:SS22018_INV_VAL_FOR_CAST
"'%0c' is not a valid character value for casting to BOOLEAN."
/*%
** Parameters:
**	%0c - character string
**
** Explanation:
**	The system is attempting to convert a character string to a boolean
**	datatype.  A value other than 'FALSE' or 'TRUE' has been found.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Fix the character string to be only 'FALSE', 'TRUE' or a
**	case-insensitive variant.
*/
E_AD106B_INV_INT_FOR_BOOL_CAST:SS22018_INV_VAL_FOR_CAST
"'%0d' is not a valid integer value for casting to BOOLEAN."
/*%
** Parameters:
**	%0d - integer value
**
** Explanation:
**	The system is attempting to convert an integer to a boolean
**	datatype.  A value other than 0 or 1 has been found.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Fix the integer value to be 0 or 1.
*/
E_AD1070_BIT_TO_STR_OVFL:SS22003_NUM_VAL_OUT_OF_RANGE
"String Overflow.  The length of the bit string exceeds the length of the
character string provided."
/*%
** Parameters:
**	None.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Check the specification of the buffers or fields used in the query.
**	For each valid bit in the bit string being converted, there must
**	be one character position.
*/
E_AD1071_NOT_BIT:SS22003_NUM_VAL_OUT_OF_RANGE
"'%0c' is not a valid bit string.
Only '0', '1', or spaces are legal."
/*%
** Parameters:
**	%0c - bit string
**
** Explanation:
**	The system is attempting to convert a character string to a bit
**	datatype.  An invalid character has been found.
**
** System Status:
**	The query is aborted.
**
** Recommendation:
**	Fix the character string to include only the characters 0, 1 , or
**	whitespace (spaces, tabs, etc.).
*/
E_AD1072_STR_TO_BIT_OVFL:SS22003_NUM_VAL_OUT_OF_RANGE
"String to long for bit string."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to convert a character string into a bit-string
**	type.  The conversion would require more bits in the bit string than
**	there is room for.
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	Check the value of the character string and the specification of the
**	bit string.
*/
E_AD1073_SOP_MAC_READ:SS50004_SYS_CONFIG_ERROR
"An error was returned while using the extended Security Operations interface
to perform MAC arbitration on a security label/id."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The user is not allowed to execute queries that need compare
**	actions on security labels in a non-B1 secure environment.
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	The user should not include security label columns in their
**	queries on non-B1 secure machines.
*/
E_AD1074_SOP_NOT_ALLOWED:SS50004_SYS_CONFIG_ERROR
"Compare operations on security labels are ONLY allowed in a B1 secure
environment."
/*%
** Parameters:
**	None.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Retry the operation. 
*/
E_AD1080_STR_TO_DECIMAL:SS5000B_INTERNAL_ERROR
"String is not coercible to a decimal datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to convert an illegal decimal string to
**	a decimal datatype.
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	Contact your system manager.
*/
E_AD1081_STR_TO_FLOAT:SS5000B_INTERNAL_ERROR
"String is not coercible to a float datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to convert an illegal DEFAULT value.  This
**	error is being mapped to a PSF error, so the user should not be
**	seeing this error code.
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	Change the DEFAULT to a valid value.
*/
E_AD1082_STR_TRUNCATE:SS22001_STRING_RIGHT_TRUNC
"String '%0c' is too long to fit into the resulting string type."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to convert a character string that is
**	too long to fit into the output string.  
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	Check the value of the character string and the specification of the
**	output string.
*/
E_AD1083_UNTERM_C_STR:SS22024_UNTERM_C_STRING
"Unable to convert program data. \n\
A host variable contains an unterminated C string."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to convert a character string that is
**	too long to fit into the output string.  
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	Check the value of the character string and the specification of the
**	output string.
*/
E_AD1090_BAD_DTDEF:SS5000B_INTERNAL_ERROR
"Invalid DEFAULT value specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is attempting to convert an illegal decimal string to
**	a decimal datatype.
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	Contact your system manager.
*/
E_AD1095_BAD_INT_SECLABEL:SS5000B_INTERNAL_ERROR
"An invalid internal security label was detected. The secure operating
system returned an error indicating a security label in internal format
was invalid or not known to the secure operating system. This could be
caused by a corrupted security label, or by the secure operating system
being changed such that the label is no longer known."
/*%
** Parameters:
**	None.
**
** Explanation:
** 	The secure operating system rejected an existing internal security
**	label as invalid. This could be caused by either a corrupted 
**	security label, or by the secure operating system being changed 
**	such that the label is no longer known; for example, the OS label 
**	mapping file (if applicable) may have been changed in an 
**	incompatible way.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Correct the label/and or secure operating system configuration.
*/
E_AD1096_SESSION_PRIV:SS5000B_INTERNAL_ERROR
"Error occurred while determining the session privileges within ADF"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An error occurred while attempting to get the session privilege mask
**	information within ADF.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	If this condition persists, contact Ingres Corporation Technical 
**	Support.
*/
E_AD1097_BAD_DT_FOR_SESSPRIV:SS5000A_QUERY_ERROR
"Illegal datatype specified when requesting session privileges"
/*%
** Parameters:
**	None.
**
** Explanation:
**	An illegal datatype was specified when requesting session privileges.
**	The session privilege name must be specified as a string.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Specify the user privilege as a string.
*/
E_AD1098_BAD_LEN_SEC_INTRNL:SS5000A_QUERY_ERROR
"An illegal data length was specified for an internal security label value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	An illegal data length was specified when accessing a security
**	label in internal format.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Specify an appropriate length for the security label value.
*/
E_AD1120_INTDIV_ERROR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD1121_INTOVF_ERROR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD1122_FLTDIV_ERROR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD1123_FLTOVF_ERROR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD1124_FLTUND_ERROR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD1125_MNYDIV_ERROR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD1126_DECDIV_ERROR:SS22012_DIVISION_BY_ZERO
"Packed Decimal divide by zero detected in query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operation ADF was asked to perform resulted in a division where
**	the divisor was a packed decimal number with a zero value.  ADF was
**	set to generate errors for all arithmetic exceptions, so the query
**	was immediately aborted.
**
** System Status:
**	The query was aborted.
**
** Recommendation:
**	If this error was unexpected, it may be occurring because a
**	small value was truncated to zero while being coerced or converted
**	to a packed decimal number with insufficient scale to hold this small
**	number.  Using the decimal() conversion function to convert the
**	intermediate result to a value with more scale digits might help.
**
**	Alternative handling for arithmetic exceptions is available and fully
**	explained in the documentation set.
*/
E_AD1127_DECOVF_ERROR:SS22003_NUM_VAL_OUT_OF_RANGE
"Packed Decimal overflow detected in query."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The operation ADF was asked to perform resulted in a packed decimal
**	expression or sub-expression whose result exceeded the number of
**	integral digits available in the result value.  Packed decimal result
**	precision and scale are calculated according to the formulas shown
**	in the documentation.  If an intermediate result has more
**	significant integral (i.e., to the left of the decimal point) digits
**	than were available in the result, overflow will occur.  Another
**	possibility is that an attempt was made to coerce or convert a number
**	with more significant integral digits to a packed decimal number
**	which did not have space for these integral digits.  This could occur
**	when assigning a value to a column in a table, or when using the
**	decimal() conversion function.
**
**	An example of overflow is easy to construct: decimal(123.1, 4, 2) will
**	cause packed decimal overflow because the number of significant
**	integral digits in the number being converted exceeds the number
**	allowed for in the result.  (123.1 has 3 significant non-zero digits,
**	the resulting packed decimal number will have space for 4-2=2 integral
**	digits, which is insufficient.)
**
**	ADF was set to generate errors for all arithmetic exceptions, so this
**	query will be aborted.
**
** System Status:
**	The query was aborted.
**
** Recommendation:
**	If this error was unexpected, try simplifying the expression which
**	is generating this overflow until you can reproduce the problem with as
**	few variables, column names, arithmetic operators, etc. as possible.
**	Then using the formulas given in the documentation, calculate
**	the result size for your simplified expression.  It should become clear
**	why the expression is overflowing.  Once you determine why the overflow
**	is occurring, you may want to use the decimal() conversion function to
**	change the precision and scale of your intermediate result, or if the
**	result is very large, you may want to convert to a floating point
**	datatype using the float4() or float8() conversion function.
**
**	Alternative handling for arithmetic exceptions is available and fully
**	explained in the documentation set.
*/
/*
**  *************************************************************************
**  *                                                                       *
**  *   E_AD1200 - E_AD12FF are reserved for the ESL work.  Do not use      *
**  *   these numbers.                                                      *
**  *                                                                       *
**  *************************************************************************
*/
E_AD2001_BAD_OPNAME:SS5000B_INTERNAL_ERROR
"ADF routine was given an unknown operator."
E_AD2002_BAD_OPID:SS5000B_INTERNAL_ERROR
"ADF routine was passed an invalid operator id."
E_AD2003_BAD_DTNAME:SS22500_INVALID_DATA_TYPE
"ADF routine was passed an unknown datatype name."
E_AD2004_BAD_DTID:SS22500_INVALID_DATA_TYPE
"ADF routine was passed an invalid datatype id."
E_AD2005_BAD_DTLEN:SS5000B_INTERNAL_ERROR
"ADF routine found DB_DATA_VALUE with an invalid length."
E_AD2006_BAD_DTUSRLEN:SS22500_INVALID_DATA_TYPE
"ADF routine encountered user-declared datatype length to be wrong."
E_AD2007_DT_IS_FIXLEN:SS22500_INVALID_DATA_TYPE
"Datatype is fixed length, yet a length specification was given."
E_AD2008_DT_IS_VARLEN:SS22500_INVALID_DATA_TYPE
"Datatype is variable length, but no length specification was given."
E_AD2009_NOCOERCION:SS5000B_INTERNAL_ERROR
"No coercion is available for datatypes given."
E_AD200A_NOCOPYCOERCION:SS5000B_INTERNAL_ERROR
"No copy-coercion is available for datatypes given."
E_AD200B_BAD_PREC:SS50002_LIMIT_EXCEEDED
"Invalid precision specified for DECIMAL value ... 1 <= precision <= 39."
E_AD200C_BAD_SCALE:SS50002_LIMIT_EXCEEDED
"Invalid scale specified for DECIMAL value ... 0 <= scale <= precision."
E_AD200D_BAD_BASE_DTID:SS22500_INVALID_DATA_TYPE
"Illegal or unknown base datatype ID."
E_AD2010_BAD_FIID:SS5000B_INTERNAL_ERROR
"ADF found a bad function instance id."
E_AD2020_BAD_LENSPEC:SS5000B_INTERNAL_ERROR
"ADF does not recognize the length specification code %0d."
E_AD2021_BAD_DT_FOR_PRINT:SS5000B_INTERNAL_ERROR
"Illegal datatype for the PRINT style lenspecs."
E_AD2022_UNKNOWN_LEN:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 The length of the result can not be determined because one (or more) of the\
 input lengths that are required to determine the result length is\
 ADE_LEN_UNKNOWN."
E_AD2030_LIKE_ONLY_FOR_SQL:SS5000B_INTERNAL_ERROR
"The LIKE/NOTLIKE operators are only valid in SQL."
E_AD2040_INCONSISTENT_TPL_CNT:SS5000B_INTERNAL_ERROR
"The number of T/P/L's for base datatype ID `%0d' listed in the datatypes\
 table differs from the number generated by that datatype's routine."
E_AD2041_TPL_ARRAY_TOO_SMALL:SS5000B_INTERNAL_ERROR
"The T/P/L array passed in was not big enough to hold the number of T/P/L's\
 that needed to be generated."
E_AD2042_MEMALLOC_FAIL:SS5000B_INTERNAL_ERROR
"The memory allocation routine supplied by GCA failed with a status of %0x."
E_AD2043_ATOMIC_TPL:SS5000B_INTERNAL_ERROR
"The T/P/L triad being decomposed is already an `ATOMIC' element.  No further\
 decomposition is possible."
E_AD2050_NO_COMVEC_FUNC:SS5000B_INTERNAL_ERROR
"Missing function pointer in the datatypes table for function `%0c()',\
 datatype ID `%1d'."
E_AD2055_CHECK_DIGIT_SCHEME:SS5000B_INTERNAL_ERROR
"Unknown check digit scheme '%0c'."
E_AD2056_CHECK_DIGIT_STRING:SS5000B_INTERNAL_ERROR
"Unexpected character '%0c' in input string."
E_AD2057_CHECK_DIGIT_EMPTY:SS5000B_INTERNAL_ERROR
"Cannot %0c check digit on an empty string."
E_AD2058_CHECK_DIGIT_STRING_LENGTH:SS5000B_INTERNAL_ERROR
"Cannot generate check digit on a string of length '%0d'. \
Correct length is '%1d'"
E_AD2060_BAD_DT_COUNT:SS5000B_INTERNAL_ERROR
"Some ADF function was told that it was receiving some number of datatypes\
 other than 0, 1, or 2."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Currently, functions are limited to 0, 1, or 2, parameters.
**	This error indicates that some facility called ADF with a number of
**	input datatypes that was none of these.
**
** System Status:
**	This is an internal error.  The current query will most likely be
**	aborted.
**
** Recommendation:
**	None.
*/
E_AD2061_BAD_OP_COUNT:SS5000B_INTERNAL_ERROR
"Although this operation does exist and does have function instances associated\
 with it, none of these function instances had the number of input datatypes\
 specified."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Some facility called ADF for datatype resolution, but none of ADF's
**	function instance entries matched the requested operation ID or had the
**	specified number of input datatypes.
**
** System Status:
**	The current query will most likely be aborted.
**
** Recommendation:
**	Ensure that the function you are using accepts the number of inputs you
**	are attempting to pass to it.
*/
E_AD2062_NO_FUNC_FOUND:SS5000B_INTERNAL_ERROR
"No function instances exist which can be used for the operation id and\
 datatypes given to this ADF routine.  Make certain you have used datatypes\
 which are correct for this operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This operation is known to ADF, but the datatype(s) given to ADF is not
**	coercible to the type required to use any of the entries in ADF's
**	function instance table.
**
** System Status:
**	The current query will most likely be aborted.
**
** Recommendation:
**	Refer to the documentation for information on which datatypes
**	can be used for this operation.
*/
E_AD2063_FUNC_AMBIGUOUS:SS5000B_INTERNAL_ERROR
"Two or more function instances required the same number of coercions and had\
 the same composite rankings when attempting to resolve an operation in the\
 adi_resolve routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	ADF's datatype resolution rules have been unable to decide on a function
**	instance from its internal tables.  After using several tie-breaking
**	strategies, several equally suitable candidate function instances
**	remained.
**
** System Status:
**	The current query will most likely be aborted.
**
** Recommendation:
**	Explicitly convert your parameters to the input types desired.
*/
E_AD2070_NO_EMBED_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"The column specification given contains an embedded number in the datatype's\
 name.  This is not allowed for this datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A column specification with an embedded number was detected where it
**	is not allowed.  For example, specifying "char10" will generate this
**	error, because you must use parentheses when specifying the length for
**	the "char" datatype.  Specifying a length for the "date" datatype
**	(e.g., "date(10)") will also generate this error, because you are not
**	allowed to specify a length for the "date" datatype.
**
** System Status:
**	The current action will most likely be aborted.
**
** Recommendation:
**	Use the correct syntax when specifying columns with this datatype.
*/
E_AD2071_NO_PAREN_ALLOWED:SS42000_SYN_OR_ACCESS_ERR
"The column specification given had parenthesized numbers after the datatype's\
 name.  This is not allowed for this datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A column specification with parenthesized numbers following the
**	datatype name was detected where it is not allowed.  For example,
**	specifying "c(10)" will generate this error because you must not use
**	parentheses when specifying the length for the "c" datatype. Specifying
**	a length for the "date" datatype (e.g., "date(10)") will also generate
**	this error because you are not allowed to specify a length for the
**	"date" datatype.
**
** System Status:
**	The current action will most likely be aborted.
**
** Recommendation:
**	Use the correct syntax when specifying columns with this datatype.
*/
E_AD2072_NO_PAREN_AND_EMBED:SS42000_SYN_OR_ACCESS_ERR
"The column specification given contains both an embedded number in the\
 datatype's name and parenthesized numbers following it.  This is never\
 allowed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A column specification with an embedded number and parenthesized
**	numbers was given.  For example, specifying "char10(10)" will
**	generate this error.  You are allowed to specify the length for
**	a datatype in only one way.
**
** System Status:
**	The current action will most likely be aborted.
**
** Recommendation:
**	Use the correct syntax when specifying columns with this datatype.
*/
E_AD2073_BAD_USER_PREC:SS50002_LIMIT_EXCEEDED
"Precision %0d is out of range.  Please specify a precision in the range\
 1 <= precision <= 31."
/*%
** Parameters:
**	%0d - precision specified for this column
**
** Explanation:
**	The precision specified for a column with the "decimal" datatype is
**	invalid.  For example, attempting to create a table with a column
**	specified as "decimal(99)" will generate this error, since the
**	maximum precision supported for the "decimal" datatype is 31.
**
** System Status:
**	The current action will most likely be aborted.
**
** Recommendation:
**	Specify a precision in the proper range.
*/
E_AD2074_BAD_USER_SCALE:SS50002_LIMIT_EXCEEDED
"Scale %0d is invalid.  You must specify a scale which is 0 or larger AND less\
 than or equal to the precision you have specified.  For this column you have\
 specified a precision of %1d, so your scale must fall into the range\
 0 <= scale <= %1d."
/*%
** Parameters:
**	%0d - scale specified for this column
**	%1d - precision specified for this column
**
** Explanation:
**	The scale specified for a column with the "decimal" datatype is
**	invalid.  For example, attempting to create a table with a column
**	specified as "decimal(10,11)" will generate this error, since the scale
**	(the second number in the parentheses) must not be larger than the
**	precision (the first number in the parentheses).  The scale must always
**	be in the range 0 <= scale <= precision.
**
** System Status:
**	The current action will most likely be aborted.
**
** Recommendation:
**	Specify a scale in the proper range.
*/
E_AD2080_IFTRUE_NEEDS_INT:SS5000B_INTERNAL_ERROR
"The datatype of the ii_iftrue() function's first argument must be INTEGER."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error cannot be incurred directly as a result of a user's query
**	using the ii_iftrue() function; use of a non-integral first parameter
**	by the user would have been caught by the datatype resolution algorithm.
**
**	Instead, this is an internal problem probably caused by OPF, most
**	likely as a result of an outer join in some query.
**
** System Status:
**	The current query will be aborted.
**
** Recommendation:
**	Contact your system manager.
*/
E_AD2085_LOCATE_NEEDS_STR:SS22011_SUBSTRING_ERROR
"The locate() function does not accept non-string parameters.  Explicitly\
 convert parameters with non-string types to parameters with string types."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The locate() function takes two arguments, both of which must have
**	a string datatype (a string datatype is one of: C, TEXT, CHAR, or
**	VARCHAR).  An invalid datatype was given.
**
** System Status:
**	The current query will be aborted.
**
** Recommendation:
**	Explicitly convert any non-string datatypes to string datatypes.  For
**	example, if you are trying to find the first occurrence of "x" in the
**	string "32123", where "x" is an integer column, you cannot use
**	"locate('32123', x)", because the second parameter is an non-string
**	type.  Instead, use: locate('32123', varchar(x)).
*/
E_AD2090_BAD_DT_FOR_STRFUNC:SS22011_SUBSTRING_ERROR
"Illegal datatype for the char(), varchar(), ascii() or text() function."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error can be incurred when a string function (i.e., c(),
**	text(), varchar(), char()) is invoked with a datatype that is
**	not supported by these string functions (such as UDT).
**
** System Status:
**	The current query will be aborted.
**
** Recommendation:
**	Contact your system manager.
*/
E_AD2092_BAD_START_FOR_SUBSTR:SS22011_SUBSTRING_ERROR
"The starting point for substring() function is past the character string."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This error is incurred when the string function is
**	invoked with a starting point that is past the length of
**	the character expression.
**
** System Status:
**	The current query will be aborted.
**
** Recommendation:
**	Fix the query and retry.
*/
E_AD2093_BAD_LEN_FOR_SUBSTR:SS22011_SUBSTRING_ERROR
"Bad length value may be specified for the substring function, sum of\
 length and start position specified, is less than the start value."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is incurred when the substring function is
**      invoked with a length which implies that the sum of "FOR" length
**	and the start position is less than the start value. 
**
** System Status:
**      The current query will be aborted.
**
** Recommendation:
**      Fix the query and retry.
*/
E_AD2094_INVALID_LOCATOR:SS5000B_INTERNAL_ERROR
"Invalid locator specified."
E_AD20A0_BAD_LEN_FOR_PAD:SS22011_SUBSTRING_ERROR
"Bad length value was specified for the lpad/rpad function, less\
 than 0 or greater than result length."
/*%
** Parameters:
**      None.
**
** Explanation:
**      This error is incurred when the lpad or rpad functions are
**      invoked with a length which is either less than 0 or grerater
**	than the result length.
**
** System Status:
**      The current query will be aborted.
**
** Recommendation:
**      Fix the query and retry.
*/
E_AD20B0_REPLACE_NEEDS_STR:SS22011_SUBSTRING_ERROR
"The replace() function does not accept non-string parameters.  Explicitly\
 convert parameters with non-string types to parameters with string types."
/*%
** Parameters:
**      None.
**
** Explanation:
**      The replace() function takes three arguments, all of which must have
**      a string datatype (a string datatype is one of: C, TEXT, CHAR, or
**      VARCHAR).  An invalid datatype was given.
**
** System Status:
**      The current query will be aborted.
**
** Recommendation:
**      Explicitly convert any non-string datatypes to string datatypes.
*/
E_AD2100_NULL_RESTAB_PTR:SS5000B_INTERNAL_ERROR
"The callback function for the resolve_table function has not been set prior
 to its use: the callback address is NULL.  This is an internal error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon trying to execute the resolve_table() function, ADF discovered
**	that the address to the callback routine for this function was NULL.
**	This usually means that adg_add_fexi() was never called to set the
**	address prior to the invocation of resolve_table().  This is an 
**	internal error.
**
** System Status:
**	The query involving the resolve_table() function will be aborted.
**
** Recommendation:
**	Call Ingres Corporation Technical Support with this information.
*/
E_AD2101_RESTAB_FCN_ERROR:SS5000B_INTERNAL_ERROR
"The callback function for the resolve_table function returned an error.  This
 should never happen since resolve_table always has some value defined for all
 inputs.  This is an internal error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon executing the callback function for resolve_table(), ADF received
**	a status indicating that an error occurred.  This is abnormal and is
**	considered an internal error.
**
** System Status:
**	The query involving the resolve_table() function will be aborted.
**
** Recommendation:
**	Call Ingres Corporation Technical Support with this information.
*/
E_AD2102_NULL_ALLOCATED_PTR:SS5000B_INTERNAL_ERROR
"The callback function for the iitotal_allocated_pages function has not been 
set prior to its use: the callback address is NULL.  This is an 
internal error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon trying to execute the iitotal_allocated_pages() function, ADF 
**	discovered that the address to the callback routine for this function 
**	was NULL.
**
**	This usually means that adg_add_fexi() was never called to set the
**	address prior to the invocation of iitotal_allocated_pages(). This 
**	is an internal error.
**
**	This function is normally called when selecting information from the
**	standard catalog, iitables.
**
** System Status:
**	The query involving the iitotal_allocated_pages() function will be 
**	aborted. 
**
** Recommendation:
**	Call Ingres Corporation Technical Support with this information.
*/
E_AD2103_ALLOCATED_FCN_ERR:SS5000B_INTERNAL_ERROR
"The callback function for the iitotal_allocated_pages function returned an 
error check the DBMS error log for more information"
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon executing the callback function for iitotal_allocated_pages(),
**	ADF received a status indicating that an error occurred.  Check the
**	error log for more information.
**
**	This function is normally called when selecting information from the
**	standard catalog, iitables.
**
** System Status:
**	The query involving the iitotal_allocated_pages() function will be 
**	aborted.
**
** Recommendation:
**	Call Ingres Corporation Technical Support with this information.
*/
E_AD2104_NULL_OVERFLOW_PTR:SS5000B_INTERNAL_ERROR
"The callback function for the iitotal_overflow_pages function has not been 
set prior to its use: the callback address is NULL.  This is an 
internal error."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon trying to execute the iitotal_overflow_pages function(), ADF 
**	discovered that the address to the callback routine for this function 
**	was NULL.
**
**	This usually means that adg_add_fexi() was never called to set the
**	address prior to the invocation of iitotal_overflow_pages(). This is 
**	an internal error.
**
**	This function is normally called when selecting information from the
**	standard catalog, iitables.
**
** System Status:
**	The query involving the iitotal_overflow_pages() function will be 
**	aborted.
**
** Recommendation:
**	Call Ingres Corporation Technical Support with this information.
*/
E_AD2105_OVERFLOW_FCN_ERR:SS5000B_INTERNAL_ERROR
"The callback function for the iitotal_overflow_pages function returned an 
error check the DBMS error log for more information"
E_AD2106_DMF_ADF_FEXI_ERR:SS5000B_INTERNAL_ERROR
"The callback function for the iitableinfo function returned an error.
Check the DBMS error log for more information."
E_AD2107_LASTID_FCN_ERR:SS5000B_INTERNAL_ERROR
"The callback function for the last_identity function returned an error.
Check the DBMS error log for more information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	Upon executing the callback function for iitotal_overflow_pages(),
**	ADF received a status indicating that an error occurred check the
**	error log for more information.
**
**	This function is normally called when selecting information from the
**	standard catalog, iitables.
**
** System Status:
**	The query involving the iitotal_overflow_pages() function will be 
**	aborted.
**
** Recommendation:
**	Call Ingres Corporation Technical Support with this information.
*/
E_AD3001_DTS_NOT_SAME:SS5000B_INTERNAL_ERROR
"Some ADF function was given two datatypes that were expected to be the same\
 but were not.  The most likely routines would be adc_keybld(), adc_compare(),\
 adc_minmaxdv(), or the ADE_KEYBLD CX instruction."
E_AD3002_BAD_KEYOP:SS5000B_INTERNAL_ERROR
"KEYBLD (either the adc_keybld() routine or the ADE_KEYBLD instruction) was\
 passed an unknown key operator."
E_AD3003_DLS_NOT_SAME:SS5000B_INTERNAL_ERROR
"KEYBLD (either the adc_keybld() routine or the ADE_KEYBLD instruction) was\
 passed low and high data values with inconsistent data lengths."
E_AD3004_ILLEGAL_CONVERSION:SS5000B_INTERNAL_ERROR
""      /* Dead error code */
E_AD3005_BAD_EQ_DTID:SS5000B_INTERNAL_ERROR
""      /* Dead error code */
E_AD3006_BAD_EQ_DTLEN:SS5000B_INTERNAL_ERROR
""      /* Dead error code */
E_AD3007_BAD_DS_DTID:SS5000B_INTERNAL_ERROR
""      /* Dead error code */
E_AD3008_BAD_DS_DTLEN:SS5000B_INTERNAL_ERROR
""      /* Dead error code */
E_AD3009_BAD_HP_DTID:SS5000B_INTERNAL_ERROR
""      /* Dead error code */
E_AD3010_BAD_HP_DTLEN:SS5000B_INTERNAL_ERROR
"Illegal data length for a hash-prep value."
E_AD3011_BAD_HG_DTID:SS5000B_INTERNAL_ERROR
"Illegal datatype for a histogram element."
E_AD3012_BAD_HG_DTLEN:SS5000B_INTERNAL_ERROR
"Illegal data length for a histogram element."
E_AD4001_FIID_IS_AG:SS5000B_INTERNAL_ERROR
"Trying to use an aggregate function instance as a non-aggregate."
E_AD4002_FIID_IS_NOT_AG:SS5000B_INTERNAL_ERROR
"Trying to use a non-aggregate function instance as an aggregate."
E_AD4003_AG_WORKSPACE_TOO_SHORT:SS5000B_INTERNAL_ERROR
"Aggregate needs more work space ... ADF_AG_STRUCT.adf_agwork.db_length needs\
 to be >= ADI_FI_DESC.adi_agwsdv_len."
E_AD4004_BAD_AG_DTID:SS22500_INVALID_DATA_TYPE
"Illegal datatype for result of an aggregate."
E_AD4005_NEG_AG_COUNT:SS5000B_INTERNAL_ERROR
"A negative number of values have been aggregated!?  How can this be?"
E_AD5001_BAD_STRING_TYPE:SS22500_INVALID_DATA_TYPE
"Datatype is not a string datatype."
E_AD5002_BAD_NUMBER_TYPE:SS22500_INVALID_DATA_TYPE
"Datatype is not a numeric datatype."
E_AD5003_BAD_CVTONUM:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5004_OVER_MAXTUP:SS50002_LIMIT_EXCEEDED
"Length of a data value exceeds maximum tuple length."
E_AD5005_BAD_DI_FILENAME:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5007_BAD_HEX_CHAR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5009_BAD_RESULT_STRING_TYPE:SS22500_INVALID_DATA_TYPE
"Result datatype is not a varying length string datatype."
E_AD5010_SOURCE_STRING_TRUNC:SS5000B_INTERNAL_ERROR
"UTF8 string supplied is of incorrect format"
E_AD5011_NOCHARMAP_FOUND:SS5000B_INTERNAL_ERROR
"Cannot coerce to unicode no unicode mapping found \
in coercion map table for character %0c."
E_AD5012_UCETAB_NOT_EXISTS:SS5000B_INTERNAL_ERROR
"No unicode collation table found for the installation."
E_AD5015_INVALID_BYTESEQ:SS5000B_INTERNAL_ERROR
"%0x, is not a valid byte sequence for server codepage."
E_AD5016_NOUNIMAP_FOUND:SS5000B_INTERNAL_ERROR
"Unicode code point %0x cannot be mapped to local character set."
E_AD5017_NOMAPTBL_FOUND:SS5000B_INTERNAL_ERROR
"Specified unicode converter mapping table is not found in \
this installation. Unicode conversion is not possible. "
E_AD5019_UDECOMP_COL:SS5000B_INTERNAL_ERROR
"Unicode normalization has requested decomposition \
for a character which does not have any decomposition \
information." 
E_AD5020_BADCH_MNY:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5021_MNY_SIGN:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5022_DECPT_MNY:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5031_MAXMNY_OVFL:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5032_MINMNY_OVFL:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5050_DATEADD:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5051_DATESUB:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5052_DATEVALID:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5053_DATEYEAR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5054_DATEMONTH:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5055_DATEDAY:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5056_DATETIME:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5058_DATEBADCHAR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5059_DATEAMPM:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD505A_DATEYROVFLO:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD505B_DATEYR:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD505C_DOWINVALID:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD505D_DATEABS:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD505E_NOABSDATES:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD505F_DATEINTERVAL:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5060_DATEFMT:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5061_DGMT_ON_INTERVAL:SS42000_SYN_OR_ACCESS_ERR
"The date_gmt function only works on absolute dates."
E_AD5062_DATEOVFLO:SS5000B_INTERNAL_ERROR
""      /* Maps to user error message in ERUSF.MSG */
E_AD5063_BAD_SYSTIME:SS5000B_INTERNAL_ERROR
"Bad High resolution system time passed to ADF date conversion."
/*%
** Parameters:
**	None.
**
** Explanation:
**	A conversion from a datatype to either a "table_key" or an
**	"object_key" was either explicitly (i.e., table_key() or object_key()
**	functions) or implicitly (conversion attempted automatically by the
**	DBMS) attempted.  This conversion only works if the lengths of the
**	datatype are the same.  The length of the "table_key" datatype is 8
**	bytes; the length of the "object_key" datatype is 16 bytes.  The
**	conversion attempted failed because the length of the source
**	datatype and the length of the destination datatype were different.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	If attempting to convert strings to logical key datatype, make sure
**	to specify a string of the appropriate length (8 or 16 bytes).
*/
E_AD5064_DATEANSI:SS5000B_INTERNAL_ERROR
""	/* Maps to user error message in ERUSF.MSG */
E_AD5065_DATE_ALIAS_NOTSET:SS5000B_INTERNAL_ERROR
""	/* Maps to user error message in ERUSF.MSG */
E_AD5066_DATE_COERCION:SS5000B_INTERNAL_ERROR
""	/* Maps to user error message in ERUSF.MSG */
E_AD5080_LOGKEY_BAD_CVT_LEN:SS42000_SYN_OR_ACCESS_ERR
"The conversion from the datatype specified (length %1d bytes) to logical key
type (ie. table_key or object_key with length %0d bytes) failed because the
lengths of the two datatypes were not the same."
E_AD5081_UNICODE_FUNC_PARM:SS42000_SYN_OR_ACCESS_ERR
"A Unicode function has a parameter data type that is unsupported."
E_AD5082_NON_UNICODE_DB:SS42000_SYN_OR_ACCESS_ERR
"Unicode operations were attempted on a non-Unicode enabled database."
E_AD5090_DATE_MUL:SS5000B_INTERNAL_ERROR
"Date multiply operates only on interval forms with float"
E_AD5091_DATE_DIV:SS5000B_INTERNAL_ERROR
"Date divide operates only on interval forms with float or between two interval forms"
E_AD5092_DATE_DIV_BY_ZERO:SS22012_DIVISION_BY_ZERO
""	/* Maps to user error message in ERUSF.MSG */
E_AD5093_DATE_DIV_ANSI_INTV:SS42000_SYN_OR_ACCESS_ERR
""	/* Maps to user error message in ERUSF.MSG */
E_AD5094_DATE_ARITH_NOABS:SS42000_SYN_OR_ACCESS_ERR
""	/* Maps to user error message in ERUSF.MSG */
E_AD50A0_BAD_UUID_PARM:SS5000B_INTERNAL_ERROR
"Bad parameter to uuid_from_char() function."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The uuid_from_char() function was coded with a non-conformant 
**	parameter. Parameter values to this function call must contain
**	36 byte string values that precisely follow the format:
**	aaaaaaaa-bbbb-cccc-dddd-eeeeeeeeeeee
**	where the a's, b's, c's, d,s and e's are all valid hexidecimal
**	digits.
**
** System Status:
**	The operation is rejected.
**
** Recommendation:
**	Assure the uuid_from_char() parameter values follow the 
**	formatting requirements.
*/
E_AD5500_BAD_SEG:SS5000B_INTERNAL_ERROR
"Unknown CX segment."
E_AD5501_BAD_SEG_FOR_ICODE:SS5000B_INTERNAL_ERROR
"The instruction being compiled cannot be placed in the requested CX segment."
E_AD5502_WRONG_NUM_OPRS:SS5000B_INTERNAL_ERROR
"Wrong number of operands for a function instance.\n\
 It is possible that there have been changes to ADF's function instance table,\
 that make some views, integrities, or permits no longer valid."
E_AD5503_BAD_DTID_FOR_FIID:SS5000B_INTERNAL_ERROR
"Datatype of operand (input or output) for a function instance is not correct. \
 It is possible that there has been a change to the datatypes accepted or\
 returned for this function instance in ADF's function instance table, thus\
 making a view, integrity, or permit no longer valid."
E_AD5504_BAD_RESULT_LEN:SS5000B_INTERNAL_ERROR
"Length of result data for a function instance is not correct.\n\
 It is possible that there has been a change to the lenspec for this function\
 instance in ADF's function instance table, thus making a view, integrity, or\
 permit no longer valid."
E_AD5505_UNALIGNED:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 ADE has detected an attempt to compile an unaligned piece of data."
E_AD5506_NO_SPACE:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 The CX is out of space; caller must make it bigger."
E_AD5507_BAD_DTID_FOR_KEYBLD:SS5000B_INTERNAL_ERROR
"The output operand's length for the ADE_KEYBLD instruction must be `int'."
E_AD5508_BAD_DTLEN_FOR_KEYBLD:SS5000B_INTERNAL_ERROR
"The output operand's length for the ADE_KEYBLD instruction must be 2."
E_AD5509_BAD_RANGEKEY_FLAG:SS5000B_INTERNAL_ERROR
"Range-key flag supplied when compiling the CX special instruction,\
 ADE_KEYBLD, is not recognized by the ade_instr_gen() routine."
E_AD550A_RANGE_FAILURE:SS5000B_INTERNAL_ERROR
"< Expected internal error code ... No message needed. >\n\
 The ADE_KEYBLD instruction has detected a RANGE-FAILURE; that is, when it was\
 compiled it was told to accept only RANGE-type keys, and the key found is not,\
 or it was told to accept only non-RANGE-type keys, and the key found was."
E_AD550B_TOO_FEW_BASES:SS5000B_INTERNAL_ERROR
"The CX header states that there should be more base addresses that the caller\
 has supplied."
E_AD550C_COMP_NOT_IN_PROG:SS5000B_INTERNAL_ERROR
"%0c is attempting to compile an instruction or constant into a CX without\
 first calling ade_bgn_comp() to initialize the CX."
E_AD550D_WRONG_CX_VERSION:SS5000B_INTERNAL_ERROR
"This version of ADF cannot execute the supplied CX due to a version mis-match."
E_AD550E_TOO_MANY_VLTS:SS5000B_INTERNAL_ERROR
"The maximum number of VLTs (Variable Length Temporaries) has been exceeded.\n\
 This query cannot be executed."
E_AD550F_BAD_CX_REQUEST:SS5000B_INTERNAL_ERROR
"Unknown request code detected by ade_cxinfo() routine: %0d."
E_AD5510_BAD_DTID_FOR_ESCAPE:SS5000B_INTERNAL_ERROR
"The operand's type for the ADE_ESC_CHAR instruction must be 'char' or 'nchar'."
E_AD5511_BAD_DTLEN_FOR_ESCAPE:SS5000B_INTERNAL_ERROR
"The output operand's length for the ADE_ESC_CHAR instruction must be 1 character."
E_AD5512_BAD_DTID2_FOR_ESCAPE:SS5000B_INTERNAL_ERROR
"The 2nd operand's type for the ADE_ESC_CHAR instruction must be 'int'."
E_AD5513_BAD_DTLEN2_FOR_ESCAPE:SS5000B_INTERNAL_ERROR
"The 2nd operand's length for the ADE_ESC_CHAR instruction must be 2."

E_AD5600_GEOSPATIAL_USER:SS50000_MISC_ERRORS
"An error has occured in Ingres Geospatial: %0c"
E_AD5601_GEOSPATIAL_INTERNAL:SS5000B_INTERNAL_ERROR
"An internal error has occured in Ingres Geospatial: %0c"
E_AD5602_SPATIAL_VERSION:SS5000B_INTERNAL_ERROR
"An invalid spatial data version was found."
E_AD6000_BAD_MATH_ARG:SS5000B_INTERNAL_ERROR
"Invalid argument in call to math function."
E_AD6001_BAD_MATHOPT:SS5000B_INTERNAL_ERROR
"Unknown math exception option found in ADF's session control block."
E_AD7000_BAD_ADP_INFO:SS5000B_INTERNAL_ERROR
"Error calling peripheral datatype extension routine to get information."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system is unable to determine the parameters for use in obtaining
**	peripheral datatypes.
**
** System Status:
**	Peripheral datatype processing will not operate.
**	All non-peripheral datatypes should operate normally.
**
** Recommendation:
**	Call the site technical contact.  This is an internal error, and
**	probably requires vendor support.
*/
E_AD7001_ADP_NONEXT:SS5000B_INTERNAL_ERROR
"No more peripheral datatype segments found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	This is a normal occurrence.  This error message should not be seen by
**	users.
**
** System Status:
**	No change in system status.
**
** Recommendation:
**	If this error appears, contact the site technical representative.
**	This is an internal error, and probably requires vendor support.
*/
E_AD7002_ADP_BAD_GET:SS5000B_INTERNAL_ERROR
"Error getting the next segment of a peripheral datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to fetch the component parts of a peripheral
**	datatype.  In doing so, an error was encountered.  Other, more detailed
**	errors will be found in the error log associated with this error.
**
** System Status:
**	The operation is aborted.
**
** Recommendation:
**	This is an internal error.  Contact the site technical representative.
**	Vendor support is probably required, depending on the content of the
**	other, more detailed, messages associated with this error.
*/
E_AD7003_ADP_BAD_RECON:SS5000B_INTERNAL_ERROR
"An error occurred while trying to reconstitute a peripheral datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system encountered an error (which should be found in the log file)
**	while attempting to reconstitute a peripheral datatype from its
**	component parts.
**
** System Status:
**	The system should remain operational;  the query in which this error
**	occurred will be aborted.
**
** Recommendation:
**	Examine the other errors in the error log file.  Other, more detailed,
**	messages should be found which will indicate the actual cause of the
**	problem.
*/
E_AD7004_BAD_BLOB:SS5000B_INTERNAL_ERROR
"A peripheral datatype (BLOB) was found whose coupon length did not agree with\
 the amount of data found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to reconstitute a peripheral datatype (BLOB).
**	While doing so, the amount of data found did not agree with the amount
**	required.  Therefore, the peripheral datatype was declared inconsistent.
**
** System Status:
**	The system will continue to operate normally;  however, this particular
**	data element will not be available.
**
** Recommendation:
**	Examine any other errors in the log file.  Because this is an internal
**	error, contact with the product vendor is probably necessary.
*/
E_AD7005_BAD_POP:SS5000B_INTERNAL_ERROR
"An invalid peripheral operation was requested."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to operate on a peripheral datatype. In doing
**	so, a call was made requesting a peripheral operation that was invalid.
**
** System Status:
**	The system will continue to operate normally.
**
** Recommendation:
**	This is an internal error. Please contact Ingres Corporation 
**	Technical Support with information about the query in use and 
**	the runtime environment.  Information about the datatypes in use 
**	at the time (especially any user defined types) will be very 
**	helpful in resolving the problem.
*/
E_AD7006_ADP_PUT_PUT_ERROR:SS5000B_INTERNAL_ERROR
"An error was encountered in attempting to perform the peripheral put operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a PUT operation for a peripheral
**	datatype.  In doing so, an error PUTting the underlying segment was
**	encountered.
**
** System Status:
**	The system will continue to operate normally.
**
** Recommendation:
**	The error log should be examined to determine more detail about this
**	error.  Other errors should exist in the error log in support of this
**	high-level error.  Follow the action recommended for these other, more
**	detailed errors.
*/
E_AD7007_ADP_GET_ERROR:SS5000B_INTERNAL_ERROR
"An error was encountered performing the ADP GET operation on a peripheral datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system encountered an error in fetching a portion of a peripheral
**	datatype.
**
** System Status:
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** Recommendation:
**	Examine the error log for other, more detailed errors in support of this
**	high-level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD7008_ADP_DELETE_FAILURE:SS5000B_INTERNAL_ERROR
"An error was encountered performing an ADP_DELETE operation on a peripheral datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system encountered an error in deleting a portion of a peripheral
**	datatype.
**
** System Status:
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** Recommendation:
**	Examine the error log for other, more detailed errors in support of this
**	high-level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD7009_ADP_MOVE_FAILURE:SS5000B_INTERNAL_ERROR
"An error was encountered performing an ADP_MOVE operation on a peripheral datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system encountered an error in moving a peripheral datatype.
**
** System Status:
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** Recommendation:
**	Examine the error log for other, more detailed errors in support of this
**	high-level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD700A_ADP_REPLACE_FAILURE:SS5000B_INTERNAL_ERROR
"An error was encountered performing an ADP_REPLACE operation on a peripheral datatype."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system encountered an error in replacing a peripheral
**	datatype.
**
** System Status:
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** Recommendation:
**	Examine the error log for other, more detailed errors in support of this
**	high-level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD700B_BAD_POP_CB:SS5000B_INTERNAL_ERROR
"An invalid peripheral operations control block was passed."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a peripheral operation
**	However, the control block used to request the operation was
**	invalid.
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** Recommendation:
**	This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information
**	and report the problem to Ingres Corporation Technical Support.
*/
E_AD700C_BAD_POP_UA:SS5000B_INTERNAL_ERROR
"Internal synchronization with peripheral operation's user argument has been lost."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a peripheral operation.
**	However, the control block used to request the operation was
**	invalid -- the user argument field was corrupted.
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** Recommendation:
**	This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information
**	and report the problem to Ingres Corporation Technical Support.
*/
E_AD700D_ADP_NO_COUPON:SS5000B_INTERNAL_ERROR
"No coupon value was passed as required."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a peripheral operation
**	However, the control block used to request the operation was
**	invalid -- the coupon value was not a coupon.
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** Recommendation:
**	This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information
**	and report the problem to Ingres Corporation Technical Support.
*/
E_AD700E_ADP_BAD_COUPON:SS5000B_INTERNAL_ERROR
"An invalid coupon value was passed to the DMPE routines."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a peripheral operation
**	However, the control block used to request the operation was
**	invalid -- the coupon was invalid.
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** Recommendation:
**	This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information
**	and report the problem to Ingres Corporation Technical Support.
*/
E_AD700F_ADP_BAD_CPNIFY:SS5000B_INTERNAL_ERROR
"An error converting a large object to a coupon."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a peripheral operation
**	However, an error, described in surrounding messages, was
**      encountered converting a large object to a coupon.  In
**      particular, the specified length of the object and length
**      discovered in storing it did not agree (discovered length was
**      less than specified).
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered the error will be aborted.
**
** Recommendation:
**      This is an internal error.  Please gather information about the
**	query, datatypes in use, tables in use, and other
**	environmental information and report the problem to Ingres Corporation 
**	Technical Support.
*/
E_AD7010_ADP_USER_INTR:SS5000B_INTERNAL_ERROR
"A user interrupt was encountered while performing a peripheral operation."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a peripheral operation
**	However, an error, described in surrounding messages, was
**      encountered converting a large object to a coupon.
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered the interrupt will be aborted.
**
** Recommendation:
**      This is an minor internal error, in that this error should not
**	be seen in the error log.  Please gather information about the
**	query, datatypes in use, tables in use, and other
**	environmental information and report the problem to Ingres Corporation 
**	Technical Support.
*/
E_AD7011_ADP_BAD_CPNIFY_OVER:SS5000B_INTERNAL_ERROR
"An error converting a large object to a coupon."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to perform a peripheral operation
**	However, an error, described in surrounding messages, was
**      encountered converting a large object to a coupon.  In
**      particular, the specified length of the object and length
**      discovered in storing it did not agree (discovered length was
**      greater than specified).
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered the error will be aborted.
**
** Recommendation:
**      This is an internal error.  Please gather information about the
**	query, datatypes in use, tables in use, and other
**	environmental information and report the problem to Ingres Corporation 
**	Technical Support.
*/
E_AD7012_ADP_WORKSPACE_TOOLONG:SS5000B_INTERNAL_ERROR
"The required workspace is too long to be supported by adc_cvinto()\
 routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was attempting to execute a function instant 
**  	using adc_cvinto() that require temporary workspace to
**      to handle BLOB value.  The workspace size is too long to 
**	be handled by adc_cvinto().
**
** System Status:
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** Recommendation:
**	This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information
**	and report the problem to Ingres Corporation Technical Support.
*/
E_AD7013_ADP_CPN_TO_LOCATOR:SS5000B_INTERNAL_ERROR
"An error was encountered converting a coupon to a locator"
E_AD7014_ADP_LOCATOR_TO_CPN:SS5000B_INTERNAL_ERROR
"An error was encountered converting a locator to a coupon"
E_AD7015_ADP_FREE_LOCATOR:SS5000B_INTERNAL_ERROR
"An error was encountered freeing a locator"
E_AD7016_ADP_MVCC_INCOMPAT:SS5000B_INTERNAL_ERROR
"The blob extension table is incompatible with lock level MVCC"
E_AD8000_OP_COUNT_SPEC:SS5000B_INTERNAL_ERROR
"Add datatype control block specified a nonzero number\
 of operators to add;  no operator descriptions were provided."
/*%
** Parameters:
**	None.
**
** System Status:
**	The server will start up with no user defined datatypes added (unless
**	otherwise specified in the datatype control block).  Only vendor
**	supplied datatypes (the defaults) will be available.
**
** Recommendation:
**	Correct the problem.  Either the add_fo_dfn field should be set, or the
**	add_fo_cnt field should be set to zero (0).
*/
E_AD8001_OBJECT_TYPE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The dtd_object_type,\
fod_object_type, or fid_object_type field was not set to match the object \
being defined."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will (by default) start up with no user defined datatypes
**	added.  Only the system default datatypes will be available.
**
** Recommendation:
**	Correct the problem.  The field in question should be set to either
**	ADD_O_DATATYPE, ADD_O_OPERATOR, or ADD_O_FUNCTION_INSTANCE respectively.
*/
E_AD8002_OP_OPID:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  the fod_id field was invalid."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the field value -- fod_id must be greater than or equal to the
**	ADD_OPSTART constant provided in iiadd.h.
*/
E_AD8003_OP_OPTYPE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The operation type was invalid."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the field. The fod_optype field must correspond to one of the
**	values provided in the header file.
*/
E_AD8004_OP_COUNT_WRONG:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The number of operators added\
 does not agree with the number requested."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Examine the operators (and any previous errors).  If these are all
**	correct, correct the add_op_cnt field.
*/
E_AD8005_DT_COUNT_SPEC:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  A count of datatypes to \
add was specified (nonzero), but no datatype definitions were included."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the error.  Either provide the datatype definitions or set the
**	add_dt_cnt field to zero.
*/
E_AD8006_DT_DATATYPE_ID:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The dtd_id field contained \
an illegal value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system has found a value in the dtd_id field which was not
**	acceptable.  Values in this field must fall in the range specified
**	by ADD_LOW_USER and ADD_HIGH_USER.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the value.
*/
E_AD8007_DT_FUNCTION_ADDRESS:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  A required function \
address was missing."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system has found a missing or invalid address for a required
**	datatype function.  All required functions must be specified with
**	valid addresses.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the problem.
*/
E_AD8008_DT_COUNT_WRONG:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  Fewer datatypes were added \
than requested."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system has added fewer datatype definitions than were requested.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the problem. Either the datatype definitions or the count field
**	(add_dt_cnt) must be corrected.
*/
E_AD8009_FI_SORT_ORDER:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The function instance \
definitions were not provided in the correct order."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system has determined that the sort order for the proposed function
**	instances is incorrect.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the sort order.  The function instances must be provided in
**	order of increasing operation types, and in order of increasing operator
**	IDs within a given type.
*/
E_AD800A_FI_COUNT_SPEC:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The add_fi_cnt specified \
function instances to be defined, but no definitions were found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system was requested to define a number of function instances,
**	but no function instance definitions were found.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the control blocks.  Either set the count field (add_fi_cnt)
**	to zero, or provide the appropriate definitions.
*/
E_AD800B_FI_RLTYPE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The fid_rltype field was invalid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found an invalid value in the fid_rltype field.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the field.  The only valid values are provided in the iiadd.h
**	header file.
*/
E_AD800C_FI_INSTANCE_ID:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The fid_id field contained \
an invalid value."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found an invalid value in the fid_id field.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the field.  All values must be >= the ADD_FISTART
**	value provided in the iiadd.h header file.
*/
E_AD800D_FI_OPTYPE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The value of the fid_optype field \
was invalid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found an invalid operation type field in a function instance
**	definition (fid_optype).
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the value.  The only legal values here match those provided for
**	the fod_optype field in iiadd.h.
*/
E_AD800E_FI_NUMARGS:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The number of arguments is \
not compatible with the function instance operation type."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found an incompatible number of arguments. Various operation
**	types require specific argument counts.  Please check the documentation
**	to determine the correct range of numbers.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the value.
*/
E_AD800F_FI_NO_COMPLEMENT:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  Comparison function \
instance found with no associated complement."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system has found a comparison function instance with either no
**	complement or one which was invalid.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the instance by providing a valid complement function instance.
*/
E_AD8010_FI_ARG_SPEC:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  A non-zero argument count \
was provided without corresponding argument definitions."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found a function instance with a non-zero argument count.
**	However, the argument definition specification was missing or invalid.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct either the argument count or the definition field.
*/
E_AD8011_FI_DUPLICATE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  A duplication function \
instance definition was found."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found a function instance definition for a function instance
**	ID which was already in use.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the value for the fid_id.  All function instance identification
**	numbers must be unique.
*/
E_AD8012_UD_ADDRESS:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The system found an invalid \
address in a user defined datatype definition."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found (either via inspection or because of an address fault)
**	that a user provided an address that was invalid.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Please ensure that all values passed to the system are valid.
*/
E_AD8013_FI_ROUTINE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes: The system found a function \
instance definition which had no corresponding implementing routine."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found, by inspection, that a function instance was defined
**	with no routine specified to implement it.  All function instance
**	definitions must provide a routine specification.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the definition.
*/
E_AD8014_DT_NAME:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  Datatype name is invalid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found, by inspection, that a datatype name was specified
**	which is not a valid.  User defined datatype names must follow the 
**	naming conventions.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the name so that it is a valid.
*/
E_AD8015_OP_NAME:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  Function or Operator name is invalid."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found, by inspection, that a function or operator name was
**	not valid.  User defined datatype names must follow the documented
**	naming conventions.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Correct the name so that it is valid. 
*/
E_AD8016_OP_DUPLICATE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  Operator identifier (fod_id) field already exists."
/*%
** Parameters:
**	None.
**
** Explanation:
**	The system found, by inspection, that a function or operator ID
**	duplicates another being defined.  All function or operators must have
**	unique identifiers.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Provide a unique identifier for each function or operator.
*/
E_AD8017_INVALID_RESULT_TYPE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  A function instance was found with a result datatype\
 identifier which is unknown."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Make sure that all datatype identifiers in use are known by the system.
**	Verify that the datatype count field in use correctly matches the number
**	of new datatypes being provided to the system.
*/
E_AD8018_INVALID_PARAM_TYPE:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  A function instance was found with the type \
specified for one of its parameters which is unknown."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will start (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** Recommendation:
**	Make sure that all datatype identifiers in use are known by the system.
**	Verify that the datatype count field in use correctly matches the number
**	of datatypes being provided to the system.
*/
E_AD8019_OPTYPE_MISMATCH:SS5000B_INTERNAL_ERROR
"User Defined Datatypes:  The operation type specified for a function instance \
does not match that specified for the corresponding operation."
/*%
** Parameters:
**	None.
**
** System Status:
**	The program will start (by default) with no user defined datatypes
**	available.  Only system defaults will be available.
**
** Recommendation:
**	Make sure that function instance operation types agree with the
**	corresponding operation definition.
*/
E_AD8999_FUNC_NOT_IMPLEMENTED:SS50008_UNSUPPORTED_STMT
"The function `%0c()' is not currently implemented."
E_AD9000_BAD_PARAM:SS5000B_INTERNAL_ERROR
"Invalid parameter(s) passed to adg..._size(), _startup(), _sz_augment(),\
or _augment()."
E_AD9001_BAD_CALL_SEQUENCE:SS5000B_INTERNAL_ERROR
"Adg_sz_augment() or Adg_augment() was called before ADF had been initialized."
E_AD9002_AUGMENT_SIZE:SS5000B_INTERNAL_ERROR
"Size of control block passed to adg_augment() is too small."
E_AD9003_TABLE_INIT:SS5000B_INTERNAL_ERROR
"Error initializing ADF's internal tables."
E_AD9004_SECURITY_LABEL:SS5000B_INTERNAL_ERROR
"An ADF internal error has been detected in the Security label routines."
E_AD9998_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"An ADF internal coding error has been detected (%0c)."
/*%
** Parameters:
**	%0c - a tag indicating where the error was detected.
**
** Explanation:
**	The Abstract Data Facility found itself in an unexpected
**	state.  This is usually a bug in ADF itself, and should
**	be reported.
**
** System Status:
**	Depending on what the error is, the operation, query, or session
**	may be aborted.
**
** Recommendation:
**	This error usually means that a bug exists, and should be
**	reported to Ingres Corporation Technical Support.  Be sure to report the
**	entire error message exactly.
*/
E_AD9999_INTERNAL_ERROR:SS5000B_INTERNAL_ERROR
"An ADF internal coding error has been detected."

NO_SQLSTATE_MAPPING
