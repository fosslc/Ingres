GENERIC_MAPPING_REQUIRED

/*
**  30-nov-90 (sandyd)
**	This is a concatenated snapshot of message source from the 
**	ingres6302p piccolo library, representing the following versions
**	of files from ingres6302p:
**
**	    generic!common!hdr!hdr eradf.msg	1.1
**	    generic!common!hdr!hdr erclf.msg	1.2
**	    generic!common!hdr!hdr erdmf.msg	1.4
**	    generic!common!hdr!hdr ergcf.msg	1.1
**	    generic!common!hdr!hdr eropf.msg	1.1
**	    generic!common!hdr!hdr erpsf.msg	1.2
**	    generic!common!hdr!hdr erqef.msg	1.1
**	    generic!common!hdr!hdr erqsf.msg	1.1
**	    generic!common!hdr!hdr errdf.msg	1.1
**	    generic!common!hdr!hdr erscf.msg	1.1
**	    generic!common!hdr!hdr erulf.msg	1.1
**	    generic!common!hdr!hdr erusf.msg	1.1
**
**	These are all the .msg files that include generic error mappings.
**	(That's all the er*.msg files from common!hdr!hdr EXCEPT erduf.msg.)
**
**	Note that ergwf.msg, errqf.msg, and ertpf.msg were not being
**	referenced in the ingres6302p common!hdr!hdr!MINGS at the time
**	I took this snapshot, so those are not included.  (Those are just
**	just RMS gateway and STAR messages, so they are unused in 6.3/02 
**	UNIX releases.)
*/


/*
**    Message file for Facility code: _AD
*/

#define _AD_CLASS 2


E_AD0000_OK:E_GE0000_OK
"ADF successful return status."
E_AD0001_EX_IGN_CONT:NO_GE
"< Expected internal error code ... No message needed. >\n\
 An ADF math exception has occurred with the math exception option set to\
 ignore."
E_AD0002_INCOMPLETE:NO_GE
"An operation on a peripheral datatype requires more space and/or data;\n\
please call again.  This message is expected and should not appear in a log\n\
file."
/*%
** Parameters:
**      None.
**
** System Status:
**      All is normal.  In that this is an expected message.  However, if this
**	message appears in a log file, then something is wrong.
**
** User Action:
**      Call product vendor if this message appears.  Internally, it is an
**	expected event;  however, it should never appear externally.
*/
E_AD0101_EMBEDDED_CHAR_TRUNC:E_GE0032_WARNING
"Truncation has occurred while converting a string type data base value into\
 a string type embedded value."
E_AD0102_NULL_IN_TEXT:E_GE0032_WARNING
"Warning: Null character(s) converted to blank(s) in text string."
E_AD0115_EX_WRN_CONT:E_GE0032_WARNING
"< Expected internal error code ... No message needed. >\n\
 An ADF math exception has occurred with the math exception option set to\
 warn."
E_AD0116_EX_UNKNOWN:E_GE0032_WARNING
"< Expected internal error code ... No message needed. >\n\
 The current exception is not one that ADF recognizes."
E_AD0120_INTDIV_WARN:E_GE0032_WARNING
""	/* Maps to user warning message in ERUSF.MSG */
E_AD0121_INTOVF_WARN:E_GE0032_WARNING
""	/* Maps to user warning message in ERUSF.MSG */
E_AD0122_FLTDIV_WARN:E_GE0032_WARNING
""	/* Maps to user warning message in ERUSF.MSG */
E_AD0123_FLTOVF_WARN:E_GE0032_WARNING
""	/* Maps to user warning message in ERUSF.MSG */
E_AD0124_FLTUND_WARN:E_GE0032_WARNING
""	/* Maps to user warning message in ERUSF.MSG */
E_AD0125_MNYDIV_WARN:E_GE0032_WARNING
""	/* Maps to user warning message in ERUSF.MSG */
E_AD0126_DECDIV_WARN:E_GE0032_WARNING
"Warning: %0d Packed Decimal divide by zero operation(s) detected."
/*%
** Parameters:
**      %0d	Number of attempted divisions by zero detected
**
** Description:
**	The operations ADF was asked to perform resulted in a division where
**	the divisor was a packed decimal number with a zero value.  ADF was
**	set to generate warnings for all arithmetic exceptions, so no error
**	was generated.
**
** System Status:
**	This is just a warning message.  The results of all attempted divisions
**	with a zero divisor were set to zero.
**
** User Action:
**	None required.  Alternative handling for arithmetic exceptions is
**	available and fully explained in the INGRES documentation set.
**
**	If these warnings were unexpected, they may be occurring because a
**	small value was truncated to zero while being coerced or converted
**	to a packed decimal number with insufficient scale to hold this small
**	number.  Using the decimal() conversion function to convert the
**	intermediate result to a value with more scale digits might help.
*/
E_AD0127_DECOVF_WARN:E_GE0032_WARNING
"Warning: %0d Packed Decimal overflow(s) detected."
/*%
** Parameters:
**      %0d	Number of overflows detected
**
** Description:
**	The operations ADF was asked to perform resulted in a packed decimal
**	expression or sub-expression whose result exceeded the number of
**	integral digits available in the result value.  Packed decimal result
**	precision and scale are calculated according to the formulas shown
**	in the INGRES documentation.  If an intermediate result has more
**	significant integral (i.e. to the left of the decimal point) digits
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
**	ADF was set to generate warnings for all arithmetic exceptions, so no
**	error was generated.
**
** System Status:
**	This is just a warning message.  The results of all overflowed
**	expressions were set to zero.
**
** User Action:
**	None required.  Alternative handling for arithmetic exceptions is
**	available and fully explained in the INGRES documentation set.
**
**	If these warnings were unexpected, try simplifying the expression which
**	is generating this overflow until you can reproduce the problem with as
**	few variables, column names, arithmetic operators, etc. as possible.
**	Then using the formulas given in the INGRES documentation, calculate
**	the result size for your simplified expression.  It should become clear
**	why the expression is overflowing.  Once you determine why the overflow
**	is occurring, you may wish to use the decimal() conversion function to
**	change the precision and scale of your intermediate result, or if the
**	result is very large, you may wish to convert to a floating point
**	datatype using the float4() or float8() conversion function.
*/
E_AD0500_ABS_DATE_IN_AG:E_GE0032_WARNING
""	/* Maps to user warning message in ERUSF.MSG */
E_AD0FFF_NOT_IMPLEMENTED_YET:NO_GE
"ADF function has not been implemented yet."
E_AD1001_BAD_DATE:E_GE9D0E_DATAEX_DTINV
"ADF date format, %0d, is invalid."
E_AD1002_BAD_MONY_SIGN:E_GE8D68_CONFIG_ERROR
"ADF money sign, %0c, is invalid."
E_AD1003_BAD_MONY_LORT:E_GE8D68_CONFIG_ERROR
"ADF money sign position designater, %0d, is invalid."
E_AD1004_BAD_MONY_PREC:E_GE8D68_CONFIG_ERROR
"ADF money precision specification, %0d, is invalid."
E_AD1005_BAD_DECIMAL:E_GE8D68_CONFIG_ERROR
"Bad decimal character specification, '%0x'."
E_AD1006_BAD_OUTARG_VAL:E_GE8D68_CONFIG_ERROR
"Bad output argument specification (i.e. one of your OUTARG members is bogus)."
E_AD1007_BAD_QLANG:E_GE79E0_UNSUP_LANGUAGE
"Unknown query language ... query language ID = %0d."
E_AD1008_BAD_MATHEX_OPT:E_GE9D24_DATAEX_OTHER
"Unknown math exception option, %0d."
E_AD1009_BAD_SRVCB:NO_GE
"ADF Server control block passed into adg_init() or adg_add_fexi() is invalid."
E_AD100A_BAD_NULLSTR:E_GE8D68_CONFIG_ERROR
"The `NULLstring' passed into adg_init() is invalid."
E_AD100B_BAD_MAXSTRING:NO_GE
"The max size INGRES string given to ADF at session init time (%0d) is invalid.\
  It must be between >= 1 and <= %1d."
E_AD1010_BAD_EMBEDDED_TYPE:E_GE9D17_DATAEX_TYPEINV
"ADF routine was passed an invalid embedded type id."
E_AD1011_BAD_EMBEDDED_LEN:NO_GE
"ADF routine found DB_EMBEDDED_DATA with an invalid length."
E_AD1012_NULL_TO_NONNULL:E_GE7D00_QUERY_ERROR
"An attempt to place a null value in a non-nullable datatype."
E_AD1013_EMBEDDED_NUMOVF:E_GE9D1C_DATAEX_FIXOVR
"Numeric overflow while converting a data base value into an embedded value."
E_AD1014_BAD_VALUE_FOR_DT:E_GE9D17_DATAEX_TYPEINV
"< Expected internal error code ... No message needed. >\n\
 Invalid data for datatype found while doing value checking."
E_AD1015_BAD_RANGE:E_GE7918_SYNTAX_ERROR
"Illegal pattern match specified.  One of the following:\n\
    o  An opening `[' without a closing `]'\n\
    o  Vice-versa\n\
    o  Range ending with `-]'\n\
    o  Range of the form `[x-y]' where char `y' comes before `x'."
E_AD1016_PMCHARS_IN_RANGE:E_GE7918_SYNTAX_ERROR
"Illegal pattern match specified:  You cannot have pattern match\
 characters included in a `[ ]' range."
E_AD1017_ESC_AT_ENDSTR:E_GE7918_SYNTAX_ERROR
"Illegal pattern match specified:  ESCAPE character at end of string."
E_AD1018_BAD_ESC_SEQ:E_GE7918_SYNTAX_ERROR
"Illegal pattern match specified:  Illegal ESCAPE sequence.\n\
 The ESCAPE char must be followed by one of:\n\
    `\%' (percent)\n\
    `_' (underscore)\n\
    `[' (left square bracket)\n\
    `]' (right square bracket)\n\
    another ESCAPE char."
E_AD1019_NULL_FEXI_PTR:NO_GE
"The adg_add_fexi() routine was called with a NULL pointer given as the address
 of the callback routine.  This is an Ingres internal error."
/*%
** Parameters:
**	None
**
** Description:
**	There was an error in the adg_add_fexi() routine.  This means that
**	ADF was unable to successfully obtain the proper callback address for
**	certain external functions it requires.  This is an Ingres internal
**	error.
**
** System Status:
**	This usually occurs during startup.  This error will cause startup
**	failure.
**
** User Action:
**	Call your company's INGRES technical contact with this information.
*/
E_AD1020_BAD_ERROR_LOOKUP:NO_GE
"ADF was unable to lookup error code: %0x"
E_AD1021_BAD_ERROR_PARAMS:NO_GE
"adu_error() was called with a bad argument list."
E_AD1022_BAD_USER_LOOKUP:NO_GE
"ADF was unable to lookup the USER error code, %0d"
E_AD1025_BAD_ERROR_NUM:NO_GE
"adu_error() was called with an unknown error code, %0x"
E_AD1030_F_COPY_STR_TOOSHORT:E_GE8D68_CONFIG_ERROR
"The floating point number being copied will not fit in the field specified. \
 Either give a longer field, or use a different setting of the -f flag on the\
 command line."
E_AD1040_CV_DVBUF_TOOSHORT:NO_GE
"ADF routine, adc_tolocal(), was passed a data value buffer that was too small\
 to do the conversion from the network standard form."
E_AD1041_CV_NETBUF_TOOSHORT:NO_GE
"ADF routine, adc_tonet(), was passed a buffer that was too small\
 to do the conversion to the network standard form."
E_AD1050_NULL_HISTOGRAM:NO_GE
"< Expected internal error code ... No message needed. >\n\
 Attempt to create a histogram element for a NULL value."
E_AD1120_INTDIV_ERROR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD1121_INTOVF_ERROR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD1122_FLTDIV_ERROR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD1123_FLTOVF_ERROR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD1124_FLTUND_ERROR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD1125_MNYDIV_ERROR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD1126_DECDIV_ERROR:E_GE9D20_DATAEX_DCDIV
"Packed Decimal divide by zero detected in query."
/*%
** Parameters:
**	None
**
** Description:
**	The operation ADF was asked to perform resulted in a division where
**	the divisor was a packed decimal number with a zero value.  ADF was
**	set to generate errors for all arithmetic exceptions, so the query
**	was immediately aborted.
**
** System Status:
**	The query was aborted.
**
** User Action:
**	If this error was unexpected, it may be occurring because a
**	small value was truncated to zero while being coerced or converted
**	to a packed decimal number with insufficient scale to hold this small
**	number.  Using the decimal() conversion function to convert the
**	intermediate result to a value with more scale digits might help.
**
**	Alternative handling for arithmetic exceptions is available and fully
**	explained in the INGRES documentation set.
*/
E_AD1127_DECOVF_ERROR:E_GE9D0B_DATAEX_NUMOVR
"Packed Decimal overflow detected in query."
/*%
** Parameters:
**      %0d	Number of overflows detected
**
** Description:
**	The operation ADF was asked to perform resulted in a packed decimal
**	expression or sub-expression whose result exceeded the number of
**	integral digits available in the result value.  Packed decimal result
**	precision and scale are calculated according to the formulas shown
**	in the INGRES documentation.  If an intermediate result has more
**	significant integral (i.e. to the left of the decimal point) digits
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
** User Action:
**	If this error was unexpected, try simplifying the expression which
**	is generating this overflow until you can reproduce the problem with as
**	few variables, column names, arithmetic operators, etc. as possible.
**	Then using the formulas given in the INGRES documentation, calculate
**	the result size for your simplified expression.  It should become clear
**	why the expression is overflowing.  Once you determine why the overflow
**	is occurring, you may wish to use the decimal() conversion function to
**	change the precision and scale of your intermediate result, or if the
**	result is very large, you may wish to convert to a floating point
**	datatype using the float4() or float8() conversion function.
**
**	Alternative handling for arithmetic exceptions is available and fully
**	explained in the INGRES documentation set.
*/
E_AD2001_BAD_OPNAME:NO_GE
"ADF routine was given an unknown operator."
E_AD2002_BAD_OPID:NO_GE
"ADF routine was passed an invalid operator id."
E_AD2003_BAD_DTNAME:E_GE9D17_DATAEX_TYPEINV
"ADF routine was passed an unknown datatype name."
E_AD2004_BAD_DTID:E_GE9D17_DATAEX_TYPEINV
"ADF routine was passed an invalid datatype id."
E_AD2005_BAD_DTLEN:NO_GE
"ADF routine found DB_DATA_VALUE with an invalid length."
E_AD2006_BAD_DTUSRLEN:NO_GE
"ADF routine encountered user-declared datatype length to be wrong."
E_AD2007_DT_IS_FIXLEN:NO_GE
"Datatype is fixed length, yet a length specification was given."
E_AD2008_DT_IS_VARLEN:NO_GE
"Datatype is variable length, but no length specification was given."
E_AD2009_NOCOERCION:NO_GE
"No coercion is available for datatypes given."
E_AD200A_NOCOPYCOERCION:NO_GE
"No copy-coercion is available for datatypes given."
E_AD200B_BAD_PREC:E_GE8CA0_SYSTEM_LIMIT
"Invalid precision specified for DECIMAL value ... 1 <= precision <= 31."
E_AD200C_BAD_SCALE:E_GE8CA0_SYSTEM_LIMIT
"Invalid scale specified for DECIMAL value ... 0 <= scale <= precision."
E_AD200D_BAD_BASE_DTID:E_GE9D17_DATAEX_TYPEINV
"Illegal or unknown base datatype ID."
E_AD2010_BAD_FIID:NO_GE
"ADF found a bad function instance id."
E_AD2020_BAD_LENSPEC:NO_GE
"ADF does not recognize the length specification code %0d."
E_AD2021_BAD_DT_FOR_PRINT:NO_GE
"Illegal datatype for the PRINT style lenspecs."
E_AD2022_UNKNOWN_LEN:NO_GE
"< Expected internal error code ... No message needed. >\n\
 The length of the result can not be determined because one (or more) of the\
 input lengths that are required to determine the result length is\
 ADE_LEN_UNKNOWN."
E_AD2030_LIKE_ONLY_FOR_SQL:NO_GE
"The LIKE/NOTLIKE operators are only valid in SQL."
E_AD2040_INCONSISTENT_TPL_CNT:NO_GE
"The number of T/P/L's for base datatype ID `%0d' listed in the datatypes\
 table differs from the number generated by that datatype's routine." 
E_AD2041_TPL_ARRAY_TOO_SMALL:NO_GE
"The T/P/L array passed in was not big enough to hold the number of T/P/L's\
 that needed to be generated."
E_AD2042_MEMALLOC_FAIL:NO_GE
"The memory allocation routine supplied by GCA failed with a status of %0x."
E_AD2043_ATOMIC_TPL:NO_GE
"The T/P/L triad being decomposed is already an `ATOMIC' element.  No further\
 decomposition is possible."
E_AD2050_NO_COMVEC_FUNC:NO_GE
"Missing function pointer in the datatypes table for function `%0c()',\
 datatype ID `%1d'."
E_AD2060_BAD_DT_COUNT:NO_GE
"Some ADF function was told that it was receiving some number of datatypes\
 other than 0, 1, or 2."
/*%
** Parameters:
**      None
**
** Description:
**	Currently INGRES functions are limited to 0, 1, or 2, parameters.  This
**	error indicates that some facility called ADF with a number input
**	datatypes which was none of these.
**
** System Status:
**	This is an internal error.  The current query will most likely be
**	aborted.
**
** User Action:
**	None
*/
E_AD2061_BAD_OP_COUNT:NO_GE
"Although this operation does exist and does have function instances associated\
 with it, none of these function instances had the number of input datatypes\
 specified."
/*%
** Parameters:
**      None
**
** Description:
**	Some facility called ADF for datatype resolution, but none of ADF's
**	function instance entries matched the requested operation id and had the
**	specified number of input datatypes.
**
** System Status:
**	The current query will most likely be aborted.
**
** User Action:
**	Ensure that the function you are using accepts the number of inputs you
**	are attempting to pass to it.
*/
E_AD2062_NO_FUNC_FOUND:NO_GE
"No function instances exist which can be used for the operation id and\
 datatypes given to this ADF routine.  Make certain you have used datatypes\
 which are correct for this operation."
/*%
** Parameters:
**      None
**
** Description:
**	This operation is known to ADF, but the datatype(s) given to ADF are not
**	coercible to the type required to use any entries in ADF's function
**	instance table.
**
** System Status:
**	The current query will most likely be aborted.
**
** User Action:
**	Look in an INGRES reference manual for information on which datatypes
**	can be used for this operation.
*/
E_AD2063_FUNC_AMBIGUOUS:NO_GE
"Two or more function instances required the same number of coercions and had\
 the same composite rankings when attempting to resolve an operation in the\
 adi_resolve routine."
/*%
** Parameters:
**      None
**
** Description:
**	ADF's datatype resolution rules have been unable to decide on a function
**	instance from its internal tables.  After using several tie-breaking
**	strategies, several equally suitable candidate function instances
**	remained.
**
** System Status:
**	The current query will most likely be aborted.
**
** User Action:
**	Explicitly convert your parameters to the input types desired.
*/
E_AD2070_NO_EMBED_ALLOWED:E_GE7918_SYNTAX_ERROR
"The column specification given contains an embedded number in the datatype's\
 name.  This is not allowed for this datatype."
/*%
** Parameters:
**      None
**
** Description:
**	A column specification with an embedded number was detected where it
**	is not allowed.  For instance, specifying "char10" will generate this
**	error because you must use parentheses when specifying the length for
**	the `char' datatype.  Also, specifying "date10" will generate this
**	error because you are not allowed to specify a length for the `date'
**	datatype.
**
** System Status:
**	The current action will most likely be aborted.
**
** User Action:
**	Use the correct syntax when specifying columns with this datatype.
*/
E_AD2071_NO_PAREN_ALLOWED:E_GE7918_SYNTAX_ERROR
"The column specification given had parenthesized numbers after the datatype's\
 name.  This is not allowed for this datatype."
/*%
** Parameters:
**      None
**
** Description:
**	A column specification with parenthesized numbers following the
**	datatype name was detected where it is not allowed.  For instance,
**	specifying "c(10)" will generate this error because you must not use
**	parentheses when specifying the length for the `c' datatype.  Also,
**	specifying "date(10)" will generate this error because you are not
**	allowed to specify a length for the `date' datatype.
**
** System Status:
**	The current action will most likely be aborted.
**
** User Action:
**	Use the correct syntax when specifying columns with this datatype.
*/
E_AD2072_NO_PAREN_AND_EMBED:E_GE7918_SYNTAX_ERROR
"The column specification given contains both an embedded number in the\
 datatype's name and parenthesized numbers following it.  This is never\
 allowed."
/*%
** Parameters:
**      None
**
** Description:
**	A column specification with an embedded number and parenthesized
**	numbers; this is not allowed.  For instance, specifying "char10(10)"
**	will generate this error.  You are allowed to specify the length for
**	a datatype in only one way.
**
** System Status:
**	The current action will most likely be aborted.
**
** User Action:
**	Use the correct syntax when specifying columns with this datatype.
*/
E_AD2073_BAD_USER_PREC:E_GE8CA0_SYSTEM_LIMIT
"Precision %0d is out of range.  Please specify a precision in the range\
 1 <= precision <= 31."
/*%
** Parameters:
**      %0d	-  Precision which user is attempting to give this column
**
** Description:
**	The precision specified for a column with the `decimal' datatype is
**	invalid.  For example, attempting to create a table with with a column
**	specified as "decimal(99)" will generate this error since INGRES
**	supports a maximum precision of 31 for the `decimal' datatype.
**
** System Status:
**	The current action will most likely be aborted.
**
** User Action:
**	Specify a precision in the proper range.
*/
E_AD2074_BAD_USER_SCALE:E_GE8CA0_SYSTEM_LIMIT
"Scale %0d is invalid.  You must specify a scale which is 0 or larger AND less\
 than or equal to the precision you have specified.  For this column you have\
 specified a precision of %1d, so your scale must fall into the range\
 0 <= scale <= %1d."
/*%
** Parameters:
**      %0d	-  Scale which user is attempting to give this column
**	%1d	-  Precision specified for this column
**
** Description:
**	The scale specified for a column with the `decimal' datatype is
**	invalid.  For example, attempting to create a table with with a column
**	specified as "decimal(10,11)" will generate this error since the scale
**	(the second number in the parentheses) must not be larger than the
**	precision (the first number in the parentheses).  The scale must always
**	be in the range 0 <= scale <= precision.
**
** System Status:
**	The current action will most likely be aborted.
**
** User Action:
**	Specify a scale in the proper range.
*/
E_AD2080_IFTRUE_NEEDS_INT:NO_GE
"The datatype of the ii_iftrue() function's first argument must be INTEGER."
/*%
** Parameters:
**	None
**
** Description:
**	This error cannot be incurred directly as a result of a user's query
**	using the ii_iftrue() function; use of a non-integral first parameter
**	by the user would have been caught by the datatype resolution algorithm.
**
**	Instead this is an internal problem probably caused by OPF, most
**	likely as a result of an outer join in some query.
**
** System Status:
**	The current query will be aborted.
**
** User Action:
**	Contact your INGRES system manager.
*/
E_AD2085_LOCATE_NEEDS_STR:E_GE80E8_LOGICAL_ERROR
"The locate() function does not accept non-string parameters.  Explicity\
 convert parameters with non-string types to parameters with string types."
/*%
** Parameters:
**	None
**
** Description:
**	The locate() function takes two arguments, both of which must have
**	a string datatype (a string datatype is one of: C, TEXT, CHAR, or
**	VARCHAR).  You have given it some parameter whose datatype is none
**	of these.
**
** System Status:
**	The current query will be aborted.
**
** User Action:
**	Explicitly convert any non-string datatypes to string datatypes.  For
**	example, if you are trying to find the first occurrence of x in the
**	string '32123' where x is an integer column, you cannot use
**	locate('32123', x) because the second parameter is an non-string type.
**	Instead use locate('32123', varchar(x)).
*/
E_AD2100_NULL_RESTAB_PTR:NO_GE
"The callback function for the resolve_table function has not been set prior
 to its use: the callback address is NULL.  This is an Ingres internal error."
/*%
** Parameters:
**	None
**
** Description:
**	Upon trying to execute the resolve_table function, ADF discovered
**	that the address to the callback routine for this function was NULL.
**	This usually means that adg_add_fexi() was never called to set the
**	address prior to the invocation of resolve_table.  This is an Ingres
**	internal error.
**
** System Status:
**	The query involving the resolve_table function will be aborted.
**
** User Action:
**	Call your company's INGRES technical contact with this information.
*/
E_AD2101_RESTAB_FCN_ERROR:NO_GE
"The callback function for the resolve_table function returned an error.  This
 should never happen since resolve_table always has some value defined for all
 inputs.  This is an Ingres internal error."
/*%
** Parameters:
**	None
**
** Description:
**	Upon executing the callback function for resolve_table, ADF received
**	a status indicating that an error occurred.  This is abnormal and is
**	considered an Ingres internal error.
**
** System Status:
**	The query involving the resolve_table function will be aborted.
**
** User Action:
**	Call your company's INGRES technical contact with this information.
*/
E_AD3001_DTS_NOT_SAME:NO_GE
"Some ADF function was given two datatypes that were expected to be the same\
 but were not.  The most likely routines would be adc_keybld(), adc_compare(),\
 adc_minmaxdv(), or the ADE_KEYBLD CX instruction."
E_AD3002_BAD_KEYOP:NO_GE
"KEYBLD (either the adc_keybld() routine or the ADE_KEYBLD instruction) was\
 passed an unknown key operator."
E_AD3003_DLS_NOT_SAME:NO_GE
"KEYBLD (either the adc_keybld() routine or the ADE_KEYBLD instruction) was\
 passed low and high data values with inconsistent data lengths."
E_AD3004_ILLEGAL_CONVERSION:NO_GE
""	/* Dead error code */
E_AD3005_BAD_EQ_DTID:NO_GE
""	/* Dead error code */
E_AD3006_BAD_EQ_DTLEN:NO_GE
""	/* Dead error code */
E_AD3007_BAD_DS_DTID:NO_GE
""	/* Dead error code */
E_AD3008_BAD_DS_DTLEN:NO_GE
""	/* Dead error code */
E_AD3009_BAD_HP_DTID:NO_GE
""	/* Dead error code */
E_AD3010_BAD_HP_DTLEN:NO_GE
"Illegal data length for a hash-prep value."
E_AD3011_BAD_HG_DTID:NO_GE
"Illegal datatype for a histogram element."
E_AD3012_BAD_HG_DTLEN:NO_GE
"Illegal data length for a histogram element."
E_AD4001_FIID_IS_AG:NO_GE
"Trying to use an aggregate function instance as a non-aggregate."
E_AD4002_FIID_IS_NOT_AG:NO_GE
"Trying to use a non-aggregate function instance as an aggregate."
E_AD4003_AG_WORKSPACE_TOO_SHORT:NO_GE
"Aggregate needs more work space ... ADF_AG_STRUCT.adf_agwork.db_length needs\
 to be >= ADI_FI_DESC.adi_agwsdv_len."
E_AD4004_BAD_AG_DTID:E_GE9D17_DATAEX_TYPEINV
"Illegal datatype for result of an aggregate."
E_AD4005_NEG_AG_COUNT:NO_GE
"A negative number of values have been aggregated!?  How can this be?"
E_AD5001_BAD_STRING_TYPE:E_GE9D17_DATAEX_TYPEINV
"Datatype is not a string datatype."
E_AD5002_BAD_NUMBER_TYPE:E_GE9D17_DATAEX_TYPEINV
"Datatype is not a numeric datatype."
E_AD5003_BAD_CVTONUM:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5004_OVER_MAXTUP:E_GE8CA0_SYSTEM_LIMIT
"Length of a data value exceeds maximum tuple length."
E_AD5005_BAD_DI_FILENAME:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5020_BADCH_MNY:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5021_MNY_SIGN:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5022_DECPT_MNY:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5031_MAXMNY_OVFL:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5032_MINMNY_OVFL:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5050_DATEADD:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5051_DATESUB:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5052_DATEVALID:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5053_DATEYEAR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5054_DATEMONTH:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5055_DATEDAY:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5056_DATETIME:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5058_DATEBADCHAR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5059_DATEAMPM:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD505A_DATEYROVFLO:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD505B_DATEYR:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD505C_DOWINVALID:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD505D_DATEABS:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD505E_NOABSDATES:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD505F_DATEINTERVAL:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5060_DATEFMT:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD5061_DGMT_ON_INTERVAL:NO_GE
"The date_gmt function only works on absolute dates."
/*%
** Parameters:
**      None
**
** System Status:
**	A conversion from a some datatype to either a "table_key" or a
**	"object_key" was either explicitly (ie. table_key() or object_key()
**	functions) or implicitly (conversion atttempted automatically by the
**	dbms) attempted.  This conversion only works if the lengths of the
**	datatype is the same.  The length of the "table_key" datatype is 8 
**	bytes, while the length of the "object_key" datatype is 16 bytes.
**	The conversion attempted failed because the length of the source
**	datatype and the destination datatype were different.
**
** User Action:
**	If attempting to convert strings to logical key datatype make sure
**	to specify string of the appropriate length (8,16).
*/
E_AD5080_LOGKEY_BAD_CVT_LEN:NO_GE
"The conversion from the datatype specified (length %1d bytes) to logical key 
type (ie. table_key or object_key with length %0d bytes) failed because the 
lengths of the two datatypes were not the same."
E_AD5500_BAD_SEG:NO_GE
"Unknown CX segment."
E_AD5501_BAD_SEG_FOR_ICODE:NO_GE
"The instruction being compiled cannot be placed in the requested CX segment."
E_AD5502_WRONG_NUM_OPRS:NO_GE
"Wrong number of operands for a function instance.\n\
 It is possible that there have been changes to ADF's function instance table,\
 that make some views, integrities, or permits no longer valid."
E_AD5503_BAD_DTID_FOR_FIID:NO_GE
"Datatype of operand (input or output) for a function instance is not correct. \
 It is possible that there has been a change to the datatypes accepted or\
 returned for this function instance in ADF's function instance table, thus\
 making a view, integrity, or permit no longer valid."
E_AD5504_BAD_RESULT_LEN:NO_GE
"Length of result data for a function instance is not correct.\n\
 It is possible that there has been a change to the lenspec for this function\
 instance in ADF's function instance table, thus making a view, integrity, or\
 permit no longer valid."
E_AD5505_UNALIGNED:NO_GE
"< Expected internal error code ... No message needed. >\n\
 ADE has detected an attempt to compile an unaligned piece of data."
E_AD5506_NO_SPACE:NO_GE
"< Expected internal error code ... No message needed. >\n\
 The CX is out of space; caller must make it bigger."
E_AD5507_BAD_DTID_FOR_KEYBLD:NO_GE
"The output operand's length for the ADE_KEYBLD instruction must be `int'."
E_AD5508_BAD_DTLEN_FOR_KEYBLD:NO_GE
"The output operand's length for the ADE_KEYBLD instruction must be 2."
E_AD5509_BAD_RANGEKEY_FLAG:NO_GE
"Range-key flag supplied when compiling the CX special instruction,\
 ADE_KEYBLD, is not recognized by the ade_instr_gen() routine."
E_AD550A_RANGE_FAILURE:NO_GE
"< Expected internal error code ... No message needed. >\n\
 The ADE_KEYBLD instruction has detected a RANGE-FAILURE; that is, when it was\
 compiled it was told to accept only RANGE-type keys, and the key found is not,\
 or it was told to accept only non-RANGE-type keys, and the key found was."
E_AD550B_TOO_FEW_BASES:NO_GE
"The CX header states that there should be more base addresses that the caller\
 has supplied."
E_AD550C_COMP_NOT_IN_PROG:NO_GE
"INGRES is attempting to compile an instruction or constant into a CX without\
 first calling ade_bgn_comp() to initialize the CX."
E_AD550D_WRONG_CX_VERSION:NO_GE
"This version of ADF cannot execute the supplied CX due to a version mis-match."
E_AD550E_TOO_MANY_VLTS:NO_GE
"The maximum number of VLTs (Variable Length Temporaries) has been exceeded.\n\
 This query cannot be executed."
E_AD550F_BAD_CX_REQUEST:NO_GE
"Unknown request code detected by ade_cxinfo() routine: %0d."
E_AD5510_BAD_DTID_FOR_ESCAPE:NO_GE
"The output operand's type for the ADE_ESC_CHAR instruction must be `char'."
E_AD5511_BAD_DTLEN_FOR_ESCAPE:NO_GE
"The output operand's length for the ADE_ESC_CHAR instruction must be 1."
E_AD6000_BAD_MATH_ARG:NO_GE
""	/* Maps to user error message in ERUSF.MSG */
E_AD6001_BAD_MATHOPT:E_GE9D24_DATAEX_OTHER
"Unknown math exception option found in ADF's session control block."
E_AD7000_BAD_ADP_INFO:NO_GE
"Error calling peripheral datatype extension routine to get information."
/*%
** Parameters:
**      None
**
** System Status:
**      The system is unable to determine the parameters for use in obtaining
**	peripheral datatypes.  Peripheral datatype processing will not operate.
**	All non-peripheral datatypes should operate normally.
**
** User Action:
**      Call the site technical contact.  This is an internal error, and
**	probably requires vendor support.
*/
E_AD7001_ADP_NONEXT:NO_GE
"No more peripheral datatype segments found."
/*%
** Parameters:
**      None.
**
** System Status:
**      This is a normal occurrence.  This error message should not be seen by
**	users.
**
** User Action:
**      If this error appears, then contact the site technical representative.
**	This is an internal error, and probably requires vendor support.
*/
E_AD7002_ADP_BAD_GET:NO_GE
"Error getting the next segment of a peripheral datatype."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was attempting to fetch the component parts of a peripheral
**	datatype.  In so doing, an error was encountered.  Other, more detailed
**	errors will be found in the error log associated with this error.
**
** User Action:
**      This is an internal error.  Contact the site technical representative.
**	Vendor support is probably required, depending upon the content of the
**	other, more detailed, messages associated with this error.
*/
E_AD7003_ADP_BAD_RECON:NO_GE
"An error occurred while trying to reconstitute a peripheral datatype."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system encountered an error (which should be found in the log file)
**	while attempting to reconstitute a peripheral datatype from its
**	component parts.  The system should remain operational;  the query
**	in which this error occurred will be aborted.
**
** User Action:
**      Examine the other errors in the error log file.  Other, more detailed,
**	messages should be found which will indicate the actual cause of the
**	problem.
*/
E_AD7004_BAD_BLOB:NO_GE
"A peripheral datatype (BLOB) was found whose coupon length did not agree with\
 the amount of data found."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was attempting to reconstitute a peripheral datatype (BLOB).
**	While doing so, the amount of data found did not agree with the amount
**	required.  Therefore, the peripheral datatype was declared inconsistent.
**
**	The system will continue to operate normally;  however, this particular
**	data element will not be available.
**
** User Action:
**      Examine any other errors in the log file.  In that this is an internal
**	error, contact with the product vendor is probably necessary.
*/
E_AD7005_BAD_POP:NO_GE
"An invalid peripheral operation was requested."
/*%
** Parameters:
**
** System Status:
**      The system was attempting to operate on a peripheral datatype.  In so
**	doing, a call was made requesting a peripheral operation which was
**	invalid.
**
**	The system will continue to operate normally.
**
** User Action:
**      This is an internal error.  Please contact site technical contact with
**	information about the query in use and the runtime environment.
**	Information about the datatypes in use at the time (especially any user
**	defined types) will be very helpful in resolving the problem.
*/
E_AD7006_ADP_PUT_PUT_ERROR:NO_GE
"An error was encountered in attempting to perform the peripheral put operation."
/*%
** Parameters:
**
** System Status:
**      The system was attempting to perform a PUT operation for a peripheral
**	datatype.  In so doing, an error PUTting the underlying segment was
**	encountered.
**
**	The system will continue to operate normally.
**
** User Action:
**      The error log should be examined to determine more detail about this
**	error.  Other errors should exist in the error log in support of this
**	high level error.  Follow the action recommended for these other, more
**	detailed errors.
*/
E_AD7007_ADP_GET_ERROR:NO_GE
"An error was encountered performing the ADP GET operation on a peripheral datatype."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system encountered an error in fetching a portion of a peripheral
**	datatype.
**
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** User Action:
**      Examine the error log for other, more detailed errors in support of this
**	high level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD7008_ADP_DELETE_FAILURE:NO_GE
"An error was encountered performing an ADP_DELETE operation on a peripheral datatype."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system encountered an error in deleting a portion of a peripheral
**	datatype.
**
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** User Action:
**      Examine the error log for other, more detailed errors in support of this
**	high level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD7009_ADP_MOVE_FAILURE:NO_GE
"An error was encountered performing an ADP_MOVE operation on a peripheral datatype."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system encountered an error in moving a peripheral datatype.
**
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** User Action:
**      Examine the error log for other, more detailed errors in support of this
**	high level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD700A_ADP_REPLACE_FAILURE:NO_GE
"An error was encountered performing an ADP_REPLACE operation on a peripheral datatype."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system encountered an error in replacing a peripheral
**	datatype.
**
**	Other portions of the system will continue to operate normally.  The
**	query which encountered this error will be aborted.
**
** User Action:
**      Examine the error log for other, more detailed errors in support of this
**	high level message.  Follow the actions recommended for these more
**	detailed messages.
*/
E_AD700B_BAD_POP_CB:NO_GE
"An invalid peripheral operations control block was passed."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was attempting to perform a peripheral operation of some
**	sort.  However, the control block used to request the operation was
**	invalid.
**
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** User Action:
**      This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information and
**	report the problem to the site technical contact.  Contact with the
**	product vendor is probably necessary.
*/
E_AD700C_BAD_POP_UA:NO_GE
"Internal synchronization with peripheral operation's user argument has been lost."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was attempting to perform a peripheral operation of some
**	sort.  However, the control block used to request the operation was
**	invalid -- the user argument field was corrupted.
**
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** User Action:
**      This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information and
**	report the problem to the site technical contact.  Contact with the
**	product vendor is probably necessary.
*/
E_AD700D_ADP_NO_COUPON:NO_GE
"No coupon value was passed as required."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was attempting to perform a peripheral operation of some
**	sort.  However, the control block used to request the operation was
**	invalid -- the coupon value was not a coupon.
**
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** User Action:
**      This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information and
**	report the problem to the site technical contact.  Contact with the
**	product vendor is probably necessary.
*/
E_AD700E_ADP_BAD_COUPON:NO_GE
"An invalid coupon value was passed to the DMPE routines."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was attempting to perform a peripheral operation of some
**	sort.  However, the control block used to request the operation was
**	invalid -- the coupon was invalid.
**
**	The system will continue to operate normally.  The query which
**	encountered this error will be aborted.
**
** User Action:
**      This is an internal error.  Please gather information about the query,
**	datatypes in use, tables in use, and other environmental information and
**	report the problem to the site technical contact.  Contact with the
**	product vendor is probably necessary.
*/
E_AD8000_OP_COUNT_SPEC:NO_GE
"Add datatype control block specified a nonzero number\
 of operators to add;  no operator descriptions were provided."
/*%
** Parameters:
**      None
**
** System Status:
**      Server will startup with no user defined datatypes added (unless
**	otherwise specified in the datatype control block).  Only vendor
**	supplied datatypes (the defaults) will be available.
**
** User Action:
**      Correct the problem.  Either the add_fo_dfn field should be set, or the
**	add_fo_cnt field should be set to zero (0).
*/
E_AD8001_OBJECT_TYPE:NO_GE
"User Defined Datatypes:  The dtd_object_type,\
fod_object_type, or fid_object_type field was not set to match the object \
being defined."
/*%
** Parameters:
**      None
**
** System Status:
**      The program will (by default) startup with no user defined datatypes
**	added.  Only the system default datatypes will be available.
**
** User Action:
**      Correct the problem.  The field in question should be set to either
**	ADD_O_DATATYPE, ADD_O_OPERATOR, or ADD_O_FUNCTION_INSTANCE respectively.
*/
E_AD8002_OP_OPID:NO_GE
"User Defined Datatypes:  the fod_id field was invalid."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
** User Action:
**      Correct the field value.  fod_id must be greater than or equal to the
**	ADD_OPSTART constant provided in iiadd.h
*/
E_AD8003_OP_OPTYPE:NO_GE
"User Defined Datatypes:  The operation type was invalid."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
** User Action:
**      Correct the field.  The fod_optype field must correspond to one of the
**	values provided in the header file.
*/
E_AD8004_OP_COUNT_WRONG:NO_GE
"User Defined Datatypes:  The number of operators added\
 does not agree with the number requested."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
** User Action:
**      Examine the operators (and any previous errors).  If these are all
**	correct, correct the add_op_cnt field.
*/
E_AD8005_DT_COUNT_SPEC:NO_GE
"User Defined Datatypes:  A count of datatypes to \
add was specified (nonzero), but no datatype definitions were included."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
** User Action:
**      Correct the error.  Either provide the datatype definitions or set the
**	add_dt_cnt field to zero.
*/
E_AD8006_DT_DATATYPE_ID:NO_GE
"User Defined Datatypes:  The dtd_id field contained \
an illegal value."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
** User Action:
**      Correct the value.  The system has found a value in the dtd_id field
**	which was not acceptable.  Values in this field must fall in the range
**	specified by ADD_LOW_USER and ADD_HIGH_USER.
*/
E_AD8007_DT_FUNCTION_ADDRESS:NO_GE
"User Defined Datatypes:  A required function \
address was missing."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system has found a missing or invalid address for a required
**	datatype function.
**
** User Action:
**	Correct the problem.  All required functions must be specified with
**	valid addresses.
*/
E_AD8008_DT_COUNT_WRONG:NO_GE
"User Defined Datatypes:  Fewer datatypes were added \
than requested."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system has added fewer datatype definitions than were requested.
** User Action:
**      Correct the problem.  Either the datatype definitions or the count field
**	(add_dt_cnt) must be corrected.
*/
E_AD8009_FI_SORT_ORDER:NO_GE
"User Defined Datatypes:  The function instance \
definitions were not provided in the correct order."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system has determined that the sort order for the proposed function
**	instances is incorrect.
**	
** User Action:
**      Correct the sort order.  The function instances must be provided in
**	order of increasing operation types, and in order of increasing operator
**	ids within a given type.
*/
E_AD800A_FI_COUNT_SPEC:NO_GE
"User Defined Datatypes:  The add_fi_cnt specified \
function instances to be defined, but no definitions were found."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system was requested to define a number of function instances but no
**	function instance definitions were found.
**	
** User Action:
**      Correct the control blocks.  Either set the count field (add_fi_cnt) to
**	zero, or provide the appropriate definitions.
*/
E_AD800B_FI_RLTYPE:NO_GE
"User Defined Datatypes:  The fid_rltype field was invalid."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system found an invalid value in the fid_rltype field.
**	
** User Action:
**      Correct the field.  The only valid values are provided in the iiadd.h
**	header file.
*/
E_AD800C_FI_INSTANCE_ID:NO_GE
"User Defined Datatypes:  The fid_id field contained \
an invalid value."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system found an invalid value in the fid_id field.
**
** User Action:
**	Correct the field.  All values must be >= the ADD_FISTART
**	value provided in the iiadd.h header file.
*/
E_AD800D_FI_OPTYPE:NO_GE
"User Defined Datatypes:  The value of the fid_optype field \
was invalid."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**      The system found invalid operation type field in a function instance
**	definition (fid_optype).
**
** User Action:
**      Correct the value.  The only legal values here match those provided for
**	the fod_optype field in iiadd.h.
*/
E_AD800E_FI_NUMARGS:NO_GE
"User Defined Datatypes:  The number of arguments is \
not compatible with the function instance operation type."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**	
**      The system found an incompatible number of arguments.  Various operation
**	types require specific argument counts.  Please check the documentation
**	to determine the correct range of numbers.
**
** User Action:
**      Correct the value.
*/
E_AD800F_FI_NO_COMPLEMENT:NO_GE
"User Defined Datatypes:  Comparison function \
instance found with no associated complement."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**	
**      The system has found a comparison function instance with either no
**	complement or one which was invalid.
**
** User Action:
**      Correct the instance by providing a valid complement function instance.
*/
E_AD8010_FI_ARG_SPEC:NO_GE
"User Defined Datatypes:  A non-zero argument count \
was provided without corresponding argument definitions."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**      The system found a function instance with a non-zero argument count.
**	However, the argument definition specification was missing or invalid.
**
** User Action:
**      Correct either the argument count or definition field.
*/
E_AD8011_FI_DUPLICATE:NO_GE
"User Defined Datatypes:  A duplication function \
instance definition was found."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system found a function instance definition for a function instance
**	id which was already in use.
**
** User Action:
**      Correct the value for the fid_id.  All function instance identification
**	numbers must be unique.
*/
E_AD8012_UD_ADDRESS:NO_GE
"User Defined Datatypes:  The system found an invalid \
address in a user defined datatype definition."
/*%
** Parameters:
**      None.
**
** System Status:
**      Program will startup (by default) with no user defined datatypes.  Only
**	system defaults will be available.
**
**	The system found (either via inspection or because of an address fault)
**	that a user provided address was invalid.
**
** User Action:
**      Please ensure that all values passed to the system are valid.
*/
E_AD8013_FI_ROUTINE:NO_GE
"User Defined Datatypes: The system found a function \
instance definition which had no corresponding implementing routine."
/*%
** Parameters:
**      None.
**
** System Status:
**      The program will startup (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
**	The system found, by inspection, that a function instance was defined
**	with no routine specified to implement it.  All function instance
**	definitions must provide a routine specification.
**
** User Action:
**      Correct the definition.
*/
E_AD8014_DT_NAME:NO_GE
"User Defined Datatypes:  Datatype name is invalid."
/*%
** Parameters:
**      None
**
** System Status:
**      The program will startup (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
**	The system found, by inspection, that a datatype name was specified
**	which is not a valid INGRES name.  User defined datatype names must
**	follow INGRES naming conventions.
**
** User Action:
**      Correct the name so that it is a valid INGRES name.
*/
E_AD8015_OP_NAME:NO_GE
"User Defined Datatypes:  Function or Operator name is invalid."
/*%
** Parameters:
**      None
**
** System Status:
**      The program will startup (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
**	The system found, by inspection, that a function or operator name was
**	not a valid INGRES name.  User defined datatype names must follow INGRES
**	naming conventions.
**
** User Action:
**      Correct the name so that it is a valid INGRES name.
*/
E_AD8016_OP_DUPLICATE:NO_GE
"User Defined Datatypes:  Operator identifer (fod_id) field already exists."
/*%
** Parameters:
**      None
**
** System Status:
**      The program will startup (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
**	The system found, by inspection, that a function or operator id
**	duplicates another being defined.  All function or operators must have
**	unique identifiers.
**
** User Action:
**      Provide a unique identifer for each function or operator.
*/
E_AD8017_INVALID_RESULT_TYPE:NO_GE
"User Defined Datatypes:  A function instance was found with a result datatype\
 identifier which is unknown."
/*%
** Parameters:
**      None.
**
** System Status:
**      The program will startup (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** User Action:
**      Make sure that all datatype idenifiers in use are known by the system.
**	Verify that the datatype count field in use correctly matches the number
**	of new datatypes being provided to the system.
*/
E_AD8018_INVALID_PARAM_TYPE:NO_GE
"User Defined Datatypes:  A function instance was found with the type \
specified for one of its parameters which is unknown."
/*%
** Parameters:
**      None
**
** System Status:
**      The program will startup (by default) with no user defined datatypes.
**	Only system defaults will be available.
**
** User Action:
**      Make sure that all datatype identifiers in use are known by the system.
**	Verify that the datatype count field in use correctly matches the number
**	of datatypes being provided to the system.
*/
E_AD8019_OPTYPE_MISMATCH:NO_GE
"User Defined Datatypes:  The operation type specified for a function instance \
does not match that specified for the corresponding operation."
/*%
** Parameters:
**      None.
**
** System Status:
**      The program will startup (by default) with no user defined datatypes
**	available.  Only system defaults will be available.
**
** User Action:
**      Make sure that function instance operation types agree with the
**	corresponding operation definition.
*/
E_AD8999_FUNC_NOT_IMPLEMENTED:E_GEA0F0_SQL_STMT_INV
"The function `%0c()' is not currently implemented."
E_AD9000_BAD_PARAM:NO_GE
"Invalid parameter(s) passed to adg..._size(), _startup(), _sz_augment(),\
or _augment()."
E_AD9001_BAD_CALL_SEQUENCE:NO_GE
"Adg_sz_augment() or Adg_augment() was called before ADF had been initialized."
E_AD9002_AUGMENT_SIZE:NO_GE
"Size of control block passed to adg_augment() is too small."
E_AD9003_TABLE_INIT:NO_GE
"Error initializing ADF's internal tables."
E_AD9999_INTERNAL_ERROR:NO_GE
"An ADF internal coding error has been detected."

/*
**    Message file for Facility code: _CL
*/


#define _CL_CLASS   1


E_CL0301_CK_BAD_PARAM:NO_GE
"Bad parameter passed to a CK routine"
E_CL0302_CK_SPAWN:E_GE94D4_HOST_ERROR
"Could not spawn a subprocess to perfrom backup"
E_CL0303_CK_BACKUP:E_GE94D4_HOST_ERROR
"Unexpected backup error received from operating system"
E_CL0304_CK_CMKRNL:E_GE94D4_HOST_ERROR
"Could not enter change mode to kernel to perform backup"
E_CL0305_CK_NOTFOUND:NO_GE
"Checkpoint object could not be found."
E_CL0306_CK_BADDELETE:E_GE94D4_HOST_ERROR
"Error deleting file or directory"
E_CL0307_CK_BADPATH:NO_GE
"The path specified was in error."
E_CL0308_CK_ENDFILE:NO_GE
"End of file"
E_CL0309_CK_BADLIST:NO_GE
"Error listing objects in a directory"
E_CL030A_CK_FILENOTFOUND:NO_GE
"The file specified could not be found"
E_CL030B_CK_DIRNOTFOUND:NO_GE
"The checkpoint directory could not be found"
E_CL030C_CK_BADDIR:NO_GE
"Error verifying directory"
E_CL030D_CK_EXISTS:NO_GE
"The object specified already exists"
E_CL0501_CV_SYNTAX:NO_GE
"Bad syntax in ascii form of number"
E_CL0502_CV_UNDERFLOW:E_GE9D21_DATAEX_FXPUNF
"Number underflow in the conversion"
E_CL0503_CV_OVERFLOW:E_GE9D1C_DATAEX_FIXOVR
"Number overflow in the conversion"
E_CL0504_CV_TRUNCATE:E_GE9D09_DATAEX_TRUNC
"Output too wide for space passed in"
E_CL0601_DI_BADPARAM:NO_GE
"Bad parameter(s) passed to routine"
E_CL0602_DI_BADFILE:NO_GE
"Illegal file control block pointer"
E_CL0603_DI_ENDFILE:NO_GE
"End of file"
E_CL0604_DI_BADOPEN:E_GE94D4_HOST_ERROR
"Error trying to open file"
E_CL0605_DI_BADCLOSE:E_GE94D4_HOST_ERROR
"Error trying to close file"
E_CL0606_DI_BADWRITE:E_GE94D4_HOST_ERROR
"Error writing page to disk"
E_CL0607_DI_BADREAD:E_GE94D4_HOST_ERROR
"Error reading page from disk"
E_CL0608_DI_BADEXTEND:E_GE94D4_HOST_ERROR
"Error allocating disk space"
E_CL0609_DI_BADDIR:E_GE94D4_HOST_ERROR
"Error verifying directory"
E_CL060A_DI_BADINFO:E_GE94D4_HOST_ERROR
"Error seeking to page in file"
E_CL060B_DI_BADSENSE:E_GE94D4_HOST_ERROR
"Error reading file attributes"
E_CL060C_DI_BADCREATE:E_GE94D4_HOST_ERROR
"Error creating file or directory"
E_CL060D_DI_BADDELETE:E_GE94D4_HOST_ERROR
"Error deleting file or directory"
E_CL060E_DI_BADRNAME:E_GE94D4_HOST_ERROR
"Error renaming file"
E_CL060F_DI_EXCEED_LIMIT:E_GE94D4_HOST_ERROR
"File resource quota exceeded"
E_CL0610_DI_DIRNOTFOUND:NO_GE
"The directory pointed to path could not be found"
E_CL0611_DI_FILENOTFOUND:NO_GE
"The file specified could not be found"
E_CL0612_DI_EXISTS:NO_GE
"The object specified already exists"
E_CL0613_DI_BADLIST:NO_GE
"Error listing objects in a directory"
E_CL0701_DSN_RECUR:NO_GE
"Recursive call attempted"
E_CL0702_DSN_LANG:NO_GE
"Unsupported language specifier"
E_CL0703_DSN_ALIGN:NO_GE
"Illegal alignment constraint"
E_CL0704_DSN_VIS:NO_GE
"Illegal visibility level"
E_CL0705_DSN_UNKNOWN:NO_GE
"Unknown operation requested"
E_CL0801_DY_NOSTAB:NO_GE
"No symbol table generated"
E_CL0802_DY_NOEXE:NO_GE
"No executable built"
E_CL0803_DY_NOMEM:NO_GE
"Error allocating memory to load image"
E_CL0804_DY_NOREAD:NO_GE
"Error reading in image"
E_CL0805_DY_NOFILE:NO_GE
"Error locating image file"
E_CL0901_ER_NO_FILE:E_GE94D4_HOST_ERROR
"A II_SYSTEM:[INGRES.FILES]*.MNX file could not be opened"
E_CL0902_ER_NOT_FOUND:NO_GE
"No text found for message identifier"
E_CL0903_ER_BADPARAM:NO_GE
"Bad parameter"
E_CL0904_ER_BADREAD:E_GE94D4_HOST_ERROR
"Error reading the INGRES message file."
E_CL0905_ER_TOOSMALL:NO_GE
"Buffer smaller then size of the message name"
E_CL0906_ER_TRUNCATED:NO_GE
"Message text truncated because buffer was too small"
E_CL0907_ER_BADOPEN:E_GE9088_COMM_ERROR
"Can't open/create receive channel for error logger"
E_CL0908_ER_BADRECEIVE:E_GE9088_COMM_ERROR
"Error receiving error logger message"
E_CL0909_ER_BADSEND:E_GE9088_COMM_ERROR
"Error sending message to error logger"
E_CL090A_ER_DIRERR:E_GE9088_COMM_ERROR
"Error accessing error logger directory"
E_CL090B_ER_NOALLOC:E_GE9088_COMM_ERROR
"Error allocating space for error logger"
E_CL090C_ER_MSGOVER:E_GE9088_COMM_ERROR
"Error message overflow occurred sending message to error logger"
E_CL090D_ER_NOFREE:E_GE9088_COMM_ERROR
"Error freeing memory for error logger"
E_CL090E_ER_BADCLASS:E_GE9088_COMM_ERROR
"Invalid class associated with error message"
E_CL090F_ER_BADLANGUAGE:E_GE9088_COMM_ERROR
"Invalid language associated with error message"

/*
** Template for formatting UNIX system errors.
**
** Parameters:
**    0    errno
**    1    library or system call name, e.g. "open", "read"
**    2    sys_errlist[errno]
*/
E_CL0910_ER_UNIXERROR:E_GE94D4_HOST_ERROR
"%1c() failed with UNIX error %0d (%2c)"

/* Test message- internal CL error */
E_CL0911_ER_INTERR_TESTMSG:NO_GE
"Internal compatibility routine failed, parameter was \"%0c\"."
E_CL0912_ER_NO_AUDIT:E_GE9088_COMM_ERROR
"No connection exists to security audit process"

E_CL0A01_EXEXIT:E_GE9858_FATAL_ERROR
"Exiting program"
E_CL0A17_EXBADRETURN:NO_GE
"Invalid return from a signal. Valid returns are EXDECLARE, EXRESIGNAL, and
EXCONTINUE"
E_CL0AFF_EXDECLARE:NO_GE
"Unwind to declaring frame"
E_CL0E01_IN_BACKEND_NAME:E_GE8D68_CONFIG_ERROR
"INingres: Unable to start ingres -- unable to find name for backend"
E_CL0E02_IN_INTR_NAME:E_GE8D68_CONFIG_ERROR
"INingres: Unable to start ingres -- unable to find name for interrupt process"
E_CL0E03_IN_NO_DB_NAME:E_GE75B2_NOT_FOUND
"INingres: no database name specified"
E_CL0E04_IN_RD_UNDEF:NO_GE
"INread: receive pipe was not defined INingres did not complete normally"
E_CL0E05_IN_RD_SYSERR:NO_GE
"INread: unable to get complete PIPEBLK -- system error during read"
E_CL0E06_IN_RD_INTR:NO_GE
"INread: read call was interrupted"
E_CL0E07_IN_RD_PIPE_GONE:E_GE94D4_HOST_ERROR
"INread: read pipe is no longer accessible"
E_CL0E08_IN_RD_NOTWHOLE:E_GE94D4_HOST_ERROR
"INread: system read function did not write an entire pipe block"
E_CL0E09_IN_RD_CTLC:NO_GE
"INread: got control-C"
E_CL0E0A_IN_WR_UNDEF:E_GE94D4_HOST_ERROR
"INwrite: send pipe was not defined INingres did not complete normally"
E_CL0E0B_IN_WR_SYSERR:E_GE94D4_HOST_ERROR
"INwrite: unable to send complete PIPEBLK -- system error during write"
E_CL0E0C_IN_WR_INTR:NO_GE
"INwrite: write call was interrupted"
E_CL0E0D_IN_WR_PIPE_GONE:E_GE94D4_HOST_ERROR
"INwrite: write pipe is no longer accessible"
E_CL0E0E_IN_WR_NOTWHOLE:E_GE94D4_HOST_ERROR
"INwrite: system write function did not write an entire pipe block"
E_CL0E0F_IN_WR_CTLC:NO_GE
"INwrite: got control-C"
E_CL0E10_IN_NO_BACKEND:NO_GE
"INintrpt: there is no Ingres backend process to interrupt"
E_CL0E11_IN_INACTIVE:NO_GE
"INtimeout: timeout period exceeded"
E_CL0F01_LG_CANTINIT:E_GE94D4_HOST_ERROR
"Error trying to initialize log file"
E_CL0F02_LG_CANTOPEN:E_GE94D4_HOST_ERROR
"Error trying to open log file"
E_CL0F03_LG_CANTCLOSE:E_GE94D4_HOST_ERROR
"Error trying to close log file"
E_CL0F04_LG_BADPARAM:NO_GE
"Bad input parameter passed to LG routine"
E_CL0F05_LG_READERROR:E_GE94D4_HOST_ERROR
"Error reading from log file"
E_CL0F06_LG_BADHEADER:NO_GE
"Log file has bad header context"
E_CL0F07_LG_INITHEADER:NO_GE
"Error trying to initialize log file header context"
E_CL0F08_LG_ENDOFFILE:NO_GE
"Log logical end of file has been reached"
E_CL0F09_LG_OFFLINE:E_GE8D68_CONFIG_ERROR
"Logging system not running"
E_CL0F0A_LG_BADFORMAT:NO_GE
"Log file record has a bad format"
E_CL0F0B_LG_NOTLOADED:E_GE8D68_CONFIG_ERROR
"Logging system device has not been loaded"
E_CL0F0C_LG_UNEXPECTED:E_GE94D4_HOST_ERROR
"Logger received unexpected error from operating system"
E_CL0F0D_LG_CHK_SUM_ERROR:NO_GE
"Bad checksum for log page"
E_CL0F30_LG_NO_MASTER:E_GE8D68_CONFIG_ERROR
"Could not initialize the logging system for this process because the RCP
process is not running.
This indicates that either the RCP abnormally exited,
or that the server has attempted to connect to the logging system before the
RCP was brought up.
Shutdown all processes associated with this installation, and bring up the
installation from scratch using the supported startup tools, documented in
Installation and Operations guide."
/*
** Parameters:
**	none.
**
** Explanation:
**	Each installation contains a single process designated to be the
**	"master" of the logging and locking operations of that installation.
**	The process named "dmfrcp" is the usual master process.  If this
**	process exits abnormally without running certain cleanup routines
**	it may be possible for other processes to connect to the data
**	structures left about in shared memory, and get into a state where
**	they wait forever from a response from the non-existent "master"
**	process.
**
**	A check for the existence of the "master" is performed whenever a
**	process connects to the logging system.  If the "master" process
**	does not exist then this error is returned.
**
**	This error indicates that the dmfrcp process
**	has exited abnormally (causes include "kill -9" and system coding
**	errors).  A server cannot be allowed to start up when this condition
**	exists, so this error is returned.
**
**  System Status:
**	The process being started up (the dmfacp or dbms server) fails to
**	come up, printing error messages to the error log (errlog.log), 
**	and possibly, in the case of the dmfacp, to the II_ACP.LOG file.
**
**  User Action:
**	1. Check to see if a "dmfrcp" process is running in you're
**	   installation.  If one exists, this error may have been caused
**	   by a race condition between starting you're process and 
**	   starting the "dmfrcp" process.  Try starting you're server
**	   once more.
**	2. IF 1 is not true THEN:
**	All the following should only be done by a System Administrator.
**	  a. Shutdown entire installation using "rcpconfig" with the
**	     "imm_shutdown" option.
**	  b. If any other processes remain active in your installation
**	     following this shutdown,  then shutdown these processes manually 
**	     with the appropriate operating system command.
**	  c. Restart the installation from scratch using the appropriate
**	     startup command documented in you're Installation and Operations
**	     guide.
*/
E_CL0F31_LG_CANT_OPEN:E_GE94D4_HOST_ERROR
"Could not initialize the logging system for this process because the
database recovery log file (%0c) could not be opened.  We attempted to
open the file with the a flag mask of (%1c).  Check the ownership and
permissions on the Ingres log file."
/*
** Parameters:
**	%0  The name of the recovery log file which the system attempted to open.
**	%1  The unix flag mask passed to the open() system call.
**
** Explanation:
**	All transaction recovery information is written to a single log
**	file.  The file is expected to be found in a file named:
**	$II_LOG_FILE/ingres/log/ingres_log.  For some reason the system's
**	attempt to open this file has failed.
**
**  System Status:
**	The process being started up halts with a fatal error.
**
**  User Action:
**	1) Check the file permissions on the Ingres recovery log file.
**
**	   This file is located in $II_LOG_FILE/ingres/log/ingres_log,
**	   where $II_LOG_FILE is the value of II_LOG_FILE printed when
**	   the "ingprenv" command is issued.
**
**	   This file should be owned by the same user who owns the rest
**	   of the Ingres installation (ie. usually "ingres").  The 
**	   path to the file should be accessible by the "ingres" user.
**	   The file must be readable and writable by the "ingres" user.
**
**	   Make sure that the environment variable II_LOG_FILE is not
**	   set in your local shell environment.
**
**	2) Check permissions on executables being run.
**	   
**	   All executables which must access the recovery log file must
**	   have the "setuid" bit on and be owned by the "ingres" user.
**	   These executables include: dmfacp, dmfrcp, dmfjsp, iidbms, 
**	   csinstall, and others.
**
*/
E_CL0F32_LG_SEM_OWNER_DEAD:NO_GE
"A process exited while holding a semaphore critical to a logging operation.
The semaphore will never be granted so an error has been returned.  In the
currentt system this error is unrecoverable, and is fatal to any process
encountering it.  If this error is encountered the installation should be
shut down (killing processes if necessary).  Use normal startup procedures
(allowing automatic recovery of any outstanding transactions)."
/*
** Parameters:
**      none.
**
** Explanation:
**      An owner of a logging critical (LK) semaphore has exited.
**      The requesting process would "wait" forever for
**      access to this resource if this error were not returned (possibly
**      spinning using all available cpu on the machine).
**
** System Status:
**      All processes encountering this error will shut down.
**
** User Action:
**      If any process associated with an installation does not come down,
**      then the installation should be shut down by hand, and then restarted
**      (refer to the I&O guide for OS specific information on how to do this).
**      Information in the errlog.log may point to the cause of the
**      semaphore failure.
*/
E_CL1001_LK_BUSY:E_GE8D04_NO_RESOURCE
"Resource is busy"
E_CL1002_LK_TIMEOUT:NO_GE
"Lock timed out"
E_CL1003_LK_BADPARAM:NO_GE
"Bad parameter(s) passed to routine"
E_CL1004_LK_DEADLOCK:E_GEC2EC_SERIALIZATION
"Deadlock detected"
E_CL1005_LK_NOLOCKS:E_GE8D04_NO_RESOURCE
"Out of lock resources"
E_CL1006_LK_UNEXPECTED:NO_GE
"Lock manager internal error"
E_CL1007_LK_NEW_LOCK:NO_GE
"New lock was obtained"
E_CL1020_LK_BUSY_ALTER:NO_GE
"LKalter call failed because lkd->lkd_llb_inuse was set to
%0d."
E_CL1021_LK_LLB_ALLOC_FAILED:NO_GE
"LKalter's call to allocate %0d locks lists ran out of memory"
E_CL1022_LK_LOCK_ALLOC_FAILED:NO_GE
"LKalter's call to allocate %0d locks ran out of memory"
E_CL1023_LK_LKH_ALLOC_FAILED:NO_GE
"LKalter's call to allocate %0d hash buckets for the lock
hash table ran out of memory"
E_CL1024_LK_RSH_ALLOC_FAILED:NO_GE
"LKalter's call to allocate %0d hash buckets for the lock
hash table ran out of memory"
E_CL1025_LK_NOINTERRUPT_ALTER:NO_GE
"LKalter's call to change the interrupt status failed due to
a bad paramter; the lock id = %0d; the number of locks in the
system = %d."
E_CL1026_LK_NOINTERRUPT_ALTER:NO_GE
"LKalter's call to change the interrupt status failed due to
a bad paramter; input lock id instance = %0d; the system 
lock type = %1d; the system id instance = %2d." 
E_CL1027_LK_BAD_ALTER_REQUEST:NO_GE
"LKalter recieved request to alter parameter %0d, which is
not a supported LKalter parameter."
E_CL1028_LK_CANCEL_BADPARAM:NO_GE
"LKcancel() failed due to a lock id bad paramter; the input
lock id = %0d; the number of locks in the system = %d."
E_CL1029_LK_CANCEL_BADPARAM:NO_GE
"LKcancel() failed due to a lock id bad paramter; input lock id instance = %0d; the system lock type = %1d; 
the system id instance = %2d." 
E_CL102A_LK_CREATE_BADPARAM:NO_GE
"LKcreate_list() failed due to a lock id bad paramter; the 
input related lock id = %0d; the number of locks in the 
system = %d."
E_CL102B_LK_CREATE_BADPARAM:NO_GE
"LKcreate_list() failed due to a lock id bad paramter; input
related lock id instance = %0d; the system related lock 
type = %1d; the system related id instance = %2d." 
E_CL102C_LK_CREATE_BADPARAM:NO_GE
"LKcreate_list() failed due to a NULL lock list pointer."
E_CL102D_LK_EXPAND_LIST_FAILED:NO_GE
"LK failed while trying to allocate more objects of type
%0d, The reason for failure was that the locking and logging
shared memory pool was exhausted.  %1d lock lists and %2d
locks have already been allocated"
E_CL102E_LK_EXPAND_LIST_FAILED:NO_GE
"LK failed while trying to allocate more locks.  There 
are %0d locks, allocated at system startup time, of which %1d
are used.  The request was made with either 
(LLB_RECOVER | LLB_MASTER) status."
E_CL102F_LK_EXPAND_LIST_FAILED:NO_GE
"LK failed while trying to allocate more locks.  There 
are %0d locks, allocated at system startup time, of which %1d
are used.  %2d locks have been reserved for use by the
recovery system."
E_CL1030_LK_EXPAND_LIST_FAILED:NO_GE
"LK failed while trying to allocate more locks lock lists.  
There are %0d locks lists, allocated at system startup time,
of which %1d are used."
E_CL1031_LK_BAD_UNIQUE_ID:NO_GE
"LK failed while allocating a lock list, due to a bad unique
identifier passed in by the caller.  The unique id's value is
%0d, and the lk_uhigh part of this structure is %1d"
E_CL1032_LK_DUPLICATE_LOCK_ID:NO_GE
"LK failed while allocating a lock list, due to a bad unique
identifier passed in by the caller.  The unique id's value is
%0d, and the lk_uhigh part of this structure is %1d"
E_CL1033_LK_EVENT_BAD_PARAM:NO_GE
"LKevent() failed due to a lock id bad paramter; the 
input related lock id = %0d; the number of locks in the 
system = %1d."
E_CL1034_LK_EVENT_BAD_PARAM:NO_GE
"LKevent() failed due to a lock id bad paramter; input
lock id instance = %0d; the system related lock 
type = %1d; the system related id instance = %2d." 
E_CL1035_LK_EVENT_BAD_PARAM:NO_GE
"LKevent() failed due to a bad paramter. The lock 
identified was already waiting for an event; status of the
lock is %0d, and flag is %1d."
E_CL1036_LK_RELEASE_BAD_PARAM:NO_GE
"LKrelease() failed due to a lock list id bad paramter; the 
input lock list id = %0d; the number of lock lists in the 
system = %1d."
E_CL1037_LK_RELEASE_BAD_PARAM:NO_GE
"LKrelease() failed due to a lock id bad paramter; input
lock id instance = %0d; the system lock type = %1d; 
the system id instance = %2d." 
E_CL1038_LK_RELEASE_BAD_PARAM:NO_GE
"LKevent() failed due to a lock id bad paramter; the 
input related lock id = %0d; the number of locks in the 
system = %1d."
E_CL1039_LK_RELEASE_BAD_PARAM:NO_GE
"LKrelease() failed due to a lock id bad paramter; input
lock id instance = %0d; the system lock type = %1d; 
the system id instance = %2d." 
E_CL103A_LK_RELEASE_BAD_PARAM:NO_GE
"LKrelease() failed due to a bad paramter, neither lock_key
nor lockid were set."
E_CL103B_LK_RELEASE_BAD_PARAM:NO_GE
"LKrelease() failed due to a bad paramter, either the 
system could not find the lock (lkb = %0d) or the lock found
had an incorrect attribute (lkb_attribute = %1x)."
E_CL103C_LK_RELEASE_BAD_PARAM:NO_GE
"LKrelease() failed due to a bad paramter, no lock_key
was specified for a partial release."
E_CL103D_LK_REQUEST_BAD_PARAM:NO_GE
"LKrequest() failed due to a lock list id bad paramter, or a
bad mode (lock mode = %2d); the input lock list id = %0d; 
the number of locks in the system = %1d."
E_CL103E_LK_REQUEST_BAD_PARAM:NO_GE
"LKrequest() failed due to a lock list id bad paramter; input
lock list id instance = %0d; the system lock type = %1d; 
the system lock list id instance = %2d." 
E_CL103F_LK_REQUEST_BAD_PARAM:NO_GE
"LKrequest() failed due to a bad paramter, no lockid was
specified for a convert."
E_CL1040_LK_REQUEST_BAD_PARAM:NO_GE
"LKrequest() failed due to a lock list id bad paramter; the 
input lock list id = %0d; the number of lock lists in the 
system = %1d."
E_CL1041_LK_REQUEST_BAD_PARAM:NO_GE
"LKrequest() failed due to a lock id bad paramter; input
lock id instance = %0d; the system lock type = %1d; 
the system id instance = %2d." 
E_CL1042_LK_REQUEST_BAD_PARAM:NO_GE
"LKrequest() failed due to no lock key being passed in."
E_CL1043_LK_SHOW_BAD_PARAM:NO_GE
"LKshow() failed to show LK_S_ORPHANED'd locks.  Size
requested is %0d, size of a LK_ID is %1d, and lock_key is
%2x."
E_CL1044_LK_SHOW_BAD_PARAM:NO_GE
"LKshow() failed to show LK_S_RESOURCE.  Count
requested is %0d."
E_CL1045_LK_SHOW_BAD_PARAM:NO_GE
"LKshow() failed to show LK_S_LOCKS.  Count
requested is %0d."
E_CL1046_LK_SHOW_BAD_PARAM:NO_GE
"LKshow() failed to show LK_S_OWNER.  Bad lockid pointer
(%0x) or bad lock_key (%1x)."
E_CL1047_LK_SHOW_BAD_PARAM:NO_GE
"LKshow() failed due to an unrecognized show flag (%0d)."
E_CL1048_LK_CANCEL_BADPARAM:NO_GE
"Error in LKcancel with Shared Lock List - Lock list type or status is
incorrect.  Input list id: %0d, shared list type: %1d, shared list status: %2d."
E_CL1049_LK_CANCEL_BADPARAM:NO_GE
"Error in LKcancel with Shared Lock List - Client lock list references
no shared list.  Input list id: %0d."
E_CL1050_LK_LOCKID_NOTFOUND:NO_GE
"Error in LKrequest call with specified lock id - Lock with specified id
not found.  Unique id (%0d, %1d), Request flags %2d."
E_CL1051_LK_DUPLICATE_LOCK_ID:NO_GE
"Error in LKcreate_list allocating shared lock list - duplicated lock id.
Lock unique id (%0d, %1d)."
E_CL1052_LK_RELEASE_BADPARAM:NO_GE
"Error in LKrelease with Shared Lock List - Lock list type or status is
incorrect.  Input list id: %0d, shared list type: %1d, shared list status: %2d."
E_CL1053_LK_RELEASE_BADPARAM:NO_GE
"Error in LKrelease with Shared Lock List - Client lock list references
no shared list.  Input list id: %0d."
E_CL1054_LK_REQUEST_BADPARAM:NO_GE
"Error in LKrequest with Shared Lock List - Lock list type or status is
incorrect.  Input list id: %0d, shared list type: %1d, shared list status: %2d."
E_CL1055_LK_REQUEST_BADPARAM:NO_GE
"Error in LKrequest with Shared Lock List - Client lock list references
no shared list.  Input list id: %0d."
E_CL1056_LK_SEM_OWNER_DEAD:NO_GE
"A process exited while holding a semaphore critical to a locking operation.   
The semaphore will never be granted so an error has been returned.  In the
currentt system this error is unrecoverable, and is fatal to any process
encountering it.  If this error is encountered the installation should be
shut down (killing processes if necessary).  Use normal startup procedures
(allowing automatic recovery of any outstanding transactions)."
/*
** Parameters:
**      none.
**
** Explanation:
**      An owner of a locking critical (LK) semaphore has exited.  
**	The requesting process would "wait" forever for
**      access to this resource if this error were not returned (possibly
**      spinning using all available cpu on the machine).
**
** System Status:
**	All processes encountering this error will shut down.  
**
** User Action:
**	If any process associated with an installation does not come down,
**	then the installation should be shut down by hand, and then restarted 
**	(refer to the I&O guide for OS specific information on how to do this).
**      Information in the errlog.log may point to the cause of the 
**	semaphore failure.
*/
E_CL1101_LO_ADD_BAD_SYN:NO_GE
"LOaddpath: Bad syntax in arguments to LOaddpath -- tail shouldn't begin at
root"
E_CL1102_LO_FR_BAD_SYN:NO_GE
"LOfroms: String argument to LOfroms has bad syntax"
E_CL1103_LO_NO_SUCH:NO_GE
"No such location exists"
E_CL1104_LO_NO_PERM:E_GE84D0_NO_PRIVILEGE
"You do not have permission to access the location with this call"
E_CL1105_LO_NO_SPACE:E_GE8D04_NO_RESOURCE
"Not enough space on file system"
E_CL1106_LO_NOT_FILE:NO_GE
"Location argument was not a FILENAME"
E_CL1107_LO_NULL_ARG:NO_GE
"Illegal null pointer was passed to location routine"
E_CL1108_LO_CANT_TELL:NO_GE
"Undetermined error in LO routines"
E_CL1109_LONOSAVE:NO_GE
"LOreset called without matching call to LOsave"
E_CL110A_LO_TOO_LONG:NO_GE
"LOgt:Location obtained overflowed buffer"
E_CL1201_ME_GOOD:E_GE0000_OK
"ME routine: The Status returned was good"
E_CL1202_ME_BD_CHAIN:NO_GE
"MEdump: correct parameter value must be one of ME_D_ALLOC, ME_D_FREE,
ME_D_BOTH"
E_CL1203_ME_BD_CMP:NO_GE
"MEcmp: number of bytes to compare must be > 0"
E_CL1204_ME_BD_COPY:NO_GE
"MEcopy: number of bytes to copy must be > 0"
E_CL1205_ME_BD_FILL:NO_GE
"MEfill: number of bytes to fill must be > 0"
E_CL1206_ME_BD_TAG:NO_GE
"MEt[alloc, free]: tags must be > 0"
E_CL1207_ME_ERR_PROGRAMMER:NO_GE
"MEfree: Internal Error."
E_CL1208_ME_FREE_FIRST:NO_GE
"ME[t]free: can't free a block before any blocks have been allocated"
E_CL1209_ME_GONE:E_GE8D04_NO_RESOURCE
"ME[t]alloc: system can't allocate any more memory for this process"
E_CL120A_ME_NO_ALLOC:NO_GE
"ME[t]alloc: request to allocate a block with zero (or less) bytes was ignored"
E_CL120B_ME_NO_FREE:NO_GE
"MEfree: can't find a block with this address in the free list"
E_CL120C_ME_NO_TFREE:NO_GE
"MEtfree: process hasn't allocated any memory with this tag"
E_CL120D_ME_00_PTR:NO_GE
"ME[t]alloc: passed a null pointer"
E_CL120E_ME_00_CMP:NO_GE
"MEcmp: passed a null pointer"
E_CL120F_ME_00_COPY:NO_GE
"MEcopy: passed a null pointer"
E_CL1210_ME_00_DUMP:NO_GE
"MEdump: passed a null pointer"
E_CL1211_ME_00_FILL:NO_GE
"MEfill: passed a null pointer"
E_CL1212_ME_00_FREE:NO_GE
"MEfree: passed a null pointer"
E_CL1213_ME_TR_FREE:NO_GE
"MEfree: the memory has been corrupted"
E_CL1214_ME_TR_SIZE:NO_GE
"MEsize: the memory has been corrupted"
E_CL1215_ME_TR_TFREE:NO_GE
"MEtfree: the memory has been corrupted"
E_CL1216_ME_OUT_OF_RANGE:NO_GE
"ME routine: address is out of process's data space, referencing will cause bus
error"
E_CL1217_ME_BF_OUT:E_GE8D04_NO_RESOURCE
"MEneed: 'buf' doesn't have 'nbytes' left to allocate"
E_CL1218_ME_BF_ALIGN:NO_GE
"MEinitbuf: 'buf' not aligned"
E_CL1219_ME_BF_FALIGN:NO_GE
"MEfbuf: 'buf' not aligned"
E_CL121A_ME_BF_PARAM:NO_GE
"MEfbuf: 'bytes' argument must come from call to MEfbuf()"
E_CL121B_ME_TOO_BIG:NO_GE
"MEreqmem/MEreqlng/ME[t][c]alloc: Object too big"
E_CL121C_ME_OUT_OF_MEM:NO_GE
"MEget_pages: Can not expand memory size"
E_CL121D_ME_BAD_PARAM:NO_GE
"Bad Paramater to ME routines"
E_CL121E_ME_ILLEGAL_USAGE:NO_GE
"Combination of arguments to an ME routine is invalid"
E_CL121F_ME_BAD_ADVICE:NO_GE
"An ME routine will not function with the given MEadvice)"
E_CL1220_ME_ALREADY_EXISTS:NO_GE
"A shared memory segment to be created already exists"
E_CL1221_ME_NO_SUCH_SEGMENT:NO_GE
"A shared memory segment to be manipulated does not exist"
E_CL1222_ME_NO_SHARED:NO_GE
"This CL does not support shared memory"
E_CL1223_ME_NOT_ALLOCATED:NO_GE
"The memory specified for some ME operation was not all allocated"
E_CL1224_ME_NOT_SUPPORTED:NO_GE
"The ME operation requested is not supported in this CL"
E_CL1225_ME_NO_PERM:NO_GE
"Insufficient permission for some ME operation"
E_CL1226_ME_SHARED_ALLOC_FAIL:NO_GE
"ME_get_shared: The request to the operating system to allocate %0d bytes of
shared memory failed.  This request most probably failed because the size of
the allocation exceeds the maximum size shared memory segment configured for
the OS.  In most system V interface compliant shared memory OS implementations
this maximum size is user tunable by altering the SHMSIZE parameter in the
kernel configuration procedure."
/*
** Parameters:
**	%0	amount in bytes of shared memory requested.
** Explanation:
**	Shared memory has been requested using the system V "shmget" interface.
**	This routine has returned "EINVAL" which can mean one of 2 things
**	(see "man shmget" for information specific to you're OS):
**
**     		size is less than the system-imposed minimum or greater
**     		than the system imposed maximum.
**
**     		A shared memory identifier exists for key but
**     		the size of the segment associ- ated with it is less
**     		than size and size is not equal to zero.
**
**	The maximum size shared memory segment a system can provide is site
**	specific and is given by the system administrator defined value assigned
**	to SHMSIZE (in 1k units).  This value usually is defaulted to 1024,
**	which sets an upper limit of 1 megabyte for a single shared memory
**	buffer.  This means that under a system configured with the defaults
**	one may not start up a dbms server using a shared buffer cache bigger
**	than 512 ingres pages:
**
**		512 ingres pages * 2048 bytes/ingres page = 1meg.
**
**	The value of SHMSIZE should be set to at least twice the number of
**	ingres pages that you wish to start up a dbms server with.  For instance
**	if you want to start up an ingres server with a 1024 page ingres cache
**	you should rebuild the kernel with SHMSIZE set to 2048 (actually you
**	should set it slightly higher since there is some amount of overhead
**	used by the dmf cache.)
**	
** System Status:
**	Server will not start up. 
** User Action:
**	Either restart server with the dmf.cache_size set to less than 1/2 the
**	current value of SHMSIZE, or rebuild the kernel with SHMSIZE increased
**	enough to support the required size buffer cache.
*/
E_CL1301_MH_BADARG:NO_GE
"Bad argument"
E_CL1302_MH_PRECISION:NO_GE
"Lost precision during calculation"
E_CL1303_MH_INTERNERR:NO_GE
"Internal error"
E_CL1304_MH_MATHERR:NO_GE
"Math error"
E_CL1401_NM_PWDOPN:E_GE94D4_HOST_ERROR
"NMpathIng: Unable to open passwd file"
E_CL1402_NM_PWDFMT:NO_GE
"NMpathIng: Bad passwd file format"
E_CL1403_NM_INGUSR:NO_GE
"NMpathIng: There is no INGRES user in the passwd file"
E_CL1404_NM_INGPTH:NO_GE
"NM[sg]tIngAt: No path returned for INGRES user"
E_CL1405_NM_STOPN:E_GE94D4_HOST_ERROR
"NM[sg]tIngAt: Unable to open symbol table"
E_CL1406_NM_STPLC:E_GE94D4_HOST_ERROR
"NMstIngAt: Unable to position to record in symbol table"
E_CL1407_NM_STREP:E_GE94D4_HOST_ERROR
"NMstIngAt: Unable to replace record in symbol table"
E_CL1408_NM_STAPP:E_GE94D4_HOST_ERROR
"NMstIngAt: Unable to append symbol to symbol table"
E_CL1409_NM_LOC:NO_GE
"NM_loc: first argument must be 'f' or 't'"
E_CL140A_NM_BAD_PWD:NO_GE
"NMpathIng: Bad home directory for INGRES superuser"
E_CL1601_PC_CM_CALL:NO_GE
"PCcmdline: arguments incorrect"
E_CL1602_PC_CM_EXEC:NO_GE
"PCcmdline: cannot execute command specified, bad magic number"
E_CL1603_PC_CM_MEM:E_GE8D04_NO_RESOURCE
"PCcmdline: system temporarily out of core or process requires too many
segmentation registers"
E_CL1604_PC_CM_OWNER:E_GE84D0_NO_PRIVILEGE
"PCcmdline: must be owner or super-user to execute this command"
E_CL1605_PC_CM_PATH:NO_GE
"PCcmdline: part of specified path didn't exist"
E_CL1606_PC_CM_PERM:E_GE84D0_NO_PRIVILEGE
"PCcmdline: didn't have permission to execute"
E_CL1607_PC_CM_PROC:E_GE8CA0_SYSTEM_LIMIT
"PCcmdline: system process table is temporarily full"
E_CL1608_PC_CM_REOPEN:NO_GE
"PCcmdline: couldn't associate argument [in, out]_name with std[in, out]"
E_CL1609_PC_CM_SUCH:NO_GE
"PCcmdline: command doesn't exist"
E_CL160A_PC_CM_BAD:NO_GE
"PCcmdline: child didn't execute, e.g. bad parameters"
E_CL160B_PC_CM_TERM:NO_GE
"PCcmdline: child we were waiting for returned bad termination status"
E_CL160C_PC_END_PIPE:E_GE9088_COMM_ERROR
"PCendpipe: had trouble closing PIPE argument passed in"
E_CL160D_PC_SP_CALL:NO_GE
"PC[f][s]spawn: arguments incorrect"
E_CL160E_PC_SP_EXEC:NO_GE
"PC[f][s]spawn: cannot execute command specified, bad magic number"
E_CL160F_PC_SP_MEM:E_GE8D04_NO_RESOURCE
"PC[f][s]spawn: system temporarily out of core or process requires too many
segmentation registers"
E_CL1610_PC_SP_OWNER:E_GE84D0_NO_PRIVILEGE
"PC[f][s]spawn: must be owner or super-user to execute this command"
E_CL1611_PC_SP_PATH:NO_GE
"PC[f][s]spawn: part of specified path didn't exist"
E_CL1612_PC_SP_PERM:E_GE84D0_NO_PRIVILEGE
"PC[f][s]spawn: didn't have permission to execute"
E_CL1613_PC_SP_PROC:E_GE8D04_NO_RESOURCE
"PC[f][s]spawn: system process table is temporarily full"
E_CL1614_PC_SP_SUCH:NO_GE
"PC[f][s]spawn: command doesn't exist"
E_CL1615_PC_SP_CLOSE:E_GE9088_COMM_ERROR
"PC[f]sspawn: had trouble closing unused sides of pipes"
E_CL1616_PC_SP_DUP:E_GE9088_COMM_ERROR
"PC[f]sspawn: had trouble associating, dup()'ing, the pipes with child's
std[in,out]"
E_CL1617_PC_SP_PIPE:E_GE9088_COMM_ERROR
"PC[f]sspawn: couldn't establish communication pipes"
E_CL1618_PC_SP_OPEN:E_GE9088_COMM_ERROR
"PCfsspawn: had trouble associating child's std[in,out] with passed in FILE
pointers"
E_CL1619_PC_SP_REOPEN:E_GE9088_COMM_ERROR
"PCspawn: couldn't associate argument [in, out]_name with std[in, out]"
E_CL161A_PC_RD_CALL:NO_GE
"PCread: one of the passed in arguments is NULL"
E_CL161B_PC_RD_CLOSE:NO_GE
"PCread: stream corrupted, should never happen"
E_CL161C_PC_RD_OPEN:E_GE9088_COMM_ERROR
"PCread: trouble establishing communication pipes"
E_CL161D_PC_SD_CALL:E_GE9088_COMM_ERROR
"PCsend: invalid exception value"
E_CL161E_PC_SD_NONE:E_GE9088_COMM_ERROR
"PCsend: No process corresponding to the pid (process argument) exists"
E_CL161F_PC_SD_PERM:E_GE9088_COMM_ERROR
"PCsend: Can't signal specified process.  Effective uid of caller isn't 'root'
and doesn't match real uid of receiving process"
E_CL1620_PC_WT_BAD:E_GE9088_COMM_ERROR
"PCwait: child didn't execute, e.g bad parameters"
E_CL1621_PC_WT_NONE:E_GE9088_COMM_ERROR
"PCwait: no children of this process are currently alive"
E_CL1622_PC_WT_TERM:E_GE9088_COMM_ERROR
"PCwait: child we were waiting for returned bad termination status"
E_CL1623_PC_WT_INTR:E_GE9088_COMM_ERROR
"PCwait: child was interrupted by a signal"
E_CL1624_PC_NOT_MBX:E_GE9088_COMM_ERROR
"PEendpipe: channel not assigned to mailbox"
E_CL1625_PC_INTERLOCK:E_GE9088_COMM_ERROR
"PEendpipe: mailbox locked by another process"
E_CL1626_PC_BADCHAN:E_GE9088_COMM_ERROR
"PEendpipe: not a legal channel number"
E_CL1627_PC_NOPRIV:E_GE84D0_NO_PRIVILEGE
"PEendpipe: mailbox not assigned or insufficient privilege"
E_CL1701_PE_BAD_PATTERN:NO_GE
"Permission pattern passed to a permission routine had syntax errors"
E_CL1702_PE_NULL_LOCATION:NO_GE
"Location passed to a permission routine was null"
E_CL1800_QU:NO_GE
"No errors from these routines"
E_CL1901_SI_BAD_SYNTAX:NO_GE
"SIopen: Bad syntax in mode argument to SIopen"
E_CL1902_SI_EMFILE:E_GE8CA0_SYSTEM_LIMIT
"Can't open another file -- maximum number of files already opened"
E_CL1903_SI_BAD_MODE:NO_GE
"SIfopen: Bad mode parameter"
E_CL1904_SI_CANT_OPEN:E_GE94D4_HOST_ERROR
"SIfopen: Can't open file"
E_CL1B01_SR_BADFILE:NO_GE
"Bad sort file identifier"
E_CL1B02_SR_ENDFILE:NO_GE
"End of File"
E_CL1B03_SR_BADOPEN:E_GE94D4_HOST_ERROR
"Error opening sort file"
E_CL1B04_SR_BADCLOSE:E_GE94D4_HOST_ERROR
"Error closing sort file"
E_CL1B05_SR_BADWRITE:E_GE94D4_HOST_ERROR
"Error writing to sort file"
E_CL1B06_SR_BADREAD:E_GE94D4_HOST_ERROR
"Error reading from sort file"
E_CL1B07_SR_BADEXTEND:E_GE94D4_HOST_ERROR
"Error allocating space to sort file"
E_CL1B08_SR_BADCREATE:E_GE94D4_HOST_ERROR
"Error creating sort file"
E_CL1F01_TM_BAD_MSECS:NO_GE
"Number of milliseconds for an alarm was less than zero"
E_CL1F02_TM_ZERO_TIME:NO_GE
"Zero seconds resulted when rounding milliseconds for an alarm"
E_CL1F03_TM_NOTIMEZONE:E_GE8D68_CONFIG_ERROR
"The environment variable, II_TIMEZONE, is undefined"
E_CL1F04_TM_SYNTAX:NO_GE
"syntax error in time format"
E_CL2101_TR_BADCREATE:E_GE94D4_HOST_ERROR
"Error creating trace output file"
E_CL2102_TR_BADOPEN:E_GE94D4_HOST_ERROR
"Error opening trace output file"
E_CL2103_TR_ENDINPUT:NO_GE
"TRrequest: End of File encountered"
/*
** Parameters:
**	none.
**
** Explanation:
**	End of input file has been encountered or user has input EOF
**	in response to prompt.
**
**  System Status:
**	Client code requesting input should terminate reading.
**
**  User Action:
**	No user action appropriate.
*/
E_CL2104_TR_BADPARAM:NO_GE
"Bad parameter(s) passed to this routine"
E_CL2105_TR_BADREAD:E_GE94D4_HOST_ERROR
"Error reading trace file"
E_CL2106_TR_BADWRITE:E_GE94D4_HOST_ERROR
"Error writing trace file"
E_CL2201_UT_CO_FILE:E_GE94D4_HOST_ERROR
"Can't open utcom.def"
E_CL2202_UT_CO_IN_NOT:E_GE94D4_HOST_ERROR
"Can't open input file"
E_CL2203_UT_CO_LINES:NO_GE
"Bad line in utcom.def"
E_CL2204_UT_CO_CHARS:NO_GE
"Bad line in utcom.def"
E_CL2205_UT_CO_SUF_NOT:NO_GE
"Couldn't find suffix"
E_CL2206_UT_ED_CALL:NO_GE
"UTedit: NULL filename argument"
E_CL2207_UTENOEQUAL:NO_GE
"No equal sign ('=') in argument list"
E_CL2208_UTENOPERCENT:NO_GE
"No percent sign ('\%') in argument list"
E_CL2209_UTENOSPEC:NO_GE
"No type specifier in argument list"
E_CL220A_UTEBADARG:NO_GE
"The argument list has does not have a comma between two arguments or ends too
early"
E_CL220B_UTENOARG:NO_GE
"Given argument not in utexe.def entry for program"
E_CL220C_UTEBIGCOM:NO_GE
"Command length exceeds utexe's limit"
E_CL220D_UTEBADSPEC:NO_GE
"Utexe.def has a bad type specifier in one entry for the program"
E_CL220E_UTENOBIN:NO_GE
"No executable found for program"
E_CL220F_UTENOPROG:NO_GE
"The given program name is not in utexe.def"
E_CL2210_UT_PR_CALL:NO_GE
"UTprint: NULL filename argument"
E_CL2211_UTENoInit:NO_GE
"No text"
E_CL2212_UTENoWriteFunc:NO_GE
"No text"
E_CL2213_UTENoReadFunc:NO_GE
"No text"
E_CL2214_UTENoModuleType:NO_GE
"No text"
E_CL2215_UTENOSYM:NO_GE
"The actual argument for a \%C parameter was not a valid user command"
E_CL2216_UTELATEEXE:NO_GE
"A module being run as a procedure had a \%C or \%E actual argument"
E_CL2217_UTEBADTYPE:NO_GE
"The type specifier for one argument was not of an expected type"
E_CL2218_UT_EXE_DEF:NO_GE
"Can't open utexe.def"
E_CL2300_NT:NO_GE
"No errors for these routines"
E_CL2401_JF_BAD_FILE:NO_GE
"Bad journal file"
E_CL2402_JF_END_FILE:NO_GE
"End of journal file reached"
E_CL2403_JF_BAD_OPEN:E_GE94D4_HOST_ERROR
"Error trying to open a journal file"
E_CL2404_JF_BAD_CLOSE:E_GE94D4_HOST_ERROR
"Error trying to close a journal file"
E_CL2405_JF_BAD_WRITE:E_GE94D4_HOST_ERROR
"Error trying to write a journal file"
E_CL2406_JF_BAD_READ:E_GE94D4_HOST_ERROR
"Error trying to read a journal file"
E_CL2407_JF_BAD_ALLOC:E_GE94D4_HOST_ERROR
"Error trying to extend a journal file"
E_CL2408_JF_BAD_UPDATE:E_GE94D4_HOST_ERROR
"Error trying to updating the header of a journal file"
E_CL2409_JF_BAD_CREATE:E_GE94D4_HOST_ERROR
"Error trying to create a journal file"
E_CL240A_JF_BAD_DELETE:E_GE94D4_HOST_ERROR
"Error trying to delete a journal file"
E_CL2500_CS_OK:E_GE0000_OK
"All is fine."
E_CL2501_CS_INVALID_STATE:NO_GE
"The Server control block indicates an invalid state."
E_CL2502_CS_SCB_ALLOC_FAIL:E_GE94D4_HOST_ERROR
"Unable to allocate a session control block."
E_CL2503_CS_NOT_QUIESCENT:NO_GE
"Server has active sessions:   Unable to perform shutdown."
E_CL2504_CS_BAD_PARAMETER:NO_GE
"Invalid parameter on CS call."
E_CL2506_CS_TIMER_ERROR:NO_GE
"Unable to queue timer request."
E_CL2507_CS_RDY_QUE_CORRUPT:NO_GE
"The ready queue for the server is in an inconsistent state."
E_CL2508_CS_INTERRUPTED:NO_GE
"The request was interrupted at user request."
E_CL2509_CS_TIMEOUT:NO_GE
"The request has returned due to timeout."
E_CL250A_CS_NO_SEMAPHORE:NO_GE
"The task requesting a semaphore operation does not own a semaphore."
E_CL250C_CS_INVALID_MODE:NO_GE
"The session control block indicates an invalid mode."
E_CL250D_CS_INVALID_RTN_MODE:NO_GE
"The routine called by CS returned an invalid mode."
E_CL250E_CS_SESSION_LIMIT:E_GE8CA0_SYSTEM_LIMIT
"Too many sessions have been requested."
E_CL250F_CS_REQUEST_ABORTED:NO_GE
"The request has been aborted."
E_CL2510_CS_QUANTUM_INVALID:NO_GE
"The user specified timer quantum is invalid."
E_CL2511_CS_BAD_CNX_STATUS:E_GE9088_COMM_ERROR
"Unable to make connection with the working partner."
E_CL2512_CS_BAD_ADMIN_SCB:NO_GE
"The admin session has been called with a non-admin scb."
E_CL2513_CS_CORRUPT_ADMIN_SCB:NO_GE
"The session control block for the admin SCB has been corrupted."
E_CL2514_CS_ESCAPED_EXCEPTION:E_GE9858_FATAL_ERROR
"An exception has escaped from user session.  This session will be terminated."
E_CL2515_CS_FORCED_EXIT:E_GE9858_FATAL_ERROR
"The server has been forced to exit involuntarily."
E_CL2516_CS_SYSTEM_ERROR:E_GE94D4_HOST_ERROR
"The server has encountered an unexpected system error."
E_CL2517_CS_SMPR_DEADLOCK:E_GEC2EC_SERIALIZATION
"A session has requested a semaphore which it already owns."
E_CL2518_CS_NORMAL_SHUTDOWN:E_GE0000_OK
"The Server has terminated normally."
E_CL2519_CS_INVALID_READY:NO_GE
"A session has been RESUMED before it became runnable."
E_CL251A_CS_STACK_DEAL_FAIL:E_GE94D4_HOST_ERROR
"OS Error deallocating a STACK region."
E_CL251B_CS_IDLE_RESUME:NO_GE
"Attempt to resume the idle job"
E_CL251C_CS_TERM_W_SEM:NO_GE
"Session terminated while holding semaphore(s)." 
E_CL2520_CS_AMBIGUIOUS:NO_GE
"%0c ambiguous"
E_CL2521_CS_ARG_UNKNOWN:NO_GE
"argument %0c unknown"
E_CL2522_CS_NO_VALUE:NO_GE
"value not supplied to %0c"
E_CL2523_CS_TOO_MANY_OPTIONS:E_GE8D68_CONFIG_ERROR
"Too many options have been used, please re-issue with fewer options"
E_CL2524_CS_NON_NUMERIC_ARG:NO_GE
"%0c requires non-numeric arguments"
E_CL2525_CS_NUMERIC_ARG:NO_GE
"invalid numeric argument to %0c"
E_CL2526_CS_POSITIVE_VALUE:NO_GE
"%0c value must be greater than zero"
E_CL2527_CS_PARSE_ERROR:NO_GE
"internal error parsing options"
E_CL2528_CS_ARG_USAGE:NO_GE
"Option names must begin with a '-'"
E_CL2529_CS_SEM_OWNER_DEAD:NO_GE
"A process exited while holding the requested semaphore.   The semaphore 
will never be granted so an error has been returned, if the calling routine
cannot cleanup whatever was left behind by the exited process it must exit
as the semaphore for the critical section cannot be granted."
/*
** Parameters:
**      none.
** Explanation:
**	An owner of a semaphore controlling access to a resource shared across
**	processes has exited.  The requesting process would "wait" forever for
**      access to this resource if this error were not returned (possibly 
**	spinning using all available cpu on the machine).
**
** System Status:
**	Either the server or the installation will be shutdown (depending on
**	whether the resource is server or installation critical).  In 6.3 there
**	is a single semaphore controlling access to the buffer manager which
**	is server critical.  There are a few semaphores which control access to
**	the logging and locking components which are installation critical.
**
** User Action:
**      Either restart server, or installation depending on whether the server
**	or installation depending on what was shut down.  Information in the
**	errlog.log may point to the cause of the semaphore failure.
*/
E_CL25F9_CS_2STK_OVFL:E_GE9858_FATAL_ERROR
"Stack overflow while correcting previous stack overflow.  The server will be terminated."
E_CL25FA_CS_UNWIND_FAILURE:E_GE94D4_HOST_ERROR
"OS error while attempting to unwind an exception stack."
E_CL25FB_CS_UNDELIV_AST:E_GE94D4_HOST_ERROR
"VMS was unable to deliver an AST to a thread."
E_CL25FC_CS_CRTCL_RESRC_HELD:NO_GE
"Thread encountered a thread fatal error while holding a critical resource."
E_CL25FD_CS_THREAD_STK_OVFL:E_GE9858_FATAL_ERROR
"Thread operating for <%0c> has exhausted stack space (%1d. bytes).  The thread
will be terminated.  \nPlease consult your INGRES system administrator.  \nNOTE:
The stack space can be expanded with the use of the /stack_size qualifier at
iirundbms time."
E_CL25FE_CS_25FE_RET_FROM_IDLE:E_GE9858_FATAL_ERROR
"The idle thread has erroneously returned.  This is server fatal."
E_CL25FF_CS_FATAL_ERROR:E_GE9858_FATAL_ERROR
"The server has encountered a FATAL error.  The server will be terminated."
E_CL2601_CI_NOSTR:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) is not available.
Please consult the INGRES system manager."
E_CL2602_CI_TOOLITTLE:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) contains too few characters.
Please consult the INGRES system manager."
E_CL2603_CI_TOOBIG:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) contains too many characters.
Please consult the INGRES system manager."
E_CL2604_CI_BADCHKSUM:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) checksums incorrectly.  This
is probably a typographical input error.
Please consult the INGRES system manager."
E_CL2605_CI_BADEXPDATE:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) indicates that your license has
expired.  Please consult the INGRES system manager."
E_CL2606_CI_BADSERNUM:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) does not match the serial number of
the machine on which you are running.
Please consult the INGRES system manager."
E_CL2607_CI_BADCPUMODEL:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) does not match the CPU model of
the machine on which you are running.
Please consult the INGRES system manager."
E_CL2608_CI_BADERROR:E_GE8D68_CONFIG_ERROR
"An unexpected error occurred during authorization string processing.
Please consult the INGRES system manager."
E_CL2609_CI_BADKEY:E_GE8D68_CONFIG_ERROR
"The authorization string (II_AUTHORIZATION) was improperly created for this
version of the software.
Please consult the INGRES system manager."
E_CL260A_CI_NOCLUSTER:E_GE8D68_CONFIG_ERROR
"The current installation indicates that VAXcluster processing is desired, but
the authorization string (II_AUTHORIZATION) does not provide VAXcluster
processing.
Please consult the INGRES system manager."
E_CL2614_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (DBMS) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2615_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (VAR_INGRES) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2616_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (NET) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2617_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (QUEL TM) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2618_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (SQL TM) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2619_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ABF) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL261A_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (GBF) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL261B_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (VIGRAPH) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL261C_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (MENU) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL261D_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (OSLQUEL) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL261E_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (OSLSQL) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL261F_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (QBF) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2620_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (RBF) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2621_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (REPORT) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2622_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (VIFRED) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2623_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Equel/ADA) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2624_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Equel/BASIC) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2625_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Equel/C) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2626_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Equel/COBOL) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2627_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Equel/FORTRAN) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2628_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Equel/PASCAL) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2629_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Equel/PL-1) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL262A_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ESQL/ADA) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL262B_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ESQL/BASIC) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL262C_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ESQL/C) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL262D_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ESQL/COBOL) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL262E_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ESQL/FORTRAN) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL262F_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ESQL/PASCAL) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2630_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (ESQL/PL-1) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2631_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (PC-LINK) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2632_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (VAXcluster) capability is not provided by the authorization
string (II_AUTHORIZATION). 
Please consult the INGRES system manager."
E_CL2633_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (BLOCK/3270 Mode) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2634_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (Distributed DBMS) capability is not provided by the authorization string
(II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL263B_CI_BADCAP:E_GE8D68_CONFIG_ERROR
"The required (VisiBuild) capability is not provided by the authorization
string (II_AUTHORIZATION).
Please consult the INGRES system manager."
E_CL2781_GC_CONNECT_FAIL:E_GE9088_COMM_ERROR
"An attempted network connection failed"
E_CL2782_GC_DISCONNECT_FAIL:E_GE9088_COMM_ERROR
"An attempted network disconnect failed"
E_CL2783_GC_SEND_FAIL:E_GE9088_COMM_ERROR
"An attempted network send failed"
E_CL2784_GC_RECEIVE_FAIL:E_GE9088_COMM_ERROR
"An attempted network receive failed"
E_CL2785_GC_LISTEN_FAIL:E_GE9088_COMM_ERROR
"An attempted network listen failed"
E_CL2786_GC_PINIT_FAIL:E_GE9088_COMM_ERROR
"Protocol initialization failed"
E_CL2787_GC_OPEN_FAIL:E_GE9088_COMM_ERROR
"An attempted network open failed"
E_CL2788_GC_NTWK_STATUS:E_GE9088_COMM_ERROR
"Error status returned by network"
E_CL2801_SL_CMP_ERROR:E_GE9088_COMM_ERROR
"Error comparing security labels"
E_CL2802_SL_CNVT_ERROR:E_GE9088_COMM_ERROR
"Error converting security label"
E_CL2803_SL_LABEL_ERROR:E_GE9088_COMM_ERROR
"Error obtaining security label associated with process"
E_CLFE01_BS_SOCK_ERR:NO_GE
"Could not create socket - socket(2) call failed; presumably system error."
E_CLFE02_BS_BIND_ERR:NO_GE
"Server bind to listen port failed: listen port is invalid or already in use.
Another server may be running or have recently exited."
E_CLFE03_BS_LISTEN_ERR:NO_GE
"Server listen(2) call failed; presumably system error."
E_CLFE04_BS_ACCEPT_ERR:NO_GE
"Server accept(2) call failed; presumably system error."
E_CLFE05_BS_CONNECT_ERR:NO_GE
"Connect failed: NAME server, DBMS server, or COMM server not running or busy.
First retry, and then check servers."
E_CLFE06_BS_WRITE_ERR:NO_GE
"Write to peer process failed; it may have exited."  
E_CLFE07_BS_READ_ERR:NO_GE
"Read from peer process failed; it may have exited."
E_CLFE08_BS_CLOSE_ERR:NO_GE
"Error closing connection; presumably system error."
E_CLFE0A_BS_BADADDR_ERR:NO_GE
"Register/connection address not formatted properly; check listen addresses."
E_CLFF01_INTERRUPTED:NO_GE
"Communication operation interrupted."
E_CLFF02_TIMEOUT:NO_GE
"Communication operation timed out."
E_CLFF03_FAILED:NO_GE
"Select(2) call failed; presumably system error."
/*
**    Message file for Facility code: _DM
*/


#define _DM_CLASS  3


E_DM0000_OK:E_GE0000_OK
"DMF successful return status."
E_DM0001_EXCPTN_FRM_EXCPTN:E_GE9D24_DATAEX_OTHER
"An exception occurred while processing an exception."
E_DM0006_BAD_ATTR_FLAGS:NO_GE
"Illegal flags for attribute characteristics."
E_DM0007_BAD_ATTR_NAME:NO_GE
"Illegal attribute name."
E_DM0008_BAD_ATTR_PRECISION:NO_GE
"Illegal value for attribute precision."
E_DM0009_BAD_ATTR_SIZE:NO_GE
"Illegal size for attribute."
E_DM000A_BAD_ATTR_TYPE:NO_GE
"Illegal type for attribute."
E_DM000B_BAD_CB_LENGTH:NO_GE
"Incorrect length for DMF control block."
E_DM000C_BAD_CB_TYPE:NO_GE
"Incorrect type for DMF control block."
E_DM000D_BAD_CHAR_ID:NO_GE
"Unknown characteristics identifier."
E_DM000E_BAD_CHAR_VALUE:NO_GE
"Illegal characteristics value."
E_DM000F_BAD_DB_ACCESS_MODE:NO_GE
"Unknown database access mode."
E_DM0010_BAD_DB_ID:NO_GE
"Illegal database identifier."
E_DM0011_BAD_DB_NAME:NO_GE
"Illegal database name."
E_DM001A_BAD_FLAG:NO_GE
"Illegal flag value or combination, in control block."
E_DM001B_BAD_INDEX:NO_GE
"Corrupt index table found."
E_DM001C_BAD_KEY_SEQUENCE:NO_GE
"Illegal key sequence to modify."
E_DM001D_BAD_LOCATION_NAME:NO_GE
"Unknown location name."
E_DM001F_LOCATION_LIST_ERROR:NO_GE
"The new and old location lists specified for the relocate command 
are incorrect."
E_DM0020_DMU_TOO_MANY:NO_GE
"No more than 255 DMU statements can be executed in a transaction."
E_DM0021_TABLES_TOO_MANY:NO_GE
"No more than 0x3fffffff tables can be created in a database."
E_DM002A_BAD_PARAMETER:NO_GE
"Bad parameter to DMF operation."
E_DM002B_BAD_RECORD_ID:NO_GE
"Illegal record identifier."
E_DM002C_BAD_SAVEPOINT_NAME:NO_GE
"Unknown savepoint name."
E_DM002D_BAD_SERVER_ID:NO_GE
"Illegal server identifier."
E_DM002E_BAD_SERVER_NAME:NO_GE
"Illegal server name."
E_DM002F_BAD_SESSION_ID:NO_GE
"Illegal session identifier."
E_DM0030_BAD_SESSION_NAME:NO_GE
"Illegal session name."
E_DM0039_BAD_TABLE_NAME:NO_GE
"Illegal table name."
E_DM003A_BAD_TABLE_OWNER:NO_GE
"Illegal table owner."
E_DM003B_BAD_TRAN_ID:NO_GE
"Illegal transaction identifier."
E_DM003C_BAD_TID:NO_GE
"Illegal tuple identifier."
E_DM003E_DB_ACCESS_CONFLICT:NO_GE
"Database access conflict."
E_DM003F_DB_OPEN:NO_GE
"Database is open."
E_DM0040_DB_OPEN_QUOTA_EXCEEDED:NO_GE
"Open database quota exceeded."
E_DM0041_DB_QUOTA_EXCEEDED:NO_GE
"Server database quota exceeded."
E_DM0042_DEADLOCK:NO_GE
"Resource deadlock."
E_DM0044_DELETED_TID:NO_GE
"Tuple at tid already deleted."
E_DM0045_DUPLICATE_KEY:NO_GE
"Attempt to insert duplicate key."
E_DM0046_DUPLICATE_RECORD:NO_GE
"Attempt to insert duplicate record."
E_DM0047_CHANGED_TUPLE:NO_GE
"Attempt to changed a record which has already been changed by the query."
E_DM004A_INTERNAL_ERROR:NO_GE
"Internal DMF error detected."
E_DM004B_LOCK_QUOTA_EXCEEDED:NO_GE
"Lock quota exceeded."
E_DM004C_LOCK_RESOURCE_BUSY:NO_GE
"Lock resource is currently busy."
E_DM004D_LOCK_TIMER_EXPIRED:NO_GE
"Lock timer expired before lock granted."
E_DM004F_LOCK_RETRY:NO_GE
"Lock is held by a run away server. 
Recover the transactions on the run away server and retry."
E_DM0050_TABLE_NOT_LOADABLE:NO_GE
"The table requested cannot be loaded: it is indexed, journaled, or not empty."
E_DM0051_NOT_ALLOWED_IN_MST:NO_GE
"The with recovery=false COPY option is not allowed in multi-statement transaction."
E_DM0053_NONEXISTENT_DB:NO_GE
"Nonexistent database."
E_DM0054_NONEXISTENT_TABLE:NO_GE
"Nonexistent table."
E_DM0055_NONEXT:NO_GE
"No more data satisfies request."
E_DM0056_NOPART:NO_GE
"NOPART btree error."
E_DM0057_NOROOM:NO_GE
"No space in current page."
E_DM0058_NOTIDP:NO_GE
"No tid pointer in index."
E_DM0059_NOT_ALL_KEYS:E_GE7D00_QUERY_ERROR
"Not all keys specified in modify of a index table."
E_DM005A_CANT_MOD_CORE_STRUCT:NO_GE
"The structure of the core system catalogs:\n
iirelation, iiattribute, iiindexes, ii_relidx, iidevices cannot be modified."
E_DM005B_SESSION_OPEN:NO_GE
"Session(s) are open."
E_DM005C_SESSION_QUOTA_EXCEEDED:NO_GE
"Server quota on sessions exceeded."
E_DM005D_TABLE_ACCESS_CONFLICT:NO_GE
"Table access conflict."
E_DM005E_CANT_UPDATE_SYSCAT:E_GE84D0_NO_PRIVILEGE
"User does not have enough privilege to change the core system catalogs:\n
	iirelation, iiattribute, iiindexes, iidevices."
E_DM005F_CANT_INDEX_CORE_SYSCAT:NO_GE
"User cannot place an index on the core system catalogs:\n
	iirelation, iiattribute, iiindexes, ii_relidx, iidevices."
E_DM0060_TRAN_IN_PROGRESS:NO_GE
"Transaction in progress."
E_DM0061_TRAN_NOT_IN_PROGRESS:NO_GE
"Transaction in not in progress."
E_DM0062_TRAN_QUOTA_EXCEEDED:NO_GE
"Session transaction quota exceeded."
E_DM0063_TRAN_TABLE_OPEN:NO_GE
"Table still open at end of transaction."
E_DM0064_USER_ABORT:NO_GE
"User abort forced DMF operation abort."
E_DM0065_USER_INTR:NO_GE
"User interrupt forced DMF operation abort."
E_DM0066_BAD_KEY_TYPE:E_GE7D00_QUERY_ERROR
"Key specified for modify or index operation is of a datatype which is either\n
non-keyable or non-sortable.  All key fields must be both keyable and sortable."
/*%
** Parameters:
**      None.
**
** System Status:
**      The index or modify operation is aborted.  Depending upon cursor state,
**	the transaction may be aborted.
**	 
**	This error occurs because one of the keys which has been requested of a
**	modify or index operation is not suitable for being a key.  This can
**	because either the datatype is marked as non-keyable or as non-sortable.
**	INGRES requires that any attribute used as a key be sortable, as sorts
**	are required as part of the modify and/or index operations.
**
** User Action:
**      Alter the query so that the non-keyable field is not used as a key to
**	the index or modify operation.
*/
E_DM006D_BAD_OPERATION_CODE:NO_GE
"Unrecognized DMF operation code."
E_DM006F_SERVER_ACTIVE:NO_GE
"Server is active."
E_DM0070_SERVER_STOPPED:NO_GE
"Server is stopped."
E_DM0071_LOCATIONS_TOO_MANY:NO_GE
"Number of locations exceeds the maximum allowed for this operation."
E_DM0072_NO_LOCATION:NO_GE
"Location list must be specified for this operation."
E_DM0073_RECORD_ACCESS_CONFLICT:NO_GE
"Attempted to insert or delete or replace record in a table open for read."
E_DM0074_NOT_POSITIONED:NO_GE
"Attempted to get/replace/delete next record in scan but 
record is not currently positioned."
E_DM0075_BAD_ATTRIBUTE_ENTRY:NO_GE
"Invalid attribute record read while attempting to open a table."
E_DM0076_BAD_INDEX_ENTRY:NO_GE
"Invalid index record read while attempting to open a table."
E_DM0077_BAD_TABLE_CREATE:NO_GE
"Could not create table."
E_DM0078_TABLE_EXISTS:NO_GE
"Table already exists."
E_DM0079_BAD_OWNER_NAME:NO_GE
"Invalid characters in owner name."
E_DM007C_BAD_KEY_DIRECTION:NO_GE
"Illegal key direction."
E_DM007D_BTREE_BAD_KEY_LENGTH:NO_GE
"Invalid key length for btree table."
E_DM007E_LOCATION_EXISTS:NO_GE
"Location already exists."
E_DM007F_ERROR_STARTING_SERVER:NO_GE
"Could not start the server due to an internal error."
E_DM0080_BAD_LOCATION_AREA:E_GE75BC_UNKNOWN_OBJECT
"Location Area specified is not valid."
E_DM0081_NO_LOGGING_SYSTEM:NO_GE
"The Logging System is not active on this node.  The server cannot be
brought up until the logging system is started."
E_DM0083_ERROR_STOPPING_SERVER:NO_GE
"Error trying to stop the server."
E_DM0084_ERROR_ADDING_DB:NO_GE
"Error trying to add database to server."
E_DM0085_ERROR_DELETING_DB:NO_GE
"Error deleting database from server."
E_DM0086_ERROR_OPENING_DB:NO_GE
"Error opening database in server."
E_DM0087_ERROR_CLOSING_DB:NO_GE
"Error closing database in server."
E_DM0088_ERROR_ALTERING_SERVER:NO_GE
"Error altering server."
E_DM0089_ERROR_SHOWING_SERVER:NO_GE
"Error Showing server characteristics."
E_DM008A_ERROR_GETTING_RECORD:NO_GE
"Error trying to get a record."
E_DM008B_ERROR_PUTTING_RECORD:NO_GE
"Error trying to put a record."
E_DM008C_ERROR_REPLACING_RECORD:NO_GE
"Error trying to replace a record."
E_DM008D_ERROR_DELETING_RECORD:NO_GE
"Error trying to delete a record."
E_DM008E_ERROR_POSITIONING:NO_GE
"Error trying to position a table."
E_DM008F_ERROR_OPENING_TABLE:NO_GE
"Error opening a table."
E_DM0090_ERROR_CLOSING_TABLE:NO_GE
"Error closing a table."
E_DM0091_ERROR_MODIFYING_TABLE:NO_GE
"Error modifying a table."
E_DM0092_ERROR_INDEXING_TABLE:NO_GE
"Error creating an index for a table."
E_DM0093_ERROR_SORTING:NO_GE
"Error sorting data."
E_DM0094_ERROR_BEGINNING_TRAN:NO_GE
"Error beginning a transaction."
E_DM0095_ERROR_COMMITING_TRAN:NO_GE
"Error committing a transaction."
E_DM0096_ERROR_ABORTING_TRAN:NO_GE
"Error aborting a transaction."
E_DM0097_ERROR_SAVEPOINTING:NO_GE
"Error creating a savepoint."
E_DM0098_NOJOIN:NO_GE
"Did not join two leafs during a BTREE merge."
E_DM0099_ERROR_RESUME_TRAN:NO_GE
"Error in resuming a transaction."
E_DM009A_ERROR_SECURE_TRAN:NO_GE
"Error in securing a transaction."
E_DM009B_ERROR_CHK_PATCH_TABLE:NO_GE
"Error Checking or Patching a Table."
E_DM009D_BAD_TABLE_DESTROY:NO_GE
"Error trying to destroy a table."
E_DM009E_CANT_OPEN_VIEW:NO_GE
"It is illegal to try to open a view, base table(s) must be opened."
E_DM009F_ILLEGAL_OPERATION:NO_GE
"DMF illegal operation for current state, such as trying to begin a
transaction when a transaction is already in progress."
E_DM00A0_UNKNOWN_COLLATION:NO_GE
"Could not open a database because collation sequence not installed."
E_DM00D0_LOCK_MANAGER_ERROR:NO_GE
"Error creating a server lock list."
E_DM00D1_BAD_SYSCAT_MOD:NO_GE
"Error trying to modify core system catalogs:\n
	iirelation, iiattribute or iiindexes."
E_DM0100_DB_INCONSISTENT:NO_GE
"Database is inconsistent."
E_DM0101_SET_JOURNAL_ON:E_GE0032_WARNING
"Journaling was turned on for the table created."
E_DM0102_NONEXISTENT_SP:NO_GE
"Error aborting to a nonexistent savepoint."
E_DM0103_TUPLE_TOO_WIDE:NO_GE
"Error trying to create a table with illegal tuple size."
E_DM0104_ERROR_RELOCATING_TABLE:NO_GE
"An error occurred trying to relocate a table."
E_DM0105_ERROR_BEGIN_SESSION:NO_GE
"An error occurred trying to begin a session."
E_DM0106_ERROR_ENDING_SESSION:NO_GE
"An error occurred trying to end a session."
E_DM0107_ERROR_ALTERING_SESSION:NO_GE
"An error occurred trying to alter session parameters."
E_DM0108_ERROR_DUMPING_DATA:NO_GE
"An error occurred trying to truncate the data of a temporary file."
E_DM0109_ERROR_LOADING_DATA:NO_GE
"An error occurred while trying to load data into a file."
E_DM010A_ERROR_ALTERING_TABLE:NO_GE
"An error occurred while alter the characteristics of a table."
E_DM010B_ERROR_SHOWING_TABLE:NO_GE
"An error occurred while showing information about a table."
E_DM010C_TRAN_ABORTED:NO_GE
"The transaction has been aborted by the logging system in order to reclaim 
log space."
E_DM010D_ERROR_ALTERING_DB:NO_GE
"An error occurred while altering the characteristics of a database."
E_DM010F_ISAM_BAD_KEY_LENGTH:NO_GE
"Invalid key length for isam table."
E_DM0110_COMP_BAD_KEY_LENGTH:NO_GE
"Invalid key length for compressed table."
E_DM0111_MOD_IDX_UNIQUE:NO_GE
"It is not allowed to modify a secondary index with non-unique keys
to a unique structure."
E_DM0112_RESOURCE_QUOTA_EXCEED:NO_GE
"Error allocating resource, resource limit exceeded."
E_DM0113_DB_INCOMPATABLE:NO_GE
"Database is incompatible with the current version of INGRES."
E_DM0114_FILE_NOT_FOUND:E_GE75BC_UNKNOWN_OBJECT
"Specified Table is missing underlying physical file."
E_DM0115_FCMULTIPLE:NO_GE
"Database must be added in SINGLE SERVER mode in order for FAST COMMIT
to be used."
E_DM0116_FAST_COMMIT:NO_GE
"An Error occurred in the DMF Fast Commit procedure - Server can no longer
support Fast Commit protocol."
E_DM0117_WRITE_BEHIND:NO_GE
"An error occurred in a session used for Asynchronous Write Behind.  Write
Behind session terminated."
E_DM0118_CANT_OPEN_GATEWAY:NO_GE
"Gateway tables do not have an underlaying data file in INGRES format.  DMF
cannot access this gateway table as thought it were a local table."
E_DM0119_TRAN_ID_NOT_UNIQUE:NO_GE
"The distributed transaction ID is not unique in the local system."
E_DM011A_ERROR_DB_DIR:NO_GE
"An error occurred trying to create database directory."
E_DM011B_ERROR_DB_CREATE:NO_GE
"An error occurred trying to create core database."
E_DM011C_ERROR_DB_DESTROY:NO_GE
"An error occurred trying to destroy database."
E_DM011D_ERROR_DB_ALTER:NO_GE
"An error occurred trying to alter state of a database."
E_DM011E_ERROR_ADD_LOC:NO_GE
"An error occurred trying to add a location to a database."
E_DM0120_DIS_TRAN_UNKNOWN:NO_GE
"The distributed transaction ID is not known to the local system."
E_DM0121_DIS_TRAN_OWNER:NO_GE
"The distributed transaction is owned by another server in the local system.\n
Disconnect the previous association and try to re-connect later."
E_DM0122_CANT_SECURE_IN_CLUSTER:NO_GE
"Can't support two-phase commit protocol in a cluster installation."
E_DM0123_BAD_CACHE_NAME:NO_GE
"Invalid Buffer Cache name specified."
E_DM0124_DB_EXISTS:NO_GE
"Database already exists."
E_DM0125_DDL_SECURITY_ERROR:NO_GE
"You cannot execute a DDL statement for a relation unless you
security label exactly equals the label of the relation."
E_DM0126_ERROR_WRITING_SECAUDIT:NO_GE
"An error occurred trying to write a security audit record."
E_DM0127_ERROR_SETTING_SECAUDIT:NO_GE
"An error occurred trying to update the security audit state."
E_DM0128_SECURITY_ATTR_BADNAME:NO_GE
"You have tried to create a table with an attribute named the
same as one of the system generated security attributes,
_ii_sec_label or _ii_sec_tabkey.  Please re-enter your create
table statement with attribute names which will not conflict
with these internally generated attributes."
E_DM0129_SECURITY_CANT_UPDATE:NO_GE
"You must have SECURITY privilege to update this system catalog."
E_DM0130_DMLIST_ERR:NO_GE
"An error occurred while attempting to list files in a directory"
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  DMF encountered an error during DMM_LIST request.  The error message
    **	indicating why dmm_list failed is logged and translated to this msg.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  none.
*/
E_DM0131_DMDEL_ERR:NO_GE
"An error occurred while attempting to delete a file."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  DMF encountered an error during DMM_LIST request.  The error message
    **	indicating why dmm_list failed is logged and translated to this msg.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  none.
*/
E_DM0132_ILLEGAL_STMT:NO_GE
"Illegal statement specified. Only ROLLBACK or COMMIT statement is allowed \n
immediately after the PREPARE TO COMMIT statement."
E_DM0133_ERROR_ACCESSING_DB:NO_GE
"Error accessing database due to invalid internal information. Access this \n
database from another server or shut this server down and start a new one."
E_DM0134_DIS_DB_UNKNOWN:NO_GE
"The database connected by the distributed transaction is unknown to the local system."
E_DM0135_GATEWAY_ERROR:E_GE9470_GATEWAY_ERROR
"Error returned from DMF Gateway."
/*%
** Parameters:
**	None
**
** Explanation:
**	An error was returned on a call to the DMF Gateway.
**	The error log should contain a previous error message giving the
**	exact nature of the error.
**
** System Status:
**	The current query will be aborted.  Everything else should be OK.
**
** User Action:
**	Check error log for exact nature of gateway error.  Make sure the
**	intended gateway is available from the DBMS server.
*/
E_DM0136_NO_GATEWAY:E_GE9470_GATEWAY_ERROR
"Server is not configured to access Gateway objects.  An attempt was made to
access a gateway object from a server not able to make gateway requests."
/*%
** Parameters:
**	None
**
** Explanation:
**	A table marked as a Gateway table (DMT_GATEWAY bit is set in the
**	iirelation.relstat field) was accessed, but the server is not
**	connected to any gateways.
**
** System Status:
**	System remains OK, but no access will be allowed to gateway tables.
**
** User Action:
**	Make sure the user is connected to the correct server to access the
**	requested data.  Make sure the server has been started up correctly.
*/
E_DM0137_GATEWAY_ACCESS_ERROR:E_GE9470_GATEWAY_ERROR
"An illegal operation was attempted on a gateway table.  An operation was
specified (Alter, Relocate, Reorganize) that is not allowed on gateway tables."
/*%
** Parameters:
**	None
**
** Explanation:
**	An operation was specified for a gateway table that cannot be
**	performed on foreign tables.  Such operations are:
**
**		Set [no]journaling
**		Modify to truncated
**		Modify to merge
**		Modify to reorganize
**		Relocate
**
**	These operations are allowed only on tables that are managed by
**	the Ingres database system.
**
** System Status:
**	Current query rejected.
**
** User Action:
**	These types of operations have no meaning to the Ingres system for
**	gateway tables.  The user may wish to alter the configuration of
**	the table in the foreign database - this should be done directly
**	by the foreign database manager.
*/
E_DM0138_TOO_MANY_PAGES:E_GE8CA0_SYSTEM_LIMIT
"An attempt was made to grow a file to a size larger than an INGRES TID can handle."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system will continue to operate.  An attempt was made to allocate a
**	new page in a file which would exceed the page number limit (23 bits)
**	imposed by INGRES TIDs.
**
**	The query attempting this will be aborted.
**
** User Action:
**      Contact the product vendor.  This is an internal limitation.  However,
**	for the short term, the only real choice is to somehow make the table
**	smaller.  Compression is a likely choice.
*/
E_DM0139_ERROR_ALTERING_RCB:NO_GE
"An error occurred attempting to alter record access information."
/*%
** Parameters:
**	None.
** Description:
**	DMF encountered an error during a DMR_ALTER call.  This probably
**	indicates a protocol error by the caller of DMF.
**
** System Status:
**	The statement is aborted.
** User Action:
**	None.
*/
E_DM013A_INDEX_COUNT_MISMATCH:NO_GE
"A DMT_SHOW request was made where the number of secondary indexes requested\n
did not match the actual number of indexes."
/*%
** Parameters:
**	None.
** Description:
**	The DMT_SHOW requestor must allocate memory to hold each secondary
**	index (iiindex) tuple they request.  The number of indexes that
**	really exist (determined from TCB) does not match the number that the
**	caller requested.  Rather than risk an AV by overflowing the allocated
**	memory, NO tuples are returned.
** System Status:
**	RDF will do recovery by indicating the table has no secondary indexes.
**	This will allow the query to continue, but the a less than optimal
**	query plan will be chosen.  However, QEF will find a timestamp
**	difference (since RDF's copy of the iirelation tuple is obviously
**	outdated), and will invalidate the query.  RDF should get current
**	iirelation info on the retry, and this error condition should not be
**	repeated.
** User Action:
**	None.
*/
E_DM013B_LOC_NOT_AUTHORIZED:E_GE84D0_NO_PRIVILEGE
"You either do not have permission to access this location, or the location is
not of the proper type."
/*%
** Parameters:
**	None.
**	
** Description:
**	An attempt was made to access some location which either has be denied
**	to the current session or is the improper type of location for the
**	operation being attempted.
**
**	This error should never be returned to the user; it is a DMF status
**	indicator which should be translated by the caller into a user error.
**	
** System Status:
**	The statement will be aborted.
**
** User Action:
**	This error should never be reported to the user; if it is, please
**	report this as a bug.
*/
E_DM013C_BAD_LOC_TYPE:NO_GE
"This location's type is incorrect for the attempted operation."
/*%
** Parameters:
**	None.
**	
** Description:
**	An attempt was made to perform some operation on a location of the
**	incorrect type.  Only locations of a given type are allowed to undergo
**	this operation.
**
**	This error should never be returned to the user; it is a DMF status
**	indicator which should be translated by the caller into a user error.
**	
** System Status:
**	The statement will be aborted.
**
** User Action:
**	This error should never be reported to the user; if it is, please
**	report this as a bug.
*/
E_DM013D_LOC_NOT_EXTENDED:NO_GE
"Database is not extended to this location."
/*%
** Parameters:
**	None.
**	
** Description:
**	The caller attempted an operation under the assumption that the
**	current database was extended to the given location; however upon
**	searching the config file, the extent was not found.
**
**	This error should never be returned to the user; it is a DMF status
**	indicator which should be translated by the caller into a user error.
**	
** System Status:
**	The statement will be aborted.
**
** User Action:
**	This error should never be reported to the user; if it is, please
**	report this as a bug.
*/
E_DM013E_TOO_FEW_LOCS:NO_GE
"There are currently no WORK locations being used for this session."
/*%
** Parameters:
**	None.
**	
** Description:
**	The caller attempted an operation which requires a minimum of 1
**	WORK location; this session has none.
**
**	This error should never be returned to the user; it is a DMF status
**	indicator which should be translated by the caller into a user error.
**	
** System Status:
**	The statement will be aborted.
**
** User Action:
**	This error should never be reported to the user; if it is, please
**	report this as a bug.
*/
E_DM013F_TOO_MANY_LOCS:NO_GE
"A maximum of 256 WORK locations can be used for this operation; an attempt was\
 made to use more than 256 WORK locations."
/*%
** Parameters:
**	None.
**	
** Description:
**	The caller supplied too many WORK locations for the attempted operation.
**	This operation supports at maximum of 256 WORK locations.
**
**	This error should never be returned to the user; it is a DMF status
**	indicator which should be translated by the caller into a user error.
**	
** System Status:
**	The statement will be aborted.
**
** User Action:
**	This error should never be reported to the user; if it is, please
**	report this as a bug.
*/
E_DM0140_EMPTY_DIRECTORY:NO_GE
"The specified data directory is empty."
/*%
** Parameters:
**	None.
**	
** Description:
**	The caller requested a dmm_list operation on an empty directory.
**	This is considered a recoverable error - so the user should never
**	see this message.
**
**	dmm_list() initializes error to this before calling DIlistfile.  If
**	DIlistfile returns with error still initialized to this value, then
**	we can deduce that the directory was emtpy.  It is not illegal to
**	have empty data directories (this can occur if you use accessdb to
**	extend a db to a new location, but have not yet put any tables in
**	that location.
**
**	dmm_list will translate this error back to a success status and
**	continue.
**	
** System Status:
**	The statement will continue.
** User Action:
**	This error should never be reported to the user; if it is, please
**	report this as a bug.
*/

E_DM0141_GWF_DATA_CVT_ERROR:NO_GE
"A data conversion error occurred while accessing Gateway information."

/*%
** Parameters:
**	None
**
** Description:
**	This error message is used internally by DMF to 'remember' that a
**	recoverable data conversion error occurred while processing a gateway
**	table. The actual data conversion error has already been reported to
**	the user. This error message is not reported to the user, nor is it
**	logged. It is used internally to record that no additional logging
**	of traceback information is needed while processing the data conversion
**	error.
**
** System Status:
**	The query will be aborted. Note that not all gateways support query
**	abort processing, so the query operations may or may not be undone.
**
** User Action:
**	This error should never be reported to the user, nor should it ever be
**	logged in the error log. If E_DM0141 is ever reported to a user or
**	logged to the error log, it should be considered an internal error and
**	reported to Ingres Technical Support.
*/

E_DM0142_GWF_ALREADY_REPORTED:NO_GE
"The Gateway Facility has already reported this error."

/*%
** Parameters:
**	None
**
** Description:
**	This error message is used internally by DMF to 'remember' that a
**	gateway facility call has returned a recoverable user error. The actual
**	error has already been reported to the user. This message is not
**	reported to the user, nor is it
**	logged. It is used internally to record that no additional logging
**	of traceback information is needed while processing the data conversion
**	error.
**
** System Status:
**	The query will be aborted. Note that not all gateways support query
**	abort processing, so the query operations may or may not be undone.
**
** User Action:
**	This error should never be reported to the user, nor should it ever be
**	logged in the error log. If E_DM0142 is ever reported to a user or
**	logged to the error log, it should be considered an internal error and
**	reported to Ingres Technical Support.
*/

E_DM1001_JSP_INIT:NO_GE
"An error occurred initializing the journal support program. Make sure program is
installed or you are running with proper privilege."
E_DM1002_JSP_MISS_ARGS:NO_GE
"Wrong number of arguments given.%!
dmfjsp operation [ operation-specific-arguments ]"
E_DM1003_JSP_UNRECOGNIZED:NO_GE
"Unrecognized command line flag."
E_DM1004_JSP_BAD_ARGUMENT:NO_GE
"Bad argument given."
E_DM1005_JSP_BAD_ARG_COMBO:NO_GE
"An inconsistent combination of arguments was given."
E_DM1006_JSP_TAPE_ONE_DB:NO_GE
"You can only checkpoint/rollforward one database at a time to/from tape."
E_DM1007_JSP_MEM_ALLOC:NO_GE
"Error allocating memory for the journal support program."
E_DM1008_CANT_OPEN_DB:NO_GE
"Error occurred trying to open the database."
E_DM1009_JSP_DBDB_OPEN:NO_GE
"Error occurred trying to open IIDBDB database."
E_DM100A_JSP_OPEN_IIUSER:NO_GE
"Error occurred trying to open IIUSER table."
E_DM100B_JSP_NOT_USER:NO_GE
"You are not a valid INGRES user."
E_DM100C_JSP_NOT_OVERUSER:NO_GE
"The user specified with -u flag is not a valid INGRES user."
E_DM100D_JSP_CLOSE_IIUSER:NO_GE
"Error occurred trying to close the IIUSER table. "
E_DM100E_JSP_NO_SUPERUSER:NO_GE
"You do not have superuser privilege."
E_DM100F_JSP_OPEN_IIDB:NO_GE
"Error occurred trying to open the IIDATABASE table."
E_DM1010_JSP_CLOSE_IIDB:NO_GE
"Error occurred trying to close the IIDATABASE table."
E_DM1011_JSP_DB_NOTFOUND:NO_GE
"The database specified was not found.  If no database name
was specified, there were no databases owned by you. "
E_DM1013_JSP_CLOSE_IILOC:NO_GE
"Error occurred trying to close the IILOCATION table. "
E_DM1014_JSP_DB_NOT_DB_OWNER:NO_GE
"You do not own the database specified."
E_DM1016_JSP_MUST_USE_TABLE:NO_GE
"Must specify a table using the -t flag when using the -f flag."
E_DM1017_JSP_II_DATABASE:NO_GE
"II_DATABASE must be defined for the INGRES LAR utilities to function."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES LAR utilities (ckpdb, rollforwarddb and auditdb)
    **	    can not function unless II_DATABASE is correctly defined.
    **	    In fact all of INGRES is dependent on these values being
    **	    properly defined.
    **	System Status:
    **	    Processing aborts.
    **	User Action:
    **	    Consult the INGRES Installation and Operations Guide.
    **	    
*/
E_DM1018_JSP_II_LOCATION:NO_GE
"The INGRES LAR utilities could not open the IILOCATION table in IIDBDB."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES LAR utilities (ckpdb, rollforwarddb and auditdb)
    **	    can not function unless II_LOCATION is available and consistent.
    **	System Status:
    **	    Processing aborts.
    **	User Action:
    **	    Consult your INGRES system administrator.
    **	    
*/
E_DM1019_JSP_NO_ATP_DB:NO_GE
"No database name passed to auditdb. Check the command line arguments."
E_DM1020_JSP_NO_DTP_DB:NO_GE
"No database name passed to dumpdb. Check the command line arguments."
E_DM1021_JSP_SEC_LAB_ERROR:NO_GE
"The INGRES LAR utilities encountered an operating system error trying
to determine the security label for this process."
/*%
**      Parameters:
**          None.
**      Explanation:
**          The INGRES LAR utilities (ckpdb, rollforwarddb and auditdb)
**          can not function in a B1 environment unless a security label
**          associated with this process can be obtained from the operating
**          system.
**      System Status:
**          Processing aborts.
**      User Action:
**          Consult your INGRES system administrator.
**
**	*/
E_DM1100_CPP_BAD_LOCATION:NO_GE
"Error checkpoint database, unusable location found."
E_DM1101_CPP_WRITE_ERROR:NO_GE
"Error writing checkpoint."
E_DM1102_CPP_FAILED:NO_GE
"Previous errors caused checkpoint to fail."
E_DM1103_CPP_CONFIG_UPDATE:NO_GE
"Error occurred updating configuration file."
E_DM1104_CPP_CKP_ERROR:NO_GE
"Error occurred taking checkpoint."
E_DM1105_CPP_CNF_OPEN:NO_GE
"Error occurred opening configuration file."
E_DM1106_CPP_JNL_CREATE:NO_GE
"Error occurred creating journal file."
E_DM1107_CPP_CATALOG_ERROR:NO_GE
"Error occurred updating system catalogs."
E_DM1108_CPP_OPEN_DB:NO_GE
"Error occurred trying to open the database."
E_DM1109_CPP_MEM_ALLOC:NO_GE
"Checkpoint requesting more memory than available."
E_DM110A_CPP_CLOSE_DB:NO_GE
"Error occurred closing the database."
E_DM110B_CPP_FAILED:NO_GE
"Error occurred checkpointing the database."
E_DM110C_CPP_CNF_CLOSE:NO_GE
"Error occurred closing the configuration file for the database."
E_DM110D_CPP_DB_BUSY:NO_GE
"Database is currently being used. Database lock conflict."
E_DM110E_CPP_FAIL_NO_JNL:NO_GE
"Error occurred trying to checkpoint the database, the database remains not journaled."
E_DM110F_CPP_FAIL_NO_CHANGE:NO_GE
"Error occurred trying to checkpoint the database, the database journal status remains unchanged." 
E_DM1110_CPP_DELETE_JNL:NO_GE
"Error deleting old journal files. "
E_DM1111_CPP_DELETE_CKP:NO_GE
"Error deleting old checkpoint files. "
E_DM1112_CPP_JNLCKP_NODEL:NO_GE
"Error deleting journal or checkpoint files. "
E_DM1113_CPP_NOARCHIVER:NO_GE
"Need to drain journal records from the log and the Archiver Process
does not exist.  Must restart the archiver."
E_DM1115_CPP_II_CHECKPOINT:NO_GE
"INGRES cannot function unless II_CHECKPOINT is properly defined."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES LAR utilities (ckpdb, rollforwarddb and auditdb)
    **	    can not function unless II_CHECKPOINT is correctly defined.
    **	    In fact all of INGRES is dependent on these values being
    **	    properly defined.
    **	System Status:
    **	    Processing aborts.
    **	User Action:
    **	    Consult the INGRES Installation and Operations Guide.
    **	    
*/
E_DM1116_CPP_II_JOURNAL:NO_GE
"INGRES cannot function unless II_JOURNAL is properly defined."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES LAR utilities (ckpdb, rollforwarddb and auditdb)
    **	    can not function unless II_JOURNAL is correctly defined.
    **	    In fact all of INGRES is dependent on these values being
    **	    properly defined.
    **	System Status:
    **	    Processing aborts.
    **	User Action:
    **	    Consult the INGRES Installation and Operations Guide.
    **	    
*/
E_DM1117_CPP_II_DATABASE:NO_GE
"INGRES cannot function unless II_DATABASE is properly defined."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES LAR utilities (ckpdb, rollforwarddb and auditdb)
    **	    can not function unless II_DATABASE is correctly defined.
    **	    In fact all of INGRES is dependent on these values being
    **	    properly defined.
    **	System Status:
    **	    Processing aborts.
    **	User Action:
    **	    Consult the INGRES Installation and Operations Guide.
    **	    
*/
E_DM1118_CPP_JNL_OPEN:NO_GE
"Error opening journal file while checkpointing the database."
E_DM1119_CPP_JNL_TRUNCATE:NO_GE
"Error truncating journal file while checkpointing the database."
E_DM1120_CPP_JNL_CLOSE:NO_GE
"Error closing journal file while checkpointing the database."
E_DM1121_CPP_BEGIN_ERROR:NO_GE
"Error starting checkpoint sequence."
E_DM1122_CPP_END_ERROR:NO_GE
"Error ending checkpoint sequence."
E_DM1123_CPP_NOCLUSTER:NO_GE
"Error requesting not supported cluster feature."
E_DM1124_CPP_SBACKUP:NO_GE
"Error occurred starting online backup.  A common cause of this error is an
attempt to start multiple online backups at the same time.  Only a single 
online backup on a database may be active at a given time."
E_DM1125_CPP_LGBGN_ERROR:NO_GE
"Error occurred starting a transaction."
E_DM1126_CPP_DM0LSBACKUP:NO_GE
"Error occurred writing start backup log record."
E_DM1127_CPP_LGEND_ERROR:NO_GE
"Error occurred ending a transaction."
E_DM1128_CPP_RESUME:NO_GE
"Error occurred requesting LG to resume stalled transactions."
E_DM1129_CPP_DM0LEBACKUP:NO_GE
"Error occurred writing end backup log record."
E_DM1130_CPP_EBACKUP:NO_GE
"Error occurred requesting LG the end of online backup."
E_DM1131_CPP_LK_FAIL:NO_GE
"Error occurred requesting LK services."
E_DM1132_CPP_FAIL_NO_DMP:NO_GE
"Error occurred trying to checkpoint the database, the database remains not dumped."
E_DM1133_CPP_II_DUMP:NO_GE
"Error not defining ii_dump logical."
E_DM1134_CPP_DELETE_DMP:NO_GE
"Error occurred deleting dump file."
E_DM1135_CPP_UPD2_JNLDMP:NO_GE
"Error occurred updating phase two of config file."
E_DM1137_CPP_FBACKUP:NO_GE
"Error occurred requesting LG the completion of archiver."
E_DM1139_CPP_DBAKCUP:NO_GE
"Error occurred requesting LG to remove backup status from context."
E_DM1140_CPP_UPD1_JNLDMP:NO_GE
"Error occurred updating phase one of config file."
E_DM1141_CPP_DMP_OPEN:NO_GE
"Error occurred opening the dump file."
E_DM1143_CPP_DMP_CLOSE:NO_GE
"Error occurred closing the dump file."
E_DM1144_CPP_DEAD_ARCHIVER:NO_GE
"Error occurred during online ckpdb.  The INGRES archiver failed for some reason
causing the online ckpdb to fail.  Check the II_ACP.LOG file in the INGRES files
directory for information concerning the archiver failure."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when the INGRES
    **	    archiver (dmfacp) failed.  The online checkpoint facility depends
    ** 	    on the archiver to successfully function, in order to guarantee a
    **	    consistent checkpoint of the database.  This error occurred early
    **	    in the online checkpoint process.
    **
    **	    Consult both the "errlog.log" and the "II_ACP.LOG" file for more
    **	    information concerning the cause of the failures.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    After successfully restarting the archiver process (dmfacp), restart
    **	    the online checkpoint.
    **	    
*/
E_DM1145_CPP_DEAD_ARCHIVER:NO_GE
"Error occurred during online ckpdb.  The INGRES archiver failed for some reason
causing the online ckpdb to fail.  Check the II_ACP.LOG file in the INGRES files
directory for information concerning the archiver failure."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when the INGRES
    **	    archiver (dmfacp) failed.  The online checkpoint facility depends
    ** 	    on the archiver to successfully function, in order to guarantee a
    **	    consistent checkpoint of the database.  This error occurred early
    **	    in the online checkpoint process.
    **
    **	    Consult both the "errlog.log" and the "II_ACP.LOG" file for more
    **	    information concerning the cause of the failures.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    After successfully restarting the archiver process (dmfacp), restart
    **	    the online checkpoint.
    **	    
*/
E_DM1146_CPP_DEAD_ARCHIVER:NO_GE
"Error occurred during online ckpdb.  The INGRES archiver failed for some reason
causing the online ckpdb to fail.  Check the II_ACP.LOG file in the INGRES files
directory for information concerning the archiver failure."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when the INGRES
    **	    archiver (dmfacp) failed.  The online checkpoint facility depends
    ** 	    on the archiver to successfully function, in order to guarantee a
    **	    consistent checkpoint of the database.
    **
    **	    Consult both the "errlog.log" and the "II_ACP.LOG" file for more
    **	    information concerning the cause of the failures.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    After successfully restarting the archiver process (dmfacp), restart
    **	    the online checkpoint.
    **	    
*/
E_DM1147_CPP_DEAD_ARCHIVER:NO_GE
"Error occurred during online ckpdb.  The INGRES archiver failed for some reason
causing the online ckpdb to fail.  Check the II_ACP.LOG file in the INGRES files
directory for information concerning the archiver failure."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when the INGRES
    **	    archiver (dmfacp) failed.  The online checkpoint facility depends
    ** 	    on the archiver to successfully function, in order to guarantee a
    **	    consistent checkpoint of the database.  This error occurred early
    **	    in the online checkpoint process.
    **
    **	    Consult both the "errlog.log" and the "II_ACP.LOG" file for more
    **	    information concerning the cause of the failures.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    After successfully restarting the archiver process (dmfacp), restart
    **	    the online checkpoint.
    **	    
*/
E_DM1148_CPP_CNF_DUMP:NO_GE
"Error occurred saving config file during online checkpoint.  The checkpoint
process could not save a copy of the database config file in the DUMP directory
as part of the checkpoint."
/*%
    **	Parameters:
    **      None
    **	Explanation:
    **	    The INGRES online checkpoint facility saves a copy of the database
    **	    config file in the database DUMP directory (II_DUMP).  This
    **	    error is raised when it is unable to do so.
    **
    **	    This message should be preceded by a more detailed error message
    **	    which gives more information on the cause of the error.
    **
    **	    This error could be caused by the system being out of disk space
    **	    on the DUMP device.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    Check previous error messages for cause of the error.  After
    **	    correcting the problem, restart online checkpoint.  If problem
    **	    cannot be resolved, try using OFFLINE checkpoint (-l flag).
    **	    
*/
E_DM1149_CPP_BACKUP_FAILURE:NO_GE
"Error occurred during online ckpdb.  The INGRES archiver encountered an error
(most probably ran out of disk space on the II_DUMP disk) while processing
records associated with the current online checkpoint.  This checkpoint on
database %0c has been aborted.
Check the II_ACP.LOG file in the INGRES files
directory for information concerning the error which the archiver encountered."
/*%
    **	Parameters:
    **	    %0c		database name
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when the INGRES
    **	    archiver (dmfacp) failed.  The online checkpoint facility depends
    ** 	    on the archiver to successfully function, in order to guarantee a
    **	    consistent checkpoint of the database.  This error occurred early
    **	    in the online checkpoint process.
    **
    **	    Consult both the "errlog.log" and the "II_ACP.LOG" file for more
    **	    information concerning the cause of the failures.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    After successfully restarting the archiver process (dmfacp), restart
    **	    the online checkpoint.
    **	    
*/
E_DM1150_CPP_BACKUP_FAILURE:NO_GE
"Error occurred during online ckpdb.  The INGRES archiver encountered an error
(most probably ran out of disk space on the II_DUMP disk) while processing
records associated with the current online checkpoint.  This checkpoint
has been aborted.
Check the II_ACP.LOG file in the INGRES files
directory for information concerning the error which the archiver encountered."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when the INGRES
    **	    archiver (dmfacp) failed.  The online checkpoint facility depends
    ** 	    on the archiver to successfully function, in order to guarantee a
    **	    consistent checkpoint of the database.  This error occurred early
    **	    in the online checkpoint process.
    **
    **	    Consult both the "errlog.log" and the "II_ACP.LOG" file for more
    **	    information concerning the cause of the failures.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **      Look for additional error information in II_ACP.LOG.  After 
    **	    resolving these errors (usually out of disk space), restart the
    **	    online checkpoint of the database.
    **	    
*/
E_DM1151_CPP_CPNEEDED:NO_GE
"Error occurred signaling a Consistency Point during Checkpoint processing."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when it could not
    **	    successfully execute a Consistency Point.
    **
    **	    At some points during the backup, the checkpoint facility must
    **	    signal a consistency point and then wait for that consistency
    **	    point to complete.  This is generally in order to form a complete
    **	    archive window on which the ACP can then do archive processing.
    **
    **	    If the Checkpoint process gets an error signaling or waiting for
    **	    the consistency point, then this error will be raised.  There 
    **	    should be previous error messages giving more detail about what
    **	    went wrong.
    **	    
    **	System Status:
    **	    Checkpoint processing aborts.
    **	User Action:
    **      Look for additional error information in ERRLOG.LOG.  After 
    **	    resolving these errors, restart the checkpoint of the database.
    **	    
*/
E_DM1152_CPP_ONLINE_PURGE:NO_GE
"Error occurred signaling a Purge during Online Checkpoint processing."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when it could not
    **	    successfully execute an Online Purge.
    **
    **	    During an Online Checkpoint, the checkpoint facility must
    **	    signal the Archiver to execute an Online Purge of the database
    **	    being backed-up.
    **
    **	    If the Checkpoint process gets an error signaling or waiting for
    **	    the purge, then this error will be raised.  There should be
    **	    previous error messages giving more detail about what went wrong.
    **	    
    **	System Status:
    **	    Checkpoint processing aborts.
    **	User Action:
    **      Look for additional error information in ERRLOG.LOG.  After 
    **	    resolving these errors, restart the checkpoint of the database.
    **	    
*/
E_DM1153_CPP_CLUSTER_MERGE:NO_GE
"Error occurred merging cluster journal files during Checkpoint processing."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when it could not
    **	    successfully merge old journal files in a cluster installation.
    **
    **	    When a checkpoint is performed in a cluster installation, we must
    **	    take all the old journal files created for each node, and merge
    **	    the records into a single journal file.
    **
    **	    This is done by the checkpoint process.  If this operation fails,
    **	    then this error is raised and the checkpoint aborts.
    **
    **	    There should be previous error messages describing in more detail
    **	    the exact nature of the problem.
    **	    
    **	System Status:
    **	    Checkpoint processing aborts.
    **	User Action:
    **      Look for additional error information in ERRLOG.LOG.  After 
    **	    resolving these errors, restart the checkpoint of the database.
    **	    
*/
E_DM1154_CPP_CLUSTER_BUSY:NO_GE
"The online checkpoint failed because other nodes of the Ingres cluster
had the database %0c open.  Only the node which is executing the checkpoint may
have sessions with the database open.  The nodes which had the database
open are: (%2c) (%1x)"
/*%
    **	Parameters:
    **	    0c	- database name
    **	    1x	- bitmask of node id's
    **	    2c	- formatted list of node names
    **
    **	Explanation:
    **	    When online checkpoint is run in a cluster installation, the
    **	    database can only be open on the node which is executing the
    **	    checkpoint.  No other node can have the database open at the
    **	    time the checkpoint starts.
    **
    **	    This is so journal records can be flushed to the journal files
    **	    before the checkpoint begins.
    **
    **	    The database must be closed on all other nodes of the cluster
    **	    before the checkpoint can begin.
    **	    
    **	System Status:
    **	    Checkpoint processing aborts.
    **	User Action:
    **	    Close the database on all nodes except the one on which the
    **	    checkpoint is being executed.  Then restart the online checkpoint.
    **	    
*/

E_DM1155_CPP_TMPL_MISSING:NO_GE
"The checkpoint template definitions file could not be located.\n
This file contains information which is used by the customizable backup
routines to determine which checkpoint or rollforward processing will be done.\n
The name of this file is 'cktmpl.def' and it should be found in the
'II_CONFIG' directory.\n
If 'II_CKTMPL_FILE' was used to specify the template file location, check the
value for typographical errors.\n
Otherwise, ensure that the template file is in the correct location and
restart the checkpoint."

/*%
** Parameters:
**	None
**
** Description:
**	Individual installations will in future releases be able to customize
**	the behavior of the checkpoint and restore operations by specifying
**	the contents of checkpoint template file in the II_CONFIG area. This
**	file, whose name is 'cktmpl.def', could not be found.
**
**	When the checkpoint template file cannot be found, checkpoint and
**	rollforward cannot be run. The file may be missing because the
**	installation of Ingres encountered errors, or because the file was
**	accidentally deleted or renamed.
**
**	A logical name may be provided to explicitly specify the location of
**	the checkpoint template file, if it is not in the standard location.
**	This logical name, which is 'II_CKTMPL_FILE', may be
**	set to the fully-qualified filename of the checkpoint template file.
**	
** System Status
**	The checkpoint is halted, and this error is displayed. Additional
**	error messages are logged in the standard Ingres error log.
**
** User Action:
**	If the logical name override was used to explicitly specify the location
**	of the checkpoint template file, ensure that the value of the logical
**	name is correct and that the file is located in the indicated location.
**
**	If the logical name override was NOT used, ensure that the file is
**	present in the standard location.
**
**	Check the file permissions to verify that the file can be read by the
**	user who is running the checkpoint or rollforward.
*/

E_DM1200_ATP_DB_NOT_JNL:NO_GE
"This database is not journaled, thus cannot be audited."
E_DM1201_ATP_WRITING_AUDIT:NO_GE
"Error occurred writing audit trail."
E_DM1202_ATP_JNL_OPEN:NO_GE
"Error occurred opening a journal file."
E_DM1203_ATP_JNL_READ:NO_GE
"Error occurred reading a journal file."
E_DM1204_ATP_JNL_CLOSE:NO_GE
"Error occurred closing a journal file."
E_DM1205_ATP_NO_BT_FOUND:NO_GE
"Audit records found for a unknown transaction."
E_DM1206_ATP_TOO_MANY_TX:NO_GE
"Too many simultaneous transactions."
E_DM1207_ATP_NON_DB:NO_GE
"Nonexistent database."
E_DM1208_ATP_OPEN_DB:NO_GE
"Error occurred opening the database."
E_DM1209_ATP_CNF_OPEN:NO_GE
"Error occurred opening the configuration file."
E_DM120A_ATP_CNF_CLOSE:NO_GE
"Error occurred closing the configuration file."
E_DM120B_ATP_CATALOG_ERROR:NO_GE
"Error occurred retrieving catalog information."
E_DM120C_ATP_ADF_ERROR:NO_GE
"Unexpected ADF runtime error."
E_DM120D_ATP_DESCRIBE:NO_GE
"Error occurred retrieving table description."
E_DM120E_ATP_DB_BUSY:NO_GE
"Can't audit database at this time.
    Database is currently being used by an exclusive (SOLE_SERVER) server."
E_DM120F_ATP_TBL_NOTFOUND:NO_GE
"Attempting to audit for a nonexistent table."
E_DM1210_ATP_BAD_TRL_CREATE:NO_GE
"Error creating Audit Trail Fail.  An error was returned attempting to
create the file 'audit.trl' in the current user directory.  Check permission
to create files in the current directory and for available disk space."
/*%
** Parameters:
**      None.
**
** Explanation:
**      An error was encountered creating the file 'audit.trl' in the
**	current directory.  This file is created when the audit-file
**	option is specified for Auditdb.  This file will hold all of
**	the audit trail output.
**
**	This error probably indicates file create permissions problems,
**	but could also be caused by disk resource limitations.
**
** System Status:
**	The Auditdb process will not start.
**
** User Action:
**	Check for permissions to create a file in the current directory.
**	Check for sufficient disk space.
*/
E_DM1211_ATP_MEM_ALLOCATE:NO_GE
"Error allocating memory for Auditdb process."
/*%
** Parameters:
**      None.
**
** Explanation:
**	An error was encountered allocating memory during startup of the
**	Ingres Auditdb process.  This is probably caused by insufficient
**	memory resources.
**
** System Status:
**	The Auditdb process will not start.
**
** User Action:
**	Check for previous errors which indicate the amount of memory
**	which was requested.  Check process quotas, if any.  Check system 
**	memory usage.
*/
E_DM1212_ATP_DISPLAY:NO_GE
"Error writing display record during Auditdb processing."
/*%
** Parameters:
**      None.
**
** Explanation:
**	An error was encountered in the routine which formats and displays
**	journal records for audit processing.  A previous error message
**	should give more specific details about the display error.
**
**	This error could be caused by display format errors or by the
**	inability of the system to allocate a buffer large enough to
**	hold the row.
**
** System Status:
**	Auditdb will skip the row which it cannot display and go on to the
**	next record.  It is possible that this error could continue to
**	be displayed for each record in the audit trail if the problem
**	is not specific to a single record.
**
** User Action:
**	Check for previously logged errors which give further detail on
**	the problem.  If caused by a memory problem, check for process
**	memory quotas.
*/
E_DM1213_ATP_COLUMN_DISPLAY:NO_GE
"Error converting column number %0d into its display format during audit 
processing.  This column will not be displayed."
/*%
** Parameters:
**	%d		- column number of bad data value.
**      None.
**
** Explanation:
**	An error was encountered converting a data value into its
**	display format (VARCHAR) during auditdb processing.
**
**	When displaying records in an audit trail, auditdb calls
**	ADF to convert each column datatype to VARCHAR so that it
**	can be displayed.  If an error is returned from the conversion,
**	then the column cannot be displayed.
**
**	This error probably indicates a bad or unknown data value.
**
**	If the column is of a User-Defined-Datatype, check the user routine
**	which converts the datatype to VARCHAR format.
**
** System Status:
**	The indicated column will not be displayed.  Audit processing will
**	continue.
**
** User Action:
**	Check for bad data values in the indicated table/column.
**	Check for previous error messages giving more detail about
**	the nature of the conversion error.
*/
E_DM1214_ATP_DISPLAY_OVFL:NO_GE
"Error during audit trail processing.  A row could not be displayed completely
as its ascii-formatted output exceeded the size of the display buffer (16,000 
bytes).  The row will be truncated for display."
/*%
** Parameters:
**      None.
**
** Explanation:
**	During audit processing, a row was encountered which when each
**	column was converted to varchar format could not be displayed in
**	a 16,000 byte display buffer.
**
**	Rows larger than 16,000 bytes cannot be displayed in audit trails.
**
** System Status:
**	The row will be truncated for display.  Audit processing will
**	continue.
**
** User Action:
**	Check previous error messages for conversion errors.
**	Check if row contains User-Defined datatypes which require large
**	amounts of space when converted to ascii format.
*/
E_DM1300_RFP_OPEN_DB:NO_GE
"Error occurred opening the database."
E_DM1301_RFP_ROLL_FORWARD:NO_GE
"An error occurred that precludes further processing."
E_DM1302_RFP_CNF_OPEN:NO_GE
"Error occurred opening the configuration file."
E_DM1303_RFP_CNF_CLOSE:NO_GE
"Error occurred closing the configuration file."
E_DM1304_RFP_MEM_ALLOC:NO_GE
"Error occurred allocating memory."
E_DM1305_RFP_JNL_OPEN:NO_GE
"Error occurred opening journal file."
E_DM1306_RFP_APPLY_RECORD:NO_GE
"Error occurred attempting to apply roll forward record."
E_DM1307_RESTORE:NO_GE
"Error occurred restoring checkpoint."
E_DM1308_RFP_BAD_RECORD:NO_GE
"Bad record type found in journal."
E_DM1309_RFP_RFC_DELETE:NO_GE
"Error deleting the temporary configuration file aaaaaaaa.rfc during restoration."
E_DM130A_RFP_NO_CNFRFC	:NO_GE
"Error occurred restoring the checkpoint, no aaaaaaaa.cnf or aaaaaaaa.rfc
configuration file exists for the database."
E_DM130B_RFP_RENAME_CNF	:NO_GE
"Error occurred renaming the configuration file."
E_DM130C_RFP_DELETE_ALL	:NO_GE
"Error deleting files from directory before restoring from checkpoint."
E_DM130D_RFP_BAD_RESTORE:NO_GE
"Error occurred trying to restore the checkpoint."
E_DM130F_RFP_RESTORED:NO_GE
"The checkpoint has been successfully restored." 
E_DM1310_RFP_CLOSE_DB:NO_GE
"Error occurred trying to close the database."
E_DM1311_RFP_MUST_RESTORE:NO_GE
"The database has not been restored, can't rollforward only from journals."
E_DM1312_RFP_JNL_CREATE:NO_GE
"Error creating the new journal file. "
E_DM1313_RFP_CKP_HISTORY:NO_GE
"Error updating checkpoint history."
E_DM1314_RFP_DRAIN_ERROR:NO_GE
"Error occurred while records were being journaled from the logfile."
E_DM1315_RFP_DB_BUSY:NO_GE
"Database is currently being used. Database lock conflict."
E_DM1316_RFP_NO_CHECKPOINT:NO_GE
"Database has not been checkpointed. Cannot rollforward."
E_DM1317_RFP_CKP_MISSING:NO_GE
"One of the Checkpoint files for this database is missing. Cannot rollforward."
E_DM1318_RFP_NO_CKP_FILE:NO_GE
"The Checkpoint file %1c
for database %0c
at location:%2c does not exist."
E_DM1319_RFP_NO_BT_FOUND:NO_GE
"No begin transaction record found for current journal record."
E_DM1320_RFP_TOO_MANY_TX:NO_GE
"Error allocating new transaction queue entry in rollforward."
E_DM1321_RFP_BEGIN_ERROR:NO_GE
"Error starting rollforward sequence."
E_DM1322_RFP_END_ERROR:NO_GE
"Error ending rollforward sequence."
E_DM1324_RFP_DMP_OPEN:NO_GE
"Error occurred opening dump file."
E_DM1325_RFP_APPLY_DRECORD:NO_GE
"Error occurred applying the dump log record."
E_DM1326_RFP_BAD_DRECORD:NO_GE
"Error occurred finding invalid dump log record."
E_DM1327_RFP_INV_CHECKPOINT:NO_GE
"Error trying to apply incomplete checkpoint."
E_DM1328_DTP_DB_NOT_DMP:NO_GE
"Database not dumped."
E_DM1329_DTP_WRITING_AUDIT:NO_GE
"Error occurred writing audit."
E_DM132A_DTP_DMP_OPEN:NO_GE
"Error occurred opening the dump file."
E_DM132B_DTP_DMP_READ:NO_GE
"Error occurred reading the dump file."
E_DM132C_DTP_DMP_CLOSE:NO_GE
"Error occurred closing the dump file."
E_DM132F_DTP_NON_DB:NO_GE
"Non-existent database."
E_DM1330_DTP_OPEN_DB:NO_GE
"Error occurred opening the database."
E_DM1331_DTP_CNF_OPEN:NO_GE
"Error occurred opening the configuration file."
E_DM1332_DTP_CNF_CLOSE:NO_GE
"Error occurred closing the configuration file."
E_DM1333_DTP_CATALOG_ERROR:NO_GE
"Error occurred retrieving catalog information."
E_DM1334_DTP_ADF_ERROR:NO_GE
"Unexpected ADF runtime error."
E_DM1335_DTP_DESCRIBE:NO_GE
"Error occurred retrieving table description."
E_DM1336_DTP_DB_BUSY:NO_GE
"Can't audit database at this time.
    Database is currently being used by an exclusive (/SOLE) server."
E_DM1337_DTP_TBL_NOTFOUND:NO_GE
"Attempting to audit for a nonexistent table."
E_DM1339_RFP_DMP_READ:NO_GE
"Error occurred reading dump file."
E_DM133A_RFP_DMP_SCAN:NO_GE
"Error occurred scanning dump file."
E_DM133B_RFP_DUMP_CONFIG:NO_GE
"Error occurred restoring config file during ROLLFORWARD.  The database config
file could not be copied from the DUMP directory."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    During an online checkpoint, INGRES writes a copy of the config
    **	    file to the database DUMP directory (II_DUMP).  When restoring
    **	    to that checkpoint, the config file is overwritten with the
    **	    contents in the DUMP copy.
    **
    **	    This error is raised when ROLLFORWARD is unable to restore the
    **	    config file using the DUMP copy.  This may occur because of IO
    **	    errors, permission errors, or if the config copy is missing.
    **
    **	    The DUMP copy of the config file will be named CNNNNNN.DMP where
    **	    NNNNNN is the checkpoint sequence number.
    **
    **	    This message should be preceded by more detailed error messages
    **	    giving the exact nature of the error.
    **
    **	System Status:
    **	    Rollforward processing aborts.
    **	User Action:
    **	    Check previous error messages to determine if problem can be
    **	    resolved by fixing system condition (freeing disk space, bringing
    **	    disk volume on-line ...).  If problem seems to be that config file
    **	    copy is corrupted, then the checkpoint may not be usable.  It is
    **	    possible that using a different copy of the config file will
    **	    resolve the problem.
    **	    
*/

E_DM133C_RFP_UNUSABLE_CKP:NO_GE
"The indicated checkpoint has been marked INVALID. A checkpoint may
be marked INVALID if the backup operation failed due to a disk or
tape error (such as disk full or tape write error). Additionally,
an online checkpoint may be marked invalid if the Archiver was
unable to copy the journal and dump file information to the journal
and dump directories (check the Archiver error log for messages).
You can restore your database from the most recent valid checkpoint
by issuing the rollforwarddb command with the '#c' argument. In a
future release this action will be taken automatically."

/*%
** Parameters:
**	None
**
** Explanation:
**	This message is issued by the rollforwarddb utility when the checkpoint
**	which was to be used is discovered to be unusable. In order for a
**	checkpoint to be usuable for rolling forward a database, the backup
**	operation must have successfully written all the locations which
**	comprise the database to the backup area (whether disk or tape).
**
**	Additionally, if the checkpoint was an Online Checkpoint, the archiver
**	must have successfully copied all the transaction and backup information
**	from the Ingres Log File to the Journal and Dump files associated with
**	this checkpoint.
**
**	If any or all of these operations fail, the checkpoint is marked as
**	INVALID and cannot be used for rolling forward the database.
**
** System Status:
**	The rollforwarddb operation is aborted. The database is unaltered. No
**	action has been taken on the database.
**
** User Action:
**	Depending on what has been done with this database in the past, there
**	may be a valid checkpoint earlier in the history of this database. The
**	'infodb' utility may be used to display the checkpoint history of
**	this database. If a previous checkpoint is marked as valid, the database
**	can be recovered by rolling forward from the previous checkpoint.
**
**	You can restore your database from the most recent valid checkpoint by
**	re-issuing the rollforwarddb command with the '#c' argument.
**
**	If the '#c' argument is used, the database will be rolled back to the
**	most recent valid checkpoint, and then the database will be rolled
**	forward by re-applying all transactions which have been executed since
**	the checkpoint was taken. The transaction information is kept in the
**	journal files.
*/

E_DM2001_SAU_OK:NO_GE
"Security Audit Program Completed Successfully, :%0d records matched the
search criteria."
E_DM2002_SAU_INIT:NO_GE
"An error occurred initializing the security audit utility program.
Make sure the program is installed and your are running with proper
privilege."
E_DM2003_SAU_INTERNAL_ERROR:NO_GE
"The internal state of the security audit program has been corrupted.
Please save all information and report the problem as a bug to
Ingres Corporation."
E_DM2004_SAU_BAD_ARGUMENT:NO_GE
"Bad argument given."
E_DM2005_SAU_COMMAND_ONLY:NO_GE
"If you have specified the '-a' flag to issue a command to the
audit process, you cannot specify any other command line flags except
the '-f' flag with the CHANGEFILE command."
E_DM2006_SAU_NEED_FILE:NO_GE
"You have specified the CHANGEFILE command with the '-a' flag, but
have not provided the filename for the new file.  Re-issue the command
with both the '-a' and '-f' flags specified."
E_DM2007_SAU_SAF_BAD_OPEN:NO_GE
"An error occurred trying to open the security audit file %0c.
Check to make sure this file exists and is accessible by you.  The
file must be on disk, this program cannot read a file from tape.
The file must be closed.  If you want to access the file currently
open by the security audit process, then use AUDTISEC to switch to
a new file, so the current file can be audited."
E_DM2008_SAU_SAF_READ_ERROR:NO_GE
"An error occurred trying to read the security audit file %0c.
Make sure you have privilege to read this file and that the
file is not currently open by another process.  If the audit
process is running and has this file open, you must tell the
audit process to switch to another file by issuing the CHANGEFILE
command."
E_DM2009_SUA_SAF_BAD_RECORD:NO_GE
"The data in the security audit file %0c is corrupted.  Keep
this file along with your configuration information and report this
as a bug to Ingres Corporation."
E_DM200A_SAU_SAF_BAD_CLOSE:NO_GE
"An error occurred trying to close the security audit file %0c."
E_DM200B_SAU_BAD_EVENT:NO_GE
"Event type %0c is not a valid event.  Please re-enter 'eventlist'
parameters using valid event types."
E_DM200C_SAU_BAD_ACCESS_FORMAT:NO_GE
"The access parameter %0c is incorrectly specified.  Check the
documentation for AUDITSEC and re-enter with valid information."
E_DM200D_SAU_BAD_ACCESS_TYPE:NO_GE
"Access type %0c is not a valid access type.  Please re-enter 'accesslist'
parameters using valid access types."
E_DM200E_SAU_ACCESS_MISMATCH:NO_GE
"The command %0c has an illegal access type for the event specified."
E_DM200F_SAU_CNF_OPEN_ERROR:NO_GE
"An error occurred trying to open the security audit configuration
file %0c.  This file contains the name of the current security audit
file.  If this file does not exist or cannot be accessed you must
use the '-f' flag to specify which file to read."
E_DM2010_SAU_CNF_READ_ERROR:NO_GE
"An error occurred trying to open the security audit configuration
file %0c.  This file contains the name of the current security audit
file.  If this file does not exist or cannot be accessed you must
use the '-f' flag to specify which file to read."
E_DM2011_SAU_CNF_BAD_DATA:NO_GE
"The record in the security audit configuration file %oc is bad.
This file should contain the name of the current security audit
file.  If this file does not exist or cannot be accessed you must
use the '-f' flag to specify which file to read."
E_DM2012_SAU_DB_CONNECT:NO_GE
"In order to convert from a group name to a list of users within the
group, a connection must be made to the iidbdb database to
read the system relation containing group information.  A connection
request to the iidbdb was denied.  You can re-enter your command
without the '-g' flag to circumvent this problem."
E_DM2013_SAU_BAD_GROUP:NO_GE
"While trying to convert the group specified by you '-g' flag to
a list of users, an error was encountered accessing the system
catalog iiusergroup.  Either this group does not exist or has no
users."
E_DM2014_SAU_NO_MEM:NO_GE
"This program could not allocate enough memory to execute properly.
Check your process environment to insure you have sufficient resources
to execute this program."
E_DM2015_SAU_COMMAND_ERROR:NO_GE
"Command type %0c is not a valid command.  Please re-enter the '-a'
flag using a valid command."
E_DM2016_SAU_SEND_ERROR:NO_GE
"An error occurred trying to issue your command to the audit process.
Please check to make sure that your INGRES audit process is running
and that your program is running with sufficient privilege to
communicate with the audit process."
W_DM5001_BAD_PG_NUMBER:NO_GE
"Warning.  Page %0d has an incorrect page number of %1d."
/*%
** Parameters:
**	%0d -	real page number
**	%1d -   page number that appears on page
** Explanation:
**	The page_page field contains the page number in the file.  In this
**	case, the page number is incorrect on the data page.
** System Status:
**	This is a diagnostic message printed during the check/patch operation.
** User Action:
**	If the check operation is in process, then the user must decide if they
**	wish to run the patch operation.  This operation will fix the diagnosed
**	problem, but will also convert the table to heap and destroy all
**	secondary indices on the table.  NOTE:  if this is the check operation
**	and a secondary index is being examined, then the user should drop the
**	secondary index and recreate it.
*/
W_DM5002_BAD_DATA_PAGESTAT:NO_GE
"Warning:  page %0d is a data page, but the page_stat indicates it is NOT."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	The page_stat field indicates the page type.  In this case, the page
**	is believed to be a data page.  However, the page_stat has bits set
**	indicating it is some other type of page.
** System Status:
**	This is a diagnostic message printed during the check/patch operation.
** User Action:
**	If the check operation is in process, then the user must decide if they
**	wish to run the patch operation.  This operation will fix the diagnosed
**	problem, but will also convert the table to heap and destroy all
**	secondary indices on the table.  NOTE:  if this is the check operation
**	and a secondary index is being examined, then the user should drop the
**	secondary index and recreate it.
*/
W_DM5003_UNEXPECTED_VERSION:NO_GE
"Warning:  page %0d has an unexpected version of %1d.\n
This is not necessarily an error, but it is unexpected."
/*%
** Parameters:
**	%0d -	page number
**	%1d -   version
** Explanation:
**	Page->page_version is usually set to zero, as it is currently an
**	unused field on the page.  It had a non-zero value.
** System Status:
**	This is a diagnostic message printed during the check/patch operation.
** User Action:
**	None required for this message.
*/
W_DM5004_STRUCTURAL_DAMAGE:NO_GE
"Warning:  Page %0d is structurally unsound.  None of the tuples on the page
are recoverable."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	A data page consists of a header section, a structural section and a
**	data section.  The structural section is used to locate tuples in the
**	data section.  If damage occurs to the structural section, then DMF
**	cannot find the data associated with specific tuples on the page.  There
**	is no way to recover from this other than throwing away all tuples on
**	this page.
** System Status:
**	This is a diagnostic message printed during the check/patch operation.
** User Action:
**	If this is a check operation, user must decide whether or not to patch.
**	If the user patches, then the user will lose all data on this page.  If
**	this is a patch operation, then all data on this page will be deleted.
*/
W_DM5005_INVALID_TID:NO_GE
"WARNING:  Unable to get data from page for TID:  page = %0d, line = %1d\n"
/*%
** Parameters:
**	%0d -	page number
**	%1d -	line number
** Explanation:
**	The check/patch operation has already verified that the page is
**	structurally sound.  However, for some reason, it is not possible to
**	read the record (via normal DMF mechanisms).
** System Status:
**	This is a diagnostic message printed during the check/patch operation.
** User Action:
**	If this is a check operation, user must decide whether or not to patch.
**	If the user patches, then the user will lose the data for this tuple,
**	and user has no option at this point.
*/
W_DM5006_BAD_ATTRIBUTE:NO_GE
"Warning: BAD Attribute #%0d in tuple at TID:  page = %1d, line = %2d"
/*%
** Parameters:
**	%0d -	attribute number
**	%1d -	page number
**	%2d -	line number
** Explanation:
**	The data contained for this attribute is not legal for the attribute
**	type.  Example, a '\0' in a text string.
** System Status:
**	This is a diagnostic message printed during the check/patch operation.
** User Action:
**	If this is a check operation, user must decide whether or not to patch.
**	If the user patches, then the user will lose the data for this
**	attribute, and possibly the whole tuple.
*/
W_DM5007_ADF_ERROR:NO_GE
"Warning:  ADF error (%0x) was reported when operating on attribute %1d
on page %2d, line %3d"
/*%
** Parameters:
**	%0x -	adf error number
**	%1d -	attribute number
**	%2d -	page number
**	%3d -	line number
** Explanation:
**	Either adc_valchk or adc_getempty returned an unexpected error.  These
**	routines are called by dm1u_datapg when checking tuples on a data page.
** System Status:
**	This is a diagnostic message.  Processing starts on the next attribute.
** User Action:
**	Check/patch will not operate on the tuple if there are unexpected ADF
**	errors.  The user may wish to restart the server and rerun patch later.
*/
W_DM5008_TOO_MANY_BAD_ATTS:NO_GE
"Warning: there were too many bad attributes in TID: page = %0d, line = %1d\n
This tid should be deleted from the page."
/*%
**  Parameters:
**	%0d - page number
**	%1d - line number
**  Explanation
**	The check/patch operation checks each attribute of each tuple to see
**	if the data contained there is valid for the attribute type.  If it
**	finds more than a single bad attribute in a tuple, then it simply
**	makes the assumption that the whole tuple is corrupted.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	If the user is running check, they must decide whether or not to run
**	patch.  If they run patch, they will lose the contents of this tuple.
*/
W_DM5009_MERGED_DATA_CHAIN:NO_GE
"WARNING:  Page %0d is referenced more than once due to a merged chain.\n
This means that dmf may retrieve each tuple on this page (or its associated\n
overflow pages) more than once."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The check table patch operation has found a data page (or overflow data
**	page) that is referenced more than once.  This is an error condition,
**	that will cause incorrect values to be retrieved from the table.
**	(When there is a merged chain, there are usually orphan pages in the
**	file as well.  An orphan page is a page that is not referenced at all,
**	and the DBMS server never sees values on an orphan page.)
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM500A_CIRCULAR_DATA_CHAIN:NO_GE
"WARNING:  Page %0d is referenced more than once due to a circular chain.\n
This means that dmf will start following a data page overflow chain, and will\n
get stuck in an endless loop because the chain points back to a page in the\n
chain."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The check table patch operation has found a data page (or overflow data
**	page) that is referenced more than once.  This is an error condition,
**	that will cause incorrect values to be retrieved from the table.
**	(When there is a circular chain, then the server will get caught in an
**	endless loop while walking that chain, and will not return from some
**	queries.)
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM500B_WRONG_PAGE_MAIN:NO_GE
"WARNING: page %0d should have a page_main of %1d, but instead the value is %2d"
/*%
**  Parameters:
**	%0d - page number
**	%1d - value page_main should have
**	%2d - value page_main really does have
**  Explanation
**	Each data page has a page_main which DMF will use to determine what main
**	page to go to next while walking a file.  The interpretation of
**	page_main varies depending on file storage structure.  However, it is
**	necessary for page main to be correct, and it is not.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM500C_ORPHAN_DATA_PAGE:NO_GE
"WARNING:  data page %0d is an unreferenced page (or orphan).  All data on this\n
page or any overflow pages chained to it is effectively lost."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The specified data page is not referenced.  This means that the data on
**	this page, or on any overflow pages chained to this page, is lost.  The
**	server will never see the data because the page is not referenced.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM500D_WRONG_PAGE_OVFL:NO_GE
"WARNING: page %0d should have a page_ovfl of 0, but instead the value is %1d"
/*%
**  Parameters:
**	%0d - page number
**	%1d - value page_ovfl really does have
**  Explanation
**	Each data page has a page_ovfl which DMF will use to search overflow
**	data pages.  However, overflow data pages are NOT allowed in BTREES.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM500E_UNKNOWN_BTREE_PAGE:NO_GE
"WARNING: page %0d (page_stat = %1x) is NOT a legal type of btree page."
/*%
**  Parameters:
**	%0d - page number
**	%1x - value of page->page_stat
**  Explanation
**	The page_stat field of a page is used to determine what type of page
**	this is.  This page is not marked as anything that is legal for a
**	btree table.  This may or may not cause problems for DMF, depending on
**	what type of page this really is.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM500F_BAD_FREE_LIST_HDR:NO_GE
"WARNING:  Unable to read the btree free page list header (page 1)."
/*%
**  Parameters:
**	none.
**  Explanation
**	Page 1 is the btree free page list header page.  It is used to determine
**	where to go to get a free page.  If there are not any free pages, then
**	new memory is allocated and chained to this page.  It is very serious
**	if DMF cannot read this page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5010_BAD_HDR_PAGE_STAT:NO_GE
"WARNING:  Page Stat for the Free Page List Header Page does not indicate that
it is an index page."
/*%
**  Parameters:
**	none.
**  Explanation
**	Page 1 is the btree free page list header page.  It is used to determine
**	where to go to get a free page.  The page_stat for this page should be
**	set to DMPP_INDEX, and it is not.  This is probably not a serious
**	error (if this is all that is wrong with the file), as DMF will not
**	check the page_stat for this page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user may run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5011_ILLEGAL_DATA_PAGE:NO_GE
"WARNING:  Page %0d is a data page, but btree secondary indices do not contain
data pages."
/*%
**  Parameters:
**	%0d - Page number of data page.
**  Explanation
**	This page indicates that it is a data page.  However, iirelation
**	indicates that this table is really a btree secondary index.  Btree
**	secondary index tables do not contain data pages.  Instead, the leaf
**	pages reference data pages in the main file.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5012_NO_FREE_PAGES:NO_GE
"WARNING:  The free list header page indicates that there are not any free pages
in the file.  This is not an error condition unless further messages warn of
unreferenced free pages."
/*%
**  Parameters:
**	None.
**  Explanation
**	All btree files have a free list header page, which contains a chain of
**	free pages currently in the file.  It is possible that there are not
**	any free pages in the file.  However, it is also possible that there
**	are free pages and that the free list header is corrupted.  If there are
**	not any messages about unreferenced free pages, then the first case is
**	what occurred.  Otherwise, it is the latter.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user may run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5013_CIRCULAR_FREE_LIST:NO_GE
"Page %0d is referenced on the Free List more than once."
/*%
**  Parameters:
**	%0d - Page number of free page.
**  Explanation
**	The free list chain is corrupted.  Each page on the free list should
**	be referenced only once.  This page is referenced twice.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5014_FREE_PAGE_UNREADABLE:NO_GE
"WARNING:  Page %0d on the free list is not readable.\n
It is not possible to follow the free page chain any further."
/*%
**  Parameters:
**	%0d - Page number of free page.
**  Explanation
**	The free list chain is corrupted.  Each page on the free list should
**	reference another page on the free page chain (except for the end of
**	chain page).  In this case, the free page is unreadable, so it not
**	possible to follow the chain any further.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5015_FREE_PAGE_UNKNOWN:NO_GE
"WARNING:  Page %0d on the free list is not a valid page.\n
It is not possible to follow the free page chain any further."
/*%
**  Parameters:
**	%0d - Page number of free page.
**  Explanation
**	The free list chain is corrupted.  Each page on the free list should
**	reference another page on the free page chain (except for the end of
**	chain page).  In this case, this page in the free page chain is not
**	marked as a free page.  Actually, the page type is not marked at all.
**	It is not possible to follow the chain any further.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5016_NOT_A_FREE_PAGE:NO_GE
"WARNING:  Page %0d on the free list is not a free page, but is another type of
valid btree page.\n
It is not possible to follow the free page chain any further."
/*%
**  Parameters:
**	%0d - Page number of free page.
**  Explanation
**	The free list chain is corrupted.  Each page on the free list should
**	reference another page on the free page chain (except for the end of
**	chain page).  In this case, this page in the free page chain is not
**	marked as a free page, so it should not be in the free page chain.  It
**	is not possible to follow the free page chain any further.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5017_NO_1ST_LEAF:NO_GE
"WARNING:  Unable to determine leftmost leaf page in btree.\n
Unable to verify btree leaf sideways pointers are correct."
/*%
**  Parameters:
**	None.
**  Explanation
**	The leftmost btree leaf page should be marked as bt_sequence[-2] !=0.
**	That is how the check table operation finds the leftmost btree leaf
**	page.  It needs to know that information to verify that the leaf
**	sideways pointers (bt_nextpage) are correct.  In this case, the portion
**	of dm1u that checks the leaf pages was unable to find the leftmost
**	leaf page, so dm1u_btree cannot check the leaf sideways pointers.  This
**	indicates an unhealthy btree table.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5018_LEAF_CHAIN_BROKEN:NO_GE
"WARNING:  The leaf page sideways pointer chain is broken at leaf page # %0d."
/*%
**  Parameters:
**	%0d - leaf page number in sideways leaf chain
**  Explanation
**	One of the pages in the btree leaf page chain (bt_nextpage) is not
**	a leaf page, or is a page that cannot be read from disk.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5019_LEAF_CHAIN_DUPLICATE:NO_GE
"WARNING:  The leaf page sideways pointer chain is broken at leaf page # %0d.\n
This is because the chain points to an earlier member (ie, is circular)\n
or because it points to an unreferenced leaf page.  Neither case is legal."
/*%
**  Parameters:
**	%0d - page number in sideways leaf chain (4=> 4th page in chain)
**  Explanation
**	One of the pages in the btree leaf page chain (bt_nextpage) has been
**	referenced in that chain more than once, or is a leaf page that was not
**	referenced by any of the index pages.   This is an error condition, and
**	the btree should be patched.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM501A_BAD_LINK_IN_CHAIN:NO_GE
"WARNING:  the value of leaf page sideways pointer # %0d in chain is illegal."
/*%
**  Parameters:
**	%0d - page number of next leaf page in sideways leaf chain 
**  Explanation
**	The leaf page sideways pointer on the specified page is larger than
**	the number of pages in the file.  This means that the chain is
**	corrupt.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM501B_UNREF_LEAF_FROM_CHAIN:NO_GE
"WARNING:  About %0d Leaf page(s) are not referenced in the leaf page sideways ptr chain."
/*%
**  Parameters:
**	%0d -  number of unreferenced leaf pages.
**  Explanation
**	Every leaf page in the file should be referenced precisely once via
**	leaf page sideways pointers.  In this case, some pages are unreferenced.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM501C_REFERENCED_BTREE_ROOT:NO_GE
"WARNING:  the btree root page is referenced by another page.\n
No btree index or leaf page should reference the root page."
/*%
**  Parameters:
**	none.
**  Explanation
**	The btree root page, page 0, is the top of the tree.  No btree leaf or
**	index pages should ever point up the tree to this page.  However, in
**	this case, either an index or leaf page has the root page for one of
**	its bids/tids.  This indicates severe structural damage.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM501D_REFERENCED_FREEHDR:NO_GE
"WARNING:  the btree root page is referenced by another page.\n
No btree index or leaf page should reference the root page."
/*%
**  Parameters:
**	none.
**  Explanation
**	The btree free list header page, page 1, is not in the chain to data
**	pages, and therefore no btree leaf or index pages should ever point to
**	this page.  In this case, either an index or leaf page has the free
**	list header page for one of its bids/tids.  This indicates severe
**	structural damage to the btree.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM501E_UNEXAMINED_PAGE:NO_GE
"WARNING:  Page %0d was not examined during the Check Table operation.\n
Also, no other pages in the btree file referenced this page."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This should not be possible.  The check table operation checks every
**	page in the file, so there is probably memory corruption in the
**	dm1u_cb, which is allocated during dm1u_table() execution.  If the user
**	sees this message, then the user may wish to bring down the server,
**	restart it, and rerun the check table operation.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should be critical of check table results if check table
**	issues this message.  It is probably indicative of memory corruption.
**	The customer should bring down the server, restart it, then reissue
**	the check table command.  If the user still gets this message, the user
**	should call Tech Support.
*/
W_DM501F_ORPHAN_BTREE_DATA_PG:NO_GE
"WARNING:  data page %0d is an unreferenced page (or orphan).\n
All data on this page is effectively lost until the file is patched."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is an orphan (ie, is not referenced by any other pages in
**	the btree table.)  This indicates an unhealthy situation that results
**	in data loss.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5020_ORPHAN_BTREE_FREE_PG:NO_GE
"WARNING:  free page %0d is an unreferenced page (or orphan)."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is an orphan (ie, is not referenced by any other pages in
**	the btree table.)  This indicates an unhealthy situation.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5021_ORPHAN_BTREE_LEAF_PG:NO_GE
"WARNING:  leaf page %0d is an unreferenced page (or orphan).  All data on\n
pages subordinate to this page is effectively lost until the file is patched\n.
Check Table will now examine this orphan leaf page..."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is an orphan (ie, is not referenced by any other pages in
**	the btree table.)  This indicates an unhealthy situation that results
**	in data loss.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5022_ORPHAN_BTREE_OVFL_PG:NO_GE
"WARNING:  leaf overflow page %0d is an unreferenced page (or orphan).  All\n
data on pages subordinate to this page is effectively lost until the file is patched."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is an orphan (ie, is not referenced by any other pages in
**	the btree table.)  This indicates an unhealthy situation that results
**	in data loss.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5023_ORPHAN_BTREE_INDEX:NO_GE
"WARNING:  index page %0d is an unreferenced page (or orphan).  All data on\n
pages subordinate to this page is effectively lost until the file is patched\n.
Check Table will now examine this orphan index page..."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is an orphan (ie, is not referenced by any other pages in
**	the btree table.)  This indicates an unhealthy situation that results
**	in data loss.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5024_ORPHAN_UNREADABLE_PG:NO_GE
"WARNING:  page %0d is unreadable and is an orphan.\n
That means that this unreadable page is NOT used by the btree."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is an orphan (ie, is not referenced by any other pages in
**	the btree table.)  It is also a page that DMF could not read from disk.
**	It probably does not hurt that this page could not be read from disk
**	since DMF will not use it.  (OF COURSE, THERE IS THE CHANCE THAT THIS
**	PAGE IS A LEAF PAGE THAT WILL BE POINTED TO BY THE LEAF PAGE SIDEWAYS
**	POINTERS.  HOWEVER, THAT WOULD BE AN ERROR CONDITON, AS ALL LEAF PAGES
**	SHOULD BE REFERENCED BY INDEX PAGES.)  The user should probably patch
**	the table to be safe.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5025_ORPHAN_ILLEGAL_PG:NO_GE
"WARNING:  page %0d is illegal because it does not identify what type of page it
is.  It is also an orphan.\n
That means that this page is NOT used by the btree."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is an orphan (ie, is not referenced by any other pages in
**	the btree table.)  It is also an invalid page, because it does not
**	identify what type of page it is.  This means that DMF would not know
**	what to do with it.  It probably does not hurt that this page is illegal
**	since DMF will not use it.  (OF COURSE, THERE IS THE CHANCE THAT THIS
**	PAGE IS A LEAF PAGE THAT WILL BE POINTED TO BY THE LEAF PAGE SIDEWAYS
**	POINTERS.  HOWEVER, THAT WOULD BE AN ERROR CONDITON, AS ALL LEAF PAGES
**	SHOULD BE REFERENCED BY INDEX PAGES.)  The user should probably patch
**	the table to be safe.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5026_IDX_CHAIN_BROKEN:NO_GE
"WARNING:  BTREE Index Chain is broken.  This means there is some data loss.\n
The data can be recovered via the Patch Table operation."
/*%
**  Parameters:
**	none.
**  Explanation
**	The index page tree structure is damaged.  This means that some/all of
**	the leaf and data pages could not be referenced by following the tree.
**	THIS IS VERY SERIOUS.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5027_NOT_INDEX_PG:NO_GE
"WARNING:  Page %0d should be a btree index page, but is not marked as one."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is expected to be a btree index page.  It should either be
**	the root page, or an index page referenced by another index page in
**	the tree.  However, this page is not marked as an index page.  So,
**	the Check Operation will have to assume it is not an index page, and
**	must assume that the tree index structure is broken.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5028_MULT_REF_INDEX_PG:NO_GE
"WARNING:  BTREE index page %0d is referenced more than once."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	The btree index structure references each leaf and index page in the
**	tree exactly once.  If a page is not referenced, that means that some
**	of the data in the table is effectively lost.   If a page is referenced
**	more than once, that means we will get undesired duplicate tuples.  (Of
**	course, data pages may be referenced several times).
**
**	In this case, an index page is referenced more than once.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5029_WRONG_BT_NEXTPAGE:NO_GE
"WARNING: page %0d should have bt_nextpage = 0, but instead the value is %1d."
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of page->bt_nextpage
**  Explanation
**	bt_nextpage is the leaf page forward pointer.  On leaf pages it is
**	expected to contain the page number of a valid leaf page.  On index
**	pages, it is expected to be =0.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM502A_WRONG_BT_DATA:NO_GE
"WARNING: page %0d should have bt_data = 0, but instead the value is %1d."
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of page->bt_data
**  Explanation
**	bt_data is the associated data page pointer.  On leaf pages it is
**	expected to contain the page number of a valid data page.  On index
**	pages, it is expected to be =0.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM502B_INVALID_BT_KIDS:NO_GE
"WARNING:  index/leaf page %0d is unreadable because bt_kids has invalid value\n
of %1d.  (Valid values range from 0 to %2d.)"
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of page->bt_kids
**	%2d - DM1U_MAXBIDS
**  Explanation
**	bt_kids indicates the number of bids on an index page or the number of
**	tids on a leaf pages.  This must have a value between 0 and
**	DM1B_MAXBIDS.  bt_kids is used to index into bt_sequence to get the
**	offset to the bid/tid.  If the max index has an improper value, then how
**	can we get data  off of this page?
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM502C_BAD_BID_OFFSET:NO_GE
"WARNING:  The offset to bid # %0d on page # %1d is invalid."
/*%
**  Parameters:
**	%0d - line number
**	%1d - page number
**  Explanation
**	To get a BID from an index page, one must first get the offset to the
**	bid from bt_sequence.  In this case, the offset in bt_sequence is
**	invalid.  This is a corruption of the index chain, which means the table
**	should be patched.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM502D_IDX_REFERENCES_SELF:NO_GE
"WARNING:  index page %0d references itself in bid # %1d.\n
This means DMF will end up in an endless loop walking this index page."
/*%
**  Parameters:
**	%0d - page number
**	%1d - line number
**  Explanation
**	The btree index structure references each leaf and index page in the
**	tree exactly once.  If a page is not referenced, that means that some
**	of the data in the table is effectively lost.   If a page is referenced
**	more than once, that means we will get undesired duplicate tuples.  (Of
**	course, data pages may be referenced several times).
**
**	In this case, an index page references itself, which sets up an endless
**	loop should DMF try to walk this tree.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM502E_IDX_REFERENCES_ROOT:NO_GE
"WARNING:  index page %0d references the root page in bid # %1d.\n
This means DMF will end up in an endless loop walking this index page."
/*%
**  Parameters:
**	%0d - page number
**	%1d - line number
**  Explanation
**	The btree index structure references each leaf and index page in the
**	tree exactly once.  If a page is not referenced, that means that some
**	of the data in the table is effectively lost.   If a page is referenced
**	more than once, that means we will get undesired duplicate tuples.  (Of
**	course, data pages may be referenced several times).
**
**	In this case, this index page references the root index page, which
**	sets up a large endless loop should DMF try to walk this tree.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM502F_IDX_REFERENCES_FREE:NO_GE
"WARNING:  index page %0d references the free list header in bid # %1d.\n
This means DMF will end up in walking free pages while expecting index pages."
/*%
**  Parameters:
**	%0d - page number
**	%1d - line number
**  Explanation
**	The btree index structure references each leaf and index page in the
**	tree exactly once.  If a page is not referenced, that means that some
**	of the data in the table is effectively lost.   If a page is referenced
**	more than once, that means we will get undesired duplicate tuples.  (Of
**	course, data pages may be referenced several times).
**
**	In this case, this index page references the free list header, which
**	means it will be treating a free page as an index page, which is
**	meaningless.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5030_NOT_LEAF_PG:NO_GE
"WARNING:  Page %0d should be a btree leaf page, but is not marked as one."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is expected to be a btree leaf page.  However, this page is
**	not marked as a leaf page.  So, the Check Operation will have to
**	assume it is not a leaf, and must assume that the tree index
**	structure is broken.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5031_MULT_REFERENCED_LEAF:NO_GE
"WARNING:  BTREE leaf page %0d is referenced more than once."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The check table patch operation has found a leaf page (or overflow leaf
**	page) that is referenced more than once.  This is an error condition,
**	that will cause incorrect values to be retrieved from the table.
**	(The check table logic will attempt to distinguish between a merged and
**	circular leaf page chain if there was enough memory to allocate a
**	dm1u_chain bitmap.  If so, then either W_DM5032 or W_DM5033 will follow
**	this error message.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5032_MERGED_LEAF_CHAIN:NO_GE
"WARNING:  Page %0d is referenced more than once due to a merged chain.\n
This means that dmf may retrieve each tuple on this page (or its associated\n
overflow pages) more than once."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The check table patch operation has found a leaf page (or overflow leaf
**	page) that is referenced more than once.  This is an error condition,
**	that will cause incorrect values to be retrieved from the table.
**	(When there is a merged chain, there are usually orphan pages in the
**	file as well.  An orphan page is a page that is not referenced at all,
**	and the DBMS server never sees values on an orphan page.)
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5033_CIRCULAR_LEAF_CHAIN:NO_GE
"WARNING:  Page %0d is referenced more than once due to a circular chain.\n
This means that dmf will start following a leaf page overflow chain, and will\n
get stuck in an endless loop because the chain points back to a page in the\n
chain."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The check table patch operation has found a leaf page (or overflow leaf
**	page) that is referenced more than once.  This is an error condition,
**	that will cause incorrect values to be retrieved from the table.
**	(When there is a circular chain, then the server will get caught in an
**	endless loop while walking that chain, and will not return from some
**	queries.)
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5034_TOOMANY_NEG_INFINITY:NO_GE
"WARNING: Too many leaf pages are marked as negative infinity.\n
Page %0d and page %1d are both marked as containing the minimum key."
/*%
**  Parameters:
**	%0d - page number
**	%1d - page number
**  Explanation
**	A btree has one or more leaf pages.  The leaf page containing the
**	smallest key (known as negative infinity), is marked by a nonzero tid
**	on bt_sequence[DM1B_LRANGE].  The leaf page containing the largest key
**	(known as positive infinity) is marked by a nonzero tid on
**	bt_sequence[DM1B_RRANGE].  If the file contains only a single page, then
**	that page will be marked as both positive and negative infinity.
**	Regardless of the number of pages in the file, only 1 leaf page should
**	be marked as negative infinity and only one should be marked as positive
**	infinity.
**	In this case, at least two pages were marked as negative infinity.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5035_TOOMANY_POS_INFINITY:NO_GE
"WARNING: Too many leaf pages are marked as positive infinity.\n
Page %0d and page %1d are both marked as containing the maximum key."
/*%
**  Parameters:
**	%0d - page number
**	%1d - page number
**  Explanation
**	A btree has one or more leaf pages.  The leaf page containing the
**	smallest key (known as negative infinity), is marked by a nonzero tid
**	on bt_sequence[DM1B_LRANGE].  The leaf page containing the largest key
**	(known as positive infinity) is marked by a nonzero tid on
**	bt_sequence[DM1B_RRANGE].  If the file contains only a single page, then
**	that page will be marked as both positive and negative infinity.
**	Regardless of the number of pages in the file, only 1 leaf page should
**	be marked as negative infinity and only one should be marked as positive
**	infinity.
**	In this case, at least two pages were marked as positive infinity.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5036_CANT_READ_LEAF_TID:NO_GE
"WARNING: cannot read tid/key pair %0d from leaf page %1d."
/*%
**  Parameters:
**	%0d - line number
**	%1d - page number
**  Explanation
**	Leaf pages contain tid/key pairs in lieu of data.  The bt_sequence
**	array contains an offset of where on the page they tid/key pair
**	resides.  Something was structurally wrong with the page, probably
**	bt_sequence[] had invalid value.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5037_NOT_LEAF_OVFL_PG:NO_GE
"WARNING:  Page %0d should be a leaf overflow page, but is not marked as one."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is expected to be a leaf overflow page.  However, this page is
**	not marked as an overflow page.  So, the Check Operation will have to
**	assume it is not a leaf overflow, and must assume that the tree index
**	structure is broken.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5038_WRONG_BTNEXTPG:NO_GE
"WARNING: leaf overflow page %0d has a different bt_nextpage than parent leaf
page %1d."
/*%
**  Parameters:
**	%0d - overflow leaf page number.
**	%1d - leaf page number that overflow page is chained to.
**  Explanation
**	All leaf overflow pages should have the same bt_nextpage value as
**	their parent leaf page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5039_WRONG_OVFL_PAGEMAIN:NO_GE
"WARNING: leaf overflow page %0d has a different page_main than parent leaf
page %1d."
/*%
**  Parameters:
**	%0d - overflow leaf page number.
**	%1d - leaf page number that overflow page is chained to.
**  Explanation
**	All leaf overflow pages should have the same page_main value as
**	their parent leaf page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM503A_BAD_OVFL_PG_KEY:NO_GE
"WARNING:  Overflow page %0d (line %1d) has a different key than the\n
parent leaf page.  All keys on parent and overflow pages should be identical."
/*%
**  Parameters:
**	%0d - overflow leaf page number.
**	%1d - line number
**  Explanation
**	The only reason for btree leaf overflow pages to exist is when there are
**	more tuples in the table with the identical key than fit on a single
**	leaf page.  So, a leaf overflow page is created.  The leaf overflow page
**	must have the same key as the leaf page they overflowed from.
**	In this case, a different key was found on the leaf page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM503B_BAD_OVFLPARENT_KEY:NO_GE
"WARNING.  Leaf page %0d has overflow pages chained to it.
All keys on this page should be identical, but key at line %1d is different."
/*%
**  Parameters:
**	%0d - leaf page number.
**	%1d - line number
**  Explanation
**	The only reason for btree leaf overflow pages to exist is when there are
**	more tuples in the table with the identical key than fit on a single
**	leaf page.  So, a leaf overflow page is created.  The leaf overflow page
**	must have the same key as the leaf page they overflowed from.
**	In this case, the leaf page that overflows has more than 1 key value on
**	it.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM503C_BAD_LEAF_KEY:NO_GE
"WARNING:  Leaf page %0d has an invalid key at line %1d."
/*%
**  Parameters:
**	%0d - leaf page number.
**	%1d - line number
**  Explanation
**	The keys on a leaf page must be in increasing order on the page.  (Of
**	course, it is legal to have identical keys).  However, the keys must
**	never be in decreasing order.  The key at the specified line is
**	less than a preceding key on this page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM503D_NO_SUCH_TID:NO_GE
"WARNING:  Leaf page %0d references data page tid (page=%1d, line=%2d).\n
However, the specified tid does not exist in this file."
/*%
**  Parameters:
**	%0d - leaf page number.
**	%1d - tid.page
**	%2d - tid.line
**  Explanation
**	The leaf page references a tid that does not exist in this file. IE,
**	the leaf page says to go to data page X, line Y to get the data.  But
**	there is no line Y on page X, or there is no page X.
**	THIS IS THE MOST COMMON TYPE OF BTREE CORRUPTION.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM503E_FIXED_LEAF:NO_GE
"CONSISTENCY CHECK:  fixed leaf page %0x on entry to dm1u_lchain."
/*%
**  Parameters:
**	%0x - address of fixed leaf page
**  Explanation
**	dm1u_lchain expects no pages fixed to dm1u_cb->dm1u_leaf_pg_ptr, but
**	a page is fixed there.  So, it is skipping the checking of leaf page
**	sideways pointers.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM503F_NOT_A_LEAF:NO_GE
"WARNING:  page %0d is not a leaf page."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	We are walking the leaf page sideways pointers (bt_nextpage), so we
**	expect this page to be a leaf page, but it is not.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5040_KEY_HOLE:NO_GE
"WARNING:  There is a hole in the key range.  Page %0d's first key has a\n
different value than the last key of the previous leaf page."
/*%
**  Parameters:
**	%0d - leaf page number.
**  Explanation
**	We are walking the leaf page sideways pointers (bt_nextpage).  The last
**	key on one page should be identical to the first key on the next page.
**	In this case, the keys did not match.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5041_MULTIREF_LEAFS:NO_GE
"WARNING:  %0d more leaf page(s) are not referenced in the leaf page sideways\n
ptr chain than there are leaf pages in the file."
/*%
**  Parameters:
**	%0d -  number of unreferenced leaf pages.
**  Explanation
**	Every leaf page in the file should be referenced precisely once via
**	leaf page sideways pointers.  In this case, some pages are referenced
**	multiple times.  THIS ERROR MSG SHOULD NOT BE POSSIBLE SINCE CHECK LOGIC
**	DETECTS CIRCULAR CHAINS AND STOPS, BUT THIS CONDITION IS HANDLED FOR
**	COMPLETENESS.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5042_NOT_DATA_PAGE:NO_GE
"  PAGE %0d is a NOT a data page, but should be one."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	The Check operation has encountered a non-data page, referenced by
**	a leaf page.  THIS IS A SERIOUS ERROR.
** System Status:
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5043_LEAF_REFERENCES_SELF:NO_GE
" Leaf page %0d references itself on its overflow page pointer.
This is a circular chain and the table should be patched."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	The Check operation has encountered a leaf page with a page_ovfl that
**	points to itself.  This means that if DMF tries to access this page,
**	it will end up in an endless loop.  THIS IS A SERIOUS ERROR.
** System Status:
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5044_IDX_CHAIN_BROKEN:NO_GE
"WARNING:  ISAM Index Chain is broken.  This means there is some data loss.\n
The data can be recovered via the Patch Table operation."
/*%
**  Parameters:
**	none.
**  Explanation
**	The index page tree structure is damaged.  This means that some/all of
**	the leaf and data pages could not be referenced by following the tree.
**	THIS IS VERY SERIOUS.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5045_NOT_INDEX_PG:NO_GE
"WARNING:  Page %0d should be an isam index page, but is not marked as one."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	This page is expected to be an isam index page.  It should either be
**	the root page, or an index page referenced by another index page in
**	the tree.  However, this page is not marked as an index page.  So,
**	the Check Operation will have to assume it is not an index page, and
**	must assume that the tree index structure is broken.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5046_MULT_REF_INDEX_PG:NO_GE
"WARNING:  ISAM index page %0d is referenced more than once."
/*%
**  Parameters:
**	%0d - page number.
**  Explanation
**	The isam index structure references each index page in the
**	tree exactly once.  If a page is not referenced, that means that some
**	of the data in the table is effectively lost.   If a page is referenced
**	more than once, that means we will get undesired duplicate tuples.  (Of
**	course, data pages may be referenced several times).
**
**	In this case, an index page is referenced more than once.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5047_INVALID_NUM_KEYS:NO_GE
"WARNING:  isam index page %0d is unreadable because page_next_line has invalid value\n
of %1d.  (Valid values range from 0 to %2d.)"
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of page->page_next_line
**	%2d - DM_TIDEOF
**  Explanation
**	page_next_line indicates the number of keys on an index page.  This
**	must have a value between 0 and DM_TIDEOF.  Page_next_line is used to
**	index into page_line_tab to get the offset to the key.  If the max
**	number of keys on page has an improper value, then how acn we get
**	get data  off of this page?
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5048_INVALID_PAGE_OVLF :NO_GE
"WARNING:  ISAM index page %0d is corrupted.  The value of page_ovfl should\n
indicate the page number of the data page containing the smallest value on\n
this page.  The value of the invalid page_ovfl is %1d."
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of page->page_ovfl
**	%2d - DM_TIDEOF
**  Explanation
**	page_ovfl indicates the page number of the data or index page containing
**	the smallest key on this page.  If this is the lowest level index page
**	(page_main=0), then this must point to a data page.  If this is a higher
**	level index page (page_main >0), then this must indicate an index page.
**	Data pages must fall between 0 and iirelation.relprim-1.  Index pages
**	must fall between iirelation.relprim and iirelation.relmain.
**
**	In this case, this is the lowest level index page, so we expect
**	page_ovfl to indicate a data page, but it does not.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5049_INVALID_PAGE_OVLF :NO_GE
"WARNING:  ISAM index page %0d is corrupted.  The value of page_ovfl should\n
indicate the page number of the next lower level index page containing the\n
smallest key on this page.  The value of the invalid page_ovfl is %1d."
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of page->page_ovfl
**  Explanation
**	page_ovfl indicates the page number of the data or index page containing
**	the smallest key on this page.  If this is the lowest level index page
**	(page_main=0), then this must point to a data page.  If this is a higher
**	level index page (page_main >0), then this must indicate an index page.
**	Data pages must fall between 0 and iirelation.relprim-1.  Index pages
**	must fall between iirelation.relprim and iirelation.relmain.
**
**	In this case, this is a higher level level index page, so we expect
**	page_ovfl to indicate an isam index page, but it does not.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM504A_MISSING_ISAM_KEY:NO_GE
"Warning:  the isam index page should contain densely packed keys.\n
However, the key for entry %1d is missing."
/*%
**  Parameters:
**	%0d - offset into page_line_tab where NULL offset resides.
**  Explanation
**	The isam index structure contains 1 or more keys on a page.  There
**	should be page_next_line number of keys on the index page, and the
**	page_line_tab should have a valid offset to each key.  Valid offsets
**	are greater than zero and less than the page size.
**  System Status;
**	This is a diagnostic message.  
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM504B_IDX_REFERENCES_SELF:NO_GE
"WARNING:  index page %0d references itself for key # %1d.\n
This means DMF will end up in an endless loop walking this index page."
/*%
**  Parameters:
**	%0d - page number
**	%1d - line number
**  Explanation
**	The isam index structure references each index page in the
**	tree exactly once.  If a page is not referenced, that means that some
**	of the data in the table is effectively lost.   If a page is referenced
**	more than once, that means we will get undesired duplicate tuples.  (Of
**	course, data pages may be referenced several times).
**
**	In this case, an index page references itself, which sets up an endless
**	loop should DMF try to walk this tree.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM504C_IDX_REFERENCES_ROOT:NO_GE
"WARNING:  index page %0d references the root page for key # %1d.\n
This means DMF will end up in an endless loop walking this index page."
/*%
**  Parameters:
**	%0d - page number
**	%1d - line number
**  Explanation
**	The isam index structure references each index page in the
**	tree exactly once.  If a page is not referenced, that means that some
**	of the data in the table is effectively lost.   If a page is referenced
**	more than once, that means we will get undesired duplicate tuples.  (Of
**	course, data pages may be referenced several times).
**
**	In this case, this index page references the root index page, which
**	sets up a large endless loop should DMF try to walk this tree.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM504D_INDEX_KEY_OFFSET:NO_GE
"WARNING:  isam index page %0d has an invalid key offset"
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The isam index page uses the same structure as a data page, except that
**	the only "data" contained on the page is 1 or more keys.  Each key
**	has an entry in page_line_tab indicating the offset to where on the
**	data page the key is stored.  The value in page_line_tab for one of the
**	keys is either null or points past the end of the page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM504E_WRONG_NUM_KEYS:NO_GE
"WARNING: isam index page %0d has keys with the wrong number of attributes in them."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The TCB reflects a disagreement between iirelation.relkeys (actual
**	field is tcb_relatts) and the number of iiattribute attributes that
**	describe the key (tcb_keys).  This could either reflect inconsistent
**	dbms catalogs or a DMF internal error.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the verifydb dbms_catalogs operation to assure
**	that the dbms catalogs are consistent.  If so, repeat the table check.
**	If the same error occurs after verifying that catalogs are consistent,
**	the user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM504F_KEYS_NOT_SEQUENTIAL:NO_GE
"Warning: ISAM index page %0d has a key with the attributes in the wrong order."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The TCB contains a copy of iiattribute records that describe the
**	attributes in the key.  These DMP_ATTS records should be ordered by
**	"key" in ascending order.  In this case they are not.  This could
**	reflect inconsistent dbms catalogs or could reflect a dmf internal
**	error.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the verifydb dbms_catalogs operation to assure
**	that the dbms catalogs are consistent.  If so, repeat the table check.
**	If the same error occurs after verifying that catalogs are consistent,
**	the user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5050_BAD_ISAM_KEY_ORDER:NO_GE
"WARNING: isam index page %0d has keys that are out of order"
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The keys on an isam index page should be in increasing order.  The
**	check table utility found a page where they did not follow that order.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5051_MULTIREF_DATA_PAGE:NO_GE
"Warning:  isam data page %0d is referenced more than once."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	the isam index page should only reference a data page once.  In this
**	case, the page has been referenced more than once.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5052_BAD_PAGE_MAIN:NO_GE
"Warning:  isam data page %0d has an invalid page main of %1d"
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of bad page_main.
**  Explanation
**	Isam main pages must fall between the range of
**	    0 <= page_main < iirelation.relmain
**	However, this data page has a page main that does not fall in this
**	range.  This means that the next "data" page referenced is really an
**	overflow page or an index page.  This is not a healthy condition.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5053_BAD_ISAM_OVFL:NO_GE
"WARNING: data page %0d has an illegal page_ovfl of %1d"
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of bad page_ovfl.
**  Explanation
**	Isam overflow page come after the index pages in the file, and must be:
**		page_main > iirelation.relprim
**	However, this data page has a page_ovfl that does not meet this 
**	condition.  This means that the overflow " page referenced is really a
**	data page or an index page.  This is not a healthy condition.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5054_MULTIREF_ISAM_OVFL_PG:NO_GE
"Warning:  isam overflow page %0d is referenced more than once."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	the isam overflow chain should only reference an overflow page once.
**	In this case, the page has been referenced more than once.  This means
**	that we have a circular or merged chain.  Either case is VERY unhealthy.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5055_BAD_OVFL_PAGEMAIN:NO_GE
"Warning:  isam overflow page %0d should have a page main of %1d\n
but it does not."
/*%
**  Parameters:
**	%0d - page number
**	%1d - value of expected page_main.
**  Explanation
**	On an isam overflow page, the page_main should be the same value as
**	the main page that this overflow page is chained to.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5056_ORPHAN_ISAM_MAIN_PG:NO_GE
"Warning:  Page %0d is an orphan isam main (or data) page."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The isam index structure starts at the root.  The root pages references
**	lower level index pages.  The lowest level index page references MAIN
**	or data pages.  (It is legal for the root page to also be the lowest
**	level index page.)  If there is not enough room on a data page to hold
**	all tuples with keys in the key range for that page, then overflow
**	pages are created and chained to the main page.  If the overflow page
**	fills up, then an additional overflow page is chained to it.  Thus,
**	index pages should reference lower level index pages or main data pages.
**	Main data pages may only reference overflow pages, but will not
**	always do so.  Overflow pages may only reference other overflow pages.
**
**	In this case there is a main data page that was not referenced by any
**	isam index page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5057_ORPHAN_ISAM_INDEX_PG:NO_GE
"Warning:  Page %0d is an orphan isam index page."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The isam index structure starts at the root.  The root pages references
**	lower level index pages.  The lowest level index page references MAIN
**	or data pages.  (It is legal for the root page to also be the lowest
**	level index page.)  If there is not enough room on a data page to hold
**	all tuples with keys in the key range for that page, then overflow
**	pages are created and chained to the main page.  If the overflow page
**	fills up, then an additional overflow page is chained to it.  Thus,
**	index pages should reference lower level index pages or main data pages.
**	Main data pages may only reference overflow pages, but will not
**	always do so.  Overflow pages may only reference other overflow pages.
**
**	In this case there is an index data page that was not referenced by any
**	other isam index page.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM5058_ORPHAN_ISAM_OVFL_PG:NO_GE
"Warning:  Page %0d is an orphan isam overflow page."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	The isam index structure starts at the root.  The root pages references
**	lower level index pages.  The lowest level index page references MAIN
**	or data pages.  (It is legal for the root page to also be the lowest
**	level index page.)  If there is not enough room on a data page to hold
**	all tuples with keys in the key range for that page, then overflow
**	pages are created and chained to the main page.  If the overflow page
**	fills up, then an additional overflow page is chained to it.  Thus,
**	index pages should reference lower level index pages or main data pages.
**	Main data pages may only reference overflow pages, but will not
**	always do so.  Overflow pages may only reference other overflow pages.
**
**	In this case there is an overflow page that was not referenced by an
**	isam data page or by any other isam overflow pages.
**  System Status;
**	This is a diagnostic message.
**  User Action:
**	The user should run the patch table operation on this file.  The patch
**	table operation will remove all secondary indexes and convert the table
**	to heap.  Then the user may recreate secondary indexes and modify the
**	table to the desired storage structure.
*/
W_DM51FB_NO_CHAIN_MAP:NO_GE
"Unable to allocate memory for BitMap to mark pages referenced this pass.\n
This means it will not be possible to differentiate between merged and circular
chain."
/*%
** Parameters:
**	None.
** Explanation:
**	Check table keeps a bitmap indicating which pages have been referenced
**	this particular pass (ie, while walking a specific overflow chain, etc)
**	When it tried to allocate this bit map, it got an error.  So, it will
**	continue processing, but skip any logic that requires this bit map.
** System Status:
**	Processing continues, but if a page is referenced more than once, it
**	will not be able to distinguish whether this is a circular chain or a
**	merged chain.
** User Action:
**	The user probably wont care if this is a circular or merged chain.  The
**	fact that the index or overflow chain is damaged is enough to merit
**	patching the table.
*/
W_DM51FC_ERROR_PUTTING_PAGE:NO_GE
"WARNING:  An error was encountered putting back page %0d, which was patched.\n
This type of error should not occur under normal patch table operation.\n
Exiting due to the inability to update patched pages to the disk file..."
/*%
** Parameters:
**	%0d - page number.
** Explanation:
**	DMF was able to successfully read the page from disk via dm0p_fix_page.
**	However, it was not able to put the page back via dm0p_unfix_page.
**	This type of error should not occur unless the page cache is corrupted.
** System Status:
**	the statement is aborted.
** User Action:
**	User should check the errlog.log to see why the page could not be
**	unfixed.
*/
W_DM51FD_TOO_MANY_BAD_PAGES:NO_GE
"WARNING:  more than 1/2 of the pages in the file were unreadable.\n
This is too many 'bad' pages.  Exiting due to the number of unreadable pages..."
/*%
** Parameters:
**	none.
** Explanation:
**	There was an error reading a page into memory via dm0p_fix_page.  This
**	occurred for more than 1/2 of the pages in the file.  So, the patch/
**	check operation is giving up on this file.
** System Status:
**	the statement is aborted.
** User Action:
**	User cannot recover this table, it is too far gone...
*/
W_DM51FE_CANT_READ_PAGE:NO_GE
"WARNING:  Unable to read page %0d from table's file."
/*%
** Parameters:
**	%0d - page number.
** Explanation:
**	Each page of the open file is read in via dm0p_fix_page.  This is
**	a normal DMF method for obtaining pages from an open file.  Since DMF
**	cannot read the page, the check/patch operation cannot operate on this
**	page.
** System Status:
**	Processing continues -- it tries to go on to the next page.
** User Action:
**	User should set iirelation.relspec to the correct value.  If user does
**	not know what storage structure the table is, then they cannot check
**	it.  However, they can still patch it, which will convert it to heap
**	and remove all secondary indices.
*/
W_DM51FF_BAD_RELSPEC:NO_GE
"WARNING: Invalid iirelation storage structure specification (relspec)\n
THE CHECK OPERATION CANNOT CONTINUE.\nEXITING..."
/*%
** Parameters:
**	None.
** Explanation:
**	The check operation uses a different check algorithm for each storage
**	structure: TCB_HEAP (3), TCB_HASH (7), TCB_ISAM (5) and TCB_BTREE(11).
**	The value of iirelation.relspec is none of the above, so 
** System Status:
**	The check operation statement is aborted.
** User Action:
**	User should set iirelation.relspec to the correct value.  If user does
**	not know what storage structure the table is, then they cannot check
**	it.  However, they can still patch it, which will convert it to heap
**	and remove all secondary indices.
*/
I_DM5201_BAD_PG_NUMBER:NO_GE
"The page number for page %0d has been corrected from the erroneous value of %1d."
/*%
** Parameters:
**	%0d -	real page number
**	%1d -   erroneous page number that appeared on page
** Explanation:
**	The page_page field contains the page number in the file.  In this
**	case, the page number is incorrect on the data page.
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	None.
*/
I_DM5202_FIXED_DATA_PAGE:NO_GE
"Page %0d had structural damage, but it was successfully patched."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	Either page_next_line was invalid, was incorrect or did not agree with
**	the values in page_line_tab.  Therefore, page_next_line was reassigned
**	to a valid/correct value.  This means that page_line_tab was healthy,
**	or such a correction would not be possible.  When a page is structurally
**	patched, it is marked as page_stat = DMPP_FIXED.  If the strict
**	algorithm is run on this page before the file is modified to another
**	storage structure, then all tuples on this page will be thrown away.
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	None.
*/
I_DM5204_STRUCTURAL_DAMAGE:NO_GE
"Page %0d has been reformatted as an empty data page."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	All information on this page has been thrown away.  The page has been
**	reformatted as an empty data page, and may be resued at some future
**	date.
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	None.
*/
W_DM5205_INVALID_TID:NO_GE
"  TID:  page = %0d, line = %1d is deleted from page\n"
/*%
** Parameters:
**	%0d -	page number
**	%1d -	line number
** Explanation:
**	The check/patch operation has already verified that the page is
**	structurally sound.  However, for some reason, was unable to 
**	read the record (via normal DMF mechanisms).  Therefore, it cleared the
**	tid.
** System Status:
**	This is an informative message.
** User Action:
**	none.
*/
I_DM5206_BAD_ATTRIBUTE:NO_GE
" ZAPPED Attribute #%0d in tuple at TID:  page = %1d, line = %2d"
/*%
** Parameters:
**	%0d -	attribute number
**	%1d -	page number
**	%2d -	line number
** Explanation:
**	The data contained for this attribute is not legal for the attribute
**	type.  Example, a '\0' in a text string.  This data has been set to
**	whatever the correct empty value is for the data type.
** System Status:
**	This is an informative message.
** User Action:
**	none.
*/
W_DM5208_TOO_MANY_BAD_ATTS:NO_GE
"The TID (page=%0d, line=%1d) is deleted."
/*%
**  Parameters:
**	%0d - page number
**	%1d - line number
**  Explanation
**	The check/patch operation checks each attribute of each tuple to see
**	if the data contained there is valid for the attribute type.  If it
**	finds more than a single bad attribute in a tuple, then it simply
**	makes the assumption that the whole tuple is corrupted.  So, it deletes
**	the offending tuple from the page.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM5209_FREE_PAGE:NO_GE
"Page %0d is a btree free page."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	This message simply identifies which btree page is being processed, and
**	what type of page it is.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM520A_LEAF_PAGE:NO_GE
"Page %0d is a btree leaf page."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	This message simply identifies which btree page is being processed, and
**	what type of page it is.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM520B_LEAF_OVERFLOW:NO_GE
"Page %0d is a btree leaf overflow page."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	This message simply identifies which btree page is being processed, and
**	what type of page it is.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM520C_INDEX_PAGE:NO_GE
"Page %0d is a btree index page."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	This message simply identifies which btree page is being processed, and
**	what type of page it is.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM520D_ROOT_PAGE:NO_GE
"Page 0 is the btree root page."
/*%
**  Parameters:
**	none.
**  Explanation
**	This message simply identifies which btree page is being processed, and
**	what type of page it is.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM520E_FREE_HDR_PAGE:NO_GE
"Page 1 is the btree free list header page."
/*%
**  Parameters:
**	none.
**  Explanation
**	This message simply identifies which btree page is being processed, and
**	what type of page it is.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM520F_CHECK_LEAF:NO_GE
"CHECK TABLE will now examine btree leaf page # %0d."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	This identifies which btree page is being examined and that it is
**	an index page.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM5210_CHECK_BINDEX:NO_GE
"CHECK TABLE will now examine btree index page # %0d."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	Check Table walks the table twice for btrees.  The first pass simply
**	identifies which type of page each page is, and also checks free and
**	data pages.  The second pass checks index and leaf pages.  Since  these
**	pages are identified much earlier than they are processed, an
**	informative message is output when it is time to process the index page.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM5211_NUM_BTREE_PAGES:NO_GE
"This btree table has %0d pages.  CHECK TABLE will check examine page."
/*%
**  Parameters:
**	%0d - the number of pages in the btree table's file.
**  Explanation
**	Identifies the number of pages in the file.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM5212_STARTING_BORPHAN:NO_GE
"CHECK TABLE:  starting check for btree orphan pages."
/*%
**  Parameters:
**	none.
**  Explanation
**	Btree processing is complex and multipass.  This message is intended to
**	let the user know where they are in the check table process:
**	    a)  read in each page and identify page type (also check free/data pages)
**	    b)  check free page list header and free list chain.
**	    c)  check btree index/leaf/overflow pages
**	    d)	check for orphans
**	    e)	check leaf page sideways pointers.
**	Obviously, check table is starting step D.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM5213_CHECK_LEAF_PTRS:NO_GE
"CHECK TABLE:  starting check for btree orphan pages."
/*%
**  Parameters:
**	none.
**  Explanation
**	Btree processing is complex and multipass.  This message is intended to
**	let the user know where they are in the check table process:
**	    a)  read in each page and identify page type (also check free/data pages)
**	    b)  check free page list header and free list chain.
**	    c)  check btree index/leaf/overflow pages
**	    d)	check for orphans
**	    e)	check leaf page sideways pointers.
**	Obviously, check table is starting step E.
**  System Status;
**	This is an informative message.
**  User Action:
**	none.
*/
I_DM5214_UNABLE_TO_CK_ORPHANS:NO_GE
"There was not memory to allocate a bit map of referenced pages.\n
Therefore, the Check Table operation is unable to check for orphaned pages."
/*%
** Parameters:
**	none.
** Explanation:
**	Normally check table detects unreferenced pages and processed them.
**	It uses a bit map to do this.  In this case, patch table was not able
**	to allocate memory (1 bit per page for btree, 2 bits/page for hash/heap.
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	User may wish to reconfigure system to provide more memory for server.
**	If there were other diagnostic messages, then user may wish to patch
**	table instead.  (Patching the table takes much less memory than
**	checking it.)
*/
I_DM5215_EMPTY_LEAF:NO_GE
"WARNING:  Leaf page %0d is an empty page"
/*%
** Parameters:
**	%0d  - page number.
** Explanation:
**	This leaf page is empty
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	User may wish to run modify to merge on this btree if it turns out to
**	be healthy but has a lot of empty leaf pages.
*/
I_DM5216_EMPTY_LEAF_OVFL:NO_GE
"WARNING:  Leaf Overflow page %0d is an empty page"
/*%
** Parameters:
**	%0d  - page number.
** Explanation:
**	This leaf page is empty
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	User may wish to run modify to merge on this btree if it turns out to
**	be healthy but has a lot of empty leaf pages.
*/
I_DM5220_CHECK_IINDEX:NO_GE
"CHECK TABLE will now examine isam index page # %0d."
/*%
**  Parameters:
**	%0d - page number
**  Explanation
**	This identifies which isam page is being examined and that it is
**	an index page.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM5221_NUM_ISAM_PAGES:NO_GE
"This ISAM table has %0d pages.  CHECK TABLE will check examine page."
/*%
**  Parameters:
**	%0d - the number of pages in the isam table's file.
**  Explanation
**	Identifies the number of pages in the file.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM5222_STARTING_IORPHAN:NO_GE
"CHECK TABLE:  starting check for isam orphan pages."
/*%
**  Parameters:
**	none.
**  Explanation
**	first check table attempts to walk the index chain, starting at the
**	root page and going all the way down to the data pages.  Each time
**	check table references a page, it sets a bit in a bitmap indicating
**	that it has seen that page.  If the index chain is fully intact, then
**	all pages in the file have been visited.  This is an informative
**	message that check table is now checking that bit map to assure that
**	all pages in the file have been visited.  If it finds any unvisited
**	pages, it will call the appropriate routine to check them.
**  System Status;
**	This is an  informative message.
**  User Action:
**	none.
*/
I_DM53FB_CHECK_DONE:NO_GE
"Table Check Operation Completed."
/*%
** Parameters:
**	none.
** Explanation:
**	The Patch operation has completed.
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	None.
*/
I_DM53FC_PATCH_DONE:NO_GE
"Table Patch Operation Completed."
/*%
** Parameters:
**	none.
** Explanation:
**	The Patch operation has completed.
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	None.
*/
I_DM53FD_NOT_DATA_PAGE:NO_GE
"  PAGE %0d is a NOT a data page.  It will be converted to an empty data page."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	The Patch operation has encountered a non-data page.  It will convert
**	that page to an empty data page, throwing away anything that is
**	currently on the page.  This is normal healthy operation. (Patch throws
**	away btree and isam index pages.)
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	None.
*/
I_DM53FE_DATA_PAGE:NO_GE
"  PAGE %0d is a data page."
/*%
** Parameters:
**	%0d -	page number
** Explanation:
**	The Check or Patch operation has encountered a -data page.  It will
**	check the page for structural and data errors.  If any are encountered,
**	it will report the error and possibly throw away some/all tuples on
**	the page (depending on the specified operation)
** System Status:
**	This is an informative message printed during the check/patch operation.
** User Action:
**	None.
*/
I_DM8000_SEC_TABLE_ACCESS:NO_GE
"Attempt to Access TABLE"
I_DM8001_SEC_RECORD_ACCESS:NO_GE
"Attempt to Access RECORD: Record Id in HEX = '%0c'"
I_DM8002_SEC_DATABASE_ACCESS:NO_GE
"Attempt to access DATABASE"
I_DM8003_SEC_RECBYTID_ACCESS:NO_GE
"Attempt to Access RECORD by TID, ID is :%0c."
I_DM8004_SEC_RECBYKEY_ACCESS:NO_GE
"Attempt to Access RECORD by EXACT KEY, ID is:%0c."
I_DM8005_SEC_USER_ACCESS:NO_GE
"Attempt to Access INGRES. "
E_DM9000_BAD_FILE_ALLOCATE:NO_GE
"Disk file allocation error on
database:%0c table:%1c
pathname:%2c filename:%3c"
E_DM9001_BAD_FILE_CLOSE:NO_GE
"Disk file close error on
database:%0c table:%1c
pathname:%2c filename:%3c"
E_DM9002_BAD_FILE_CREATE:NO_GE
"Disk file create error on
database:%0c table:%1c
pathname:%2c filename:%3c"
E_DM9003_BAD_FILE_DELETE:NO_GE
"Disk file delete error on
database:%0c table:%1c
pathname:%2c filename:%3c"
E_DM9004_BAD_FILE_OPEN:NO_GE
"Disk file open error on
database:%0c table:%1c
pathname:%2c filename:%3c"
E_DM9005_BAD_FILE_READ:NO_GE
"Disk file read error on
database:%0c table:%1c
pathname:%2c filename:%3c page:%4d"
E_DM9006_BAD_FILE_WRITE:NO_GE
"Disk file write error on
database:%0c table:%1c
pathname:%2c filename:%3c page:%4d"
E_DM9007_BAD_FILE_SENSE:NO_GE
"Disk file sense error on
database:%0c table:%1c
pathname:%2c filename:%3c"
E_DM9008_BAD_FILE_FLUSH:NO_GE
"Disk file flush error on
database:%0c table:%1c
pathname:%2c filename:%3c"
E_DM9009_BAD_FILE_RENAME:NO_GE
"Disk file rename error on
database:%0c
physical location:%1c
old:%2c
new:%3c"
E_DM900A_BAD_LOG_DBADD:NO_GE
"Error adding a database to the logging system for the server:%0x
on database:%1c
owner:%2c
physical location:%3c."
E_DM900B_BAD_LOG_ALTER:NO_GE
"Error altering the characteristics of the logging system,
characteristics:%0x."
E_DM900C_BAD_LOG_BEGIN:NO_GE
"Error trying to begin a transaction on the database %0x."
E_DM900D_BAD_LOG_CLOSE:NO_GE
"Error removing the server:%0x from the logging system."
E_DM900E_BAD_LOG_END:NO_GE
"Error trying to end the transaction:%0x."
E_DM900F_BAD_LOG_EVENT:NO_GE
"Error waiting for the logging system event:%0x."
E_DM9010_BAD_LOG_FORCE:NO_GE
"Error forcing the log page to disk for transaction:%0x."
E_DM9011_BAD_LOG_INIT:NO_GE
"Error initializing the logging system."
E_DM9012_BAD_LOG_OPEN:NO_GE
"Error adding a server to the logging system."
E_DM9013_BAD_LOG_POSITION:NO_GE
"Error positioning the log file for transaction:%0x."
E_DM9014_BAD_LOG_READ:NO_GE
"Error reading record from log file for transaction:%0x."
E_DM9015_BAD_LOG_WRITE:NO_GE
"Error writing record to log file for transaction:%0x."
E_DM9016_BAD_LOG_REMOVE:NO_GE
"Error removing the database:%0x from the logging system."
E_DM9017_BAD_LOG_SHOW:NO_GE
"Error trying to display the logging system information:%0x."
E_DM9018_BAD_LOG_ERASE:NO_GE
"Error initializing the log file, %0c."
E_DM9019_BAD_LOCK_ALTER:NO_GE
"Error altering the characteristics of the locking system,
characteristics:%0x."
E_DM901A_BAD_LOCK_CREATE:NO_GE
"Error creating a lock list."
E_DM901B_BAD_LOCK_RELEASE:NO_GE
"Error releasing the lock list:%0x."
E_DM901C_BAD_LOCK_REQUEST:NO_GE
"Error requesting a lock on mode:%0x for the lock list:%1x."
E_DM901D_BAD_LOCK_SHOW:NO_GE
"Error trying to display the locking system information:%0x."
E_DM901E_BAD_LOCK_INIT:NO_GE
"Error initializing the locking system."
E_DM901F_BAD_TABLE_CREATE:NO_GE
"Error creating the table:%1c
in database:%0c."
E_DM9020_BM_BAD_UNLOCK_PAGE:NO_GE
"Error unlocking the page:%0d
for table:%1c
in database:%2c
for lock list:%3x."
E_DM9021_BM_BAD_LOCK_PAGE:NO_GE
"Error locking the page:%0d
for table:%1c 
in database:%2c
with mode:%3d
for lock list:%4x."
E_DM9022_BM_BAD_PROTOCOL:NO_GE
"Buffer manager protocol error in the page:%0d
for table:%1c
in database:%2c
with mode:%3d
for lock list:%4x."
E_DM9023_ERROR_OPENING_TABLE:NO_GE
"Error opening the table:%1c
in database:%0c."
E_DM9024_ERROR_CLOSING_TABLE:NO_GE
"Error closing the table:%1c
in database:%0c."
E_DM9025_BAD_TABLE_DESTROY:NO_GE
"Error destroying the table:%1c
in database:%0c."
E_DM9026_REL_UPDATE_ERR:NO_GE
"Error updating iirelation system table on database %0c
while executing some other operation such as modify, index, etc.
Check consistency of system catalogs."
E_DM9027_INDEX_UPDATE_ERR:NO_GE
"Error updating iiindexes system table on database %0c
while executing some other operation such as modify, index, etc.
Check consistency of system catalogs."
E_DM9028_ATTR_UPDATE_ERR:NO_GE
"Error updating iiattribute system table on database %0c
while executing some other operation such as modify, index, etc.
Check consistency of system catalogs."
E_DM9029_UNEXPECTED_FILE:NO_GE
"When trying to relocate a table a file was found at the
new location %0c
with the same name %1c
as the file your are relocating.
Check the new location directory and delete any files which
should not exist at this location."
E_DM902A_BAD_JNL_CLOSE:NO_GE
"Error closing journal file for database %0c
file sequence %1d."
E_DM902B_BAD_JNL_CREATE:NO_GE
"Error creating journal file %1c
at location %0c
for sequence %2d."
E_DM902C_BAD_JNL_OPEN:NO_GE
"Error opening journal file for database %0c
file sequence %1d."
E_DM902D_BAD_JNL_READ:NO_GE
"Error reading journal file for database %0c
file sequence %1d block %2d."
E_DM902E_BAD_JNL_WRITE:NO_GE
"Error writing journal file for database %1c
file sequence %0d block %2d."
E_DM902F_BAD_JNL_UPDATE:NO_GE
"Error updating journal file for database %0c
file sequence %1d."
E_DM9030_BAD_JNL_FORMAT:NO_GE
"Format error in journal file for database %0c
file sequence %1d block %2d."
E_DM9031_BAD_JNL_LENGTH:NO_GE
"Record length error in journal file for database %0c
file sequence %1d block %2d."
E_DM9032_BAD_JNL_DELETE:NO_GE
"Error occurred trying to delete a journal file."
E_DM9033_BAD_JNL_DIRCREATE:NO_GE
"Error occurred trying to create a journal directory '%0c'."
E_DM9034_BAD_JNL_NOT_FOUND:NO_GE
"Error occurred trying to access a journal file."
E_DM9035_BAD_JNL_TRUNCATE:NO_GE
"Error occurred trying to truncate a journal file."
E_DM9036_JNLCKP_CREATE_ERROR:NO_GE
"Error checking or creating directories for this database in the JOURNAL, 
CHECKPOINT and DUMP locations.  The root directories for these locations
may not exist or may not be accessible.  Check definitions of environment
variables used to point at these locations (II_JOURNAL, II_CHECKPOINT,
II_DUMP), existence of the root directories, and permissions to access them."
/*%
** Parameters:
**      None.
**
** Explanation:
**	An error was encountered in the routine which checks/creates
**	subdirectories for the database in the journal, checkpoint and
**	dump locations.
**
**	This is done when checkpointing a database.
**
**	This error could be caused by the non-existence of some ingres
**	parent directory (for example the II_SYSTEM/ingres/dmp directory),
**	or the lack of definition of an ingres environment variable:
**	II_JOURNAL, II_CHECKPOINT, II_DUMP.
**
**	This error should be preceded by an error message which should
**	give more specific information about the error.
**
** System Status:
**	The Checkpoint operation will fail.  The database state will remain
**	unchanged.
**
** User Action:
**	Check error log for specific reason of failure.  When error is
**	corrected, re-execute the checkpoint command.
*/
E_DM9040_BAD_CKP_DIRCREATE:NO_GE
"Error occurred trying to create a checkpoint directory."
E_DM9041_LK_ESCALATE_TABLE:NO_GE
"Maximum number of locks for transaction exceeded.
Escalating to table level lock on table %0c
in database %1c. "
E_DM9042_PAGE_DEADLOCK:NO_GE
"Deadlock encountered locking page %0d
for table %1c
in database %2c
with mode %3d."
E_DM9043_LOCK_TIMEOUT:NO_GE
"Timeout occurred during lock request on table %0c
in database %1c
with mode %2d."
E_DM9044_ESCALATE_DEADLOCK:NO_GE
"Deadlock encountered while escalating to table level locking on table %0c
in database %1c
with mode %2d."
E_DM9045_TABLE_DEADLOCK:NO_GE
"Deadlock encountered locking table %0c
in database %1c
with mode %2d."
E_DM9046_TABLE_NOLOCKS:NO_GE
"Lock quota exceeded with lock request on table %0c
in database %1c."
E_DM9047_LOCK_RETRY:NO_GE
"Lock is held by a run away server. 
Recover the transactions on the run away server and retry."
E_DM9048_DEV_UPDATE_ERROR:NO_GE
"An error occurred while trying to update the iidevices table."
E_DM9049_UNKNOWN_EXCEPTION:NO_GE
"An Unexpected Exception occurred in the DMF Facility, exception number %0d."
E_DM904A_FATAL_EXCEPTION:NO_GE
"A Fatal Error has occurred in the DMF Facility."
E_DM904B_BAD_LOCK_EVENT:NO_GE
"Error requesting lock event with flag %0d and value %1d for lock list %2x."
E_DM904C_ERROR_GETTING_RECORD:NO_GE
"Error getting a record from 
database:%0c, owner:%1c, table:%2c."
E_DM904D_ERROR_PUTTING_RECORD:NO_GE
"Error putting a record to 
database:%0c, owner:%1c, table:%2c."
E_DM904E_BAD_LOG_COPY:NO_GE
"Error copying one log file to the other."
E_DM9100_LOAD_TABLE_ERROR:NO_GE
"Error occurred while loading a table for a modify, index, or load table
operation."
E_DM9101_UPDATE_CATALOGS_ERROR:NO_GE
"Error occurred while executing the update system tables phase for
a modify or index table operation."
E_DM9102_BAD_MSG_FORMAT:NO_GE
"Error occurred while attempting to format a message during a Check/Patch Table
operation."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dm1u_talk encountered a message from ule_format while attempting to
    **	format a warning or informative message from the check/patch table
    **	operation.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  user should assure they have a valid readable slow msg file.
*/
E_DM9103_ERR_SENDING_MSG:E_GE9088_COMM_ERROR
"Error occurred while attempting to send a check/patch table message from DMF
to the FE via SCC_MESSAGE"
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dm1u_talk encountered a message from SCF while attempting to
    **	send a warning or informative message to the verifydb frontend.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Assure that server resources are available and that GCN is functioning.
*/
E_DM9104_ERR_CK_PATCH_TBL:NO_GE
"Error occurred while attempting to check/patch a table."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dm1u_table encountered an error while attempting to check or patch a
    **	table.  This is a generic catch-all type of error message that all
    **	lower level DMF INTERNAL error messages are translated to, so that
    **	the error exit path through DMF code can be traced for debugging
    **	purposes.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  User should look at the server error log to determine what DMF internal
    **	error was translated to this message, and take the appropriate action
    **	for that error.
*/
E_DM9105_ERR_DEALLOC_MEMORY:E_GE8D04_NO_RESOURCE
"Error occurred while attempting to deallocate memory that was previously allocated."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **	dm1u_table was unable to deallocate memory (via a dm0m_deallocate call)
    **	that it had previously allocated (via a dm0m_allocate call).
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Memory is corrupted.  The user should shut down the server and restart
    **	it at the first reasonable opportunity.
*/
E_DM9106_ERR_ALLOC_MEMORY:E_GE8D04_NO_RESOURCE
"Error occurred while attempting to allocate memory via a dm0m_allocate call."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **	dm1u_table was unable to allocate memory (via a dm0m_deallocate call).
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Memory is corrupted.  The user should shut down the server and restart
    **	it at the first reasonable opportunity.
*/
E_DM9107_BAD_DM1U_COMPRESS:NO_GE
"Error occurred while compressing a record that was being patched."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dm1u_datapg had determined that an attribute in a tuple was invalid
    **	during a patch operation, and was attempting to replace that bad
    **	attribute with a null or empty value.  The table had compressed records,
    **	so dm1u_datapg had to compress the record before replacing it on the
    **	page (via a call to dm1c_comp_rec).  Unfortunately, dm1c_comp_rec
    **	returned an error status.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Previous message from dm1c_comp_rec will indicate error condition (out
    **	of locks, etc).  User should correct that condition.
*/
E_DM9108_FIXED_PAGE:NO_GE
"CONSISTENCY CHECK:  there should not be any fixed pages when dm1u_dchain is\n
called.  However, page %0x is fixed."
/*%
    ** Parameters:
    **  %0x - address of fixed page.
    ** Description:
    **	dm1u_dchain will fix and unfix pages to _RCB->rcb_data_page_ptr as it
    **	does its work.  Therefore, no pages should be fixed to the rcb when
    **	dm1u_dchain is called, or information about that fixed page will be
    **	overwritten during dm1u_dchain processing.  If there is a fixed page
    **	before dm1u_dchain is called, then the information about that fixed page
    **	will be lost, and DMF will not be able to unfix the page.  This will
    **	cause a server crash when DMF goes to close the table and the buffer
    **	manager detects that a page is still fixed.
    ** System Status:
    **  The statement (execute iiQEF_check_table) is aborted.
    ** User Action:
    **	This error should not be able to occur.  If it does occur, then either
    **	of two conditions have occurred:  1) it is a coding error that must be
    **	filed as a bug and corrected or 2) there is memory corruption in the
    **	RCB, which will cause all sorts of DMF failures.  The user should call
    **	Ingres Corp. Tech Support.
*/
E_DM9109_BAD_RELMAIN:NO_GE
"CONSISTENCY CHECK: iirelation says that there are %0d main pages in the table\n
but there are only %1d pages in the table's disk file."
/*%
    ** Parameters:
    **  %0d - iirelation.relmain
    **	%1d - number of pages in the file
    ** Description:
    **	In order to check a hash table, we must know how many main pages there
    **	are in that hash table.  (Each main page may have 1 or more overflow
    **	pages associated with it).  The value of iirelation.relmain tells DMF
    **	how many main pages are in this hash table.  In this case, the
    **	iirelation.relmain value is not valid.  It is larger than the total
    **	number of pages in the file.
    ** System Status:
    **  The statement (execute iiQEF_check_table) is aborted.
    ** User Action:
    **	The user can update iirelation to have the correct values, if the user
    **	knows what those values should be (that is unlikely).  The other option
    **	is to run the Patch Table operation, which will convert the table to
    **	heap and update iirelation to have correct table description.  Then
    **	the user may modify the table to whatever storage structure s/he
    **	desires.
*/
E_DM9180_CANT_ADD_LIST_TUPLE:NO_GE
"Error occurred while placing DI list information into user table."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dm2r_put was unable to place formatted tuple into open table.  That
    **	tuple contained a filename found from DIlistdir or DIlistfile and
    **	nulls/default for any other columns in the file.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Previous message from dm2r_put will indicate error condition (out of
    **	locks, etc).  User should correct that condition.
*/
E_DM9181_SPECIFY_DEL_FILE:NO_GE
"DMM_DELETE input error: caller must specify the name of the file to delete."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  DMM_DELETE's dmm_cb was missing dmm_db_location or dmm_filename.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  None.
*/
E_DM9182_CANT_MAKE_LIST_TUPLE:NO_GE
"Error occurred while formatting a tuple that contains a file or directory
name found by DIlistfile or DIlistdir."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  ADF error occurred while attempting to format a tuple with a filename
    **	that DIlistdir/DIlistfile found.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  None.
*/
E_DM9183_DILIST_ERR:NO_GE
"Error occurred while performing a DIlistdir or DIlistfile operation."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  DIlistdir/DIlistfile encountered a system error while attempting to
    **	list files subordinate to a specified directory.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  None.
*/
E_DM9184_DMMLIST_FAILED:NO_GE
"An internal error occurred in dmm_makelist."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dmm_makelist encounted an error while attempting to list
    **	files/directories subordinate to user specified directory.  An earlier
    **	more explicit error will have been logged and will be translated to this
    **	error
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Verify the iiQEF_listdir or iiQEF_listfile parameters were valid.
*/
E_DM9185_CANT_LIST_DIR:NO_GE
"Error occurred while attempting a DIlistdir or DIlistfile operation."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dmm_makelist detected an error after calling DIlistfile or DIlistdir.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  none.
*/
E_DM9186_LISTDIR_NONEXISTENT:NO_GE
"DMM_LIST input error: Specified directory does not exist."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  DIlistdir/DIlistfile was not able to list the directory because it
    **	did not exist.
    ** System Status:
    **  The statement is aborted
    ** User Action:
    **  Reissue iiQEF_listfile with valid directory name.
*/
E_DM9187_INVALID_LIST_TBL_FMT:NO_GE
"DMM_LIST input error: Specified LIST COLUMN is not the right size and/or type
to hold a filename, or does not exist in specified table."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  The dmm_list function puts filenames into a specified column of a
    **	user table.  In this case, the user table does not contain the specified
    **	column or it is not the correct type/size to hold a filename.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Specify a valid column name for iiQEF_listfile procedure.
*/
E_DM9188_DMM_ALLOC_ERROR:NO_GE
"Internal error occurred while allocating memory for DMM list operation."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  Dmm_makelist was not able to allocate memory.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  This is probably due to a system resource problem.  User should fix
    **	system resource problem.
*/
E_DM9189_DMM_LISTTAB_OPEN_ERR:NO_GE
"Error occurred opening list table"
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dmm_makelist was not able to open the user specified table to list
    **	filenames to.  However, the table does exist.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  none.
*/
E_DM918A_LISTTAB_NONEXISTENT:NO_GE
"DMM_LIST input error: Specified List table does not exist."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dmm_list does a DIlistdir/DIlistfile and puts filenames it finds into
    **	a user specified table.  This table does not exist.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Reissue command with table that exists.
*/
E_DM918B_DMM_SYSCAT_ERROR:NO_GE
"Error occurred while attempting to access system catalog information about a
user table."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  dmm_makelist encountered an error while attempting to read information
    **	about a user table from iirel_idx or iirelation.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  none.
*/
E_DM918C_SPECIFY_LIST_COLNAME:NO_GE
"DMM_LIST input error: dmm_att_name parameter should have been specified and was not."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  The dmm_cb did not have the dmm_att_name parameter specified.  This
    **	means that QEF built the control block incorrectly.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  None.
*/
E_DM918E_SPECIFY_LIST_TABLE:NO_GE
"DMM_LIST input error: dmm_table_name or dmm_owner parameter should have been
specified was not"
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  The dmm_cb did not have the dmm_table_name or dmm_owner parameter 
    **	specified.  This means that QEF built the control block incorrectly.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  none.
*/
E_DM918F_ILLEGAL_DEL_FIL:NO_GE
"It is illegal to delete a dbms core catalog or dbms system catalog."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  The specified delete file is really a core catalog or a dbms catalog.
    **	It is not legal to delete these via dmm_delete.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  Do not attempt to delete this file, the server still needs it.
*/
E_DM9190_BAD_FILE_DELETE:NO_GE
"Internal error was encountered when attempting to DIdelete specified file."
/*%
    ** Parameters:
    **  None.
    ** Description:
    **  DIdelete encountered an error when attempting to delete the file.
    **	DMM_DELETE logs DIdelete's error, then translates it to this message.
    ** System Status:
    **  The statement is aborted.
    ** User Action:
    **  none.
*/
E_DM9200_BM_BUSY:NO_GE
"Error deallocating the buffer manager.
Buffer manager is still busy."
E_DM9201_BM_CLOSE_PAGE_ERROR:NO_GE
"Error writing pages to disk when close a table."
E_DM9202_BM_BAD_FILE_PAGE_ADDR:NO_GE
"Error specifying an invalid page number to access."
E_DM9203_DM2R_RCB_ALLOC:NO_GE
"Error allocating a RCB."
E_DM9204_BM_FIX_PAGE_ERROR:NO_GE
"Error fixing a page."
E_DM9205_BM_INVALIDATE_PAGE_ERR:NO_GE
"Error invalidating a page."
E_DM9206_BM_BAD_PAGE:NO_GE
"Error specifying a bad page buffer address."
E_DM9207_BM_INTERNAL_ERROR:NO_GE
"Buffer manager internal error."
E_DM9208_BM_UNFIX_PAGE_ERROR:NO_GE
"Error unfixing a page."
E_DM9209_BM_NO_CACHE_LOCKS:NO_GE
"Out of buffer manager cache locks."
E_DM920A_BM_ESCALATE_LOCK_ERR:NO_GE
"Error escalating page level lock to table level lock for a table."
E_DM920B_BM_NO_BUFFERS:NO_GE
"Out of buffer manager buffers."
E_DM920C_BM_BAD_FAULT_PAGE:NO_GE
"Error faulting a page."
E_DM920D_BM_BAD_GROUP_FAULTPAGE:NO_GE
"Error faulting a group of pages."
E_DM920E_BM_FORCE_PAGE_ERROR:NO_GE
"Error forcing a page to disk."
E_DM920F_BM_GROUP_FORCEPAGE_ERR:NO_GE
"Error forcing a group of pages to disk."
E_DM9210_BM_VALIDATE_PAGE_ERROR:NO_GE
"Error validating a page."
E_DM9211_DM0L_BT:NO_GE
"Error logging BEGIN TRANSACTION operation."
E_DM9212_DM0L_ROBT:NO_GE
"Error logging begin readonly transaction operation."
E_DM9213_DM0L_ET:NO_GE
"Error logging END TRANSACTION operation."
E_DM9214_DM0L_BI:NO_GE
"Error logging before image."
E_DM9215_DM0L_PUT:NO_GE
"Error logging APPEND operation."
E_DM9216_DM0L_DEL:NO_GE
"Error logging DELETE operation."
E_DM9217_DM0L_REP:NO_GE
"Error logging REPLACE operation."
E_DM9218_DM0L_SBI:NO_GE
"Error logging system catalog before image."
E_DM9219_DM0L_SPUT:NO_GE
"Error logging system catalog APPEND operation."
E_DM921A_DM0L_SDEL:NO_GE
"Error logging system catalog DELETE operation."
E_DM921B_DM0L_SREP:NO_GE
"Error logging system catalog REPLACE operation."
E_DM921C_DM0L_BM:NO_GE
"Error logging begin mini transaction operation."
E_DM921D_DM0L_EM:NO_GE
"Error logging end mini transaction operation."
E_DM921E_DM0L_BALLOC:NO_GE
"Error logging BTREE page allocation operation."
E_DM921F_DM0L_BDEALLOC:NO_GE
"Error logging BTREE page deallocation operation."
E_DM9220_DM0L_CALLOC:NO_GE
"Error logging common page allocation operation."
E_DM9221_DM0L_SAVEPOINT:NO_GE
"Error logging savepoint declaration."
E_DM9222_DM0L_ABORTSAVE:NO_GE
"Error logging abort to savepoint operation."
E_DM9223_DM0L_FRENAME:NO_GE
"Error logging file rename operation."
E_DM9224_DM0L_FCREATE:NO_GE
"Error logging file create operation."
E_DM9225_DM0L_OPENDB:NO_GE
"Error logging database open operation."
E_DM9226_DM0L_CLOSEDB:NO_GE
"Error logging database close operation."
E_DM9227_DM0L_PHASE1:NO_GE
"Error logging the begin of the first phase for two phase commit operation."
E_DM9228_DM0L_CONFIG:NO_GE
"Error logging the configuration file update operation."
E_DM9229_DM0L_CREATE:NO_GE
"Error logging table CREATE operation."
E_DM922A_DM0L_DESTROY:NO_GE
"Error logging table DESTROY operation."
E_DM922B_DM0L_RELOCATE:NO_GE
"Error logging table RELOCATE operation."
E_DM922C_DM0L_MODIFY:NO_GE
"Error logging table MODIFY operation."
E_DM922D_DM0L_INDEX:NO_GE
"Error logging table INDEX operation."
E_DM922E_DM0L_POSITION:NO_GE
"Error positioning the log file."
E_DM922F_DM0L_READ:NO_GE
"Error reading the log file."
E_DM9230_DM0L_FORCE:NO_GE
"Error forcing the log record to disk."
E_DM9231_DM0L_LOGFORCE:NO_GE
"Error forcing the last log page to disk."
E_DM9232_DM0L_ALLOCATE:NO_GE
"Error initializing the logging system and allocate the logging context."
E_DM9233_DM0L_DEALLOCATE:NO_GE
"Error terminating the logging system and deallocate the logging context."
E_DM9234_DM0L_BPEXTEND:NO_GE
"Error logging BTREE file extend operation."
E_DM9235_DM0L_BCP:NO_GE
"Error logging begin consistency point operation."
E_DM9236_DM0L_SM1_RENAME:NO_GE
"Error logging SYSMOD file rename operation."
E_DM9237_DM0L_SM2_CONFIG:NO_GE
"Error logging SYSMOD configuration file update operation."
E_DM9238_LOG_READ_EOF:NO_GE
"End of the log file is encountered."
E_DM9239_BM_BAD_PARAMETER:NO_GE
"Bad parameters to buffer manager."
E_DM923A_CONFIG_OPEN_ERROR:NO_GE
"Error occurred opening configuration file, (aaaaaaaa.cnf)."
E_DM923B_CONFIG_CLOSE_ERROR:NO_GE
"Error occurred closing the configuration file, (aaaaaaaa.cnf)."
E_DM923C_CONFIG_FORMAT_ERROR:NO_GE
"Error in configuration file format."
E_DM923D_DM0M_NOMORE:NO_GE
"DMF memory exhausted."
E_DM923E_DM0M_SIZE:NO_GE
"Size of 0 passed to memory allocator."
E_DM923F_DM2F_OPEN_ERROR:NO_GE
"Error occurred opening a file for a table."
E_DM9240_DM2F_CLOSE_ERROR:NO_GE
"Error occurred closing a file for a table."
E_DM9241_DM2F_BAD_LOCATION:NO_GE
"Error finding location for table."
E_DM9242_DM1R_EXTEND:NO_GE
"Error occurred while extending a heap, isam or hash table."
E_DM9243_DM1R_PUT:NO_GE
"Error occurred while putting a record to a heap, isam or hash table."
E_DM9244_DM1R_REPLACE:NO_GE
"Error occurred while replacing a record in a heap, isam or hash table."
E_DM9245_DM1S_ALLOCATE:NO_GE
"Error occurred allocating disk space for a heap table."
E_DM9246_DM1S_DEALLOCATE:NO_GE
"Error occurred deallocating disk space for a temporary table."
E_DM9247_DM1H_ALLOCATE:NO_GE
"Error occurred allocating disk space for a hash table."
E_DM9248_DM1H_SEARCH:NO_GE
"Error occurred searching a hash table."
E_DM9249_DM1I_ALLOCATE:NO_GE
"Error occurred allocating disk space for a isam table."
E_DM924A_DM1I_SEARCH:NO_GE
"Error occurred searching a isam table."
E_DM924B_DM1S_BBEGIN:NO_GE
"Error occurred beginning to build a heap table."
E_DM924C_DM1S_BEND:NO_GE
"Error occurred ending the build of a heap table."
E_DM924D_DM1S_BPUT:NO_GE
"Error occurred adding a record to a heap table being built."
E_DM924E_DM1H_BBEGIN:NO_GE
"Error occurred beginning to build a hash table."
E_DM924F_DM1H_BEND:NO_GE
"Error occurred ending the build of a hash table."
E_DM9250_DM1H_BPUT:NO_GE
"Error occurred adding a record to hash table being built."
E_DM9251_DM1I_BADD:NO_GE
"Error occurred adding disk space to an isam table during a build."
E_DM9252_DM1I_BEND:NO_GE
"Error occurred ending the build of a isam table."
E_DM9253_DM1I_BOTTOM:NO_GE
"Error occurred building the index of an isam table."
E_DM9254_DM1B_EXTEND:NO_GE
"Error occurred extending the disk file for a btree table."
E_DM9255_DM1B_FREEDATA:NO_GE
"Error occurred adding a page to the free list."
E_DM9256_DM1B_FINDFREE:NO_GE
"Error occurred finding a page on the free list."
E_DM9257_DM1B_FINDDATA:NO_GE
"Error occurred finding disk space for a new record."
E_DM9258_DM1B_MERGE:NO_GE
"Error occurred merging the index of a btree table."
E_DM9259_DM1B_BEND:NO_GE
"Error occurred completing the build of a btree table."
E_DM925A_DM1B_BEGIN:NO_GE
"Error occurred preparing to build a btree table."
E_DM925B_DM1B_BPUT:NO_GE
"Error occurred adding a record to a btree table being built."
E_DM925C_DM1B_BADD:NO_GE
"Error occurred adding disk space to a btree table during a build."
E_DM925D_DM1B_BOTTOM:NO_GE
"Error occurred adding or updating an index page in a btree during a build."
E_DM925E_DM1B_ALLOCATE:NO_GE
"Error occurred allocating disk space for a new record."
E_DM925F_DM1B_GETBYBID:NO_GE
"Error occurred accessing a record by it's index address."
E_DM9260_DM1B_DELETE:NO_GE
"Error occurred deleting a record."
E_DM9261_GET:NO_GE
"Error occurred getting a record."
E_DM9262_DM1B_PUT:NO_GE
"Error occurred adding a record."
E_DM9263_DM1B_REPLACE:NO_GE
"Error occurred replacing a record."
E_DM9264_DM1B_SEARCH:NO_GE
"Error occurred searching the btree index."
E_DM9265_ADD_DB:NO_GE
"Error occurred while trying to add a database."
E_DM9266_UNLOCK_CLOSE_DB:NO_GE
"Error occurred unlocking a database during a close."
E_DM9267_CLOSE_DB:NO_GE
"Error occurred closing a database."
E_DM9268_OPEN_DB:NO_GE
"Error occurred opening a database."
E_DM9269_LOCK_OPEN_DB:NO_GE
"Error occurred locking a database during a open."
E_DM926A_TBL_CACHE_LOCK:NO_GE
"Error occurred cache locking a table."
E_DM926B_BUILD_TCB:NO_GE
"Error occurred while building a TCB."
E_DM926C_TBL_CLOSE:NO_GE
"Error occurred during a close table."
E_DM926D_TBL_LOCK:NO_GE
"Error occurred locking a table."
E_DM926E_RECLAIM_TCB:NO_GE
"Error occurred while reclaiming unused TCB's."
E_DM926F_TBL_CACHE_CHANGE:NO_GE
"Error occurred updating table cache lock."
E_DM9270_RELEASE_TCB:NO_GE
"Error occurred releasing a TCB."
E_DM9271_BUILD_TEMP_TCB:NO_GE
"Error occurred building a TCB for a temporary table."
E_DM9272_REL_UPDATE_ERROR:NO_GE
"Error occurred updating a system table."
E_DM9273_TBL_UPDATE:NO_GE
"Warning due to resource shortages an error occurred updating 
the tuple count information for a table."
E_DM9274_TBL_VERIFY:NO_GE
"Error occurred verifying a cache table."
E_DM9275_DM2R_LOAD:NO_GE
"Error adding a record to a sort table."
E_DM9276_TBL_OPEN:NO_GE
"Error occurred opening a table."
E_DM9277_DM0L_LOCATION:NO_GE
"Error occurred writing a log record for adding a location."
E_DM9278_DM0L_SETBOF:NO_GE
"Error occurred positioning Log File BOF."
E_DM9279_DM0L_JNLEOF:NO_GE
"Error occurred updating Log File EOF."
E_DM927A_DM0L_ARCHIVE:NO_GE
"Error occurred logging archive operation."
E_DM927B_DM0L_STARTDRAIN:NO_GE
"Error occurred starting drain operation."
E_DM927C_DM0L_ENDDRAIN:NO_GE
"Error occurred ending drain operation."
E_DM927D_DM0L_LOAD:NO_GE
"Error occurred writing a log record for LOAD TABLE."
E_DM927E_UPDATE_TCB:NO_GE
"Error updating TCB tuple and pages counts for system catalog."
E_DM927F_DCB_INVALID:NO_GE
"An Error closing the database '%0c' has made it unavailable by this server.
In order to access this database, the server should be shut down and a new
server started up."
E_DM9280_DM0J_CLOSE_ERROR:NO_GE
"Error occurred closing a journal file."
E_DM9281_DM0J_CREATE_ERROR:NO_GE
"Error occurred creating a journal file."
E_DM9282_DM0J_OPEN_ERROR:NO_GE
"Error occurred opening a journal file."
E_DM9283_DM0J_READ_ERROR:NO_GE
"Error occurred reading a journal file."
E_DM9284_DM0J_WRITE_ERROR:NO_GE
"Error occurred writing a journal file."
E_DM9285_DM0J_UPDATE_ERROR:NO_GE
"Error occurred updating a journal file."
E_DM9286_DM0J_FORMAT_ERROR:NO_GE
"Error occurred in the journal file format."
E_DM9287_DM0J_LENGTH_ERROR:NO_GE
"Error occurred in the length of a journal record."
E_DM9288_TBL_CTRL_LOCK:NO_GE
"Error requesting a table control lock."
E_DM9289_BM_BAD_MUTEX_PROTOCOL:NO_GE
"Page mutex protocol error."
E_DM928A_CONFIG_EXTEND_ERROR:NO_GE
"Error occurred extending the configuration file, (aaaaaaaa.cnf)."
E_DM928B_CONFIG_NOT_COMPATABLE:NO_GE
"Configuration file is not compatible with the current version of INGRES.  
Config version = %0d.%1d, INGRES version = %2d.%3d."
E_DM928C_CONFIG_CONVERT_ERROR:NO_GE
"Error converting config from Ingres 6.0 to Ingres 6.3 format.  Config file
could not be converted.  Database cannot be used by Ingres 6.3 until it has
been converted."
/*
** Parameters:
**	none
** Explanation:
**	An error was encountered while converting the config file from 6.0
**	to 6.3 format.  This could be because of an IO error, the config
**	file could need to be extended and there is no disk space, the
**	server could have run out of memory, or there could be a format
**	error in the old config file.  There should be a previous error
**	message in the error log detailing exactly what went wrong.
**
**	This problem should only occur the first time a database that was
**	created by Ingres 6.0 is opened by Ingres 6.3. When this is done
**	the system automatically converts the 6.0 config file to 6.3 format.
**
** System Status:
**	The database will not be accessible by Ingres 6.3 until it can be
**	properly converted.
** User Action:
**	The problem must be resolved and the config file converted in order
**	to access the database with Ingres 6.3.  If all else fails, the database
**	can be unloaded, then reloaded with a new 6.3 database.
*/
E_DM928D_NO_DUMP_LOCATION:NO_GE
"Could not find dump location in config file location list in order to
perform an on-line backup operation."
/*
** Parameters:
**	none
** Explanation:
**	During online backup processing, the config file is written to the
**	dump location.  This error is written if the config write routine
**	cannot find the dump location in the config file's extent list.
**
** System Status:
**	On line backup operation will not succeed.
** User Action:
**	May be a config file format problem.  Not much the user can do to
**	fix problem other than to unload and reload data in a new database -
**	or go back to a checkpoint that did not have this problem.
*/
E_DM928E_CONFIG_FORMAT_ERROR:NO_GE
"Error in configuration file format--
  size = %0d, rel1check = %1d, rel2check = %2d attcheck = %3d, indcheck = %4d."
/*
** Parameters:
**	%0  size of structures
**	%1  first iirelation check
**	%2  second iirelation check
**	%3  iiattribute check
**	%4  iiindex check
** Explanation:
**	The config file, aaaaaaaa.cnf, is inconsistent with the core system
**	catalogs.
** System Status:
**	The database is inconsistent.
** User Action:
**	May be a config file format problem.  Not much the user can do to
**	fix problem other than to unload and reload data in a new database -
**	or go back to a checkpoint that did not have this problem.
*/
E_DM9290_DM2F_DELETE_ERROR:NO_GE
"Error occurred while attempting to delete a file."
E_DM9291_DM2F_FILENOTFOUND:NO_GE
"When attempting to open a file, the file did not exist."
E_DM9292_DM2F_DIRNOTFOUND:NO_GE
"When attempting to open a file, the directory did not exist."
E_DM9293_DM2F_BADDIR_SPEC:NO_GE
"When attempting to open a file, the directory specification was incorrect."
E_DM9294_DM2R_BADLOGICAL:NO_GE
"II_SRT_IN_DEV value is invalid for this database."
E_DM9295_DM2U_BADLOGICAL:NO_GE
"II_SRT_OUT_DEV value is invalid for this database."
E_DM9296_DM0L_ASSOC:NO_GE
"Error logging page ASSOC operation."
E_DM9297_DM0L_ALLOC:NO_GE
"Error logging page ALLOC operation."
E_DM9298_DM0L_DEALLOC:NO_GE
"Error logging page DEALLOC operation."
E_DM9299_DM0L_EXTEND:NO_GE
"Error logging table EXTEND operation."
E_DM9300_DM0P_CLOSE_PAGE:NO_GE
"Buffer still fixed."
E_DM9301_DM0P_FORCE_PAGES:NO_GE
"Buffer still fixed."
E_DM9302_DM0P_INVALIDATE:NO_GE
"Page is fixed or busy. "
E_DM9303_DM0P_MUTEX:NO_GE
"Inconsistent page and buffer."
E_DM9304_DM0P_UNMUTEX:NO_GE
"Inconsistent page and buffer."
E_DM9305_DM0P_TRAN_INVALIDATE:NO_GE
"Page is fixed or busy."
E_DM9306_DM0P_UNFIX_PAGE:NO_GE
"Bad page address."
E_DM9307_DM0P_WBEHIND_SIGNAL:NO_GE
"Error signaling Write Behind Threads to wakeup."
E_DM9308_DM0P_TOSS_PAGES:NO_GE
"Error tossing page from Buffer Manager.  Database %0d, table (%1d, %2d),
page number %3d."
E_DM9309_DM0P_RECLAIM:NO_GE
"Error tossing page from Buffer Manager in order to free up cache locks."
E_DM930A_DM0P_ALLOCATE:NO_GE
"Error starting up the Buffer Manager."
E_DM930B_DM0P_DEALLOCATE:NO_GE
"Error shutting down the Buffer Manager."
E_DM9310_DM2R_RELEASE_RCB:NO_GE
"Not a rcb."
E_DM9311_DM2R_DELETE:NO_GE
"Not a rcb or tcb."
E_DM9312_DM2R_GET:NO_GE
"Not a rcb. "
E_DM9313_DM2R_POSITION:NO_GE
"Not a rcb or tcb."
E_DM9314_DM2R_PUT:NO_GE
"Not a rcb or tcb."
E_DM9315_DM2R_REPLACE:NO_GE
"Not a rcb or tcb."
E_DM9316_DM2R_UNFIX_PAGES:NO_GE
"Error occurred releasing page held in rcb during query to system catalog."
E_DM9317_DM2R_TID_ACCESS_ERROR:NO_GE
"Error with record access mode.  Attempt was made to reference a table record
with a TID qualifier on a table that does not support TID access."
/*
** Parameters:
**	None
**
** Explanation:
**	A table that does not support tids (probably a gateway table) was
**	queried with a tid qualifier.  This could be caused by the server
**	choosing a bad query plan which uses TID access inappropriately, or
**	could be caused by the user specifying a query with a tid qualifier.
**
** System Status:
**	The query will be rejected.
**
** User Action:
**	If the user is using queries with a TID qualifier on a table which does
**	not contain TID's, then the user should restructure the query.
*/
E_DM931F_DM2T_RENAME_FILE:NO_GE
"Error occurred renaming load file during a LOAD operation."
E_DM9320_DM2T_BUILD_TCB:NO_GE
"Not a dcb. "
E_DM9321_DM2T_CLOSE:NO_GE
"Not a rcb. "
E_DM9322_DM2T_FIND_TCB:NO_GE
"Not a dcb. "
E_DM9323_DM2T_OPEN:NO_GE
"Not a dcb. "
E_DM9324_DM2T_DEALLOCATE_TCB:NO_GE
"Not a tcb. "
E_DM9325_DM2T_UPDATE_REL:NO_GE
"Not a tcb. "
E_DM9326_DM2T_VERIFY_TCB:NO_GE
"Not a tcb. "
E_DM9327_BAD_DB_OPEN_COUNT:NO_GE
"The database open count was greater than zero for the first opener
of the database, therefore the database must be considered inconsistent.
This can happen when the log file was not readable, therefore if any
transaction were in progress they could not be backed out.  This database
must be rolled forward from a checkpoint or destroyed and recreated."
E_DM9328_DM2T_NOFREE_TCB:NO_GE
"Cannot free resources, no TCB's to reclaim."
E_DM9329_DM0J_M_ALLOCATE:NO_GE
"An allocation error occurred trying to merge the cluster journal files."
E_DM932A_DM0J_M_LIST_FILES:NO_GE
"An error occurred trying to list the cluster journal files to merge. "
E_DM932B_DM0J_M_NO_FILES:NO_GE
"The config file indicates there should be cluster journal 
files to merge but none were found."
E_DM932C_DM0J_M_BAD_NODE_JNL:NO_GE
"An invalid node id was found for one of the journal cluster files. "
E_DM932D_DM0J_M_BAD_READ:NO_GE
"An error occurred reading one of the cluster journal files. "
E_DM932E_DM0J_M_BAD_RECORD:NO_GE
"An illegal record was found when reading one of the cluster journal files. "
E_DM932F_DM0L_ALTER:NO_GE
"An error occurred trying to log an ALTER log record. "
E_DM9330_DM2T_PURGE_TCB:NO_GE
"An error occurred while invalidating a TCB."
E_DM9331_DM2T_TBL_IDX_MISMATCH:NO_GE
"Base table missing or indices missing."
E_DM9332_DM2T_IDX_DOMAIN_ERROR:NO_GE
"Index domain out of range of incorrect length and/or type."
E_DM9333_DM2T_EXTRA_IDX:NO_GE
"Extra entry found in index table."
E_DM9334_DM2T_MISSING_IDX:NO_GE
"Missing index entry in index table."
E_DM9335_DM2F_ENDFILE:NO_GE
"End of file was reached during a read or write operation."
E_DM9336_DM2F_BUILD_ERROR:NO_GE
"Error building a File Control Block."
E_DM9337_DM2F_CREATE_ERROR:NO_GE
"Error creating physical file(s) for database table."
E_DM9338_DM2F_INIT_ERROR:NO_GE
"Error initializing newly created physical file(s) for database table."
E_DM9339_DM2F_ALLOC_ERROR:NO_GE
"Error allocating space in physical file(s) for database table."
E_DM933A_DM2F_FORCE_ERROR:NO_GE
"Error forcing physical file for database table."
E_DM933B_DM2F_FLUSH_ERROR:NO_GE
"Error flushing physical file for database table."
E_DM933C_DM2F_READ_ERROR:NO_GE
"Error reading pages for database table."
E_DM933D_DM2F_SENSE_ERROR:NO_GE
"Error sensing physical file for database table."
E_DM933E_DM2F_RENAME_ERROR:NO_GE
"Error renaming physical file."
E_DM933F_DM2F_WRITE_ERROR:NO_GE
"Error writing pages for database table."
E_DM9340_DM2F_RELEASE_ERROR:NO_GE
"Error releasing a File Control Block."
E_DM9341_DM1I_FLUSH:NO_GE
"Error occurred flushing pages to an isam table during a build."
E_DM9342_DM2U_REORG:NO_GE
"Error occurred during MODIFY TO REORGANIZE operation."
E_DM9343_DM0L_ECP:NO_GE
"Error logging end consistency point operation."
E_DM9344_BM_CP_FLUSH_ERROR:NO_GE
"Error flushing dirty pages from Buffer Manager during Consistency Point."
E_DM9345_BM_WBFLUSH_WAIT:NO_GE
"Error in Write Behind Thread waiting for WBFLUSH event."
E_DM9346_DIRTY_PAGE_NOT_FIXED:NO_GE
"Buffer Manager protocol error : page %0d of table (%1c, %2c) in database '%3c'
is not fixed for write but is marked as modified - buffer state 0x%4x."
E_DM9347_CONFIG_FILE_PATCHED:NO_GE
"Config file for data base '%0c'\n
was marked inconsistent by '%1c' (time = %2d).\n
It is now being marked consistent (time = %3d)."
E_DM9348_BM_FLUSH_PAGES_ERROR:NO_GE
"Error flushing dirty pages from Buffer Manager in Write Behind Thread."
E_DM9349_DM1H_NOFULL:NO_GE
"Error updating primary page of hash chain to indicate that it is not full."
E_DM9350_BTREE_INCONSIST:NO_GE
"Inconsistency found in Btree Index"
E_DM9351_BTREE_BAD_TID:NO_GE
"Btree Error on table (%0c, %1c) of database '%2c' :
Leaf page references non existent tuple : Bid (%3d, %4d), Tid (%5d, %6d),
Key is '%7c' (%8c)."
E_DM9352_BAD_DMP_NOT_FOUND:NO_GE
"Error occurred trying to access a dump file."
E_DM9353_BAD_DMP_CLOSE:NO_GE
"Error closing dump file for database %0c
file sequence %1d."
E_DM9354_BAD_DMP_CREATE:NO_GE
"Error creating dump file %1c
at location %0c
for sequence %2d."
E_DM9355_BAD_DMP_OPEN:NO_GE
"Error opening dump file for database %0c
file sequence %1d."
E_DM9356_BAD_DMP_READ:NO_GE
"Error reading dump file for database %0c
file sequence %1d block %2d."
E_DM9357_BAD_DMP_WRITE:NO_GE
"Error writing dump file for database %1c
file sequence %0d block %2d."
E_DM9358_BAD_DMP_UPDATE:NO_GE
"Error updating dump file for database %0c
file sequence %1d."
E_DM9359_BAD_DMP_FORMAT:NO_GE
"Format error in dump file for database %0c
file sequence %1d block %2d."
E_DM935A_BAD_DMP_LENGTH:NO_GE
"Record length error in dump file for database %0c
file sequence %1d block %2d."
E_DM935B_BAD_DMP_DELETE:NO_GE
"Error occurred trying to delete a dump file."
E_DM935C_BAD_DMP_DIRCREATE:NO_GE
"Error occurred trying to create a dump directory '%0c'."
E_DM935E_DM0D_CLOSE_ERROR:NO_GE
"Error occurred closing a dump file."
E_DM935F_DM0D_CREATE_ERROR:NO_GE
"Error occurred creating a dump file."
E_DM9360_DM0D_OPEN_ERROR:NO_GE
"Error occurred opening a dump file."
E_DM9361_DM0D_READ_ERROR:NO_GE
"Error occurred reading a dump file."
E_DM9362_DM0D_WRITE_ERROR:NO_GE
"Error occurred writing a dump file."
E_DM9363_DM0D_UPDATE_ERROR:NO_GE
"Error occurred updating a dump file."
E_DM9364_DM0D_FORMAT_ERROR:NO_GE
"Error occurred in the dump file format."
E_DM9365_DM0D_LENGTH_ERROR:NO_GE
"Error occurred in the length of a dump record."
E_DM936C_DM0L_SBACKUP:NO_GE
"Error writing start backup log record."
E_DM936D_DM0L_EBACKUP:NO_GE
"Error writing end backup log record."
E_DM936E_DM0C_GETENV:NO_GE
"Error trying to get environment variable."
E_DM936F_DM0C_LOINGPATH:NO_GE
"Error trying to build Ingres location path."
E_DM9370_DM0L_SECURE:NO_GE
"Error in logging phase one log record for a transaction."
E_DM9371_RELEASE_TCB_WARNING:NO_GE
"Table Control Block could not be released due to inability to flush
out modified pages from cache."
E_DM9372_BM_CANT_CREATE:NO_GE
"Error creating shared memory segment for Buffer Manager."
E_DM9373_BM_CANT_CONNECT:NO_GE
"Error connecting to Buffer Manager shared memory segment."
E_DM9374_BM_CANT_DESTROY:NO_GE
"Error destroying Buffer Manager shared memory segment in order to create a new
one."
E_DM9375_BM_PARMS_MISMATCH:NO_GE
"Server startup options for Buffer Manager do not match the options specified
when the Shared Buffer Manager was first initialized."
E_DM9376_BM_LOCK_CONNECT:NO_GE
"Error connecting to the Shared Buffer Manager lock list - id: 0x%0x."
E_DM9377_BM_LOCK_RELEASE:NO_GE
"Error releasing the Shared Buffer Manager lock list - id: 0x%0x."
E_DM9378_BM_NO_DBCACHE_LOCKS:NO_GE
"Buffer manager cannot obtain database cache lock - out of locks."
E_DM9379_BM_CANT_OPEN_DB:NO_GE
"Database cannot be open due to inability to obtain a database cache lock."
E_DM937A_BM_NO_TBLCACH_LOCKS:NO_GE
"Buffer Manger cannot get Table Cache Lock - cached information must be tossed."
E_DM937B_BM_CANT_CHANGE_TABLE:NO_GE
"Table cannot be changed due to inability of Buffer Manager to change Table
Cache Lock."
E_DM937C_BM_BUFMGR_LOCK_ERROR:NO_GE
"Error managing Buffer Manager Lock."
E_DM937D_BM_CONNECTIONS:NO_GE
"Shared Buffer Manager '%0c' has wrong connection count: %1d, actual number of
connections is %2d."
/*
** Parameters:
**	Buffer manager name
**	New connect count
**	Old connect count
** Explanation:
**	Buffer manager's internal count of the number of servers connected to
**	it does not match the number of holders of the buffer manager lock.
**	This probably indicates that a server exited abnormally without
**	decrementing the buffer manager connect count.  This message should
**	be followed by E_DMF025_BM_CONNECT_COUNT and then a shutdown of the
**	server.  See notes by that error message for further information.
** System Status:
**	Server is shut down.  All other servers connected to this same buffer
**	manager will also be shut down automatically.
** User Action:
**	Restart servers - examine log to see reason why original server exited
**	abnormally.  This could have happened due to a server fatal error or
**	because the user stopped the dbms server process rather than shutting
**	it down normally.
*/
E_DM937E_TCB_BUSY:NO_GE
"Table Control Block is unexpectedly busy when attempting to deallocate."
E_DM937F_BM_NOT_A_BUFMGR:NO_GE
"Memory segment connected to for cache name '%0c' does not contain a valid
Buffer Cache."
/*
** Parameters:
**	Buffer manager name
** Explanation:
**	Server connected to specified shared memory segment, but the segment
**	did not contain a proper control block type - BMCB_CB.  Could mean
**	that the buffer manager has been trashed.
** System Status:
**	Server will not start up.
** User Action:
**	Probably should shut down all servers connected to this buffer manager.
*/
E_DM9380_TAB_KEY_OVERFLOW:NO_GE
"Error assigning a table_key in table (%0c,  %1c)
of database '%2c'.  The value of the table_key for
this relation has overflowed the 6 bytes available."
E_DM9381_DM2R_PUT_ERROR:NO_GE
"Error putting a record."
E_DM9382_OBJECT_KEY_OVERFLOW:NO_GE
"Error assigning a object_key in table (%0c,  %1c)
of database '%2c'.  The value of the object_key for
this relation has overflowed the 16 bytes available."
E_DM9383_DM2T_UPDATE_LOGICAL_KEY_ERROR:NO_GE
"Error updating iirelation system table on database %0c
while executing some other as insert.
Check consistency of system catalogs."
E_DM9384_DM2T_UPDATE_LOGICAL_KEY_ERROR:NO_GE
"Error updating logical key value in iirelation."
E_DM9385_NO_INDEX_TUPLE:NO_GE
"Cannot locate secondary index row for base table row at tid %0d."
/*
** Parameters:
**	Tid of base table row being updated
** Explanation:
**	An update was made on a table that has a secondary index.  When the
**	system attempted to update the corresponding secondary index row, it
**	was unable to find a row describing the updated tuple of the base
**	table.
** System Status:
**	Update will be rejected and statement aborted.
** User Action:
**	Index table may be no good.
**	Other errors in error log may give indication of reason for failing
**	to find index row.  If system seems to be functioning correctly,
**	but row cannot be updated, recreating the secondary index may be
**	necessary.
*/
E_DM9386_SEC_INDEX_ERROR:NO_GE
"Error updating secondary index '%0c' for change to base table row at tid %1d."
/*
** Parameters:
**	Name of secondary index - Blank if failed during open of 2nd index.
**	Tid of base table row being updated
** Explanation:
**	An update was made on a table that has a secondary index.  A failure
**	occurred attempting to update the corresponding secondary index entry.
** System Status:
**	Update will be rejected and statement aborted.
** User Action:
**	Other errors in error log should give indication of reason for failure.
**	If bringing down server cannot solve problem, recreating the secondary
**	index may be necessary.
*/
E_DM9387_INDEX_UPDATE_ERROR:NO_GE
"Update failed due to inability to update secondary index table."
/*
** Parameters:
**	None.
** Explanation:
**	An update was made on a table that has a secondary index.  A failure
**	occurred attempting to update the corresponding secondary index entry.
** System Status:
**	Update will be rejected and statement aborted.
** User Action:
**	Other errors in error log should give indication of reason for failure.
**	If bringing down server cannot solve problem, recreating the secondary
**	index may be necessary.
*/
E_DM9388_BAD_DATA_LENGTH:NO_GE
"Character data value in row has an illegal length descriptor - length is %0d."
/*
** Parameters:
**	%0d	- length given in length descriptor.
** Explanation:
**	A VARCHAR or TEXT datatype has been encountered that has a length
**	descriptor that gives an impossible length.  This means this row
**	is probably trashed.  This error is probably encountered while trying
**	to compress or uncompress the tuple - in these cases the length value
**	is needed.
**
**	If the error is returned on an insert, then the bad row will not be
**	added to the database.  If this error is returned on a select, then
**	the bad row is probably already in the database.
** System Status:
**	Query will be rejected in normal case and the transaction aborted.
**	These actions can be altered by trace flags described below.
** User Action:
**	This error probably indicates a bad row.  If this error is returned on
**	a select, then the table needs to be fixed.  Following this error should
**	be an E_DM938A_BAD_DATA_ROW error that will give the TID of the bad
**	row.  The user should attempt to select the row by TID to verify that
**	this row is really unable to be returned from the database.
**
**	The table probably needs some type of recovery to fix the bad row.
**	See the User Action described below on error E_DM938A_BAD_DATA_ROW.
*/
E_DM9389_BAD_DATA_VALUE:NO_GE
"Bad data value encountered in row of database."
/*
** Parameters:
**	None.
** Explanation:
**	A bad data value was encountered in the database that could not be
**	interpreted by DMF.  This error should be followed by an E_DM938A
**	error which indicates the TID of the bad tuple.
**
**	If the error is returned on an insert, then the bad row will not be
**	added to the database.  If this error is returned on a select, then
**	the bad row is probably already in the database.
** System Status:
**	Query will be rejected and the transaction aborted.
** User Action:
**	See error E_DM938A_BAD_DATA_ROW.
*/
E_DM938A_BAD_DATA_ROW:NO_GE
"Bad row encountered in database '%0c', table (%1c, %2c) - TID '%3d'."
/*
** Parameters:
**	%0c	- database name
**	%1c	- table name
**	%2c	- table owner
**	%3d	- TID of bad row
** Explanation:
**	A bad row was encountered in the database that could not be
**	interpreted by DMF.
**
**	If the error is returned on an insert, then the bad row will not be
**	added to the database.  If this error is returned on a select, then
**	the bad row is probably already in the database.
** System Status:
**	Query will be rejected in normal case and the transaction aborted.
**	The error recovery actions can be altered by the trace flags below.
** User Action:
**	If this error is returned on a SELECT statement then there is probably
**	a bad row in the database.  The user should do a select by TID of the
**	reported row to make sure that it is really bad.  The table should be
**	recovered - methods for doing this are described below.
**
**	The supported method of fixing a table like this is to recover it from
**	the last checkpoint.  If this is not practical or possible, the table
**	can be patched by removing the offending row.  There are two trace
**	flags provided for this - these are undocumented trace flags that are
**	not intended to be distributed for general use!
**
**	SET TRACE POINT DM801 will cause DMF to skip bad rows like this when
**	scanning the table.  Thus the user could do
**
**		set trace point DM801
**		modify table to storage_structure
**
**	which would have the effect of rebuilding the table without any
**	of the offending rows.
**
**	SET TRACE POINT DM802 will cause DMF to return a garbage row when a
**	bad row is encountered.  This allows the user to do
**
**		set trace point DM802
**		delete table where table.tid = BAD_TID
**		set notrace point DM802
**
**	This will delete the offending row directly.  This is the only operation
**	that should be attempted while having this traceflag on as it may cause
**	unexpected and unpredictable results if used while SELECTing a bad row.
*/
E_DM938B_INCONSISTENT_ROW:NO_GE
"Bad row encountered in database."
/*
** Parameters:
**	None.
** Explanation:
**	A bad row was encountered in the database that could not be
**	interpreted by DMF.
**
**	If the error is returned on an insert, then the bad row will not be
**	added to the database.  If this error is returned on a select, then
**	the bad row is probably already in the database.
** System Status:
**	Query will be rejected and the transaction aborted.
** User Action:
**	See E_DM938A_BAD_DATA_ROW description.
*/
E_DM938C_BM_CLUSTER_CONNECT:NO_GE
"Error connecting to Shared Cache - '%0c'.  This may be caused by trying to
start two DBMS servers in a VAXcluster installation using the same shared cache
name.  Check for other DBMS servers on other nodes using the shared cache name
'%0c'."
/*
** Parameters:
**	%0c	- Name of shared buffer cache.
** Explanation:
**	When running in a VAXcluster environment, a DBMS server thought that
**	the specified shared buffer cache should be available on the system
**	since the lock that identifies that cache was held.  The server,
**	however, was unable to connect to the segment and got a NO SUCH SEGMENT
**	return code. 
**
**	The error is probably caused by the user trying to start two servers
**	on different nodes of the vaxcluster specifying the same shared cache
**	name.
**
** System Status:
**	Server will fail to start up.
** User Action:
**	Check for servers on other nodes using the same cache name.  Use
**	different shared cache for servers on different nodes.
*/
E_DM938D_SHARED_MEM_ALLOCATE:NO_GE
"Cannot allocate sufficient shared memory for Shared Cache.  Cache Size is %0d
Ingres Pages.  Required shared memory is %1d bytes (%2d pages)."
/*
** Parameters:
**	%0d	- Number of Ingres Pages in buffer cache.
**	%1d	- Number of Bytes required for shared cache.
**	%2d	- Number of OS Pages required for shared cache.
** Explanation:
**	Operating system could not give enough shared memory to allocate a
**	cache with the specified size.
**
** System Status:
**	Server will fail to start up.
** User Action:
**	Start server with smaller cache size or make more shared memory
**	available to system.
*/
E_DM938E_BM_MEDESTROY:NO_GE
"Error Destroying Shared Cache memory segment.  Cache segment not destroyed -
it will be destroyed by the next server to connect to this shared cache."
/*
** Parameters:
**	none
** Explanation:
**	Could not destroy the shared memory segment associated with the
**	shared buffer cache.  This occurred during server shutdown when the
**	last server connected to the shared cache was exiting.
**
**	On VMS systems, the shared segment will be automatically deallocated
**	when the server process exits.  On most UNIX systems, where the
**	shared segments must be explicitly deallocated, the shared segment will
**	be left allocated until the next server attempts to use this cache.
**
** System Status:
**	On some systems (UNIX), shared memory segment will be left on system.
** User Action:
**	The next time a server starts up it will destroy the segment.  Otherwise
**	the segment must be destroyed by hand using the CACHEUTIL program.
*/
E_DM938F_DM0L_DMU:NO_GE
"Error logging DM0L_DMU record for DMU operation."
/*
** Parameters:
**	none
** Explanation:
**	Could not write log record for DMU operation.  This record is written
**	for CREATE, DESTROY, MODIFY, INDEX, RELOCATE and ALTER operations.
**
**	This error will occur following a bad Log Alter (LGalter) or Log
**	Write (LGwrite) while logging the DMU operation.
**
**	There should have been a previous log message describing the reason
**	for the failure.
**
** System Status:
**	Transaction will be aborted.  DMU operation will not succeed.
** User Action:
**	Check the error log for the exact reason for the failure.  There should
**	be an E_DM900B_BAD_LOG_WRITE or E_DM900B_BAD_LOG_ALTER error message
**	describing in more detail what operation failed.
*/
E_DM9391_DM0L_FSWAP:NO_GE
"Error logging DM0L_FSWAP record for LOAD or MODIFY operation."
/*
** Parameters:
**	none
** Explanation:
**	Could not write log record for FSWAP operation.  This record is written
**	to log file rename operations during MODIFY or LOAD operation.
**
**	This error will occur following a bad Log Write (LGwrite) while logging
**	the FSWAP operation.
**
**	There should have been a previous log message describing the reason
**	for the failure.
**
** System Status:
**	Transaction will be aborted.  DMU operation will not succeed.
** User Action:
**	Check the error log for the exact reason for the failure.  There should
**	be an E_DM900B_BAD_LOG_WRITE error message describing in more detail
**	what operation failed. 
*/
E_DM9392_DM0D_CONFIG_RESTORE:NO_GE
"Error occurred restoring config file during ROLLFORWARD.  The database config
file could not be copied from the DUMP directory."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    During an online checkpoint, INGRES writes a copy of the config
    **	    file to the database DUMP directory (II_DUMP).  When restoring
    **	    to that checkpoint, the config file is overwritten with the
    **	    contents in the DUMP copy.
    **
    **	    This error is raised when ROLLFORWARD is unable to restore the
    **	    config file using the DUMP copy.  This may occur because of IO
    **	    errors, permission errors, or if the config copy is missing.
    **
    **	    The DUMP copy of the config file will be named CNNNNNN.DMP where
    **	    NNNNNN is the checkpoint sequence number.
    **
    **	    This message should be preceded by more detailed error messages
    **	    giving the exact nature of the error.
    **
    **	System Status:
    **	    Rollforward processing aborts.
    **	User Action:
    **	    Check previous error messages to determine if problem can be
    **	    resolved by fixing system condition (freeing disk space, bringing
    **	    disk volume on-line ...).  If problem seems to be that config file
    **	    copy is corrupted, then the checkpoint may not be usable.  It is
    **	    possible that using a different copy of the config file will
    **	    resolve the problem.
    **	    
*/
E_DM9393_DM0D_NO_DMP_CONFIG:NO_GE
"Error occurred restoring config file during ROLLFORWARD.  There is no copy
of the database config file in the DUMP directory."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    During an online checkpoint, INGRES writes a copy of the config
    **	    file to the database DUMP directory (II_DUMP).  When restoring
    **	    to that checkpoint, the config file is overwritten with the
    **	    contents in the DUMP copy.
    **
    **	    This error is raised when ROLLFORWARD is unable to find the
    **	    config file that corresponds to this checkpoint.
    **
    **	    The DUMP copy of the config file will be named CNNNNNN.DMP where
    **	    NNNNNN is the checkpoint sequence number.
    **
    **	System Status:
    **	    Rollforward processing aborts.
    **	User Action:
    **	    This checkpoint may not be usable.  It is possible that using a
    **	    different copy of the config file will resolve the problem.
    **	    
*/
E_DM9394_DM0D_CONFIG_DELETE:NO_GE
"Error occurred deleting old copies of config file from DUMP directory."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    This error occurs during checkpoint if the '-d' option is specified
    **	    and old DUMP config files cannot be deleted.
    **
    **	    The '-d' option specifies to delete old checkpoint information.
    **	    This includes:  checkpoint files, dump files, and config files.
    **
    **	    This message should be preceded by more detailed error messages
    **	    giving the exact nature of the error.
    **
    **	System Status:
    **	    The checkpoint will fail.
    **	User Action:
    **	    Check previous error messages to see if there is a system problem
    **	    which can be resolved (permissions).  If problem cannot be resolved,
    **	    a workaround is to use checkpoint without the '-d' flag and to
    **	    delete old checkpoints by hand.
    **	    
*/
E_DM9395_SYSTEM_SHUTDOWN_TEST:NO_GE
"The Ingres Installation is being forced to shutdown immediately as part
of a System Crash Test.  This simulates an actual system crash for testing
purposes."
/*%
** Parameters:
**      None.
**
** Explanation:
**	The system was brought down by the system crash test trace flag.
**	This is used during product validation to simulate actual system
**	crashes so that the recovery system can be tested.
** 
** System Status:
**	The Logging System shuts down immediately - possibly leaving
**	active transactions to recover when the system is restarted.
**
** User Action:
**	Restart the installation and allow recovery to clean up any
**	work in progress.
*/
E_DM9396_BT_WRONG_PAGE_FIXED:NO_GE
"A protocol error occurred during update of a Btree index page on table
(%0c, %1c).  The wrong page was passed in by the caller of the btree routine.
Page %2d was being updated but page %3d was held by the caller.  The entry
being updated was %4x (TID %5x).  This error indicates an internal DBMS bug
and should be reported to Ingres Technical Support." 
/*%
** Parameters:
**	%0	The Name of the table being updated.
**	%1	The Owner of the table being updated.
**	%2	The page number which should have been fixed.
**      %3	The page number which was fixed.
**	%4	The Btree Id of the leaf entry being updated.
**	%5	The Tuple Id of the row being updated.
**
** Explanation:
**	This is an internal consistency check in the btree update code.
**	If code in the DBMS passes in the wrong page to the btree update
**	routines, then this error will be generated.  There should be
**	no user error or hardware/environment problem which can cause this
**	error.
** 
** System Status:
**	The query will fail.
**
** User Action:
**	Report the error as an Ingres bug.
**	Check the table for btree consistency problems.
*/
E_DM9397_CP_INCOMPLETE_ERROR:NO_GE
"A Buffer Manager protocol error occurred during processing of a Consistency
Point.  All buffers could not be flushed during the CP.  The error occurred
on buffer # %0d.  Buffer information: Table_id (%1d, %2d), status: %3x,
cp_count %4d (%5d), ref_count %6d, tcb %7x, validation value %8d (%9d)." 
/*%
** Parameters:
**	%0	The buffer which was skipped.
**      %1	The base table id for the buffer.
**	%2	The index table id for the buffer.
**	%3	The buffer status mask.
**	%4	The cp count in the buffer header.
**	%5	The global buffer manager cp count.
**	%6	The buffer's reference count.
**	%7	The server's TCB pointer for the buffer's table.
**	%8	The buffer's tcb validation value.
**	%9	The validation stamp in the TCB.
**
** Explanation:
**	This error is written just prior to the server shutting down due to an
**	error executing a consistency point.  If at the end of processing a
**	consistency point, a server notices that it has not flushed all the
**	necessary pages from the buffer manager, it will signal a protocol
**	error.  This message attempts to output enough useful information to
**	help track down the cause of the error.
** 
** System Status:
**	The server will crash shortly after logging this error.
**
** User Action:
**	Report the error as an Ingres bug.
**	Save the error log.  If there is a dbms trace file then save that as
**	it will contain the dmdcheck which is to follow.  If there is no
**	dbms trace file then define one when the server is restarted so that
**	if the problem is repeated, more complete information can be obtained.
*/
E_DM9400_RCP_LOGSTAT:NO_GE
"Error writing the logging statistics to the output log file."
E_DM9401_RCP_LOGINIT:NO_GE
"Error initialize the logging system."
E_DM9402_RCP_BLDHDR:NO_GE
"Error reconstructing the state of the log file header."
E_DM9403_RCP_GETBLKSIZE:NO_GE
"Error determining the block size of the log file."
E_DM9404_RCP_GETLASTBLK:NO_GE
"Error locating the last block of the log file."
E_DM9405_RCP_GETCP:NO_GE
"Error locating the last consistency point of the log file."
E_DM9406_RCP_GETAP:NO_GE
"Error locating the last archiver point of the log file."
E_DM9407_RCP_RECOVER:NO_GE
"Error recovering transactions from the log file."
E_DM9408_RCP_P0:NO_GE
"Error occurred in phase zero of recovering transactions from the log file."
E_DM9409_RCP_P1:NO_GE
"Error occurred in phase one of recovering transactions from the log file."
E_DM940A_RCP_P2:NO_GE
"Error occurred in phase two of recovering transactions from the log file."
E_DM940C_RCP_ABORT:NO_GE
"Error aborting a transaction."
E_DM940E_RCP_RCPRECOVER:NO_GE
"Error recovering the transactions for the server."
E_DM940F_RCP_CHKEVENT:NO_GE
"Error waiting for the logging system events."
E_DM9410_RCP_SHUTDOWN:NO_GE
"Error shutting down the recovery process."
E_DM9411_DMF_INIT:NO_GE
"Error initializing the DMF working environment."
E_DM9412_DMF_TERM:NO_GE
"Error terminating the DMF working environment."
E_DM9413_RCP_COUNT_OPENS:NO_GE
"Error tracking open/close database operations in the recovery process."
E_DM9414_RCP_REDO_RECOV:NO_GE
"Error in RCP Redo Recovery."
E_DM9415_RCP_GENDB:NO_GE
"Error in RCP generating list of Redo Databases during REDO recovery."
E_DM9416_RCP_REDOINIT:NO_GE
"Error initializing RCP for REDO recovery."
E_DM9417_RCP_REDO_PROC:NO_GE
"Error in RCP processing REDO transactions."
E_DM9418_RCP_RDMUINS:NO_GE
"Error in RCP adding entry to REDO recovery DMU list."
E_DM9419_RCP_RDDBINS:NO_GE
"Error in RCP adding entry to REDO recovery DB list."
E_DM941A_RCP_RBINS:NO_GE
"Error in RCP adding entry to REDO recovery BI list."
E_DM941B_RCP_REDOFINALE:NO_GE
"Error in RCP cleaning up after REDO recovery."
E_DM941C_RCP_GENDB:NO_GE
"Error in RCP getting list of REDO databases"
E_DM941D_RCP_MP0:NO_GE
"Error in RCP processing mini phase-0 of redo recovery"
E_DM941E_RCP_MP1:NO_GE
"Error in RCP processing mini phase-1 of redo recovery"
E_DM941F_PROC_REDO:NO_GE
"Error in RCP processing redo recovery"
E_DM9420_RCP_WILLING_COMMIT:NO_GE
"Error in RCP processing a willing commit transaction."
E_DM9421_RECOVER_WILLING_COMMIT:NO_GE
"Error in RCP recovering a willing commit transaction."
E_DM9422_RCP_COMMIT:NO_GE
"Error in RCP manually committing a transaction."
E_DM9423_RCP_DIS_DUAL_LOGGING:NO_GE
"Error in RCP handling the request of disabling dual logging."
E_DM9424_RCP_HARD_SHUTDOWN:NO_GE
"The Logging System was shutdown while the Recovery Process (RCP) was still
active (probably by the rcpconfig imm_shutdown command).  There may have been
active Ingres processes and/or active transactions which require recovery.  Any
necessary recovery will be done automatically when the Ingres installation is
restarted." 
/*%
** Parameters:
**      None.
**
** Explanation:
**	The RCP was told to shutdown while there were still active Ingres
**	processes.
**
**	This error will be logged when the imm_shutdown option of rcpconfig
**	is issued.  It may also be logged if rcpconfig shutdown is issued
**	and some error occurs during the shutdown process.
**
** System Status:
**	The Ingres Installation will have been shutdown - the system is not
**	in a bad state - but there may be active transactions which require
**	recovery in order to bring the installation to a consistent state.
**	This recovery will be done automatically when the Ingres installation
**	is restarted.
**
** User Action:
**	When the installation is restarted, then all needed recovery will
**	be performed.
*/
E_DM9425_MEMORY_ALLOCATE:NO_GE
"Error allocating memory for DMF operation.  Could not allocate requested
memory.  Memory request was for %0d bytes."
/*%
** Parameters:
**      %0d		- amount of memory requested (in bytes).
**
** Explanation:
**	A memory allocation request failed.
**
**	This error should be followed by further error messages giving
**	the specific operation which required the memory.
**
** System Status:
**	The operation requested could not be performed due to the memory
**	allocation error.
**
** User Action:
**	Check the operation being performed and the requested memory size.
**	If the memory size seems abnormally large, then this error could
**	be the result of an internal bug.  Otherwise this probably indicates
**	that the system has exhausted its memory resources.
*/
E_DM9426_LGADD_SIZE_MISMATCH:NO_GE
"There is a mismatch in sizes of control blocks used to make LGadd calls.
This occurs when the size of the DM0L_ADDDB structure (%0d bytes) is increased
so that it exceeds the LG_DBINFO_MAX constant (%1d).  The DM0L_ADDDB structure
may become larger as a result of changes to the DB_MAXNAME or DB_AREA_MAX 
constants.  The LG_DBINFO_MAX constant should be redefined to reflect the new 
database information size."
/*%
** Parameters:
**	%d		- size of DM0L_ADDDB structure.
**	%d		- definition of LG_DBINFO_MAX.
**
** Explanation:
**	This error probably indicates that a change was made to the 
**	Ingres constants DB_MAXNAME or DB_AREA_MAX and that a similar
**	increase needs to be made to the logging system LG_DBINFO_MAX
**	constant.  This is so the logging system will allocate enough
**	space to store the maximum size path/name for a database.
**
**	This error should never occur on a user system.  It indicates
**	an internal Ingres build error and will be generated when the 
**	system is first started up.
**
** System Status:
**	The installation will not run.  The logging system must be rebuilt
**	with the new LG constant definition.
**
** User Action:
**	None - this error should never occur on a user system.
*/
E_DM9500_DMXE_BEGIN:NO_GE
"Error occurred beginning a transaction."
E_DM9501_DMXE_COMMIT:NO_GE
"Error occurred committing a transaction."
E_DM9502_DMXE_SAVE:NO_GE
"Error occurred attempting to savepoint a transaction."
E_DM9503_DMXE_ABORT:NO_GE
"Error occurred aborting a transaction."
E_DM9504_DMXE_WRITEBT:NO_GE
"Error writing delayed Begin Transaction record."
E_DM9505_DMXE_WILLING_COMMIT:NO_GE
"Error in willing commit a transaction."
E_DM9506_DMXE_DISCONNECT:NO_GE
"Error in recovering a disconnecting transaction."
E_DM9507_DMXE_RESUME:NO_GE
"Error in resuming a willing commit transaction."
E_DM9600_DMVE_BALLOC:NO_GE
"Error recovering BTREE page allocation operation."
E_DM9601_DMVE_BAD_PARAMETER:NO_GE
"Bad parameters to the recovery routine."
E_DM9602_DMVE_BDEALLOC:NO_GE
"Error recovering BTREE page deallocation operation."
E_DM9603_DMVE_BIPAGE:NO_GE
"Error performing before image undo operation."
E_DM9604_DMVE_BPEXTEND:NO_GE
"Error recovering BTREE file extend operation."
E_DM9605_DMVE_CALLOC:NO_GE
"Error recovering common page allocation operation."
E_DM9606_DMVE_CREATE:NO_GE
"Error recovering table CREATE operation."
E_DM9607_DMVE_DEL:NO_GE
"Error recovering DELETE operation."
E_DM9608_DMVE_DESTROY:NO_GE
"Error recovering table DESTROY operation."
E_DM9609_DMVE_FCREATE:NO_GE
"Error recovering file create operation."
E_DM960A_DMVE_FRENAME:NO_GE
"Error recovering file rename operation."
E_DM960B_DMVE_INDEX:NO_GE
"Error recovering table INDEX operation."
E_DM960C_DMVE_MODIFY:NO_GE
"Error recovering table MODIFY operation."
E_DM960D_DMVE_PUT:NO_GE
"Error recovering PUT operation."
E_DM960E_DMVE_RELOC:NO_GE
"Error recovering table RELOCATE operation."
E_DM960F_DMVE_REP:NO_GE
"Error recovering REPLACE operation."
E_DM9610_DMVE_SDEL:NO_GE
"Error recovering system catalog DELETE operation."
E_DM9611_DMVE_SM1RENAME:NO_GE
"Error recovering SYSMOD file rename operation."
E_DM9612_DMVE_SM2CONFIG:NO_GE
"Error recovering SYSMOD configuration file update operation."
E_DM9613_DMVE_SPUT:NO_GE
"Error recovering system catalog PUT operation."
E_DM9614_DMVE_SREP:NO_GE
"Error recovering system catalog REPLACE operation."
E_DM9615_DMVE_LK_FILE_EXTEND:NO_GE
"Error extending the file:%0c
in database:%1c
for lock list:%2x
while recovering a transaction."
E_DM9616_DMVE_UNLK_FILE_EXTEND:NO_GE
"Error extending the file:%0c
in database:%1c
for lock list:%2x
while recovering a transaction."
E_DM9617_DMVE_LOCATION:NO_GE
"Error recovering database ADD LOCATION operation."
E_DM961B_DMVE_ALTER:NO_GE
"Error recovering table ALTER operation."
E_DM961C_DMVE_LOAD:NO_GE
"Error recovering LOAD TABLE operation."
E_DM961D_DMVE_BAD_LOADTAB:NO_GE
"Error recovering LOAD TABLE operation.
Table is unexpected type."
E_DM961E_DMVE_REDEL:NO_GE
"Error processing REDO recovery on Delete operation."
E_DM961F_DMVE_REPUT:NO_GE
"Error processing REDO recovery on Insert operation."
E_DM9620_DMVE_REREP:NO_GE
"Error processing REDO recovery on Replace operation."
E_DM9621_DMVE_BAD_BEFORE_IMAGE:NO_GE
"Error during recovery attempting to apply page Before Image.  Logged BI record
is not consistent."
E_DM9622_DMVE_DMU:NO_GE
"Error recovering DMU operation."
/*
** Parameters:
**	none
** Explanation:
**	An error occurred aborting a DMU operation.
**	The backout algorithm involves logging an ABORT_SAVE record when the
**	DM0L_DMU record is encountered.  An error was encountered while
**	processing the DM0L_DMU operation.
**
**	There should be a previous log record giving further details on what
**	went wrong.
**
**	If this error occurs in the DBMS server, then the server will signal the
**	Recovery System to abort the transaction.  If this error occurs in the
**	Recovery System (RCP), then the database will be marked inconsistent.
**
** System Status:
**	System was not able to recover open transaction in database.  If RCP
**	is unable to recover the transaction, the database will be marked
**	inconsistent.
** User Action:
**	Check the error log for the exact reason for the failure.  There should
**	be a previous log record giving more detail on the error.  Check the
**	II_RCP log for further information on the state of the system when this
**	error occurred.
*/
E_DM9623_DMVE_FSWAP:NO_GE
"Error recovering FSWAP operation."
/*
** Parameters:
**	none
** Explanation:
**	An error occurred aborting an FSWAP operation.
**	The backout algorithm involves renaming the files back to their
**	original versions.
**
**	There should be a previous log record giving further details on what
**	went wrong.
**
**	If this error occurs in the DBMS server, then the server will signal the
**	Recovery System to abort the transaction.  If this error occurs in the
**	Recovery System (RCP), then the database will be marked inconsistent.
**
** System Status:
**	System was not able to recover open transaction in database.  If RCP
**	is unable to recover the transaction, the database will be marked
**	inconsistent.
** User Action:
**	Check the error log for the exact reason for the failure.  There should
**	be a previous log record giving more detail on the error.  Check the
**	II_RCP log for further information on the state of the system when this
**	error occurred.
*/
E_DM9624_DMVE_SBIPAGE:NO_GE
"Error recovering System Catalog Before Image."
/*
** Parameters:
**	none
** Explanation:
**	An error occurred processing System Catalog Before Image.
**	The backout algorithm involves replacing the existing page with the
**	Before Image copy.
**
**	There should be a previous log record giving further details on what
**	went wrong.
**
**	System catalog before images are only used during online checkpoint
**	processing;  thus this message should occur only when processing the
**	ONLINE DUMP information while restoring to a checkpoint.
**
** System Status:
**	The ROLLFORWARD will abort.
** User Action:
**	Check the error log for the exact reason for the failure.  There should
**	be a previous log record giving more detail on the error.  If there
**	is a correctable system status, the rollforward can be retried.  If the
**	problem is a corruption in the checkpoint information, then this
**	checkpoint cannot be applied.
*/
E_DM9700_SR_OPEN_ERROR:NO_GE
"Error occurred opening sort work file."
E_DM9701_SR_CLOSE_ERROR:NO_GE
"Error occurred deleting a sort work file."
E_DM9702_SR_READ_ERROR:NO_GE
"Error occurred reading a sort work file."
E_DM9703_SR_WRITE_ERROR:NO_GE
"Error occurred writing a sort work file.\n
Check disk space & quota on the sorting device(s)."
E_DM9704_SORT_ALLMEM_ERROR:NO_GE
"Error occurred allocating memory for a sort."
E_DM9706_SORT_PROTOCOL_ERROR:NO_GE
"Sort routine called out of phase."
E_DM9707_SORT_BEGIN:NO_GE
"Error occurred beginning a sort."
E_DM9708_SORT_END:NO_GE
"Error occurred ending a sort."
E_DM9709_SORT_INPUT:NO_GE
"Error occurred switching from write to read phase."
E_DM970A_SORT_GET:NO_GE
"Error occurred getting next sorted record."
E_DM970B_SORT_PUT:NO_GE
"Error occurred adding a record to be sorted."
E_DM970C_SORT_DOMERGE:NO_GE
"Error occurred merging sorted runs."
E_DM970D_SORT_WRITE_RECORD:NO_GE
"Error occurred writing a record to the sort work file."
E_DM970E_SORT_READ_RECORD:NO_GE
"Error occurred reading a record from the sort work file."
E_DM970F_SORT_OUTPUT_RUN:NO_GE
"Error occurred preparing work file for a new run."
E_DM9710_SORT_INPUT_RUN:NO_GE
"Error occurred preparing to read a run from the work file."
E_DM9711_SR_OPEN_ERROR:NO_GE
"Error occurred opening sort work file--path = %0c, file = %1c."
/*%
** Parameters:
**	0 -- path of sort file
**	1 -- name of sort file
** Explanation:
**	An error occurred opening the sort work file.  If the problem
**	is a lack of disk space another message will be logged.
**
** System Status:
**	The sort will abort.  This will cause the statement which invoked
**	the sort to fail as well (whether it be a MODIFY, INDEX, SELECT, etc).
**
** User Action:
**	Check the error log for the exact reason for the failure.  There should
**	be a previous log record giving more detail on the error.  If there
**	is a correctable system status, the statement causing the sort can be
**	retried.  For example if the problem is a lack of disk space, the
**	table can be relocated and the statement retried.
*/
E_DM9800_ARCH_TERMINATE:NO_GE
"Archiver is Terminating."
E_DM9801_ARCH_PHASE_ZERO:NO_GE
"Error occurred during phase zero of the archiver."
E_DM9802_ARCH_PHASE_ONE:NO_GE
"Error occurred during phase one of the archiver."
E_DM9803_ARCH_PHASE_TWO:NO_GE
"Error occurred during phase two of the archiver."
E_DM9804_ARCH_PHASE_THREE:NO_GE
"Error occurred during phase three of the archiver."
E_DM9805_ARCH_CP_FORMAT:NO_GE
"Archiver read a consistency point log record which had a bad format."
E_DM9806_ARCH_BAD_DBID:NO_GE
"Archiver was given a bad LG database id from an LGshow call."
E_DM9807_ARCH_REUSED_DBID:NO_GE
"Archiver was given a LG database id from an LGshow call which it 
had previously returned."
E_DM9809_ARCH_LOG_SHOW:NO_GE
"The Archiver got an error while trying to do an LGshow call."
E_DM980A_ARCH_LOG_READ:NO_GE
"The Archiver got an error reading the log file."
E_DM980B_ARCH_LOG_FORCE:NO_GE
"The Archiver got an error trying to force the log file."
E_DM980C_ARCH_LOG_EVENT:NO_GE
"The Archiver got an error while trying to request a LG event."
E_DM980D_ARCH_LOG_ALTER:NO_GE
"The Archiver got an error while trying to execute an LGalter."
E_DM980E_ARCH_LOG_ADD:NO_GE
"The Archiver got an error trying to do an LGadd call."
E_DM980F_ARCH_LOG_REMOVE:NO_GE
"The Archiver got an error trying to an LGremove call."
E_DM9810_ARCH_LOG_BEGIN:NO_GE
"The Archiver got an error trying to do an LGbegin call."
E_DM9811_ARCH_LOG_END:NO_GE
"The Archiver got an error trying to do an LGend call."
E_DM9812_ARCH_OPEN_LOG:NO_GE
"The Archiver got an error trying to open the log file."
E_DM9813_ARCH_CLOSE_LOG:NO_GE
"The Archiver got an error trying to close the log file."
E_DM9814_ARCH_LOCK_CREATE:NO_GE
"The Archiver got an error trying to create a lock list."
E_DM9815_ARCH_SHUTDOWN:NO_GE
"The Archiver was told to shutdown."
E_DM9818_ARCH_ADD_DBID:NO_GE
"The Archiver received a bad or duplicate database id from LG."
E_DM9819_ARCH_ADD_JOURNAL:NO_GE
"The Archiver got an error constructing the list of databases to 
be archived."
E_DM981C_ARCH_PURGE_JOURNAL:NO_GE
"The Archiver got an error constructing the list of databases to 
be purged from the archiver."
E_DM981D_ARCH_PURGE_DBID:NO_GE
"The Archiver received a bad or duplicate database id from LG."
E_DM981E_ARCH_READ_CP:NO_GE
"The Archiver got an error trying to read a consistency point."
E_DM981F_ARCH_CK_SBACKUP:NO_GE
"Error checking start online backup."
E_DM9820_ARCH_UPDATE_CTX:NO_GE
"Error updating online backup context from LG."
E_DM9821_ARCH_BACKUP_DBID:NO_GE
"Invalid dbid."
E_DM9823_ARCH_ONLINE_BACKUP:NO_GE
"Error occurred during online backup."
E_DM9824_ARCH_PHASE_B2:NO_GE
"Error occurred during backup phase two."
E_DM9825_ARCH_INIT_DUMP:NO_GE
"Error occurred during init dump context."
E_DM9826_ARCH_COPY_DUMP:NO_GE
"Error occurred during prepare copy dump."
E_DM9827_ARCH_LOG2DUMP:NO_GE
"Error occurred during copy phase of dump file."
E_DM9828_ARCH_PHASE_B3:NO_GE
"Error occurred during backup phase three."
E_DM9829_ARCH_BAD_LOG_TYPE:NO_GE
"Invalid log type."
E_DM982A_ARCH_SBACKUP_UNEXPTD:NO_GE
"Error occurred during Online Checkpoint.  An unexpected Start Backup log record
was encountered by the Archiver during backup processing."
/*%
    **	Parameters:
    **	    none.
    **	Explanation:
    **	    During Online Backup processing in the Archiver, an unexpected
    **	    Start Backup record was found in the log file.  This indicates
    **	    a protocol error in the checkpoint.
    **
    **	    This error is signaled when a consistency check fails in the
    **	    archiver.  There is no known condition that might cause this
    **	    type of failure.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    Restart the checkpoint.
    **	    
*/
E_DM9830_ARCH_DWRITE:NO_GE
"Error occurred writing dump file."
E_DM9831_ARCH_JWRITE:NO_GE
"Error occurred writing journal file."
E_DM9832_ARCH_OBCLEAN:NO_GE
"Error finalize acp backup context."
E_DM9833_ARCH_BACKUP_FAILURE:NO_GE
"Error occurred during online ckpdb of database %0c.  The INGRES archiver 
failed for some reason
causing the online ckpdb to fail (usually out of disk space on the II_DUMP
disk).  Check the II_ACP.LOG file in the INGRES files
directory for information concerning the archiver failure."
/*%
    **	Parameters:
    **      %0c		database name.
    **	Explanation:
    **	    The INGRES online checkpoint facility failed when the INGRES
    **	    archiver (dmfacp) encountered an error.  The online checkpoint 
    **	    facility depends
    ** 	    on the archiver to successfully function, in order to guarantee a
    **	    consistent checkpoint of the database.  This error occurred
    **      sometime during the archiver processing during the online 
    **      checkpoint.
    **
    **	    Consult both the "errlog.log" and the "II_ACP.LOG" file for more
    **	    information concerning the cause of the failures.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.
    **	User Action:
    **	    After addressing the error conditions detailed in the error log
    **      files restart the online checkpoint.
    **	    
*/
E_DM9834_ARCH_OLPURGE_ERROR:NO_GE
"Error occurred during an Online Purge operation in the Archiver.  The Archiver
found that the database was not in a state that allows an Online Purge to
be signaled.  This event is signaled as a part of Online Backup processing."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    The Archiver was signaled to execute an Online Purge event -
    **	    probably as part of an Online Checkpoint.  The archiver found
    **	    that there were open transactions that spanned past the
    **	    boundaries of the Online Purge window.
    **
    **	    The Online Purge protocols dictate that there be no active
    **	    transactions when the purge is signaled and that no transactions
    **	    span past the boundaries of the purge window.  That is that any
    **	    transactions which have a BT record inside the purge window
    **	    must have that transaction resolved within the purge window.
    **
    **	    This condition will cause the archiver to fail.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.  The Archiver is brought down.
    **	User Action:
    **	    Restart the Archiver and restart the Checkpoint.
    **	    
*/
E_DM9835_ARCH_NEW_JNL_ENTRY:NO_GE
"Archiver encountered an error while creating a new Journal History entry in
the Config File as part of an Online Checkpoint."
/*%
    **	Parameters:
    **	    None.
    **	Explanation:
    **	    When an Online Checkpoint is signaled on a journaled database,
    **	    the journal history is not updated to indicate a new journal
    **	    file until the SBACKUP record is encountered in the archiver.
    **
    **	    At this time, the archiver creates the new journal history entry
    **	    and creates a new journal file.
    **
    **	    This error occurs when an error is encountered creating a new
    **	    journal history entry.
    **
    **	    There should be previous error messages in the ERRLOG which
    **	    give a better indication of the exact error.
    **
    **	System Status:
    **	    On line checkpoint processing aborts.  Archiver will probably fail.
    **	User Action:
    **	    After addressing the error conditions detailed in the error log
    **      files restart the archiver and retry the online checkpoint.
    **	    
*/
E_DM9836_ARCH_MISMATCHED_ET:NO_GE
"The transaction %0x.%1x was mismatched. The log file cannot be processed.
An Archiver mini-dump will be produced. Please contact Ingres Technical Support
to report this problem so that it can be diagnosed and repaired."
/*%
** Parameters:
**	%0x, %1x    - Log file internal transaction ID, in hex
**
** Description:
**	This message is produced by the Archiver when a very specific internal
**	error has occurred: During its scanning of the log file's Consistency
**	Point window, the Archiver found an End Transaction record for a
**	transaction which had no corresponding Begin Transaction record.
**
**	This is a SERIOUS internal error in the Ingres log file, and means that
**	the log file information for the indicated transaction will be lost.
**
**	The Archiver must be restarted in a special fashion in order to
**	proceed. Instructions for this are given in the mini-dump which the
**	Archiver produced.
**
** System Status:
**	An Archiver mini-dump has been produced and is contained in the Archiver
**	log file "II_DBMS_CONFIG:II_ACP.LOG". This mini-dump contains problem
**	determination information which is crucial to the diagnosis of this
**	problem.
**
**	After producing the mini-dump the Archiver will terminate. In order to
**	restart the Archiver properly, follow the instructions at the end of
**	the Archiver mini-dump.
**
** User Action:
**	Please follow the instructions at the end of the Archiver mini-dump in
**	order to restart the archiver. Please report this problem to Ingres
**	Technical Support and forward the Archiver mini-dump to Ingres
**	Technical Support for their use.
**
**	Once the Archiver has been successfully restarted, please take an
**	up-to-date checkpoint of every database which was in use at the time of
**	the problem. This ensures that the databases will not need to depend on
**	the possibly damaged journal files should rollforward be necessary in
**	the future.
*/

E_DM9837_ARCH_ET_CONTINUE:NO_GE
"By providing the value %0c for II_ACP_CONTINUE_ON_ERROR, the Archiver has
been signalled to continue processing even though a mismatched transaction was
found in the log file. The transaction ID was %1x.%2x.
The Archiver will continue processing, but the transaction was not copied to
the journal files for database %3c, owned by %4c. Please contact the owner of
the database immediately so that a new checkpoint can be taken on this database.
Since the journal files for this database are incomplete, a new checkpoint is
needed."
/*%
** Parameters:
**	%0c	    - the value of II_ACP_CONTINUE_ON_ERROR
**	%1x,%2x	    - the transaction ID which was not journalled
**	%3c	    - the database which contained this transaction
**	%4c	    - the owner of the indicated database
**
** Description:
**	This message is produced when the Archiver acknowledges a request to
**	continue processing after an internal log file error has occurred. The
**	Archiver will resume its activity of copying committed Ingres
**	transaction information to the Ingres log file, but the indicated
**	transaction's information was NOT copied to the indicated database's
**	journal files, and so the journal files are incomplete.
**
** System Status:
**	Archiver processing continues, but the transaction indicated in the
**	message was not archived, and the journals for the indicated database
**	are incomplete.
**
** User Action:
**	If you have not already done so, please report this problem to Ingres
**	Technical Support, as it indicates a severe internal error in the
**	Ingres transaction log which needs to be diagnosed and repaired.
**
**	Once the archiver finishes this processing, please take a checkpoint
**	of the indicated database. You may wish to take a checkpoint of ALL
**	your Ingres databases which were in action at the time of this error,
**	but only the database indicated in this message needs an immediate
**	checkpoint; all other databases are unaffected by this problem.
*/

E_DM9900_CSP_INITIALIZE:NO_GE
"The CSP process couldn't startup due to initialization errors."
E_DM9901_CSP_ACTION_FAILURE:NO_GE
"Unexpected failure processing node action event."
E_DM9902_CSP_CONFIG_ERROR:NO_GE
"Unexpected error opening/reading II_CONFIG:CLUSTER.CNF."
E_DM9903_CSP_CONFIG_FORMAT:NO_GE
"Unexpected format for file II_CONFIG:CLUSTER.CNF."
E_DM9904_CSP_DECNET:NO_GE
"Error creating DECnet object.  Program requires an operational DECnet and SYSNAM privilege."
E_DM9905_CSP_NOT_NODE:NO_GE
"This node is not listed in the configuration file."
E_DM9906_CSP_RCPCONFIG:NO_GE
"Error opening/reading RCP configuration file for this node."
E_DM9907_CSP_CREATE_RCP:NO_GE
"Error creating process to execute as the RCP."
E_DM9908_CSP_CREATE_ACP:NO_GE
"Error creating process to execute as the RCP."
E_DM9909_CSP_TIMER:NO_GE
"Error queuing timer request."
E_DM990A_CSP_STALL_CLEAR:NO_GE
"Error occurred when attempting to continue stalled lock requests.  Wrong driver could be loaded."
E_DM990B_CSP_DEADLOCK_CLEAR:NO_GE
"Error occurred when attempting to continue distributed deadlock searches.  Wrong driver could be loaded."
E_DM990C_CSP_MASTER_LOCK:NO_GE
"Unexpected error occurred attempting to request MASTER lock."
E_DM990D_CSP_NO_MSGS:NO_GE
"No free messages available."
E_DM990E_CSP_READ_ACTION:NO_GE
"Unexpected action associated with message being read."
E_DM990F_CSP_NETSHUT:NO_GE
"DECnet is shutting down.  The CSP will now shutdown."
E_DM9910_CSP_NETMSG:NO_GE
"Unrecognized network message received."
E_DM9911_CSP_CANIO:NO_GE
"Expected cancel I/O request found."
E_DM9912_CSP_READ_QUEUE:NO_GE
"Error queuing read request."
E_DM9913_CSP_READ_COMPLETE:NO_GE
"Read request completed with unexpected error."
E_DM9914_CSP_WRITE_COMPLETE:NO_GE
"Write request completed with unexpected error."
E_DM9915_CSP_CONNECT_FAIL:NO_GE
"Unexpected connect failure."
E_DM9916_CSP_ACCEPT_FAILURE:NO_GE
"Unexpected accept failure."
E_DM9917_CSP_REJECT_FAILURE:NO_GE
"Unexpected reject failure."
E_DM9918_CSP_WRITE_ERROR:NO_GE
"Unexpected error queuing write request."
E_DM9919_CSP_WRITE_ACTION:NO_GE
"Unexpected action associated with message being written."
E_DM991A_CSP_NODE_LOCK:NO_GE
"Unexpected error attempting to obtain NODE lock."
E_DM991B_CSP_FAILOVER:NO_GE
"Unexpected error attempting to obtain FAILOVER lock."
E_DM991C_CSP_MASTER_SIX:NO_GE
"Unexpected error converting MASTER lock to SIX."
E_DM991D_CSP_NO_NODE_LOCK:NO_GE
"Unexpected error attempting to block on node lock for another node."
E_DM991E_CSP_BAD_BROADCAST:NO_GE
"Unexpected broadcast attempted."
E_DM991F_CSP_CSID:NO_GE
"Unexpected error asking for cluster system node identifier."
E_DM9920_CSP_GETLKI:NO_GE
"Unexpected error asking for lock information from VMS."
E_DM9921_CSP_REFLECT_CONVERT:NO_GE
"Unexpected error reflecting convert-convert deadlock."
E_DM9922_CSP_BAD_DEADLOCK:NO_GE
"Unexpected status from LKdeadlock call."
E_DM9923_CSP_ACTION_ORDER:NO_GE
"Node action was executed out of order."
E_DM9925_CSP_MASTER_PATHLOST:NO_GE
"DECnet path to master node lost. CSP will shutdown."
E_DM9926_CSP_CONNECT_MASTER:NO_GE
"Cannot connect to the master node. CSP will shutdown."
E_DM9A00_GATEWAY_RECOVER:NO_GE
"Warning - log record for gateway table encountered during transaction
recovery.  Gateway tables are not recovered as part of server recovery.  This
log record will be ignored. (Log record type = %0d)"
/*
** Parameters:
**	%0d	- log record type.
**
** Explanation:
**	While recovering a user transaction, a log record was encountered
**	for a table that is marked as a gateway relation (DMT_GATEWAY bit
**	set in iirelation.relstat).
**
**	No log records should be written for gateway operations.  This
**	error is written as an informational message, the query continues
**	to be backed out normally.
**
** System Status:
**	No change in status.
**
** User Action:
**	This error should be reported to Ingres Corporation.
**	The user's database should be recovered properly.
*/
E_DM9A01_GATEWAY_INIT:NO_GE
"Error establishing connection to Gateway during server startup."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered at server startup time attempting to 
**	initialize the gateway system.  There should be a previous error
**	in the error log describing the exact nature of the error.
**
** System Status:
**	Server will not start up.
**
** User Action:
**	Check installation and gateway configuration.
*/
E_DM9A02_GATEWAY_TERM:NO_GE
"Error closing connection to Gateway during server shutdown."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered at server shutdown time while
**	disconnecting from the gateway.  The server will continue
**	shutting down normally.
**
** System Status:
**	Server should continue shutting down normally.  If server was
**	shutdown with gateway transactions in progress, the state of
**	the gateway database may not be consistent.  If the foreign
**	database provides its own recovery, then it should have handled
**	any actions necessary when the connection was broken.
**
** User Action:
**	Check the error log for any previous messages which may clarify
**	the exact nature of the problem.  If gateway transactions were
**	in progress, the user may want to check the foreign database for
**	consistency.
*/
E_DM9A03_GATEWAY_SESS_INIT:NO_GE
"Error establishing connection to Gateway during session startup."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was returned attempting to create a session connection to
**	the gateway system.  There should be a previous message giving more
**	detail about the failure.
**
** System Status:
**	The session will be terminated.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration.
*/
E_DM9A04_GATEWAY_SESS_TERM:NO_GE
"Error closing connection to Gateway during session shutdown."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered terminating a session with the gateway system.
**	There should be a previous message in the error log giving more detail
**	about the failure.
**
** System Status:
**	The session should continue shutting down normally.  If the session
**	was shutdown with a gateway transaction in progress, the state of
**	the gateway database may not be consistent.  If the foreign
**	database provides its own recovery, then it should have handled
**	any actions necessary when the connection was broken.
**
** User Action:
**	Check the error log for any previous messages which may clarify
**	the exact nature of the problem.  If gateway transactions were
**	in progress, the user may want to check the foreign database for
**	consistency.
*/
E_DM9A05_GATEWAY_REGISTER:NO_GE
"Error registering Gateway table with the Ingres Database."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered attempting to register a gateway table with
**	the Ingres database.  There should be a previous message in the error
**	log giving more detail on the error.
**
** System Status:
**	The table registration is backed out.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration.
*/
E_DM9A06_GATEWAY_REMOVE:NO_GE
"Error removing registration of gateway table with the Ingres Database."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered attempting to remove a gateway table from
**	the Ingres database.  There should be a previous message in the error
**	log giving more detail on the error.
**
** System Status:
**	The request is rejected and the remove operation is backed out.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration.
*/
E_DM9A07_GATEWAY_OPEN:NO_GE
"Error attempting to open a table in the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while trying to open a table that resides
**	in a foreign gateway.  The error could be caused by an error in
**	the foreign database.
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A08_GATEWAY_CLOSE:NO_GE
"Error closing a table in the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while trying to close a table that resides
**	in a foreign gateway.  The error could be caused by an error in
**	the foreign database.
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A09_GATEWAY_POSITION:NO_GE
"Error positioning a table in the Ingres Gateway for record access."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while trying to position a table that resides
**	in a foreign gateway.  The error could be caused by an error in
**	the foreign database.
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A0A_GATEWAY_GET:NO_GE
"Error getting record from a table in the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while trying to read a table that resides
**	in a foreign gateway.  The error could be caused by an error in
**	the foreign database.
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A0B_GATEWAY_PUT:NO_GE
"Error writing a record to a table in the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while attempting to write to a table that
**	resides in a foreign gateway.  The error could be caused by an error in 
**	the foreign database. 
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A0C_GATEWAY_REPLACE:NO_GE
"Error updating a record in a table in the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while attempting to update a record of a 
**	table that resides in a foreign gateway.  The error could be caused
**	by an error in the foreign database.
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A0D_GATEWAY_DELETE:NO_GE
"Error deleting a record in a table in the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while attempting to delete a record of a 
**	table that resides in a foreign gateway.  The error could be caused
**	by an error in the foreign database.
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A0E_GATEWAY_BEGIN:NO_GE
"Error registering a Begin Transaction operation with the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while informing the gateway of a
**	Begin Transaction operation.  This error could be caused by
**	an error in the foreign database.
**
** System Status:
**	The current query is rejected.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A0F_GATEWAY_COMMIT:NO_GE
"Error registering an Commit Transaction operation with the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while informing the gateway of a Commit
**	Transaction operation.  This error could be caused by an error in
**	the foreign database.
**
** System Status:
**	The transaction is aborted in the local database.  If the foreign
**	database handles transaction recovery, then it should recover the
**	foreign database properly.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9A10_GATEWAY_ABORT:NO_GE
"Error registering an Abort Transaction operation with the Ingres Gateway."
/*
** Parameters:
**	None
**
** Explanation:
**	An error was encountered while informing the gateway of an
**	Abort Transaction operation.  This error could be caused by
**	an error in the foreign database.  The Ingres transaction will
**	continue to be aborted.  If the foreign database supports
**	transaction recovery, then it should recover the foreign database
**	properly.  The user should check the state of the foreign
**	database to ensure it is consistent.
**
** System Status:
**	The transaction is aborted in the local database.  If the foreign
**	database handles transaction recovery, then it should recover the
**	foreign database properly.
**
** User Action:
**	Check the error log for previous errors.  Check the server gateway
**	configuration and the state of the foreign database.
*/
E_DM9B00_DELETE_DMPE_ERROR:NO_GE
"An error was encountered during tuple delete processing in deleting the
associated peripheral records."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was deleting a tuple which contained some peripheral
**	datatype(s).  In attempting to delete the associated peripheral data, an
**	error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B01_GET_DMPE_ERROR:NO_GE
"An error was encountered during tuple get processing in getting the
associated peripheral records."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was fetching a tuple which contained some peripheral
**	datatype(s).  In attempting to validate the associated peripheral data, an
**	error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B02_PUT_DMPE_ERROR:NO_GE
"An error was encountered during low-level put tuple processing with
associated peripheral records."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was putting a tuple which contained some peripheral
**	datatype(s).  In attempting to put the associated peripheral data, an
**	error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B03_REPLACE_DMPE_ERROR:NO_GE
"An error was encountered during low-level replace tuple processing with
associated peripheral records."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was replacing a tuple which contained some peripheral
**	datatype(s).  In attempting to replace the associated peripheral data, an
**	error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B04_LOAD_DMPE_ERROR:NO_GE
"An error was encountered during low-level load tuple processing with
associated peripheral records."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was loading a tuple which contained some peripheral
**	datatype(s).  In attempting to load the associated peripheral data, an
**	error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B05_DMPE_PUT_CLOSE_TABLE:NO_GE
"An error was encountered closing a table during an ADP_PUT operation on
peripheral datatypes."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was putting a tuple which contained some peripheral
**	datatype(s).  In attempting to put the associated peripheral data, an
**	error was encountered in closing a table.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for those errors.
*/
E_DM9B06_DMPE_BAD_CAT_SCAN:NO_GE
"An error was encountered scanning the extended relation catalog during DMPE
processing."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was putting a tuple which contained some peripheral
**	datatype(s).  In attempting to scan the extended relation catalog,
**	an error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B07_DMPE_ADD_EXTENSION:NO_GE
"An error was encountered during the addition of a table extension (DMPE
processing)."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was putting a tuple which contained some peripheral
**	datatype(s).  In attempting to put the associated peripheral data, an
**	error was encountered while adding an new table extension.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B08_DMPE_NEED_TRAN:NO_GE
"An attempt was made to define a temporary DMPE object while no transaction was
in progress."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was attempting to save a temporary peripheral object with no
**	existing transaction.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**	Please record any queries in progress, datatypes and/or applications in
**	use, and any other environmental information.  This is an internal
**	error.  Please contact the product vendor.
*/
E_DM9B09_DMPE_TEMP:NO_GE
"An error was encountered creating a temporary peripheral object."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was running a query involving peripheral
**	datatype(s).  In attempting to create a temporary peripheral object, an
**	error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error for the more detailed reason behind the error.  Follow
**	the recommendations for that error.
*/
E_DM9B0A_DMPE_CREATE_CATALOG:NO_GE
"An error was encountered creating the iiextended_relation catalog."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system was running a query involving peripheral
**	datatype(s).  In attempting to create the `iiextended_relation' system
**	catalog, an error was encountered.
**
**	The system will remain operational.  The query which encounted this
**	error will be aborted.
**
** User Action:
**      Examine the error log for the more detailed reason behind the error.
**	(Other, more detailed messages are logged before this message.)
**	Follow the recommendations for those errors.
*/
E_DMF000_MUTEX_INIT:NO_GE
"Error initializing a mutex."
E_DMF001_MUTEX_LOCK:NO_GE
"Error locking a mutex."
E_DMF002_MUTEX_FREE:NO_GE
"Error releasing a mutex."
E_DMF003_MUTEX_UNLOCK:NO_GE
"Error unlocking a mutex."
E_DMF004_EVENT_DECLARE:NO_GE
"Error declaring an event. This is a result of a known bug that
occurs when trying to end a transaction or close a database and 
no more locks are available.  To avoid this problem until it 
has been fixed, configure your system with more locks."
E_DMF005_EVENT_PREPARE:NO_GE
"Error preparing an event. This is a result of a known bug that
occurs when trying to end a transaction or close a database and 
no more locks are available.  To avoid this problem until it 
has been fixed, configure your system with more locks."
E_DMF006_EVENT_RELEASE:NO_GE
"Error releasing an event."
E_DMF007_WAIT:NO_GE
"Error waiting for an event. This is a result of a known bug that
occurs when trying to end a transaction or close a database and 
no more locks are available.  To avoid this problem until it 
has been fixed, configure your system with more locks."
E_DMF008_DM0M_BAD_OBJECT:NO_GE
"Memory manager given bad object to deallocate."
E_DMF009_DM0M_FREE_OVERLAP_PREV:NO_GE
"Memory manager given object to free that overlaps the previous free object.%!
Possible double deallocation."
E_DMF00A_DM0M_FREE_OVERLAP_PREV:NO_GE
"Memory manager given object to free that overlaps the next free object.%!
Possible double deallocation."
E_DMF00B_DM0M_PROTECT:NO_GE
"Memory manager couldn't change memory protection."
E_DMF00C_DM0M_FREE_OVERLAP_PREV:NO_GE
"Memory manager data structures corrupted."
E_DMF010_DM1R_NO_PAGE:NO_GE
"Expected page not given."
E_DMF011_DM1R_BAD_TID:NO_GE
"Given tid is not valid."
E_DMF012_ADT_FAIL:NO_GE
"Unexpected ADT routine failure."
E_DMF013_DM1B_CURRENT_INDEX:NO_GE
"Current btree index page is inconsistently marked."
E_DMF014_DM1B_PARENT_INDEX:NO_GE
"Parent btree index page is inconsistently marked."
E_DMF015_EVENT_SET:NO_GE
"Unexpected error set event wait reason."
E_DMF016_EVENT_LWAIT:NO_GE
"Unexpected error waiting for event."
E_DMF020_DM0P_VALIDATE:NO_GE
"Unexpected error getting lock value for cached page."
E_DMF022_DM0P_CACHE_IX_IS:NO_GE
"Unexpected error converting page cached in IX to IS."
E_DMF023_BM_CP_FLUSH_SEVERE:NO_GE
"Severe Error flushing pages from Buffer Manager during Consistency Point."
E_DMF024_INCOMPLETE_CP:NO_GE
"Fatal Error: All modified pages could not be flushed out of Buffer Manager
during Consistency Point."
E_DMF025_BM_CONNECT_COUNT:NO_GE
"Server has exited abnormally while connected to a Shared Cache causing an
inconsistency of the cache information.  Servers connected to this cache will
be shut down."
/*
** Parameters:
**	None
** Explanation:
**	The Buffer Manager keeps an internal count of the number of servers
**	which are connected to it.  When servers start up or shut down, they
**	are expected to increment/decrement this count accordingly.
**
**	Each server connected to a buffer manager holds a shared buffer manager
**	lock on its server lock list.  At strategic times (server startup and
**	at consistency points) servers will check the number of holders
**	of the buffer manager lock and compare it with the connect count.
**	If the counts do not match then some server has exited without adjusting
**	the connect count.
**
**	This is currently considered a fatal error as the buffer manager is
**	left in an unpredictable state - lists may be trashed and data may
**	be corrupt.
**
**	When this message is displayed, it means that DMF is shutting down the
**	server.
**
** System Status:
**	Server is shut down.  All other servers connected to this same buffer
**	manager will also be shut down automatically.
** User Action:
**	Restart servers - examine log to see reason why original server exited
**	abnormally.  This could have happened due to a server fatal error or
**	because the user stopped the dbms server process rather than shutting
**	it down normally.
*/


/*
**    Message file for Facility code: _GC
*/

/*  History:
**      12-May-89 (pasker) 1.001
**          added messages to support discrimination of PL input event
**	    errors
**	16-Jul-89 (seiwald)
**	    Added GCN errlog.log messages
**	16-Jul-89 (jorge)
**	    reapplied Generic error support
**      01-Aug-89 (cmorris)
**          added error for GCA request response failure
**	25-Sep-89 (seiwald)
**	    New GCFFFE_INCOMPLETE status to indicate service must
**	    be reinvoked with GCA_RESUME indicator.
**	02-oct-89 (pasker) 1.002
**	    added error for when GCusrpwd fails.
**      08-Nov-89 (cmorris)
**          added error for disassociate failure.
**	24-Nov-89 (seiwald)
**	    Errors for GCN_RESOLVE operation - all parameterless.
**	26-Nov-89 (seiwald)
**	    New E_GC0104_GCN_USERS_FILE.
**	31-Dec-89 (seiwald)
**	    New E_GC0032_NO_PEER.
**	05-Feb-90 (seiwald)
**	    Made GC0101 like GC0125, and added advice about checking II_CONFIG.
**	    New E_GC2219_GCA_REGISTER_FAIL.
**	06-Feb-90 (seiwald)
**	    New E_GC2815_NTWK_OPEN.
**	23-Mar-90 (seiwald)
**	    Changed text of E_GC0103_GCN_NOT_USER.
*/

#define _GC_CLASS 12

/*
** GCA errors
*/

E_GC0001_ASSOC_FAIL:E_GE9088_COMM_ERROR
"Association failure"
E_GC0002_INV_PARM:E_GE9088_COMM_ERROR
"Invalid service request parameter"
E_GC0003_INV_SVC_CODE:E_GE9088_COMM_ERROR
"Invalid service code in call sequence"
E_GC0004_INV_PLIST_PTR:E_GE9088_COMM_ERROR
"Invalid parameter list pointer"
E_GC0005_INV_ASSOC_ID:E_GE9088_COMM_ERROR
"Invalid association identifier"
E_GC0006_DUP_INIT:E_GE9088_COMM_ERROR
"Duplicate initiate request"
E_GC0007_NO_PREV_INIT:E_GE9088_COMM_ERROR
"No previous initiation request"
E_GC0008_INV_MSG_TYPE:E_GE9088_COMM_ERROR
"Invalid message type identifier"
E_GC0009_INV_BUF_ADDR:E_GE9088_COMM_ERROR
"Invalid buffer address"
E_GC000A_INT_PROT_LVL:E_GE9088_COMM_ERROR
"Internal IPC protocol level mismatch between client and server"
E_GC000B_RMT_LOGIN_FAIL:E_GE9088_COMM_ERROR
"Remote login failure: invalid password/userid"
E_GC000C_API_VERSION_INVALID:E_GE9088_COMM_ERROR
"API version specified is invalid."
E_GC000D_ASSOCN_REFUSED:E_GE9088_COMM_ERROR
"Association request refused by server"
E_GC0010_BUF_TOO_SMALL:E_GE9088_COMM_ERROR
"Buffer is too small: no room for data"
E_GC0011_INV_CONTENTS:E_GE9088_COMM_ERROR
"Invalid buffer contents"
E_GC0012_LISTEN_FAIL:E_GE9088_COMM_ERROR
"Failure to issue listen"
E_GC0013_ASSFL_MEM:E_GE9088_COMM_ERROR
"Association failure (memory allocation)"
E_GC0014_SAVE_FAIL:E_GE9088_COMM_ERROR
"Save failure: CL level system error"
E_GC0015_BAD_SAVE_NAME:E_GE9088_COMM_ERROR
"Restore failure: save name not recognized"
E_GC0016_RESTORE_FAIL:E_GE9088_COMM_ERROR
"Restore failure: CL level system error"
E_GC0017_RSTR_OPEN:E_GE9088_COMM_ERROR
"Restore failure: error opening restore file"
E_GC0018_RSTR_READ:E_GE9088_COMM_ERROR
"Restore failure: error writing to restore file"
E_GC0019_RSTR_CLOSE:E_GE9088_COMM_ERROR
"Restore failure: error closing restore file"
E_GC0020_TIME_OUT:E_GE9088_COMM_ERROR
"Time_out expiration: service request incomplete"
E_GC0021_NO_PARTNER:E_GE9088_COMM_ERROR
"Requested association partner is unavailable"
E_GC0022_NOT_IACK:E_GE9088_COMM_ERROR
"Association is in purge state due to expedited data receipt.
Only GCA_IACK may be sent."
E_GC0023_ASSOC_RLSED:NO_GE
"Association was terminated by a GCA_RELEASE sent by one partner"
E_GC0024_DUP_REQUEST:E_GE9088_COMM_ERROR
"A duplicate request was currently outstanding on this association"
E_GC0025_NM_SRVR_ID_ERR:E_GE9088_COMM_ERROR
"Unable to connect to Name Server: Name Server ID incorrect"
E_GC0026_NM_SRVR_ERR:E_GE9088_COMM_ERROR
"Unable to connect to Name Server: system error during connection"
E_GC0027_RQST_PURGED:E_GE9088_COMM_ERROR
"Request was purged by interrupt message sent or received"
E_GC0028_LISTEN_RESOURCE:E_GE9088_COMM_ERROR
"GCA_LISTEN failed: server resource shortage.  See system administrator"
E_GC0029_RQST_FAIL:E_GE9088_COMM_ERROR
"GCA_REQUEST failed: unable to connect to server"
E_GC002A_RQST_RESOURCE:E_GE9088_COMM_ERROR
"GCA_REQUEST failed: server resource shortage.  See system administrator"
E_GC002B_SND1_FAIL:E_GE9088_COMM_ERROR
"GCA_SEND failed: see system error status."
E_GC002C_SND2_FAIL:E_GE9088_COMM_ERROR
"GCA_SEND failed: see system error status."
E_GC002D_SND3_FAIL:E_GE9088_COMM_ERROR
"GCA_SEND failed: see system error status."
E_GC002E_RCV1_FAIL:E_GE9088_COMM_ERROR
"GCA_RECEIVE failed: see system error status."
E_GC002F_RCV2_FAIL:E_GE9088_COMM_ERROR
"GCA_RECEIVE failed: see system error status."
E_GC0030_RCV3_FAIL:E_GE9088_COMM_ERROR
"GCA_RECEIVE failed: see system error status."
E_GC0031_USRPWD_FAIL:E_GE9088_COMM_ERROR
"Unable to validate username/password. See system error status."
E_GC0032_NO_PEER:E_GE9088_COMM_ERROR
"GCA_LISTEN peer read failed: probably Name Server connection."

/*
** GCN errors
*/

E_GC0100_GCN_SVRCLASS_NOTFOUND:E_GE9088_COMM_ERROR
"Name Service failed: Server Class %0c is not found."
E_GC0101_GCN_FILE_OPEN:E_GE9088_COMM_ERROR
"Name server can't read/write file %0c.
Check II_CONFIG and name subdirectory."
E_GC0103_GCN_NOT_USER:E_GE9088_COMM_ERROR
"%0c is not a name server superuser."
E_GC0104_GCN_USERS_FILE:E_GE9088_COMM_ERROR
"Cannot open users file to verify user's privileges."
E_GC0110_GCN_SYNTAX_ERR:E_GE9088_COMM_ERROR
"Syntax Error: %0c"
E_GC0120_GCN_BAD_RECLEN:E_GE9088_COMM_ERROR
"Name server file %0c has wrong record length."
E_GC0121_GCN_NOMEM:E_GE9088_COMM_ERROR
"Name server allocator call failed reading file %0c."
E_GC0122_GCN_2MANY:E_GE9088_COMM_ERROR
"Name server startup failed - another name server already running."
E_GC0123_GCN_NSID_FAIL:E_GE9088_COMM_ERROR
"Name server startup failed - can't set name server public listen id."
E_GC0124_GCN_NO_IINAME:E_GE9088_COMM_ERROR
"Name server startup failed - can't read file iiname.all.  
Check II_CONFIG and name subdirectory."
E_GC0125_GCN_BAD_FILE:E_GE9088_COMM_ERROR
"Name server can't read/write file %0c.
Check II_CONFIG and name subdirectory."

E_GC0130_GCN_BAD_SYNTAX:E_GE9088_COMM_ERROR
"User supplied database name is not formatted properly. 
Format for database name is: vnode::dbname/class.
Vnode:: and /class are optional."
E_GC0131_GCN_BAD_RMT_VNODE:E_GE9088_COMM_ERROR
"A default remote vnode has been defined (by II_GCN_RMT_VNODE), 
but connection or login information for that vnode is missing."
E_GC0132_GCN_VNODE_UNKNOWN:E_GE9088_COMM_ERROR
"User provided a vnode as part of the database name (vnode::dbname),
but connection information for that vnode is missing.
Enter connection information for the vnode using NETU."
E_GC0133_GCN_LOGIN_UNKNOWN:E_GE9088_COMM_ERROR
"User provided a vnode as part of the database name (vnode::dbname),
but login information for that vnode is missing.
Enter login information for the vnode using NETU."
E_GC0134_GCN_INT_NQ_ERROR:E_GE9088_COMM_ERROR
"The Name Server was unable to access the internal name queue
of available servers."
E_GC0135_GCN_BAD_SVR_TYPE:E_GE9088_COMM_ERROR
"A default server class has been defined (by II_GCN_SVR_TYPE),
but that server class is not known to the Name Server."
E_GC0136_GCN_SVRCLASS_UNKNOWN:E_GE9088_COMM_ERROR
"User provided a server class as part of the database name (dbname/class),
but that server class is not known to the Name Server."
E_GC0137_GCN_NO_GCC:E_GE9088_COMM_ERROR
"User attempted INGRES/NET connection by providing a vnode as part of
the database name (vnode::dbname), but no Communication Server is 
running in this installation."
E_GC0138_GCN_NO_SERVER:E_GE9088_COMM_ERROR
"User provided a server class as part of the database name (dbname/class),
but no servers of that class are running in this installation."
E_GC0139_GCN_NO_DBMS:E_GE9088_COMM_ERROR
"No INGRES DBMS servers are running in this installation."

E_GC0151_GCN_STARTUP:E_GE9088_COMM_ERROR
"Name Server normal startup."
E_GC0152_GCN_SHUTDOWN:E_GE9088_COMM_ERROR
"Name Server normal shutdown."

/*
** GCC errors
*/

E_GC2001_STARTUP:NO_GE
"Communication Server normal startup."
E_GC2002_SHUTDOWN:E_GE9088_COMM_ERROR
"Communication Server normal shutdown."
E_GC2003_STARTUP_PARM:NO_GE
"A Communication Server startup parameter was incorrectly specified."
E_GC2004_ALLOCN_FAIL:E_GE9088_COMM_ERROR
"Memory allocation failure in Communication Server."
E_GC2005_INIT_FAIL:NO_GE
"Communication Server initialization failed."
E_GC2006_STARTUP:NO_GE
"Communication Server normal startup: server rev. level %0c."
E_GC2007_NO_AUTH_INET:NO_GE
"Communication Server startup failure: No authorization for INGRES/NET"
E_GC2008_OUT_OF_MEMORY:E_GE9088_COMM_ERROR
"Comm Server memory shortage: unable to allocate object type %0c."

E_GC2201_ASSOC_END:NO_GE
"Normal local association termination: session end."
E_GC2202_AL_FSM_INP:E_GE9088_COMM_ERROR
"AL internal error: Invalid input event in AL FSM"
E_GC2203_AL_FSM_STATE:E_GE9088_COMM_ERROR
"AL internal error: Invalid state transition in AL FSM"
E_GC2204_ASSOC_START:NO_GE
"Normal local association initiation: session start."
E_GC2205_RMT_ABORT:E_GE9088_COMM_ERROR
"Session failure: ABORT received from remote partner."
E_GC2206_LCL_ASSOC_FAIL:E_GE9088_COMM_ERROR
"Session failure: association with local partner failed."
E_GC2207_RMT_INIT_FAIL:E_GE9088_COMM_ERROR
"Session failure: remote association initiation failed, status follows."
E_GC2208_CONNCTN_ABORT:E_GE9088_COMM_ERROR
"Session failure: releasing local association."
E_GC2209_SND_FAIL:E_GE9088_COMM_ERROR
"GCA send failure: reason follows."
E_GC220A_NRM_RCV_FAIL:E_GE9088_COMM_ERROR
"GCA normal receive failure: reason follows."
E_GC220B_EXP_RCV_FAIL:E_GE9088_COMM_ERROR
"GCA expedited receive failure: reason follows."
E_GC220C_AL_INIT_FAIL:E_GE9088_COMM_ERROR
"GCC AL initialization failure: reason follows."
E_GC220D_AL_PASSWD_FAIL:E_GE9088_COMM_ERROR
"GCC AL received an invalid (null) password from a remote client."
E_GC220E_AL_FSM_STATE:E_GE9088_COMM_ERROR
"AL internal error: Invalid state transition in AL FSM.\
 Input = %0d, state = %1d"
E_GC220F_AL_INTERNAL_ERROR:E_GE9088_COMM_ERROR
"GCC Comm Server AL internal error. Please see system admimistrator."
E_GC2210_LCL_ABORT:E_GE9088_COMM_ERROR
"Session failure: Local ABORT condition."
E_GC2211_INCMG_ASSOC:NO_GE
"Incoming association initiation from local user %0c, as remote user %1c."
E_GC2212_OUTGNG_ASSOC:NO_GE
"Outgoing local association initiation for remote user %0c."
E_GC2213_AL_NO_CCB:E_GE9088_COMM_ERROR
"GCC internal error: CCB for connection %0d cannot be found."
E_GC2214_MAX_IB_CONNS:E_GE9088_COMM_ERROR
"GCC server has exceeded max inbound connections."
E_GC2215_MAX_OB_CONNS:E_GE9088_COMM_ERROR
"GCC server has exceeded max outbound connections."
E_GC2216_GCA_LSN_FAIL:E_GE9088_COMM_ERROR
"GCC AL processing of a GCA_LISTEN completion has failed."
E_GC2217_GCA_RQRESP_FAIL:E_GE9088_COMM_ERROR
"GCA request response failure: reason follows."
E_GC2218_GCA_DISASSOC_FAIL:E_GE9088_COMM_ERROR
"GCA disassociate failure: reason follows."
E_GC2219_GCA_REGISTER_FAIL:E_GE9088_COMM_ERROR
"GCA registration with Name Server failed: reason follows."

E_GC2401_CONNCTN_END:NO_GE
"Normal Presentation Connection termination."
E_GC2402_PL_FSM_INP:E_GE9088_COMM_ERROR
"PL internal error: Invalid input event in PL FSM"
E_GC2403_PL_FSM_STATE:E_GE9088_COMM_ERROR
"PL internal error: Invalid state transition in PL FSM"
E_GC2404_PL_FSM_STATE:E_GE9088_COMM_ERROR
"PL internal error: Invalid state transition in PL FSM.\
 Input = %0d, state = %1d"
E_GC2405_PL_INTERNAL_ERROR:E_GE9088_COMM_ERROR
"GCC Comm Server PL internal error. Please see system admimistrator."

/*
** new codes added to discriminate between different PL input event failures
*/
E_GC2406_IE_BAD_PCON_RSP_RSLT:E_GE9088_COMM_ERROR
"PL internal error. P_CONNECT response result not ACCEPT or REJECT.\
 Please see system admimistrator."
E_GC2407_IE_BAD_PCON_TYPE:E_GE9088_COMM_ERROR
"PL internal error. P_CONNECT type not REQUEST or RESPONSE.\
 Please see system admimistrator."
E_GC2408_IE_BAD_PRSL_TYPE:E_GE9088_COMM_ERROR
"PL internal error. P_RELEASE type not REQUEST or RESPONSE.\
 Please see system admimistrator."
E_GC2409_IE_BAD_SCON_RSP_RSLT:E_GE9088_COMM_ERROR
"PL internal error. S_CONNECT confirm result not ACCEPT or REJECT.\
 Please see system admimistrator."
E_GC240A_IE_BAD_SCON_TYPE:E_GE9088_COMM_ERROR
"PL internal error. S_CONNECT type not REQUEST or RESPONSE.\
 Please see system admimistrator."
E_GC240B_IE_BAD_SREL_CNF_RSLT:E_GE9088_COMM_ERROR
"PL internal error. S_RELEASE confirm result not AFFIRMATIVE or NEGATIVE.\
 Please see system admimistrator."
E_GC240C_IE_BAD_SREL_TYPE:E_GE9088_COMM_ERROR
"PL internal error. S_RELEASE type not INDICATION or CONFIRM.\
 Please see system admimistrator."
E_GC240D_IE_BAD_SUAB_PDU:E_GE9088_COMM_ERROR
"PL internal error. S_U_ABORT PPDU_ID not ARP or ARU.\
 Please see system admimistrator."
E_GC240E_IE_BAD:E_GE9088_COMM_ERROR
"PL internal error. Bad INPUT event.\
 Please see system admimistrator."
E_GC240F_INIT_DOC_STACK:E_GE9088_COMM_ERROR
"PL internal error. INIT_DOC_STACK Failed.\
 Please see system admimistrator."
E_GC2410_PERF_CONV:E_GE9088_COMM_ERROR
"PL internal error. PERFORM_CONVERSION Failed.\
 Please see system admimistrator."
E_GC2411_OD_PTR_NULL:E_GE9088_COMM_ERROR
"PL internal error. OD_PTR is NULL in INIT_DOC_STACK.\
 Please see system admimistrator."
E_GC2412_DOC_EL_NULL:E_GE9088_COMM_ERROR
"PL internal error. DOC_EL is NULL in PERFORM_CONVERSION.\
 Please see system admimistrator."
E_GC2413_GET_NEXT_ATOM:E_GE9088_COMM_ERROR
"PL internal error. GET_NEXT_ATOM failed in PERFORM_CONVERSION.\
 Please see system admimistrator."

E_GC2601_CONNCTN_END:NO_GE
"Normal Session Connection termination."
E_GC2602_SL_FSM_INP:E_GE9088_COMM_ERROR
"SL internal error: Invalid input event in SL FSM"
E_GC2603_SL_FSM_STATE:E_GE9088_COMM_ERROR
"SL internal error: Invalid state transition in SL FSM"
E_GC2604_SL_FSM_STATE:E_GE9088_COMM_ERROR
"SL internal error: Invalid state transition in SL FSM.\
 Input = %0d, state = %1d"
E_GC2605_SL_INTERNAL_ERROR:E_GE9088_COMM_ERROR
"GCC Comm Server SL internal error. Please see system admimistrator."

E_GC2801_NTWK_CONNCTN_START:NO_GE
"Normal Network Connection initiation."
E_GC2802_NTWK_CONNCTN_END:NO_GE
"Normal Network Connection termination."
E_GC2803_PROT_ID:E_GE9088_COMM_ERROR
"Invalid (unknown) protocol identifier %0c."
E_GC2804_PROT_ID:E_GE9088_COMM_ERROR
"Invalid (unknown) protocol identifier - use NETU to check 'network software \
 type' for target node."
E_GC2805_INTERNAL_ERROR:E_GE9088_COMM_ERROR
"TL internal error: Invalid input event in TL FSM"
E_GC2806_TL_FSM_INP:E_GE9088_COMM_ERROR
"TL internal error: Invalid input event in TL FSM"
E_GC2807_TL_FSM_STATE:E_GE9088_COMM_ERROR
"TL internal error: Invalid state transition in TL FSM"
E_GC2808_NTWK_OPEN_FAIL:E_GE9088_COMM_ERROR
"Network open failed for protocol %0c, listen port %1c; status follows."
E_GC2809_NTWK_CONNECTION_FAIL:E_GE9088_COMM_ERROR
"Network connection failed for protocol %0c, to node %1c, port %2c;\
 status follows."
E_GC280A_NTWK_INIT_FAIL:E_GE9088_COMM_ERROR
"Network initialization failure: all protocol opens failed"
E_GC280B_TL_INIT_FAIL:E_GE9088_COMM_ERROR
"GCC TL initialization failure."
E_GC280C_TL_FSM_STATE:E_GE9088_COMM_ERROR
"TL internal error: Invalid state transition in TL FSM.\
 Input = %0d, state = %1d"
E_GC280D_NTWK_ERROR:E_GE9088_COMM_ERROR
"Network operation error: See Comm. Server log for detail."
E_GC280E_TL_INTERNAL_ERROR:E_GE9088_COMM_ERROR
"GCC Comm Server TL internal error. Please see system admimistrator."
E_GC280F_OUTGNG_NTWK_CONNCTN:E_GE9088_COMM_ERROR
"Outgoing network connection: remote connection id = %0x."
E_GC2810_INCMG_NTWK_CONNCTN:E_GE9088_COMM_ERROR
"Incoming network connection: remote connection id = %0x."
E_GC2811_TL_INVALID_TPDU:E_GE9088_COMM_ERROR
"TL internal error: Received invalid TPDU type %0x."
E_GC2812_TL_INVALID_CONNCTN:E_GE9088_COMM_ERROR
"TL internal error: Received unknown connection id %0d from partner."
E_GC2813_NTWK_LSN_FAIL:E_GE9088_COMM_ERROR
"Network listen failed for protocol %0c, listen port %1c; status follows."
E_GC2815_NTWK_OPEN:E_GE9088_COMM_ERROR
"Network open complete for protocol %0c, listen port %1c."

E_GCFFFE_INCOMPLETE:E_GE9088_COMM_ERROR
"Client must invoke service again."
E_GCFFFF_IN_PROCESS:E_GE9088_COMM_ERROR
"Request is not complete"                               

/*
**    Message file for Facility code: _OP
*/


#define _OP_CLASS 4


E_OP0000_OK:E_GE0000_OK
"successful completion"
E_OP0001_USER_ERROR:NO_GE
"user error has been reported by optimizer"
E_OP0002_NOMEMORY:E_GE8D04_NO_RESOURCE
"optimizer ran out of 
\n    memory before generating execution plan"
E_OP0003_ASYNCABORT:NO_GE
"asynchronous abort was detected "
E_OP0004_RDF_GETDESC:NO_GE
"request for relation 
\n    information failed - possibly because table was modified or deleted"
E_OP0005_GRANGETABLE:E_GE8CA0_SYSTEM_LIMIT
"global range table 
\n    overflow - query too complex"
E_OP0006_TIMEOUT:NO_GE
"optimizer timeout 
\n    warning - suboptimal solution found"
E_OP0007_INVALID_OPCODE:NO_GE
"opf_call entry point 
\n    received an invalid operation code"
E_OP0008_NOEXECUTE:NO_GE
"query optimized 
\n   - trace flag indicates no execution should take place "
E_OP0009_TUPLE_SIZE:E_GE8CA0_SYSTEM_LIMIT
"query cannot be processed,
\n   - size of tuple intermediate node is too large for all cases"
E_OP0080_ULM_CLOSESTREAM:NO_GE
"consistency check 
\n    - error deallocating a memory stream "
E_OP0081_NOSTATUS:NO_GE
"consistency check 
\n    - no status provided by error handling mechanism "
E_OP0082_UNEXPECTED_EX:NO_GE
"consistency check 
\n    - unexpected exception occurred"
E_OP0083_UNEXPECTED_EXEX:NO_GE
"consistency check 
\n    - really bad, got an unexpected exception trying to recover 
\n    from an exception"
E_OP0084_DEALLOCATION:NO_GE
"consistency check 
\n    - resource deallocation error "
E_OP0085_QSO_LOCK:NO_GE
"consistency check 
\n    - query tree could not be retrieved from QSF "
E_OP0086_ULM_STARTUP:NO_GE
"consistency check 
\n    - server startup error - cannot startup ULM"
E_OP0087_RDF_INITIALIZE:NO_GE
"consistency check 
\n    - server startup error - cannot startup RDF"
E_OP0088_ULM_SHUTDOWN:NO_GE
"consistency check 
\n    - server shutdown error - cannot shutdown ULM"
E_OP0089_ALTER:NO_GE
"consistency check 
\n    - invalid alter opcode"
E_OP008A_SCF_SERVER_MEM:E_GE8D04_NO_RESOURCE
"consistency check 
\n    - error obtaining memory for OPF server control block"
E_OP008B_SCF_SERVER_MEM:NO_GE
"consistency check 
\n    - error releasing memory for OPF server control block"
E_OP008C_SEMAPHORE:NO_GE
"consistency check 
\n    - could not initialize optimizer server semaphore"
E_OP008D_RDF_UNFIX:NO_GE
"consistency check 
\n    - could not unfix an RDF table"
E_OP008E_RDF_INVALIDATE:NO_GE
"consistency check 
\n    - could not invalidate an RDF table"
E_OP008F_RDF_MISMATCH:NO_GE
"The RDF cache 
\n    could not be synchronized with parser time stamp, SCF should reparse 
\n    query,  this message should not be reported by SCF"
E_OP0090_PAINE:NO_GE
"consistency check 
\n    - PAINE handling routine could not be initialized by OPF"
E_OP0091_AIC:NO_GE
"consistency check 
\n    - AIC handling routine could not be initialized by OPF"
E_OP0092_SEMWAIT:NO_GE
"consistency check 
\n    - semaphore wait error for OPF server control block"
E_OP0093_ULM_ERROR:NO_GE
"consistency check 
\n    - memory manager error"
E_OP0094_RDF_SHUTDOWN:NO_GE
"consistency check 
\n    - error shutting down RDF facility"
E_OP0095_PARSE_TREE:NO_GE
"consistency check 
\n    - invalid parse tree from PSF"
E_OP0096_INSF_OPFMEM:NO_GE
"insufficient opf memory
\n    - must be at least 2 * number_sessions * 50K"
E_OP0200_TUPLEOVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"The intermediate
\n    aggregate relation required too large a tuple size"
E_OP0201_ATTROVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"too many columns 
\n    in aggregate temporary relation"
E_OP0202_VAR_OVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"query too complicated
\n    too many tables referenced, subselects or aggregates specified"
E_OP0280_SCOPE:NO_GE
"consistency check 
\n    - scoping rules for variables used in query were violated"
E_OP0281_SUBQUERY:NO_GE
"consistency check 
\n    - unexpected subquery type in optimizer"
E_OP0282_FAGG:NO_GE
"consistency check
\n    - function aggregate structure inconsistent"
E_OP0283_PROJECT:NO_GE
"consistency check
\n    - unexpected aggregate projections request"
E_OP0284_RESDOM:NO_GE
"consistency check
\n    - resdom list not monotonically decreasing"
E_OP0285_UNIONAGG:NO_GE
"consistency check
\n    - union or aggregate unexpected in this context"
E_OP0300_ATTRIBUTE_OVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"the joinop attributes
\n    array overflowed - query too complex"
E_OP0301_EQCLS_OVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"equivalence class 
\n    array overflowed - query too complex"
E_OP0302_BOOLFACT_OVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"too many boolean 
\n    factors defined in query - query too complex "
E_OP0303_RTOVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"joinop range table 
\n    does not have sufficient room for variables - query is too complex"
E_OP0304_FUNCATTR_OVERFLOW:E_GE8CA0_SYSTEM_LIMIT
"function attribute
\n    table overflowed - query too complex"
E_OP0305_NOCONVERSION:NO_GE
"cannot convert 
\n    constant to key - warning, keyed access cannot be used"
E_OP0380_JNCLAUS:NO_GE
"Consistency check 
\n    - left and right part of conditional are incompatible"
E_OP0381_FUNCATTR:NO_GE
"consistency check 
\n    - inconsistent typing error in function attributes "
E_OP0382_FATTR_CREATE:NO_GE
"consistency check 
\n    - function attribute should have been created "
E_OP0383_TYPEMISMATCH:NO_GE
"consistency check 
\n    - unexpected type mismatch in equivalence class"
E_OP0384_NOATTS:NO_GE
"consistency check 
\n    - no attribute in equivalence class "
E_OP0385_EQCLS_MERGE:NO_GE
"consistency check 
\n    - more than one equivalence class in merge "
E_OP0386_BFCREATE:NO_GE
"consistency check 
\n    - assumption about ordering of boolean factor list failed "
E_OP0387_VARNO:NO_GE
"consistency check 
\n    - varno from parser range table out of range"
E_OP0388_VARBITMAP:NO_GE
"consistency check 
\n    - var bit map is invalid"
E_OP0389_EQUIJOIN:NO_GE
"consistency check 
\n    - equi join , unexpected types for operands"
E_OP038A_REPEAT:NO_GE
"consistency check 
\n    - repeat query parameter not available when expected"
E_OP038B_NO_SUBSELECT:NO_GE
"consistency check 
\n    - cannot find subselect for correlated variable"
E_OP038C_UNION:NO_GE
"consistency check 
\n    - union subquery expected"
E_OP038D_CORRELATED:NO_GE
"consistency check
\n    - expecting correlated aggregate"
E_OP0400_MEMORY:E_GE8D04_NO_RESOURCE
"ran out of 
\n    enumeration memory, optimizer needs more memory"
E_OP0480_MISSING_TEMP_HISTO:NO_GE
"consistency check
\n    - histogram from temporary relation expected"
E_OP0481_KEYINFO:NO_GE
"Consistency check
\n    - cannot find OPB_BFKEYINFO pointer when expected"
E_OP0482_HISTOGRAM:NO_GE
"Consistency check
\n    - cannot find OPH_HISTOGRAM structure for equivalence class"
E_OP0483_HIST_BUFFER:E_GE8CA0_SYSTEM_LIMIT
"Consistency check
\n    - overflow of temporary histogram buffer "
E_OP0484_HISTTYPE:NO_GE
"Consistency check
\n    - unexpected histogram type "
E_OP0485_HISTAND:NO_GE
"consistency check
\n    - attempt to AND two incompatible histograms "
E_OP0486_NOKEY:NO_GE
"consistency check
\n    - no key found when one expected "
E_OP0487_NOEQCLS:NO_GE
"consistency check
\n    - no joining equivalence class found when expected "
E_OP0488_JOINNOEQCLS:NO_GE
"consistency check
\n    - no joining equivalence class found when expected "
E_OP0489_NOFUNCHIST:NO_GE
"consistency check
\n    - no histogram found for function attribute when expected "
E_OP048A_NOCARTPROD:NO_GE
"consistency check
\n    - no joining equivalence class and there is not a cartesean product "
E_OP048B_COST:NO_GE
"consistency check
\n    - cpu or disk i/o cost is negative "
E_OP048C_SORTCOST:NO_GE
"consistency check
\n    - sort cost routine failed"
E_OP048D_QUERYTYPE:NO_GE
"consistency check
\n    - unknown subquery type"
E_OP048E_COTYPE:NO_GE
"consistency check
\n    - unknown CO node type"
E_OP048F_NULL:NO_GE
"consistency check
\n    - expecting IS NULL or IS NOT NULL operator id"
E_OP0490_CSSTATISTICS:NO_GE
"consistency check
\n    - CSstatistics called failed, so enumeration phase cannot
\n    be bounded"
E_OP0491_NO_QEP:NO_GE
"consistency check
\n    - no query plan was found"
E_OP0492_ORIG_NODE:NO_GE
"consistency check
\n    - orig node expected on end of CO list"
E_OP0493_MULTIATTR_SORT:NO_GE
"consistency check
\n    - invalid multi-attribute sort type"
E_OP0494_ORDERING:NO_GE
"consistency check
\n   - unexpected ordering present"
E_OP0495_CO_TYPE:NO_GE
"consistency check
\n   - unexpected CO node type"
E_OP0496_EXACT:NO_GE
"consistency check
\n   - exact multi-attribute ordering expected"
E_OP0497_INCOMPATIBLE:NO_GE
"consistency check
\n   - incompatible orderings found in CO node"
E_OP0498_NO_EQCLS_FOUND:NO_GE
"consistency check
\n   - equivalence class not found when expected"
E_OP0499_NO_CO:NO_GE
"consistency check
\n   - expected CO memory to be available "
E_OP049A_CSALTR_SESSION:NO_GE
"consistency check
\n   - could not turn on/off cpu accounting via CSaltr_session"
E_OP049B_SORT:NO_GE
"consistency check
\n   - useless sort  node found in query plan"
E_OP049C_INEXACT:NO_GE
"consistency check - inexact ordering expected"
/*%
** Parameters:
**
** System Status:
**      The query will be terminated.  Ongoing transactions are not aborted.
**
**	The system has found, during optimization, that a query plan has been
**	generated which has an unnecessary sort node.
**	This type of query plan should not be generated.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0680_TARGETLIST:NO_GE
"consistency check
\n    - target list has non PST_RESDOM nodes in it"
E_OP0681_UNKNOWN_NODE:NO_GE
"consistency check
\n    - unknown query tree node type found"
E_OP0682_UNEXPECTED_NODE:NO_GE
"consistency check
\n    - unexpected query tree node - PST_CURVAL"
E_OP0683_DUMPQUERYTREE:NO_GE
"consistency check
\n    - error dumping query tree "
E_OP0684_DUMPQUERYNODE:NO_GE
"consistency check
\n    - error dumping query tree node "
E_OP0685_RESOLVE:NO_GE
"consistency check
\n    - error resolving query tree node "
E_OP0686_CQMODE:NO_GE
"consistency check
\n    - Illegal query mode for compilation."
E_OP0687_NOQTREE:NO_GE
"consistency check
\n    - No query tree when expected."
E_OP0688_INEXACT:NO_GE
"consistency check
\n    - expecting inexact ordering nesting in an exact ordering"
E_OP0689_BAD_TREE:NO_GE
"consistency check
\n    - bad query tree defined, applying deMorgan's laws"
E_OP068A_BAD_TREE:NO_GE
"consistency check
\n    - error converting query tree to text"
E_OP068B_BAD_VAR_NODE:NO_GE
"consistency check
\n    - error converting query tree to text; var node"
E_OP068C_BAD_TARGET_LIST:NO_GE
"A bad select (or subselect) target list has been found"
E_OP068D_QTREE:NO_GE
"error copying query tree nodes"
E_OP0700_ADC_CVINTO:NO_GE
"cannot build key 
\n    because conversion does not exist"
E_OP0701_ADF_EXCEPTION:NO_GE
"ADF exception 
\n    - see error block for info, ADF exception warning"
E_OP0702_ADF_EXCEPTION:NO_GE
"ADF exception 
\n    - user error number between 120 and 16F "
E_OP0780_ADF_HISTOGRAM:NO_GE
"consistency check
\n    - the ADF returned an unexpected error - adc_hg_dtln"
E_OP0781_ADI_FIDESC:NO_GE
"consistency check
\n    - error calling adi_fidesc "
E_OP0782_ADI_CALCLEN:NO_GE
"consistency check
\n    - error calling adi_calclen "
E_OP0783_ADI_FICOERCE:NO_GE
"consistency check
\n    - error calling adi_ficoerce"
E_OP0784_ADC_KEYBLD:NO_GE
"consistency check
\n    - error calling adc_keybld"
E_OP0785_ADC_HELEM:NO_GE
"consistency check
\n    - error calling adc_helem"
E_OP0786_ADC_DHMAX:NO_GE
"consistency check
\n    - default upper bound for uniform histogram error from ADF "
E_OP0787_ADC_DHMIN:NO_GE
"consistency check
\n    - default lower bound for uniform histogram error from ADF "
E_OP0788_ADC_CVINTO:NO_GE
"consistency check
\n    - conversion of histogram element unexpectedly failed"
E_OP0789_ADC_HMIN:NO_GE
"consistency check
\n    - error calling min histogram value function routine"
E_OP078A_ADC_HMAX:NO_GE
"consistency check
\n    - error calling max histogram value function routine"
E_OP078C_ADC_COMPARE:NO_GE
"consistency check
\n    - ADF compare routine failed "
E_OP078D_ADI_OPID:NO_GE
"consistency check
\n    - ADF operator ID routine failed in startup"
E_OP078E_ADE_CXSPACE:NO_GE
"consistency check
\n    - Error estimating the size of a CX."
E_OP078F_ADE_BGNCOMP:NO_GE
"consistency check
\n    - Error beginning a compilation."
E_OP0790_ADE_CONSTGEN:NO_GE
"consistency check
\n    - Error generating a constant."
E_OP0791_ADE_INSTRGEN:NO_GE
"consistency check
\n    - Error generating an instruction."
E_OP0792_ADE_INFORMSP:NO_GE
"consistency check
\n    - Error informing ADF of a larger CX."
E_OP0793_ADE_ENDCOMP:NO_GE
"consistency check
\n    - Error ending a compilation."
E_OP0794_ADI_OPNAME:NO_GE
"consistency check
\n    - cannot get operator name from operator ID"
E_OP0795_ADF_EXCEPTION:NO_GE
"ADF exception - non-user error, 
\n    ADF error return code returned in error block"
E_OP0796_NO_COMPLEMENT:NO_GE
"consistency check
\n    - no complement exists for function when one was expected"
E_OP0797_OP_USE:NO_GE
"consistency check
\n	- unexpected operator construction PREFIX, POSTFIX, or INFIX mismatch"
E_OP0798_OP_TYPE:NO_GE
"consistency check
\n	- unexpected operator type"
E_OP0800_NODEFAULT:NO_GE
"The user has not 
\n    specified all of the required columns for insertion."
E_OP0880_NOT_READY:NO_GE
"This feature or 
\n    section of code is not yet implemented."
E_OP0881_DMUFUNC:NO_GE
"Illegal DMU function 
\n    type for compilation."
E_OP0882_QSF_CREATE:E_GE8D04_NO_RESOURCE
"Error calling QSF 
\n    to create a new object."
E_OP0883_QSF_SETROOT:NO_GE
"Error calling QSF 
\n    to set the root of an object."
E_OP0884_RELEQC:NO_GE
"Consistency error: 
\n    Orig node returns an eqc that is not found in the originated relation."
E_OP0885_COTYPE:NO_GE
"Consistency error: 
\n    Illegal CO node type."
E_OP0886_QSF_PALLOC:E_GE8D04_NO_RESOURCE
"Error calling QSF
\n     to allocate a piece of memory."
E_OP0887_CONSTTYPE:NO_GE
"Illegal datatype 
\n    or length for a constant in a query tree."
E_OP0888_ATT_NOT_HERE:NO_GE
"Attribute is not 
\n    available at a CO node."
E_OP0889_EQ_NOT_HERE:NO_GE
"Eqc is not available 
\n    at a CO node."
E_OP0890_MAX_BASE:NO_GE
"Too many bases 
\n    added to a QEN_ADF struct."
E_OP0891_MAX_ROWS:NO_GE
"Too large a row 
\n    number used to add a base to a QEN_ADF struct."
E_OP0892_MAX_REPEATS:NO_GE
"Too large a repeat 
\n    query number used to add a base to a QEN_ADF struct."
E_OP0893_NV_REPEAT:NO_GE
"A repeat query 
\n    number that is not valid is being added to a QEN_BASE."
E_OP0894_EXCESSATTS:NO_GE
"Too many attributes 
\n    are being returned from a subselect"
E_OP0895_NO_SEJQTREE:NO_GE
"Consistency Check: 
\n    There is no query tree for a subselect join."
E_OP0896_MODIFY_ATTR:NO_GE
"Consistency Check: the default attribute number 1 to modify does not exist."
E_OP0897_BAD_CONST_RESDOM:NO_GE
"Consistency Check: a query tree constant or resdom node uses an
\n    source/destination type."
E_OP0898_ILL_LVARNO:NO_GE
"Consistency Check: An out of range local variable number was found in a
\n    query tree for a DB procedure."
E_OP0899_BAD_TARGNO:NO_GE
"A target number was found (in a query tree or otherwise) that is illegal."
E_OP089A_DIFF_BOOL_FACT:NO_GE
"Consistency check: new and old boolean factor maps are different."
/*%
**  Parameters:
**	None.
**	
**  Explanation:
**	This error is raised only in the debugging version
**	of the code. It will be raised when tha boolean factor
**	bitmap received from OPF is different from the one obtained
**	using the old version of the code.
**
**  System Status:
**	Execution of the query will be terminated.
**
**  User Action:
**	Report problem to Development.
*/
E_OP089B_PJOIN_FOUND:NO_GE
"Consistency check: PJOIN implied."
/*%
**  Parameters:
**	None.
**	
**  Explanation:
**	This error is raised when the query plan implies a PJOIN.
**	PJOINs are not supported by this version of the code.
**
**  System Status:
**	Execution of the query will be terminated.
**
**  User Action:
**	Report problem to Technical Support.
*/
E_OP089C_BAD_VALID_LIST:NO_GE
"Consistency check: Invalid valid list."
/*%
**  Parameters:
**	None.
**	
**  Explanation:
**	This is an error raised when internal DBMS 
**	structures contain invalid data. 
**
**  System Status:
**	Execution of the query will be terminated.
**
**  User Action:
**	Report problem to Technical Support.
*/
E_OP0900_INTERRUPT:NO_GE
"\nExecution aborted due to user interrupt."
E_OP0901_UNKNOWN_EXCEPTION:NO_GE
"\nUnknown exception occurred in optimizer utility. Value is %0d."
E_OP0902_ADFSTARTUP:NO_GE
"%0c: internal error occurred when starting up Abstract Data Type Facility."
E_OP0903_ADFSESSION:NO_GE
"%0c: internal error occurred when starting up Abstract Data Type Facility session."
E_OP0904_OPENFILE:E_GE75B2_NOT_FOUND
"%0c: cannot open input file '%1c', OS status:%2d.\n%3c"
E_OP0905_ARGUMENTS:E_GE8CA0_SYSTEM_LIMIT
"%0c: more than %1d arguments."
E_OP0906_STACK:E_GE8D04_NO_RESOURCE
"%0c: ran out of stack space."
E_OP0907_UNIQUECELLS:E_GE80E8_LOGICAL_ERROR
"%0c: bad unique cells value: The value %3d specified in the -zu# flag
\nwas not a number, or not in the range greater than %1d or less than %2d."
E_OP0908_HISTOCELLS:E_GE80E8_LOGICAL_ERROR
"%0c: bad histogram cell count, -zr# range allowed %1d<#<%2d, but got :%3d."
E_OP0909_INGRESFLAGS:E_GE8CA0_SYSTEM_LIMIT
"%0c: more than %1d ingres flags, or unrecognizable flags."
E_OP090A_DATABASE:E_GE80E8_LOGICAL_ERROR
"%0c: unexpected parameter:'%1c'."
E_OP090B_DBLENGTH:E_GE8CA0_SYSTEM_LIMIT
"%0c: database name length must be <= %1d :'%2c'."
E_OP090C_RELLENGTH:E_GE8CA0_SYSTEM_LIMIT
"%0c: relation name length must be <= %1d :'%2c'."
E_OP090D_ATTLENGTH:E_GE8CA0_SYSTEM_LIMIT
"%0c: attribute name length must be <= %1d :'%2c'."
E_OP090E_PARAMETER:E_GE80E8_LOGICAL_ERROR
"%0c: unrecognized parameter: '%1c'."
E_OP090F_TABLES:E_GE8CA0_SYSTEM_LIMIT
"%0c: more than %1d tables for database  '%2c'."
E_OP0910_DUPTABID:NO_GE
"%0c: duplicate table IDs in database '%1c', table '%2c'."
E_OP0911_NOTABLE:E_GE7594_TABLE_NOT_FOUND
"%0c: database '%1c', table '%2c' cannot be found or is not owned by you."
E_OP0912_VIEW:E_GE80E8_LOGICAL_ERROR
"%0c: database '%1c', table '%2c' is a view and will be ignored."
E_OP0913_ATTRIBUTES:E_GE8CA0_SYSTEM_LIMIT
"%0c: database '%1c', number of columns in table '%2c' exceeds maximum of %3d."
E_OP0914_ATTRTUPLES:NO_GE
"%0c: database '%1c', table '%2c', column %3c inconsistent system catalog,
\n    more than one attribute tuple for attribute ID."
E_OP0915_NOATTR:E_GE759E_COLUMN_UNKNOWN
"optimizedb: database '%0c', table '%1c', column '%2c' not found."
E_OP0916_TYPE:NO_GE
"%0c: bad datatype id =%1d in iiattribute.attfrmt, column name '%2c'."
E_OP0917_HTYPE:NO_GE
"%0c: histogram type=%1d prec=%2d len=%3d in iiattribute, column name %4c,
\n   error->%5c"
E_OP0918_NOCOLUMNS:E_GE759E_COLUMN_UNKNOWN
"optimizedb: no columns for database '%0c', table '%1c'."
E_OP0919_READ_ERR:NO_GE
"Error %0d occurred when reading input file.\n%1c"
/*%
** Parameters:
**	%0d - error number.
**	%1c - error message if any.
**	
** System Status:
**      Problem occurred when reading contents of
**	an input file.
**
** User Action:
**      Try again, if problem persists contact vendor.
*/
E_OP091A_STACK:E_GE8D04_NO_RESOURCE
"%0c: ran out of dynamic memory to allocate (status %1d, size requested %2d)."
/*%
** Parameters:
**	%0d - utility name.
**	%1d - error status.
**	%2d - size requested.
**	
** System Status:
**      Problem occurred when allocating dynamic memory. Execution of
**	utility will be terminated.
**
** User Action:
**      Check values following command line flags -za and -zt,
**	try again. If problem persists contact vendor.
*/
E_OP091B_TUPLECOUNT:NO_GE
"optimizedb: bad retrieval, database '%0c', table '%1c', i:%2d."
E_OP091C_NOROWS:E_GE75B2_NOT_FOUND
"optimizedb: table '%0c' in database '%1c' contains no rows."
E_OP091D_HISTOGRAM:NO_GE
"optimizedb: histogram conversion failed, column '%0c', code = %1d."
E_OP091F_DATATYPE:NO_GE
"%0c: error displaying type %1d, prec %2d, length %3d"
E_OP0920_DATATYPE:NO_GE
"optimizedb: unsupported histogram datatype; type %0d, prec %1d, length %2d"
E_OP0921_ERROR:E_GE8CA0_SYSTEM_LIMIT
"consistency check - parameter count too large for optimizer utility."
E_OP0922_BAD_ERLOOKUP:NO_GE
"Cannot read error message %0d (%1x) for optimizer utility."
E_OP0923_DMF_BIT:NO_GE
"%0c: statistics exist; this should be reflected in system catalogs, but is not."
E_OP0924_NON_BASE:NO_GE
"%0c: non-base relation has statistics '%1c', data base '%2c'."
E_OP0925_NOSTATS:E_GE75B2_NOT_FOUND
"statdump: database '%0c', table '%1c', column '%2c' - statistics not found."
E_OP0926_DUPLICATES:NO_GE
"\nstatdump: database '%0c', table '%1c', column '%2c' - inconsistent iistats
catalog; %3d rows found."
E_OP0927_DUPLICATES:NO_GE
"\nstatdump: database '%0c', table '%1c', column '%2c' - inconsistent
iihistograms catalog; %3d rows with sequence %4d found."
E_OP092A_STATSDELETAED:NO_GE
"statdump: statistics for database '%0c', table '%1c', column '%2c' deleted."
E_OP092B_COMPARE:NO_GE
"optimizedb: data comparison failed, column '%0c', code = %1d."
E_OP092C_NULL:E_GE80E8_LOGICAL_ERROR
"optimizedb: Statistics for table '%0c', column '%1c' not created - NULL values found."
E_OP0930_NOT_BASE:E_GE80E8_LOGICAL_ERROR
"%0c: database '%1c', table '%2c' is not a base table and will be ignored."
E_OP0931_NO_FILE:E_GE80E8_LOGICAL_ERROR
"-i/-o flag must be followed by a file name."
E_OP0932_BAD_PREC:E_GE80E8_LOGICAL_ERROR
"Precision for floating point numbers should be in the [0,30] range."
E_OP0933_BAD_USER:E_GE80E8_LOGICAL_ERROR
"-u flag must be followed by a user name."
E_OP0934_BAD_SCAT_TYPE:NO_GE
"Table_type in iitables for 'iistats', 'iihistograms', 'iitables' or 'iiphysical_tables'
has an unexpected value of '%0c'."
E_OP0935_LVSN_CERR:NO_GE
"Error %0d occurred when converting SQL_LEVEL value of '%1c'."
E_OP0936_ADI_TYID:NO_GE
"An ADF error %0d occurred when converting data type '%1c'."
E_OP0937_OPEN_ERROR:E_GE75B2_NOT_FOUND
"%0c: Error %2d when opening file '%1c'.\n%3c"
E_OP0938_CLOSE_ERROR:NO_GE
"%0c: Error %2d when closing file '%1c'.\n%3c"
E_OP0939_TOO_MANY_COLS:E_GE8CA0_SYSTEM_LIMIT
"Internal query generated to retrieve MINMAX statistics data for
'%1c' table in '%0c' database' has too many columns in the target list.
Rerun optimizedb for the table and specify columns using -a flag."
E_OP093A_ADC_LENCHK:NO_GE
"An ADF error %0d occurred when computing result length for column '%1c'."
E_OP093B_TOO_MANY_ROWS:NO_GE
"More rows returned than expected."
E_OP093C_INP_FAIL:E_GE80E8_LOGICAL_ERROR
"%0c: Failed to read in statistics from '%1c' file."
E_OP093D_MINMAX_NOT_ALLOWED:E_GE80E8_LOGICAL_ERROR
"Requests to create minmax statistics and to read statistics from a file
are incompatible."
E_OP093E_NO_STATS:NO_GE
"%0c: Database '%1c', table '%2c' owned by '%3c' - no statistics found."
E_OP093F_BAD_COUNT:E_GE80E8_LOGICAL_ERROR
"Actual number of cells retrieved (%0d) differs from the number of cells
declared (%1d)"
/*%
** Parameters:
**	%0  The actual number of cells retrieved.
**	%1  The declared number of cells.
** System Status:
**      Creation of statistics for the given column will be aborted.
**
** User Action:
**      Please make sure that the declared number of cells following
**	words "histogram cells:" agrees with actual number of cell
**	entries. Check that cell numbers are numeric and in ascending
**	order.
*/
E_OP0940_READ_ERR:E_GE80E8_LOGICAL_ERROR
"Error %0d occurred when reading in statistical information from a file.\n%1c"
E_OP0941_TEXT_NOTFOUND:E_GE80E8_LOGICAL_ERROR
"Text line described by '%0c' not found in the input file when expected."
E_OP0942_ADI_PM_ENCODE:NO_GE
"An ADF error %0d occurred when executing adi_pm_encode()."
E_OP0943_ADI_FICOERCE:NO_GE
"An ADF error %0d occurred when executing adi_ficoerce()."
E_OP0944_ADF_FUNC:NO_GE
"An ADF error %0d occurred when executing adf_func()."
E_OP0945_HISTVAL_NOT_INC:E_GE80E8_LOGICAL_ERROR
"Histogram values for table '%0c', column '%1c' are not in ascending order.
\nProblem occurs around cell number %2d."
E_OP0946_CELL_NO:E_GE80E8_LOGICAL_ERROR
"Wrong cell number found; should start at 0 and should increase."
E_OP0947_WRONG_COUNT:E_GE80E8_LOGICAL_ERROR
"Cell count must be in the [0.0,1.0] range."
E_OP0948_CELL_TOO_FEW:E_GE80E8_LOGICAL_ERROR
"There must be at least %0d cells in a histogram."
E_OP0949_CELL_TOO_MANY:E_GE8CA0_SYSTEM_LIMIT
"Maximum number of cells exceeded."
E_OP094A_TRUNC:NO_GE
"Truncation occurred when assigning a character string to host language
variable."
E_OP094B_COL_NO_MISMATCH:E_GE80E8_LOGICAL_ERROR
"Mismatching number of result columns and result host language variables
in a FETCH or SELECT statement."
E_OP094C_ERR_CRE_SAMPLE:E_GE80E8_LOGICAL_ERROR
"Could not create table for sample data (sqlcode: %0d, sqlerrd[0]: %1d).\n%2c"
E_OP094D_CANT_SAMPLE:E_GE80E8_LOGICAL_ERROR
"Sampling feature can be used only on Ingres tables."
E_OP094E_INV_SAMPLE:E_GE80E8_LOGICAL_ERROR
"'-zs' flag must be followed by a number in the (0.0, 100.0) range
expressing percentage of table rows to be sampled."
E_OP094F_SAMPLE_NOT_ALLOWED:E_GE80E8_LOGICAL_ERROR
"Requests to create statistics based on sample data and to read statistics
from a file are incompatible."
E_OP0950_NOSAMPLING:E_GE80E8_LOGICAL_ERROR
"Sampling of '%0c' is not possible; no statistics will be created."
E_OP0951_BAD_NO_UNIQUE:E_GE80E8_LOGICAL_ERROR
"Wrong input value '%0f' for statistics on table '%1c', column '%2c';
\nnumber of unique values can never be less than 1."
E_OP0952_BAD_NO_ROWS:E_GE80E8_LOGICAL_ERROR
"Wrong input value '%0d' for statistics on table '%1c', column '%2c';
\nnumber of rows can never be less than 1."
E_OP0953_BAD_NO_PAGES:E_GE80E8_LOGICAL_ERROR
"Wrong input value '%0d' for statistics on table '%1c', column '%2c';
\nnumber of pages can never be less than 1."
E_OP0954_BAD_NO_OVFLOW:E_GE80E8_LOGICAL_ERROR
"Wrong input value '%0d' for statistics on table '%1c', column '%2c';
\nnumber of overflow pages can never be less than 0."
E_OP0955_BAD_NULL_COUNT:E_GE80E8_LOGICAL_ERROR
"Wrong input value '%0f' for statistics on table '%1c', column '%2c';
\nnumber of null values can never be less than 0."
E_OP0956_TOTAL_COUNT_OFF:E_GE80E8_LOGICAL_ERROR
"The sum total of normalized cell counts for table '%1c', column '%2c' is '%0f';
expected value is 1.0 +/- %3f."
E_OP0957_NO_CAPABILITY:E_GE80E8_LOGICAL_ERROR
"Capability '%0c' is missing in iidbcapabilities catalog."
E_OP0958_COUNTING_ROWS:E_GE80E8_LOGICAL_ERROR
"Row count for table '%0c' not available from catalogs. Counting rows."
E_OP0959_CAP_ERR:NO_GE
"IIdbcapabilities catalog is inconsistent, DBMS_TYPE indicates STAR, but DISTRIBUTED
value is missing or its value is not equal to 'Y'."
E_OP095A_TUPLE_OVFLOW:E_GE8CA0_SYSTEM_LIMIT
"Error when creating minmax statistics on '%0c' table: resulting row is too wide.
Try creating statistics using -a flag, one column at a time."
E_OP095B:NO_GE
"System or hardware exception occurred in optimizer utility.\n%0c"
/*%
** Parameters:
**	%0  Pointer to the string containing message describing exception.
** System Status:
**      Execution of the utility will be aborted.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP095C:NO_GE
"Internal error ocurred when converting character string to
a floating point number."
/*%
** Parameters:
**	None.
** System Status:
**      Problem occurred when convering character string to
**	a floating point number. This should never happen.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP095D:NO_GE
"Cannot create statistics on column '%0c' of table '%1c'.
\nDatatype for the column is a User Defined Datatype (UDT)."
/*%
** Parameters:
**	%0c	- name of the attribute in error
**	%1c	- name of the table to which the attribute belongs.
**	
** System Status:
**      An attempt was made to create statistics on a column with
**	a User Defined Datatype.
**
** User Action:
**      User attempted to create statistics on a column with
**	a User Defined Datatype (UDT). Request will be ignored
**	and optimizedb will continue creating statistics on remaining
**	columns.
*/
E_OP095E:NO_GE
"Invalid unique flag value `%0c' specified for column `%1c' in
table `%2c'.\nExpected values are `Y' or `N'."
/*%
** Parameters:
**	%0c	- unique flag value.
**	%1c	- name of the attribute.
**	%2c	- name of the table to which the attribute belongs.
**	
** System Status:
**      Invalid unique flag value has been found when reading
**	statistics from a file.
**
** User Action:
**      User attempted to read in statistics from a file and an
**	invalid unique flag value was specified. Correct the error
**	in the file; ensure that the value is either 'Y' or `N'.
*/
E_OP095F:NO_GE
"The unique flag value `%0c' specified for column `%1c' in table `%2c'
does not conform to optimizer rules."
/*%
** Parameters:
**	%0c	- unique flag value.
**	%1c	- name of the attribute.
**	%2c	- name of the table to which the attribute belongs.
**	
** System Status:
**      Optimizedb, when reading in statistics from a file,
**	discovered that the unique flag value does not conform
**	with the optimizer rules (see below). The value and statistics
**	will be stored as requested.
**
** User Action:
**      User should consider changing the unique flag value according
**	to the optimizer rule below:
**
**	if the number_of_unique_values >= 0.9 * row_count
**	    set value to `Y'
**	else
**	    set value to `N'
**
**	number_of_unique_values	- number of unique values in the
**	relation (labelled `unique values:') in the statistics file.
**	row_count - number of rows in the relation (labelled `rows:')
**	in the statistics file.
*/  
E_OP0961:NO_GE
"Truncation occurred when assigning a character string to host language
variable."
/*%
** Parameters:
**	None.
** System Status:
**      The query will continue. Data retrieved from DBMS has been
**	truncated.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0962:NO_GE
"Null values eliminated from aggregates."
/*%
** Parameters:
**	None.
** System Status:
**      The query will continue. NULL values have been eliminated from
**	aggregates.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0963:E_GE80E8_LOGICAL_ERROR
"Mismatching number of result columns and result host language variables
in a FETCH or SELECT statement."
/*%
** Parameters:
**	None.
** System Status:
**      The query will continue. Number of host language variables
**	receiving is different from result columns.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0964:NO_GE
"UPDATE or DELETE statement does not have the WHERE clause."
/*%
** Parameters:
**	None.
** System Status:
**      The query will continue.
**
** User Action:
**      None.
*/
E_OP0965:NO_GE
"Unexpected warning."
/*%
** Parameters:
**	None.
** System Status:
**      The query will continue.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0966:NO_GE
"Abnormal termination of an active transaction."
/*%
** Parameters:
**	None.
** System Status:
**      An active transaction was abnormally terminated.
**	Possible cause: serialization error (deadlock,
**	log full).
**
** User Action:
**      Re-execute your request.
*/
E_OP0967:NO_GE
"Unexpected warning."
/*%
** Parameters:
**	None.
** System Status:
**      The query will continue.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0970:E_GEC2EC_SERIALIZATION
"Transaction has been automatically restarted due to serialization error."
/*%
** Parameters:
**	None.
** System Status:
**      Optimizedb, statdump utility will continue execution.
**	Serialization error was returned by the DBMS and transaction
**	has been automatically restarted.
**
** User Action:
**      None necessary. However, if this happens often, the DBA should
**	consider increasing the size of the log file.
*/
E_OP0971:E_GEC2EC_SERIALIZATION
"Transaction has been aborted due to serialization error."
/*%
** Parameters:
**	None.
** System Status:
**      Execution of optimizedb, statdump will be terminated.
**	Transaction was automatically restarted and it failed
**	again.
**
** User Action:
**      Please report the problem to your DBA. Consider increasing
**	the size of the log file.
*/
E_OP0972:NO_GE
"Internal error occurred when scaling number of unique values.
\nSample row count:%0d, Unique values in sample:%1f, Total row count:%2d."
/*%
** Parameters:
**	%0d - number of rows in table holding sample data.
**	%1f - number of unique values in table holding sample data.
**	%2d - number of rows in table on which statistics are created.
**	
** System Status:
**      When scaling number of unique values for sampled statistics
**	an internal error occurred.
**
** User Action:
**      Please report the problem and displayed data to vendor.
*/
E_OP0973_TOTAL_COUNT_OFF:E_GE80E8_LOGICAL_ERROR
"Statistics for table '%1c', column '%2c': the sum total of normalized
cell counts ('%0c') is expected to be in the 1.0 +/- %3c range."
/*%
** Parameters:
**	%0c - the sum total of normalized cell counts.
**	%1c - table name.
**	%2c - column name.
**	%3c - expected error value
**	
** System Status:
**      All normalized cell counts are expected to add up to 1.0.
**	When reading statistics in, the cell counts are added up
**	and a check is made whether the result is within error
**	margin (1 +/- delta). The delta value takes into account the
**	number of cells added. If the result is outside of the
**	error margin, a warning is issued. Statistics will be stored
**	in catalogs.
**
** User Action:
**      Check the cell count values.
*/
E_OP0974_BAD_MAX_COLS:E_GE80E8_LOGICAL_ERROR
"Error %0d occurred when converting iidbcapabilities value '%1c' of
MAX_COLUMNS to integer format."
/*%
** Parameters:
**	%0d - error code passed by conversion routine.
**	%1c - value of MAX_COLUMNS from iidbcapabilities.
**	
** System Status:
**	An error occurred when converting an ascii number to an integer
**	format.
**	
** User Action:
**      Check the MAX_COLUMNS value in iidbcapabilities catalog.
*/
E_OP0A00_MAXSITE:E_GE8CA0_SYSTEM_LIMIT
"distributed user error - max number of sites in query exceeded"
E_OP0A01_USELESS_PLAN:NO_GE
"distributed plan expected to be useful but it was not"
E_OP0A80_UNSUPPORTED:NO_GE
"distributed user error - this type of distributed query not supported"
E_OP0A81_SITE:NO_GE
"site has not been selected for query plan"
E_OP0A82_RESULT_VAR:NO_GE
"unexpected result variable number for update statement"
E_OP0A83_CONSTRAINT:NO_GE
"consistency check - unexpected state for correlated subselect constraint"
E_OP0A84_TEMP_REQ:NO_GE
"consistency check - temp table required but not a data transfer"
E_OP0A85_UPDATE_VAR:NO_GE
"consistency check - expecting update variable to be defined"
E_OP0A86_EQCLASS:NO_GE
"consistency check - cannot add attribute to equivalence class for update"
E_OP0A87_UPDT_MATCH:NO_GE
"consistency check - new range variable for target relation expected to be
the last item in base relation list"
E_OP0A88_REPEAT:NO_GE
"consistency check - repeat query parameter is out of order"
E_OP0A89_SAGG:NO_GE
"consistency check - simple aggregate constant count is inconsistent"
E_OP0A8A_OPCSAGG:NO_GE
"consistency check - unexpected tree structure found for simple aggregate"
E_OP0A8B_OPCPARM:NO_GE
"consistency check - parameter nodes out of order for simple aggregate"
E_OP0A8C_COST:NO_GE
"consistency check - cost resolution for final query plan cannot be found"
E_OP0A8D_VARIABLE:NO_GE
"consistency check - OPF variable list is not ordered"
E_OP0A8E_SUB_TYPE:NO_GE
"consistency check - unexpected subquery type"
E_OP0A8F_UPDATE_MODE:NO_GE
"consistency check - unknown update mode"
E_OP0A90_NO_ATTRIBUTES:NO_GE
"consistency check - no attributes in the for update list of the cursor"
E_OP0A91_CORELATED:NO_GE
"consistency check 
\n	- query tree corelation inconsistent"
E_OP0A92_BAD_LANGUAGE:NO_GE
"consistency check 
\n	- bad language ID found for query tree to text convertion"
E_OP0A93_BAD_QUERYMODE:NO_GE
"consistency check 
\n	- unexpected query mode"
E_OP0A94_OPCFPROJ:NO_GE
"consistency check
\n	- projection/function aggregate error"
E_OP0A95_OPCBYTARGET:NO_GE
"consistency check
\n	- function aggregate has no by/target list"
E_OP0A96_OPCSUBQTYPE:NO_GE
"consistency check
\n	- unrecognized subquery type"
E_OP0A97_OPCSORTTYPE:NO_GE
"Consistency Check
\n	- Sort requires keying on unsortable datatype."
/*%
** Parameters:
**
** System Status:
**      The query will be terminated.  Ongoing transactions are not aborted.
**
**	The system has found, during optimization, that a query plan has been
**	generated which depends upon the sort order of an unsortable datatype.
**	This type of query plan should not be generated.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0B80_UNKNOWN_STATEMENT:NO_GE
"internal DB procedure error- unknown statement type"
E_OP0B81_VARIABLE_DEC:NO_GE
"internal DB procedure error- expecting variable declaration at beginning of procedure"
/*%
** Parameters:
**
** System Status:
**      The query will be terminated.  Ongoing transactions are not aborted.
**
**	The system has found, during optimization, that the internal structure
**	of a DB Procedure has a missing declaration statement.
**
** User Action:
**      Please report the problem to your product vendor.
*/
E_OP0B82_MISSING_PARAMETER:NO_GE
"internal DB procedure error- internally generated variables for a DB procedure
\n      are not in order or are missing "
/*%
** Parameters:
**
** System Status:
**      The query will be terminated.  Ongoing transactions are not aborted.
**
**	The system has found, during optimization, that the internal structure
**	of a DB Procedure has local variables which are out of expected order
**
** User Action:
**      Please report the problem to your product vendor.
*/

/*
**    Message file for Facility code: _PS
*/


#define _PS_CLASS 5


E_PS0000_OK:E_GE0000_OK
"PSF successful return status"
E_PS0001_USER_ERROR:E_GEA154_RAISE_ERROR
"User error in query"
E_PS0002_INTERNAL_ERROR:NO_GE
"Internal error in PSF"
E_PS0003_INTERRUPTED:NO_GE
"Query aborted due to user request."
E_PS0004_EXCEPTION:E_GE9D08_DATAEX_NOSUB
"Exception occurred in PSF"
E_PS0005_USER_MUST_ABORT:NO_GE
"An event has occurred which requires the user to abort the current 
transaction."
E_PS0006_UNKNOWN_OPERATION:NO_GE
"The requested operation is unknown to PSF."
E_PS0007_INT_OTHER_FAC_ERR:NO_GE
"PSF detected an internal error when calling other facility."
E_PS0008_RETRY:NO_GE
"Operation should be retried."
E_PS0101_BADERRTYPE:NO_GE
"psf_error: bad error type"
E_PS0102_TOO_MANY_ERRS:NO_GE
"PSF has lost track of some of the errors that have occurred in this
request."
E_PS0103_ERROR_NOT_FOUND:NO_GE
"Error message corresponding to error, %0d, not found."
E_PS0201_BAD_QLANG:E_GE79E0_UNSUP_LANGUAGE
"psq_bgn_session: bad query language id"
E_PS0202_QLANG_NOT_ALLOWED:E_GE79E0_UNSUP_LANGUAGE
"query language not allowed"
E_PS0203_NO_DECIMAL:E_GE8D68_CONFIG_ERROR
"psq_bgn_session: decimal marker not specified"
E_PS0204_BAD_DISTRIB:E_GE8D68_CONFIG_ERROR
"psq_bgn_session: bad distributed/non-distributed id"
E_PS0205_SRV_NOT_INIT:NO_GE
"psq_bgn_session: parser facility has not been started up"
E_PS0206_SES_NOT_FOUND:E_GE8D04_NO_RESOURCE
"no such session in the parser facility"
E_PS0207_YACC_ALLOC:E_GE8D04_NO_RESOURCE
"Could not allocate the YACC control block."
E_PS0208_TOO_MANY_SESS:E_GE8CA0_SYSTEM_LIMIT
"Maximum number of sessions exceeded for parser facility"
E_PS0309_ALLOC_SYMTAB:E_GE8D04_NO_RESOURCE
"Could not allocate the symbol table."
E_PS030A_MULT_STMT_ALT:NO_GE
"Attempt to parse second statement in alternate query"
E_PS030B_RETRY_AND_ALT:NO_GE
"Attempt to retry an alternate query"
E_PS030C_NO_PREV_STMT:NO_GE
"Attempt to parse the second statement of a query without doing the first"
E_PS030D_NO_MORE_QRYS:NO_GE
"No more queries in this go block"
E_PS030F_BACKUP:NO_GE
"Error while trying to back up a token"
E_PS0311_YACC_UNKNOWN:NO_GE
"Unknown error from yacc"
E_PS0351_UNKNOWN_LOCKPARM:E_GE7918_SYNTAX_ERROR
"line %0d, Unknown internal parameter '%1d' for the SET LOCKMODE command.
          This is an internal error.  Please report it to your vendor."
E_PS035E_BAD_DECIMAL:E_GE7D00_QUERY_ERROR
"An illegal decimal specifier was requested."
E_PS0365_BAD_FUNC_INST:NO_GE
"pstnorml.c: Bad function instance %0d."
E_PS0366_NO_COMPLEMENT:NO_GE
"pstnorml.c: Comparison operator '%1c' with no complement."
E_PS0370_OPEN_TEXT_CHAIN:E_GE8D04_NO_RESOURCE
"Could not open a memory stream for text chain."
E_PS0371_ALLOC_TEXT_CHAIN:E_GE8D04_NO_RESOURCE
"Could not allocate a new block from the text chain."
E_PS0372_CLOSE_TEXT_CHAIN:NO_GE
"Could not close the text chain."
E_PS0373_OPEN_QSF_TEXT:E_GE8D04_NO_RESOURCE
"Could not open a QSF stream for storing query text information."
E_PS0374_ALLOC_QSF_TXT:E_GE8D04_NO_RESOURCE
"Could not allocate a block in QSF for query text information."
E_PS0375_UNLOCK_QSF_TEXT:NO_GE
"Could not unlock the QSF object containing query text information."
E_PS0376_INSUF_MEM:E_GE8D04_NO_RESOURCE
"Newly allocated scanner symbol block too small to accomodate data."
E_PS0377_ADI_PM_ERR:NO_GE
"Unexpected error executing adi_pm_encode call."
E_PS0378_SCF_TERM_ERR:NO_GE
"Unexpected error calling SCF to obtain terminal info."
E_PS0379_QSF_T_OR_D_ERR:NO_GE
"Error calling QSF to translate/define."
E_PS037A_QSF_TRANS_ERR:NO_GE
"Error calling QSF to translate."
E_PS03A0_SUBSEL_IN_JQUAL:E_GE7D00_QUERY_ERROR
"Subselects are not supported inside join qualifications."
/*
** Parameters:
**	None
** Explanation:
**	When parsing join qualification, a subselect is encountered.  Presently
**	this is not supported.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Reformulate this query so that there are no subselects inside the join
**	qualification.  
*/
E_PS03A1_JQUAL_IN_NATJOIN:E_GE7D00_QUERY_ERROR
"You may not provide join_search qualifications for NATURAL joins."
/*
** Parameters:
**	None
** Explanation:
**	Qualification was specified with a NATURAL join which is not supported.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Reformulate the query so it conforms to correct syntax.
*/
E_PS03A2_MISSING_JQUAL:E_GE7D00_QUERY_ERROR
"Join_search qualification is mandatory for joins other than NATURAL."
/*
** Parameters:
**	None
** Explanation:
**	Qualification was not supplied, and the join type is not NATURAL.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Reformulate the query so it conforms to correct syntax.
*/
E_PS03A3_COL_NOT_IN_JREL:E_GE7D00_QUERY_ERROR
"Line %0d: Column '%1c' appearing in join_search condition is
\nnot an attribute of any relation involved in the join."
/*
** Parameters:
**	Line where column name appeared
**	Name of the column
** Explanation:
**	join_search condition referenced a column from a relation not
**	involved in the join
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS03A4_NOT_A_JREL:E_GE7D00_QUERY_ERROR
"Line %0d: Relation '%1c' appearing in join_search condition is not
\ninvolved in the join."
/*
** Parameters:
**	Line where relation name appears
**	Name of the relation
** Explanation:
**	join_search condition referenced a relation not involved in the join
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS03A5_NO_COMMON_COLS:E_GE7D00_QUERY_ERROR
"Line %0d: Left and right relations of this NATURAL join
\ncontain no like-named columns which may be compared."
/*
** Parameters:
**	Line where the last right relation of the join appears
** Explanation:
**	In order to form a natural join, left and right relations must contain
**	at least one pair of like-named comparable columns so that an equi-join
**	may be formed.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS03A6_AGGR_IN_JOIN_COND:E_GE7918_SYNTAX_ERROR
"Use of aggregates inside the join_search condition is illegal."
/*
** Parameters:
**	None
** Explanation:
**	It is illegal to have aggregates in the join_search condition of outer
**	joins. This restriction only applies to use of aggregates in join_search
**	condition outside of subselects (which are presently also disallowed in
**	join_search conditioon, but this may change.)
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0401_CUR_NOT_FOUND:E_GE75A8_CURSOR_UNKNOWN
"no such cursor in the parser facility"
E_PS0421_CRTIND_NOT_IND_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not create an index owned by '%0c'."
/*
** Parameters:
**	Name of the index owner as specified by the user.
** Explanation:
**	User entered CREATE INDEX owner.index ... and owner is different from
**	the current user
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0422_CRTIND_NOT_TBL_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not create an index on table '%0c' owned by '%1c'."
/*
** Parameters:
**	Name of table for which an index can not be created
**	Name of the table owner as specified by the user.
** Explanation:
**	User entered CREATE INDEX indexname on owner.table ... and owner is
**	different from the current user.  One may not create an index on someone
**	else's table.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0423_CRTIND_INSUF_PRIV:E_GE84D0_NO_PRIVILEGE
"Insufficient privilege to CREATE INDEX on catalog '%0c'."
/*
** Parameters:
**	Name of catalog for which an index can not be created
** Explanation:
**	Either user attempted to create an index on a core catalog or
**	user didn't have catalog update privilege and tried to create an index
**	on a catalog which is not an extended catalog.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0424_CRTTBL_NOT_OBJ_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not create a table owned by '%0c'."
/*
** Parameters:
**	Name of the table owner as specified by the user.
** Explanation:
**	User entered CREATE TABLE owner.tbl_name ... and owner is different from
**	the current user
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0425_CRTVIEW_NOT_OBJ_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not create a view owned by '%0c'."
/*
** Parameters:
**	Name of the view owner as specified by the user.
** Explanation:
**	User entered CREATE VIEW owner.view_name ... and owner is different from
**	the current user
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0426_ILLEGAL_TBL_REF:E_GE84D0_NO_PRIVILEGE
"%0c: You may not apply this query to
\n table '%1c' owned by '%2c'."
/*
** Parameters:
**	- offending query
**	- name of the table
**	- name of the object owner as specified by the user
** Explanation:
**	User attempted to execute a query explicitly referencing a table
**	which may not be referenced by this user.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0427_ILLEGAL_DBP_REF:E_GE84D0_NO_PRIVILEGE
"%0c: You may not apply this query to
\n database procedure '%1c' owned by '%2c'."
/*
** Parameters:
**	- offending query
**	- name of the database procedure
**	- name of the object owner as specified by the user
** Explanation:
**	User attempted to execute a query explicitly referencing a database
**	procedure which may not be referenced by this user.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0428_CRTDBP_NOT_DBP_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not create a database procedure owned by '%0c'."
/*
** Parameters:
**	Name of the database procedure owner as specified by the user.
** Explanation:
**	User entered [CREATE] PROCEDURE owner.dbp_name ... and owner is
**	different from the current user
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0429_DROPDBP_NOT_DBP_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not drop a database procedure owned by '%0c'."
/*
** Parameters:
**	Name of the database procedure owner as specified by the user.
** Explanation:
**	User entered DROP PROCEDURE owner.dbp_name ... and owner is
**	different from the current user
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS042A_CANT_GRANT_VIEW_PERM:E_GE84D0_NO_PRIVILEGE
"You may not CREATE or GRANT permit on view '%0c'.
\nYou do not own one or more of the underlying tables."
/*
** Parameters:
**	Name of a view as specified by the user.
** Explanation:
**	User attempted to GRANT a permit on a view, but at least one of the
**	underlying relations is not owned by him/her.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS042B_GRANTTAB_NOT_FOUND:E_GE7594_TABLE_NOT_FOUND
"GRANT: could not obtain description of an underlying table
\nfor the view on which a permit is being granted.
\nPlease contact your technical representative."
/*
** Parameters:
**	None
** Explanation:
**	To grant a permit on a view, one has to own all underlying relations.
**	PSF has been unable to obtain a description for at least one of the
**	underlying relations.  This is a very unusual and troublesome
**	occurrence.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Contact the system administrator.
*/
E_PS042C_CANT_GRANT_DBP_PERM:E_GE84D0_NO_PRIVILEGE
"You may not grant permit on database procedure '%0c'.
\nYou are not allowed to grant permit on one or more tables
\nand/or database procedures used in '%0c'."
/*
** Parameters:
**	Name of a database procedure as specified by the user.
** Explanation:
**	User attempted to GRANT a permit on a database procedure, but at the
**	time the database procedure has been created, it has been determined
**	that he will not be allowed to grant permits on it because he was not
**	allowed to grant a permit on one or more tables and/or database
**	procedures used in the definition of the database procedure.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS042D_CRTSYN_NOT_OBJ_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not create a synonym owned by '%0c'."
/*
** Parameters:
**	Name of the synonym owner as specified by the user.
** Explanation:
**	User entered CREATE SYNONYM owner.synonym_name... and owner is different
**	from the current user
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter.
*/
E_PS042E_DROPSYN_NOT_OBJ_OWNER:E_GE84D0_NO_PRIVILEGE
"You may not drop a synonym owned by '%0c'."
/*
** Parameters:
**	Name of the synonym owner as specified by the user.
** Explanation:
**	User entered DROP SYNONYM owner.synonym_name... and owner is different
**	from the current user
** System Status:
**	Synonym owned by another user will not be dropped.
**	If any other synonyms were listed in this statement, they will be
**	processed in turn.
** User Action:
**	Correct the error and reenter
*/
E_PS042F_UNGRANTABLE_DBPROC:E_GE84D0_NO_PRIVILEGE
"You may not execute database procedure '%0c' owned by '%1c'.
\nPermits on database procedure '%0c' owned by '%1c' will
\nnot be honored: '%1c' may not grant permit on one or
\nmore tables and/or database procedures used in '%0c'."
/*
** Parameters:
**	Name of a database procedure as specified by the user.
**	Name of the owner of the database procedure
** Explanation:
**	While trying to recreate a database procedure P1 which is not owned by
**	the current user, it was determined that the permits on P1 shall not
**	be honored.  This can occur if P1 uses at least one table or database
**	procedure which is not owned by the owner of P1.  Since we disallow
**	users to grant access to objects not owned by them, we will similarly
**	disregard any permits which may have been issued on P1.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Contact owner of the database procedure or change the query which
**	caused the error to not use database procedure in question.
*/
I_PS0451_SYN_WAS_SPECIFIED:NO_GE
"%0c: '%1c' owned by '%2c' is a synonym for
\n'%3c' owned by '%4c'."
/*
** Parameters:
**	query
**	name of a synonym
**	name of synonym owner
**	name of the object for which the above synonym is defined
**	name of the object
** Explanation:
**	This is an informational message which will be displayed in conjunction
**	with various error messages if the object specified by the user was a
**	synonym.
** System status:
**	N/A
** User action:
**	N/A
*/
E_PS0452_CANT_DROP_SYN:E_GE7D00_QUERY_ERROR
"DROP: '%0c' is a synonym.
\nUse 'DROP SYNONYM' to drop a synonym or
\n'DROP [TABLE|VIEW|INDEX]' to drop an object on which a synonym
\nhas been defined."
/*
** Parameters:
**	query
**	name of a synonym
** Explanation:
**	User entered DROP synonym_name.
**	One must use DROP SYNONYM to drop the synonym or
**	DROP [TABLE|VIEW|INDEX] to drop the object for which the synonym was
**	defined.
** System status:
**	The synonym will not be dropped.  If there were other objects listed in
**	the DROP statement, they will be processed in turn.
** User action:
**	Correct error and reenter.
*/
E_PS0453_SYN_ALREADY_EXISTS:E_GE75F8_DEF_RESOURCE
"%0c: object cannot be created.
\nYou already own synonym '%1c'."
/*
** Parameters:
**	query
**	name of the object as specified by the user
** Explanation:
**	User tried to create a new object, but he already owns a synonym by the
**	same name.  This message is intended to be used in conjunction with QUEL
**	queries: CREATE/DEFINE VIEW/RETRIEVE INTO.
** System status:
**	Current statement is aborted.
** User action:
**	User can give the new object a different name or use
**	DROP SYNONYM (SQL) to destroy an existing synonym.
*/
E_PS0454_CREATE_SYN_ERROR:E_GE98BC_OTHER_ERROR
"CREATE SYNONYM: Failed to create synonym '%0c'.
\nPlease contact system administrator."
/*
** Parameters:
**	%0c -	name of synonym
** Explanation:
**	CREATE SYNONYM query failed due to some internal error.
** System Status:
**	Current statement is aborted.
** User Action:
**	Contact system administrator.
*/
E_PS0455_CRT_SYN_ON_SYN:E_GE7D00_QUERY_ERROR
"CREATE SYNONYM: '%0c' is a synonym.
\nSynonyms may not be created on other synonyms."
/*
** Parameters:
**	%0c -	Name of the synonym on which user tried to create a synonym
** Explanation:
**	User attempted to create a synonym on another synonym.
** System Status:
**	Current statement is aborted.
** User Action:
**	Correct error and continue.
*/
E_PS0456_DROP_SYN_ERROR:E_GE98BC_OTHER_ERROR
"DROP SYNONYM: Failed to drop synonym '%0c'.
\nPlease contact system administrator."
/*
** Parameters:
**	%0c -	name of synonym
** Explanation:
**	DROP SYNONYM query failed due to some internal error.
** System Status:
**	Current statement is aborted.
** User Action:
**	Contact system administrator.
*/
E_PS0457_DROPSYN_NOT_SYN:E_GE7D00_QUERY_ERROR
"DROP SYNONYM: '%0c' is a %1c.
\nUse '%2c'."
/*
** Parameters:
**	object name as specified by the user
**	object type
**	query to be used to drop this object
** Explanation:
**	User entered DROP SYNONYM obj_name, but obj_name does not refer to a
**	synonym.
** System status:
**	If user specified other synonyms to be dropped, they will be processed
**	in turn.
** User action:
**	Correct error and reenter.
*/
E_PS0458_ULH_ALIAS_ERROR:E_GE8D04_NO_RESOURCE
"A ULH error occurred due to concurrent activity and/or
\nimproperly chosen RDF startup parameters."
/*
** Parameters:
**	none
** Explanation:
**      RDF called ulh_define_alias(), and ULH returned E_UL0119_ALIAS_MEM.
**	This situation would occur if there is too much concurrent activity
**	and/or if the RDF startup parameters were poorly chosen.
**
**	To correct this problem, one could rerun the query anticipating
**	that some of the concurrent activity has died down.  However, a more
**	permanent solution would be to stop the server and restart it with
**	larger maximum 	number of tables and larger number of synonyms per table
**	which can be specified in iirundbms.com_<node> using
**	/rdf=(max_tbls=N1, tbl_synonyms=N2).
** System status:
**      Current statement is aborted; program continues.
** User action:
**      Modify iirundbms.com_<node> RDF startup parameters.  Specify
**      larger numbers, restart the server and try again.
*/
E_PS0470_REFERENCES_ON_VIEW:E_GE7D00_QUERY_ERROR
"GRANT: '%0c' is a view.
\n'REFERENCES' privilege may not be granted on views."
/*
** Parameters:
**	%0c	name of a view
** Explanation:
**	User attempted to grant REFERENCES privilege on a view, which is
**	illegal.
** System status:
**      Current statement is aborted; program continues.
** User action:
**      Change query and reenter.  Either REFERENCES must be removed from the
**	privilege list or the offending view must be removed from the object
**	list.
*/
E_PS0471_CHECK_CONSTRAINT:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: aggregates and subselects cannot
\nbe used inside a CHECK constraint."
/*
** Parameters:
**	none
** Explanation:
**	A subselect or an aggregate function was encountered inside a CHECK
**	constrint of CREATE TABLE statement.  These constructs are explicitly
**	disallowed inside a CHECK constraint.
** System status:
**      Current statement is aborted; program continues.
** User action:
**      Correct an error and reenter.
*/
E_PS0472_COL_CHECK_CONSTRAINT:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: CHECK constraint for column '%0c' of table
\n'%1c' cannot reference column '%2c'."
/*
** Parameters:
**	%0c	name of the column for which a CHECK constraint was being
**		defined
**	%1c	name of the table being created
**	%2c	name of another column which illegally appeared inside the
**		first column's CHECK constraint
** Explanation:
**	Column CHECK constraint cannot reference any other columns.  A reference
**	to a column name different from the one for which the CHECK constraint
**	is being defined has been found.
** System status:
**      Current statement is aborted; program continues.
** User action:
**      Correct an error and reenter.
*/
E_PS0473_TBL_CHECK_CONSTRAINT:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: CHECK constraint for table '%0c' references
\nnonexistent attribute '%1c'."
/*
** Parameters:
**	%0c	name of a table being created
**	%1c	name of nonexistent attribute found in one of the table CHECK
**		constraints.
** Explanation:
**	Table CHECK constraint may reference only the attributes of the table
**	being created.  Name of a nonexistent attribute was discovered in one of
**	table CHECK constraints for the table being created.
** System status:
**      Current statement is aborted; program continues.
** User action:
**      Correct an error and reenter.
*/
E_PS0474_CHECK_CONSTRAINT:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: CHECK constraint for table '%0c' may
\nnot references attributes of table '%1c'."
/*
** Parameters:
**	%0c	name of a table being created
**	%1c	name of table attribute of which was illegally used
**		in a CHECK constraint.
** Explanation:
**	A CHECK constraint may reference only the attributes of the table
**	being created.  User attempted to use an attribute of a table other than
**	the one being created inside a CHECK constraint.
** System status:
**      Current statement is aborted; program continues.
** User action:
**      Correct an error and reenter.
*/
E_PS0501_SESSION_OPEN:NO_GE
"There were open sessions when trying to shut down the parser facility."
E_PS0502_LEFTOVER_MEM:NO_GE
"psq_shutdown: After shutting down all sessions, there was still allocated
memory in the parser facility."
E_PS0519_QSF_INFO:NO_GE
"Error getting QSF information on object."
E_PS0601_CURSOR_OPEN:NO_GE
"There were open cursors when ending a session."
E_PS0602_CORRUPT_MEM:NO_GE
"PSF memory was discovered corrupted when ending a session."
E_PS0701_SRV_ALREADY_UP:NO_GE
"The parser facility was already initialized for the server when SCF asked to
initialize it again."
E_PS0702_NO_MEM:E_GE8D04_NO_RESOURCE
"Error allocating the PSF memory pool."
E_PS0703_RDF_ERROR:NO_GE
"Error occurred while calling RDF."
E_PS0704_INSF_MEM:NO_GE
"Insufficient memory allocated per parser session."
E_PS0901_REDESC_RNG:NO_GE
"An attempt was made to re-describe a range variable."
E_PS0902_BAD_DESCTYPE:NO_GE
"An illegal description request was specified in a range table operation."
E_PS0903_TAB_NOTFOUND:E_GE7594_TABLE_NOT_FOUND
"Requested table '%0c' was not found in the database."
E_PS0904_BAD_RDF_GETDESC:NO_GE
"RDF error occurred when getting description for an object."
E_PS0905_DBP_NOTFOUND:E_GE75B2_NOT_FOUND
"Requested procedure '%0c' was not found in the database."
E_PS0906_TOO_MANY_COLS:E_GE8CA0_SYSTEM_LIMIT
"Result table has too many columns."
E_PS0907_CANT_FREE_DESC:NO_GE
"pstclrrng.c: Can't free a table description."
E_PS0908_RANGE_VAR_MISSING:NO_GE
"line %0d, Can't find range variable number %1d."
E_PS0909_NO_RANGE_ENTRY:NO_GE
"CREATE VIEW: Could not find range table entry for attribute associated
\nwith resdom '%0c'."
E_PS090A_NO_ATTR_ENTRY:NO_GE
"CREATE VIEW: Could not find attribute description for attribute associated
\nwith resdom '%0c'."
E_PS090B_TABLE_EXISTS:E_GE75F8_DEF_RESOURCE
"Table '%0c' already exists in the database."
E_PS090C_NONEXISTENT_TABLE:E_GE7594_TABLE_NOT_FOUND
"Table '%0c' does not exist in the database."
E_PS090D_NO_LDB_TABLE:E_GE7594_TABLE_NOT_FOUND
"LDB table '%0c' does not exist
\n on node '%1c', database '%2c', dbms '%3c'."
E_PS090E_COL_MISMATCH:E_GE7D00_QUERY_ERROR
"Number of columns in LDB table is different from that specified for the LINK\n
object."
E_PS090F_BAD_COL_NAME:E_GE7D00_QUERY_ERROR
"LDB table column name '%0c' is not a valid INGRES name and can not be used as\n
a LINK object column name."
E_PS0910_DUP_LDB_TABLE:E_GE75F8_DEF_RESOURCE
"LDB table '%0c' owned by you already exists on NODE '%1c', DATABASE '%2c'."
E_PS0911_BAD_LDB_USER:E_GEA028_USER_ID_INV
"You are not an authorized user on NODE '%0c', DATABASE '%1c'."
E_PS0912_MISSING_LDB_TBL:E_GE7594_TABLE_NOT_FOUND
"There is no LDB table corresponding to STAR object '%0c'."
E_PS0913_SCHEMA_MISMATCH:NO_GE
"Structure of LDB table corresponding to STAR object '%0c'
\nhas been changed since the time '%0c' was created."
W_PS0914_MISSING_LDB_TBL:E_GE0032_WARNING
"DROP/DESTROY: LDB table has not been dropped.\n
There is no LDB table corresponding to STAR object '%0c'."
E_PS0915_MISSING_LDB_TBL:E_GE7594_TABLE_NOT_FOUND
"There is no LDB table corresponding to STAR object
\nwith base id '%0d', index id '%1d'."
/*
** E_PS0915_MISSING_LDB_TBL:
**
** Parameters:
**	base id of a STAR object
**	index id of a STAR object
**
** Description:
**	This message will be produced instead of PS0912 if a failure occurred
**	while trying to obtain table description using its id.
*/
E_PS0916_USER_NOT_OWNER:E_GE7D00_QUERY_ERROR
"DROP/DESTROY: You do not own LDB object corresponding
\nto STAR object '%0c'.
\nYou may use REMOVE to remove the registration."
E_PS0917_CANNOT_CONNECT:NO_GE
"Can not connect
\nto node '%0c', database '%1c', dbms '%2c'."
E_PS0918_MULTI_SITE_DROP:E_GE7D00_QUERY_ERROR
"Table '%0c' was not dropped.
\n    Dropping it would violate single-site update requirement."
E_PS0919_BAD_CATALOG_ENTRY:NO_GE
"Catalog entry for DDB object '%0c' is inconsistent.
\n Try 'REGISTER %0c AS LINK WITH REFRESH' or
\n REMOVE and then REGISTER %0c."
E_PS091A_CANNOT_CONNECT:E_GE7D00_QUERY_ERROR
"DROP/DESTROY: Cannot establish connection with the site
\nof the LDB table corresponding to STAR object '%0c'.
\nYou may use REMOVE to remove the registration."
E_PS091B_NO_CONNECTION:E_GE75BC_UNKNOWN_OBJECT
"LDB is temporarily inaccessible or it does not exist."
E_PS0A01_BADBLKSIZE:NO_GE
"psf_mopen: bad block size"
E_PS0A02_BADALLOC:E_GE8D04_NO_RESOURCE
"psf memory function couldn't allocate memory"
E_PS0A03_BADMEMTYPE:NO_GE
"psf_mopen: unknown memory type"
E_PS0A04_REQ_GT_BLOCK:NO_GE
"psf_malloc: request for memory greater than block size for stream"
E_PS0A05_BADMEMREQ:NO_GE
"PSF: bad memory request"
E_PS0A06_CANTFREE:NO_GE
"psf_mclose: can't free memory"
E_PS0A07_CANTCHANGE:NO_GE
"Can't change object type in QSF memory."
E_PS0A08_CANTLOCK:NO_GE
"Can't lock QSF object."
E_PS0A09_CANTDESTROY:NO_GE
"Can't destroy QSF object."
E_PS0A0A_CANTGETINFO:NO_GE
"Can't get info about a QSF object."
E_PS0A0B_CANTGETHNDLE:NO_GE
"Can't obtain QSF object handle."
E_PS0A0C_CANT_CREATE_ALIAS:NO_GE
"Can't create an alias for a QSF QP object."
E_PS0B04_CANT_GET_TEXT:NO_GE
"Failed attempt to get the query text from SCF."
E_PS0B05_CANT_UNLOCK:NO_GE
"Can't unlock the QSF object."
E_PS0B06_NOWHILE:E_GE7D00_QUERY_ERROR
"Internal PSF error: corresponding while node not found."
E_PS0BA0_SHORT_REMARK_TOO_LONG:E_GE7D00_QUERY_ERROR
"COMMENT: length of short remark may not exceed %0d characters."
/*
** Parameters:
**	%0d -	Maximum length of short remark in COMMENT statement.
** Explanation:
**	Length of the text of short remark exceeds maximum legal length.
** System Status:
**	Current statement is aborted.
** User Action:
**	Reenter query with short remark of legitimate length.
*/
E_PS0BA1_LONG_REMARK_TOO_LONG:E_GE7D00_QUERY_ERROR
"COMMENT: length of long remark may not exceed %0d characters."
/*
** Parameters:
**	%0d -	Maximum length of long remark in COMMENT statement.
** Explanation:
**	Length of the text of long remark exceeds maximum legal length.
** System Status:
**	Current statement is aborted.
** User Action:
**	Reenter query with long remark of legitimate length.
*/
E_PS0BA2_NO_REMARK_SPECIFIED:E_GE7D00_QUERY_ERROR
"COMMENT: Long remark, short remark, or both must be specified."
/*
** Parameters:
**	none
** Explanation:
**	COMMENT query specified neither long nor short remark.
** System Status:
**	Current statement is aborted.
** User Action:
**	Reenter query with long remark, short remark, or both.
*/
E_PS0BA3_NONEXISTENT_COLUMN:E_GE759E_COLUMN_UNKNOWN
"COMMENT ON COLUMN: Table '%0c' does not contain column '%1c'."
/*
** Parameters:
**	%0c -	name of table as specified by the user
**	%1c -	name of column as specified by the user
** Explanation:
**	User attempted to create a comment on a non-existent column.
** System Status:
**	Current statement is aborted.
** User Action:
**	Reenter query with name of existent column.
*/
E_PS0BA4_TABLE_COMMENT_ERROR:E_GE98BC_OTHER_ERROR
"COMMENT ON TABLE: Failed to create/modify comment on table '%0c'.
\n Please contact system administrator."
/*
** Parameters:
**	%0c -	name of table as specified by the user.
** Explanation:
**	COMMENT query failed due to some internal error.
** System Status:
**	Current statement is aborted.
** User Action:
**	Contact system administrator.
*/
E_PS0BA5_COLUMN_COMMENT_ERROR:E_GE98BC_OTHER_ERROR
"COMMENT ON COLUMN: Failed to create/modify comment
\non column '%0c' of table '%1c'.
\nPlease contact system administrator."
/*
** Parameters:
**	%0c -	name of column as specified by the user
**	%1c -	name of table as specified by the user.
** Explanation:
**	COMMENT query failed due to some internal error.
** System Status:
**	Current statement is aborted.
** User Action:
**	Contact system administrator.
*/
E_PS0BAF_SHORT_REM_UNSUPPORTED:E_GEA0F0_SQL_STMT_INV
"COMMENT: Use of short remark is not supported in this release."
/*
** Parameters:
**	none
** Explanation:
**	User attempted to specify short remark which is not supported in this
**	release.
** System Status:
**	Current statement is aborted.
** User Action:
**	Reenter query specifying only the long remark.
*/
E_PS0C02_NULL_PTR:NO_GE
"Unexpected NULL pointer in query tree."
E_PS0C03_BAD_NODE_TYPE:NO_GE
"Unknown node type in query tree."
E_PS0C04_BAD_TREE:NO_GE
"Bad query tree."
E_PS0C05_BAD_ADF_STATUS:NO_GE
"Unexpected status from ADF."
E_PS0C06_BAD_STMT:NO_GE
"Invalid PST_STATEMENT node - '%0c' node corrupt or missing."
/*
** Parameters:
**	None
** Explanation:
**	When reconstructing the rule statement tree from internal data
**	structures an invalid relationship between statement nodes was found.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	If this is a Dynamic SQL statement try to re-PREPARE the statement
**	otherwise, if this statement has a rule associated with it, your rule
**	definition may have been corrupted.  Contact your INGRES administrator
**	and request that they save the current "rule row" in iirule together
**	with the associated objects in iitree and iiqrytext.  Recreate the
**	rule and try again.
*/
E_PS0C80_NO_DEFLT_ON_DT:E_GE80E8_LOGICAL_ERROR
"No default values are provided for the decimal() conversion function when a\
 value with datatype `%0c' is given as the first parameter.  Explicitly\
 specify precision and scale."
/*
** Parameters:
**	%0c	    Datatype of value passed to decimal() conversion function
**	
** Explanation:
**	Currently only float, integer, decimal, and money datatypes are
**	assigned default precision and scale values when using the decimal()
**	conversion function.  Receiving this error means that a value with
**	some other datatype was passed as the first parameter of the decimal()
**	function with the precision or scale omitted.
**
** System Status:
**	Query is aborted.
**	
** User Action:
**	Correct usage of decimal() conversion function by either changing the
**	datatype of the first parameter to a type which provides default values
**	for precision and scale, or explicitly provide precision and scale
**	values in your decimal() function call.
*/
E_PS0C81_MUST_BE_CONST:E_GEA0F0_SQL_STMT_INV
"The second and third arguments to the decimal() conversion function must be\
 constants.  Expressions, column names, host variables, etc. are not allowed."
/*
** Parameters:
**	None
**	
** Explanation:
**	The second and third arguments to the decimal() conversion function
**	MUST be constants.  You are not allowed to use a column name from a
**	table, a host variable (regardless of its storage class in the host
**	language), repeat query parameters, or expressions of any kind.  For
**	example, decimal(123, 10, 2) is legal while decimal(123, 8+2, 2) is
**	illegal.
**
** System Status:
**	Query is aborted.
**	
** User Action:
**	Correct usage of decimal() conversion function by using only smallint
**	constants for the precision and scale arguments to the function.
*/
E_PS0C82_BAD_PRECISION:E_GE8CA0_SYSTEM_LIMIT
"Precision specified in decimal() conversion function was invalid.  Precision\
 must be a smallint literal constant between 1 and 31, inclusive."
/*
** Parameters:
**	None
**	
** Explanation:
**	The precision specified as the second parameter to the decimal()
**	conversion function was either not a smallint literal, or was outside
**	the legal range (1 <= precision <= 31).  You may not specify any type
**	other than smallint; integer, decimal, and float literals are illegal.
**	For example, decimal(123, 10, 2) is legal, but decimal(123, 10.0, 2)
**	is illegal.
**
** System Status:
**	Query is aborted.
**	
** User Action:
**	Change usage of decimal() conversion function to conform to rules
**	stated above.
*/
E_PS0C83_BAD_SCALE:E_GE8CA0_SYSTEM_LIMIT
"Scale specified in decimal() conversion function was invalid.  Scale\
 must be a smallint literal constant between 0 and `precision' (in this case\
 %0d), inclusive."
/*
** Parameters:
**	%0d	    precision given in this decimal() function invocation
**	
** Explanation:
**	The scale specified as the third parameter to the decimal()
**	conversion function was either not a smallint literal, or was outside
**	the legal range (1 <= scale <= precision).  You may not specify any type
**	other than smallint; integer, decimal, and float literals are illegal.
**	For example, decimal(123, 10, 2) is legal, but decimal(123, 10, 2.0)
**	is illegal.
**
** System Status:
**	Query is aborted.
**	
** User Action:
**	Change usage of decimal() conversion function to conform to rules
**	stated above.
*/
E_PS0C84_TERNARY_FUNC:E_GE7918_SYNTAX_ERROR
"Only decimal() conversion function may be passed 3 arguments."
/*
** Parameters:
**	None
**
** Explanation:
**	User passed 3 arguments to a function other than decimal().
**
** System Status:
**	Query is aborted.
**	
** User Action:
**	Either change function name to decimal or get rid of an extra parameter.
*/
E_PS0D01_BAD_TREE_IN_VFIND:NO_GE
"A bad query tree was detected in psy_vfind."
E_PS0D02_BAD_TID_NODE:NO_GE
"An illegal TID node was discovered."
E_PS0D03_ATT_NOT_FOUND:NO_GE
"The view attribute was not found in the view tree."
E_PS0D05_NULL_TREE:NO_GE
"A NULL tree was found in psy_apql."
E_PS0D06_QUAL_NOT_NORML:NO_GE
"An un-normalized qualification was discovered."
E_PS0D07_VARNO_OUT_OF_RANGE:NO_GE
"An out of sequence variable number was found."
E_PS0D08_RNGVAR_UNUSED:NO_GE
"An attempt was made to use an undefined range variable."
E_PS0D09_MRG_DIFF_TABS:NO_GE
"An attempt was made to merge two different tables."
E_PS0D0A_CANT_GET_EMPTY:NO_GE
"Failed call to get an empty datatype from ADF."
E_PS0D0B_RESDOM_EXPECTED:NO_GE
"A non-resdom node was found where a resdom node was expected."
E_PS0D0C_NOT_RESDOM:NO_GE
"A non-resdom node was found."
E_PS0D0D_NULL_LHS:NO_GE
"A NULL target list was found where one was not expected."
E_PS0D0E_BAD_QMODE:NO_GE
"An illegal query mode was discovered."
E_PS0D0F_BAD_AGGREGATE:NO_GE
"An illegal node was discovered in an aggregate tree fragment."
E_PS0D10_BAD_QUALIFICATION:NO_GE
"An illegal node was discovered in the qualification."
E_PS0D11_NULL_TREE:NO_GE
"An unexpected NULL query tree was found."
E_PS0D12_BYHEAD_OR_AOP:NO_GE
"An unexpected AGHEAD or AOP node was found in query tree."
E_PS0D13_SCU_INFO_ERR:NO_GE
"Failed attempt to get information from SCU."
E_PS0D14_VIEW_CLEANUP:NO_GE
"An error occurred while cleaning up a view definition."
E_PS0D15_RDF_GETINFO:NO_GE
"RDF error occurred when getting information on an object."
E_PS0D16_RDF_UPDATE:NO_GE
"Error updating object in RDF."
E_PS0D17_RDF_UNFIX:NO_GE
"Error unfixing object in RDF."
E_PS0D18_QSF_LOCK:NO_GE
"Error getting QSF lock on object."
E_PS0D19_QSF_INFO:NO_GE
"Error getting QSF information."
E_PS0D1A_QSF_DESTROY:NO_GE
"Error destroying QSF object."
E_PS0D1B_PERMIT_CLEANUP:NO_GE
"An error occurred cleaning up permit definition."
E_PS0D1C_QSF_UNLOCK:NO_GE
"Error unlocking QSF object."
E_PS0D1D_BAD_FUNC_NAME:E_GE75BC_UNKNOWN_OBJECT
"An illegal aggregate function name was specified."
E_PS0D1E_QEF_ERROR:NO_GE
"An error occurred calling QEF while defining integrity."
E_PS0D1F_OPF_ERROR:NO_GE
"An error occurred calling OPF while defining integrity."
E_PS0D20_QEF_ERROR:NO_GE
"An error occurred calling QEF while destroying view or table."
E_PS0D21_QEF_ERROR:E_GEC2EC_SERIALIZATION
"Access conflict occurred while dropping table - your previous activity
resulted in acquiring a lock that is still being held."
E_PS0D22_QEF_ERROR:NO_GE
"Internal transaction error occurred while executing QRYMOD."
E_PS0D23_QEF_ERROR:E_GE8D04_NO_RESOURCE
"Resource related error occurred while executing QRYMOD. Try again later,
if problem persists consult your system administrator."
E_PS0D24_BGN_TX_ERR:NO_GE
"Error beginning transaction during QRYMOD operation."
E_PS0D25_CMT_TX_ERR:NO_GE
"Error committing transaction during QRYMOD operation."
E_PS0D26_ABT_TX_ERR:NO_GE
"Error aborting transaction during QRYMOD operation."
E_PS0D27_RDF_INITIALIZE:NO_GE
"Error occurred while initializing RDF facility."
E_PS0D28_RDF_TERMINATE:NO_GE
"Error occurred while terminating RDF facility."
E_PS0D29_RDF_INVALIDATE:NO_GE
"Error occurred while invalidating a descriptor in the RDF cache."
E_PS0D30_UNEXPECTED_NODE:NO_GE
"Unexpected node found by protection checking routine."
E_PS0D31_NO_TUPLES:NO_GE
"No tree tuples retrieved for a QUERYMOD object due to concurrent
activity. Try again later."
E_PS0D32_RDF_INV_NODE:NO_GE
"The tree returned from RDF has a node with an invalid pst_len value."
E_PS0D33_RULE_PARAM:E_GE7D00_QUERY_ERROR
"Procedure argument counter/argument list mismatch in rule tuple/tree."
/*
** Parameters:
**	None
** Explanation:
**	When reconstructing the rule statement tree from internal RDF tuple
**	and tree definitions an inconsistency was found between the number
**	of parameters in the master rule tuple and the first node of the
**	parameter list tree.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Contact your INGRES administrator and request that they save the
**	current "rule row" in iirule together with the associated objects
**	in iitree and iiqrytext.  Recreate the rule and try again.
*/
E_PS0D34_EVENT_ABSENT_PRIV:E_GE75B2_NOT_FOUND
"%0c EVENT: Event '%1c' does not exist or you do not have permission
to execute this statement."
/*%
** Parameters:
**	%0 - The REGISTER, REMOVE or RAISE EVENT statement.
**	%1 - Name of event that is being registered or raised.  This name is
**	     the same name as the event specified in the statement.
** Explanation:
**	The named event does not exist or the current user does not have
**	permission to execute the statement.  This could result from the event
**	being dropped (explicitly or by a transaction rollback), from
**	insufficient privileges, or from the revoking of previous privileges.
**	Note that when you REGISTER or RAISE from within a database procedure
**	you are assigned the privileges of the owner of the procedure.
** System Status:
**	The current REGISTER, REMOVE or RAISE statement is aborted.  If this
**	statement is issued from within a database procedure then the database
**	procedure compilation is aborted.
** User Action:
**	Create the event or, if it is already created, request that the 
**	correct execution privileges be granted to you.
*/
E_PS0D35_EVENT_GRANT_REVOKE:E_GE7918_SYNTAX_ERROR
"GRANT/REVOKE: Only REGISTER and RAISE privileges may be associated with
an EVENT.  These privileges may not be associated with any other objects."
/*%
** Parameters:
**	None
** Explanation:
**	The only privileges that may be granted on or revoked from an event
**	are REGISTER and RAISE.  Also, those privileges may only apply to
**	events and not to tables, procedures, databases or other objects.
** System Status:
**	The current GRANT or REVOKE statement is aborted.
** User Action:
**	Correct the statement to fit the syntax.
*/
E_PS0D36_EVENT_TEXT_QUOTE:E_GE7918_SYNTAX_ERROR
"RAISE EVENT text identified by unquoted name '%0c' outside of database
procedure.  The text value must be a quoted string or database procedure 
variable."
/*%
** Parameters:
**	%0 - Name used instead of text.
** Explanation:
**	Outside of a database procedure you may only use a quoted string.
** System Status:
**	The current statement is aborted.
** User Action:
**	Correct the statement syntax.
*/
E_PS0D37_EVENT_TEXT_TYPE:E_GE7918_SYNTAX_ERROR
"RAISE EVENT text parameter '%0c' must be of a non-nullable character type."
/*%
** Parameters:
**	%0 - Procedure variable used for text.
** Explanation:
**	The text field for RAISE EVENT must be a non-nullable character
**	or varying-length character value.
** System Status:
**	The current statement is aborted.
** User Action:
**	Reissue your RAISE EVENT statement with the correct data type.
*/
E_PS0D38_EVENT_WITH:E_GE7918_SYNTAX_ERROR
"RAISE EVENT: Invalid word '%0c' in WITH clause of statement.  Only [NO]SHARE
is allowed."
/*%
** Parameters:
**	%0 - Invalid word
** Explanation:
**	The WITH clause syntax is incorrect.
** System Status:
**	The current statement is aborted.
** User Action:
**	Correct the syntax.
*/
E_PS0D39_EVENT_GET:NO_GE
"Invalid request to retrieve event data.  The PSF routine '%0c' has
been called with statement mode (psq_mode) %1d."
/*%
** Parameters:
**	%0 - Internal name of parser routine.
**	%1 - Invalid query mode that can be mapped using internal source.
** Explanation:
**	A call has been made to retrieve an event tuple in the wrong context.
** System Status:
**	The current statement is aborted.
** User Action:
**	Try the statement again and submit an error report.
*/
E_PS0D3A_ILLEGAL_EVENT_REF:E_GE84D0_NO_PRIVILEGE
"%0c: You may not apply this query to
\n event '%1c' owned by '%2c'."
/*
** Parameters:
**	- offending query
**	- name of the event
**	- name of the event owner as specified by the user
** Explanation:
**	User attempted to execute a query explicitly referencing an event
**	which may not be referenced by this user.
** System Status:
**	Program continues; current statement is aborted.
** User Action:
**	Correct the error and reenter
*/
E_PS0D40_INVALID_GRPID_OP:NO_GE
"Invalid PSF operation for iiusergroup encountered."
/*%
** Parameters: (none)
**
** Explanation:
**	psy_group() encountered an invalid operation in psy_grpflag
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D41_INVALID_APLID_OP:NO_GE
"Invalid PSF operation for iirole encountered."
/*%
** Parameters: (none)
**
** Explanation:
**	psy_aplid() encountered an invalid operation in psy_aplflag
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D42_INVALID_GRANTEE_TYPE:NO_GE
"Invalid grantee type encountered in iiprotect tuple"
/*%
** Parameters: (none)
**
** Explanation:
**	psy_permit found an invalid grantee type in dbp_gtype
**	of an iiprotect catalog record.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**	Delete the permit associated with this error.
**	Valid grantee types are 0 (user), 1 (group), and 2 (aplid).
*/
E_PS0D43_XENCODE_ERROR:NO_GE
"An error was encountered during encryption"
/*%
** Parameters: (none)
**
** Explanation:
**	psy_[ca]aplid() got an error back from scu_xencode()
**	when attempting to encrypt an role password.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D44_SCU_IDEF_ERR:NO_GE
"Failed attempt to store information into SCU."
/*%
** Parameters: (none)
**
** Explanation:
**	scu_idefine returned an error
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D45_INVALID_GRANT_OP:NO_GE
"Invalid grant operation encountered"
/*%
** Parameters: (none)
**
** Explanation:
**	psy_dbpriv detected an invalid operation in psy_grant.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D46_INVALID_USER_OP:NO_GE
"Invalid user operation encountered"
/*%
** Parameters: (none)
**
** Explanation:
**	psy_user detected an invalid operation in psy_usflag.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D47_INVALID_LOC_OP:NO_GE
"Invalid location operation encountered"
/*%
** Parameters: (none)
**
** Explanation:
**	psy_loc detected an invalid operation in psy_loflag.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D48_INVALID_AUDIT_OP:NO_GE
"Invalid audit operation encountered"
/*%
** Parameters: (none)
**
** Explanation:
**	psy_audit detected an invalid operation in psy_auflag.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0D49_INVALID_ALARM_OP:NO_GE
"Invalid alarm operation encountered"
/*%
** Parameters: (none)
**
** Explanation:
**	psy_alarm detected an invalid operation in psy_alflag.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_PS0E01_STMT_NFND:E_GE75A8_CURSOR_UNKNOWN
"Dynamic SQL statement was not found."
E_PS0E02_NOEXEC:E_GE7D00_QUERY_ERROR
"The select statement is not allowed to executed."
E_PS0E03_RGENT_ERR:NO_GE
"Error %0d occurred when reentering table info into the range table."
E_PS0F01_CACHE_FULL:E_GE8D04_NO_RESOURCE
"The descriptor cache is currently full. Try again later."
E_PS0F02_MEMORY_FULL:E_GE8D04_NO_RESOURCE
"There is no more available memory. Try again later."
E_PS0F03_TRANSACTION_ABORTED:NO_GE
"Transaction had to be aborted. Try again later."
E_PS0F04_RDF_RESOURCE_ERR:E_GE8D04_NO_RESOURCE
"Resource error occurred while retrieving a table description.
Try again or consult your system administrator."
E_PS0F05_BAD_MEM_SIZE:NO_GE
"Possibly bad memory size: %0d."
E_PS1001_ERR_SCU_INFO:NO_GE
"Error occurred on an SCU INFORMATION call."

/*
**    Message file for Facility code: _QE
*/


#define _QE_CLASS   6


E_QE0000_OK:E_GE0000_OK
"QEF successful return."
E_QE0001_INVALID_ALTER:E_GE7918_SYNTAX_ERROR
"Invalid parameter in alter QEF setting array."
E_QE0002_INTERNAL_ERROR:E_GE9858_FATAL_ERROR
"A QEF internal error occurred."
E_QE0003_INVALID_SPECIFICATION:E_GE7918_SYNTAX_ERROR
"An out-of-range parameter value was specified."
E_QE0004_NO_TRANSACTION:E_GE9E34_TRAN_STATE_INV
"No transaction is currently in progress."
E_QE0005_CANNOT_ESCALATE:E_GE9E34_TRAN_STATE_INV
"Cannot convert the internal transaction to an SST or MST."
E_QE0006_TRANSACTION_EXISTS:E_GE9E34_TRAN_STATE_INV
"A transaction is currently in progress."
E_QE0007_NO_CURSOR:E_GE9DD0_CUR_STATE_INV
"Single table cursor is not open."
E_QE0008_CURSOR_NOT_OPENED:E_GE9DD0_CUR_STATE_INV
"The cursor specified has not been opened."
E_QE0009_NO_PERMISSION:E_GE84D0_NO_PRIVILEGE
"The specified cursor does not support the requested action."
E_QE000A_READ_ONLY:E_GE9DD0_CUR_STATE_INV
"The cursor was opened read-only."
E_QE000B_ADF_INTERNAL_ERROR:NO_GE
"An internal error occurred while executing an ADF function."
E_QE000C_CURSOR_ALREADY_OPENED:E_GE9DD0_CUR_STATE_INV
"The cursor is already opened."
E_QE000D_NO_MEMORY_LEFT:E_GE8D04_NO_RESOURCE
"QEF is out of memory."
E_QE000E_ACTIVE_COUNT_EXCEEDED:E_GE8D04_NO_RESOURCE
"There are too many active query plans in QEF."
E_QE000F_OUT_OF_OTHER_RESOURCES:E_GE8D04_NO_RESOURCE
"QEF is out of resources."
E_QE0010_DUPLICATE_ROW:E_GE75F8_DEF_RESOURCE
"This update duplicates a row in the table."
E_QE0011_AMBIGUOUS_REPLACE:E_GE80E8_LOGICAL_ERROR
"Attempted to update the same row twice."
E_QE0012_DUPLICATE_KEY:E_GE75F8_DEF_RESOURCE
"This update duplicates a key in the table."
E_QE0013_INTEGRITY_FAILED:E_GE9D6C_CONSTR_VIO
"Update failed because an integrity constraint was violated."
E_QE0014_NO_QUERY_PLAN:E_GE75BC_UNKNOWN_OBJECT
"The specified query plan is unknown to QEF."
E_QE0015_NO_MORE_ROWS:E_GE0064_NO_MORE_DATA
"No more rows are available."
E_QE0016_INVALID_SAVEPOINT:E_GE7918_SYNTAX_ERROR
"Nonexistent savepoint."
E_QE0017_BAD_CB:NO_GE
"An invalid control block was passed to QEF."
E_QE0018_BAD_PARAM_IN_CB:NO_GE
"Illegal parameter in control block."
E_QE0019_NON_INTERNAL_FAILURE:NO_GE
"An error occurred in a routine called by QEF."
E_QE001A_DATATYPE_OVERFLOW:NO_GE
"Datatype overflow."
E_QE001B_COPY_IN_PROGRESS:E_GE75BC_UNKNOWN_OBJECT
"A copy command is currently in progress."
E_QE001C_NO_COPY:NO_GE
"An error in the copy command occurred."
E_QE001D_FAIL:NO_GE
"General failure in QEF (%0x)."
E_QE001E_NO_MEM:E_GE8D04_NO_RESOURCE
"Unable to allocate memory from SCF."
E_QE001F_INVALID_REQUEST:E_GE80E8_LOGICAL_ERROR
"An unsupported function was requested."
E_QE0020_OPEN_CURSOR:E_GE9DD0_CUR_STATE_INV
"A cursor is currently open."
E_QE0021_NO_ROW:E_GE9DD0_CUR_STATE_INV
"The cursor is not currently positioned on a row."
E_QE0022_QUERY_ABORTED:E_GEC2EC_SERIALIZATION         
"The query has been aborted."
E_QE0023_INVALID_QUERY:NO_GE
"This query plan is no longer valid."
E_QE0024_TRANSACTION_ABORTED:E_GE8D04_NO_RESOURCE
"The transaction log file is full.  The transaction will be aborted."
E_QE0025_USER_ERROR:NO_GE
"A user error has occurred."
E_QE0026_DEFERRED_EXISTS:E_GE9DD0_CUR_STATE_INV
"Only one deferred cursor can be open at a time."
E_QE0027_BAD_QUERY_ID:E_GE9E98_INV_SQL_STMT_ID
"Query identifier is not valid."
E_QE0028_BAD_TABLE_NAME:E_GE797C_INVALID_IDENT
"Table name is not valid."
E_QE0029_BAD_OWNER_NAME:E_GEA028_USER_ID_INV
"Owner name is not valid."
E_QE002A_DEADLOCK:E_GEC2EC_SERIALIZATION
"Deadlock detected."
E_QE002B_TABLE_EXISTS:E_GE75F8_DEF_RESOURCE
"The operation specified tried to create a table that already exists."
E_QE002C_BAD_ATTR_TYPE:E_GE7918_SYNTAX_ERROR
"Invalid attribute type specified."
E_QE002D_BAD_ATTR_NAME:E_GE797C_INVALID_IDENT
"A bad attribute name was specified."
E_QE002E_BAD_ATTR_SIZE:E_GE7918_SYNTAX_ERROR
"Invalid size for specified attribute type."
E_QE002F_BAD_ATTR_PRECISION:E_GE7918_SYNTAX_ERROR
"Invalid precision for specified attribute type."
E_QE0030_BAD_LOCATION_NAME:E_GE75BC_UNKNOWN_OBJECT
"The specified location name is invalid."
E_QE0031_NONEXISTENT_TABLE:E_GE7594_TABLE_NOT_FOUND
"The operation required the use of a table that does not exist."
E_QE0032_BAD_SAVEPOINT_NAME:E_GE797C_INVALID_IDENT
"An invalid savepoint name was specified."
E_QE0033_BAD_TID:E_GE7918_SYNTAX_ERROR
"An invalid TID was specified."
E_QE0034_LOCK_QUOTA_EXCEEDED:E_GE8D04_NO_RESOURCE
"The locking mechanism is out of locks."
E_QE0035_LOCK_RESOURCE_BUSY:E_GE8D04_NO_RESOURCE
"The requested lock is not available."
E_QE0036_LOCK_TIMER_EXPIRED:E_GE8D04_NO_RESOURCE
"The requested lock could not be granted in during the time allotted."
E_QE0037_ERROR_DELETING_RECORD:NO_GE
"An error occurred while deleting a record."
E_QE0038_DUPLICATE_RECORD:E_GE7602_INS_DUP_ROW
"Duplicate records were found."
E_QE0039_DUPLICATE_KEY:E_GE7602_INS_DUP_ROW
"Duplicate primary keys were found."
E_QE003A_FOPEN_FROM:E_GE75BC_UNKNOWN_OBJECT
"An error occurred opening the file in a COPY FROM command."
E_QE003B_FOPEN_TO:E_GE75BC_UNKNOWN_OBJECT
"An error occurred opening the file in a COPY INTO command."
E_QE003C_COLUMN_TOO_SMALL:E_GE7918_SYNTAX_ERROR
"A column is too small for data conversion in the COPY command."
E_QE003D_BAD_INPUT_STRING:E_GE7918_SYNTAX_ERROR
"The input string is not valid for the COPY command."
E_QE003E_UNEXPECTED_EOF:E_GE80E8_LOGICAL_ERROR
"An unexpected end of file was encountered while reading the input file."
E_QE003F_UNTERMINATED_C0:E_GE80E8_LOGICAL_ERROR
"An unterminated c0 field was encountered."
E_QE0040_NO_FULL_PATH:E_GE7918_SYNTAX_ERROR
"The full path of the file must be specified."
E_QE0041_DUPLICATES_IGNORED:E_GE0032_WARNING
"Duplicate tuples were encountered and ignored."
E_QE0042_CONTROL_TO_BLANK:E_GE0032_WARNING
"Control characters were converted to blanks."
E_QE0043_TRUNC_C0:E_GE80E8_LOGICAL_ERROR
"A c0 format field was truncated."
E_QE0044_ILLEGAL_FTYPE:E_GE7918_SYNTAX_ERROR
"An illegal file type was specified."
E_QE0045_BINARY_F_T0:E_GE7918_SYNTAX_ERROR
"The use of the Text(0) format requires the use of binary files."
E_QE0046_ERROR_ON_ROW:NO_GE
"An error occurred processing row."
E_QE0047_INVALID_CHAR:E_GE80E8_LOGICAL_ERROR
"Invalid character value found while processing row."
E_QE0048_TUP_TOO_WIDE:E_GE80E8_LOGICAL_ERROR
"Tuple is too wide for query processing."
E_QE0049_ALL_KEY:E_GE7918_SYNTAX_ERROR
"All keys must be specified for this MODIFY operation."
E_QE004A_KEY_SEQ:E_GE7918_SYNTAX_ERROR
"An illegal key sequence was specified."
E_QE004B_NOT_ALL_ROWS:NO_GE
"Could not SELECT all rows from the subselect."
E_QE004C_NOT_ZEROONE_ROWS:E_GE9CA4_CARDINALITY
"Subselect returned more that one row."
E_QE004D_DUPLICATE_KEY_UPDATE:E_GE7602_INS_DUP_ROW
"The requested update was not performed because it would duplicate a
key existing in the table."
E_QE004E_ULH_INIT_FAILURE:NO_GE
"An error occurred while initializing ULH for the DSH environment."
E_QE0050_TEMP_TABLE_EXISTS:NO_GE
"An error occurred trying to create a temporary table."
E_QE0051_TABLE_ACCESS_CONFLICT:E_GE80E8_LOGICAL_ERROR
"Table access conflict, table opened twice."
E_QE0052_RESOURCE_QUOTA_EXCEEDED:E_GE8D04_NO_RESOURCE
"Out of disk space, disk quota, or open file quota."
E_QE0053_TRAN_ID_NOT_UNIQUE:NO_GE
"The distributed transaction ID is not unique in the local system."
E_QE0054_DIS_TRAN_UNKNOWN:NO_GE
"The distributed transaction ID is unknown to the local system."
E_QE0055_DIS_TRAN_OWNER:NO_GE
"The distributed transaction is owned by another server in the local system.\n
Disconnect the previous association and try to re-connect later."
E_QE0056_CANT_SECURE_IN_CLUSTER:NO_GE
"Can't support two-phase commit protocol in a cluster installation."
E_QE0057_ERROR_RESUME_TRAN:NO_GE
"Error in resuming a transaction."
E_QE0058_ERROR_SECURE_TRAN:NO_GE
"Error in securing a transaction."
E_QE0059_ILLEGAL_STMT:NO_GE
"Illegal statement specified. Only ROLLBACK or COMMIT statement is allowed \n
immediately after the PREPARE TO COMMIT statement."
E_QE005A_DIS_DB_UNKNOWN:NO_GE
"The database connected by the distributed transaction is unknown to the local system."
E_QE005F_BAD_DB_ACCESS_MODE:E_GE7918_SYNTAX_ERROR
"Bad database access mode."
E_QE0060_BAD_DB_ID:E_GE797C_INVALID_IDENT
"Bad database identifier."
E_QE0061_BAD_DB_NAME:E_GE797C_INVALID_IDENT
"Bad database name."
E_QE0062_BAD_INDEX:NO_GE
"Corrupt index table found."
E_QE0063_BAD_TABLE_OWNER:E_GE797C_INVALID_IDENT
"Bad table owner."
E_QE0064_BAD_TRAN_ID:E_GE797C_INVALID_IDENT
"Bad transaction identifier."
E_QE0065_DB_ACCESS_CONFLICT:E_GE75B2_NOT_FOUND
"Database access conflict."
E_QE0066_DB_OPEN:NO_GE
"Database is already open."
E_QE0067_DB_OPEN_QUOTA_EXCEEDED:E_GE8D04_NO_RESOURCE
"Open database quota exceeded."
E_QE0068_DB_QUOTA_EXCEEDED:E_GE8D04_NO_RESOURCE
"Server database quota exceeded."
E_QE0069_DELETED_TID:NO_GE
"Tuple at tid already deleted."
E_QE006A_NONEXISTENT_DB:E_GE75B2_NOT_FOUND
"Nonexistent database."
E_QE006B_SESSION_QUOTA_EXCEEDED:E_GE8D04_NO_RESOURCE
"Server quota on sessions exceeded."
E_QE006C_CANT_MOD_CORE_SYSCAT:E_GE84D0_NO_PRIVILEGE
"User does not have enough privilege to modify the core system catalogs:\n
	iirelation, iiattribute, iiindexes, ii_relidx, iidevices."
E_QE006D_CANT_INDEX_CORE_SYSCAT:E_GE84D0_NO_PRIVILEGE
"User cannot place an index on the core system catalogs:\n
	iirelation, iiattribute, iiindexes, ii_relidx, iidevices."
E_QE006E_TRAN_IN_PROGRESS:E_GE9E34_TRAN_STATE_INV
"Transaction already in progress."
E_QE006F_TRAN_NOT_IN_PROGRESS:E_GE9E34_TRAN_STATE_INV
"Transaction in not in progress."
E_QE0070_TRAN_QUOTA_EXCEEDED:E_GE8D04_NO_RESOURCE
"Session transaction quota exceeded."
E_QE0071_TRAN_TABLE_OPEN:NO_GE
"Table still open at end of transaction."
E_QE0072_LOCATIONS_TOO_MANY:E_GE8D04_NO_RESOURCE
"Number of locations specified exceeds the maximum allowed for this operation."
E_QE0074_BAD_TABLE_CREATE:NO_GE
"Could not create table."
E_QE0075_BAD_KEY_DIRECTION:E_GE7918_SYNTAX_ERROR
"Illegal key direction."
E_QE0076_BTREE_BAD_KEY:E_GE7918_SYNTAX_ERROR
"Invalid key length for btree table."
E_QE0077_LOCATION_EXISTS:E_GE75F8_DEF_RESOURCE
"Location already exists."
E_QE0078_ERROR_ADDING_DB:NO_GE
"Error trying to add database to server."
E_QE0079_ERROR_DELETING_DB:NO_GE
"Error deleting database from server."
E_QE007A_ERROR_OPENING_DB:NO_GE
"Error opening database in server."
E_QE007B_ERROR_CLOSING_DB:NO_GE
"Error closing database in server."
E_QE007C_ERROR_GETTING_RECORD:NO_GE
"Error trying to get a record."
E_QE007D_ERROR_PUTTING_RECORD:NO_GE
"Error trying to put a record."
E_QE007E_ERROR_REPLACING_RECORD:NO_GE
"Error trying to replace a record."
E_QE0080_ERROR_POSITIONING:NO_GE
"Error trying to position a table."
E_QE0081_ERROR_OPENING_TABLE:NO_GE
"Error opening a table."
E_QE0082_ERROR_CLOSING_TABLE:NO_GE
"Error closing a table."
E_QE0083_ERROR_MODIFYING_TABLE:NO_GE
"Error modifying a table."
E_QE0084_ERROR_INDEXING_TABLE:NO_GE
"Error creating an index for a table."
E_QE0085_ERROR_SORTING:NO_GE
"Error sorting data."
E_QE0086_ERROR_BEGINNING_TRAN:NO_GE
"Error beginning a transaction."
E_QE0087_ERROR_COMMITING_TRAN:NO_GE
"Error committing a transaction."
E_QE0088_ERROR_ABORTING_TRAN:NO_GE
"Error aborting a transaction."
E_QE0089_ERROR_SAVEPOINTING:NO_GE
"An error occurred while trying to create a savepoint."
E_QE008A_BAD_TABLE_DESTROY:NO_GE
"Error trying to destroy a table."
E_QE008B_CANT_OPEN_VIEW:NO_GE
"Illegal attempt was made to open a view, base table(s) must be opened."
E_QE008C_LOCK_MANAGER_ERROR:NO_GE
"Error creating a server lock list."
E_QE008D_BAD_SYSCAT_MOD:NO_GE
"Error trying to modify core system catalogs:\n
	 iirelation,iiattribute, iiindexes, ii_relidx, iidevices."
E_QE008E_ERROR_RELOCATING_TABLE:NO_GE
"An error occurred trying to relocate a table."
E_QE008F_ERROR_DUMPING_DATA:NO_GE
"An error occurred trying to truncate the data of a temporary file."
E_QE0090_ERROR_LOADING_DATA:NO_GE
"An error occurred while trying to load data into a file."
E_QE0091_ERROR_ALTERING_TABLE:NO_GE
"An error occurred while alter the characteristics of a table."
E_QE0092_ERROR_SHOWING_TABLE:NO_GE
"An error occurred while showing information about a table."
E_QE0093_ERROR_ALTERING_DB:NO_GE
"An error occurred while altering the characteristics of a database."
E_QE0094_ISAM_BAD_KEY_LENGTH:NO_GE
"Invalid key length for isam table."
E_QE0095_COMP_BAD_KEY_LENGTH:NO_GE
"Invalid key length for compressed table."
E_QE0096_MOD_IDX_UNIQUE:E_GE7602_INS_DUP_ROW
"It is not allowed to modify a secondary index with non-unique keys
to a unique structure."
E_QE0097_NOT_A_VIEW:E_GE7918_SYNTAX_ERROR
"Error trying to retrieve the query id for a relation which is not a view."
E_QE0098_SEM_FAILS:NO_GE
"Error trying to acquire or to release QEF semaphore."
E_QE0099_DB_INCONSISTENT:NO_GE
"Database becomes inconsistent.  The transaction will be aborted"
E_QE009A_DUP_LOCATION_NAME:E_GE75F8_DEF_RESOURCE
"The same location has been specified twice."
E_QE009B_LOCATION_LIST_ERROR:E_GE7918_SYNTAX_ERROR
"The two location lists specified for table relocation have different number
of entries."
E_QE0100_CREATE_DSH_OBJECT:NO_GE
"Error trying to create DSH object in ULH table."
E_QE0101_DESTROY_DSH_OBJECT:NO_GE
"Error trying to destroy DSH object in ULH table."
E_QE0102_CATALOG_DSH_OBJECT:NO_GE
"Error trying to catalog DSH object into a ULH class."
E_QE0103_ACCESS_DSH_OBJECT:NO_GE
"Error trying to access DSH object in ULH table."
E_QE0104_RELEASE_DSH_OBJECT:NO_GE
"Error trying to release DSH object in ULH table."
E_QE0105_CORRUPTED_CB:NO_GE
"Internal call block corrupted in the QEF procedure '%0c'.\n
The internal diagnostics are: %1c."
/*%
**  Parameters:
**	%0  Name of procedure where error was detected.
**	%1  Internal diagnostics to report to your site contact.
**
**  Explanation:
**	This error is an internal system error.  Please report this error
**	within the context in which it was generated.
**
**  System Status:
**	The query is aborted.
**
**  User Action:
**	Report this error.
*/
E_QE0110_NO_ROWS_QUALIFIED:E_GE0064_NO_MORE_DATA
"No rows would satisfied the qualification specification."
E_QE0111_ERROR_SENDING_MESSAGE:NO_GE
"Error trying to send a database procedure message to the user's application."
E_QE0113_PARAM_CONVERSION:E_GE7918_SYNTAX_ERROR
"Error converting value for the database procedure parameter '%0c'.
The formal and actual parameter types referenced by an EXECUTE PROCEDURE statement 
or database rule invocation are not compatible."
E_QE0114_PARAM_MISSING:E_GE7918_SYNTAX_ERROR
"The values for one or more parameters of the database procedure are missing.
Neither default nor null values have been specified for the parameter in an
EXECUTE PROCEDURE statement or database rule invocation."
E_QE0115_TABLES_MODIFIED:E_GE75BC_UNKNOWN_OBJECT
"A table referenced in the currently executing database procedure
was modified in such a way to invalidate the procedure plan.  The table
modification may be due to transaction related statements (COMMIT or 
ROLLBACK), transaction related errors (dead-lock or force-abort) or
transaction context upon initial entry to the procedure (open transaction 
or cursors).  Procedure execution is terminated."
E_QE0116_CANT_ALLOCATE_DBP_ID:NO_GE
"Error allocating a database-wide unique id for the database procedure."
E_QE0117_NONEXISTENT_DBP:E_GE75B2_NOT_FOUND
"Procedure not found when dropping or modifying procedure attributes."
E_QE0118_COMPRESSION_CONFLICT:NO_GE
"Table must be modified to take advantage of new tuple compression routines."
E_QE0123_PARAM_INVALID:E_GE7918_SYNTAX_ERROR
"The database procedure parameter '%0c' is invalid.
The parameter name does not exist in the procedure referenced by an
EXECUTE PROCEDURE statement or database rule invocation."
/*%
**  Parameters:
**	%0  Name of database procedure parameter in error.
**
**  Explanation:
**	This error will occur when executing a procedure via the EXECUTE
**	PROCEDURE statement or via the firing of a rule.  
**
**  System Status:
**	If this is an EXECUTE PROCEDURE statement then the statement is aborted.
**	If this is a rule invocation then the original query triggering the
**	rule is aborted with a fatal error.
**
**  User Action:
**	Correct the parameter name in the EXECUTE PROCEDURE statement, or
**	EXECUTE PROCEDURE clause in the CREATE RULE statement.
*/
E_QE0124_PARAM_MANY:E_GE7918_SYNTAX_ERROR
"A database procedure parameter has been specified more than once
in an EXECUTE PROCEDURE statement or database rule invocation. There are
%0d actual parameter(s) and %1d formal parameter(s)."
/*%
**  Parameters:
**	%0  Number of actual parameters passed to the procedure invocation.
**	%1  Number of formal parameters specified in the procedure definition.
**
**  Explanation:
**	This error will occur when executing a procedure via the EXECUTE
**	PROCEDURE statement or via the firing of a rule.  
**
**  System Status:
**	If this is an EXECUTE PROCEDURE statement then the statement is aborted.
**	If this is a rule invocation then the original query triggering the
**	rule is aborted with a fatal error.
**
**  User Action:
**	Remove the duplicate parameter instance in the EXECUTE PROCEDURE 
**	statement, or EXECUTE PROCEDURE clause in the CREATE RULE statement.
*/
E_QE0130_NONEXISTENT_IPROC:NO_GE
"Error:  Specified internal procedure does not exist."
/*%
** Parameters:
**  None.
** Description:
**  No Internal Procedure by that name exists.
** System Status:
**  The Execute Procedure statement is aborted.
** User Action:
**  The Internal Procedure "User" is a Ingres Corp. supplied FE program.
**  It should not generate Internal Procedure Errors, so a call to Ingres
**  Corp. tech support is merited.
*/
E_QE0131_MISSING_IPROC_PARAM:NO_GE
"Error:  mismatch between expected and actual parameters for Internal Procedure"
/*%
** Parameters:
**  None.
** Description:
**  The internal procedure that QEF knows expects a different number of
**  parameters that it is being supplied.
** System Status:
**  The Execute Procedure statement is aborted.
** User Action:
**  The Internal Procedure "User" is a Ingres Corp. supplied FE program.
**  It should not generate Internal Procedure Errors, so a call to Ingres
**  Corp. tech support is merited.
*/
E_QE0132_WRONG_PARAM_NAME:NO_GE
"Error:  Invalid parameter name supplied to Internal Procedure"
/*%
** Parameters:
**  None.
** Description:
**  The internal procedure that qef knows expects certain parameter names.
**  One or more of the supplied parameter names does not match what we expect.
** System Status:
**  The Execute Procedure statement is aborted.
** User Action:
**  The Internal Procedure "User" is a Ingres Corp. supplied FE program.
**  It should not generate Internal Procedure Errors, so a call to Ingres
**  Corp. tech support is merited.
*/
E_QE0133_WRONG_PARAM_TYPE:NO_GE
"Error:  Invalid parameter supplied to Internal Procedure"
/*%
** Parameters:
**  None.
** Description:
**  The internal procedure that qef knows expects certain parameter names/types.
**  One or more of the supplied parameter types does not match what we expect.
** System Status:
**  The Execute Procedure statement is aborted.
** User Action:
**  The Internal Procedure "User" is a Ingres Corp. supplied FE program.
**  It should not generate Internal Procedure Errors, so a call to Ingres
**  Corp. tech support is merited.
*/
E_QE0134_LISTFILE_ERROR:NO_GE
"Error encounted while performing iiQEF_listfile Internal Procedure.  See errlog.log
 file for details."
/*%
** Parameters:
**  None.
** Description:
**  DMD encountered (and logged in errlog.log) an error while attempting a
**  DIlistfile or DIlistdir.
** System Status:
**  The Execute Procedure statement is aborted.
** User Action:
**  The Internal Procedure "User" is a Ingres Corp. supplied FE program.
**  It should not generate Internal Procedure Errors, so a call to Ingres
**  Corp. tech support is merited.
*/
E_QE0135_DROPFILE_ERROR:NO_GE
"Error encounted while performing iiQEF_dropfile Internal Procedure.  See errlog.log
 file for details."
/*%
** Parameters:
**  None.
** Description:
**  DMF encountered (and logged) and error while attempting a DIdelete.
** System Status:
**  The Execute Procedure statement is aborted.
** User Action:
**  The Internal Procedure "User" is a Ingres Corp. supplied FE program.
**  It should not generate Internal Procedure Errors, so a call to Ingres
**  Corp. tech support is merited.
*/
E_QE0160_INVALID_GRPID_OP:NO_GE
"Invalid QEF operation for iiusergroup encountered."
/*%
** Parameters: (none)
**
** Explanation:
**	qeu_group() encountered an invalid operation code in qeu_agid_mask.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_QE0161_INVALID_APLID_OP:NO_GE
"Invalid QEF operation for iiapplication_id encountered."
/*%
** Parameters: (none)
**
** Explanation:
**	qeu_aplid() encountered an invalid operation code in qeu_agid_mask.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_QE0162_GRPID_ANCHOR_MISSING:NO_GE
"Missing anchor detected for group '%0c'."
/*%
** Parameters:
**	0 - group name
**
** Explanation:
**	qeu_dgroup() discovered a group member record that did not
**	have an associated anchor record.  This should never happen.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support.
**	Ensure that no user deleted the anchor tuple.
**
*/
E_QE0163_GRPMEM_PURGE:NO_GE
"Invalid request to purge group member encountered\n
	 Group: '%0c'\n
	Member: '%0c'"
/*%
** Parameters:
**	0 - group name
**	1 - member name
**
** Explanation:
**	qeu_dgroup() detected a request to purge, but the input
**	tuple represented a member, not an anchor.  This is 
**	inconsistent.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_QE0164_GRPID_PURGE:NO_GE
"Unable to recreate anchor during purge of group '%0c'."
/*%
** Parameters:
**	0 - group name
**
** Explanation:
**	When purging a group, qeu_dgroup() successfully deleted
**	all group records, but could not add the anchor record
**	back using qeu_cgroup().  This is a traceback message.
**	qeu_cgroup() should have issued a previous message
**	indicating the exact nature of the failure.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_QE0199_CALL_ALLOC:NO_GE
"Failed attempt to nest procedure (rule) '%0c' (%1c) at query depth %2d
due to insufficient resources in the DBMS server.  The current query is
aborted with a fatal error.  Increase the server startup option
CURSORS_PER_SESSION (and internal option QEP_SIZE) in order to increase 
the amount of available memory for your session."
/*%
** Parameters:
**	%0 - Name of procedure being nested.
**	%1 - Name of rule (if from a rule) or NULL.
**	%2 - The current nesting depth of the procedure query plan.
**
** Explanation:
**	A deeply nested or recursive rule (or procedure) has caused the server
**	facility QEF to exceed the amount of memory allocated for this session.
**
** System Status:
**	Program continues and current statement is aborted.
**
** User Action:
**	Request from your server administrator to allocate more memory per
**	session by increasing the values of the startup options specified
**	in the error message.  If this is not an alternative then try to
**	modify the flow of the procedure (or rule) so that the nesting
**	depth is decreased.
*/
E_QE019A_EVENT_MESSAGE:E_GE7918_SYNTAX_ERROR
"The %0c EVENT statement may not be issued because your application is
running with an older communication (GCA) version.  This statement, when
executed, will cause a new GCA message type to be transmitted
but your application is not prepared to receive such a message type."
/*%
** Parameters:
**	%0 - name of EVENT operation.
**
** Explanation:
**	An attempt to REGISTER an EVENT fails because when the event is later
**	raised, the system will attempt to transmit a new GCA message type
**	to your registered application.  This message type, however, belongs
**	to a more recent version of GCA and cannot be interpreted by your
**	application (losing the functionality of events).
**
** System Status:
**	Program continues and current statement is aborted.
**
** User Action:
**	Relink your application with a more recent set of run-time libraries.
*/
E_QE0200_NO_EVENTS:E_GE7918_SYNTAX_ERROR
"The %0c EVENT statement may not be issued because the DBMS event subsystem 
has not been initialized.  This may be caused by the use of a zero EVENTS
startup option, by a server startup error, by the forced removal of the
server event thread, or an internal error during event processing.  Check 
the server error log for event-related errors."
/*%
** Parameters:
**	%0 - name of EVENT operation.
**
** Explanation:
**	An attempt to REGISTER, REMOVE or RAISE an EVENT fails because the
**	internal event subsystem has not been initialized.  This can 
**	occur:
**	  If the server EVENTS=0 command-line parameter was used.
**	  If the DBMS server startup phase encountered a memory allocation 
**	     error.
**	  If the event subsystem encountered a connect-time error.
**	  If the DBMS event thread was removed (through IIMONITOR) or exited
**	     due to internal event errors.
**
** System Status:
**	Program continues and current statement is aborted.
**
** User Action:
**	If the server has been installed with EVENTS=0 then the above statements
**	are not allowed.
**	
**	All other errors are detailed in the server error log.
**	Determine which server the user is currently using and check that
**	server's error log for event-related errors.  The action
**	to take will depend on the error discovered.
*/
E_QE0201_BAD_SORT_KEY:E_GE80E8_LOGICAL_ERROR
"Illegal QP detected.  A sort key is not one of sort tuple attributes."
E_QE0202_RESOURCE_CHCK_INVALID:NO_GE
"QEF consistency check. An attempt was made to apply resource checking
to an action that did not contain a QEP."
/*
** Parameters:
**	none
**
** Explanation:
**	An error internal to QEF has occurred. QEF has attempted to apply
**	resource checking to a node that does not use query resources. Only
**	nodes containing QEP's use query resources. Essentially, resource
**	checking has been invoked by an action procedure that does not
**	implement query processing.
**
** System Status:
**	The current query is aborted with a fatal error status.
**
** User Action:
**	Report this internal system error.
*/
E_QE0203_DIO_LIMIT_EXCEEDED:E_GE8CA0_SYSTEM_LIMIT
"The estimated number of IO requests to run this query (%0d) exceeds
the IO limit (%1d) established for this session. \n
The query is aborted with a fatal error."
/*
** Parameters:
**	%0  Estimated number of IO operations required to execute the query.
**	%1  IO limit established for this session.
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or System Administrator. The limits may be individually
**	established for each database via the GRANT statement.
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the estimate of IO operations
**	exceeded the limit for this session.
**
**  System Status:
**	The query is aborted with a fatal error.
**
**  User Action:
**	1. Attempt to run a variant of the query that will use fewer
**	   resources. This may be possible by adding additional qualifications
**	   to the query or by further restricting the range of values in
**	   current qualifications.
**
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**
**	3. Ask your DBA or System Administrator to increase your resource
**	   limits.
*/
E_QE0204_ROW_LIMIT_EXCEEDED:E_GE8CA0_SYSTEM_LIMIT
"The estimated number of rows returned to the application (%0d) exceeds
the row limit (%1d) established for this session.\n
The query is aborted with a fatal error."
/*
** Parameters:
**	%0  Estimated number of rows returned by the query.
**	%1  Row limit established for this session.
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or System Administrator. The limits may be individually
**	established for each database via the GRANT statement.
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the estimate of the number of
**	rows to be returned to the application exceeded the limit for this
**	session. Note that this limit applies only to SELECT and OPEN CURSOR
**	statements. Statements that do not return rows to the application
**	such as INSERT, UPDATE, DELETE and SELECT AS ignore this limit.
**
**  System Status:
**	The query is aborted with a fatal error.
**
**  User Action:
**	1. Attempt to run a variant of the query that will return fewer
**	   rows. This may be possible by adding additional qualifications
**	   to the query or by further restricting the range of values in
**	   current qualifications.
**
**	2. Ask your DBA or System Administrator to increase your resource
**	   limits.
*/
E_QE0205_CPU_LIMIT_EXCEEDED:E_GE8CA0_SYSTEM_LIMIT
"The estimated CPU time to run this query (%0d normalized units) exceeds
the CPU limit (%1d normalized units) established for this session.\n
The query is aborted with a fatal error."
/*
** Parameters:
**	%0  Estimated units of CPU resource required to execute the query.
**	%1  CPU resource limit established for this session.
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or System Administrator. The limits may be individually
**	established for each database via the GRANT statement.
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the estimate of CPU resources
**	exceeded the limit for this session. Note that CPU resources are
**	estimated in normalized units and provide only a relative measure
**	of the cpu time required to execute a query. The estimates do not
**	correspond to the actual cpu time used on a specific processor.
**
**  System Status:
**	The query is aborted with a fatal error.
**
**  User Action:
**	1. Attempt to run a variant of the query that will use fewer
**	   resources. This may be possible by adding additional qualifications
**	   to the query or by further restricting the range of values in
**	   current qualifications.
**
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**
**	3. Ask your DBA or System Administrator to increase your resource
**	   limits.
*/
E_QE0206_PAGE_LIMIT_EXCEEDED:E_GE8CA0_SYSTEM_LIMIT
"The estimated number of pages referenced to run this query (%0d) exceeds
the page reference limit (%1d) established for this session.\n
The query is aborted with a fatal error."
/*
** Parameters:
**	%0  Estimated number of pages referenced to execute the query.
**	%1  Page reference limit established for this session.
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or System Administrator. The limits may be individually
**	established for each database via the GRANT statement.
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the estimate of the number of
**	database pages referenced by the query exceeded the limit for this
**	session.
**
**  System Status:
**	The query is aborted with a fatal error.
**
**  User Action:
**	1. Attempt to run a variant of the query that will use fewer
**	   resources. This may be possible by adding additional qualifications
**	   to the query or by further restricting the range of values in
**	   current qualifications.
**
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**
**	3. Ask your DBA or System Administrator to increase your resource
**	   limits.
*/
E_QE0207_COST_LIMIT_EXCEEDED:E_GE8CA0_SYSTEM_LIMIT
"The total estimated cost to run this query (%0d) exceeds
the cost limit (%1d) established for this session.\n
The query is aborted with a fatal error."
/*
** Parameters:
**	%0  Estimated cost to execute the query.
**	%1  Cost limit established for this session.
**
** Explanation:
**	Each user may be constrained by resource limits established by the
**	DBA or System Administrator. The limits may be individually
**	established for each database via the GRANT statement.
**	Each query that is executed contains an estimate of the resources
**	required to execute the query. If the estimate for a query exceeds
**	the resource limit established for the current session, the query
**	request is rejected. In this case, the total estimated cost
**	exceeded the limit for this session. The total estimated cost is
**	a normalized function of the estimated number of IO operations and
**	estimated cpu cost to execute the query. This number serves only
**	as a relative measure and does not represent physical resources
**	consumed on a specific processor.
**
**  System Status:
**	The query is aborted with a fatal error.
**
**  User Action:
**	1. Attempt to run a variant of the query that will use fewer
**	   resources. This may be possible by adding additional qualifications
**	   to the query or by further restricting the range of values in
**	   current qualifications.
**
**	2. Determine if the database is properly organized for efficient
**	   execution of the query under consideration. If this is a common
**	   query it may be advisable to change the storage structures or
**	   index structures on the tables involved.
**
**	3. Ask your DBA or System Administrator to increase your resource
**	   limits.
*/
E_QE0208_EXCEED_MAX_CALL_DEPTH:E_GE8CA0_SYSTEM_LIMIT
"Query exceeds the limit (%0d) for cascading rules and nested database 
procedures.  The query is aborted with a fatal error."
/*
**  Parameters:
**	%0  Maximum nesting level for database procedures.
**
**  Explanation:
**	This error will occur when executing queries that trigger rule
**	processing. The rule processing may be such that a query triggers
**	a rule, a query within that rule triggers another rule, and so on.
**	Each such nested rule firing causes the execution of the database
**	procedure associated with the rule to be stacked and the database
**	procedure invoked by the newly triggered rule to begin execution.
**	The maximum depth of such nesting is constrained by an internal
**	system parameter. The current query has triggered rule execution
**	that has cascaded beyond the maximum depth allowed.
**
**  System Status:
**	The original query triggering rule execution is aborted with a
**	fatal error. All queries run by the database procedures invoked
**	by rule processing are rolled back.
**
**  User Action:
**
**	The following are possible causes of this error:
**
**	    1. The collection of rules defined in the database establishes
**	       dependencies between more tables than are supported by the
**	       system. Both the database design and the set of rules defined
**	       should be inspected and changed to limit the dependencies
**	       between tables.
**
**	    2. Circular dependencies may be defined that will cause rules
**	       to cascade indefinitely. Inspect the set of rules defined
**	       and modify one or more rules in the offending rule set.
*/
E_QE0209_BAD_RULE_STMT:E_GE7918_SYNTAX_ERROR
"The %0c statement cannot be issued while firing a database rule.
The current query is aborted."
/*
**  Parameters:
**	%0  Name of statement that is being issued.
**
**  Explanation:
**	This error will occur when executing queries that trigger rule
**	processing.  Various statements cannot be issued while processing a
**	rule as they modify the state of the database in relation to the
**	original query that fired the rule.  Currently statements that cannot
**	be issued from within rule processing are:
**		COMMIT
**		ROLLBACK
**
**  System Status:
**	The original query triggering rule execution is aborted with a
**	fatal error. All queries run by the database procedures invoked
**	by rule processing are rolled back.
**
**  User Action:
**
**	If the database procedure invoked by the rule is also used 
**	independently (via the EXECUTE PROCEDURE statement) then add another
**	parameter to the procedure that enables the conditional issuing of
**	the violating statement.
**
**	If the database procedure is always invoked from a rule then remove 
**	the violating statement and issue it after control returns to the
**	application.
*/
E_QE020A_EVENT_EXISTS:E_GE75F8_DEF_RESOURCE
"CREATE EVENT: Event '%0c' already exists."
/*
** Parameters:
**	%0 - Name of event that is being created.
** Explanation:
**	The named event already exists for the current user.  Duplicate event
**	names are not allowed.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Choose a unique name for the new event or delete the existing named
**	event.
*/
E_QE020B_EVENT_ABSENT:E_GE75B2_NOT_FOUND
"DROP EVENT: Event '%0c' does not exist or is not owned by you."
/*%
** Parameters:
**	%0 - Name of event that is being dropped.
** Explanation:
**	The named event does not exist for the current user.  Events owned
**	by other users cannot be dropped.
** System Status:
**	Program continues and current DROP statement is aborted.
** User Action:
**	Retype the event name.
*/
E_QE020C_EV_REGISTER_EXISTS:E_GE75F8_DEF_RESOURCE
"REGISTER EVENT: Event '%0c.%1c' is already registered.
This new registration is ignored."
/*%
** Parameters:
**	%0 - The event owner to uniquely identify the event.
**	%1 - Name of event that is being registered.
** Explanation:
**	The named event is already registered for the current application.
**	Duplicate event registrations are not allowed.
** System Status:
**	The current REGISTER statement is aborted, but the original
**	registration remains in effect.  If this statement was issued from
**	within a database procedure then the error causes the default
**	error semantics of database procedures.
** User Action:
**	Control the registration of events to only register them once.
**	The REMOVE EVENT statement may be used to de-register an event.
*/
E_QE020D_EV_REGISTER_ABSENT:E_GE75B2_NOT_FOUND
"REMOVE EVENT: Event '%0c.%1c' is not registered.  This statement is ignored."
/*%
** Parameters:
**	%0 - The event owner to uniquely identify the event.
**	%1 - Name of event that is being removed.
** Explanation:
**	The named event is not (yet) registered for the current application.
**	Registration removal may only apply to registered events.  Note
**	that application (session) termination automatically removes all your
**	registrations.
** System Status:
**	The current REMOVE statement is aborted without modifying the state
**	of already registered events.  If this statement was issued from
**	within a database procedure then the error causes the default
**	error semantics of database procedures.
** User Action:
**	Control the registration and deregistration of events to only
**	register events once, and remove them when registered.
*/
E_QE020E_EV_PERMIT_ABSENT:E_GE75B2_NOT_FOUND
"Event '%0c' owned by '%1c' does not exist for GRANT, REVOKE or DROP PERMIT
privilege modification."
/*%
** Parameters:
**	%0 - Name of event for which privileges are being granted or revoked.
**	%1 - The current user is assumed to be the event owner.
** Explanation:
**	The named event does not exist.  Privileges may only be granted or
**	revoked on existing events.  If an event is dropped all privileges
**	associated with that event are also dropped.
** System Status:
**	The current GRANT, REVOKE or DROP PERMIT statement is aborted.
** User Action:
**	Use the CREATE EVENT statement to recreate the event.
*/
E_QE020F_EVENT_SCF_FAIL:NO_GE
"SCF failure on operation '%0c', with error number %1x (%1d),
on event '%2c.%3c'.  Check the installation error log file for
more details, as this may be an internal error."
/*%
** Parameters:
**	%0 - Name of current statement - REGISTER, REMOVE, RAISE
**	%1 - Internal failure status.
**	%2 - Event owner.
**	%3 - Event name.
** Explanation:
**	The current operation failed due to an internal failure.
** System Status:
**	The current statement is aborted.
** User Action:
**	Check the DBMS error log file for more information.  This may be
**	an internal error, or you may have encountered server initialization
**	errors.  Check the error log for earlier event-related errors.
*/

E_QE0210_QEC_ALTER:NO_GE
"An error occurred while establishing QEF session parameters.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	QEC_ALTER returned bad status when storing session database
**	privileges.  This is am internal error, probably due to
**	a bad control block.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_QE022A_IIUSERGROUP:NO_GE
"An error occurred while processing the
iiusergroup catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iiusergroup catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Note: group identifiers are not
**	verified for super users.
**	If it occurs during CREATE/ALTER/DROP processing,
**	the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_QE022B_IIAPPLICATION_ID:NO_GE
"An error occurred while processing the iirole catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iirole catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Note: role identifiers are not
**	verified for super users.
**	If it occurs during CREATE/ALTER/DROP processing,
**	the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_QE022C_IIDBPRIV:NO_GE
"An error occurred while processing the iidbpriv catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iidbpriv catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Super users are allowed to continue.
**	If it occurs while processing GRANT/REVOKE statements
**	for database privileges, the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_QE022D_IIDATABASE:NO_GE
"An error occurred while processing the iidatabase catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if QEF has trouble opening,
**	reading, or closing the iidatabase catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs while processing GRANT/REVOKE statements
**	for database privileges, the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_QE022E_IIUSER:NO_GE
"An error occurred while processing the iiuser catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if QEF has trouble opening,
**	reading, or closing the iiuser catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs while processing GRANT/REVOKE statements
**	for database privileges, the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_QE022F_SCU_INFO_ERROR:NO_GE
"Internal error:\n
QEF detected an error in scu_information.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if QEF gets a bad return code
**	from scu_information().
**
** System Status:
**	The query is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_QE0230_DMU_TOO_MANY:NO_GE
"Transactions cannot contain more than 255 DDL/DMU operations."
E_QE0240_BAD_LOC_TYPE:NO_GE
"The iiQEF_alter_extension internal procedure currently supports changing a
defaultable WORK location extension to an auxiliary WORK location extension,
and vice versa.  No further operations are supported."
/*%
** Parameters: (none)
**
** Explanation:
**	An attempt was made to invoke the iiQEF_alter_extension internal
**	procedure with invalid parameters.  This procedure is used to change a
**	defaultable WORK location extension to an auxiliary WORK location
**	extension, or vice versa.  Upon decoding the parameters sent to it, we
**	discovered that neither of these operations was being attempted.
**	Anything else is invalid, so we issue this error.
**
**	Users should not be invoking this procedure themselves.  The ACCESSDB
**	utility should be the only program to call this procedure, and if this
**	error is encountered while using ACCESSDB, then this should be
**	considered an ACCESSDB bug.
**
** System Status:
**	The procedure invocation is nullified.  No net effect.
**
** User Action:
**	Call the INGRES system administrator.
*/
E_QE0241_DB_NOT_EXTENDED:NO_GE
"This database has not been extended to this location.  Before altering the
extension type of this location you must first extend the database to it."
/*%
** Parameters: (none)
**
** Explanation:
**	An attempt was made to invoke the iiQEF_alter_extension internal
**	procedure before the database was extended to this location.  This
**	is not allowed.  You must first extend the database to a location,
**	and then alter the extension.
**
** System Status:
**	The procedure invocation is nullified.  No net effect.
**
** User Action:
**	Extend the database to this location first.
*/
E_QE0242_BAD_EXT_TYPE:NO_GE
"The location extension you are attempting to alter is not the correct type;
either it is not a WORK location extension or the operation you are attempting
has already been done."
/*%
** Parameters: (none)
**
** Explanation:
**	An attempt was made to invoke the iiQEF_alter_extension internal
**	procedure to change a location extension; however, the type of this
**	extension was either not a WORK location (which is illegal) or it
**	was not the specified type of WORK location.
**
**	For example, if you attempt to change a defaultable WORK location
**	extension to an auxiliary WORK location extension, but this extension
**	is already an auxiliary WORK location extension then you will get this
**	error.
**
** System Status:
**	The procedure invocation is nullified.  No net effect.
**
** User Action:
**	Correct your action based on above information.
*/
E_QE0243_CNF_INCONSIST:NO_GE
"Config file is inconsistent with information in IIEXTEND catalog.  Either the
extent listed in the IIEXTEND catalog was not in the database's config file, or
the extension type bits in the IIEXTEND catalog were inconsistent with the
corresponding type bits in the config file."
/*%
** Parameters: (none)
**
** Explanation:
**	Preliminary checks in QEF based on the information in the IIEXTEND
**	catalog were satisfied, but DMF found that the information in the
**	database's configuration file was inconsistent with this information.
**
**	This is an internal error and should be reported as a bug.
**
** System Status:
**	The procedure invocation is nullified.  No net effect.
**
** User Action:
**	Contact your INGRES site administrator.
*/
E_QE0244_TOO_FEW_LOCS:NO_GE
"There are currently no WORK locations being used for this session.  The
operation you are attempting requires at least one WORK location."
/*%
** Parameters: (none)
**
** Description:
**	The caller attempted an operation which required a minimum of 1
**	WORK location; he had none.  WORK locations are required for
**	operations such as sorting.  See the documentation set for more
**	information.  In particular, see sections on WORK locations and
**	the SET WORK LOCATIONS command.
**
** System Status:
**	The procedure invocation is nullified.  No net effect.
**
** User Action:
**	Correct your action based on above information.
*/
E_QE0245_TOO_MANY_LOCS:NO_GE
"There are currently more than 256 WORK locations being used for this session.
This exceeds the maximum allowed."
/*%
** Parameters: (none)
**
** Description:
**	The caller attempted an operation which can make use of a maximum of
**	256 WORK locations; he had more.  WORK locations are required for
**	operations such as sorting.  See the documentation set for more
**	information.  In particular, see sections on WORK locations and
**	the SET WORK LOCATIONS command.
**
** System Status:
**	The procedure invocation is nullified.  No net effect.
**
** User Action:
**	Correct your action based on above information.
*/
E_QE0500_RQF_GENERIC_ERR:NO_GE
"RQF reported an unspecified error."
E_QE0501_WRONG_COLUMN_COUNT:NO_GE
"RQF detected a column-number mismatch."
E_QE0502_NO_TUPLE_DESCRIPTION:NO_GE
"RQF detected a missing tuple description."
E_QE0503_TOO_MANY_COLUMNS:NO_GE
"RQF detected an excessive column count."
E_QE0504_BIND_BUFFER_TOO_SMALL:NO_GE
"RQF detected an excessively small bind buffer."
E_QE0505_CONVERSION_FAILED:NO_GE
"RQF detected a data conversion failure."
E_QE0506_CANNOT_GET_ASSOCIATION:E_GE75BC_UNKNOWN_OBJECT
"Either the node or the database is not defined or cannot be reached."
E_QE0507_BAD_REQUEST_CODE:NO_GE
"RQF detected an invalid operation."
E_QE0508_SCU_MALLOC_FAILED:NO_GE
"RQF failed to allocate space from SCU."
E_QE0509_ULM_STARTUP_FAILED:NO_GE
"RQF failed to start up ULM."
E_QE0510_ULM_OPEN_FAILED:NO_GE
"RQF failed to open with ULM."
E_QE0511_INVALID_READ:NO_GE
"RQF detected an invalid read operation."
E_QE0512_INVALID_WRITE:NO_GE
"RQF detected an invalid write opeation."
E_QE0513_ULM_CLOSE_FAILED:NO_GE
"RQF failed to close with ULM."
E_QE0514_QUERY_ERROR:NO_GE
"An (LDB) query error or warning occurred."
E_QE0515_UNEXPECTED_MESSAGE:NO_GE
"RQF detected an unexpected message."
E_QE0516_CONVERSION_ERROR:NO_GE
"RQF detected a conversion error."
E_QE0517_NO_ACK:NO_GE
"RQF failed to obtain an acknowledgment."
E_QE0518_SHUTDOWN_FAILED:NO_GE
"RQF failed to shutdown."
E_QE0519_COMMIT_FAILED:NO_GE
"RQF failed to commit."
E_QE0520_ABORT_FAILED:NO_GE
"RQF failed to abort."
E_QE0521_BEGIN_FAILED:NO_GE
"RQF failed to begin a transaction."
E_QE0522_END_FAILED:NO_GE
"RQF failed to end a transaction."
E_QE0523_COPY_FROM_EXPECTED:NO_GE
"RQF detected a missing COPY FROM operation."
E_QE0524_COPY_DEST_FAILED:NO_GE
"RQF failed to copy to a destination LDB."
E_QE0525_COPY_SOURCE_FAILED:NO_GE
"RQF failed to copy from a source LDB."
E_QE0526_QID_EXPECTED:NO_GE
"RQF detected a missing query id."
E_QE0527_CURSOR_CLOSE_FAILED:NO_GE
"RQF failed to close a local cursor."
E_QE0528_CURSOR_FETCH_FAILED:NO_GE
"RQF failed to fetch from a local cursor."
E_QE0529_CURSOR_EXEC_FAILED:NO_GE
"RQF failed to open a local cursor."
E_QE0530_CURSOR_DELETE_FAILED:NO_GE
"RQF failed to delete the current row of a local cursor."
E_QE0531_INVALID_CONTINUE:NO_GE
"RQF detected an invalid CONTINUE operation."
E_QE0532_DIFFERENT_TUPLE_SIZE:NO_GE
"The schema of a local table has been changed since it was created."
E_QE0533_FETCH_FAILED:NO_GE
"RQF failed to retrieve data."
E_QE0534_COPY_CREATE_FAILED:NO_GE
"RQF failed to create a temporary LDB table."
E_QE0535_BAD_COL_DESC_FORMAT:NO_GE
"RQF detected a column-description format mismatch."
E_QE0536_COL_DESC_EXPECTED:NO_GE
"RQF detected a missing column description."
E_QE0537_II_LDB_NOT_DEFINED:NO_GE
"RQF detected an undefined LDB."
E_QE0538_ULM_ALLOC_FAILED:NO_GE
"RQF failed to allocate space from ULM."
E_QE0539_INTERRUPTED:NO_GE
"RQF was interrupted."
E_QE0540_UNKNOWN_REPEAT_Q:NO_GE
"RQF detected an unknown repeat query id."
E_QE0541_ERROR_MSG_FROM_LDB:NO_GE
"RQF detected an error message from an LDB."
E_QE0542_LDB_ERROR_MSG:NO_GE
"RQF detected an LDB error message."
E_QE0543_CURSOR_UPDATE_FAILED:NO_GE
"RQF detected a CURSOR UPDATE failure."
E_QE0544_CURSOR_OPEN_FAILED:NO_GE
"RQF detected a CURSOR OPEN failure."
E_QE0545_CONNECTION_LOST:E_GE9088_COMM_ERROR
"RQF detected the loss of an LDB association."
E_QE0546_RECV_TIMEOUT:E_GE9088_COMM_ERROR
"RQF detected a timeout condition."
E_QE0700_TPF_GENERIC_ERR:NO_GE
"TPF reported an unspecified error."
E_QE0701_INVALID_REQUEST:NO_GE
"TPF detected an invalid operation."
E_QE0702_SCF_MALLOC_FAILED:NO_GE
"TPF failed to allocate space from SCF."
E_QE0703_SCF_MFREE_FAILED:NO_GE
"TPF failed to free space with SCF."
E_QE0704_MULTI_SITE_WRITE:E_GE9D6C_CONSTR_VIO
"Updating more than 1 site is not allowed within a transaction."
E_QE0705_UNKNOWN_STATE:E_GE9E34_TRAN_STATE_INV
"TPF detected an unknown transaction state."
E_QE0706_INVALID_EVENT:E_GE9E34_TRAN_STATE_INV
"TPF detected an invalid transaction event."
E_QE0707_INVALID_TRANSITION:E_GE9E34_TRAN_STATE_INV
"TPF detected an invalid transaction transition."
E_QE0708_TXN_BEGIN_FAILED:NO_GE
"TPF failed to begin a transaction."
E_QE0709_TXN_FAILED:NO_GE
"TPF detected a transaction failure."
E_QE0710_SAVEPOINT_FAILED:NO_GE
"TPF failed to establish a savepoint."
E_QE0711_SP_ABORT_FAILED:NO_GE
"TPF failed to abort to a savepoint."
E_QE0712_SP_NOT_EXIST:NO_GE
"TPF detected a non-existent savepoint."
E_QE0713_AUTO_ON_NO_SP:E_GE80E8_LOGICAL_ERROR
"AUTOCOMMIT is not allowed since a savepoint exists."
E_QE0714_ULM_STARTUP_FAILED:NO_GE
"TPF detected a ULM startup failure."
E_QE0715_ULM_OPEN_FAILED:NO_GE
"TPF detected a ULM open failure."
E_QE0716_ULM_ALLOC_FAILED:NO_GE
"TPF detected a ULM allocation failure."
E_QE0717_ULM_CLOSE_FAILED:NO_GE
"TPF detected a ULM close failure."
E_QE0720_INTERNAL:NO_GE
"TPF detected an internal error."
/*%
** Parameters:
**      NONE.
**
** System Status:
**      This is an internal error.  TPF will act accordingly.
**
** User Action:
**      This is an informational message logged to the error_log file, other
**      error messages will suggest remedies if any.  
*/
E_QE0721_LOGDX_FAILED:NO_GE
"TPF could not log the distributed transaction."
/*%
** Parameters:
**      NONE.
**
** System Status:
**      TPF has aborted the distributed transaction.
**
** User Action:
**      This is an informational message logged to the error_log file, other
**      error messages will suggest remdeies if any.
**
**      (A possible contributing problem: the CDB becomes unavailable.)
*/
E_QE0722_SECURE_FAILED:NO_GE
"TPF could not secure the distributed transaction."
/*%
** Parameters:
**      NONE.
**
** System Status:
**      TPF has aborted the distributed transaction.
**
** User Action:
**      This is an informational message logged to the error_log file, other
**      error messages will suggest remedies if any.  
**
**      (One possible contributing problem: an LDB becomes unavailable.)
*/
E_QE0723_COMMIT_FAILED:NO_GE
"TPF could not commit the distributed transaction."
/*%
** Parameters:
**      NONE.
**
** System Status:
**      TPF has aborted the distributed transaction.
**
** User Action:
**      This is an informational message logged to the error_log file, other
**      error messages will suggest remedies if any.  
**
**      (One possible contributing problem: an LDB refuses to commit or becomes 
**      unavailable before a unanimous COMMIT decision can be reached.)
*/
E_QE0902_NO_LDB_TAB_ON_CRE_LINK:E_GE7918_SYNTAX_ERROR
"Cannot register a non-existent LDB table."
E_QE0960_CONNECT_ON_CONNECT:E_GE7918_SYNTAX_ERROR
"Invalid nested DIRECT CONNECT."
E_QE0961_CONNECT_ON_XACTION:E_GE9E34_TRAN_STATE_INV
"DIRECT CONNECT is not allowed within a transaction."
E_QE0962_DDL_CC_ON_XACTION:E_GE9E34_TRAN_STATE_INV
"DDL_CONCURRENCY cannot be changed within a transaction."
E_QE096E_INVALID_DISCONNECT:E_GE7918_SYNTAX_ERROR
"Invalid DIRECT DISCONNECT."
E_QE0970_BAD_USER_NAME:E_GEA028_USER_ID_INV
"Invalid user name detected."
E_QE0980_LDB_REPORTED_ERROR:NO_GE
"Error reported by LDB."
E_QE0990_2PC_BAD_LDB_MIX:NO_GE
"The query involves LDBs that cannot process 2PC or cause a 1PC/2PC \
conflict in the current distributed transaction."
/*%
** Parameters:
**	NONE.
**
** System Status:
**	QEF will reject the query and leave the distributed transaction intact.
**
** User Action:
**	(Analyze the LDBs involved in the transaction and correct the offending
**	query to eliminate the LDBs that would conflict with the transaction.)
*/
E_QE1001_NO_ACTIONS_IN_QP:NO_GE
"QEF consistency check.  There are no actions in the current QP."
E_QE1002_BAD_ACTION_COUNT:NO_GE
"QEF consistency check. The count of actions does not agree with the list
of action headers."
E_QE1003_NO_VALID_LIST:NO_GE
"QEF consistency check.  The list of validation is missing from the current 
action."
E_QE1004_TABLE_NOT_OPEN:NO_GE
"QEF consistency check.  Error validating an action.  Trying to position a
table that has not been opened."
E_QE1005_WRONG_JOIN_NODE:NO_GE
"QEF consistency check. Error in QP, qen_sort is being used by a node other
than qen_sijoin."
E_QE1006_BAD_SORT_DESCRIPTOR:NO_GE
"QEF consistency check. The QEN_SHD structure is not clean.  Some memory
could be lost."
E_QE1007_BAD_SORT_COUNT:NO_GE
"QEF consistency check. No tuples in the sort buffer or the list of DM_MDATA
does not agree with the count of tuples."
E_QE1009_DSH_NOTMATCH_QP:NO_GE
"QEF consistency check.  The number of sort nodes in the QP is not the same
as the actual count."
E_QE1010_HOLDING_SORT_BUFFER:NO_GE
"QEF consistency check. The session terminates while holding sort buffers."
E_QE1996_BAD_ACTION:NO_GE
"QEF consistency check.  Invalid action detected."
E_QE1997_NO_ACTION:NO_GE
"QEF consistency check.  No or invalid action detected."
E_QE1998_BAD_QMODE:NO_GE
"QEF consistency check.  Invalid query mode detected."
E_QE1999_BAD_QP:NO_GE
"QEF consistency check.  Invalid query plan detected."

/*
**  TheI_QE2nnn_  Messages are used as Informational
**  text in the security audit messages.  These
**  messages are put here in order to insure that they
**  can be language sensitive.
*/
I_QE2001_ROLE_CREATE:NO_GE
"Attempt to define new ROLE."
I_QE2002_ROLE_DROP:NO_GE
"Attempt to remove a ROLE."
I_QE2003_DBACCESS_CREATE:NO_GE
"Attempt to grant database access to user '%0c'."
I_QE2004_DBACCESS_DROP:NO_GE
"Attempt to revoke database access for user '%0c'."
I_QE2005_DBPRIV_CREATE:NO_GE
"Attempt to grant database privileges '%1c' to user '%0c'."
I_QE2006_DBPRIV_DROP:NO_GE
"Attempt to revoke database privileges '%1c' for user '%0c'."
I_QE2007_GROUP_CREATE:NO_GE
"Attemp to define new GROUP."
I_QE2008_GROUP_MEM_CREATE:NO_GE
"Attempt to add new member '%0c' to group."
I_QE2009_GROUP_DROP:NO_GE
"Attempt to remove a GROUP."
I_QE200A_GROUP_MEM_DROP:NO_GE
"Attempt to remove member '%0c' from group."
I_QE200B_LOCATION_CREATE:NO_GE
"Attempt to define a new LOCATION."
I_QE200C_USER_CREATE:NO_GE
"Attempt to define a new USER."
I_QE200D_USER_DROP:NO_GE
"Attempt to remove a USER."
I_QE200E_SEC_EVENT_CREATE:NO_GE
"Attempt to begin auditing event '%0c'."
I_QE200F_SEC_EVENT_DROP:NO_GE
"Attempt to end auditing event '%0c'."
I_QE2010_SEC_LEVEL_CREATE:NO_GE
"Attempt to begin auditing level '%0c'."
I_QE2011_SEC_LEVEL_DROP:NO_GE
"Attempt to end auditing level '%0c'."
I_QE2012_DBPROC_CREATE:NO_GE
"Attempt to create DATABASE PROCEDURE."
I_QE2013_DBPROC_DROP:NO_GE
"Attempt to drop a DATABASE PROCEDURE."
I_QE2014_VIEW_CREATE:NO_GE
"Attempt to define a VIEW."
I_QE2015_VIEW_DROP:NO_GE
"Attempt to remove a VIEW."
I_QE2016_PROT_TAB_CREATE:NO_GE
"Attempt to define a PROTECTION."
I_QE2017_PROT_DBP_CREATE:NO_GE
"Attempt to define a PROTECTION."
I_QE2018_PROT_TAB_DROP:NO_GE
"Attempt to remove a PROTECTION."
I_QE2019_PROT_DBP_DROP:NO_GE
"Attempt to remove a PROTECTION."
I_QE201A_DATABASE_ACCESS:NO_GE
"Attempt to access a DATABASE."
I_QE201B_ROLE_ACCESS:NO_GE
"Attempt to access a ROLE."
I_QE201C_SEC_CAT_UPDATE:NO_GE
"Attempt to directly update security system catalog."
I_QE201D_DBPROC_EXECUTE:NO_GE
"Attempt to access a DATABASE PROCEDURE."
I_QE201E_TABLE_CREATE:NO_GE
"Attempt to define a TABLE."
I_QE201F_DATABASE_EXTEND:NO_GE
"Attempt to extend a DATABASE to location '%0c'."
I_QE2020_PROT_REJECT:NO_GE
"Protection violation when accessing a TABLE."
I_QE2021_ROLE_ALTER:NO_GE
"Attempt to alter a ROLE."
I_QE2022_GROUP_ALTER:NO_GE
"Attempt to alter a GROUP."
I_QE2023_USER_ALTER:NO_GE
"Attempt to alter a USER."
I_QE2024_USER_ACCESS:NO_GE
"Attempt to authorize USER."
I_QE2025_TABLE_DROP:NO_GE
"Attempt to drop a TABLE."
I_QE2026_TABLE_ACCESS:NO_GE
"Attempt to access a TABLE."
I_QE2027_VIEW_ACCESS:NO_GE
"Attempt to access a VIEW."
I_QE2028_ALARM_EVENT:NO_GE
"Alarm signaled for TABLE."
I_QE2029_DATABASE_CREATE:NO_GE
"Attempt to create a DATABASE."
I_QE202A_DATABASE_DROP:NO_GE
"Attempt to drop a DATABASE."
I_QE202B_LOCATION_ALTER:NO_GE
"Attempt to alter a LOCATION."
I_QE202C_LOCATION_DROP:NO_GE
"Attempt to drop a LOCATION."
I_QE202D_ALARM_CREATE:NO_GE
"Attempt to create a SECURITY_ALARM."
I_QE202E_ALARM_DROP:NO_GE
"Attempt to drop a SECURITY_ALARM."
I_QE202F_RULE_ACCESS:NO_GE
"Attempt to access a RULE."
I_QE2030_PROT_EV_CREATE:NO_GE
"Attempt to define a PROTECTION for an EVENT."
I_QE2031_PROT_EV_DROP:NO_GE
"Attempt to remove a PROTECTION for an EVENT."
I_QE2032_EVENT_ACCESS:NO_GE
"Attempt to access an EVENT."


E_QE7600_LDB_GENERIC_ERR:NO_GE
"QEF detected a general failure reported by an LDB."
E_QE7601_LDB_BAD_SQL_QRY:E_GE7918_SYNTAX_ERROR
"QEF detected an SQL query that cannot be processed by an LDB."
E_QE7602_LDB_XACT_ENDED:E_GEC2EC_SERIALIZATION      
"QEF aborted the distributed transaction because of an LDB's unilateral abort 
action."
/*
**    Message file for Facility code: _QS
*/


#define _QS_CLASS   7


E_QS0000_OK:E_GE0000_OK
"  Normal QSF return status."
E_QS0001_NOMEM:E_GE8D04_NO_RESOURCE
"  QSF dynamic memory pool is exhausted."
E_QS0002_SEMINIT:NO_GE
"  Could not initialize semaphore from SCF."
E_QS0003_OP_CODE:NO_GE
"  Unknown QSF op code."
E_QS0004_SEMRELEASE:NO_GE
"  Could not release semaphore from SCF."
E_QS0005_ACTIVE_OBJECTS:NO_GE
"  Attempting to shut down QSF while there are still active objects."
E_QS0006_ACTIVE_SESSIONS:NO_GE
"  Attempting to shut down QSF while there are still active sessions."
E_QS0007_MEMPOOL_RELEASE:NO_GE
"  Could not release QSF's dynamic memory pool."
E_QS0008_SEMWAIT:NO_GE
"  Error while attempting to set a semaphore from SCF."
E_QS0009_BAD_OBJTYPE:NO_GE
"  Unknown type of object.  QSF only recognizes objects which are
  QUERY TEXTs, QUERY TREEs, or QUERY PLANs."
E_QS000A_OBJ_ALREADY_EXISTS:NO_GE
"  An object with the requested name and type already exists in QSF memory."
E_QS000B_BAD_OBJNAME:NO_GE
"  Illegal name for a QSF object."
E_QS000C_ULM_ERROR:NO_GE
"  Unexpected ULM error detected by QSF."
E_QS000D_CORRUPT:NO_GE
"  QSF's dynamic memory has been corrupted."
E_QS000E_SEMFREE:NO_GE
"  Could not free semaphore from SCF."
E_QS000F_BAD_HANDLE:NO_GE
"  The supplied object handle does not refer to a QSF object."
E_QS0010_NO_EXLOCK:NO_GE
"  QSF object is not exclusively locked by you."
E_QS0011_ULM_STREAM_NOT_CLOSED:NO_GE
"  Could not close a ULM memory stream."
E_QS0012_OBJ_NOT_LOCKED:NO_GE
"  QSF object is not locked by you."
E_QS0013_BAD_LOCKID:NO_GE
"  The lock ID supplied does not match the one assigned
  to the QSF object when it was exclusively locked."
E_QS0014_EXLOCK:NO_GE
"  QSF Object is already locked exclusively."
E_QS0015_SHLOCK:NO_GE
"  QSF Object is already locked shared."
E_QS0016_BAD_LOCKSTATE:NO_GE
"  Illegal lock state specified."
E_QS0017_BAD_REQUEST_BLOCK:NO_GE
"  QSF request control block is not set up properly."
E_QS0018_BAD_PIECE_SIZE:NO_GE
"  Request for an illegal piece size.  Size must be a positive number of bytes."
E_QS0019_UNKNOWN_OBJ:NO_GE
"  No object with the supplied name and type exists in QSF's dynamic memory."
E_QS001A_NOT_STARTED:NO_GE
"  QSF has not been started for the server."
E_QS0500_INFO_REQ:NO_GE
"  QSF info request #%0d:  %1c"
E_QS9001_NOT_YET_AVAILABLE:NO_GE
"  This QSF function is not yet available."
E_QS9002_UNEXPECTED_EXCEPTION:NO_GE
"  Unexpected exception detected by QSF."
E_QS9999_INTERNAL_ERROR:NO_GE
"  A QSF internal error has occurred."
/*
**    Message file for Facility code: _RD 
*/


#define _RD_CLASS 8


E_RD0000_OK:E_GE0000_OK
"successful completion."
E_RD0001_NO_MORE_MEM:E_GE8D04_NO_RESOURCE
"Not enough memory for RDF temporary work area."
E_RD0002_UNKNOWN_TBL:E_GE7594_TABLE_NOT_FOUND
"Requested table is not known to DMF."
E_RD0003_BAD_PARAMETER:NO_GE
"RDF function was called with bad parameters."
E_RD0006_MEM_CORRUPT:NO_GE
"Memory allocation control structure was corrupted."
E_RD0007_MEM_NOT_OWNED:NO_GE
"Not the owner of the memory block."
E_RD0008_MEM_SEMWAIT:NO_GE
"Error in ULM waiting for a memory semaphore."
E_RD0009_MEM_SEMRELEASE:NO_GE
"Error in ULM releasing a memory semaphore."
E_RD000A_MEM_NOT_FREE:NO_GE
"Memory allocation control structure was corrupted, cannot reclaim memory."
E_RD000B_MEM_ERROR:NO_GE
"General error from ULM."
E_RD000C_USER_INTR:NO_GE
"User interrupts while requesting DMF function."
E_RD000D_USER_ABORT:NO_GE
"User's abort detected."
E_RD000E_DMF_ERROR:NO_GE
"General error from DMF."
E_RD000F_INFO_OUT_OF_DATE:NO_GE
"The information block is out of date."
E_RD0010_QEF_ERROR:NO_GE
"Internal error from QEF."
E_RD0011_NO_MORE_ROWS:E_GE0064_NO_MORE_DATA
"No more tuples in the current table."
E_RD0012_BAD_HISTO_DATA:NO_GE
"Histogram tuples are out of sequence."
E_RD0013_NO_TUPLE_FOUND:E_GE75B2_NOT_FOUND
"No tuples found in the current table."
E_RD0014_BAD_QUERYTEXT_ID:NO_GE
"The requested query text is not found in the system catalog."
E_RD0015_AMBIGUOUS_REPLACE:NO_GE
"Attempted to update the same row twice."
E_RD0016_KEY_SEQ:NO_GE
"An illegal key sequence was specified."
E_RD0017_TABLE_EXISTS:NO_GE
"The operation specified tried to create a table that already exists."
E_RD0018_NONEXISTENT_TABLE:E_GE7594_TABLE_NOT_FOUND
"The operation required the use of a table that does not exist."
E_RD0019_TABLE_ACCESS_CONFLICT:NO_GE
"Table access conflict, table opened twice."
E_RD001A_RESOURCE_ERROR:E_GE8D04_NO_RESOURCE
"RDF received out-of-resource error from DMF"
E_RD001B_HISTOGRAM_NOT_FOUND:E_GE75B2_NOT_FOUND
"RDF could not found histogram associated with statistics tuple"
E_RD0020_INTERNAL_ERR:NO_GE
"RDF internal error."
E_RD0021_FILE_NOT_FOUND:E_GE75B2_NOT_FOUND
"Underlying physical file for table not found"
E_RD0025_USER_ERROR:NO_GE
"error by caller, error has been reported or logged by RDF"
E_RD0030_SCF_ERROR:NO_GE
"General error from SCU."
E_RD0031_SEM_INIT_ERR:NO_GE
"Error in SCU initializing a semaphore."
E_RD0032_NO_SEMAPHORE:NO_GE
"The semaphore is not known to SCU."
E_RD0034_RELEASE_SEMAPHORE:NO_GE
"Error releasing ULH semaphore"
E_RD0040_ULH_ERROR:NO_GE
"General error from ULH."
E_RD0041_BAD_FCB:NO_GE
"RDF facility control block contains bad cache id."
E_RD0042_CACHE_NOT_EMPTY:NO_GE
"Error detected in closing a non-empty cache."
E_RD0043_CACHE_FULL:E_GE8D04_NO_RESOURCE
"No more space in cache to build or extend a descriptor block."
E_RD0044_BAD_INFO_BLK:NO_GE
"User's information block is corrupted."
E_RD0045_ULH_ACCESS:NO_GE
"Failed to gain access to a ULH object due to ULH error"
E_RD0050_TRANSACTION_ABORTED:NO_GE
"The transaction has been aborted."
E_RD0060_DMT_SHOW:NO_GE
"Cannot access table information due to a non-recoverable DMT_SHOW error"
E_RD0071_NO_PROT_INT:NO_GE
"error by caller 
\n    asking for QTUPLES but no request for protect, rule or integrity tuples"
E_RD0072_NOT_OPEN:NO_GE
"error by caller 
\n    attempt to close PROTECTION, INTEGRITY or RULE tuple stream which was not open"
E_RD0073_TUPLE_COUNT:NO_GE
"error by caller 
\n    bad tuple count for qrymod retrieve"
E_RD0076_OPEN:NO_GE
"integrity, protection or rule table already opened, but record access id not
\n    available"
E_RD0077_QEU_OPEN:NO_GE
"QEU could not open query mod table"
E_RD0078_QEU_GET:NO_GE
"QEU could not get data from query mod table"
E_RD0079_QEU_CLOSE:NO_GE
"QEU could not close query mod table"
E_RD0102_SYS_CATALOG:NO_GE
"consistency check 
\n    - more than one extended system catalog was opened at
\n    the same time by RDF within the same thread"
E_RD0103_NOT_OPENED:NO_GE
"consistency check 
\n    - attempt to read system catalog which was not opened"
E_RD0104_BUFFER:NO_GE
"consistency check
\n    - internal buffer for reading extended system catalogs not large enough"
E_RD0105_NO_QDATA:NO_GE
"consistency check
\n    - QEF_QDATA structure not initialized"
E_RD0106_CELL_COUNT:NO_GE
"consistency check
\n    - iistatistics tuple contains invalid cell count"
E_RD0107_QT_SEQUENCE:NO_GE
"consistency check
\n    - query tree tuple out of sequence"
E_RD0108_ADI_FIDESC:NO_GE
"consistency check
\n    - error calling the ADI_FIDESC routine"
E_RD0110_QRYMOD:NO_GE
"consistency check
\n    - internal error processing qrymod operation"
E_RD0111_VIEW:NO_GE
"consistency check
\n    - internal error processing view operation"
E_RD0112_ATTRIBUTE:NO_GE
"consistency check
\n    - inconsistent attribute cache"
E_RD0113_KEYS:NO_GE
"consistency check
\n    - inconsistent keying info in attribute cache"
E_RD0114_HASH:NO_GE
"consistency check
\n    - inconsistent attribute hash info"
E_RD0116_0INDEX:NO_GE
"consistency check - request for 0 index tuples"
E_RD0117_STREAMID:NO_GE
"consistency check
\n    - cannot find stream id for memory allocation"
E_RD0118_ULMOPEN:NO_GE
"consistency check
\n    - error calling ULM open"
E_RD011A_ULH_RELEASE:NO_GE
"consistency check
\n    - error releasing ULH object"
E_RD011B_PRIVATE:NO_GE
"consistency check
\n    - private memory stream expected"
E_RD011C_ULM_CLOSE:NO_GE
"consistency check
\n    - error closing private memory stream"
E_RD011D_DAPERMIT:NO_GE
"consistency check
\n    - error defining permit all to all"
E_RD011E_DRPERMIT:NO_GE
"consistency check
\n    - error defining retrieve permit to all"
E_RD011F_DINTEGRITY:NO_GE
"consistency check
\n    - error defining integrity"
E_RD0120_DPERMIT:NO_GE
"consistency check
\n    - error defining permit or security alarm"
/*
** Parameters:
**	None
**
** Explanation:
**	QEU_CPROT returned error status when attempting to create a
**	permit or a security alarm.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	This is a traceback message.  See other related messages
**	for the exact nature of the problem.
*/
E_RD0121_DESTROY_VIEW:NO_GE
"consistency check
\n    - error destroying view"
E_RD0122_DESTROY_PERMIT:NO_GE
"consistency check
\n    - error destroying permit or security alarm"
/*
** Parameters:
**	None
**
** Explanation:
**	QEU_DPROT returned error status when attempting to drop a
**	permit or a security alarm.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	This is a traceback message.  See other related messages
**	for the exact nature of the problem.
*/
E_RD0123_CREATE_VIEW:NO_GE
"consistency check
\n    - error creating view"
E_RD0124_DESTROY_INTEGRITY:NO_GE
"consistency check
\n    - error destroying integrity"
E_RD0125_EXCEPTION:E_GE9D24_DATAEX_OTHER
"consistency check
\n    - unexpected exception in RDF"
E_RD0126_EXEXCEPTION:E_GE9D24_DATAEX_OTHER
"consistency check
\n    - unexpected exception processing exception in RDF"
E_RD0127_ULM_PALLOC:NO_GE
"consistency check
\n    - error calling ulm_palloc"
E_RD0128_SERVER_MEMORY:NO_GE
"consistency check
\n    - error releasing server memory to SCF"
E_RD0129_RDF_STARTUP:NO_GE
"consistency check
\n    - error obtaining memory on RDF startup"
E_RD012A_ULH_CLOSE:NO_GE
"consistency check
\n    - error shutting down ULH"
E_RD012B_ULM_CLOSE:NO_GE
"consistency check
\n    - error closing down ULM memory stream for facility"
E_RD012C_ULH_SEMAPHORE:NO_GE
"consistency check
\n    - cannot get access to ULH semaphore"
E_RD012D_ULH_ACCESS:NO_GE
"consistency check - error from ULH ALIAS command"
E_RD012E_QUERY_TREE:NO_GE
"consistency check - bad query tree"
E_RD012F_QUERY_TREE:NO_GE
"attempt to write out an invalid query tree symbol type"
E_RD0130_PST_LEN:NO_GE
"attempt to write out an invalid query tree symbol type - pst_len incorrect"
E_RD0131_QUERY_TREE:NO_GE
"attempt to write out an invalid query tree header
\n    - an uninitialized field was found"
E_RD0132_QUERY_TREE:NO_GE
"consistency check 
\n    - invalid tree node was read in from catalogs"
E_RD0133_QUERY_TREE:NO_GE
"user error
\n    - old version of query tree, update views, integrities and permits"
E_RD0134_INVALID_SECONDARY:NO_GE
"consistency check
\n    - iirelation secondary index is inconsistent"
E_RD0135_CREATE_PROCEDURE:NO_GE
"consistency check
\n    - error writing procedure object to catalogs"
E_RD0136_CONCURRENT_PROC:NO_GE
"consistency check
\n    - invalid concurrent access to procedure object"
E_RD0137_DUPLICATE_PROCS:NO_GE
"consistency check
\n    - more than one procedure defined with same name"
E_RD0138_QUERYTEXT:NO_GE
"consistency check
\n    - error reading procedure text from iiqrytext"
E_RD0139_DROP_PROCEDURE:NO_GE
"consistency check
\n    - cannot drop procedure due to QEF error"
E_RD013A_UNFIX_PROCEDURE:NO_GE
"consistency check
\n    - cannot unfix procedure"
E_RD013B_BAD_ATTR_COUNT:NO_GE
"consistency check
\n    - bad attribute count of zero for relation"
E_RD013C_BAD_QRYTRE_RNGSIZ:NO_GE
"consistency check
\n    - too many entries in range table for version 3 trees"
/*%
** Parameters:
**	None
** Explanation:
**	Version 3 query trees should never have more than PST_3NUMVARS [30]
**	range table entries.  However, this version 3 query tree has more than
**	30 range entries.  This indicates that the dbms catalogs are damaged
**	or inconsistent.  Either the tree version is incorrect or the data
**	contained in the tree is corrupted.  RDF cannot guess what is wrong and
**	attempt a recovery.  There is no way to determine what the correct
**	number of range entries is, and reading the rest of the tree depends on
**	knowning this.
** System Status:
**	The statement is aborted and RDF complains that it cannot read the
**	query tree.
** User Action:
**	The dbms system catalogs are corrupted.  It may not be possible to
**	fix the query tree.  HOwever, the user should be able to find the
**	corresponding query text in iiqrytext and recreate the qeury via
**	TM.
*/
E_RD013D_CREATE_COMMENT:NO_GE
"RDF interface to QEF (QEU) encountered an error while creating a comment"
/*%
** Parameters:
**	None
** Explanation:
**	QEF does most of the work of adding the comment to the dbms catalogs.
**	QEF returned an error, indicating that it did not successfully add the
**	comment to the dbms catalog.
** System Status:
**	The create comment statement is aborted.
** User Action:
**	The user should examine the QEF error message in the server's error log,
**	and take a corrective action based on that message.  The user may also
**	run verifydb to see if the dbms catalogs are corrupted.
*/
E_RD013E_DROP_COMMENT:NO_GE
"RDF interface to QEF (QEU) encountered an error while dropping a comment"
/*%
** Parameters:
**	None
** Explanation:
**	QEF does most of the work of adding the comment to the dbms catalogs.
**	QEF returned an error, indicating that it did not successfully drop the
**	comment from the dbms catalog.
**	[currently the drop comment statement is implemented by creating a
**	comment with null short and long text.  However, this message is
**	included for future growth.]
** System Status:
**	The drop comment statement is aborted.
** User Action:
**	The user should examine the QEF error message in the server's error log,
**	and take a corrective action based on that message.  The user may also
**	run verifydb to see if the dbms catalogs are corrupted.
*/
E_RD013F_MISSING_RELINFO:NO_GE
"Consistency Check
\n    - a request was made for attribute, key, index information for an
\n      attribute name table without first obtaining relation info."
/*%
** Parameters:
**	None
** Explanation:
**	Either PSF or OPF has called rdu_gdesc and asked for attribute, index,
**	key or attribute name information without first obtaining relation
**	information.  THIS IS NOT AN RDF ERROR, BUT IS A SEVERE LOGIC ERROR OF
**	THE CALLING ROUTINE.
** System Status:
**	The statement is aborted.
** User Action:
**	This is an internal error.  The user should call INGRES tech support.
*/
E_RD0140_BAD_RELKEY_REQ:NO_GE
"Consistency Check
\n    - a request was made for key information without first obtaining
\n	relation info."
/*%
** Parameters:
**	None
** Explanation:
**	Either PSF or OPF has called rdu_gdesc and asked for KEY information
**	without first obtaining attribute information.  THIS IS NOT AN RDF
**	ERROR, BUT IS A SEVERE LOGIC ERROR OF THE CALLING ROUTINE.
** System Status:
**	The statement is aborted.
** User Action:
**	This is an internal error.  The user should call INGRES tech support.
*/
E_RD0141_BAD_ATTHASH_REQ:NO_GE
"Consistency Check
\n    - a request was made for an attribute name table without first
\n	obtaining relation info."
/*%
** Parameters:
**	None
** Explanation:
**	PSF has called rdu_gdesc and asked for an attribute name hash table
**	without first obtaining attribute information.  THIS IS NOT AN RDF
**	ERROR, BUT IS A SEVERE LOGIC ERROR OF THE CALLING ROUTINE.
** System Status:
**	The statement is aborted.
** User Action:
**	This is an internal error.  The user should call INGRES tech support.
*/
E_RD0142_ULH_ACCESS:NO_GE
"consistency check - error from ULH ACCESS command"
/*
** Parameters:
**	None
** Explanation:
**	rdf has found a synonym and determined what the table id is for the
**	table that the synonym resolves to.  It is calling ULH_ACCESS to see
**	if the underlaying table is already on the cache.  The 'not found'
**	error, E_UL0109_NFND, is the only error that is expected here, and that
**	does not cause this error message.  This is an unexpected error.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Try stopping the server and restarting it.  There is some sort of
**	cache problem, and restarting the server should clear the cache problem.
*/
E_RD0143_CREATE_SYNONYM:NO_GE
"RDF interface to QEF (QEU) encountered an error while creating a synonym"
/*%
** Parameters:
**	None
** Explanation:
**	QEF does most of the work of adding the synonym to the dbms catalogs.
**	QEF returned an error, indicating that it did not successfully add the
**	synonym to the dbms catalog.
** System Status:
**	The create synonym statement is aborted.
** User Action:
**	The user should examine the QEF error message in the server's error log,
**	and take a corrective action based on that message.  The user may also
**	run verifydb to see if the dbms catalogs are corrupted.
*/
E_RD0144_DROP_SYNONYM:NO_GE
"RDF interface to QEF (QEU) encountered an error while dropping a synonym"
/*%
** Parameters:
**	None
** Explanation:
**	QEF does most of the work of deleting the synonym from the dbms catalogs.
**	QEF returned an error, indicating that it did not successfully drop the
**	synonym from the dbms catalog.
** System Status:
**	The drop synonym statement is aborted.
** User Action:
**	The user should examine the QEF error message in the server's error log,
**	and take a corrective action based on that message.  The user may also
**	run verifydb to see if the dbms catalogs are corrupted.
*/
E_RD0145_ALIAS_MEM:E_GE8D04_NO_RESOURCE
"RDF was attempting to define an alias, but ULH returned an error."
/*
** Parameters:
**	None
**
** Explanation:
**	RDF did a ulh_define_alias call, and ulh returned error
**	E_UL0119_ALIAS_MEM.  This is viewed as an ERROR if RDF is attempting
**	to create an alias for a table name.  However, if RDF is attempting to
**	create an alias for a synonym, then this is just a warning.
**
**	In this case, RDF was attempting to create an alias to a table name, so
**	this is an error.  It is probably caused because the system is not
**	configured with enough synonym memory space or enough tables in the
**	rdf cache.  /rdf=(tbl_synonyms=n, max_tbls=n) in iirundbms.com_<node>
**	where n is a user specified number.  (defaults are tbl_synonyms=1 and
**	max_tbls=300).  
** System Status:
**	Program continues and current statement is aborted.
** User Action:
**	Modify iirundbms.com_<node> RDF startup parameters.  Specify
**	larger numbers, restart the server and try again.
*/
E_RD0150_CREATE_RULE:NO_GE
"RDF interface to QEF (QEU) encountered error while creating a rule."
/*
** Parameters:
**	None
**
** Explanation:
**	Upon return from QEU_CRULE QEF indicated a failure.  If this is
**	not a user error (such as a duplicate rule name), this error message
**	will be logged.  Parameters to QEU_CRULE should be checked for in
**	RDF_UPDATE/RDR_APPEND/RDR_RULE.
**
** System Status:
**	Program continues and current CREATE statement is aborted.
**
** User Action:
**	Try again with a different rule name.
*/
E_RD0151_DROP_RULE:NO_GE
"RDF interface to QEF (QEU) encountered error while dropping a rule."
/*
** Parameters:
**	None
**
** Explanation:
**	Upon return from QEU_DRULE QEF indicated a failure.  If this is
**	not a user error (such as an invalid rule name), this error message
**	will be logged.  Parameters to QEU_DRULE should be checked for in
**	RDF_UPDATE/RDR_DELETE/RDR_RULE.
**	
** System Status:
**	Program continues and current DROP statement is aborted.
**
** User Action:
*/
E_RD0160_AGROUP:NO_GE
"Internal QEF error detected while appending tuples to iiusergroup"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when appending tuples to iiusergroup.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0161_AAPLID:NO_GE
"Internal QEF error detected while appending tuples to iirole"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when appending tuples to iirole.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0162_DGROUP:NO_GE
"Internal QEF error detected while deleting tuples from iiusergroup"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when deleting tuples from iiusergroup.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0163_DAPLID:NO_GE
"Internal QEF error detected while deleting tuples from iirole"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when purging tuples from iirole.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0164_PGROUP:NO_GE
"Internal QEF error detected while purging tuples in iiusergroup"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when purging tuples from iiusergroup.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0165_RAPLID:NO_GE
"Internal QEF error detected while replacing tuples in iirole"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when replacing tuples in iirole.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0166_GDBPRIV:NO_GE
"Internal QEF error detected while granting database privileges."
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected in QEF while granting database privileges.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0167_RDBPRIV:NO_GE
"Internal QEF error detected while revoking database privileges."
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected in QEF while revoking database privileges.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0168_AUSER:NO_GE
"Internal QEF error detected while appending tuples to iiuser"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when appending tuples to iiuser.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0169_DUSER:NO_GE
"Internal QEF error detected while deleting tuples from iiuser"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when deleting tuples from iiuser.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD016A_RUSER:NO_GE
"Internal QEF error detected while replacing tuples in iiuser"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when replacing tuples in iiuser.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD016B_ALOCATION:NO_GE
"Internal QEF error detected while appending tuples to iilocation"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when appending tuples to iilocation.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD016C_DLOCATION:NO_GE
"Internal QEF error detected while deleting tuples from iilocation"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when deleting tuples from iilocation.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD016D_RLOCATION:NO_GE
"Internal QEF error detected while replacing tuples in iilocation"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when replacing tuples in iilocation.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD016E_RSECSTATE:NO_GE
"Internal QEF error detected while replacing tuples in iisecuritystate"
/*%
** Parameters: (none)
**
** Explanation:
**	This is a traceback message from rdf_update() issued if
**	an error is detected when replacing tuples in iisecuritystate.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	See the related messages.
**	If none, call Ingres Corp. Technical Support.
**
*/
E_RD0200_NO_TUPLES:E_GE75B2_NOT_FOUND
"user error - attempt to read query tree tuples but none were found
\n for the query ID, probable cause is associated table has been deleted"
E_RD0201_PROC_NOT_FOUND:E_GE75B2_NOT_FOUND
"procedure does not exist"
E_RD0202_ALIAS_DEGRADED:NO_GE
"WARNING: RDF is running at degraded performance\n
on some queries using synonyms.\n
This can be caused by too much concurrent activity, or by an improperly\n
configured RDF.  The server will still find correct answers, but may\n
take longer to do so."
/*%
** Parameters:
**	none.
** Explanation:
**	RDF accepts startup parameters from iirundbms command file (located
**	in the utility subdirectory of ingres
**	  ($II_SYSTEM/ingres/utility/iirundbms on unix and
**	   ii_system:[ingres.utility]iirundbms.com_<node_name> on VMS)
**	One of those parameters, max_tbls, indicates the max number of tables
**	RDF may have on its cache (the default is 300).  Another parameter,
**	tbl_synonyms, tells how many synonyms may be defined for each table (the
**	default is 1).
**
**	You may have your system configured reasonably, but alot of concurrent
**	activity is keeping RDF memory usage too high to add another synonym to
**	RDF's memory cache.  Or, you system may not be configured correctly for
**	RDF.
**
**	In either case, you can improve the situation by increasing the
**	number of synonyms permitted per table:
**
**	    \RDF=(max_tbls=xxxx, tbl_synonyms=xxxx)
**
**	where xxxx is a large number than the current (if specified) or
**	default (if not specified) values.  The tradeoff is that the dbms server
**	will use more memory when you increase these values.
**	
** System Status:
**	The statement continues running, but will be a bit slower, because the
**	server will have to go to disk next time it needs this information
**	instead of getting it from memory.
** User Action:
**	user may reconfigure RDF startup parameters and restart server.
*/

/*
** Reserve E_RD0250 to E_RD0300 for Titan
*/
E_RD0250_DD_QEFQUERY:E_GE7D00_QUERY_ERROR
"error occurs when calling QEF to process SQL query"
E_RD0251_DD_QEFFETCH:NO_GE
"error occurs when calling QEF to fetch tuple" 
E_RD0252_DD_QEFFLUSH:NO_GE
"error occurs when calling QEF to flush buffer"
E_RD0253_DD_NOOBJECT:E_GE7594_TABLE_NOT_FOUND
"no object was found in system catalog"
E_RD0254_DD_NOTABLE:E_GE7594_TABLE_NOT_FOUND
"local table not found in local db"
E_RD0255_DD_COLCOUNT:NO_GE
"error occurs when retrieving column count information"
E_RD0256_DD_COLMISMATCH:NO_GE
"column count mismatch"
E_RD0257_DD_NOLDB:NO_GE
"local database was no defined in system catalog"
E_RD0258_DD_NOINGTBL:NO_GE
"object was not defined in system catalog"
E_RD0259_DD_NOPHYTBL:NO_GE
"object was not defined in system catalog"
E_RD025A_ULMSTART:NO_GE
"ulm error"
E_RD025B_ULMSHUT:NO_GE
"ulm error"
E_RD025C_INVALIDDATE:NO_GE
"error occurs when converting date"
E_RD025D_INVALID_DATATYPE:NO_GE
"invalid attribute data type"
E_RD025E_QEF_LDBINFO:NO_GE
"error occurs when calling QEF for ldb information"
E_RD025F_NO_LDBINFO:NO_GE
"ldb description was not defined in system catalog"
E_RD0260_QEF_DDBINFO:NO_GE
"error occurs when calling QEF for ddb information"
E_RD0261_QEF_USRSTAT:NO_GE
"error occurs when calling QEF for user stats"
E_RD0262_QEF_IIDBDB:NO_GE
"error occurs when calling QEF for iidbdb information"
E_RD0263_QEF_LDBPLUS:NO_GE
"error occurs when calling QEF for ldb information"
E_RD0264_UNKOWN_OBJECT:E_GE75BC_UNKNOWN_OBJECT
"undefined object type"
E_RD0265_STORAGE_TYPE_ERROR:NO_GE
"undefined storage type"
E_RD0266_NO_INDEXTUPLE:E_GE75B2_NOT_FOUND
"index tuple not found"
E_RD0267_NO_STATSTUPLE:E_GE75B2_NOT_FOUND
"statistics not found"
E_RD0268_LOCALTABLE_NOTFOUND:E_GE7594_TABLE_NOT_FOUND
"local table not found in LDB"
E_RD0269_OBJ_DATEMISMATCH:NO_GE
"alter date in the local table mismatches alter date in CDB"
E_RD026A_OBJ_INDEXCOUNT:NO_GE
"consistency check error on index count" 
E_RD026B_UNKNOWN_UPDATEMODE:NO_GE
"unknown update mode" 
E_RD026C_UNKNOWN_USER:NO_GE
"unauthorized user"
E_RD026D_USER_NOT_OWNER:NO_GE
"user does not own the local table"
E_RD026E_RDFQUERY:NO_GE
"already in tuple fetching mode" 
E_RD026F_RDFFETCH:NO_GE
"fetch tuple before sending query"
E_RD0270_RDFFLUSH:NO_GE
"invalid buffer flush request"
E_RD0271_UNKNOWN_STORAGETYPE:NO_GE
"unknown storage type for an object"
E_RD0272_UNKNOWN_DDREQUESTS:NO_GE
"unknown distributed information requests"
E_RD0273_LDBLONGNAME_NOTFOUND:NO_GE
"unable to find ldb long name"
E_RD0274_LDBDESC_NOTFOUND:E_GE75B2_NOT_FOUND
"unable to find ldb descriptor"
E_RD0275_UNKNOWN_LDBCAPABILITY:NO_GE
"unknown ldb capability"
E_RD0276_INCONSISTENT_CATINFO:NO_GE
"error detected in distributed catalog"
E_RD0277_CANNOT_GET_ASSOCIATION:NO_GE
"cannot get ldb association"
E_RD0278_MISMATCH_ARCHITECT_ID:NO_GE
"qtree was created by different architecture"
E_RD0279_NO_SESSION_CONTROL_BLK:NO_GE
"unable to retrieve rdf session control block from scf"
E_RD027A_GET_ARCHINFO_ERROR:NO_GE
"error detected when calling qef for architecture info"
E_RD027B_IITABLES_NOTFOUND:E_GE7594_TABLE_NOT_FOUND
"iitables not found in local database"
/*
**    Message file for Facility code: _SC
*/


#define _SC_CLASS  9


E_SC0000_OK:NO_GE
"System Control Facility operation has been successfully completed."
E_SC0001_OP_CODE:NO_GE
"Operation code to scf_call() was not recognized."
E_SC0002_BAD_SEMAPHORE:NO_GE
"Semaphore has not been initialized or has been corrupted."
E_SC0003_INVALID_SEMAPHORE:NO_GE
"Indicated semaphore has not been initialized or has been corrupted."
E_SC0004_NO_MORE_MEMORY:NO_GE
"No more available to the server."
E_SC0005_LESS_THAN_REQUESTED:NO_GE
"Not all memory requested has been allocated."
E_SC0006_NOT_ALLOCATED:NO_GE
"Attempt to free memory which has not been allocated."
E_SC0007_INFORMATION:NO_GE
"Request for an invalid item of information."
E_SC0008_INFO_TRUNCATED:NO_GE
"Insufficient space for requested information.
The result has been truncated."
E_SC0009_BUFFER:NO_GE
"Buffer provided for information is not addressable."
E_SC000A_NO_BUFFER:NO_GE
"Request for information with no result buffer."
E_SC000B_NO_REQUEST:NO_GE
"Pointer to/portion of SCI blk not addressable."
E_SC000C_BUFFER_LENGTH:NO_GE
"Length of error message is invalid."
E_SC000D_BUFFER_ADDR:NO_GE
"Specified scf_buffer address not accessible."
E_SC000E_AFCN_ADDR:NO_GE
"AIC/PAINE address provided is not accessible."
E_SC000F_IN_USE:NO_GE
"Memory/semaphore not freed by allocator."
E_SC0010_NO_PERM:NO_GE
"Facility not authorized to use SCS_ALWAYS_MASK."
E_SC0011_NESTED_DISABLE:NO_GE
"Attempt to disable AIC's while AIC's are disabled."
E_SC0012_NOT_SET:NO_GE
"Attempt to release a semaphore which is not set."
E_SC0013_TIMEOUT:NO_GE
"Out of time before operation completed."
E_SC0014_INTERRUPTED:NO_GE
"User interrupt while awaiting completion."
E_SC0015_BAD_PAGE_COUNT:NO_GE
"Invalid (<= 0) number of pages requested."
E_SC0016_BAD_PAGE_ADDRESS:NO_GE
"Page address is improperly aligned."
E_SC0017_BAD_SESSION:NO_GE
"Session id to SCF request is neither the current session
nor the DB_NOSESSION value."
E_SC0018_BAD_FACILITY:NO_GE
"Facility id to SCF request is invalid."
E_SC0019_BAD_FUNCTION_CODE:NO_GE
"Function code is invalid."
E_SC0020_WRONG_PAGE_COUNT:NO_GE
"Page count to be freed does not match allocated quantity."
E_SC0021_BAD_ADDRESS:NO_GE
"Attemp to free memory which had not been allocated."
E_SC0022_BAD_REQUEST_BLOCK:NO_GE
"Request block to scf_call() was incorrect format."
E_SC0023_INTERNAL_MEMORY_ERROR:NO_GE
"SCF memory arena has been corrupted."
E_SC0024_INTERNAL_ERROR:NO_GE
"Internal SCF coding error."
E_SC0025_NOT_YET_AVAILABLE:NO_GE
"Function requested has not yet been implemented.
Contact the development team for scheduling information."
E_SC0026_SCB_ALLOCATE:NO_GE
"Unable to allocate SCB."
E_SC0027_BUFFER_LENGTH:NO_GE
"Invalid buffer length on SCU_INFORMATION call."
E_SC0028_SEMAPHORE_DUPLICATE:NO_GE
"Attempt to set a semaphore which is currently set by requester."
E_SC0029_HOLDING_SEMAPHORE:NO_GE
"Attempt to scd_suspend while holding a semaphore."
E_SC002B_EVENT_REGISTERED:NO_GE
"Already registered to receive event %0c.
You cannot register to receive an event for which you are already registered."
E_SC002C_EVENT_NOT_REGISTERED:NO_GE
"Event registration %0c cannot be removed because it does not exist.
You cannot remove an event registration for an event that is not currently
registered. Check that your REGISTER EVENT statement succeeded."
E_SC0100_MULTIPLE_MEM_INIT:NO_GE
"Attempt to initialize memory more than once."
E_SC0101_NO_MEM_INIT:NO_GE
"Attempt to allocate memory before initializing it."
E_SC0102_MEM_CORRUPT:NO_GE
"SCF memory arena has been corrupted."
E_SC0103_MEM_PROTECTION:NO_GE
"Error changing memory protection."
E_SC0104_CORRUPT_POOL:NO_GE
"Corrupted pool block found at %0x."
E_SC0105_CORRUPT_FREE:NO_GE
"Corrupted free block found at %0x."
E_SC0106_BAD_SIZE_REDUCE:NO_GE
"Error reducing virtual size of server."
E_SC0107_BAD_SIZE_EXPAND:NO_GE
"Error expanding virtual size of server."
E_SC0108_UNEXPECTED_EXCEPTION:NO_GE
"Unexpected exception (%0x) trapped."
E_SC0109_MULTI_DB_ADD:NO_GE
"Attempt to add database %0c to the server by session %1x.
That database has already been added to the server."
E_SC010A_RETRY:NO_GE
"Operation should be attempted again."
E_SC010B_SCB_DBCB_LINK:NO_GE
"The linkage between the database control block and the session control block
has been corrupted."
E_SC010C_DB_ADD:NO_GE
"Error adding database.  Name: %0c Owner: %1c Access Mode %2x Flags %3x"
E_SC010D_DB_LOCATION:NO_GE
"Database Location Name: %0c Physical Specification: %1c Flags: %2x"
E_SC010E_DB_DELETE:NO_GE
"Error deleting database.  Name: %0c Owner: %1c Added Id %2x"
E_SC010F_INCOMPLETE_DB_DELETE:NO_GE
"SCF Database deletion incomplete.  Name: %0c Owner: %1c"
E_SC0120_DBLIST_ERROR:NO_GE
"SCF Error while attempting known database listing."
E_SC0121_DB_OPEN:NO_GE
"Error opening database.  Name: %0c Owner: %1c Access Mode: %2x Flags %3x"
E_SC0122_DB_CLOSE:NO_GE
"Error closing database.  Name: %0c Owner: %1c"
E_SC0123_SESSION_INITIATE:NO_GE
"Error initiating session."
E_SC0124_SERVER_INITIATE:NO_GE
"Error initiating Ingres Server."
E_SC0125_POOL_ADDITION:NO_GE
"SCF Error adding memory pool."
E_SC0126_SC0M_CHECK:NO_GE
"Error discovered in SCF Memory."
E_SC0127_SERVER_TERMINATE:NO_GE
"Error terminating Server."
E_SC0128_SERVER_DOWN:NO_GE
"Ingres Server Normal Shutdown."
E_SC0129_SERVER_UP:NO_GE
"Ingres Release %0c Server -- Normal Startup."
E_SC012A_BAD_MESSAGE_FOUND:NO_GE
"Unable to format message number %0x."
E_SC012B_SESSION_TERMINATE:NO_GE
"Error terminating Session."
E_SC012C_ADF_ERROR:NO_GE
"Error encountered initiating ADF."
E_SC012D_DB_NOT_FOUND:NO_GE
"Requested Database not found in SCF's known db list. "
E_SC012E_QEP_INVALID:NO_GE
"Database reorganization activity has rendered the
query plan invalid.  Try again after the db activity is complete.  In case
of dynamic SQL, the statement must be prepared again. "
E_SC012F_RETRY:NO_GE
"Query plan has been rendered invalid.  Retry."
E_SC0200_SEM_INIT:NO_GE
"Error initializing semaphore."
E_SC0201_SEM_WAIT:NO_GE
"Error waiting for semaphore."
E_SC0202_SEM_RELEASE:NO_GE
"Error releasing semaphore."
E_SC0203_SEM_FREE:NO_GE
"Error freeing semaphore."
E_SC0204_MEMORY_ALLOC:NO_GE
"Error allocating memory."
E_SC0205_BAD_SCS_OPERATION:NO_GE
"Sequencer called incorrectly."
E_SC0206_CANNOT_PROCESS:E_GE94D4_HOST_ERROR
"An internal error prevents further processing of this query.
Please contact the INGRES system administrator."
E_SC0207_UNEXPECTED_ERROR:NO_GE
"Facility returned an undocumented error."
E_SC0208_BAD_PSF_HANDLE:NO_GE
"QSF Handle returned was unusable."
E_SC0209_PSF_REQ_DISTRIBUTED:NO_GE
"PSF requested a distributed query mode."
E_SC020A_UNRECOGNIZED_QMODE:NO_GE
"The Query Mode %0d is unknown."
E_SC020B_UNRECOGNIZED_TEXT:NO_GE
"Text is '%0c'."
E_SC020C_INPUT_OUT_OF_SEQUENCE:NO_GE
"Sequencer received unrequested input."
E_SC020D_NO_DISTRIBUTED:NO_GE
"Distributed operations are not supported in this server."
E_SC020E_UNUSED_MSG:NO_GE
"unused"
E_SC020F_LOST_LOCK:NO_GE
"The object lock for the input text is unavailable."
E_SC0210_SCS_SQNCR_ERROR:NO_GE
"Sequencer Internal Error."
E_SC0211_CLF_ERROR:NO_GE
"Error returned by CLF."
E_SC0212_ADF_ERROR:NO_GE
"Error returned by ADF."
E_SC0213_DMF_ERROR:NO_GE
"Error returned by DMF."
E_SC0214_OPF_ERROR:NO_GE
"Error returned by OPF."
E_SC0215_PSF_ERROR:NO_GE
"Error returned by PSF."
E_SC0216_QEF_ERROR:NO_GE
"Error returned by QEF."
E_SC0217_QSF_ERROR:NO_GE
"Error returned by QSF."
E_SC0218_RDF_ERROR:NO_GE
"Error returned by RDF."
E_SC0219_SCF_ERROR:NO_GE
"Error returned by SCF."
E_SC021A_ULF_ERROR:NO_GE
"Error returned by ULF."
E_SC021B_BAD_COPY_STATE:NO_GE
"Saved COPY state is invalid."
E_SC021C_SCS_INPUT_ERROR:NO_GE
"SCS_INPUT internal error."
E_SC021D_TOO_MANY_CURSORS:E_GE8D68_CONFIG_ERROR
"Number of open cursors exceeds the local limit."
E_SC021E_CURSOR_NOT_FOUND:E_GE9DD0_CUR_STATE_INV
"Specified cursor is not known to the server."
E_SC021F_BAD_SQNCR_CALL:NO_GE
"Internal Error:  Call to sequencer is incorrect."
E_SC0220_SESSION_ERROR_MAX:E_GE8D68_CONFIG_ERROR
"Too many errors for this session. "
E_SC0221_SERVER_ERROR_MAX:NO_GE
"Error count for server has been exceed."
E_SC0222_BAD_SCDNOTE_STATUS:NO_GE
"Bad status passed to SCD_NOTE() --\n
        Status was %0x (%0d), facility was %1x (%1x)."
E_SC0223_BAD_SCDNOTE_FACILITY:NO_GE
"Invalid facility code was passed to scd_note() --\n
        Status was %0x (%0d), facility was %1x (%1x)."
E_SC0224_MEMORY_FREE:NO_GE
"Unable to deallocate SCF memory."
E_SC0225_BAD_GCA_INFO:NO_GE
"Unable to get information about a GCA block."
E_SC0226_BAD_GCA_TYPE:NO_GE
"GCA block has an invalid block type -- %0x (%0d.)."
E_SC0227_MEM_NOT_FREE:NO_GE
"Facility %0x had not freed session memory at end of session."
E_SC0228_INVALID_TRACE:NO_GE
"Invalid SCF trace flag specified."
E_SC0229_INVALID_ATTN_CALL:NO_GE
"Invalid AIC delivered to SCF session."
E_SC022A_SERVER_LOAD:E_GE8CA0_SYSTEM_LIMIT
"Insufficient system/server resources to complete the current query.
Consult the system administrator and/or error log."
E_SC022B_PROTOCOL_VIOLATION:NO_GE
"Improper GCA block type (or connection block) was sent."
E_SC022C_PREMATURE_MSG_END:NO_GE
"Internal Protocol Error: Message type %0x (%0d.) ended with a dmm value of %1d."
E_SC022D_BLOCK_NOT_MD_ASSOC:NO_GE
"Internal Protocol Error: First communication block from a new session was type %0x (%0d.).  Must be GCA_MD_ASSOC (2)."
E_SC022E_WRONG_BLOCK_TYPE:NO_GE
"Internal Protocol Error: SCF received block type %0x (%0d.) when expecting type %1x (%1d.)."
E_SC022F_BAD_GCA_READ:NO_GE
"SCF was unable to read a GCF block."
E_SC0230_INVALID_CHG_PROT_LEN:NO_GE
"A protection block was sent with a length (%0d.) not equal to 1."
E_SC0231_INVALID_CHG_PROT_CODE:NO_GE
"A protection block was sent with an invalid code of %0x (%0d.).  This code must be GCA_SUPSYS."
E_SC0232_INVALID_STARTUP_CODE:NO_GE
"A startup code(index %0x (%0d.), value %1x (%1d.)) was sent which is
unknown to the server.  The server not start."
E_SC0233_SESSION_START:NO_GE
"Session created for %0c (real: %1c) on database %2c for %3c%4c access."
E_SC0234_SESSION_END:NO_GE
"Session ended."
E_SC0235_AVERAGE_ROWS:NO_GE
"On %0d. select/retrieve statements, the average row count returned was %1d."
E_SC0236_FAST_COMMIT_INVALID:NO_GE
"Server must be started with SOLE_SERVER option in order to use Fast Commit."
E_SC0237_FAST_COMMIT_ADD:NO_GE
"Error adding Fast Commit thread to the server."
E_SC0238_FAST_COMMIT_EXIT:NO_GE
"Fast Commit Thread terminated abnormally."
E_SC0239_DBMS_TASK_ERROR:NO_GE
"Internal error initializing a DBMS server task - task type (%0d.) not recognized."
E_SC023A_GCA_LISTEN_FAIL:NO_GE
"Unable to start sessions due to excessive consecutive GCA_LISTEN failures.\n
This server will no longer accept new session requests,\n
and will shut down when all user sessions disconnect."
/*%
** Parameters: (none)
**
** System Status:
**	The server enters normal shutdown processing.
**	No new sessions will be accepted, and the server will terminate
**	when all active user sessions end.
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC023B_INCONSISTENT_MSG_QUEUE:NO_GE
"The GCA message queue for this session was inconsistent.\n
All messages for this session will be discarded."
/*%
** Parameters: (none)
**
** System Status:
**	The GCA message queue for the thread was found to be inconsistent.
**	All messages on the queue are discarded.
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC0240_DBMS_TASK_INITIATE:NO_GE
"Internal error initializing a session for a DBMS server task - Server Task
name '%0c'."
E_SC0241_VITAL_TASK_FAILURE:NO_GE
"A Server Task thread necessary to the server has terminated forcing the
shutdown of the DBMS server - Server Task name '%0c'."
E_SC0242_ALTER_MAX_SESSIONS:NO_GE
"Error altering Max Sessions count to allow for DBMS tasks - old count %0d, new
count %1d."
E_SC0243_WRITE_BEHIND_ADD:NO_GE
"Error adding Write Behind thread to the server."
E_SC0244_WRITE_BEHIND_EXIT:NO_GE
"Write Behind Thread terminated abnormally."
E_SC0245_CANNOT_REGISTER:NO_GE
"Unable to start server, failed to register with the nameserver."
E_SC0246_SCA_SHOW_ERROR:NO_GE "Error performing DMF show operation on table %0c."
/*%
** Parameters:
**      %0c Table on which the error occurred.
**
** System Status:
**      Server will come up, but with no user defined datatypes defined.
**	This error occurs when the system is unable to correctly find all of the
**	user defined abstract datatype catalogs in the IIDBDB.  The actual error
**	will be contained in the [possibly sequence of] DMF error[s] logged just
**	before this message (for this server).
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC0247_SCA_OPEN_ERROR:NO_GE "Error performing DMF open operation on table %0c."
/*%
** Parameters:
**      %0c Table on which the error occurred.
**
** System Status:
**      Server will come up, but with no user defined datatypes defined.
**	This error occurs when the system is unable to correctly open all of the
**	user defined abstract datatype catalogs in the IIDBDB.  The actual error
**	will be contained in the [possibly sequence of] DMF error[s] logged just
**	before this message (for this server).
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC0248_SCA_POSITION_ERROR:NO_GE "Error performing DMF position operation on table %0c."
/*%
** Parameters:
**      %0c Table on which the error occurred.
**
** System Status:
**	Server will come up, but with no user defined datatypes defined.  This
**	error occurs when the system is unable to correctly position itself
**	within the named table.  The actual error will be contained in the
**	[possibly sequence of] DMF error[s] logged just before this message
**	(for this server).
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC0249_SCA_READ_ERROR:NO_GE "Error performing DMF read operation on table %0c."
/*%
** Parameters:
**      %0c Table on which the error occurred.
**
** System Status:
**	Server will come up, but with no user defined datatypes defined.  This
**	error occurs when the system is unable to correctly read all entries in
**	the user defined abstract datatype catalog mentioned in the IIDBDB.
**	The actual error will be contained in the [possibly sequence of] DMF
**	error[s] logged just before this message (for this server).
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC024A_SCA_CLOSE_ERROR:NO_GE "Error performing DMF close operation on table %0c."
/*%
** Parameters:
**      %0c Table on which the error occurred.
**
** System Status:
**      Server will come up, but with no user defined datatypes defined.
**	This error occurs when the system is unable to correctly close the named
**	user defined abstract datatype catalog in the IIDBDB.  The actual error
**	will be contained in the [possibly sequence of] DMF error[s] logged just
**	before this message (for this server).
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC024B_SCA_ALLOCATE:NO_GE "Error allocating memory for user defined datatype initialization."
/*%
** Parameters:
**      None
**
** System Status:
**      DBMS server will start up, but no user defined datatypes will be
**	accessible.  This error occurs as a result of failures while attempting
**	to allocate memory from the System Control Facility (SCF) memory
**	management system.  More detailed error messages concerning the precise
**	failure precede this message in the error log.
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC024C_SCA_DEALLOCATE:NO_GE "Error deallocating memory for user defined datatype initialization"
/*%
** Parameters:
**      None
**
** System Status:
**      DBMS Server will start up, although other errors may follow.
**	This error occurs as a result of failures attempting to deallocate
**	memory which is no longer needed after datatype initialization.  Coding
**	errors are the probably cause.  More detailed error messages concerning
**	the precise failure precede this message in the INGRES error log.
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC024D_SCA_ADDING:NO_GE "User Defined Datatype Initialization:\n
       Adding %0d. new datatype entries,\n
              %1d. new datatypes,\n
	      %2d. new operators,\n
         and  %3d. new function instances."
/*%
** Parameters:
**      %0d Integer representing total number of user defined additions
**	%1d Integer representing total number of datatype additions
**	%2d Integer representing total number of operator additions
**	%3d Integer representing total number of function instances additions.
**
** System Status:
**      System operating normally.  This is a status message only, informing the
**	system operator of the new additions to the system.
**
** User Action:
**      None necessary.  All is operating normally.
*/
E_SC024E_SCA_NOT_ADDED:NO_GE "User Defined Datatypes not added.  See previous errors."
/*%
** Parameters:
**      None
**
** System Status:
**      DBMS server will start up.  No user defined datatype operations will be
**	available.  This message is an overall status.  Other error messages
**	will have preceded this message in the error log.  These contain the
**	detailed cause of the problem.
**
** User Action:
**      Call Site Technical Contact.  A call to the product vendor is probably
**	necessary.
*/
E_SC024F_SCA_IMP_ARGUMENTS:NO_GE "Number of arguments in IIFI_ARGUMENTS (%0d.)\n
does not agree with the number found in IIFI_ARGUMENTS (%1d.)\n
for function instance %2d.."
/*%
** Parameters:
**	%0d Number of arguments found in the IIFI_ARGUMENTS table
**	%1d Number specified as the correct number in IIFUNCTION_INSTANCES
**	%2d Function instance ID number
**
** System Status:
**      DBMS Server will start up.  No user defined datatypes will be defined,
**	since the catalogs are inconsistent.  This message indicates that
**	datatype catalogs are inconsistent, either from errors at definition
**	time, or from other, unknown causes.
**
** User Action:
**      Call Site Technical Contact.  The product vendor should be notified to
**	determine further action.  Other out of the ordinary events in the
**	database database should be noted, as well as hardware or other site
**	machine problems (disk problems, etc.).
**
**	The function instance in question should be dropped and recreated using
**	the standard SQL statement for this purpose.
*/
E_SC0250_COPY_OUT_OF_SEQUENCE:NO_GE
"A COPY data block was received when one was not expected (or not received when expected)."
E_SC0251_COPY_CLOSE_ERROR:NO_GE
"Error ending the COPY statement."
E_SC0252_COPY_SYNC_ERROR:NO_GE
"Synchronization error with GCA COPY protocol."
E_SC0253_BAD_COPY_ERRMSG:NO_GE
"Error message for COPY statement not found."
E_SC0254_SERVER_INIT_ADD:NO_GE "Error adding the DBMS server initialization thread."
/*%
** Parameters:
**      None
**
** System Status:
**      DBMS server will not start up.  The server initialization task is
**	necessary to get the server fully operational.
**
** User Action:
**      Call Site Technical Contact.  A call should be made to the product
**	vendor to determine further action.
*/
E_SC0255_INCOMPLETE_SEND:E_GE80E8_LOGICAL_ERROR
"SCC_SEND:  Found terminating message type %0d. when type %1d. was necessary to
complete an incomplete message."
/*%
** Parameters:
**      Terminating message type -- the message found which will turn the line
**			    around.
**	Necessary message type -- the message type expected.
**
** System Status:
**      The session may continue.  However, the current query will fail,
**	probably with a frontend protocol failure.
**
**	The system has detected an internal protocol error.
**
** User Action:
**      If this has happened with no interrupts or force abort conditions, it
**	should be reported to the product vendor.  Call you Site Technical
**	Contact.
*/
E_SC0260_XENCODE_ERROR:NO_GE
"Error encountered during encryption."
/*%
** Parameters: (none)
**
** Explanation:
**	scs_dbdb_info() got a bad return code from scu_xencode()
**	when attempting to encode a role password
**
** System Status:
**	For super users, a message is logged, and the session
**	continues with the role in effect.
**	For non-super users, a message is logged and returned,
**	and the session is terminated.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_SC0261_XENCODE_BAD_PARM:NO_GE
"Invalid parameter received by scu_xencode."
/*%
** Parameters: (none)
**
** Explanation:
**	scu_xencode() received an invalid password as input.
**	Either the pointer to it was null, it was too short,
**	too long, or it contained all blanks.
**
** System Status:
**	This status is returned to either psy_[ca]aplid or scs_dbdb_info.
**	The caller will issue a traceback message and determine disposition.
**
** User Action:
**	Call Ingres Corp. Technical Support
**	scu_xencode() should never see blank passwords.
*/
E_SC0262_XENCODE_BAD_RESULT:NO_GE
"Encryption did not function properly"
/*%
** Parameters: (none)
**
** Explanation:
**	After encoding a password, scu_encode() detected that
**	the resultant string had not been altered.
**
** System Status:
**	This status is returned to either psy_[ca]aplid or scs_dbdb_info.
**	The caller will issue a traceback message and determine disposition.
**
** User Action:
**	Call Ingres Corp. Technical Support
**
*/
E_SC0263_SCA_RISK_CONSISTENCY:NO_GE "User datatype registration risks consistency.
Databases accessed in this installation may become inconsistent and/or lose data."
/*%
** Parameters:
**      None.
**
** System Status:
**	The datatypes being registered for use by this server may not be known
**	throughout the installation.  Normally, this is illegal.  However, the
**	user supplied exit has requested that this check be overridden;  this
**	means that the recovery system may not understand the format of the data
**	in use.  Consequently, it may not be able to correctly recover any data
**	for this installation (in the cases where the data formats disagree
**	enough to cause a major failure).
**
**	The server may start up (barring other errors).  However, all databases
**	in this installation now risk a lack of proper recovery.
**
** User Action:
**      None.  This is an informational message.  This request (for risking
**	consistency) is intended for use during the test of datatype code, and
**	then only in a test installation.  As soon as the system moves to
**	production status, this request should be changed.
*/
E_SC0264_SCA_ILLEGAL_MAJOR:NO_GE "Illegal User Datatype Major id (%0d./%0x).  Major ID
must be greater than zero (0)."
/*%
** Parameters:
**      %0d/x -- User defined datatype id
**
** System Status:
**      The system found an illegal value for a major id.  It is display as both
**	decimal (followed by a '.') and hex.  The only correct values are
**	greater than 0.
**
**	The system will startup up, ignoring the user defined datatypes.
**
** User Action:
**      Correct the value and retry.
*/
E_SC0265_SCA_STATE:NO_GE "User define datatype identity: Major %0d./%0x -- Minor
%1d./%1x -- <%2c>"
/*%
** Parameters:
**      %0 -- User defined major id
**	%1 -- User defined minor id
**	%2 -- User defined string
**
** System Status:
**      The system is attempting to add the specified user datatypes.  This
**	message is informational -- providing the user supplied information.
**
** User Action:
**      None necessary -- this is an informational message only.
*/
E_SC0266_USER_SHUTDOWN:NO_GE "User requested shutdown on datatype initialization
failure.  Server shutting down."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system has failed to correctly add the user supplied datatypes (for
**	reasons listed previously in the log file).  The user supplied
**	information requests that the server not startup if this occurs.  The
**	server will shutdown (i.e. not complete startup).
**
** User Action:
**      Correct the problem and retry.
*/
E_SC0267_SCA_ID_UNKNOWN:NO_GE "System cannot determine the known ADF identification.
Original ADF is assumed."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system cannot determine the User defined datatype level.  The system
**	will assume that no user defined datatypes have been added.  Further
**	action will be based upon that assumption.
**
** User Action:
**      Correct the problem.  This is probably some sort of installation
**	problem;  perhaps a portion of the recovery system has been killed.
*/
E_SC0268_SCA_RISK_INVOKED:NO_GE "User datatypes are now in use which may compromise
installation consistency.  Data may be lost."
/*%
** Parameters:
**      None.
**
** System Status:
**      The system is approving the use of datatypes based upon the request to
**	override the major and minor id's in use by the installation.  See
**	message E_SC0263_RISK_CONSISTENCY for more details upon the risk.  Data
**	in this installation may be lost and/or databases may be marked
**	inconsistent.
**
** User Action:
**      This is an informational message.  No action is necessary.  However, it
**	is advisable to remedy this situation as soon as possible, making the
**	installation aware of the datatypes in use and consistent.
*/
E_SC0269_SCA_INCOMPATIBLE:NO_GE "The proposed datatype id is incompatible with the
installation level <%0d./%0x,%1d./%1x>.  The server will shutdown."
/*%
** Parameters:
**      %0 -- Major id of datatypes in use in the installation.
**	%1 -- Minor id ...
**
**	Note <0x80000000,0> is the INGRES default.
**
** System Status:
**      The system cannot use the datatypes being proposed, as they are
**	incompatible with the installation level.  The server will shutdown.
**
** User Action:
**      Correct and restart the installation.
**
**	Alternatively, the add_risk_consistency flag can be set, but only if
**	this is a test installation.  SETTING THIS FLAG RISKS THE CONSISTENCY OF
**	DATA THROUGHOUT THE INSTALLATION.
*/
E_SC026A_SCA_REGISTER_ERROR:NO_GE "The IIUDADT_REGISTER routine returned a non-zero:NO_GE
status."
/*%
** Parameters:
**      None.
**
** System Status:
**	The system is attempting to define user datatypes.  In so doing, it
**	calls the IIUDADT_REGISTER() routine, to obtain the definitions.  This
**	routine has returned a non-zero status, which is viewed as an error.
**	New datatypes will not be defined.  The system will come up with the
**	vendor supplied defaults.
**
** User Action:
**      Correct the error in the IIUDADT_REGISTER() routine.
*/

E_SC0270_NO_EVENT_MESSAGE:NO_GE
"Invalid GCA protocol level for event registration."
/* Mapped to E_QE019A_EVENT_MESSAGE for users */

E_SC0271_EVENT_THREAD:NO_GE
"The SCF alert subsystem event thread has been altered.
The operation code is %0d (0 = REMOVE, 1 = ADD, 2 = MODIFY)."
/*%
** Parameters:
**	%0 - Operation code corresponding to action.
**
** System Status:
**	A system command has been given to alter the state of the event
**	thread.  This message is not an error, but may be used to resolve
**	event-related errors that are encountered later on.
**
** User Action:
**	This is not an error.
*/
E_SC0272_EVENT_THREAD_ADD:NO_GE
"Error adding event thread to SCF alert system."
/*%
** Parameters:
**	None
**
** System Status:
**	SCF could not initialize the event thread.
**
** User Action:
**	If your application does not require event processing you may ignore
**	this error message.
*/
E_SC0273_EVENT_RETRY:NO_GE
"The SCF alert system has failed to connect to the Event Subsystem.
After %0d connect attempts SCF has given up."
/*%
**  Parameters:
**	%0	Number of attempts at connecting to the Event Subsystem.
**
**  System State:
**	The event subsystem (EV), when connected to, must allow multiple
**	servers to "attach" themselves to the shared memory segment
**	associated with EV.  The current server was not able to attach
**	to the segment because EV has locked out any other servers.
**	This may be because the shared EV segment is currently being taken
**	down or initialized.
**	
**  User Action:
**	If there are many running servers on your machine find out if any
**	of them are current just starting or going down.  A server that
**	could not attach to EV will not allow event processing.
*/
E_SC0274_NOTIFY_EVENT:NO_GE
"An internal error occurred while trying to notify the application of
an event.  The DBMS alert system attempted to notify you, but found
that you had no queued alert instances."
/*%
**  Parameters:
**	None
**
**  System State:
**	The alert system attempted to notify the client that an event for
**	which the client is registered was raised.  This failure was due to
**	some form of internal flow failure, where an alert resource was
**	lost when the client was about to receive it.
**	
**  User Action:
**	Apply normal error handling procedures as the event notification has
**	been discarded.
*/
E_SC0275_IGNORE_ASTATE:NO_GE
"An internal error occurred while trying to read more data from the 
application.  The state of alerts was set to SCS_ASIGNORE.  This state
indicates to ignore GCA completion routines, which will not enable the program
to continue."
/*%
**  Parameters:
**	None
**
**  System State:
**	When SCF was about to read more data from the client it noticed that
**	the alert state was set to SCS_ASIGNORE.  This state means that when
**	the read operation would complete it would not have resumed the
**	current session.  Consequently, the current session is terminated.
**	
**  User Action:
**	Report this problem to the system administrator.  If the DBMS
**	event subsystem was taken down while your application was running
**	then ignore this error.  Otherwise report this error to the product
**	vendor.
*/
E_SC0280_NO_ALERT_INIT:NO_GE
"The SCF alert subsystem has not been initialized.
This is most likely caused by a server initiation error.
Check the server event log for initialization errors."
/*%
** Parameters:
**	None.
**
** System Status:
**	SCF is attempting to register, remove or raise an alert(event)
**	but the SCF event subsystem has not been initialized. This can only
**	occur if SCF initialization encountered a memory allocation error
**	or an EV connect error.
**
** User Action:
**	All initialization errors are detailed in the server error log.
**	Determine which server the user is currently using and check that
**	server's error log for initialization time errors. The action
**	to take will depend on the error discovered.
*/
E_SC0281_ALERT_ALLOCATE_FAIL:E_GE8D04_NO_RESOURCE
"Memory could not be allocated to register the user event.
This error is caused by too many alerts currently being registered.
Check that users are properly using events or reconfigure the server
with more SCF memory."
/*%
**  Parameters:
**	None
**
**  System State:
**	SCF could not allocate an SCE_ALERT block to contain the event
**	registration requested by the user. This will occur if the users
**	have allocated too many event registrations or have raised a
**	sufficiently large number of events that have not been
**	transmitted to the target clients. Both types of operations require
**	SCF memory. It is also possible that other SCF memory allocations
**	have reduced the amount of SCF memory available.
**
**  User Action:
**	Check your application programs to be sure they are not registering
**	an inordinate number of events or raising a large number of events
**	that are not being sent to the client applications. Either correct
**	these applications or allocate more SCF memory.
*/
E_SC0282_RSES_ALLOCATE_FAIL:E_GE8D04_NO_RESOURCE
"Memory could not be allocated to register the user event.
This error is caused by too many alerts currently being registered.
Check that users are properly using events or reconfigure the server
with more SCF memory."
/*%
**  Parameters:
**	None
**
**  System State:
**	SCF could not allocate an SCE_RSESSION block to contain the event
**	registration requested by the user. This will occur if the users
**	have allocated too many event registrations or have raised a
**	sufficiently large number of events that have not been
**	transmitted to the target clients. Both types of operations require
**	SCF memory. It is also possible that other SCF memory allocations
**	have reduced the amount of SCF memory available.
**
**  User Action:
**	Check your application programs to be sure they are not registering
**	an inordinate number of events or raising a large number of events
**	that are not being sent to the client applications. Either correct
**	these applications or allocate more SCF memory.
*/
E_SC0283_AINST_ALLOCATE_FAIL:E_GE8D04_NO_RESOURCE
"Memory could not be allocated to raise an event.
The registered client (%0x) will not be notified of event '%1c'.
This error is caused by exhausting SCF memory.
Check that users are properly using events or reconfigure the server
with more SCF memory."
/*%
**  Parameters:
**	%0	Session id for the session that cannot be notified.
**	%1	Name of the event that cannot be raised for this session
**
**  System State:
**	SCF could not allocate an SCE_AINST block to contain the event
**	notification requested by the user. This will occur if the users
**	have allocated too many event registrations or have raised a
**	sufficiently large number of events that have not been
**	transmitted to the target clients. Both types of operations require
**	SCF memory. It is also possible that other SCF memory allocations
**	have reduced the amount of SCF memory available.
**
**  User Action:
**	Check your application programs to be sure they are not registering
**	an inordinate number of events or raising a large number of events
**	that are not being sent to the client applications. Either correct
**	these applications or allocate more SCF memory.
*/
E_SC0284_BUCKET_INIT_ERROR:NO_GE
"The SCF event handling data structures cannot be allocated because the
user specified %0d event buckets; this value is out of bounds.
Restart the server changing the invocation parameter 'events' to a valid value."
/*%
**  Parameters:
**	%0	Number of buckets requested by the user
**
**  System State:
**	The server event thread would not start because the specified parameter
**	for the number of event buckets is out of bounds. This value must be
**	larger than zero and less than some large but environment specific
**	value. This parameter only affects the number of hash buckets into
**	which EVENT registrations are placed so reducing this value should
**	have little effect on the operation of the system.
**
**  User Action:
**	Either remove this option from the server initialization command,
**	make the value positive (if < 0) or make the value smaller
**	(if very large). Sanity check: values larger than 10,000 for this
**	parameter are thought to make little sense.
*/
E_SC0285_EVENT_MEMORY_ALLOC:NO_GE
"The SCF event handling data structures could not be allocated.
A request was made to allocate %0d bytes (%1d alert buckets).
This error is caused by exhausting SCF memory.
Restart the server with fewer event buckets (or users) or with more SCF memory."
/*%
**  Parameters:
**	%0	Total memory (in bytes) required for alert structures
**	%1	Number of buckets requested for server (or number of users)
**
**  System State:
**	The server event thread would not start because the SCF alert data
**	structures could not be allocated.
**
**  User Action:
**	Check what other threads are running and that they are not (already)
**	consuming a lot of SCF memory.  Try to reconfigure your server with
**	more SCF memory.
*/
E_SC0286_EVENT_SEM_INIT:NO_GE
"The SCF event handling data structures could not be initialized.\
A request to initialize semaphore '%0c' failed.
Try to restart the server if event processing is required."
/*%
**  Parameters:
**	%0	Internal name of semaphore for SCF alert structures.
**
**  System State:
**	The server event thread would not start because the SCF alert semaphore
**	could not be initialized.
**
**  User Action:
**	Check what other threads are running and that they are not (already)
**	consuming a lot of system locks.  If event processing is not required 
**	then this error can be ignored.  Otherwise try to restart the server.
*/
E_SC0287_AINST_SESSION_ID:NO_GE
"The SCF event thread found a non-existing session, %0x, registered for an event."
/*%
** Parameters:
**	%0 	Session id of registered session
**
** System Status:
**	SCF is attempting to notify a client of the occurrence of an event.
**	The client's session id, however, indicates that the client is no
**	longer active.  This could occur if a user thread was removed through
**	unusual methods and consequently did not remove all of its
**	registrations.  The system continues processing the event for other
**	registered sessions.
**
** User Action:
**	This error is entered into the log file and does not affect user
**	processing.
*/
E_SC0288_XSEV_SM_DESTROY:NO_GE
"Event Subsystem: Shared memory segment '%0c' could not be destroyed.
During server initialization or shutdown an attempt to destroy the
shared event segment failed."
/*%
**  Parameters:
**	%0	Internal name of shared event segment
**
**  System State:
**	The event subsystem (EV) may attempt to destroy the EV shared segment
**	if an EV initialization failed or when the last server disconnects
**	from EV.  If the segment cannot be destroyed other servers will not
**	be able to connect to a refreshed EV.
**
**  User Action:
**	Check if other servers are starting up or shutting down at the exact
**	same moment.  If so try to start up a new server (to refresh EV).
**	This error may be ignored if your server will not be processing
**	events on behalf of applications.  Otherwise report this problem to
**	the product vendor.
*/
E_SC0289_XSEV_ALLOC:NO_GE
"Event Subsystem: Shared memory segment could not be allocated.
An attempt to allocate %0d shared pages failed."
/*%
**  Parameters:
**	%0	Number of pages attempted to allocate for EV.
**
**  System State:
**	The event subsystem (EV) when initialized, attempts to allocate 
**	a number of shared memory pages.  The number of pages is a function
**	of the number of users and event buckets of the the first server that
**	connects to EV.
**
**  User Action:
**	If the number of pages is too high try to reduce the event-related
**	parameters to the "starting server".  This error may be ignored if
**	your server will not be processing events on behalf of applications.
**	Otherwise report this problem to the product vendor.
*/
E_SC028A_XSEV_CRLOCKLIST:NO_GE
"Event Subsystem: Event lock list could not be created (LKcreate_list).
The server could not be connected to the event subsystem."
/*%
**  Parameters:
**	None
**
**  System State:
**	The event subsystem (EV), when initialized and when connected to,
**	creates a "lock list" on behalf of the connecting server.  This
**	list allows multiple servers to be notified (through locks) of events
**	that they should process from other servers (through EV).  If the
**	lock list could not be created then the server cannot be notified.
**	The system, therefore, does not allow any interaction between this
**	server and events.
**	
**  User Action:
**	This error may be ignored if your server will not be processing
**	events on behalf of applications.  Otherwise report this problem to
**	the product vendor.
*/
E_SC028B_XSEV_ATTACH:NO_GE
"Event Subsystem: Unsuccessful attempt to attach to established subsystem.
The server could not attached to shared memory segment '%0c'."
/*%
**  Parameters:
**	%0	Internal name of shared event subsystem segment
**
**  System State:
**	The event subsystem (EV), when connected to, must allow multiple
**	servers to "attach" themselves to the shared memory segment
**	associated with EV.  The current server was not able to attach
**	to the segment.  The system, therefore, does not allow any interaction
**	between this server and events.
**	
**  User Action:
**	If there are many running servers on your machine find out if they
**	are all using events.  Maybe some of them can be initialized without
**	events (see the "events" startup qualifier).  This error may be
**	ignored if your server will not be processing events on behalf of
**	applications.  Otherwise report this problem to the product vendor.
*/
E_SC028C_XSEV_EALLOCATE:NO_GE
"Event Subsystem: Unsuccessful attempt to allocate data structure.
The data structure '%0c' (requiring %1d bytes) could not be allocated.
There are only %2d available bytes left in the subsystem memory pool."
/*%
**  Parameters:
**	%0	Internal name of data structures being allocated.
**	%1	Size of data structure.
**	%2	Available space (in bytes) of shared memory pool.
**
**  System State:
**	The event subsystem (EV) could not execute an operation because
**	all the shared EV space was exhausted.  If there are many applications
**	running with many events then the EV subsystem may have been initialized
**	with too small an amount (determined as a function of the number of
**	users and event buckets of the the first server that connected to EV).
**	Depending on when this error was encountered it may affect one
**	application of the whole server.
**	event.
**
**  User Action:
**	If there are many running servers on your machine find out if they
**	are all using events.  Maybe some of them can be initialized without
**	events (see the "events" startup qualifier).  This error may be
**	ignored if your server will not be (or is not already) processing
**	events on behalf of applications.  Otherwise report this problem to
**	the product vendor.
*/
E_SC028D_XSEV_REGISTERED:NO_GE
"Event Subsystem: Invalid attempt by server to re-register an event.
This server is already registered for event '%0c' or for ALL events."
/*%
**  Parameters:
**	%0	Full internal name of event (with blanks).
**
**  System State:
**	The event subsystem (EV) was asked to register a server for an event
**	for which the server is already registered.  It is an error to register
**	twice.  If the server has registered for ALL events then it may not
**	register for a specific event.
**
**  User Action:
**	If the request to register the event WITHIN the server works (i.e.
**	an application will be notified of the event), then you can ignore
**	this error.  Otherwise this will impact the behavior of the application
**	and should be reported to the product vendor.
*/
E_SC028E_XSEV_NOT_REGISTERED:NO_GE
"Event Subsystem: Invalid attempt by server to de-register an event.
This server is not registered for event '%0c'."
/*%
**  Parameters:
**	%0	Full internal name of event (with blanks).
**
**  System State:
**	The event subsystem (EV) was asked to de-register a server for an event
**	for which the server is not (yet) registered.  It is an error to
**	de-register a non-existing event.
**
**  User Action:
**	If the request to remove the event WITHIN the server works (i.e.
**	an application will NOT be notified of the event), then you can ignore
**	this error.  Otherwise this will impact the behavior of the application
**	and should be reported to the product vendor.
*/
E_SC0290_XSEV_SIGNAL:NO_GE
"Event Subsystem: An event could not be broadcast to server 0x%0x.
An LKevent call failed to broadcast event '%1c' to the registered server."
/*%
**  Parameters:
**	%0	Process id of server registered for the event
**	%1	Full internal name of event (includes blanks).
**
**  System State:
**	The event subsystem (EV) attempted to raise an event to all servers
**	registered to receive it.  When signaling s specific registered
**	server EV encountered an LKevent error.  This is usually an error
**	on the senders side and not the recipients side.  This event will
**	NOT be added to the registered server's event instance queue, with
**	the consequence that registered applications will not receive this
**	event.
**
**  User Action:
**	This error may be ignored if your server will not be processing
**	events on behalf of applications.  Otherwise report this problem to
**	the product vendor.
*/
E_SC0291_XSEV_NOT_CONNECT:NO_GE
"Event Subsystem: Invalid event request from server 0x%0x.
An attempt to execute the '%1c' operation failed because the server
is not connected to the subsystem."
/*%
**  Parameters:
**	%0	Process id of server executing the operation.
**	%1	Internal name of requested operation.
**
**  System State:
**	The event subsystem (EV) may only interact with servers that have
**	formally connected to EV.  A server that did not formally (or 
**	successfully) connect may not execute any operations against EV
**	(even disconnecting - though the disconnect error may be ignored).
**
**  User Action:
**	If this same server had event-related initialization errors (earlier
**	in the error log file) then ignore this subsequent error.  Otherwise
**	report this problem to the product vendor.
*/
E_SC0292_XSEV_SEM_INIT:NO_GE
"Event Subsystem: Unsuccessful attempt to initialize the subsystem.
A request to initialize the cross-process semaphore '%0c' failed.
Try to restart the server if event processing is required."
/*%
**  Parameters:
**	%0	Internal name of semaphore.
**
**  System State:
**	The event subsystem (EV) could not be initialized because an attempt
**	to allocate/clear cross-process semaphores (that control shared memory
**	data structures) failed.
**
**  User Action:
**	This error may be ignored if your server will not be processing
**	events on behalf of applications.  Otherwise report this problem to
**	the product vendor.
*/
E_SC0293_XSEV_VERSION:NO_GE
"Event Subsystem: Unsuccessful attempt to attach to the subsystem.
Event subsystem '%0c' was established with structural version %1d.
The current server is attempting to connect with version %2d.
All event subsystems in a single installation must be the same version."
/*%
**  Parameters:
**	%0	Internal name of shared event subsystem segment.
**	%1	Version of already-established event subsystem.
**	%2	Event-connection version of server attempting to connect.
**
**  System State:
**	The current server could not connect to the event subsystem (EV)
**	because the servers EV version did not match the already-established
**	version of EV.  Version numbers are modified when internal changes are
**	made to the EV data structures (all allocated out of memory shared
**	among multiple servers).  Normally all servers in an installation are
**	of the same level.  Internal developers and VARS may have installations
**	constructed with multiple servers of different levels.
**	
**  User Action:
**	This error may be ignored if your server will not be processing
**	events on behalf of applications.  Otherwise report this problem to
**	the product vendor.
*/
E_SC0294_XSEV_CONFIG_LOC:NO_GE
"The Event System config file could not be opened as the system could not
build a path to the config file (events.cnf) in the II_DBMS_CONFIG
directory (error status 0x%0x).  This server will use default values for
event configuration parameters rather than values contained in a config file.
Check the environment variable II_DBMS_CONFIG to correct this warning."
/*%
** Parameters:
**	%0	- CL status returned from NMloc call.
**
** Description:
**	The system attempted an NMloc call to find a pointer to the event
**	config file 'events.cnf'.  This call returned an error status.
**	The event config file should exist in the II_DBMS_CONFIG directory.
**	The NMloc call should succeed even if the file does not exist.
**	This error is probably the result of a missing or incorrectly
**	defined environment variable - probably II_DBMS_CONFIG.
**
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
** System Status:
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
**	If this error is actually evidence of more serious environment setup
**	problems then it is likely that other portions of Ingres will also
**	encounter problems and the system may not start up.
**
** User Action:
**	Check the setting of II_DBMS_CONFIG.  Check for existence of the
**	II_DBMS_CONFIG directory and the events.cnf file.  Check for
**	file and directory access permissions.
*/
E_SC0295_XSEV_CONFIG_OPEN:NO_GE
"The Event System config file (events.cnf) could not be opened and probably
does not exist (error status %0x).  This server will continue to start up,
but will use default values for event configuration parameters.  To create a
config file with non-default parameters, use the Event Utility.  If this has
already been done, check file and directory access permissions on the event
config file.  The event config file is located in the II_DBMS_CONFIG directory."
/*%
** Parameters:
**	%0	- CL status returned from TRset_file call.
**
** Description:
**	The server was unable to open the event config file.  This file
**	should exist in the II_DBMS_CONFIG directory and is named 'events.cnf'.
**	This problem is probably caused by non-existence of the event config
**	file.  If the file exists, the problem could be caused by file
**	access permissions or incorrect setting of II_DBMS_CONFIG.
**
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
** System Status:
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
**	If this error is actually evidence of more serious environment setup
**	problems then it is likely that other portions of Ingres will also
**	encounter problems and the system may not start up.
**
** User Action:
**	Check the setting of II_DBMS_CONFIG.  Check for existence of the
**	II_DBMS_CONFIG directory and the events.cnf file.  Check for
**	file and directory access permissions.
*/
E_SC0296_XSEV_CONFIG_READ:NO_GE
"The server encountered an error reading the event config file (error
status 0x%0x).  The server will start up using default values for event
configuration parameters that it did not find in the config file."
/*%
** Parameters:
**	%0	- CL status returned from TRrequest call.
**
** Description:
**	The server encountered an error attempting to read a record from
**	the event config file.  This may indicate that a record of the
**	config file has a bad format.
**
**	A new config file may be created using the Event Utility.
**
**	When this error is encountered, the server will continue starting
**	up, but will use default values for any configuration parameters
**	it did not find in the config file.  Parameters found before the
**	read error will be used.
**
**	The configuration parameters used by the system are written to
**	the server trace file if one is defined at startup time.  Use the
**	II_DBMS_LOG environment variable to define a server trace file.
**
** System Status:
**	The server/system will continue to start up.  Default values will
**	be used for event system configuration parameters.
**
** User Action:
**	Use the Event Utility to examine the event config file and create
**	a new one if necessary.
*/
E_SC0297_XSEV_CONFIG_VALUE:NO_GE
"The server encountered an error reading the event config file - the value
for the parameter %0c (value = %1d) is out of range for this parameter.
The minimum value %2d will be used instead."
/*%
** Parameters:
**	%0	- Parameter name
**	%1	- Parameter value
**	%2	- Value substituted
**
** Description:
**	The server encountered an error attempting to read a record from
**	the event config file.  A value for one of the parameters was out
**	of range.  The minimum value for that parameter is substituted
**	instead.
**
** System Status:
**	The server/system will continue to start up.
**
** User Action:
**	This error message can be avoided by changing the value in the
**	event config file.
*/
E_SC0298_EVENT_CONFIG:NO_GE
"The SCF alert subsystem could not be initialized due to an error attempting
to read the event system config file.  Check the server error log for 
initialization errors."
/*%
** Parameters:
**	None.
**
** Description:
**	At server startup time, the server connects to the
**	installation's Event System.  In order to do this, it must open
**	and read the event system's config file.  This action is normally
**	tolerant of errors such as non-existent config file, read errors
**	or data format errors.  The reporting of this error probably means
**	that some error condition involving the event config file was
**	overlooked.
**	
** System Status:
**	When this error is logged, the server will fail to start up.
**
** User Action:
**	The user should look in the installation error log for further
**	detail on why the event config file could not be read.  If this
**	condition cannot be remedied, the user can create a new event
**	config file or start up the server specifying to not use the
**	event system.
*/
E_SC0299_EVENTS_DISABLED:NO_GE
"Warning: The server startup parameters specified to use the Ingres Event
System, but the event system is disabled in this installation.  This server
will run without event processing (as if the NOEVENTS startup option was
specified).  To enable the Ingres Event System, use the Event Utility."
/*%
** Parameters:
**	None.
**
** Description:
**	The server startup options specified to use events.  The event
**	system config file currently specifies that the event system is
**	disabled in this installation.  The server will start up but will
**	not connect to the event system.  Applications connected to this
**	server will not be able to use Alerts or other features which rely
**	on the Ingres Event System.  To enable the event system, use the
**	Event Utility.  After enabling events, the installation must be
**	restarted to bring up the event system.
**	
** System Status:
**	The server will start up, but will not connect to the event system.
**	Applications connected to this sever will not be able to use Alerts
**	or other features which rely on the Ingres Event System.
**
** User Action:
**	If the user wishes to run without events, no action is necessary.
**	Since events were disabled in the event config file, this is probably
**	what was desired.  The user can get rid of this error message by
**	using the NOEVENTS server startup option.  If the user wants to
**	use events, then the event system should be enabled using the Event
**	Utility.
*/
E_SC029A_EVENT_CLUSTER_INIT:NO_GE
"This server cannot connect to the Event System because it is not yet
initialized.  In an Ingres Cluster Installation, the cluster process (CSP)
must initialize the Event System so that events can be properly signaled
to other nodes.  In this installation, the CSP is not connected to the
Event System.  This probably means that the Event System was not enabled
at the time that the installation was started, but has since been modified
to indicate that events should be used.  To enable the Event System, the
installation on this node must be stopped and restarted.  To start a server
without event connecting to the Event System, use the NOEVENTS startup option."
/*%
** Parameters:
**	None.
**
** Description:
**	The Ingres Event System is initialized by the first process which
**	connects to it.  In a cluster installation, this must be the CSP
**	process.  If a server on a cluster installation finds that it is
**	the first connector to the Event System, then it will fail to
**	start up.  This is because it will be unable to properly communicate
**	events to other nodes of the Ingres Installation.
**
**	This error will most likely occur if the system is initialized while
**	events are not enabled in the installation, and then later the
**	system is modified to specify that events should be used.  Servers
**	that start up after events have been enabled will try to connect
**	to the Event System, generating this error.
**
**	After enabling events in an installation in which they were previously
**	disabled, the entire installation should be brought down and restarted.
**
** System Status:
**	The server will not start up until either events are explicitly
**	turned off, or until the installation is restarted so that the CSP
**	can be connected to the Event System.
**
** User Action:
**	To start a server with event handling, restart the installation to
**	connect the CSP to the Event System.
**
**	In order to start a server without restarting the installation, the
**	server must be started with options specifying to not do event
**	processing.  This is done by using the NOEVENTS startup option or
**	by turning off events in the installation with the Event Utility.
*/
E_SC0300_EXDB_LOCK_REQ:NO_GE
"This operation requires an exclusive database lock."
E_SC0301_LOC_EXISTS:NO_GE
"This location already exists."
E_SC0302_AUDIT_INIT:NO_GE
"Error initializing auditing state.  This server will be terminated."
/*%
** Parameters: (none)
**
** System Status:
**      DBMS has encountered an error when initializing the auditing
**	state for the server.  Either the IISECURITYSTATE catalog could
**	not be processed, or DMF returned an error when initializing
**	internal control blocks.  DBMS server is terminated.
**
** User Action:
**	Contact INGRES Customer Support.
*/
E_SC0303_SCA_CREATE:NO_GE "Error creating user defined datatype catalog <%0c>"
/*%
** Parameters:
**      %0c Table name
**
** System Status:
**      DBMS has encountered an error creating the IIDBDB catalog noted.  The
**	DBMS server will start up, but the IIDBDB will not have been converted
**	to support user defined datatypes.
**
** User Action:
**      Correct the error and restart the server.  This is a summary message --
**	the real problem will precede this message in the log.
*/
E_SC0304_SCA_MODIFY:NO_GE "Error modifying user defined datatype catalog <%0c>"
/*%
** Parameters:
**      %0c Table name
**
** System Status:
**      DBMS has encountered an error modifying the IIDBDB catalog noted.  The
**	DBMS server will start up, but the IIDBDB will not have been converted
**	to support user defined datatypes.
**
** User Action:
**      Correct the error and restart the server.  This is a summary message --
**	the real problem will precede this message in the log.
*/
E_SC0305_SCA_QTXT_CREATE:NO_GE "Error adding database query text to the IIQRYTEXT catalog."
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS server will start, but no IIDBDB conversion will have taken place.
**	The actual error precedes this message in the log.
**
** User Action:
**	The system is attempting to add query text for a system database
**	procedure used to maintain the system.  These procedures are necessary
**	for the correct maintenance of the user defined datatypes.
**	
**      Correct the aforementioned problem and restart the server.
*/
E_SC0306_SCA_DBP_CREATE:NO_GE "Error adding database procedure to the IIPROCEDURES catalog"
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS server will start, but no IIDBDB conversion will have taken place.
**	The actual error precedes this message in the log.
**
** User Action:
**	The system is attempting to add query text for a system database
**	procedure used to maintain the system.  These procedures are necessary
**	for the correct maintenance of the user defined datatypes.
**	
**      Correct the aforementioned problem and restart the server.
*/
E_SC0307_SCA_CONVERTING:NO_GE "Beginning conversion of IIDBDB database for User
Defined Datatype Support."
/*%
** Parameters:
**      None
**
** System Status:
**      DBMS is attempting the conversion of the IIDBDB for user defined
**	database support.  This is done by the first server to start up after
**	user defined datatype support has been authorized (purchased).
**
** User Action:
**      Status message -- no action necessary
*/
E_SC0308_SCA_CONVERTED:NO_GE "IIDBDB User Defined Datatype Conversion successfully completed."
/*%
** Parameters:
**      None
**
** System Status:
**      DBMS has completed the conversion.
**
** User Action:
**      Status message -- no action necessary.
*/
E_SC0309_SCA_NOT_CONVERTED:NO_GE "IIDBDB User Defined Datatype Conversion unsuccessful."
/*%
** Parameters:
**      None.
**
** System Status:
**      The DBMS was unsuccessful at converting the IIDBDB.  Any problems
**	encountered are logged before this message.  Server will start normally;
**	new datatype support will be unavailable until a successful conversion
**	has taken place.  Partial conversion should not occur -- either the
**	conversion is successful or it is aborted.
**
** User Action:
**      Fix previous problem and retry.  Previous message contains the detailed
**	information.
*/
E_SC030A_SCA_ALTER:NO_GE "Error altering DMF to allow system catalog updates."
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS is attempting to convert the IIDBDB -- to do so, it must create &
**	modify a set of system catalogs.  This altering could not occur, so the
**	conversion will be aborted.
**
** User Action:
**      Correct problem and retry.  Previous message contains the detailed
**	information.
*/
E_SC030B_SCA_BEGIN_XACT:NO_GE "Error beginning a transaction"
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS cannot start a transaction.  Conversion or reading of datatype
**	catalogs will not occur.  Server will start without completing this activity.
**
** User Action:
**      Correct problem and retry.  Previous message contains the detailed
**	information.
*/
E_SC030C_SCA_COMMIT:NO_GE "Error committing transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**      DBMS cannot commit the transaction after converting or reading the IIDBDB.
**	The server will start; extended datatype support will be unavailable.
**
** User Action:
**      Correct problem and retry.  Previous message contains the detailed
**	information.
*/
E_SC030D_SCA_ABORT:NO_GE "Error aborting transaction."
/*%
** Parameters:
**	None.
**
** System Status:
**      DBMS cannot abort the transaction after converting or reading the IIDBDB.
**	The server will start; extended datatype support will be unavailable.
**
** User Action:
**      Correct problem and retry.  Previous message contains the detailed
**	information.
*/
E_SC030E_SCA_ADD_DB:NO_GE "Error performing DMF Add database operation."
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS was in the process of looking for user defined datatype
**	information, and was unable to add the IIDBDB.  The DBMS server will
**	startup with no additional datatype information.
**
** User Action:
**      Previous message (in the log file) contains the reason for the error.
**	Correct the problem and restart the server.
*/
E_SC030F_SCA_OPEN_DB:NO_GE "Error performing DMF open database operation."
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS was in the process of looking for user defined datatype
**	information, and was unable to open the IIDBDB.  The DBMS server will
**	startup with no additional datatype information.
**
** User Action:
**      Previous message (in the log file) contains the reason for the error.
**	Correct the problem and restart the server.
*/
E_SC0310_SCA_CLOSE_DB:NO_GE "Error performing DMF Close database operation."
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS was in the process of looking for user defined datatype
**	information, and was unable to close the IIDBDB.  The DBMS server will
**	startup with the database left open, assuming the DMF returned a
**	non-fatal error.  The server is left up as an aid in diagnosing the
**	problem.
**
** User Action:
**      Previous message (in the log file) contains the reason for the error.
**	Correct the problem and restart the server.
*/
E_SC0311_SCA_DELETE_DB:NO_GE "Error performing DMF Delete database operation."
/*%
** Parameters:
**      None.
**
** System Status:
**      DBMS was in the process of looking for user defined datatype
**	information, and was unable to delete the IIDBDB.  The DBMS server will
**	startup with the database left added, assuming the DMF returned a
**	non-fatal error.  The server is left up as an aid in diagnosing the
**	problem.
**
** User Action:
**      Previous message (in the log file) contains the reason for the error.
**	Correct the problem and restart the server.
*/
E_SC0312_SCA_NO_LICENSE:E_GE8D68_CONFIG_ERROR
"An attempt was made to add User Defined Datatypes, Operators, or Functions,\
 but this node is not currently licensed to do so."
/*%
** Parameters:
**      None.
**
** System Status:
**	When the DBMS server attempted to start, the IIudadt_register function
**	was called and it returned a non-NULL pointer indicating that this node
**	is attempting to use UserADT enhancements.  Upon checking the
**	authorization string (II_AUTHORIZATION) the server discovered that this
**	node is not licensed for this functionality.
**
** User Action:
**	If you believe this node is licensed for this functionality, check your
**	authorization string to ensure it is current and correctly entered.  If
**	you are not licensed for this functionality, either change the
**	IIudadt_register function in your IIUSERADT sharable image to return
**	NULL, or contact Ingres Corporation to obtain licensing for the
**	`UserADT' extension.
*/
E_SC0313_NOT_LICENSED:E_GE8D68_CONFIG_ERROR
"An attempt was made to use the `%0c' statement.  This functionality is part of\
 the `%1c' extension which this node is not licensed to use."
/*%
** Parameters:
**      %0c	Statement whose execution was attempted
**	%1c	Name of Ingres DBMS extension containing this feature
**
** System Status:
**	An attempt was made to execute a statement which is listed as an
**	optional feature.  Upon checking the authorization string
**	(II_AUTHORIZATION) the server discovered that this node is not licensed
**	for this feature.
**
** User Action:
**	If you believe this node is licensed for this functionality, check your
**	authorization string to ensure it is current and correctly entered.  If
**	you are not licensed for this functionality, either change the
**	application which caused this statement to be issued, or contact
**	Ingres Corporation to obtain licensing for the extension listed in the
**	error message.
*/
E_SC0314_RECOVER_THREAD_ADD:NO_GE
"Error adding the DX recovery thread to the server"
/*%
** Parameters:
**      NONE.
**
** System Status:
**	CS failed to add a new thread to perform the DX recovery.
**	This error may show up during STAR server startup, it is
**	a fatal error the server will die.
**
** User Action:
**	Make sure the server has enough resources and is confirgured to
**	start at least the DX recovery thread and an iimonitor thread,
**	in case someone needs to monitor the server from iimonitor.
*/
E_SC0315_RECOVERING_ORPHAN_DX:NO_GE
"STAR server is not open for business yet, it is recovering orphan distributed\
 transactions, try again latter."
/*%
** Parameters:
**      NONE.
**
** System Status:
**	The STAR server is performing recovery on orphan distributed transactions.
**	The server will not be open for business util the recovery is complete.
**	Only iimonitor can connect to the server and monitor server activity.
**
** User Action:
**	Wait until the TPF recovers all the orphan distributed transactions
**	of the installation.
*/
E_SC0316_LOC_NOT_FOUND:NO_GE
"This database is not extended to location '%0c'."
/*%
** Parameters:
**      %0c - Name of location.
**
** Description:
**	The SET WORK LOCATIONS command was issued with at least one work
**	location which doesn't exist in this database's list of locations.
**	The name of this location is given in the error text.  SET WORK
**	LOCATIONS can be used only with locations which the database has
**	been extended to.
**
** System Status:
**	The SET command is disregarded.  No net effect.
**
** User Action:
**	Use ACCESSDB to extend the location to the database if you wish to
**	use this location for WORK in this database.
*/
E_SC0317_LOC_NOT_AUTHORIZED:NO_GE
"Either you are not authorized to use location '%0c' or it is not a WORK
location."
/*%
** Parameters:
**      %0c - Name of location.
**
** Description:
**	An attempt was made to use the SET WORK LOCATIONS command to begin
**	using the location named above.  Either you are not allowed to use
**	this location because of a GRANT issued against you, or you have
**	specified a location which is not a WORK location.  In either case,
**	you are not authorized to use the location specified for WORK.
**
** System Status:
**	The SET command is disregarded.  No net effect.
**
** User Action:
**	Either gain access through the proper channels or do not attempt to use
**	this location as a WORK location.
*/
E_SC0318_SCA_USER_TRACE:NO_GE
"User Trace Message: '%0c'"
/*%
** Parameters:
**      %0c - Message provided by the user.
**
** System Status:
**      The system is operating normally.  Some user defined datatype code has
**	requested that a trace message be placed into the system error log.
**
** User Action:
**      Check with the author of the user defined datatype.  Presumably,
**	that person is interested in the contents of the message.
*/
E_SC0319_SERVER_CRASH_TEST:NO_GE
"The Ingres Server has been shutdown as part of a server crash test.  The
shutdown was triggered by the server crash trace flag.  This is used to
simulate actual server crashes during product validation."
/*%
** Parameters:
**     None.
**
** System Status:
**	The server will crash immediately.
**
** User Action:
**	None - presumably the user has requested the server crash by
**	issueing the server crash trace flag.  Recovery should be done
**	automatically by the Ingre Recovery System.
*/
E_SC031A_LO_PARAM_ERROR:E_GE9D24_DATAEX_OTHER
"Error processing Large Object Parameter:  See previous error(s) in error log
for details."
/*%
** Parameters:
**      None
**
** System Status:
**      This is a summary error, which indicates the presence of other, more
**	specific error in the error log.  The current query will be aborted.
**
** User Action:
**      See the more specific messages in the error log which precede this
**	error.  Follow the actions indicated for these errors.
*/
/*
**    Message file for Facility code: _UL
*/


#define _UL_CLASS 10


E_UL0000_OK:E_GE0000_OK
"successful completion"
E_UL0001_BAD_SYSTEM_LOOKUP:NO_GE
"ULF error"
E_UL0002_BAD_ERROR_LOOKUP:NO_GE
"Could not find message in error message file"
E_UL0003_BADPARM:NO_GE
"Bad parameter to error message lookup routine."
E_UL0004_CORRUPT:NO_GE
"ULM has detected a corrupted memory pool."
E_UL0005_NOMEM:NO_GE
"Either ULM cannot get enough memory from SCF to initialize a memory
 pool, or the memory pool has been exhausted."
E_UL0006_BADPTR:NO_GE
"Bad pointer has been detected by ULM; pointer to piece of memory lies outside
 of the supplied memory pool."
E_UL0007_CANTFIND:NO_GE
"ULM cannot find the block of memory in the pool's allocated list."
E_UL0008_MEM_SEMINIT:NO_GE
"SCF would not initialize semaphore."
E_UL0009_MEM_SEMWAIT:NO_GE
"Error attempting to set semaphore."
E_UL000A_MEM_SEMRELEASE:NO_GE
"Error attempting to release semaphore."
E_UL000B_NOT_ALLOCATED:NO_GE
"Attempt to perform some ULM operation on a memory pool that was not
 allocated by you."
E_UL000C_BAD_ACK_MODE:NO_GE
"ULF error"
E_UL000D_NEWBLOCK:NO_GE
"ULF error"
E_UL000E_BAD_DO_BLOCK:NO_GE
"ULF error"
E_UL000F_BAD_BLOCK_TYPE:NO_GE
"ULF error"
E_UL0010_NEED_MORE:NO_GE
"ULF error"
E_UL0011_MORE_TO_SEND:NO_GE
"ULF error"
E_UL0012_NETWORK_UNKNOWN:NO_GE
"ULF error"
E_UL0013_IN_BLOCK_FULL:NO_GE
"ULF error"
E_UL0014_BAD_ULM_PARM:NO_GE
"Bad parameter supplied to one of the ULM routines."
E_UL0015_BAD_STREAM_MARK:NO_GE
"ULM was handed an invalid `memory stream mark'."
E_UL0101_TABLE_ID:NO_GE
"Invalid hash table id found in ULH request block."
E_UL0102_TABLE_SIZE:NO_GE
"Invalid hash table size specified."
E_UL0103_NOT_EMPTY:NO_GE
"Trying to close a hash table containing locked or fixed objects."
E_UL0104_FULL_TABLE:NO_GE
"No more space in the hash table."
E_UL0105_FULL_CLASS:NO_GE
"No more space in the specified class."
E_UL0106_OBJECT_ID:NO_GE
"Invalid object id found in ULH request block."
E_UL0107_ACCESS:NO_GE
"Invalid mode of access to the object."
E_UL0108_DUPLICATE:NO_GE
"Duplicate object id."
E_UL0109_NFND:NO_GE
"Requested object/class not found in the hash table."
E_UL010A_MEMBER:NO_GE
"The object is already a member of a class."
E_UL010B_NOT_MEMBER:NO_GE
"The object is not a member of any class."
E_UL010C_UNKNOWN:NO_GE
"Unknown information code."
E_UL010D_TABLE_MEM:E_GE8D04_NO_RESOURCE
"Error in opening a memory stream for the hash table."
E_UL010E_TCB_MEM:E_GE8D04_NO_RESOURCE
"Error in allocating memory space for the hash table."
E_UL010F_BUCKET_MEM:E_GE8D04_NO_RESOURCE
"Error in allocating memory space for hash buckets."
E_UL0110_CHDR_MEM:E_GE8D04_NO_RESOURCE
"Error in allocating memory space for class headers"
E_UL0111_OBJ_MEM:E_GE8D04_NO_RESOURCE
"Error in allocating memory for a hash object"
E_UL0112_INIT_SEM:NO_GE
"Error in initializing a semaphore."
E_UL0113_TABLE_SEM:NO_GE
"Error in locking the hash table semaphore."
E_UL0114_BUCKET_SEM:NO_GE
"Error in locking a bucket semaphore."
E_UL0115_CLASS_SEM:NO_GE
"Error in locking a class semaphore."
E_UL0116_LRU_SEM:NO_GE
"Error in locking the LRU queue semaphore."
E_UL0117_OBJECT_SEM:NO_GE
"Error in locking an object semaphore."
E_UL0118_NUM_ALIASES:NO_GE
"Invalid number of aliases specified."
/*
** Parameters:
**	None
** Explanation:
**	This is an internal error which may only occur on server startup.
**	ulh_init() has been called with a flag indicating that aliases will be
**	allowed but the maximum number of aliases specified by the caller was
**	non-positive (<=0).
**	This should not occur under normal circumstances.
** System Status:
**	Server will not start.
** User Action:
**	Contact your INGRES technical representative.
*/
E_UL0119_ALIAS_MEM:NO_GE
"Error in allocating memory for alias."
/*
** Parameters:
**	None
** Explanation:
**	In order to create a new alias in ULH cache, there should be enough
**	memory to accomodate a new alias and the total number ofexisting aliases
**	should not exceed some number specified at server startup.  If one of
**	these conditions exists, at least one object must be removed from the
**	cache before the new alias can be created.
**	This error will occur if no cache entries (with the possible exception
**	of the one for which the alias was being created) could be removed from
**	the cache.
** System Status:
**	Statement will be aborted.
** User Action:
**	Contact your database administrator.
*/
E_UL0120_DUP_ALIAS:NO_GE
"Duplicated alias."
E_UL0121_NO_ALIAS:NO_GE
"The hash table does not support alias."
E_UL0122_ALIAS_REDEFINED:NO_GE
"Object alias cannot be redefined."
E_UL0123_NAME_TOO_LONG:NO_GE
"The name string is too long."
E_UL0201_ULM_FAILED:E_GE8D04_NO_RESOURCE
"Unable to obtain memory from ULM to build tree for display."

/*
**    Message file for Facility code: _US
*/


#define _US_CLASS   0
E_US0001:E_GE8CA0_SYSTEM_LIMIT
"Number of users has exceeded limit."
E_US0002:E_GE84D0_NO_PRIVILEGE
"User does not have permission to use 
one of the flags given ingres."
E_US0003:E_GE84D0_NO_PRIVILEGE
"User does not have permission to access this database."
E_US0004:NO_GE
"Could not access the database. 
Make sure images are installed correctly."
E_US0005:NO_GE
"Could not open the iirelation table."
E_US0006:NO_GE
"Could not open the iiattribute table."
E_US0007:NO_GE
"Could not read the config file."
E_US0008:NO_GE
"The requested database is not compatible with the INGRES server.
\nIf a Release 5 database, convert it with the CONVTO60 utility.
\nIf a Release 6 database, upgrade it with the UPGRADEDB utility."
/*%
** Parameters:
**	(none)
**
** Explanation:
**	The specified database was not compatible with
**	the server.
**
** System Status:
**	The session is terminated.
**
** User Action:
**	Convert the specified database using the appropriate conversion
**	utility.  To determine what utility to use:
**	
**	    $ SQL iidbdb
**	    > select dbcmptlvl from iidatabase where name = 'databasename'
**	    > \q
**
**	If dbcmptlvl is "5.0", run CONVTO60; else run UPGRADEDB.
**
*/
E_US0009:NO_GE
"Bad parameter given to ingres."
E_US000A:NO_GE
"iiuser table does not exist."
E_US000B:NO_GE
"Cannot open iiuser table."
E_US000C:NO_GE
"You are not a valid ingres user."
E_US000D:NO_GE
"The -u flag specified a nonexistent user."
E_US000E:NO_GE
"iidatabase table does not exist."
E_US000F:NO_GE
"Could not open or read the iidatabase table."
E_US0010:NO_GE
"Database does not exist."
E_US0011:NO_GE
"Table iidbaccess does not exist."
E_US0012:NO_GE
"Cannot open or read iidbaccess table."
E_US0013:NO_GE
"Request for a lock failed."
E_US0014:NO_GE
"Database not available at this time.\n
    o The database may be marked inoperable.  This can occur if\n
      CREATEDB failed or CONVTO60 is in progress.\n
    o An exclusive database lock may be held by another session.\n
    o The database may be open by an exclusive (/SOLE) DBMS server."
E_US0015:NO_GE
"Error initializing transaction system."
E_US0016:NO_GE
"Error writing to CONFIG file."
E_US0017:NO_GE
"Couldn't find an CONFIG file."
E_US0018:NO_GE
"Couldn't find CONFIG file."
E_US0019:NO_GE
"The database may be inconsistent; please contact the\n
	ingres system manager"
E_US001A:NO_GE
"Could not find database default AREA in IILOCATIONS."
E_US001B:NO_GE
"Could not find database checkpoint AREA in IILOCATIONS."
E_US001C:NO_GE
"Could not find database journal AREA in IILOCATIONS."
E_US001D:NO_GE
"Fatal in looking up an AREA in IILOCATIONS."
E_US001E:NO_GE
"Could not find an AREA for LOCATIONNAMES in IIEXTEND catalog."
E_US001F:NO_GE
"Could not open or read the IIEXTEND catalog."
E_US0020:NO_GE
"Could not open or read the IILOCATIONS catalog."
E_US0021:NO_GE
"Fatal error in forming list of available LOCATIONNAMES."
E_US0022:NO_GE
"Either the flag format or one of the flags is incorrect,\n
or the parameters are not in proper order."
E_US0023:NO_GE
"You are not authorized to use one of the specified flags."
E_US0024:NO_GE
"Couldn't access DB directory."
E_US0025:NO_GE
"Couldn't access JNL directory."
E_US0026:NO_GE
"Database is inconsistent. please contact the\n
	ingres system manager"
E_US0027:NO_GE
"The are no more journal logs available. Please try later."
E_US0028:NO_GE
"Couldn't open or write to the journal file."
E_US0029:NO_GE
"The global section containing the page cache is incompatible\n
with this version of the software."
E_US002A:E_GE9858_FATAL_ERROR
"Couldn't acquire cache lock, either privilege or quota problem."
E_US002B:NO_GE
"Could not open the iidbdb database."
E_US002C:NO_GE
"The iidbdb database is inconsistent; please contact the\n
	ingres system manager"
E_US002D:NO_GE
"An error occurred while closing a database in preparation\n
for opening another one."
E_US002E:NO_GE
"An error occurred in trying to get a lock\n
	while opening a database."
E_US002F:NO_GE
"Could not map global section containing the global cache."
E_US0030:NO_GE
"An error occurred while initializing DMF."
E_US0031:NO_GE
"An error occurred while cleaning up from an error\n
	while opening a database."
E_US0032:NO_GE
"An error occurred while finding the name of the
database directory."
E_US0033:NO_GE
"An error occurred while initializing the database directory."
E_US0034:NO_GE
"An error occurred while resetting the database directory."
E_US0035:NO_GE
"An error occurred while creating the DMF buffer."
E_US0036:NO_GE
"An error occurred while reading and processing the config file\n
	during database initialization."
E_US0037:NO_GE
"An error occurred while initializing the descriptor cache."
E_US0038:NO_GE
"An error occurred while setting a lock on the config file."
E_US0039:NO_GE
"An error occurred while opening the config file."
E_US003A:NO_GE
"An error occurred while initializing the transaction system.\n
	 Please contact the ingres system manager"
E_US003B:NO_GE
"An error occurred while re-writing the CONFIG file."
E_US003C:NO_GE
"An error occurred while closing the CONFIG file."
E_US003D:NO_GE
"An error occurred while unlocking the CONFIG file."
E_US003E:NO_GE
"An error occurred while opening the iirelation table.\n
	Please contact the ingres system manager"
E_US003F:NO_GE
"An error occurred while opening the CONFIG table."
E_US0040:NO_GE
"INGRES can only be invoked from Application Processor 00."
E_US0041:NO_GE
"The user code of dba in CONFIG header does not match that in iidbdb."
E_US0042_COMPAT_LEVEL:NO_GE
"The major database compatibility level does not match that of the server.
\nBe sure the server is compatible with the requested database."
/*%
** Parameters:
**	(none)
**
** Explanation:
**	The accessed database's dbcmptlvl does not match the server's
**	major compatibility level.
**
** System Status:
**	The session is terminated.
**
** User Action:
**	Convert the database using the appropriate conversion utility.
**
*/
E_US0043_COMPAT_MINOR:NO_GE
"The minor database compatibility level supersedes that of the server.
\nBe sure the server is compatible with the requested database."
/*%
** Parameters:
**	(none)
**
** Explanation:
**	The accessed database's dbcmptminor is greater than the server's
**	minor compatibility level.
**
** System Status:
**	The session is terminated.
**
** User Action:
**	Convert the database using the appropriate conversion utility.
**
*/
E_US0044_CONVERT_DBDB:NO_GE
"The iidbdb database is not compatible with the INGRES server.
\nYou must convert the iidbdb database with the UPGRADEDB utility
\nbefore attempting to access any other database in this installation."
/*%
** Parameters:
**	(none)
**
** Explanation:
**	The iidbdb database's config file was not compatible with
**	the server, and the session was attempting to connect to
**	a non-iidbdb database.
**
** System Status:
**	The session is terminated.
**
** User Action:
**	Convert the iidbdb database using the UPGRADEDB utility.
**
*/
E_US0046:E_GE8D68_CONFIG_ERROR
"The environment variable, II_TIMEZONE, is undefined."
E_US0047:E_GE797C_INVALID_IDENT
"Database name invalid."
E_US0048:E_GE8CA0_SYSTEM_LIMIT
"Database quota for this server has been exceeded."
E_US0049:NO_GE
"Invalid internal data prevents database access."
E_US0067:E_GE8D68_CONFIG_ERROR
"Remote node not configured to run INGRES/Net."
E_US0068:E_GE8D68_CONFIG_ERROR
"Local node not configured to run INGRES/Net."
E_US006D:E_GE8D68_CONFIG_ERROR
"Local node not licensed to run INGRES/Net."
E_US006E:E_GE8D68_CONFIG_ERROR
"Remote node not licensed to run INGRES/Net."
E_US0070:E_GE8D68_CONFIG_ERROR
"Local node not licensed to run the DBMS."
E_US0071:E_GE8D68_CONFIG_ERROR
"Remote node not licensed to run the DBMS."
E_US0072:NO_GE
"The IIEXTEND entry for the database is missing.\n
	This can be caused by one of two events.\n
	1) The IIDBDB is corrupted. Run FINDDBS.\n
	2) The database was not build properly.\n
	   Destroy it and then recreate it."
E_US0073:NO_GE
"The IIDBDB is unavailable at this time,\n
	please try again later when it has been unlocked.\n
Could not open the iidbdb database."
E_US0080_LOCATION_NOT_FOUND:NO_GE
"One of the locations named in the CREATEDB command doesn't exist\n
	in the IILOCATIONS catalog."
E_US0081_LOCATION_NOT_ALLOWED:NO_GE
"One of the locations named in the CREATEDB command exists but\n
	the type data to be allocated doesn't match the type of\n
	data allowed in the location.  Use CATALOGDB to check\n
	location names and types."
E_US0082_DATABASE_UPDATE:NO_GE
"Error occurred updating the IIDATABASE table in the IIDBDB database.\n
    Check error log (ii_system:[ingres.files]errlog.log) for more\n
    information."
E_US0083_EXTEND_UPDATE:NO_GE
"Error occurred updating the IIEXTEND table in the IIDBDB database.\n
    Check error log (ii_system:[ingres.files]errlog.log) for more\n
    information."
E_US0084_DBACCESS_UPDATE:NO_GE
"Error occurred updating the IIDBACCESS table in the IIDBDB database.\n
    Check error log (ii_system:[ingres.files]errlog.log) for more\n
    information."
E_US0085_DATABASE_EXIST:NO_GE
"Attempted to create a database that already exists."
E_US0086_DBPRIV_UPDATE:NO_GE
"Error occurred updating the IIDBPRIV table in the IIDBDB database.\n
    Check error log (ii_system:[ingres.files]errlog.log) for more\n
    information."
E_US0087_SPARE_2:NO_GE
"Spare"
E_US0088_SPARE_3:NO_GE
"Spare"
E_US0089_SPARE_4:NO_GE
"Spare"
E_US00C8_200:E_GE7918_SYNTAX_ERROR
"Bad parameter format '%0c' in repeat query."
E_US07D1_2001:E_GE7D00_QUERY_ERROR
"CREATE TABLE: number of column names and number of target list elements
in the subselect do not match."
E_US07D2_2002:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: Bad storage structure '%0c'.\n
The valid storage structure names are [c]btree, [c]isam, [c]hash, [c]heap."
E_US07D3_2003:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: invalid option '%0c'. Try NOT NULL WITH DEFAULT."
E_US07D4_2004:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: either invalid attribute qualifier '%0c', wrong order, 
or wrong combination of qualifiers.  Try
'[[not null [with | not default]] | with null] [not | with system_maintained]'."
/*%
** Parameters:
**	%0 - The attribute qualifer which caused the error.
**
** Explanation:
**	The accepted syntax diagram for the column qualifier clause is:
**      '[[not null [with | not default]] | with null] 
**					[not | with system_maintained]'."
**	with one exception. It is not allowed to specify either "not default" or
**	"with null" in conjunction with the "with system_maintained" 
**	qualifier.
**		
**	The order of these qualifiers is significant, and must appear as
**	described.  The following is a complete list of accepted attribute
**	qualifiers (a few options although syntactically correct per the 
**	above diagramas described by the syntax diagram, are disallowed due
**	to the "with system_maintained" restrictions):
**		with null
**		not null with default
**		not null not default
**		not null with default with system_maintained
**		with null not system_maintained
**		not null with default not system_maintained
**		not null not default not system_maintained
**		with system_maintained
**		not system_maintained
**
**
** System Status:
**	Current statement is aborted.
**
** User Action:
**	Reissue the CREATE statement after correcting the problem with
**	the attribute qualifier clause.
*/
E_US07D5_2005:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: '%0c' is either an invalid option or the syntax of\n
	the WITH subclause is invalid.
\nSTRUCTURE is the only valid keyword for this type of subclause."
E_US07D6_2006:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Location specified twice: '%0c' conflicts with '%1c'."
E_US07D7_2007:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: '%0c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n[NO]DUPLICATES, [NO]JOURNALING."
E_US07D8_2008:E_GE8CA0_SYSTEM_LIMIT
"CREATE TABLE: excessive table width on column '%0c'."
E_US07D9_2009:E_GE7D00_QUERY_ERROR
"'%0c' is a system table."
E_US07DA_2010:E_GE75F8_DEF_RESOURCE
"Duplicate object name '%0c'."
E_US07DB_2011:E_GE8CA0_SYSTEM_LIMIT
"CREATE TABLE: the number of columns must not exceed 300."
E_US07DC_2012:E_GE797C_INVALID_IDENT
"CREATE TABLE: invalid column name '%0c'."
E_US07DD_2013:E_GE7D00_QUERY_ERROR
"CREATE TABLE/VIEW: duplicate column name '%0c'."
E_US07DE_2014:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: invalid column format '%0c' on column '%1c'."
E_US07DF_2015:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Too many options in the WITH clause."
E_US07E0_2016:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Improper use of NULL in the target list;\n
	use conversion function to specify data type of the null value."
E_US07E1_2017:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Invalid FILLFACTOR of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US07E2_2018:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Invalid MINPAGES of '%0d' specified. Must be greater than 0."
E_US07E3_2019:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Invalid MAXPAGES of '%0d' specified. Must be greater than 0."
E_US07E4_2020:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Invalid NONLEAFFILL of '%0d' specified.
\nMust be in 1 - 100 range (inclusive)."
E_US07E5_2021:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Invalid LEAFFILL/INDEXFILL of '%0d' specified.
\nMust be in 1 - 100 range (inclusive)."
E_US07E6_2022:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: '%0c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are: KEY, LOCATION."
E_US07E7_2023:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Number of key columns must not be greater than
the number of columns in the table."
E_US07E8_2024:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Key column '%0c' is undefined."
E_US07E9_2025:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Duplicate key column '%0c'."
E_US07EA_2026:E_GE7D00_QUERY_ERROR
"CREATE TABLE: WITH option '%0c' allowed only in the version of the
\n	CREATE TABLE statement using a subselect."
E_US07EB_2027:E_GE7D00_QUERY_ERROR
"CREATE TABLE: Illegal MINPAGES value '%0d' in a CREATE TABLE command.
\n	The value must be less than or equal to the MAXPAGES value."
E_US07EC_2028:E_GE7D00_QUERY_ERROR
"CREATE TABLE: The '%0c' entry in the WITH clause
is incompatible with the storage structure."
E_US07ED_2029:E_GE7918_SYNTAX_ERROR
"CREATE TABLE: '%0c' is either an invalid option or the syntax of the WITH
subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL."
E_US07EE_2030:E_GE7918_SYNTAX_ERROR
"RETRIEVE/CREATE TABLE: Storage structure type may not be quoted."
E_US07EF_2031:E_GE7D00_QUERY_ERROR
"%0c: In the with clause, NODE and DATABASE clauses are
\n	optional, but only as a unit."
E_US07F0_2032:E_GE7D00_QUERY_ERROR
"%0c: duplicate column name '%1c'."
E_US07F1_2033:E_GE7918_SYNTAX_ERROR
"REGISTER INDEX: Syntax error on '%0c'.  The correct syntax is:
\n    REGISTER INDEX indexname ON tablename AS LINK [FROM source]"
E_US07F2_2034:E_GE7918_SYNTAX_ERROR
"CREATE: '%0c = %1c' is not allowed by the syntax of the WITH clause."
E_US07F3_2035:E_GE7918_SYNTAX_ERROR
"CREATE: '%0c' is either an invalid option or the syntax of\n
	the WITH subclause is invalid. \n
Valid keywords for this type of subclause are TABLE, NODE, DBMS, and DATABASE."
E_US07F4_2036:E_GE7918_SYNTAX_ERROR
"REGISTER: Syntax error on '%0c'.  The correct syntax is:
\n REGISTER [TABLE|VIEW] objname [(col_name {, col_name})]
\n	AS LINK [FROM source] [with_clause]
\n
\n REGISTER objname AS LINK WITH REFRESH"
E_US07F5_2037:E_GE7D00_QUERY_ERROR
"REGISTER/CREATE LINK: '%0c' is an index.
\n Indices may not be registered in this release."
E_US07F6_2038:E_GE7D00_QUERY_ERROR
"REGISTER %0c: '%1c' is of type '%2c'.\n
You must use 'REGISTER [%2c]'"
E_US07F7_2039:E_GE7D00_QUERY_ERROR
"REGISTER/CREATE LINK: You have to specify column names.\n
 Converting to lower case produced duplicate column name '%0c'."
E_US07F8_2040:E_GEA0F0_SQL_STMT_INV
"'REGISTER INDEX' in not supported in this release."
E_US07F9_2041:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c' in the KEY option of the WITH clause.
\n Correct syntax is:
\n	WITH ... KEY = (key_name {, key_name})"
E_US07FA_2042:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c' in the LOCATION option of the WITH clause.
\n Correct syntax is:
\n	WITH ... LOCATION = (loc_name {, loc_name})"
E_US07FB_2043:E_GE7D00_QUERY_ERROR
"REGISTER ... WITH REFRESH: %0c is a view."
E_US07FC_2044:NO_GE
"REGISTER ... WITH REFRESH: Schema of the LDB object has changed."
E_US0803_2051:E_GE7D00_QUERY_ERROR
"CREATE VIEW: number of column names and number of target list elements
\n	in the subselect do not match."
E_US0804_2052:E_GE759E_COLUMN_UNKNOWN
"CREATE VIEW: invalid column name '%0c'."
E_US081B_2075:E_GE7918_SYNTAX_ERROR
"DIRECT DISCONNECT: no parameters may be specified"
E_US081C_2076:E_GE7918_SYNTAX_ERROR
"Syntax error.  Last symbol read was: '%0c'."
E_US081D_2077:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c', 'CONNECT' was expected.\n
The correct syntax is:\n
	DIRECT CONNECT [with_clause]"
E_US081E_2078:E_GE7D00_QUERY_ERROR
"'DIRECT DISCONNECT' may only be issued if connected to a LDBMS via\n
'DIRECT CONNECT'"
E_US081F_2079:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
	[NODE = [\']node_name[\'],\n
	 DATABASE = [\']ldb_name[\']]\n
	[,DBMS = [\']dbms_name[\']]" 
E_US0820_2080:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:
\n  WITH
\n	[NODE = [\']node_name[\'], DATABASE = [\']ldb_name[\']]
\n 	[,DBMS = [\']dbms_name[\']]
\n
\n  WITH REFRESH" 
E_US0823_2083:E_GE7918_SYNTAX_ERROR
"A with_clause is not allowed with DIRECT DISCONNECT command"
E_US0826_2086:E_GEA0F0_SQL_STMT_INV
"%0c command is not allowed in non-distributed INGRES."
E_US0827_2087:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
	[NODE = [\']node_name[\'],\n
	 DATABASE = [\']ldb_name[\']]\n
	[,DBMS = [\']dbms_name[\']]\n
	[,TABLE =[\']table_name[\']]"
E_US0829_2089:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c', 'CONNECT' was expected.\n
The correct syntax is:\n
 DIRECT CONNECT\n
     [WITH\n
	[NODE = [\"]node_name[\"],\n
	 DATABASE = [\"]database_name[\"]]\n
	[,DBMS = [\"]dbms_name[\"]]]" 
E_US082A_2090:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
	[NODE = [\"]node_name[\"],\n
	 DATABASE = [\"]ldb_name[\"]]\n
	[,DBMS = [\"]dbms_name[\"]]" 
E_US082B_2091:E_GE7918_SYNTAX_ERROR
"Syntax error on '%0c'.  The correct syntax of WITH clause is:\n
     WITH\n
	[NODE = [\"]node_name[\"],\n
	 DATABASE = [\"]ldb_name[\"]]\n
	[,DBMS = [\"]dbms_name[\"]\n
	[,TABLE = [\"]table_name[\"]]" 
E_US0834_2100:E_GE759E_COLUMN_UNKNOWN
"line %0d, Table '%1c' owned by '%2c' does not contain column '%3c'."
E_US0835_2101:E_GE7D00_QUERY_ERROR
"line %0d, Column '%1c' found in more than one FROM list table."
E_US0836_2102:E_GE759E_COLUMN_UNKNOWN
"line %0d, Column '%1c' not found in any specified table."
E_US0837_2103:E_GE7D00_QUERY_ERROR
"line %0d, FROM clause is mandatory when a column ('%1c') has been specified
in the target list."
E_US0838_2104:E_GE7D00_QUERY_ERROR
"line %0d, 'select *' type of subselect requires a FROM clause."
E_US0839_2105:E_GE7D00_QUERY_ERROR
"line %0d, Column '%1c' must not be specified in this context."
E_US083B_2107:E_GE84D0_NO_PRIVILEGE
"line %0d, You are not allowed to update '%1c' table."
E_US083C_2108:E_GE7594_TABLE_NOT_FOUND
"line %0d, Result table '%1c' for APPEND does not
exist or cannot be accessed by you."
E_US083D_2109:E_GE7D00_QUERY_ERROR
"line %0d, The range variable '%1c' has not been defined."
E_US083E_2110:E_GE7918_SYNTAX_ERROR
"line %0d, The construct '$number = name' may not be used except
\nwhen defining a repeat query.  This error is usually caused by
\nan internal error in the pre-processor.  Please report this error
\nto your vendor if you have not explicitly used this construct yourself."
E_US083F_2111:E_GE8CA0_SYSTEM_LIMIT
"line %0d, Too many columns in key of INDEX."
E_US0840_2112:E_GE797C_INVALID_IDENT
"line %0d, Invalid column name 'tid'."
E_US0841_2113:E_GE7D00_QUERY_ERROR
"line %0d, Too many entries in argument list to process in this statement."
E_US0842_2114:E_GE7D00_QUERY_ERROR
"line %0d, Attempt to specify location more than once."
E_US0843_2115:E_GE7D00_QUERY_ERROR
"line %0d, Too many locations specified."
E_US0844_2116:E_GE7D00_QUERY_ERROR
"line %0d, Location '%1c' has already been specified."
E_US0845_2117:E_GE7594_TABLE_NOT_FOUND
"Table '%0c' does not exist or is not owned by you."
E_US0846_2118:E_GE7D00_QUERY_ERROR
"Invalid qualifier '%0c'. '%0c' must be declared in the FROM clause or
equivalent. If used in a target list, '%0c' must be declared at the current
scope."
E_US0847_2119:E_GE7594_TABLE_NOT_FOUND
"User '%0c' does not own table '%1c'."
E_US0848_2120:E_GE7594_TABLE_NOT_FOUND
"The table(s) that the view is dependent upon does not exist."
E_US0849_2121:E_GE7D00_QUERY_ERROR
"line %0d, Column '%1c' was declared 'for update', but is not
updatable because it is based on a constant or a calculation."
E_US084B_2123:E_GE7D00_QUERY_ERROR
"line %0d, '.ALL' is not allowed in the target list of a REPLACE command."
E_US084C_2124:E_GE7D00_QUERY_ERROR
"line %0d, Update of BTREE index '%1c' not allowed."
E_US084D_2125:E_GE7D00_QUERY_ERROR
"SAVE: You do not own table '%0c'."
E_US084E_2126:E_GE7D00_QUERY_ERROR
"SAVE: '%0c' is a system table."
E_US084F_2127:E_GE7D00_QUERY_ERROR
"SAVE: '%0c' is not a base table."
E_US0852_2130:E_GE8CA0_SYSTEM_LIMIT
"line %0d, Maximum number of columns exceeded for query."
E_US0857_2135:E_GE75F8_DEF_RESOURCE
"line %0d, You cannot duplicate the name of an existing relation '%1c'."
E_US0858_2136:E_GE7D00_QUERY_ERROR
"line %0d, There is no such hour as '%1d', use a 24 hour clock system."
E_US0859_2137:E_GE7D00_QUERY_ERROR
"line %0d, There is no such minute as '%1d', use a 24 hour clock system."
E_US085A_2138:E_GE7D00_QUERY_ERROR
"line %0d, There is no such time as 24:%1d, use a 24 hour clock system."
E_US085B_2139:E_GE7918_SYNTAX_ERROR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nSTRUCTURE is the only valid keyword for this type of subclause."
E_US085C_2140:E_GE7918_SYNTAX_ERROR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL."
E_US085D_2141:E_GE7D00_QUERY_ERROR
"You must not specify a WITH clause on a RETRIEVE command
unless it is a RETRIEVE INTO."
E_US085E_2142:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Improper use of NULL in the target list; use conversion
\nfunction to specify data type of the null value."
E_US085F_2143:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Illegal MINPAGES value '%0d' in a RETRIEVE command.
\nThe value must be less or equal to the MAXPAGES value."
E_US0860_2144:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Bad storage structure '%0c'.
\nThe valid storage structure names are [c]btree, [c]isam, [c]hash, [c]heap."
E_US0861_2145:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Invalid FILLFACTOR of '%0d' specified.
\nMust be in 1 - 100 range (inclusive)."
E_US0862_2146:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Invalid MINPAGES of '%0d' specified. Must be greater than 0."
E_US0863_2147:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Invalid MAXPAGES of '%0d' specified. Must be greater than 0."
E_US0864_2148:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Invalid NONLEAFFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US0865_2149:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: Invalid LEAFFILL/INDEXFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US0866_2150:E_GE7918_SYNTAX_ERROR
"RETRIEVE INTO: Unrecognized option '%0c' in the WITH clause."
E_US0867_2151:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: duplicate key '%0c'."
E_US0868_2152:E_GE7D00_QUERY_ERROR
"RETRIEVE INTO: The '%0c' entry in the WITH clause
is incompatible with the storage structure."
E_US0869_2153:E_GE75BC_UNKNOWN_OBJECT
"Repeat query is unknown to server.  Please send the definition"
E_US086A_2154:E_GE7918_SYNTAX_ERROR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
of the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n[NO]DUPLICATES, [NO]JOURNALING."
E_US086B_2155:E_GE7918_SYNTAX_ERROR
"RETRIEVE: line %0d, '%1c' is either an invalid option or the syntax
\nof the WITH subclause is invalid.
\nValid keywords for this type of subclause are STRUCTURE, NODE, DATABASE,
\n DBMS, and TABLE."
E_US0871_2161:E_GE759E_COLUMN_UNKNOWN
"Sort column '%0c' not found in target list."
E_US0872_2162:E_GE759E_COLUMN_UNKNOWN
"Sort column '%0d' not found in target list."
E_US0873_2163:E_GE7918_SYNTAX_ERROR
"line %0d, Unrecognized sort order '%1c' in a RETRIEVE command.
Please use one of the following:
\n
\n          ascending, a, descending, d"
E_US0879_2169:E_GE7D00_QUERY_ERROR
"Invalid name specified for a savepoint."
E_US087A_2170:E_GE7D00_QUERY_ERROR
"Attempt to abort to a non-existent savepoint."
E_US087B_2171:E_GE9E34_TRAN_STATE_INV
"You cannot abort when you are not in a multi-query transaction."
E_US087C_2172:E_GE9E34_TRAN_STATE_INV
"You are already in a transaction, try a savepoint."
E_US087D_2173:E_GE9E34_TRAN_STATE_INV
"There is no multi-query transaction to end."
E_US087E_2174:E_GE9E34_TRAN_STATE_INV
"You cannot declare a savepoint if you are not in a
multi-query transaction."
E_US0880_2176:E_GE9E34_TRAN_STATE_INV
"You cannot modify autocommit during a transaction."
E_US0881_2177:E_GE9E34_TRAN_STATE_INV
"No transaction is currently in progress."
E_US0882_2178:E_GE9E34_TRAN_STATE_INV
"You cannot declare a savepoint when autocommit is on."
E_US0883_2179:E_GE7D00_QUERY_ERROR
"Line %0d. CREATE/MODIFY:  Column '%1c' cannot be used as a key field.
The datatype of the field has been restricted from use as a key."
/*%
** Parameters:
**	Line Number
**      Column Name
**
** System Status:
**      An error is returned for the query.  Any in progress transactions are
**	not aborted.
**
** User Action:
**      The column involved has a datatype which is designated as non-keyable.
**	Either use a keyable datatype for the column, or pick a different key.
*/
E_US0884_2180:E_GE7D00_QUERY_ERROR
"Line %0d. INDEX: Column '%1c' cannot be used in a secondary index.
The datatype of the column has been restricted from use in indicies."
/*%
** Parameters:
**	Line Number
**      Column Name
**
** System Status:
**      An error is returned for the query.  Any in progress transactions are
**	not aborted.
**
** User Action:
**      The column involved has a datatype which is designated as non-keyable,
**	which implicitly disqualifies the column for use in a secondary index.
**	Either pick a keyable datatype for the column (in the base table), or do
**	not include this column in the secondary index.
*/
E_US0885_2181:E_GE7D00_QUERY_ERROR
"Column '%0c' cannot be sorted.  The datatype of the column is not sortable."
/*%
** Parameters:
**      Column Name
**
** System Status:
**      An error is returned for the query.  No in progress transactions are
**	aborted.
**
** User Action:
**      The column involved has been specified in a SORT or ORDER BY clause, but
**	the datatype for the column is not sortable.  Do not use the SORT or
**	ORDER BY clause in conjunction with this column.
*/
E_US0886_2182:E_GE7D00_QUERY_ERROR
"Column '%0d' cannot be sorted.  The datatype of the column is not sortable."
/*%
** Parameters:
**      Column Position Number
**
** System Status:
**      An error is returned for the query.  No in progress transactions are
**	aborted.
**
** User Action:
**      The column involved has been specified in a SORT or ORDER BY clause, but
**	the datatype for the column is not sortable.  Do not use the SORT or
**	ORDER BY clause in conjunction with this column.
*/
E_US0898_2200:E_GE7D00_QUERY_ERROR
"line %0d.  If an update column list in cursor '%1c'
\nis not specified, the FOR UPDATE clause must not contain
\nDIRECT or DEFERRED keywords."
E_US0899_2201:E_GE9DD0_CUR_STATE_INV
"line %0d, Cursor '%1c' already open."
E_US089A_2202:E_GE7D00_QUERY_ERROR
"line %0d, Column '%2c' in cursor '%1c' is not updatable
\n	because it is based on a constant or a calculation."
E_US089B_2203:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the cursor
\n	definition contains none or more than one range variable."
E_US089C_2204:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the cursor
\n	definition contains a 'sort by' or 'unique' clause."
E_US089D_2205:E_GE9DD0_CUR_STATE_INV
"line %0d, Cursor '%1c' is not currently open."
E_US089E_2206:E_GE7D00_QUERY_ERROR
"line %0d, Column '%2c' does not exist in cursor '%1c'."
E_US089F_2207:E_GE7D00_QUERY_ERROR
"line %0d, Column '%2c' in cursor '%1c' is not updatable."
E_US08A0_2208:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' was declared 'for update', but uniqueness was
specified in the retrieve statement."
E_US08A1_2209:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' was declared with a 'retrieve into' statement.
No result table is allowed with cursors."
E_US08A2_2210:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is already defined."
E_US08A3_2211:E_GE84D0_NO_PRIVILEGE
"line %0d, Cursor '%1c' does not have delete permission."
E_US08A4_2212:E_GE7D00_QUERY_ERROR
"line %0d, Range variables are not allowed in REPLACE CURSOR statements."
E_US08A5_2213:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' was declared 'for update', but it is
not updatable because the cursor definition contains an
aggregate with a BY list."
E_US08A6_2214:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' was declared 'for update', but it is
not updatable because the cursor definition contains an
aggregate function with a GROUP BY clause."
E_US08A7_2215:E_GE7D00_QUERY_ERROR
"Cursor has been declared for table '%0c' not '%1c'."
E_US08A8_2216:E_GE7D00_QUERY_ERROR
"Repeat queries cannot use 'where current of' clause."
E_US08A9_2217:E_GE9DD0_CUR_STATE_INV
"Cursor '%0c' has not been opened for update."
E_US08AA_2218:E_GE7D00_QUERY_ERROR
"The parameter in the target list of the repeat query is not allowed
in this context."
E_US08AB_2219:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because of implied request
to remove duplicates."
E_US08AC_2220:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because of implied UNION."
E_US08AD_2221:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the specified variable
is a view, which contains either DISTINCT or UNION in its definition."
E_US08AE_2222:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because the specified variable
is an index."
E_US08AF_2223:E_GE7D00_QUERY_ERROR
"line %0d, Cursor '%1c' is not updatable because resulting query has
an aggregate in its target list."
E_US08B0_2224:E_GE7D00_QUERY_ERROR
"line %0d, 'for update' clause in cursor '%1c' contains nonupdatable
column '%2c'."
E_US08B1_2225:E_GE7D00_QUERY_ERROR
"Updatable cursor '%0c' defined on an nonupdatable view '%1c'."
E_US08B2_2226:E_GE7D00_QUERY_ERROR
"Ambiguous dynamically specified cursor name '%0c' in a DELETE or UPDATE statement."
/*%
** Parameters:
**	%0c - the cursor name
**
** Explanation:
**	The dynamically specified cursor name is not unique among currently
**	open cursors.  It may be unique within the original source module
**	but must also be unique within the current transaction.
**
** System Status:
**	The UPDATE/DELETE statement is ignored.  The cursor is left open.
**
** User Action:
**	Modify the original DECLARE CURSOR statement to specify a unique
**	cursor name if a dynamic DELETE/UPDATE statement will be issued.
**	This restriction does not apply to static cursor DELETE/UPDATE
**	statements.
*/
E_US08B3_2227:E_GE7D00_QUERY_ERROR
"Cursor has been declared for table '%0c' owned by '%1c'
\nnot for '%2c' owned by '%3c'."
/*%
** Parameters:
**	%0c - name of the table as it appeared in DECLARE CURSOR
**	%1c - name of the table owner as it was determined in DECLARE CURSOR
**	%2c - name of the table as it was specified in
**	      DELETE/UPDATE ... WHERE CURRENT OF ...
**	%3c - name of the table owner as it was determined in
**	      DELETE/UPDATE ... WHERE CURRENT OF ...
**
** Explanation:
**	Name of the table and/or table owner as they were determined (note that
**	owner name is optional) in UPDATE/DELETE ... WHERE CURRENT OF ... are/is
**	different from those/that determined in DECLARE CURSOR.
**
** System Status:
**	The UPDATE/DELETE statement is ignored.  The cursor is left open.
**
** User Action:
**	Modify UPDATE/DELETE statement to make sure that names of table and
**	table owner are the same as they were determined in DECLARE CURSOR.
**	It is possible that the table which existed when DECLARE CURSOR was
**	executed has since been dropped, in which case user may consider some
**	corrective action (e.g. redeclare the cursor to use an existing table.)
*/
E_US08FC_2300:E_GE7D00_QUERY_ERROR
"line %0d, Argument(s) missing."
E_US08FD_2301:E_GE7D00_QUERY_ERROR
"line %0d, Expecting more arguments."
E_US08FE_2302:E_GE7D00_QUERY_ERROR
"line %0d, The dynamically defined statement '%1c' is either a SELECT statement
that cannot be EXECUTED, or a non-SELECT statement that cannot be OPENED."
E_US08FF_2303:E_GE7918_SYNTAX_ERROR
"line %0d, Invalid keyword '%1c'. Only NAMES option allowed."
E_US0900_2304:E_GE75B2_NOT_FOUND
"line %0d, The dynamically defined statement '%1c' not found.\n
Perhaps a PREPARE or DESCRIBE wasn't successful."
E_US0901_2305:E_GE7D00_QUERY_ERROR
"line %0d, The dynamically defined statement '%1c' is not a SELECT statement."
E_US0902_2306:E_GE7D00_QUERY_ERROR
"line %0d, Parameter markers not allowed in this statement."
E_US0903_2307:E_GE7D00_QUERY_ERROR
"line %0d, Parameter markers not allowed as both operands of a single operator."
E_US0904_2308:E_GE7D00_QUERY_ERROR
"line %0d, Parameter markers not allowed as both the first and second operands used with BETWEEN or IN."
E_US0905_2309:E_GE7D00_QUERY_ERROR
"line %0d, Parameter markers not allowed in the target of a SELECT statement."
E_US0906_2310:E_GE9DD0_CUR_STATE_INV
"Invalid attempt to execute Dynamic SQL statement '%0c'.\n
The statement refers to cursor '%1c' that is already closed."
/*%
** Parameters:
**	%0c - the statement name
**	%1c - the cursor name
**
** Explanation:
**	The statement was prepared from a cursor statement while the specified
**	cursor was open.  By the time the statement was executed the cursor
**	was already closed.
**
** System Status:
**	The EXECUTE statement is ignored.
**
** User Action:
**	Only issue this statement while the cursor is open.  This statement
**	may not apply to the same named cursor that is opened twice.  You
**	must reprepare the statement to associate it with the correct cursor.
*/
E_US0915_2325:E_GEA0F0_SQL_STMT_INV
"The GRANT statement is not allowed in distributed INGRES."
E_US0916_2326:E_GE84D0_NO_PRIVILEGE
"GRANT/REVOKE: only the DBA can use the GRANT and REVOKE statements."
E_US0917_2327:E_GE84D0_NO_PRIVILEGE
"GRANT: Table '%0c' is not owned by you."
E_US0918_2328:E_GE7D00_QUERY_ERROR
"GRANT: Object ('%0c') must not be an index."
E_US0919_2329:E_GE7D00_QUERY_ERROR
"GRANT: Only EXECUTE privilege may be granted on a DB procedure."
E_US091A_2330:E_GE7D00_QUERY_ERROR
"GRANT: EXECUTE privilege may not be granted on a table."
E_US091B_2331:E_GE0032_WARNING
"line %0d, Warning: creating non GRANT compatible permit on table '%1c'."
E_US092E_2350:E_GE7918_SYNTAX_ERROR
"EXECUTE IMMEDIATE: a SELECT statement cannot be a target of this command."
E_US0960_2400:E_GE7D00_QUERY_ERROR
"Procedure '%0c' already exists."
E_US0961_2401:E_GE7D00_QUERY_ERROR
"Parameter/local variable '%0c' already exists."
E_US0962_2402:E_GE7918_SYNTAX_ERROR
"Invalid column format '%0c'."
E_US0963_2403:E_GE7D00_QUERY_ERROR
"Parameter/local variable '%0c' has not been declared."
E_US0964_2404:E_GE84D0_NO_PRIVILEGE
"You are not the owner of '%0c' procedure."
E_US0965_2405:E_GE75B2_NOT_FOUND
"Procedure '%0c' not found."
E_US0966_2406:E_GE7D00_QUERY_ERROR
"Label '%0c' already declared."
E_US0967_2407:E_GE7D00_QUERY_ERROR
"Label '%0c' not declared or out of scope."
E_US0968_2408:E_GE7D00_QUERY_ERROR
"MESSAGE statement requires first parameter to be of INTEGER type."
E_US0969_2409:E_GE7D00_QUERY_ERROR
"MESSAGE statement requires second parameter to be of character type."
E_US096A_2410:E_GE7D00_QUERY_ERROR
"MESSAGE statement can only be followed by a parameter of integer
or character type."
E_US096B_2411:E_GEA0F0_SQL_STMT_INV
"This statement has not been implemented yet."
E_US096C_2412:E_GE7D00_QUERY_ERROR
"line %0d, The outermost target list of the first subselect in a SELECT
statement in a database procedure must retrieve data into local variables/parameters.
Target list element '%1c' violates this rule."
E_US096D_2413:E_GE7D00_QUERY_ERROR
"Local variable/parameter '%0c' has not been declared or is out of scope."
E_US096E_2414:E_GE7D00_QUERY_ERROR
"Local variable/parameter ':%0c' can only be used within a database procedure."
E_US096F_2415:E_GE7D00_QUERY_ERROR
"Line %0d: This type of predicate is not allowed in a condition of a database
procedure control statement."
E_US0970_2416:E_GEA0F0_SQL_STMT_INV
"The assignment statement is not supported yet. Use SELECT statement
instead."
E_US0971_2417:E_GE7D00_QUERY_ERROR
"Target list expression of type '%0c' cannot be coerced into local
variable/parameter of type '%1c'."
E_US0972_2418:E_GE7D00_QUERY_ERROR
"A target list with the INTO clause may be used only in the outermost
level of a SELECT statement."
E_US0973_2419:E_GE7D00_QUERY_ERROR
"A SELECT statement with the INTO clause in the target list must not be
used outside of a database procedure context."
E_US0974_2420:E_GEA0F0_SQL_STMT_INV
"Nesting of BEGIN END blocks is not supported yet."
E_US0975_2421:E_GE7D00_QUERY_ERROR
"Return value must be of non-nullable data type."
E_US0976_2422:E_GE7D00_QUERY_ERROR
"Message statement parameters must be of non-nullable data type."
E_US0977_2423:E_GE8CA0_SYSTEM_LIMIT
"Number of arguments passed to a procedure must not exceed %0d."
E_US0978_2424:E_GE7D00_QUERY_ERROR
"Return value must be of INTEGER type."
E_US0979_2425:E_GE7D00_QUERY_ERROR
"Empty statement blocks are not allowed."
E_US097A_2426:E_GE7D00_QUERY_ERROR
"'ENDLOOP [label]' statement is only allowed in a WHILE."
E_US097B_2427:E_GE7D00_QUERY_ERROR
"INTO clause: number of elements in the parameter/local variable list
doesn't match the number of elements in the target list."
E_US097C_2428:E_GE7D00_QUERY_ERROR
"INTO clause must appear only once in the SELECT statement in
the outermost target list of the first subselect."
E_US097D_2429:E_GE7D00_QUERY_ERROR
"line %0d, INTO clause must not be mixed with '[:]loc_var_name = expression'
way of assigning values to local variables."
E_US097E_2430:E_GE7D00_QUERY_ERROR
"line %0d, The outermost target lists of all subselects other than the first one in a SELECT
with UNION statement in a database procedure must not retrieve data into local variables/parameters.
Target list element '%1c' violates this rule."
E_US097F_2431:E_GE7D00_QUERY_ERROR
"line %0d, Local variables/parameters can appear only in the SELECT statement
in the outermost target list of the first subselect."
E_US0980_2432:E_GE797C_INVALID_IDENT
"`%0c' is not a valid database procedure name.
\n Names starting with 'ii' are reserved for system use."
E_US0981_2433:E_GE797C_INVALID_IDENT
"`%0c' is not a valid parameter/local variable name. Name must not start with `ii'."
E_US0982_2434:E_GE7D00_QUERY_ERROR
"line %0d, IIROWCOUNT and IIERRORNUMBER built-in variables are read-only."
E_US0983_2435:E_GEA0F0_SQL_STMT_INV
"SELECT..UNION..SELECT is not supported in this release of STAR."
E_US0984_2436_RSER_TYPE:E_GE7D00_QUERY_ERROR
"RAISE ERROR parameters must be of type integer and character respectively."
/*
** Parameters:
**	None
** Explanation:
**	The parameters to the RAISE ERROR procedure statement, must be a
**	non-nullable integer error number, optionally followed by a
**	non-nullable error message string.
** System Status:
**	Program continues and current statement is aborted.
** User Action:
**	Redeclare your RAISE ERROR parameters as the correct data types and
**	with the NOT NULL attribute.
*/
E_US0985_2437_RSER_NUL:E_GE7D00_QUERY_ERROR
"RAISE ERROR statement parameters must be of a non-nullable data type."
/*
** Parameters:
**	None
** Explanation:
**	The parameters to the RAISE ERROR procedure statement, must be a
**	non-nullable integer error number, optionally followed by a
**	non-nullable error message string.
** System Status:
**	Program continues and current statement is aborted.
** User Action:
**	Redeclare your RAISE ERROR parameters with the NOT NULL attribute.
*/
E_US0986_2438_EXEC_COL:E_GE7918_SYNTAX_ERROR
"Invalid correlation or column name '%0c' in EXECUTE PROCEDURE statement.
Column expressions of the form [table.]column are not allowed in the
target list." 
/*%
** Parameters:
**	%0 	- Column name or correlation name.
** Explanation:
**	Values in the EXECUTE PROCEDURE target list may be literal values,
**	some symbolic constants (such as USER and NULL) and, when the 
**	statement is nested within a database procedure, a local variable
**	or parameter.  A value such as:
**		EXECUTE PROCEDURE p (a = table.col)
**	is invalid as the meaning is ambiguous (is this a full column
**	aggregate or the column value of "some current row".
** System Status:
**	The current statement is aborted.  If the statement is nested within
**	a database procedure then the procedure creation is aborted.
** User Action:
**	Correct the value usage.
*/
E_US0987_2439_EXEC_TYPE:E_GE7D00_QUERY_ERROR
"The data type of the EXECUTE PROCEDURE result variable '%0c' must be INTEGER."
/*%
** Parameters:
**	%0 	- Result variable name.
** Explanation:
**	The result data type of any procedure is INTEGER.  All variables
**	and parameters that are to retrieve the result of a procedure (via
**	the "result_variable =" clause) must be declared as INTEGER.
** System Status:
**	The current statement is aborted.  If the statement is nested within
**	a database procedure then the procedure creation is aborted.
** User Action:
**	Correct the data type of the specified variable or parameter.
*/
E_US09C4_2500:E_GE7918_SYNTAX_ERROR
"Syntax error on line %0d.  Last symbol read was: '%1c'."
E_US09C5_2501:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SELECT [ALL|DISTINCT] target_list
\n  FROM table(s)
\n  [WHERE search_cond]
\n  [GROUP BY col(s)]
\n  [HAVING search_cond]
\n [UNION subselect]
\n [ORDER BY col(s)]"
E_US09C6_2502:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE TABLE tablename
\n   [(columnname {, ... })]
\n   AS subselect
\n   [with_clause]"
E_US09C7_2503:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n INSERT INTO tablename [(column {, ... })]
\n   VALUES (select_expr {, ... })
\n
\n INSERT INTO tablename [(column {, ... })] subselect"
E_US09C8_2504:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n UPDATE tablename
\n   [FROM tablename]
\n   SET column = select_expr|NULL {, ... }
\n   [WHERE search_condition]"
E_US09C9_2505:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DELETE FROM tablename [WHERE search_condition]"
E_US09CA_2506:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n COPY TABLE tablename (columnname = format [null_clause] {, ... })
\n   INTO|FROM 'filename'
\n   [with_clause]"
E_US09CB_2507:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE TABLE tablename
\n   (columnname format [null_default_clause] {, ... })
\n   [with_clause]
\n
\n CREATE TABLE tablename [(colname_list)]
\n   AS subselect
\n   [with_clause]"
E_US09CC_2508:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP [TABLE|VIEW|INDEX|LINK] object_name {, object_name}"
E_US09CE_2510:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE [UNIQUE] INDEX indexname
\n   ON table (column [ASC|DESC] {, ... })
\n   [with_clause]"
E_US09CF_2511:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n MODIFY table TO storage-structure [UNIQUE]
\n   [ON column [ASC|DESC] {, ... }]
\n   [with_clause]"
E_US09D2_2514:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SAVE tablename UNTIL month day year"
E_US09D3_2515:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n OPEN CURSORID CURSOR FOR fullselect
\n   [FOR [DIRECT | DEFERRED] UPDATE OF column {, ... }]
\n   [FOR READONLY]"
E_US09D5_2517:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE VIEW viewname [ (column {, ... }) ]
\n   AS subselect [WITH CHECK OPTION]"
E_US09D6_2518:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ABORT [TO savepoint_name]"
E_US09D7_2519:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE PERMIT SELECT|UPDATE|DELETE|INSERT
\n   ON|OF|TO table [ (column(s)) ]
\n   TO user_name [AT term] [FROM time TO time]
\n   [ON day TO day] [WHERE search_cond]
\n
\n CREATE PERMIT ALL ..."
E_US09D8_2520:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE INTEGRITY ON tablename IS search_condition"
E_US09D9_2521:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CLOSE cursor_name"
E_US09DA_2522:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n RELOCATE tablename TO locationname"
E_US09DB_2523:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DEFINE QUERY queryname IS delete|select|update
\n
\n DEFINE QUERY open_cursor"
E_US09DC_2524:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n EXECUTE stmnt_name USING const {, ... }"
E_US09DE_2526:E_GE7918_SYNTAX_ERROR
"line %0d Syntax error on '%1c'.  The correct syntax is:
\n BEGIN TRANSACTION"
E_US09DF_2527:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n END TRANSACTION"
E_US09E0_2528:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SAVEPOINT savepoint_name"
E_US09E1_2529:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ABORT [TO savepoint_name]"
E_US09E2_2530:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP INTEGRITY ON table integer {, ... }
\n
\n DROP INTEGRITY ON table ALL"
E_US09E3_2531:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP PERMIT ON table integer {, ... }
\n
\n DROP PERMIT ON table ALL"
E_US09E5_2533:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    CREATE EVENT event_name"
E_US09E6_2534:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n EXECUTE stmnt_name USING const {, ... }"
E_US09E7_2535:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n [result_variable =] EXECUTE PROCEDURE | CALLPROC proc_name
\n        [(parameter = value, ...)]"
E_US09E8_2536:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    DROP EVENT event_name"
E_US09E9_2537:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    REGISTER EVENT [event_owner.]event_name"
E_US09EA_2538:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
    REMOVE EVENT [event_owner.]event_name"
E_US09EB_2539:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
  RAISE EVENT [event_owner.]event_name  [event_text] [WITH [NO]SHARE]"
E_US09EC_2540:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n UPDATE table
\n   SET column = select_expr|NULL {, column = select_expr}
\n   WHERE CURRENT OF cursor_name"
E_US09ED_2541:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n FETCH CURSOR cursor_name"
E_US09F3_2547:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET CPUFACTOR integer"
E_US09F4_2548:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET DATE_FORMAT \"us\" | \"multinational\"
\n   | \"iso\" | \"finland\" | \"sweden\" | \"german\""
E_US09F6_2550:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET DECIMAL \".\" | \",\""
E_US09F7_2551:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET WORK LOCATIONS
\n    ADD | DROP | USE
\n        ( work_location {, work_location} )"
E_US09F8_2552:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET JOINOP [NO]TIMEOUT [milliseconds]"
E_US09F9_2553:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]JOURNALING [ON tablename]"
E_US09FA_2554:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET MONEY_FORMAT \"T|L:string\""
E_US09FB_2555:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET MONEY_PREC integer"
E_US09FE_2558:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET RESULT_STRUCTURE
\n   \"heap | cheap | heapsort | cheapsort | hash
\n   | chash | isam | cisam | btree | cbtree\""
E_US0A01_2561:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]STATISTICS"
E_US0A02_2562:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\nSET LOCKMODE SESSION|ON table
\n WHERE [LEVEL=level_option]
\n [,READLOCK=readlock_option]
\n [,MAXLOCKS=maxlock_option]
\n [,TIMEOUT=timeout_option]"
E_US0A03_2563:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT DMnnnn [integer {, integer }]"
E_US0A04_2564:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT PSnnnn [integer {, integer }]"
E_US0A05_2565:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT SCnnnn [integer {, integer }]"
E_US0A06_2566:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT RDnnnn [integer {, integer }]"
E_US0A07_2567:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT QEnnnn [integer {, integer }]"
E_US0A08_2568:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT OPnnnn [integer {, integer }]"
E_US0A09_2569:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT QSnnnn [integer {, integer }]"
E_US0A0A_2570:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE POINT ADnnnn [integer {, integer }]"
E_US0A0B_2571:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]TRACE ..."
E_US0A0D_2573:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]QEP"
E_US0A0E_2574:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n COMMIT [WORK]"
E_US0A0F_2575:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DESCRIBE stmnt_name [USING NAMES]"
E_US0A10_2576:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n PREPARE stmnt_name [INTO name] FROM statement_text"
E_US0A11_2577:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n PREPARE stmnt_name [INTO name] FROM statement_text"
E_US0A12_2578:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ROLLBACK [WORK] [TO savepoint]"
E_US0A13_2579:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET AUTOCOMMIT ON|OFF"
E_US0A14_2580:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ROLLBACK [WORK] [TO savepoint]"
E_US0A15_2581:E_GEA0F0_SQL_STMT_INV
"line %0d, Syntax error on '%1c'.
\n This SET statement is not supported."
E_US0A16_2582:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET JOINOP [NO]TIMEOUT [milliseconds]"
E_US0A18_2584:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n GRANT ALL|privilege(s) ON [obj_type] object(s)
\n TO {PUBLIC| [grantee_type] grantee(s)}
\n privilege: SELECT|INSERT|DELETE|UPDATE[(col(s))]|EXECUTE."
E_US0A19_2585:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]JOURNALING"
E_US0A1B_2587:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 [CREATE] PROCEDURE procedure[(parms)]\n
    AS | =\n
    [DECLARE local-variables]\n
    BEGIN\n
     DML/control statements\n
    END"
E_US0A1C_2588:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DROP PROCEDURE procedure."
E_US0A1D_2589:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 EXECUTE PROCEDURE procedure."
E_US0A1E_2590:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 IF [(] condition [)] THEN statement-list\n
 {ELSEIF [(] condition [)] THEN statement-list}\n
 [ELSE statement-list]\n
 ENDIF"
E_US0A1F_2591:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 RETURN	[return-status]."
E_US0A20_2592:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 MESSAGE message-number\n
       | message-text\n
       | message-number message-text"
E_US0A21_2593:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DECLARE var_name {, var_name} [=] var_type;\n
        {var_name {, var_name} [=] var_type;}"
E_US0A22_2594:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 [label:] WHILE [(] condition [)] DO\n
                statement-list\n
          ENDWHILE\n
Use of ENDLOOP [label] is allowed."
E_US0A23_2595:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 [:] parm/local var [:] = expression"
E_US0A24_2596:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT CONNECT\n
     [WITH\n
	[NODE = [\']node_name[\'],\n
	 DATABASE = [\']database_name[\']]\n
	[,DBMS = [\']ingres | db2 | ds | ims | dg ...[\']]]" 
E_US0A25_2597:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT DISCONNECT"
E_US0A27_2599:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT EXECUTE IMMEDIATE \'query_text\'\n
     [WITH\n
	[NODE = [\']node_name[\'],\n
	 DATABASE = [\']ldb_name[\']]\n
	[,DBMS = [\']dbms_name[\']]]" 
E_US0A28_2600:E_GE7918_SYNTAX_ERROR
"Syntax error on line %0d.  Last symbol read was: %1c."
E_US0A29_2601:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE [[INTO] tablename] (target_list)
\n                [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]
\n        RETRIEVE UNIQUE (target_list) [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]"
E_US0A2A_2602:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE [[INTO] tablename] (target_list)
\n                [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]
\n        RETRIEVE UNIQUE (target_list) [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]"
E_US0A2B_2603:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        APPEND [TO] tablename (target_list) [WHERE qual]"
E_US0A2C_2604:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        REPLACE row_variable (target_list) [WHERE qual]"
E_US0A2D_2605:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DELETE row_variable [WHERE qual]"
E_US0A2E_2606:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        COPY tablename (columnname = format [null_clause] {, ... })
\n            INTO|FROM \"filename\"
\n            [with_clause]"
E_US0A2F_2607:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        CREATE tablename
\n            (columnname1 = format{, columnname2 = format})
\n	      [with clause]"
E_US0A30_2608:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DESTROY [LINK] object_name {, object_name}"
E_US0A31_2609:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP
\n        HELP tablename {,tablename}
\n        HELP ALL"
E_US0A32_2610:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        INDEX ON tablename IS indexname (column1{, column2})"
E_US0A33_2611:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n MODIFY tablename TO storage-structure
\n  [ON key1 [: sortorder] [{, key2 [:sortorder]}]]
\n  [with_clause]"
E_US0A34_2612:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        PRINT tablename{, tablename}"
E_US0A35_2613:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RANGE OF variable IS tablename"
E_US0A36_2614:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SAVE tablename UNTIL month day year"
E_US0A37_2615:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        OPEN CURSOR CURSORID <retrieve statement> [FOR [DIRECT | DEFERRED]
\n                UPDATE [OF (<column list>)]] [FOR READONLY]"
E_US0A38_2616:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE [[INTO] tablename] (target_list)
\n                [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]
\n        RETRIEVE UNIQUE (target_list) [WHERE qual] [SORT [BY]
\n                columnname1 [:sortorder] {, columnname2 [:sortorder]} ]"
E_US0A39_2617:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DEFINE VIEW name (target list) [WHERE qual]"
E_US0A3A_2618:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        ABORT TO savepoint_name"
E_US0A3B_2619:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DEFINE PERMIT oplist ON|OF|TO var [(columnlist)]
\n                TO name [AT term] [FROM time TO time]
\n                [ON day TO day] [WHERE qual]"
E_US0A3C_2620:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DEFINE INTEGRITY ON var IS qual"
E_US0A3D_2621:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A3E_2622:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RELOCATE tablename TO locationname"
E_US0A3F_2623:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A40_2624:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        EXECUTE query_name [( parameter, ... )]"
E_US0A41_2625:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A42_2626:E_GE7918_SYNTAX_ERROR
"line %0d Syntax error on '%1c'.  The correct syntax is:
\n        BEGIN TRANSACTION"
E_US0A43_2627:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        END TRANSACTION"
E_US0A44_2628:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SAVEPOINT savepoint_name"
E_US0A45_2629:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        ABORT"
E_US0A46_2630:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DESTROY INTEGRITY tablename integer{, integer}
\n        DESTROY INTEGRITY tablename ALL"
E_US0A47_2631:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        DESTROY PERMIT tablename integer{, integer}
\n        DESTROY PERMIT tablename ALL"
E_US0A48_2632:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A49_2633:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A4A_2634:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        EXECUTE CURSOR CURSORID [( parameter, ...)]"
E_US0A4B_2635:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP INTEGRITY tablename[, tablename]"
E_US0A4C_2636:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP PERMIT tablename[, tablename]"
E_US0A4D_2637:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        HELP VIEW tablename[, tablename]"
E_US0A4E_2638:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A4F_2639:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:"
E_US0A50_2640:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        REPLACE CURSOR CURSORID (target list)"
E_US0A51_2641:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        RETRIEVE CURSOR CURSORID"
E_US0A52_2642:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ1 integer"
E_US0A53_2643:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ2 integer"
E_US0A54_2644:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ3 integer"
E_US0A55_2645:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET J_FREESZ4 integer"
E_US0A56_2646:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET AGGREGATE [NO]PROJECT"
E_US0A57_2647:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET CPUFACTOR integer"
E_US0A58_2648:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET DATE_FORMAT \"us\" | \"multinational\"
\n			 | \"iso\" | \"finland\" | \"sweden\" | \"german\""
E_US0A59_2649:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]DEADLOCK"
E_US0A5A_2650:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET DECIMAL \".\" | \",\""
E_US0A5B_2651:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]EMBEDDED
\n        SET [NO]EQUEL"
E_US0A5C_2652:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]JOINOP"
E_US0A5D_2653:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]JOURNALING [ON tablename]"
E_US0A5E_2654:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET MONEY_FORMAT \"T|L:string\""
E_US0A5F_2655:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n	  SET MONEY_PREC integer"
E_US0A60_2656:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET QBUFSIZE integer"
E_US0A61_2657:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET QUERY_SIZE integer"
E_US0A62_2658:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET RET_INTO
\n             \"heap | cheap | heapsort | cheapsort | hash
\n              | chash | isam | cisam | btree | cbtree\""
E_US0A63_2659:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET SORTBUFSIZE integer"
E_US0A64_2660:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET SQL"
E_US0A65_2661:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET STATISTICS \"tablename\""
E_US0A66_2662:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\nSET LOCKMODE SESSION|ON table
\n WHERE [LEVEL=PAGE|TABLE|SESSION|SYSTEM]
\n [,READLOCK=NOLOCK|SHARED|EXCLUSIVE|SESSION|SYSTEM]
\n [,MAXLOCKS|TIMEOUT=int|SESSION|SYSTEM]"
E_US0A67_2663:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT DMnnnn [integer [, integer]]"
E_US0A68_2664:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT PSnnnn [integer [, integer]]"
E_US0A69_2665:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT SCnnnn [integer [, integer]]"
E_US0A6A_2666:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT RDnnnn [integer [, integer]]"
E_US0A6B_2667:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT QEnnnn [integer [, integer]]"
E_US0A6C_2668:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT OPnnnn [integer [, integer]]"
E_US0A6D_2669:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT QSnnnn [integer [, integer]]"
E_US0A6E_2670:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE POINT ADnnnn [integer [, integer]]"
E_US0A6F_2671:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n        SET [NO]TRACE ..."
E_US0A79_2681:E_GEA0F0_SQL_STMT_INV
"line %0d, Syntax error on '%1c'.
\n This SET statement is not supported."
E_US0A7A_2682:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n	  SET JOINOP [NO]TIMEOUT [milliseconds]"
E_US0A7B_2683:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. The correct syntax is:
\n	  DEFINE LOCATION location_name WITH TYPE= DATA,
\n	       AREA = \"area_name\""
E_US0A7D_2685:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n	  SET [NO]JOURNALING"
E_US0A88_2696:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT CONNECT\n
     [WITH\n
	[NODE = [\"]node_name[\"],\n
	 DATABASE = [\"]database_name[\"]]\n
	[,DBMS = [\"]ingres | db2 | ds | ims | dg ...[\"]]]" 
E_US0A89_2697:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT DISCONNECT"
E_US0A8B_2699:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:\n
 DIRECT EXECUTE IMMEDIATE \"query_text\"\n
     [WITH\n
	[NODE = [\"]node_name[\"],\n
	 DATABASE = [\"]database_name[\"]]\n
	[,DBMS = [\"]ingres | db2 | ds | ims | dg ...[\"]]]" 
E_US0A8C_2700:E_GE7918_SYNTAX_ERROR
"line %0d, Non-terminated string."
E_US0A8D_2701:E_GE8CA0_SYSTEM_LIMIT
"line %0d, String too long."
E_US0A8E_2702:E_GE7918_SYNTAX_ERROR
"line %0d, Invalid operator '%1c'."
E_US0A8F_2703:E_GE797C_INVALID_IDENT
"line %0d, Name too long: '%1c'."
E_US0A90_2704:E_GE8D04_NO_RESOURCE
"Attempt to allocate symbol table space failed."
E_US0A91_2705:E_GE7918_SYNTAX_ERROR
"line %0d, Non-terminated comment."
E_US0A92_2706:E_GE7918_SYNTAX_ERROR
"Missing closing delimiter in the delimited identifier."
/*%
** Parameters:
**	none
**
** Explanation:
**	Scanner encountered end of the query buffer while trying to read a
**	delimited identifier.  This error can be caused by a missing closing
**	double quote.
** System Status:
**      The query is rejected.
**
** User Action:
**      Correct the error and reenter.
*/
E_US0A93_2707:E_GE7D00_QUERY_ERROR
"line %0d, Bad floating constant: '%1c'."
E_US0A94_2708:E_GE7D00_QUERY_ERROR
"line %0d, Unexpected character '%1c'."
E_US0A95_2709:E_GE7918_SYNTAX_ERROR
"line %0d, Bad value in hex constant: '%1c'."
E_US0A96_2710:E_GE7918_SYNTAX_ERROR
"line %0d, Illegal character in string constant."
E_US0A97_2711:E_GE8CA0_SYSTEM_LIMIT
"line %0d, Symbol stack overflow. Reduce target list complexity."
E_US0A98_2712:E_GE7918_SYNTAX_ERROR
"line %0d, Illegal pattern matching notation."
E_US0A99_2713:E_GE7918_SYNTAX_ERROR
"line %0d, String constant '%1c' specified for the ESCAPE clause
\nmust be a single character."
E_US0A9A_2714:E_GE7918_SYNTAX_ERROR
"Delimited identifier \"%0c\" is too long."
/*%
** Parameters:
**	%0c - the delimited identifier.
**
** Explanation:
**	Delimited identifier being read exceeds the maximum length defined for
**	an INGRES name.
**
** System Status:
**      The query is rejected.
**
** User Action:
**      Correct the error and reenter.
*/
E_US0A9C_2716:E_GE7918_SYNTAX_ERROR
"line %0d, Illegal binary value in query text."
E_US0A9D_2717:E_GE7918_SYNTAX_ERROR
"line %0d, Illegal character in query."
E_US0A9E_2718:E_GE7918_SYNTAX_ERROR
"line %0d, Illegal location type '%1c'. Use DATA."
E_US0A9F_2719:E_GE7D00_QUERY_ERROR
"line %0d, Illegal area name '%1c'."
E_US0AA0_2720:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. Make sure that the specified column name
is not a keyword and is a valid identifier."
E_US0AA1_2721:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. Make sure that the specified table location
or table name is not a keyword and is a valid identifier."
E_US0AA2_2722:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. An integer constant expected."
E_US0AA3_2723:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. A string constant expected."
E_US0AA4_2724:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. Make sure that the specified table name is
not a keyword and is a valid identifier."
E_US0AA5_2725:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'. Make sure that the specified index location
or index name is not a keyword and is a valid identifier."
E_US0ABE_2750:E_GE7D00_QUERY_ERROR
"DROP TABLE: object '%0c' is not a base table."
E_US0ABF_2751:E_GE7D00_QUERY_ERROR
"DROP VIEW: object '%0c' is not a view."
E_US0AC0_2752:E_GE7D00_QUERY_ERROR
"DROP INDEX: object '%0c' is not an index."
E_US0AC1_2753:E_GE7594_TABLE_NOT_FOUND
"%0c: '%1c' does not exist or is not owned by you."
E_US0AC2_2754:E_GE7D00_QUERY_ERROR
"DROP: Too many objects specified."
E_US0AC3_2755:E_GE7D00_QUERY_ERROR
"DROP/DESTROY %0c: STAR object '%1c' was
\n created via CREATE %2c.  Use DROP [%2c] or DESTROY."
E_US0AC4_2756:E_GE7D00_QUERY_ERROR
"DROP/DESTROY %0c:  STAR object '%1c' was used
\n to REGISTER an LDB object of type '%2c'.
\n Use DROP [%2c] or DESTROY."
E_US0AC5_2757:E_GE7D00_QUERY_ERROR
"REMOVE %0c:  STAR object '%1c' corresponds to an LDB\n
object of type '%2c'.  Use REMOVE [%2c]."
E_US0AC6_2758:E_GE7D00_QUERY_ERROR
"DROP LINK/REMOVE: STAR object '%0c' was created using
\n CREATE VIEW.  Use DROP [VIEW]."
E_US0AC8_2760:E_GEA0F0_SQL_STMT_INV
"SET RESULT_STRUCTURE is not allowed in distributed INGRES."
E_US0AC9_2761:E_GE7918_SYNTAX_ERROR
"SET: invalid storage structure '%0c'."
E_US0ACA_2762:E_GE7D00_QUERY_ERROR
"SET [NO]JOURNALING is not allowed on catalogs.\n
`%0c' is a catalog."
E_US0ACB_2763:E_GE7918_SYNTAX_ERROR
"REMOVE: syntax error on '%0c'.  The correct syntax is:\n
REMOVE [TABLE|VIEW] object_name {, object_name}"
E_US0ACC_2764:E_GE84D0_NO_PRIVILEGE
"%0c: '%1c' is not owned by %2c."
E_US0ACD_2765:E_GE84D0_NO_PRIVILEGE
"You may not %0c '%1c' owned by %2c."
E_US0ACE_2766:E_GE7D00_QUERY_ERROR
"SET [NO]JOURNALING is not allowed on indices.\n
`%0c' is an index."
E_US0ACF_2767:E_GEA0F0_SQL_STMT_INV
"The SET WORK LOCATIONS statement is not allowed in distributed INGRES."
E_US0AD6_2774:E_GE7D00_QUERY_ERROR
"INSERT: Duplicate column '%0c' specified."
E_US0AD7_2775:E_GE7D00_QUERY_ERROR
"INSERT: number of target columns must equal the number of specified values."
E_US0AD8_2776:E_GE7D00_QUERY_ERROR
"INSERT: number of target columns must equal the number of target list
elements in the specified subselect."
E_US0AD9_2777:E_GE7D00_QUERY_ERROR
"INSERT: Must not insert NULL value into a nonnullable column '%0c'."
E_US0ADA_2778:E_GE7D00_QUERY_ERROR
"INSERT: Only constant expressions allowed in the value list."
E_US0ADB_2779:E_GE7D00_QUERY_ERROR
"No value specified for mandatory column '%0c'."
E_US0ADC_2780:E_GE7D00_QUERY_ERROR
"UPDATE: Column '%0c' is nonnullable and cannot accept a NULL value."
E_US0ADD_2781:E_GE7D00_QUERY_ERROR
"INSERT: No value specified for mandatory column '%0c'
\nof base table '%1c' on which view '%2c' is based."
E_US0AF0_2800:E_GE8CA0_SYSTEM_LIMIT
"line %0d, Stack overflow while parsing query.
Please simplify the query."
E_US0AF2_2802:E_GEA0F0_SQL_STMT_INV
"line %0d, The RETRIEVE INTO statement is not allowed in OEM INGRES."
E_US0B59_2905:E_GE7918_SYNTAX_ERROR
"Unary PLUS or MINUS operators may be attached only to numeric literals
in this context.  Expressions, symbolic constants or column values may not
be preceded by a unary operator."
/*%
** Parameters:
**	None
** Explanation:
**	In this statement an expression in the target list must be a numeric
**	literal in order for it to be preceded by a unary plus or minus.  This
**	is likely to occur in a statement that does not allow expressions in
**	its target lists.
** System Status:
**	Program continues and current statement is aborted.
** User Action:
**	If this is a syntax error then correct the error.  If your program
**	requires the unary operator on a dynamic value then either calculate
**	the negative value prior to the use of the value or consider delaying 
**	the assumption that the value is negative until later in the program.
*/
E_US0B5A_2906:E_GE7918_SYNTAX_ERROR
"line %0d, There is no such function as '%1c'."
E_US0B5B_2907:E_GE7D00_QUERY_ERROR
"line %0d, Function '%1c' does not take an argument of type '%2c'.
Explicitly convert the argument to the desired type."
/*%
** Parameters:
**	%0  - line of script the error occurred in
**	%1  - name of function invoked
**	%2  - first argument of function %1
** Explanation:
**	The function specified takes an argument of a specified type.  See the
**	appropriate Ingres Language reference manual section on functions.
** System Status:
**	Program continues and current statement is aborted.
** User Action:
**	Convert the argument to the required type.
*/
E_US0B5C_2908:E_GE7D00_QUERY_ERROR
"line %0d, Function '%1c' is not defined for arguments of type '%2c' and '%3c'.
Explicitly convert its arguments to the desired type(s)."
/*%
** Parameters:
**	%0  - line of script the error occurred in
**	%1  - name of function invoked
**	%2  - first argument of function %1
**	%3  - second argument of function %2
** Explanation:
**	The function specified takes arguments of a specified type.  See the
**	appropriate Ingres Language reference manual section on functions.
** System Status:
**	Program continues and current statement is aborted.
** User Action:
**	Convert the arguments to the required type, e.g.
**	change
**	    retrieve (x=concat(str_field, int_field))
**	to
**	    retrieve (x=concat(str_field, c(int_field))
*/
E_US0B5D_2909:E_GE7D00_QUERY_ERROR
"line %0d, Function '%1c' with argument type '%2c' is ambiguous.
Explicitly convert its argument to the desired type."
E_US0B5E_2910:E_GE7D00_QUERY_ERROR
"line %0d, Operator '%1c' with operand(s) '%2c' and '%3c' is ambiguous."
E_US0B5F_2911:E_GE7D00_QUERY_ERROR
"line %0d, Datatype '%1c' can't be converted to '%2c'."
E_US0B60_2912:E_GE7D00_QUERY_ERROR
"line %0d, Aggregate function '%1c' with result type '%2c' is ambiguous.
Explicitly convert the function or its arguments to the desired
type(s)."
E_US0B61_2913:E_GE7D00_QUERY_ERROR
"line %0d, You cannot assign  a value of type '%1c' to a column of
type '%2c'. Explicitly convert the value to the required type."
E_US0B62_2914:E_GE7918_SYNTAX_ERROR
"line %0d, You cannot have an aggregate function in a WHERE clause."
E_US0B63_2915:E_GE7918_SYNTAX_ERROR
"line %0d, The columns in the SELECT clause must be contained in
the GROUP BY clause."
E_US0B64_2916:E_GE7D00_QUERY_ERROR
"line %0d, Too many select items in SUBSELECT."
E_US0B65_2917:E_GE7D00_QUERY_ERROR
"line %0d, Number of target list elements in all UNIONed subselects
must be the same."
E_US0B66_2918:E_GE7D00_QUERY_ERROR
"line %0d, SELECT item types '%1c' and '%2c' are not coercible
to the same type."
E_US0B67_2919:E_GE7D00_QUERY_ERROR
"line %0d, Conversion of types '%1c' and '%2c' to the same type is ambiguous."
E_US0B68_2920:E_GE7918_SYNTAX_ERROR
"line %0d, Illegal expression in the HAVING clause."
E_US0B69_2921:E_GE7D00_QUERY_ERROR
"line %0d, Table reference '%1c' is ambiguous."
E_US0B6A_2922:E_GE7918_SYNTAX_ERROR
"line %0d, Queries without from lists may not contain HAVING,
WHERE, or GROUP BY clauses."
E_US0B6B_2923:E_GE7D00_QUERY_ERROR
"line %0d, Aggregates cannot contain correlated variables."
E_US0B6C_2924:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error.
There are too many entries in the WITH or WHERE clause."
E_US0B6D_2925:E_GE7D00_QUERY_ERROR
"SELECT: FOR UPDATE clause found in non-updatable SELECT statement.
\n	May not use set function, GROUP, HAVING, DISTINCT, UNION, ORDER or more
\n	than one variable in the FROM clause, at the outermost query level."
E_US0B6E_2926:E_GE7918_SYNTAX_ERROR
"DESCRIBE: invalid USING '%0c' clause. May specify NAMES only."
E_US0B6F_2927:E_GE7D00_QUERY_ERROR
"UNION and UNION ALL set operators must not be mixed."
E_US0B70_2928:E_GE7D00_QUERY_ERROR
"line %0d, aggregate function '%1c' not allowed in this context."
E_US0B71_2929:E_GE7D00_QUERY_ERROR
"Value of host language variable of type '%0c' must not be negative."
E_US0B72_2930:E_GE7D00_QUERY_ERROR
"line '%0d'.  Illegal use of aggregate function in a WHERE clause."
/*%
** Parameters:
**	%0  - line of script the error occurred in
** Explanation:
**	given
**	    [SELECT ...] SELECT ... FROM from_list WHERE where_clause ...,
**	use of aggregate(x.col_name) where x is a part of the from_list will be
**	prohibited.  This constitutes a relaxation of the restriction as it
**	existed in 6.3 where use of correlated aggregates was prohibited + use
**	of aggregates in WHERE clause outside of SUBSELECT was also prohibited.
** System Status:
**	Current statement is aborted and the program continues.
** User Action:
**	Correct error and reenter
*/
E_US0C1C_3100:E_GE8CA0_SYSTEM_LIMIT
"Query requires too many range variables."
E_US0CE4_3300:E_GE7D00_QUERY_ERROR
"Cannot update a view."
E_US0CEE_3310:E_GE7D00_QUERY_ERROR
"%0c on view %1c: can only update simple columns. No constants or
expressions."
E_US0CF8_3320:E_GE7D00_QUERY_ERROR
"%0c on view %1c: column occurs in qualification of view."
E_US0CF9_3321:E_GE7D00_QUERY_ERROR
"%0c on view '%1c' may not be performed because definition
\nof '%1c' involved WITH CHECK OPTION and a qualification."
E_US0D02_3330:E_GE7D00_QUERY_ERROR
"%0c on view %1c: update of a multi-variable view not allowed."
E_US0D0C_3340:E_GE7D00_QUERY_ERROR
"%0c on view %1c: views do not have TID's."
E_US0D16_3350:E_GE7D00_QUERY_ERROR
"%0c on view %1c: cannot update views containing an aggregate."
E_US0D20_3360:E_GE7D00_QUERY_ERROR
"%0c on view %1c: that update might be non-functional."
E_US0D21_3361:E_GE7D00_QUERY_ERROR
"%0c on view %1c: cannot update this view."
E_US0D2A_3370:E_GE7594_TABLE_NOT_FOUND
"%0c on view %1c: base table %2c does not exist."
E_US0D34_3380:E_GE84D0_NO_PRIVILEGE
"No permission to update base table '%0c'."
E_US0D35_3381:E_GE7594_TABLE_NOT_FOUND
"Permission on the subject table has qualification that
references nonexistent table."
E_US0D36_3382:E_GE84D0_NO_PRIVILEGE
"User has no permission to execute database procedure '%0c'."
E_US0DA2_3490:E_GE7D00_QUERY_ERROR
"INTEGRITY on %0c: cannot handle aggregates."
E_US0DA3_3491:E_GE7D00_QUERY_ERROR
"INTEGRITY on %0c: cannot handle multivariable constraints."
E_US0DA4_3492:E_GE7D00_QUERY_ERROR
"INTEGRITY on %0c: constraint does not initially hold."
E_US0DA5_3493:E_GE80E8_LOGICAL_ERROR
"INTEGRITY on %0c: cannot define integrity on a view."
E_US0DA6_3494:E_GE84D0_NO_PRIVILEGE
"INTEGRITY on %0c: you must own %0c."
E_US0DA7_3495:E_GEA0F0_SQL_STMT_INV
"The CREATE/DEFINE INTEGRITY command is not allowed in distributed INGRES."
E_US0DA8_3496:E_GE7D00_QUERY_ERROR
"You cannot define an integrity constraint on a TID."
E_US0DA9_3497:E_GE7D00_QUERY_ERROR
"INTEGRITY on %0c: cannot have subselects in constraint."
E_US0DAA_3498:E_GE84D0_NO_PRIVILEGE
"line %0d, You are not allowed to create integrity on '%1c' table."
E_US0DAC_3500:E_GE84D0_NO_PRIVILEGE
"%2c on table %0c: protection violation."
E_US0DAD_3501:E_GE8CA0_SYSTEM_LIMIT
"Number of protection qualifications for %0c in this query
exceeds maximum (%1d)."
E_US0DAE_3502:E_GE84D0_NO_PRIVILEGE
"%2c on table %0c: no GRANT or GRANT compatible permit exists."
E_US0E06_3590:E_GE7D00_QUERY_ERROR
"PERMIT: bad terminal identifier '%0c'."
E_US0E07_3591:E_GEA028_USER_ID_INV
"PERMIT: bad user name '%0c'."
E_US0E08_3592:E_GE84D0_NO_PRIVILEGE
"PERMIT: Table '%0c' not owned by you."
E_US0E09_3593:E_GE7D00_QUERY_ERROR
"PERMIT: Table '%0c' must be a real table (not a view)."
E_US0E0A_3594:E_GE7D00_QUERY_ERROR
"PERMIT on %0c: bad day-of-week '%1c'."
E_US0E0B_3595:E_GE84D0_NO_PRIVILEGE
"PERMIT on %0c: only the DBA can use the PERMIT statement."
E_US0E0C_3596:E_GEA0F0_SQL_STMT_INV
"The CREATE/DEFINE PERMIT statement is not allowed in distributed INGRES."
E_US0E0D_3597:E_GE7D00_QUERY_ERROR
"PERMIT: Error on '%0c'. Views are not allowed in the qualification."
E_US0E0E_3598:E_GE7D00_QUERY_ERROR
"GRANT: You cannot grant permissions on views created in QUEL."
E_US0E0F_3599:E_GEA0F0_SQL_STMT_INV
"CREATE PERMIT: This release of STAR only supports creation of
\n	ALL TO ALL and SELECT TO ALL permits."
E_US0E74_3700:E_GE7918_SYNTAX_ERROR
"Tree buffer overflow in query modification."
E_US0E75_3701:E_GE8CA0_SYSTEM_LIMIT
"Tree build stack overflow in query modification."
/*
** NOTE: range 3800-3999 is RESERVED for syntax error messages.
** PLEASE do not use it for any other purposes
*/
E_US0ED8_3800:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 CREATE LINK link_name [(col_list)]\n
     [WITH\n
	[NODE = [\']node_name[\'],\n
	 DATABASE = [\']ldb_name[\']]\n
	[,DBMS = [\']dbms_name[\']\n
	[,TABLE = [\']table_name[\']]]"
E_US0ED9_3801:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REGISTER [TABLE|VIEW] objname [(col_name {, col_name})]
\n	AS LINK [FROM source] [with_clause]
\n
\n REGISTER objname AS LINK WITH REFRESH"
E_US0EDA_3802:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 REMOVE [TABLE|VIEW] object_name {, object_name}"
E_US0EDC_3804:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 REGISTER objname AS LINK WITH REFRESH"
E_US0EDD_3805:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
SET DDL_CONCURRENCY ON | OFF"
E_US0EE2_3810:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  Correct syntax is:\n
 CREATE RULE rul\n
	AFTER DELETE|INSERT|UPDATE[(col)] ON tbl\n
	[REFERENCING new_old] [WHERE qual]\n
	|\n
	AT date [REPEAT intvl]\n
    EXECUTE PROCEDURE proc [(plist)]"
E_US0EE3_3811:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 DROP RULE rule_name"
E_US0EE5_3813:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE GROUP groupname {,groupname}
\n   [WITH USERS = (user {,user})]"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the CREATE GROUP statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EE6_3814:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER GROUP groupname {,groupname}
\n     ADD  USERS (user {,user})
\n   | DROP USERS (user {,user})"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the ALTER GROUP statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EE7_3815:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER GROUP groupname {,groupname}
\n     ADD  USERS (user {,user})
\n   | DROP USERS (user {,user})"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the ALTER GROUP...DROP  statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EE8_3816:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP GROUP groupname {,groupname}"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the DROP GROUP statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EE9_3817:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE ROLE roleid {,roleid}
\n   WITH
\n          PASSWORD = 'password'
\n        | NOPASSWORD"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the CREATE ROLE statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EEA_3818:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER ROLE roleid {,roleid}
\n   WITH
\n          PASSWORD = 'password'
\n        | NOPASSWORD"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the ALTER ROLE statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EEB_3819:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP ROLE roleid {,roleid}"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the DROP ROLE statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EEC_3820:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n REVOKE dbpriv {,dbpriv}
\n  ON DATABASE dbname {,dbname}
\n  FROM [authtype] authid {,authid}"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the REVOKE statement
**	for database privileges.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EED_3821:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n GRANT dbpriv {,dbpriv}
\n  ON DATABASE dbname {,dbname}
\n  TO [authtype] authid {,authid}"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the GRANT statement
**	for database privileges.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EEE_3822:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n REVOKE dbpriv {,dbpriv}
\n  ON DATABASE dbname {,dbname}
\n  FROM [authtype] authid {,authid}"
/*%
** Parameters:
**	0 - line number
**	1 - token
**
** Explanation:
**	Syntax error while parsing the REVOKE statement
**	for database privileges.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF0_3824:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]MAXxxxx [nnnn]
\n  where xxxx is IO, QUERY, or ROW;
\n  nnnn is a positive integer less than or equal to
\n  the value of QUERY_xxxx_LIMIT for your session.
\n  A value must not be specified with [NO]MAXxxxx."
/*%
** Parameters:
**	(none)
**
** Explanation:
**	Syntax error while parsing the SET MAX... statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF1_3825:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE USER <user_name>
\n  [WITH
\n	[  PRIVILEGES = (<priv> {,<priv>})  |  NOPRIVILEGES]
\n	[, GROUP = '<group>'  | NOGROUP]
\n  ]
\n
\n  where <priv> is CREATEDB, TRACE, SECURITY, OPERATOR,
\n		    SYSTEM_ADMIN, or AUDIT_ALL
\n  and <group> is the user's default group."
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the CREATE USER statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF2_3826:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER USER <user_name>
\n  WITH
\n	[  PRIVILEGES = (<priv> {,<priv>})  |  NOPRIVILEGES]
\n	[, GROUP = '<group>'  | NOGROUP]
\n
\n  where <priv> is CREATEDB, TRACE, SECURITY, OPERATOR,
\n		    SYSTEM_ADMIN, or AUDIT_ALL
\n  and <group> is the user's default group."
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the ALTER USER statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF3_3827:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP USER <user_name>"
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the DROP USER statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF4_3828:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE LOCATION <location_name>
\n  WITH
\n	   AREA  = '<area_name>'
\n	[, USAGE = (<usage> {,<usage>}) | NOUSAGE]
\n
\n  where <usage> is DATABASE, SORT, JOURNAL, CHECKPOINT, DUMP,
\n		     ALL, or NONE."
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the CREATE LOCATION statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF5_3829:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ALTER LOCATION <location_name>
\n  WITH USAGE = (<usage> {,<usage>}) | NOUSAGE
\n
\n  where <usage> is DATABASE, SORT, JOURNAL, CHECKPOINT, DUMP,
\n		     ALL, or NONE."
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the ALTER LOCATION statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF6_3830:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP LOCATION <location_name>"
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the DROP LOCATION statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF7_3831:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n CREATE SECURITY_ALARM
\n  ON [TABLE] <table_name> {,<table_name>}
\n  [IF <condition> {,<condition>}]
\n  [WHEN <privilege> {,<privilege>}]
\n  [BY [USER] <user_name> {,<user_name>} | PUBLIC]
\n
\n  where
\n	<condition> is SUCCESS or FAILURE
\n	<privilege> is SELECT, UPDATE, INSERT or DELETE"
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the CREATE SECURITY_ALARM statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF8_3832:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DROP SECURITY_ALARM on <table_name> <number> {,<number>}
\n
\n  where <number> is the security alarm number as returned
\n		    by 'HELP SECURITY_ALARM <table_name>'"
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the DROP SECURITY_ALARM statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EF9_3833:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n ENABLE SECURITY_AUDIT <audit_type>
\n
\n  where <audit_type> is one of:
\n	    ALL
\n	    ROW
\n	    TABLE
\n	    VIEW
\n	    DATABASE
\n	    PROCEDURE
\n	    LOCATION
\n	    USER
\n	    SECURITY
\n	    ALARM
\n	    ROLE
\n	    RULE
\n	    EVENT
\n	    LEVEL('<security_label>')"
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the ENABLE SECURITY_AUDIT statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EFA_3834:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n DISABLE SECURITY_AUDIT <audit_type>
\n
\n  where <audit_type> is one of:
\n	    ALL
\n	    ROW
\n	    TABLE
\n	    VIEW
\n	    DATABASE
\n	    PROCEDURE
\n	    LOCATION
\n	    USER
\n	    SECURITY
\n	    ALARM
\n	    ROLE
\n	    RULE
\n	    EVENT
\n	    LEVEL('<security_label>')"
/*%
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the DISABLE SECURITY_AUDIT statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US0EFB_3835:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n  COMMENT ON TABLE tbl_name [IS long_remark]
\n	[WITH SHORT_REMARK = short_remark]
\n
\n		or
\n
\n  COMMENT ON COLUMN tbl_name.column_name [IS long_remark]
\n	[WITH SHORT_REMARK = short_remark]."
/*
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the COMMENT ON TABLE/COLUMN statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
*/
E_US0EFC_3836:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n  CREATE SYNONYM synonym_name FOR [owner.]tbl_name."
/*
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the CREATE SYNONYM statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
*/
E_US0EFD_3837:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n
\n  DROP SYNONYM synonym_name {, synonym_name}."
/*
** Parameters:
**	%0 - line number
**	%1 - last token parsed
**
** Explanation:
**	Syntax error while parsing the DROP SYNONYM statement.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
*/
E_US0F3C_3900:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 DEFINE LINK link_name [(col_list)]\n
     [WITH\n
	[NODE = [\"]node_name[\"],\n
	 DATABASE = [\"]ldb_name[\"]]\n
	[,DBMS = [\"]dbms_name[\"]\n
	[,TABLE = [\"]table_name[\"]]]"
E_US0F3D_3901:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:
\n REGISTER [TABLE|VIEW] objname [(col_name {, col_name})]
\n	AS LINK [FROM source] [with_clause]
\n
\n REGISTER objname AS LINK WITH REFRESH"
E_US0F3E_3902:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 REMOVE [TABLE|VIEW] object_name {, object_name}"
E_US0F40_3904:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
 REGISTER objname AS LINK WITH REFRESH"
E_US0F41_3905:E_GE7918_SYNTAX_ERROR
"Line %0d, syntax error on '%1c'.  The correct syntax is:\n
SET DDL_CONCURRENCY ON | OFF"
E_US0F54_3924:E_GE7918_SYNTAX_ERROR
"line %0d, Syntax error on '%1c'.  The correct syntax is:
\n SET [NO]MAXxxxx [nnnn]
\n  where xxxx is IO, QUERY, or ROW;
\n  nnnn is a positive integer less than or equal to
\n  the value of QUERY_xxxx_LIMIT for your session.
\n  A value must not be specified with [NO]MAXxxxx."
/*%
** Parameters:
**	(none)
**
** Explanation:
**	Syntax error while parsing the SET MAX... statement.
**	This is the QUEL version of E_US0FE0_3824.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Correct the syntax error.
**
*/
E_US100F_4111:E_GE7918_SYNTAX_ERROR
"String cannot be converted to numeric due to incorrect syntax."
E_US1036_4150:NO_GE
"Illegal filename for an INGRES table was specified: '%0c'"
E_US1068_4200:E_GE9D1C_DATAEX_FIXOVR
"Integer overflow detected in query."
E_US1069_4201:E_GE9D1E_DATAEX_FPDIV
"Integer divide by zero detected in query."
E_US106A_4202:E_GE9D1D_DATAEX_EXPOVR
"Floating overflow detected in query."
E_US106B_4203:E_GE9D1F_DATAEX_FLTDIV
"Floating divide by zero detected in query."
E_US106C_4204:E_GE9D22_DATAEX_EPUNF
"Floating underflow detected in query."
E_US1071_4209:NO_GE
"Invalid argument in call to math function."
E_US1077_4215:E_GE0032_WARNING
"Warning: %0d Integer overflows detected."
E_US1078_4216:E_GE0032_WARNING
"Warning: %0d Integer divide by zero operations detected."
E_US1079_4217:E_GE0032_WARNING
"Warning: %0d Floating overflows detected."
E_US107A_4218:E_GE0032_WARNING
"Warning: %0d Floating divide by zero operations detected."
E_US107B_4219:E_GE0032_WARNING
"Warning: %0d Floating underflows detected."
E_US1080_4224:E_GE0032_WARNING
"Warning: Non-printable character(s) converted to blank(s)."
E_US10CC_4300:E_GE80E8_LOGICAL_ERROR
"Addition of two absolute dates is undefined.  Addition on dates must
involve an absolute date and an interval."
E_US10CD_4301:E_GE80E8_LOGICAL_ERROR
"Subtracting an absolute date from an interval is not supported."
E_US10CE_4302:E_GE9D0E_DATAEX_DTINV
"'%0c' is not a valid date."
E_US10CF_4303:E_GE9D0E_DATAEX_DTINV
"%0d is not a valid year for a date column."
E_US10D0_4304:E_GE9D0E_DATAEX_DTINV
"%0d is not a valid month for a date column."
E_US10D1_4305:E_GE9D0E_DATAEX_DTINV
"%0d is not a valid day for a date column."
E_US10D2_4306:E_GE9D0E_DATAEX_DTINV
"%0d:%1d:%2d is not a valid time for a date column."
E_US10D3_4307:E_GE9D0E_DATAEX_DTINV
"%0c is not a valid time for use with the '%1c' qualifier in a date column."
E_US10D4_4308:E_GE9D0E_DATAEX_DTINV
"bad character found in date string beginning with '%0c'."
E_US10D5_4309:E_GE9D0E_DATAEX_DTINV
"cannot use am/pm specifications in a time interval."
E_US10D6_4310:E_GE9D0E_DATAEX_DTINV
"%0d years is out of valid range for a date interval."
E_US10D7_4311:E_GE9D0F_DATAEX_DATEOVR
"date is out of range of valid years."
E_US10D8_4312:NO_GE
"function dow (day-of-week) only valid on absolute dates."
E_US10D9_4313:NO_GE
"the date_trunc and date_part functions only work on absolute dates."
E_US10DA_4314:E_GE9D0E_DATAEX_DTINV
"weeks are not implemented in dates."
E_US10DB_4315:E_GE9D0E_DATAEX_DTINV
"the interval function recognizes no such unit as %0c."
E_US10DC_4316:NO_GE
"the interval function does not work on absolute dates."
E_US10DD_4317:E_GE0032_WARNING
"Warning: %0d absolute dates ignored in sum() or avg() aggregate."
E_US1130_4400:E_GE9D08_DATAEX_NOSUB
"illegal character to integer conversion or non-numeric character
in money field."
E_US1131_4401:E_GE9D08_DATAEX_NOSUB
"exceeded the maximum money value allowed."
E_US1132_4402:E_GE9D08_DATAEX_NOSUB
"value specified is less than the minimal legal value."
E_US1133_4403:E_GE9D08_DATAEX_NOSUB
"illegal blanks detected in money field."
E_US1134_4404:E_GE9D08_DATAEX_NOSUB
"multiple dollar signs detected in money field."
E_US1135_4405:E_GE9D08_DATAEX_NOSUB
"multiple positive and/or negative signs detected in money field."
E_US1136_4406:E_GE9D08_DATAEX_NOSUB
"multiple decimal points detected in money field."
E_US1137_4407:E_GE9D08_DATAEX_NOSUB
"comma detected in money field - enter money values without commas."
E_US1138_4408:E_GE9D08_DATAEX_NOSUB
"positive or negative sign detected in numeric part of money field."
E_US1139_4409:E_GE9D08_DATAEX_NOSUB
"dollar sign detected in numeric part of money field."
E_US113A_4410:E_GE9D08_DATAEX_NOSUB
"money divide by zero detected."
E_US1194_4500:E_GE7602_INS_DUP_ROW
"Duplicate key on INSERT detected."
E_US1195_4501:E_GE7602_INS_DUP_ROW
"Duplicate key on UPDATE detected."
E_US1196_4502:E_GE9CA4_CARDINALITY
"SELECT returned multiple values."
E_US1197_4503:E_GE7D00_QUERY_ERROR
"line %0d, %0c is a non-nullable datatype."
E_US1198_4504:E_GE7602_INS_DUP_ROW
"This update duplicates a row in the table."
E_US1199_4505:E_GE7D00_QUERY_ERROR
"Update failed because an integrity constraint was violated."
E_US11F7_4599:NO_GE
"Unexpected end of table during cursor update.  Transaction is aborted."
E_US11F8_4600:E_GE9DD0_CUR_STATE_INV
"Single table cursor is not open."
E_US11F9_4601:E_GE9DD0_CUR_STATE_INV
"The cursor specified has not been opened."
E_US11FA_4602:E_GE9DD0_CUR_STATE_INV
"The specified cursor does not support the requested action."
E_US11FB_4603:E_GE9DD0_CUR_STATE_INV
"The cursor was opened read-only."
E_US11FC_4604:E_GE9DD0_CUR_STATE_INV
"Either this cursor is already open or an attempt was\n
made to declare a savepoint when a cursor was open."
E_US11FD_4605:E_GE9DD0_CUR_STATE_INV
"The cursor is not currently positioned on a row."
E_US11FF_4607:E_GE9DD0_CUR_STATE_INV
"Only one deferred cursor can be opened at a time."
E_US1200_4608:NO_GE
"Table name is not valid."
E_US1201_4609:NO_GE
"Owner name is not valid."
E_US1202_4610:NO_GE
"Invalid attribute type specified."
E_US1203_4611:NO_GE
"A bad attribute name was specified."
E_US1204_4612:NO_GE
"Invalid size for specified attribute type."
E_US1205_4613:NO_GE
"Invalid precision for specified attribute type."
E_US1206_4614:NO_GE
"The specified location name is invalid."
E_US1207_4615:NO_GE
"An error occurred while deleting a record."
E_US1208_4616:NO_GE
"Duplicate records were found."
E_US1209_4617:NO_GE
"Database access conflict."
E_US120A_4618:NO_GE
"Database is already open."
E_US120B_4619:E_GE75BC_UNKNOWN_OBJECT
"Nonexistent database."
E_US125C_4700:E_GEC2EC_SERIALIZATION
"Deadlock detected, your single or multi-query transaction has been aborted."
E_US125D_4701:E_GE7D00_QUERY_ERROR
"Ambiguous replace: you have specified several replacements for the same row."
E_US125E_4702:NO_GE
"Timeout occurred on lock request (see SET LOCKMODE command)."
E_US125F_4703:E_GE8CA0_SYSTEM_LIMIT
"Out of locks in INGRES.  Reduce number of tables referenced inside of
multi-query transaction or number of page locks allowed on a table
(see SET LOCKMODE command)."
E_US1260_4704:E_GE8CA0_SYSTEM_LIMIT
"The IIDBDB is unavailable at this time because INGRES is out of locks.\n
	Please reduce the number of locks being held or enlarge the\n
	lock table."
E_US1261_4705:E_GE8D04_NO_RESOURCE
"Out of locks in INGRES.  Reduce number of tables referenced inside of
multi-query transaction or number of page locks allowed on a table
(see SET LOCKMODE command)."
E_US1262_4706:E_GEC2EC_SERIALIZATION
"Your transaction has been aborted due to the transaction log file\n
having reached one of the limits set by the system administrator.\n
These limits are log_full, force_abort, and 90 percent of force_abort\n
when using the fast_commit option to start DBMS servers."
E_US1263_4707:E_GE8CA0_SYSTEM_LIMIT
"Error allocating file system resource or bad file specification given.
Check disk space, disk quota, open file quota and the physical location."
E_US1264_4708:E_GEC2EC_SERIALIZATION
"The query has been aborted."
E_US1265_4709:E_GE80E8_LOGICAL_ERROR
"Database reorganization activity has rendered the
query plan invalid.  Try again after the db activity is complete.  In case
of dynamic SQL, the statement must be prepared again. "
E_US1266_4710_TRAN_ID_NOTUNIQUE:NO_GE
"The distributed transaction ID is not unique in the local system."
E_US1267_4711_DIS_TRAN_UNKNOWN:NO_GE
"The distributed transaction ID is unknown to the local system."
E_US1268_4712_DIS_TRAN_OWNER:NO_GE
"The distributed transaction is owned by another server in the local system.\n
Disconnect the previous association and try to re-connect later."
E_US1269_4713_NOSECUREINCLUSTER :NO_GE
"Can't support two-phase commit protocol in a cluster installation."
E_US126A_4714_ILLEGAL_STMT:E_GE9E34_TRAN_STATE_INV
"Illegal statement specified. Only ROLLBACK or COMMIT statement is allowed \n
immediately after the RE-CONNECT or PREPARE TO COMMIT statement."
E_US126B_4715_DIS_DB_UNKNOWN:NO_GE
"The database connected by the distributed transaction is unknown to the local system."
E_US1271_4721_IIDBDB_TRAN_LIMIT:E_GE8D04_NO_RESOURCE
"The IIDBDB is unavailable at this time because a Logging System quota
has been exceeded.  Either reduce the number of active sessions
or alter the logging system parameters."
/*
** Parameters:
**	None.
** Explanation:
**	The INGRES Logging System is configured with quotas on the number of
**	open databases in the installation and the number of active transactions
**	in the installation.  One of these limits has been reached.  The exact
**	quota which has been exceeded can be determined from the error log -
**	which will indicate either an error opening/adding a database (database
**	quota) or beginning a transaction (transaction quota).
** System Status:
**	Application or front end program will not start up.
** User Action:
**	User must either wait for active sessions to complete so that the
**	system falls below its defined quotas, or the logging system must
**	be reconfigured with greater limits.
*/
E_US1272_4722_DB_TRAN_LIMIT:E_GE8D04_NO_RESOURCE
"The database cannot be opened at this time because a Logging System quota
has been exceeded.  Either reduce the number of active sessions
or alter the logging system parameters."
/*
** Parameters:
**	None.
** Explanation:
**	The INGRES Logging System is configured with quotas on the number of
**	open databases in the installation and the number of active transactions
**	in the installation.  One of these limits has been reached.  The exact
**	quota which has been exceeded can be determined from the error log -
**	which will indicate either an error opening/adding a database (database
**	quota) or beginning a transaction (transaction quota).
** System Status:
**	Application or front end program will not start up.
** User Action:
**	User must either wait for active sessions to complete so that the
**	system falls below its defined quotas, or the logging system must
**	be reconfigured with greater limits.
*/
E_US1389_5001:E_GE797C_INVALID_IDENT
"PRINT: bad table name %0c."
E_US138A_5002:E_GE7D00_QUERY_ERROR
"PRINT: %0c is a view and can't be printed."
E_US138B_5003:NO_GE
"PRINT: Table %0c is protected."
E_US138C_5004:E_GEA0F0_SQL_STMT_INV
"The PRINT command is not allowed in distributed INGRES."
E_US138D_5005:NO_GE
"Table '%0c' is already in use. Close that table and try again."
E_US138E_5006:NO_GE
"Table is in use. Close that table and try again."
E_US13EE_5102:E_GE75F8_DEF_RESOURCE
"%0c: duplicate table name '%1c'."
E_US13EF_5103:E_GE84D0_NO_PRIVILEGE
"%0c: '%1c' is a system table."
E_US13F0_5104:E_GE797C_INVALID_IDENT
"CREATE %0c: invalid column name '%1c'."
E_US13F1_5105:E_GE75F8_DEF_RESOURCE
"CREATE %0c: duplicate column name '%1c'."
E_US13F2_5106:E_GE7918_SYNTAX_ERROR
"CREATE %0c: invalid column format '%1c' on column '%2c'."
E_US13F3_5107:E_GE8CA0_SYSTEM_LIMIT
"CREATE: It is not allowed to create a table with more than 300 columns."
E_US13F4_5108:E_GE8CA0_SYSTEM_LIMIT
"CREATE %0c: excessive table width on column '%1c'."
E_US13F5_5109:E_GE797C_INVALID_IDENT
"CREATE %0c: underscore not allowed in table names."
E_US13F6_5110:NO_GE
"CREATE %0c: journaling not allowed on this database."
E_US13F7_5111:E_GE75F8_DEF_RESOURCE
"CREATE %0c: You cannot create this table because the dba's table of
the same name has already been referenced by this query or
multi-query transaction."
E_US13F8_5112:E_GE8D04_NO_RESOURCE
"CREATE %0c: Disk error trying to create table.  You are probably out
of disk space or disk quota."
E_US13F9_5113:NO_GE
"CREATE: '%0c' is an invalid locationname for your database.  It is
either not defined or your database can't use it.  Define it through ACCESSDB."
E_US13FA_5114:E_GEA0F0_SQL_STMT_INV
"The CREATE command is not allowed in distributed INGRES."
E_US13FB_5115:E_GE7918_SYNTAX_ERROR
"The CREATE command does not recognize the parameter '%1c'."
E_US13FC_5116:E_GE797C_INVALID_IDENT
"%0c: Illegal tablename '%1c'. 'ii' is reserved for system catalogs."
E_US13FD_5117:E_GE0000_OK
"CREATE: %0c: Journaling will be enabled at the next checkpoint."
E_US13FE_5118:E_GEA0F0_SQL_STMT_INV
"%0c is not allowed in distributed INGRES."
E_US141E_5150:E_GEA0F0_SQL_STMT_INV
"The EXCLUDE NODE command is not allowed in non-distributed INGRES."
E_US141F_5151:E_GEA0F0_SQL_STMT_INV
"The INCLUDE NODE command is not allowed in non-distributed INGRES."
E_US1420_5152:E_GE7918_SYNTAX_ERROR
"The DEFINE REFERENCE command doesn't recognize the parameter '%1c'."
E_US1451_5201:NO_GE
"DESTROY: %0c is a system table."
E_US1452_5202:E_GE7594_TABLE_NOT_FOUND
"DESTROY: %0c does not exist or is not owned by you."
E_US1453_5203:E_GE75B2_NOT_FOUND
"DROP/DESTROY: %0d does not identify an existing integrity constraint."
E_US1454_5204:E_GE75B2_NOT_FOUND
"DROP/DESTROY: %0d does not identify an existing protection constraint."
E_US1455_5205:E_GEA0F0_SQL_STMT_INV
"The DEFINE REFERENCE command is not allowed in non-distributed INGRES."
E_US1456_5206:E_GEA0F0_SQL_STMT_INV
"The DROP/DESTROY INTEGRITY command is not allowed in distributed INGRES."
E_US1457_5207:E_GEA0F0_SQL_STMT_INV
"The DROP/DESTROY PERMIT command is not allowed in distributed INGRES."
E_US1458_5208:E_GEA0F0_SQL_STMT_INV
"DROP/REMOVE INDEX is not allowed in distributed INGRES."
E_US1459_5209:E_GE8CA0_SYSTEM_LIMIT
"The maximum number of views in a HELP VIEW command is %0d."
E_US145A_5210:E_GE84D0_NO_PRIVILEGE
"DROP/DESTROY PERMIT: only the DBA can use the DROP/DESTROY PERMIT statement."
E_US145B_5211:NO_GE
"Can not %1c '%0c'.
\n One may not %1c a DDB object whose underlying LDB object is an index."
E_US145C_5212:E_GEA0F0_SQL_STMT_INV
"QUEL queries are not supported in this release of distributed INGRES."
E_US145D_5213:E_GE75B2_NOT_FOUND
"DROP SECURITY_ALARM: %0d does not identify an existing security alarm."
/*%
** Parameters:
**	%0 - Security alarm number specified on the DROP SECURITY_ALARM
**	     statement.
**
** Explanation:
**	The specified security alarm number does not exist on the 
**	specified table.
**
** System Status:
**	This error message is issued, and the remaining security alarms
**	(if any) are processed.
**
** User Action:
**	Verify that the specified number identifies a valid security alarm.
**	If not, respecify the command with a valid security alarm number.
**
*/
E_US14B4_5300:E_GE7594_TABLE_NOT_FOUND
"CREATE INDEX: cannot find primary table '%0c'."
E_US14B5_5301:E_GE8CA0_SYSTEM_LIMIT
"CREATE INDEX: A secondary index may be created on at most 6 columns."
E_US14B6_5302:E_GE759E_COLUMN_UNKNOWN
"CREATE INDEX: Invalid column '%0c'."
E_US14B7_5303:E_GE7594_TABLE_NOT_FOUND
"CREATE INDEX: Table '%0c' not owned by you."
E_US14B8_5304:E_GE7D00_QUERY_ERROR
"CREATE INDEX: Table '%0c' is already an index."
E_US14B9_5305:E_GE7D00_QUERY_ERROR
"CREATE INDEX: Table '%0c' is a system table."
E_US14BA_5306:E_GE7D00_QUERY_ERROR
"CREATE INDEX: '%0c' is a view and an index can't be built on it."
E_US14BB_5307:E_GE75F8_DEF_RESOURCE
"CREATE INDEX: duplicate column '%0c'."
E_US14BE_5310:NO_GE
"CREATE INDEX: index %0c has been corrupted.  It contains a pointer (tid:%1d),
which points to a non-existent row in base table %2c. The error status was %3d."
E_US14BF_5311:E_GEA0F0_SQL_STMT_INV
"The INDEX statement is not allowed in distributed INGRES."
E_US14C0_5312:E_GE75F8_DEF_RESOURCE
"CREATE INDEX: Location specified twice: '%0c' conflicts with '%1c'."
E_US14C1_5313:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Bad storage structure '%0c'.
The valid storage structure names are [c]btree, [c]isam, [c]hash."
E_US14C2_5314:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: '%0c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nSTRUCTURE is the only valid keyword for this type of subclause."
E_US14C3_5315:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Structure must be specified before '%0c'."
E_US14C4_5316:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Invalid FILLFACTOR of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US14C5_5317:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: FILLFACTOR can be specified only for [c]btree, [c]isam, [c]hash
storage structure."
E_US14C6_5318:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Invalid MINPAGES of '%0d' specified. Must be greater than 0."
E_US14C7_5319:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: MINPAGES can be specified only for [c]hash storage structure."
E_US14C8_5320:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Invalid MAXPAGES of '%0d' specified. Must be greater than 0."
E_US14C9_5321:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: MAXPAGES can be specified only for [c]hash storage structure."
E_US14CA_5322:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Invalid NONLEAFFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US14CB_5323:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: NONLEAFFILL can be specified only for [c]btree
storage structure."
E_US14CC_5324:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Invalid LEAFFILL/INDEXFILL of '%0d' specified.
Must be in 1 - 100 range (inclusive)."
E_US14CD_5325:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: LEAFFILL/INDEXFILL can be specified only for [c]btree
storage structure."
E_US14CE_5326:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: '%0c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nValid options for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL."
E_US14CF_5327:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Too many options in the WITH clause."
E_US14D0_5328:E_GE8CA0_SYSTEM_LIMIT
"CREATE INDEX: Too many columns in key of index."
E_US14D1_5329:E_GE759E_COLUMN_UNKNOWN
"CREATE INDEX: %0c: Key '%1c' is out of sequence or is not in the column
list.  The key list must be a prefix of the column list."
E_US14D3_5331:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: The number of elements in the KEY clause must not be greater
than the number of indexed columns."
E_US14D4_5332:E_GE7918_SYNTAX_ERROR
"CREATE INDEX: Illegal MINPAGES value '%0d' in a CREATE INDEX command.
The value must be less or equal to the MAXPAGES value."
E_US1518_5400:NO_GE
"HELP: Internal errors preclude further operation.  Please report
problem to product vendor."
E_US1519_5401:E_GE7594_TABLE_NOT_FOUND
"HELP: table %0c does not exist."
E_US151A_5402:E_GE75BC_UNKNOWN_OBJECT
"HELP: cannot find manual section '%0c'."
E_US151B_5403:E_GE7D00_QUERY_ERROR
"HELP: table %0c is not a view."
E_US151C_5404:NO_GE
"HELP: table %0c has no permissions granted on it."
E_US151D_5405:NO_GE
"HELP: table %0c has no integrity constraints on it."
E_US1522_5410:NO_GE
"HELP: tree buffer overflowed."
E_US1523_5411:NO_GE
"HELP: tree stack overflowed."
E_US1524_5412:E_GEA0F0_SQL_STMT_INV
"The HELP INTEGRITY command is not allowed in distributed INGRES."
E_US1525_5413:E_GEA0F0_SQL_STMT_INV
"The HELP PERMIT command is not allowed in distributed INGRES."
E_US1526_5414:E_GEA0F0_SQL_STMT_INV
"The HELP VIEW command is not allowed in distributed INGRES."
E_US157C_5500:E_GE7594_TABLE_NOT_FOUND
"MODIFY: table %0c does not exist."
E_US157D_5501:E_GE7594_TABLE_NOT_FOUND
"MODIFY: you do not own table %0c."
E_US157E_5502:E_GE7918_SYNTAX_ERROR
"MODIFY %0c: you may not provide keys on a heap."
E_US157F_5503:E_GE8CA0_SYSTEM_LIMIT
"MODIFY: It is not allowed to list more than 300 columns in a MODIFY command."
E_US1580_5504:E_GE84D0_NO_PRIVILEGE
"MODIFY %0c: cannot modify system table."
E_US1581_5505:E_GE7918_SYNTAX_ERROR
"MODIFY %0c: CHECKLINK and PATCHLINK are no longer part of the MODIFY
command. Check the reference manual for details."
E_US1583_5507:E_GE75F8_DEF_RESOURCE
"MODIFY %0c: duplicate key '%1c'."
E_US1584_5508:E_GE8CA0_SYSTEM_LIMIT
"MODIFY %0c: key width (%1d) too large for ISAM; maximum size is %2d."
E_US1586_5510:E_GE7918_SYNTAX_ERROR
"MODIFY: Invalid option '%0c'.
\n Try one of the following:
\n   [C]ISAM, [C]HASH, [C]HEAP, [C]HEAPSORT, [C]BTREE,
\n   MERGE, TRUNCATED, REORGANIZE, REORGANISE, RELOCATE"
E_US1587_5511:E_GE797C_INVALID_IDENT
"MODIFY %0c: bad column name '%1c'."
E_US1588_5512:E_GE7918_SYNTAX_ERROR
"MODIFY %0c: '%1c' not allowed or specified more than once."
E_US1589_5513:E_GE7918_SYNTAX_ERROR
"MODIFY: Illegal FILLFACTOR '%0d' in a MODIFY command.  The value
must be between 1 and 100, inclusive."
E_US158A_5514:E_GE7918_SYNTAX_ERROR
"MODIFY: Illegal MINPAGES value '%0d' in a MODIFY command.
The value must be greater than zero."
E_US158B_5515:E_GE7918_SYNTAX_ERROR
"MODIFY: column keys must not be specified unless storage
structure is present."
E_US158C_5516:E_GE7918_SYNTAX_ERROR
"MODIFY: Illegal MAXPAGES value '%0d' in a MODIFY command.
The value must be greater than zero."
E_US158D_5517:E_GE7918_SYNTAX_ERROR
"MODIFY: Illegal MINPAGES value '%0d' in a MODIFY command.
The value must be less or equal to the MAXPAGES value."
E_US158E_5518:E_GE7918_SYNTAX_ERROR
"MODIFY: invalid sequence specifier '%0c' for column '%1c'."
E_US158F_5519:E_GE7918_SYNTAX_ERROR
"MODIFY: %0c is a view and can't be modified."
E_US1590_5520:E_GE7918_SYNTAX_ERROR
"MODIFY: %0c: sequence specifier '%1c' on column %2c is not allowed
with the specified storage structure."
E_US1591_5521:E_GE80E8_LOGICAL_ERROR
"MODIFY: table could not be modified because rows contain duplicate keys."
E_US1592_5522:E_GE80E8_LOGICAL_ERROR
"INDEX: table could not be indexed because rows contain duplicate keys."
E_US1593_5523:E_GE8D04_NO_RESOURCE
"MODIFY: %0c: ran out of disk space while performing modify."
E_US1594_5524:E_GE8CA0_SYSTEM_LIMIT
"MODIFY: %0c: key is %1d bytes; maximum size key for btrees is %2d bytes."
E_US1595_5525:E_GE7D00_QUERY_ERROR
"MODIFY: %0c: table is not a btree; only a btree table can be modified to
merge."
E_US1596_5526:E_GE7D00_QUERY_ERROR
"MODIFY: %0c: all columns except the 'tidp' column should be part of the
key in a btree secondary index."
E_US1597_5527:E_GE7D00_QUERY_ERROR
"MODIFY: %0c: compressed row size can exceed maximum row width."
E_US1598_5528:E_GE7D00_QUERY_ERROR
"MODIFY: May not alter key sequence of secondary index table."
E_US1599_5529:E_GE7918_SYNTAX_ERROR
"MODIFY: '%0c' is either an invalid option or the syntax of
the WITH subclause is invalid.
\nThe valid keywords for this type of subclause are:
\n   FILLFACTOR, MINPAGES, MAXPAGES, NONLEAFFILL, LEAFFILL, MAXINDEXFILL."
E_US159A_5530:E_GEA0F0_SQL_STMT_INV
"The MODIFY command is not allowed in distributed INGRES."
E_US159B_5531:E_GE7D00_QUERY_ERROR
"MODIFY: %0c: The '%1c' entry in the WHERE or WITH clause
is incompatible with the storage structure."
E_US159C_5532:E_GE7D00_QUERY_ERROR
"INDEX: %0c: The '%1c' entry in the WITH clause
is incompatible with the storage structure."
E_US159D_5533:NO_GE
"MODIFY: %0c: Can't change the storage structure of this catalog."
E_US159E_5534:E_GE7918_SYNTAX_ERROR
"INDEX: Unrecognized parameter '%0c' in the WITH clause.
 Use one of the following.
\n              STRUCTURE, KEY, FILLFACTOR, MINPAGES, MAXPAGES, 
\n              LEAFFILL, INDEXFILL, NONLEAFFILL, MAXINDEXFILL"
E_US159F_5535:E_GE7918_SYNTAX_ERROR
"INDEX: Unrecognized storage structure '%0c' in an INDEX command.
 Please use one of the following:
\n          [C]ISAM, [C]HASH, [C]BTREE"
E_US15A0_5536:E_GE7D00_QUERY_ERROR
"MODIFY: %0c: maximum size of a compressed row could exceed maximum
row width. Remember that the compressed C type uses an extra byte
and the compressed CHAR type uses two extra."
E_US15A1_5537:E_GE8CA0_SYSTEM_LIMIT
"INDEX: %0c: key width (%1d) too large for ISAM; maximum size is %2d."
E_US15A2_5538:E_GE8CA0_SYSTEM_LIMIT
"INDEX: %0c: key is %1d bytes; maximum size key for btrees is %2d bytes."
E_US15A3_5539:E_GE8CA0_SYSTEM_LIMIT
"INDEX: %0c: maximum size of a compressed row could exceed maximum
row width. Remember that the compressed C type uses an extra byte."
E_US15A4_5540:E_GE7D00_QUERY_ERROR
"MODIFY: %0c: HEAP, CHEAP, and HEAPSORT are not valid storage structures
for a secondary index."
E_US15A5_5541:E_GE7D00_QUERY_ERROR
"MODIFY: %0c: It is not allowed to modify a secondary index with
\nnon-unique keys to one with unique keys.  You can destroy and
\nre-create the index with unique keys."
E_US15A6_5542:E_GE7918_SYNTAX_ERROR
"MODIFY: The RELOCATE variant requires new and old location lists,
they must be of equal length."
E_US15A7_5543:E_GE7918_SYNTAX_ERROR
"MODIFY: Location clause must not be specified with MERGE or TRUNCATED."
E_US15A8_5544:E_GE7918_SYNTAX_ERROR
"MODIFY: line %0d, The WITH clause of the RELOCATE variant must have
only NEWLOCATION and OLDLOCATION subclauses."
E_US15A9_5545:E_GE7918_SYNTAX_ERROR
"MODIFY: line %0d, LOCATION keyword not allowed in the RELOCATE variant.
Use NEWLOCATION or OLDLOCATION instead."
E_US15AA_5546:E_GE7918_SYNTAX_ERROR
"MODIFY: line %0d, '%1c' can be only used with RELOCATE variant."
E_US15AB_5547:E_GE7918_SYNTAX_ERROR
"MODIFY: '%0c' option has already been specified."
E_US15AC_5548:E_GE7918_SYNTAX_ERROR
"MODIFY: REORGANIZE variant requires location(s) to be specified."
E_US15AD_5549:E_GE7918_SYNTAX_ERROR
"MODIFY: line %0d, '%1c' option cannot be used with REORGANIZE variant."
E_US15E0_5600:E_GE7D00_QUERY_ERROR
"Save: Can't save system table '%0c'."
E_US15E1_5601:E_GE797C_INVALID_IDENT
"Save: bad month '%0c'."
E_US15E2_5602:E_GE797C_INVALID_IDENT
"Save: bad day '%0d'."
E_US15E3_5603:E_GE797C_INVALID_IDENT
"Save: bad year '%0d'."
E_US15E4_5604:E_GE7594_TABLE_NOT_FOUND
"Save: table '%0c' does not exist or is not owned by you."
E_US15E5_5605:E_GEA0F0_SQL_STMT_INV
"The SAVE command is not allowed in distributed INGRES."
E_US16A8_5800:E_GE7594_TABLE_NOT_FOUND
"COPY: table '%0c' doesn't exist or is not owned by you."
E_US16A9_5801:E_GE759E_COLUMN_UNKNOWN
"COPY: column '%0c' in table '%1c' doesn't exist or it has been listed twice."
E_US16AB_5803:E_GE8CA0_SYSTEM_LIMIT
"COPY: too many columns."
E_US16AC_5804:E_GE7918_SYNTAX_ERROR
"COPY: bad length for column '%0c'.  Length = '%1c'."
E_US16AD_5805:NO_GE
"COPY: can't open file '%0c'."
E_US16AE_5806:NO_GE
"COPY: can't create file '%0c'."
E_US16AF_5807:E_GE7918_SYNTAX_ERROR
"COPY: unrecognizable dummy domain '%0c'."
E_US16B0_5808:E_GE7D00_QUERY_ERROR
"COPY: domain '%0c', size too small for conversion.
There were %2c rows successfully copied from '%3c' into '%4c'."
E_US16B1_5809:E_GE9D08_DATAEX_NOSUB
"COPY: bad input string for domain '%0c'. Input was '%1c'.
There were %2c rows successfully copied from '%3c' into '%4c'."
E_US16B2_5810:E_GE9D08_DATAEX_NOSUB
"COPY: unexpected end of file while filling domain '%0c'."
E_US16B3_5811:E_GE7918_SYNTAX_ERROR
"COPY: bad type for column '%0c'. Type = '%1c'."
E_US16B4_5812:E_GE7D00_QUERY_ERROR
"COPY: The table '%0c' has a secondary index.
The index(es) must be destroyed before doing a 'copy from'."
E_US16B5_5813:E_GE84D0_NO_PRIVILEGE
"COPY: You are not allowed to update the table '%0c'."
E_US16B6_5814:E_GE7594_TABLE_NOT_FOUND
"COPY: You do not own the table '%0c'."
E_US16B7_5815:E_GE9D08_DATAEX_NOSUB
"COPY: An unterminated 'c0' or 'text(0)' field occurred while filling domain
'%0c'."
E_US16B8_5816:E_GE7918_SYNTAX_ERROR
"COPY: The full pathname must be specified for the file '%0c'."
E_US16B9_5817:E_GE8CA0_SYSTEM_LIMIT
"COPY: The maximum width of the output file cannot exceed 2008 bytes per row."
E_US16BA_5818:E_GE7D00_QUERY_ERROR
"COPY: '%0c' is a view and can't be copied."
E_US16BB_5819:NO_GE
"COPY: Warning: %0c duplicate rows were ignored."
E_US16BC_5820:NO_GE
"COPY: Warning: %0c domains had control characters which were converted
to blanks."
E_US16BD_5821:NO_GE
"COPY: Warning: %0c c0 or text(0) character domains were truncated."
E_US16BE_5822:E_GE9D6C_CONSTR_VIO
"COPY: Table '%0c' is protected."
E_US16BF_5823:E_GE7918_SYNTAX_ERROR
"COPY: Copy Into a text file must have 'd0nl' delimiter as the last domain."
E_US16C0_5824:E_GE7918_SYNTAX_ERROR
"COPY: '%0c' is an invalid filetype.
\nFiletype must be 'text', 'binary', or 'variable'."
E_US16C1_5825:E_GE7D00_QUERY_ERROR
"COPY: The filetype must not be 'binary' when any of the columns are 'text(0)'."
E_US16C2_5826:E_GE9D08_DATAEX_NOSUB
"COPY: Error encountered while processing row %0c."
E_US16C3_5827:E_GE9D08_DATAEX_NOSUB
"COPY: Invalid character value (numeric value %0d) found while
processing row %1c."
E_US16C4_5828:E_GEA0F0_SQL_STMT_INV
"The COPY command is not allowed in distributed INGRES."
E_US16C5_5829:E_GE8CA0_SYSTEM_LIMIT
"COPY: Computed row length exceeds allowable row length."
E_US16C6_5830:E_GE7D00_QUERY_ERROR
"COPY: Column '%0c', cannot convert type '%1c' to type '%2c'."
E_US16C7_5831:E_GE7918_SYNTAX_ERROR
"COPY: Column '%0c', bad delimiter '%1c'."
E_US16C8_5832:E_GE7918_SYNTAX_ERROR
"COPY: Column '%0c', delimiter not allowed.  Use type 'd0' to specify
delimiter."
E_US16C9_5833:E_GE7918_SYNTAX_ERROR
"COPY: Column '%0c', illegal column type"
E_US16CA_5834:NO_GE
"COPY: Internal error initializing COPY."
E_US16CB_5835:NO_GE
"COPY: INGRES internal error processing row %0c."
E_US16CC_5836:NO_GE
"COPY: Unexpected END OF FILE while processing row %0c."
E_US16CD_5837:E_GE8D04_NO_RESOURCE
"COPY: Error allocating memory while processing row %0c."
E_US16CE_5838:E_GE7D00_QUERY_ERROR
"COPY: underlying LDB object for '%0c' is a view.\n
You may not use COPY on views."
E_US16D0_5840:NO_GE
"COPY: Warning: Copy completed with %0c warnings.  %1c rows successfully
copied."
E_US16D1_5841:NO_GE
"COPY: Copy terminated abnormally.  %0c rows successfully copied."
E_US16D2_5842:NO_GE
"COPY: Copy has been aborted."
E_US16D3_5843:NO_GE
"COPY: %0c rows could not be processed."
E_US16D4_5844:NO_GE
"COPY: %0c rows were written to the Copy Log File '%1c'."
E_US16D5_5845:E_GE7594_TABLE_NOT_FOUND
"Table specified no longer exists."
E_US16D6_5846:E_GE7594_TABLE_NOT_FOUND
"Table specified already exists."
E_US16D7_5847:NO_GE
"COPY: Error writing to Copy Log File '%0c' while processing row %1c."
E_US16D8_5848:NO_GE
"COPY: Error closing Copy Log File '%0c'."
E_US16D9_5849:NO_GE
"COPY: Error closing Copy File '%0c'."
E_US16DA_5850:E_GE9D08_DATAEX_NOSUB
"COPY: Bad data value for column '%0c' encountered while processing row %1c."
E_US16DB_5851:E_GE7918_SYNTAX_ERROR
"COPY: Invalid Copy option '%0c'."
E_US16DC_5852:E_GE7918_SYNTAX_ERROR
"COPY: Invalid Copy option value '%0c'."
E_US16DD_5853:E_GE7918_SYNTAX_ERROR
"COPY: Ambiguous Copy options."
E_US16DE_5854:NO_GE
"COPY: Error opening Copy Log File '%0c'."
E_US16DF_5855:E_GE8D04_NO_RESOURCE
"COPY: Error allocating memory while initializing Copy."
E_US16E0_5856:E_GE8D04_NO_RESOURCE
"COPY: Error allocating memory while processing row %0c."
E_US16E1_5857:E_GE9D08_DATAEX_NOSUB
"COPY: Error processing row %0c.  Cannot convert column '%1c' to tuple format."
E_US16E2_5858:E_GE9D08_DATAEX_NOSUB
"COPY: Error processing row %0c.  Cannot convert column '%1c' to row format."
E_US16E3_5859:E_GE9D08_DATAEX_NOSUB
"COPY: Error writing to Copy File while processing row %0c."
E_US16E4_5860:E_GE9D08_DATAEX_NOSUB
"COPY: Error reading row from INGRES Application while processing row %0c."
E_US16E5_5861:E_GE9D08_DATAEX_NOSUB
"COPY: Error writing row to INGRES Application while processing row %0c."
E_US16E6_5862:E_GE7D00_QUERY_ERROR
"COPY: NULL specification not allowed for the dummy column '%0c'."
E_US16E7_5863:E_GE7D00_QUERY_ERROR
"COPY: Column '%0c', NULL without null value may not be used with variable
length format."
E_US16E8_5864:E_GE7D00_QUERY_ERROR
"COPY: Column '%0c', NULL specification missing.  Cannot copy rows
containing nulls."
E_US16E9_5865:E_GE7D00_QUERY_ERROR
"COPY: Column '%0c', NULL value not compatible with format specification."
E_US16EA_5866:NO_GE
"A copy command is currently in progress."
E_US16EB_5867:NO_GE
"An error in the copy command occurred."
E_US170C_5900:E_GE7918_SYNTAX_ERROR
"SET: %0c is not a valid set function."
E_US170D_5901:E_GE7918_SYNTAX_ERROR
"SET JOURNALING: Set journaling does not have any parameters."
E_US170E_5902:NO_GE
"SET JOURNALING: Tables created during session will be logged."
E_US170F_5903:NO_GE
"SET JOURNALING: Tables create during session will NOT be logged."
E_US1710_5904:E_GE7918_SYNTAX_ERROR
"SET: Parameter must be constant or string name."
E_US1712_5906:E_GE84D0_NO_PRIVILEGE
"SET TRACE: You do not have permission to set trace flags."
E_US1713_5907:E_GE7918_SYNTAX_ERROR
"SET TRACE: Trace flag specification too long."
E_US1714_5908:E_GE7918_SYNTAX_ERROR
"SET JOURNAL_SIZE: This function no longer available."
E_US1715_5909:NO_GE
"SET JOURNALING: Journaling not enabled on this database."
E_US1716_5910:E_GE7918_SYNTAX_ERROR
"SET: This set command takes only a single argument (in quotes)."
E_US1717_5911:E_GE7918_SYNTAX_ERROR
"SET: An integer argument is required (in quotes)."
E_US1718_5912:E_GE7918_SYNTAX_ERROR
"SET: The merge factor must be between 3 and 15 inclusive."
E_US1719_5913:E_GE7918_SYNTAX_ERROR
"SET: The sort buffer size must be between 1000 and 1000000 inclusive."
E_US171A_5914:E_GE7918_SYNTAX_ERROR
"SET: The freespace buffer size must be between 1000 and 1000000 inclusive."
E_US171B_5915:E_GE7918_SYNTAX_ERROR
"Unrecognized storage structure in a SET RET_INTO command.
Please use one of the following:
\n
\n          HEAP, CHEAP, HEAPSORT, CHEAPSORT, HASH, CHASH, ISAM, CISAM,
\n          BTREE, CBTREE"
E_US171C_5916:E_GE7918_SYNTAX_ERROR
"Unrecognized storage structure in a SET RESULT_STRUCTURE command.
Please use one of the following:
\n
\n          HEAP, CHEAP, HEAPSORT, CHEAPSORT, HASH, CHASH, ISAM, CISAM,
\n          BTREE, CBTREE"
E_US1724_5924:E_GE7918_SYNTAX_ERROR
"Unrecognized value for the LEVEL parameter in a SET LOCKMODE
command.  Please use one of the following:
\n
\n          PAGE, TABLE, SESSION, SYSTEM"
E_US1725_5925:E_GE7918_SYNTAX_ERROR
"Unrecognized value for the READLOCK parameter of a
SET LOCKMODE command.  Please use one of the following:
\n
\n          NOLOCK, SHARED, EXCLUSIVE, SESSION, SYSTEM"
E_US1726_5926:E_GE7918_SYNTAX_ERROR
"Unrecognized value for the MAXLOCKS parameter of a
SET LOCKMODE command.  Please use one of the following:
\n
\n          a number, SESSION, SYSTEM"
E_US1727_5927:E_GE7918_SYNTAX_ERROR
"Unrecognized value for the TIMEOUT parameter of a SET LOCKMODE
command.  Please use one of the following:
\n
\n          a number, SESSION, SYSTEM"
E_US1728_5928:E_GE7918_SYNTAX_ERROR
"Unrecognized parameter '%0c' in a SET LOCKMODE command.
Please use one of the following:
\n
\n          LEVEL, READLOCK, MAXLOCKS, TIMEOUT"
E_US1729_5929:E_GE797C_INVALID_IDENT
"SET LOCKMODE: Invalid table name in a set command."
E_US172A_5930:E_GE7918_SYNTAX_ERROR
"SET LOCKMODE: Cannot set session lock default to 'session'."
E_US172B_5931:E_GE7918_SYNTAX_ERROR
"SET LOCKMODE: Too many lockmode parameters."
E_US172C_5932:E_GE9E34_TRAN_STATE_INV
"SET LOCKMODE: This statement is not allowed in a multi-query transaction."
E_US172D_5933:E_GEA0F0_SQL_STMT_INV
"The SET [NO]JOINOP command is no longer supported.
All queries now use JOINOP."
E_US172E_5934:E_GE7918_SYNTAX_ERROR
"Bad date format '%0c' in a SET DATE_FORMAT command.
Please choose from one of the following:
\n
\n  \"US\", \"MULTINATIONAL\", \"ISO\", \"FINLAND\", \"SWEDEN\" | \"GERMAN\""
E_US172F_5935:E_GE7918_SYNTAX_ERROR
"Bad money format '%0c' in a SET MONEY_FORMAT command.
Please use the following form:
\n
\n          \"L:xxxx\" or \"T:xxxx\"
\n
where xxxx is a zero to four character currency symbol.  
L indicates a leading currency symbol, and T a trailing one.
\n
Example: \"T:DM\" would result in the value 10.85 displayed as '10.85 DM'"
E_US1730_5936:E_GE7918_SYNTAX_ERROR
"Bad decimal indicator '%0c' in a SET DECIMAL command.
Please use either a comma (\",\") or a period (\".\")."
E_US1731_5937:E_GE7918_SYNTAX_ERROR
"Bad precision '%0d' in a SET MONEY_PREC command.  Money precision
determines the number of digits displayed after the decimal indicator
in a money value.  Allowed precision values are '0', '1', or '2'
\n
\n  '0' indicates no digits after the decimal indicator (e.g., 10)
\n  '1' indicates one digit (e.g. 10.8)
\n  '2' indicates two digits (e.g. 10.85)"
E_US1732_5938:E_GE7918_SYNTAX_ERROR
"SET CACHE: Invalid cache size.  Legal range is 0 - 500.  Setting the
size of the cache to a larger size than 500 will result in a cache size of 500."
E_US1733_5939:E_GE9E34_TRAN_STATE_INV
"SET CACHE: You may not issue a SET CACHE command in a multi-query transaction."
E_US1734_5940:E_GEA0F0_SQL_STMT_INV
"The SET J_FREESZ1 command is no longer supported.
This function is now done automatically by the system."
E_US1735_5941:E_GEA0F0_SQL_STMT_INV
"The SET J_FREESZ2 command is no longer supported.
This function is now down automatically by the system."
E_US1736_5942:E_GEA0F0_SQL_STMT_INV
"The SET J_FREESZ3 command is no longer supported.
This function is now done automatically by the system."
E_US1737_5943:E_GEA0F0_SQL_STMT_INV
"The SET J_FREESZ4 command is no longer supported.
This function is now done automatically by the system."
E_US1738_5944:E_GEA0F0_SQL_STMT_INV
"The SET AGGREGATE command is not allowed in distributed INGRES."
E_US1739_5945:E_GEA0F0_SQL_STMT_INV
"The SET CPUFACTOR command is not allowed in distributed INGRES."
E_US173A_5946:E_GEA0F0_SQL_STMT_INV
"The SET DATE_FORMAT command is not allowed in distributed INGRES."
E_US173B_5947:E_GEA0F0_SQL_STMT_INV
"The SET [NO]DEADLOCK command is no longer supported."
E_US173C_5948:E_GEA0F0_SQL_STMT_INV
"The SET DECIMAL command is not allowed in distributed INGRES."
E_US173D_5949:E_GEA0F0_SQL_STMT_INV
"The SET EMBEDDED and SET EQUEL commands are not allowed in distributed INGRES."
E_US173E_5950:E_GEA0F0_SQL_STMT_INV
"The SET [NO]JOINOP command is not allowed in distributed INGRES."
E_US173F_5951:E_GEA0F0_SQL_STMT_INV
"The SET [NO]JOURNALING command is not allowed in distributed INGRES."
E_US1740_5952:E_GEA0F0_SQL_STMT_INV
"The SET LOCKMODE command is not allowed in distributed INGRES."
E_US1741_5953:E_GEA0F0_SQL_STMT_INV
"The SET MONEY_FORMAT command is not allowed in distributed INGRES."
E_US1742_5954:E_GEA0F0_SQL_STMT_INV
"The SET MONEY_PREC command is not allowed in distributed INGRES."
E_US1743_5955:E_GEA0F0_SQL_STMT_INV
"The SET QBUFSIZE command is not longer supported.
This function is now done automatically by the system."
E_US1744_5956:E_GEA0F0_SQL_STMT_INV
"The SET RET_INTO command in not allowed in distributed INGRES."
E_US1745_5957:E_GEA0F0_SQL_STMT_INV
"The SET SORTBUFSIZE command is not allowed in distributed INGRES."
E_US1746_5958:E_GEA0F0_SQL_STMT_INV
"The SET SQL command is not allowed in distributed INGRES."
E_US1747_5959:E_GEA0F0_SQL_STMT_INV
"The SET [NO]STATISTICS command is not allowed in distributed INGRES."
E_US1748_5960:E_GEA0F0_SQL_STMT_INV
"The SET [NO]TRACE command is not allowed in distributed INGRES."
E_US1749_5961:E_GEA0F0_SQL_STMT_INV
"The CREATE/DEFINE VIEW command is not allowed in distributed INGRES."
E_US174A_5962:E_GE7918_SYNTAX_ERROR
"Unrecognized parameter '%0c' given in a SET [NO]TRACE command.
Please use one of the following:
\n
\n          TERMINAL, INPUT, OUTPUT, POINT"
E_US174B_5963:E_GE7918_SYNTAX_ERROR
"It is not allowed to give a file name with the SET NOTRACE TERMINAL command."
E_US174C_5964:E_GE7918_SYNTAX_ERROR
"It is not allowed to give a file name with the SET NOTRACE INPUT
 and SET NOTRACE OUTPUT commands."
E_US174D_5965:E_GE7918_SYNTAX_ERROR
"The SET [NO]TRACE command must have a facility code and flag number."
E_US174E_5966:E_GE7918_SYNTAX_ERROR
"Unrecognized facility code in SET [NO]TRACE %0c.
Please use one of the following:
\n
\n          DM - Data Manipulation Facility
\n          PS - Parser Facility
\n          SC - System Control Facility
\n          RD - Relation Description Facility
\n          QE - Query Execution Facility
\n          OP - Optimizer Facility
\n          QS - Query Storage Facility
\n          AD - Abstract Datatype Facility"
E_US174F_5967:E_GE7918_SYNTAX_ERROR
"Bad flag number in SET [NO]TRACE %0c.  It must be
between 0 and 9999, inclusive."
E_US1750_5968:E_GE7918_SYNTAX_ERROR
"Integer parameters are not allowed in the
SET [NO]TRACE TERMINAL, SET [NO]TRACE OUTPUT,
and SET [NO]TRACE INPUT commands."
E_US1751_5969:E_GE7918_SYNTAX_ERROR
"In a SET command, the non-integer '%0c' was found
where an integer was expected."
E_US1752_5970:E_GE7918_SYNTAX_ERROR
"Unrecognized scope '%0c' in a SET LOCKMODE command.
Please use one of the following:
\n
\n          SESSION, ON tablename"
E_US1753_5971:E_GE7918_SYNTAX_ERROR
"Unrecognized parameter in a SET AGGREGATE command.
Please use one of the following:
\n
\n          PROJECT, NOPROJECT"
E_US1754_5972:E_GEA0F0_SQL_STMT_INV
"The SET QUERY_SIZE command is no longer supported.
This function is now done automatically by the system."
E_US1755_5973:E_GE7918_SYNTAX_ERROR
"No file name was given for a 'SET TRACE INPUT' or 'SET TRACE OUTPUT' command."
E_US1756_5974:E_GEA0F0_SQL_STMT_INV
"The SET AUTOCOMMIT command is not supported in QUEL."
E_US1757_5975:E_GEA0F0_SQL_STMT_INV
"The SET CACHE command is no longer supported."
E_US1758_5976:E_GEA0F0_SQL_STMT_INV
"The SET J_SORBUFSZ command is no longer supported."
E_US1759_5977:E_GEA0F0_SQL_STMT_INV
"The SET RESULT_STRUCTURE command is no longer supported."
E_US175A_5978:E_GEA0F0_SQL_STMT_INV
"The SET RET_INTO command is no longer supported."
E_US175B_5979:E_GEA0F0_SQL_STMT_INV
"The SET AGGREGATE command is no longer supported."
E_US175C_5980:E_GEA0F0_SQL_STMT_INV
"The SET JOINOP [NO]TIMEOUT command is not allowed in distributed INGRES."
E_US175D_5981:E_GE7918_SYNTAX_ERROR
"SET JOINOP: invalid option '%0c'. Must be [NO]TIMEOUT."
E_US175E_5982:E_GEA0F0_SQL_STMT_INV
"The SET J_SORTBUFSZ command is no longer supported."
E_US175F_5983:E_GE7918_SYNTAX_ERROR
"SET AUTOCOMMIT: invalid option '%0c'. Must be ON or OFF."
E_US1760_5984:E_GE80E8_LOGICAL_ERROR
"A compressed table is not accessible due to a change in the\n
tuple compression routines. MODIFYing the table to a compressed\n
structure under this version makes it accessible once again."
E_US1761_5985:E_GE7918_SYNTAX_ERROR
"SET DDL_CONCURRENCY: invalid option '%oc'.
\nMust be ON or OFF."
E_US1762_5986:E_GEA0F0_SQL_STMT_INV
"'SET LOCKMODE' on views is not supported in distributed INGRES"
E_US1763_5987:E_GEA0F0_SQL_STMT_INV
"SET LOCKMODE SESSION: parameter '%0c' is not supported
\n    in distributed INGRES"
E_US1839_6201:E_GE7594_TABLE_NOT_FOUND
"RELOCATE: Table '%0c' doesn't exist."
E_US183A_6202:NO_GE
"RELOCATE: Table '%0c' can not be relocated."
E_US183B_6203:E_GE797C_INVALID_IDENT
"RELOCATE: Locationname '%0c' is not valid for this data base."
E_US183C_6204:E_GE7594_TABLE_NOT_FOUND
"RELOCATE: Table '%0c' does not belong to you."
E_US183D_6205:E_GE7D00_QUERY_ERROR
"RELOCATE: Table '%0c' is a view."
E_US183E_6206:E_GEA0F0_SQL_STMT_INV
"The RELOCATE command is not allowed in distributed INGRES."
E_US183F_6207:E_GE7D00_QUERY_ERROR
"RELOCATE: To relocate table '%0c', which resides in multiple locations,
use RELOCATE variant of the MODIFY command.
\nRELOCATE command can only be used on a table residing in a single location."
E_US185B_6235:E_GEA0F0_SQL_STMT_INV
"The SET [NO]MAX... statements are not allowed in distributed INGRES.\n
Enter this command only under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered a SET [NO]MAX... statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command only under local INGRES.
**
*/
E_US185C_6236:E_GE7918_SYNTAX_ERROR
"You cannot specify a value on SET NOMAX... statements.\n
Reissue the statement with no value specified."
/*%
** Parameters:	(none)
**
** Explanation:
**	The user specified a value on a SET NOMAX... statement.
**	This is a syntax error.
**
** System Status:
**	The statement is aborted.
**
** User action:
**	Issue the statement with no value specified.
**
*/
E_US185D_6237:E_GE7918_SYNTAX_ERROR
"You must specify a non-negative integral value on SET MAX... statements.\n
Reissue the statement with a non-negative integral value specified."
/*%
** Parameters:	(none)
**
** Explanation:
**	The user omitted a value on a SET MAX... statement,
**	or an invalid value was specified.
**	This is a syntax error.
**
** System Status:
**	The statement is aborted.
**
** User action:
**	Issue the statement with a value specified.
**
*/
E_US1860_EXCESSIVE_MAXCOST:E_GE84D0_NO_PRIVILEGE
"The value specified on the SET MAXCOST statement (%0d)\n
exceeds your session's QUERY_COST_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXCOST to reset MAXCOST to your limit."
/*%
** Parameters:
**	%0 - The value specified on the SET MAXCOST statement
**	%1 - The session's value for QUERY_COST_LIMIT
**
** Explanation:
**	The user specified a value on the SET MAXCOST statement
**	that was greater than the session's QUERY_COST_LIMIT.
**	This is not allowed.
**
** System Status:
**	The session's value for MAXCOST is not adjusted.
**
** User action:
**	Reissue the statement with a value less than or
**	equal to the session's QUERY_COST_LIMIT.
**
*/
E_US1861_EXCESSIVE_MAXCPU:E_GE84D0_NO_PRIVILEGE
"The value specified on the SET MAXCPU statement (%0d)\n
exceeds your session's QUERY_CPU_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXCPU to reset MAXCPU to your limit."
/*%
** Parameters:
**	%0 - The value specified on the SET MAXCPU statement
**	%1 - The session's value for QUERY_CPU_LIMIT
**
** Explanation:
**	The user specified a value on the SET MAXCPU statement
**	that was greater than the session's QUERY_CPU_LIMIT.
**	This is not allowed.
**
** System Status:
**	The session's value for MAXCPU is not adjusted.
**
** User action:
**	Reissue the statement with a value less than or
**	equal to the session's QUERY_CPU_LIMIT.
**
*/
E_US1862_EXCESSIVE_MAXIO:E_GE84D0_NO_PRIVILEGE
"The value specified on the SET MAXIO/MAXQUERY statement (%0d)\n
exceeds your session's QUERY_IO_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXIO to reset MAXIO/MAXQUERY to your limit."
/*%
** Parameters:
**	%0 - The value specified on the SET MAXIO/MAXQUERY statement
**	%1 - The session's value for QUERY_IO_LIMIT
**
** Explanation:
**	The user specified a value on the SET MAXIO/MAXQUERY statement
**	that was greater than the session's QUERY_IO_LIMIT.
**	This is not allowed.
**
** System Status:
**	The session's value for MAXIO is not adjusted.
**
** User action:
**	Reissue the statement with a value less than or
**	equal to the session's QUERY_IO_LIMIT.
**
*/
E_US1863_EXCESSIVE_MAXPAGE:E_GE84D0_NO_PRIVILEGE
"The value specified on the SET MAXPAGE statement (%0d)\n
exceeds your session's QUERY_PAGE_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXPAGE to reset MAXPAGE to your limit."
/*%
** Parameters:
**	%0 - The value specified on the SET MAXPAGE statement
**	%1 - The session's value for QUERY_PAGE_LIMIT
**
** Explanation:
**	The user specified a value on the SET MAXPAGE statement
**	that was greater than the session's QUERY_PAGE_LIMIT.
**	This is not allowed.
**
** System Status:
**	The session's value for MAXPAGE is not adjusted.
**
** User action:
**	Reissue the statement with a value less than or
**	equal to the session's QUERY_PAGE_LIMIT.
**
*/
E_US1864_EXCESSIVE_MAXROW:E_GE84D0_NO_PRIVILEGE
"The value specified on the SET MAXROW statement (%0d)\n
exceeds your session's QUERY_ROW_LIMIT (%1d).\n
Specify a value within range,\n
or use SET NOMAXROW to reset MAXROW to your limit."
/*%
** Parameters:
**	%0 - The value specified on the SET MAXROW statement
**	%1 - The session's value for QUERY_ROW_LIMIT
**
** Explanation:
**	The user specified a value on the SET MAXROW statement
**	that was greater than the session's QUERY_ROW_LIMIT.
**	This is not allowed.
**
** System Status:
**	The session's value for MAXROW is not adjusted.
**
** User action:
**	Reissue the statement with a value less than or
**	equal to the session's QUERY_ROW_LIMIT.
**
*/
E_US1865_6245:E_GE84D0_NO_PRIVILEGE
"You are not allowed to create tables in this database.\n
Contact your INGRES Database Administrator for further information."
/*%
** Parameters:  (none)
**
** Explanation:
**	The user attempted to create a permanent table,
**	but the session does not have CREATE_TABLE privileges
**	on the active database.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Contact the Database Administrator to get CREATE_TABLE
**	privileges if required.
**	Note: This privilege may be granted to one of several
**	authorization identifier that apply to a session, including
**	the role, group and user identifiers, as well as PUBLIC.
**
*/
E_US1866_6246:E_GE84D0_NO_PRIVILEGE
"You are not allowed to create database procedures in this database.\n
Contact your INGRES Database Administrator for further information."
/*%
** Parameters:  (none)
**
** Explanation:
**	The user attempted to issue the [CREATE] PROCEDURE command,
**	but the session does not have CREATE_PROCEDURE privileges
**	on the active database.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Contact the Database Administrator to get CREATE_PROCEDURE
**	privileges if required.
**	Note: This privilege may be granted to one of several
**	authorization identifier that apply to a session, including
**	the role, group and user identifiers, as well as PUBLIC.
**
*/
E_US1867_6247:E_GE84D0_NO_PRIVILEGE
"You are not allowed to specify the SET LOCKMODE statement\n
while connected to this database.\n
Contact your INGRES Database Administrator for further information."
/*%
** Parameters:  (none)
**
** Explanation:
**	The user attempted to issue the SET LOCKMODE command,
**	but the session does not have LOCKMODE privileges
**	on the active database.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Contact the Database Administrator to get LOCKMODE
**	privileges if required.
**	Note: This privilege may be granted to one of several
**	authorization identifier that apply to a session, including
**	the role, group and user identifiers, as well as PUBLIC.
**
*/
E_US1868_6248:E_GE7D00_QUERY_ERROR
"One or more users have group '%0c' as a default group.\n
You cannot drop a group (or all users from a group)\n
if the group is the default for any user.\n
Use ACCESSDB to remove this group as default and try again."
/*%
** Parameters:
**	%0	Specified group  identifier 
**
** Explanation:
**	The user attempted to drop a group, or all members
**	from a group, but one or more users have the group
**	as a default group.
**
** System Status:
**	The statement is not aborted; subsequent groups
**	are processed.
**
** User Action:
**	Make sure the group was properly entered.
**	If so, use ACCESSDB to change the default
**	group for all users who have this group
**	as their default.
**
*/
E_US1869_6249:E_GE7D00_QUERY_ERROR
"Group '%0c' is the default group for user '%1c'.\n
You cannot drop a user from a group\n
if the group is the user's default group.\n
Use ACCESSDB to change the user's default group and try again."
/*%
** Parameters:
**	%0	Specified group  identifier 
**	%1	Specified member identifier 
**
** Explanation:
**	The user attempted to drop a member from a group
**	with ALTER GROUP, but the group is the specified
**	user's default group.
**
** System Status:
**	The statement is not aborted; subsequent members
**	are processed.
**
** User Action:
**	Make sure the member was properly entered.
**	If so, use ACCESSDB to change the user's default
**	group before dropping the user from the group.
**
*/
E_US186A_6250:E_GE80E8_LOGICAL_ERROR
"An error occurred while processing the iiusergroup catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iiusergroup catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Note: group identifiers are not
**	verified for super users.
**	If it occurs during CREATE/ALTER/DROP processing,
**	the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US186B_6251:E_GE80E8_LOGICAL_ERROR
"An error occurred while processing the iirole catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iirole catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Note: role identifiers are not
**	verified for super users.
**	If it occurs during CREATE/ALTER/DROP processing,
**	the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US186C_6252:E_GE75BC_UNKNOWN_OBJECT
"Group authorization check failed.\n
Either the specified (or default) group does not exist,\n
or you are not a member of the specified (or default) group.\n
Contact your INGRES system manager for further assistance."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued by SCF during session initialization
**	if the session's group identifier is not defined, or if
**	the user is not a member of the session's group.  
**	Note: if no group was specified on the connect statement,
**	the default group (if any) from iiuser is used.
**
** System Status:
**	For non-super users, the session is disconnected.
**	For super users, this message is written to the log only,
**	and the session is allowed to continue with the specified
**	group identifier in effect.
**
** User Action:
**	Ensure the proper group identifier was specified.
**	Call the INGRES system administrator if no trouble found.
**
*/
E_US186D_6253:E_GE75BC_UNKNOWN_OBJECT
"Role authorization check failed.\n
Either the specified role does not exist,\n
or the specified password is invalid.\n
Contact your INGRES system manager for further assistance."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued by SCF during session initialization
**	if the session's role identifier is not defined, or if
**	the specified password is not valid.
**	Note: the user may not be aware that a role identifier
**	was specified on the connect statement by the invoked
**	application.
**
** System Status:
**	For non-super users, the session is disconnected.
**	For super users, this message is written to the log only,
**	and the session is allowed to continue with the specified
**	role identifier in effect.
**
** User Action:
**	Ensure the proper role identifier was specified.
**	Call the INGRES system administrator if no trouble found.
**
*/
E_US186E_6254:E_GE84D0_NO_PRIVILEGE
"You are not authorized to issue\n
CREATE/ALTER/DROP GROUP statements.\n
Contact your INGRES system manager for further assistance."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the CREATE GROUP, ALTER GROUP,
**	or DROP GROUP statement, but the user was not a super user
**	or did not have update system catalog privileges.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US186F_6255:E_GE84D0_NO_PRIVILEGE
"You are not authorized to issue\n
CREATE/ALTER/DROP ROLE statements.\n
Contact your INGRES system manager for further assistance."
/*%
** Parameters:
**
** Explanation:
**	The user attempted to issue the CREATE ROLE,
**	ALTER ROLE, or DROP ROLE statement,
**	but the user was not a super user or did not have update
**	system catalog privileges.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US1870_6256:E_GE84D0_NO_PRIVILEGE
"You cannot CREATE/ALTER/DROP GROUP\n
while in session with this database.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the CREATE GROUP, ALTER GROUP,
**	or DROP GROUP statement, but the user was not in session
**	with the iidbdb database.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue these commands when connected to the iidbdb database.
**
*/
E_US1871_6257:E_GE84D0_NO_PRIVILEGE
"You cannot CREATE/ALTER/DROP ROLE\n
while in session with this database.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the CREATE ROLE,
**	ALTER ROLE, or DROP ROLE statement,
**	but the user was not in session with the iidbdb database.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue these commands when connected to the iidbdb database.
**
*/
E_US1872_6258:E_GE75F8_DEF_RESOURCE
"Group '%0c' already exists.\n
You cannot create a group that already exists.\n
Ensure you entered the correct group identifier."
/*%
** Parameters:
**	%0	Specified group identifier 
**
** Explanation:
**	The user attempted to create a group that already exists.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Ensure the group name was properly entered.
**
*/
E_US1873_6259:E_GE75F8_DEF_RESOURCE
"Role '%0c' already exists.\n
You cannot create a role that already exists.\n
Ensure you entered the correct role identifier."
/*%
** Parameters:
**	%0	Specified role identifier 
**
** Explanation:
**	The user attempted to create a role
**	that already exists.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Ensure the role was properly entered.
**
*/
E_US1874_6260:E_GE75BC_UNKNOWN_OBJECT
"Group '%0c' does not exist.\n
You cannot alter or drop a non-existing group.\n
Ensure you entered the correct group identifier."
/*%
** Parameters:
**	%0	Specified group identifier 
**
** Explanation:
**	The user attempted to alter or drop a group
**	identifier that does not exist.
**
** System Status:
**	For ALTER GROUP, the statement is aborted.
**	For DROP GROUP the statement is not aborted;
**	subsequent specified groups are processed.
**
** User Action:
**	Make sure the group identifier was properly entered.
**
*/
E_US1875_6261:E_GE75BC_UNKNOWN_OBJECT
"Role '%0c' does not exist.\n
You cannot alter or drop a non-existing role.\n
Ensure you entered the correct role identifier."
/*%
** Parameters:
**	%0	Specified role identifier 
**
** Explanation:
**	The user attempted to alter or drop a role
**	identifier that does not exist.
**
** System Status:
**	For ALTER ROLE, the statement is aborted.
**	For DROP ROLE, the statement is not aborted;
**	subsequent specified roles are processed.
**
** User Action:
**	Make sure the role was properly entered.
**
*/
E_US1876_6262:E_GE75F8_DEF_RESOURCE
"Group '%0c' already contains user '%1c'.\n
You cannot add a member to a group\n
if the member already exists in the group.\n
Ensure you entered the correct group and user identifiers."
/*%
** Parameters:
**	%0	Specified group  identifier 
**	%1	Specified member identifier 
**
** Explanation:
**	The user attempted to add a member to a group
**	with ALTER GROUP, but the specified member already
**	exists in the group.
**
** System Status:
**	The statement is not aborted; subsequent members
**	are processed.
**
** User Action:
**	Make sure the member was properly entered.
**
*/
E_US1877_6263:E_GE75BC_UNKNOWN_OBJECT
"Group '%0c' does not contain user '%1c'.\n
You cannot drop a member from a group\n
if the member does not exist in the group.\n
Ensure you entered the correct group and user identifiers."
/*%
** Parameters:
**	%0	Specified group  identifier 
**	%1	Specified member identifier 
**
** Explanation:
**	The user attempted to drop a member from a group
**	with ALTER GROUP, but the specified member does not
**	exist in the group.
**
** System Status:
**	The statement is not aborted; subsequent members
**	are processed.
**
** User Action:
**	Make sure the member was properly entered.
**
*/
E_US1878_6264:E_GE7D00_QUERY_ERROR
"Group '%0c' is not empty.\n
You cannot drop a group when members exist in the group.\n
Use 'ALTER GROUP %0c DROP ALL'\n
to drop all members of the group before you drop the group."
/*%
** Parameters:
**	%0	Specified group  identifier 
**
** Explanation:
**	The user attempted to drop a group identifier
**	that still has members.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Make sure the group identifier was properly specified.
**	If so, drop all members from the group before attempting
**	to drop the group.  This can be done with the ALTER GROUP
**	... DROP ALL statement.
**
*/
E_US1879_6265:E_GE75F8_DEF_RESOURCE
"Unable to create group '%0c'.\n
A user with this name already exists in this installation.\n
Group names must be unique within groups, roles, and users.\n
Please specify a different group name."
/*%
** Parameters:
**	%0	Specified group  identifier 
**
** Explanation:
**	The user attempted to create a group that has
**	the same name as an existing user.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Make sure the group identifier was properly specified.
**	Select a different group identifier.
**
*/
E_US187A_6266:E_GE75F8_DEF_RESOURCE
"Unable to create group '%0c'.\n
A role with this name already exists in this installation.\n
Group names must be unique within groups, roles, and users.\n
Please specify a different group name."
/*%
** Parameters:
**	%0	Specified group  identifier 
**
** Explanation:
**	The user attempted to create a group that has
**	the same name as an existing role.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Make sure the group identifier was properly specified.
**	Select a different group identifier.
**
*/
E_US187B_6267:E_GE75F8_DEF_RESOURCE
"Unable to create role '%0c'.\n
A user with this name already exists in this installation.\n
Role names must be unique within groups, roles, and users.\n
Please specify a different role name."
/*%
** Parameters:
**	%0	Specified role identifier 
**
** Explanation:
**	The user attempted to create a role that has
**	the same name as an existing user.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Make sure the role identifier was properly specified.
**	Select a different role identifier.
**
*/
E_US187C_6268:E_GE75F8_DEF_RESOURCE
"Unable to create role '%0c'.\n
A group with this name already exists in this installation.\n
Role names must be unique within groups, roles, and users.\n
Please specify a different role name."
/*%
** Parameters:
**	%0	Specified role identifier 
**
** Explanation:
**	The user attempted to create a role that has
**	the same name as an existing group.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Make sure the role identifier was properly specified.
**	Select a different role identifier.
**
*/
E_US187D_6269:NO_GE
"Unable to add '%1c' to group '%0c'.\n
You cannot add a member to a group\n
if the member does not exist as a user in the installation.\n
Ensure you entered the correct user identifier."
/*%
** Parameters:
**	%0	Specified group  identifier 
**	%1	Specified member identifier 
**
** Explanation:
**	The user attempted to add a member to a group
**	with CREATE/ALTER GROUP, but the specified member 
**	does not exist as a user in the iiuser catalog.
**
** System Status:
**	The statement is not aborted; subsequent members
**	are processed.
**
** User Action:
**	Make sure the member was properly entered.
**
*/
E_US187E_6270:E_GE7918_SYNTAX_ERROR
"CREATE GROUP: '%0c' is either an invalid option or the syntax
of the WITH clause is invalid.
\nThe syntax of the WITH clause for this statement is:
\n	WITH USERS = (user {,user})"
/*%
** Parameters:
**	%0	The invalid keyword
**
** Explanation:
**	The user entered an invalid WITH clause on the
**	CREATE GROUP statement
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Reenter the command, using the correct syntax.
**
*/
E_US187F_6271:E_GE7918_SYNTAX_ERROR
"ALTER GROUP: '%0c' is either an invalid option or the syntax
of the ADD/DROP clause is invalid.
\nSpecify one of:
\n	ADD  USERS (user {,user})
\n	DROP USERS (user {,user})
\n	DROP ALL"
/*%
** Parameters:
**	%0	The invalid keyword
**
** Explanation:
**	The user entered an invalid ADD or DROP clause on the
**	ALTER GROUP statement
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Reenter the command, using the correct syntax.
**
*/
E_US1880_6272:E_GE7918_SYNTAX_ERROR
"CREATE ROLE: '%0c' is either an invalid option or the syntax
of the WITH clause is invalid.
\nSpecify one of:
\n	WITH PASSWORD = 'password'
\n	WITH NOPASSWORD"
/*%
** Parameters:
**	%0	The invalid keyword
**
** Explanation:
**	The user entered an invalid WITH clause on the
**	CREATE ROLE statement
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Reenter the command, using the correct syntax.
**
*/
E_US1881_6273:E_GE7918_SYNTAX_ERROR
"ALTER ROLE: '%0c' is either an invalid option or the syntax
of the WITH clause is invalid.
\nSpecify one of:
\n	WITH PASSWORD = 'password'
\n	WITH NOPASSWORD"
/*%
** Parameters:
**	%0	The invalid keyword
**
** Explanation:
**	The user entered an invalid WITH clause on the
**	ALTER ROLE statement
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Reenter the command, using the correct syntax.
**
*/
E_US1882_6274:E_GEA0F0_SQL_STMT_INV
"The CREATE GROUP statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered the CREATE GROUP statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US1883_6275:E_GEA0F0_SQL_STMT_INV
"The ALTER GROUP statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered the ALTER GROUP statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US1884_6276:E_GEA0F0_SQL_STMT_INV
"The DROP GROUP statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered the DROP GROUP statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US1885_6277:E_GEA0F0_SQL_STMT_INV
"The CREATE ROLE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered the CREATE ROLE statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US1886_6278:E_GEA0F0_SQL_STMT_INV
"The ALTER ROLE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered the ALTER ROLE statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US1887_6279:E_GEA0F0_SQL_STMT_INV
"The DROP ROLE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered the DROP ROLE statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US1888_6280:NO_GE
"An error occurred while processing the iidbpriv catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iidbpriv catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Super users are allowed to continue.
**	If it occurs while processing GRANT/REVOKE statements
**	for database privileges, the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US1889_6281:E_GE84D0_NO_PRIVILEGE
"You are not authorized to GRANT or REVOKE privileges\n
on database '%0c'.\n
Contact your INGRES system manager for further assistance."
/*%
** Parameters:
**
** Explanation:
**	The user attempted to issue the GRANT/REVOKE statement
**	for database privileges,
**	but the user was not a super user, did not have update
**	system catalog privileges, or was the DBA for the specified
**	database.
**
** System Status:
**	The statement continues with other databases specified, if any.
**
** User Action:
**	Be sure the database name was properly specified.
**	Call the INGRES system administrator if no trouble found.
**
*/
E_US188A_6282:NO_GE
"You cannot GRANT or REVOKE database privileges\n
while in session with this database.\n
Ensure your session is connected to the iidbdb database."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the GRANT or REVOKE statement
**	for database privileges, but the user was not in session
**	with the iidbdb database.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue these commands when connected to the iidbdb database.
**
*/
E_US188B_6283:E_GE7D00_QUERY_ERROR
"You cannot GRANT or REVOKE database privileges\n
on non-database objects.\n
Ensure the privileges that you are granting or revoking\n
are consistent with the objects that you specify."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the GRANT or REVOKE statement
**	for database privileges, but specified objects are not 
**	databases.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Check to ensure ON DATABASE is specified when granting
**	database privileges.
**
*/
E_US188C_6284:E_GE7D00_QUERY_ERROR
"You cannot GRANT or REVOKE non-database privileges\n
on database objects.\n
Ensure the privileges that you are granting or revoking\n
are consistent with the objects that you specify."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the GRANT or REVOKE statement
**	for non-database privileges, but specified objects are 
**	databases.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Check to ensure ON DATABASE is not specified when granting
**	non-database privileges.
**
*/
E_US188D_6285:E_GE7918_SYNTAX_ERROR
"You must specify a non-negative integral value for '%0c'\n
when granting this database privilege.\n
Please reenter this statement with a non-negative integral value specified."
/*
** Parameters:
**	%0 - Privilege for which value was omitted.
**	
** Explanation:
**	The user attempted to GRANT a database privilege that requires
**	a value, but no value was specified.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with a value specified
**	for the database privilege.
**	
*/
E_US188E_6286:E_GE7918_SYNTAX_ERROR
"You must not specify a value for '%0c'\n
when revoking this database privilege.\n
Please reenter this statement without a value specified."
/*
** Parameters:
**	%0 - Privilege for which value was specified.
**	
** Explanation:
**	The user attempted to REVOKE a database privilege,
**	but a value was specified for that privilege.
**	This is not consistent with the REVOKE syntax.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command without a value specified
**	for the database privilege.
**	
*/
E_US188F_6287:E_GE7918_SYNTAX_ERROR
"You must never specify a value for '%0c'\n
when granting or revoking this privilege.\n
Please reenter this statement without a value specified."
/*
** Parameters:
**	%0 - Privilege for which value was specified.
**	
** Explanation:
**	The user attempted to GRANT or REVOKE a
**	privilege that does not require a value, but
**	a value was specified.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command without a value specified
**	for the database privilege.
**	
*/
E_US1890_6288:E_GE7918_SYNTAX_ERROR
"Unrecognized privilege specified: '%0c'\n
Please check the spelling of this privilege and reenter the command."
/*
** Parameters:
**	%0 - the invalid privilege
**	
** Explanation:
**	The user attempted to GRANT or REVOKE an unknown privilege.
**	This statement is also issued for CREATE/ALTER USER.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the correct
**	privilege specified.
**	
*/
E_US1891_6289:E_GE7D00_QUERY_ERROR
"Conflicting values specified for '%0c'\n
Please reenter the command with one value specified for this privilege."
/*
** Parameters:
**	%0 - the invalid privilege
**	
** Explanation:
**	The user attempted to GRANT a database privilege that
**	requires a value, but the privilege was specified with
**	multiple values.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the privilege
**	specified once.
**	
*/
E_US1892_6290:E_GEA0F0_SQL_STMT_INV
"The REVOKE statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters: (none)
**
** Explanation:
**	The user entered the REVOKE statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US1893_6291:E_GE75BC_UNKNOWN_OBJECT
"Database '%0c' does not exist.\n
You cannot GRANT or REVOKE privileges on a database that doesn't exist.\n
Be sure you specified the correct database name."
/*%
** Parameters:
**	%0c - the database name
**
** Explanation:
**	The user specified a non-existing database on the GRANT/REVOKE
**	statement.  NOTE: Super users and users with update system
**	catalog privileges are allowed ro REVOKE privileges on non-
**	existing databases.
**
** System Status:
**	The statement continues with other databases specified, if any.
**
** User Action:
**	Be sure the database name was properly specified.
**	Call the INGRES system administrator if no trouble found.
**
*/
E_US1894_6292:E_GE7D00_QUERY_ERROR
"REVOKE error:\n
One or more of the specified privileges were never granted\n
on database '%0c' to '%1c'.\n
You cannot revoke privileges that were never granted.\n
Be sure you specified the correct privileges."
/*%
** Parameters:
**	%0c - the database name
**	%1c - the grantee name
**
** Explanation:
**	The user attempted to REVOKE privileges that were never
**	granted.
**
** System Status:
**	The statement continues, but the noted database/grantee
**	is ignored. All other database/grantees are processed.
**
** User Action:
**	Be sure the privilege and grantee were properly specified.
**
*/
E_US1895_6293:E_GE75B2_NOT_FOUND
"User '%1c' does not exist.\n
You cannot GRANT database privileges to a user that doesn't exist.\n
Be sure you specified the correct grantee type and name."
/*%
** Parameters:
**	%0c - the database name
**	%1c - the grantee name
**
** Explanation:
**	The user attempted to GRANT database privileges to
**	a user that does not exist.
**
** System Status:
**	The statement continues, but the noted database/grantee
**	is ignored. All other database/grantees are processed.
**
** User Action:
**	Be sure the grantee was properly specified.
**
*/
E_US1896_6294:E_GE75B2_NOT_FOUND
"Group '%1c' does not exist.\n
You cannot GRANT database privileges to a group that doesn't exist.\n
Be sure you specified the correct grantee type and name."
/*%
** Parameters:
**	%0c - the database name
**	%1c - the grantee name
**
** Explanation:
**	The user attempted to GRANT database privileges to
**	a group that does not exist.
**
** System Status:
**	The statement continues, but the noted database/grantee
**	is ignored. All other database/grantees are processed.
**
** User Action:
**	Be sure the grantee was properly specified.
**
*/
E_US1897_6295:E_GE75B2_NOT_FOUND
"Role '%1c' does not exist.\n
You cannot GRANT database privileges to a role that doesn't exist.\n
Be sure you specified the correct grantee type and name."
/*%
** Parameters:
**	%0c - the database name
**	%1c - the grantee name
**
** Explanation:
**	The user attempted to GRANT database privileges to
**	a role that does not exist.
**
** System Status:
**	The statement continues, but the noted database/grantee
**	is ignored. All other database/grantees are processed.
**
** User Action:
**	Be sure the grantee was properly specified.
**
*/
E_US1898_6296:NO_GE
"An error occurred while processing the iidatabase catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if QEF has trouble opening,
**	reading, or closing the iidatabase catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs while processing GRANT/REVOKE statements
**	for database privileges, the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US1899_6297:NO_GE
"An error occurred while processing the iiuser catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if QEF has trouble opening,
**	reading, or closing the iiuser catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs while processing GRANT/REVOKE statements
**	for database privileges, the statement is aborted.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US189A_6298:E_GE0032_WARNING
"All database privileges have been removed for '%0c'\n
Database privileges granted to a user, group, or role \n
are removed when that user, group, or role is dropped."
/*%
** Parameters:
**	%0	Specified user, group, or role identifier 
**
** Explanation:
**	When dropping a user, group, or role, all database
**	privileges granted to that grantee are implicitly
**	revoked.  This is an informational message only.
**
** System Status:
**	The statement is continues.
**
** User Action:
**	None.
**
*/
E_US189C_6300_RULE_EXISTS:E_GE75F8_DEF_RESOURCE
"CREATE RULE: Rule '%0c' already exists."
/*
** Parameters:
**	%0 - Name of rule that is being created.
** Explanation:
**	The named rule already exists for the current user.  Duplicate rule
**	names, even if applied to different tables, are not allowed.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Choose a unique name for the new rule or delete the existing named
**	rule.
*/
E_US189D_6301_RULE_ABSENT:E_GE75B2_NOT_FOUND
"DROP RULE: Rule '%0c' does not exist."
/*
** Parameters:
**	%0 - Name of rule that is being dropped.
** Explanation:
**	The named rule does not exist for the current user.  Rules owned
**	by other users cannot be dropped.
** System Status:
**	Program continues and current DROP statement is aborted.
** User Action:
**	Retype the rule name.
*/
E_US189E_6302_RULE_QUAL:E_GE7918_SYNTAX_ERROR
"CREATE RULE: Invalid predicate in WHERE clause."
/*
** Parameters:
**	None
** Explanation:
**	WHERE clauses associated with the CREATE RULE statement may not
**	reference sub-SELECT queries or set functions (such as SUM or COUNT).
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Restructure your WHERE clause (or remove it) so that it is accepted
**	as a rule qualification.  You may also move your WHERE clause into
**	procedure being fired by the rule and use it as part of qualifying
**	SELECT statement to filter out the row.
*/
E_US189F_6303_RULE_COL:E_GE7D00_QUERY_ERROR
"CREATE RULE: Unqualified reference to column '%0c' - use OLD or NEW."
/*
** Parameters:
**	%0 - Name of a column referenced in the rule.
** Explanation:
**	Since OLD and NEW (and the table name) are implicit correlation
**	names for column values before and after updating a row in the
**	referenced table, you must qualify the column.  Unqualified columns
**	may yield ambiguous results.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Qualify all column names referenced in the rule.
*/
E_US18A0_6304_RULE_STAR:E_GEA0F0_SQL_STMT_INV
"The CREATE and DROP RULE statements are not allowed in distributed INGRES."
/*
** Parameters:
**	None
** Explanation:
**	INGRES/STAR does not yet support rule creation.
** System Status:
**	Program continues and current statement is aborted.
** User Action:
**	Remove statement or send it through a DIRECT CONNECT session.
*/
E_US18A1_6305_RULE_II:E_GE797C_INVALID_IDENT
"CREATE RULE: Invalid rule name '%0c' starts with INGRES reserved prefix 'ii'."
/*
** Parameters:
**	%0 - Name of rule that is being created.
** Explanation:
**	INGRES reserves all objects that begin with "ii".  Rule names are no
**	exception.  Try "jj" - INGRES doesn't yet reserve this.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Try the prefix "jj" - INGRES doesn't yet reserve this.
*/
E_US18A2_6306_RULE_COLTAB:E_GE759E_COLUMN_UNKNOWN
"CREATE RULE: Column '%0c' is not in table '%1c'."
/*
** Parameters:
**	%0 - Name of column referenced in rule.
**	%1 - Name of table referenced in rule.
** Explanation:
**	The column specified in the rule was not found in the table for
**	which the rule is being created.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Retype the column name (or table name).
*/
E_US18A3_6307_RULE_STMT:E_GE7918_SYNTAX_ERROR
"CREATE RULE: Duplicate statement types listed in AFTER clause."
/*
** Parameters:
**	None
** Explanation:
**	The AFTER clause of a CREATE RULE statement may not specify a 
**	statement type twice.  This could lead to ambiguous rule application,
**	specifically for the UPDATE operation.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Specify each statement type at most once.
*/
E_US18A4_6308_RULE_VIEW:E_GE7D00_QUERY_ERROR
"CREATE RULE: Rules cannot be applied to a view '%0c'."
/*
** Parameters:
**	%0 - Name of view to which rule is being applied.
** Explanation:
**	Rules may only be applied to base tables.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Apply the rule to the base table for the view.
*/
E_US18A5_6309_RULE_OWN:E_GE84D0_NO_PRIVILEGE
"CREATE RULE: You must be the owner of '%0c' in order to apply rules."
/*
** Parameters:
**	%0 - Name of table to which rule is being applied.
** Explanation:
**	You can only apply a rule to a table if you own the table.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
*/
E_US18A6_6310_RULE_OLDNEW:E_GE7918_SYNTAX_ERROR
"CREATE RULE: Invalid word '%0c' in REFERENCING clause - use OLD or NEW."
/*
** Parameters:
**	%0 - Error token.
** Explanation:
**	The syntax of a rule REFERENCING clause is:
**	   REFERENCING ref_clause [ref_clause]
**	where ref_clause is:
**	   OLD | NEW  AS correlation_name
**	This clause is available in order to apply rules to tables actually
**	called NEW or OLD with the intent of using the old and/or new values
**	of a column.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Use OLD and/or NEW.
*/
E_US18A7_6311_RULE_WHEN:E_GE7918_SYNTAX_ERROR
"CREATE RULE: A rule can only be fired AFTER an SQL statement.
Check spelling and position of the word AFTER."
/*
** Parameters:
**	None
** Explanation:
**	Rules cannot yet be fired BEFORE an SQL statement.
** System Status:
**	Program continues and current CREATE statement is aborted.
** User Action:
**	Check the spelling of AFTER and correct the statement.
*/
E_US18A8_6312_RULE_DRPTAB:E_GE7594_TABLE_NOT_FOUND
"DROP RULE: Rule '%0c' dropped but table referenced by rule no longer exists."
/*
** Parameters:
**	%0 - Name of rule that is being dropped.
** Explanation:
**	The specified rule was removed from the system catalogs, but the table
**	referenced by the rule (in the original CREATE RULE statement) no
**	longer exists.  This could occur due to direct removal of the table
**	from the system catalogs without using the DROP TABLE statement or
**	issuing the DROP TABLE statement through an older version of INGRES.
** System Status:
**	Rule is dropped and program continues.
** User Action:
**	Verify that rule no longer exists, and that the table referenced by
**	the rule did not exist.  If the table still exists under the 
**	original owner then contact the INGRES system administrator.
*/
E_US18A9_6313_RULE_CAT:E_GE84D0_NO_PRIVILEGE
"CREATE RULE: You do not have permission to apply a rule to table '%0c'."
/*
** Parameters:
**	%0 - Name of table to which rule is being applied.
** Explanation:
**	The specified table is an INGRES protected table and rules cannot
**	be applied to it without explicit "update catalog" permission.
** System Status:
**	Program continues and current CREATE RULE statement is aborted.
** User Action:
**	Only the DBA or INGRES system administrator may apply rules to
**	INGRES tables.
*/
E_US18AA_6314_SET_NORULES:E_GE84D0_NO_PRIVILEGE
"SET [NO]RULES:  You do not have permission to turn rules on or off.
Only the DBA may issue this statement or equivalent SET TRACE statement."
/*%
** Parameters:
**	None
** Explanation:
**	The SET [NO]RULES statement may only be issued by the DBA.  This
**	statement is provided for rule development purposes and DBA tools.
** System Status:
**	Program continues and statement is aborted.
** User Action:
** 	Consult your DBA for permission to turn rules on or off.
**	Alternatives:
**	  1. DROP rules and recreate them later.
**	  2. Insert a check into your procedures (invoked by a rule) to 
**	     SELECT some global debug parameter from a global table.  If this 
**	     is set to say, "rules_off", then immediately return from the 
**	     procedure.
*/		
E_US18B6_6326_USER_EXISTS:E_GE75F8_DEF_RESOURCE
"CREATE USER: The user '%0c' already exists."
/*
** Parameters:
**	%0 - Name of user to be added.
** Explanation:
**	The specified table is an INGRES user table and duplicates
**	are not allowed.
** System Status:
**	Program continues and current CREATE USER statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can add a user.
*/
E_US18B7_6327_USER_ABSENT:E_GE75B2_NOT_FOUND
"DROP USER: The user '%0c' not found."
/*
** Parameters:
**	%0 - Name of user to be dropped.
** Explanation:
**	The specified table is an INGRES user table.
** System Status:
**	Program continues and current DROP USER statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can drop a user.
*/
E_US18B8_6328_LOC_EXISTS:E_GE75F8_DEF_RESOURCE
"CREATE LOCATION: The location '%0c' already exists."
/*
** Parameters:
**	%0 - Name of location to be added.
** Explanation:
**	The specified table is an INGRES location table and duplicates
**	are not allowed.
** System Status:
**	Program continues and current CREATE LOCATION statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can add a location.
*/
E_US18B9_6329_LOC_ABSENT:E_GE75B2_NOT_FOUND
"ALTER/DROP LOCATION: The location '%0c' not found."
/*
** Parameters:
**	%0 - Name of location to be dropped.
** Explanation:
**	The specified table is an INGRES location table and duplicates
**	are not allowed.
** System Status:
**	Program continues and current DROP LOCATION statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can drop a location.
*/
E_US18BA_6330_DB_UNKNOWN:E_GE75B2_NOT_FOUND
"DBACCESS: The database '%0c' not found."
/*
** Parameters:
**	%0 - Name of database for granting dbaccess.
** Explanation:
**	The specified table is an INGRES dbaccess table.
** System Status:
**	Program continues and current DBACCESS statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can grant access to a database.
*/
E_US18BB_6331_USR_UNKNOWN:E_GE75B2_NOT_FOUND
"DBACCESS: The user '%0c' not found."
/*
** Parameters:
**	%0 - Name of user for granting dbaccess.
** Explanation:
**	The specified table is an INGRES dbaccess table.
** System Status:
**	Program continues and current DBACCESS statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can grant access to a database.
*/
E_US18BC_6332_DBACC_EXISTS:E_GE75F8_DEF_RESOURCE
"DBACCESS: Database access already granted to user '%0c'."
/*
** Parameters:
**	%0 - Name of user to be given database access.
** Explanation:
**	The specified table is an INGRES dbaccess table and duplicates
**	are not allowed.
** System Status:
**	Program continues and current DBACCESS statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can grant access to a database.
*/
E_US18BD_6333_DBACC_ABSENT:E_GE75B2_NOT_FOUND
"DBACCESS: User'%0c' does not have access to database."
/*
** Parameters:
**	%0 - Name of user to be having database access.
** Explanation:
**	The specified table is an INGRES dbaccess table and duplicates
**	are not allowed.
** System Status:
**	Program continues and current DBACCESS statement is aborted.
** User Action:
**	Only a user with SECURITY privilege can revoke access to a database.
*/
E_US18BE_6334:NO_GE
"An error occurred while processing the iiuser catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iiuser catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Super users are allowed to continue.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US18BF_6335:NO_GE
"An error occurred while processing the iilocation catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iilocation catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Super users are allowed to continue.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US18C0_6336:NO_GE
"An error occurred while processing the iidbaccess catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iidbaccess catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Super users are allowed to continue.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US18C1_6337:NO_GE
"An error occurred while processing the iisecuritystate catalog.\n
Please contact your INGRES system manager."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if SCF or QEF have trouble opening,
**	reading, or writing the iisecuritystate catalog.  It will
**	appear if that catalog has been clobbered, or if it doesn't
**	exist.
**
** System Status:
**	If it occurs during session initialization, the session
**	is disconnected.  Super users are allowed to continue.
**
** User Action:
**	Call the INGRES system administrator.
**
*/
E_US18C2_6338:E_GE80E8_LOGICAL_ERROR
"ORDER BY column list is either illegal or contradictory."
/*%
** Parameters: (none)
**
** Explanation:
**	This message is issued if QEF cannot sort resulting tuples by
**	specified "order by" list because column is either specified more than
**	once, or has both descending and ascending attributes associated in
**	the ordering.
**
** System Status:
**	Current statement is aborted.
**
** User Action:
**	Please check that "order by" list specifies each column only once.
**
*/
E_US18C4_6340:E_GE7918_SYNTAX_ERROR
"GRANT/REVOKE:  You must not specify a list of objects\n
when granting or revoking privileges ON INSTALLATION.\n
Reenter this statement without objects specified."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the GRANT or REVOKE statement
**	ON INSTALLATION, but a list of objects was specified.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Check to ensure no objects are specified when
**	granting or revoking privileges on installations.
**
*/
E_US18C5_6341:E_GE7918_SYNTAX_ERROR
"GRANT/REVOKE:  You must specify a list of objects\n
when granting or revoking privileges on specific\n
tables, procedures, or databases.\n
Reenter this statement with objects specified."
/*%
** Parameters: (none)
**
** Explanation:
**	The user attempted to issue the GRANT or REVOKE statement,
**	but no object list was specified, and ON INSTALLATION was
**	not specified.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Check to ensure objects are specified when
**	granting or revoking privileges, except when
**	ON INSTALLATION is specified.
**
*/
E_US18C6_6342:E_GE84D0_NO_PRIVILEGE
"You are not authorized to GRANT or REVOKE privileges ON INSTALLATION.\n
Please contact your INGRES System Administrator for further assistance."
/*
** Parameters: (none)
** 
** Explanation:
**	The non-privileged user attempted to grant privileges
**	on installation.  This requires security privileges.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should contact the sysadmin to determine who
**	should handle this request.
**	
*/
E_US18CE_6350:E_GEA0F0_SQL_STMT_INV
"The %0c statement is not allowed in distributed INGRES.\n
Enter this command under non-distributed INGRES"
/*%
** Parameters:
**	%0 - The statement type (i.e. "CREATE USER")
**
** Explanation:
**	The user entered the specified statement
**	under distributed INGRES.  This is not supported.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Issue the command under local INGRES.
**
*/
E_US18CF_6351:E_GE7D00_QUERY_ERROR
"%0c: '%1c' specified multiple times.\n
Please reenter the statement with one instance of this parameter."
/*
** Parameters:
**	%0 - the statement type (i.e. "CREATE/ALTER USER")
**	%1 - the conflicting parameter (i.e. "GROUP")
**	
** Explanation:
**	The user attempted to issue a statement, 
**	but a parameter was specified multiple times.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the parameter
**	specified once.
**	
*/
E_US18D1_6353:E_GE7918_SYNTAX_ERROR
"%0c: Syntax error in the 'WITH' clause.\n
Last token read was '%1c'.\n
Please check the syntax of the 'WITH' clause and reenter the statement."
/*
** Parameters:
**	%0 - the statement type (i.e. "CREATE/ALTER LOCATION")
**	%1 - the last token read (i.e. "AREA")
**	
** Explanation:
**	The user attempted to issue a statement, 
**	but the WITH clause was not well formed.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should correct the WITH clause and reenter
**	the command.
**	
*/
E_US18D2_6354:E_GE84D0_NO_PRIVILEGE
"%0c: only the DBA can use the '%0c' statement."
/*
** Parameters:
**	%0 - the attempted statement type (i.e. CREATE SECURITY_ALARM).
** 
** Explanation:
**	The non-dba user attempted to issue a statement that
**	can only be issued by the dba.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should contact the dba to perform this operation.
**	
*/
E_US18D3_6355_NOT_AUTH:E_GE84D0_NO_PRIVILEGE
"You do not have privileges to issue '%0c' statements.\n
Please contact your INGRES System Administrator for further assistance."
/*
** Parameters:
**	%0 - the attempted statement type (i.e. CREATE USER).
** 
** Explanation:
**	The non-privileged user attempted to issue a statement that
**	requires special privileges.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should contact the sysadmin to determine who
**	should handle this request.
**	
*/
E_US18D4_6356_NOT_DBDB:NO_GE
"You must be connected to the 'iidbdb' database\n
to issue '%0c' statements.\n
Ensure your session is connected to the iidbdb database."
/*
** Parameters:
**	%0 - the attempted statement type (i.e. CREATE USER).
** 
** Explanation:
**	The user attempted to issue a statement requiring
**	connection to the iidbdb database while connected
**	to a non-iidbdb database.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reissue the statement while connected
**	to the iidbdb database.
**	
*/
E_US18D5_6357_FORM_NOT_AUTH:E_GE84D0_NO_PRIVILEGE
"You do not have privileges to issue this form of '%0c' statements.\n
Please contact your INGRES System Administrator for further assistance."
/*
** Parameters:
**	%0 - the attempted statement type (i.e. ALTER USER).
** 
** Explanation:
**	The non-privileged user attempted to issue a special form of
**	a statement that requires special privileges.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should contact the sysadmin to determine who
**	should handle this request.
**	
*/
E_US18D6_6358_ALTER_BAD_PASS:E_GE84D0_NO_PRIVILEGE
"ALTER USER: Either you specified a bad value for OLDPASSWORD,\n
or the OLDPASSWORD clause was omitted altogether.\n
Please respecify the statement with the correct value for OLDPASSWORD."
/*
** Parameters: (none)
** 
** Explanation:
**	The non-privileged user attempted to change passwords with the
**	ALTER USER statement, but the value specified for OLDPASSWORD
**	did not match the user's old password, or the OLDPASSWORD clause
**	was omitted altogether.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the statement with the correct value
**	for OLDPASSWORD specified on the ALTER USER statement.
**	
*/
E_US18D8_6360:E_GE7D00_QUERY_ERROR
"'AREA' is a required parameter on the CREATE LOCATION statement.\n
Please reenter this statement with the 'AREA' parameter specified."
/*
** Parameters: (none)
**	
** Explanation:
**	The user omitted the AREA parameter on the CREATE LOCATION
**	statement.  This is a required parameter.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the statement with the AREA parameter
**	specified.
**	
*/
E_US18D9_6361:E_GE7D00_QUERY_ERROR
"'AREA' cannot be specified on the ALTER LOCATION statement.\n
Please reenter this statement with the 'AREA' parameter omitted."
/*
** Parameters: (none)
**	
** Explanation:
**	The user specified the AREA parameter on the ALTER LOCATION
**	statement.  This parameter cannot be specified.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the statement with the AREA parameter
**	omitted.
**	
*/
E_US18DA_6362:E_GE7918_SYNTAX_ERROR
"CREATE/ALTER LOCATION: Unrecognized usage specified: '%0c'\n
Please check the spelling of this usage and reenter the command."
/*
** Parameters:
**	%0 - the invalid usage
**	
** Explanation:
**	The user attempted to CREATE or ALTER a location,
**	but one of the specified usages was invalid.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the correct
**	usage specified.
**	
*/
E_US18DB_6363:E_GE7D00_QUERY_ERROR
"CREATE/ALTER LOCATION: Conflicting usages specified.\n
Usages 'ALL' and 'NONE' cannot be specified with any other usage.\n
Please eliminate conflicting usages and reenter the statement."
/*
** Parameters: (none)
**	
** Explanation:
**	The user attempted to CREATE or ALTER a location,
**	and specified ALL or NONE in the usage clause with
**	other usages.  When ALL or NONE is specified, no
**	other usages may be specified.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command without conflicting
**	usages specified.
**	
*/
E_US18DC_LOC_IN_USE:NO_GE
"Unable to drop location `%0c'.\n
This location is currently used by one or more databases,\n
as reflected by the iidatabase and/or iiextend catalogs.\n
You must destroy all databases that occupy a location\n
before that location can be dropped."
/*
** Parameters
**	%0 - the location being dropped.
**	
** Explanation:
**	The user attempted to drop a location,
**	but the location is referenced by one or more
**	iidatabase and/or iiextend tuples.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user must destroy all databases that reference the
**	location before it can be dropped.
**	
*/
E_US18E2_6370:E_GE7918_SYNTAX_ERROR
"ENABLE/DISABLE AUDIT: Either audit type '%0c' is invalid,\n
or this audit type was specified incorrectly.\n
Please check the specification of this audit type and reenter the command."
/*
** Parameters:
**	%0 - the invalid audit type
**	
** Explanation:
**	The user attempted to ENABLE or DISABLE SECURITY_AUDIT,
**	but the specified audit types was invalid, or improperly
**	specified.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the correct
**	audit type specified.
**	
*/
E_US18E3_6371:E_GE7918_SYNTAX_ERROR
"ENABLE/DISABLE AUDIT:  A value must be provided when 'LEVEL' is specified.\n
Please reenter the statement with a value specified for 'LEVEL'."
/*
** Parameters: (none)
**	
** Explanation:
**	The user attempted to ENABLE or DISABLE SECURITY_AUDIT,
**	and specified LEVEL, but omitted the level parameter.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the level
**	parameter specified.
**	
*/
E_US18E4_6372:E_GE7D00_QUERY_ERROR
"ENABLE/DISABLE AUDIT:  The specified level parameter,\n
'%0c', is invalid.\n
Please check that the level parameter was properly specified\n
and reenter the statement."
/*
** Parameters:
**	%0 - the invalid level parameter
**	
** Explanation:
**	The user attempted to ENABLE or DISABLE SECURITY_AUDIT LEVEL,
**	but the specified level parameter was invalid.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the correct
**	level parameter specified.
**	
*/
E_US18EC_6380:E_GE7D00_QUERY_ERROR
"DROP SECURITY_ALARM:  Security alarms can be dropped only from tables.\n
Please ensure the object from which you are attempting\n
to drop a security alarm is a table."
/*
** Parameters: (none)
**	
** Explanation:
**	The user attempted to drop a security alarm from a 
**	non-table object.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should ensure all specified objects are tables
**	and reenter the command.
**	
*/
E_US18ED_6381:E_GE7D00_QUERY_ERROR
"CREATE SECURITY_ALARM:  Security alarms can be created only on tables.\n
Please ensure the object upon which you are attempting\n
to create a security alarm is a table."
/*
** Parameters: (none)
**	
** Explanation:
**	The user attempted to create a security alarm on a 
**	non-table object.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should ensure all specified objects are tables
**	and reenter the command.
**	
*/
E_US18EE_6382:E_GE7918_SYNTAX_ERROR
"CREATE SECURITY_ALARM:  Invalid privileges specified.\n
You may only specify SELECT, UPDATE, INSERT and/or DELETE\n
on the WHEN clause of CREATE SECURITY_ALARM.\n
Please reenter the statement will valid privileges specified."
/*
** Parameters: (none)
**	
** Explanation:
**	The user attempted to create a security alarm
**	with invalid privileges specified on the WHEN clause.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should only specify SELECT, UPDATE, INSERT
**	and/or DELETE on the WHEN clause.
**	
*/
E_US18EF_6383:E_GE7918_SYNTAX_ERROR
"CREATE SECURITY_ALARM:  Column list specified.\n
You cannot specify a column list in the WHEN clause\n
of the CREATE SECURITY_ALARM statement.\n
Please reenter the statement, but omit the column list."
/*
** Parameters: (none)
**	
** Explanation:
**	The user attempted to create a security alarm
**	with invalid privileges specified on the WHEN clause.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should only specify SELECT, UPDATE, INSERT
**	and/or DELETE on the WHEN clause.
**	
*/
E_US18F0_6384:E_GE7918_SYNTAX_ERROR
"CREATE SECURITY_ALARM: Invalid condition specified.\n
'%0c', is not a valid condition.\n
Only SUCCESS and/or FAILURE can be specified on the IF clause.\n
Please reenter the statement with correct conditions specified."
/*
** Parameters:
**	%0 - the invalid condition
**	
** Explanation:
**	The user attempted to create a security alarm,
**	but the specified condition was invalid.
**	
** System Status:
**	The statement is aborted.
**	
** User Action:
**	The user should reenter the command with the correct
**	condition specified.
**	
*/
E_US18F6_6390:E_GE75BC_UNKNOWN_OBJECT
"CREATE/ALTER USER: Group '%0c' does not exist.\n
You cannot specify a non-existing group as a user's default group.\n
Ensure you entered the correct group identifier."
/*%
** Parameters:
**	%0	Specified group identifier 
**
** Explanation:
**	The user attempted to create or alter a user,
**	but specified a default group identifier that
**	does not exist.
**
** System Status:
**	For CREATE USER, the statement is not aborted;
**	the user does not have a default group.
**	For ALTER USER the statement is not aborted;
**	the user's default group is not changed.
**
** User Action:
**	Make sure the group identifier was properly entered.
**
*/
E_US18F7_6391:E_GE0032_WARNING
"CREATE/ALTER USER: User '%1c' added to group '%0c'\n
The user was not a member of the specified group,\n
but was added to the group due to the CREATE/ALTER USER request."
/*%
** Parameters:
**	%0	Specified group  identifier 
**	%1	Specified member identifier 
**
** Explanation:
**	This is a warning message to let the user know
**	that the user was added to the specified group.
**
** System Status:
**	The statement is not aborted.
**	This is not an error.
**
** User Action:
**	None.
**
*/
E_US18F8_6392:E_GE0032_WARNING
"User '%0c' has been removed from all groups.\n
A user is implicitly removed from all groups for which\n
that user was a member when the user is dropped."
/*%
** Parameters:
**	%0	Specified user identifier 
**
** Explanation:
**	When dropping a user, the user is removed from all
**	groups for which that user was a member.
**	This is an informational message only.
**
** System Status:
**	The statement is continues.
**
** User Action:
**	None.
**
*/
E_US18F9_6393:E_GE75F8_DEF_RESOURCE
"Unable to create user '%0c'.\n
A role with this name already exists in this installation.\n
User names must be unique within groups, roles, and users.\n
Please specify a different user name."
/*%
** Parameters:
**	%0	Specified user identifier 
**
** Explanation:
**	The user attempted to create a user that has
**	the same name as an existing role.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Make sure the user identifier was properly specified.
**	Select a different user identifier.
**
*/
E_US18FA_6394:E_GE75F8_DEF_RESOURCE
"Unable to create user '%0c'.\n
A group with this name already exists in this installation.\n
User names must be unique within groups, roles, and users.\n
Please specify a different user name."
/*%
** Parameters:
**	%0	Specified user identifier 
**
** Explanation:
**	The user attempted to create a user that has
**	the same name as an existing group.
**
** System Status:
**	The statement is aborted.
**
** User Action:
**	Make sure the user identifier was properly specified.
**	Select a different user identifier.
**
*/
E_US18FF_6399:E_GE75BC_UNKNOWN_OBJECT
"User authorization check failed.\n
Either your user identifier was not known to this INGRES installation,\n
a required user password was omitted,\n
or the specified user password was invalid.\n
Contact your INGRES system manager for further assistance."
/*%
** Parameters:
**      (none)
**
** Explanation:
**      User authorization identifier check failed due to one of the following:
**          o The real user id was not known to the installation
**          o A password was defined for the real user id, but was not
**            specified, or the specified password was not correct.
**
** System Status:
**      The session is terminated.
**
** User Action:
**      Contact the INGRES security officer to determine if the user id
**      is valid, or if a password is required.  Note that there is no
**      way to determine what a user's password is, if one is defined.
**
*/
E_US1900_6400_UPD_LOGKEY:E_GE7D00_QUERY_ERROR
"Error occurred on line %0d, Table '%1c' owned by '%2c', column '%3c'.
The UPDATE or INSERT statement illegally specified a value for a 
system_maintained column."
/*%
** Parameters: (none)
**	%0 - Line number (in a terminal monitor script).
**	%1 - Name of the table.
**	%2 - Name of owner of the table.
**	%3 - Name of the column in the table which caused the error.
**
** Explanation:
**	This message is issued by PSF when a system_maintained column
**	is specified in either an INSERT or UPDATE statement.  The
**	value of a system_maintained column cannot be specified by the
**	user; the value is assigned internally by the dbms.
**
** System Status:
**	Current statement is aborted.
**
** User Action:
**	Reissue the INSERT or UPDATE without specifying a value for the
**	system_maintained column.
*/
E_US1901_6401_BAD_SYSMNT:E_GE7D00_QUERY_ERROR
"CREATE TABLE: error on attribute '%1c'.\n  The 'with system_maintained' 
qualifier is not supported on the datatype specified for this attribute
(%0c)."
/*%
** Parameters: (none)
**	%0 - Name of the type of attribute being created.
**	%1 - Name of the attribute being created.
**
** Explanation:
**	This message is issued by PSF when a system_maintained column
**	is specified in a CREATE TABLE statement on a datatype which 
**	doesn't support 'with system_maintained'.  Currently only the
**	'table_key' and 'object_key' datatypes support the 
**	'with system_maintained' attribute qualifier.
**
** System Status:
**	Current statement is aborted.
**
** User Action:
**	Reissue the CREATE TABLE statement after correcting the offending
**	attribute declaration.
*/
E_US1F42_8002:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: Bad gateway storage structure '%0c'.\n
The valid storage structure names are 'keyed', 'fullkey', 'sortkeyed' or 'none'.
Not all structures are valid for all gateways.  Consult the User's Manual for
your gateway for information on which structures are valid and how to use them."
E_US1F43_8003:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: invalid option '%0c'. Try NOT NULL WITH DEFAULT."
E_US1F44_8004:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: either invalid attribute qualifier '%0c', wrong order, 
or wrong combination of qualifiers.  Try
'[[not null [with | not default]] | with null]'."
/*%
** Parameters:
**	%0 - The attribute qualifer which caused the error.
**
** Explanation:
**	The accepted syntax diagram for the column qualifier clause is:
**      '[[not null [with | not default]] | with null] 
**					[not | with system_maintained]'."
**	with one exception. It is not allowed to specify either "not default" or
**	"with null" in conjunction with the "with system_maintained" 
**	qualifier.
**		
**	The order of these qualifiers is significant, and must appear as
**	described.  The following is a complete list of accepted attribute
**	qualifiers (a few options although syntactically correct per the 
**	above diagramas described by the syntax diagram, are disallowed due
**	to the "with system_maintained" restrictions):
**		with null
**		not null with default
**		not null not default
**
** System Status:
**	Current statement is aborted.
**
** User Action:
**	Reissue the REGISTER statement after correcting the problem with
**	the attribute qualifier clause.
*/
E_US1F45_8005:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: Invalid option '%0c' in 'with' clause.  Either the option
itself is invalid, or it has been incorrectly qualified."
/*%
**  Parameters:
**	%0  -	The option for which the error occurred.
**
**  Explanation:
**	Either an incorrect option has been specified, or it has been
**	qualified incorrectly.
**
**	Valid options are:
**
**	dbms = rms	    only valid dbms qualifier in 6.3/02 (vax.vms)
**	rows = nnnn	    where nnnn is an integer indicating # of rows
**	path = 'string'	    where string indicates a path value
**	[no]duplicates	    no qualifier
**	[no]journaling	    no qualifier
**	[no]recovery	    no qualifier
**	[no]update	    no qualifier
**
**  System Status:
**	Current statement is aborted.
**
**  User Action:
**	Reissue the REGISTER statement after correcting the problem.
**
*/
E_US1F47_8007:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: invalid option '%0c'. Legal single-word options for the 'with'
clause are:  [no]duplicates, [no]journaling, [no]update, [no]recovery."
E_US1F48_8008:E_GE8CA0_SYSTEM_LIMIT
"REGISTER TABLE: excessive table width on column '%0c'."
E_US1F49_8009:E_GE7D00_QUERY_ERROR
"REGISTER TABLE: '%0c' is a system table."
E_US1F4A_8010:E_GE75F8_DEF_RESOURCE
"REGISTER TABLE: Duplicate object name '%0c'."
E_US1F4B_8011:E_GE8CA0_SYSTEM_LIMIT
"REGISTER TABLE: the number of columns must not exceed 300."
E_US1F4C_8012:E_GE797C_INVALID_IDENT
"REGISTER TABLE: invalid column name '%0c'."
E_US1F4D_8013:E_GE7D00_QUERY_ERROR
"REGISTER TABLE/INDEX: duplicate column name '%0c'."
E_US1F4E_8014:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: invalid column format '%0c' on column '%1c'."
E_US206E_8302:E_GE759E_COLUMN_UNKNOWN
"REGISTER INDEX: Invalid column '%0c'."
E_US206F_8303:E_GE7594_TABLE_NOT_FOUND
"REGISTER INDEX: Table '%0c' not owned by you."
E_US2070_8304:E_GE7D00_QUERY_ERROR
"REGISTER INDEX: Table '%0c' is already an index."
E_US2072_8306:E_GE7D00_QUERY_ERROR
"REGISTER INDEX: '%0c' is a view and an index can't be built on it."
E_US2073_8307:E_GE75F8_DEF_RESOURCE
"REGISTER INDEX: duplicate column '%0c'."
E_US2077_8311:E_GEA0F0_SQL_STMT_INV
"The REGISTER INDEX statement is not allowed in distributed INGRES."
E_US2079_8313:E_GE7918_SYNTAX_ERROR
"REGISTER INDEX: Bad storage structure '%0c'."
/*%
**  An invalid storage structure was specified when REGISTERing a gateway
**  secondary index.  Current valid storage structures for a gateway secondary
**  index are:
**
**	keyed
**	fullkey
**	sortkeyed
**
**  Not all of the above storage structures are valid for every gateway.  See
**  your gateway's User's Guide for a list of valid storage structures.
**
**  System Status:
**	Current statement is aborted.
**
**  User Action:
**	Reissue the REGISTER statement after correcting the storage structure.
**
*/
E_US207A_8314:E_GE7918_SYNTAX_ERROR
"REGISTER INDEX: '%0c' is either an invalid option or the syntax of the WITH
subclause is invalid."
E_US2087_8327:E_GE7918_SYNTAX_ERROR
"REGISTER INDEX: Too many options in the WITH clause."
E_US2088_8328:E_GE8CA0_SYSTEM_LIMIT
"REGISTER INDEX: Too many columns in key of index."
E_US213E_8510:E_GE7918_SYNTAX_ERROR
"Column name in list incorrectly modified (word seen is '%0c').  The format of
a column list is '(col_name [ASC | DESC] {,col_name [ASC | DESC]})."
E_US215C_8540:E_GE7D00_QUERY_ERROR
"REGISTER INDEX: %0c: invalid or no storage structure specified for gateway
secondary index."
/*%
**  An invalid storage structure, or no storage structure, was specified when
**  REGISTERing a gateway secondary index.  Current valid storage structures
**  for a gateway secondary index are:
**
**	keyed
**	fullkey
**	sortkeyed
**
**  Not all of the above storage structures are valid for every gateway.  See
**  your gateway's User's Guide for a list of valid storage structures.
**
**  System Status:
**	Current statement is aborted.
**
**  User Action:
**	Reissue the REGISTER statement after correcting the storage structure.
**
*/
E_US222E_8750:E_GE7D00_QUERY_ERROR
"REMOVE TABLE: object '%0c' is not a base table."
E_US2230_8752:E_GE7D00_QUERY_ERROR
"REMOVE INDEX: object '%0c' is not an index."
E_US2232_8754:E_GE7D00_QUERY_ERROR
"REMOVE: Too many objects specified."
E_US239A_9114:E_GEA0F0_SQL_STMT_INV
"The REGISTER command is not allowed in distributed INGRES."
E_US245B_9307:E_GE7D00_QUERY_ERROR
"REGISTER TABLE/INDEX: duplicate column name '%0c' specified in 'key' clause."
E_US245C_9308:E_GE8CA0_SYSTEM_LIMIT
"REGISTER TABLE: name for gateway object in 'from' clause is too long."
E_US245D_9309:E_GE7D00_QUERY_ERROR
"REGISTER TABLE: duplicate '%0c' entry supplied in 'with' clause."
E_US245E_9310:E_GE797C_INVALID_IDENT
"REGISTER TABLE: invalid dbms type '%0c' supplied in 'with' clause."
/*%
**  An invalid dbms type was specified in the 'with dbms=' clause of the
**  REGISTER TABLE statement.  Current valid dbms types are:
**
**	rms
**
**  System Status:
**	Current statement is aborted.
**
**  User Action:
**	Reissue the REGISTER statement after correcting the dbms type.
**
*/
E_US245F_9311:E_GE7D00_QUERY_ERROR
"REGISTER TABLE: invalid option '%0c' specified in 'with' clause."
/*%
**  An invalid option was specified in the 'with' clause.  In this context
**  (NAME = NAME), valid options are:
**
**	structure = keyed | fullkey | sortkeyed | none
**	dbms = rms
**
**  System Status:
**	Current statement is aborted.
**
**  User Action:
**	Reissue the REGISTER statement with one of the valid options 'structure'
**	or 'dbms'.
**
*/
E_US2460_9312:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: Bad gateway storage structure '%0c' specified with UNIQUE
qualifier.  Valid storage structures which can be qualified with UNIQUE are:
'keyed', 'fullkey', or 'sortkeyed'.  Not all structures are valid for all
gateways.  Consult the User's Manual for your gateway for information on which
structures are valid and how to use them."
E_US2461_9313:E_GE7918_SYNTAX_ERROR
"REGISTER TABLE: Bad option '%0c' specified with UNIQUE qualifier.  Only the
'structure' option include the UNIQUE qualifier."
E_US2462_9314:E_GE7D00_QUERY_ERROR
"REGISTER TABLE: duplicate or conflicting '%0c' entry in 'with' clause."
/*%
**  A duplicate or conflicting option was specified in the 'with' clause.
**  For example, 'recovery' was specified.  Or both 'recovery' and 'norecovery'
**  were specified.
**
**  System Status:
**	Current statement is aborted.
**
**  User Action:
**	Reissue the REGISTER statement with the desired option specified only
**	once.
**
*/
E_US2463_9315:E_GE7D00_QUERY_ERROR
"REGISTER TABLE:  No 'dbms' clause specified.  'dbms' is a required parameter
in the 'register table' command."
E_US2464_9316:E_GE7D00_QUERY_ERROR
"REGISTER TABLE:  A 'key' clause has been specified, but no column names were
provided."
/*%
**  The 'with' clause included a 'key=' specifier, but no columns were listed.
**
**  System Status:
**	Current statement is aborted.
**
**  User Action:
**	Reissue the REGISTER statement, either leaving out the 'key=' specifier
**	or listing one or more columns as key columns.  NOTE that there may be
**	limitations on how many and/or what types of columns may be in a key
**	for a gateway table.  These limitations may be different from those in
**	a native INGRES database.  Consult the User's Guide for your Gateway to
**	find out whether such limitations apply and if so, what they are.
**
*/
E_US2465_9317:E_GE7D00_QUERY_ERROR
"REGISTER TABLE:  one or more key columns has been specified for the table, but
the structure has been specified as 'none' or it has not been specified
(default storage structure is 'none')."
E_US2466_9318:E_GE7D00_QUERY_ERROR
"REGISTER TABLE:  table structure other than 'none' has been specified, but no
keys have been specified."
E_US2468_9320:E_GE8CA0_SYSTEM_LIMIT
"REGISTER TABLE: path parameter is too long."
