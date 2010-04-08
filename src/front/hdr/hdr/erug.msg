/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erug.msg -	Front-End Utility UG Message Definitions File.
**
** Description:
**	Defines the messages for the general front-end utilities.
**
** History:
**	Revision 6.0  87/07/28  peter
**	Initial revision.
**	10-feb-92 (essi)
**		Added messages for new utility IIUGvfVerifyFile
**	25-apr-1994 (lan)
**	   Corrected a grammatical error (E_UG00C2_PathTrap).
**	01-sep-94   (donc)
**	   Added E_UG004F_TokenTooLong
**	02-sep-94   (donc)
**	   Added E_UG0050_MissingQuote
**	13-nov-95 (thaju02)
**	   Added RAAT API front end error codes.
**	16-oct-96 (cohmi01)
**	   Additional RAAT API front end error codes.
**/

# define	_UG_CLASS	137


E_UG0000_OK	"UG returns ok"

E_UG0001_Bad_Language	"A bad value of II_LANGUAGE was given.\n
Incorrect value of '%0c' was specified for the name II_LANGUAGE.  English
will be used instead.  Set the INGRES name of II_LANGUAGE to a valid
supported language name."

/*%
** Explanation:
**	This error occurs if a bad II_LANGUAGE is given.  The message
**	system uses a directory with the name of the II_LANGUAGE,
**	and if that directory is not found, this error will occur.
**	Since english is always available, it will be used.
**
** System Status:
**	English will be used as the language for the messages.
**
** User Action:
**	Set a valid value for the II_LANGUAGE name.
*/

E_UG0002_Bad_Msgid	"A bad message id was found.\n
An incorrect message identifier of '%0x' was sent to the error processing
routine.  This is an internal coding error, and should be reported to
your technical support representative for INGRES."

/*%
** Explanation:
**	This error represents a missing error number which was
**	found in an internal call.  This should never occur,
**	so it should be reported as a problem to your INGRES
**	representative.
**
** System Status:
**	Program continues.
**
** User Action:
**	Report to INGRES technical support.
*/

E_UG0003_Bad_Classid	"A bad class id for a message was found.\n
An incorrect class identifier for message '%0x' was sent to the 
error processing routine.  This is an internal coding error, and 
should be reported to your technical support representative for INGRES."

/*%
** Explanation:
**	This error represents an incorrect class id, which
**	was found in an internal call.  This should never
**	occur, so it should be reported as a problem to 
**	your INGRES representative.
**
** System Status:
**	Program continues.
**
** User Action:
**	Report to INGRES technical support.
*/

/*
** Name:    IIUG_VERIFICATIONS -	FrontEnd Utility Verification Responses.
**
** Description:
**	The following list of strings represent the responses for 'yes' and 'no'
**	in a given language and are used for checking both prompt responses and
**	values entered in fields.  Up to five values each are supported for a
**	positive or negative response.  These are case insensitive since the
**	verification routines will automatically use the lower case equivalent.
**
**	Not all five choices need be specified; leave the last few the same as
**	the first (as is done in the English file) if this is the case.  Note
**	that all the prefixes that are to be allowed for a response should be
**	specified.  Also note that the first choice, F_UG0001_Yes1, is to be
**	used as the one character representation of "Yes", and the second 
**	choice, F_UG0002_Yes2 is the fully spelled out representation of "Yes."
*/

F_UG0001_Yes1	"y"
F_UG0002_Yes2	"yes"
F_UG0003_Yes3	"ye"
F_UG0004_Yes4	"ok"
F_UG0005_Yes5	"ok"

/*
**	The following list of strings should represent the 
**	response for 'no' in a given language.  Follow the
**	same conventions as for the 'yes' shown above.
*/

F_UG0006_No1	"n"
F_UG0007_No2	"no"
F_UG0008_No3	"n"
F_UG0009_No4	"n"
F_UG000A_No5	"n"

/*
**	The following are status returns from IIUGyn.  These
**	represent the values which are given when trying to translate
**	the 'yes' and 'no' strings into a TRUE or FALSE.
*/

E_UG0004_Yes_Response	"Yes response given"	/* DIAG */
E_UG0005_No_Response	"No response given"	/* DIAG */


E_UG0006_NULL_Response	"Please answer with 'y' or 'n'.\n
An empty response was detected.  You must respond with 'yes' ('y')
or 'no' ('n') as no default response has been defined."

/*%
** Explanation:
**	This message is given if an empty response is given for
**	a yes/no question.  Some of the frontends will require
**	and response, and this message will be given to tell
**	the user to enter a 'yes' or a 'no'.
**
** System Status:
**	The prompt will be redisplayed.
**
** User Action:
**	You must enter a valid value.
*/

E_UG0007_Illegal_Response
"Please answer with 'y' or 'n'.\n
The answer given was not a valid 'yes' or 'no'.  You may also answer
with a simple 'y'('yes') or 'n'('no')."

/*%
** System Status:
**	The prompt will be redisplayed.
**
** User Action:
**	You must enter a valid value.
*/

E_UG0008_Required_Response
"Please enter a value in response to the prompt.\n
This prompt requires an answer as a response that is not empty or blank.\ 
Please type something other than blanks or a return."

/*%
** System Status:
**	The prompt will be redisplayed.
**
** User Action:
**	You must enter a valid value.
*/

E_UG0009_Bad_Memory_Allocation
"Fatal memory allocation error in routine '%0c'.\n
The product has run out of memory in trying to do the previous
operation in such a way that no recovery is possible.  The
program will stop.  The internal routine in which the error
occurred is '%0c'.  Usually, restarting the program and trying
the operation will succeed in this case.  If that is not the
case, please contact your INGRES system administrator."

/*%
** Explanation:
**	A memory allocation or free error has occurred somewhere in
**	the code in a place where it was unexpected, or where it
**	was not possible to recover from the error.  This may
**	indicate a bug, or a limitation which we would like to
**	know about.
**
** System Status:
**	The program will abort.
**
** User Action:
**	You should restart the program and try the operation again.
**	If it still fails, call your INGRES support person.
*/

E_UG000A_No_Form_File	"Could not find form index file, `%0c'.\n
The form index file could not be found in the current installation.  Please
report this installation problem to your system administrator."

/*%
** Explanation:
**	INGRES front-ends read the forms that they will use from a special
**	file that contains all the INGRES forms.  A separate file exists for
**	each ethnic language that INGRES supports.  The file for the current
**	language and installation could not be found.  This is either because
**	it does not exist for the language specified through II_LANGUAGE
**	("$II_CONFIG/$II_LANGUAGE/rtiforms.fnx",) or the pathname specified
**	through II_FORMFILE is incorrect.  This is most likely because the
**	installation is incorrect.
**
** System Status:
**	No forms will be initialized.  The program will terminate.
**
** User Action:
**	Determine that the language installation is correct, and re-install
**	if necessary.  Or, if II_FORMFILE was specified make sure that it
**	specifies a valid pathname.
*/

E_UG000B_ExitsLimit	"FEset_exit:  too many module exits.\n
Please report this problem to your technical support representative."

/*%
** Explanation:
**	The number of exits supported by the exits module has been exceeded.
**	This is an internal programming problem.
**
** System Status:
**	The program will terminate.
**
** User Action:
**	Report this internal problem to your technical support representative.
*/

E_UG000C_ExitsMissing	"FEclr_exit:  module exit not found.\n
Please report this problem to your technical support representative."

E_UG000E_BadName	"The name `%0c' is not a valid INGRES name.\n
Valid names must begin with either an underscore `_' or a letter, and can
consist of letters, digits, and the characters _, $, @, and # up to a maximum
of 32 characters."

/*%
** Explanation:
**	A valid INGRES name must be entered as described in the message.
**	Examples of valid INGRES names are "_123", "_$name", or "name1".
**	Some invalid names are "%xyz", "$name", or "1left".
**
** System Status:
**	The program will not accept the name until you enter a valid INGRES
**	name (or in some cases, no value.)
**
** User Action:
**	Enter a valid INGRES name.
*/

E_UG000F_NoDBconnect	"Error connecting to database `%0c'."

/*%
** Explanation:
**	The program could not successfully connect to the named database
**	most likely because the database does not exist, or possibly because
**	the commands specified using the ING_SET environment logical are
**	incorrect.
**
**	Occasionally, successful database connection may be precluded by
**	other errors (e.g., GCA errors, etc.) usually evidence of some sort
**	of installation problem or hardware or system failure.
**
** System Status:
**	The program will not start up.
**
** User Action:
**	Check that the database name was specified correctly and re-enter
**	the command line.  Also, check that the commands specified using
**	the ING_SET logical are correct.
*/

/*
** Name:    IIUG_FEUTA_ERROR -	Front-End Argument Parsing Error Messages.
**
** Description:
**	User error messages for the FEUTA module, which parses command-line
**	parameters for INGRES subsystems.
*/

E_UG0010_NoPrompt	"%0c:  No user response to request for parameter input."
 
E_UG0011_ArgSyntax	"Correct syntax:\n
%0c %1c"

E_UG0012_NoArgValue	"%0c:  Expects value."

E_UG0013_TooManyArgs	"%0c:  Too many arguments."

E_UG0014_BadArgument	"%0c:  Bad argument."

E_UG0015_ArgMissing	"%0c:  not specified on command line."

E_UG0016_ArgConflict	"%0c:  Conflicts with other arguments."

E_UG0017_ListConflict	"%0c:  Conflicts with '%1c' list."

E_UG0018_Override	"'%0c' specified, '%1c' ignored."

E_UG0019_ArgIntReq	"%0c:  Integer required."

E_UG001A_ArgFltReq	"%0c:  Real number required."

/*
**	FEUTA Fatal Bugchecks ("ug/feuta.c".)
*/

E_UG001B	"FEutapfx: too many internal parameters, maximum is %0d.\n
Please check that your installation is correct before reporting this problem
to your technical support representative."

/*%
** Explanation:
**	Too many parameter specifications were found in the system
**	specification file.  This is an internal programming problem.
**
** System Status:
**	The program execution will be terminated.
**
** User Action:
**	Report this internal problem to your technical support representative.
*/

E_UG001C	"FEutaget: unknown internal parameter name %0c.\n
Please check that your installation is correct before reporting this problem
to your technical support representative."

/*%
** Explanation:
**	The internal parameter name has not been specified in the parameter
**	specifications for this INGRES subsystem even though the subsystem
**	expects such a parameter.  This is an internal programming problem.
**
** System Status:
**	The program execution will be terminated.
**
** User Action:
**	Report this internal problem to your technical support representative.
*/

E_UG001D	"FEuta:  no parameter specification for %0c.\n
Please check that your installation is correct before reporting this problem
to your technical support representative."

/*%
** Explanation:
**	No parameter specifications have been defined for the INGRES subsystem 
**	even though the subsystem is using the FE UTA module to parse expected
**	command-line parameters.  This is an internal programming problem.
**
** System Status:
**	The program execution will be terminated.
**
** User Action:
**	Report this internal problem to your technical support representative.
*/

E_UG001E	"FEuta:  Could not open parameter specification file, `%0c'.\n
The file used to parse command-line parameters for this INGRES subsystem could
not be opened.  This subsystem will terminate execution.\ 
Please verify that the INGRES files installation is correct and begin again."

/*%
** Explanation:
**	The file containing the execution and parameter specifications for
**	INGRES subsystems could not be opened.  Without this file the command-
**	line parameters for this subsystem cannot be parsed.  This is likely an
**	installation problem with the file, "$ING_FILES/utexe.def".
**
** System Status:
**	The program execution will be terminated.
**
** User Action:
**	Verify the INGRES installation and that the execution and parameter
**	specifications file exist in the installation, and then begin execution
**	of the INGRES subsystem again.
*/

E_UG001F	"FEuta: syntax error in parameter specification, option %0d.\n
Please check that your installation is correct before reporting this problem
to your technical support representative."

/*%
** Explanation:
**	The parameter specifications for this INGRES subsystem contain an
**	internal syntax error.  The FE UTA module cannot continue to parse any
**	command-line parameters.  This is an internal programming problem.
**
** System Status:
**	The program execution will be terminated.
**
** User Action:
**	Report this internal problem to your technical support representative.
*/

/*
** Allocate ADF Control Block Bug Checks ("ug/feadfcb.c".)
*/

E_UG0020_BadDateFormat	"A bad name was given in II_DATE_FORMAT.\n
The name `%0c' specified by the logical variable II_DATE_FORMAT is not
recognized as specifying a valid date format.  The format specified by the
name `us' will be used instead.\ 
Set the INGRES logical variable II_DATE_FORMAT to a valid date format name."

/*%
** Explanation:
**	The name given in II_DATE_FORMAT is not recognized as corresponding
**	to any known date formats.  One of these formats will be used to
**	display or print dates on forms or reports.  Acceptable names are
**	"multinational", "finland" (or "sweden",) "iso", or "us".
**
** System Status:
**	The date format specified by "us" will be used to
**	display or print dates.
**
** User Action:
**	Set a valid name for the II_DATE_FORMAT name.
*/

E_UG0021_BadMnyFormat	"A bad format was given in II_MONEY_FORMAT.\n
The format `%0c' specified by the logical variable II_MONEY_FORMAT is not
accepted for money values.  The format `l:$' will be used instead.\ 
Set the INGRES logical variable II_MONEY_FORMAT to a valid money format."

/*%
** Explanation:
**	The format given in II_MONEY_FORMAT is not accepted as a valid for
**	money values.  This format is used to display or print money values on
**	forms or reports.  Acceptable formats are of the form:  `[lt]:$$$$'
**	indicating whether the currency symbol precedes (`l') or follows (`t')
**	the money value, and the currency symbol upto a maximum of 4 characters
**	(following the colon `:'.)
**
** System Status:
**	The money format `l:$' will be used to display or print money values.
**
** User Action:
**	Set a valid format for the II_MONEY_FORMAT name.
*/

E_UG0022_BadMnyPrec	"A bad value was given in II_MONEY_PREC.\n
The value `%0c' specified by the logical variable II_MONEY_PREC is incorrect.
Valid precisions are 0, 1 or 2.  A precision of 2 will be used instead.\ 
Set the INGRES logical variable II_MONEY_PREC to a valid precision."

/*%
** Explanation:
**	The precision given in II_MONEY_PREC is not a valid precision for
**	money values.  This precision is used to display or print money values
**	on forms or reports.  Valid precisions for money values are 0, 1 or 2.
**
** System Status:
**	A precision of 2 will be used to display money values.
**
** User Action:
**	Set a valid precision for the II_MONEY_PREC name.
*/

E_UG0023_BadDecimal	"A bad value was given in II_DECIMAL.\n
The character `%0c' specified by the logical variable II_DECIMAL is not an
acceptable decimal point character.   The character `.' will be used instead.\ 
Set the INGRES logical variable II_DECIMAL to a valid decimal point character."

/*%
** Explanation:
**	The character given in II_DECIMAL is not a valid decimal point
**	character.  This character is used to display or print numbers that can
**	have a fractional part, such as money values or real numbers.  Valid
**	decimal point characters are `.' or `,'.
**
** System Status:
**	A decimal point character of `.' will be used to
**	display or print numbers.
**
** User Action:
**	Set a valid single-character string for the II_DECIMAL name.
*/

E_UG0024_CharInitError	"Error while processing character set attribute file.\n
%0c.  Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	The externally defined character set attribute file cannot be loaded 
**      correctly.  
**
** User Action:
**	Please report this internal problem to your Site Technical Contact.
*/

E_UG002F_BadMemoryTag	"Internal program error:  Memory tag exceeded maximum.\n
An internal memory tag exceeded the maximum allowed for its type.  No more
memory tags can be allocated by the tag allocation utility.  Please report
this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	INGRES programs may allocate memory in tagged blocks with the tags
**	being allocated by an internal tag allocation utility.  This tag
**	allocator has allocated all possible tags and cannot allocate anymore
**	tags when requested by an INGRES program.  This is an internal program
**	problem.
**
** User Action:
**	Please report this internal problem to your Site Technical Contact.
*/

/*
** Interrupt Handler Messages ("ug/feihdlr.c".)
**
**	Retain new-lines.
*/

S_UG0030_Exiting	"Exiting . . ."
S_UG0031_Because	"\ndue to "
S_UG0032_Interrupt	"Interrupt"
S_UG0033_Termination	"being terminated"
S_UG0034_Quit		"Quit signal"
S_UG0035_EOF		"EOF on input"
S_UG0036_NoMemory	"Front-End memory allocation failure"
S_UG0037_BugCheck	"Front-End internal bug check"
S_UG0038_UnknownException	"UNKNOWN SOFTWARE EXCEPTION - 0x%x"
S_UG0039_Hardware_Exception	"System/Hardware Exception"
S_UG003A_Software_Exception	"Software Exception"
S_UG003B_Routine	"%c(in `%s')"
S_UG003C_BugMessage	"%cRoutine `%s'."
S_UG003F_Contact	"\nPlease contact your Technical Support representative."

/* Messages used by the expression evaluation code in ugexpr.c */

/*%
** Explanation:
**	Expression contains a syntax error.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression & retry.
*/
E_UG0040_Syntax_Error	"Syntax error in expression."

/*%
** Explanation:
**	Expression contains unbalanced number of parenthesis.
**	For example:	#if ($a > $b
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression to balance parenthesis & retry.
*/
E_UG0041_Unbalanced_Parens	"Unbalanced parentheses in expression."

/*%
** Explanation:
**	Empty expression found.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression & retry.
*/
	/* fyi. E_UG0042_No_Expression is no longer used: */
E_UG0042_No_Expression	"Empty expression found....\n
No expression is present in a place where one is required."

/*%
** Explanation:
**	Division by zero in an expression.
**	For example:	#define $var 1/0
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression to remove the division by zero & retry.
*/
E_UG0043_Divide_By_Zero	"Division by zero in an expression."

/*%
** Explanation:
**	Expression contains unknown word.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression to remove unknown word & retry.
*/
E_UG0044_Invalid_Token	"Unknown word in expression...\n
Unrecognized word or variable name found in expression: '%0c'."

/*%
** Explanation:
**	A portion of a boolean expression did not evaluate to TRUE or FALSE.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
E_UG0045_Cant_AND_OR_Expr	"Character expression is not boolean...\n
One of the expressions around the AND or OR connector is a character
expression that did not evaluate to a simple TRUE or FALSE."

/*%
** Explanation:
**	Internal error occurred while evaluating an expression.
**
** System Status:
**	Cannot evaluate expression due to internal error.
**
** User Action:
**	Narrow down conditions necessary to make this occur and report to
**	your Site Technical Contact.
*/
E_UG0046_AV_SegViol     "Internal Error while evaluating expression...\n
An internal error (known as an Access Violation, Bus Error, or
Segmentation Violation on many systems) occurred while evaluating
an expression. Please try to narrow down the conditions required to make
this error occur, until it goes away; then report this problem
to Ingres Technical Support."

/*%
** Explanation:
**	Expression contains comparison of unlike types.
**	For example:	#if  10 > 'abc'
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
E_UG0047_Mixed_Type_Compare	"Comparing unlike types...\n
Trying to do comparison of unlike types. The left and right hand sides of
the comparison operator are not of the same type and cannot be compared."

/*%
** Explanation:
**	Expression contains AND or OR in wrong place.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
E_UG0048_Bad_AND_OR	"Bad AND/OR position...\n
AND and OR can only appear after a (usually) parenthesized expression."

/*%
** Explanation:
**	Boolean Expression is invalid.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
	/* fyi. E_UG0049_Bad_Bool_Oper is no longer used: */
E_UG0049_Bad_Bool_Oper	"Bad comparison operator...\n
Boolean condition has an invalid comparison operator (should be = or <>)."

/*%
** Explanation:
**	Expression is incomplete.
**	For example:	#if  $var >
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
E_UG004A_Bad_Condition	"Missing part of condition...\n
Condition has missing comparison operator or right-hand-side."

/*%
** Explanation:
**	Tried to do arithmetic on character type.
**	For example:	#define $var 'abc'/2
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
E_UG004B_Char_Arith	"Tried to do arithmetic on character type...\n
Cannot do arithmetic with character types. Only the '+' (concatenate) operator
may be used with character types."

/*%
** Explanation:
**	Mixture of data types in expression.
**	For example:	#define $var  'abc' + 2
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
E_UG004C_Mixed_Types	"Incompatible mixture of types...\n
Cannot mix character with numeric/boolean $variables and constants."

/*%
** Explanation:
**	Expression contains multiple assignments.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
	/* fyi. E_UG004D_Multiple_Assigns is no longer used: */
E_UG004D_Multiple_Assigns	"Multiple assignments...\n
Only one $variable assignment is allowed."

/*%
** Explanation:
**	Trying to set value of builtin $var.
**	For example:	#define $_var  'abc'
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression and retry.
*/
	/* fyi. E_UG004E_No_Assign_To_Builtin is no longer used: */
E_UG004E_No_Assign_To_Builtin	"Cannot assign value to built-in variable...\n
Error: a value was assigned to the built-in variable '%0c'.
You are only allowed to assign values to variables that are created
with the ##DEFINE statement."

/*%
** Explanation:
**	Expression contains a token that is too long.
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression to shorten the token & retry.
*/
E_UG004F_TokenTooLong	"Token exceeds allowable length...\n
The token exceeds the maximum allowable length of %0d characters."

/*%
** Explanation:
**	Expression contains a token that is missing a trailing quote. 
**
** System Status:
**	Error in expression.
**
** User Action:
**	Correct the expression to include a trailing quote & retry.
*/
E_UG0050_MissingQuote	"Character string token is missing a trailing quote\n
character...\n
Terminate the character string with a quote(%0c)."

/*
** Messages used by IIUGvfVerifyFile utility in ugvf.c which checks 
** file/directory existence and validity.
*/

E_UG00C0_BadParameter		"Internal coding error. Unsupported parameter." 

E_UG00C1_IsNull			"A null or empty string was passed as a file/directory name."
E_UG00C2_PathTrap		"One or more directories in path <%0c> do not exist."

E_UG00C3_BadPath		"Could not map the string <%0c> to a location."

E_UG00C4_IsDir 			"<%0c> is a directory."

E_UG00C5_IsNotDir		"<%0c> is not a directory."

E_UG00C6_IsWrtDir		"<%0c> is a writable directory."

E_UG00C7_IsNotWrtDir		"<%0c> is not a writable directory."

E_UG00C8_IsExistingFile		"<%0c> is an existing file."

E_UG00C9_IsNotExistingFile	"<%0c> is not an existing file."

E_UG00CA_IsNotWrtExistingFile	"<%0c> is not a writable existing file."

E_UG00CB_IsWrtExistingFile	"<%0c> is a writable existing file."

E_UG00CC_CannotParse		"Cannot parse string <%0c>."

E_UG00CD_IsNonExistingFile	"<%0c> is a non-existing file."

/*
** RAAT API Front-end Error Codes
*/
E_UG00D0_RaatTblNotOpen 	"RAAT API - table is not open."

E_UG00D1_RaatInvOperSecIdx	"RAAT API - invalid insert/delete/replace on a secondary index."

E_UG00D2_RaatTblHndlAlloc	"RAAT API - table handle memory allocation error."

E_UG00D3_RaatCantGetBaseofBase  "RAAT API - RAAT_BAS_RECDATA flag not valid on base table."
/*%
** Explanation:
**	The RAAT_BAS_RECDATA flag may only be used when performing an operation
**	against a secondary index, but it has been specified in a RAAT_CB
**	of a base table.
**
** System Status:
**	Transaction and table is still open, but request failed.
**
** User Action:
**	Turn off RAAT_BAS_RECDATA flag in the base table's RAAT_CB.
*/

E_UG00D4_RaatInvRecDataFlag	"RAAT API - Invalid combination of flags specified along with RAAT_BAS_RECDATA."
/*%
** Explanation:
**	The RAAT_BAS_RECDATA flag was specified along with other flags such
**	as RAAT_REC_BYNUM which is an invalid combination.
**
** System Status:
**	Transaction and table is still open, but request failed.
**
** User Action:
**	Turn off extraneous flags when using RAAT_BAS_RECDATA flag.
*/

E_UG00D5_RaatNoBaseHandle	"RAAT API - No base table handle  supplied for a RAAT_BAS_RECDATA request."
/*%
** Explanation:
**	The RAAT_BAS_RECDATA flag was specified in a secondary index's RAAT_CB
**	in order to read base table data, but the RAAT_CB's bas_table_handle
**	field was not filled in with the table handle from the base table
**	RAAT_CB's table_handle field.
**
** System Status:
**	Transaction and table is still open, but request failed.
**
** User Action:
**	Place the base table's table handle in the  bas_table_handle
**	field of the secondary index's RAAT_CB.
*/
