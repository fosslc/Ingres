/*
**	Copyright (c) 2004, 2008 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	ereq.msg -	EQUEL/ESQL Message Definitions File.
**
** Description:
**	This file defines the messages for class EQ (EQUEL/ESQL).
**
** History:
**	 6-aug-1987 (barbara)
**		Created.
**	05-may-1988 (marge)
**		Added EQ0450-457 for forms with clause errors (EQFW Unit)
**	30-jan-88 (marian)
**		Added E_EQ0504_COMMON_WARN and added F_EQ0505_eqFL11
**		to add common sql warning messages.
**	10-feb-89 (marian)
**		Changed the wording for E_EQ0504_COMMON_WARN so it now says
**		'may not be portable' instead of 'is not portable'.
**	02-aug-89 (barbara)
**		Added FIPS warnings messages.
**	10-aug-89 (teresal)
**		Added new message (EQ015A) for bad ACTIVATE FRSKEY number.
**	18-aug-89 (sylviap)
**		Added E_EQ0505_NO_SQLDA	for DG ESQL/FORTRAN.
**	12-sep-89 (sylviap)
**		Changed EQ0126 to E_EQ0126_sqCOMMA.  Decided the more frequent
**		error would be to forget the comma in a WITH clause than to 
**		misspell ASC or DESC when creating an index.  (Bug 7871)
**	26-sep-89 (sylviap)
**		Changed E_EQ0505_NO_SQLDA to E_EQ0125_NO_SQLDA so as not to
**		collide with the FIPS error messages.
**      09-oct-89 (sylviap)
**              B 8174: Added E_EQ006A_grxBADWORD for DG bug.  Open SQL (DG)
**              does not allow multiple connections.  Error is very similar
**              to E_EQ0069, but no keyword is assumed.
**	31-oct-89 (marian)
**		Replace RTI with Ingres Corp.
**	01-nov-89 (marian)
**		Remove Corp. from Ingres Corp.
**	12-dec-89 (barbara)
**		Added messages for alerters.
**	11-jan-90 (barbara)
**		Added E_EQ0510_OPEN_FUNC for all non-Open SQL function
**		references.
**	13-mar-90 (barbara)
**		Added E_EQ000A_TRAIL_FLAG message for bug fix #7282.
**	14-nov-1989 (fredv)
**		Integrated ingresfe change 80447.  Changed RTI to INGRES.
**	10-oct-1990 (kathryn) Bug 33032
**		Added E_EQ0511_FIPS_SUBCLAUSE.
**	11-oct-1990 (kathryn) 
**		Added E_EQ0512_OPEN_CLAUSE.
**	23-oct-1990 (mgw)
**		Corrected some misspellings. All are in comments, not messages,
**		but these comments get picked up for the messages.txt file via
**		erxtract and are thus in a deliverable.
**	14-feb-1991 (teresal)
**		Added E_EQ0513_OPEN_OP.
**	05-mar-1991 (barbara)
**		Added E_EQ0378_repWHERE to flag use of a dynamic WHERE clause 
**		on a REPEATED statement.
**	15-mar-1991 (kathryn)
**		Added E_EQ0090_HDL_TYPE_INVALID.
**	02-apr-1991 (barbara)
**		Added E_EQ0091_COPYHANDLER to flag missing copyhandler
**		clause on COPY FROM/INTO PROGRAM statement.
**	19-jul-1991 (teresal)
**		Changed EVENT to DBEVENT.
**	05-aug-1992 (teresal)
**		Added some more FIPS warning messages for an illegal
**		identifier.
**	03-oct-1992 (teresal)
**		Add messages for esql help file for new "-wsql=" command
**		flag.
**	06-nov-1992 (lan)
**		Added FIPS warning messages E_EQ050C_FIPS_DATTYP and
**		E_EQ050D_FIPS_VAR.
**	18-nov-1992 (Mike S)
**		Added messages needed by EXEC 4GL.
**	18-nov-1992 (larrym)
**		Added E_EQ050E_NOSQLCA to flag usage of FIPS SQLCODE before
**		issuance of INCLUDE SQLCA.
**	27-nov-1992 (larrym)
**		Modified E_EQ0517_SQLSTATE_DECL to E_EQ0517_SQLSTATUS_DECL
**		to allow it to be used for both SQLSTATE and SQLCODE.
**	16-dec-1992 (larrym)
**	    Added fast messages F_EQ0021_eqFL25 and F_EQ0022_eqFL26.
**	14-jan-1993 (lan)
**	    Added FIPS warning message E_EQ050F_FIPS_USE.
**	    Also replaced all references to -wfips with -wsql=entry_sql92.
**	24-feb-1993 (lan)
**		Added E_EQ0500_OPEN_SUBCLAUSE. 
**	26-apr-1993 (lan)
**	    Added FIPS warning message E_EQ0500_FIPS_UNIONSEL.
**	20-may-1993 (kathryn)
**	    Change message number E_EQ0500_FIPS_UNIONSEL to E_EQ051D. The 
**	    message numder E_EQ0500 was previously allocated for another
**	    error.
**	24-may-1993 (lan)
**	    Added FIPS warning message E_EQ051E_FIPS_CINITVAL.
**	25-jun-1993 (kathryn)
**	    Added FIPS warning message E_EQ051F_FIPS_ESQLFLAG.
**	8/93 (Mike S)
**	    Add error E_EQ0520_NOT_BYREF.
**	20-aug-1993 (sandyd)
**	    Massaged the wording of the FIPS flagger messages to make
**	    them a bit more diplomatic (and more precise).  (Deactivated
**	    E_EQ051E_FIPS_CINITVAL, which is now obsolete.  It was an
**	    SQL89 issue, no longer valid in SQL92 entry level.)
**	14-apr-1994 (teresal)
**	    Added messages for ESQL/C++ help including '-extension='.
**	    Also, added ESQL/C new 'prototypes' flag help message.
**	28-mar-00 (inkdo01)
**	    Added EQ006B for constraint WITH clause support.
**	6-sep-02 (toumi01)
**	    Added message F_EQ0026 for eqcbl/esqlcbl -F free format flag.
**	15-mar-04 (nansa02) (Problem:-INGEMB119 & bug 111963)    
**          Added slow messages F_EQ0035_eqFL31 
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**	8-dec-05 (inkdo01)
**	    Added EQ0232 for bad escape sequences in Unicode literals.
**	12-feb-2007 (dougi)
**	    Added EQ0430, EQ0431 for scrollable cursors.
**	16-aug-2007 (dougi)
**	    Added EQ0233 for date/time literals.
**	20-Jun-2008 (hweho01)
**	    Added F_EQ0036 into the esqlf/eqf help menu for 64-bit   
**	    Fortran file on hybrid platforms.   
**/

# define	_EQ_CLASS	140



E_EQ0001_eqNOMEM	"Fatal memory allocation error in routine '%1c'.\n
The product has run out of memory after requesting %0c bytes for internal data."

/*%
** Explanation:
**	A memory allocation or free error has occurred somewhere in
**	the code in a place where it was unexpected, or where it
**	was not possible to recover from the error.  This may indicate an
**	internal coding error or a program that is too large. 
**
** System Status:
**	The preprocessor will abort.
**
** User Action:
**	Check your system quotas.  If you suspect they might be too low, 
**	try to break up the input file or the embedded language blocks.
**	If the preprocessor still fails, call your INGRES support person.
*/



E_EQ0002_eqNOTABLE	"Fatal attempt to access internal table.\n
No host code generator table is defined in the preprocessor."

/*%
** Explanation:
**	A required internal data structure could not be accessed.  This
**	may be caused by an incorrectly linked preprocessor image or an
**	internal coding error.
**
** System Status:
**	The preprocessor will abort.
**
** User Action:
**	Contact your INGRES support person.
*/

E_EQ0003_eqNULL "Null type pointer in routine '%0c'.\n
An internal symbol has not been initialized or has been reset." 

/*%
** Explanation:
**	An internal symbol can not be accessed.   This may be caused by
**	an internal coding error.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	If the preprocessor reported syntax errors, correct them and
**	repreprocess your file.  If the error occurs again, contact your
**	INGRES support person.
*/

E_EQ0004_eqTRACEFL	"Conflicting %0c '-to' and '-tl' options."

/*%
** Explanation:
**	This error is caused by an inconsistent use of the internal trace
**	-t flag when invoking the preprocessor.  Under normal circumstances
**	it should not be necessary to use trace flags.
**
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Retype the preprocessor command line using the correct flags.
*/

E_EQ0005_eqINFILE	"%0c input file '%1c' cannot be opened.\n
Error in location or name of file or you have no read permission."

/*%
** Explanation:
**	The preprocessor cannot find the input file named on the command
**	line.  The location/directory path specified may be incorrect,
**	the file name itself may be incorrectly entered, or the file may
**	be non-existent.  An extremely long path name will also cause this
**	error.
**	
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Retype the preprocessor command line correctly specifying the 
**	input file.
*/

E_EQ0006_eqOUTOPT	"Conflicting %0c '-f' options.\n
You cannot direct both to the standard output channel and to a file."

/*%
** Explanation:
**	This error occurs when the -f flag is used twice on the command line,
**	once without an argument and once with a file name.  For example:
**		preproc -f -foutfilename
**	Note that if the the -f flag is not used, the preprocessor will
**	create an output file with the default name.
**	
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Select the appropriate output option and retype the preprocessor
**	command line.
*/

E_EQ0007_eqOUTFILE	"Cannot open %0c output file '%1c'.\n
Check write permission and -f flag argument, if used."

/*%
** Explanation:
**	Output files cannot be opened if a user does not have write
**	privileges.  If the -f flag was specified, the location or directory
**	path given must be a legal location.  There may be other system 
**	dependent reasons why a file cannot be opened.
**
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**	
** User Action:
**	Attempt to open a file with the same name at the system level and take
**	note of the system response.
*/

E_EQ0008_eqLISTFILE	"Cannot open %0c listing file '%1c'.\n
Check write permission."

/*%
** Explanation:
**	The listing file is opened in the same directory as the input
**	file, and the user must have write permission in this location.
**	There may be other system dependent reasons why a file cannot be
**	opened.
**
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**	
** User Action:
**	Attempt to open a file with the same name at the system level and take
**	note of the system response.
*/

E_EQ0009_eqBADOPT	"Invalid or missing %0c option: %1c"

/*%
** Explanation:
**	The user has invoked the preprocessor with a non-existent flag
**	or missing command line option.
**
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Retype the preprocessor command line with the correct flags.
*/

E_EQ000A_TRAIL_FLAG	"Trailing flags on the preprocessor command line are
ignored.  The preprocessor reads only those flags that are followed by a file
name or names."

/*%
** Explanation:
**	Command line flags for the preprocessor must precede the
**	filename.  There can be more than one file name, each preceded
**	by its own set of flags.  Any trailing flags are ignored.
**
** System Status:
**	The preprocessor ignores the trailing flags.
**
** User Action:
**	Make sure that all applicable flags precede the file name.
*/

E_EQ0010_eqCAPAB	"No INGRES capability for the %0c/%1c preprocessor."

/*%
** Explanation:
**	Based on the installed INGRES authorization key you are not allowed
**	to use this preprocessor.
**	
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Check with your INGRES system administrator for preprocessor
**	usability.
*/

E_EQ0011_eqSAMEFL1	"Input and Output files are both %0c."

/*%
** Explanation:
**	
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Retype the preprocessor command line specifying different input
**	and output files.
*/

E_EQ0012_eqSAMEFL2	"Input and Listing files are both %0c."

/*%
** Explanation:
**	
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Retype the preprocessor command line specifying different input
**	and listing files.
*/

E_EQ0013_eqSAMEFL3	"Output and Listing files are both %0c."

/*%
** Explanation:
**	
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Retype the preprocessor command line specifying different output
**	and listing files.
*/

E_EQ0051_grDISPTL	"DISPLAY statement target list is not supported.\n
To retrieve run-time information from the FRS, use the INQUIRE_FRS command."

/*%
** Explanation:
**	The INGRES version 2.1 DISPLAY statement arguments (such as _FIELD
**	and _TYPE) are no longer supported and you may not use a target
**	list in a DISPLAY statement.  The INQUIRE_FRS statement allows you
**	to retrieve the equivalent information.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the syntax of the DISPLAY statement in the embedded program.
*/

E_EQ0052_grFMFUNC	"Illegal context for Forms '%0c' function.\n
You have incorrectly placed the '%0c' function within another statement."

/*%
** Explanation:
**	The forms functions, such as GETOPER, which retrieves relational
**	operators from fields and table fields, may be legally used as follows:
**		GETFORM form (var = GETOPER(field), ...)
**		GETROW form table [row] (var = GETOPER(column), ...)
**	This also applies to the PUTOPER function.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the syntax of the current statement.
*/

E_EQ0053_grFMINTFUNC	"Forms '%0c' function is integer valued.\n
You must use an integer variable for the result of this function."

/*%
** Explanation:
**	The variable used to receive the result of the above FRS function 
**	has a type other than integer.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the current syntax to use an integer variable which
**	has been declared to the preprocessor.
*/

E_EQ0054_grNOPARENS	"Empty parentheses are ignored.\n
No parentheses are required on empty target lists."

/*%
** Explanation:
**	Some statements have optional target lists.  In some cases, when
**	empty target lists are used, this warning is displayed. 
**
** System Status:
**	The preprocessor will ignore the parentheses and will return
**	a SUCCESS status if no other errors occur during preprocessing.
**
** User Action:
**	Remove the unnecessary parentheses.
*/

E_EQ0055_grFMPARAM	"Illegal context for PARAM clause in forms statement.\n
You may not use the PARAM clause in this statement."

/*%
** Explanation:
**	In the languages that support PARAM target lists, not all forms 
**	target lists have a PARAM equivalent.  The statements that have
**	a PARAM equivalent are documented in the preprocessor manual.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Rewrite the forms target list using host variables declared to the
**	preprocessor instead of the PARAM clause.
*/

E_EQ0056_grFORMNAME	"Form name required for %0c statement.\n
The %0c statement used outside of a DISPLAY loop requires a form name."

/*%
** Explanation:
**	A forms I/O statement that is syntactically outside of a DISPLAY
**	loop requires a form name specification.   Inside the DISPLAY
**	loop the currently displayed form is assumed if no form is
**	specified.  This error may also occur if the DISPLAY statement
**	contained a syntax error.
**	
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Check the syntax on the DISPLAY statement.  Otherwise, include a 
**	form name in the statement, or move the statement inside a DISPLAY loop.
*/

E_EQ0057_grHOSTVAR	"'%0c' expected to be a simple variable.\n
Use a scalar numeric or character string variable instead of '%0c'."

/*%
** Explanation:
**	The named identifier was used in an I/O statement, but was not
**	declared to the preprocessor as a scalar numeric or character
**	string variable.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Declare the identifier as a variable to the preprocessor.
*/

E_EQ0058_grINLOOP	"The %0c statement must be in a %1c loop.\n"

/*%
** Explanation:
**	Some statements (such as ENDLOOP) that syntactically belong inside
**	a specific loop or block cannot be issued outside the loop or
**	or block, as they generate host language GOTO statements.  Also, a
**	syntax error on the block or loop statement will cause this error
**	to occur on subsequent statements.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct any bad syntax on the block (ie, DISPLAY) statement.
**	Otherwise, move the statement producing the error inside the correct
**	block statement.
*/

E_EQ0059_grINT	"Integer variable, constant or literal expected.\n
The identifier '%0c' was used instead of an integer value."

/*%
** Explanation:
**	The current statement requires an integer value or variable.  You 
**	have used a non-integer variable or an identifier which has not been
**	declared to the preprocessor as a variable.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Substitute an integer value or a variable which has been declared
**	to the preprocessor.
*/

E_EQ005A_grBYREF "BYREF parameters may not be coded in an EXECUTE PROCEDURE\n
with a RESULT ROW clause."

/*%
** Explanation:
**	The current statement is an EXECUTE PROCEDURE which contains both
**	BYREF parameters and a RESULT ROW clause. This combination is not
**	permitted.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the BYREF designation from all parameters if a RESULT ROW is
**	being returned by the procedure.
*/

E_EQ005B_grEXLOOP "An EXECUTE PROCEDURE loop may only be coded if the\n
EXECUTE PROCEDURE statement includes a RESULT ROW clause."

/*%
** Explanation:
**	The current statement is an EXECUTE PROCEDURE which includes a
**	BEGIN/ENDEXECUTE block, but no RESULT ROW clause. The block may
**	only be coded if the statement includes a RESULT ROW clause.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Either remove the BEGIN/ENDEXECUTE block or add a RESULT ROW 
**	clause to the EXECUTE PROCEDURE statement.
*/

E_EQ0060_grINTSTRVAR	"Integer or character string expected.\n
Type of variable '%0c' should be an integer or a character string."

/*%
** Explanation:
**	The current statement requires an integer or character string
**	variable or value.  You have used a variable of some other type.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Substitute an integer or character string (variable or literal).
*/

E_EQ0061_grINTVAR	"Type of variable '%0c' should be an integer.\n"

/*%
** Explanation:
**	You have used a variable of some other type than integer.  The
**	current statement requires the use of an integer variable.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Substitute an integer variable for the wrongly-typed variable.
*/

E_EQ0062_grNAME	"Variable '%0c' used when name expected.\n
Try dereferencing the variable name by preceding it with a '#'."

/*%
** Explanation:
**	This is a common error in EQUEL, where an object name (ie, database
**	column or form field) has the same name as a numeric variable.
**	The preprocessor will never issue this error when a character string 
**	variable is used because object names can be stored inside string 
**	variables.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Dereferencing the variable with the # sign will cause the name
**	to be used and not the contents of the named variable.
*/

E_EQ0063_grNESTQUEL	"Illegal nesting of database statements.\n
The '%0c' statement appears in a data retrieval query loop."

/*%
** Explanation:
**	This error occurs when a database query is coded within the
**	scope of a database data retrieval loop statement (SELECT or RETRIEVE).
**	The statements that may be placed within the data retrieval loop are
**	forms statements, the INQUIRE/SET status statements and the loop break 
**	statements (ENDSELECT or ENDRETRIEVE).
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Move the flagged statement outside the scope of the data retrieval
**	loop.
*/

E_EQ0064_grQRSTRVAR	"'%0c' expected to be a character string variable.\n
Query text can be replaced only by string variables."

/*%
** Explanation:
**	When replacing parts of a query, such as a WHERE clause, the
**	specified replacement must be a string variable.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Replace the variable marked as an error with a string variable
**	declared to the preprocessor.
*/

E_EQ0065_grRETREPEAT	"Incorrect use of '@' on REPEAT statement variable.\n
The '@' is a valid prefix only on variables that replace QUEL constants."

/*%
** Explanation:
**	A common error in the REPEAT RETRIEVE statement is to use the
**	@ sign on result variable names.  The @ sign can only prefix variables
**	to be sent to the database as constant values.  In the RETRIEVE case,
**	the @ sign can be correctly used in the WHERE clause.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the @ sign from variables that do not replace QUEL constants.
*/

E_EQ0066_grSTR	"String constant expected instead of '%0c'."

/*%
** Explanation:
**	Either a numeric variable or an unquoted name was used instead
**	of a string variable or quoted string.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use quotation marks around the flagged name or a string variable.
*/

E_EQ0067_grSTRVAR	"Object named '%0c' should be a character string variable."

/*%
** Explanation:
**	The current statement requires the use of a character string variable
**	to retrieve results or replace a string literal.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Substitute a character string variable, declared to the preprocessor,
**	for the name flagged as an error.
*/

E_EQ0068_grUNLOADTB	"Illegal nesting of UNLOADTABLE statements.\n
You may not code an UNLOADTABLE statement within the scope of another
UNLOADTABLE statement."

/*%
** Explanation:
**	Because of run-time implementation considerations, an UNLOADTABLE
**	statement cannot appear within the block of another UNLOADTABLE.
**	Note that the dynamic nesting of UNLOADTABLE statements (ie, via
**	a subroutine call) is also not allowed and will cause a run-time error.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Code UNLOADTABLE statements consecutively instead of nesting them.
*/

E_EQ0069_grBADWORD	"Illegal word '%0c' in %1c statement.\n
The keyword %2c is assumed."

/*%
** Explanation:
**	Some statements have words that are reserved only within the
**	context of the statement.  Misspelling or misplacing the word
**	will generate the above message.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Update the statement to use the correct keyword.
*/

E_EQ006A_grxBADWORD     "Illegal word '%0c' in %1c statement."

/*%
** Explanation:
**      Some statements have words that are reserved only within the
**      context of the statement.  Misspelling or misplacing the word
**      will generate the above message.
**
** System Status:
**      Upon completion, the preprocessor will return a FAIL status.
**      A "SYNTAX ERROR...." is generated in the source code.
**
** User Action:
**      Update the statement to use the correct keyword.
*/

E_EQ006B_WITH_NOCONS	"Constraint WITH clause doesn't follow constraint definition."

/*%
** Explanation:
**      The UNIQUE/PRIMARY KEY/FOREIGN KEY constraint WITH clause must
**	immediately follow the constraint definition. The syntax as coded
**	in this statement is incorrect.
**
** System Status:
**      Upon completion, the preprocessor will return a FAIL status.
**      A "SYNTAX ERROR...." is generated in the source code.
**
** User Action:
**      Update the statement to properly use the constraint WITH clause.
*/


E_EQ0070_grMISSWORD	"Missing word '%0c' in %1c statement."

/*%
** Explanation:
**	Some statements have "noise" words or operators that are expected
**	in some contexts.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**  	Update the statement to reflect the correct syntax.
*/

E_EQ0071_grASSGNOP	"Assignment operator should be '=', not '%0c'."

/*%
** Explanation:
**	This statement requires the use of the equals sign to indicate
**	that the value from a field in a form is to be assigned to a 
**	program variable.  Some older versions of the preprocessor allowed
**	other words (IS, BY) in this context.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the statement to use the '=' assignment operator.
*/

E_EQ0072_grNONULL	"Incorrect use of indicator variable.\n
Indicators may be used only in conjunction with host program variables."

/*%
** Explanation:
**	This error occurs when a null indicator is used with a variable 
**	or identifier representing an object name.  Null indicators may be 
**	used only on I/O statements in conjunction with program variables
**	transmitting data.  Their purpose is to send/receive null data
**	to or from the database or a form .
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the flagged null indicator from the current statement.
*/

E_EQ0073_grINDVAR	"Indicator variable '%0c' must be a 2-byte integer."

/*%
** Explanation:
**	You have used an indicator variable which is either a non-integer
**	or is the wrong size.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use an indicator variable which has been declared to the preprocessor
**	as a 2-byte integer.
*/

E_EQ0074_grMISVAR	"Incorrect use of variable '%0c' in '%1c' statement."

/*%
** Explanation:
**	You have used a variable together with names or string literals
**	in a context where the preprocessor expects either a single 
**	character variable, a string literal, or one or more unquoted
**	identifiers.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the syntax so that the clause in question appears as
**	unquoted identifiers, a quoted string literal or a string
**	variable.
*/

E_EQ0075_grFUTUNSUPP	"The '%0c' statement will be discontinued.\n
Future releases of the preprocessor will not support this statement."

/*%
** Explanation:
**	Refinements to the query language and forms system statements
**	are made from time to time.  Functionality is always preserved
**	by offering alternate statements.
**
** System Status:
**	The preprocessor will preprocess the current statement correctly
**	and will return a SUCCESS status if no other errors occur during 
**	preprocessing.
**
** User Action:
**	See the manual for an alternative to the statement you have used.
*/

E_EQ0076_grNOWUNSUPP	"The '%0c' statement is not supported.\n
Its use has been discontinued or is not supported in this host language." 

/*%
** Explanation:
**	You have used a statement which is not supported in this release.
**	The statement has either been superseded by an alternate statement
**	or was never available in preprocessors in the particular host
**	language.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the current statement and repreprocess.
*/

E_EQ0077_grISUNSUPP	"This style of %0c statement is not supported.\n
The syntax of this statement has been updated."

/*%
** Explanation:
**	The statement is one that previously supported an old and
**	a new syntax.  The old syntax is not supported any more.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Refer to the manual for the correct syntax of this statement.
*/

E_EQ0078_grSTYUNSUPP	"This style of %0c statement will be discontinued.\n
In future releases, the preprocessor will expect an updated syntax."

/*%
** Explanation:
**	The statement has been supported in two forms.  In future, the old
**	syntax will not be supported.
**
** System Status:
**	The preprocessor will preprocess the current statement correctly
**	and will return a SUCCESS status if no other errors occur during 
**	preprocessing.
**
** User Action:
**	See the manual for the alternative syntax for this statement.
*/

E_EQ0079_grCONNECTID	"Incorrect WITH clause containing the distributed
transaction id."

/*%
** Explanation:
**	The 'with' clause on a connect statement is incorrectly
**	specified.  When  the 'with' clause is used to pass the
**	distributed transaction id to the dbms for reassociation,
**	there must be no other information in the with clause.
**	Distributed transaction ids must be represented as
**	integer variables or values.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	See the manual for the correct syntax for this statement.
*/

E_EQ0080_TL_STMTID    "Illegal internal statement id encountered in processing a target list."

/*%
** Explanation:
**    This is an internal coding error.
**
** System Status:
**    Preprocessing continues.  No code is generated for the current
**    statement.
**
** User Action:
**    Contact your Ingres support person.
*/

E_EQ0081_TL_ATTR_UNKNOWN      "The attribute '%0c' named in the target list is unknown."

/*%
** Explanation:
**    The preprocessor does not recognize the attribute named in the
**    target list of the current statement.  You may have misspelled
**    the attribute's name.  Check the SQL Reference Manual for the
**    correct spelling and syntax of the current statement.
**
** System Status:
**    No code will be generated for this particular target list element
**    in the current statement.  Code for the statement as a whole will be
**    generated and the preprocessor will continue preprocessing the
**    input.
**
** User Action:
**    See the manual for the correct syntax for this statement.
**    Correct the name of the attribute and repreprocess the file.
*/

E_EQ0082_TL_ATTR_INVALID      "The attribute '%0c' is invalid in the scope of
the current statement."

/*%
** Explanation:
**    The preprocessor recognizes the attribute named in the target
**    list but cannot process it in the context of the current statement.
**    The attribute is only legal in the the target list of some
**    other statement.
**
** System Status:
**    No code will be generated for this particular target list element
**    in the current statement.  Code for the statement as a whole will be
**    generated and the preprocessor will continue preprocessing the
**    input.
**
** User Action:
**    Check the manual for the names of the attributes allowable as
**    target list elements of the current statement.  Correct the
**    attribute name and repreprocess the file.
*/

E_EQ0083_TL_TYPE_INVALID      "The type of attribute '%0c' is not compatible with the type of the program variable or value."

/*%
** Explanation:
**    The type of the attribute named in the target list is not
**    compatible with the type of the associated program variable or value.
**    Check that the variable has been declared in a declare section.
**    In general, numeric attributes must be matched with numeric variables
**    or values and character attributes must be matched with character
**    variables.
**
** System Status:
**    No code will be generated for this particular target list element
**    in the current statement.  Code for the statement as a whole will be
**    generated and the preprocessor will continue preprocessing the
**    input.
**
** User Action:
**    Ensure that the variable has been declared in a declare section.
**    Check the manual for the type of the attribute and supply a
**    compatible user variable or value.  Repreprocess the file.
*/

E_EQ0084_TL_ATTR_DUP  "The attribute '%0c' has already appeared in the target
list of the current statement."

/*%
** Explanation:
**    In a single target list, you may specify each attribute only once.
**    The attribute mentioned in the error message has been specified more
**    than once in the target list in the current statement.
**
** System Status:
**    No code will be generated for this particular target list element
**    in the current statement.  Code for the statement as a whole will be
**    generated and the preprocessor will continue preprocessing the
**    input.
**
** User Action:
**    Remove the element containing the duplicate attribute from the
**    target list.  Repreprocess the file.
*/

E_EQ0090_HDL_TYPE_INVALID "Illegal SET HANDLER value '%0c'.  Function name or 0 expected."
/*%
** Explanation:
**    The attribute named in the target list of the SET_SQL statement is a
**    user defined handler with a specified value of something other than
**    a function name or zero.
**    The only acceptable values for user defined handlers are zero, or the
**    function name. 
**
** System Status:
**    Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**    Change the value specified in the SET_SQL statement.  To "set" the 
**    handler specifiy the function name, to "unset" the handler specify "0".
**    Repreprocess the file.
*/

E_EQ0091_COPYHANDLER "The COPY FROM/INTO PROGRAM statement requires the specification of a user-defined handler in the WITH clause."

/*%
** Explanation:
**	Copying data from/to a program requires the name of a user-defined
**	function to be specified on the COPY statement, i.e.,
**	"COPY TABLE tbl () FROM PROGRAM WITH COPYHANDLER = myfunc".
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Change the COPY FROM/INTO PROGRAM statement to specify the name
**	of a user-defined handler in the WITH clause.
*/

E_EQ0125_NO_SQLDA	"The SQLDA structure is not supported in Embedded
SQL/FORTRAN.  The INCLUDE SQLDA statement is ignored." 

/*%
** Explanation:
**	F77 does not support data structures so in some environments
**	ESQL/FORTRAN cannot support the SQLDA.
**
** System Status:
**	Pre-processing continues. The INCLUDE SQLDA statement is ignored.
**
** User Action:
**	Take the INCLUDE SQLDA statement out of the program.
*/



E_EQ0126_sqCOMMA	"Missing comma between list elements in a WITH clause."

/*%
** Explanation:
**	In a WITH clause, the different elements need to be separated by 
**	commas.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Insert a comma to separate the list elements.
*/

E_EQ0127_sqEXECMD	"Statement should begin 'EXEC %0c %1c'."

/*%
** Explanation:
**	Database query and status statements should begin "EXEC SQL" 
**	and forms statements should begin "EXEC FRS".
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Insert the correct statement demarker: "EXEC SQL" for database
**	query and status statements and "EXEC FRS" for forms statements.
*/

E_EQ0128_sqNOSQLCA	"No INCLUDE SQLCA statement seen so far.\n
An INCLUDE SQLCA statement must precede a WHENEVER statement."

/*%
** Explanation:
**	In response to the WHENEVER statement, the preprocessor generates
**	code which accesses fields in the SQLCA.  Therefore you must have
**	included an SQLCA in your program module via the INCLUDE SQLCA
**	statement.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Put in an INCLUDE SQLCA statement preceding the first SQL statement.
*/

E_EQ0129_sqVARCOLON	"Variable name must be prefixed by a colon ':'.\n
The current SQL/FRS statement contains a variable used without a colon."

/*%
** Explanation:
**	All variable names used with Embedded SQL and FRS statements must
**	be prefixed by a colon to indicate to the preprocessor that they
**	are variables and not object names.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Prefix all variables in embedded SQL/FRS statements with a colon.
*/

E_EQ0130_sqWCOND	"Illegal WHENEVER condition '%0c'.\n
The conditions must be SQLERROR, SQLWARNING, SQLMESSAGE, DBEVENT or NOT FOUND."

/*%
** Explanation:
**	The WHENEVER statement is used to test the status of SQLCA
**	variables to ascertain if the last executed SQL statement resulted
**	in an error (SQLERROR), a warning (SQLWARNING) or no data matching
**	the query (NOT FOUND).  In addition the SQLMESSAGE condition ascertains
**	if any messages returned from the EXECUTE PROCEDURE statement,
**	and the DBEVENT condition checks for database event information 
**	returning from the dbms.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the WHENEVER statement to reflect the correct condition.
*/

E_EQ0131_sqWSTOP	"Incorrect WHENEVER NOT FOUND statement.\n
The WHENEVER NOT FOUND statement may not specify the STOP action."

/*%
** Explanation:
**	The WHENEVER NOT FOUND is commonly used to break out of a
**	CURSOR FETCH loop.  This is not a useful place to stop execution
**	of a program.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Change the specified action to CONTINUE, GOTO a label or CALL
**	a procedure.
*/

E_EQ0132_sqILLSTRUCTEXPR	"Illegal use of record variable.\n
A record (or structure) variable may not be a component of an expression."

/*%
** Explanation:
**	This error occurs when a record variable is used as part of an 
**	expression, ie, "1 + :record_var".  Record variables may be used 
**	in the INSERT, SELECT and FETCH statements but they must be used alone.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use an elementary variable in the expression.
*/

E_EQ0133_sqNOFROM	 "The SELECT statement requires a FROM clause.\n
You may not use a WHERE, GROUP BY, HAVING or FOR UPDATE clause in a SELECT 
statement without a FROM clause."

/*%
** Explanation:
**	In order for a WHERE or any of the other above clauses to qualify
**	the results of a query, a table name must be specified in a FROM clause.
**	The FROM clause is optional if you only SELECT constants.
**	
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Insert a FROM clause into the current query.
*/

E_EQ0134_sqNAME	"Name or string variable expected instead of '%0c'.\n
Do not use the colon if you mean to specify just a name."

/*%
** Explanation:
**	A numeric variable has been used where a string variable or a
**	simple name was expected. 
**	
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Change the statement to use a string variable (preceded by a colon)
**	or a simple name (with no colon).
*/

E_EQ0135_sqXACT	"SQL transaction syntax '%0c' has been modified.\n
Version 5.0 syntax (BEGIN/END TRANSACTION, ABORT) should be updated.
Ignore this warning if you intend to use the run-time SET AUTOCOMMIT option."

/*%
** Explanation:
**	The SQL version 5.0 syntax was used (BEGIN TRANSACTION, END
**	TRANSACTION, ABORT).  The BEGIN TRANSACTION will generate a run-time
**	error too, as an SQL program is implicitly inside a transaction.
**	The END TRANSACTION should be replaced by COMMIT, and the ABORT by
**	ROLLBACK.  This warning can be ignored if you intend to use the
**	SET AUTOCOMMIT option at run-time, implying that BEGIN TRANSACTION
**	is necessary.
**	
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Change the statement to use the SQL transaction syntax.
*/

E_EQ0151_fsBAD	"Unknown identifier '%0c' in SET/INQUIRE statement.\n
The object-type or FRS constant must be a legal name (not in a variable)."

/*%
** Explanation:
**	You have used an object-type or constant that is unknown to the
**	preprocessor in an INQUIRE_FRS or SET_FRS statement.  You may not
**	use a variable name to specify the object you wish to inquire about
**	or set.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Check the manual for the FRS objects which may be legally inquired
**	about or set.
*/

E_EQ0152_fsCONSTSYN	"Illegal SET/INQUIRE target list syntax '%0c(%1c)'.\n
A non-existent FRS object '%1c' has been specified."

/*%
** Explanation:
**	This error occurs when a non-existent FRS object is used with a
**	legal FRS constant.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Check the manual for the FRS objects which may be legally inquired
**	about or set.
*/

E_EQ0153_fsREQOBJ	"SET/INQUIRE constant '%0c' requires an object."

/*%
** Explanation:
**	This error occurs when a legal FRS constant has been used but
**	the FRS object has been omitted.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Refer to the manual for the list of objects, one of which must
**	follow the FRS constant.
*/

E_EQ0154_fsNOTOBJ	"Object name is not allowed for constant '%0c'.\n
Some SET/INQUIRE objects have no associated constant values."

/*%
** Explanation:
**	This error occurs when an object name is used with an FRS constant,
**	but the FRS constant is not one which takes an object.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the FRS object name from the statement.
*/

E_EQ0155_fsMAXPAR	"The maximum number of FRS object parent names is %0c.\n
You have used an extra form or table name in a SET/INQUIRE statement."

/*%
** Explanation:
**	Some INQUIRE/SET statements require no parent-name, some require
**	a form name, and some a form name and a table name.  Refer to
**	the manual for the correct syntax.
**	
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the extra parent name from the INQUIRE/SET statement.
*/

E_EQ0156_fsNOSET	"Constant '%0c' is only for the INQUIRE statement.\n
You have tried to SET constant '%0c'."

/*%
** Explanation:
**	There are many FRS constants (ie, field names and types) which cannot be
**	set.  The common FRS features that may be set are display attributes 
**	for fields, validations and mapping of keys.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Refer to the manual for a list of settable FRS features.
*/

E_EQ0157_fsNUM	"%0c number must be within range 1 to %1c.\n
The FRS object name specified in a SET/INQUIRE statement is out of range."

/*%
** Explanation:
**	Specific object names and values, such as MENU1, FRSKEY3 and PF5
**	have numbers associated with them (ie, 1, 3 and 5).  The numbers
**	must be within the specified range, as that is the range of 
**	run-time support.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Refer to the manual for the legal range of numbers that
**	qualify object names.
*/

E_EQ0158_fsOBJARG	"SET/INQUIRE object '%0c' requires %1c parent name(s).\n
A form name or table name is required with object '%0c'."

/*%
** Explanation:
**	Different FRS object types require the user to specify a different
**	list of parent names.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Refer to the manual for the required parent names for this statement.
*/

E_EQ0159_fsOBJCONST	"Illegal use of SET/INQUIRE constant '%0c'.\n
FRS constant '%0c' cannot be used with object type '%1c'."

/*%
** Explanation:
**	An FRS object type (ie, FRS, FORM, FIELD, TABLE or COLUMN) has
**	an associated set of FRS constant values that may be inquired about or
**	set.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Refer to the manual for the list of FRS constants which
**	may associate with the FRS object used in the SET/INQUIRE statement.
*/


E_EQ015A_fsBADNUM	"The %0c number is out of range in the %1c statement.\n
The number must be within range 1 to %2c.  '%3c' is assumed."

/*%
** Explanation:
**	Specific object names and values, such as MENU1, FRSKEY3 and PF5
**	have numbers associated with them (ie, 1, 3 and 5).  The numbers
**	must be within the specified range, as that is the range of 
**	run-time support.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Refer to the manual for the legal range of numbers that
**	qualify object names.
*/

E_EQ0160_fsPFCTRL	"PF key number or control key character expected\n.
You have used identifier '%0c'."

/*%
** Explanation:
**	Some FRS statements allow the assignment of a control key
**	(ie, controlF) or a function key (ie, PF2).  You may not use
**	a variable to represent this assignment, or use a control key 
**	character or function key number which is out of range.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Specify a legal control key or function key in the current statement.
*/

E_EQ0201_scBADINCEXT	"Illegal or missing suffix on INCLUDE file '%0c'.\n
The correct suffix is '.%1c'."

/*%
** Explanation:
**	INCLUDE files must be correctly specified.  We do not support
**	system-dependent file specifications, such as the inclusion of
**	files from text libraries.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the name of the file to be included.	
*/

E_EQ0202_scBADDEREF	"Illegal character following dereference character.\n
The dereference character '#' should be immediately followed by a name."

/*%
** Explanation:
**	The first character following the dereference character '#' was
**	not one of the characters which may legally start the name of
**	an INGRES object.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the name to be dereferenced.
*/

E_EQ0203_scDEREF	"No name specified after dereference character '#'.\n
The dereference character and the name must be on the same line."

/*%
** Explanation:
**	This error will occur if a new line intervened between the
**	dereference character and the name to be dereferenced.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Update the statement so that the name to be dereferenced
**	immediately follows the dereference character.
*/

E_EQ0204_scEOFCMT	"Premature End Of File found in comment."

/*%
** Explanation:
**	The preprocessor unexpectedly encountered the end of the input
**	file when scanning an embedded comment.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Close the comment before the end of file.
*/

E_EQ0205_scEOFMIDLINE	"End of file found in middle of line.\n
The current line will be ignored by the preprocessor."

/*%
** Explanation:
**	This error may occur if you run the preprocessor interactively
**	using the -s flag.  An input file containing a mid-line end of file
**	is usually one which has been created by a program and not by a
**	standard editor.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the input file by completing the last statement.
*/

E_EQ0206_scEOFSTR	"End Of File found in string '%0c'.\n
The closing quote is missing from the string."

/*%
** Explanation:
**	This error occurs when the closing quote has been omitted from
**	a string.  The preprocessor scans to the end of file searching
**	for the closing quote.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the input file by terminating the string with a quote.
*/

E_EQ0207_scILLINCNAM	"Illegal syntax in INCLUDE file name '%0c'."

/*%
** Explanation:
**	The system is unable to open the included file because its name
**	does not have a legal format.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the name of the file to be included.
*/

E_EQ0208_scINCINOPN	"Cannot open INCLUDE input file '%0c'.\n
The file is non-existent or you do not have permission to read it."

/*%
** Explanation:
**	An INCLUDE input file cannot be read because it does not exist,
**	because it is not in the current directory or the location specified
**	or because you do not have read privileges.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Check the existence, location and read privileges on the file
**	you wish to include.
*/

E_EQ0209_scINCRCRSVOPN	"Attempt to recursively open INCLUDE input file '%0c'.\n
Files may not include themselves, either directly or indirectly."

/*%
** Explanation:
**	This error will occur if the included file contains an INCLUDE file 
**	statement attempting to include itself.  This error will arise 
**	if a file in a hierarchy of included files attempts to include a file 
**	already in the hierarchy.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the INCLUDE statement in the file that includes the
**	included file.
*/

E_EQ0210_scINCOUTOPN	"Cannot open INCLUDE output file '%0c'.\n
Check your permission to write a file in the current location."

/*%
** Explanation:
**	Output files cannot be opened because a user does not have write
**	privileges.  There may be other system dependent reasons.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Attempt to open a file with the same name at the system level and take
**	note of the system response.
*/

E_EQ0211_scINCQUOTE	"Illegal INCLUDE file name delimiter character '%0c'.\n
Use legal delimiters or omit delimiters altogether."

/*%
** Explanation:
**	This error occurs when a delimiter other than a single or double
**	quote is used.  Note that embedded languages in C may also use
**	the '<' and '>' delimiters.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Replace the delimiters with the correct ones or omit delimiters
**	altogether.
*/

E_EQ0212_scLINLONG	"Input line exceeds maximum line length, %0c.\n
The rest of line is ignored."

/*%
** Explanation:
**	This error occurs on very long input lines.  Note that the 
**	line length acceptable to the preprocessor is the same as that
**	acceptable to many compilers.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Break up the flagged line into smaller lines.  If the line contains
**	a SQL statement, use host language line continuation.  EQUEL
**	statements require no special line continuation.

*/

E_EQ0213_scHEXLONG	"Maximum hex or Unicode constant length, %0c, is exceeded by <%1c>."

/*%
** Explanation:
**	The constant used contains too many characters.  Note that the
**	maximum number of characters allowed is extremely large and users
**	are unlikely to come up against this limit.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Substitute a smaller constant, or use an expression comprised
**	of smaller constants.
*/

E_EQ0214_scNUMLONG	"Maximum number length, %0c, is exceeded by '%1c'."

/*%
** Explanation:
**	The decimal number used contains too many digits.  Note that the
**	maximum number of digits allowed is extremely large and users
**	are unlikely to come up against this limit.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Substitute a smaller numeric constant, or use an expression comprised
**	of smaller constants.
*/

E_EQ0215_scSTRLONG	"Maximum string length, %0c, is exceeded by <%1c...>."

/*%
** Explanation:
**	The string literal used contains too many characters.  Note that
**	the maximum string length allowed is as big as that allowed by
**	most compilers.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	If the host language allows string continuation, try to split the 
**	string over two lines.
*/

E_EQ0216_scWRDLONG	"Maximum word length, %0c, is exceeded by '%1c'."

/*%
** Explanation:
**	The word used contains too many characters.  Note that the
**	maximum number of characters allowed in an word is extremely 
**	large and users are unlikely to come up against this limit.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Reduce the size of the name.
*/

E_EQ0217_scIDLONG	"Maximum identifier length, %0c, is exceeded by '%1c%2c'."

/*%
** Explanation:
**	The variable name or object name used is too long.  A variable name
**	in this context includes a fully qualified record variable or a 
**	subscripted variable.  An object name includes, for example, a
**	column name along with a correlation or range variable.  The
**	maximum identifier length is extremely large and users are
**	unlikely to come up against this limit.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use a shorter variable or object name.
*/

E_EQ0218_scMISSRIGHT	"Missing delimiter(s) <%0c> (from line %1c).\n
Encountered End Of File when scanning <%2c...>."

/*%
** Explanation:
**	The preprocessor was scanning input looking for a terminating 
**	or delimiting character (ie, a closing parenthesis or bracket
**	or a closing quotation mark) when it encountered an End of File. 
**	This is probably a syntax error.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Check the flagged line for a syntax error or missing delimiter.	
*/

E_EQ0219_scNLSTR	"New-line in string <%0c>.\n
Use query language convention for continuing strings over multiple lines."

/*%
** Explanation:
**	This error occurs when the user types a new-line in a quoted string
**	without using any form of line continuation.  If the host language
**	permits string continuation, the QUEL convention for continuation
**	is a back-slash, "\" on the line to be continued with the string
**	continuing in the leftmost column of the next line.  The SQL
**	convention for continuation follows the host language rules for
**	string continuation.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Continue string using appropriate query language convention
**	if the host language allows string continuation.
*/

E_EQ0220_scNUMCONV	"Conversion of '%0c' to integer/float failed.\n
'%0c' cannot be represented in an internal numeric format."

/*%
** Explanation:
**	This error occurs when a very large numeric constant cannot be
**	converted into an internal numeric format.  The preprocessor and the
**	run-time code use standard numeric storage.  Very large numbers, for
**	instance floating point numbers requiring 16-byte storage, will
**	cause an error from the preprocessor and from the run-time code.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Reduce the size of the numeric constant.
*/

E_EQ0221_scOPERATOR	"Unknown operator '%0c'."

/*%
** Explanation:
**	This error occurs when an operator used in an embedded statement 
**	is not recognized by the preprocessor.  The query languages recognize 
**	specific operators for comparisons, assignments and pattern
**	matching. 
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the operator from the embedded statement.
*/

E_EQ0222_scSCCMNT	"Unknown Comment start '%0c'.\n
Comment will be assumed to end with end of line."

/*%
** Explanation:
**	The preprocessor recognizes embedded comment lines using the
**	host language comment indicator.  For all host languages the
**	preprocessor recognizes the embedded comment delimiters.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use the conventional host language comment delimiters or the
**	generic embedded comment delimiters.
*/

E_EQ0223_scINCMANY	"Too many directories for included files.\n
The number of locations of files to be included exceeds the maximum, %0c."

/*%
** Explanation:
**	This error occurs when the preprocessor command line specifies
**	with the -h flag too many directories to be searched for include files.
**
** System Status:
**	The preprocessor will exit without preprocessing the input file.
**
** User Action:
**	Consolidate the files to be included into fewer directories.
*/

E_EQ0224_scINCBADDIR	"Invalid directory '%0c' for included files."

/*%
** Explanation:
**	The directory specified with the -h command line flag is not
**	a valid directory name on this system.  The user may have typed
**	the complete path name incorrectly.
**
** System Status:
**	The preprocessor will exit without preprocessing the user's input.
**
** User Action:
**	Invoke the preprocessor again using a valid path and directory name.
*/

E_EQ0225_scINCNULDIR	"Empty directory name specified for included files."

/*%
** Explanation:
**	A null directory has been specified with the -h flag on the
**	preprocessor command line.
**
** System Status:
**	The preprocessor will exit without preprocessing the user's input.
**
** User Action:
**	Follow the -h flag with the name of a directory in which files
**	to be included can be found.
*/

E_EQ0226_scINCNOTFOUND	"Cannot find INCLUDE input file '%0c'.\n
The file '%0c' is not in any of the directories specified by the '-h' flag."

/*%
** Explanation:
**	This errors occurs when the INCLUDE statement is attempting to
**	include a file from one of the directories specified with the
**	'-h' flag on the command line, and that file does not exist in
**	any of the specified directories.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Either correct the directory named with the -h flag, or correct
**	the file name used in the INCLUDE statement.
*/

E_EQ0227_scHEXCHAR	"Illegal character '%0c' found in hex constant."

/*%
** Explanation:
**	A hex constant (ie, X'1FFF') can contain digits and the alphabetic
**	characters A through F only.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the syntax of the flagged hex constant.
*/

E_EQ0228_scHEXODD	"Odd number of characters in hex constant '%0c'.
Use a leading zero."

/*%
** Explanation:
**	The preprocessor expects the first character/digit of a
**	hex constant to map to a high order byte boundary.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	If your hex constant contains an odd number of characters, include a
**	leading zero.  For example, X'1FF' becomes X'01FF'.
*/

E_EQ0229_scBADQUOTE	"The correct delimiter for literal strings is <%1c>.\n
Wrong string delimiter <%0c> used."

/*%
** Explanation:
**	Literal strings in Embedded SQL statements must be delimited by
**	the single quote <'>.  Embedded host declarations follow the rules
**	of the host language for quoted strings.  The manual contains examples
**	of quoted strings in both embedded statements and embedded host 
**	declarations; it also instructs on how to include a quotation within 
**	an embedded string.  This message is only a warning, so as to be
**	compatible with earlier versions of Embedded SQL.
**
** System Status:
**	This message is just a warning.
**
** User Action:
**	Change the string delimiter in the current statement to the legal
**	Embedded SQL string delimiter.
*/

E_EQ0230_scEMBEDQUOTE	"Illegal embedding of quote in a SQL string.\n
The string is truncated after the <%0c>."

/*%
** Explanation:
**	Some host language compilers do not allow the outer string
**	delimiter to appear inside a string.  In Embedded SQL, the
**	SQL delimiter can always appear inside the string by doubling it.
**	However, in some languages the preprocessor does not allow a non-SQL
**	string delimiter to appear within an SQL string.  Note that because
**	the preprocessor stops processing the string upon encountering
**	the illegal delimiter, the remainder of the string is likely to
**	produce a syntax error.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use the doubled SQL delimiter if you wish to place a string delimiter
**	inside a string.
*/


E_EQ0231_scMISSEND	"No matching END for BEGIN on line %0c.\n
Encountered End Of File when scanning BEGIN/END block."

/*%
** Explanation:
**	The preprocessor was scanning a BEGIN/END block, looking for the 
**	END, when it encountered an End of File. This is probably a 
**	syntax error.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Check for a misspelled END, or for a BEGIN without a matching END.
*/


E_EQ0232_scUESC		"Invalid escape sequence in Unicode literal."

/*%
** Explanation:
**	Escape sequences in Unicode literals must follow the convention
**	\hhhh where hhhh are the 4 hex digits of the code point to be 
**	inserted, or \+hhhhhh where hhhhhh are the 6 hex digits of the 
**	4 byte code point to be inserted (the high order byte is always
**	set to 0). A \ followed by anything other than the notations 
**	described above, or by a newline or a second \ (meaning the 
**	actual '\' character is being coded) will result in this error.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the escape sequence.
*/

E_EQ0233_scILIT	"Invalid format for interval literal."

/*%
** Explanation:
**	An interval literal is of the format "INTERVAL'nnnnn' <interval type>"
**	where <interval type> is of the format <field1> TO <field2>. This
**	literal did not satisfy the required format of <interval type>.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the interval literal.
*/

E_EQ0240_yyOVFL	"Internal error: Parser stack overflow."

/*%
** Explanation:
**	An internal data structure is not big enough to allow the
**	preprocessor to complete the current statement.  This may indicate
**	an internal coding error or an embedded statement that is too complex.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Try to restructure your statement so that it is syntactically
**	simpler.
*/

E_EQ0241_yySYNTAX	"Syntax error."

/*%
** Explanation:
**	A syntax error has occurred on the current statement.  Usually
**	the rest of the statement is ignored.  However, some syntax errors
**	will confuse the preprocessor's syntactic recovery causing more
**	errors in subsequent statements.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the statement causing the syntax error.
*/

E_EQ0242_yySYNHOST	"Syntax error on host code."

/*%
** Explanation:
**	This error occurs when host code appears in the middle of an
**	embedded statement.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Complete the embedded statement that appears before the flagged
**	host code line.
*/

E_EQ0243_yySYNTERM	"Syntax error on terminator or missing %0c continuation indicator."

/*%
** Explanation:
**	For those host languages that do not have SQL statement terminators, 
**	the preprocessor allows the ';' terminator on most embedded statements.
**	(This proves useful when using the preprocessor interactively.)
**	However, a terminator is prohibited on declarations and some
**	statements and will produce the above error.  This error is also
**	caused when an embedded SQL statement is continued over more than
**	one line and the host continuation indicator (if any) is omitted.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the statement terminator or supply the host language
**	continuation indicator.
*/

E_EQ0244_yySYNWRD	"Syntax error on '%0c'."

/*%
** Explanation:
**	The preprocessor has identified the word or symbol which is 
**	syntactically incorrect.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Correct the syntax.
*/

E_EQ0245_yySYNEOF	"Syntax error on End-of-File."

/*%
** Explanation:
**	The preprocessor has unexpectedly encountered an End of File.  This
**	error usually occurs if the end of file occurred in the middle of
**	a block-type embedded statement or an SQL declaration section.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Complete the embedded block or statement.
*/

E_EQ0251_syBADLEVEL	"Invalid record level %0c in declaration for '%1c'."

/*%
** Explanation:
**	If record levels are required when declaring variables (ie, as in
**	PL/I or COBOL), they must be ordered correctly according
**	to the rules of the host language.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
*/

E_EQ0252_syCHEKNULL	" Symbol table: Null pointer to check_type_loop."

/*%
** Explanation:
**	If you have defined a type and are referencing a variable of that type,
**	try to make the variable a base type.  This may reduce the complexity
**	of the variable look-up.  This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0253_syDEEPIMPORT	"Symbol table IMPORT of '%0c' is too deep."

/*%
** Explanation:
**	If your host language allows the IMPORT of variables from an outer
**	scope, you cannot import deeper than 20 levels.
**
** System Status:
**
** User Action:
*/

E_EQ0254_syEXPORTVIS	"Illegal attempt to EXPORT already visible name '%0c'."

/*%
** Explanation:
**	If your host language allows the EXPORT of variables to an outer
**	scope, you must confirm that the variable has not been declared
**	in the outer scope.
**
** System Status:
**
** User Action:
*/

E_EQ0255_syFSTAKOFLO	"Symbol table record field stack overflow - fields discarded."

/*%
** Explanation:
**	If you have qualified a structure variable, it should have less than
**	20 member names qualifying it.
**
** System Status:
**
** User Action:
*/

E_EQ0256_syLOOKANON	"Symbol table lookup of an anonymous entry."

/*%
** Explanation:
**	In a language in which you do not have to name a structure type (such
**	as C), the preprocessor is trying to reference the type name.  Try
**	naming all your structure types (ie, use structure tags or type
**	declarations).  This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0257_syMODIFY	"Symbol table modify of '%0c' is neither imported nor exported."

/*%
** Explanation:
**	The preprocessor is confused about what it thinks is either an IMPORT
**	or EXPORT statement.  If you have any try changing them to explicit
**	declarations in their relevant scopes.  This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0258_syNOTVISIBL	"'%0c' is not visible for IMPORT statement."

/*%
** Explanation:
**	If your language allows IMPORT from an outer scope, then the name
**	of the variable must be visible in the parent scope.
**
** System Status:
**
** User Action:
*/

E_EQ0259_syQNOTSUCC	"Symbol table q_remove: q_elm not successor of q_before."

/*%
** Explanation:
**	The symbol table data structures have been damaged.  Try to use fewer
**	type and variable declarations.  You may have to split your file into
**	two smaller files.  This is an internal error.
**	
** System Status:
**
** User Action:
*/

E_EQ0260_syREDECLARE	"Redeclaration of identifier '%0c' is ignored."

/*%
** Explanation:
**	The named identifier as been redeclared.  The preprocessors stores
**	all declaration objects in a single name space, consequently, type
**	names, constant names and variable names must be different.
**
** System Status:
**
** User Action:
*/

E_EQ0261_syREIMPORT	"Attempt to IMPORT already visible name '%0c'."

/*%
** Explanation:
**	If your language allows IMPORT from an outer scope, then the imported
**	name must not be an already-declared object in the nested scope.
**
** System Status:
**
** User Action:
*/

E_EQ0262_sySSTKOFLO	"Symbol table scope stack overflow - nesting limit is %0c."

/*%
** Explanation:
**	Rearrange the nesting of embedded blocks so that they are not so nested.
**
** System Status:
**
** User Action:
*/

E_EQ0263_sySSTKUFLO	"Symbol table scope stack underflow."

/*%
** Explanation:
**	If there were syntax error while preprocessing the current statement
**	then correct the errors and try again.  Also try to rearrange the
**	nesting of embedded blocks.  This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0264_sySVISZERO	"Symbol table scope end: found '%0c' with visibility of zero."

/*%
** Explanation:
**	The symbol table data structures have been damaged.  Try reducing the
**	number of declarations in the module.  This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0265_syTYPLOOP	"Symbol table type loop for ('%0c', '%1c')."

/*%
** Explanation:
**	You have defined a type recursively causing confusion in the symbol
**	table data structures.  This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0266_syUNDECSTAT	"'%0c' is undeclared in IMPORT/EXPORT statement."

/*%
** Explanation:
**	You have tried to IMPORT or EXPORT a name that was never declared.
**	All names must be declared before you can expand their scope.
**
** System Status:
**
** User Action:
*/

E_EQ0267_sySQREDEC	"Redeclaration of identifier '%0c' with incompatible type."

/*%
** Explanation:
**	Embedded SQL allows the redeclarations of some variables.  The variables
**	must be elementary variables (not structures or types).  Any
**	redeclaration requires an exact type match.
**
** System Status:
**
** User Action:
*/

E_EQ0268_syTYPEUNDEF	"'%0c' references '%1c', which is not yet defined."

/*%
** Explanation:
**	If your host language allows forward declarations (ADA, BASIC, PASCAL)
**	your must make sure that you have declared the forward-defined type
**	before using a variable of that type.
**
** System Status:
**
** User Action:
*/

E_EQ0301_actBAD	"Illegal ACTIVATE type, %0c, passed."

/*%
** Explanation:
**	The preprocessor does not recognize the ACTIVATE type used.
**	This may be an internal error.  If you have complex ACTIVATE 
**	block control flow, try to flatten out any nested blocks and 
**	reduce the number of RESUME statements.
**
** System Status:
**
** User Action:
*/

E_EQ0302_actCOL	"Missing ACTIVATE COLUMN name - table is '%0c'."

/*%
** Explanation:
**	This is a syntax error in an ACTIVATE COLUMN statement that was caught
**	too late.  Add the column name to the ACTIVATE statement.
**
** System Status:
**
** User Action:
*/

E_EQ0303_actNEST	"ACTIVATE blocks are nested too deeply - limit is %0c."

/*%
** Explanation:
**	Rearrange the nesting of DISPLAY and ACTIVATE blocks so that there
**	are fewer levels of nesting.
**
** System Status:
**
** User Action:
*/

E_EQ0304_actNULL	"Null ACTIVATE object passed - type is %0c."

/*%
** Explanation:
**	If you used a variable name for an ACTIVATE object try to replace it
**	with a string literal.  This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0305_actUNDFL	"ACTIVATE block underflow."

/*%
** Explanation:
**	If there were syntax errors while processing DISPLAY and ACTIVATE
**	blocks but not within the code enclosed in the block, correct the
**	errors.  If there were no syntax errors then this is an internal
**	coding error.
**
** System Status:
**
** User Action:
*/

E_EQ0306_actARGS	"Illegal ACTIVATE target list syntax (%0c = %1c)."

/*%
** Explanation:
**	The target lists allowed with various ACTIVATE conditions must be
**	of the format:
**		(object = value {, object = value})
**	with the correct type and range for the value.
**
** System Status:
**
** User Action:
*/

E_EQ0307_actRESNEXT	"Cannot RESUME NEXT in an ACTIVATE %0c block."

/*%
** Explanation:
**	RESUME NEXT should be used to resume from an ACTIVATE FIELD or an
**	ACTIVATE COLUMN.  RESUME NEXT within other ACTIVATE blocks may
**	generate this warning.
**
** System Status:
**
** User Action:
*/

E_EQ0308_actTIMEOUT	"Only one ACTIVATE TIMEOUT block is allowed in a display loop."

/*%
** Explanation:
**	There may be only one activate block to handle timeout in a
**	display loop.  Only the first ACTIVATE TIMEOUT block will be
**	used at run-time.
**
** System Status:
**
** User Action:
*/

E_EQ0309_actTARGET	"An ACTIVATE %0c block may not have a target list."

/*%
** Explanation:
**	The optional target list may not be used on all ACTIVATE
**	statements.  Consult the embedded language User's Guide for
**	the list of ACTIVATE statements which allow a target list.
**
** System Status:
**
** User Action:
*/

E_EQ0310_actBADRES	"Cannot RESUME %0c in an ACTIVATE %1c block."

/*%
** Explanation:
**	Only a "plain" RESUME can be used in an ACTIVATE DBEVENT block.
**	RESUME NEXT, RESUME FIELD, RESUME COLUMN, RESUME MENU, RESUME
**	ENTRY are not allowed.
**
** System Status:
**	Preprocessor will return a FAIL status.
**
** User Action:
**	Change user code to use the RESUME statement.
*/

E_EQ0311_actONLYEVENT	"ACTIVATE DBEVENT cannot be the only ACTIVATE block in a DISPLAY loop."
/*%
** Explanation:
**	An ACTIVATE DBEVENT by itself in a DISPLAY loop will simply not work.
**	The DBEVENT block is dependent on other activations to "trigger"
**	the FRS to do the "get dbevent"; otherwise, the DBEVENT block will 
**	never be called.
**
** System Status:
**
** User Action:
**	Add other activations to the display loop.
*/

E_EQ0316_argMAX	"Statement has too many arguments.  The limit is %0c arguments."

/*%
** Explanation:
**	Reduce the number of arguments to the statement.  This is probably the
**	SQL CONNECT statement or QUEL INGRES statement, as most other statements
**	have a fixed number of arguments.  The specified limit may include
**	internally generated arguments.
**
** System Status:
**
** User Action:
*/

E_EQ0317_argNOARGS	"There are %0c argument(s), statement requires more."

/*%
** Explanation:
**	There was probably a syntax error in the current statement, and the
**	preprocessor error recovery routine did not supply enough dummy
**	arguments for code generation.
**
** System Status:
**
** User Action:
*/

E_EQ0326_lblOVERFLOW	"Too many nested blocks, maximum is %0c."

/*%
** Explanation:
**	Rearrange the nesting of embedded blocks (DISPLAY, ACTIVATE,
**	UNLOADTABLE, etc) so that they are not so deeply nested.
**
** System Status:
**
** User Action:
*/

E_EQ0327_lblUNDERFLOW	"Label block manager underflow."

/*%
** Explanation:
**	If there were syntax errors while processing embedded blocks
**	(DISPLAY, ACTIVATE, UNLOADTABLE, etc) but not within the code enclosed
**	in the block, then correct the errors.  If there were no syntax
**	errors then this is an internal coding error.
**
** System Status:
**
** User Action:
*/

E_EQ0328_lblUNKNOWNMOD	"Unknown label mode - %0c."

/*%
** Explanation:
**	This is an internal error.
**
** System Status:
**
** User Action:
*/

E_EQ0336_outFUNC	"Null function pointer passed for Output state trap."

/*%
** Explanation:
**	If you are declaring many cursors together with using repeated 
**	queries, try rearranging their order.  If there were syntax errors
**	during the cursor declarations and repeated queries then correct the
**	syntax errors.  If there were no syntax errors then this is an
**	internal coding error.
**
** System Status:
**
** User Action:
*/

E_EQ0341_strBADPTR	"String Table pointer is out of bounds."

/*%
** Explanation:
**	This is an internal coding error.
**
** System Status:
**
** User Action:
*/

E_EQ0342_strSIZE	"Illegal String Table size initializer, %0c."

/*%
** Explanation:
**	This is an internal coding error.
**
** System Status:
**
** User Action:
*/

E_EQ0343_strSPACE	"Not enough String Table space for %0c bytes."

/*%
** Explanation:
**	If this occurs after an error indicating that an object has exceeded
**	its maximum size (such as a string literal that is too long), then
**	correct the previous error.  Otherwise this is an internal coding
**	error.
**
** System Status:
**
** User Action:
*/

E_EQ0344_strTRUNC	"String '%0c' too large for String Table buffer (%1c bytes)."

/*%
** Explanation:
**	If the specified string refers to objects in the preprocessed file,
**	then try to break up the object names into smaller pieces
**	(ie, replacing large string literals with variables).  If the
**	specified string does not include objects from the file, then this is
**	an internal coding error.
**
** System Status:
**
** User Action:
*/

E_EQ0376_repARG	"Use REPEAT queries for compiling parameters with '@'."

/*%
** Explanation:
**	In order to use the '@' operator with variables replacing QUEL
**	constants, you must prefix the query with the word "REPEAT".
**
** System Status:
**
** User Action:
*/

E_EQ0377_repNOTIMP	"REPEAT queries are not implemented with this version."

/*%
** Explanation:
**	Some versions of the preprocessor do not support REPEAT queries.
**
** System Status:
**
** User Action:
*/

E_EQ0378_repWHERE	"A variable may not substitute for a complete WHERE clause in a REPEATED statement."

/*%
** Explanation:
**	You may not use a host variable to substitute for a WHERE clause in
**	a REPEATED statement.  The dbms saves the query execution plan
**	of a REPEATED query using the initial values specified in the
**	WHERE clause.  This means that the dbms will not automatically apply
**	any change to the WHERE clause on repeated execution of the query.
**	This rule applies to changes to the WHERE clause while the application
**	is running and changes resulting from more than one user running the
**	same application and supplying different WHERE clause values.  Note
**	that this rule does not apply to substituting a variable for a single
**	value or expression in a WHERE clause.  Changes to these variables
**	are applied by the dbms to repeated query executions.
**	
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.  The
**	error will not prevent the application from compiling, but it will
**	produce a run-time error.
**
** User Action:
**	Either remove the 'REPEATED' word from the statement or reconstruct
**	the WHERE clause to substitute variables for individual expressions.
**	Repreprocess the module.
*/

E_EQ0386_retVAR	"Result column, '%0c', for data retrieval must be a variable."

/*%
** Explanation:
**	The result variable used in an SQL SELECT or QUEL RETRIEVE statement
**	to collect the data must have been properly declared to the
**	preprocessor.
**
** System Status:
**
** User Action:
*/

E_EQ0396_csBADTYPE	"Retrieval of cursor '%0c' into '%1c' conflicts with earlier retrieval."

/*%
** Explanation:
**	The base type (numeric or string type) of a result variable in
**	an SQL FETCH or QUEL RETRIEVE CURSOR statement conflicts with the type
**	of the same column on a preceding retrieval in the same file.
**
** System Status:
**
** User Action:
*/

E_EQ0397_csDECNULL	"Declaration of a cursor without a name."

/*%
** Explanation:
**	You have declared an empty string literal as a cursor name.  String
**	literals can be used to declare cursors whose names are reserved words,
**	but they must not be null strings.
**
** System Status:
**
** User Action:
*/

E_EQ0398_csNOCSR	"Cursor named '%0c' is not yet declared."

/*%
** Explanation:
**	You have issued a cursor statement (such as a retrieval) without
**	a preceding cursor declaration (DECLARE CURSOR) or the named cursor.
**
** System Status:
**
** User Action:
*/

E_EQ0399_csNOCURCSR	"No cursor referenced in current statement."

/*%
** Explanation:
**	If you used an empty string to declare a cursor then correct the
**	previous error.  Otherwise the preprocessor has not retained the
**	current cursor name within a cursor statement, and then this
**	is an internal coding error.
**
** System Status:
**
** User Action:
*/

E_EQ0400_csNOTUPD	"%0c of read-only cursor '%1c' is not legal."

/*%
** Explanation:
**	The specified cursor is can not be updated (ie, it may refer
**	to a join of multiple tables).  The update operation specified in
**	the first argument (such as DELETE) is not allowed.
**
** System Status:
**
** User Action:
*/

E_EQ0401_csOBJNULL	"In cursor '%0c', object name is empty."

/*%
** Explanation:
**	An empty string literal has been given as an object name in a cursor
**	declaration (other than for a cursor name).  This may be a table or
**	a column name.  Correct the object to correctly specify the name.
**
** System Status:
**
** User Action:
*/

E_EQ0402_csREDECCSR	"Redeclaration of cursor '%0c'."

/*%
** Explanation:
**	The named cursor has already been declared.  The scope of a cursor
**	declaration is the whole source file.  The same cursor name cannot
**	be redeclared to the preprocessor.  This does not affect the executable
**	OPEN statement, which can occur multiple times. 
**
** System Status:
**
** User Action:
*/

E_EQ0403_csUPDEXPR	"Cannot update column '%0c' of cursor '%1c'.\n
Result columns defined as expressions cannot be updated."

/*%
** Explanation:
**	Some result columns may not specified for update.  A result column name
**	that names a constant expression may not be specified in the FOR
**	UPDATE list.
**
** System Status:
**
** User Action:
*/

E_EQ0404_csUPDMANY	"Cannot update column '%0c' of cursor '%1c' as it is not a unique name."

/*%
** Explanation:
**	Some result columns may not specified for update.  A result column name
**	that names two distinct columns within the cursor declaration may not
**	be specified in the FOR UPDATE list.
**	
** System Status:
**
** User Action:
*/

E_EQ0405_csUPDCONST	"Cannot update column '%0c' of cursor '%1c' as it is a constant."

/*%
** Explanation:
**	Some result columns may not specified for update.  A result column name
**	that is an INGRES constant (such as 'username') may not be specified
**	in the FOR UPDATE list.
**
** System Status:
**
** User Action:
*/

E_EQ0406_csUPDXIST	"Cannot update column '%0c' of cursor '%1c' as it is not in the target list."

/*%
** Explanation:
**	Some result columns may not specified for update.  A result column name
**	that is not in the target list of the cursor declaration may not be
**	specified in the FOR UPDATE list.
**
** System Status:
**
** User Action:
*/

E_EQ0407_csUPDTABLES	"Cannot declare cursor '%0c' FOR UPDATE as it has more than one table."

/*%
** Explanation:
**	A cursor declaration that refers to more than one table cannot be
**	updated.
**
** System Status:
**
** User Action:
*/

E_EQ0408_csUPDUNIQ	"Cannot declare cursor '%0c' FOR UPDATE as it has a %1c clause."

/*%
** Explanation:
**	The specified cursor cannot be updated as it includes a query language
**	clause, such as the SQL ORDER or QUEL SORT clause.
**
** System Status:
**
** User Action:
*/

E_EQ0409_csUPDNOCOL	"Cannot declare cursor '%0c' FOR UPDATE as it has no updatable columns."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_EQ0410_csUPDFUNC	"Cannot declare cursor '%0c' FOR UPDATE as it has a function in the target list."

/*%
** Explanation:
**	If a cursor has a function in the target list (ie, _dbms_info()) then
**	the cursor cannot be updated.
**
** System Status:
**
** User Action:
*/

E_EQ0411_csUPDALL	"Cannot use ALL in the FOR UPDATE list for cursor '%0c'."

/*%
** Explanation:
**	You must enumerate all columns in the cursor result table in order to
**	update all of them.
**
** System Status:
**
** User Action:
*/

E_EQ0412_csCLMNUPD	"Cannot update cursor '%0c' as no columns were named in the update clause."

/*%
** Explanation:
**	Because there was no FOR UPDATE clause (or not columns specified
**	within the FOR UPDATE clause) the cursor cannot be updated.
**
** System Status:
**
** User Action:
*/

E_EQ0413_csRANGEREPL	"Cannot use range variable expression '%0c.%1c' in REPLACE CURSOR '%2c'."

/*%
** Explanation:
**	In the EQUEL REPLACE CURSOR statement, you cannot refer to other tables
**	within the target list expressions.
**
** System Status:
**
** User Action:
*/

E_EQ0414_csCOLUPD	"Cannot update column '%0c' of cursor '%1c' as it is not updatable."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_EQ0415_csNOUPDCOLS	"Cursor '%0c' has no updatable columns."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_EQ0416_csCOLNOSUCH	"Cannot update column '%0c' as it is not in cursor '%1c'."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_EQ0417_csPARAMUPD	"Cannot REPLACE PARAM cursor '%0c' as there are no updatable columns."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_EQ0418_csNOLISTUPD	"Cannot update cursor '%0c' as there was no list of updatable columns."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_EQ0419_csTBLUPD	"Cannot %0c cursor '%1c' as it has more than one table."

/*%
** Explanation:
**
** System Status:
**
** User Action:
*/

E_EQ0420_csCSRNULL	"Reference to a cursor without a name."

/*%
** Explanation:
**	If you used an empty string to declare a cursor then correct the
**	previous error.  Otherwise the preprocessor has not retained the
**	current cursor name within a cursor statement, and then this
**	is an internal coding error.
**
**
** System Status:
**
** User Action:
*/

E_EQ0421_csGETMANY	"Too many variables for retrieval from cursor '%0c'."

/*%
** Explanation:
**	The cursor declaration did not include as many result columns as
**	variables specified in the cursor retrieval statement.  You must
**	retrieve exactly the same number of columns as specified in the
**	cursor declaration.
**
** System Status:
**
** User Action:
*/

E_EQ0422_csGETFEW	"Too few variables for retrieval from cursor '%0c'."

/*%
** Explanation:
**	The cursor declaration included more result columns than
**	variables specified in the cursor retrieval statement.  You must
**	retrieve exactly the same number of columns as specified in the
**	cursor declaration.
**
** System Status:
**
** User Action:
*/

E_EQ0423_csBADREPEAT	"The cursor '%0c' statement may not be a repeated query."

/*%
** Explanation:
**	The cursor retrieval and update statements may not be specified in 
**	the context of a repeated query.
**
** System Status:
**
** User Action:
*/

E_EQ0424_csILLCORRELNAME "Correlation name '%0c' is not allowed in a cursor '%1c' statement."

/*%
** Explanation:
**	In SQL you cannot specify a correlation name for a cursor
**	DELETE or UPDATE.  Only the table name is allowed and the table name
**	must be identical to the table referred to in the cursor declaration.
**
** System Status:
**
** User Action:
*/

E_EQ0425_csUSINGDYN	"A OPEN statement USING clause is legal only for a dynamic cursor."

/*%
** Explanation:
**	The USING clause of the SQL OPEN statement:
**		USING DESCRIPTOR descriptor_name | variable {, variable}
**	is only allowed if the cursor was declared for a statement_name, as
**	specified for dynamic SQL.
**
** System Status:
**
** User Action:
*/

E_EQ0426_csREPEAT	"Cursor declarations for REPEATED queries are not supported."

/*%
** Explanation:
**	The DECLARE CURSOR statement cannot refer to a REPEATED query.
**	The INGRES run-time system does not yet support this construct.
**
** System Status:
**
** User Action:
*/

E_EQ0427_csTBLBAD	"Invalid table '%0c' in cursor '%1c' statement."

/*%
** Explanation:
**	The table name referenced in the cursor DELETE or UPDATE
**	statement is not the same as the table name specified in the
**	DECLARE CURSOR statement.
**
** System Status:
**
** User Action:
*/

E_EQ0428_csCURSORNAME	"Reference to cursor '%0c' is incompatible with declaration."

/*%
** Explanation:
**	If a DECLARE CURSOR statement uses a variable to represent the
**	cursor name, all references to that cursor must also use the
**	same variable name.  Similarly, if a DECLARE CURSOR statement
**	uses a static name, subsequent cursor operations must also
**	use the same static name.
**
** System Status:
**
** User Action:
*/

E_EQ0429_csBADFROM	"The cursor '%0c' statement may not contain a FROM clause."

/*%
** Explanation:
**	The optional FROM clause cannot be used in a cursor update statement;
**	it is only allowed in a non-cursor update statement.
**
** System Status:
**
** User Action:
*/

E_EQ0430_csNOTSCROLL	"Scrolling FETCH cannot be used on non-scrollable cursor '%0c'"

/*%
** Explanation:
**	An attempt to use a scrolling FETCH on a non-scrollable cursor was
**	detected. Cursor must be declared with SCROLL option.
**
** System Status:
**
** User Action:
*/

E_EQ0431_csKEYSET	"KEYSET SCROLL cursor '%0c' must be updateable"

/*%
** Explanation:
**	KEYSET cursors must be updateable.
**
** System Status:
**
** User Action:
*/

E_EQ0450_fwILLEGAL_FWC	
"This statement does not support the forms WITH clause."

/*%
** Explanation:
**	The forms WITH clause is not supported by this statement.
**
** System Status:
**
** User Action:
**	Refer to the manual for the statements supporting this feature.
**
*/

E_EQ0451_fwINVALID_OP_SUBOP
"Invalid WITH clause option or suboption: '%0c = %1c'"

/*%
** Explanation:
**	This option or suboption is invalid.
**
** System Status:
**
** User Action:
**	Refer to the manual for valid options and suboptions in the
**	forms with clause.
*/

E_EQ0452_fwBAD_COMBO  "Invalid combination of statement, option, and suboption\n
(current option: '%0c = %1c')."

/*%
** Explanation:
**	Although these are valid options, the combination of the suboption 
**	and option are not valid for this statement.
**
** System Status:
**
** User Action:
**	Refer to the manual for the correct syntax of this statement.
*/

E_EQ0453_fwTYPE_MISMATCH	
"Type of '%0c' does not match the type required for suboption '%1c'."

/*%
** Explanation:
**	The current statement requires a special word, constant, or variable
**	type other than that specified. 
**
** System Status:
**
** User Action:
**	Refer to the manual for the allowed types.
*/

E_EQ0454_fwOP_REDEF	"Forms WITH clause option '%0c' has been redefined.\n
The last definition will be used."

/*%
** Explanation:
**	If a suboption is defined more than once in a forms WITH clause,
**	this warning is displayed. 
**
** System Status:
**	The preprocessor will ignore the earlier definitions and will return
**	a SUCCESS status if no other errors occur during preprocessing.
**
** User Action:
**	Remove the unnecessary definition(s).
**
*/

E_EQ0455_fwTABLE_FULL	"Fatal attempt to access internal table.\n
An internal forms WITH clause table is full."

/*%
** Explanation:
**	A required internal table could not be accessed.  This
**	is caused by an internal coding error.
**
** System Status:
**
** User Action:
**	Contact your INGRES support person.
*/

E_EQ0456_fwBAD_VALUE	
"'%0c' is not a valid value for suboption '%1c'."

/*%
** Explanation:
**	The combination of the suboption and value is not valid. 
**	The current suboption requires another special word, constant, or 
**	variable value other than that specified.
**
** System Status:
**
** User Action:
**	Refer to the manual for the correct syntax of this statement.
*/

E_EQ0500_OPEN_SUBCLAUSE	"The '%0c' clause does not comply with OPEN SQL."

/*%
** Explanation:
**	When the '-wopen' flag is used, the embedded SQL preprocessor will
**	generate a warning message when it encounters a clause in an SQL
**	statement that is not compliant with OPEN SQL.  The clause flagged
**	may be an INGRES extension to SQL.  
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	If users wish to produce portable code to be used on heterogeneous
**	DBMS's, they should eliminate any statements that produce this warning
**	message.
*/

E_EQ0501_hBADTYPE	"Unsupported %0c type '%1c' used.  Declaration is ignored."

/*%
** Explanation:
**	Across all host languages, there are some types that are not supported
**	by the preprocessors, as they are not compatible with an INGRES
**	type.  The declaration will either be ignored or the variable will
**	be treated as a simple integer.
**
** System Status:
**
** User Action:
*/

E_EQ0502_hNOVAR	"%0c variable '%1c' has not been declared."

/*%
** Explanation:
**	The identifier was referenced as though it was a variable, but was
**	not declared to the preprocessor.
**
** System Status:
**
** User Action:
*/

E_EQ0503_hSQLCA	"INCLUDE SQLCA must be issued before statement '%0c'."

/*%
** Explanation:
**	In most host languages, you must specify INCLUDE SQLCA before any
**	embedded SQL statement.  The INCLUDE SQLCA statement also generates
**	host language statements to make the generated calls acceptable to
**	the host language compiler.
**
** System Status:
**
** User Action:
*/

E_EQ0504_OPEN_WARN	"The statement '%0c'\nis not an OPEN SQL command.  
This statement may not be portable\nto a non-INGRES DBMS."

/*%
** Explanation:
**	When the '-wopen' flag is given, the esql preprocessor will produce
**	a warning message when it encounters a statement that is not an OPEN SQL
**	statement.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	If users wish to produce portable code to be used on heterogeneous
**	DBMS's, they should eliminate any statements that produce this warning
**	message.
*/

E_EQ0505_FIPS_STATMT
"The '%0c' statement is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a statement that is not compliant with SQL92 (entry level).  The 
**	statement flagged is probably an INGRES extension to SQL92.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92 (entry level), this statement should 
**	be removed or replaced with an SQL92 compliant SQL statement.
*/

E_EQ0506_FIPS_CLAUSE
"The '%0c' clause in the '%1c' statement is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a clause in an SQL statement that is not compliant with SQL92
**	(entry level).  The clause flagged may be an INGRES extension to 
**	SQL92.  It may be that a list of objects has been stated where 
**	only a single object is allowed.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92 (entry level), the non-compliant 
**	clause should be omitted from the SQL statement.
*/

E_EQ0507_FIPS_NOTOP
"The '%0c' clause on the '%1c' statement is not optional in SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a statement that is not compliant with SQL92 (entry level) because a 
**	clause has been omitted.  The required clause is probably 
**	optional in INGRES SQL.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92 (entry level), the SQL statement 
**	should be rewritten using the required clause.
*/

E_EQ0508_FIPS_OP	
"The '%0c' operator is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a comparison operator that is not compliant with SQL92 (entry level).
**	The valid comparison operators are =, <>, <, >, <=, and >=.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92 (entry level), a complying 
**	comparison operator must be substituted for the flagged operator.
*/

E_EQ0509_FIPS_FUNC
"The '%0c' function is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a function that is not compliant with SQL92 (entry level).  The valid 
**	SQL92 functions are CONT, AVG, SUM, MAX and MIN.  The flagged 
**	function is probably an INGRES extension to SQL.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	In order to comply with entry level SQL92, the flagged 
**	function reference should be removed from the SQL statement.
*/

E_EQ050A_FIPS_EXTSN
"The '%0c' statement is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a statement that does not comply with SQL92 (entry level).  While 
**	outside the set of SQL92-compliant statements, this flagged statement 
**	is in the category of allowable implementor-defined extensions.
**	The statement is required to establish the connection and/or the
**	run-time environment of the application.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	The flagged statement may need modification when the application
**	is run in a non-INGRES environment.
*/

E_EQ050B_FIPS_FRS
"INGRES Forms System statements are INGRES extensions to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a statement that does not comply with SQL92 (entry level).  All 
**	statements beginning with 'EXEC FRS' belong to the INGRES Forms 
**	System language, which is an INGRES extension to SQL92.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92, this statement should be removed.
*/

E_EQ050C_FIPS_DATTYP
"The data type in the '%0c' statement is an INGRES extension to SQL92 (entry level).  Valid data types are char, varchar, float, smallint, integer."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a data type that does not comply with SQL92 (entry level).  The 
**	valid entry level SQL92 data types are char(), varchar(), float(), 
**	smallint, integer, float.  The flagged data type is probably an 
**	INGRES extension to SQL92.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	In order to comply with SQL92 (entry level), a complying data type 
**	must be substituted for the flagged data type.
*/

E_EQ050D_FIPS_VAR	
"The type of host variable referenced in this statement is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters an
**	SQL statement that contains references to non-simple host variables
**	such as structure members, arrays, pointers etc.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	In order to comply with SQL92 (entry level), only use simple host 
**	variables.
*/

E_EQ050E_NOSQLCA "INCLUDE SQLCA must be issued to use SQLCODE."

/*%
** Explanation:
**	You have either declared an SQLCODE in a declare section or have 
**	indicated it's use with the -sqlcode flag, but you have not issued
**	an INCLUDE SQLCA statement before issueing a statement that could
**	affect the value of SQLCODE.  
**
** System Status:
**	The preprocessor will continue; however, status information will not
**	be available at runtime via the SQLCODE variable.
**
** User Action:
**	Issue an INCLUDE SQLCA statement before any other embedded SQL 
**	statement that could set query status.
*/

E_EQ050F_FIPS_USE
"The use of '%0c' is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	an SQL statement that contains a keyword that does not comply with
**	SQL92.
**	
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	In order to comply with SQL92 (entry level), the flagged statement 
**	will need modification.
*/

E_EQ0510_OPEN_FUNC	"The '%0c' function does not comply with OPEN SQL.  This statement may not be portable to a non-INGRES DBMS."

/*%
** Explanation:
**	When the '-wopen' flag is given, the esql preprocessor will generate
**	a warning message when it encounters a function that is not supported
**	in OPEN SQL.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	If users wish to produce portable code to be used on heterogeneous
**	DBMS's, they should eliminate any statements that produce this warning
**	message.
*/

E_EQ0511_FIPS_SUBCLAUSE  
"The 'GROUP BY/HAVING' clause in the subquery of the comparison predicate is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a HAVING clause or a GROUP BY clause in a sub-select of a comparison
**	predicate of an SQL statement.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92 (entry level), the HAVING/GROUP BY 
**	clause should be omitted from the sub-select of the SQL statement.
*/

E_EQ0512_OPEN_CLAUSE	"The '%0c' clause in the '%1c' statement does not comply with OPEN SQL.  This statement may not be portable to a non-INGRES DBMS."
/*%
** Explanation:
**	When the '-wopen' flag is used, the embedded SQL preprocessor will
**	generate a warning message when it encounters a clause in an SQL
**	statement that is not compliant with OPEN SQL.  The clause flagged
**	may be an INGRES extension to SQL.  
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**      If users wish to produce portable code to be used on heterogeneous
**      DBMS's, they should eliminate any statements that produce this warning
**      message.
*/


E_EQ0513_OPEN_OP	"The '%0c' operator does not comply with OPEN SQL."

/*%
** Explanation:
**	When the '-wopen' flag is used, the embedded SQL preprocessor will
**	generate a warning message when it encounters a comparison operator
**	that is not compliant with OPEN SQL.  The valid comparison operators
**	are =, <>, <, >, <=, and >=.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**      If users wish to produce portable code to be used on heterogeneous
**      DBMS's, they should substitute a complying comparison operator
**	for the flagged operator.
*/


E_EQ0514_FIPS_IDLEN	
"The identifier '%0c' exceeds the 18 character limit specified by SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	an identifier that does not comply with SQL92 (entry level).  An 
**	identifier is not compliant if it is longer than what the standard 
**	allows.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	The flagged identifier may need modification when the application
**	is run in a non-INGRES environment.
*/

E_EQ0515_FIPS_IDCHAR	
"The identifier '%0c' contains characters that are an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	an identifier that does not comply with SQL92 (entry level).  An 
**	identifier is not compliant if it contains Ingres special characters 
**	'@#$' or has an underscore as the first or last character in the 
**	identifier.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	The flagged identifier may need modification when the application
**	is run in a non-INGRES environment.
*/

E_EQ0516_FIPS_IDSQL2	
"The identifier '%0c' does not comply with SQL92 because it matches an ANSI SQL reserved word."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	an identifier that does not comply with SQL92.  An identifier
**	is not compliant if it matches an ANSI SQL2 reserved word.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	produce a FAIL status at the end of preprocessing.
**
** User Action:
**	The flagged identifier may need modification when the application
**	is run in a non-INGRES environment.
*/

E_EQ0517_SQLSTATUS_DECL	"The '%0c' identifier has been incorrectly declared.  It should be declared as '%1c'."

/*%
** Explanation:
**	This status variable needs to be declared in a manner conforming
**	to the ANSI SQL2 specification.   The current declaration
**	is non-conforming.
**
** System Status:
**	The preprocessor will continue; however, this status variable will
**	NOT be updated with status information during application execution.  
**
** User Action:
**	Correct the declaration to conform to the ANSI SQL2 specification.
**	Consult your ESQL language companion guide and possibly the Release 
**	Notes for more information. 
*/

E_EQ0518_grOBJVAR	"4GL object variable '%0c' must be a 4-byte integer."

/*%
** Explanation:
**	You have used a variable to represent a 4GL Object which is either a 
**	non-integer or is the wrong size.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use a variable which has been declared to the preprocessor
**	as a 4-byte integer.
*/

E_EQ0519_BAD4GLLANG	
"EXEC 4GL is not supported for this host programming language."

/*%
** Explanation:
**	You have used an EXEC 4GL statement in a language which doesn't 
**	support it.
**
** System Status:
**	The preprocessor will exit with a FAIL status.
**
** User Action:
**	Use a language which supports EXEC 4GL.
*/

E_EQ051A_NOROWNUM	
"The INSERTROW statement requires the ROWNUMBER attribute."

/*%
** Explanation:
**	You have omitted the ROWNUMBER attribute in an INSERTROW statement.	
**	This attribute is mandatory.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Add a row number.
*/

E_EQ051B_ARGS_TOEND_ERROR
"An internal error has occurred in the routine 'args_toend'."

/*%
** Explanation:
**	The routine args_toend was called with invalid parameters. This may `
**	be caused by an internal coding error.
**
** System Status:
**	The preprocessor will exit.
**
** User Action:
**	If the preprocessor reported syntax errors, correct them and
**	repreprocess your file.  If the error occurs again, contact your
**	INGRES support person.
*/

E_EQ051C_FIPS_4GL	
"INGRES 4GL access is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a statement that does not comply with SQL92 (entry level).  All 
**	statements beginning with 'EXEC 4GL' belong to the INGRES 4GL 
**	access language.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92, this statement should be removed.
*/

E_EQ051D_FIPS_UNIONSEL	
"Union of a column specification with a literal is an INGRES extension to SQL92 (entry level)."

/*%
** Explanation:
**	When the '-wsql=entry_sql92' flag is used, the embedded SQL
**	preprocessor will generate a warning message when it encounters
**	a union of a column specification with a literal.
**
** System Status:
**	The preprocessor will continue.  Unlike errors, warnings do not
**	cause the preprocessor to return a FAIL status.
**
** User Action:
**	In order to comply with SQL92 (entry level), the select lists of 
**	the union operands must be column specifications.
*/

E_EQ051E_FIPS_CINITVAL " "
/* This is an obsolete message, flagging an SQL89 restriction. */

E_EQ051F_FIPS_ESQLFLAG  
"The '%0c' flag is required for ESQL to generate applications that are SQL92 compliant."

/*%
** Explanation:
**      When the '-wsql=entry_sql92' flag is used, but other flags required
**	for the application to be SQL92 compliant have not been given, the
**      preprocessor will generate a warning message. 
**
** System Status:
**      The preprocessor will continue.  Unlike errors, warnings do not
**      cause the preprocessor to return a FAIL status.
**
** User Action:
**      In order to comply with SQL92, add the specified flag to the ESQL
**	command.
*/

E_EQ0520_NOT_BYREF "The keyword 'BYREF' was expected instead of '%0c'."

/*%
** Explanation:
**	The only keyword which can modify an argument specification in
**	a CALLFRAME or CALLPROC statement is 'BYREF'.  A different
**	word was seen.  
** 
** System Status:
**      Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Use BYREF instead of the invalid word.
*/

/*
** The EQUEL informational messages begin here.  The start up error messages
** are fast messages because the preprocessor is going to quit anyway.
*/

/*
**	eq_CMMDLINE:	Fake "file name" for preprocessor error messages
**	For example, a preprocessor error message related to the command
**	line will be printed "% File <Command Line> Line 0, Error ...."
*/

F_EQ0001_eqCMMDLINE	"<Command Line>"

/*
**	Eeq_INTACTBANNR: A line that gets printed after the interactive
**	preprocessor has been successfully started up.
*/

F_EQ0002_eqINTACTBANNR "%0c standard input:"

/*
**	eq_BANNER:	A line that gets printed to the terminal after the 
**			preprocessor has been successfully started up, 
**			e.g. "EQUEL file_name".
*/

F_EQ0003_eqBANNER	"%0c %1c:"

/*
**	eqUSRABORT: The preprocessor is aborting due to a user interrupt.
*/

F_EQ0004_eqUSRABORT	"%0c: Aborting because of User Interrupt . . ."

/*
**	eqINTRNLEX:  The preprocessor has encountered an internal exception.
*/

F_EQ0005_eqINTRNLEX	"%0c: Internal exception %1d in file '%2c', line %3d."

/*
**	eqINTRNSAV: This message accompanies S_EQ0002_EeqINTRNLEX
*/

F_EQ0006_eqINTRNLSAV	"     Please save input and output files."

/*
**	eqUSAGE:	This message is printed when the preprocessor
**			is incorrected invoked.  It is followed by the
**			following messages describing the correct command
**			line syntax.
*/		

F_EQ0007_eqUSAGE	"%0c/%1c usage: %2c%3c [FLAGS] [files]"

/*
**	eqFL[n]:	The following messages are printed out if the
**			user invokes the preprocessor with no arguments.
**			They follow the above message.
*/

F_EQ0008_eqFL0	" where FLAGS are:"
F_EQ0009_eqFL1	"  -l[o]       Produce a listing file; -lo includes generated code."
F_EQ000A_eqFL2	"  -d          Turn on run-time debugging."
F_EQ000B_eqFL3	"  -n.EXT      Set default EQUEL input INCLUDE suffix."
F_EQ000C_eqFL4	"  -o.EXT      Set default EQUEL output INCLUDE suffix."
F_EQ000D_eqFL5	"  -o.EXT      Set default ESQL output INCLUDE suffix."
F_EQ000E_eqFL6	"  -o          Do not generate an output file for INCLUDE files."
F_EQ000F_eqFL7	"  -f[FILE]    Send EQUEL output to FILE, or to standard output if FILE is omitted."
F_EQ0010_eqFL8	"  -f[FILE]    Send ESQL output to FILE, or to standard output if FILE is omitted."
F_EQ0011_eqFL9	"  -s          Read input file from standard input."
F_EQ0012_eqFL10	"  -w          Print warning messages." 
F_EQ0013_eqFL11	"  -wfips      Print warning messages when non-FIPS SQL statements found."
F_EQ0014_eqFL12	"  -wopen      Print warning messages when non-OPEN SQL statements found."
F_EQ0015_eqFL13	"  -rN         Set size of reals to N bytes (N=4|8)."
F_EQ0016_eqFL14	"  -iN         Set size of integers to N bytes (N=1|2|4)."
F_EQ0017_eqFL15	"  -{#|p}      Generate #line directives to the C compiler."
F_EQ0018_eqFL16	"  -a          Generate ANSI format code."
F_EQ0019_eqFL17	"  -F          INGRES internal flags (F=c|y|tl|to|q)."
F_EQ001A_eqFL18	"  -hDIR       Look in DIR for include files."
F_EQ001B_eqFL19	"  --          Print this help message."
F_EQ001C_eqFL20	"  -?          Print this help message."
F_EQ001D_eqFL21 "  -wsql=      Print warning messages when non-standard SQL syntax found."
F_EQ001E_eqFL22 "              Valid standards are:"
F_EQ001F_eqFL23 "                -wsql=entry_sql92       Flag non-Entry ANSI SQL92 syntax."
F_EQ0020_eqFL24 "                -wsql=open              Flag non-OPEN SQL syntax."
F_EQ0021_eqFL25 "  -sqlcode    Assume application declares SQLCODE."
F_EQ0022_eqFL26 "  -nosqlcode  Ignore application declaration of SQLCODE."
F_EQ0023_eqFL27 "  -extension= Set default ESQL output file suffix, e.g., '-extension=EXT'."
F_EQ0024_eqFL28 "  -{#|p}      Generate #line directives to the C++ compiler."
F_EQ0025_eqFL29 "  -prototypes Include ANSI C function protoytpes for INGRES runtime routines."
F_EQ0026_eqFL30 "  -F          Generate free format code."
F_EQ0035_eqFL31 "  -{m|multi}  Generate an output file for multi threaded operation."
F_EQ0036_eqFL32 "  -g64        Generate 64-bit Fortran source file."
/*
**	eq{E,W,F}MSGSTD:  A line printed before any preprocessor error message
**		    giving the file name and line number of the error.
**		    One of these messages is given when the input file is the
**		    standard input, i.e., when the preprocessor is  being
**		    used interactively.
*/

F_EQ0027_eqEMSGSTD	"%%%% Error in standard input, Line %0d:\n"
F_EQ0028_eqWMSGSTD	"%%%% Warning in standard input, Line %0d:\n"
F_EQ0029_eqFMSGSTD	"%%%% Fatal error in standard input, Line %0d:\n"

/*		    
**	eq{E,W,F}MSGFILE: Same as the above messages (EeqMSGSTD) but this 
**		    message is given when the input is from a file.
*/

F_EQ0030_eqEMSGFILE	"%%%% Error in file %0c.%1c, Line %2d:\n"
F_EQ0031_eqWMSGFILE	"%%%% Warning in file %0c.%1c, Line %2d:\n"
F_EQ0032_eqFMSGFILE	"%%%% Fatal error in file %0c.%1c, Line %2d:\n"

/*
**	eqPRMPT:    This message appears when the preprocessor is being
**		    used interactively.  A statement has generated
**		    a whole screen of code and there is still more.
*/

F_EQ0033_eqPRMPT	"[ return to continue, q to quit ] "

/*
**	The following string represents the first letter of the 
**	word that means "quit".  These string is compared to the user's
**	response to EqPRMPT above.  Case is ignored.
*/

F_EQ0034_eqQUIT	"q"

/*
** Slow  messages start here.  Note that a slow message cannot have the
** same number as an error message.  We start at 901, leaving a big range for 
** extra error numbers.
*/

