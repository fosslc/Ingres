/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    erw4.msg -	4GL Translator Message Definitions File.
**
** Description:
**	Contains the definitions for messages and error messages for
**	the 4GL translators.
**
** History:
**	Revision 6.0  87/08/03  wong
**	Initial revision.
**	10-feb-89 (marian)
**		Added E_W40224 for conversion warning messages.
**
**	W4GL I
**	25-jul-1990 (Joe)
**	    Added errors for incorrect type of field for the data
**	    it is attached to.
**
**	W4GL II
**	30-apr-1991 (sandyd)
**	     Changed W4_CLASS from 53 to 211 to avoid collision with old ABF.
**	11/20/91 (dkh) - ER changes for internationalization.
**	01/09/92 (emerson)
**		Changed messages S_W40263 and S_W40264 (which were created by
**		the previous change) and added S_W402B3 and S_W402B4,
**		so that line number and script name can be extracted from them.
**		(This facilitates invoking an editor on the appropriate
**		script source file).
**	03/03/92 (emerson)
**		Refined message S_W40289_UNDEF_PROC_IN_EXP.
**		(Part of fix for bug 42411).
**	04/13/92 (emerson)
**		Replaced message S_W4029E_SINGLE_EXP with a less obscure message
**		(Part of fix for bug 43046).
**
**	W4GL II FCS
**	07/27/92 (emerson)
**		Added message E_W4010D_STACK_OVERFLOW (for bug 43951).
**		Also added E_W400F0, S_W400F1, S_W400F2, and E_W400F3
**		(to remove redundant info from syntax error messages).
**
**		Also made several changes to messages S_W40255 - S_W402B2:
**		In W4GL II beta, the texts of these messages were used as
**		formats in calls to STprintf; the resulting string was printed
**		with a message ID of E_W4000A_TESTERROR.  In W4GL II FCS,
**		I print these messages with unique IDs; I skip the STprintf and
**		use the unique ID as input to e.g. IIUGfer.  This involved the
**		following changes to each of the messages S_W40255 - S_W402B2:
**		(1) If the message text contained no '%', I simply changed
**		    the leading S in the message ID to E.
**		(2) If the message text contained a '%', I created a new
**		    E_W4nnnn message ID, whose text is similar to the original
**		    S_W4mmmm message text, but with '%0c' or '%1c' in place of
**		    '%s', '%0x' or '%1x' in place of '%x', and so forth.
**		    The old S_W4mmmm message is retained in support of old
**		    executables.
**
**		Also clarified E_W40271_CANT_ASSIGN_PROC_OBJ (bug 44327).
**
**	W4GL III
**	5-feb-1993 (emerson)
**		Added E_W402C5 - E_W402CC and modified E_W40024, E_W40026,
**		and E_W40027 for local procedures.
**		Changed "debugger expression" to "dynamic expression"
**		in E_W402B5 and E_W402B6.
**	3-may-1993 (emerson)
**		For repeating fields: added E_W402D4 and E_W402D5,
**		and modified the wording of several messages that used
**		to talk only about table fields.
**		Also corrected the wording of E_W40277.
**	5-may-1993 (emerson)
**		Further refinement to E_W40277.
**	22-jun-1993 (emerson)
**		Modified E_W4026F, E_W40278, and E_W402D5
**		to reflect changes in syntax for referring to
**		the layout copy of a repeating field.
**	28-jul-1993 (emerson)
**		Added E_W402E1 and E_W402E2 for dynamic field declarations.
**	28-dec-1993 (emerson)
**		Added E_W402E3_TOO_MANY_PARMS.
**	13-mar-1994 (emerson)
**		Added E_W402E4 - E_W402EF.
**	17-mar-1994 (blaise)
**		Added S_W40267_IN_INCLUDE.
**	26-oct-1995 (connie) Bug #70175
**		Added E_W402F0_LENGTH_DECL_EXPECTED.
**	21-mar-96 (rogerwh)
**		Added E_W402F1_CASE_TYPECHK, E_W402F2_DUPLICATE_DEFAULT,
**		E_W402F3_DUPLICATE_CASE_VALUE errors for the case statement.
**	26-apr-1996 (rogerwh)
**		Added explanations to these messages.
**	12-dec-2002 (zhayu01) SIR 111125
**		Added nchar and nvarchar to E_W402F0_LENGTH_DECL_EXPECTED and
**		E_W402F5_NCHAR_LITERALS.
**	21-jul-2005 (malsa02) bug 114900
**		Added E_W402F5
**	25-aug-2005 (crido01)
**		Changed the last E_W402F5 to E_W402F6
**/

#define	_W4_CLASS	211

/*
** Name:    IIOS_ERROR -	4GL Translator Error Messages.
*/

E_W40000	"No database name was specified on the command line.\n
Please report this internal problem to your technical representative."

/*%
** Explanation:
**	4GL should only be executed by ABF, which should pass the database
**	name as a command-line argument to the program.  Failure to do so
**	is an internal program (ABF) error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative.
*/

E_W40001	"The input file '%0c' could not be opened.\n
Please check that the file exist and that you have access rights to read it."

/*%
** Explanation:
**	4GL could not open the source file that it is to compile either because
**	it does not exist or because you do not have access rights to read it.
**
** System Status:
**	4GL will terminate with an error status.
**
** User Action:
**	Verify that the name of the 4GL source file was specified correctly, or
**	create the file using the edit option of either the ABF user frame
**	definition frame or the ABF procedure definition frame.
*/

E_W40002	"A internal output file '%0c' was specified and could not be
opened.  Please report this internal problem to your technical representative."

/*%
** Explanation:
**	4GL should only be executed by ABF, which should not pass the debug flag
**	or internal debug file name as a command-line argument to the program.
**	However, an internal debug file was specified and could not be opened.
**	In any case, this is an internal program (ABF) error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative.
*/

E_W40003	"No form was specified for this frame.\n
4GL frames cannot be compiled unless they have a form specified when they were
defined.  Compilation terminated."

/*%
** Explanation:
**	4GL frames require a form to be specified when they are defined.
**	The frame cannot be compiled unless this is true.
**
** System Status:
**	4GL will terminate with an error status.
**
** User Action:
**	Specify a form in the appropriate field in the ABF user frame
**	definition frame.
*/

E_W40004	"The listing file '%0c' could not be opened.\n
Please check that you have access rights and enough disk space
to create files in the working directory."

/*%
** Explanation:
**	4GL could not open the specified file to write a program listing for
**	the source file being compiled.  This could be for any number of
**	reasons, but most likely is because you do not have access rights to
**	the current directory or because you do not have enough disk space
**	available to create files in that directory.
**
** System Status:
**	4GL will continue compile the source file, but no listing will
**	be produced.
**
** User Action:
**	Make sure that the you have access rights to the current directory
**	and enough disk space to create files there before reporting this
**	problem to your technical representative.
*/

/*
**	Summary Message ("osl/oserr.c")
*/
S_W40005	"\n\nThe following '%d' variables were referenced but not defined:\n"

#define	_UndefinedVars	S_W40005

/*
**	Completion Messages ("osl/osl.c")
*/
S_W40006	"\nThere were %d errors.\n"

#define	_Errors		S_W40006

S_W40007	"\nThere was %d error.\n"

#define	_1Error		S_W40007

S_W40008	"No errors\n"

#define _NoErrors	S_W40008

E_W40009	"This feature is not supported in Windows4GL yet.\n
Feature not supported is '%0c'."

/*
** This is a special message for testing.  It is simply something that
** allows a string to be output.
**
** This should never be in any code a user will see.
*/
E_W4000A_TESTERROR	"%0c"

/*
**	Fatal Bugchecks
*/

E_W40010	"An internal problem exists within the 4GL compiler.\n
Please report this problem to your technical representative.  The problem
appeared in the routine '%0c' while compiling line number '%1d'."

/*%
** Explanation:
**	An internal consistency check failed in the 4GL compiler in the
**	indicated routine.  This is an internal program error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative noting the routine
**	specification in the body of the message and the number of the line
**	that was being compiled.
*/

E_W40011	"The 4GL compiler ran out of memory.\n
Please report this problem to your technical representative.  The problem
appeared in the routine '%0c' while compiling line number '%1d'."

/*%
** Explanation:
**	The 4GL compiler could not allocate any more memory required to compile
**	the 4GL frame or procedure.  This is an internal program error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative noting the routine
**	specification in the body of the message and the number of the line
**	that was being compiled.
*/

E_W40012	"Badly formed statement within 4GL compiler.  A value was
expected of type '%0c' while compiling line number '%1d'.  Please report this
problem to your technical representative."

/*%
** Explanation:
**	The 4GL compiler had an internal consistency check failure while
**	building a statement parse tree.  This is an internal program error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative noting the type
**	specification in the body of the message and the number of the line
**	that was being compiled.
*/

E_W40013	"Badly formed statement within 4GL compiler.  A value was
expected to be acted on by '%0c' while compiling line number '%1d'.\ 
Please report this problem to your technical representative."

/*%
** Explanation:
**	The 4GL compiler had an internal consistency check failure while
**	building a statement parse tree.  This is an internal program error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative noting the type
**	specification in the body of the message and the number of the line
**	that was being compiled.
*/

E_W40014	"Badly formed expression within 4GL compiler while compiling
line number '%0c'.  Please report this problem to your technical
representative."

/*%
** Explanation:
**	The 4GL compiler had an internal consistency check failure while
**	building an expression parse tree.  This is an internal program error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative noting the line
**	that was being compiled.
*/

/*
**	Command Line Parsing Errors ("osl/getargs.c")
*/
F_W40015	"too many source files"	/* DIAG */

#define _TooManyArgs	F_W40015

F_W40016	"bad number: osAppid"	/* DIAG */

#define _BadAppid	F_W40016

F_W40017	"bad number: osFid"	/* DIAG */

#define _BadFid		F_W40017

F_W40018	"bad argument: %s"	/* DIAG */

#define _BadArg		F_W40018

/*
**	Form Declaration Errors
*/

E_W40020	"The form '%0c' does not exist in the Form System Catalogs,
or exists but is not owned by you."

/*%
** Explanation:
**	The form specified for the 4GL frame does not exist, or exists
**	but is not accessible to you.
**
** System Status:
**	4GL will terminate with an error status.
**
** User Action:
**	Verify that the name of the form is correct, or 
*/

E_W40021	"Invalid fields on form.\n
Two fields with the same name '%0c' were found on the form '%1c'.  This
indicates that the Form System catalogs may have been corrupted.\ 
Please report this problem to your technical representative."

/*%
** Explanation:
**	4GL detected an inconsistency in the Form System catalogs in that
**	two fields have the same name.  This is an internal FRS problem.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative.
*/

E_W40022	"Invalid table field on form.\n
Besides the table field '%0c', there is another field with the same name on
the form '%1c'.  This indicates that the Form System catalogs may have been
corrupted.\ 
Please report this problem to your technical representative."

/*%
** Explanation:
**	4GL detected an inconsistency in the Form System catalogs in that
**	a field has the same name as a table field.  This is an internal FRS
**	problem.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative.
*/

E_W40023	"Invalid columns on form.\n
Two columns have the name '%0c' in the table field '%1c'.  This
indicates that the Form System catalogs may have been corrupted.\ 
Please report this problem to your technical representative."

/*%
** Explanation:
**	4GL detected an inconsistency in the Form System catalogs in that
**	two columns for a table field have the same name.  This is an internal
**	FRS problem.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative.
*/

E_W40024	"Duplicate declaration.\n
The variable or local procedure '%0c' has the same name as another field,
variable, or local procedure declared in the frame or procedure '%1c'.\ 
Use a name different from '%0c' for the variable or local procedure."

/*%
** Explanation:
**	The frame or 4GL procedure already has a field, variable,
**	or local procedure with the same name as the variable
**	or local procedure that is being declared.
**	Because this is ambiguous, you should use a different name
**	for the variable or local procedure.
**
** System Status:
**	The definition of this variable or local procedure will be ignored.
**
** User Action:
**	Use a different name for the variable or local procedure.
*/

E_W40025	"Duplicate column declaration.\n
The hidden column '%0c' has the same name as another
column in the table field '%1c' on the form '%2c'.\ 
Use a name different than '%0c' for the hidden column."

/*%
** Explanation:
**	The table field on the form specified for the 4GL frame has a
**	column with the same name as the hidden column that is being declared
**	for this table field.  Because this is ambiguous, you should use a
**	different name for the hidden column.
**
** System Status:
**	The definition of this hidden column will be ignored.
**
** User Action:
**	Use a different name for the hidden column.
*/

E_W40026	"Invalid datatype.\n
The data type declared for the variable or local procedure '%0c' is invalid.\ 
Please consult the INGRES Reference Manual for valid data types."

/*%
** Explanation:
**	The variable or local procedure declaration did not specify
**	a recognized INGRES data type.
**
** System Status:
**	The type of this variable or local procedure will be undefined.
**
** User Action:
**	Consult the INGRES Reference Manual to determine those types that are
**	recognized, and then correct the variable or local procedure declaration
*/

E_W40027	"Invalid data size.\n
The size specified for the variable or local procedure '%0c' is not legal.\ 
Please consult the INGRES Reference Manual for legal data type sizes."

/*%
** Explanation:
**	The variable or local procedure declaration did not specify
**	a recognized INGRES data type size.
**
** System Status:
**	The type of this variable or local procedure will be undefined.
**
** User Action:
**	Consult the INGRES Reference Manual to determine those types that are
**	recognized, and then correct the variable or local procedure declaration
*/

E_W40028	"Duplicate constant declaration.\n
The constant '%0c' has the same name as another constant
or field in the frame or procedure '%1c'.\ 
Use a name different than '%0c' for the constant."

/*%
** Explanation:
**	The frame or 4GL procedure already has a field, table field, hidden
**	field or constant with the same name as the constant that is being
**	defined.  Because this is ambiguous, the constant cannot have this name.
**
** System Status:
**	The definition of this constant will be ignored.
**
** User Action:
**	Use a different name for the constant.
*/

E_W40029	"Declared procedure name mis-match.\n
The name, '%0c', defined for the procedure does not match the name
declared in the source code for the procedure, '%1c'."

#define	    OSXPROC	E_W40029
/*%

** Explanation:
**	The name of the procedure defined in ABF does not match the name
**	of the procedure syntactically specified in the 4GL source code
**	for the procedure.  (That is, as "procedure <name> ...".)
**
** System Status:
**	4GL will terminate.
**
** User Action:
**	Make sure the name of the procedure as defined in ABF matches the
**	name of the procedure specified in the 4GL source code file.
*/

/*
**	Lexical Errors.
**
**	Note that E_W40030 is superseded by E_W400F3 as of W4GL II FCS.
*/

E_W40030	"Syntax error on line '%0d'.  Unexpected End-Of-File."

E_W40031	"Unterminated comment."

/*%
** Explanation:
**	The 4GL scanner detected a comment (beginning with "/*") that was not
**	terminated (by a "* /"; note: no space) before the End-of-File was seen.
**	This usually means you forgot or mis-typed the comment terminator
**	character sequence.
**
** System Status:
**	4GL could not determine where the comment ended and assumed the rest
**	of the source file was a comment.  Since no more lines can be compiled,
**	the compilation will be terminated with an error status.
**
** User Action:
**	Insert the comment termination sequence "* /" (note:  no space) at the
**	appropriate place after the comment that caused the error.  Then compile
**	the source file.
*/

E_W40032	"Unterminated string constant (strings cannot span lines.)"

/*%
** Explanation:
**	You probably forgot to terminate the string with a string delimiter
**	character.  Strings cannot contain embedded new-lines.  The string on
**	this line must be terminated with a string delimiter character also on
**	this line.
**
** System Status:
**	The string will be terminated at the new-line and parsing will continue.
**	This may inadvertently result in spurious errors being recognized.  The
**	source file will not be compiled.
**
** User Action:
**	Terminate the string with a string delimiter character.  Or, if you
**	want a new-line to be embedded in the string, use text notation ('\n')
**	in 4GL/QUEL, or use hexadecimal specification (X'0C', ASCII) in 4GL/SQL.
**	Then compile the source file.
*/

E_W40033	"String constant too long for 4GL.  The limit is 253,
including any escape sequences (\xxx).\n
String truncated to: '%0c'"

E_W40034	"Bad number '%0c' in ERROR comment."	/* DIAG */

E_W40035	"Bad number '%0c' in LINE comment."	/* DIAG */

E_W40036	"Bad number '%0c' in special comment."	/* DIAG */

E_W40037	"Warning:  Strings delimited by double-quotes (\") are
no longer supported by 4GL/SQL.  String interpreted as single-quote or
hexadecimal string.  Use single-quotes (') to delimit strings or specify
strings using the hexadecimal notation.\n
Please consult the 4GL/SQL Reference Manual for details."

/*%
** Explanation:
**	4GL/SQL no longer supports strings delimited by double-quotes (").
**
** System Status:
**	The string will be converted to a single-quote string if it contains
**	only printable characters or a hexadecimal string if it does not.  No
**	error will be generated.
**
** User Action:
**	Use single-quotes (') to delimit strings or use the hexadecimal
**	notation in 4GL/SQL.
*/

E_W40038	"String constant is too long for 4GL.  The limit is 253,
including any doubled single-quotes (').\n
String truncated to: '%0c'"

E_W40039	"Invalid single-quote on line '%0d.'\n
Single-quote delimited strings are not accepted by 4GL."

/*
**	SQL Lexical Errors.
*/

E_W40040	"Hexadecimal string constant is too long for 4GL.  The limit
is 252 hexadecimal digits.\n
String truncated to: '%0c'"

E_W40041	"An invalid character was used in a hexadecimal string constant
instead of a hexadecimal digit.  (Hexadecimal digits are \"0123456789ABCDEF\".)"

E_W40042	"Hexadecimal string constants must contain an even number of
hexadecimal digits since each character requires two digits to specify it.\ 
Please consult the INGRES Reference Manual for details."

/*
**    Logical symbol errors
*/
E_W40050_BadDecimal   "A bad value was given in II_4GL_DECIMAL.\n
The character '%0c' specified by the logical variable II_4GL_DECIMAL is not an
acceptable decimal point character.   The character '.' will be used instead.\

Set the INGRES logical variable II_4GL_DECIMAL to a valid decimal point
character."

/*%
** Explanation:
**    The character given in II_4GL_DECIMAL is not a valid decimal point
**    character.  This is the character you have used in 4GL source
**    statements to specify numeric literals that can have a fractional part,
**    such as money values or real numbers.  Valid decimal point characters
**    are '.' or ','.  Important note: this logical symbol is provided
**    only for backward compatibility.  It is recommended that you use
**    a decimal point character of '.' in your 4GL source code,
**    and leave II_4GL_DECIMAL unset (it will default to '.').
**
** System Status:
**    The 4GL compiler will assume that your 4GL statements will use
**    a decimal point character of '.' to specify the fractional parts
**    of numeric literals.  If your 4GL statements in fact use some other
**    character, a syntax error will be reported.
**
** User Action:
**    Set a valid single-character string for the II_4GL_DECIMAL name.
*/


/*
**	Syntax Errors.
**
**	Note: As of W4GL II FCS, E_W40103 is obsolete,
**	E_W40030 is superseded by E_W400F3, and
**	E_W40100 - E_W40102 are superseded by E_W400F0 - S_W400F2.
**
**	S_W400F1 and S_W400F2 are always immediately preceded by E_W400F0.
*/

E_W400F0	"Syntax error.  The last symbol read was '%0c'."

S_W400F1	"Expecting the symbol '%0c'."

S_W400F2	"Expecting the symbol 'ENDIF'; possibly you typed 'ELSE IF'
when you meant 'ELSEIF' in your 'IF-ELSEIF...' statement."

E_W400F3	"Syntax error.  Unexpected End-Of-File."

S_W400F4	"Expecting one of the symbols '%0c'."

E_W40100	"Syntax error on line '%0d'.  The last symbol read
was '%1c'."

E_W40101	"Syntax error on line '%0d'.  The last symbol read
was '%1c'.  Expecting one of '%2c'."

E_W40102	"Syntax error on line '%0d'.  The last symbol read
was '%1c'.  Expecting an 'ENDIF'; possibly you typed 'ELSE IF'
when you meant 'ELSEIF' in your 'IF-ELSEIF...' statement."

E_W40103	"Operation specification syntax error on line '%0d'."

E_W40104	"The 4GL source file does not have any definitions in it."

E_W40105	"A ';' or '}' is missing."

E_W40106	"Operation specification not recognized in this context."

E_W40107	"Operation specification not recognized in this context.\n
Queries with an attached menu require assignment to a valid query object,
either the form or a table field on the form.  Otherwise, a query loop is
assumed, which can only have a block of statements as its body."

E_W40108	"Missing block for query loop.\n
Queries must be assigned to a valid query object, either the form or a table
field on the form.  Otherwise, a query loop is assumed, which must have a block
of statements as its body."

E_W40109	"Warning:  Not an Open SQL command.\n
The statement '%0c' is an extension to Open SQL; it may not be portable to a
non-INGRES DBMS."

#define OSNOTSQL0	E_W40109

/*%
** Explanation:
**	When the '-wopen' flag is given to ABF, the 4GL/SQL compiler will
**	produce warning messages when it encounters statements that are not
**	part of the Open SQL specification.
**
** System Status:
**	None.
**
** User Action:
**	If you want to build applications to be used on heterogeneous DBMS
**	systems using an INGRES/Gateway, you should not use any statements
**	that produce this warning message.
*/

/*
**	SET/INQUIRE FRS Errors ("osl/osfrs.c")
*/
S_W4010A	"%%%% Error, Line %d:"

#define	_FrsError	S_W4010A

S_W4010B	"%%%% Warning, Line %d:"

#define	_FrsWarn	S_W4010B

S_W4010C	"%%%% Fatal Error, Line %d:"

#define	_FrsFatal	S_W4010C

/*
**	Unrecoverable parser errors
*/
E_W4010D_STACK_OVERFLOW "The 4GL parser's stack overflowed.\n
The 4GL source file contains 4GL language constructs
that are nested too deeply."

/*%
** Explanation:
**	The 4GL compiler's internal parser stack overflowed.
**	This resulted from 4GL language constructs that were nested too deeply.
**
** System Status:
**	The 4GL compilation will be aborted.
**
** User Action:
**	Change the logic of the application so that constructs
**	are not nested so deeply.  Here are some recommendations:
**	(a) Make sure that each BEGIN has a matching END, each IF has a
**	    matching ENDIF, each left parenthesis has a matching right
**	    parenthesis, and so forth.
**	(b) Break up complex expressions into simpler expressions,
**	    using variables to hold intermediate results.
*/

/*
**	Type or Definition Errors.
*/

E_W40110	"An assignment statement had incompatible types between
the field or column being assigned to (on the left-hand side) and the
expression or field or column value being assigned (on the right-hand side.)"

E_W40111 "Warning:  Nullable expression assigned to non-Nullable variable.\n
A non-Nullable field or column is being assigned a Nullable expression, which
possibly can be Null.  Use the IFNULL() function to assign Nullable values to
non-Nullable fields or columns."

E_W40112	"Operands acted on by the operator '%0c'
had incompatible or illegal types."

/*%
** Explanation:
**	An expression specified an operation on two or more operands that had
**	incompatible or illegal types for such an operation.  For example, a
**	date and a number are incompatible types and cannot be compared using
**	any of the relational operators such as '='.  Or as another example,
**	strings are illegal operands for any operator but '+'.
**
** System Status:
**	Neither the expression nor the source file will be compiled, but
**	the source file will be checked for other errors.
**
** User Action:
**	Correct the operation.
*/

E_W40113	"An operand acted on by the operator
'unary minus' had an illegal type."

/*%
** Explanation:
**	An expression specified a unary operation on an operand that had an
**	illegal type for such an operation.  For example, the unary minus
**	operation cannot be applied to any but a numeric or money operand.
**
** System Status:
**	Neither the expression nor the source file will be compiled, but
**	the source file will be checked for other errors.
**
** User Action:
**	Correct the operation.
*/

E_W40114	"An operand acted on by the logical
operator '%0c' had a non-boolean type."

/*%
** Explanation:
**	A logical expression specified a logical operation (either AND, OR or
**	NOT) on an operand (expression) that was not a boolean type.  Boolean
**	expressions are either other logical expressions or an expression
**	specifying a comparison operation (e.g., "<=" or "like".)
**
** System Status:
**	Neither the logical expression nor the source file will be compiled,
**	but the source file will be checked for other errors.
**
** User Action:
**	Correct the logical expression.
*/

E_W40115	"The undefined symbol '%0c' was referenced
in an expression or in an assignment statement (on the right-hand side.)"

E_W40116	"Undefined table field '%0c'.\n
The name '%0c' is being used to refer to a table field.  No table field of
that name exists on the form '%1c'."

E_W40117	"Undefined column '%2c'.\n
The table field '%1c' on the form '%0c' does not contain the column '%2c'."

E_W40118	"The symbol '%0c' is undefined."

E_W40119	"The field or column used in an assignment
from a prompt must be of a character type."

E_W4011A	"The symbol '%0c' was used in an assignment statement
(on the left-hand side) but cannot have a value assigned to it."

/*%
** Explanation:
**	The symbol being assigned to in an assignment statement (on the
**	left-hand side of the assignment operator, '=' or ':=') is a constant
**	or a table.  Constants have a single (constant) value assigned to them
**	by definition and cannot have other values assigned to them.  Tables
**	can not have values assigned in this way.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Make sure that the symbol being assigned to represents a field or
**	column and then compile the source file.
*/

E_W4011B	"Warning:  Superfluous use of ':' to dereference a field
or column.
"

/*%
** Explanation:
**	The field or column being assigned to is superfluously dereferenced by
**	':'.  In some contexts such as when accessing the DBMS or the Forms
**	System, 4GL expects symbols to represent names rather than values.  But
**	within that context, 4GL also allows reference to be made to a value
**	that will contain the name by allowing use of the dereference indicator,
**	':'.  Assignments, however, always unambiguously refer to fields or
**	columns on the form and should not be dereferenced by ':'.
**
** System Status:
**	4GL will ignore the superfluous ':' and continue compiling the source
**	file.
**
** User Action:
**	Edit the source file and remove any superfluous ':'.
*/

E_W4011C	"Operands passed to the function '%0c'
had illegal types or were incorrect in number."

/*%
** Explanation:
**	An expression specified an internal built-in function operating on zero
**	or more operands that had illegal types or were incorrect in number for
**	such a function.  For example, the 'length()' function operates on
**	exactly one argument of a string type, no more, no less.
**
** System Status:
**	Neither the expression nor the source file will be compiled, but
**	the source file will be checked for other errors.
**
** User Action:
**	Correct the function call.
*/

E_W4011D "Warning:  Possible Null used when a non-Null value is required.\n
A Nullable field or column is being used in a context that will not recognize
a Null value if the field or column should happen to contain one.  (For example,
anywhere a 4GL name can be used.)  Any such fields should be declared NOT NULL
to guarantee that this does not happen."

#define OSNULVCHK	E_W4011D

E_W4011E	"Illegal operand passed to a function."

/*%
** Explanation:
**	A function was called with an illegal operand.  For example, 
**	a table field may have been passed to a host-language procedure.
**
** System Status:
**	Neither the expression nor the source file will be compiled, but
**	the source file will be checked for other errors.
**
** User Action:
**	Correct the function call.
*/

/*%
** Explanation:
**	A field or column that is of Nullable type was used in a context that
**	will not recognize a Null value if the field or column should contain
**	one.  Such contexts include form or field names, etc.  4GL warns about
**	these cases because if a Null value happens to be used here at runtime
**	an error will occur and further behavior of the application may be
**	unpredictable.
**
** System Status:
**	The 4GL frame or procedure will be compiled assuming no other errors
**	occur.  However, if by chance at runtime a Null value is placed in
**	the variable and it is used in this context, a runtime error will be
**	reported and the statement will be aborted with unpredictable results.
**
** User Action:
**	Define the field or column to be NOT NULL.  Null values are illegal in
**	this context so the field or column (usually a hidden one) should be
**	non-Nullable.
*/

E_W40120	"An expression or value of a character type is required in
this context, such as in a message or prompt statement,
or as the filename in a copy statement."

/*%
** Explanation:
**	4GL requires a string-valued expression in the context being translated,
**	e.g., as the 'message' for a message statement.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Correct the statement.
*/

E_W40121	"A expression or value of integer type is required,
such as in a row number for a table field reference."

E_W402CD	"A FOR loop variable must be of integer type and non-nullable."

E_W40122	"A string or integer reference is required in
this Form System or DBMS context."

/*%
** Explanation:
**	
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Correct the statement.
*/

E_W40123	"A string value or field or column reference of
a character type is required as a '%0c'."

F_W40124	"parameter keyword"

#define _PrmKeyWord	F_W40124

F_W40125	"table field name"

#define _TblFldName	F_W40125

F_W40126	"create with list"

#define _CrtWithList	F_W40126

F_W40127	"Forms System or DBMS reference"

#define _FrsName	F_W40127

/*%
** Explanation:
**	4GL requires a string value in the specified context.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Make sure that a string value is specified in the required context
**	and then compile the source file.
*/

E_W40128	"Illegal NULL value returned.\n
A Null value cannot be returned for the frame or procedure '%0c' since it has
been defined as NOT returning a Null value."

#define OSNULLRET	E_W40128

E_W40129 "Warning:  Nullable expression returned for non-Nullable procedure.\n
A Nullable expression, which possibly can be Null, is being returned for the
frame or procedure '%0c', which has been defined as NOT returning a Nullable
value.  Use the IFNULL() function to return Nullable values for non-Nullable
frames or procedures."

#define OSNULRETWARN	E_W40129

E_W4012A	"The frame or procedure '%0c' has been
defined as NOT returning a value.  Because of this, it cannot be used
in an expression or in an assignment statement (on the right-hand side.)"

E_W4012B	"Warning:  Return value ignored.  The frame or procedure
'%0c' has been defined as returning a value,
which is being ignored."

E_W4012C	"The return value for this function '%0c' was
ignored and will result in no operation being performed."

E_W4012D	"This frame or procedure '%0c' has
been defined as NOT returning a value.  Because of this,
you cannot return a value for it."

E_W4012E	"The type of the return value is incompatible with that
defined for this frame or procedure '%0c'."

E_W4012F	"Warning:  No return value.  The frame or procedure
'%0c' has been defined as returning a value.\ 
Although a default value will be returned, you should always explicitly return
values for frames or procedures that have been defined as returning a value."

/*%
** Explanation:
**	A return statement with no return value was recognized in the source for
**	a frame or procedure that was defined as returning a value.  Although a
**	default value will be returned, it is likely that you forgot to return
**	a value for the frame or procedure.  Also possible is that you defined
**	the frame or procedure as returning a value when no values should ever
**	be returned for it.
**
** System Status:
**	Compilation will continue with no errors.  At run-time a default value
**	will be returned for the frame or procedure.
**
** User Action:
**	Always explicitly return a value for frames or procedures that have been
**	defined as returning a value.  Or, if the frame or procedure is not
**	supposed to return values, define it as such.
*/

E_W40130	"Illegal Null-valued expression.  The result
of a Null-valued expression will always be Null."

E_W40131	"Illegal Null comparison.  Use \"IS NULL\" or \"IS NOT NULL\"
to compare for Null-values."

E_W40132	"ON event done to non-field object.\n
The ON event operation specification can only be done with a displayed
object."

E_W40133	"Arrays not supported in this context yet.\n"

E_W40134_BadWithNull
"Syntax error in copy statement. '%0c' is not a valid value in the
with null clause."

/*%
** Explanation:
**      Correct syntax is: copy table <tblname> (<colname> = <format>
**                              with null (<value>) ...)
**      where <value> can be a number if <format> is numeric or a hex
**      constant like X'393939' for the string '999' or a quoted string
**      constant like 'NULL'. An unquoted string is not allowed.
**
** System Status:
**      The source file will be checked for other errors but will not be
**      compiled.
**
** User Action:
**      Correct the syntax error and re-compile the source file.
*/


/*
**	Query Errors.
*/

E_W40150	"Warning:  Obsolete master-detail query syntax in parameter
list.  Master-detail syntax no longer uses ';' as a separator."

E_W40151	"A '%0c' in the target list of an attached query
in this frame or procedure is invalid since no fields have
been defined on the form '%1c'."

E_W40152	"The target list of an attached query must be fully
specified in the 4GL source file.  You cannot use fields to
dynamically specify the fields or columns of the target list at run-time."

E_W40153	"The table field '%0c' was illegally
referenced in the target list of an attached query."

E_W40154	"The '%0c' construct cannot be used in the
target list of an attached query to reference a table field row."

E_W40155	"The field '%0c' was
referenced in the target list of a query.  No field
of that name exists on the form '%1c'."

E_W40156	"The column '%0c' was referenced in
the target list of a query.  No column of that name exists
in the table field '%1c' on the form '%2c'."

E_W40157	"The name '%0c' is not the name of the
form but is being used to refer to the form in the specification
of a query object.  The form name is '%1c'."

E_W40158	"Too many attached queries in a parameter list.\n
Only one attached query can be passed in a parameter list."

/*%
** Explanation:
**	At the moment, 4GL only supports passing a single query (master-detail
**	or otherwise) as a parameter to a 4GL frame.  More than one such query
**	was specified in the parameter list for the CALLFRAME statement being
**	compiled and is an error.
**
** System Status:
**	Additional query parameters will be ignored, and although the source
**	file will be checked for other errors, it will not be compiled.
**
** User Action:
**	Change the parameter list so that it contains only one query and then
**	compile the source file.
*/

E_W40159	"Non-existent query object, '%0c'.\n
The name '%0c' is being used to refer to a query object that does not exist on
the form '%1c'.  It is not the name of the form nor is it the name of a table
field that exists on the form."

/*%
** Explanation:
**	A query object was expected either as the target of an assignment
**	(on the left-hand side) or as a value list (as part of a ".all")
**	in a query statement.  The name used to refer to the query object
**	is not the name of the form or of a table field on the form, which
**	are the only defined query objects for a frame or procedure.
**
** System Status:
**	The query statement will be ignored, and although the source file will
**	be checked for other errors, it will not be compiled.
**
** User Action:
**	Correct the name to be a defined query object and then compile the file.
*/

E_W40170	"Duplicate label definition.\n
The label '%0c' has already been defined in this frame or procedure.  Label
names must be unique."

/*%
** Explanation:
**	The frame or 4GL procedure already has a label with the same name
**	as the label that is being defined at this point.  Because this is
**	ambiguous, you should use a different name for this label.
**
** System Status:
**	The definition of this label will be ignored.
**
** User Action:
**	Use a different name for the label.
*/

E_W40171	"Undefined label.\n
No label named '%0c' has been defined in this frame or procedure."

/*%
** Explanation:
**	The label referenced in an ENDLOOP statement was not defined for any
**	of the WHILE, UNLOADTABLE, or query loops active at this point in the
**	source code.  Labels must be defined for an active loop at the point
**	where they are referenced by an ENDLOOP statement.
**
** System Status:
**	The ENDLOOP statement will be ignored.
**
** User Action:
**	Make sure the name refers to a defined and active label, and then
**	compile the source file.
*/

E_W40172	"The loop with label '%0c'
has passed out of scope and is no longer active.  You can
no longer execute an ENDLOOP with respect to this label."

/*%
** Explanation:
**	The label referenced in an ENDLOOP statement was defined for a WHILE,
**	UNLOADTABLE, or query loop that is no longer active at this point in
**	the source code.  Labels must be defined for an active loop at the point
**	where they are referenced by an ENDLOOP statement.
**
** System Status:
**	The ENDLOOP statement will be ignored.
**
** User Action:
**	Make sure the name refers to a defined and active label, and then
**	compile the source file.
*/

E_W40173	"The '%0c' statement is not allowed
within a 4GL procedure."

F_W40174	"column declaration"	/* syntactic construct */

#define _ColumnDecl	F_W40174

/*%
** Explanation:
**	Certain statements in 4GL implicitly assume that a form is associated
**	with the object being compiled.  Since forms can never be associated
**	with procedures, the presence of these statements in a procedure is
**	recognized as an error.
**
** System Status:
**	The statement will be ignored, and although the 4GL procedure will
**	be checked for other errors, it will not be compiled.
**
** User Action:
**	Remove the unsupported statements from the procedure
**	and then compile the source file for the procedure.
*/

E_W40175	"ENDLOOP can only be used within an unloadtable or
while loop, or within an attached query loop or sub-menu."

E_W40176	"UNLOADTABLE statements cannot be nested."

E_W40177	"Warning:  Hidden columns are not defined for table fields
that are initialized dynamically."

E_W40178	"Illegal character in text string.\n
The escaped character '\0' is not legal in a text string constant.\ 
Please consult the INGRES Reference Manual for details."

/*%
** Explanation:
**	The character '\0' was specified in a TEXT string constant in the
**	source file being compiled.  Character strings of type TEXT cannot
**	contain the character '\0' (the zero bit pattern.)  A more complete
**	discussion of this can be found in the INGRES Reference Manual.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Remove the character '\0' from the TEXT string constant and
**	then compile the source file.
*/

E_W40179	"A qualification cannot appear in this statement."

E_W4017A	"Invalid field '%0c' in qualification.\n
 Hidden fields cannot be used in a qualification predicate."

/*%
** Explanation:
**	The named hidden field was listed as a qualification predicate.  Since
**	hidden fields are not known to the Forms System, they cannot contain
**	both operators and values as can visible fields in query mode.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Remove the hidden field from the qualification predicate.  (Where
**	clauses can be built in a string variable to implement dynamic queries.
**	Please consult the 4GL Reference Manual for details.)
*/

E_W40200	"The range variable '%0c' has the same name
as a field on the form.  This will result in an ambiguity
in the target lists of queries and cannot be allowed."

E_W40201	"Unqualified column attribute."

E_W40210	"A field or column was not specified for a target list
element in an attached query select.  Specify the field or column
through a target list assignment, \"field = expression\".\ 
Please consult the 4GL/SQL Reference Manual for details."

E_W40211	"An INTO list is not allowed in 4GL/SQL queries.\ 
Use a target list assignment, \"field = expression\", instead.\ 
Please consult the 4GL/SQL Reference Manual for details."

E_W40212	"A '.all' in the values list of an INSERT statement
must correspond to a '*' in the column list."

E_W40213	"A '*' in the column list of an INSERT statement
must correspond to a '.all' in a values list."

E_W40214	"Unbalanced column and values lists in an INSERT statement.\n
Whenever a column list is specified for an INSERT statement, its elements
must correspond one-to-one with the elements of a VALUES clause."

E_W40215	"Invalid WITH|NOT DEFAULT clause.\n
"

E_W40220	"The DEFINE TEMPORARY or DEFINE PERMANENT command was
not followed by LINK."

E_W40221	"The word '%0c' cannot appear in a DESTROY LINK statement.\ 
The word(s) '%1c' expected."

E_W40222	"The statement ADD NODE is not supported in 4GL/QUEL."

E_W40223	"The statement REMOVE NODE is not supported in 4GL/QUEL."

E_W40224	"SQL transaction syntax '%0c' has been modified.\nVersion 
5.0 syntax (BEGIN/END TRANSACTION, ABORT) should be updated.\nIgnore 
this warning if you intend to use the run-time SET AUTOCOMMIT option."

/*%
** Explanation:
**	This is a warning message that is produced when a 5.0 transaction
**	statement is found in oslsql code in release 6.
**
** System Status:
**
** User Action:
**	The user should modify their code to follow 6.0 sql transaction
**	semantics or use the SET AUTOCOMMIT option to maintain 5.0 semantics.
*/

E_W40230	"Warning: The class name '%0c' is undefined in the database\n
4GL will assume this is a class name that will be defined later."

/*%
** Explanation:
**	4GL saw a name it thought was a class name, but it couldn't find
**	the class name in the database.  4GL will continue as if the name
**	is a class, assuming that the class will be defined by the time
**	this 4GL code is executed.
**
** System Status:
**	4GL will continue.
**
** User Action:
**	Be sure to define the class before executing the application.
*/

E_W40231	"Variable '%0c' used in a method invocation is not an object.\n
The variable '%0c' was used in a method invocation.  However this variable
is not an object reference.  Method invocations can only be done to variables
which reference objects."

/*%
** Explanation:
**	A variable was used in a method invocation, yet the variable was
**	not an object reference.
**
** System Status:
**	The 4GL parsing will still continue, but will exit with an error.
**
** User Action:
**	Check the type of the variable you are using in the method 
**	invocation.  It must be an object reference.
*/

E_W40232	"The method '%0c' is not defined for this class.\n
An attempt was made to invoke the method '%0c' on an object whose class
does not contain the method '%0c'.  Check the definition of the class
or the spelling of the method name."

/*%
** Parameters:
**	0: This is the name of the method that the user tried to 
**	   invoke on an object.
**
** Explanation:
**	A method that 4GL does not know about was used.
**
** System Status:
**	4GL will continue to compile the code, but the error must
**	be fixed before the code can be run.
**
** User Action:
**	Check the definition of the class of the object to find out
**	the legal methods, and also check the spelling of the method.
*/

E_W40233	"Attribute '%0c' is unknown for the class.\n
An attribute access was done with an attribute that is not part of the
class of the variable."

/*%
** Explanation:
**	An attribute that 4GL does not know about was used.
**
** System Status:
**	4GL will continue, but this is an error that must be
**	corrected before the code can be executed.
**
** User Action:
*/

E_W40234	"A '.' was used on a value which isn't an object.  
The dot operator '.' can only be used on an object value."

/*%
** Explanation:
** System Status:
** User Action:
*/

E_W40235	"A '.' was used on a value which is an array.  It is
illegal to apply '.' to an array."

/*%
** Explanation:
** System Status:
** User Action:
*/

E_W40236	"Indexing '[]' was done on a non-array value.  It is
only legal to index an array value."

E_W40240	"Missing object for ON event.\n
The ON event operation specification must be given an object for which the
event is to be caught."

E_W40241_NOFLDCLASS	"A field was built incorrectly.\n
An internal structure built for the field '%0c' was built incorrectly.
The field does not have a field class attached to it.  This is a problem
within Windows 4GL that must be reported to your Ingres Representative."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W40242_NOTYPESTRUCT	"A field was built incorrectly.\n
An internal structure built for the field '%0c' was built incorrectly.
The field does not have a type structure attached to it.  This is a problem
within Windows 4GL that must be reported to your Ingres Representative."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W40243_BADFLD	"Incorrect kind of data given for a field.\n
The datatype given for the field '%0c' is a simple type, yet the
field '%0c' is a '%1c'.  To contain a simple typed value, the field
must be a ScalarField of some kind.  Either change the type of the field
or change the kind of field."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W40244_NOTARRAY	"Incorrect kind of data given for a field.\n
The datatype given for the %1c '%0c' is not an array.  Normally, this
means that the TableField is associated with an attribute of a user class,
but the attribute is not defined to be an array.  Either change the type
of the attribute to be an array, or change the kind of field used."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W40245_NOTTFLD	"Incorrect kind of data given for a field.\n
The datatype given for the %1c '%0c' is an array, but '%0c' is not a
TableField.  Only TableFields can be associated with arrays.  Normally, this
means that the field is associated with an attribute of a user class that
is defined to be an array, but the field was not created as a TableField.
Either change the type of the attribute, or change the field."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W40246_NOTCOMPOSITE	"Incorrect kind of data given for a field.\n
The datatype given for the %1c '%0c' is an object, but '%0c' is not a
CompositeField.  Only CompositeFields (e.g. SubForms) can be associated
with objects.  Normally, this means that the field is associated with an
attribute of a user class that is defined to be an object, but the field
was not created as a SubForm.  Either change the type of the attribute,
or change the field."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W40247_CHARSTAR	"Attribute '%0c' can not be used in a form.\n
The field '%1c' contains a child field that is associated to the attribute '%0c'
of %1's class %2c.  The type of this attribute is a special type that can
not be used within a form."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W40248_BADATTR	"Attribute '%0c' is unknown.\n
The field '%1c' contains a child field that is named '%0c'.  For this to
be legal, '%0c' must be an attribute in %1c's class '%2c', but there is no
attribute named '%0c' in the class '%2c'."
/*%
** Parameters:
**	0		The name of the attribute that is unknown.
**
**	1		The name of the field that this attribute was
**			used in.
**
**	2		The name of the class of the field.
**
** Explanation:
** System Status:
** User Action:
*/

E_W40249_COMPATTR	"Internal problem with attribute '%0c'.\n
The field '%1c' contains a child field associated with the attribute '%0c' of
%1's class '%2c'.  This attribute is defined in the class '%2c', and is
known as a computed attribute.  However, there is no %3c method for this
attribute.  This is an internal problem within Windows 4GL that must be
reported to your Ingres Representative."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W4024A_NOTOBJ	"Incorrect kind of data given for field.\n
The field '%1c' contains a child named '%0c'.  However, the data for
'%1c' is not an object, or an array so '%1c' can not contain any children.
Normally, this means that '%1c' is associated with an attribute of a user
class that is not an object.  Either change the type of the attribute
or change the field."
/*%
** Parameters:
** Explanation:
** System Status:
** User Action:
*/

E_W4024B_CAST_NORHS	"Internal Casting Error.\n
A value is being assigned to a reference or array variable.  However, the
value does not have any information about what class it is.  This is an
internal error.  You can workaround this error by explicitly casting the
value to the class of the variable being assigned.  Report this to your
Ingres representative."

/*%
** Explanation:
**	An object value is being assigned to a variable, and the information
**	about the class of the object value is missing.  This is an
**	internal error.
**
** System Status:
** 	The 4GL code will not successfully compile.
**
** User Action:
**	Try explicitly casting the value to the class of the variable.
**	Report this to your Ingres representative."
*/

E_W4024C_CAST_NOLHS	"Internal Casting Error.\n
A value is being assigned to a reference or array variable.  However, the
variable does not have any information about what class it is.  This is an
internal error.  Report this to your Ingres representative."

/*%
** Explanation:
**	An object value is being assigned to a variable, and the information
**	about the variable is missing.  This is an internal error.
**
** System Status:
** 	The 4GL code will not successfully compile.
**
** User Action:
**	Report this to your Ingres representative."
*/

E_W4024D_CAST_NOCLASS	"Internal Casting Error.\n
A value is being assigned to a reference or array variable.  However, the
class of the value could not be found.  This is an internal error.
You can workaround this error by explicitly casting the value to the
class of the variable being assigned.  Report this to your Ingres
representative."

/*%
** Explanation:
**	An object value is being assigned to a variable, and the information
**	about the class of the object value could not be found.  This is an
**	internal error.
**
** System Status:
** 	The 4GL code will not successfully compile.
**
** User Action:
**	Try explicitly casting the value to the class of the variable.
**	Report this to your Ingres representative."
*/

E_W4024E_CAST_NOTARRAY "Attempt to assign a non-array value to an array variable.\n
A non-array value of class '%1c' was assigned to an array variable
of class '%0c'.  You can only assign an array value of class '%0c' to
this array variable."

/*%
** Parameters
**	0	The name of the class of the array. For example, if
**		the array variable is defined as ARRAY OF StringObject
**		this will be StringObject.
**
**	1	The name of the class of the value.
**
** Explanation:
**	A reference value was assigned to an array variable.  This
**	is not legal.  Only an array can be assigned to an array variable.
**
** System Status:
**	The 4GL code will not compile successfully.
**
** User Action:
**	Fix this statement so that an array value is assigned to
**	the array variable.
*/

E_W4024F_CAST_ARRAY	"The class of the array value does not match the array variable.\n
An array value of class '%1c' is being assigned to an array
variable of class '%0c'.  Classes '%1c' and '%0c' are not related, so the
assignment is illegal.\n"

/*%
** Parameters
**	0	The name of the declared class of the array. For example, if
**		the array variable is defined as ARRAY OF StringObject
**		this will be StringObject.
**
**	1	The name of the class of the value.
**
** Explanation:
**	For two arrays to be assignment compatible, the class of the
**	right-hand side must be related to the class of the left-hand side.
**
** System Status:
**	The 4GL code will not compile successfully.
**
** User Action:
**	Fix this statement so the arrays have the same class.
*/

E_W40250_CAST_WARN	"Warning: '%1c' is not a subclass of '%0c'.\n
A value of class '%1c' is being assigned to a reference variable declared
as class '%0c'.  The class '%1c' is not a subclass of '%0c'.  An implicit
cast is being done to cast the value to '%0c'.  If this cast fails at
runtime, a runtime error will be reported."

/*%
** Parameters:
**	0	The name of the class of the variable.
**
**	1	The name of the class of the value.
**
** Explanation:
**	A value is being assigned to a reference variable.  For this
**	to be legal, the value must be the same class or a subclass of
**	the variable.  In this case, the value is a superclass of
**	the variable.  It is possible that at runtime, the value might
**	contain a value that is the same class or a subclass of the
**	variable, so the compiler is issuing a warning and then adding
**	a cast to make sure the value is legal.
**
** System Status:
**	The 4GL code will compile successfully, but a runtime error
**	might be reported.
**
** User Action:
**	Check the code to make sure the runtime check will succeed.
*/

E_W40251_CAST_ERROR	"'%1c' is not a subclass of '%0c'.\n
A value of class '%1c' is being assigned to a reference variable of
class '%0c'.  The class '%1c' is not related to the class '%0c', so
this assignment is illegal."

/*%
** Parameters:
**	0	The name of the class of the variable.
**
**	1	The name of the class of the value.
**
** Explanation:
**	A value is being assigned to a reference variable.  For this
**	to be legal, the value must be the same class or a subclass of
**	the variable.  In this case, the class of the value is a not related in
**	anyway to the class of the variable, so this assignment is illegal.
**
** System Status:
**	The 4GL code will not compile successfully.
**
** User Action:
**	Fix the statement so the class of the right side is
**	a subclass of the left side.
*/

E_W40252_OBJECT_DBMS	"Reference or array variable used in a DBMS statement.\n
The reference or array variable '%0c' was used in a DBMS statement.  Only
simple valued variables can be used in DBMS statements.  Reference and
array variables are illegal in DBMS statements."

/*%
** Parameters:
**	0	The name of the variable.
**
** Explanation:
**	The only 4GL variables that can be used in a DBMS statement
**	are those declared with a simple type (e.g. Integer, Float, Varchar,
**	Date and Money).  In this case a variable whose type is some
**	kind of object (i.e. it is a reference or array variable) was
**	used in a DBMS statement.  This is illegal.
**
** System Status:
**	The 4GL code will not compile successfully.
**
** User Action:
**	Remove the use of this variable from the DBMS statement.
*/

E_W40253_BadRepeat    "Warning: query is not repeatable.\n
You have specified the REPEATED keyword on a query.
However, the query contains a variable reference in a context
which is not proper for a repeated query.
The REPEATED specification will be ignored."
/*%
** Explanation:
** 	The or REPEATED keyword was specified on a query.
** 	However, the query contains a variable reference in a context
**	which is not proper for a repeated query.  Examples of such
**	improper contexts are using a string variable to specify an entire
**	WHERE clause or to specify the name of a DBMS object (such as a
**	table or column).
**
** System Status:
**    	The REPEATED specification will be ignored. Compilation will continue.
**
** User Action:
**    	If you want the query to be repeatable, use a literal in place of
**   	the variable reference.  If you want to retain the variable reference
**      (thereby forcing the query to be marked non-repeatable), no action
**  	is necessary (but you might want to remove the REPEATED
**   	keyword so that the 4GL source file reflects what will actually occur,
**      and so that subsequent compilations won't get the warning message).
*/

/*
**	osappsrc.c
*/

E_W40254_ALLOCREF_ERR	"An error occurred while preparing to compile scripts.  The most probable cause is that internal state information were not setup correctly"

/*%
** Explanation:
**	Windows4gl had problems setting up to compile scripts.  This is
**	just a warning.
**
** System Status:
**	Windows4gl will try to continue running.
**
** User Action:
**	User can't do much since this is an internal error.
*/

/*
**	osblock.c
*/

E_W40255_CONT_STMT	"CONTINUE statement with no active loop"

E_W40256_CONT_LOOP	"Can only continue a loop statement"

/*
**	oscall.c
*/

E_W40257_NO_QUERY_IN_WITH	"A query can not appear in a with clause"

E_W40258_NO_ALL_IN_WITH	"A .all or * can not appear in a with clause"

E_W402E3_TOO_MANY_PARMS		"Too many parameters.\n
More than 255 parameters were specified on an invocation of a
frame, procedure, or method."

/*%
** Explanation:
**	More than 255 parameters were specified on an invocation of a
**	frame, procedure, or method.
**
** System Status:
**	The 4GL code will not compile successfully.
**
** User Action:
**	Reduce the number of parameters.  You may wish to create a user class
**	which represents a set of parameters.  Then you can populate it
**	and pass it as a single parameter.
*/

/*
**	oscursor.c
*/

E_W40259_UPDATE_NOUPD_CURSOR	"FOR UPDATE given with non-updatable cursor."

E_W4025A_FROM_NEEDED	"A from clause must be given with an updatable cursor"

E_W4025B_NO_REPEAT_CURSORS	"repeated cursors are not supported."

E_W4025C_NO_CORRELATION_VAR	"The cursor DELETE statement may not contain a correlation variable"

E_W4025D_NO_CUR_DEL_AS_REPEAT	"The cursor DELETE statement may not be a repeated query"

E_W4025E_NO_CORR_VAR_IN_UPD	"The cursor UPDATE statement may not contain a correlation variable"

E_W4025F_NO_UPD_AS_REPEAT	"The cursor UPDATE statement may not be a repeated query"

E_W40260_NO_FROM_IN_UPDATE	"The cursor UPDATE statement may not contain a FROM clause"

S_W40261_VAR_NOT_CURSOROBJ	"Variable used in cursor %s statement is not a Cursorobject.\n"

E_W402C3_VAR_NOT_CURSOROBJ	"Variable used in cursor %0c statement is not a Cursorobject.\n"

/*
**	Unrecoverable compiler errors
*/
E_W40262_CANT_RECOV_ERRORS "The 4GL compiler can't recover from earlier errors."

/*%
** Explanation:
**	The 4GL compiler encountered an error in the 4GL source file from which
**	it could not recover.  The error is somewhere near the end of the
**	portion of the 4GL source file that appears in the error listing,
**	and has already been described by a previous error message.
**
** System Status:
**	The 4GL compilation will be aborted.
**
** User Action:
**	Correct the errors that were described by previous error messages.
*/

/*
**	oserr.c
**
**	Notes to translators:
**
**	Messages S_W40263, S_W40264, S_W402B3, and S_W402B4 are used in very
**	specific ways, which must be borne in mind when translating them:
**	(1) The first substitution parameter in S_W40263_ERROR_AT_LINE is %s;
**	    its value indicates the severity of the error.
**	    This value will always be S_W402B3_ERROR or S_W402B4_WARNING;
**	    S_W402B3_ERROR and S_W402B4_WARNING are used nowhere else.
**	(2) The second substitution parameter in S_W40263_ERROR_AT_LINE is %ld;
**	    its value indicates the number of the line containing the error.
**	(3) S_W40264_IN_SCRIPT will appear immediately after
**	    S_W40263_ERROR_AT_LINE (on the same line) *if* the error or warning
**	    is in a widget script (as opposed to a frame or procedure script);
**	    S_W40264_IN_SCRIPT is used nowhere else.
**	    The two substitution parameters in S_W40264_IN_SCRIPT are each %s;
**	    their values indicate the type and name of the widget, in that order
**	(4) W4GL contains logic that parses the "stuff" that's generated
**	    in the listing file from these messages.  (The purpose of this
**	    logic is to bring up an editor on the script that contains a line
**	    on which the compiler reported an error).  This logic assumes that
**	    this "stuff" is all on one line.  Therefore, no translation
**	    may contain a new-line ('\n') in any of these messages.
**	(5) S_W40267_IN_INCLUDE will appear immediately after
**	    S_W40263_ERROR_AT_LINE (on the same line) *if* the error or warning
**	    is in an #include script. The substitution parameter is the name
**	    of the include script.
*/

S_W40263_ERROR_AT_LINE	"%%%% %s at line %ld"

S_W40264_IN_SCRIPT	" in script for %s %s"

S_W40267_IN_INCLUDE	" in include script %s"

S_W402B3_ERROR	"Error"

S_W402B4_WARNING	"Warning"

/*
**	osevt.c
*/

E_W40265_BAD_EVENT_NAME	"Bad event name or incorrect type of object for event."

E_W40266_DUP_EVENT_SPEC	"A duplicate event specification for the same object was used in an ON EVENT"

/*
**	osfrdecl.c
*/
E_W402C4_ERR_DECL_VAR	"Error declaring variable %0c.\n"

E_W40268_NOT_NULL_ARRAY	"A NOT NULL clause can not be given with an array or reference variable declaration.  All array and reference variables are nullable."

E_W40269_LEN_SPEC_NOT_ALLOW	"A length specification (e.g., (10)) can not be given with an array or reference variable declaration.  Remove the length specification."

E_W402C5_ERR_DECL_PROC	"Error declaring local procedure %0c.\n"

E_W402C6_MisplacedProcDecl "Misplaced local procedure declaration.\n
A declaration of a local procedure appears among the parameter
declarations for '%0c'.  Local procedure declarations should appear
in the 'declare' section (following the keyword 'declare')."

/*%
** Explanation:
**	Local procedure declarations should appear in the DECLARE section
**	in the INITIALIZE statement or in a field script (if they're used by
**	a frame), or in the DECLARE section in a procedure (if they're used by
**	that procedure).
**
** System Status:
**	The local procedure declaration will be processed anyway.
**	The error, however, is fatal.  (In future releases, a local procedure
**	declaration as a parameter may have a different meaning).
**
** User Action:
**	Move the local procedure declaration into the DECLARE section.
*/

E_W402C7_NestedLocalProc	"A local procedure may not be declared
inside another local procedure."

/*%
** Explanation:
**	A local procedure may not be declared inside another local procedure.
**
** System Status:
**	The statement will be processed, but the compilation will not succeed.
**
** User Action:
**	Remove the declaration, or move it into the declarations
**	for the main frame or procedure.
*/

E_W402C8_ProcNotDecl	"Procedure not declared.\n
The local procedure '%0c' is being defined, but did not appear
in a local procedure declaration."

/*%
** Explanation:
**	The name specified in the definition of a local procedure
**	has not yet appeared in any local procedure declaration.
**	A local procedure must be declared before it's defined.
**
** System Status:
**	A declaration of PROCEDURE RETURNING NONE is assumed.
**
** User Action:
**	Make sure the procedure is declared before it's defined.
*/

E_W402C9_DupProcDefn	"Duplicate procedure definition.\n
The name '%0c' has already appeared in an earlier local procedure definition."

/*%
** Explanation:
**	The name of the procedure specified in a local procedure definition
**	has already appeared in an earlier local procedure definition.
**
** System Status:
**	The 4GL compiler will examine the procedure definition
**	for further errors.
**
** User Action:
**	Make sure the name of the procedure is unique.
*/

E_W402CA_ProcNotDefn	"Procedure not defined.\n
The name '%0c' was declared as a local procedure,
but no local procedure of that name was ever defined."

/*%
** Explanation:
**	The name specified in a local procedure declaration
**	never appeared in any local procedure definition.
**	Each local procedure must be declared and defined exactly once.
**
** System Status:
**	A fatal error is reported.
**
** User Action:
**	Change the 4GL source program to define the procedure,
**	or remove the declaration of the procedure and all references to it.
*/

E_W402CB_CANT_ADD_CUROBJ_VAR	"The variable %0c could not be declared.\n
Possibly a field of the same name exists in the form."

/*%
** Explanation:
**	The compiler was unable to declare a variable representing
**	the current frame, procedure, field script, or event block.
**	The can occur for CurFrame if a field in the form is also named
**	CurFrame.
**
** System Status:
**	A fatal error is reported.
**
** User Action:
**	If the error occurs for the variable CurFrame or CurrentFrame,
**	and there is field in the form that's named CurFrame or CurrentFrame,
**	change the name of the field in the form.
**	Otherwise, contact Technical Support.
*/

E_W402CC_ProcDeclInEventBlock "Misplaced local procedure declaration.\n
A declaration of a local procedure appears in the 'declare' section
for an event.  Local procedure declarations should appear in a 'declare'
section for a field script or for the entire frame."

/*%
** Explanation:
**	Local procedure declarations should appear in the DECLARE section
**	in the INITIALIZE statement or in a field script (if they're used by
**	a frame), or in the DECLARE section in a procedure (if they're used by
**	that procedure).
**
** System Status:
**	The local procedure declaration will be processed anyway.
**	The error, however, is fatal.  (In future releases, a local procedure
**	declaration as a parameter may have a different meaning).
**
** User Action:
**	Move the local procedure declaration into the DECLARE section for
**	the field script or frame.
*/

E_W402CE_PROC_DEFAULT_VALUE
"A procedure declaration may not contain a default value clause."

/*%
** Explanation:
**      A local procedure declarations can not define a default value.
**
** System Status:
**      The default value definition will be ignored.
**
** User Action:
**      Remove the default value definition.
*/

E_W402CF_REDUNDANT_WITH_CLAUSE
"A declaration contains redundant or conflicting WITH clauses.\n"

/*%
** Explanation:
**      A declaration can contain only one WITH clause to define
**	nullability or a default.  If two appear, this message results.
**
** System Status:
**      The compilation will fail.
**
** User Action:
**      Remove the superfluous WITH clause.
*/

E_W402D0_EMPTY_DEFAULT_CLAUSE
"DEFAULT must be followed by a default string or NULL.\n"

/*%
** Explanation:
**      A declaration which contains a DEFAULT clause must specify a default
**	of some kind.  None is specified here.
**
** System Status:
**      The compilation will fail.
**
** User Action:
**      Remove the DEFAULT keyword or specify a default.
*/

E_W402D1_INVALID_INIT_TYPE
"The type of the default value is incompatible with the type of the
variable being declared.\n"

E_W402DA_NOMETHOD
"Method '%0c' is not defined for class '%1c'."

E_W402DB_METHOD_REDEFINED
"The name '%0c' has already appeared in an earlier method definition."

E_W402DC_METHOD_UNDEFINED
"Method not defined.\n
The name '%0c' was declared as a method,
but no method of that name was ever defined."

/*
**	osobjini.c
*/

E_W4026A_OBJINI_REF_ERR	"An inconsistency in internal state has been detected while preparing to generate code for a frame."

/*%
** Explanation:
**	Windows4gl had problems setting up internal state for generating
**	IL code for a frame.  This is a warning.
**
** System Status:
**	Windows4gl will continue running.
**
** User Action:
**	User can't do much since this is an internal inconsistency.
*/

E_W4026B_CANT_ADD_CURFRM_VAR	"Couldn't add frame variable-CurrentFrame"

E_W4026C_NO_CURFRAME_VAR	"Couldn't add frame variable-CurFrame"

E_W4026D_PROCINI_REF_ERR	"An inconsistency in internal state has been detected while preparing to generate code for a procedure."

/*%
** Explanation:
**	Windows4gl had problems setting up internal state for generating
**	IL code for a procedure.  This is a warning.
**
** System Status:
**	Windows4gl will continue running.
**
** User Action:
**	User can't do much since this is an internal inconsistency.
*/

E_W4026E_CANT_ADD_CURPROC	"Couldn't add procedure variable-CurProcedure"

/*
**	osreffrm.c
*/

E_W4026F_COL_REF_IN_EXP	"A reference to all rows ([*]) was used in an expression.  This is not supported."

E_W40270_ARRAY_REF_NOT_DISP	"An array reference using the [] or [<>] operators was done on an array that isn't displayed in a table field or repeating field."

E_W40271_CANT_ASSIGN_PROC_OBJ	"Invalid target of assignment or BYREF."

E_W40272_CANT_CAST_ON_LEFT	"A cast can't be used on the left hand side"

E_W40273_ASSIGN_TO_READONLY	"Attempt to assign (or pass byref) to a readonly variable or a constant"

E_W40274_EVENT_ON_TF_ROW	"ON EVENT applied to a row of a table field."

E_W40275_FLD_FUNC_TF_ROW	"FIELD special function applied to a row of a table field."

E_W40276_FLD_FUNC_NON_FLDOBJ	"FIELD special function applied to non-field object"

E_W40277_FUNC_COMP_IN_TF	"Using the FIELD special function on an attribute of a composite field within a row of a table field is not supported."

E_W40278_COL_AND_NON_COL_REF	"A reference to all rows ([*]) was mixed with a reference to a specific row ([integer], [<integer>], or []) in a FIELD special function or in an ON EVENT."

/* E_W40279 is longer used (as of W4GL III) */

E_W40279_REF_ON_EVENT	"Either a non-column (e.g., [integer], [<integer>], or []) array reference was done in an ON EVENT, or a column array reference (e.g., [*]) was mixed with a non-column array reference in a FIELD special function."

E_W4027A_FUNC_ON_NESTED_TF	"Using the FIELD special function on a nested table field reference is not supported.  There are no CellAttributes for a nested table field."

E_W4027B_CAST_NON_VAR_VAL	"ON EVENT/FIELD done on cast on non variable value."

E_W4027C_ON_FUNC_ON_METHOD	"ON EVENT/FIELD special function not supported on method invocations, procedure calls or special functions."

E_W402D4_RF_IN_TF		"A reference was made to a repeating field inside a table field.  This is not supported."

E_W402D5_TF_IN_RF		"A reference was made to a table field inside a repeating field.  This is not supported."

/*
**	ossymchk.c
*/

S_W4027D_NO_PACKAGE_NAME	"Package name not found for reference "

E_W402B7_NO_PACKAGE_NAME	"Package name not found for reference %0c!%1c."

S_W4027E_NAME_HIDES_OBJECT	"Warning: The name %s hides the object %s.\n"

E_W402B8_NAME_HIDES_OBJECT	"Warning: The name %0c hides the object %1c.\n"

/*
**	ostree.c
*/

E_W4027F_CAST_ON_SINGLE_VAL	"A cast can only be done on a single value that is an object of some kind."

E_W40280_ATTR_OF_TYPE_CHAR	"Attempt to use an attribute whose type is char *.  This is not supported in 4GL."

E_W40281_CANT_EXEC_NON_PROC	"Executing procedure call on non-procedure object."

S_W40282_PROC_UNDEFINED	"Warning - Procedure %s is undefined.\n"

E_W402B9_PROC_UNDEFINED	"Warning - Procedure %0c is undefined.\n"

E_W40283_OPEN_GOTO_WITH_BYREF	"An OPENFRAME or GOTOFRAME statement was passed parameters BYREF.  This is not allowed."

E_W40284_CANT_EXEC_NON_FRAME	"Executing frame call on non-frame."

S_W40285_FRAME_UNDEFINED	"Warning - Frame %s is undefined.\n"

E_W402BA_FRAME_UNDEFINED	"Warning - Frame %0c is undefined.\n"

S_W40286_EXPECT_VAR	"The %s '%s' was used in a context where a variable is expected."

E_W402BB_EXPECT_VAR	"The %0c '%1c' was used in a context where a variable is expected."

E_W40287_ILLEGAL_ARG_TO_FUNC	"Illegal argument to a special function.  Argument must be a single variable reference."

E_W40288_FIELD_FUNC_TO_NONFLD	"FIELD special function applied to non-field."

E_W40289_UNDEF_PROC_IN_EXP	"Undefined or dynamically-named procedure or method used inside an expression."

E_W402DE_PRIVATE_MEMBER
"'%0c' is a private %1c of class '%2c'.  It is only available to methods of
class '%2c' and its subclasses."

/*%
** Parameters:
**	0		The name of the private member.
**
**	1		Either 'attribute' or 'method'
**
**	2		The class that the attribute or method belongs to.
**
** Explanation:
**    Only public members of user classes are available except to 
**    their methods and their subclasses' methods.
**
** System Status:
**    The compilation will fail, because the reference to the private member 
**    is not valid.
**
** User Action:
**    Remove the reference to the private member.
*/

E_W402DF_BADSENDSUPER	
"The SendSuperclass method can only be invoked on curmethod.\n"

/*
** Explanation:
**    SendSuperclass can only be invoked on the currently executing method,
**    that is, curmethod.
**
** System Status:
**    The compilation will fail.
**
** User Action:
**    Remove or correct the invalid SendSuperclass invocation.
*/

E_W402E0_SENDSUPERARGS
"Arguments to the SendSuperclass method will be ignored.\n
The same arguments passed to the current method will be passed to the
superclass's method."

/*
** Explanation:
**	SendSuperclass passes the calling method's arguments to the
**	superclass's method.  Arguments specified in 4GL are ignored.
**
** System Status:
**	Only a warning.
**
** User Action:
**	Remove the arguments from the SendSuperclass invocation.
*/

E_W402E1_BAD_UNDECL_INDEX
"An invalid reference was made to a row of the field '%0c',
for which no variable was declared.\n
Such references must appear within the 'field' special function,
and must not refer to a specific row: only references to all rows
(%0c[*]) are permitted."

/*%
** Parameters:
**	0		The name of the field.
**
** Explanation:
**    When the specified tablefield was created (e.g. in apped),
**    its 'declare' attribute was set to false.  Such a field
**    may be referred to only inside the special function 'field',
**    and the only legal row reference is to "[*]".
**
** System Status:
**    The compilation will fail, because the reference to the field
**    is not valid.
**
** User Action:
**    Remove the reference to the field, or correct it,
**    or change its 'declare' attribute to true and recompile.
*/

E_W402E2_REF_TO_UNDECL
"An invalid reference was made to the field '%0c',
for which no variable was declared.
Only references to field(%0c) are allowed."

/*%
** Parameters:
**	0		The name of the field.
**
** Explanation:
**    When the specified field was created (e.g. in apped),
**    its 'declare' attribute was set to false.  Such a field
**    may be referred to only inside the special function 'field'.
**
** System Status:
**    The compilation will fail, because the reference to the field
**    is not valid.
**
** User Action:
**    Remove the reference to the field, or enclose it in 'field(...)',
**    or change its 'declare' attribute to true and recompile.
*/

/*
**	osvarref.c
*/

E_W4028A_BAD_USE_SET_SQL	"Illegal use of a SQL SET function in a target list Left Hand Side."

E_W4028B_CANT_USE_PKG_ID	"Package syntax ID!ID is not accepted in a DBMS expression."

E_W4028C_BAD_4GL_VAR_REF	"Illegal 4GL variable reference in DBMS expression use :"

E_W4028D_BAD_4GL_ARR_REF	"Illegal 4GL array reference in DBMS expression use :"

E_W4028E_CANT_USE_4GL_METH	"4GL method invocations can not be used in DBMS expressions."

E_W4028F_NO_PKGID_DBMS_FUNC	"Package syntax ID!ID not accepted in DBMS functions."

E_W40290_BAD_PARM_DBMS_FUNC	"Incorrect parameter passing to DBMS function."

E_W40291_ONLY_USE_ID_EXP	"In this select target list, you can only use ID = expression or expression AS ID."

/*
**	scan.c
*/

E_W40292_BAD_FLOAT_CONST	"Invalid floating constant."

S_W40293_NON_PRINT_FOUND	"Non-printing character %s(0x%x) found.\n"

E_W402BC_NON_PRINT_FOUND	"Non-printing character %0c found.\n"

S_W40294_NON_PRINT_HEX_CONST	"Non-printing character %s(0x%x) found in hexadecimal constant.\n"

E_W402BD_NON_PRINT_HEX_CONST	"Non-printing character %0c found in hexadecimal constant.\n"

/*
**	w4sql!comp4gl.c
*/

S_W40295_BUG_CHECK_COMP	"Internal bug check during compilation in routine %s"

E_W402BE_BUG_CHECK_COMP	"Internal bug check during compilation in routine %0c"

S_W40296_NO_MORE_MEM_COMP	"Ran out of memory during compilation in routine %s"

E_W402BF_NO_MORE_MEM_COMP	"Ran out of memory during compilation in routine %0c"

S_W40297_UNKNOWN_EX_COMP	"Unknown exception (%x) during compilation.  Attempting to recover."

E_W402C0_UNKNOWN_EX_COMP	"Unknown exception (%0x) during compilation.  Attempting to recover."

/*
**	w4sql!sql.sy
*/

E_W40298_ATTACH_QUERIES	"Attached queries are not support in Windows/4GL."

S_W40299_COMP_TARG_LIST	"* as complete target list"

E_W4029A_VAR_TO_STAR	"variable = * is not supported in Windows/4GL."

E_W4029B_STAR_AS_VAR	"* AS variable is not supported in Windows/4GL."

E_W4029C_BAD_USE_OF_DOT_STAR	"Illegal use of .* in target list"

E_W4029D_ID_DOT_STAR	"ID . * not supported in Windows/4GL."

E_W4029E_SINGLE_EXP	"A target variable must be specified for the database expression or column."

/*%
** Explanation:
**	In a singleton select, a database expression or column was specified,
**	but no target variable was specified to receive it.
**
** System Status:
**	The 4GL code will not compile successfully.
**
** User Action:
**	Instead of "database_expression_or_column", code
**	":simple_var = database_expression_or_column" or
**	"database_expression_or_column AS :simple_var".
*/

E_W4029F_DOT_ALL_NO_SUPP	".ALL is not supported in Windows/4GL."

E_W402A0_READONLY_EXPECTED	"READONLY expected"

E_W402A1_READ_ONLY_EXPECTED	"READ ONLY expected"

E_W402A2_DIRECT_EXPECTED	"DIRECT or DEFERRED expected"

S_W402A3_INTO_EXPECTED	"INTO expected after fetch"

/*
**	mgram!oslgram.my
*/

E_W402A4_FRAME_SCRIPT_PROC	"The script is a script for a frame, but this object is a procedure.  Change the script to a procedure definition."

E_W402A5_PROC_SCRIPT_FRAME	"The script is a script for a procedure, but this object is a frame.  Change the script to a frame script."

E_W402A6_HIDDEN_COLS_NO_SUP	"Hidden columns are not supported in Windows/4GL.  You need to build a class for the table field."

E_W402A7_ARRAY_DECL_EXPECTED	"'Array' expected in declaration.\n"

E_W402A8_ON_TIMEOUT_NO_SUP	"ON TIMEOUT is not supported in Windows/4GL"

E_W402A9_NEED_USEREVENT_NAME	"An ON USEREVENT must be followed by an event name in a string constant."

E_W402AA_NEED_FLD_OBJ_NAME	"An ON event must be followed by a field object."

E_W402AB_OLD_STYLE_MENU_ACT	"Old style 4GL menu activations are not supported in Windows/4GL"

E_W402AC_NO_RET_VAL	"Can't get a return value from a GOTOFRAME."

S_W402AD_WITH_NOT_FRAMEEXEC	"The with clause name '%s' is not an attribute of FrameExec, so it can't be used in a with clause."

E_W402C1_WITH_NOT_FRAMEEXEC	"The with clause name '%0c' is not an attribute of FrameExec, so it can't be used in a with clause."

S_W402AE_RHS_NOT_COMPAT	"The type of the right hand side for the with clause item '%s' is not compatible with the type expected."

E_W402C2_RHS_NOT_COMPAT	"The type of the right hand side for the with clause item '%0c' is not compatible with the type expected."

E_W402AF_PASSING_ALL_AS_PARAM	"Passing parameters using .ALL is not supported in Windows/4GL."

E_W402B0_PASSING_DOT_NAME	"Parameter names of the form name1.name2 are not supported in Windows/4GL."

E_W402B1_NEXT_IN_SELECT_LOOP	"The NEXT statement should only be used within a SELECT loop within Windows/4GL."

E_W402B2_CANT_USE_QUAL	"The QUALIFICATION function is not supported in Windows/4GL."

/* S_W402B3 and S_W402B4 are used by oserr.c */

E_W402B5_CALL_IN_FRAGMENT
"Procedures and frames cannot be called from a dynamic expression."

E_W402B6_METH_IN_FRAGMENT
"Methods cannot be called from a dynamic expression."

/* E_W402B7 - E_W402B8 are used by ossymchk.c */
/* E_W402B9 - E_W402BB are used by ostree.c */
/* E_W402BC - E_W402BD are used by scan.c */
/* E_W402BE - E_W402C0 are used by comp4gl.c */
/* E_W402C1 - E_W402C2 are used by oslgram.my */
/* E_W402C3 is used by oscursor.c */
/* E_W402C4 - E_W402CB are used by osfrdecl.c */

/* sql.y */
E_W402D2_NOT_CONSTANT "'%0c' is not a system-defined constant.\n"

E_W402D3_ERROR_WITH_CONSTANT
"Internal error acessing system-defined constant '%0c'.\n"
/*%
** Explanation:
**	An internal error occurred getting the value of the constant.
**
** System Status:
**	The compilation will fail.
**
** User Action:
**	Contact Technical Support to report this internal error.
*/

/* E_W402D4 - E_W402D5 are used by osreffrm.c */

E_W402D7_BADUSERCLASS
"The UserClass could not be initialized.  Compilation of its methods
will be terminated."

E_W402D8_ONLY_FORWARD_DECLS
"Only local procedures can be declared in the initial DECLARE section
of a UserClass's script."

E_W402D9_METHOD_DECL_EXPECTED	"'Method' expected in declaration.\n"

E_W402EE_TOO_MANY_SOURCE_LINES
"Too many lines in source file.\n
No 4gl component may contain more than %0d lines in its source file.
But the 4gl component being compiled contains %1d lines."

/*%
** Parameters:
**	0	The maximum number of lines allowed in a 4gl component.
**
**	1	The number of lines in the 4gl component being compiled.
**
** Explanation:
**	The Windows/4GL compiler was asked to compile a 4gl component
**	which contains too many lines in its source file.
**
** System Status:
**	The compilation will fail.
**
** User Action:
**	Reduce the number of lines in the 4gl component, for example
**	by converting local procedures into global procedures.
*/

E_W402F0_LENGTH_DECL_EXPECTED
"A Length is required in declaration of varchar, char, nvarchar or nchar.\n"

/*
** igcase.c
*/
E_W402F1_CASE_TYPECHK
"The type of a case statement value does not match the type of the case
statement selector expression."
/*%
** Explanation:
**	Within the compilation of a case statement, one of the case values
**	could not be converted to the type of the selector expression.
**
** System Status:
**	Incorrect code will be generated for this case statement.
**
** User Action:
**	Check the type of the selector expression and make sure all the
**	values given are type compatible with it.
*/

E_W402F2_DUPLICATE_DEFAULT
"Only one 'default' case is allowed in a case statement."
/*%
** Explanation:
**      During the compiling of a 'case' statement, more than one 'default'
**	case was specified.  Only one is allowed in each case statement.
**
** System Status:
**	Only the first default statement will be honored.
**
** User Action:
**	Remove the duplicate default cases from the case statement.
*/

E_W402F3_DUPLICATE_CASE_VALUE
"Two or more case statement values were the same."
/*%
** Explanation:
**      While compiling a 'case' statement, the compiler noticed that one
**	or more constant values were the same.	This will cause a problem
**	at runtime, since there is an ambiguity in which piece of code to
**	execute.
**
** System Status:
**	The program will probably not work properly when run.
**
** User Action:
**	Check all the values in the case statement and eliminate any
**	duplicate values found.
*/

/*
** ossymdec.c
*/
E_W402DD_PRIVATTR "'%0c' is a private attribute.\n
The field '%1c' contains a child field that is named '%0c'.  For this to
be legal, '%0c' must be a public attribute in %1c's class '%2c', 
but '%0c' a private attribute."
/*%
** Parameters:
**	0		The name of the private attribute.
**
**	1		The name of the field that this attribute was
**			used in.
**
**	2		The name of the class of the field.
**
** Explanation:
**    Only public attributes of user classes are mapped to children
**    of composite fields.
**
** System Status:
**    The compilation will fail, because the field does not map to
**    an attribute of its parent's user class properly.
**
** User Action:
**    Change the field name, or redeclare the attribute to be public.
*/

/* E_W402DE - E_W402E2 are used by ostree.c */

/* E_W402E3 is used by oscall.c */

/*
** igconst.c
*/
E_W402E4_TOO_MANY_LITERALS
"Too many literals.\n
The overall 4gl component being compiled contains %0d %1c.
No more than 32767 such literals are permitted within a compilation."

/*%
** Parameters:
**	0	The number of literals of the specified type
**		in the 4GL component being compiled.
**
**	1	The type of literals of which there are too many.
**		(This type will be given by one of the slow messages below).
**
** Explanation:
**	The Windows/4GL compiler has detected more than 32767 literals
**	of a specific type.
**
** System Status:
**	The compilation will fail.
**
** User Action:
**	Try reducing the number of the specified type of literal
**	in the component being compiled.  If that is not possible,
**	split parts of it (each part including some of the specified type
**	of literal) off into separate global procedures.
*/

S_W402E5_VCHAR_LITERALS		"string literals within expressions"

S_W402E6_CHAR_LITERALS		"string literals outside expressions
(including compiler-generated string literals)"

S_W402E7_INT_LITERALS		"integer literals
(including compiler-generated integer literals)"

S_W402E8_FLOAT_LITERALS		"floating-point literals"

S_W402E9_DECIMAL_LITERALS	"decimal literals"

S_W402F5_NVCHAR_LITERALS	"unicode string literals within expressions"

/*
** igalloc.c
*/
E_W402EA_TOO_MANY_VARS
"Too many variables declared.\n
No executable object (application, frame, global procedure, local procedure,
or method) may declare more than 32767 variables.  This includes
all user-declared variables and compiler-generated temporary variables.
(For a frame, it also includes all fields except those
for which a variable was explicitly not declared).
But an executable object in the 4gl component being compiled declares
%0d variables.  (If the offending executable object is a local procedure
or method, this error message appears immediately after its last statement;
otherwise, this error message appears at the end of the compilation listing)."

/*%
** Parameters:
**	0	The number of variables declared in the executable object
**		being compiled.
**
** Explanation:
**	The Windows/4GL compiler, running under a platform other than
**	Microsoft Windows, release 3.x, was asked to compile an executable
**	object which contains more than 32767 declared variables.
**	[On Microsoft Windows, release 3.x, a lower limit is imposed,
**	and E_W402EB_TOO_MANY_VARS_ON_WIN will appear if it's exceeded].
**
** System Status:
**	The compilation will fail.
**
** User Action:
**	Try reducing the number of variables in the executable object,
**	or try splitting parts of it (each part including some variable
**	declarations) off into separate local procedures.
*/

E_W402EB_TOO_MANY_VARS_ON_WIN
"Too many variables declared.\n
No executable object (application, frame, global procedure, local procedure,
or method) can run on Microsoft Windows, release 3.x,
if it declares more than %0d variables.  This includes
all user-declared variables and compiler-generated temporary variables.
(For a frame, it also includes all fields except those
for which a variable was explicitly not declared).
But an executable object in the 4gl component being compiled declares
%1d variables.  (If the offending executable object is a local procedure
or method, this error message appears immediately after its last statement;
otherwise, this error message appears at the end of the compilation listing)."

/*%
** Parameters:
**	0	The maximum number of variables allowed on Microsoft Windows.
**
**	1	The number of variables declared in the executable object
**		being compiled.
**
** Explanation:
**	The Windows/4GL compiler, running under Microsoft Windows,
**	release 3.x, was asked to compile an executable object 
**	which contains too many declared variables.
**
** System Status:
**	The compilation will fail.
**
** User Action:
**	Try reducing the number of variables in the executable object,
**	or try splitting parts of it (each part including some variable
**	declarations) off into separate local procedures.
*/

E_W402EC_TOO_MANY_VARS_FOR_WIN
"Warning: too many variables declared for Windows.\n
No executable object (application, frame, global procedure, local procedure,
or method) can run on Microsoft Windows, release 3.x,
if it declares more than %0d variables.  This includes
all user-declared variables and compiler-generated temporary variables.
(For a frame, it also includes all fields except those
for which a variable was explicitly not declared).
But an executable object in the 4gl component being compiled declares
%1d variables.  (If the offending executable object is a local procedure
or method, this error message appears immediately after its last statement;
otherwise, this error message appears at the end of the compilation listing).
Although %1d variables is acceptable on the current platform,
the executable object will not be runnable on Windows."

/*%
** Parameters:
**	0	The maximum number of variables allowed on Microsoft Windows.
**
**	1	The number of variables declared in the executable object
**		being compiled.
**
** Explanation:
**	The Windows/4GL compiler, running on a platform other than
**	Microsoft Windows, release 3.x, was asked to compile an executable
**	object which contains too many declared variables for Windows.
**
** System Status:
**	The compilation will continue.
**
** User Action:
**	If you do not intend to run this application on Windows,
**	no user action is necessary.  But if you *do* so intend,
**	try reducing the number of variables in the executable object,
**	or try splitting parts of it (each part including some variable
**	declarations) off into separate local procedures.
*/

E_W402ED_TOO_MANY_BLOCKS
"Too many blocks.\n
No 4gl component may define more than %0d blocks.
(A 'block' is a field script, event block, local procedure, or method).
But the 4gl component being compiled defines %1d blocks."

/*%
** Parameters:
**	0	The maximum number of blocks allowed in a 4gl component.
**
**	1	The number of blocks defined in the 4gl component being
**		compiled.
**
** Explanation:
**	The Windows/4GL compiler was asked to compile a 4gl component
**	which contains too many field scripts, event blocks, local procedures,
**	and methods.
**
** System Status:
**	The compilation will fail.
**
** User Action:
**	Reduce the number of blocks in the 4gl component, for example
**	by converting local procedures into global procedures.
*/

/* E_W402EE is used by sql.y */

/*
** igstmt.c
*/
E_W402EF_EXPR_TOO_COMPLEX
"4GL expression is too complex.\n
A 4GL expression is too complex to be evaluated.
Try splitting it into multiple expressions."

/*%
** Explanation:
**	The Windows/4GL compiler was asked to compile an expression
**	which is too complex to be evaluated on any platform.
**
** System Status:
**	The compilation will fail.
**
** User Action:
**	Try splitting the expression into multiple expressions
**	(by evaluating subexpressions into 4GL variables).
*/

E_W402F4_NOTEXTOBJ	"Incorrect kind of data given for a field.\n
The datatype given for the %1c '%0c' is not an External Object.
Only ExtObjects can be associated with ExtObjFields."


/*%
** Parameters:
**	0: This is the name of the method that the user tried to 
**	   invoke on an object.
**
** Explanation:
**	A method that 4GL does not know about was used.
**
** System Status:
**	4GL will continue to compile the code, but the error must
**	be fixed before the code can be run.
**
** User Action:
**	Check the definition of the class of the object to find out
**	the legal methods, and also check the spelling of the method.
*/

E_W402F6	"Attribute '%0c' is unknown for the class '%1c'.\n
An attribute access was done with an attribute that is not part of the
class of the variable."

