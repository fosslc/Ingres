/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ere3.msg -	Message file for class E3
**
** Description:
**	This file defines the messages for class E3.
**
** History:
**	28-aug-1987 (mark)
**		Created.
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**/

# define	_E3_CLASS	144



E_E30001_hbARRAY	"BASIC array '%0c' should be subscripted."

/*%
** Explanation:
**	A variable declared as an array must be subscripted when used.
**
** System Status:
**
** User Action:
*/

E_E30002_hbCONS	"Value assigned does not match BASIC constant type."

/*%
** Explanation:
**	The type of the literal assigned to the constant name does not
**	match the type of the CONSTANT declaration.  Numerics and strings 
**	cannot be mixed.
**
** System Status:
**
** User Action:
*/

E_E30003_hbNESTDEF	"Nested BASIC DEF statements are not allowed."

/*%
** Explanation:
**	(EQUEL)
**	Nested DEF statements are not supported by the preprocessor.
**
** System Status:
**
** User Action:
*/

E_E30004_hbSINGDEF	"Single-line BASIC DEF statements are not allowed."

/*%
** Explanation:
**	(EQUEL)
**	The preprocessor should only be informed of a DEF statement if it 
**	includes embedded statements.  Single-line DEF statements, which
**	cannot include embedded statements, should not be marked to the
**	preprocessor.
**
** System Status:
**
** User Action:
*/

E_E30005_hbEND	"BASIC identifier '%0c' expected on END RECORD/END GROUP statement."

/*%
** Explanation:
**	If you name the RECORD or GROUP declaration on the END RECORD or END
**	GROUP statement, then the name must be the same with which the RECORD
**	or GROUP was declared.
**	
** System Status:
**
** User Action:
*/

E_E30006_hbNOSUBS	"RECORD or GROUP subscripts are required in '%0c'."

/*%
** Explanation:
**	Within the specified variable reference, the record component lacks
**	subscripts at the group or record level.
**
** System Status:
**
** User Action:
*/

E_E30007_hbXSUBS	"RECORD or GROUP subscripts should not be used in '%0c'."

/*%
** Explanation:
**	Within the specified variable reference, the record component has extra
**	subscripts at the group or record level.
**
** System Status:
**
** User Action:
*/

E_E30008_hbIGNODEC	"DECLARE INGRES statement in function or subroutine is ignored."

/*%
** Explanation:
**	(EQUEL)
**	The ## DECLARE INGRES statement is automatically generated at the
**	entry to a function or subroutine whose definition is known to the
**	preprocessor. In this instance, the statement is ignored.
**
** System Status:
**
** User Action:
*/

E_E30009_hbDEFPARM	"Subscripts/string lengths are not allowed on DEF function parameters."

/*%
** Explanation:
**	(EQUEL)
**	Array subscripts and string lengths cannot be declared with parameters
**	to DEF functions.
**
** System Status:
**
** User Action:
*/

E_E3000A_hbEXTNTYPE	"Incorrect type used on EXTERNAL variable or constant."

/*%
** Explanation:
**	EXTERNAL variables can be declared with a limited subset of data types.
**	The declaration refers to an unknown or non-EXTERNAL data type.
**
** System Status:
**
** User Action:
*/

E_E3000B_hbEXTID	"EXTERNAL identifiers may not have subscripts or an assignment clause."

/*%
** Explanation:
**	The preprocessor does not support EXTERNAL arrays, or size-initialized
**	variables.  Use DIMENSION or COMMON for global non-scalar declarations.
**
** System Status:
**
** User Action:
*/

E_E3000C_hbCNSTYPE	"CONSTANT declaration may not refer to program-defined RECORD type."

/*%
** Explanation:
**	CONSTANT declarations may not refer to RECORD data types, even if they
**	have been previously defined.
**
** System Status:
**
** User Action:
*/

E_E3000D_hbCNSUBS	"CONSTANT declaration may not be subscripted."

/*%
** Explanation:
**	CONSTANT declarations may not refer to arrays.
**
** System Status:
**
** User Action:
*/

E_E3000E_hbCNSASSG	"Assignment clause missing from BASIC CONSTANT declaration."

/*%
** Explanation:
**	A CONSTANT declaration must include an assignment to a numeric or string
**	literal.
**
** System Status:
**
** User Action:
*/

E_E3000F_hbDIMSUB	"Array subscripts missing from BASIC DIMENSION declaration."

/*%
** Explanation:
**	DIMENSION declarations must include array subscripts.
**
** System Status:
**
** User Action:
*/

E_E30010_hbDIMSTR	"String length is not allowed on BASIC DIMENSION declaration."

/*%
** Explanation:
**	DIMENSION declarations may not include string lengths nor an assignment
**	clause.
**
** System Status:
**
** User Action:
*/

E_E30011_hbSTR	"String length may only qualify a variable of STRING type."

/*%
** Explanation:
**	An assignment clause (string length) is only allowed with STRING
**	declarations.
**
** System Status:
**
** User Action:
*/

E_E30012_hbDYNSTR	"String length is not allowed on dynamic string variable."

/*%
** Explanation:
**	A dynamic STRING type may not specify a length.  A length may only
**	be specified with static STRING declarations.
**
** System Status:
**
** User Action:
*/

E_E30013_hbTYPE	"BASIC variables must have an explicit type."

/*%
** Explanation:
**	All variable declarations must have an explicit type.  Default types
**	are not accepted by the preprocessor.
**
** System Status:
**
** User Action:
*/

E_E30014_hbLIT	"Found identifier '%0c' where literal expected."

/*%
** Explanation:
**	You must use numeric or string literals to initialize constants.  You
**	must use a numeric literal when declaring the length of a static string
**	variable.
**
** System Status:
**
** User Action:
*/

E_E30015_hbNOING	"No ## DECLARE INGRES before first EQUEL statement."

/*%
** Explanation:
**	(EQUEL)
**	You must issue the ## DECLARE statement before the first embedded
**	statement.  The preprocessor generates code that references subroutines
**	and functions declared within a file included by the ## DECLARE
**	statement.  Without issuing the ## DECLARE statement, the BASIC
**	compiler will not accept those references.
**
** System Status:
**
** User Action:
*/

E_E30016_hbPASS	"BY %0c is an incorrect BASIC parameter-passing mechanism."

/*%
** Explanation:
**	(EQUEL)
**	The preprocessor only accepts BY REF and BY DEF as parameter-passing
**	mechanisms for subprogram parameters.
**
** System Status:
**
** User Action:
*/

E_E30017_hbQUOTE	"Quotes may not be embedded in string literals."

/*%
** Explanation:
**	In order to embed a quote within a string literal, you must 
**	use the BASIC rules to assign the string literal to a string
**	variable, and use the variable in the embedded statement.
**
** System Status:
**
** User Action:
*/

E_E30018_hbSEENDEC	"DECLARE INGRES statement already seen."

/*%
** Explanation:
**	(EQUEL)
**	You have issued more than one ## DECLARE statement in your main
**	program, or you have issued a ## DECLARE statement in a subprogram
**	that was not defined to the preprocessor.
**
** System Status:
**
** User Action:
*/

E_E30019_hbUNEXEND	"Unexpected BASIC END%0c statement is ignored."

/*%
** Explanation:
**	(EQUEL)
**	The END statement to a FUNCTION, SUB, DEF or main program is out
**	of context.
**
** System Status:
**
** User Action:
*/

E_E3001A_hbELEM	"Field '%0c' in record '%1c' is not elementary."

/*%
** Explanation:
**	The specified field was used as a variable.  However, the field is
**	not a scalar-valued variable (numeric or string).  You cannot
**	use arrays or records to set or retrieve data in this context.
**
** System Status:
**
** User Action:
*/

E_E3001B_hbPREC		"Illegal precision '%0' for a decimal declaration."

/*%
** Explanation:
**	A decimal declaration "DECLARE DECIMAL(p,s)" specified a precision 
**	of zero or the conversion of the input precision from a string 
**	to an integer failed.  Precision must be an integer constant 
**	greater than zero.
**
** System Status:
**
** User Action:
*/

E_E3001C_hbSCALE	"Illegal scale '%0' for a decimal declaration."

/*%
** Explanation:
**	The scale in a decimal declaration "DECLARE DECIMAL(p,s)" failed  
**      to be converted from a string to an integer. Scale must be 
**      integer constant.
**
** System Status:
**
** User Action:
*/
E_E3001D_hbNOSQLCODE   "SQLCODE has been declared, but the -sqlcode flag
was not specified on the command line."
/*%
** Explanation:
**      Because the standard SQLCA include file declares an SQLCODE,
**      which would duplicate the one you've just declared, you must
**      use the -sqlcode flag.  This flag tells the preprocessor that
**      the application has already declared an SQLCODE before it tries
**      to include the SQLCA.
**
** System Status:
**      You will likely get a multiply defined symbol error, either from
**      the fortran compiler or from the linker.
**
** User Action:
**      recompile your application using the -sqlcode flag.  See the BASIC
**      companion guide (and possibly the release notes) for information on
**      command line options.
*/

E_E3001E_hbSINGLE    "The SINGLE datatype is unsupported on AlphaVMS, 
convert to SFLOAT"
/*%
** Explanation:
**      Only IEEE float data types are supported on Alpha VMS. SINGLE is 
**      not an IEEE float data type.
**
** System Status:
**      The BASIC pre-processor will flag a SINGLE data type specification
**      as an error message.  It must be changed before the BASIC program
**      can be pre-processed.
**
** User Action:
**      Update all references to the SINGLE data type to SFLOAT as this is
**      the equivalent IEEE data type.
*/

E_E3001F_hbDOUBLE    "The DOUBLE datatype is unsupported on AlphaVMS, 
convert to TFLOAT"
/*%
** Explanation:
**      Only IEEE float data types are supported on Alpha VMS. DOUBLE is 
**      not an IEEE float data type.
**
** System Status:
**      The BASIC pre-processor will flag a DOUBLE data type specification
**      as an error message.  It must be changed before the BASIC program
**      can be pre-processed.
**
** User Action:
**      Update all references to the DOUBLE data type to TFLOAT as this is
**      the equivalent IEEE data type.
*/
