/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ere1.msg -	Message file for class E1
**
** Description:
**	This file defines the messages for class E1.
**
** History:
**	25-aug-1987 (mark)
**		Created.
**	16-dec-1992 (larrym)
**	    added E_E1000D_hfNOSQLCODE.
**/

# define	_E1_CLASS	142



E_E10001_hfBADTYPE	"Unsupported FORTRAN type '%0c' used.  DOUBLE assumed.\n
INGRES does not support the FORTRAN types COMPLEX and DOUBLE COMPLEX."

/*%
** Explanation:
**	There is no INGRES type corresponding to COMPLEX or DOUBLE COMPLEX,
**	so the preprocessor does not map this declaration to an INGRES type.
**
** System Status:
**	The preprocessor will continue to generate code as if you had declared
**	the variable in question to be of type DOUBLE PRECISION.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Do not make this declaration visible to the preprocessor unless
**	necessary.  If you want to store the two REAL (or DOUBLE PRECISION)
**	components of a COMPLEX (or DOUBLE COMPLEX) variable then declare a
**	pair of REAL (or DOUBLE PRECISION) variables to the preprocessor,
**	copy the components to them, and then store the copies.
*/

E_E10002_hfDEFINE	"FORTRAN PARAMETER may only be used with values.\n
Type names, variable names, and PARAMETER names are not allowed."

/*%
** Explanation:
**	You have used the FORTRAN "PARAMETER name = value" statement,
**	but "value" is not an integer constant, a floating constant, or a
**	string constant.  You may have used the name of a FORTRAN data type,
**	or a variable (or PARAMETER) name instead of one of the legal constant
**	types.
**
** System Status:
**	The statement is ignored.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Do not make this statement known to the preprocessor if you do not use
**	the name being defined in any INGRES statement.  If you do wish INGRES
**	to know about this name then you must change the "value" to be a
**	constant.
*/

E_E10003_hfINDIR	"Incorrect indirection on variable '%0c'.\n
The variable is declared as an array and is not subscripted, or is
subscripted but is not declared as an array (%1c, %2c)."

/*%
** Explanation:
**	This error occurs when the value of a variable is
**	incorrectly expressed because of faulty indirection.  For example,
**	the name of an integer array has been given instead of a single
**	array element, or, in the case of string variables, a single
**	element of the string (ie, a character) has been given instead
**	of the name of the array.
**
** System Status:
**	The preprocessor will continue to generate code, but the program will
**	not execute correctly if it is compiled and run.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Either redeclare the variable with the intended indirection,
**	or change its use in the current statement.
*/

E_E10004_hfSTRUCTREF	"Last FORTRAN structure field referenced in '%0c' is unknown."

/*%
** Explanation:
**	This error occurs when the preprocessor encounters an unrecognized
**	name in a structure reference.
**
** System Status:
**	The preprocessor will continue to generate code, but this statement
**	will either cause a run-time error or produce the wrong result if
**	the resulting program is compiled and run.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Check for misspellings in field names and ensure that all of
**	the structure fields have been declared to the preprocessor.
*/

E_E10005_hfUNCLOSED	"Unclosed FORTRAN block - %0c unbalanced END(s)."

/*%
** Explanation:
**	The preprocessor reached the end of the file still expecting one or
**	more closing END statements.  Make sure that you have no 'ENDs' in an
**	unclosed character or string constant, or have not accidentally
**	commented out a closing END.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Balance each PROGRAM, PROCEDURE, or FUNCTION statement with a closing
**	END.
*/

E_E10006_hfNESTFUNCDEF	"Unsupported definition of nested FORTRAN function '%0c'."

/*%
** Explanation:
**	EQUEL read the beginning of a subprogram (PROGRAM, PROCEDURE, or
**	FUNCTION) while still in a previous subprogram definition.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Ensure that the END statement for a previous subprogram definition is
**	not missing.  PASCAL programmers should remember that FORTRAN does
**	not allow nested subprogram definitions.
*/

E_E10007_hfDECLARE	"No ## DECLARE before first EQUEL/FORTRAN statement '%0c'."

/*%
** Explanation:
**	You must issue a ## DECLARE statement lexically before any INGRES
**	statement.
**
** System Status:
**	The generated will probably not compile.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Issue a DECLARE statement anywhere a declaration would be legal
**	before you issue any INGRES statement.
*/

E_E10008_hfREDECLARE	"Reissue of ## DECLARE in FORTRAN program unit.\n
The second declaration is ignored."

/*%
** Explanation:
**	The DECLARE statement should be issued exactly once in each FORTRAN
**	program unit.  This can also be caused by forgetting to ## the
**	PROGRAM, PROCEDURE, or FUNCTION line (and the matching END).
**
** System Status:
**	EQUEL will ignore the extraneous DECLARE statement.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Remove the extra DECLARE statement, or ## the subprogram head
**	and END.
*/

E_E10009_hfDECFORMS	"No ## DECLARE FORMS before forms statement '%0c' in FORTRAN program unit."

/*%
** Explanation:
**	You must issue a ## DECLARE FORMS statement lexically before any FORMS
**	statement.
**
** System Status:
**	The generated will probably not compile.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Issue a DECLARE FORMS statement anywhere a declaration would be legal
**	before you issue any FORMS statement.
*/

E_E1000A_hfNOSTRUCT	"Undefined structure name '%0c' used in RECORD declaration."

/*%
** Explanation:
**	You have declared a RECORD variable using the name of a STRUCTURE
**	that is unknown to the preprocessor.
**
** System Status:
**	The preprocessor will continue to generate code, but the resulting
**	program will not run properly.
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	If you do not use this variable with an INGRES statement then
**	remove the record declaration.  Otherwise ensure that the
**	corresponding STRUCTURE declaration is made known to the preprocessor.
*/

E_E1000B_hfELEM	"Field '%0c' in record '%1c' is not an elementary variable."

/*%
** Explanation:
**	RECORD variables used in SQL as a single object must contain only
**	scalar fields.  Arrays and nested records are not allowed in this
**	context.
**
**	Example:
**	    EXEC SQL BEGIN DECLARE SECTION
**		structure /object/
**		    character*10	oname
**		    integer		ovals(4)
**		end structure
**		record /object/	obj
**	    EXEC SQL END DECLARE SECTION
**	    EXEC SQL SELECT * INTO :obj FROM objects
**	This will cause an error on "obj.oname", as it is an array variable.
**		
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
**	Either flatten the record variable declaration or enumerate all
**	fields when using the variable.
*/

E_E1000C_hfBADSIZE	"Illegal length specified for FORTRAN numeric variable."

/*%
** Explanation:
**	FORTRAN integer variables may be either 1, 2, or 4 bytes, and floating-
**	point variables may be either 4 or 8 bytes.  Specifying any other
**	value is illegal.
**
** System Status:
**	Upon completion, the preprocessor will return a FAIL status.
**
** User Action:
*/

E_E1000D_hfNOSQLCODE	"SQLCODE has been declared, but the -sqlcode flag
was not specified on the command line."
/*%
** Explanation:
**	Because the standard SQLCA include file declares an SQLCODE,
**	which would duplicate the one you've just declared, you must 
**	use the -sqlcode flag.  This flag tells the preprocessor that 
**	the application has already declared an SQLCODE before it tries
**	to include the SQLCA.
**
** System Status:
**	You will likely get a multiply defined symbol error, either from 
**	the fortran compiler or from the linker.
**
** User Action:
**	recompile your application using the -sqlcode flag.  See the FORTRAN
**	companion guide (and possibly the release notes) for information on 
**	command line options.
*/
