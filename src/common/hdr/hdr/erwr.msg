/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    erar.msg -	ABF Run-time System Message Definitions File.
**
** Description:
**	Contains the message definitions for the ABF run-time system.
**
** History:
**	Revision 6.0  87/07/31  wong
**	Initial revision.
**	30-apr-91 (sandyd)
**	    Changed WR_CLASS from 41 to 206 to avoid collision with old ABF.
**	11/20/91 (dkh) - ER changes for internationalization.
**	12-jul-1993 (rogerl) - message for glob/consts init fails (abvarini.c)
**      20-Jun-2005 (hanal04) Bug 114700 INGSRV3337
**          Ingres message file cleanup.
**/

#define	_WR_CLASS	206


E_WR0000_NOSUCHDB	"Error connecting to database '%0c'."

/*%
** Explanation:
**	The application could not successfully connect to the named database
**	most likely because the database does not exist, or possibly because
**	the commands specified using the ING_SET environment logical are
**	incorrect.
**
**	Occasionally, successful database connection may be precluded by
**	other errors (e.g., GCA errors, etc.) usually evidence of some sort
**	of installation problem or hardware or system failure.
**
** System Status:
**	The application will not start up.
**
** User Action:
**	Check that the database name was specified correctly and re-enter
**	the command line.  Also, check that the commands specified using
**	the ING_SET logical are correct.
*/

E_WR0001_OUTOFMEM
"The ABF run-time system has run out of dynamic memory.
This condition usually represents an internal programming problem.
The routine that ran out of memory was '%0c'."

/*%
** Explanation:
**
** System Status:
**	The application exits after issuing the error message.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR0002_ALLOCFRM	"Cannot initialize ABF form.\n
The ABF run-time system could not initialize the form '%1c', which is
used internally.  This is an installation problem.  Please report it
to your technical support representative."

/*%
** Explanation:
**	The ABF runtime system could not start the Ingres Forms System or
**	initialize its required forms.  This could be for several reasons
**	including ...
**
** System Status:
**	The application terminates immediately.
**
** User Action:
**	Check that the environment logical TERM_INGRES has been set correctly
**	before reporting this problem to your technical support representative.
*/

E_WR0003_PRMTYPE	"Value passed to parameter of incompatible type.\n
A value of type '%4c' was passed to the parameter '%2c' in the %1c '%0c'.  The
type '%4c' is not compatible with the type of the parameter, '%3c'."
 
/*%
** Explanation:
**	A value was passed to a parameter in a frame or procedure that did
**	not agree in type with that declared for the parameter.  That is,
**	the type of the value is not compatible with (coercible to) that
**	of the parameter.
**
** System Status:
**	The value of the parameter will be undefined.
**
** User Action:
**	Ensure that the type of a value passed to a parameter agrees
**	with the type declared for the parameter.
*/

E_WR0004_POSTOOSL	"Positional parameters passed to 4GL procedure.\n
The 4GL procedure '%0c' was called with parameters specified by position
(i.e., of the form arg1, arg2, ...).  A 4GL procedure can only be called with
parameters passed by name (i.e., of the form param1 = arg1, param2 = arg2,
...)."

/*%
** Explanation:
**	4GL procedures do not define any positional parameters.  All
**	parameters passed to 4GL procedures must be passed by name.
**
** System Status:
**	The value passed to the procedure will be ignored.
**
** User Action:
**	Pass parameters to 4GL procedures by name only.
*/

E_WR0069_POSTODBPROC	"Positional parameters passed to database procedure.\n
The database procedure '%0c' was called with parameters specified by position
(i.e., of the form arg1, arg2, ...).  A database procedure can only be called
with parameters passed by name (i.e., of the form param1 = arg1, param2 =
arg2, ...)."

/*%
** Explanation:
**	Database procedures do not define any positional parameters.  All
**	parameters passed to database procedures must be passed by name.
**
** System Status:
**	The database procedure will not be called.
**
** User Action:
**	Pass parameters to database procedures by name only.
*/

S_WR006A_DBPROC_WITH_BYREF	
"A database procedure was called with BYREF parameters.\n\
Database procedure '%0c' was called with parameter '%1c' passed BYREF.
Passing BYREF parameters to database procedures is not supported. The BYREF
specification will be ignored."

/*%
** Explanation:
**	Database procedures cannot be called with BYREF parameters.
**
** System Status:
**	The BYREF will be ignored.
**
** User Action:
**	Pass parameters to database procedures by value only.
*/

E_WR006B_NO_SESSION
"Database procedure called without a valid database session.\n
A frame or procedure that does not have a valid database session as its
DBsession attrribute has called a database procedure.  The database procedure
cannot be executed."

/*%
** Explanation:
**      A frame or procedure which does not have a valid database session
**      has call a database procedure.  The frame's session may have been
**      closed previously or may be NULL.  The database procedure can not be
**      executed, since there is no database session in which to execute it.
**
** System Status:
**      The database procedure will not be executed.
**
** User Action:
**      Ensure that a valid database session is always current when a
**      database procedure is called.
*/

E_WR0005_KEYTOHOST	"Named parameters passed to host-language procedure.\n
The host-language procedure '%0c' was called with parameters specified by name
(i.e., of the form param1 = arg1, param2 = arg2, ...) as if it were an 4GL
procedure.  Host-language procedures can only be called with parameters
specified by position (i.e., of the form arg1, arg2, ...)."

/*%
** Explanation:
**	Host language procedures do not define parameters passed by name.  All
**	parameters passed to such procedures must be passed by position.
**
** System Status:
**	The value passed to the procedure will be ignored.
**
** User Action:
**	Pass parameters to host procedures by position only.
*/

E_WR0006_RETNOTYPE
"The %1c '%0c' was called expecting a return type of '%2c'. '%0c' does
not return a value since it was declared as returning 'none' in ABF."

/*%
** Explanation:
**	A frame or procedure was called and was expected to return a
**	value.  However, the frame/procedure had been declared in ABF
**	as not returning any value.
**
** System Status:
**	No value will be returned from the called frame/procedure.
**
** User Action:
**	Eliminate the reference to a return value in the calling frame,
**	or change the declaration of the called frame/procedure.
*/

E_WR0007_ABNULLPRM	"Possible Null value passed to non-Null parameter.\n
A value of Nullable type was passed to the parameter '%2c', which is not
Nullable, in the %1c '%0c'.  This will result in a run-time error when a Null
value is passed to this parameter.  Use the 'ifnull()' function when passing
this value to ensure that a run-time error will not occur."
 
/*%
** Explanation:
**	A value of a Nullable type is being passed to a parameter that has been
**	declared as being not Nullable.  Passing a Null value to this parameter
**	will result in a run-time error.  Since this value could be Nullable
**	you can use the 'ifnull()' function to pass it, which will guarantee
**	that a Null value is never passed to the non-Null parameter.
**
** System Status:
**	If the value is not Null, it will be passed to the parameter; otherwise
**	a run-time error will occur and the parameter will be undefined.
**
** User Action:
**	Change the type of the parameter to be Nullable, or pass the
**	value using the 'ifnull()' function, such as in 'ifnull(value)',
**	instead.
*/

E_WR0008_ABNULLBYREF	"Possible Null value returned by reference.\n
The parameter '%0c' is a Nullable type in the current %1c and is being returned
by reference to a non-Nullable field.  This will result in a run-time error when
a Null value is returned by reference to this field.  Define the parameter to
be non-Nullable to ensure that a run-time error will not occur."
 
/*%
** Explanation:
**	A parameter that is a Nullable type is being returned by reference to
**	a field that is not Nullable.  At some point, if the parameter contains
**	a Null value and is returned by reference, a run-time error will occur.
**
** System Status:
**	If not Null, the parameter will be returned by reference; otherwise
**	a run-time error will occur and the value of the field will be
**	undefined.
**
** User Action:
**	Change the type of the parameter to be not Nullable, or change
**	the type of the field passed by reference to be Nullable.
*/

E_WR0009_NOTBLFLD	"Table field report parameters are not yet supported.\n
Values entered in the table field '%0c' on the current form cannot be passed as
parameters to the INGRES/REPORT command."

/*%
** Explanation:
**	A table field exists on a form used by a Report Frame.  At present
**	no semantics have been defined for passing rows in the table field
**	as parameters to the Report command.
**
** System Status:
**	Table fields are ignored when passing parameters to the Report command.
**
** User Action:
**	Do not create table fields on forms to be used by a Report Frame.
*/

E_WR000A_DBGFILE	"Could not open FRS debug file.\n
The application could not open the FRS debugging file named as a command-line
argument.  Please check your current directory access rights and disk space
before reporting this as a problem to your technical support representative."

/*%
** Explanation:
**	DIAGNOSTIC.
**
** System Status:
**	The application will terminate immediately.
**
** User Action:
**	Make sure that the you have access rights to the current directory and
**	enough disk space to create and write files there before reporting this
**	as a problem to your technical support representative.
*/

E_WR000B_NOINIFRM	"Unable to initialize form for frame.\n
The form '%1c' for the frame '%0c' could not be initialized.  The frame will
not be executed."

/*%
** Explanation:
**	The Forms System returned an error when the ABF run-time system
**	tried to initialize a form for a frame before calling the frame.
**	Possibly, this is because the form does not exist, or its name was
**	typed incorrectly on the User Frame definition form.
**
** System Status:
**	The called frame will not be run.
**
** User Action:
**	Create the form if it does not exist, or correct the name of the form
**	defined for the frame before reporting this as a problem to your
**	technical representative.
*/

E_WR000C_ABNXTYPE	"Unknown frame type called.\n
The called frame was of an unknown type.  The error occurred in routine '%0c'.
Please report this problem to your technical support representative."

/*%
** Explanation:
**
** System Status:
**	The application exits after issuing the error message.
**
** User Action:
*/

E_WR000D_ABRETFRM	"Return value expected from non-4GL frame.\n
The frame '%0c' was called expecting it to return a value.  Since it is
a '%1c' frame, it cannot return a value."

/*%
** Explanation:
**	Frames such as QBF or Report frames that are not "User" frames (i.e.,
**	that are not defined by 4GL source code) do not return values to frames
**	that call them.
**
** System Status:
**	The field designated to hold the return value has an undefined value.
**
** User Action:
**	Delete the code assigning the return value from the frame.
*/

E_WR000E_ABNULLOBJ	"Undefined routine for %1c '%0c'.\n
The internal ABF run-time structures contain a NULL reference for the routine
for the %1c '%0'.  Please report this internal problem to your technical
support representative."

/*%
** Explanation:
**	The ABF run-time system found a NULL reference to a routine in its
**	internal structures when the application tried to call or run an 4GL
**	frame or procedure.  (This routine contains the executable code for
**	the frame or procedure.)  ABF should generate a symbol for this routine,
**	both in the internal run-time structures that it builds and in the code
**	generated for the frame or procedure, when an application is built.
**	Otherwise, this is an internal programming problem.
**
**	In versions before release 6, this error could occur when the source
**	files for a frame and a procedure shared a common file name but had
**	different extensions (e.g., "xyz.osl" and "xyz.c".)  Then, the resultant
**	object files would collide, most likely overwriting the code for the
**	frame's routine.  ABF guarantees that this cannot occur in release 6
**	applications by not allowing files to be specified for frames or
**	procedures that share a common file name.
**
** System Status:
**	The called frame will not be run.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR000F_RETTYPE	"Incompatible return type.\n
The %1c '%0c' returns a value of type '%2c' that is not compatible
with the expected type of '%3c'."

/*%
** Explanation:
**	A return value from a frame or procedure did not agree in type with the
**	expected type.  That is, the type of the return value is not compatible
**	with (coercible to) the expected type declared for the frame or
**	procedure.
**
** System Status:
**	The return value will be undefined.
**
** User Action:
**	Make sure that the type of the value returned agrees with the
**	expected type declared for the frame or procedure.
*/

E_WR0010_ABEXPREVAL	"Error in evaluating an expression.\n
An error occurred when the ABF run-time system tried to evaluate an expression.\ 
Please report this internal problem to your technical support representative."

/*%
** Explanation:
**	The ABF run-time system evaluates all expressions using the internal
**	Abstract Data Facility defined for Ingres.  This facility returned an
**	error for an expression is was to evaluate.  Since, 4GL generates the
**	internal representation for these expressions from the source code,
**	it is likely that it did so incorrectly.
**
** System Status:
**	The result of the expression is undefined.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR0011_ABNULLNAME	"Null was used where a string is required.\n
A Null value was used where an 4GL name or string value is required."

/*%
** Explanation:
**
** System Status:
**	The 4GL statement containing the NULL value will not be executed.
**
** User Action:
**	Use a string constant or non-NULL field for the expected 4GL name.
*/

E_WR0012_ABNOFIELD	"Bad field specified in query passed between frames.\n
The field '%0c' has been named in the query passed between frames.  No such
field exists in the form '%1c' in the frame being called."

/*%
** Explanation:
**
** System Status:
**	The named field will be ignored in the processing of the query.
**
** User Action:
**	Correct the query being passed into the called frame so
**	that it lists the proper fields.
*/

E_WR0013_ABNOTFCOL	"Unknown table field column in parameter query.\n
The column '%0c' in the table field '%1c' has been named in the query passed
between frames.  No such table field column exists in the form '%2c' in the
frame being called."

/*%
** Explanation:
**
** System Status:
**	The named table field column will be ignored in the
**	processing of the query.
**
** User Action:
**	Correct the query being passed into the called frame so
**	that it lists the proper table field columns.
*/

E_WR0014_NOQRYFRM	"Wrong form referenced in query passed between frames.\n
The frame '%0c' was passed a query that references the form '%1c'.  The frame's
form is named '%2c', and that form will be used instead of '%1c'."

/*%
** Explanation:
**
** System Status:
**	The form for the called frame will be used rather than the
**	form named in the passed-in query.
**
** User Action:
**	Correct the name of the called frame's form in the query being
**	passed between frames.
*/

E_WR0015_NOQRYTBL	"Wrong table field referenced in passed-in query.\n
The frame '%0c' was passed a query that references the table field '%2c'.  No
table field with that name exists in the form '%1c'.  The query will not be
executed."

/*%
** Explanation:
**
** System Status:
**	The query will not be executed.
**
** User Action:
**	Correct the name of the called frame's table field in the
**	query being passed between frames.
*/

E_WR0016_DOMOVER	"Too many domains in query.\n
Too many elements have been specified in the target list of the current
query.  Please reduce this number."

/*%
** Explanation:
**
** System Status:
**	The query will not be executed.
**
** User Action:
**	Reduce the number of elements in the target list of the query.
*/

E_WR0017_ABBADQUAL	"Bad internal type within qualification function.\n
A bad internal type occurred in the processing of a qualification
function.  The error occurred in routine '%0c'.
Please report this internal problem to your technical support representative."

/*%
** Explanation:
**
** System Status:
**	The query will not be executed.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR0018_PARAMSPACE	"Not enough space in query.\n
The query passed to this frame or procedure does not have enough
extra space for ABF to insert some needed information.  Please recompile
the 4GL code for the frame that called this frame or procedure."

/*%
** Explanation:
**
** System Status:
**	The query will not be executed.
**
** User Action:
**	Please recompile the 4GL code for the frame that passed
**	a query to the current frame.
*/

E_WR0019_NOPARAM	"Incorrect parameter name.\n
The parameter '%0c' was passed to the %2c '%1c'.  No parameter named '%0c' 
exists in '%1c'."

/*%
** Explanation:
**	A parameter with an incorrect name was passed to an 4GL
**	frame or procedure.
**
** System Status:
**	The parameter will be ignored.
**
** User Action:
**	Correct the name of the parameter in the calling frame or
**	procedure.
*/

E_WR001A_ABPROGERR	"Execution failure in program '%0c'.\n
The program '%0c' could not be executed, or failed to execute successfully.  The
execution module returned a value of '%1c'('0x%2c').  %3c"

/*%
** Explanation:
**	The program could not be executed, possibly because it could not
**	be found, or it was executed but returned a failure status because
**	of an internal programming error.
**
** System Status:
**	The program could not be called, or was called but returned
**	with a failure status.
**
** User Action:
**	Check your installation before reporting this problem to your
**	technical support representative.
*/

E_WR001B_ABBIGCOM	"Command too long.\n
The command to run the program '%0c' is too long to be executed on this
system.  Please refer to the INGRES Windows4GL Reference Manual and reduce the
number and/or size of the parameters passed to this sub-system."

/*%
** Explanation:
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Reduce the size of the command by reducing the number and/or
**	size of the parameters.
*/

E_WR001C_ABARGLIST		"Internal sub-system format error.\n
The parameter format list for the sub-system '%0c' was incorrectly built by the
4GL compiler.  The internal error number was '%1c'.  Please report this
problem to your Site Technical Contact."

/*%
** Explanation:
**      4GL compiles a parameter format list for each sub-system call that
**      specifies the names and the types of the parameters being passed to
**      the sub-system.  This format list is passed to the sub-system execution
**      module when the CALL statement is executed, and it found an error in
**      this list.  This is an internal programming problem.
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR001D_ABBADSPEC		"Sub-system parameter specification error.\n
The sub-system execution module contains an error in the specification line for
the sub-system '%0c'.  Please report this installation problem to your Site
Technical Contact."

/*%
** Explanation:
**      The sub-system execution module uses an internal file that specifies
**      the acceptable parameters and their command line formats for each
**      sub-system, which is part of the installation for INGRES.  In this file,
**      the format specification of the named parameter for this sub-system has
**      been incorrectly specified.  This is an internal specification or
**      installation problem.
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR001E_ABBADTYPE
"Internal sub-system format type error.\n
A parameter format type for the sub-system '%0c' was incorrectly specified by
the 4GL compiler.  The internal error number is '%1c'.  Please report this
problem to your Site Technical Contact."

/*%
** Explanation:
**      4GL compiles a parameter format list for each sub-system call that
**      specifies the names and the types of the parameters being passed to the
**      sub-system.  This format list is passed to the sub-system execution
**      module when the CALL statement is executed, and it found an illegal
**      value specified for the type of the parameter in this format list.  This
**      is an internal programming problem.
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR001F_ABNOSYM
"Undefined application symbol.\n
A CALL was made to the application by its symbol name, but no such symbol was
found in this system.  Please refer to the INGRES Windows4GL Reference Manual for
more information."

/*%
** Explanation:
**      A CALL statement in 4GL was made to an application sub-system whose
**      name was specified as the "name" parameter to the call.  No symbol
**      with that name specifying the application to be called can be found
**      defined in the system.
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Check for the correct name of the desired command.
*/

E_WR0020_ABNOPROG
"A CALL was made to the sub-system '%0c'.  No sub-system with that name is
recognized for this installation.  Please check the INGRES Windows4GL Reference
Manual for allowed sub-systems."

/*%
** Explanation:
**      A CALL statement in 4GL specified the name of a sub-system that is not
**      known to INGRES.  Most likely, this is because you incorrectly specified
**      the sub-system in your 4GL source-code, but could also be because of an
**      installation problem.
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Correct the name of the program to be called.
**	If necessary, refer to the ABF User's Guide for a list of
**	legal program names.
*/

E_WR0021_ABMODTYPE
"Internal sub-system module error.\n
The sub-system execution module contains an error in the module type specification for the sub-system '%0c'.  Please report this installation problem to your Site Technical Contact."

/*%
** Explanation:
**      The sub-system execution module uses an internal file that specifies
**      the execution parameters for each sub-system, which is part of the
**      installation for INGRES.  In this file, the module type for this
**      sub-system has been incorrectly specified.  This is an internal or
**      installation problem.
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Report this error to your technical representative.
*/

E_WR0022_ABNOARG
"Unknown sub-system parameter.\n
The sub-system '%0c' was called from 4GL with a parameter which is
not recognized for the sub-system in this installation.  Please refer to the
INGRES Windows4GL Reference Manual to determine the legal parameters defined for
this sub-system."

/*%
** Explanation:
**      A CALL sub-system statement in 4GL specified a parameter that has not
**      been defined for the sub-system.  Most likely, this is because you
**      incorrectly specified the parameters for this sub-system in your 4GL
**      source-code.  Rarely, this could also be because of an installation
**      problem.
**
** System Status:
**	The called program could not be run successfully.
**
** User Action:
**	Correct the arguments to the program being called.
**	If necessary, refer to the ABF User's Guide.
*/

E_WR0025_ReqQueryMode	"Warning:  Form not in query mode.\n
A qualification predicate has been specified for a query, but the form '%0c' is
not in query mode.  Since fields can only be qualified when the form is in
query mode, the query will apply to all rows."

#define ABQRYMODE	E_WR0025_ReqQueryMode

/*%
** Parameters:
**	%0c	the name of the currently running form.
**
** Explanation:
**	When a form is run in query mode, both values and operators can be
**	entered into fields.  These values and operators can then be used to
**	build a query using the 4GL qualification predicate.  No qualification
**	predicate can be constructed when the form is not in query mode.
**
** System Status:
**	The query will be executed with a predicate of "1 = 1" in place of
**	qualification predicate (meaning a predicate that is always true.)
**	This will result in all rows being retrieved by the query.
**
** User Action:
**	Make sure the mode of the form is set to query (using the MODE query
**	statement from 4GL) whenever a qualification predicate is used for
**	a query.
*/

E_WR004F_ABTRCFILE	"Could not open trace file.\n
ABF could not open the diagnostic trace file '%0c' in the temporary directory.
Please check your temporary directory access rights and disk space before
reporting this as a problem to your technical support representative."

/*%
** Explanation:
**	DIAGNOSTIC.
**
** System Status:
**	ABF or ABFIMAGE will terminate immediately.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory and
**	enough disk space to create and write files there before reporting this
**	as a problem to your technical support representative.
*/

E_WR0050	"The application '%0c' does not exist in the ABF catalogs."

/*%
** Explanation:
**
** System Status:
**	The application requested by the user could not be brought up.  ABF
**	will be brought up without an application or abfimage will be terminated
**
** User Action:
**	Check that name was spelled correctly.
*/

E_WR0051	"'%0c' is not a legal name for an application.  Application names can only contain letters and digits, and must be no more than 24 characters long."

/*%
** Explanation:
**
** System Status:
**	The application requested by the user could not be brought up.  ABF
**	will be brought up without an application or abfimage will be terminated
**
** User Action:
**	Check that name was spelled correctly.  Use a different name for a new
**	application that follows correct conventions.
*/

E_WR0052	"Cannot create temporary files.\n
Please check your temporary directory access rights and disk space before
reporting this as a problem to your technical support representative."

/*%
** Explanation:
**	The temporary directory cannot have files created within it.  This could
**	be for several reasons, but most likely is because you do not have the
**	required access rights to the temporary directory or because you do not
**	have enough disk space available to create files in that directory.
**
**	The temporary directory is either the current directory or the directory
**	specified by the environment logical II_TEMPORARY.
**
** System Status:
**	ABF or ABFIMAGE will terminate immediately.
**
** User Action:
**	Make sure that the you have access rights to the temporary directory and
**	enough disk space to create and write files there before reporting this
**	as a problem to your technical support representative.
*/

E_WR0053_KEYTOMETHOD	"Named parameters passed to system function.\n
The system function '%0c' was called with parameters specified by name
(i.e., of the form param1 = arg1, param2 = arg2, ...).  This system function
does not accept named parameters and must be called with parameters
specified by position (i.e., of the form arg1, arg2, ...)."

/*%
** Parameters:
**	0	This is the name of the system function that is being
**		called.
**
** Explanation:
**	Some system functions do not accept parameters specified by
**	name.  They only accept positional parameters.
**	Such is the case with this system function.
**
** System Status:
**	The method will not be called.
**
** User Action:
**	Correct the call of this system function so that parameters
**	are passed positionally.
*/


/*
**	Report Frame Messages ("abrtrw.qc")
*/
/*
** Names of ABF objects, used in error messages.
*/
F_WR0004_method		"method"
F_WR0005_frame		"frame"
F_WR0006_procedure	"procedure"

/*
**	String appended to a data format to show that it is Nullable.  Used
**	in error messages.  ("abrtutil.qc")
*/
S_WR004E__with_NULL	" with NULL"

/*
** Started adding varinit errors as E_WR006X
*/

E_WR0060_NOGLOBALSYMBOL		"Can't find global object.\n
While initializing a frame or a procedure, then global object
'%0c' could not be found.  The frame or procedure will not run, and
should be recompiled."

/*%
** Parameters:
**	0	The name of the global object that couldn't be found.
**
** Explanation:
**	A global object used by the frame or procedure could not
**	be found.  This means the frame or procedure can not run since
**	an object is needed can't be found.
**
** System Status:
**	The frame or procedure will not be run.
**
** User Action:
**	Try recompiling the frame or procedure.  If that does not work,
**	contact your Ingres Representative.
*/

E_WR0061_NODATA		"Can't find included application.\n
While initializing a frame or a procedure, the data for global object
'%1c' in the (included) application '%0c' could not be found.  The frame
or procedure will not run, and should be recompiled."

/*%
** Parameters:
**	0	The name of the application (possibly included) that
**		the object appears in.
**
**	1	The name of the global object that couldn't be found.
**
** Explanation:
**	The data for a global object used by the frame or procedure could not
**	be found.  This means the frame or procedure can not run since
**	an object is needed can't be found.
**
** System Status:
**	The frame or procedure will not be run.
**
** User Action:
**	Try recompiling the frame or procedure.  If that does not work,
**	contact your Ingres Representative.
*/

E_WR0062_BADTYPE	"Bad type information found.\n
While initializing a frame or a procedure, the type information for
the global type '%0c' was found to be bad.  The frame or procedure will
not run, and should be recompiled."

/*%
** Parameters:
**	0	The name of the type that is bad.
**
** Explanation:
**	The type information for a global type used by the frame or procedure
**	is bad.  This means the frame or procedure can not run.
**
** System Status:
**	The frame or procedure will not be run.
**
** User Action:
**	Try recompiling the frame or procedure.  If that does not work,
**	contact your Ingres Representative.
*/

E_WR0063_NOPACKAGE	"Can't find global data for an application.\n
While initializing a frame or a procedure, the global data for a
(possibly included) application '%0c' could not be found.  This occurred
while looking for the data for the global object '%1c'.  The frame or
procedure will not run, and should be recompiled."

/*%
** Parameters:
**	0	The name of the application (possibly included) that
**		couldn't be found.
**
**	1	The name of the global object that couldn't be found.
**
** Explanation:
**	The data for a (possibly included) application used by a
**	frame or procedure could not be found.  This means the frame
**      or procedure can not run since an object is needed can't be found.
**
** System Status:
**	The frame or procedure will not be run.
**
** User Action:
**	Try recompiling the frame or procedure.  If that does not work,
**	contact your Ingres Representative.
*/

E_WR0064_NULLPACKAGE	"The record for a global object has no application.\n
While initializing a frame or a procedure, the global object '%0c' used
by the frame or procedure does not contain an application name, and it
couldn't be found. The frame or procedure will not run, and should be
recompiled."

/*%
** Parameters:
**	1	The name of the global object that couldn't be found.
**
** Explanation:
**	Some information needed about a global object was not present.
**	This means the frame or procedure can not run since an object
**      is needed can't be found.
**
** System Status:
**	The frame or procedure will not be run.
**
** User Action:
**	Try recompiling the frame or procedure.  If that does not work,
**	contact your Ingres Representative.
*/

E_WR0066_User3GLException "An exception occurred in a 3GL procedure.\n
The exception described immediately below occurred in procedure '%0c'.
The procedure will cease to execute, and control
will be returned to the 4GL statement following the CALLPROC.\n\n\"%1c\""

/*%
** Parameters:
** 	1 The procedure name.
**	2 A description of the exception.
**
** Explanation:
**	An exception occurred in a 3GL procedure.
**
** System Status:
**	The remainder of the 3GL procedure will not execute.  Execution
**	will resume following the CALLPROC.  No BYREF parameters will be
**	returned, and the procedure, if it returns a value, will return
**	NULL.
**
** User Action:
**	Determine the cause of the exception, and correct the 3GL procedure.
**	
*/

S_WR0067_INGRESException	"INGRES Exception '%0d'"

/*
**	abrtnqry.c
*/

E_WR0068_INTERNAL_ERR_PASSING	"ERROR: The actual or formal type is NULL while passing to parameter %s.\n"

/*%
** Explanation:
**	Windows4gl found an a NULL handle when checking the type of an
**	object parameter.
**
** System Status:
**	Windows4gl will continue processing but the statement that caused
**	will not be executed.
**
** User Action:
**	User needs to make sure that a real object handle is being passed.
*/

E_WR006C_CANTPASSIN
"Error assigning a value to parameter '%0c'.\n"

/*%
** Explanation:
**	Windows4gl found an error when trying to pass a value to a 
**	frame or procedure parameter.
**
** System Status:
**	Windows4gl will continue processing but the assignment will not 
**	take place.
**
** User Action:
**	Correct the error, details of which will be given in a following 
**	message.
*/

E_WR006DCANTPASSBACK
"Error returning a value from BYREF parameter '%0c'.\n"

/*%
** Explanation:
**	Windows4gl found an error when trying to pass a value back from a 
**	BYREF parameter.
**
** System Status:
**	Windows4gl will continue processing but the assignment will not 
**	take place.
**
** User Action:
**	Correct the error, details of which will be given in a following 
**	message.
*/

E_WR006E_MaxSubSysPrms  "Maximum sub-system parameters exceeded.\n
The maximum allowed number of parameters for a sub-system has been exceeded.
Please refer to the INGRES Windows/4GL Reference Manual for the parameters 
allowed for this sub-system."

/*%
** Explanation:
**  A CALL sub-system statement in 4GL specified more parameters than are
**  allowed for any sub-system.
**
** System Status:
**  The sub-system will not be executed.
**
** User Action:
**  Please refer to the INGRES Windows/4GL Reference Manual to determine the
**  correct, legal parameters allowed for this sub-system call.  Then
**  correct the parameters passed by the CALL statement in your 4GL source
**  code file.
*/

E_WR0033_PMFail	"Internal failure of PM.\n
A failure was encountered while attempting to initialize the PM internal
data structures.  Initialization of Globals and Constants from the user
supplied resource file will not succeed."

/*%
** Explanation:
**	Bad return value from PMload indicates PM will not be useful.
**
** System Status:
**	Windows4gl will continue processing with standard defaults for the
**	globals and constants.
**
*/

E_WR0034_PMFileFail	"A failure occurred at access of file '%0c'.\n
A failure was encountered while attempting to access the Global/Constant
initialization data file.  Initialization of Globals and Constants from
the file will not succeed."

/*%
** Explanation:
**	User supplied glob/consts init PM file could not be accessed.
**
** System Status:
**	Windows4gl will continue, but the user-supplied resource file will
**	be ignored.
**
*/

E_WR0035_SetDefFail	"Failed to assign Variable or Constant '%0c'.\n
The default string for user variable '%0c' could not have the value
'%1c' assigned to it.  The initialization has failed."

/*%
** Explanation:
**	Attempts to set a new default string (def_str attribute) for the
**	global or constant has failed.  The old value will be used.
**
** System Status:
**	Windows4gl will continue processing but the assignment will not 
**	take place.
*/

E_WR0036_DataInitFail	"The data area of '%0c' was not initialized.\n
A failure occurred while attempting to initialize the data area for '%0c'
with value '%1c'.  Check to make sure the types are compatible."

/*%
** Explanation:
**	Initialization of the data area for this global or constant has failed.
**	This is potentially a serious problem, and may cause run-time errors.
**
** System Status:
**	Windows4gl stop processing.
**
*/
