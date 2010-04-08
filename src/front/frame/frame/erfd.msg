/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	erfd.msg -	Message file for class FD
**
** Description:
**	This file defines the messages for class FD.
**
** History:
**	15-aug-1987 (dave)
**		Created.
**	10/12/87 (dkh) - Deleted reference to current field in certain
**			 error messages.
**	10/14/87 (dkh) - Moved some error messages to FI class 181.
**	10/16/87 (dkh) - Cleanup up message file.
**	25-nov-87 (bruceb)
**		Added a scrolling field error message.
**	12/07/87 (dkh) - Added message id E_FD0025_Syntax_error.
**	12/10/87 (dkh) - Fixed jup bug 1478 & 1489.
**	12/12/87 (dkh) - Changed text of error E_FD0001_Could_not_format.
**	13-apr-88 (bruceb)
**		Changed E_FD0001: "led to error" => "led to the error"
**	03-feb-89 (bruceb)
**		Added E_FD0028 for dynamic format changes.
**	14-feb-89 (bruceb)
**		Added S_FD0029, 2A, 2B as datatypes for dynamic format
**		change error message.
**	10-jul-89 (Mike S)
**		Added E_FD002C-30 as errors in IIFDffFormatField (in dyn.c).
**	02-nov-89 (marian)
**		Replace "Technical Support Representative" with "Site
**		Technical Contact".
**	21-nov-89 (bruceb)
**		Removed E_FD002C-30 since dyn.c now resides in uf directory.
**	02-jul-90 (bruceb)	Fixed bug 31054.
**		FD001A no longer mentions 'help'.
**	03-jul-90 (bruceb)	Fix for 31018.
**		New messages added:  F_FD0002-4.
**	20-sep-90 (bruceb)	Fix for 32823 +.
**		Changed references to QBF manual to "Using INGRES Through
**		Forms and Menus".
**/

# define	_FD_CLASS	55
/*
**	datafmt.c
*/
E_FD0001_Could_not_format
"Could not format field \"%0c\".\n  An internal error prevented
the field value from being formated.  Please write down circumstances
that led to the error and report it to your Site Technical Contact.
Either try the operation again or restart your application."

/*%
**  Description:
**	An error occurred while trying to format a single line
**	simple field.
**
**  System Status:
**	System will ignore field that error occurred in and continue processing.
**
**  User Action:
**	User needs to retry operation or restart application.
*/

/*
**	fdfrcrfl.c
*/
E_FD0004_FormRead	"Error reading form '%0c' from index file.\n
The Form Index file module did not read in a form correctly.  Please report
this internal problem to your technical representative."
/*%
** Explanation:
**	The Form Index file failed to read in a form correctly from the Forms
**	Index file.  This is an internal programming problem.
**
** System Status:
**	The program will abort.
**
** User Action:
**	Report this internal problem to your technical support representative.
*/

E_FD0005_FormLoadErr	"Error loading form '%0c' from index file.\n
The form '%0c' could not be loaded into the Forms System.  This is an
installation problem.  Please contact your INGRES system administrator
and inform them that a problem exists with the installation of the
Forms Index file."
/*%
** Explanation:
**	An error was detected when trying to load the named form read from the
**	Form Index file.  Internal checks are made to see if the data structure
**	is internally consistent, and one of them failed.  This usually is
**	because the data for the form was either corrupted or incomplete when
**	it was either written to or read from the Form Index file.  This shows
**	that the installation of the Forms Index file is incorrect.
**
** System Status:
**	The program will abort.
**
** User Action:
**	Check with your INGRES system administrator to determine if the
**	installation is correct.  If the installation is correct, report
**	this problem to your technical support representative.
*/

/*
**	frcreate.qc
*/
E_FD0008_CFdecode_Error	"FDwfrcreate: Error in value returned by CFdecode\n\r"
E_FD0009_DSload_Error	"FDwfrcreate: Error status returned by DSload\n\r"

/*
**	intrhdlr.c
*/
S_FD000E_Exiting	"Exiting . . ."
S_FD000F_INTERRUPT	"\nDue to INTERRUPT.\n"
S_FD0010_KILLED	"\nDue to being KILLED.\n"
S_FD0011_QUIT_signal	"\nDue to QUIT signal.\n"
S_FD0012_out_of_input	"\nDue to out of input.\n"
S_FD0013_Access_Viola	"\nDue to Access Violation.\n"
/*
**	newtbl.c
*/
E_FD0014_no_columns	"TBL INIT: no columns in table field%s\n" /* DIAG */
E_FD0015_no_rows	"TBL INIT: no rows in table field %s\n" /* DIAG */
/*
**	rangechk.c
*/

E_FD0017_No_operand_fou
"No operand found inside parentheses.\n  At least one operand
must be within an opening and closing parenthesis.  Please check
that the qualification string that you have entered does not include
\"()\" or \"(())\".  Please refer to Using INGRES Through Forms
and Menus for a detailed discussion on the proper syntax."

/*%
**  Explanation:
**	No operand was found inside a matched pair of left and
**	right parenthesis.  User probably entered "()" or "(())".
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD0018_unmatched_righ
"Extra right parenthesis found.\n  The qualification string contains
a right parenthesis for which there is no matching left parenthesis.
Please check the entered qualification string for the same number of
left and right parentheses and that they are matched properly.
Please refer to Using INGRES Through Forms and Menus for a detailed
discussion on the proper syntax."

/*%
**  Explanation:
**	Unmatched right parenthesis was found.  User must have entered
**	one too many.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD0019_excess_charact
"Excess characters found at end of string.\n  Please check the
end of the qualification string for correct syntax.  Using INGRES Through
Forms and Menus contains the complete and detailed discussion on the
proper syntax."

/*%
**  Explanation:
**	Extra characters that do not conform to the proper syntax
**	was found at the end of the qualification string.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD001A_bad_datatype
"Invalid operand found.\n  This may be due to having entered invalid
characters (based on the field's datatype), the value entered
was too big or small for the field or the operand does not match
the format for the field.  An example of invalid characters
is alphabetic characters in an integer field.  Also, entering '650000'
into a two byte integer field will not work.
Refer to the \"Using INGRES Through Forms and Menus\" manual for
information on datatypes."

/*%
**  Explanation:
**	Could not convert input into a db_data_value.  String
**	either contains characters not allowed for the field's
**	datatype or some sort of overflow/underflow occurred.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD001B_unable_to_crea
"Internal processing error has occurred.\n  An internal
error prevents the system default value from being obtained
for the current field.  Please write down all relevant
circumstances that led to this error and report it to
your Site Technical Contact.  Please try again
and restart the program."

/*%
**  Explanation:
**	Internal error occurred when adc_getempty was called.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User should try again and restart the program after writing down
**	what led to the error and report it to their Site Technical
**	Contact.
*/

E_FD001C_No_closing_par
"No closing parenthesis found.\n  The qualification string
contains too many left parentheses.  Please check that you
have entered the same number of left and right parentheses
and that they are matched.  Using INGRES Through Forms and Menus
contains a complete and detailed discussion on the proper syntax."

/*%
**  Explanation:
**	There are too many left parentheses in the qualification
**	string.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD001D_excess_charact
"Excess characters found at end of string.\n  Please check the
end of the qualification string for correct syntax.  Using INGRES Through
Forms and Menus contains the complete and detailed discussion on the
proper syntax."

/*%
**  Explanation:
**	Extra characters that do not conform to the proper syntax
**	was found at the end of the qualification string.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD001E_unmatched_righ
"Extra right parenthesis found.\n  The qualification string contains
a right parenthesis for which there is no matching left parenthesis.
Please check the entered qualification string for the same number of
left and right parentheses and that they are matched properly.
Please refer to Using INGRES Through Forms and Menus for a detailed
discussion on the proper syntax."

/*%
**  Explanation:
**	Unmatched right parenthesis was found.  User must have entered
**	one too many.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD001F_unrecognized
"Internal error has occurred - unknown state.\n  An internal error has occurred
while parsing a qualification string.  Please write down relevant
information leading to this error and report it to your Site Technical
Contact.  You may try again or restart the program."

/*%
**  Explanation:
**	Parser is in an undefined state.  Something is wrong
**	internally.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User should first write down relevant information leading to
**	this error and report it to their Site Technical
**	Contact.  Then the user can either
**	try again or restart the program.
*/

E_FD0020_unterminated
"No closing quote found.\n  The qualification string contains a
beginning quote, but no closing quote was found when the end
of the string was encountered.  Please check the string for matching
quote characters.  Using INGRES Through Forms and Menus contains a
complete and detailed discussion on the proper syntax."

/*%
**  Explanation:
**	No closing quote found.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

E_FD0021_May_only_use
"Invalid operators used with the NULL value.\n  Only the '='
or '!=' operators may be used with the NULL value.  Please check
the string for proper the syntax.  Using INGRES Through Forms and Menus
contains a complete and detailed discussion on the proper syntax."

/*%
**  Explanation:
**	User has specified an operator other than '=' or '!=' with
**	a NULL value.
**
**  System Status:
**	The qualification string for the field will be rejected.
**	Processing continues.
**
**  User Action:
**	User must correct the qualification string.
*/

/*
**	scrmsg.c
*/
F_FD0001_HIT_RETURN	" [PRESS RETURN]"
/*
**	traverse.c
*/
E_FD0022_Cant_find	"FDtbltrv: Can't find frame for table field" /* DIAG */
E_FD0023_Cant_find	"FDrowtrv: Can't find frame for table field" /* DIAG */
/*
**	scrlfmt.c
*/
E_FD0024_bad_scroll_fmt
"Unable to create the internal format for a scrolling field.\n
The original format was '%0c' and the new format is '%1c'.
This represents an internal coding error and should be reported
to your INGRES administrator."
/*%
** Explanation:
**	An error occurred on attempting to create the special format
**	used by the scrolling field code to translate between the
**	long scrolling buffers and the internal values.  This should
**	never occur and is an internal error.
**
** System Status:
**	The program will abort.
**
** User Action:
**	Report the problem to the proper contacts.
*/

/*
**	rangechk.c
*/

E_FD0025_Syntax_error
"Syntax error in the qualification string.\n  The qualification string
entered into the field contains a syntax error.  Usual causes are
incorrect query operators or missing operands.  Please check that
proper query operators have been entered and that there is an operand
for each query operator.  Please refer to Using INGRES Through Forms
and Menus for a detailed discussion on the proper syntax."

/*%
** Explanation:
**	Qualification string contained a syntax error.  Usually
**	because of bad query operators or missing operands.
**
** System Status:
**	The qualification string will simply be rejected.
**	Processing continues.
**
** User Action:
**	User needs to correct the qualification string.
*/

E_FD0026_rel_used_with_pattern
"Pattern matching characters were used with '>' or '<'.\n
A query using the comparison operators '<' or '>' must
not contain any pattern matching characters.  Remove the characters
from the query or use the escape character, '\\', in front of the 
pattern matching characters to disable their special meaning."

E_FD0027_illegal_gateway_query
"Illegal query for a gateway.\n
A query was entered that can not be performed on a gateway.  Query
features not supported on a gateway are: the bracket pattern matching
characters, '[' and ']'; the combination of '*' and '?', when used as
the forms system pattern matching characters, and the SQL pattern
matching characters, '%0c' and '_'; the use of the escape character
when it requires the generation of an escape clause for the DBMS."

E_FD0028_internal_fmt_error
"Call your STC.  Internal fmt error.\n
Unable to determine the characteristics of the '%0c' format
for field (column) %1c.  Please call your Site Technical Contact."

/*%
** Explanation:
**	An internal error has occurred.  Attempts to perform basic
**	fmt routines on the current/new format (for dynamically
**	changing formats) have returned bad status despite other
**	routines having determined that all is OK.
**
** System Status:
**	Attempt to dynamically change a field display format has
**	failed.  No further work done.
**
** User Action:
**	Report this.
*/

S_FD0029_numeric	"numeric"
S_FD002A_character	"character"
S_FD002B_date		"date"

/*%
** Explanation:
**	Datatype names used in an error message.  If an attempt to
**	dynamically change a field display format fails because of
**	datatype mismatch, these strings indicate the nature of the
**	field datatype.
**
** System Status:
**	None.
**
** User Action:
**	Try another format.
*/

F_FD0002_current_field	"current field"
F_FD0003_title_int_name	"field (title: %s, internal name: %s)"
F_FD0004_int_name	"field (internal name: %s)"

/*%
** Explanation:
**	References to field used in field data error messages.  Preference
**	is to us FD0003, then FD0004, and only as a last resort, FD0002.
**	Used as input to erfi.msg messages.
**
** System Status:
**	None.
**
** User Action:
**	None.
*/
