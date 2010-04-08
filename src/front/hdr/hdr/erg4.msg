/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erg4.msg - messages for the G4 class.
**
** Description:
**	These are messages the user sees from EXEC 4GL.  The messages are the
**	same for ABF/4GL and Windows4GL, although some apply only to one of
**	the 4GLs.
**
** History:
**	10/92 (Mike S) Initial version
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**/

# define	_G4_CLASS	216


/*
** Fast messages
*/

/*
** object is W4GL-speak; record is ABF-speak.  Many of the error messages which
** are common between ABF and W4GL are parameterized to use one of them.
*/
F_G40001_OBJECT	"object"
F_G40002_RECORD "record"

F_G40003_CONSTANT "global constant"
F_G40004_VARIABLE "global variable"
F_G40005_ATTRIBUTE "attribute"
F_G40006_PARAMETER "parameter"

E_G42710_NULLOBJECT	
"A null object was passed to EXEC 4GL %0c.\n\
A null object was used in a context where a non-null object is required.
The EXEC 4GL %0c statement will fail."

/*%
** Explanation:
** 	A NULL object was used in an EXEC 4GL statement in a context which
** 	requires a non-NULL object; for example, in GET ATTRIBUTE.
**
** System Status:
** 	The EXEC 4GL statement will fail.
**
** User Action:
**	Correct the program to use a non-NULL object.
**
*/

E_G42711_BADVALUE
"An invalid value was passed to EXEC 4GL %0c.\n\
The value given does not represent a valid %1c or array.
The EXEC 4GL %0c statement will fail."

/*%
** Explanation:
**      The value given to EXEC 4GL does not represent a valid 4GL object,
** 	record, or array.  No further processing can be done.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program to use a valid value.
**
*/

E_G42712_NOTARRAY
"A non-array was passed to EXEC 4GL %0c.\n\
A non-array was used in a context where an array is required.
The EXEC 4GL %0c statement will fail."

/*%
** Explanation:
**      The value given to EXEC 4GL does not represent an array.
**	No further processing can be done.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program to use an array.
**
*/

E_G42713_BADINDEX
"A bad index was passed to EXEC 4GL %0c.\n\
%1d is not the index of a row in the given array.  The EXEC 4GL %0c
statement will fail."

/*%
** Explanation:
**      The value given to EXEC 4GL in not a valid index for the given array.
**      No further processing can be done.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program to use a valid index.
*/

E_G42714_BADROWTYPE
"Invalid object type for array.\n\
An attempt was made by EXEC 4GL %0c to place a row of an invalid type into an
array.  The row's type is %1c, which is not a subclass of the array's
type, %2c.  The statement will fail."

/*%
** Explanation:
**      The type of an array row must be a subclass of the array's type.
**      This was not the case, so the row cannot become part of the array.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program to use an object of the correct type.
*/

E_G42715_NOT_SUPPORTED
"EXEC 4GL %0c is not supported by %1c.\n\
This statement, which is not supported by the product currently running,
will fail."

/*%
** Explanation:
**      The EXEC 4GL statement last issued is not supported by this
**	product.  It will fail.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program not to use this statement.
*/

E_G42716_NO_INCL_APP
"Included application '%1c' not found.\n\
The %2c '%3c' could not be found because the indicated included
application is not in the current scope.  The EXEC 4GL %0c statement will
fail."

/*%
** Explanation:
**	An included application which should contain the global was explicitly
**	specified.  This included application does not exist.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program to name the correct included application.
*/

E_G42717_NO_GLOBAL
"The %1c '%2c' was not found.\n\
The %1c does not exist.  The EXEC 4GL %0c statement will fail."

/*%
** Explanation:
**	An attempt was made to get or set a global which does not exist.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program to name the correct global.
*/

E_G42718_WRONG_GLOBAL_TYPE
"'%1c' is a %2c, not a %3c.\n\
The EXEC 4GL %0c statement will fail because the named global is not
the correct kind of component."

/*%
** Explanation:
**      An attempt was made to get or set a global.  The name used for the
**	global actually names a different kind of component.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**      Correct the program to name the correct kind of global.
*/

E_G42719_INTERNAL_ERROR
"An internal error has occured.\n\
Internal error number %1d occured while trying to execute the
statement EXEC 4GL %0c.  Please notify your Site Technical Contact."

/*%
** Explanation:
**	An internal error occured.
**
** System Status:
**      The EXEC 4GL statement will fail.
**
** User Action:
**	Notify the Site Technical Contact.
*/

E_G4271A_NO_NULL_INDICATOR
"Program variable does not have null indicator.\n\
%1c '%2c' contains a NULL value.  However,
the program variable where its value is to be placed
does not have an attached null indicator.  It will not be
possible to place the NULL value into the program
variable."

/*%
** Explanation:
**	An attempt was made to fetch a NULL value into a program
**	variable, but no null indicator was specified.
**
** System Status:
**	No change will be made to the contents of the
**	program variable.
**
** User Action:
**	Add a null indicator to the program variable.
*/

E_G4271B_DATA_CONVERSION_GET
"A datatype conversion error occurred.\n\
A datatype conversion error occurred while getting a value from
%1c '%2c'.  The value of the program variable will not be changed."

/*%
** Explanation:
**      An attempt was made to place a value into a program
**	variable, but a datatype conversion error occurred.
**
** System Status:
**      No change will be made to the contents of the
**      program variable.
**
** User Action:
**	Use the correct type of program variable.
*/

E_G4271C_DATA_CONVERSION_SET
"A datatype conversion error occurred.\n\
A datatype conversion error occurred while setting the value
of %1c '%2c' from program variables.  The value of the
%1c will not be set."

/*%
** Explanation:
**      An attempt was made to get a value from a program
**	variable into a 4GL variable or parameter, but a datatype 
**	conversion error occurred.
**
** System Status:
**      No change will be made to the contents of the
**      4GL variable or parameter.
**
** User Action:
**	Use the correct type of program variable.
*/

E_G4271D_NULL_VALUE
"Cannot place a NULL value into %1c '%2c'.\n\
An attempt was made to place a NULL value into a non-nullable
%1c.  The value of the %1c will not be changed."

/*%
** Explanation:
**      An attempt was made to place a NULL value into a
**	non-NULLable 4GL variable.
**
** System Status:
**      No change will be made to the contents of the
**      4GL variable.
**
** User Action:
**	Use the correct type of 4GL variable.
*/

E_G4271E_BAD_OBJECT
"Bad value for %3c or array passed to %1c '%2c'.\n\
An attmept was made to place an illegal %3c or array value
into the %1c.  The value of the %1c will not be set."

/*%
** Explanation:
**      A bad object value was passed into a 4GL variable or parameter.
**
** System Status:
**      No change will be made to the contents of the
**      4GL variable or parameter.
**
** User Action:
**	Use a correct object value.
*/

E_G4271F_BAD_ATTR
"No attribute named '%1c'.\n\
An attempt was made to access an attribute which is not part of the current
%2c.  The EXEC 4GL %0c statement will fail."

/*%
** Explanation:
**	An invalid attribute name was used as an attribute to get or set.
**
** System Status:
**	No action will be taken for that attribute.
**
** User Action:
**      Use a correct attribute name.
*/

E_G42720_NULL_SQLDA
"The address of the SQLDA is a null pointer.\n\
The EXEC 4GL %0c statement will fail."

/*%
** Explanation:
**	A null pointer was used for the SQLDA in a dynamic EXEC 4GL
**	statement.
**
** System Status:
**	The statement will fail.  No data will be transferred.
**
** User Action:
**      Supply a valid SQLDA pointer.
*/

E_G42721_NULL_DATA
"The SQLDA data pointer is null for element %1d.\n\
The EXEC 4GL %0c statement will fail for this element."

/*%
** Explanation:
**      A null pointer was used for an SQLDA data pointer for
**	a dynamic EXEC 4GL statement.
**
** System Status:
**      The statement will fail to transfer data for this element.
**
** User Action:
**      Supply a valid SQLDA data pointer.
*/

E_G42722_BAD_NAME
"Invalid attribute name in SQLDA for element %1d.\n\
The attribute name in the SQLDA has a negative length.
The EXEC 4GL %0c statement will fail for this element."

/*%
** Explanation:
**      A negative attribute name length was used in the SQLDA for
**      a dynamic EXEC 4GL statement.
**
** System Status:
**      The statement will fail to transfer data for this element.
**
** User Action:
**      Supply a valid attribute name and length.
*/

E_G42723_BAD_CLASS
"Invalid object class.\n\
The object supplied is of class '%1c'.  An object of class '%2c' or a subclass
is required.  The EXEC 4GL %0c statement will fail."

/*%
** Explanation:
**	A specific object class is required for this statement to succeed,
**	but a different class was given.
** System Status:
**      The statement will fail.
** User Action:
**      Supply a valid object.
*/

E_G42724_PREVIOUS_ERRS
"Errors prevent the execution of the %0c statement."

/*%
** Explanation:
**	The specified statement cannot be executed because of 
**	the following errors.
** System Status:
**      The statement will fail.
** User Action:
**      Correct the errors shown following this message.
*/

E_G42725_BAD_VALUE
"An unknown or invalid value was specified for the %0c '%1c'."

/*%
** Explanation:
**	A value which was of the wrong type or unknown was specified
**	for the variable or parameter shown.
** System Status:
**      The statement will fail.
** User Action:
**	Specify a correct value.
*/

E_G42726_NO_INIT
"An internal error has occured.\n\
The 4GL run-time object stack has not been initialized, which prevents passing
complex objects to 3GL procedures. Please notify your Site Technical Contact."
/*%
** Explanation:
**	An internal error occured.
**
** System Status:
**      All CALLPROC statements which attempt to pass complex objects to
**	3GL procedures will fail.
**
** User Action:
**	Notify the Site Technical Contact.
*/

E_G42727_MEM_ALLOC_FAIL
"An internal error has occured.\n\
An attempt to extend the 4GL run-time object stack has failed due to a memory
allocation error. The current operation will fail.  Please notify your 
Site Technical Contact."
/*%
** Explanation:
**	An internal error occured in a an attempt to allocate memory for
**	4GL complex objects that are accessible from 3GL procedures.
**
** System Status:
**      The current 4GL or EXEC 4GL statement will fail.
**	3GL procedures will fail.
**
** User Action:
**	Notify the Site Technical Contact.
*/

E_G42728_BAD_STACK_POP
"An internal error has occured.\n\
An attempt to free objects from the 4GL run-time object stack has failed
with internal error '%0c'. Please notify your Site Technical Contact."
/*%
** Explanation:
**	An internal error occured while trying to release the complex objects
**	that were made accessible to 3GL procedures after returning
**	from a CALLPROC to a 3GL procedure.  This should not occur.
**
** System Status:
**	The application may continue to execute normally with only a minor
**	memory leak occurring.  This problem should be reported, however, 
**	as a more serious internal problem may exist.
**
** User Action:
**	Notify the Site Technical Contact.
*/

E_G42729_GETDATA_ERROR
"Internal 4GL data retrieval error occurred.\n\
An internal error occurred while getting a value from
%1c '%2c'.  The value of the program variable will not be changed."

/*%
** Explanation:
**      An attempt was made to place a value into a program
**	variable, but an internal error occurred.
**
** System Status:
**      No change will be made to the contents of the
**      program variable.
**
** User Action:
**	Notify the Site Technical Contact.
*/

E_G4272A_BAD_INSERTROW_OPTS
"Unsupported options specified in an EXEC 4GL INSERTROW statement.\n\
The STATE and ROW options are not supported in %0c."

/*%
** Explanation:
**      One or more of the EXEC 4GL INSERTROW statement options specified
**	are not supported by this product.
**
** System Status:
**      The EXEC 4GL INSERTROW statement will fail.
**
** User Action:
**      Correct the program not to use these options.
*/

