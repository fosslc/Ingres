/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erfv.msg -	Message file for class FV
**
** Description:
**	This file defines the messages for class FV.
**
** History:
**	14-aug-1987 (dave)
**		Created.
**	11/12/87 (dkh) - Fixed typos.
**	12/12/87 (dkh) - Modified messages E_FV0001 to E_FV0005.
**	19-apr-88 (bruceb)
**		Modified message E_FV001B.
**	04-aug-88 (sylviap)
**		Modified message E_FV0007.
**	17-may-89 (sylviap)
**		Modified message E_FV000A.
**	07/20/89 (dkh) - Added support for derived fields.
**	09/18/90 (dkh) - Added E_FV001F to fix bug 32960.
**	01/13/91 (dkh) - New messages to support hard math exceptions
**			 per daveb's suggestions.
**	06/13/92 (dkh) - Added message E_FV0026 for 6.5 support.
**	09/25/92 (dkh) - Added support for owner.table.
**	07/24/93 (dkh) - Added new message that used to be hardcode in the code.
**	16-mar-94 (lan)
**		Fixed bug #58066 - Replaced 'Technical Support Representative'
**		with 'Site Technical Contact'.
**/

# define	_FV_CLASS	60
/*
**	chklist.c
*/
E_FV0001_Internal_error
"Internal error in datatype facility.\n  An internal error occurred
while checking datatype compatibility in the validation string.  This
may indicate that the datatype manipulation facility has been corrupted.
Please write down circumstances that led to the error
and report it to your Site Technical Contact.
If this error occurred while trying to save a form in VIFRED, then you
will be afforded the chance to change the validation string.  For the
case where a form is being initialized for use in an application
the form will not be initialized and one may need to restart the
application."

/*%
**  Description:
**	An error occurred while trying to compare field value against
**	the list of valid values.
**
**  System Status:
**	The field value will be rejected.
**
**  User Action:
**	User can try the operation again or may need to restart the
**	application.
*/

/*
**	evaltree.c
*/
E_FV0002_Bad_unary_function
"Unknown operator found.\n  While trying to check a field value,
the system came across a value that does not correspond to any
known operators.  This may be due to corrected data structures
used by the validation system.
Please write down circumstances that led to the error
and report it to your Site Technical Contact.
One may try the operation again or try restarting the application."

/*%
**  Description:
**	Data structure did not have a known operator for use
**	in validating the field value.
**
**  System Status:
**	Field value will be rejected.
**
**  User Action:
**	User may retry the operation or restart the application.
*/

F_FV0001_ERROR_Validation	"ERROR: Validation check failed"

/*
**  String to use in liue of the form name when putting
**  a derivation formula error message together at vifred save time.
*/
F_FV0002_being_saved	"being saved"

E_FV0003_Inconsistency_in_data
"Inconsistency in data structures found.\n  The information
needed to check the field value was not found in the data
structures used by the validation system.  This may be
due to an error in the datatype manipulation facility.  Please
write down circumstances that led to error and report it to
your Site Technical Contact.
One may try the operation again or try restarting the application."

/*%
**  Description:
**	The instance id for the comparison was not found.
**
**  System Status:
**	Field value will be rejected.
**
**  User Action:
**	User may retry the operation or restart the application.
*/

E_FV0004_Internal_error
"Fatal comparison error has occurred.\n  An internal error occurred
while trying to compare the field value to a valid value.  This may
be due to an error in the datatype manipulation facility.  Please
write down circumstances that led to error and report it to
your Site Technical Contact.
One can retry the operation or restart the application."

/*%
**  Description:
**	An error was returned from the comparison routine.
**
**  System Status:
**	Field value will be rejected.
**
**  User Action:
**	User may retry the operation or restart the application.
*/

E_FV0005_Conversion_error
"Can not obtain field value.\n
An error occurred while trying to obtain the field value to
check.  This may be due to incorrect information in the
data structures used by the validation system.  Please
write down circumstances that led to error and report it to
your Site Technical Contact.
One can retry the operation or restart the application."

/*%
**  Description:
**	Error occurred while coercing field value to new type
**	for comparison.
**
**  System Status:
**	Field value will be rejected.
**
**  User Action:
**	User may retry the operation or restart the application.
*/

/*
**	getlist.qc
*/
E_FV0006_Cant_get_ownercode	"Can't get ownercode."
E_FV0007_table_not_found	"Table '%0c' not found.\n"
E_FV0008_Cant_get_table_attri	"Can't get table attribute."
E_FV0009_No_attribute_found	"No attribute found."
E_FV000A_Table_column	"Table-column (%0c.%1c) not found.\n"
/*
**	lex.c
*/
E_FV000B_Bad_string	"Bad string \"%s\" found"
/*
**	maketree.c
*/
E_FV000C_types_not_compat	"type \"%s\" is not compatible with type \"%s\""
S_FV000D_MAKETREE	"MAKETREE"	/* DIAG */
E_FV000E_parser_error	"parser error"
E_FV000F_Does_not_make_sense_t	"Does not make sense to have constants on both sides of an operator"
E_FV0010_Parser_error	"Parser error"
E_FV0011_Unkown_operator_found	"Unknown operator found"
E_FV0012_Error_getting_operato	"Error getting operator"
E_FV0013_Internal_parser_error	"Internal parser error."
E_FV0014_Coercion_error	"Coercion error"
E_FV0015_Coercion_error	"Coercion error"
E_FV0016_Coercion_error	"Coercion error"
E_FV0017_Failed_to_convert	"Failed to convert constant to appropriate datatype"
E_FV0018_MAKETREE	"MAKETREE"	/* DIAG */
E_FV0019_Parser_error	"Parser error"
/*
**	mkcoltree.c
*/
E_FV001A_Can_only_use	"Can only use '.' with a table field"
E_FV001B_Bad_column_name	"Bad column name '%0c' for table '%1c'"
/*
**	mklist.c
*/
E_FV001C_Unknown_constant	"Unknown constant found"
/*
**	type.c
*/
E_FV001D_parser_error	"parser error"
/*
**	grammar.y
*/
E_FV001E_syntax_error	"syntax error"


/*%
**  Description:
**	User tried to specify a validation for a simple field that
**	depends on a table field column.  This doesn't make sense.
**
**  System Status:
**	Field validation will be rejected.
**
**  User Action:
**	User needs to change the validation.
*/
E_FV001F_simple_to_col
"Validation can not depend on a table field column.\n
It is inappropriate for a simple field validation to depend
on the value of a table field column.  Please correct the
validation syntax."

S_FV0020_FloatHardDivide "hard floating point divide by zero"

S_FV0021_FloatHardOverflow "hard floating point overflow"

S_FV0022_FloatHardUnderflow "hard floating point underflow"

S_FV0023_IntHardDivide "hard integer divide by zero"

S_FV0024_IntHardOverflow "hard integer overflow"

S_FV0025_DuringDerive " exception\nduring derived field calculation."

E_FV0026_BAD_DATATYPE
"Table-column(%0c.%1c) references datatype (%2c) that is not
currently supported by the FORMS SYSTEM.  Please change the
validation to use a column that is defined on a supported datatype.
The FORMS SYSTEM currently supports the following datatypes: integer,
float, c, text, date, money, decimal, char and varchar."

/*%
**  Description:
**	The user specified a column that is defined on an unsupported
**	datatype (e.g., long varchar).
**
**  System Status:
**	Field validation will be rejected.
**
**  User Action:
**	User needs to change the validation.
*/

E_FV0027_BAD_TBL_ID
"'%0c' is not a valid identifier for a table name.  Please check
for invalid characters as well as the correct use of quotes when
specifying delimited identifiers."

/*%
**  Description:
**	The user specified an identifier that is not valid for use
**	as a table name.
**
**  System Status:
**	Field validation will be rejected.
**
**  User Action:
**	User needs to change the validation.
*/

E_FV0028_BAD_OWN_ID
"'%0c' is not a valid identifier for an owner name.  Please check
for invalid characters as well as the correct use of quotes when
specifying delimited identifiers."

/*%
**  Description:
**	The user specified an identifier that is not valid for use
**	as an owner name.
**
**  System Status:
**	Field validation will be rejected.
**
**  User Action:
**	User needs to change the validation.
*/

E_FV0029_BAD_COL_ID
"'%0c' is not a valid identifier for a column name.  Please check
for invalid characters as well as the correct use of quotes when
specifying delimited identifiers."

/*%
**  Description:
**	The user specified an identifier that is not valid for use
**	as a column name.
**
**  System Status:
**	Field validation will be rejected.
**
**  User Action:
**	User needs to change the validation.
*/

E_FV0030_CURSOR_OPEN_ERR
"Unable to retrieve values for table lookup list validation."

/*%
**  Description:
**	The forms system was not able to get the values for a
**	table lookup list validation.  This may be due to various
**	problems including not being able to open a cursor to
**	do the select.
**
**  System Status:
**	The form will not be initialized for use.
**
**  User Action:
**	The user will most likely need to restart to clear up the
**	db connection.
*/
