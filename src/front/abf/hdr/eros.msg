/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	eros.msg -	4GL Translator Message Definitions File.
**
** Description:
**	Contains the definitions for messages and error messages for
**	the 4GL translators.
**
** History:
**	Revision 6.0  87/08/03  wong
**	Initial revision.
**	10-feb-89 (marian)
**		Added E_OS0224 for conversion warning messages.
**
**	Revision 6.4
**	03/13/91 (emerson)
**		Added E_OS0050_BadDecimal (for bad II_4GL_DECIMAL).
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Modified E_OS0024 and added E_OS0060 through E_OS0068
**		and E_OS016F.
**	04/22/91 (emerson)
**		Added E_OS017C thru E_OS017F for alerter support.
**	05/07/91 (emerson)
**		More modifications for local procedures:
**		Added messages E_OS016D and E_OS016E.
**	09-may-91 (mgw)
**		Added E_OS0132 for syntax error on COPY 'with null' clause.
**		Also corrected typos and moved a commented Explanation back
**		to the error message it describes.
**	06/20/91 (emerson)
**		Added E_OS000A_TypeMismatch (for bug 38203).
**	07/22/91 (emerson)
**		Added E_OS0266_ProcInVar (for bug 38753).
**	07/25/91 (emerson)
**		Added E_OS0267_CallProcSysFunc (for bug 38849).
**	02-aug-1991 (davel)
**		Added E_OS0268_BadTblFld (for bug 35665).
**	08/04/91 (emerson)
**		Provided support for the user to manually issue the osl
**		or oslsql command:  Added E_OS000B thru E_OS000D
**		and changed the wording of E_OS0000 (it can now be
**		a user error).  Also changed the explanatory comments
**		for E_OS0002.
**	08/06/91 (emerson)
**		Added E_OS0269_BadDbmsinfo for bug 21745.
**	08/15/91 (emerson)
**		Added E_OS015A_BadRepeat and E_OS015B_QualRepeat for bug 35468.
**		Also fixed E_OS0159 to reflect the fact that query objects
**		can be arrays or records.
**
**	Revision 6.4/02
**	10/09/91 (emerson)
**		Added comment to the effect that E_OS0257
**		has been superseded by E_OS0260.
**		Refined the E_OS0260 message.
**	12/16/91 (emerson)
**		Integrated a change from the PC group to add a new-line
**		to the beginning of the "No errors" message (S_OS0008).
**		This provides consistency with other OSL compiler completion
**		messages.
**
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Refined the text of E_OS0153 slighty: it said "table field"
**		when it should have said "table field, record, or array".
**		Also reworded the text of E_OS0155 and E_OS0210: target list
**		items can be variables and (for E_OS0210) attributes
**		as well as fields and (for E_OS0210) table field columns.
**		Also changed "attached query" to "query" in several messages.
**		Added E_OS015C_BadColonInSelectTarget (for bugs 38299 and 44004)
**
**	Revision 6.5
**	18-aug-92 (davel)
**		Added E_OS003A (for unterminated delimited identifier).
**	19-may-93 (davel)
**		Added E_OS025B (a warning for "type of table" declaration 
**		problems).
**	07/28/93 (dkh) - Added support for the PURGETABLE and RESUME
**			 NEXTFIELD/PREVIOUSFIELD statements.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**/

#define	_OS_CLASS	53

/*
** Name:	IIOS_ERROR -	4GL Translator Error Messages.
*/

E_OS0000	"No database name was specified on the command line.\n
The -d flag is missing from the osl or oslsql command line."

/*%
** Explanation:
**	The 4GL compiler (osl or oslsql) was invoked without the -d flag.
**
** System Status:
**	The 4GL compiler will terminate with a fatal status.
**
** User Action:
**	If you issued the osl or oslsql command, supply a -d flag
**	and re-issue the command.  If the error occurred when you selected
**	'Compile' or 'Go' during an ABF session, report the error
**	to your technical representative.
*/

E_OS0001	"The input file '%0c' could not be opened.\n
Please check that the file exists and that you have access rights to read it."

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

E_OS0002	"A internal output file '%0c' was specified and could not be
opened.  Please report this internal problem to your technical representative."

/*%
** Explanation:
**	An internal debug file was specified on the osl or oslsql command line,
**	but it could not be opened.  This is an internal program (ABF) error.
**
** System Status:
**	4GL will terminate with a fatal status.
**
** User Action:
**	Report this error to your technical representative.
*/

E_OS0003	"No form was specified for this frame.\n
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

E_OS0004	"The listing file '%0c' could not be opened.\n
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

E_OS000A_TypeMismatch	"Source file and ABF catalog specify different
object types.\n
The ABF catalog specifies that a frame is being compiled, but the source file
appears to define a procedure (or vice versa)."

/*%
** Explanation:
**	When you created this object you told ABF that it was a frame,
**	but your source file appears to define a 4GL procedure (or vice versa).
**
** System Status:
**	The ABF catalog will be ignored.
**
** User Action:
**	Correct the source file, or delete the frame/procedure object
**	and recreate it as the other type.
*/

E_OS000B_TooManyArgs	"Too many source files in command-line\n
More than one source file was specified on the osl or oslsql command line."

/*%
** Explanation:
**	The 4GL compiler (osl or oslsql) was passed more than one source file.
**	(Any argument not beginning with '-' or '+' is taken to be a source file
**	name).
**
** System Status:
**	The 4GL compiler will terminate with a fatal status.
**
** User Action:
**	If you issued the osl or oslsql command, correct the command
**	and re-issue it.  If the error occurred when you selected
**	'Compile' or 'Go' during an ABF session, report the error
**	to your technical representative.
*/

E_OS000C_BadArg		"Invalid command-line argument\n
The invalid argument '%0c' was specified on the osl or oslsql command line."

/*%
** Explanation:
**	The 4GL compiler (osl or oslsql) was passed an invalid argument.
**
** System Status:
**	The 4GL compiler will terminate with a fatal status.
**
** User Action:
**	If you issued the osl or oslsql command, correct the command
**	and re-issue it.  If the error occurred when you selected
**	'Compile' or 'Go' during an ABF session, report the error
**	to your technical representative.
*/

E_OS000D_BadFrameOrProc	"Bad frame or procedure\n
The frame or procedure could not be retrieved from the ABF catalog.
Either the -a or -f flag on the osl or oslsql command line was missing
or invalid, or the ABF catalog contains invalid data."

/*%
** Explanation:
**	The 4GL compiler (osl or oslsql) could not retrieve
**	the requested frame or procedure from the ABF catalog.
**	Either the -a or -f flag was missing or invalid,
**	or the ABF catalog contains invalid data.
**
** System Status:
**	The 4GL compiler will terminate with a fatal status.
**
** User Action:
**	If you issued the osl or oslsql command, correct the -a or -f flag
**	and re-issue the command.  If the error occurred when you selected
**	'Compile' or 'Go' during an ABF session, report the error
**	to your technical representative.
*/

/*
**	Summary Message ("osl/oserr.c")
*/
S_OS0005	"\n\nThe following `%d' variables were referenced but not defined:\n"

#define	_UndefinedVars	S_OS0005

/*
**	Completion Messages ("osl/osl.c")
*/
S_OS0006	"\nThere were %d errors.\n"

#define	_Errors		S_OS0006

S_OS0007	"\nThere was %d error.\n"

#define	_1Error		S_OS0007

S_OS0008	"\nNo errors\n"

#define _NoErrors	S_OS0008

/*
**	Fatal Bugchecks
*/

E_OS0010	"An internal problem exists within the 4GL compiler.\n
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

E_OS0011	"The 4GL compiler ran out of memory.\n
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

E_OS0012	"Badly formed statement within 4GL compiler.  A value was
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

E_OS0013	"Badly formed statement within 4GL compiler.  A value was
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

E_OS0014	"Badly formed expression within 4GL compiler while compiling
line number '%0d'.  Please report this problem to your technical
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
F_OS0015	"too many source files"	/* DIAG */

#define _TooManyArgs	F_OS0015

F_OS0016	"bad number: osAppid"	/* DIAG */

#define _BadAppid	F_OS0016

F_OS0017	"bad number: osFid"	/* DIAG */

#define _BadFid		F_OS0017

F_OS0018	"bad argument: %s"	/* DIAG */

#define _BadArg		F_OS0018

/*
**	Form Declaration Errors
*/

E_OS0020	"The form '%0c' does not exist in the Form System Catalogs,
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

E_OS0021	"Invalid fields on form.\n
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

E_OS0022	"Invalid table field on form.\n
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

E_OS0023	"Invalid columns on form.\n
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

E_OS0024	"Duplicate variable or local procedure declaration.\n
The variable or local procedure '%0c' has the same name as another field,
table field, variable or local procedure in the frame or procedure '%1c'.\ 
Use a name different from '%0c' for the hidden field."

/*%
** Explanation:
**	The frame or 4GL procedure already has a field, table field, variable,
**	or local procedure with the same name as the variable or local procedure
**	that is being defined.  Because this is ambiguous, you should use
**	a different name for the variable or local procedure.
**
** System Status:
**	The definition of this variable or local procedure will be ignored.
**
** User Action:
**	Use a different name for the variable or local procedure.
*/

E_OS0025	"Duplicate column declaration.\n
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

E_OS0026	"Invalid datatype.\n
The data type declared for the hidden field or column '%0c' is invalid.\ 
Please consult the INGRES ABF/4GL Reference Manual for valid data types."

/*%
** Explanation:
**	The hidden field or column definition did not specify
**	a recognized INGRES data type.
**
** System Status:
**	The type of this hidden field or column will be undefined.
**
** User Action:
**	Consult the INGRES ABF/4GL Reference Manual to determine those types
**	that are recognized, and then correct the hidden field or column
**	declaration.
*/

E_OS0027	"Invalid data size.\n
The size specified for the hidden field or column '%0c' is not legal.\ 
Please consult the INGRES ABF/4GL Reference Manual for legal data type sizes."

/*%
** Explanation:
**	The hidden field or column definition did not specify
**	a recognized INGRES data type size.
**
** System Status:
**	The type of this hidden field or column will be undefined.
**
** User Action:
**	Consult the INGRES ABF/4GL Reference Manual to determine those types
**	that are recognized, and then correct the hidden field or column
**	declaration.
*/

E_OS0028	"Duplicate constant declaration.\n
The constant '%0c' has the same name as another constant,
field or table field in the frame or procedure '%1c'.\ 
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

E_OS0029	"Declared procedure name mis-match.\n
The name, '%0c', defined for the procedure does not match the name
declared in the source code for the procedure, '%1c'."

#define		OSXPROC	E_OS0029

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
*/

E_OS0030	"Syntax error on line '%0d'.  Unexpected End-Of-File."

E_OS0031	"Unterminated comment."

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

E_OS0032	"Unterminated string constant (strings cannot span lines.)"

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

E_OS0033	"String constant too long for 4GL.  The limit is 253,
including any escape sequences (\xxx).\n
String truncated to: `%0c'"

E_OS0034	"Bad number `%0c' in ERROR comment."	/* DIAG */

E_OS0035	"Bad number `%0c' in LINE comment."	/* DIAG */

E_OS0036	"Bad number `%0c' in special comment."	/* DIAG */

E_OS0037	"Warning:  Strings delimited by double-quotes (\") are
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

E_OS0038	"String constant is too long for 4GL.  The limit is 253,
including any doubled single-quotes (').\n
String truncated to: `%0c'"

E_OS0039	"Invalid single-quote on line '%0d.'\n
Single-quote delimited strings are not accepted by 4GL."

E_OS003A_UntermDelimID	"Unterminated delimited identifier (delimited\n
identifiers cannot span lines.)"

/*%
** Explanation:
**	You probably forgot to terminate a delimited identifier with a double
**	quote.  Delimited identifiers cannot contain embedded new-lines.  The 
**	delimited identifier on this line must be terminated with a double quote
**	character also on this line.
**
** System Status:
**	The delimited identifier will be terminated at the new-line and parsing 
**	will continue.  This may inadvertently result in spurious errors being 
**	recognized.  The source file will not be compiled.
**
** User Action:
**	Terminate the delimited identifier with a double quote character.
**	Then re-compile the source file.
*/

/*
**	SQL Lexical Errors.
*/

E_OS0040	"Hexadecimal string constant is too long for 4GL.  The limit
is 252 hexadecimal digits.\n
String truncated to: `%0c'"

E_OS0041	"An invalid character was used in a hexadecimal string constant
instead of a hexadecimal digit.  (Hexadecimal digits are \"0123456789ABCDEF\".)"

E_OS0042	"Hexadecimal string constants must contain an even number of
hexadecimal digits since each character requires two digits to specify it.\ 
Please consult the INGRES ABF/4GL Reference Manual for details."

/*
**	Logical symbol errors
*/
E_OS0050_BadDecimal	"A bad value was given in II_4GL_DECIMAL.\n
The character `%0c' specified by the logical variable II_4GL_DECIMAL is not an
acceptable decimal point character.   The character `.' will be used instead.\ 
Set the INGRES logical variable II_4GL_DECIMAL to a valid decimal point
character."

/*%
** Explanation:
**	The character given in II_4GL_DECIMAL is not a valid decimal point
**	character.  This is the character you have used in 4GL source
**	statements to specify numeric literals that can have a fractional part,
**	such as money values or real numbers.  Valid decimal point characters
**	are `.' or `,'.  Important note: this logical symbol is provided
**	only for backward compatibility.  It is recommended that you use
**	a decimal point character of `.' in your 4GL source code,
**	and leave II_4GL_DECIMAL unset (it will default to `.').
**
** System Status:
**	The 4GL compiler will assume that your 4GL statements will use
**	a decimal point character of `.' to specify the fractional parts
**	of numeric literals.  If your 4GL statements in fact use some other
**	character, a syntax error will be reported.
**
** User Action:
**	Set a valid single-character string for the II_4GL_DECIMAL name.
*/

/*
**	Local Procedure Errors.
*/

E_OS0060_BadProcName	"Invalid procedure name qualification.\n
The name, '%0c.%1c', contains a period. Procedure names may not be qualified."

/*%
** Explanation:
**	The name of the procedure specified in a local procedure declaration
**	contains a period. Procedure names must begin with a letter
**	and contain only alphanumeric characters. Procedure names may not
**	be qualified.
**
** System Status:
**	The local procedure declaration will be ignored.	
**
** User Action:
**	Make sure the name of the procedure is spelled correctly.
*/

E_OS0061_DupProcDecl	"Duplicate procedure declaration.\n
The name '%0c' has already appeared in a local procedure declaration
or definition, or is the name of the main procedure or form."

/*%
** Explanation:
**	The name of the procedure specified in a local procedure declaration
**	has already appeared in an earlier local procedure declaration or
**	definition (the body of a local procedure),
**	or else it's the name of the frame's form (if this is a frame),
**	or else it's the name of the main procedure (if this is a procedure),
**
** System Status:
**	The local procedure declaration will be ignored.	
**
** User Action:
**	Make sure the name of the procedure is unique.
*/

E_OS0062_AmbiguousVar	"Ambiguous variable name.\n
The variable '%0c' has the same name as the local procedure it's declared in,
or the same name as a procedure or form to which that local procedure
is subordinate."

/*%
** Explanation:
**	The name of the variable specified in a variable declaration is also
**	the name of the local procedure in which the declaration appears,
**	or else it's the name of a "higher" local procedure (whose variables
**	are visible to the local procedure in which the declaration appears),
**	or else it's the name of the frame's form (if this is a frame),
**	or else it's the name of the main procedure (if this is a procedure),
**
** System Status:
**	The variable declaration will be processed, but subsequent references
**	to the variable may be interpreted as references to the procedure or
**	form of the same name, or vice versa.
**
** User Action:
**	Make sure the name of the variable is unique.
*/

E_OS0063_BadProcType	"Invalid procedure return type.\n
The procedure '%0c' has been declared to return a complex type."

/*%
** Explanation:
**	A local procedure has been declared to return a complex type.
**	Procedures must return a simple type or "none".
**
** System Status:
**	The procedure declaration will be processed as if a return type
**	of "none" were specified.
**
** User Action:
**	Make sure the return type of the procedure is a valid simple type
**	or "none".
*/

E_OS0064_MisplacedProcDecl "Misplaced local procedure declaration.\n
The declaration of the local procedure '%0c' appears among the parameter
declarations for '%1c'.  Local procedure declarations should appear
in the 'declare' section (following the keyword 'declare')."

/*%
** Explanation:
**	Local procedure declarations should appear in the DECLARE section
**	of the INITIALIZE statement (if they're used by a frame)
**	or in the DECLARE section of a procedure (if they're used by
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

E_OS0065_ProcNotDecl	"Procedure not declared.\n
The local procedure '%0c' is being defined, but has not yet appeared
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

E_OS0066_DupProcDefn	"Duplicate procedure definition.\n
The name '%0c' has already appeared in an earlier local procedure definition,
or is the name of the main procedure or form."

/*%
** Explanation:
**	The name of the procedure specified in a local procedure definition
**	has already appeared in an earlier local procedure definition,
**	or else it's the name of the frame's form (if this is a frame),
**	or else it's the name of the main procedure (if this is a procedure),
**
** System Status:
**	The 4GL compiler will examine the procedure definition
**	for further errors.
**
** User Action:
**	Make sure the name of the procedure is unique.
*/

E_OS0067_ProcNotDefn	"Procedure not defined.\n
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

E_OS0068_BadProcRef	"Invalid reference to local procedure.\n
A reference was made to local procedure '%0c' where a reference to a variable
or constant was expected."

/*%
** Explanation:
**	A reference was made to a local procedure where a reference to
**	a variable or constant was expected.
**
** System Status:
**	The local procedure specification will be ignored.	
**	Further error messages may appear.
**
** User Action:
**	Specify a variable or constant instead of a local procedure.
*/

/*
**	Syntax Errors.
*/

E_OS0100	"Syntax error on line '%0d'.  The last symbol read
was '%1c'."

E_OS0101	"Syntax error on line '%0d'.  The last symbol read
was '%1c'.  Expecting one of '%2c'."

E_OS0102	"Syntax error on line '%0d'.  The last symbol read
was '%1c'.  Expecting an 'ENDIF'; possibly you typed 'ELSE IF'
when you meant 'ELSEIF' in your 'IF-ELSEIF...' statement."

E_OS0103	"Operation specification syntax error on line '%0d'."

E_OS0104	"The 4GL source file does not have any definitions in it."

E_OS0105	"A ';' or '}' is missing."

E_OS0106	"Operation specification not recognized in this context."

E_OS0107	"Operation specification not recognized in this context.\n
Queries with an attached menu require assignment to a valid query object,
either the form or a table field on the form.  Otherwise, a query loop is
assumed, which can only have a block of statements as its body."

E_OS0108	"Missing block for query loop.\n
Queries must be assigned to a valid query object, either the form or a table
field on the form.  Otherwise, a query loop is assumed, which must have a block
of statements as its body."

E_OS0109	"Warning:  Not an Open SQL command.\n
The statement '%0c' is an extension to Open SQL; it may not be portable to a
non-INGRES DBMS."

#define OSNOTSQL0	E_OS0109

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
/*
** Do NOT change these characteristics of these messages:
**	1. Those which don't include section names start "<< "; those which do 
**	   start "<<* ".
**	2. Don't remove existing whitespace or introduce new whitespace; these
**	   messages are parsed by counting words.
**	3. Don't remove the whitespace between the line number and the colon.
**	4. The severity code names in S_OS010B, S_OS0231, and S_OS0235
**		must all be identical.  
**	
**	See vq!vqerrfil.c for details of how these facts are used.
*/
S_OS0233_ErrStart	"<<"
S_OS0234_SecErrStart	"<<*"
S_OS0235_WarningName	"Warning,"

S_OS010A	"<< Error, Line %d :\n"

#define	_ErrorSeverity	S_OS010A

S_OS010B	"<< Warning, Line %d :\n"

#define	_WarningSeverity	S_OS010B

S_OS010C	"<< Fatal_Error, Line %d :\n"

#define	_FatalSeverity	S_OS010C

S_OS0230	"<<* Error, Section %s Line %d :\n"

#define	_SecErrorSeverity	S_OS0230

S_OS0231	"<<* Warning, Section %s Line %d :\n"

#define	_SecWarningSeverity	S_OS0231

S_OS0232	"<<* Fatal_Error, Section %s Line %d :\n"

#define	_SecFatalSeverity	S_OS0232

/* Section name used outside all sections */
F_OS0019_NoSection	"None"

/*
**	Type or Definition Errors.
*/

E_OS0110	"An assignment statement had incompatible types between
the field or column being assigned to (on the left-hand side) and the
expression or field or column value being assigned (on the right-hand side.)"

E_OS0111 "Warning:  Nullable expression assigned to non-Nullable variable.\n
A non-Nullable field or column is being assigned a Nullable expression, which
possibly can be Null.  Use the IFNULL() function to assign Nullable values to
non-Nullable fields or columns."

E_OS0112	"Operands acted on by the operator '%0c'
had incompatible or illegal types, or were too many in number."

#define _IllegalOperands	E_OS0112

/*%
** Explanation:
**	An expression specified an operation on two or more operands that had
**	incompatible or illegal types for such an operation.  For example, a
**	date and a number are incompatible types and cannot be compared using
**	any of the relational operators such as '='.  Or as another example,
**	strings are illegal operands for any operator but '+'.
**
**	Also, an expression may have specified an operation using an ADF
**	function such as 'left()' or 'concat()' with too many operands.  For
**	example, both of the preceding functions require exactly two operands.
**
** System Status:
**	Neither the expression nor the source file will be compiled, but
**	the source file will be checked for other errors.
**
** User Action:
**	Correct the operation.
*/

E_OS0113	"An operand acted on by the operator
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

E_OS0114	"An operand acted on by the logical
operator '%0c' had a non-boolean type."

/*%
** Explanation:
**	A logical expression specified a logical operation (either AND, OR or
**	NOT) on an operand (expression) that was not a boolean type.  Boolean
**	expressions are either other logical expressions or an expression
**	specifying a relational operation (e.g., "<=" or "like".)
**
** System Status:
**	Neither the logical expression nor the source file will be compiled,
**	but the source file will be checked for other errors.
**
** User Action:
**	Correct the logical expression.
*/

E_OS0115	"The undefined symbol '%0c' was referenced
in an expression or in an assignment statement (on the right-hand side.)"

E_OS0116	"Undefined table field '%0c'.\n
The name '%0c' is being used to refer to a table field.  No table field of
that name exists on the form '%1c'."

E_OS0117	"Undefined column '%2c'.\n
The table field '%1c' on the form '%0c' does not contain the column '%2c'."

E_OS0118	"The undefined symbol '%0c' was used in an
assignment statement (on the left-hand side.)"

E_OS0119	"The field or column used in an assignment
from a prompt must be of a character type."

E_OS011A	"The symbol '%0c' was used in an assignment statement
(on the left-hand side) but cannot have a value assigned to it."

/*%
** Explanation:
**	The symbol being assigned to in an assignment statement (on the
**	left-hand side of the assignment operator, '=' or ':=') is a constant
**	or a table.  Constants have a single (constant) value assigned to them
**	by definition and cannot have other values assigned to them.  Tables
**	cannot have values assigned in this way.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Make sure that the symbol being assigned to represents a field or
**	column and then compile the source file.
*/

E_OS011B	"Warning:  Superfluous use of ':' to dereference a field
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

E_OS011C	"Operands passed to the function '%0c'
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

E_OS011D "Warning:  Possible Null used when a non-Null value is required.\n
A Nullable field or column is being used in a context that will not recognize
a Null value if the field or column should happen to contain one.  (For example,
anywhere a 4GL name can be used.)  Any such fields should be declared NOT NULL
to guarantee that this does not happen."

#define OSNULVCHK	E_OS011D

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

E_OS011E	"Illegal operand passed to a function."

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

E_OS0120	"An expression or value of a character type is required in
this context, such as in a message or prompt statement,
or as the filename in a copy statement."

/*%
** Explanation:
**	4GL requires a string-valued expression in the context being translated,
**	e.g., as the `message' for a message statement.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Correct the statement.
*/

E_OS0121	"A expression or value of integer type is required,
such as in a row number for a table field reference."

E_OS0122	"A string or integer reference is required in
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

E_OS0123	"A string value or field or column reference of
a character type is required as a `%0c'."

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

F_OS0124	"parameter keyword"

#define _PrmKeyWord	F_OS0124

F_OS0125	"table field name"

#define _TblFldName	F_OS0125

F_OS0126	"create with list"

#define _CrtWithList	F_OS0126

F_OS0127	"Forms System or DBMS reference"

#define _FrsName	F_OS0127

E_OS0128	"Illegal NULL value returned.\n
A Null value cannot be returned for the frame or procedure '%0c' since it has
been defined as NOT returning a Null value."

#define OSNULLRET	E_OS0128

E_OS0129 "Warning:  Nullable expression returned for non-Nullable procedure.\n
A Nullable expression, which possibly can be Null, is being returned for the
frame or procedure '%0c', which has been defined as NOT returning a Nullable
value.  Use the IFNULL() function to return Nullable values for non-Nullable
frames or procedures."

#define OSNULRETWARN	E_OS0129

E_OS012A	"The frame or procedure '%0c' has been
defined as NOT returning a value.  Because of this, it cannot be used
in an expression or in an assignment statement (on the right-hand side.)"

E_OS012B	"Warning:  Return value ignored.  The frame or procedure
'%0c' has been defined as returning a value,
which is being ignored."

E_OS012C	"The return value for this function '%0c' was
ignored and will result in no operation being performed."

E_OS012D	"This frame or procedure '%0c' has
been defined as NOT returning a value.  Because of this,
you cannot return a value for it."

E_OS012E	"The type of the return value is incompatible with that
defined for this frame or procedure '%0c'."

E_OS012F	"Warning:  No return value.  The frame or procedure
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

E_OS0130	"Illegal Null-valued expression.  The result
of a Null-valued expression will always be Null."

E_OS0131	"Illegal Null comparison.  Use \"IS NULL\" or \"IS NOT NULL\"
to compare for Null-values."

E_OS0132_BadWithNull	"Syntax error in copy statement. '%0c' is not a valid
value in the with null clause."

/*%
** Explanation:
**	Correct syntax is: copy table <tblname> (<colname> = <format>
**				with null (<value>) ...)
**	where <value> can be a number if <format> is numeric or a hex
**	constant like X'393939' for the string '999' or a quoted string
**	constant like 'NULL'. An unquoted string is not allowed.
**
** System Status:
**      The source file will be checked for other errors but will not be
**      compiled.
**
** User Action:
**	Correct the syntax error and re-compile the source file.
*/

/*
**	Query Errors.
*/

E_OS0150	"Warning:  Obsolete master-detail query syntax in parameter
list.  Master-detail syntax no longer uses ';' as a separator."

E_OS0151	"A '%0c' in the target list of a query
in this frame or procedure is invalid since no fields have
been defined on the form '%1c'."

E_OS0152	"The target list of a query must be fully
specified in the 4GL source file.  You cannot use fields to
dynamically specify the fields or columns of the target list at run-time."

E_OS0153	"The table field, record, or array '%0c' was illegally
referenced in the target list of a query."

E_OS0154	"The '%0c' construct cannot be used in the
target list of a query to reference a table field row."

E_OS0155	"The field or variable '%0c' was referenced in the target list
of a query.  No variable of that name is declared in a current scope,
and no field of that name exists on the form '%1c'."

E_OS0156	"The column '%0c' was referenced in
the target list of a query.  No column of that name exists
in the table field '%1c' on the form '%2c'."

E_OS0157	"The name '%0c' is not the name of the
form but is being used to refer to the form in the specification
of a query object.  The form name is '%1c'."

E_OS0158	"Too many attached queries in a parameter list.\n
Only one attached query can be passed in a parameter list."

/*%
** Explanation:
**	At the moment, 4GL only supports passing a single query (master-detail
**	or otherwise) as a parameter to an 4GL frame.  More than one such query
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

E_OS0159	"Non-existent query object, '%0c'.\n
The name '%0c' is being used to refer to a query object that does not exist on
the form '%1c'.  It is not the name of the form, nor does it name a table field
or array, or a row of a table field or array, or a record."

/*%
** Explanation:
**	A query object was expected either as the target of an assignment
**	(on the left-hand side) or as a value list (as part of a ".all")
**	in a query statement.  The name used to refer to the query object
**	is not the name of the form, nor does it name a table field on
**	the form, an array, a row of a table field or array, or a record.
**	These are the only defined query objects for a frame or procedure.
**
** System Status:
**	The query statement will be ignored, and although the source file will
**	be checked for other errors, it will not be compiled.
**
** User Action:
**	Correct the name to be a defined query object and then compile the file.
*/

E_OS015A_BadRepeat	"Warning: query is not repeatable.\n
You have specified the REPEAT (QUEL) or REPEATED (SQL) keyword on a query.
However, the query contains a variable reference in a context
which is not proper for a repeated query.
The REPEAT or REPEATED specification will be ignored."

/*%
** Explanation:
**	The REPEAT (QUEL) or REPEATED (SQL) keyword was specified on a query.
**	However, the query contains a variable reference in a context
**	which is not proper for a repeated query.  Examples of such
**	improper contexts are using a string variable to specify an entire
**	WHERE clause or to specify the name of a DBMS object (such as a
**	table or column).
**
** System Status:
**	The REPEAT or REPEATED specification will be ignored.
**	Compilation will continue.
**
** User Action:
**	If you want the query to be repeatable, use a literal in place of
**	the variable reference.  If you want to retain the variable reference
**	(thereby forcing the query to be marked non-repeatable), no action
**	is necessary (but you might want to remove the REPEAT or REPEATED
**	keyword so that the 4GL source file reflects what will actually occur,
**	and so that subsequent compilations won't get the warning message).
*/

E_OS015B_QualRepeat	"Warning: query is not repeatable.\n
You have specified the REPEAT (QUEL) or REPEATED (SQL) keyword on a query.
However, the query contains the QUALIFICATION function,
which is not proper for a repeated query.
The REPEAT or REPEATED specification will be ignored."

/*%
** Explanation:
**	The REPEAT (QUEL) or REPEATED (SQL) keyword was specified on a query.
**	However, the query contains the QUALIFICATION function,
**	which is not proper for a repeated query.
**
** System Status:
**	The REPEAT or REPEATED specification will be ignored.
**	Compilation will continue.
**
** User Action:
**	If you want the query to be repeatable, remove the QUALIFICATION
**	function.  If you want to retain the QUALIFICATION function
**	(thereby forcing the query to be marked non-repeatable), no action
**	is necessary (but you might want to remove the REPEAT or REPEATED
**	keyword so that the 4GL source file reflects what will actually occur,
**	and so that subsequent compilations won't get the warning message).
*/

E_OS015C_BadColonInSelect	"Invalid use of colon in a SELECT target.\n
In a SELECT, a target list element must be in one of the following formats:
(1) column_name,
(2) table_name_or_correlation_name.column_name,
(3) identifier = database_expression, or
(4) :field_or_variable = database_expression.
In formats (3) and (4), a colon may appear (preceding the name of a field
or variable) within the database expression.
Note that format (4) begins with a colon.
However, format (4) is NOT allowed in any of the following:
(a) a SELECT into a table field, table field row, array, array row, or record,
(b) a SELECT passed to a called frame,
(c) a subselect (e.g. in an INSERT or CREATE TABLE statement), or
(d) following the first UNION in a SELECT statement with UNIONs."

/*
**	Miscellaneous semantic and syntactic errors.
*/

E_OS016D_NestedLocalProc	"A local procedure may not be declared
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

E_OS016E_NotInDisplayLoop	"The '%0c' statement is not allowed
within a 4GL procedure or local procedure except inside a DISPLAY SUBMENU."

/*%
** Explanation:
**	The specified statement is not allowed within a 4GL procedure,
**	nor is it allowed in a local procedure in a 4GL frame,
**	unless it's inside a DISPLAY SUBMENU.
**
** System Status:
**	The statement will be ignored.
**
** User Action:
**	Remove the statement or move it to the body of a 4GL frame,
**	or move it inside a DISPLAY SUBMENU.
*/

E_OS016F_InProcOrLocalProc	"The '%0c' statement is not allowed
within a 4GL procedure or local procedure."

/*%
** Explanation:
**	The specified statement is not allowed within a 4GL procedure,
**	nor is it allowed in a local procedure in a 4GL frame.
**
** System Status:
**	The statement will be ignored.
**
** User Action:
**	Remove the statement or move it to the body of a 4GL frame.
*/

E_OS0170	"Duplicate label definition.\n
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

E_OS0171	"Undefined label.\n
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

E_OS0172	"The loop with label '%0c'
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

E_OS0173	"The '%0c' statement is not allowed
within a 4GL procedure."

/*%
** Explanation:
**	The specified statement is not allowed within a 4GL procedure.
**	(It *is* allowed, however, in a local procedure in a 4GL frame).
**
** System Status:
**	The statement will be ignored.
**
** User Action:
**	Remove the statement or change the 4GL procedure to a frame.
*/

F_OS0174	"column declaration"	/* syntactic construct */

#define _ColumnDecl	F_OS0174

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

E_OS0175	"ENDLOOP can only be used within an unloadtable or
while loop, or within a query loop or sub-menu."

E_OS0176	"UNLOADTABLE statements cannot be nested."

E_OS0177	"Warning:  Hidden columns are not defined for table fields
that are initialized dynamically."

E_OS0178	"Illegal character in text string.\n
The escaped character '\0' is not legal in a text string constant.\ 
Please consult the INGRES/SQL Reference Manual for details."

/*%
** Explanation:
**	The character '\0' was specified in a TEXT string constant in the
**	source file being compiled.  Character strings of type TEXT cannot
**	contain the character '\0' (the zero bit pattern.)  A more complete
**	discussion of this can be found in the INGRES/SQL Reference Manual.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Remove the character '\0' from the TEXT string constant and
**	then compile the source file.
*/

E_OS0179	"A qualification cannot appear in this statement."

E_OS017A	"Invalid field '%0c' in qualification.\n
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
**	Remove the hidden field from the qualification predicate.  (WHERE
**	clauses can be built in a string variable to implement dynamic queries.
**	Please consult the 4GL Reference Manual for details.)
*/

E_OS0229	"Invalid column '%0c.%1c' in qualification.\n
Hidden columns cannot be used in a qualification predicate."

/*%
** Explanation:
**	The named hidden column was listed as a qualification predicate.  Since
**	hidden columns are not known to the Forms System, they cannot contain
**	both operators and values as can visible columns in query mode.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Remove the hidden column from the qualification predicate.  (WHERE
**	clauses can be built in a string variable to implement dynamic queries.
**	Please consult the 4GL Reference Manual for details.)
*/


E_OS017B	"Invalid field '%0c' in qualification.\n
Only visible simple fields on the form can be specified in the qualification
predicate; other 4GL variable types are not supported."

#define OSBADQUAL E_OS017B

/*%
** Explanation:
**	The named 4GL variable (a table field, record, array, constant or global
**	variable) was listed as a qualification predicate.  Since these data
**	objects are not known to the Forms System, they cannot contain both
**	operators and values as can visible fields in query mode, and hence,
**	cannot be used in a qualification predicate.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Remove the invalid field from the qualification predicate.  (WHERE
**	clauses can be built in a string variable to implement dynamic queries.
**	Please consult the 4GL Reference Manual for details.)
*/

E_OS017C_SubmTooDeep	"Excessive nesting of submenus.\n
Submenus may not be nested more than 30 levels deep."

/*%
** Explanation:
**	Submenus (defined by DISPLAY SUBMENU or RUN SUBMENU)
**	may not be nested more than 30 levels deep.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Reduce the level of submenu nesting.
*/

E_OS017D_BabSubmAct	"Invalid activation in RUN SUBMENU.\n
The specified activation (e.g. on a field or alerter event) is not allowed
directly inside a RUN SUBMENU."

/*%
** Explanation:
**	The specified activation (e.g. on a field or alerter event) is not 
**	allowed directly inside a RUN SUBMENU.  (But it could appear inside
**	a DISPLAY SUBMENU that's inside a RUN SUBMENU).
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Remove the invalid activation from the list of activations
**	for the RUN SUBMENU.
*/

E_OS017E_BadResInAlert	"Invalid RESUME in an alerter activation.\n
Any RESUME inside an alerter activation must have no operands."

/*%
** Explanation:
**	Any RESUME directly inside an alerter activation must have no operands.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Remove the RESUME or change it to a RESUME with no operands.
*/

E_OS017F_MissingActs	"Missing activations.\n
A frame or submenu must have at least one non-alerter activation."

/*%
** Explanation:
**	A frame or submenu must have at least one non-alerter activation.
**	The menu or submenu either has no activations at all
**	(which probably means you have other syntax errors as well),
**	or it has only an alerter event activation.
**
** System Status:
**	The source file will be checked for other errors but will not be
**	compiled.
**
** User Action:
**	Add an activation to the frame or submenu.
*/

E_OS0200	"The range variable '%0c' has the same name
as a field on the form.  This will result in an ambiguity
in the target lists of queries and cannot be allowed."

E_OS0201	"Unqualified column attribute."

E_OS0210	"A target list element in a SELECT specified a target that was
not a field, variable, table field column, array attribute, or record attribute.
Please consult the 4GL/SQL Reference Manual for details."

E_OS0211	"An INTO list is not allowed in 4GL/SQL queries.\ 
Use a target list assignment, \"field = expression\", instead.\ 
Please consult the 4GL/SQL Reference Manual for details."

E_OS0212	"A `.all' in the values list of an INSERT statement
must correspond to a `*' in the column list."

E_OS0213	"A `*' in the column list of an INSERT statement
must correspond to a `.all' in a values list."

E_OS0214	"Unbalanced column and values lists in an INSERT statement.\n
Whenever a column list is specified for an INSERT statement, its elements
must correspond one-to-one with the elements of a VALUES clause."

E_OS0215	"Invalid WITH|NOT DEFAULT clause.\n
"

E_OS0220	"The DEFINE TEMPORARY or DEFINE PERMANENT command was
not followed by LINK."

E_OS0221	"The word `%0c' cannot appear in a DESTROY LINK statement.\ 
The word(s) `%1c' expected."

E_OS0222	"The statement ADD NODE is not supported in 4GL/QUEL."

E_OS0223	"The statement REMOVE NODE is not supported in 4GL/QUEL."

E_OS0224	"SQL transaction syntax `%0c' has been modified.\nVersion 
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

E_OS0228	"Nullability clause not allowed for complex data types.\n
Type `%0c' cannot be declared `%1c'.  (Null clause ignored.)"

/*%
** Explanation:
**	The named hidden field was declared with a NULL clause (WITH NULL
**	or NOT NULL).  NULL clauses are not allowed on complex data types.
**	(Complex objects are theoretically nullable, but there is no way to
**	assign NULL to them, and there is no way to test for NULL.)
**
** System Status:
**	This is a warning message.  The NULL clause will be ignored and
**	compilation will continue.
**
** User Action:
**	Remove the WITH NULL or NOT NULL clause from the declaration.
*/

E_OS0250	"The undefined symbol `%0c' was referenced
in an expression or in an assignment statement"

E_OS0251	"`%0c' is not a record, array, or table field.\n
The `dot' (.) operator can only be used on objects, arrays, or table fields."

E_OS0252	"`%0c' not an array or table field."

E_OS0253	"`%0c': no such attribute in `%1c'."

E_OS0254	"Table `%0c' does not exist or is not owned by you."

E_OS0255	"Invalid datatype for hidden column `%0c.%1c'.\n
A hidden column may not be declared as a complex datatype." 

E_OS0256	"Cannot declare array of type `%0c'.\n
Arrays can only be of complex datatypes."

/*
**	The following message has been superseded by E_OS0260
*/
E_OS0257	"Access to array `%0c' requires an index."

E_OS0258	"Operation `%0c' not supported on arrays.\n
`%0c' is only supported on table fields."

E_OS0259	"`%0c', of type `%1c', is contained in another object of
type `%1c'.  Complex types may not be nested."

E_OS025A_BadFormRef	"Illegal form reference.\n
The form cannot be used to qualify a data reference in this context."

/*%
** Explanation:
**	You do not need to qualify the data reference with the name of the
**	form in this context.  That is, "form . id . id . . ." is illegal
**	when referencing the data.
**
** System Action:
**	4GL will not recognize the data reference in this case.
**
** User Action:
**	Edit the 4GL source-code and remove the form name from the data
**	reference.
*/

E_OS025B_DatatypeUnsupp	"Warning: datatype encountered in a \"type of table\"
declaration that is not supported in 4GL.\n  The table  `%0c' contains at least
one column that is of an unsupported datatype. This column will not be included
in the record declaration."

/*%
** Explanation:
**	A "type of table" declaration encountered a column in the table that
**	is of a datatype that 4GL doesn't recognize (e.g. long varchar).
**	These columns will not be included in the declared record.  The rest
**	of the columns in the table will be included in the record declaration.
**
** System Action:
**	The compile will proceed.  Other compiler errors will result if the
**	bad columns are referenced as members of this declared record.
**
** User Action:
**	Create a record in ABF/VISION that has the desired attributes rather
**	than basing on the DBMS table.
*/
E_OS0260	"Record number required for array `%0c'.\n
Arrays do not define a `current' row (except within an UNLOADTABLE)
so a record number (enclosed in square brackets) is required in this context."

E_OS0261	"Global `%0c' has non-existent datatype `%1c'.\n
`%0c' is defined to have a datatype `%1c', but no such 
datatype exists for this application."

E_OS0262	"Attribute `%0c' has non-existent datatype `%1c'.\n
`%0c' is defined to have a datatype `%1c', but no such 
datatype exists for this application."

E_OS0263	"Procedure `%0c' is undefined.\n
'%0c' is undefined, and so cannot be used inside an expression.  Please check
that it is spelled correctly.  If necessary, create it with the correct
datatype."

/*%
** Explanation:
**	An undefined procedure was used inside an expression.  The compiler
**	cannot generate code for the expression, because the undefined
**	procedure's data type is unknown.
**
** System Action:
**	4GL will not be able to compile the current frame or procedure.
**	It will continue to look for other errors.
**
** User Action:
**	If the procedure's name was misspelled, correct the spelling.  
**	Otherwise, create it with the proper data type.
*/

E_OS0264		"Do not specify row number for LOADTABLE.\n
A row number was specified for LOADTABLE.  LOADTABLE always adds a row
to the end of the table field, so no row number is needed."

/*%
** Explanation:
**	LOADTABLE does not accept a row number, but one was specified.
**
** System Action:
**	4GL will not be able to compile the current frame or procedure.
**	It will continue to look for other errors.
**
** User Action:
**	Remove the row number and recompile the frame.
*/

E_OS0265		"LOADTABLE needs an assignment list\n
A LOADTABLE statement did not assign any values to columns in the new row.
This is illegal.  LOADTABLE must assign the value for at least one column."
/*%
** Explanation:
**	LOADTABLE requires at least one column assignment.
**
** System Action:
**	4GL will not be able to compile the current frame or procedure.
**	It will continue to look for other errors.
**
** User Action:
**	Add a column assignment to the LOADTABLE statement and recompile.
*/

E_OS0266_ProcInVar	"Procedure name is specified via a variable.\n
A procedure name specified via a variable cannot be used inside an expression."

/*%
** Explanation:
**	A procedure whose name was specified in a string variable
**	was used inside an expression.  The compiler cannot generate code
**	for the expression, because the procedure's data type is unknown.
**
** System Action:
**	4GL will not be able to compile the current frame or procedure.
**	It will continue to look for other errors.
**
** User Action:
**	Assign the result of the procedure call to a variable whose type
**	is compatible with all possible procedure names.  Then use
**	the variable instead of the procedure call in the expression.
**	For example:  Suppose the logic of your application ensures
**	that the variable PROCNAME (of type VARCHAR(8)) may contain
**	either 'proc1' or 'proc2'.  Suppose proc1 returns a result
**	of type SMALLINT and proc2 returns a result of type INTEGER.
**	Suppose both proc1 and proc2 take a single integer argument PARM.
**	Suppose i, j, and k are integer variables.
**	Then instead of coding
**		i = j + :procname(parm=k);
**	code
**		i = :procname(parm=k);
**		i = j + i;
*/

E_OS0267_CallProcSysFunc	"CALLPROC must not specify a system function.\n
You have coded CALLPROC %0c, but `%0c' is a system function, not a procedure."

/*%
** Explanation:
**	CALLPROC must not specify a system function.
**
** System Action:
**	4GL will not be able to compile the current frame or procedure.
**	It will continue to look for other errors.
**
** User Action:
**	Remove the keyword CALLPROC: code 
**
**		result = system_func(...)
**
**	instead of
**
**		result = CALLPROC system_func(...)
**
**	Alternatively, if you intended to override the system function
**	with a procedure of your own, create the procedure.
**	(Even if it's written in a 3GL, you must create an entry for it
**	in the ABF catalog).
*/

E_OS0268_BadTblFld	"A table field is invalid in this context.\n
Table field '%0c' encountered."

/*%
** Explanation:
**	A table field was detected in an illegal context, such as on the
**	right hand side of a positional parameter in a callproc or callframe
**	statement.
**
** System Status:
**	4GL will not be able to compile the current frame or procedure.
**	It will continue to look for other errors.
**
** User Action:
**	Correct this illegal table field reference.
*/

E_OS0269_BadDbmsinfo	"Warning: invalid use of DBMSINFO\n
The system function DBMSINFO is intended for use only inside a DBMS statement
(such as a SELECT).  When used elsewhere, it generally will return the empty
string."

/*%
** Explanation:
**	DBMSINFO is intended for use only inside a DBMS statement.
**
** System Status:
**	The 4GL compilation will continue.
**
** User Action:
**	In general, place the invocation of DBMSINFO inside a SELECT.
**	For example, if v is a VARCHAR variable, code
**
**		SELECT v = DBMSINFO('username');
**
**	instead of
**
**		v = DBMSINFO('username');
**
**	One possible exception is DBMSINFO('_bintim').  Outside a DBMS
**	statement, this has, in prior releases, returned the current time
**	on the machine which is running your application, expressed as
**	the number of seconds elapsed since Jan 1, 1970 00:00:00 G.M.T.
**	(This has never been a supported feature).  If you code
**
**		SELECT v = DBMSINFO('_bintim');
**
**	you'll get the current time on the DBMS server machine,
**	which may be different from the current time on the machine
**	which is running your application.  To get the latter time, code
**
**		v = VARCHAR(INT4(INTERVAL('sec', 'now' -
**			DATE('1970.01.01 00:00:00 gmt'))));
*/

E_OS0270_NoPurgeTableName
"No table field name for PURGETABLE statement.\n  The PURGETABLE statement
requires at least the name of a table field."

/*%
** Explanation:
**	A syntax error.  The user did not specify a table field name
**	for the purgetable statement.  User must supply at least the
**	name of a table field.
**
** System Status:
**      The source file will be checked for other errors but will not be
**      compiled.
**
** User Action:
**	The source code must be changed to provide a table field name.
*/
