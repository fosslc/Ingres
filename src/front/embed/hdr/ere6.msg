/*
**  Copyright (c) 2004 Ingres Corporation
*/

/**
** Name:	ere6.msg -	Message file for class E6
**
** Description:
**	This file defines the messages for class E6.
**
** History:
**	27-aug-1987 (mark)
**		Created.
**	15-dec-1987 (neil)
**		Added comments/modified messages.
**/

# define	_E6_CLASS	147



E_E60001_haMUSTINDIR	"The ADA variable '%0c' is an array and must be subscripted."

/*
** Explanation:
**	A variable declared as an array must be subscripted when referenced.
**	The preprocessor does not confirm that you use the correct number
**	of subscripts.  A variable declared as a 1-dimensional array of
**	characters, must not be subscripted as it refers to a character
**	string.
**
** System Status:
**
** User Action:
*/

E_E60002_haCANTINDIR	"The ADA variable '%0c' is not an array and must not be subscripted."

/*%
** Explanation:
**	A variable not declared as an array cannot be subscripted.  You cannot
**	subscript string variables in order to refer to a single character or
**	a slice of a string (sub-string).
**
** System Status:
**
** User Action:
*/

E_E60003_haBADTYPE	"The ADA identifier '%0c' is not a declared type."

/*%
** Explanation:
**	The identifier was used as an ADA type name in an object or type
**	declaration.  This identifier has not yet been declared to the
**	preprocessor and is not a preprocessor-predefined type name.
**
** System Status:
**
** User Action:
*/

E_E60004_haCHARDIMS	"The ADA CHARACTER variable '%0c' must be a 1-dimensional array."

/*%
** Explanation:
**	Variables of type CHARACTER can only be declared as 1-dimensional
**	arrays.  You cannot use a single character or a multi-dimensional
**	array of characters as an INGRES string.  Note that you can use a 
**	multidimensional array of type STRING.
**
** System Status:
**
** User Action:
*/

E_E60005_haDIGITS	"The ADA DIGITS clause '%0c' is out of the range 1..16."

/*%
** Explanation:
**	Embedded ADA supports D_FLOAT floating point variables.  Consequently,
**	all DIGITS specifications must be within the specified range.
**
** System Status:
**
** User Action:
*/

E_E60006_haINCPKG	"Statement '%0c' is embedded in INCLUDE file package specification."

/*%
** Explanation:
**	Preprocessor INCLUDE files may only be used for ADA package
**	specifications.  The preprocessor generates an ADA WITH clause
**	for the package. No executable statements may be included in the
**	file because the code generated will not be accepted by the ADA
**	compiler in a package specification.
**
** System Status:
**
** User Action:
*/

E_E60007_haNMOVERFLOW	"Too many names (%0c) in ADA identifier list.  Maximum is %1c."

/*%
** Explanation:
**	ADA identifier lists cannot have too many names in the comma-separated
**	name list.  The name specified in the error message caused the
**	overflow, and the remainder of the list is ignored.  Rewrite the
**	declaration so that there are fewer names in the list.
**
** System Status:
**
** User Action:
*/

E_E60008_haNMUNDERFLOW	"The ADA identifier list has come up short."

/*%
** Explanation:
**	The stack used to store comma separated names in ADA declarations has
**	been corrupted.  Try rearranging the list of names in the declaration.
**
** System Status:
**
** User Action:
*/

E_E60009_haNOTINITED	"The ADA CONSTANT declaration of '%0c' must be initialized."

/*%
** Explanation:
**	CONSTANT declarations must include an initialization clause.
**
** System Status:
**
** User Action:
*/

E_E6000A_haNOTVAR	"The ADA identifier '%0c' is either a constant or an enumerated literal."

/*%
** Explanation:
**	The named identifier was used to retrieve data from INGRES.  A constant,
**	an enumerated literal and a formal parameter with the IN mode are
**	all considered illegal for the purpose of retrieval.
**
** System Status:
**
** User Action:
*/

E_E6000B_haBADRECALL	"The ADA variable '%0c' with '.ALL' clause is illegal."

/*%
** Explanation:
**	The ADA .ALL clause, as specified with access objects, can be used only
**	if the variable is an access object pointing at a single scalar-valued
**	type.  If the type is not scalar valued, or if the access object is 
**	pointing at a record or array, then the use of .ALL is illegal.
**
** System Status:
**
** User Action:
*/

E_E6000C_haSCALRECALL	"The ADA variable '%0c' with '.ALL' clause is not a scalar type."

/*%
** Explanation:
**	The ADA .ALL clause, as specified with access objects, can be used only
**	if the variable is an access object pointing at a single scalar-valued
**	type.  If the type is not scalar valued, or if the access object is 
**	pointing at a record or array, then the use of .ALL is illegal.
**
** System Status:
**
** User Action:
*/

E_E6000D_haRECORDREF	"Last component in ADA record qualification '%0c' is illegal."

/*%
** Explanation:
**	The last component referenced in a record qualification is not a
**	member of the record.  If this component was supposed to be declared
**	as a record, the following components will cause preprocessor syntax
**	errors.
**
** System Status:
**
** User Action:
*/

E_E6000E_haRNMBAD	"In ADA RENAMES statement, '%0c' must be a constant or a variable."

/*%
** Explanation:
**	The target object of a RENAMES statement must be a constant or a 
**	variable, and the item being declared is used a synonym for the
**	target object.
**
** System Status:
**
** User Action:
*/

E_E6000F_haRNMCOMPAT	"In ADA RENAMES statement, object is incompatible with type."

/*%
** Explanation:
**	The type of the target object in the RENAMES statement must be 
**	compatible in base type, size and array dimensions with the type
**	name specified in the declaration.
**
** System Status:
**
** User Action:
*/

E_E60010_haRNMMANY	"Only one name may be declared in an ADA RENAMES statement."

/*%
** Explanation:
**	One object can rename only one other object.
**
** System Status:
**
** User Action:
*/

E_E60011_haUNCLOSED	"Unclosed ADA block.  There are %0c block(s) left open."

/*%
** Explanation:
**	(EQUEL)
**	If a file is terminated early or the END statement closing an ADA
**	compilation unit is missing, this error will occur.  If syntax errors
**	were issued while parsing the compilation unit header, correct those
**	errors first.
**	
** System Status:
**
** User Action:
*/

E_E60012_haUNDEC	"The ADA variable '%0c' has not been declared."

/*%
** Explanation:
**	The named identifier was used where a variable must be used to
**	set or retrieve INGRES data.  The variable has not yet been declared.
**
** System Status:
**
** User Action:
*/

E_E60013_haTYPUNSUPP	"The ADA type %0c is not supported."

/*%
** Explanation:
**	Some ADA types are not supported because they are not compatible
**	with the INGRES run-time system.
**
** System Status:
**
** User Action:
*/

E_E60014_haVARRECORD	"The ADA variable '%0c' is a record, not a scalar value."

/*%
** Explanation:
**	The named variable qualification refers to a record.  It was used
**	where a variable must be used to set or retrieve INGRES data.  This
**	error may also cause syntax errors on record component references.
**
** System Status:
**
** User Action:
*/

E_E60015_haWITHEQUEL	"You must issue a '## WITH %0c' before statement '%1c'."

/*%
** Explanation:
**	(EQUEL)
**	If your compilation unit includes forms statements you must issue the
**	WITH EQUEL_FORMS clause.  Otherwise you must issue the WITH EQUEL
**	clause.
**
** System Status:
**
** User Action:
*/

E_E60016_haUNSUPP	"The ADA statement %0c is not supported."

/*%
** Explanation:
**	Statements that modify the internal representation of variables
**	that interact with INGRES are not supported.
**
** System Status:
**
** User Action:
*/
