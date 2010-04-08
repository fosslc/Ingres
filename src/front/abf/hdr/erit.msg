/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	erit.msg -	Interpreted Frame Object Message Definitions.
**
** Description:
**	Contains the definitions of the messages and errors for the interpreted
**	frame object modules, ILRF, IAOM, INTERP, and IOI.
**
** History:
**	Revision 6.4
**	03/15/91 (emerson)
**		Added new message S_IT0021_Enter_app_parms
**		(for new system procedure CommandLineParameters).
**		Topaz project.
**
**	Revision 6.2  89/07  wong
**	Added E_IT0020_MaxIL for JupBug #6367.
**	89/08  wong  Edited E_IT000B_badfid to be clearer.  JupBug #7226.
**
**	Revision 6.0  87/08/10  arthur
**	Initial revision.
**/

# define	_IT_CLASS	49

/*
** This is the name of the command line arguments for the interpreter.
** These are used in an error message, and should be the same as
** what is documented to the user.
**
** They are currently used in the E_ITXXXX_duparg error.
*/
F_IT0000_executable	"executable"
F_IT0001_frame		"frame"
F_IT0002_database	"database"
F_IT0005_rtt		"runtime table"

/*
** These are the kinds of callable objects that exist for an application,
** either a frame or a procedure.  These are used in certain errors to
** tell the user what type of object has a problem.
**
** For example, an error might be something like:
**	Problem calling the frame X.
**
** The word 'frame' above would be whatever F_IT0003_frame is.
**
** Note these are separate from F_IT0001_frame, since that is the name
** of a command line argument, and not an object type.  I suppose in
** some cases they may be different.
*/
F_IT0003_frame		"frame"
F_IT0004_procedure	"procedure"


E_IT0000_comline
"The command line for the 4GL interpreter contained too many
or too few arguments.
Usage:  run4gl database-name image-file-name frame-name"

/*%
** Explanation:
**	This error appears when the 4GL interpreter has received
**	the wrong number of arguments on the command line.
**	If 'run4gl' has been called from the operating system
**	level, re-enter the command with the proper arguments.
**	If 'run4gl' has been called within ABF, then the command line
**	has been built by ABF, and this error message indicates an internal
**	error which should be reported to your technical support
**	representative.
**
** System Status:
**	The 4GL interpreter will not start up.
**
** User Action:
**	If 'run4gl' was entered from the command line, refer to
**	the ABF User's Guide for further information.
**	If 'run4gl' was called within ABF, report the problem to
**	an INGRES technical support representative.
*/

E_IT0001_flagbad
"An illegal flag `%0c' was specified for the 4GL interpreter."

/*%
** Explanation:
**	This error appears when the 4GL interpreter has received
**	an illegal flag on the command line.
**	If 'run4gl' has been called from the operating system
**	level, re-enter the command with the proper arguments.
**	If 'run4gl' has been called within ABF, then the command line
**	has been built by ABF, and this error message indicates an internal
**	error which should be reported to your technical support
**	representative.
**
** System Status:
**	The 4GL interpreter will not start up.
**
** User Action:
**	If 'run4gl' was entered from the command line, refer to
**	the ABF User's Guide for further information.
**	If 'run4gl' was called within ABF, report the problem to
**	an INGRES technical support representative.
*/

E_IT0002_fail
"4GL interpreter internal error.\n
An error occurred in an internal routine of the 4GL interpreter.
Please report this internal problem to your Site Technical Contact."

/*%
** Explanation:
**	An error occurred in an internal routine of the 4GL interpreter.  For
**	example, an error may have occurred in the process of reading to of
**	writing from the ABF system catalogs.  This is an internal programming
**	problem.
**
** System Status:
**
** User Action:
**	Please report this internal problem to your Site Technical Contact.
*/

E_IT0003_argbad	"Internal interpreter argument error.\n
A bad argument was passed to the internal routine `%0c' in the 4GL interpreter.
Please report this problem to your Site Technical Contact."

/*%
** Explanation:
**	An argument was found to be NULL or out-of-range in the specified
**	routine within the 4GL  interpreter.  This is an internal programming
**	error.
**
** System Status:
**
** User Action:
**	Please report this internal problem to your Site Technical Contact.
*/

E_IT0004_filopen
"The file `%0c' could not be opened."

/*%
** Explanation:
**	This error occurs when a required file could not be opened.
**	Check for the existence of and protections on the file.
**	If the the file is an internal one created and managed
**	by ABF, report the problem to your technical support
**	representative.
**
** System Status:
**
** User Action:
**	Check the file's protection status.
**	If an internal ABF file, report to INGRES technical
**	support representative.
*/

E_IT0005_ilorder
"An intermediate language statement was found out of order."

/*%
** Explanation:
**	An unexpected statement was reached in the intermediate
**	language read by the 4GL interpreter.  For example, a
**	statement which should always be preceded by another specific
**	statement was found stand-alone.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**	The current frame/procedure is exited.  Execution
**	continues with the frame/procedure which called this one.
**
** User Action:
**	Please report this internal problem to your Site Technical Contact.
*/

E_IT0006_ilmissing
"An intermediate language statement with operator `%0c' was
expected, but not found."

/*%
** Explanation:
**	An expected statement in the intermediate language read by the
**	4GL interpreter was not found.  For example, a statement
**	which should always follow another specific statement was missing.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**	The current 4GL statement is skipped.  Execution
**	continues with the next 4GL statement in the
**	frame/procedure.
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_IT0007_eofil
"An unexpected end-of-file was reached in the intermediate language."

/*%
** Explanation:
**	An end-of-file marker was found within the intermediate language
**	read by the 4GL interpreter.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**	The current frame/procedure is exited.  Execution
**	continues with the frame/procedure which called this one.
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_IT0008_trfile
"The trace file `%0c' could not be opened."

/*%
** Explanation:
**	An internal file used by ABF for tracing could not be opened.
**
** System Status:
**	Execution continues.
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_IT0009_formname
"No form was specified for the frame `%0c'."

/*%
** Explanation:
**
** System Status:
**	The frame which has no form to display is exited.  Execution
**	continues with the frame/procedure which called it.
**
** User Action:
*/

E_IT000A_client
"An internal error occurred in interfacing with the Intermediate
Language Retrieval Facility."

/*%
** Explanation:
**	An internal error occurred, in which the structure used by
**	the 4GL interpreter to interface with the Intermediate Language
**	Retrieval Facility was not properly initialized.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**	This is a fatal error.  The 4GL interpreter exits.
**
** User Action:
**	Report to technical support representative.
*/

E_IT000B_fidbad	"Missing interpreted frame object.\n
The interpreted frame object for an ABF/4GL frame or procedure cannot be found
in the system catalogs.  Please try recompiling the frame or procedure to
correct this problem.  Then, if the problem persists with this same frame or
procedure, report it as a problem to your Site Technical Contact."

/*%
** Explanation:
**	For some reason, the interpreted frame object associated with an ABF/4GL
**	frame or procedure could not be found in the system catalogs.  The frame
**	or procedure is associated with an interpreted frame object through an
**	identifier stored in the ABF system catalogs, but no object with that
**	numeric identifier exists in the object manager system catalogs.  This
**	is an unusual condition and may indicate an inconsistency in the INGRES
**	system catalogs.
**
** System Status:
**	The frame or procedure cannot be run.  Execution continues from the
**	frame or procedure that called the missing frame or procedure, or the
**	program exits if the frame or procedure was the top (or entry) object.
**
** User Action:
**	Force recompilation of the frame or procedure in ABF or through
**	ABFIMAGE.  Report this problem to your Site Technical Contact only
**	if the problem persists with the same frame or procedure.
*/

E_IT000C_cframe
"A new frame could not be allocated, and the current frame
could not be restored."

/*%
** Explanation:
**	Because of memory limitations, a called frame/procedure
**	could not be run, and the calling frame/procedure, which
**	had been swapped out of memory, could not be restored.
**
** System Status:
**	This is a fatal error.  The 4GL interpreter exits.
**
** User Action:
**	Report to technical support representative.
*/

E_IT000D_frdwr
"An internal error occurred in reading from or writing to the database."

/*%
** Explanation:
**	An error occurred in reading from/writing to the INGRES system
**	catalogs while accessing/saving the definition of a frame.
**
** System Status:
**	The current frame/procedure is exited.  Execution
**	continues with the frame/procedure which called this one.
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_IT000E_radbad
"An error occurred in accessing the return address block for a frame."

/*%
** Explanation:
**	An internal error occurred in retrieving the return address block
**	for a frame which had called another frame which just returned.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**	The current frame/procedure (which had just been returned to)
**	 is exited.  Execution continues with the frame/procedure which
**	called it.
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_IT000F_frmdisp
"An error occurred in displaying the form '%0c'."

/*%
** Explanation:
**	An error occurred in displaying the specified form for the
**	current frame.
**
** System Status:
**	The current frame/procedure is exited.  Execution
**	continues with the frame/procedure which called it.
**
** User Action:
*/

E_IT0010_jmpbad
"An attempt was made to go to an intermediate language statement
with an illegal address."

/*%
** Explanation:
**	An out-of-bounds address was specified for the next
**	intermediate language statement to be read by the 4GL interpreter.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**	The current frame/procedure is exited.  Execution
**	continues with the frame/procedure which called it.
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_IT0011_expbad "4GL expression evaluation error.\n
An error occurred in the evaluation of a 4GL expression.  The results will be
undefined."

/*%
** Explanation:
**	The Abstract Data Type facility returned an error status when
**	attempting to evaluate an 4GL expression.
**
** System Status:
**	The result of the expression is undefined.
**	The current 4GL statement is skipped.  Execution
**	continues with the next 4GL statement in the
**	frame/procedure.
**
** User Action:
*/

E_IT0012_divzero
"An attempt to divide by 0 occurred within an expression."

/*%
** Explanation:
**
** System Status:
**	The result of the expression is undefined.
**	The current 4GL statement is skipped.  Execution
**	continues with the next 4GL statement in the
**	frame/procedure.
**
** User Action:
*/

E_IT0013_numovf
"A numeric overflow or underflow occurred within an expression."

/*%
** Explanation:
**
** System Status:
**	The result of the expression is undefined.
**	The current 4GL statement is skipped.  Execution
**	continues with the next 4GL statement in the
**	frame/procedure.
**
** User Action:
*/

E_IT0014_fltbad
"An error occurred in evaluating floating point numbers
within an expression.  For example, there may have been numeric
overflow or underflow, or an attempt to divide by 0."

/*%
** Explanation:
**
** System Status:
**	The result of the expression is undefined.
**	The current 4GL statement is skipped.  Execution
**	continues with the next 4GL statement in the
**	frame/procedure.
**
** User Action:
*/

E_IT0015_imgbad
"An error occurred in reading the run-time table or image file."

/*%
** Explanation:
**	An error occurred in reading the run-time table or image file.
**	Since these files are built by ABF, such an error should never
**	occur, and should be reported as a problem to your technical support
**	representative.
**
** System Status:
**	The current frame/procedure is exited.  Execution
**	continues with the frame/procedure which called it.
**
** User Action:
**	Report to INGRES technical support representative.
*/

E_IT0016_calbad
"An error occurred in calling the %0c '%1c'.\n
The value returned from the internal routine that tried to
get the IL for this object was `%2x'."

/*%
** Explanation:
**	An error occurred in starting up the frame or procedure
**	called within the application.
**
** System Status:
**	The called frame/procedure cannot be run.  Execution
**	continues with the frame/procedure which called it.
**
** User Action:
*/

/*
** In this error, the argument name is the name of the command line
** argument as described in documentation.  Currently it is one of:
**	
**	F_IT0000_executable
**	F_IT0001_frame
**	F_IT0002_database
*/
E_IT0017_duparg
"The command line contained a duplicate argument.
The `%0c' name was specified both with and without a flag."

/*%
** Explanation:
**	This error appears when one of the arguments to the 4GL
**	interpreter appears in both possible formats:  with and
**	without a flag.
**	If 'run4gl' has been called from the operating system
**	level, re-enter the command with the proper arguments.
**	If 'run4gl' has been called within ABF, then the command line
**	has been built by ABF, and this error message indicates an internal
**	error which should be reported to your technical support
**	representative.
**
** System Status:
**	The 4GL interpreter will not start up.
**
** User Action:
**	If 'run4gl' was entered from the command line, refer to
**	the ABF User's Guide for further information.
**	If 'run4gl' was called within ABF, report the problem to
**	an INGRES technical support representative.
*/

E_IT0018_nomem
"The 4GL interpreter ran out of memory or could not free
dynamic memory."

/*%
** Explanation:
**
** System Status:
**	If the error occurred while the interpreter was attempting
**	to execute a single 4GL statement within a frame, then that
**	statement is skipped.  Execution continues with the next 4GL
**	statement in the frame/procedure.
**	If the error occurred while the interpreter was reading in
**	a new frame/procedure, that frame cannot be run.  Execution
**	continues with the statement following the call in the
**	calling frame/procedure.
**
** User Action:
*/

E_IT0019_sycall
"The command '%0c' could not be executed."

/*%
** Explanation:
**	An error occurred in attempting to execute an operating system
**	command via the 4GL 'call system' statement.
**
** System Status:
**	The current 4GL statement is skipped.  Execution
**	continues with the next 4GL statement in the
**	frame/procedure.
**
** User Action:
*/

E_IT001A_noobj
"An internal error occurred in retrieving an ABF object from
the system catalogs."

/*%
** Explanation:
**	An ABF object such as a frame could not be found in the
**	system catalogs by the Application Object Manager.
**	Since such an error should never occur, it should be
**	reported as a problem to your technical support
**	representative.
**
** System Status:
**	The current frame/procedure is exited.  Execution
**	continues with the frame/procedure which called it.
**
** User Action:
**	Report to INGRES technical support representative.
*/

/*
**	The following prompt message appears after a report has
**	been run from inside an application.
*/
S_IT001B_Hit_Return_when_done	"Press <RETURN> when done"
/*
**	The following message appears after a query has been
**	run which did not retrieve any rows.
**	NOTE: This message also appears in users' code produced
**	by the code generator.  Any translation of this message
**	must, therefore, be exactly the same as the translation
**	of message S_CG_0008___No_rows_retrieved__ (except for
**	the double quotes in that string constant).
*/
S_IT001C_No_rows_retrieved	"No rows retrieved"
/*
**	The following message appears if an 4GL 'next' statement is
**	executed in a frame which is not in the process of
**	handling a query.
**	NOTE: This message also appears in users' code produced
**	by the code generator.  Any translation of this message
**	must, therefore, be exactly the same as the translation
**	of message S_CG_0009__No_query_currently_ (except for
**	the double quotes in that string constant).
*/
S_IT001D_No_query_currently_ac	"No query currently active"
/*
**	The following message appears when an 4GL 'next' statement
**	is executed after all rows retrieved by a query (with submenu)
**	have already been displayed.
**	NOTE: This message also appears in users' code produced
**	by the code generator.  Any translation of this message
**	must, therefore, be exactly the same as the translation
**	of message S_CG_000A___No_more_rows__ (except for
**	the double quotes in that string constant).
*/
S_IT001E_No_more_rows	"No more rows"

E_IT001F_NULLINT
"An integer expression containing a NULL value was used in a `%0c' statement.
A NULL value is not legal in this context.  The expression should be changed
to be non-NULLable, or the ifnull function should be used.  The statement
will continue with a default value."

/*%
** Explanation:
**	Some statements that take integer expressions do not allow
**	NULL values.  For example, a table field reference can not
**	have a NULL value for the row number.  However, this error
**	is not detectable until runtime, since the expression may
**	evaluate to a non-NULL value.  The 4GL code needs to be
**	changed so that the expression produces a non-NULL value.
**
** System Status:
**	The statement continues with a default value.
**
** User Action:
**	Change the 4GL code so that a non-NULL value is produced.
*/
/*
** The argument is the keyword for the statement in which the error
** occurred.  This argument is a hard-code English string in the interpreter.
** This was done, since 4GL keywords are always English.
*/

E_IT0020_MaxIL	"4GL source-code exceeded internal limit.\n
The size of the internal code generated for a 4GL source-code file exceeded
the maximum allowed by %0d percent.  Decrease the size of the 4GL source-code
by extracting some functionality into separate 4GL procedures."

#define	ILE_MaxIL	E_IT0020_MaxIL

/*%
** Explanation:
**	4GL source-code is translated by the 4GL compilers into an internal
**	intermediate code.  The maximum size of this internal intermediate
**	code per frame and procedure is limited, and the amount of source-code
**	written for the frame or procedure caused the 4GL compilers to exceed
**	this limit.
**
** System Status:
**	The 4GL compilation will not complete and no internal intermediate
**	code will be generated for the 4GL frame or procedure.
**
** User Action:
**	Extract some of the 4GL source-code into separate 4GL procedures (which
**	can be called in place of the original source-code.)  This will make the
**	source-code smaller and enable it to be compiled.
*/

/*
**	The following prompt message appears on the first call
**	to the system procedure CommandLineParameters
**	when running an interpreted application.
*/
S_IT0021_Enter_app_parms
"Enter application-specific command line parameters:"
