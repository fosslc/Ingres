/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ere0.msg -	Message file for class E0
**
** Description:
**	This file defines the messages for class E0.
**
** History:
**	24-aug-1987 (barbara)
**		Created.
**	15-sep-1993 (sandyd)
**		Expanded E_E00008_hcVCHIGNRE to include VARBYTE as well as
**		VARCHAR.  (This message was sufficiently generic to cover
**		both cases --- with slight rewording.)
**/

# define	_E0_CLASS	141



E_E00001_hcDEFINE	"The #define statement may be used only with values, not names.\n
Use typedef if you wish to make '%0c' a synonym for a type."

/*%
** Explanation:
**	The #define directive accepts only integer, floating point or string
**	literals as the replacement token.  You may not use arbitrary text as
**	the replacement token.  To define type names you should use typedef.
**	The embedded preprocessor #define is not as versatile as
**	the C #define.
**
** System Status:
**
** User Action:
*/

E_E00002_hcIGNCAST	"Cast of #define value is ignored."

/*%
** Explanation:
**	The preprocessor ignores a cast of the replacement value in a
**	#define statement.  Casts in general are not supported by the
**	embedded C preprocessor.
**
**	Remove the cast from the #define statement.
**
** System Status:
**
** User Action:
*/

E_E00003_hcINDIR	"Incorrect indirection on variable '%0c'.\n
Variable is subscripted, [], or dereferenced, *, %1c time(s) but declared
with indirection of %2c."

/*%
** Explanation:
**	This error occurs when the address or value of a variable is
**	incorrectly expressed because of faulty indirection.  For example,
**	the name of an integer array has been given instead of a single
**	array element, or, in the case of character string variables, a single
**	element of the string (ie, a character) has been given instead
**	of a pointer to the string or the name of the array.
**
**	Either redeclare the variable with the intended indirection,
**	or change its use in the current statement. 
**
** System Status:
**
** User Action:
*/

E_E00004_hcSTRUCTREF	"Last component of structure reference '%0c' is illegal."

/*%
** Explanation:
**	This error occurs when the preprocessor encounters an unrecognized
**	name in a structure reference.  The user may have incorrectly typed
**	the name of structure element, or may have failed to declare it to the 
**	preprocessor.
**
**	Check for misspellings in component names and that all of the
**	structure components have been declared to the preprocessor.
**
** System Status:
**
** User Action:
*/

E_E00005_hcUNCLOSED	"Unclosed C block - %0c unbalanced left brace(s)."

/*%
** Explanation:
**	The preprocessor reached the end of the file still expecting one or
**	more closing curly braces '}'.  Make sure that you have no '{' in
**	an unclosed character or string constant, or have not accidentally
**	commented out a closing curly brace.  Also remember that the
**	preprocessor ignores #ifdef directives, so having several '{' in
**	alternate paths of an #ifdef will confuse the preprocessor.
**
**	Balance curly braces declared to the preprocessor.
**
** System Status:
**
** User Action:
*/

E_E00006_hcNOFUNCDEC	"Unsupported forward declaration of C function '%0c'."

/*%
** Explanation:
**	(EQUEL)
**	The preprocessor does not support function declarations.
**	For example, the declaration:
**
**	##	int func();
**
**	will cause this error.
**
**	Remove the ## mark from the function declaration.
**
** System Status:
**
** User Action:
*/

E_E00007_hcNESTFUNCDEF	"Unsupported definition of nested C function '%0c'.\n
Check for missing closing brace of preceding function."

/*%
** Explanation:
**	(EQUEL)
**	The preprocessor does not support nested function definitions.  This
**	error commonly occurs when the user has omitted the ## mark on the
**	closing brace of the previous function definition.
**
**	Check that functions to be defined to the preprocessor have opening and
**	closing braces.
**
** System Status:
**
** User Action:
*/

E_E00008_hcVCHIGNRE	"Incorrect declaration of C varchar (or varbyte) variable is ignored.\n
The members of a varchar (or varbyte) structure variable may consist only 
of a short integer and a fixed length character array."

/*%
** Explanation:
**	Varchar or varbyte variables (variables declared with the varchar or
**	varbyte storage class) must conform to an exact varying-length string 
**	template so that INGRES can map to and from them at run-time.  The 
**	length field must be exactly two-bytes (derived from a short), and 
**	the character string field must be a single-dimensioned C character 
**	array.  The varchar (or varbyte) clause must be associated with a 
**	variable declaration and a not a type definition or structure tag 
**	declaration.
**	
**	Check the varchar (or varbyte) structure declaration.  Make sure that 
**	both structure members are declared properly.
**
** System Status:
**
** User Action:
*/

E_E00009_hcMISSEQUAL	"Missing '=' in the initialization part of a C declaration."

/*%
** Explanation:
**	The preprocessor allows automatic initialization of variables and
**	expects the regular C syntax.
**
**	Insert an equals sign between the variable and the initializing value.
**
** System Status:
**
** User Action:
*/
